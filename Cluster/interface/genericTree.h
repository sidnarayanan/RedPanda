#ifndef GENERICTREE
#define GENERICTREE 

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TLorentzVector.h"
#include "TClonesArray.h"
#include "TString.h"
#include "TRegexp.h"
#include <vector>

#define NMAX 8
#define NGENMAX 100

namespace redpanda {
  class genericTree {
    public:
      genericTree() {};
      virtual ~genericTree() {};
      TTree *treePtr{0};
      virtual void WriteTree(TTree *t)=0;
      virtual void RemoveBranches(std::vector<TString> droppable,
                                  std::vector<TString> keeppable={}) final;
    protected: 
      virtual bool Book(TString bname, void *address, TString leafs) final;

    private:
      std::vector<TRegexp> r_droppable, r_keeppable;
  };
};

#endif

