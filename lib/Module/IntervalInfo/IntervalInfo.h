#ifndef KLEE_INTERVALINFO_H
#define KLEE_INTERVALINFO_H

#include "klee/Config/Version.h"
#include "llvm/Pass.h"


namespace klee {

class Env;
class IntervalInfoPass : public llvm::FunctionPass {
private:
public:
  static char ID;
  IntervalInfoPass();
  virtual ~IntervalInfoPass();
  bool runOnFunction(llvm::Function &f) override;
private:
  void initializeEntryInfo(llvm::Function &f, llvm::BasicBlock &b);
};

} // namespace klee

#endif
