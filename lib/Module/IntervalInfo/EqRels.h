#ifndef EQRELS_H
#define EQRELS_H

#include "llvm/IR/Value.h"
#include <map>
#include <vector>

using Pointer = llvm::Value *;
using VRegs = llvm::Value *;
class EqRels {
  std::map<VRegs, std::vector<Pointer>> v2p;
  std::map<Pointer, std::vector<VRegs>> p2v;

public:
  void processLoad(Pointer p, VRegs v) {
    v2p[v].push_back(p);
    p2v[p].push_back(v);
  }

  void processStore(Pointer p, VRegs v) {
    v2p[v].push_back(p);
    p2v[p].clear();
    p2v[p].push_back(v);
  }

  // p2 = bitcast p1
  void processBitCast(Pointer p1, Pointer p2) {
    for (auto &&r : p2v[p1]) {
      v2p[r].push_back(p2);
    }
  }

  std::vector<Pointer> &getRelatedPointers(VRegs v) {
    return v2p[v];
  }

  std::vector<VRegs> &getRelatedVRegs(Pointer p) {
    return p2v[p];
  }

};


#endif
