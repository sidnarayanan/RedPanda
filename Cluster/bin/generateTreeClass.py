#!/usr/bin/env python

from sys import argv,exit
from os import system
from re import sub
import argparse
from os.path import isfile

parser = argparse.ArgumentParser(description='build object from configuration')
parser.add_argument('--cfg',type=str)
args = parser.parse_args()


'''
dtype can be:
    float         # float
    float:-1.     # float with default -1
    float[nJ]     # float array of variable size
    loat[nJ]:-1   # float array with default -1
'''


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

global_branches = []

class Branch:
    def __init__(self,name,dtype):
        self.name = name
        self.dtype = dtype.split(':')[0]
        self.default = dtype.split(':')[1] if ':' in dtype else None
        self.suffix = ''
        try:
            base_dtype = sub('\[.*\]','',dtype)
            self.suffix = self.dtype.replace(base_dtype, suffixes[base_dtype])
        except KeyError:
            # must be a TObject
            self.suffix = dtype
    def create_def(self):
        if '[' in self.dtype:
            base_dtype = sub('\[.*\]','',dtype)
            base_ctype = ctypes[base_dtype]
            return '    %s %s;\n'%(self.dtype.replace(base_dtype,base_ctype),self.name)
        else:
            return '    %s %s = -1;\n'%(ctypes[self.dtype],self.name)
    def create_constructor(self):
        return '' # do we need anything here?
    def create_reset(self):
        # only handle singletons for now
        if self.default:
            val = self.default
        elif 'sf_' in self.name:
            val = '1'
        elif self.dtype=='float':
            val = '-1'
        else:
            val = '0'
        return '    %s = %s;\n'%(self.name,val)
    def create_read(self):
        return '' # not implemented anymore
    def create_write(self):
        if '[' in self.dtype:
            return '' # now I'm just being lazy
        else:
            return '    Book("{0}",&{0},"{0}/{1}");\n'.format(self.name,self.suffix)

def get_template(path):
    if isfile(path):
        with open(path) as ftmpl:
            r = list(ftmpl.readlines())
            return r
    else:
        classname = path.split('/')[-1].split('.')[0]
        path = '/'.join(path.split('/')[:-1])+'/tree.tmpl'
        with open(path) as ftmpl:
            r = list([x.replace('CLASSNAME',classname) for x in ftmpl.readlines()])
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


for line in get_template(cfg_path):
    line = line.strip()
    if line[0]=='#':
        continue
    name,dtype = line.split()
    if name in predefined:
        continue
    print 'Adding new variable %s %s'%(dtype,name)
    global_branches.append( Branch(name,dtype) )


header_lines = get_template(header_path)
def_lines = get_template(def_path)

for path in [header_path,def_path]:
    system('cp {0} {0}.bkp'.format(path))

with open(header_path,'w') as fheader:
    for line in header_lines:
        fheader.write(line)
        if '//ENDCUSTOMDEF' in line:
            for b in global_branches:
                fheader.write(b.create_def())

with open(def_path,'w') as fdef:
    for line in def_lines:
        fdef.write(line)
        if '//ENDCUSTOMCONST' in line:
            for b in global_branches:
                fdef.write(b.create_constructor())
        elif '//ENDCUSTOMRESET' in line:
            for b in global_branches:
                fdef.write(b.create_reset())
        elif '//ENDCUSTOMWRITE' in line:
            for b in global_branches:
                fdef.write(b.create_write())

