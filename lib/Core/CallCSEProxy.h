#ifndef KLEE_CALLCSEPROXY_H
#define KLEE_CALLCSEPROXY_H

namespace llvm {
class Function;
}

namespace klee {
class Executor;
class SummaryManager;
class ExecutionState;

class CallCSEProxy {
private:
  Executor &executor;
  SummaryManager &summaryManager;
  ExecutionState &currentState;
  llvm::Function *func;

public:
  CallCSEProxy(Executor &e, SummaryManager &sm, ExecutionState &es,
               llvm::Function *f)
      : executor(e), summaryManager(sm), currentState(es), func(f) {}
  void execute();
};

} // namespace klee

#endif
