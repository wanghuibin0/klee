#ifndef ENVMAP_H
#define ENVMAP_H

#include "Env.h"
#include <utility>

namespace llvm {
class BasicBlock;
class Function;
}

namespace klee {
class EnvMap {
  using Edge = std::pair<llvm::BasicBlock *, llvm::BasicBlock *>;
  std::map<Edge, Env> envMap;

public:
  EnvMap() = default;
  Env &getEnv(llvm::BasicBlock *b0, llvm::BasicBlock *b1) {
    Edge e{b0, b1};
    return envMap[e];
  }
  void update(llvm::BasicBlock *b0, llvm::BasicBlock *b1, const Env &env) {
    Edge e{b0, b1};
    envMap[e] = env;
  }
  void mergeAndUpdate(llvm::BasicBlock *b0, llvm::BasicBlock *b1, const Env &env) {
    Edge e{b0, b1};
    envMap[e] = envMap[e] | env;
  }
  void dump() {
    for (auto &&it : envMap) {
      llvm::errs() << "[<" << it.first.first << "," << it.first.second << ">,";
      it.second.dump(llvm::errs());
      llvm::errs() << "]\n";
    }
  }
};

EnvMap &getGlobalEnvMap();
std::map<llvm::Function *, Env> &getGlobalEnvContext();

} // namespace klee

#endif
