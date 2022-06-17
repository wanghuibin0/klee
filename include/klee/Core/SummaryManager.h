#ifndef KLEE_INTERPRETERMANAGER_H
#define KLEE_INTERPRETERMANAGER_H

#include <memory>

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

  static SummaryManager *createSummaryManager();
  virtual Summary *getSummary(ExecutionState &es, llvm::Function *f) = 0;
  void setProto(Executor &e);
protected:
  std::unique_ptr<Executor> proto;
};


} // namespace klee

#endif
