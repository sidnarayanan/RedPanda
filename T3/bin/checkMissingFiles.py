#!/usr/bin/env python

from os import getenv,path,popen
from PandaCore.Tools.job_management import *
import subprocess
import sys
import argparse
from glob import glob
from re import sub
import cPickle as pickle
from itertools import chain 

outdir = getenv('SUBMIT_OUTDIR')
workdir = getenv('SUBMIT_WORKDIR')
parser = argparse.ArgumentParser(description='check missing files')
parser.add_argument('--infile',type=str,default=None)
parser.add_argument('--outfile',type=str,default=None)
parser.add_argument('--outdir',type=str,default=outdir)
parser.add_argument('--force',action='store_true')
parser.add_argument('--nfiles',type=int,default=-1)
parser.add_argument('--width',type=int,default=None)
args = parser.parse_args()
outdir = args.outdir

if not args.infile:
    args.infile = workdir+'/local_all.cfg'
if not args.outfile:
    args.outfile = workdir+'/local.cfg'

if not args.width:
    columns = int(popen('stty size', 'r').read().split()[-1])
    WIDTH = (columns-80)/2
else:
    WIDTH = args.width
header = ('%%-%is'%(WIDTH))%('Sample')
header += ('%%-%is'%(WIDTH+2))%('Progress')
header += ' %10s %10s %10s %10s %10s'%('Total','Running','Idle','Missing','Done')

colors = {
    'green' : 42,
    'blue' : 44,
    'grey' : 47, 
    'red' : 41,
    }

#if getenv('SUBMIT_CONFIG'):
#  setup_schedd(getenv('SUBMIT_CONFIG'))

class Output:
  def __init__(self,name):
    self.name = name
    self.total = 0
    self.done = 0
    self.idle = 0
    self.running = 0
    self.missing = 0
  def add(self,state):
    self.total += 1
    if state=='done':
        self.done += 1
    elif state=='running':
        self.running += 1
    elif state=='idle':
        self.idle += 1
    elif state=='missing':
        self.missing += 1
  def __str__(self):
    if self.total==0:
      return ''
    s = ('%%-%is '%(WIDTH-1))%self.name[:(WIDTH-1)]
    d_frac = 1.*WIDTH*self.done/self.total
    r_frac = 1.*WIDTH*(self.done+self.running)/self.total
    i_frac = 1.*WIDTH*(self.idle+self.done+self.running)/self.total
    s += '[\033[0;%im'%colors['green']
    state = 0
    for i in xrange(WIDTH):
        if i>=d_frac:
            s += '\033[0;%im'%colors['blue']
        if i>=r_frac:
            s += '\033[0;%im'%colors['grey']
        if i>=i_frac:
            s += '\033[0;%im'%colors['red']
        s += ' '
    s += '\033[0m] '
    s += '%10i '%self.total
    s += '%10i '%self.running
    s += '%10i '%self.idle
    s += '%10i '%self.missing
    s += '%10i '%self.done
    s += '(done=%.2f%%)'%(d_frac*100./WIDTH)
    return s


# determine what files have been processed and logged as such
processedfiles = []
print 'Finding locks...                      \r',
locks = glob(outdir+'/locks/*lock')
nl = len(locks)
il = 1
for lock in locks:
    print 'Reading lock %i/%i                   \r'%(il,nl),
    il += 1
    flock = open(lock)
    for l in flock:
        processedfiles.append(l.strip())

print 'Checking jobs...                 \r',

# determine what samples from previous resubmissions are still running
running_samples = []
idle_samples = []
if path.isfile(workdir+'/submission.pkl'): 
    with open(workdir+'/submission.pkl','rb') as fpkl:
      submissions = pickle.load(fpkl)
else:
    submissions = []
for s in submissions:
    results = s.query_status()
    running_samples += results['running']
    idle_samples += results['idle']

running_files = list(chain.from_iterable([x.files for x in running_samples]))
idle_files = list(chain.from_iterable([x.files for x in idle_samples]))


# for fancy display
outputs = {}
data = Output('Data')
mc = Output('MC')

print 'Rebuilding configuration...            \r',

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

    to_resubmit = []

    for f in sample.files:
        state = 'missing'
        if f in processedfiles:
            state = 'done'
        elif f in running_files:
            state = 'running'
        elif f in idle_files:
            state = 'idle'

        if state=='missing' or (args.force and state!='done'):
            out_sample.add_file(f)
            merged_sample.add_file(f)

        output.add(state)
        if sample.dtype=='MC':
            mc.add(state)
        else:
            data.add(state)

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

print '\r',
print 'Summary:                     '

print header
for n in sorted(outputs):
    print str(outputs[n])
print
print str(data)
print str(mc)
print
print 'Legend: Done=\033[0;%im    \033[0m, Running=\033[0;%im    \033[0m, Idle=\033[0;%im    \033[0m, Missing=\033[0;%im    \033[0m, '%(colors['green'],colors['blue'],colors['grey'],colors['red'])

outfile.close()

print '\nMost recent submission:'
import query
