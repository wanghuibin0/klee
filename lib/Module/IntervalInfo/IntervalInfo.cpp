#include "MyDebug.h"

#include "Env.h"
#include "EnvMap.h"
#include "Interval.h"
#include "IntervalInfo.h"
#include "Transfer.h"

#include "klee/Config/Version.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <deque>

using namespace llvm;

namespace klee {

EnvMap &getGlobalEnvMap() {
  static EnvMap gEnvMap;
  return gEnvMap;
}

std::map<Function *, Env> &getGlobalEnvContext() {
  static std::map<Function *, Env> gEnvContext;
  return gEnvContext;
}

void dump(std::map<Function *, Env> &contexMap) {
  for (auto &&it : contexMap) {
    outs() << "function: " << it.first << ", Env:" << it.second;
  }
}

// initialize env of entry to Top
// initialize all arguments to Top
void IntervalInfoPass::initializeEntryInfo(Function &f, BasicBlock &entry) {
  getGlobalEnvMap().update(nullptr, &entry, Env());

  for (auto &arg : f.args()) {
    MY_KLEE_DEBUG(outs() << "processing args: " << arg << "\n");
    llvm::Type *ty = arg.getType();
    if (isa<llvm::IntegerType>(ty)) {
      MY_KLEE_DEBUG(outs() << "this is an integer type\n");
      getGlobalEnvMap().getEnv(nullptr, &entry).set(&arg, Interval::Top());
    }
  }
}

bool IntervalInfoPass::runOnFunction(Function &f) {
  llvm::outs() << "this is the start of loop info computing\n";
  outs() << f;
  llvm::FunctionAnalysisManager FAM;
  FAM.registerPass([&] { return LoopAnalysis(); });
  FAM.registerPass([&] { return DominatorTreeAnalysis(); });
  llvm::LoopInfo LI{std::move(FAM.getResult<LoopAnalysis>(f))};
  LI.print(llvm::outs());
  llvm::outs() << "this is the end of loop info computing\n";

  MY_KLEE_DEBUG(llvm::outs()
                << "computing interval info for " << f.getName() << "\n");
  // initialize env of all blocks
  // add entry block to worklist
  BasicBlock &entry = f.getEntryBlock();
  initializeEntryInfo(f, entry);

  std::deque<BasicBlock *> worklist;
  worklist.push_back(&entry);

  while (!worklist.empty()) {
    auto bb = worklist.front();
    worklist.pop_front();
    MY_KLEE_DEBUG(outs() << "processing this basicblock: " << bb->getName()
                         << "\n");
    MY_KLEE_DEBUG(bb->print(outs()));

    std::map<BasicBlock *, Env> oldOut;
    for (auto it : successors(bb)) {
      oldOut[&*it] = getGlobalEnvMap().getEnv(bb, &*it);
    }

    Transfer trans(bb, getGlobalEnvMap(), LI);

    trans.execute(); // TODO: widening

    for (auto it : successors(bb)) {
      auto newOut = getGlobalEnvMap().getEnv(bb, &*it);
      if (oldOut[&*it] != newOut) {
        worklist.push_back(&*it);
      }
    }
  }

  MY_KLEE_DEBUG(getGlobalEnvMap().dump());

  return false;
}

char IntervalInfoPass::ID = 0;

IntervalCtxPass::IntervalCtxPass() : llvm::ModulePass(ID) {}

bool IntervalCtxPass::runOnModule(llvm::Module &M) {
  // first compute loop info
  outs() << M;
  llvm::FunctionAnalysisManager FAM;
  FAM.registerPass([&] { return LoopAnalysis(); });
  FAM.registerPass([&] { return DominatorTreeAnalysis(); });
  for (auto &&f : M) {
    outs() << f;
    if (f.isIntrinsic() || f.empty()) {
      continue;
    }
    llvm::outs() << "this is the start of loop info computing\n";
    std::unique_ptr<LoopInfo> li(
        new LoopInfo(std::move(FAM.getResult<LoopAnalysis>(f))));
    loopInfos.insert({&f, std::move(li)});
    llvm::outs() << "this is the end of loop info computing\n";
  }

  // compute call site map
  CallSitesMap callSitesMap;
  computeCallSitesMap(callSitesMap, M);

  for (auto &&f : M) {
    if (f.getName() == "main")
      continue;
    if (f.empty())
      continue;

    MY_KLEE_DEBUG(outs() << "IntervalCtxPass: processing " << f.getName()
                         << "\n");
    std::vector<llvm::Instruction *> cs = callSitesMap[&f];

    // merge env of all call sites
    Env argEnv;
    for (auto &&c : cs) {
      assert(llvm::isa<CallInst>(c) && "must be a call instruction");
      argEnv = argEnv | getArgsEnv(llvm::cast<CallInst>(c));
    }

    // update global context store
    updateFunctionContext(&f, argEnv);
  }
  klee::dump(getGlobalEnvContext());
  return false;
}

void IntervalCtxPass::computeCallSitesMap(CallSitesMap &cg, llvm::Module &M) {
  for (auto &&F : M) {
    for (auto &&BB : F) {
      for (auto &&I : BB) {
        if (auto CS = CallSite(&I)) {
          Function *Callee = CS.getCalledFunction();
          if (Callee && !Callee->isIntrinsic() && !Callee->empty()) {
            cg[Callee].push_back(&I);
          }
        }
      }
    }
  }
}

Env IntervalCtxPass::getArgsEnv(llvm::CallInst *ci) {
  llvm::BasicBlock *BB = ci->getParent();
  llvm::Function *f = BB->getParent();

  Transfer trans(BB, getGlobalEnvMap(), *loopInfos[f]);
  Env callingEnv = trans.executeToInst(ci);
  callingEnv.dump(outs());
  if (callingEnv.empty()) {
    return Env();
  }

  Env res;
  llvm::Function *called = ci->getCalledFunction();
  auto i = 0;
  auto it = called->arg_begin();
  for (; it != called->arg_end(); ++i, ++it) {
    llvm::Value *aa = ci->getOperand(i); // actual arg
    MY_KLEE_DEBUG(outs() << "actual value: " << *aa << "\n");
    MY_KLEE_DEBUG(callingEnv.dump(outs()));
    assert(callingEnv.hasValue(aa));
    Interval i = callingEnv.lookup(aa);
    llvm::Value *fa = &*it; // formal arg
    res.set(fa, i);
  }

  return res;
}

void IntervalCtxPass::updateFunctionContext(llvm::Function *f, Env &argEnv) {
  getGlobalEnvContext().insert(std::make_pair(f, argEnv));
}
char IntervalCtxPass::ID = 0;

} // namespace klee
