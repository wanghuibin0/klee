#ifndef KLEE_SUMMARY_H
#define KLEE_SUMMARY_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/Constraints.h"
#include "llvm/IR/Function.h"

#include <vector>
#include <map>

namespace klee {

class PathSummary {
  ConstraintSet preCond;
  bool isVoidRet;
  ref<Expr> retVal;
  std::map<llvm::GlobalValue*, ref<Expr>> globalsMod;

public:
  PathSummary() = default;
  void setPreCond(const ConstraintSet &cs) { preCond = cs; }
  void markVoidRet(bool isVoidRet) { this->isVoidRet = isVoidRet; }
  void setRetValue(ref<Expr> ret) {
    assert(isVoidRet == false);
    retVal = ret;
  }
  void addGlobalsModified(llvm::GlobalValue *gv, ref<Expr> val) {
    globalsMod.insert(std::make_pair(gv, val));
  }
};

class Summary {
  llvm::Function *function;
  std::vector<ConstraintSet *> context;
  std::vector<ref<Expr>> args;
  std::vector<PathSummary *> pathSummaries;

public:
  Summary(llvm::Function *f) : function(f) {}
  void addPathSummary(PathSummary *ps) { pathSummaries.push_back(ps); }
  void addContext(ConstraintSet &c) { context.push_back(&c); }
  void addArg(ref<Expr> arg) { args.push_back(arg); }
  llvm::Function *getFunction() { return function; }
};

} // namespace klee

#endif
