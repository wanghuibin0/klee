#ifndef KLEE_CSEXECUTOR_H
#define KLEE_CSEXECUTOR_H

#include "Executor.h"

namespace klee {

class BUCSExecutor : public Executor {
public:
  BUCSExecutor(const Executor &proto, llvm::Function *f);
  void run();
  Summary *extractSummary();

private:
  ExecutionState *createInitialState(llvm::Function *f);
  void initializeGlobals(ExecutionState &state);
  void makeArgsSymbolic(ExecutionState *state);
  void makeGlobalsSymbolic(ExecutionState *state);


  // remove state from queue and delete
  virtual void terminateState(ExecutionState &state) override;
  // call exit handler and terminate state
  virtual void terminateStateEarly(ExecutionState &state,
                                   const llvm::Twine &message) override;
  // call exit handler and terminate state
  virtual void terminateStateOnExit(ExecutionState &state) override;
  // call error handler and terminate state
  virtual void terminateStateOnError(ExecutionState &state,
                                     const llvm::Twine &message,
                                     enum TerminateReason termReason,
                                     const char *suffix = NULL,
                                     const llvm::Twine &longMessage = "") override;

  virtual void stepInstruction(ExecutionState &state);

private:
  llvm::Function *func;
  Summary *summary;
  std::vector<const llvm::GlobalValue*> globalsMod;
};

} // namespace klee
#endif
