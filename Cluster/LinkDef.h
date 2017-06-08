#include "RedPanda/Cluster/interface/AnalyzerUtilities.h"
#include "RedPanda/Cluster/interface/Clusterer.h"
#include "RedPanda/Cluster/interface/ImageTree.h"
#include "RedPanda/Cluster/interface/genericTree.h"
#include "RedPanda/Cluster/interface/Camera.h"
#include "RedPanda/Cluster/interface/PFTree.h"
#include "RedPanda/Cluster/interface/PFAnalyzer.h"


#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;
#pragma link C++ namespace redpanda;

#pragma link C++ class redpanda::Clusterer;
#pragma link C++ class redpanda::ImageTree;
#pragma link C++ class redpanda::Camera;
#pragma link C++ class redpanda::PFTree;
#pragma link C++ class redpanda::PFAnalyzer;

#endif 
