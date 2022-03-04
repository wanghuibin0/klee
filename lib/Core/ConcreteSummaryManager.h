#ifndef KLEE_CONCRETESUMMARYMANAGER_H
#define KLEE_CONCRETESUMMARYMANAGER_H

#include "Executor.h"
#include "Summary.h"
#include "klee/Core/SummaryManager.h"

#include <map>

namespace klee {

enum InterpreterType { BUCSE, TDCSE, CTXCSE, NOCSE };

extern llvm::cl::opt<InterpreterType> InterpreterToUse;

class BUCSESummaryManager : public SummaryManager {
public:
  // ctors
  BUCSESummaryManager(const Executor &mainExecutor)
      : summaryLib(), proto(mainExecutor) {
    proto.setSummaryManager(this);
  }

  Summary *getSummary(ExecutionState &es, llvm::Function *f) override;

private:
  std::unique_ptr<Summary> computeSummary(llvm::Function *f);

private:
  std::map<llvm::Function *, std::unique_ptr<Summary>> summaryLib;
  Executor proto; // this will be prototypes of all future summary executors
};

class CTXCSESummaryManager : public SummaryManager {
public:
  CTXCSESummaryManager(const Executor &mainExecutor) : summaryLib(), proto(mainExecutor) {
    proto.setSummaryManager(this);
  }

  Summary *getSummary(ExecutionState &es, llvm::Function *f) override;

private:
  std::unique_ptr<Summary> computeSummary(llvm::Function *f);

private:
  std::map<llvm::Function *, std::unique_ptr<Summary>> summaryLib;
  Executor proto;
};

} // namespace klee

#endif
