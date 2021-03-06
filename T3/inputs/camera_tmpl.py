#!/usr/bin/env python

from re import sub
from sys import argv,exit
from os import system,getenv,path
from time import clock,time
import json

which = int(argv[1])
submit_id = int(argv[2])
sname = argv[0]
argv=[]

import ROOT as root
from PandaCore.Tools.Misc import *
from PandaCore.Tools.Load import *
import PandaCore.Tools.job_management as cb
import RedPanda.Cluster.convert_images as ci

Load('Camera')

stopwatch = clock() 
def print_time(label):
    global stopwatch
    now_ = clock()
    PDebug(sname+'.print_time:'+str(time()),
           '%.3f s elapsed performing "%s"'%((now_-stopwatch),label))
    stopwatch = now_

def copy_local(long_name):
    full_path = long_name
    PInfo(sname+'.copy_local',full_path)

    panda_id = long_name.split('/')[-1].split('_')[-1].replace('.root','')
    input_name = 'input_%s.root'%panda_id

    cmd = "cp %s %s"%(full_path,input_name)
    PInfo(sname+'.copy_local',cmd)

    system(cmd)
            
    if path.isfile(input_name):
        PInfo(sname+'.copy_local','Successfully copied to %s'%(input_name))
        return input_name
    else:
        PError(sname+'.copy_local','Failed to copy %s'%input_name)
        return None


def fn(input_name,isData,full_path):
    
    PInfo(sname+'.fn','Starting to process '+input_name)
    # now we instantiate and configure the camera
    camera = root.redpanda.Camera()

    # read the inputs
    try:
        fin = root.TFile.Open(input_name)
        tree = fin.FindObjectAny("events")
    except:
        PError(sname+'.fn','Could not read %s'%input_name)
        return False # file open error => xrootd?
    if not tree:
        PError(sname+'.fn','Could not recover tree in %s'%input_name)
        return False

    output_name = input_name.replace('input','output')
    camera.SetOutputFile(output_name)
    camera.Init(tree)

    # run and save output
    camera.Run()
    camera.Terminate()

    ret = path.isfile(output_name)
    if ret:
        PInfo(sname+'.fn','Successfully created %s'%(output_name))
        return True
    else:
        PError(sname+'.fn','Failed in creating %s!'%(output_name))
        return False


def cleanup(fname):
    ret = system('rm -f %s'%(fname))
    if ret:
        PError(sname+'.cleanup','Removal of %s exited with code %i'%(fname,ret))
    else:
        PInfo(sname+'.cleanup','Removed '+fname)
    return ret


def hadd(good_inputs):
    good_outputs = ' '.join([x.replace('input','output') for x in good_inputs])
    cmd = 'hadd -f output.root ' + good_outputs
    ret = system(cmd)    
    if not ret:
        PInfo(sname+'.hadd','Merging exited with code %i'%ret)
    else:
        PError(sname+'.hadd','Merging exited with code %i'%ret)


def stageout(infilename,outdir,outfilename):
    if path.isdir(outdir): # assume it's a local copy
        mvargs = 'mv $PWD/%s %s/%s'%(infilename,outdir,outfilename)
        lsargs = 'ls %s/%s'%(outdir,outfilename)
    else:
        if system('which gfal-copy')==0:
            mvargs = 'gfal-copy '
            lsargs = 'gfal-ls '
        elif system('which lcg-cp')==0:
            mvargs = 'lcg-cp -v -D srmv2 -b '
            lsargs = 'lcg-ls -v -D srmv2 -b '
        else:
            PError(sname+'.stageout','Could not find a stageout protocol!')
            return -1
        mvargs += 'file://$PWD/%s srm://t3serv006.mit.edu:8443/srm/v2/server?SFN=%s/%s'%(infilename,outdir,outfilename)
        lsargs += 'srm://t3serv006.mit.edu:8443/srm/v2/server?SFN=%s/%s'%(outdir,outfilename)
    PInfo(sname+'.stageout',mvargs)
    ret = system(mvargs)
    if not ret:
        PInfo(sname+'.stageout','Move exited with code %i'%ret)
    else:
        PError(sname+'.stageout','Move exited with code %i'%ret)
        return ret
    PInfo(sname+'.stageout',lsargs)
    ret = system(lsargs)
    if ret: 
        PError(sname+'.stageout','Output file is missing!')
        return ret
    return 0


def write_lock(outdir,outfilename,processed):
    lockname = outfilename.replace('.root','.lock')
    flock = open(lockname,'w')
    for k,v in processed.iteritems():
        flock.write(v+'\n')
    PInfo(sname+'.write_lock','This job successfully processed %i inputs!'%len(processed))
    flock.close()
    return stageout(lockname,outdir+'/locks/',lockname)


if __name__ == "__main__":
    sample_list = cb.read_sample_config('local.cfg',as_dict=False)
    to_run = None #sample_list[which]
    for s in sample_list:
        if which==s.get_id():
            to_run = s
            break
    if not to_run:
        PError(sname,'Could not find a job for PROCID=%i'%(which))
        exit(3)

    outdir = 'XXXX' # will be replaced when building the job
    outfilename = to_run.name+'_%i.root'%(submit_id)
    processed = {}
    
    print_time('loading')
    for f in to_run.files:
        input_name = copy_local(f)
        print_time('copy %s'%input_name)
        if input_name:
            success = fn(input_name,(to_run.dtype!='MC'),f)
            print_time('analyze %s'%input_name)
            if success:
                processed[input_name] = f
            cleanup(input_name)
            print_time('remove %s'%input_name)
    
    if len(processed)==0:
        exit(1)

    hadd(list(processed))
    print_time('hadd')

    ci.process_file('output.root','output')
    print_time('conversion')

    ret1 = stageout('output.root',outdir,outfilename)
    ret2 = stageout('output_gen.npy',outdir,outfilename.replace('.root','_gen.npy'))
    ret3 = stageout('output_truth.npy',outdir,outfilename.replace('.root','_truth.npy'))
    print_time('stageout')

    system('rm -f *root *npy')
    
    if not any([ret1, ret2, ret3]):
        write_lock(outdir,outfilename,processed)
        print_time('create lock')
    else:
        exit(-1*max([ret1, ret2, ret3]))

    exit(0)
