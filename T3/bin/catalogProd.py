#!/usr/bin/env python

from glob import glob
from os import stat,getenv
from PandaCore.Tools.process import *
from re import sub
import sys
import argparse

print 'starting'

parser = argparse.ArgumentParser(description='make config file')
parser.add_argument('--outfile',type=str)
parser.add_argument('--include',nargs='+',type=str,default=None)
parser.add_argument('--exclude',nargs='+',type=str,default=None)
args = parser.parse_args()

eosproddirs = getenv('PANDA_PROD').split(':')

listOfFiles=[]
for d in eosproddirs:
  print 'searching in',d
  listOfFiles += glob(d+'/*/*/*/*/panda_*.root')

def checkDS(nickname,include,exclude):
  included=False
  if include:
    for i in include:
      if i in nickname:
        included=True
        break
  else:
    included=True
  excluded=False
  if exclude:
    for e in exclude:
      if e in nickname:
        excluded=True
        break
  else:
    excluded=False
  return (included and not(excluded))

print 'found %i files'%len(listOfFiles)
cfgFile = open(args.outfile,'w')

couldNotFind = []

file_counter=0
for f in sorted(listOfFiles):
  if stat(f).st_size==0:
    continue
  ff = f.split('/')
  if 'cernbox' in f:
    if 'bmaier' in f:
      pd = ff[-5]
      start=10
      fileName = "${CERNBOXB}"
    else:
      pd = ff[-5]
      start=10
      fileName = "${CERNBOX}"
  else:
    pd = ff[-5]
    start=11
    if 'phys_exotica' in f:
      fileName = "${EOS2}"
    else:
      fileName = "${EOS}"
  for iF in xrange(start,len(ff)):
    fileName += "/"
    fileName += ff[iF]
  try:
    properties = processes[pd]
    nickname = properties[0]
  except KeyError:
    if pd not in couldNotFind:
      couldNotFind.append(pd)
    properties = ('UNKNOWN','UNKNOWN',-1)
    continue
  if checkDS(nickname,args.include,args.exclude):
    nickname += '_%i'%(file_counter)
    cfgFile.write('{0:<25} {2:<10} {3:<15} {1:<180}\n'.format(nickname,fileName,properties[1],properties[2])) 
    file_counter += 1

if len(couldNotFind)>0: 
  print 'could not find:'
for pd in couldNotFind:
  print '\t',pd
