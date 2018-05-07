// encapsulation of llvm BasicBlock class

#ifndef BASICBLOCK_H
#define BASICBLOCK_H

#include "env.h"
#include "cfg.h"
#include "headers.h"

class NewCfg;

class NewBasicBlock {
private:
  BasicBlock *bb; // llvm BasicBlock
  Env prev;
  Env post;
  Env cstEnv; // for branches
  Value *id;
  vector<NewBasicBlock *> preds;
  vector<NewBasicBlock *> succs;
  bool isCall;
  NewCfg *calleeCfg;
  vector<Value *> args;
  bool isRet;
  vector<NewBasicBlock *> callingBbs;

public:
  /*constructor, also checks if the block is a call block or return block */
  NewBasicBlock(BasicBlock *bb) : bb(bb), id(bb) {
    isCall = false;
    isRet = false;

    CallInst *ci = dyn_cast<CallInst>(&(bb->front()));
    if (ci && !ci->getCalledFunction()->isDeclaration()) { // treat external
                                                           // function call as
                                                           // normal bb
      isCall = true;
      // cout<<"callinst"<<endl;
      // outs()<<*ci;
    }
    ReturnInst *ri = dyn_cast<ReturnInst>(&(bb->back()));
    if (ri) {
      isRet = true;
      // cout<<"retinst"<<endl;
      // outs()<<*ri;
    }
  }
  NewBasicBlock() : id(NULL), isCall(false), isRet(false) {}

  bool isCallBb() { return isCall; }
  NewCfg *getCalleeCfg() { return calleeCfg; }
  void setCalleeCfg(NewCfg *calleeCfg) { this->calleeCfg = calleeCfg; }
  vector<Value *> getArgs() {
    CallInst *ci = dyn_cast<CallInst>(&(bb->front()));
    int num = ci->getNumArgOperands();
    vector<Value *> ret;
    for (int i = 0; i < num; i++) {
      ret.push_back(ci->getArgOperand(i));
    }
    return ret;
  }
  bool isRetBb() { return isRet; }
  vector<NewBasicBlock *> getCallingBbs() { return callingBbs; }
  void addCallingBb(NewBasicBlock *callingBb) {
    this->callingBbs.push_back(callingBb);
  }

  void setBB(BasicBlock *bb) {
    this->bb = bb;
    id = bb;
  }

  Value *getId() { return id; }

  Env getPrevEnv() { return prev; }
  void setPrevEnv(Env prev) { this->prev = prev; }
  Env getPostEnv() { return post; }
  void setCstEnv(Env cstEnv) { this->cstEnv = cstEnv; }

  void addEdge(NewBasicBlock *other) {
    if (std::find(succs.begin(), succs.end(), other) == succs.end()) {
      succs.push_back(other);
      // cout<<"addedge"<<succs.size()<<endl;
    }
    if (std::find(other->preds.begin(), other->preds.end(), this) ==
        other->preds.end()) {
      other->preds.push_back(this);
    }
  }

  // get predecessors
  vector<NewBasicBlock *> getPreds() { return preds; }

  // get successors
  vector<NewBasicBlock *> getSuccs() { return succs; }

  // get all the envs from all the predecessors
  vector<Env> preEnvs() {
    vector<Env> preEnvs;
    for (NewBasicBlock *pred : preds) {
      preEnvs.push_back(pred->getPostEnv());
    }
    return preEnvs;
  }

  /*
   * if all envs are empty, then the result is empty,
   * which means bottom by default constructor of Interval
   */
  Env combineEnvs(vector<Env> &envs) {
    Env newEnv;
    // newEnv.setValid(false);//init to false//TODO
    // outs()<<"combine:"<<*(bb);
    for (auto &env : envs) {
      newEnv = newEnv | env;
      // env.print();
    }
    // for (auto const &env: envs){
    //    if(env.size()>0){
    //        for (auto & pair:env){
    //            Value* v = pair.first;
    //            Interval i = pair.second;
    //            if(newEnv.find(v)==newEnv.end()){//newEnv doesn't have the
    // value
    //                newEnv[v] = i;
    //            }else{
    //                Interval newi = newEnv[v] | i;
    //                newEnv[v] = newi;
    //            }
    //        }
    //    }
    //}
    return newEnv;
  }

  /*
   *Instruction::Add Sub Mul SDiv
   */
  void executeBinaryOp(Instruction *ins, bool isEqual) {
    Value *op1 = ins->getOperand(0);
    ConstantInt *c1 = dyn_cast<ConstantInt>(op1);
    Value *op2 = ins->getOperand(1);
    ConstantInt *c2 = dyn_cast<ConstantInt>(op2);

    unsigned opcode = ins->getOpcode();
    if (c1 && c2) { // both are const
      int64_t value1 = c1->getSExtValue();
      int64_t value2 = c2->getSExtValue();
      Interval i =
          apply(opcode, Interval(value1, value1), Interval(value2, value2));
      post.set(ins, i);
    } else if (c1 && !c2) { // c2 is reg
      int64_t value1 = c1->getSExtValue();
      Interval i1(value1, value1);
      Interval newI = apply(opcode, i1, post.lookup(op2));
      post.set(ins, newI);
    } else if (!c1 && c2) { // c1 is reg
      int64_t value2 = c2->getSExtValue();
      Interval i2(value2, value2);
      // Interval newI = post.lookup(op1) + i2 ;
      Interval newI = apply(opcode, post.lookup(op1), i2);
      post.set(ins, newI);
    } else { // c1 and c2 are reg
      // Interval i = post.lookup(op1) + post.lookup(op2) ;
      if (isEqual && (opcode == Instruction::Sub)) { // TODO division
        Interval i = Interval(0, 0);
        post.set(ins, i);
      } else {
        Interval i = apply(opcode, post.lookup(op1), post.lookup(op2));
        post.set(ins, i);
      }
    }
  }

  Interval apply(unsigned opcode, Interval i1, Interval i2) {
    Interval ret;
    switch (opcode) {
    case Instruction::Add:
      ret = i1 + i2;
      break;
    case Instruction::Sub:
      ret = i1 - i2;
      break;
    case Instruction::Mul:
      ret = i1 * i2;
      break;
    case Instruction::SDiv:
      ret = i1 / i2;
      break;
    }
    return ret;
  }

  void executeCmpInst(Instruction *ins) {
    // outs()<<"compare:"<<*ins<<"\n";
    Value *op1 = ins->getOperand(0);
    ConstantInt *c1 = dyn_cast<ConstantInt>(op1);
    Value *op2 = ins->getOperand(1);
    ConstantInt *c2 = dyn_cast<ConstantInt>(op2);

    map<Value *, Value *> eqrel;
    for (auto &i : *(this->bb)) {
      if (isa<LoadInst>(&i)) {
        Value *ptr = ((LoadInst *)&i)->getPointerOperand();
        // cout()<<"ptr"<<ptr<<"\n";
        eqrel[&i] = ptr;
        // outs()<<&i<<"\n";
      }
    }

    /**/ TerminatorInst *ti = this->bb->getTerminator();
    myassert(isa<BranchInst>(ti), "terminator is not brachisnt");
    myassert((ti->getNumOperands()) == 3,
             "branchinst has fewer than 3 operands");
    // for(auto io=ti->op_begin();io!=ti->op_end();io++)
    //    outs()<<**io<<"\n";
    Value *bbf = ti->getOperand(1); // TODO: the order is reversed....
    Value *bbt = ti->getOperand(2);
    NewBasicBlock *nbbt = NULL;
    NewBasicBlock *nbbf = NULL;
    for (auto &nbb : succs) {
      if (nbb->getId() == bbt) {
        nbbt = nbb;
      }
      if (nbb->getId() == bbf) {
        nbbf = nbb;
      }
    }
    myassert(nbbt != NULL && nbbf != NULL, "branch block is null");
    // cout<<"tf\t"<<nbbt->getId()<<"\t"<<nbbf->getId()<<endl;

    CmpInst::Predicate p = ((CmpInst *)ins)->getPredicate();
    if (!c1 && c2) { // c1 is reg
      applyCmpRegVal(p, op1, c2, nbbt, nbbf, eqrel);
    } else if (c1 && !c2) { // c2 is reg
      applyCmpValReg(p, c1, op2, nbbt, nbbf, eqrel);
    } else { // both are register
      applyCmpRegReg(p, op1, op2, nbbt, nbbf, eqrel);
    }
  }

  void applyCmpRegReg(CmpInst::Predicate p, Value *op1, Value *op2,
                      NewBasicBlock *nbbt, NewBasicBlock *nbbf,
                      map<Value *, Value *> &eqrel) {
    Env cstEnvT;
    Env cstEnvF;
    cstEnvT.set(eqrel[op1], post.lookup(op1));
    cstEnvF.set(eqrel[op1], post.lookup(op1));
    int valueLower = post.lookup(op2).getLower();
    int valueUpper = post.lookup(op2).getUpper();

    Interval ipredicateT;
    Interval ipredicateF;
    switch (p) {
    case CmpInst::Predicate::ICMP_SLT:
      ipredicateT = Interval(INT_MIN, valueLower - 1);
      ipredicateF = Interval(valueUpper, INT_MAX);
      break;
    case CmpInst::Predicate::ICMP_SLE:
      ipredicateT = Interval(INT_MIN, valueLower);
      ipredicateF = Interval(valueUpper + 1, INT_MAX);
      break;
    case CmpInst::Predicate::ICMP_SGT:
      ipredicateT = Interval(valueUpper + 1, INT_MAX);
      ipredicateF = Interval(INT_MIN, valueLower);
      break;
    case CmpInst::Predicate::ICMP_SGE:
      ipredicateT = Interval(valueUpper, INT_MAX);
      ipredicateF = Interval(INT_MIN, valueLower - 1);
      break;
    case CmpInst::Predicate::ICMP_EQ:
      ipredicateT = Interval(valueLower, valueUpper);
      ipredicateF = Interval(INT_MIN, INT_MAX); // the false branch is always
                                                // Top; problems happens when eq
                                                // holds
      break;
    case CmpInst::Predicate::ICMP_NE:
      ipredicateT = Interval(INT_MIN, INT_MAX); // the true branch is always
                                                // Top; problems happens when eq
                                                // holds
      ipredicateF = Interval(valueLower, valueUpper);
      break;
    }
    Interval newiT = cstEnvT.lookup(eqrel[op1]) & ipredicateT;
    Interval newiF = cstEnvF.lookup(eqrel[op1]) & ipredicateF;
    cstEnvT.set(eqrel[op1], newiT);
    cstEnvF.set(eqrel[op1], newiF);
    // cout<<"op1:"<<eqrel[op1]<<"\n";
    // cstEnvT.print();
    // cstEnvF.print();
    nbbt->setCstEnv(cstEnvT);
    nbbf->setCstEnv(cstEnvF);
  }

  void applyCmpValReg(CmpInst::Predicate p, ConstantInt *c1, Value *op2,
                      NewBasicBlock *nbbt, NewBasicBlock *nbbf,
                      map<Value *, Value *> &eqrel) {
    switch (p) {
    case CmpInst::Predicate::ICMP_SLT:
      applyCmpRegVal(CmpInst::Predicate::ICMP_SGT, op2, c1, nbbt, nbbf, eqrel);
      break;
    case CmpInst::Predicate::ICMP_SLE:
      applyCmpRegVal(CmpInst::Predicate::ICMP_SGE, op2, c1, nbbt, nbbf, eqrel);
      break;
    case CmpInst::Predicate::ICMP_SGT:
      applyCmpRegVal(CmpInst::Predicate::ICMP_SLT, op2, c1, nbbt, nbbf, eqrel);
      break;
    case CmpInst::Predicate::ICMP_SGE:
      applyCmpRegVal(CmpInst::Predicate::ICMP_SLE, op2, c1, nbbt, nbbf, eqrel);
      break;
    case CmpInst::Predicate::ICMP_EQ:
      applyCmpRegVal(CmpInst::Predicate::ICMP_EQ, op2, c1, nbbt, nbbf, eqrel);
      break;
    case CmpInst::Predicate::ICMP_NE:
      applyCmpRegVal(CmpInst::Predicate::ICMP_NE, op2, c1, nbbt, nbbf, eqrel);
      break;
    }
  }

  void applyCmpRegVal(CmpInst::Predicate p, Value *op1, ConstantInt *c2,
                      NewBasicBlock *nbbt, NewBasicBlock *nbbf,
                      map<Value *, Value *> &eqrel) {
    Env cstEnvT;
    Env cstEnvF;
    cstEnvT.set(eqrel[op1], post.lookup(op1));
    cstEnvF.set(eqrel[op1], post.lookup(op1));
    int64_t value2 = c2->getSExtValue();

    Interval ipredicateT;
    Interval ipredicateF;
    switch (p) {
    case CmpInst::Predicate::ICMP_SLT:
      ipredicateT = Interval(INT_MIN, value2 - 1);
      ipredicateF = Interval(value2, INT_MAX);
      break;
    case CmpInst::Predicate::ICMP_SLE:
      ipredicateT = Interval(INT_MIN, value2);
      ipredicateF = Interval(value2 + 1, INT_MAX);
      break;
    case CmpInst::Predicate::ICMP_SGT:
      ipredicateT = Interval(value2 + 1, INT_MAX);
      ipredicateF = Interval(INT_MIN, value2);
      break;
    case CmpInst::Predicate::ICMP_SGE:
      ipredicateT = Interval(value2, INT_MAX);
      ipredicateF = Interval(INT_MIN, value2 - 1);
      break;
    case CmpInst::Predicate::ICMP_EQ:
      ipredicateT = Interval(value2, value2);
      ipredicateF = Interval(INT_MIN, INT_MAX); // the false branch is always
                                                // Top; problems happens when eq
                                                // holds
      break;
    case CmpInst::Predicate::ICMP_NE:
      ipredicateT = Interval(INT_MIN, INT_MAX); // the true branch is always
                                                // Top; problems happens when eq
                                                // holds
      ipredicateF = Interval(value2, value2);
      break;
    }
    Interval newiT = cstEnvT.lookup(eqrel[op1]) & ipredicateT;
    Interval newiF = cstEnvF.lookup(eqrel[op1]) & ipredicateF;
    cstEnvT.set(eqrel[op1], newiT);
    cstEnvF.set(eqrel[op1], newiF);
    // cout<<"op1:"<<eqrel[op1]<<"\n";
    // cstEnvT.print();
    // cstEnvF.print();
    nbbt->setCstEnv(cstEnvT);
    nbbf->setCstEnv(cstEnvF);
    // set invalid here?
  }

  // for now, only used for testing
  void executeCallInst(Instruction *ins) {
    return; // do nothing for now
    CallInst *ci = (CallInst *)ins;
    for (int i = 0; i < ci->getNumArgOperands(); i++) {
      Value *v = ci->getArgOperand(i);
      ConstantInt *c = dyn_cast<ConstantInt>(v);
      if (!c && ci->getCalledFunction()->getName() == "printf") {
        if (post.hasValue(v)) {
          Interval in = post.lookup(v);
          cout << "i:" << i << " " << in << endl;
        }
      }
    }
  }

  /*used for inter-procedural case 3; transform the pre env, return the result
   */
  Env execute(Env prev) {
    this->prev = prev;
    if (globaIsUsed == false) { // merge the globalEnv only once
      this->prev = this->prev | globalEnv;
      globaIsUsed = true;
    }
    execute();
    return this->post;
  }
  void execute() {
    if (cstEnv.size() > 0) { // what to do if there is a Bot
      for (auto it = cstEnv.begin(); it != cstEnv.end(); it++) {
        Value *v = it->first;
        Interval i = it->second;
        if (i.isBottom()) {
          prev.clear(); // remove everything
          prev.setValid(false);
          break;
        }
      }
    }

    post = prev;
    if (cstEnv.size() > 0) {
      for (auto it = cstEnv.begin(); it != cstEnv.end(); it++) {
        Value *v = it->first;
        Interval i = it->second;
        post.set(v, i);
      }
    }

    BasicBlock::iterator itb = bb->begin();
    for (itb; itb != bb->end(); itb++) {
      Instruction *ins = (Instruction *)itb;
      // outs()<<*ins<<"\n";
      if (isa<StoreInst>(ins)) { // StoreInst
        StoreInst *tmp = (StoreInst *)ins;
        Type *t = tmp->getValueOperand()->getType();
        Value *v = tmp->getValueOperand();
        Value *ptr = tmp->getPointerOperand();
        if (ConstantInt *c = dyn_cast<ConstantInt>(v)) {
          int64_t value = c->getSExtValue();
          Interval i = Interval(value, value);
          post.set(ptr, i);
        } else {
          post.set(ptr, post.lookup(v));
        }
      } else if (isa<LoadInst>(ins)) { // LoadInst
        Value *v = ((LoadInst *)ins)->getPointerOperand();
        post.set(ins, post.lookup(v));

        /**/ Instruction *insPeek = (Instruction *)(++itb);
        if (isa<StoreInst>(insPeek)) {
          StoreInst *tmp = (StoreInst *)insPeek;
          Value *ptr = tmp->getPointerOperand();
          // if(v->hasName() && ptr->hasName()){
          post.rels.addPair(v, ptr);
          //}
        }
        itb--;
                               /**/}
			      else if (isa < BinaryOperator > (ins))
			      {

        auto itb2 = itb;
        /**/ Instruction *insPre1 = (Instruction *)(--itb2);
        Instruction *insPre2 = (Instruction *)(--itb2);
        // outs()<<*insPre1<<"\n"<<*insPre2;
        bool isEqual = false;
        // if(isa<LoadInst>(insPre1) && isa<LoadInst>(insPre2)){
        //    Value *v1 = ((LoadInst*)insPre1)->getPointerOperand();
        //    Value *v2 = ((LoadInst*)insPre2)->getPointerOperand();
        //    isEqual = post.rels.hasPair(v1, v2);
        //    outs()<<*ins<<v1->getName()<<v2->getName()<<"\n";
        //    cout<<"here"<<isEqual<<endl;
        //}
        // itb++;
        // itb++;
        /**/ executeBinaryOp(ins, isEqual);
      } else if (isa<CallInst>(ins)) {
        executeCallInst(ins);
      } else if (isa<CmpInst>(ins)) {
        executeCmpInst(ins);
      }
    }
    // post.print();
  }

  void printEnv() {
    post.print();
    // map<Value*, Interval>::iterator it;
    // for(it = post.begin();it!=post.end();it++){
    //    cout<<it->first<<"\t"<<it->second<<endl;
    //}
  }
};

#endif
