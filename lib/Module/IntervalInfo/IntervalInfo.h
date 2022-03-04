#ifndef KLEE_INTERVALINFO_H
#define KLEE_INTERVALINFO_H

#include "klee/Config/Version.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"

namespace klee {
class Env;

class IntervalInfoPass : public llvm::FunctionPass {
private:
public:
  static char ID;
  IntervalInfoPass() : llvm::FunctionPass(ID) {}
  bool runOnFunction(llvm::Function &f) override;

private:
  void initializeEntryInfo(llvm::Function &f, llvm::BasicBlock &b);
};

class IntervalCtxPass : public llvm::ModulePass {
public:
  static char ID;
  IntervalCtxPass();
  bool runOnModule(llvm::Module &M) override;

private:
  using CallSitesMap =
      std::map<llvm::Function *, std::vector<llvm::Instruction *>>;
  using LoopInfoMap = std::map<llvm::Function *, std::unique_ptr<llvm::LoopInfo>>;
  void computeCallSitesMap(CallSitesMap &cg, llvm::Module &M);
  Env getArgsEnv(llvm::CallInst *ci);
  Env getEdgeArgsEnv(llvm::BasicBlock *from, llvm::BasicBlock *to,
                     llvm::CallInst *ci);
  void updateFunctionContext(llvm::Function *f, Env &argEnv);
private:
  LoopInfoMap loopInfos;
};

} // namespace klee

#endif
