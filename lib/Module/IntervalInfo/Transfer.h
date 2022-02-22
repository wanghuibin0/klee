#ifndef TRANSFER_H
#define TRANSFER_H

#include "Env.h"
#include "EnvMap.h"
#include "Transfer.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

namespace klee {

class Transfer {
  llvm::BasicBlock *bb;
  EnvMap &gEnvMap;

  Env internalEnv;

public:
  Transfer(llvm::BasicBlock *bb, EnvMap &envMap) : bb(bb), gEnvMap(envMap) {
    if (this->bb != &this->bb->getParent()->getEntryBlock()) {
      collectEnvIn();
    } else {
      internalEnv = gEnvMap.getEnv(nullptr, bb);
    }
  }

  void execute() {
    internalEnv.dump(llvm::outs());
    for (auto &&i : *bb) {
      executeInstruction(&i);
    }
    updateEnvOut();
  }

private:
  void collectEnvIn() {
    llvm::outs() << "collect env comming in bb: " << bb << "\n";
    for (auto &&b : predecessors(bb)) {
      auto &env = gEnvMap.getEnv(b, bb);
      internalEnv = internalEnv | env;
    }
  }
  void updateEnvOut() {
    llvm::outs() << "updating env out of bb: " << bb << "\n";
    for (auto &&b : successors(bb)) {
      Env envBr = computeEnvOfBranch(bb, b);
      // should merge because the branch instruction has generated some env there.
      gEnvMap.mergeAndUpdate(bb, b, envBr);
      gEnvMap.getEnv(bb, b).dump(llvm::outs());
    }
  }

  Env computeEnvOfBranch(llvm::BasicBlock *b0, llvm::BasicBlock *b1) {
    return internalEnv;
  }

  /// change env according to the instruction
  void executeInstruction(llvm::Instruction *inst) {
    llvm::outs() << "execute instruction:\n";
    llvm::outs() << *inst << "\n";
    if (llvm::isa<llvm::StoreInst>(inst)) {
      llvm::StoreInst *i = llvm::cast<llvm::StoreInst>(inst);
      llvm::Value *v = i->getValueOperand();
      llvm::Value *p = i->getPointerOperand();
      if (auto *c = llvm::dyn_cast<llvm::ConstantInt>(v)) {
        auto value = c->getSExtValue();
        ref<Interval> i(new Interval(value, value));
        internalEnv.set(p, i);
      } else {
        assert(internalEnv.hasValue(v));
        internalEnv.set(p, internalEnv.lookup(v));
      }
    } else if (llvm::isa<llvm::LoadInst>(inst)) {
      llvm::LoadInst *i = llvm::cast<llvm::LoadInst>(inst);
      llvm::Value *p = i->getPointerOperand();
      assert(internalEnv.hasValue(p));
      auto v = internalEnv.lookup(p);
      internalEnv.set(inst, v);
    } else if (llvm::isa<llvm::BinaryOperator>(inst)) {
      executeBinaryOp(llvm::cast<llvm::BinaryOperator>(inst));
    } else if (llvm::isa<llvm::CmpInst>(inst)) {
      executeCmpInst(llvm::cast<llvm::CmpInst>(inst));
    } else if (llvm::isa<llvm::CallInst>(inst)) {
      executeCallInst(llvm::cast<llvm::CallInst>(inst));
    } else {
      llvm::outs() << "other llvm insts, ignoring\n";
    }
  }

  void executeBinaryOp(llvm::BinaryOperator *binst) {
    llvm::Value *op1 = binst->getOperand(0);
    llvm::ConstantInt *c1 = llvm::dyn_cast<llvm::ConstantInt>(op1);
    llvm::Value *op2 = binst->getOperand(1);
    llvm::ConstantInt *c2 = llvm::dyn_cast<llvm::ConstantInt>(op2);

    auto opcode = binst->getOpcode();
    ref<Interval> i;
    if (c1 && c2) {
      auto v1 = c1->getSExtValue();
      auto v2 = c2->getSExtValue();
      i = apply(opcode, new Interval(v1, v1), new Interval(v2, v2));
    } else if (c1 && !c2) {
      auto v1 = c1->getSExtValue();
      i = apply(opcode, new Interval(v1, v1), internalEnv.lookup(op2)->deref());
    } else if (!c1 && c2) {
      auto v2 = c2->getSExtValue();
      i = apply(opcode, internalEnv.lookup(op1)->deref(), new Interval(v2, v2));
    } else {
      i = apply(opcode, internalEnv.lookup(op1)->deref(), internalEnv.lookup(op2)->deref());
    }
    internalEnv.set(binst, i);
  }

  /// apply add/sub/mul/div directly on two intervals
  ref<Interval> apply(unsigned opcode, ref<Interval> i1, ref<Interval> i2) {
    ref<Interval> ret;
    switch (opcode) {
    case llvm::Instruction::Add:
      ret = ref<Interval>(new Interval(*i1 + *i2));
      break;
    case llvm::Instruction::Sub:
      ret = ref<Interval>(new Interval(*i1 - *i2));
      break;
    case llvm::Instruction::Mul:
      ret = ref<Interval>(new Interval(*i1 * *i2));
      break;
    case llvm::Instruction::SDiv:
      ret = ref<Interval>(new Interval(*i1 / *i2));
      break;
    }
    return ret;
  }

  void executeCmpInst(llvm::CmpInst *binst) {
    llvm::Value *op1 = binst->getOperand(0);
    llvm::ConstantInt *c1 = llvm::dyn_cast<llvm::ConstantInt>(op1);
    llvm::Value *op2 = binst->getOperand(1);
    llvm::ConstantInt *c2 = llvm::dyn_cast<llvm::ConstantInt>(op2);

    llvm::TerminatorInst *ti = bb->getTerminator();
    llvm::BasicBlock *bbf = llvm::cast<llvm::BasicBlock>(ti->getOperand(1));
    llvm::BasicBlock *bbt = llvm::cast<llvm::BasicBlock>(ti->getOperand(2));

    llvm::CmpInst::Predicate p = binst->getPredicate();

    if (c1 && !c2) {
      applyCmpValReg(p, c1, op2, bbt, bbf);
    } else if (!c1 && c2) {
      applyCmpRegVal(p, op1, c2, bbt, bbf);
    } else if (!c1 && !c2) {
      applyCmpRegReg(p, op1, op2, bbt, bbf);
    } else {
      assert(0);
    }
  }

  void applyCmpValReg(llvm::CmpInst::Predicate p, llvm::ConstantInt *c1,
                      llvm::Value *op2, llvm::BasicBlock *bbt,
                      llvm::BasicBlock *bbf) {
    switch (p) {
    case llvm::CmpInst::Predicate::ICMP_SLT:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_SGT, op2, c1, bbt, bbf);
      break;
    case llvm::CmpInst::Predicate::ICMP_SLE:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_SGE, op2, c1, bbt, bbf);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGT:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_SLT, op2, c1, bbt, bbf);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGE:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_SLE, op2, c1, bbt, bbf);
      break;
    case llvm::CmpInst::Predicate::ICMP_EQ:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_EQ, op2, c1, bbt, bbf);
      break;
    case llvm::CmpInst::Predicate::ICMP_NE:
      applyCmpRegVal(llvm::CmpInst::Predicate::ICMP_NE, op2, c1, bbt, bbf);
      break;
    default:
      assert(0 && "only handle integer comparison");
      break;
    }
  }

  void applyCmpRegVal(llvm::CmpInst::Predicate p, llvm::Value *op1,
                      llvm::ConstantInt *c2, llvm::BasicBlock *bbt,
                      llvm::BasicBlock *bbf) {
    auto value2 = c2->getSExtValue();

    ref<Interval> ipredicateT;
    ref<Interval> ipredicateF;
    switch (p) {
    case llvm::CmpInst::Predicate::ICMP_SLT:
      ipredicateT = new Interval(MIN, value2 - 1);
      ipredicateF = new Interval(value2, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_SLE:
      ipredicateT = new Interval(MIN, value2);
      ipredicateF = new Interval(value2 + 1, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGT:
      ipredicateT = new Interval(value2 + 1, MAX);
      ipredicateF = new Interval(MIN, value2);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGE:
      ipredicateT = new Interval(value2, MAX);
      ipredicateF = new Interval(MIN, value2 - 1);
      break;
    case llvm::CmpInst::Predicate::ICMP_EQ:
      ipredicateT = new Interval(value2, value2);
      ipredicateF = new Interval(MIN, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_NE:
      ipredicateT = new Interval(MIN, MAX);
      ipredicateF = new Interval(value2, value2);
      break;
    default:
      assert(0 && "only cares about int comparison");
      break;
    }

    // merge the original interval of op1 and set its interval for the successive bbs.
    assert(internalEnv.hasValue(op1));
    ref<Interval> i1 = cast<Interval>(internalEnv.lookup(op1)->deref());

    gEnvMap.getEnv(bb, bbt).set(op1, new Interval(*i1 & *ipredicateT));
    gEnvMap.getEnv(bb, bbf).set(op1, new Interval(*i1 & *ipredicateF));
  }

  // TODO
  void applyCmpRegReg(llvm::CmpInst::Predicate p, llvm::Value *op1,
                      llvm::Value *op2, llvm::BasicBlock *bbt,
                      llvm::BasicBlock *bbf) {

    auto valueLower = cast<Interval>(internalEnv.lookup(op2)->deref())->getLower();
    auto valueUpper = cast<Interval>(internalEnv.lookup(op2)->deref())->getUpper();

    ref<Interval> ipredicateT;
    ref<Interval> ipredicateF;
    switch (p) {
    case llvm::CmpInst::Predicate::ICMP_SLT:
      ipredicateT = new Interval(MIN, valueLower - 1);
      ipredicateF = new Interval(valueUpper, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_SLE:
      ipredicateT = new Interval(MIN, valueLower);
      ipredicateF = new Interval(valueUpper + 1, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGT:
      ipredicateT = new Interval(valueUpper + 1, MAX);
      ipredicateF = new Interval(MIN, valueLower);
      break;
    case llvm::CmpInst::Predicate::ICMP_SGE:
      ipredicateT = new Interval(valueUpper, MAX);
      ipredicateF = new Interval(MIN, valueLower - 1);
      break;
    case llvm::CmpInst::Predicate::ICMP_EQ:
      ipredicateT = new Interval(valueLower, valueUpper);
      ipredicateF = new Interval(MIN, MAX);
      break;
    case llvm::CmpInst::Predicate::ICMP_NE:
      ipredicateT = new Interval(MIN, MAX);
      ipredicateF = new Interval(valueLower, valueUpper);
      break;
    default:
      assert(0 && "only cares about int comparison\n");
    }

    assert(internalEnv.hasValue(op1));
    ref<Interval> i1 = cast<Interval>(internalEnv.lookup(op1)->deref());

    gEnvMap.getEnv(bb, bbt).set(op1, new Interval(*i1 & *ipredicateT));
    gEnvMap.getEnv(bb, bbf).set(op1, new Interval(*i1 & *ipredicateF));
  }

  void executeCallInst(llvm::CallInst *cinst) {
    internalEnv.set(cinst, new Interval(Interval::Top()));
  }
};
} // namespace klee

#endif
