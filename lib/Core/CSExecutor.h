#ifndef KLEE_CSEXECUTOR_H
#define KLEE_CSEXECUTOR_H

#include "Executor.h"
#include <utility>

namespace klee {

class CTXCSExecutor : public Executor {
public:
  CTXCSExecutor(const Executor &proto, llvm::Function *f);
  void run();
  std::unique_ptr<Summary> extractSummary();

private:
  ExecutionState *createInitialState(llvm::Function *f);
  void initializeGlobals(ExecutionState &state);
  void makeArgsSymbolic(ExecutionState *state);
  void makeGlobalsSymbolic(ExecutionState *state);
  void buildConstraintFromStaticContext(ExecutionState *es, llvm::Function *func);

  // remove state from queue and delete
  void terminateState(ExecutionState &state) override;
  // call exit handler and terminate state
  void terminateStateEarly(ExecutionState &state,
                                   const llvm::Twine &message) override;
  // call exit handler and terminate state
  void terminateStateOnExit(ExecutionState &state) override;
  void terminateStateOnRet(ExecutionState &state);
  // call error handler and terminate state
  void terminateStateOnError(ExecutionState &state,
                                     const llvm::Twine &message,
                                     enum TerminateReason termReason,
                                     const char *suffix = NULL,
                                     const llvm::Twine &longMessage = "") override;

  void stepInstruction(ExecutionState &state) override;

private:
  llvm::Function *func;
  std::unique_ptr<Summary> summary;
  ConstraintSet context;
};

class BUCSExecutor : public Executor {
public:
  BUCSExecutor(const Executor &proto, llvm::Function *f);
  void run();
  std::unique_ptr<Summary> extractSummary();

private:
  ExecutionState *createInitialState(llvm::Function *f);
  void initializeGlobals(ExecutionState &state);
  void makeArgsSymbolic(ExecutionState *state);
  void makeGlobalsSymbolic(ExecutionState *state);
  void buildConstraintFromStaticContext(ExecutionState *es, llvm::Function *func);

  // remove state from queue and delete
  void terminateState(ExecutionState &state) override;
  // call exit handler and terminate state
  void terminateStateEarly(ExecutionState &state,
                                   const llvm::Twine &message) override;
  // call exit handler and terminate state
  void terminateStateOnExit(ExecutionState &state) override;
  void terminateStateOnRet(ExecutionState &state);
  // call error handler and terminate state
  void terminateStateOnError(ExecutionState &state,
                                     const llvm::Twine &message,
                                     enum TerminateReason termReason,
                                     const char *suffix = NULL,
                                     const llvm::Twine &longMessage = "") override;

  void stepInstruction(ExecutionState &state) override;
  void executeMemoryOperation(ExecutionState &state, bool isWrite,
                              ref<Expr> address,
                              ref<Expr> value /* undef if read */,
                              KInstruction *target /* undef if write */) override;

private:
  llvm::Function *func;
  std::unique_ptr<Summary> summary;
  ConstraintSet context;
};
} // namespace klee
#endif
