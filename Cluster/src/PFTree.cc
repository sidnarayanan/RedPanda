#include "../interface/PFTree.h"

using namespace redpanda;

PFTree::PFTree() 
{
//STARTCUSTOMCONST
    for (unsigned int iP=0; iP!=NMAXPF; ++iP) {
      px[iP]=0;
      py[iP]=0;
      pz[iP]=0;
      e[iP]=0;
      ptype[iP]=0;
    }
//ENDCUSTOMCONST
}

PFTree::~PFTree() {
//STARTCUSTOMDEST
//ENDCUSTOMDEST
}

void PFTree::Reset() {
//STARTCUSTOMRESET
    for (unsigned int iP=0; iP!=NMAXPF; ++iP) {
      px[iP]=0;
      py[iP]=0;
      pz[iP]=0;
      e[iP]=0;
      ptype[iP]=0;
    }
//ENDCUSTOMRESET
    jetPt = -1;
    jetEta = -1;
    jetPhi = -1;
    jetM = -1;
    jetMSD = -1;
    runNumber = 0;
    lumiNumber = 0;
    eventNumber = 0;
    idx = 0;
    nPF = 0;
    nPFTrue = 0;
}

void PFTree::WriteTree(TTree *t) {
  treePtr = t;
//STARTCUSTOMWRITE
    Book("nPF",&nPF,"nPF/I");
    Book("px",px,TString::Format("px[%i]/F",NMAXPF).Data());
    Book("py",py,TString::Format("py[%i]/F",NMAXPF).Data());
    Book("pz",pz,TString::Format("pz[%i]/F",NMAXPF).Data());
    Book("e",e,TString::Format("e[%i]/F",NMAXPF).Data());
    Book("ptype",ptype,TString::Format("ptype[%i]/I",NMAXPF).Data());
//ENDCUSTOMWRITE
    Book("jetPt",&jetPt,"jetPt/F");
    Book("jetEta",&jetEta,"jetEta/F");
    Book("jetPhi",&jetPhi,"jetPhi/F");
    Book("jetM",&jetM,"jetM/F");
    Book("jetMSD",&jetMSD,"jetMSD/F");
    Book("runNumber",&runNumber,"runNumber/I");
    Book("lumiNumber",&lumiNumber,"lumiNumber/I");
    Book("eventNumber",&eventNumber,"eventNumber/l");
    Book("idx",&idx,"idx/I");
    Book("nPFTrue",&nPFTrue,"nPFTrue/I");
}

