#include "llvm/Support/CommandLine.h"

using namespace llvm;

namespace klee {
extern cl::OptionCategory CompExCat;
extern cl::opt<bool> SimplifySymIndices;

cl::opt<unsigned>
    MaxLoopUnroll("max-loopunroll",
                  cl::desc("Refuse to fork when above this amount of "
                           "loop unroll times (default=5)"),
                  cl::init(5), cl::cat(CompExCat));
cl::opt<unsigned>
    MaxStatesInCse("max-states-in-cse",
                   cl::desc("Inhibit forking when too many states are "
                            "generated during CSE (default=10)"),
                   cl::init(30), cl::cat(CompExCat));

} // namespace klee
