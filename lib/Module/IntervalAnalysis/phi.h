// date created:12/2/2015
// author: morgan

#ifndef PHI_H
#define PHI_H

#include "env.h"
#include "basicblock.h"
#include "headers.h"

class Work {
private:
  NewBasicBlock *nbb;
  Env env;

public:
  Work(NewBasicBlock *nbb, Env env) : nbb(nbb), env(env) {}
  Env getEnv() { return env; }
  NewBasicBlock *getNbb() { return nbb; }
  bool equals(NewBasicBlock *nbb, Env env) {
    return this->nbb == nbb && this->env == env;
  }
  void print() {
    cout << nbb << "-->\n";
    env.print();
  }
};

class PhiEntry {
public:
  NewBasicBlock *nbb;
  Env env1;
  Env env2;
  PhiEntry(NewBasicBlock *n, Env e1, Env e2) : nbb(n), env1(e1), env2(e2) {}
};

class Phi {

private:
  vector<PhiEntry> phi;

public:
  /*when there is an entry, use join*/
  void add(NewBasicBlock *n, Env e1, Env e2) {
    for (auto it = phi.begin(); it != phi.end(); it++) {
      if (it->nbb == n && it->env1 == e1) {
        it->env2 = it->env2 | e2;
        return;
      }
    }
    PhiEntry pe(n, e1, e2);
    phi.push_back(pe);
  }

  Env *getEnv(Work w) {
    return getEnv(w.getNbb(), w.getEnv());
    // for(auto it=phi.begin();it!=phi.end();it++){
    //    if (it->nbb==w.getNbb() && it->env1 == w.getEnv() ){
    //        return it->env2;
    //    }
    //}
  }

  Env *getEnv(NewBasicBlock *m, Env x) {
    for (auto it = phi.begin(); it != phi.end(); it++) {
      if (it->nbb == m && it->env1 == x) {
        return &(it->env2);
      }
    }
    return NULL; // if not in, then return bot TODO: this might be wrong
  }

  Env *getMidEnv(NewBasicBlock *m, Env x) {
    for (auto it = phi.begin(); it != phi.end(); it++) {
      if (it->nbb == m && it->env2 == x) {
        return &(it->env1);
      }
    }
    return NULL; // if not in, then return bot TODO: this might be wrong
  }

  bool hasEnv(NewBasicBlock *m, Env x) {
    for (auto it = phi.begin(); it != phi.end(); it++) {
      if (it->nbb == m && it->env1 == x) {
        return true;
      }
    }
    return false;
  }

  void propagate(NewBasicBlock *m, Env x, Env fy, deque<Work> *worklist) {
    if (hasEnv(m, x)) {
      Env oldEnv = *getEnv(m, x);
      if (fy <= oldEnv) { // without adding to worklist
        return;
      } else {
        add(m, x, fy);
        Env newEnv = *getEnv(m, x);
        // Env widened = oldEnv || newEnv;// TODO:should winden only the
        // conditional variable; otherwise not working for merging after simple
        // if
        Env widened = oldEnv | newEnv; // TODO:should winden only the
                                       // conditional variable; otherwise not
                                       // working for merging after simple if
        add(m, x, widened);
        // TODO:not sure if this check is necessary
        // for(auto &wk: *worklist){ //if the work is already in the worklist,
        // just return
        //    if (wk.equals(m, x)){
        //        return;
        //    }
        //}
        worklist->push_back(Work(m, x));
      }
    } else {
      add(m, x, fy);
      worklist->push_back(Work(m, x));
    }
  }

  Env computeXn(NewBasicBlock *n) {
    Env env;
    for (auto &phientry : phi) {
      if (phientry.nbb == n) {
        env = env | phientry.env2;
      }
    }
    return env;
  }

  void print() {
    for (auto it = phi.begin(); it != phi.end(); it++) {
      cout << "--------phi entry-------- :" << endl;
      cout << "BB:" << it->nbb << endl;
      cout << "env1:\n";
      it->env1.print();
      cout << "env2:\n";
      it->env2.print();
    }
  }
};
#endif
