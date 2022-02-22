// date created:12/11/2015
// author: morgan

#ifndef REL_H
#define REL_H

#include "headers.h"

class RelPair {
public:
  RelPair(Value *v1, Value *v2) : v1(v1), v2(v2) {}
  Value *v1;
  Value *v2;
};

class Rels {
private:
  vector<RelPair> rels;

public:
  vector<RelPair> getRels() { return rels; }

  void addPair(Value *v1, Value *v2) { // don't add (a,a); invalid before adding
    if (v1 == v2)
      return;
    remove(v1);
    remove(v2);
    RelPair p(v1, v2);
    rels.push_back(p);
  }
  void remove(Value *v) {
    for (auto it = rels.begin(); it != rels.end(); it++) {
      Value *v1 = (*it).v1;
      Value *v2 = (*it).v2;
      if (v1 == v || v2 == v) {
        rels.erase(it);
        it--;
      }
    }
  } // if v appears, then remove all pairs containing v
  bool hasPair(Value *v1, Value *v2) {
    for (auto &pair : rels) {
      if ((pair.v1 == v1 && pair.v2 == v2) || pair.v1 == v2 && pair.v2 == v1) {
        return true;
      }
    }
    return false;
  }

  // bool isEqual(Value* v1, Value* v2){//equal when only appears as a pair
  //     if (!hasPair(v1, v2)) return false;
  //     for(auto &pair: rels){
  //         if ((pair.v1 == v1 && pair.v2 != v2)|| (pair.v1==v2 && pair.v2!=v1)
  //                 || (pair.v1!=v1 && pair.v2==v2) || (pair.v1!=v2 &&
  // pair.v2==v1)){
  //             return false;
  //         }
  //     }
  //     return true;
  //}

  void print() {
    for (auto &pair : rels) {
      cout << pair.v1 << "\t" << pair.v2 << endl;
    }
    cout << endl;
  }

  Rels operator|(Rels other) {
    Rels ret;
    for (auto &p : other.getRels()) {
      Value *v1 = p.v1;
      Value *v2 = p.v2;
      if (hasPair(v1, v2)) {
        ret.addPair(v1, v2);
      }
    }
    return ret;
  }
};

#endif
