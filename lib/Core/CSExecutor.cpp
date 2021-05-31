#include "CSExecutor.h"

#include "klee/Expr/Summary.h"

using namespace klee;

CSExecutor::CSExecutor(const Executor &proto) : Executor(proto) {}

void CSExecutor::run() {}
Summary *CSExecutor::extractSummary() { return new Summary(); }