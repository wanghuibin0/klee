#include "klee/Core/SummaryManager.h"
#include "CSExecutor.h"
#include "Summary.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"

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

SummaryManager *
SummaryManager::createSummaryManager(const Executor &mainExecutor) {
  if (InterpreterToUse == BUCSE) {
    llvm::outs() << "creating bucse summary manager.\n";
    return new BUCSESummaryManager(mainExecutor);
  } else {
    return nullptr;
  }
}

Summary *BUCSESummaryManager::getSummary(ExecutionState &es,
                                         llvm::Function *f) {
  llvm::outs() << "getting summary for function " << f->getName() << "\n";
  if (summaryLib.find(f) == summaryLib.end()) {
    // summary does not exist, try to compute.
    llvm::outs() << "summary does not exist, try to compute.\n";
    std::unique_ptr<Summary> sum = computeSummary(f);
    summaryLib.insert(std::make_pair(f, std::move(sum)));
    return sum.get();
  } else {
    llvm::outs() << "summary exists, reusing.\n";
    return summaryLib[f].get();
  }
}

std::unique_ptr<Summary> BUCSESummaryManager::computeSummary(llvm::Function *f) {
  BUCSExecutor *executor = new BUCSExecutor(proto, f);
  executor->run();
  std::unique_ptr<Summary> sum = executor->extractSummary();
  delete executor;
  return sum;
}
