import ROOT as root
import numpy as np

_c = root.TCanvas()

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

def process_tree(t, fpath, n_to_print=0, do_truth=False):
    N = t.GetEntriesFast()
    imgpath = fpath.replace('.npy','_img%i.png')
    gen_arrs = []
    dims = None
    for iE in xrange(N):
        t.GetEntry(iE)
        gen = t.gen
        if n_to_print>0 and iE<n_to_print:
            img(gen, imgpath%iE)
        if not dims:
            dims = (gen.GetNbinsX(), gen.GetNbinsY())
        gen_arrs.append( conv(gen, dims) )
    gen_arr = np.array(gen_arrs)
    np.save(fpath, gen_arr)


if __name__=='__main__':
    from sys import argv
    f = root.TFile(argv[1])
    t = f.Get('album')
    fout = argv[1].replace('.root','.npy')
    process_tree(t, fout, 10)
