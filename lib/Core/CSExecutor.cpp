#include "CSExecutor.h"
#include "MemoryManager.h"
#include "Searcher.h"

#include "klee/Core/SummaryManager.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"

using namespace klee;
using namespace llvm;

BUCSExecutor::BUCSExecutor(const Executor &proto, llvm::Function *f)
    : Executor(proto), func(f), summary(new Summary(f)) {}

void BUCSExecutor::run() {
  ExecutionState *es = createInitialState(func);

  processTree = std::make_unique<PTree>(es);

  states.insert(es);

  searcher = constructUserSearcher(*this);
  std::vector<ExecutionState *> newStates(states.begin(), states.end());
  searcher->update(0, newStates, std::vector<ExecutionState *>());

  while (!states.empty()) {
    ExecutionState &state = searcher->selectState();
    KInstruction *ki = state.pc;
    stepInstruction(state);

    executeInstruction(state, ki);
    updateStates(&state);
  }

  delete searcher;
  searcher = nullptr;

  processTree = nullptr;

  return;
  // the computed summary has been stored in summaryLib path by path online,
  // so it is unnecessary to store them explicitly.
}
Summary *BUCSExecutor::extractSummary() { return summary; }

ExecutionState *BUCSExecutor::createInitialState(Function *f) {
  ExecutionState *state = new ExecutionState(kmodule->functionMap[f]);
  initializeGlobals(*state);
  makeArgsSymbolic(state);
  return state;
}

void BUCSExecutor::initializeGlobals(ExecutionState &state) {
  allocateGlobalObjects(state);
  initializeGlobalAliases();
  makeGlobalsSymbolic(&state);
}

void BUCSExecutor::makeGlobalsSymbolic(ExecutionState *state) {
  const Module *m = kmodule->module.get();

  for (const GlobalVariable &v : m->globals()) {
    MemoryObject *mo = globalObjects.find(&v)->second;

    if (v.isConstant()) {
      assert(v.hasInitializer());
      ObjectState *os = bindObjectInState(*state, mo, false);
      initializeGlobalObject(*state, os, v.getInitializer(), 0);
    } else {
      std::string name = std::string("global_") + v.getName().data();
      executeMakeSymbolic(*state, mo, name);
      globalsMod.push_back(&v);
    }
  }
}

void BUCSExecutor::makeArgsSymbolic(ExecutionState *state) {
  KFunction *kf = kmodule->functionMap[func];
  unsigned numArgs = func->arg_size();
  auto arg_it = func->arg_begin();
  for (unsigned i = 0; i < numArgs; ++i) {
    llvm::Value *arg = arg_it;
    llvm::Type *argTy = arg->getType();
    Expr::Width w = 0;

    if (isa<llvm::PointerType>(argTy)) {
      llvm::outs()
          << "in BUCSExecutor::makeArgsSymbolic: function arg is a pointer\n";
      w = Context::get().getPointerWidth();
    } else {
      w = getWidthForLLVMType(argTy);
    }

    ref<Expr> res;
    if (isa<llvm::PointerType>(argTy)) {
      // we have to allocate some memory for pointer args
      llvm::Type *eleTy = cast<llvm::PointerType>(argTy)->getElementType();
      Expr::Width w = getWidthForLLVMType(eleTy);
      unsigned size = w * 64;
      unsigned alignment = kmodule->targetData->getPrefTypeAlignment(eleTy);

      MemoryObject *mo = memory->allocate(size, false, true,
                                          &*func->begin()->begin(), alignment);
      if (!mo) {
        llvm::report_fatal_error("out of memory");
      }
      bindObjectInState(*state, mo, false);
      std::string name = "arg_" + func->getName().str() + "_" + llvm::utostr(i);
      executeMakeSymbolic(*state, mo, name);
      res = mo->getBaseExpr();
    } else {
      llvm::outs() << "BUCSExecutor::makeArgsSymbolic: creating new symbolic "
                      "array with size "
                   << w << "\n";
      const Array *array = arrayCache.CreateArray(
          "arg_" + func->getName().str() + "_" + llvm::utostr(i),
          Expr::getMinBytesForWidth(w));
      res = Expr::createTempRead(array, w);
    }
    bindArgument(kf, i, *state, res);
    summary->addArg(arg, res);
    ++arg_it;
  }
}

void BUCSExecutor::terminateState(ExecutionState &state) {
  interpreterHandler->incPathsExplored();

  std::vector<ExecutionState *>::iterator it =
      std::find(addedStates.begin(), addedStates.end(), &state);
  if (it == addedStates.end()) {
    state.pc = state.prevPC;

    removedStates.push_back(&state);
  } else {
    // never reached searcher, just delete immediately
    addedStates.erase(it);
    processTree->remove(state.ptreeNode);
    delete &state;
  }
}

void BUCSExecutor::terminateStateEarly(ExecutionState &state,
                                       const Twine &message) {
  // this state has been terminated early, it has no contricution to the summary
  // lib, just throw it.
  terminateState(state);
}

void BUCSExecutor::terminateStateOnExit(ExecutionState &state) {
  // this state has been terminate normally because of encountering a ret
  // instruction, we should record its behaviour as summary.

  // we use prevPC because pc has been updated in stepInstruction
  KInstruction *ki = state.prevPC;
  Instruction *i = ki->inst;
  ReturnInst *ri = cast<ReturnInst>(i);
  bool isVoidReturn = (ri->getNumOperands() == 0);

  // construct a path summary
  NormalPathSummary *ps = new NormalPathSummary();
  ps->setPreCond(state.constraints);
  ps->markVoidRet(isVoidReturn);

  ref<Expr> result = ConstantExpr::alloc(0, Expr::Bool);
  if (!isVoidReturn) {
    result = eval(ki, 0, state).value;
    ps->setRetValue(result);
  }

  // TODO: globals should be collected beforehand
  for (const GlobalValue *g : globalsMod) {
    llvm::Type *ty = g->getType();
    assert(isa<llvm::PointerType>(ty) &&
           "found a global that is not pointer type");
    llvm::Type *eleTy = cast<llvm::PointerType>(ty)->getElementType();

    Expr::Width w = getWidthForLLVMType(eleTy);

    MemoryObject *mo = globalObjects[g];
    const ObjectState *os = state.addressSpace.findObject(mo);
    ref<Expr> gVal = os->read(0, w);
    ps->addGlobalsModified(g, gVal);
  }

  summary->addNormalPathSummary(ps);

  terminateState(state);
}

void BUCSExecutor::terminateStateOnError(ExecutionState &state,
                                         const llvm::Twine &messaget,
                                         enum TerminateReason termReason,
                                         const char *suffix,
                                         const llvm::Twine &info) {
  // construct an error path summary
  ErrorPathSummary *eps = new ErrorPathSummary(state.constraints, termReason);

  // since the error path will be terminated immediately, we do not handle
  // globals for simplicity.

  summary->addErrorPathSummary(eps);

  terminateState(state);

  if (shouldExitOn(termReason))
    haltExecution = true;
}
