#ifndef KLEE_INTERPRETERMANAGER_H
#define KLEE_INTERPRETERMANAGER_H

#include "../lib/Core/CSExecutor.h"
#include "llvm/Support/CommandLine.h"
#include "../lib/Core/Summary.h"

#include <map>
#include <vector>

namespace llvm {
class Function;
}

namespace klee {
class Interpreter;

enum InterpreterType { BUCSE, TDCSE, CTXCSE, NOCSE };

extern llvm::cl::opt<InterpreterType> InterpreterToUse;

class SummaryManager {
public:
  SummaryManager() = default;
  virtual ~SummaryManager() = default;

  static SummaryManager *createSummaryManager(const Executor &mainExecutor);
  virtual Summary *getSummary(ExecutionState &es, llvm::Function *f) = 0;
};

class BUCSESummaryManager : public SummaryManager {
public:
  // ctors
  BUCSESummaryManager(const Executor &mainExecutor) : summaryLib(), proto(mainExecutor) {
    proto.setSummaryManager(this);
  }

  Summary *getSummary(ExecutionState &es, llvm::Function *f) override;

private:
  std::unique_ptr<Summary> computeSummary(llvm::Function *f);

private:
  std::map<llvm::Function *, std::unique_ptr<Summary>> summaryLib;
  Executor proto; // this will be prototypes of all future summary executors
};

} // namespace klee

#endif
