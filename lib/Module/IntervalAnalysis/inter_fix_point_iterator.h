// date created:12/2/2015
// author: morgan
// encapsulation of iterator

#ifndef INTER_FIX_POINT_ITERATOR_H
#define INTER_FIX_POINT_ITERATOR_H

#include "module.h"
#include "phi.h"
#include "headers.h"

class InterFixPointIter {
  NewModule *nmod;

public:
  InterFixPointIter(NewModule *nmod) : nmod(nmod) {}

  void run() {
    deque<Work> *worklist = new deque<Work>();

    NewBasicBlock *entry =
        nmod->getCfg("main")->getEntry(); // outs()<<*(entry->getId());
    Phi *phi = new Phi();
    phi->add(entry, Env(), Env());
    Work w(entry, Env());
    worklist->push_back(w);

    while (worklist->size() > 0) {
      Work w = worklist->front();
      // w.print();
      worklist->pop_front();
      NewBasicBlock *n = w.getNbb();
      Env x = w.getEnv();
      Env y = *(phi->getEnv(w));
      if (!n->isCallBb() && !n->isRetBb()) {
        Env fy = n->execute(y);
        // fy.print();
        vector<NewBasicBlock *> ms = n->getSuccs();
        for (auto &m : ms) {
          // outs()<<*(m->getId());
          phi->propagate(m, x, fy, worklist);
        }
      } else if (n->isCallBb()) {
        NewBasicBlock *ep = n->getCalleeCfg()->getExit();
        if (phi->getEnv(ep, y) != NULL) {
          Env z = *(phi->getEnv(ep, y));
          NewBasicBlock *m =
              n->getSuccs().front(); // should only contain 1 element
          phi->propagate(m, x, z, worklist);
        } else {
          NewBasicBlock *rp = n->getCalleeCfg()->getEntry();
          /*experimenting below*/
          vector<Value *> actualArgs = n->getArgs();
          Function *calledFun = n->getCalleeCfg()->getFun();
          int index = 0; // actualArgs.size();
          for (auto it = calledFun->arg_begin(); it != calledFun->arg_end();
               it++) {
            Value *formal = it;
            y.set(formal, y.lookup(actualArgs[index]));
            index++;
          }
          /*experimenting above*/
          // outs()<<"callee:"<<n->getCalleeCfg()->getName()<<"\n";
          phi->propagate(rp, y, y, worklist);
        }
      } else { // n->isRetBb()
        // 0. remove locals
        // 1. get calling BB c
        // 2. find all u where phi->getEnv(c,u) == x
        // 3. phi->propagate(m, u, y, worklist);

        Function *calledFun = ((BasicBlock *)n->getId())->getParent();
        for (auto it = calledFun->arg_begin(); it != calledFun->arg_end();
             it++) {
          Value *formal = it;
          x.remove(formal);
          y.remove(formal);
        }

        vector<NewBasicBlock *> callingBbs = n->getCallingBbs();
        for (auto &c : callingBbs) {
          if (phi->getMidEnv(c, x) != NULL) {
            // outs()<<"callingbb:"<<*(c->getId());
            Env u = *(phi->getMidEnv(c, x));
            NewBasicBlock *m =
                c->getSuccs().front(); // should only contain 1 element
            phi->propagate(m, u, y, worklist);
          }
        }
      }
    }
    computeXn(phi);
  }

  void computeXn(Phi *phi) {
    // iterate through all bbs in all cfgs
    // when see printf, get the value
    vector<NewCfg *> ncfgs = nmod->getCfgs();
    for (auto &ncfg : ncfgs) {
      vector<NewBasicBlock *> nbbs = ncfg->getnbbs();
      for (auto &nbb : nbbs) {
        Env xn = phi->computeXn(nbb);
        BasicBlock *bb = (BasicBlock *)(nbb->getId());
        for (auto i = bb->begin(); i != bb->end(); i++) {
          CallInst *ci = dyn_cast<CallInst>(i);
          if (ci) {
            const DebugLoc &Loc = ci->getDebugLoc();
            // outs() << (ins)->getName() << "\n";
            unsigned line = Loc.getLine();
            unsigned col = Loc.getCol();
            if (ci->getCalledFunction()->getName() == "printf") {
              Value *v = ci->getArgOperand(1);
              Interval itv = xn.lookup(v);
              cout << line << ":" << col << " " << ncfg->getName()
                   << " printf:" << itv << endl;
            }
          }
        }
      }
    }
    /*
    vector<NewBasicBlock*> mainNbbs = nmod->getCfg("main")->getnbbs();
    for(auto &nbb: mainNbbs){
        Env xn = *(phi->getEnv(nbb, Env())); //TODO;for main function, no
    combine for now
        BasicBlock* bb = (BasicBlock*)(nbb->getId());
        for(auto i=bb->begin();i!=bb->end();i++){
            CallInst* ci = dyn_cast<CallInst>(i);
            if(ci){
                if (ci->getCalledFunction()->getName()=="printf"){
                    Value* v = ci->getArgOperand(1);
                    Interval itv = xn.lookup(v);
                    cout<<"printf:"<<itv<<endl;
                }
            }
        }
    }
    */
  }
};

#endif
