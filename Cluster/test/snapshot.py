#!/usr/bin/env python

from re import sub
from sys import argv,exit
from os import system,getenv
import json

debug_level = 2
torun = argv[1]
output = 'snapshot.root'
if len(argv)>2:
    debug_level = int(argv[2])
    if len(argv)>3:
        output = argv[3]

argv = []

import ROOT as root
from PandaCore.Tools.Load import *

Load('Clusterer')

camera = root.redpanda.Camera(debug_level)


camera.firstEvent=1
camera.lastEvent=2
fin = root.TFile.Open(torun)

tree = fin.FindObjectAny("events")

camera.SetOutputFile(output)
camera.Init(tree)

camera.Run()
camera.Terminate()
