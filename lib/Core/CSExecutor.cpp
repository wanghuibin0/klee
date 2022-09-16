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

} // namespace klee
