#ifndef KLEE_CSEXECUTOR_H
#define KLEE_CSEXECUTOR_H

#include "Executor.h"
#include "klee/Expr/Summary.h"

namespace klee {

class CSExecutor : public Executor {
public:
  CSExecutor(const Executor &proto);
  void run();
  Summary *extractSummary();

private:
};

} // namespace klee
#endif