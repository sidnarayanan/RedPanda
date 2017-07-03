import ROOT as root
import numpy as np
from PandaCore.Tools.Misc import PInfo
from PandaCore.Tools.root_interface import read_tree

def convert(tree, branches = ['px','py','pz','e','ptype'], astype=np.float16):
    struct_arr = read_tree(tree, branches)
    arr = np.stack([struct_arr[f].astype(astype) for f in branches], axis=1).transpose((0, 2, 1))
    return arr


def process_file(infile, *args, **kwargs):
    f = root.TFile(infile)
    t = f.Get('jets')
    arr = convert(t, *args, **kwargs)
    np.save(infile.replace('.root','.npy'), arr)


if __name__=='__main__':
    from sys import argv
    process_file(argv[1])
