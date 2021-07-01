#ifndef KLEE_INTERPRETERMANAGER_H
#define KLEE_INTERPRETERMANAGER_H

//#include "klee/ADT/Ref.h"
//#include "klee/Expr/Expr.h"
#include "../lib/Core/CSExecutor.h"
#include "llvm/Support/CommandLine.h"

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
  SummaryManager() {}

  static SummaryManager *createSummaryManager(const Executor &mainExecutor);
  virtual Summary *getSummary(ExecutionState &es, llvm::Function *f) = 0;
};

class BUCSESummaryManager : public SummaryManager {
public:
  // ctors
  BUCSESummaryManager(const Executor &mainExecutor) : proto(mainExecutor) {}

  Summary *getSummary(ExecutionState &es, llvm::Function *f);

private:
  Summary *computeSummary(llvm::Function *f);
  BUCSExecutor *createBUCSExecutor(llvm::Function *f);

private:
  std::map<llvm::Function *, Summary *> summaryLib;
  const Executor proto;
};

// class SummaryManager {
// public:
//   // ctors
//   SummaryManager(enum InterpreterType type) : type(type) {}

//   Summary *getSummary(llvm::Function *f);

// private:
//   Summary *computeSummary(llvm::Function *f);
//   void putSummaryToLib(llvm::Function *f, Summary *sum);
//   CSExecutor *createCSExecutor(llvm::Function *f);

// private:
//   std::map<llvm::Function *, Summary *> summaryLib;
// };

// class SummaryManager {
// public:
//   // ctors
//   SummaryManager(enum InterpreterType type) : type(type) {}

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
