#!/usr/bin/env python

from re import sub
from sys import argv,exit
from os import system,getenv
import json

debug_level = 2
torun = argv[1]
output = 'pfskim.root'
if len(argv)>2:
    debug_level = int(argv[2])
    if len(argv)>3:
        output = argv[3]

argv = []

import ROOT as root
from PandaCore.Tools.Load import *

Load('PFAnalyzer')

skimmer = root.redpanda.PFAnalyzer(debug_level)


skimmer.firstEvent=0
skimmer.lastEvent=20
skimmer.isData=False
fin = root.TFile.Open(torun)

tree = fin.FindObjectAny("events")

skimmer.SetOutputFile(output)
skimmer.Init(tree)

skimmer.Run()
skimmer.Terminate()
