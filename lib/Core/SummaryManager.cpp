#include "klee/Core/SummaryManager.h"
#include "CSExecutor.h"
#include "ConcreteSummaryManager.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include "klee/Support/Debug.h"

using namespace llvm;
using namespace klee;

namespace klee {
cl::OptionCategory CompExCat("Compositional execution specific options",
                                  "These are Compositional execution specific options.");
cl::opt<InterpreterType> InterpreterToUse(
    "cse", cl::desc("Specify the CSExecutor to use"),
    cl::values(
        clEnumValN(BUCSE, "bucse", "bottom-up compositional symbolic executor (default)"),
        clEnumValN(TDCSE, "tdcse", "top-down compositional symbolic executor"),
        clEnumValN(CTXCSE, "ctxcse",
                   "context-aware compositional symbolic executor"),
        clEnumValN(
            NOCSE, "none",
            "do not compositional, only normal symbolic executor")
            KLEE_LLVM_CL_VAL_END),
    cl::init(BUCSE), cl::cat(CompExCat));
} // namespace klee

SummaryManager *SummaryManager::createSummaryManager() {
  if (InterpreterToUse == CTXCSE) {
    /* llvm::errs() << "creating ctxcse summary manager.\n"; */
    klee_message("creating ctxcse summary manager.\n");
    return new CTXCSESummaryManager();
  } else if (InterpreterToUse == BUCSE) {
    /* llvm::errs() << "creating bucse summary manager.\n"; */
    klee_message("creating bucse summary manager.\n");
    return new BUCSESummaryManager();
  } else {
    return nullptr;
  }
}

void SummaryManager::setProto(Executor &e) { proto.reset(new Executor(e)); }

Summary *CTXCSESummaryManager::getSummary(ExecutionState &es,
                                          llvm::Function *f) {
  KLEE_DEBUG_WITH_TYPE("cse", klee_message("getting summary for function %s\n",
                                           f->getName().data()););
  if (summaryLib.find(f) == summaryLib.end()) {
    // summary does not exist, try to compute.
    /* llvm::errs() << "summary does not exist, try to compute.\n"; */
    KLEE_DEBUG_WITH_TYPE("cse",
                         klee_message("summary does not exist, try to compute "
                                      "it by a ctxcse executor.\n"););
    std::unique_ptr<Summary> sum = computeSummary(f);
    Summary *res = sum.get();
    summaryLib.insert(std::make_pair(f, std::move(sum)));
    return res;
  } else {
    /* llvm::errs() << "summary exists, reusing.\n"; */
    KLEE_DEBUG_WITH_TYPE("cse", klee_message("summary exists, reusing.\n"););
    return summaryLib[f].get();
  }
}

std::unique_ptr<Summary>
CTXCSESummaryManager::computeSummary(llvm::Function *f) {
  CTXCSExecutor *executor = new CTXCSExecutor(*proto, f);
  executor->run();
  std::unique_ptr<Summary> sum = executor->extractSummary();
  delete executor;
  return sum;
}

Summary *BUCSESummaryManager::getSummary(ExecutionState &es,
                                         llvm::Function *f) {
  /* llvm::errs() << "getting summary for function " << f->getName() << "\n"; */
  KLEE_DEBUG_WITH_TYPE("cse", klee_message("getting summary for function %s\n",
                                           f->getName().data()););
  if (summaryLib.find(f) == summaryLib.end()) {
    // summary does not exist, try to compute.
    KLEE_DEBUG_WITH_TYPE("cse",
                         klee_message("summary does not exist, try to compute "
                                      "it by a bucse executor.\n"););
    std::unique_ptr<Summary> sum = computeSummary(f);
    Summary *res = sum.get();
    summaryLib.insert(std::make_pair(f, std::move(sum)));
    return res;
  } else {
    KLEE_DEBUG_WITH_TYPE("cse", klee_message("summary exists, reusing.\n"););
    return summaryLib[f].get();
  }
}

std::unique_ptr<Summary>
BUCSESummaryManager::computeSummary(llvm::Function *f) {
  BUCSExecutor *executor = new BUCSExecutor(*proto, f);
  executor->run();
  std::unique_ptr<Summary> sum = executor->extractSummary();
  delete executor;
  return sum;
}

void BUCSESummaryManager::dump() {
  llvm::errs() << "in CTXCSESummaryManager::dump()";
  for (const auto &x : summaryLib) {
    llvm::errs() << "summary for function: " << x.first->getName() << "\n";
    x.second->dump();
  }
}

void CTXCSESummaryManager::dump() {
  llvm::errs() << "in CTXCSESummaryManager::dump()";
}
