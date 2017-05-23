#include "../interface/Camera.h"
#include "TVector2.h"
#include "TMath.h"
#include <algorithm>
#include <vector>
#include <unordered_set>

using namespace panda;
using namespace redpanda;
using namespace std;

Camera::Camera(int debug_/*=0*/) {
  DEBUG = debug_;

  it = new ImageTree(40);

  if (DEBUG) PDebug("Camera::Camera","Calling constructor");
}


Camera::~Camera() {
  if (DEBUG) PDebug("Camera::~Camera","Calling destructor");
}


void Camera::Reset() {
  it->Reset();
}

void Camera::SetOutputFile(TString fOutName) {
  fOut = new TFile(fOutName,"RECREATE");
  tOut = new TTree("album","album");

  it->WriteTree(tOut);

  if (DEBUG) PDebug("Camera::SetOutputFile","Created output in "+fOutName);
}


int Camera::Init(TTree *t)
{
  if (DEBUG) PDebug("Camera::Init","Starting initialization");
  if (!t) {
    PError("Camera::Init","Malformed input!");
    return 1;
  }
  tIn = t;

  event.setStatus(*t, {"!*"}); // turn everything off first
  panda::utils::BranchList readlist({"runNumber", "lumiNumber", "eventNumber",
                                     "weight", "genParticles", "truthJets"});
  readlist.setVerbosity(DEBUG);
  event.setAddress(*t, readlist); 

  if (DEBUG) PDebug("Camera::Init","Set addresses");

  if (DEBUG) PDebug("Camera::Init","Finished configuration");

  return 0;
}


panda::GenParticle const *
Camera::MatchToGen(double eta, double phi, double radius, int pdgid) 
{
  panda::GenParticle const* found=NULL;
  double r2 = radius*radius;
  pdgid = abs(pdgid);

  unsigned int counter=0;
  for (map<panda::GenParticle const*,float>::iterator iG=genObjects.begin();
      iG!=genObjects.end(); ++iG) {
    if (found!=NULL)
      break;
    if (pdgid!=0 && abs(iG->first->pdgid)!=pdgid)
      continue;
    if (DeltaR2(eta,phi,iG->first->eta(),iG->first->phi())<r2)
      found = iG->first;
  }

  return found;
}


void Camera::Terminate() {
  fOut->WriteTObject(tOut);
  fOut->Close();

  delete it;

  if (DEBUG) PDebug("Camera::Terminate","Finished with output");
}


// run
void Camera::Run() {

  // INITIALIZE --------------------------------------------------------------------------

  unsigned int nEvents = tIn->GetEntries();
  unsigned int nZero = 0;
  if (lastEvent>=0 && lastEvent<(int)nEvents)
    nEvents = lastEvent;
  if (firstEvent>=0)
    nZero = firstEvent;

  if (!fOut || !tIn) {
    PError("Camera::Run","NOT SETUP CORRECTLY");
    exit(1);
  }

  // set up reporters
  unsigned int iE=0;
  ProgressReporter pr("Camera::Run",&iE,&nEvents,10);
  TimeReporter tr("Camera::Run",DEBUG);

  TH1D hEta("hEta","",100,0,3);
  TH1D hPhi("hPhi","",100,0,3.2);
  double quantiles[1] = {0.9};
  double qEta[1]; 
  double qPhi[1];
  TH2F *hMaxPt = (TH2F*) it->genImage->Clone();

  // what we consider valid hard partons:
  // include all hadronizable quarks, plus gluons
  // include taus (hadronic) 
  // const std::unordered_set<unsigned> partonIDs = {1, 2, 3, 4, 5, 21, 11, 13, 15, 22};
  const std::unordered_set<unsigned> partonIDs = {1, 2, 3, 4, 5, 21, 15};
  auto validPID = [partonIDs](int id) -> bool {
    return (partonIDs.find(abs(id)) != partonIDs.end());
  };
  // resonances we want to ignore (e.g. u u~ > Z > d d~ should have the us ignored)
  const std::unordered_set<unsigned> resonanceIDs = {6, 23, 24}; 
  auto isResonance = [resonanceIDs](int id) -> bool {
    return ( (abs(id) > 10000) || // e.g. Z'
             (resonanceIDs.find(abs(id)) != resonanceIDs.end()) );
  };


  // EVENTLOOP --------------------------------------------------------------------------
  for (iE=nZero; iE!=nEvents; ++iE) {
    tr.Start();
    pr.Report();
    event.getEntry(*tIn,iE);

    tr.TriggerEvent("read branches");

    if (event.truthJets.size()==0)
      continue;

    if (DEBUG>2) {
      unsigned nG = event.genParticles.size();
      for (unsigned iG = 0; iG != nG; ++iG) {
        auto& gen = event.genParticles.at(iG);
        PDebug("Camera::Run Dumping gen",
            TString::Format("gen idx=%3u, pT=%7.3f, E=%7.3f, parent=%3i, pdgid=%i, isHardProcess=%u",
              iG,gen.pt(),gen.e(),gen.parent.idx(),gen.pdgid,(gen.statusFlags&(1<<panda::GenParticle::kIsHardProcess))));
      }
    }
    
    // let's loop through and find particles that make sense to consider as partons
    const panda::GenParticle *proton1=0, *proton2=0;
    std::unordered_set<const panda::GenParticle*> partonCandidates;
    for (auto& gen : event.genParticles) {
      if (gen.pdgid == 2212 && !gen.parent.isValid()) { // initial state
        if (proton1)
          proton2 = &gen;
        else
          proton1 = &gen;
        continue;
      }

      if (!validPID(gen.pdgid)) 
        continue; // throw out hadrons, etc
      
      double genpt = gen.pt();
      double threshold = zcut * genpt;

      if (abs(gen.pdgid) == 22 && genpt<10) 
        continue; // throw away extremely soft photons
         
      // now look for 1-N splittings.
      // this is expensive... could imagine truncating at N=2
      std::vector<const panda::GenParticle*> children;
      for (auto& child : event.genParticles) {
        if (!child.parent.isValid() || child.parent.get() != &gen) 
          continue; // only consider particles that come from gen
        if (validPID(child.pdgid) || isResonance(child.pdgid))
          children.push_back(&child);
      }

      // now check if at least 2 of the children pass the z cut.
      // this unambiguously deals with N=2, but we may want to 
      // revisit this for N>2
      unsigned nPass = 0;
      for (auto* pchild : children) {
        if (pchild->pt() > threshold)
          nPass += 1;
      }

      // drop 1->N if 2 of the N pass the zcut
      // also drop 1->1 always <- don't do this
      // if ((children.size() == 1 && nPass == 1) || nPass >= 2) {
      if (nPass >= 2) {
        if (DEBUG>3) PDebug("Camera::Run Rejecting partons",
            TString::Format("pT=%7.3f, pdgid=%i; %lu %u",gen.pt(),gen.pdgid,children.size(),nPass));
        continue;
      }

      // remove if a parent of this parton is already a good parton
      // this avoids saving t0 and t3 in t0->t1->t2->t3
      bool foundParent = false;
      bool foundResonance = false; // had to go through a resonance to hit a valid parent
      const panda::GenParticle *iter = &gen;
      while (iter->parent.isValid()) {
        iter = iter->parent.get();
        if (!foundResonance)
          foundResonance = isResonance(iter->pdgid);
        if (partonCandidates.find(iter) != partonCandidates.end()) {
          foundParent = true;
          break;
        }
      }
      if (foundParent && !foundResonance) {
        if (DEBUG>3) PDebug("Camera::Run Rejecting partons",
            TString::Format("pT=%7.3f, pdgid=%i; found parent pT=%7.3f, pdgid=%i",gen.pt(),gen.pdgid,iter->pt(),iter->pdgid));
        continue;
      }
      // here's a question:
      // do we need to reject the precursor to a resonance?
      // it's wrong to call it a hard parton, but would we ever
      // accidentally associate it with a hadron? I think no.
      // current solution is to keep it in the collection

      partonCandidates.insert(&gen);
      if (DEBUG>2) PDebug("Camera::Run Finding partons",
          TString::Format("pT=%7.3f, E=%7.3f, pdgid=%i; %lu %u",gen.pt(),gen.e(),gen.pdgid,children.size(),nPass));
    } // gen particle loop     

    for (auto& j : event.truthJets) {
      it->Reset();
      hEta.Reset(); hPhi.Reset();
      hMaxPt->Reset();

      it->eventNumber = event.eventNumber;
      it->runNumber = event.runNumber;
      it->lumiNumber = event.lumiNumber;
      it->mcWeight = event.weight;

      double eta = j.eta();
      double phi = j.phi();
      double jpt = j.pt();
      double je = j.e();

      it->mass = j.m();
      it->pt = jpt;

      // get the absolute dimensions of the jet
      for (auto cref : j.constituents) {
        auto *c = cref.get();
        double ceta = c->eta();
        double cphi = c->phi();

        hEta.Fill(fabs(ceta - eta), c->pt());
        hPhi.Fill(fabs(SignedDeltaPhi(cphi,phi)), c->pt());
      }

      hEta.GetQuantiles(1,qEta,quantiles);
      hPhi.GetQuantiles(1,qPhi,quantiles);

      if (DEBUG) PDebug("Camera::Run",
          TString::Format("jet dimensions are deta=%.3f, dphi=%.3f",qEta[0], qPhi[0]));


      double maxpt = 0;
      std::map<const panda::GenParticle*, char> partons; // what we consider "hard"
      std::map<const panda::GenParticle*, float> partonsSumPt;
      partons[0] = 1; // unassociated or underlying event
      for (auto cref : j.constituents) {
        auto *c = cref.get();
        double ceta = c->eta();
        double cphi = c->phi();

        ceta = (ceta - eta) / qEta[0];
        cphi = SignedDeltaPhi(cphi,phi) / qPhi[0];

        it->genImage->Fill(ceta,cphi,c->pt());
        maxpt = max(maxpt, double(c->pt()));

        // now check for original hard parton
        panda::Ref<panda::GenParticle> parent = cref;
        while (cref->parent.isValid()) {
          parent = cref->parent;
          if (partonCandidates.find(parent.get()) != partonCandidates.end())
            break;
          // if (parent->e() > zcut * je) {
          //   // this is a valid hard parton
          //   break; 
          // }
        }

        
        char idx = 1; // default "parton"
        const panda::GenParticle *parton = parent.get();
        if (DEBUG>2) PDebug("Camera::Run Parton history",
            TString::Format("for hadron E=%7.3f, id=%5i, found a parton E=%7.3f, id=%5i",
              c->e(), c->pdgid, parton->e(), parton->pdgid));
        if (parton != c &&
            parton != proton1 && 
            parton != proton2) 
        { 
          // we found an actual parent
          auto found = partons.find(parton);
          if (found == partons.end()) {
            idx = partons.size()+1;
            partons[parton] = idx;
            partonsSumPt[parton] = 0;
          } else {
            idx = found->second;
          }
        }
        
        double cpt = c->pt();
        partonsSumPt[parton] += cpt;
        if (cpt > hMaxPt->GetBinContent(hMaxPt->FindBin(ceta,cphi))) {
          it->truthImage->SetBinContent(it->truthImage->FindBin(ceta,cphi),idx);
//          it->truthImage->ComputeIntegral();
          hMaxPt->SetBinContent(hMaxPt->FindBin(ceta,cphi),cpt);
        }
        
      }

      it->genImage->Scale(1./it->genImage->Integral());

      if (DEBUG) PDebug("Camera::Run",
          TString::Format("jet pT=%.3f, sum pT=%.3f, max pT=%.3f",j.pt(),it->genImage->Integral(),maxpt));
      if (DEBUG) PDebug("Camera::Run",
          TString::Format("jet has %lu partons with z>%.3f",partons.size(),zcut));
      if (DEBUG) {
        for (auto& iter : partons) {
          if (!iter.first)
            continue;
          PDebug("Camera::Run",
              TString::Format(" parton pdgid=%i, pT=%.3f, sum pT=%.3f",
                              iter.first->pdgid,
                              iter.first->pt(),
                              partonsSumPt[iter.first]));
        }
      }

      tr.TriggerSubEvent("truth jet");

      it->Fill();
      tr.TriggerSubEvent("fill");
    } // truth jet loop

    tr.TriggerEvent("all truth jets");

  } // entry loop

  if (DEBUG) { PDebug("Camera::Run","Done with entry loop"); }

} // Run()

