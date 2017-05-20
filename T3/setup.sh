#!/bin/bash

export PATH=${PATH}:${CMSSW_BASE}/src/PandaCore/bin/

export SUBMIT_NAME="v2"

export PANDA="${CMSSW_BASE}/src/RedPanda"
export PANDA_CFG="http://t3serv001.mit.edu/~snarayan/histcatalog/20170520_redpanda.cfg"

export SUBMIT_TMPL="skim_tmpl.py"

export SUBMIT_WORKDIR="/data/t3serv014/snarayan/condor/"${SUBMIT_NAME}"/work/"
export SUBMIT_LOGDIR="/data/t3serv014/snarayan/condor/"${SUBMIT_NAME}"/logs/"
export SUBMIT_OUTDIR="/mnt/hadoop/cms/store/user/snarayan/store/redpanda/"${SUBMIT_NAME}"/"
#export SUBMIT_OUTDIR="/mnt/hadoop/scratch/snarayan/panda/"${SUBMIT_NAME}"/batch/"
mkdir -p $SUBMIT_WORKDIR $SUBMIT_OUTDIR/locks/ $SUBMIT_LOGDIR

#export SUBMIT_CONFIG="SubMIT"
export SUBMIT_CONFIG="T3"
