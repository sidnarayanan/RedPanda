#include "../interface/Camera.h"
#include "TVector2.h"
#include "TMath.h"
#include <algorithm>
#include <vector>

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

  // EVENTLOOP --------------------------------------------------------------------------
  for (iE=nZero; iE!=nEvents; ++iE) {
    tr.Start();
    pr.Report();
    event.getEntry(*tIn,iE);

    tr.TriggerEvent("read branches");

    TH1D hEta("hEta","",100,0,3);
    TH1D hPhi("hPhi","",100,0,3.2);
    double quantiles[1] = {0.95};
    double qEta[1]; 
    double qPhi[1];

    for (auto& j : event.truthJets) {
      it->Reset();
      hEta.Reset(); hPhi.Reset();

      double eta = j.eta();
      double phi = j.phi();

      for (auto cref : j.constituents) {
        auto *c = cref.get();
        double ceta = c->eta();
        double cphi = c->phi();

        hEta.Fill(fabs(ceta - eta), c->pt());
        hPhi.Fill(fabs(SignedDeltaPhi(cphi,phi)), c->pt());
      }
      
      hEta.ComputeIntegral();
      hPhi.ComputeIntegral();

      hEta.GetQuantiles(1,qEta,quantiles);
      hPhi.GetQuantiles(1,qPhi,quantiles);

      if (DEBUG) PDebug("Camera::Run",TString::Format("jet dimensions are deta=%.3f, dphi=%.3f",qEta[0], qPhi[0]));

      double maxpt = 0;
      for (auto cref : j.constituents) {
        auto *c = cref.get();
        double ceta = c->eta();
        double cphi = c->phi();

        ceta = (ceta - eta) / qEta[0];
        cphi = SignedDeltaPhi(cphi,phi) / qPhi[0];

        it->genImage->Fill(ceta,cphi,c->pt());
        maxpt = max(maxpt, double(c->pt()));
      }

      if (DEBUG) PDebug("Camera::Run",TString::Format("jet pT=%.3f, sum pT=%.3f, max pT=%.3f",j.pt(),it->genImage->Integral(),maxpt));

      tr.TriggerSubEvent("truth jet");

      it->Fill();
      tr.TriggerSubEvent("fill");
    }

    tr.TriggerEvent("all truth jets");

  } // entry loop

  if (DEBUG) { PDebug("Camera::Run","Done with entry loop"); }

} // Run()

