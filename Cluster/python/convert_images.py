import ROOT as root
import numpy as np

_c = root.TCanvas('c','',600,600)

def conv(h, dims, dtype=np.float32):
    arr = h.GetArray()
    flat_dim = reduce(lambda x, y : x*y, [x+2 for x in dims])
    narr = np.ndarray( (flat_dim,), dtype=dtype, buffer=arr)
    narr = np.transpose(narr.reshape(tuple(x+2 for x in dims), order='C'), tuple(range(len(dims))[::-1]))
    if len(dims)==1: # is there a better way than hardcoding?
        narr = narr[1:-1]
    elif len(dims)==2:
        narr = narr[1:-1, 1:-1]
    elif len(dims)==3:
        narr = narr[1:-1, 1:-1, 1:-1]
    return conv

def img(h, fpath):
    _c.Clear()
    _c.cd()
    h.SetStats(0)
    h.Draw('colz')
    if type(fpath)==str:
        _c.SaveAs(fpath)
    elif type(fpath)==list:
        for f in fpath:
            _c.SaveAs(f)

def process_tree(t, fpath, n_to_print=0, do_truth=True):
    N = t.GetEntriesFast()
    imgpath = fpath+'_img%i.png'
    gen_arrs = []
    truth_arrs = []
    dims = None
    for iE in xrange(N):
        t.GetEntry(iE)
        gen = t.gen
        truth = t.truth
        if n_to_print>0 and iE<n_to_print:
            img(gen, imgpath%iE)
            img(truth, (imgpath%iE).replace('.png','_truth.png'))
        if not dims:
            dims = (gen.GetNbinsX(), gen.GetNbinsY())
        gen_arrs.append( conv(gen, dims) )
        truth_arrs.append( conv(truth, dims, dtype=np.int16) )
    gen_arr = np.array(gen_arrs)
    np.save(fpath+'_gen.npy', gen_arr)
    truth_arr = np.array(truth_arrs)
    np.save(fpath+'_truth.npy', truth_arr)

def process_file(infile, *args, **kwargs):
    f = root.TFile(infile)
    t = f.Get('album')
    process_tree(t, *args, **kwargs)


if __name__=='__main__':
    from sys import argv
    f = root.TFile(argv[1])
    t = f.Get('album')
    fout = argv[1].replace('.root','')
    process_tree(t, fout, 10)
