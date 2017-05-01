#!/usr/bin/env python

from os import getenv
from PandaCore.Tools.job_management import *
import subprocess
import sys
import argparse
from glob import glob
from re import sub

outdir = getenv('SUBMIT_OUTDIR')
parser = argparse.ArgumentParser(description='check missing files')
parser.add_argument('--infile',type=str)
parser.add_argument('--outfile',type=str)
parser.add_argument('--outdir',type=str,default=outdir)
parser.add_argument('--force',action='store_true')
parser.add_argument('--nfiles',type=int,default=-1)
args = parser.parse_args()
outdir = args.outdir

class Output:
  def __init__(self,name):
    self.name = name
    self.total = 0
    self.done = 0
  def add(self,isComplete):
    self.total += 1
    if isComplete:
      self.done += 1
  def __str__(self):
    if self.total==0:
      return ''
    s = '%-80s'%self.name
    frac = 1.*self.done/self.total
    s += '\t[\033[0;42m'
    switched = False
    for i in xrange(100):
      if i/100.<frac or switched:
        s += ' '
      else:
        s += '\033[0;41m '
        switched = True
    s += '\033[0m] %i/%i (%.2f%%)'%(self.done,self.total,100*frac)
    return s

sys.argv=[]

root_files_glob = glob(outdir+'/*root')
root_files = [f.split('/')[-1] for f in root_files_glob]

processedfiles = []

locks = glob(outdir+'/locks/*lock')
for lock in locks:
    flock = open(lock)
    for l in flock:
        processedfiles.append(l.strip())

outputs = {}
data = Output('Data')
mc = Output('MC')

all_samples = read_sample_config(args.infile)
filtered_samples = {}
merged_samples = {}
outfile = open(args.outfile,'w')
for name in sorted(all_samples):
    sample = all_samples[name]
    out_sample = DataSample(name,sample.dtype,sample.xsec)

    base_name = sub('_[0-9]+$','',name)
    if base_name not in outputs:
        outputs[base_name] = Output(base_name)
    output = outputs[base_name]
    if base_name not in merged_samples:
        merged_samples[base_name] = DataSample(base_name,sample.dtype,sample.xsec)
    merged_sample = merged_samples[base_name]

    for f in sample.files:
        found = (f in processedfiles)
        if not found:
            out_sample.add_file(f)
            merged_sample.add_file(f)
        output.add(found)
        if sample.dtype=='MC':
            mc.add(found)
        else:
            data.add(found)

    if not args.force: # do not resubmit if partial output doesn't exist
        output_exists=False
        for rf in root_files:
            rf_base = sub('_[0-9]+$','',rf)
            if name==rf_base:
                output_exists = True
                break
        if not output_exists:
            continue
    if len(out_sample.files)>0:
        filtered_samples[name] = out_sample

if args.nfiles<0:
    keys = sorted(filtered_samples)
    for k in keys:
        sample = filtered_samples[k]
        if len(sample.files)==0:
            continue
        configs = sample.get_config(-1)
        for c in configs:
            outfile.write(c)
else:
    keys = sorted(merged_samples)
    counter=0
    for k in keys:
        sample = merged_samples[k]
        if len(sample.files)==0:
            continue
        configs = sample.get_config(args.nfiles,suffix='_%i')
        for c in configs:
            outfile.write(c%(counter,counter))
            counter += 1

for n in sorted(outputs):
  print str(outputs[n])
print str(data)
print str(mc)

outfile.close()

