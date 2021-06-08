#include "CSExecutor.h"

#include "klee/Expr/Summary.h"

using namespace klee;

BUCSExecutor::BUCSExecutor(const Executor &proto, const llvm::Function *f)
    : Executor(proto), func(f) {}

void BUCSExecutor::run() {}
Summary *BUCSExecutor::extractSummary() { return new Summary(); }