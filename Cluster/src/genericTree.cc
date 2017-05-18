#include "../interface/genericTree.h"
#include "TRegexp.h"

void
redpanda::genericTree::RemoveBranches(std::vector<TString> droppable, 
                            std::vector<TString> keeppable)
{

  for (auto &s : droppable)
    r_droppable.emplace_back(s);
  for (auto &s : keeppable)
    r_keeppable.emplace_back(s);


}

bool
redpanda::genericTree::Book(TString bname, void *address, TString leaf)
{

  if (!treePtr)
    return false;

  bool mustKeep = false; // if there's an override
  for (auto &r : r_keeppable) {
    if (bname.Contains(r)) {
      mustKeep = true;
      break;
    }
  }

  if (!mustKeep) {
    for (auto &r : r_droppable) {
      if (bname.Contains(r)) 
        return false;
    }
  }

  treePtr->Branch(bname,address,leaf);
  return true;

}

bool
redpanda::genericTree::BookTObject(TString bname, TString cname, TObject *address)
{

  if (!treePtr)
    return false;

  bool mustKeep = false; // if there's an override
  for (auto &r : r_keeppable) {
    if (bname.Contains(r)) {
      mustKeep = true;
      break;
    }
  }

  if (!mustKeep) {
    for (auto &r : r_droppable) {
      if (bname.Contains(r)) 
        return false;
    }
  }

  treePtr->Branch(bname,cname,address);
  return true;

}
