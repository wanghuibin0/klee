#include "Passes.h"
#include "klee/Config/Version.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <deque>

#include "IntervalInfo/Interval.h"
#include "IntervalInfo/Env.h"

namespace klee {
  IntervalInfoPass::IntervalInfoPass() : llvm::FunctionPass(ID) {
    envIn = new std::map<llvm::BasicBlock*, Env>();
    envOut = new std::map<llvm::BasicBlock*, Env>();
  }

  IntervalInfoPass::~IntervalInfoPass() {
    delete envIn;
    delete envOut;
  }

  bool IntervalInfoPass::runOnFunction(llvm::Function &f) {
    // initialize env of all blocks
    // add all blocks to worklist
    llvm::BasicBlock &entry = f.getEntryBlock();
    envIn->insert(std::make_pair(&entry, Env::Top()));

    std::deque<llvm::BasicBlock*> worklist;
    worklist.push_back(&entry);

    while (!worklist.empty()) {
      auto bb = worklist.front();
      worklist.pop_front();
      bb->print(llvm::outs());
      auto oldOut = (*envOut)[bb];
      auto in = joinOutOfPreds(bb);
      auto out = propagate(bb);  //TODO: widening
      if (oldOut != out) {
        for (auto it : llvm::successors(bb)) {
          worklist.insert(*it);
        }
      }
    }

    Interval a(1,3);
    return false;
  }

  Env IntervalInfoPass::joinOutOfPreds(llvm::BasicBlock *bb) {
    return Env::Top();
  }


  char IntervalInfoPass::ID = 0;

} // end klee namespace
