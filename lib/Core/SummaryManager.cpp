#include "klee/Core/SummaryManager.h"
#include "CSExecutor.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Function.h"

#include <cassert>

using namespace llvm;
using namespace klee;

namespace klee {
cl::OptionCategory InterpreterCat("Interpreter type options",
                                  "These specify the mode of the interpreter.");
cl::opt<InterpreterType> InterpreterToUse(
    "cse", cl::desc("Specify the CSExecutor to use"),
    cl::values(
        clEnumValN(BUCSE, "bucse", "bottom-up compositional symbolic executor"),
        clEnumValN(TDCSE, "tdcse", "top-down compositional symbolic executor"),
        clEnumValN(CTXCSE, "ctxcse",
                   "context-aware compositional symbolic executor"),
        clEnumValN(
            NOCSE, "none",
            "do not compositional, only normal symbolic executor(default)")
            KLEE_LLVM_CL_VAL_END),
    cl::init(NOCSE), cl::cat(InterpreterCat));
} // namespace klee

SummaryManager *SummaryManager::createSummaryManager(const Executor &mainExecutor) {
  if (InterpreterToUse == BUCSE) {
    llvm::errs() << "creating bucse summary manager.\n";
    return new BUCSESummaryManager(mainExecutor);
  } else {
    return nullptr;
  }
}

Summary *BUCSESummaryManager::getSummary(ExecutionState &es, llvm::Function *f) {
  llvm::errs() << "getting summary for function " << f->getName() << "\n";
  if (summaryLib.find(f) == summaryLib.end()) {
    // summary does not exist, try to compute.
    llvm::errs() << "summary does not exist, try to compute.\n";
    Summary *sum = computeSummary(f);
    summaryLib.insert(std::make_pair(f, sum));
    return sum;
  } else {
    llvm::errs() << "summary exists, reusing.\n";
    return summaryLib[f];
  }
}

Summary *BUCSESummaryManager::computeSummary(llvm::Function *f) {
  // TODO: placeholder to be filled
  BUCSExecutor *executor = createBUCSExecutor(f);
  executor->run();
  Summary *sum = executor->extractSummary();
  // TODO: should be deleted, in case of memory leak.
  // but it can cause another problem, so leave it temporarily.
  // delete executor;
  return sum;
}

BUCSExecutor *BUCSESummaryManager::createBUCSExecutor(llvm::Function *f) {
  llvm::errs() << "creating a bucse executor\n";
  auto e = new BUCSExecutor(proto, f);
//  llvm::errs() << "proto->summaryManager = " << proto.getSummaryManager() << "\n";
//  llvm::errs() << "e->summaryManager = " << e->getSummaryManager() << "\n";
  return e;
}
