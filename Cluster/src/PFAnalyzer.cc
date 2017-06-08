#include "../interface/PFAnalyzer.h"
#include "TVector2.h"
#include "TMath.h"
#include <algorithm>
#include <vector>
#include <unordered_set>

using namespace panda;
using namespace redpanda;
using namespace std;

PFAnalyzer::PFAnalyzer(int debug_/*=0*/) {
  DEBUG = debug_;

  it = new PFTree();

  if (DEBUG) PDebug("PFAnalyzer::PFAnalyzer","Calling constructor");
}


PFAnalyzer::~PFAnalyzer() {
  if (DEBUG) PDebug("PFAnalyzer::~PFAnalyzer","Calling destructor");
}


void PFAnalyzer::Reset() {
  it->Reset();
}

void PFAnalyzer::SetOutputFile(TString fOutName) {
  fOut = new TFile(fOutName,"RECREATE");
  tOut = new TTree("jets","jets");

  it->WriteTree(tOut);

  if (DEBUG) PDebug("PFAnalyzer::SetOutputFile","Created output in "+fOutName);
}


int PFAnalyzer::Init(TTree *t)
{
  if (DEBUG) PDebug("PFAnalyzer::Init","Starting initialization");
  if (!t) {
    PError("PFAnalyzer::Init","Malformed input!");
    return 1;
  }
  tIn = t;

  event.setStatus(*t, {"!*"}); // turn everything off first
  panda::utils::BranchList readlist({"runNumber", "lumiNumber", "eventNumber",
                                     "weight", "recoJets", "pfCandidates"});
  readlist.setVerbosity(DEBUG);
  event.setAddress(*t, readlist); 

  if (DEBUG) PDebug("PFAnalyzer::Init","Set addresses");

  if (DEBUG) PDebug("PFAnalyzer::Init","Finished configuration");

  return 0;
}


panda::GenParticle const *
PFAnalyzer::MatchToGen(double eta, double phi, double radius, int pdgid) 
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


void PFAnalyzer::Terminate() {
  fOut->WriteTObject(tOut);
  fOut->Close();

  delete it;

  if (DEBUG) PDebug("PFAnalyzer::Terminate","Finished with output");
}


// run
void PFAnalyzer::Run() {

  // INITIALIZE --------------------------------------------------------------------------

  unsigned int nEvents = tIn->GetEntries();
  unsigned int nZero = 0;
  if (lastEvent>=0 && lastEvent<(int)nEvents)
    nEvents = lastEvent;
  if (firstEvent>=0)
    nZero = firstEvent;

  if (!fOut || !tIn) {
    PError("PFAnalyzer::Run","NOT SETUP CORRECTLY");
    exit(1);
  }

  // set up reporters
  unsigned int iE=0;
  ProgressReporter pr("PFAnalyzer::Run",&iE,&nEvents,10);
  TimeReporter tr("PFAnalyzer::Run",DEBUG);


  // EVENTLOOP --------------------------------------------------------------------------
  for (iE=nZero; iE!=nEvents; ++iE) {
    tr.Start();
    pr.Report();
    event.getEntry(*tIn,iE);

    tr.TriggerEvent("read branches");

    for (auto& jet : event.recoJets) {
      if (jet.pt() < 250)
        continue;

      it->nPFTrue = jet.constituents.size();
      it->nPF = min(20, it->nPFTrue);

      for (unsigned iP = 0; iP != it->nPF; ++iP) {
        auto* pf = jet.constituents.at(iP).get();
        it->px[iP] = pf->px() * pf->puppiW();
        it->py[iP] = pf->py() * pf->puppiW();
        it->pz[iP] = pf->pz() * pf->puppiW();
        it->e[iP] = pf->e() * pf->puppiW();
        it->ptype[iP] = pf->ptype;
      }

      it->jetPt = jet.pt();
      it->jetEta = jet.eta();
      it->jetPhi = jet.phi();
      it->jetM = jet.m();
      it->jetMSD = jet.mSD;

      it->Fill();
      tr.TriggerSubEvent("fill");
    }
    tr.TriggerEvent("all reco jets");

  } // entry loop

  if (DEBUG) { PDebug("PFAnalyzer::Run","Done with entry loop"); }

} // Run()

