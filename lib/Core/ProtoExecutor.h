#ifndef KLEE_PROTOEXECUTOR_H
#define KLEE_PROTOEXECUTOR_H

#include "Executor.h"

namespace klee {
void setGlobalProtoExecutor(const Executor &e);
Executor &getGlobalProtoExecutor();
} // namespace klee

#endif
