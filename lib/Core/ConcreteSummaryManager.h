#ifndef KLEE_CONCRETESUMMARYMANAGER_H
#define KLEE_CONCRETESUMMARYMANAGER_H

#include "Executor.h"
#include "Summary.h"
#include "klee/Core/SummaryManager.h"

#include <map>

namespace klee {

enum InterpreterType { BUCSE, TDCSE, CTXCSE, NOCSE };

extern llvm::cl::opt<InterpreterType> InterpreterToUse;

class CTXCSESummaryManager : public SummaryManager {
public:
  // ctors
  CTXCSESummaryManager() = default;

  Summary *getSummary(ExecutionState &es, llvm::Function *f) override;

private:
  std::unique_ptr<Summary> computeSummary(llvm::Function *f);

private:
  std::map<llvm::Function *, std::unique_ptr<Summary>> summaryLib;
};

class BUCSESummaryManager : public SummaryManager {
public:
  // ctors
  BUCSESummaryManager() = default;

  Summary *getSummary(ExecutionState &es, llvm::Function *f) override;

private:
  std::unique_ptr<Summary> computeSummary(llvm::Function *f);

private:
  std::map<llvm::Function *, std::unique_ptr<Summary>> summaryLib;
};
} // namespace klee

#endif
