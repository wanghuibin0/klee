#ifndef ENV_H
#define ENV_H

#include "Interval.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

namespace klee {
  class Env {
    std::map<llvm::Value*, Interval> m;

  public:
    Env() = default;
    Env(const Env&) = default;
    ~Env() = default;
    static Env Top() { return Env(); }
    static Env Bot() { return Env(); }
    Interval find(llvm::Value*);
    bool insert(llvm::Value*, Interval);
    bool remove(llvm::Value*);
    void clear();

    Env operator|(const Env &r); // join
    Env operator||(const Env &r); // widening
    bool operator<=(const Env &r); // comparison
    bool operator==(const Env &r); // comparison
    bool operator!=(const Env &r); // comparison

    void dump(std::ostream &os);
  };

} // end namespace klee

#endif
