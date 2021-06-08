#ifndef KLEE_CSEXECUTOR_H
#define KLEE_CSEXECUTOR_H

#include "Executor.h"
#include "klee/Expr/Summary.h"

namespace klee {

class BUCSExecutor : public Executor {
public:
  BUCSExecutor(const Executor &proto, const llvm::Function *f);
  void run();
  Summary *extractSummary();

private:
  const llvm::Function *func;
};

} // namespace klee
#endif