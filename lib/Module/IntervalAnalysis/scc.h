// date created:11/26/2015
// author: morgan
// encapsulation of llvm SCC

#ifndef SCC_H
#define SCC_H

#include "cfg.h"
#include "headers.h"

class NewScc {
private:
  vector<NewBasicBlock *> nbbs;
  NewBasicBlock *nhead;

public:
  NewScc(vector<NewBasicBlock *> nbbs) : nbbs(nbbs) {}

  vector<NewBasicBlock *> getScc() { return nbbs; }

  int size() { return nbbs.size(); }
  NewBasicBlock *back() { return nbbs.back(); }

  NewBasicBlock *getHead() { return nbbs.back(); } // the last element
};

#endif
