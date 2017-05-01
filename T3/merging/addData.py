#!/usr/bin/env python

from sys import argv
f1 = argv[1]
f2 = argv[2]
f3 = argv[3]
argv = []

import PandaCore.Tools.Load as Load
import ROOT as root

Load.Load('Tools','DuplicateRemover')

dr = root.DuplicateRemover()
dr.Merge(f1,f2,f3)

