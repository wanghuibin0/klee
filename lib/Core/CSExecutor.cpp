#include "CSExecutor.h"

#include "klee/Expr/Summary.h"

using namespace klee;

CSExecutor::CSExecutor() {}

void CSExecutor::run() {}
Summary *CSExecutor::extractSummary() { return new Summary(); }