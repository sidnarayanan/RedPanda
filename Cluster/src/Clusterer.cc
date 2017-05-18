#include "../interface/Clusterer.h"
#include "TVector2.h"
#include "TMath.h"
#include <algorithm>
#include <vector>

using namespace panda;
using namespace std;

redpanda::Clusterer::Clusterer(int debug_/*=0*/) {
  DEBUG = debug_;

  int activeAreaRepeats = 1;
  double ghostArea = 0.01;
  double ghostEtaMax = 7.0;
  double radius = 1.5;
  double sdZcut = 0.15;
  double sdBeta = 1.;
  activeArea = new fastjet::GhostedAreaSpec(ghostEtaMax,activeAreaRepeats,ghostArea);
  areaDef = new fastjet::AreaDefinition(fastjet::active_area_explicit_ghosts,*activeArea);
  jetDef = new fastjet::JetDefinition(fastjet::cambridge_algorithm,radius);
  softDrop = new fastjet::contrib::SoftDrop(sdBeta,sdZcut,radius);

  if (DEBUG) PDebug("Clusterer::Clusterer","Calling constructor");
}


redpanda::Clusterer::~Clusterer() {
  if (DEBUG) PDebug("Clusterer::~Clusterer","Calling destructor");
}



void redpanda::Clusterer::SetOutputFile(TString fOutName) {
  fOut = new TFile(fOutName,"RECREATE");
  tOut = new TTree("events","events");

  outEvent.book(*tOut, {"*"}); 

  if (DEBUG) PDebug("Clusterer::SetOutputFile","Created output in "+fOutName);
}


void redpanda::Clusterer::Init(TTree *t, TH1D *hweights)
{
  if (DEBUG) PDebug("Clusterer::Init","Starting initialization");
  if (!t || !hweights) {
    PError("Clusterer::Init","Malformed input!");
    return;
  }
  tIn = t;

  inEvent.setStatus(*t, {"!*"}); // turn everything off first

  TString jetname = "puppi";
  panda::utils::BranchList readlist({"runNumber", "lumiNumber", "eventNumber", "rho", 
                                     "isData", "npv", "npvTrue", "weight", "metFilters",});
  readlist.setVerbosity(DEBUG);

  readlist += {jetname+"CA15Jets", "subjets", jetname+"CA15Subjets","Subjets"};

  readlist.push_back("ca15GenJets");
  
  readlist.push_back("pfCandidates");

  readlist.push_back("genParticles");
  readlist.push_back("genReweight");
  
  inEvent.setAddress(*t, readlist); 

  if (DEBUG) PDebug("Clusterer::Init","Set addresses");

  TH1F hDTotalMCWeight("hDTotalMCWeight","hDTotalMCWeight",1,0,2);
  hDTotalMCWeight.SetBinContent(1,hweights->GetBinContent(1));
  fOut->WriteTObject(&hDTotalMCWeight);    

  if (DEBUG) PDebug("Clusterer::Init","Finished configuration");
}


panda::GenParticle const *
redpanda::Clusterer::MatchToGen(double eta, double phi, double radius, int pdgid) 
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


void redpanda::Clusterer::Terminate() {
  fOut->WriteTObject(tOut);
  fOut->Close();

  delete activeArea;
  delete areaDef;
  delete jetDef;
  delete softDrop;
  if (DEBUG) PDebug("Clusterer::Terminate","Finished with output");
}

void redpanda::Clusterer::SetDataDir(const char *s) {

}


// run
void redpanda::Clusterer::Run() {

  // INITIALIZE --------------------------------------------------------------------------

  unsigned int nEvents = tIn->GetEntries();
  unsigned int nZero = 0;
  if (lastEvent>=0 && lastEvent<(int)nEvents)
    nEvents = lastEvent;
  if (firstEvent>=0)
    nZero = firstEvent;

  if (!fOut || !tIn) {
    PError("Clusterer::Run","NOT SETUP CORRECTLY");
    exit(1);
  }

  panda::FatJetCollection* fatjets = &inEvent.puppiCA15Jets;
  panda::JetCollection* jets = &inEvent.chsAK4Jets;

  // set up reporters
  unsigned int iE=0;
  ProgressReporter pr("Clusterer::Run",&iE,&nEvents,10);
  TimeReporter tr("Clusterer::Run",DEBUG);

  // EVENTLOOP --------------------------------------------------------------------------
  for (iE=nZero; iE!=nEvents; ++iE) {
    tr.Start();
    pr.Report();
    inEvent.getEntry(*tIn,iE);
    outEvent.init();

    tr.TriggerEvent("read branches");

    outEvent.pfCandidates = inEvent.pfCandidates;
    outEvent.genParticles = inEvent.genParticles;
    outEvent.recoJets = inEvent.puppiCA15Jets;
    outEvent.recoSubjets = inEvent.puppiCA15Subjets;
    outEvent.genJets = inEvent.ca15GenJets;

    outEvent.eventNumber = inEvent.eventNumber;
    outEvent.lumiNumber = inEvent.lumiNumber;
    outEvent.runNumber = inEvent.runNumber;
    outEvent.weight = inEvent.weight;

    tr.TriggerEvent("copy branches");


    std::vector<panda::GenParticle*> hadrons; 
    for (auto &part : outEvent.genParticles) {
      if (!part.finalState)
        continue;
      unsigned apdgid = abs(part.pdgid);
      if (apdgid == 12 ||
          apdgid == 14 ||
          apdgid == 16)
        continue;

      float pt = part.pt();
      float eta = part.eta();
      float phi = part.phi();
      bool matched = false;
      for (auto *stored : hadrons) {
        if (DeltaR2(eta,phi,stored->eta(),stored->phi()) < 0.0001 && 
            fabs(pt-stored->pt()) < 0.01*pt ) {
          matched = true;
          break;
        }
      }
      if (matched)
        continue;

      /*
      auto &parent = part.parent;
      if (parent.isValid()) {
        // do this instead of remove-erase, since there should only be one instance 
        auto found = std::find(hadrons.begin(),hadrons.end(),parent.get());
        if (found != hadrons.end()) {
          hadrons.erase(found);
        }
      }
      */

      if (pt>0 && fabs(part.eta())<4.7) {
        hadrons.push_back(&part);
      }
    }

    tr.TriggerEvent("find hadrons");


    VPseudoJet particles = ConvertGenParticles(hadrons,0.01);
    fastjet::ClusterSequence seq(particles,*jetDef);
    VPseudoJet allJets(fastjet::sorted_by_pt(seq.inclusive_jets(0.)));
    for (auto &jet : allJets) {
      if (jet.perp() < 150)
        continue;
      auto &tjet = outEvent.truthJets.create_back();
      tjet.setPtEtaPhiM(jet.perp(),jet.eta(),jet.phi(),jet.m());

      for (auto &c : fastjet::sorted_by_pt(jet.constituents())) {
        tjet.constituents.addRef(hadrons[c.user_index()]);
      }
    }

    tr.TriggerEvent("truth jet clustering");

    outEvent.fill(*tOut);
    tr.TriggerEvent("fill");

  } // entry loop

  if (DEBUG) { PDebug("Clusterer::Run","Done with entry loop"); }

} // Run()

