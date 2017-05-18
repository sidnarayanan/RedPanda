#ifndef REDPANDA_IMAGETREE_H
#define REDPANDA_IMAGETREE_H 

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH2C.h"
#include "TString.h"
#include "genericTree.h"

namespace redpanda {
    class ImageTree : public genericTree {
            
        public:
          ImageTree(unsigned dim = 20);
          ~ImageTree();
          void WriteTree(TTree *t);
          void Fill() { treePtr->Fill(); }
          void SetBranchStatus(const char *bname, bool status, UInt_t *ret=0) 
          { 
            treePtr->SetBranchStatus(bname,status,ret); 
          }
          void Reset();

    //STARTCUSTOMDEF
          TH2F *genImage{0};
          TH2C *truthImage{0};
    //ENDCUSTOMDEF
    };
}

#endif
