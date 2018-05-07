//===-- SymbolicArrayRefresher.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SYMBOLICARRAYREFRESHER_H
#define KLEE_SYMBOLICARRAYREFRESHER_H

#include "klee/Expr.h"
#include "klee/util/ExprVisitor.h"

namespace klee {
  class SymbolicArrayRefresher : public ExprVisitor {
    ArrayCache &ac;
  protected:
    Action visitRead(const ReadExpr &re);
      
  public:
    SymbolicArrayRefresher(ArrayCache &ac) : ac(ac) {}

  };
}

#endif
