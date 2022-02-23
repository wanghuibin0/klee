#include "env.h"
#include "module.h"
#include "inter_fix_point_iterator.h"
#include "headers.h"

class InterProceduralPass : public ModulePass {

public:
  static char ID;
  NewModule *nmod;

  InterProceduralPass() : ModulePass(ID) { nmod = new NewModule(); }
  ~InterProceduralPass() {}

  virtual bool runOnModule(Module &m) {
    // cout << "Module " << m.getModuleIdentifier() << endl;
    /*****************add global variable to the env**************/
    for (auto it = m.global_begin(); it != m.global_end(); it++) {
      ConstantInt *c = dyn_cast<ConstantInt>(it->getInitializer());
      if (c) {
        int64_t value = c->getSExtValue();
        Interval i = Interval(value, value);
        globalEnv.set(&*it, i);
      }
    }
    for (Module::iterator mi = m.begin(), me = m.end(); mi != me; ++mi) {
      runOnFunction(*mi);
    }
    // cout<<nmod->size()<<endl;
    buildCallLinks();

    InterFixPointIter ifpi(nmod);
    ifpi.run();
    return false;
  }

  // virtual void getAnalysisUsage(AnalysisUsage &au) const {
  //    au.setPreservesAll();
  //}

  virtual bool runOnFunction(Function &f) {
    /*******************split call and return instructions*******************/
    set<Instruction *> splitedIns;
    for (auto it = f.begin(); it != f.end(); it++) {
      BasicBlock *b = (BasicBlock *)&*it;
      for (auto i = b->begin(); i != b->end(); i++) {
        Instruction *ins = (Instruction *)&*i;
        if (isa<CallInst>(ins) && splitedIns.find(ins) == splitedIns.end()) {
          splitedIns.insert(ins);
          // BasicBlock *tmp = b->splitBasicBlock(ins);
          // outs()<<*tmp;
          break;
        }
        if (isa<ReturnInst>(ins) && splitedIns.find(ins) == splitedIns.end()) {
          splitedIns.insert(ins);
          b->splitBasicBlock(ins);
          break;
        }

        auto idup = i;
        idup++;
        Instruction *insnext = (Instruction *)&*idup;
        if (isa<CallInst>(ins) && splitedIns.find(ins) != splitedIns.end() &&
            idup != b->end() && !isa<CallInst>(insnext)) {
          // BasicBlock *tmp = b->splitBasicBlock(insnext);
          // outs()<<*tmp;
          break;
        }
      }
    }

    /*******************split the false branch edge*******************/
    for (auto it = f.begin(); it != f.end(); it++) {
      BasicBlock *bb = (BasicBlock *)&*it;
      TerminatorInst *ti = bb->getTerminator();
      if (isa<BranchInst>(ti) && ti->getNumOperands() == 3) {
        BasicBlock *bbf = (BasicBlock *)(ti->getOperand(1)); // TODO: the order
                                                             // is reversed....
        SplitEdge(bb, bbf, this); // not sure about this, seems working TODO
      }
    }
    // f.viewCFG();

    if (f.isDeclaration()) {
      return false;
    } // no cfg for external function

    /*******************build cfg*******************/
    vector<BasicBlock *> vec;
    for (auto it = f.begin(); it != f.end(); it++) {
      BasicBlock *b = (BasicBlock *)&*it;
      vec.push_back(b);
    }
    string name = f.getName();
    // cout << name <<" "<<f.isDeclaration()<< endl;
    NewCfg *mycfg = new NewCfg(vec, name, &f);
    for (scc_iterator<Function *> I = scc_begin(&f), IE = scc_end(&f); I != IE;
         ++I) {
      mycfg->addScc(*I);
    }
    // mycfg->printsccs();

    /*******************build module*******************/
    nmod->addCfg(mycfg);
    return false;
  }

  void buildCallLinks() {
    vector<NewCfg *> ncfgs = nmod->getCfgs();
    for (auto &ncfg : ncfgs) {
      vector<NewBasicBlock *> nbbs = ncfg->getnbbs();
      for (auto &nbb : nbbs) {
        // cout<<"nbb:"<<nbb->isCallBb()<<endl;
        if (nbb->isCallBb()) {
          // cout<<"callbb:"<<endl;
          // caller
          BasicBlock *bb = (BasicBlock *)(nbb->getId());
          CallInst *ci = dyn_cast<CallInst>(&(bb->front()));
          string calleeName = ci->getCalledFunction()->getName();
          // cout<<"callee:"<<calleeName<<endl;
          NewCfg *calleeCfg = nmod->getCfg(calleeName);
          nbb->setCalleeCfg(calleeCfg);

          // callee
          NewBasicBlock *calleeExitBb = calleeCfg->getExit();
          calleeExitBb->addCallingBb(nbb);
          // outs()<<*(calleeExitBb->getId())<<"size:"<<calleeExitBb->getCallingBbs().size();
        }
      }
    }
  }

}; // class InterProceduralPass


char InterProceduralPass::ID = 0;
