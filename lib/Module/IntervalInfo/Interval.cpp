#include "Interval.h"
#include "Env.h"

namespace klee {

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const klee::Interval &i) {
  i.dump(os);
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Env &e) {
  e.dump(os);
  return os;
}

} // namespace klee
