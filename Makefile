# only generates ROOT dictionaries! compilation is left to SCRAM
PACKAGE=$(CMSSW_BASE)/src/RedPanda/
INC=-I/cvmfs/cms.cern.ch/$(SCRAM_ARCH)/external/fastjet-contrib/1.020/include/ -I/cvmfs/cms.cern.ch/$(SCRAM_ARCH)/external/fastjet/3.1.0/include/

.PHONY: clean all

all: Cluster/src/dictCluster.cc 

clean:
	rm -f */src/dict*.cc */src/dict*pcm

Cluster/src/dictCluster.cc: $(wildcard $(PACKAGE)/Cluster/interface/*.h) $(PACKAGE)/Cluster/LinkDef.h
	rootcling -f $(PACKAGE)/Cluster/src/dictCluster.cc $(CMSSW_BASE)/src/RedPanda/Cluster/interface/*.h $(INC) $(CMSSW_BASE)/src/RedPanda/Cluster/LinkDef.h
	mkdir -p $(CMSSW_BASE)/lib/$(SCRAM_ARCH)/
	cp Cluster/src/dictCluster_rdict.pcm $(CMSSW_BASE)/lib/$(SCRAM_ARCH)/


# DO NOT DELETE
