// date created:11/26/2015
// author: morgan
// encapsulation of iterator

#ifndef FIX_POINT_ITERATOR_H
#define FIX_POINT_ITERATOR_H

#include "cfg.h"
#include "headers.h"
ostream &operator<<(ostream &out, Env &env) {
  out << "env:" << endl;
  for (auto &pair : env) {
    out << pair.first;
    out << "\t";
    out << pair.second << "\n";
  }
  out << endl;
  return out;
}

class FixPointIter {
  vector<NewScc *> nsccs;

public:
  FixPointIter(NewCfg *ncfg) : nsccs(ncfg->getnSccs()) {}

  void run() {
    for (auto nsccIt = nsccs.rbegin(); nsccIt != nsccs.rend();
         nsccIt++) { // reverse order
      NewScc *nscc = *nsccIt;
      myassert(nscc->size() > 0, "fixpointiter run:");
      if (nscc->size() == 1) {
        NewBasicBlock *nbb = nscc->back(); // only 1 nbb
        auto preEnvs = nbb->preEnvs();
        nbb->setPrevEnv(nbb->combineEnvs(preEnvs));
        nbb->execute();
        // nbb->printEnv();
      } else {
        runCycle(nscc);
      }
    }
  }

  /*
   * should be run cycle later.
   * for now, just scc
   */
  void runCycle(NewScc *nscc) {
    NewBasicBlock *head = nscc->getHead();
    // auto preEnvs = head->preEnvs();
    // Env newEnv = head->combineEnvs(preEnvs);
    // cout<<newEnv<<endl;

    auto preEnvs = head->preEnvs();
    Env pre = head->combineEnvs(preEnvs);

    for (int counter = 0;; counter++) {
      vector<NewBasicBlock *> nbbs = nscc->getScc();
      for (auto nbbsIt = nbbs.rbegin(); nbbsIt != nbbs.rend();
           nbbsIt++) { // reverse order
        NewBasicBlock *nbb = *nbbsIt;
        if (nbb == head) {
          head->setPrevEnv(pre);
        } else {
          auto preEnvs2 = nbb->preEnvs();
          nbb->setPrevEnv(nbb->combineEnvs(preEnvs2));
        }
        nbb->execute();
        cout << "after a bb's execution" << endl;
        nbb->printEnv();
      }
      auto preEnvs3 = head->preEnvs();
      Env newEnv = head->combineEnvs(preEnvs3);
      // Env pre = head->getPrevEnv();
      // if(newEnv <= pre){
      cout << "new" << endl;
      newEnv.print();
      cout << "pre" << endl;
      pre.print();
      // if(counter==50)break;
      if (newEnv <= pre) {
        break;
      } else {
        if (counter >= 1) {
          // widening
          pre = pre || newEnv;
        } else {
          pre = pre | newEnv;
        }
      }
    }
  }
};

#endif
