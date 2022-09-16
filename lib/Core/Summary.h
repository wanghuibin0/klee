#ifndef SUMMARY_H
#define SUMMARY_H

#include "klee/Expr/Constraints.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"

namespace llvm {}

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
    llvm::errs() << "this is a normal path summary\n";
    llvm::errs() << "pre conditions are:\n";
    for (auto x : preCond) {
      x->dump();
    }
    llvm::errs() << "is void return? " << isVoidRet << "\n";
    if (!isVoidRet) {
      llvm::errs() << "retVal = ";
      retVal->dump();
    }
    llvm::errs() << "globals modified:\n";
    for (auto x : globalsMod) {
      llvm::errs() << x.first->getName() << ": ";
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
  Exit, // added because we want to capture `exit` in CSE.
};

class ErrorPathSummary {
  ConstraintSet preCond;
  enum ErrorReason tr;
  std::map<const llvm::GlobalValue *, ref<Expr>> globalsMod;

public:
  ErrorPathSummary(ConstraintSet &preCond, enum ErrorReason tr)
      : preCond(preCond), tr(tr) {}
  ConstraintSet getPreCond() const { return preCond; }
  ErrorReason getTerminateReason() const { return tr; }
  void dump() {
    llvm::errs() << "this is an error path summary\n";
    llvm::errs() << "pre conditions are:\n";
    for (auto x : preCond) {
      x->dump();
    }
    llvm::errs() << "terminate reason is: " << tr << "\n";
    llvm::errs() << "globals modified:\n";
    for (auto x : globalsMod) {
      llvm::errs() << x.first->getName() << ": ";
      x.second->dump();
    }
  }
};

class Summary {
  llvm::Function *function;
  ref<Expr> context;
  std::vector<ref<Expr>> args;
  std::map<const llvm::GlobalValue *, ref<Expr>> globals;
  std::vector<NormalPathSummary> normalPathSummaries;
  std::vector<ErrorPathSummary> errorPathSummaries;

public:
  Summary(llvm::Function *f)
      : function(f), context(ConstantExpr::create(1, Expr::Bool)) {}

  void addNormalPathSummary(const NormalPathSummary ps) {
    normalPathSummaries.push_back(ps);
  }
  void addErrorPathSummary(const ErrorPathSummary &ps) {
    errorPathSummaries.push_back(ps);
  }
  void addContext(ConstraintSet &c) {
    ref<Expr> aContext = ConstantExpr::create(1, Expr::Bool);
    for (auto &x : c) {
      aContext = AndExpr::create(aContext, x);
    }
    context = OrExpr::create(context, aContext);
  }
  void addArg(llvm::Value *farg, ref<Expr> arg) { args.push_back(arg); }

  llvm::Function *getFunction() const { return function; }
  ref<Expr> getContext() const { return context; }
  std::vector<ref<Expr>> getFormalArgs() const { return args; }
  std::vector<NormalPathSummary> getNormalPathSummaries() const {
    return normalPathSummaries;
  }
  const std::vector<ErrorPathSummary> &getErrorPathSummaries() const {
    return errorPathSummaries;
  }
  const std::map<const llvm::GlobalValue *, ref<Expr>> &
  getFormalGlobals() const {
    return globals;
  }
  void addFormalGlobals(const llvm::GlobalValue *gv, ref<Expr> e) {
    globals.insert(std::make_pair(gv, e));
  }

  void dump() {
    llvm::errs() << "function name: " << function->getName() << "\n";
    llvm::errs() << "context is: ";
    context->dump();
    llvm::errs() << "arguments are: \n";
    for (auto a : args) {
      a->dump();
      llvm::errs() << "; ";
    }
    llvm::errs() << "globals: \n";
    for (auto g : globals) {
      llvm::errs() << "key: " << g.first->getName() << "\n";
      llvm::errs() << "val: ";
      g.second->dump();
      llvm::errs() << "; ";
    }

    for (auto nps : normalPathSummaries) {
      nps.dump();
    }
    for (auto eps : errorPathSummaries) {
      eps.dump();
    }
  }
};

} // namespace klee

#endif
