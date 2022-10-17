#include "klee/Core/SummaryManager.h"
#include "CSExecutor.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "klee/Support/Debug.h"

using namespace llvm;
using namespace klee;

namespace klee {
cl::OptionCategory
    CompExCat("Compositional execution specific options",
              "These are Compositional execution specific options.");
cl::opt<InterpreterType> InterpreterToUse(
    "cse", cl::desc("Specify the CSExecutor to use"),
    cl::values(
        clEnumValN(BUCSE, "bucse",
                   "bottom-up compositional symbolic executor (default)"),
        clEnumValN(TDCSE, "tdcse", "top-down compositional symbolic executor"),
        clEnumValN(CTXCSE, "ctxcse",
                   "context-aware compositional symbolic executor"),
        clEnumValN(NOCSE, "none",
                   "do not compositional, only normal symbolic executor")
            KLEE_LLVM_CL_VAL_END),
    cl::init(TDCSE), cl::cat(CompExCat));
} // namespace klee

SummaryManager *SummaryManager::createSummaryManager() {
  if (InterpreterToUse == CTXCSE) {
    /* llvm::errs() << "creating ctxcse summary manager.\n"; */
    klee_message("creating ctxcse summary manager.\n");
    return new CTXCSESummaryManager();
  } else if (InterpreterToUse == TDCSE) {
    return new TDCSESummaryManager();
  } else if (InterpreterToUse == BUCSE) {
    /* llvm::errs() << "creating bucse summary manager.\n"; */
    klee_message("creating bucse summary manager.\n");
    return new BUCSESummaryManager();
  } else {
    return nullptr;
  }
}

Summary *SummaryManager::getSummary(llvm::Function *f) {
  if (summaryLib.count(f)) {
    return summaryLib[f].get();
  } else {
    return nullptr;
  }
}

void SummaryManager::setProto(Executor &e) { proto.reset(new Executor(e)); }

void SummaryManager::dump() {
  llvm::errs() << "in SummaryManager::dump()\n";
  for (const auto &x : summaryLib) {
    llvm::errs() << "summary for function: " << x.first->getName() << "\n";
    x.second->dump();
  }
}

Summary *CTXCSESummaryManager::computeSummary(ExecutionState &es,
                                              llvm::Function *f) {
  CTXCSExecutor *executor = new CTXCSExecutor(*proto, f);
  executor->run();
  std::unique_ptr<Summary> sum = executor->extractSummary();
  Summary *ret = sum.get();
  summaryLib[f] = std::move(sum);
  delete executor;
  return ret;
}

Summary *TDCSESummaryManager::computeSummary(ExecutionState &es,
                                             llvm::Function *f) {
  TDCSExecutor *executor = new TDCSExecutor(*proto, f);
  executor->run();
  std::unique_ptr<Summary> sum = executor->extractSummary();
  Summary *ret = sum.get();
  summaryLib[f] = std::move(sum);
  delete executor;
  return ret;
}

Summary *BUCSESummaryManager::computeSummary(ExecutionState &es,
                                             llvm::Function *f) {
  BUCSExecutor *executor = new BUCSExecutor(*proto, f);
  executor->run();
  std::unique_ptr<Summary> sum = executor->extractSummary();
  Summary *ret = sum.get();
  summaryLib[f] = std::move(sum);
  delete executor;
  return ret;
}
