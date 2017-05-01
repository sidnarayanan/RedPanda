#ifndef Clusterer_h
#define Clusterer_h

// STL
#include "vector"
#include "map"
#include <string>
#include <cmath>

// ROOT
#include <TTree.h>
#include <TFile.h>
#include <TMath.h>
#include <TH1D.h>
#include <TH2F.h>
#include <TLorentzVector.h>

#include "AnalyzerUtilities.h"


/////////////////////////////////////////////////////////////////////////////
// Clusterer definition
namespace redpanda {
  class Clusterer {
  public :

    enum ProcessType { 
      kNone,
      kZ,
      kW,
      kA,
      kTT,
      kTop, // used for non-ttbar top
      kV, // used for non V+jets W or Z
      kH
    };

    enum TriggerBits {
      kMETTrig       =(1<<0),
      kSingleEleTrig =(1<<1),
      kSinglePhoTrig =(1<<2),
      kSingleMuTrig     =(1<<3)
    };

    //////////////////////////////////////////////////////////////////////////////////////

    Clusterer(int debug_=0);
    ~Clusterer();
    void Init(TTree *tree, TH1D *hweights);
    void SetOutputFile(TString fOutName);
    void Run();
    void Terminate();
    void SetDataDir(const char *s);

    // public configuration
    void SetFlag(TString flag, bool b=true) { flags[flag]=b; }
    bool isData=false;                                                 // to do gen matching, etc
    int firstEvent=-1;
    int lastEvent=-1;                                                    // max events to process; -1=>all
    ProcessType processType=kNone;                         // determine what to do the jet matching to

  private:
    bool PassPreselection() { return true; }

    int DEBUG = 0; //!< debug verbosity level
    std::map<TString,bool> flags;

    std::map<panda::GenParticle const*,float> genObjects;                 //!< particles we want to match the jets to, and the 'size' of the daughters
    panda::GenParticle const* MatchToGen(double eta, double phi, double r2, int pdgid=0);        //!< private function to match a jet; returns NULL if not found
    
    // fastjet reclustering
    fastjet::JetDefinition *jetDef=0;
    fastjet::contrib::SoftDrop *softDrop=0;
    fastjet::AreaDefinition *areaDef=0;
    fastjet::GhostedAreaSpec *activeArea=0;

    // IO for the analyzer
    TFile *fOut;     // output file is owned by Clusterer
    TTree *tOut;
    TTree *tIn=0;    // input tree to read
    unsigned int preselBits=0;

    // objects to read from the tree
    panda::Event inEvent;
    panda::EventRed outEvent;

  };
};


#endif

