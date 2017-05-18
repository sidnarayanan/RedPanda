#include "../interface/ImageTree.h"

using namespace redpanda;

ImageTree::ImageTree(unsigned dim) 
{
//STARTCUSTOMCONST
  genImage = new TH2F("gen","",dim,-1,1,dim,-1,1);
  truthImage = new TH2C("truth","",dim,-1,1,dim,-1,1);  
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
}

void ImageTree::WriteTree(TTree *t) {
  treePtr = t;
//STARTCUSTOMWRITE
  BookTObject("gen","TH2F",genImage);
  BookTObject("truth","TH2C",truthImage);
//ENDCUSTOMWRITE
}

