#include "klee/Core/InterpreterManager.h"
#include "CSExecutor.h"

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

Summary *BUCSEInterpreterManager::getSummary(llvm::Function *f) {
  if (summaryLib.find(f) == summaryLib.end()) {
    // summary does not exist, try to compute.
    Summary *sum = computeSummary(f);
    putSummaryToLib(f, sum);
    return sum;
  } else {
    return summaryLib[f];
  }
}

void BUCSEInterpreterManager::putSummaryToLib(llvm::Function *f,
                                              Summary *summary) {
  assert(summaryLib.find(f) == summaryLib.end());
  summaryLib.insert(std::make_pair(f, summary));
}

Summary *BUCSEInterpreterManager::computeSummary(llvm::Function *f) {
  // TODO: placeholder to be filled
  return nullptr;
  CSExecutor *executor = createCSExecutor(f);
  executor->run();
  Summary *sum = executor->extractSummary();
  return sum;
}

CSExecutor *BUCSEInterpreterManager::createCSExecutor(llvm::Function *f) {
  return new CSExecutor(proto);
}