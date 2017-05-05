#!/usr/bin/env python

from re import sub
from sys import argv,exit
from os import system,getenv
import json

debug_level = 2
torun = argv[1]
output = 'testskim.root'
if len(argv)>2:
    debug_level = int(argv[2])
    if len(argv)>3:
        output = argv[3]

argv = []

import ROOT as root
from PandaCore.Tools.Load import *

Load('Clusterer')

skimmer = root.redpanda.Clusterer(debug_level)


skimmer.firstEvent=0
skimmer.lastEvent=1
skimmer.isData=False
skimmer.processType = root.redpanda.Clusterer.kTT
fin = root.TFile.Open(torun)

tree = fin.FindObjectAny("events")
hweights = fin.FindObjectAny("hSumW")

skimmer.SetDataDir(getenv('CMSSW_BASE')+'/src/PandaAnalysis/data/')
skimmer.SetOutputFile(output)
skimmer.Init(tree,hweights)

skimmer.Run()
skimmer.Terminate()
