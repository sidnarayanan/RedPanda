#ifndef REDPANDA_PFTree_H
#define REDPANDA_PFTree_H 

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH2S.h"
#include "TString.h"
#include "genericTree.h"

#define NMAXPF 20

namespace redpanda {
    class PFTree : public genericTree {
            
        public:
          PFTree();
          ~PFTree();
          void WriteTree(TTree *t);
          void Fill() { treePtr->Fill(); }
          void SetBranchStatus(const char *bname, bool status, UInt_t *ret=0) 
          { 
            treePtr->SetBranchStatus(bname,status,ret); 
          }
          void Reset();

    //STARTCUSTOMDEF
    //ENDCUSTOMDEF
    float jetPt = -1;
    float jetEta = -1;
    float jetPhi = -1;
    float jetM = -1;
    float jetMSD = -1;
    int runNumber = -1;
    int lumiNumber = -1;
    ULong64_t eventNumber = -1;
    int idx = -1;
    int nPF = -1;
    int nPFTrue = -1;
    float px[NMAXPF];
    float py[NMAXPF];
    float pz[NMAXPF];
    float e[NMAXPF];
    int ptype[NMAXPF];
    };
}

#endif
