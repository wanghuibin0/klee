// date created:11/25/2015
// last modified:12/2/2015
// author: morgan
// encapsulation of cfg

#ifndef CFG_H
#define CFG_H

#include "basicblock.h"
#include "scc.h"
#include "headers.h"

class NewCfg {
private:
  vector<NewBasicBlock *> nbbs;
  vector<NewScc *> nsccs; // same order as llvm's scc traversal
  string fname; // name of the function
  Function *fun;
  NewBasicBlock *entry; // entry block
  NewBasicBlock *exit; // exit block

public:
  Function *getFun() { return fun; }

  vector<NewScc *> getnSccs() { return nsccs; }
  vector<NewBasicBlock *> getnbbs() { return nbbs; }
  string getName() { return fname; }
  NewBasicBlock *getEntry() { return entry; }
  NewBasicBlock *getExit() { return exit; }

  NewCfg(vector<BasicBlock *> bbs, string fname, Function *fun) {
    this->fname = fname;
    this->fun = fun;

    /**********************************build edges for NBB; setup the entry
     * block*******************/
    for (vector<BasicBlock *>::iterator it = bbs.begin(); it != bbs.end();
         it++) {
      BasicBlock *bb = (BasicBlock *)*it;
      NewBasicBlock *nbb = new NewBasicBlock(bb);
      if ((&(fun->getEntryBlock())) == bb) {
        entry = nbb; /*cout<<"found entry;"<<endl;outs()<<*(entry->getId());*/
      }
      if (isa<ReturnInst>(&(bb->back()))) {
        exit = nbb; /*cout<<"found exit;"<<endl;outs()<<*(exit->getId());*/
      } // TODO:assume there is only one ret inst
      this->nbbs.push_back(nbb);
    }

    for (vector<BasicBlock *>::iterator it = bbs.begin(); it != bbs.end();
         it++) {
      BasicBlock *bb = (BasicBlock *)*it;
      for (auto it2 = succ_begin(bb); it2 != succ_end(bb); ++it2) {
        NewBasicBlock *newbb1 = find(bb);
        NewBasicBlock *newbb2 = find(*it2);
        newbb1->addEdge(newbb2);
      }
    }
  }

  void addScc(vector<BasicBlock *> scc) {
    vector<NewBasicBlock *> nscc;
    for (auto it = scc.begin(); it != scc.end(); it++) {
      nscc.push_back(find(*it));
    }
    nsccs.push_back(new NewScc(nscc));
  }

  NewBasicBlock *find(BasicBlock *bb) {
    for (auto it = nbbs.begin(); it != nbbs.end(); it++) {
      if (bb == (*it)->getId()) {
        return *it;
      }
    }
    myassert(0, "find cannot be reached");
  }

  int size() { return nbbs.size(); }

  void test() {
    for (auto it = nbbs.begin(); it != nbbs.end(); it++) {
      NewBasicBlock *bb = (NewBasicBlock *)*it;
      // outs()<<bb->getPreds().size()<<"\n";
      outs() << bb->getId() << "\n";
      // outs()<<bb->getSuccs2().size()<<"\n";
    }
  }
  void printsccs() {
    cout << "------------------------sccs|v---------------------" << endl;
    for (auto nscc = nsccs.begin(); nscc != nsccs.end(); nscc++) {
      cout << (*nscc)->size() << " ";
      auto tmp = (*nscc)->getScc();
      for (auto nbb = tmp.begin(); nbb != tmp.end(); nbb++) {
        cout << (*nbb)->getId() << " ";
      }
      cout << endl;
    }
    cout << "------------------------sccs|^----------------------" << endl;
  }
};

#endif
