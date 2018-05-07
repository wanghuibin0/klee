//===-- SymbolicArrayRefresher.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/util/SymbolicArrayRefresher.h"
#include "klee/util/ArrayCache.h"

using namespace klee;


ExprVisitor::Action SymbolicArrayRefresher::visitRead(const ReadExpr &re) {
  ref<Expr> v = visit(re.index);

  const UpdateList &ul = re.updates;
  UpdateList newUl(ac.CreateArray(ul.root->name + "_cse", ul.root->size), 0);
  for (const UpdateNode *un=ul.head; un; un=un->next) {
    ref<Expr> index = visit(un->index);
    ref<Expr> value = visit(un->value);
    newUl.extend(index, value);
  }

  return Action::changeTo(ReadExpr::create(newUl, v));
}



