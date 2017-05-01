#!/usr/bin/env python

from sys import argv,exit
from os import system
from re import sub
import argparse

parser = argparse.ArgumentParser(description='build object from configuration')
parser.add_argument('--cfg',type=str)
args = parser.parse_args()


suffixes = { 'float':'F', 
                         'int':'I',
                         'uint':'i',
                         'uint64':'l',
                     }
ctypes = {    
                        'uint':'unsigned int',
                        'float':'float',
                        'int':'int',
                        'uint64':'ULong64_t'
                 }

class Branch:
    def __init__(self,name,dtype):
        self.name = name
        self.dtype = dtype
        self.suffix = ''
        try:
            self.suffix = '/'+suffixes[sub('\[.*\]','',dtype)]
            if '[' in dtype:
                self.suffix = '[%s]%s'%(counter,self.suffix)
        except KeyError:
            # must be a TObject
            self.suffix = dtype
    def create_def(self):
        if '[' in self.dtype:
            basedtype = sub('\[.*\]','',self.dtype)
            basectype = ctypes[basedtype]
            return '    %s %s;\n'%(self.dtype.replace(basedtype,basectype),self.name)
        else:
            return '    %s %s = -1;\n'%(ctypes[self.dtype],self.name)
    def create_constructor(self):
        return '' # do we need anything here?
    def create_reset(self):
        # only handle singletons for now
        if 'sf_' in self.name:
            val = 1;
        elif self.dtype=='float':
            val = -1
        else:
            val = 0
        return '    %s = %i;\n'%(self.name,val)
    def create_read(self):
        return '' # not implemented anymore
    def create_write(self):
        if '[' in self.dtype:
            return '' # now I'm just being lazy
        else:
            return '    treePtr->Branch("{0}",&{0},"{0}{1}");\n'.format(self.name,self.suffix)

def get_template(path):
    with open(path) as ftmpl:
        r = list(ftmpl.readlines())
        return r


cfg_path = args.cfg
header_path = cfg_path.replace('config','interface').replace('.cfg','.h')
def_path = cfg_path.replace('config','src').replace('.cfg','.cc')

predefined = set([]) # if something is in CUSTOM, ignore it
custom = False
repl = ['[',']','{','}','=',';',',']
for line in get_template(header_path):
    if 'STARTCUSTOMDEF' in line:
        custom = True
        continue
    if custom:
        members = line.strip()
        for pattern in repl:
            members = members.replace(pattern,' ')
        members = members.split()
        for m in members:
            predefined.add(m)


branches = []
for line in get_template(cfg_path):
    line = line.strip()
    if line[0]=='#':
        continue
    name,dtype = line.split()
    if name in predefined:
        continue
    print 'Adding new variable %s %s'%(dtype,name)
    branches.append( Branch(name,dtype) )


header_lines = get_template(header_path)
def_lines = get_template(def_path)

for path in [header_path,def_path]:
    system('cp {0} {0}.bkp'.format(path))

with open(header_path,'w') as fheader:
    for line in header_lines:
        fheader.write(line)
        if '//ENDCUSTOMDEF' in line:
            for b in branches:
                fheader.write(b.create_def())

with open(def_path,'w') as fdef:
    for line in def_lines:
        fdef.write(line)
        if '//ENDCUSTOMCONST' in line:
            for b in branches:
                fdef.write(b.create_constructor())
        elif '//ENDCUSTOMRESET' in line:
            for b in branches:
                fdef.write(b.create_reset())
        elif '//ENDCUSTOMWRITE' in line:
            for b in branches:
                fdef.write(b.create_write())

