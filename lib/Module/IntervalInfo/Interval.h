#ifndef KLEE_INTERVAL_H
#define KLEE_INTERVAL_H

#include <limits>
#include <algorithm>

//using namespace std;

using IntTy = int;
static constexpr IntTy MAX = std::numeric_limits<IntTy>::max() >> 1;
static constexpr IntTy MIN = std::numeric_limits<IntTy>::min() >> 1;

class Interval {
  IntTy lower;
  IntTy upper;

public:

public:
  Interval(IntTy l, IntTy h) : lower(std::max(l, MIN)), upper(std::min(h, MAX)) {}
  Interval(const Interval &other) = default;
  static Interval Bot() { return Interval(1, 0); }
  static Interval Top() {
    return Interval(MIN, MAX);
  }

  bool isBot() const { return lower > upper; }
  bool isTop() const {
    return lower == MIN && upper == MAX;
  }

  Interval &operator=(const Interval &rhs) {
    if (this != &rhs) {
      lower = rhs.lower;
      upper = rhs.upper;
    }
    return *this;
  }

  bool operator==(const Interval &r) const {
    return (isBot() && r.isBot())
      || (isTop() && r.isTop())
      || (lower == r.lower && upper == r.upper);
  }

  bool operator<=(const Interval &r) const {
    return isBot()
      || r.isTop()
      || (lower >= r.lower && upper <= r.upper);
  }

  Interval operator+(const Interval &r) {
    if (isBot() || r.isBot()) {
      return Bot();
    } else if (isTop() || r.isTop()) {
      return Top();
    } else {
      return Interval(lower+r.lower, upper+r.upper);
    }
  }

  Interval operator-(const Interval &r) {
    if (this->isBot() || r.isBot()) {
      return Bot();
    } else if (isTop() || r.isTop()) {
      return Top();
    } else {
      return Interval(lower-r.upper, upper-r.lower);
    }
  }

  Interval operator*(const Interval &r) {
    if (isBot() || r.isBot()) {
      return Bot();
    } else if (isTop() || r.isTop()) {
      return Top();
    } else {
      int ll = lower * r.lower;
      int lh = lower * r.upper;
      int hl = upper * r.lower;
      int hh = upper * r.upper;
      auto res = std::minmax({ll, lh, hl, hh});
      return Interval(res.first, res.second);
    }
  }

  Interval operator/(const Interval &r) {
    if (this->isBot() || r.isBot()) {
      return Bot();
    } else if (isTop() || r.isTop()) {
      return Top();
    } else {
      if (r.lower <= 0 && r.upper >=0) {
        return Top();
      } else {
        double x = 1.0/r.upper;
        double y = 1.0/r.lower;
        int ll = lower * x;
        int lh = lower * y;
        int hl = upper * x;
        int hh = upper * y;
        auto res = std::minmax({ll, lh, hl, hh});
        return Interval(res.first, res.second);
      }
    }
  }

  Interval operator&(const Interval &r) {
    if (this->isBot() || r.isBot()) {
      return Bot();
    } else if (isTop()) {
      return r;
    } else if (r.isTop()) {
      return *this;
    } else {
      return Interval(std::max(lower, r.lower), std::min(upper, r.upper));
    }
  }

  Interval operator|(const Interval &r) {
    if (this->isBot()) {
      return r;
    } else if (r.isBot()) {
      return *this;
    } else if (isTop() || r.isTop()) {
      return Top();
    } else {
      return Interval(std::min(lower, r.lower), std::max(upper, r.upper));
    }
  }

  Interval operator||(const Interval &r) {
    if (isBot()) {
      return r;
    } else if (r.isBot()) {
      return *this;
    } else {
      int l = lower <= r.lower ? lower : MIN;
      int h = upper < r.upper ? MAX : upper;
      return Interval(l, h);
    }
  }

  void dump(std::ostream &os) const {
    if (isBot()) {
      os << "_|_";
    } else {
      os << "[";
      if (lower == MIN) {
        os << "-oo";
      } else {
        os << lower;
      }
      os << ",";
      if (upper == MAX) {
        os << "+oo";
      } else {
        os << upper;
      }
      os << "]";
    }
  }
};

std::ostream &operator<<(std::ostream &os, const Interval &i) {
  i.dump(os);
  return os;
}

#endif
