#ifndef KLEE_SUMMARY_H
#define KLEE_SUMMARY_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/Expr.h"

#include <vector>

namespace klee {

class PathSummary {
  std::vector<ref<Expr>> preCond;
  std::vector<ref<Expr>> postCond;

public:
  PathSummary(std::vector<ref<Expr>> &&pre, std::vector<ref<Expr>> &&post)
      : preCond(pre), postCond(post) {}
};
class Summary {
  std::vector<ref<Expr>> context;
  std::vector<ref<Expr>> args;
  std::vector<PathSummary *> pathSummaries;

public:
  Summary() {}
  void addPathSummary(PathSummary *ps) { pathSummaries.push_back(ps); }
};

} // namespace klee

#endif