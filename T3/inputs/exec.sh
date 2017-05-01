#!/bin/bash

WD=$PWD

export SCRAM_ARCH=slc6_amd64_gcc530
export VO_CMS_SW_DIR=/cvmfs/cms.cern.ch
source $VO_CMS_SW_DIR/cmsset_default.sh

ls
mv local*cfg local.cfg

export X509_USER_PROXY=${PWD}/x509up
export HOME=.

RELEASE=CMSSW_8_0_20
scram p CMSSW $RELEASE
tar xzf cmssw.tgz -C $RELEASE

cd $RELEASE
eval `scram runtime -sh`
cd -

echo -n "file length "
wc -l local.cfg

python skim.py $@

ls
rm -rf $RELEASE skim.py x509up cmssw.tgz local.cfg *root
