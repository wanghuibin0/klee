#ifndef ENV_H
#define ENV_H

#include "klee/ADT/Ref.h"
#include "Interval.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include <map>

namespace klee {
  class Env {
    std::map<llvm::Value*, Interval> m;

  public:
    Env() = default;
    Env(const Env&) = default;
    ~Env() = default;
    static Env Top() { return Env(); }
    static Env Bot() { return Env(); }
    bool hasValue(llvm::Value* v) const {
      auto it = m.find(v);
      return it != m.end();
    }
    /// should be used after hasValue
    Interval lookup(llvm::Value* v) const {
      assert(hasValue(v));
      return m.find(v)->second;
    }

    void set(llvm::Value* v, Interval i) {
      auto it = m.find(v);
      if (it != m.end()) {
        it->second = i;
      } else {
        m.insert(std::make_pair(v, i));
      }
    }
    /// should be used after hasValue
    void remove(llvm::Value* v) {
      assert(hasValue(v));
      m.erase(m.find(v));
    }
    void clear() {
      m.clear();
    }

    using iterator = std::map<llvm::Value*, Interval>::iterator;
    iterator begin() { return m.begin(); }
    iterator end() { return m.end(); }

    Env operator|(const Env &r) {
      Env newEnv{r};
      for (auto it : m) {
        auto *v = it.first;
        auto i = it.second;
        if (newEnv.hasValue(v)) {
          Interval lhs = newEnv.lookup(v);
          Interval rhs = i;
          Interval newI(lhs | rhs);
          newEnv.set(v, newI);
        } else {
          newEnv.set(v, i);
        }
      }
      return newEnv;
    }

    Env operator||(const Env &r); // widening
    bool operator<=(const Env &r) {
      if (m.size() == 0) {
        return true;
      } else if (r.m.size() == 0) {
        return false;
      } else {
        for (auto it : m) {
          auto *v = it.first;
          auto i = it.second;
          Interval lhs = i;
          Interval rhs = r.lookup(v);
          if (!r.hasValue(v) || !(lhs <= rhs)) {
            return false;
          }
        }
        return true;
      }
    }

    bool operator==(const Env &r) {
      return m == r.m;
    }
    bool operator!=(const Env &r) {
      return !(*this == r);
    }

    void dump(llvm::raw_ostream &os) const {
      os << "dumping env:\n";
      for (auto it : m) {
        os << "Value: ";
        os << *(it.first) << ", ";
        os << "Interval: ";
        os << it.second << "\n";
      }
    }
  };

  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Env &e) {
    e.dump(os);
    return os;
  }


} // end namespace klee

#endif
