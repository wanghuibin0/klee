#include "MyDebug.h"

#include "IntervalInfo.h"
#include "Env.h"
#include "EnvMap.h"
#include "Interval.h"
#include "Transfer.h"

#include "klee/Config/Version.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <deque>

using namespace llvm;

namespace klee {

EnvMap &getGlobalEnvMap() {
  static EnvMap gEnvMap;
  return gEnvMap;
}

IntervalInfoPass::IntervalInfoPass() : FunctionPass(ID) {}

IntervalInfoPass::~IntervalInfoPass() {}

// initialize env of entry to Top
// initialize all arguments to Top
void IntervalInfoPass::initializeEntryInfo(Function &f, BasicBlock &entry) {
  getGlobalEnvMap().update(nullptr, &entry, Env());

  for (auto &arg : f.args()) {
    MY_KLEE_DEBUG(outs() << "processing args: " << arg << "\n");
    llvm::Type *ty = arg.getType();
    if (isa<llvm::IntegerType>(ty)) {
      MY_KLEE_DEBUG(outs() << "this is an integer type\n");
      getGlobalEnvMap()
        .getEnv(nullptr, &entry)
        .set(&arg, Interval::Top());
    }
  }
}

bool IntervalInfoPass::runOnFunction(Function &f) {
  llvm::outs() << "this is the start of loop info computing\n";
  llvm::FunctionAnalysisManager FAM;
  FAM.registerPass([&] { return LoopAnalysis(); });
  FAM.registerPass([&] { return DominatorTreeAnalysis(); });
  llvm::LoopInfo LI{std::move(FAM.getResult<LoopAnalysis>(f))};
  LI.print(llvm::outs());
  llvm::outs() << "this is the end of loop info computing\n";

  MY_KLEE_DEBUG(llvm::outs() << "computing interval info for " << f.getName() << "\n");
  // initialize env of all blocks
  // add entry block to worklist
  BasicBlock &entry = f.getEntryBlock();
  initializeEntryInfo(f, entry);

  std::deque<BasicBlock*> worklist;
  worklist.push_back(&entry);

  while (!worklist.empty()) {
    auto bb = worklist.front();
    worklist.pop_front();
    MY_KLEE_DEBUG(outs() << "processing this basicblock: " << bb->getName() << "\n");
    MY_KLEE_DEBUG(bb->print(outs()));

    std::map<BasicBlock*, Env> oldOut;
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

} // namespace klee
