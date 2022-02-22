#ifndef ABSTRACTVALUE_H
#define ABSTRACTVALUE_H

#include "klee/ADT/Ref.h"

namespace klee {

class AbstractValue {
public:
  enum ValueKind {
    K_Pointer,
    K_Interval
  };
private:
  const ValueKind Kind;
public:
  class ReferenceCounter _refCount;
public:
  ValueKind getKind() const { return Kind; }

  AbstractValue(ValueKind vk) : Kind(vk) {}
  virtual ~AbstractValue() {}
  virtual ref<AbstractValue> deref() = 0;
  int compare(const AbstractValue &av) const;
};

class Pointer : public AbstractValue {
  ref<AbstractValue> pov;
public:
  Pointer(AbstractValue *pov) : AbstractValue(K_Pointer), pov(pov) {}
  ref<AbstractValue> deref() override {
    return pov->deref();
  }
  static bool classof(const AbstractValue *av) {
    return av->getKind() == K_Pointer;
  }
};

}
#endif
