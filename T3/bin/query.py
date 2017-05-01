import cPickle as pickle
from os import getenv

try:
      l = pickle.load(open(getenv('SUBMIT_WORKDIR')+'/submission.pkl'))
      s = l[-1]
      print 'ClusterID',s.cluster_id
      statii = s.query_status()
      print 'Job summary:'
      for k,v in statii.iteritems():
            print '\t %10s : %5i'%(k,len(v))
except IOError:
      print 'No job submitted yet!'
