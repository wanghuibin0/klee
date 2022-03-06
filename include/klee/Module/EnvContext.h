#ifndef KLEE_ENVCONTEXT_H
#define KLEE_ENVCONTEXT_H

#include <map>

namespace llvm {
class Function;
}

namespace klee {

class EnvMap;
class Env;

EnvMap &getGlobalEnvMap();
std::map<llvm::Function *, Env> &getGlobalEnvContext();

}

#endif
