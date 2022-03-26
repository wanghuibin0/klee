#ifndef TRANSFER_H
#define TRANSFER_H

#include "MyDebug.h"

#include "Env.h"
#include "EnvMap.h"
#include "EqRels.h"
#include "Transfer.h"
#include "klee/Support/Debug.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

namespace klee {

class Transfer {
  llvm::BasicBlock *bb;
  EnvMap &gEnvMap;
  llvm::LoopInfo &LI;
  std::map<llvm::BasicBlock *, Env> succEnvs; // envs formed by conditionals

  Env internalEnv;

public:
  Transfer(llvm::BasicBlock *bb, EnvMap &envMap, llvm::LoopInfo &LI)
      : bb(bb), gEnvMap(envMap), LI(LI) {
    if (this->bb != &this->bb->getParent()->getEntryBlock()) {
      collectEnvIn();
    } else {
      internalEnv = gEnvMap.getEnv(nullptr, bb);
    }
  }

  void execute() {
    /* MY_KLEE_DEBUG(internalEnv.dump(llvm::errs())); */
    for (auto &&i : *bb) {
      executeInstruction(&i);
    }
    updateEnvOut();
  }

  Env executeToInst(llvm::Instruction *inst) {
    assert(inst->getParent() == bb && "inst must be in bb");
    /* MY_KLEE_DEBUG( gEnvMap.dump() ); */
    /* MY_KLEE_DEBUG( llvm::errs() << "bb: " << bb << "\n" << *bb ); */
    /* MY_KLEE_DEBUG( internalEnv.dump(llvm::errs()) ); */

    // env of entry block is always empty, so it should be excluded
    bool isEntryBlock = (bb == &bb->getParent()->getEntryBlock());
    if (!isEntryBlock && internalEnv.empty()) {
      return Env();
    }

    for (auto &&i : *bb) {
      if (&i == inst) {
        return internalEnv;
      } else {
        executeInstruction(&i);
      }
    }
    assert(0 && "must escape from the calling inst");
  }

private:
  /// this should be called before executing the block. (in constructor)
  /// worth noting that this is not appliable to entry block.
  void collectEnvIn() {
    MY_KLEE_DEBUG(llvm::errs() << "collect env comming in bb: " << bb << "\n");
    if (LI.isLoopHeader(bb)) {
      collectEnvForLoopheader();
    } else {
      for (auto &&b : predecessors(bb)) {
        auto &env = gEnvMap.getEnv(b, bb);
        internalEnv = internalEnv | env;
      }
    }
  }

  // do widening for loop header
  void collectEnvForLoopheader() {
    // resolve which edge is back edge
    llvm::BasicBlock *latchBB = nullptr;
    llvm::BasicBlock *preHeader = nullptr;
    unsigned numPreds = 0;
    for (auto &&b : predecessors(bb)) {
      ++numPreds;
      if (isLoopBackEdge(b, bb)) {
        latchBB = b;
      } else {
        preHeader = b;
      }
    }
    assert(numPreds == 2 &&
           "the number of predecessors of loopheader is not 2");
    assert(latchBB != nullptr && "there must be a back edge");

    Env &latchEnv = gEnvMap.getEnv(latchBB, bb);
    Env &preEnv = gEnvMap.getEnv(preHeader, bb);
    if (!preEnv.isSubsetOf(latchEnv)) {
      internalEnv = preEnv;
    } else {
      internalEnv = preEnv || latchEnv; // widening
    }
  }

  /// this should be called after executing the block.
  void updateEnvOut() {
    MY_KLEE_DEBUG(llvm::errs() << "updating env out of bb: " << bb << "\n");
    assert(succEnvs.size() == 0 || succEnvs.size() == 2);
    if (succEnvs.size() == 0) {
      // only has one successor
      llvm::BasicBlock *b = bb->getUniqueSuccessor();
      gEnvMap.mergeAndUpdate(bb, b, internalEnv);
    } else {
      // has two successors
      updateEnvOutBr();
    }
  }

  /// update env for two successive blocks
  void updateEnvOutBr() {
    for (auto &&sit : succEnvs) {
      llvm::BasicBlock *b = sit.first;
      Env envBr = mergeConditionalEnv(sit.second);

      gEnvMap.update(bb, b, envBr);
    }
  }

  bool isLoopBackEdge(llvm::BasicBlock *from, llvm::BasicBlock *to) {
    llvm::Loop *loop = LI.getLoopFor(from);
    return loop && loop->isLoopLatch(from) && LI.isLoopHeader(to);
  }

  /// merge the env generated by conditionals, return the merged env.
  Env mergeConditionalEnv(const Env &condEnv) {
    Env res{internalEnv};
    for (auto &&it : internalEnv) {
      llvm::Value *v = it.first;
      Interval i = it.second;
      if (condEnv.hasValue(v)) {
        Interval condI = condEnv.lookup(v);
        if (condI.isBot()) {
          res.clear();
          break;
        }
        res.set(v, i & condI);
      }
    }
    return res;
  }

  /// change env according to the instruction
  void executeInstruction(llvm::Instruction *inst) {
    MY_KLEE_DEBUG(llvm::errs() << "execute instruction:\n");
    MY_KLEE_DEBUG(llvm::errs() << *inst << "\n");
    llvm::Type *type = inst->getType();
    MY_KLEE_DEBUG(llvm::errs() << "inst type = " << *type << "\n");
    if (llvm::isa<llvm::StoreInst>(inst)) {
      llvm::StoreInst *i = llvm::cast<llvm::StoreInst>(inst);
      llvm::Value *v = i->getValueOperand();
      llvm::Value *p = i->getPointerOperand();
      if (!v->getType()->isIntegerTy()) {
        return;
      }
      if (auto *c = llvm::dyn_cast<llvm::ConstantInt>(v)) {
        auto value = c->getSExtValue();
        Interval i(value, value);
        internalEnv.set(p, i);
      } else {
        assert(internalEnv.hasValue(v));
        internalEnv.set(p, internalEnv.lookup(v));
      }
    } else if (llvm::isa<llvm::LoadInst>(inst)) {
      llvm::LoadInst *i = llvm::cast<llvm::LoadInst>(inst);
      if (!i->getType()->isIntegerTy()) {
        return;
      }
      llvm::Value *p = i->getPointerOperand();
      if (internalEnv.hasValue(p)) {
        auto v = internalEnv.lookup(p);
        internalEnv.set(inst, v);
      } else {
        internalEnv.set(inst, Interval::Top());
      }
    } /*else if (llvm::isa<llvm::BitCastInst>(inst)) {
      llvm::BitCastInst *i = llvm::cast<llvm::BitCastInst>(inst);
      llvm::Value *src = i->getOperand(0);
      assert(internalEnv.hasValue(src));
      Interval iv = internalEnv.lookup(src);
      internalEnv.set(inst, iv);
    }*/
    else if (llvm::isa<llvm::BinaryOperator>(inst)) {
      executeBinaryOp(llvm::cast<llvm::BinaryOperator>(inst));
    } else if (llvm::isa<llvm::CmpInst>(inst)) {
      executeCmpInst(llvm::cast<llvm::CmpInst>(inst));
    } else if (llvm::isa<llvm::CallInst>(inst)) {
      executeCallInst(llvm::cast<llvm::CallInst>(inst));
    } else {
      executeOtherInst(inst);
    }
  }

  void executeBinaryOp(llvm::BinaryOperator *binst) {
    llvm::Value *op1 = binst->getOperand(0);
    llvm::Value *op2 = binst->getOperand(1);
#if 1
    auto getInterval = [&](llvm::Value *op) {
      MY_KLEE_DEBUG(llvm::errs() << "get interval: " << *op << "\n";);
      if (llvm::ConstantInt *c = llvm::dyn_cast<llvm::ConstantInt>(op)) {
        auto v = c->getSExtValue();
        return Interval(v, v);
      } else if (llvm::isa<llvm::ConstantExpr>(op)) {
        return Interval::Top();
      } else {
        Interval res = internalEnv.lookup(op);
        return res;
      }
    };

    auto opcode = binst->getOpcode();
    Interval i;
    switch (opcode) {
    case llvm::Instruction::Add:
    case llvm::Instruction::Sub:
    case llvm::Instruction::Mul:
    case llvm::Instruction::SDiv: {
      Interval i1 = getInterval(op1);
      Interval i2 = getInterval(op2);
      i = apply(opcode, i1, i2);
      break;
    }
    default: {
      i = Interval::Top();
      break;
    }
    }
    internalEnv.set(binst, i);
#else

    llvm::ConstantInt *c1 = llvm::dyn_cast<llvm::ConstantInt>(op1);
    llvm::ConstantInt *c2 = llvm::dyn_cast<llvm::ConstantInt>(op2);

    auto opcode = binst->getOpcode();
    Interval i;
    switch (opcode) {
    case llvm::Instruction::Add:
    case llvm::Instruction::Sub:
    case llvm::Instruction::Mul:
    case llvm::Instruction::SDiv:
      if (c1 && c2) {
        auto v1 = c1->getSExtValue();
        auto v2 = c2->getSExtValue();
        i = apply(opcode, Interval(v1, v1), Interval(v2, v2));
      } else if (c1 && !c2) {
        auto v1 = c1->getSExtValue();
        i = apply(opcode, Interval(v1, v1), internalEnv.lookup(op2));
      } else if (!c1 && c2) {
        auto v2 = c2->getSExtValue();
        i = apply(opcode, internalEnv.lookup(op1), Interval(v2, v2));
      } else {
        i = apply(opcode, internalEnv.lookup(op1), internalEnv.lookup(op2));
      }
      break;
    default:
      i = Interval::Top();
    }
    internalEnv.set(binst, i);
#endif
  }

  /// apply add/sub/mul/div directly on two intervals
  Interval apply(unsigned opcode, Interval i1, Interval i2) {
    Interval ret;
    switch (opcode) {
    case llvm::Instruction::Add:
      ret = i1 + i2;
      break;
    case llvm::Instruction::Sub:
      ret = i1 - i2;
      break;
    case llvm::Instruction::Mul:
      ret = i1 * i2;
      break;
    case llvm::Instruction::SDiv:
      ret = i1 / i2;
      break;
    }
    return ret;
  }

  void trackBlockEqrels(EqRels &eqrels) {
    for (auto &&i : *bb) {
      if (llvm::isa<llvm::LoadInst>(&i)) {
        llvm::Value *ptr = llvm::cast<llvm::LoadInst>(&i)->getPointerOperand();
        if (i.getType()->isIntegerTy())
          continue;
        eqrels.processLoad(ptr, &i);
      } else if (llvm::isa<llvm::StoreInst>(&i)) {
        llvm::Value *ptr = llvm::cast<llvm::StoreInst>(&i)->getPointerOperand();
        llvm::Value *val = llvm::cast<llvm::StoreInst>(&i)->getValueOperand();
        if (val->getType()->isIntegerTy())
          continue;
        eqrels.processStore(ptr, val);
      } /* else if (llvm::isa<llvm::BitCastInst>(&i)) {
         llvm::Value *ptr = llvm::cast<llvm::BitCastInst>(&i)->getOperand(0);
         eqrels.processBitCast(ptr, &i);
       }*/
    }
  }

  void executeCmpInst(llvm::CmpInst *binst) {
    llvm::Value *op1 = binst->getOperand(0);
    llvm::ConstantInt *c1 = llvm::dyn_cast<llvm::ConstantInt>(op1);
    llvm::Value *op2 = binst->getOperand(1);
    llvm::ConstantInt *c2 = llvm::dyn_cast<llvm::ConstantInt>(op2);

    bool isRegReg =
        !llvm::isa<llvm::Constant>(op1) && !llvm::isa<llvm::Constant>(op2) &&
        op1->getType()->isIntegerTy() && op2->getType()->isIntegerTy();

    llvm::TerminatorInst *ti = bb->getTerminator();
    if (!llvm::isa<llvm::BranchInst>(ti))
      return;
    if (ti->getNumOperands() != 3)
      return;

    llvm::BasicBlock *bbf = llvm::cast<llvm::BasicBlock>(ti->getOperand(1));
    llvm::BasicBlock *bbt = llvm::cast<llvm::BasicBlock>(ti->getOperand(2));

    EqRels eqrels;
    trackBlockEqrels(eqrels);

    llvm::CmpInst::Predicate p = binst->getPredicate();

    if (c1 && !c2) {
      applyCmpValReg(p, c1, op2, bbt, bbf, eqrels);
    } else if (!c1 && c2) {
      applyCmpRegVal(p, op1, c2, bbt, bbf, eqrels);
    } else if (isRegReg) {
      applyCmpRegReg(p, op1, op2, bbt, bbf, eqrels);
    } else {
      applyCmpOthers(p, op1, op2, bbt, bbf, eqrels);
    }
  }

  void applyCmpOthers(llvm::CmpInst::Predicate p, llvm::Value *c1,
                      llvm::Value *op2, llvm::BasicBlock *bbt,
                      llvm::BasicBlock *bbf, EqRels &eqrels) {}

  void applyCmpValReg(llvm::CmpInst::Predicate p, llvm::ConstantInt *c1,
                      llvm::Value *op2, llvm::BasicBlock *bbt,
                      llvm::BasicBlock *bbf, EqRels &eqrels) {
    switch (p) {
    case llvm::CmpInst::Predicate::ICMP_SLT:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_SGT, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_ULT:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_UGT, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_SLE:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_SGE, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_ULE:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_UGE, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGT:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_SLT, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_UGT:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_ULT, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGE:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_SLE, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_UGE:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_ULE, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_EQ:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_EQ, op2, c1, bbt, bbf,
                     eqrels);
      break;
    case llvm::CmpInst::Predicate::ICMP_NE:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_NE, op2, c1, bbt, bbf,
                     eqrels);
      break;
    default:
      assert(0 && "only handle integer comparison");
      break;
    }
  }

  void applyCmpRegVal(llvm::CmpInst::Predicate p, llvm::Value *op1,
                      llvm::ConstantInt *c2, llvm::BasicBlock *bbt,
                      llvm::BasicBlock *bbf, EqRels &eqrels) {
    auto value2 = c2->getSExtValue();

    Interval ipredicateT;
    Interval ipredicateF;
    switch (p) {
    case llvm::CmpInst::Predicate::ICMP_SLT:
      ipredicateT = Interval(MIN, value2 - 1);
      ipredicateF = Interval(value2, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_ULT:
      ipredicateT = Interval(0, value2 - 1);
      ipredicateF = Interval(value2, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_SLE:
      ipredicateT = Interval(MIN, value2);
      ipredicateF = Interval(value2 + 1, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_ULE:
      ipredicateT = Interval(0, value2);
      ipredicateF = Interval(value2 + 1, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGT:
      ipredicateT = Interval(value2 + 1, MAX);
      ipredicateF = Interval(MIN, value2);
      break;
    case llvm::CmpInst::Predicate::ICMP_UGT:
      ipredicateT = Interval(value2 + 1, MAX);
      ipredicateF = Interval(0, value2);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGE:
      ipredicateT = Interval(value2, MAX);
      ipredicateF = Interval(MIN, value2 - 1);
      break;
    case llvm::CmpInst::Predicate::ICMP_UGE:
      ipredicateT = Interval(value2, MAX);
      ipredicateF = Interval(0, value2 - 1);
      break;
    case llvm::CmpInst::Predicate::ICMP_EQ:
      ipredicateT = Interval(value2, value2);
      ipredicateF = Interval(MIN, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_NE:
      ipredicateT = Interval(MIN, MAX);
      ipredicateF = Interval(value2, value2);
      break;
    default:
      assert(0 && "only cares about int comparison");
      break;
    }

    // merge the original interval of op1 and set its interval for the
    // successive bbs.
    assert(internalEnv.hasValue(op1));
    Interval i1 = internalEnv.lookup(op1);

    Env envT, envF;
    genEnvsAtCond(envT, envF, eqrels, op1, i1 & ipredicateT, i1 & ipredicateF);
    succEnvs.insert(std::make_pair(bbt, envT));
    succEnvs.insert(std::make_pair(bbf, envF));
  }

  void genEnvsAtCond(Env &envT, Env &envF, EqRels &eqrels, llvm::Value *condOp,
                     Interval predT, Interval predF) {
    std::vector<Pointer> &vecP = eqrels.getRelatedPointers(condOp);
    for (auto &&p : vecP) {
      std::vector<VRegs> &vecV = eqrels.getRelatedVRegs(p);
      envT.set(p, predT);
      envF.set(p, predF);
      for (auto &&v : vecV) {
        envT.set(v, predT);
        envF.set(v, predF);
      }
    }
  }

  // TODO
  void applyCmpRegReg(llvm::CmpInst::Predicate p, llvm::Value *op1,
                      llvm::Value *op2, llvm::BasicBlock *bbt,
                      llvm::BasicBlock *bbf, EqRels &eqrels) {

    auto valueLower = internalEnv.lookup(op2).getLower();
    auto valueUpper = internalEnv.lookup(op2).getUpper();

    Interval ipredicateT;
    Interval ipredicateF;
    switch (p) {
    case llvm::CmpInst::Predicate::ICMP_SLT:
      ipredicateT = Interval(MIN, valueLower - 1);
      ipredicateF = Interval(valueUpper, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_ULT:
      ipredicateT = Interval(0, valueLower - 1);
      ipredicateF = Interval(valueUpper, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_SLE:
      ipredicateT = Interval(MIN, valueLower);
      ipredicateF = Interval(valueUpper + 1, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_ULE:
      ipredicateT = Interval(0, valueLower);
      ipredicateF = Interval(valueUpper + 1, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGT:
      ipredicateT = Interval(valueUpper + 1, MAX);
      ipredicateF = Interval(MIN, valueLower);
      break;
    case llvm::CmpInst::Predicate::ICMP_UGT:
      ipredicateT = Interval(valueUpper + 1, MAX);
      ipredicateF = Interval(0, valueLower);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGE:
      ipredicateT = Interval(valueUpper, MAX);
      ipredicateF = Interval(MIN, valueLower - 1);
      break;
    case llvm::CmpInst::Predicate::ICMP_UGE:
      ipredicateT = Interval(valueUpper, MAX);
      ipredicateF = Interval(0, valueLower - 1);
      break;
    case llvm::CmpInst::Predicate::ICMP_EQ:
      ipredicateT = Interval(valueLower, valueUpper);
      ipredicateF = Interval(MIN, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_NE:
      ipredicateT = Interval(MIN, MAX);
      ipredicateF = Interval(valueLower, valueUpper);
      break;
    default:
      assert(0 && "only cares about int comparison\n");
    }

    assert(internalEnv.hasValue(op1));
    Interval i1 = internalEnv.lookup(op1);

    Env envT, envF;
    genEnvsAtCond(envT, envF, eqrels, op1, i1 & ipredicateT, i1 & ipredicateF);
    succEnvs.insert(std::make_pair(bbt, envT));
    succEnvs.insert(std::make_pair(bbf, envF));
  }

  void executeCallInst(llvm::CallInst *cinst) {
    EqRels eqrels;
    trackBlockEqrels(eqrels);

    llvm::Function *f = cinst->getCalledFunction();
    if (f) {
      llvm::StringRef name = f->getName();
      if (name == "klee_make_symbolic") {
        llvm::Value *firstArg = cinst->getArgOperand(0);
        internalEnv.set(firstArg, Interval::Top());
      }
    }

    if (cinst->getType()->isIntegerTy() || cinst->getType()->isPointerTy()) {
      internalEnv.set(cinst, Interval::Top());
    }
  }

  void executeOtherInst(llvm::Instruction *inst) {
    if (inst->getType()->isIntegerTy() || inst->getType()->isPointerTy()) {
      internalEnv.set(inst, Interval::Top());
    }
    MY_KLEE_DEBUG(llvm::errs() << "other llvm insts, ignoring\n");
  }
};
} // namespace klee

#endif