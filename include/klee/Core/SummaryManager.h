#ifndef KLEE_INTERPRETERMANAGER_H
#define KLEE_INTERPRETERMANAGER_H

namespace llvm {
class Function;
}

namespace klee {
class Summary;
class Executor;
class ExecutionState;

class SummaryManager {
public:
  virtual ~SummaryManager() = default;

  static SummaryManager *createSummaryManager(const Executor &mainExecutor);
  virtual Summary *getSummary(ExecutionState &es, llvm::Function *f) = 0;
};


} // namespace klee

#endif
