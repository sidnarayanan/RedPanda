#!/usr/bin/env python

from sys import argv
from os import system,getenv,getuid
from time import time
import PandaCore.Tools.job_management as jm 


logpath=getenv('SUBMIT_LOGDIR')
workpath=getenv('SUBMIT_WORKDIR')
cfgpath = workpath+'/local.cfg'

now = int(time())
frozen_cfgpath = cfgpath.replace('local','local_%i'%now)
system('cp %s %s'%(cfgpath,frozen_cfgpath)) 

if getenv('SUBMIT_CONFIG'):
  jm.setup_schedd(getenv('SUBMIT_CONFIG'))

s = jm.Submission(frozen_cfgpath,workpath+'/submission.pkl')
s.execute()
s.save()

statii = s.query_status()
print 'Job summary:'
for k,v in statii.iteritems():
  print '\t %10s : %5i'%(k,len(v))
