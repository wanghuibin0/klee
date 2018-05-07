#define NDEBUG
#include "Passes.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
  void initializeRecursionFinderPassPass(PassRegistry&);
}

namespace klee {
  extern std::set<llvm::Function*> recursive;
}

using namespace llvm;
using namespace klee;
char RecursionFinderPass::ID;

INITIALIZE_PASS_BEGIN(RecursionFinderPass, "find-recursion",
                "Find Recursive Functions of this Module", false, true)
INITIALIZE_PASS_DEPENDENCY(CallGraph)
INITIALIZE_PASS_END(RecursionFinderPass, "find-recursion",
                "Find Recursive Functions of this Module", false, true)

RecursionFinderPass::RecursionFinderPass()
      : ModulePass(ID) {
      initializeRecursionFinderPassPass(*PassRegistry::getPassRegistry());
}

bool RecursionFinderPass::runOnModule(Module &M) {
  CallGraphNode *rootNode = getAnalysis<CallGraph>().getRoot();
  for (scc_iterator<CallGraphNode*> SCCI = scc_begin(rootNode),
      E = scc_end(rootNode); SCCI != E; ++SCCI) {
    if (!SCCI.hasLoop()) continue;
    const std::vector<CallGraphNode*> &nextSCC = *SCCI;
    for (std::vector<CallGraphNode*>::const_iterator I = nextSCC.begin(),
          E = nextSCC.end(); I != E; ++I) {
      errs() << "recursssssssssss: " << (*I)->getFunction()->getName() << "\n";
      recursive.insert((*I)->getFunction());
    }
  }
  return false;
}

