#include "../interface/ImageTree.h"

using namespace redpanda;

ImageTree::ImageTree(unsigned dim) 
{
//STARTCUSTOMCONST
  genImage = new TH2F("gen","",dim,-1,1,dim,-1,1);
  truthImage = new TH2S("truth","",dim,-1,1,dim,-1,1);  
//ENDCUSTOMCONST
}

ImageTree::~ImageTree() {
//STARTCUSTOMDEST
  delete genImage;
  delete truthImage;
//ENDCUSTOMDEST
}

void ImageTree::Reset() {
//STARTCUSTOMRESET
  genImage->Reset();
  truthImage->Reset();
//ENDCUSTOMRESET
    runNumber = 0;
    lumiNumber = 0;
    eventNumber = 0;
    npv = 0;
    pu = 0;
    mcWeight = -1;
    mass = -1;
    pt = -1;
    matched = 0;
    gensize = -1;
}

void ImageTree::WriteTree(TTree *t) {
  treePtr = t;
//STARTCUSTOMWRITE
  BookTObject("gen","TH2F",genImage);
  BookTObject("truth","TH2S",truthImage);
//ENDCUSTOMWRITE
    treePtr->Branch("runNumber",&runNumber,"runNumber/I");
    treePtr->Branch("lumiNumber",&lumiNumber,"lumiNumber/I");
    treePtr->Branch("eventNumber",&eventNumber,"eventNumber/l");
    treePtr->Branch("npv",&npv,"npv/I");
    treePtr->Branch("pu",&pu,"pu/I");
    treePtr->Branch("mcWeight",&mcWeight,"mcWeight/F");
    treePtr->Branch("mass",&mass,"mass/F");
    treePtr->Branch("pt",&pt,"pt/F");
    treePtr->Branch("matched",&matched,"matched/I");
    treePtr->Branch("gensize",&gensize,"gensize/F");
}

