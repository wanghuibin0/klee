#ifndef KLEE_INTERPRETERMANAGER_H
#define KLEE_INTERPRETERMANAGER_H

#include <llvm/Support/CommandLine.h>

#include <memory>
#include <map>

namespace llvm {
class Function;
}

namespace klee {

enum InterpreterType { BUCSE, TDCSE, CTXCSE, NOCSE };

extern llvm::cl::opt<InterpreterType> InterpreterToUse;

class Summary;
class Executor;
class ExecutionState;

class SummaryManager {
public:
  virtual ~SummaryManager() = default;
  
  static SummaryManager *createSummaryManager();
  Summary *getSummary(llvm::Function *f);
  void setProto(Executor &e);

  virtual Summary *computeSummary(ExecutionState &es, llvm::Function *f) = 0;
  void dump();

protected:
  std::unique_ptr<Executor> proto;
  std::map<llvm::Function *, std::unique_ptr<Summary>> summaryLib;
};

class CTXCSESummaryManager : public SummaryManager {
public:
  // ctors
  CTXCSESummaryManager() = default;

  Summary *computeSummary(ExecutionState &es, llvm::Function *f) override;
};

class BUCSESummaryManager : public SummaryManager {
public:
  // ctors
  BUCSESummaryManager() = default;

  Summary *computeSummary(ExecutionState &es, llvm::Function *f) override;
};

} // namespace klee

#endif
