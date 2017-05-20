#ifndef Camera_h
#define Camera_h

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
#include <TH2S.h>
#include <TLorentzVector.h>

#include "AnalyzerUtilities.h"
#include "ImageTree.h"


/////////////////////////////////////////////////////////////////////////////
// Camera definition
namespace redpanda {
  class Camera {
  public :

    //////////////////////////////////////////////////////////////////////////////////////

    Camera(int debug_=0);
    ~Camera();
    int Init(TTree *tree);
    void SetOutputFile(TString fOutName);
    void Run();
    void Terminate();
    void Reset();

    // public configuration
    bool isData=false;                                                 // to do gen matching, etc
    int firstEvent=-1;
    int lastEvent=-1;                                                    // max events to process; -1=>all
    double zcut = 0.25; //!< definition of a "hard" parton
    TH2F *hFilter=NULL; //!< used to downsample the image in absolute eta-phi

  private:
    int DEBUG = 0; //!< debug verbosity level
    std::map<TString,bool> flags;

    std::map<panda::GenParticle const*,float> genObjects;                 //!< particles we want to match the jets to, and the 'size' of the daughters
    panda::GenParticle const* MatchToGen(double eta, double phi, double r2, int pdgid=0);        //!< private function to match a jet; returns NULL if not found
    
    // IO for the analyzer
    TFile *fOut;     // output file is owned by Camera
    TTree *tOut;
    ImageTree *it;
    TTree *tIn=0;    // input tree to read

    // objects to read from the tree
    panda::EventRed event;

  };
};


#endif

