#include "ProtoExecutor.h"

using namespace klee;
static Executor *globalProtoExecutor = nullptr;

void setGlobalProtoExecutor(const Executor &e) {
  assert(globalProtoExecutor == nullptr && "globalProtoExecutor should only be set once");
  globalProtoExecutor = new Executor(e);
}

Executor &getGlobalProtoExecutor() {
  assert(globalProtoExecutor != nullptr && "globalProtoExecutor must be already set");
  return *globalProtoExecutor;
}
