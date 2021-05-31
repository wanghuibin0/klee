#ifndef KLEE_INTERPRETERMANAGER_H
#define KLEE_INTERPRETERMANAGER_H

//#include "klee/ADT/Ref.h"
//#include "klee/Expr/Expr.h"
#include "../lib/Core/CSExecutor.h"
#include "klee/Expr/Summary.h"

#include <map>
#include <vector>

namespace llvm {
class Function;
}

namespace klee {
class Interpreter;

enum InterpreterType { BUCSE, TDCSE, CTXCSE, NOCSE };

class InterpreterManager {
public:
  // ctors
  InterpreterManager(const Executor &mainExecutor) : proto(mainExecutor) {}

  Summary *getSummary(llvm::Function *f);

private:
  Summary *computeSummary(llvm::Function *f);
  void putSummaryToLib(llvm::Function *f, Summary *sum);
  CSExecutor *createCSExecutor(llvm::Function *f);

protected:
  Executor proto;
};

class BUCSEInterpreterManager : public InterpreterManager {
public:
  // ctors
  BUCSEInterpreterManager(const Executor &mainExecutor)
      : InterpreterManager(mainExecutor) {}

  Summary *getSummary(llvm::Function *f);

private:
  Summary *computeSummary(llvm::Function *f);
  void putSummaryToLib(llvm::Function *f, Summary *sum);
  CSExecutor *createCSExecutor(llvm::Function *f);

private:
  std::map<llvm::Function *, Summary *> summaryLib;
};

// class InterpreterManager {
// public:
//   // ctors
//   InterpreterManager(enum InterpreterType type) : type(type) {}

//   Summary *getSummary(llvm::Function *f);

// private:
//   Summary *computeSummary(llvm::Function *f);
//   void putSummaryToLib(llvm::Function *f, Summary *sum);
//   CSExecutor *createCSExecutor(llvm::Function *f);

// private:
//   std::map<llvm::Function *, Summary *> summaryLib;
// };

// class InterpreterManager {
// public:
//   // ctors
//   InterpreterManager(enum InterpreterType type) : type(type) {}

//   Summary *getSummary(llvm::Function *f);

// private:
//   Summary *computeSummary(llvm::Function *f);
//   void putSummaryToLib(llvm::Function *f, Summary *sum);
//   CSExecutor *createCSExecutor(llvm::Function *f);

// private:
//   std::map<llvm::Function *, Summary *> summaryLib;
// };

} // namespace klee

#endif
