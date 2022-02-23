// date created:11/25/2015
// last changed:11/27/2015
// author: morgan

#ifndef INTERVAL_H
#define INTERVAL_H

#include <iostream>
#include <climits>

using std::ostream;
using std::cout;
using std::endl;

class Interval {
private:
  int upper;
  int lower;

public:
  Interval(int lower, int upper) : upper(upper), lower(lower) {}
  Interval() : upper(-1), lower(1) {} // bottom no info

  Interval(const Interval &other) : upper(other.upper), lower(other.lower) {}

  bool isBottom() { return upper < lower; }
  bool isTop() { return upper == INT_MAX && lower == INT_MIN; }

  int getUpper() { return upper; }
  int getLower() { return lower; }

  // assign operator
  Interval &operator=(const Interval &other) {
    if (this == &other) {
      return *this;
    } else {
      this->upper = other.upper;
      this->lower = other.lower;
    }
    return *this;
  }

  //<= operator
  bool operator<=(Interval other) {
    if (this->isBottom()) {
      return true;
    } else if (other.isBottom()) {
      return false;
    } else {
      return (other.lower <= this->lower) && (this->upper <= other.upper);
    }
  }

  // add operator
  Interval operator+(Interval other) {
    if (this->isBottom() || other.isBottom()) {
      return Interval(); // if one is bottom, return bottom
    } else {
      return Interval(lower + other.lower, upper + other.upper);
    }
  }

  // sub operator
  Interval operator-(Interval other) {
    if (this->isBottom() || other.isBottom()) {
      return Interval(); // if one is bottom, return bottom
    } else {
      return Interval(lower - other.upper, upper - other.lower);
    }
  }

  // mul operator
  Interval operator*(Interval other) {
    if (this->isBottom() || other.isBottom()) {
      return Interval(); // if one is bottom, return bottom
    } else {
      int ll = lower * other.lower;
      int max = ll;
      int min = ll;
      int lu = lower * other.upper;
      if (lu > max) {
        max = lu;
      }
      if (lu < min) {
        min = lu;
      }
      int ul = upper * other.lower;
      if (ul > max) {
        max = ul;
      }
      if (ul < min) {
        min = ul;
      }
      int uu = upper * other.upper;
      if (uu > max) {
        max = uu;
      }
      if (uu < min) {
        min = uu;
      }
      return Interval(min, max);
    }
  }

  // div operator
  Interval operator/(Interval other) {
    if (this->isBottom() || other.isBottom()) {
      return Interval(); // if one is bottom, return bottom
    } else {
      if (other.lower <= 0 && other.upper >= 0)
        return top();
      else {
        double tmp1 = 1.0 / other.upper;
        double tmp2 = 1.0 / other.lower;

        int ll = lower * tmp1;
        int max = ll + 1;
        int min = ll;
        int lu = lower * tmp2;
        if (lu > max) {
          max = lu + 1;
        }
        if (lu < min) {
          min = lu;
        }
        int ul = upper * tmp1;
        if (ul > max) {
          max = ul + 1;
        }
        if (ul < min) {
          min = ul;
        }
        int uu = upper * tmp2;
        if (uu > max) {
          max = uu + 1;
        }
        if (uu < min) {
          min = uu;
        }
        return Interval(min, max);
      }
    }
  }
  // join operator
  Interval operator|(Interval other) {
    if (this->isBottom()) {
      return other;
    } else if (other.isBottom()) {
      return *this;
    } else {
      return Interval(lower < other.lower ? lower : other.lower,
                      upper > other.upper ? upper : other.upper);
    }
  }

  // windening operator
  Interval operator||(Interval other) {
    if (this->isBottom()) {
      return other;
    } else if (other.isBottom()) {
      return *this;
    } else {
      return Interval(other.lower < lower ? INT_MIN : lower,
                      upper < other.upper ? INT_MAX : upper);
    }
  }

  // meet operator
  Interval operator&(Interval other) {
    if (this->isBottom() || other.isBottom()) {
      return bottom();
    } else {
      return Interval(lower < other.lower ? other.lower : lower,
                      upper > other.upper ? other.upper : upper);
    }
  }

  //== operator
  bool operator==(Interval other) {
    if (isBottom() && other.isBottom())
      return true;
    return lower == other.lower && upper == other.upper;
  }

  static Interval bottom() { return Interval(); }
  static Interval top() { return Interval(INT_MIN, INT_MAX); }
};

ostream &operator<<(ostream &out, Interval &i) {
  if (i.isBottom()) {
    // cout<<"here"<<endl;
    out << "_|_";
  } else {
    out << "[";
    if (i.getLower() == INT_MIN) {
      out << "-oo";
    } else {
      out << i.getLower();
    }
    out << ",";
    if (i.getUpper() == INT_MAX) {
      out << "+oo";
    } else {
      out << i.getUpper();
    }
    out << "]";
  }
  return out;
}
#endif
