#ifndef SUMMARY_H
#define SUMMARY_H

#include "klee/Expr/Constraints.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Function.h"

namespace llvm {
}

namespace klee {

class NormalPathSummary {
  ConstraintSet preCond;
  bool isVoidRet;
  ref<Expr> retVal;
  std::map<const llvm::GlobalValue *, ref<Expr>> globalsMod;

public:
  NormalPathSummary() = default;
  void setPreCond(const ConstraintSet &cs) { preCond = cs; }
  void markVoidRet(bool isVoidRet) { this->isVoidRet = isVoidRet; }
  void setRetValue(ref<Expr> ret) {
    assert(isVoidRet == false);
    retVal = ret;
  }
  void addGlobalsModified(const llvm::GlobalValue *gv, ref<Expr> val) {
    globalsMod.insert(std::make_pair(gv, val));
  }

  ConstraintSet getPreCond() const { return preCond; }
  bool getIsVoidRet() const { return isVoidRet; }
  ref<Expr> getRetVal() const { return retVal; }
  std::map<const llvm::GlobalValue *, ref<Expr>> getGlobalsMod() const {
    return globalsMod;
  }

  void dump() const {
    llvm::outs() << "this is a normal path summary\n";
    llvm::outs() << "pre conditions are:\n";
    for (auto x : preCond) {
      x->dump();
    }
    llvm::outs() << "is void return? " << isVoidRet << "\n";
    if (!isVoidRet) {
      llvm::outs() << "retVal = ";
      retVal->dump();
    }
    llvm::outs() << "globals modified:\n";
    for (auto x : globalsMod) {
      llvm::outs() << x.first->getName() << ": ";
      x.second->dump();
    }
  }
};

enum ErrorReason {
  Abort,
  Assert,
  BadVectorAccess,
  Exec,
  External,
  Free,
  Model,
  Overflow,
  Ptr,
  ReadOnly,
  ReportError,
  User,
  UncaughtException,
  UnexpectedException,
  Unhandled,
};

class ErrorPathSummary {
  ConstraintSet preCond;
  enum ErrorReason tr;

public:
  ErrorPathSummary(ConstraintSet &preCond, enum ErrorReason tr)
      : preCond(preCond), tr(tr) {}
  ConstraintSet getPreCond() const { return preCond; }
  ErrorReason getTerminateReason() const { return tr; }
  void dump() {
    llvm::outs() << "this is an error path summary\n";
    llvm::outs() << "pre conditions are:\n";
    for (auto x : preCond) {
      x->dump();
    }
    llvm::outs() << "terminate reason is: " << tr << "\n";
  }
};

class Summary {
  llvm::Function *function;
  ref<Expr> context;
  std::vector<ref<Expr>> args;
  std::map<llvm::GlobalValue *, ref<Expr>> globals;
  std::vector<NormalPathSummary *> normalPathSummaries;
  std::vector<ErrorPathSummary *> errorPathSummaries;

public:
  Summary(llvm::Function *f) : function(f) {}

  void addNormalPathSummary(NormalPathSummary *ps) {
    normalPathSummaries.push_back(ps);
  }
  void addErrorPathSummary(ErrorPathSummary *ps) {
    errorPathSummaries.push_back(ps);
  }
  void addContext(ConstraintSet &c) {
    ref<Expr> aContext = ConstantExpr::create(1, Expr::Bool);
    for (auto &x : c) {
      aContext = AndExpr::create(aContext, x);
    }
    context = OrExpr::create(context, aContext);
  }
  void setContext(ref<Expr> c) { context = c; }
  void addArg(llvm::Value *farg, ref<Expr> arg) { args.push_back(arg); }

  llvm::Function *getFunction() const { return function; }
  ref<Expr> getContext() const { return context; }
  std::vector<ref<Expr>> getFormalArgs() const { return args; }
  std::vector<NormalPathSummary *> getNormalPathSummaries() const {
    return normalPathSummaries;
  }
  const std::vector<ErrorPathSummary *> &getErrorPathSummaries() const {
    return errorPathSummaries;
  }
  const std::map<llvm::GlobalValue *, ref<Expr>> &getFormalGlobals() const {
    return globals;
  }

  void dump() {
    llvm::outs() << "function name: " << function->getName() << "\n";
    llvm::outs() << "context is: ";
    context->dump();
    llvm::outs() << "arguments are: \n";
    for (auto a : args) {
      a->dump();
      llvm::outs() << "; ";
    }
    llvm::outs() << "globals: \n";
    for (auto g : globals) {
      llvm::outs() << "key: " << g.first->getName() << "\n";
      llvm::outs() << "val: ";
      g.second->dump();
      llvm::outs() << "; ";
    }

    for (auto nps : normalPathSummaries) {
      nps->dump();
    }
    for (auto eps : errorPathSummaries) {
      eps->dump();
    }
  }
};

} // namespace klee

#endif
