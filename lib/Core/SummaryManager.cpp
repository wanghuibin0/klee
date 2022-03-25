#include "klee/Core/SummaryManager.h"
#include "ConcreteSummaryManager.h"
#include "CSExecutor.h"
#include "Summary.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"

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
  if (InterpreterToUse == CTXCSE) {
    llvm::errs() << "creating ctxcse summary manager.\n";
    return new CTXCSESummaryManager(mainExecutor);
  } else {
    return nullptr;
  }
}

Summary *CTXCSESummaryManager::getSummary(ExecutionState &es,
                                         llvm::Function *f) {
  /* llvm::errs() << "getting summary for function " << f->getName() << "\n"; */
  if (summaryLib.find(f) == summaryLib.end()) {
    // summary does not exist, try to compute.
    /* llvm::errs() << "summary does not exist, try to compute.\n"; */
    std::unique_ptr<Summary> sum = computeSummary(f);
    Summary *res = sum.get();
    summaryLib.insert(std::make_pair(f, std::move(sum)));
    return res;
  } else {
    /* llvm::errs() << "summary exists, reusing.\n"; */
    return summaryLib[f].get();
  }
}

std::unique_ptr<Summary> CTXCSESummaryManager::computeSummary(llvm::Function *f) {
  CTXCSExecutor *executor = new CTXCSExecutor(proto, f);
  executor->run();
  std::unique_ptr<Summary> sum = executor->extractSummary();
  delete executor;
  return sum;
}

