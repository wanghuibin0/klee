#include "CSExecutor.h"
#include "CoreStats.h"
#include "MemoryManager.h"
#include "Searcher.h"
#include "StatsTracker.h"

#include "Summary.h"
#include "klee/Core/SummaryManager.h"
#include "klee/Module/EnvContext.h"
#include "../Module/IntervalInfo/Env.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"

using namespace klee;
using namespace llvm;

namespace klee {
extern cl::OptionCategory TerminationCat;
}

namespace {
cl::opt<unsigned>
    MaxLoopUnroll("max-loopunroll",
                  cl::desc("Refuse to fork when above this amount of "
                           "loop unroll times"),
                  cl::init(5), cl::cat(TerminationCat));
}

CTXCSExecutor::CTXCSExecutor(const Executor &proto, llvm::Function *f)
    : Executor(proto), func(f), summary(new Summary(f)) {
  summary->setContext(ConstantExpr::create(1, Expr::Bool));
}

void CTXCSExecutor::run() {
  ExecutionState *es = createInitialState(func);

  // bindModuleConstants should be called after initializeGlobals
  bindModuleConstants();

  buildConstraintFromStaticContext(es, func);

  // processTree = std::make_unique<PTree>(es);

  states.insert(es);

  searcher = constructUserSearcher(*this);
  std::vector<ExecutionState *> newStates(states.begin(), states.end());
  searcher->update(0, newStates, std::vector<ExecutionState *>());

  while (!states.empty() && !haltExecution) {
    ExecutionState &state = searcher->selectState();
    llvm::outs() << "new round, current state is " << &state << "\n";
    KInstruction *ki = state.pc;
    stepInstruction(state);

    executeInstruction(state, ki);
    timers.invoke();
    updateStates(&state);
    llvm::outs() << "after this round, remaining " << states.size()
                 << " states.\n";
  }

  delete searcher;
  searcher = nullptr;

  // processTree = nullptr;

  return;
  // the computed summary has been stored in summaryLib path by path online,
  // so it is unnecessary to store them explicitly.
}

void CTXCSExecutor::buildConstraintFromStaticContext(ExecutionState *es,
                                                     llvm::Function *f) {
  std::map<Function *, Env> &context = getGlobalEnvContext();
  Env &env = context[f];
  env.dump(outs());

  auto i = 0;
  auto it = f->arg_begin();
  for (; it != f->arg_end(); ++it, ++i) {
    assert(env.hasValue(it) && "interval info for arg must be in env");
    Interval interval = env.lookup(it);
    if (interval.isTop()) {
      continue;
    }
    IntTy lower = interval.getLower();
    IntTy upper = interval.getUpper();

    KFunction *kf = kmodule->functionMap[f];
    ref<Expr> arg = getArgumentCell(*es, kf, i).value;

    ref<Expr> expr1 = SgeExpr::create(arg, ConstantExpr::alloc(lower, Expr::Int32));
    ref<Expr> expr2 = SleExpr::create(arg, ConstantExpr::alloc(upper, Expr::Int32));

    ref<Expr> expr = AndExpr::create(expr1, expr2);

    es->addConstraint(expr);
  }
}

std::unique_ptr<Summary> CTXCSExecutor::extractSummary() {
  return std::move(summary);
}

ExecutionState *CTXCSExecutor::createInitialState(Function *f) {
  ExecutionState *state = new ExecutionState(kmodule->functionMap[f]);
  initializeGlobals(*state);
  makeArgsSymbolic(state);
  return state;
}

void CTXCSExecutor::initializeGlobals(ExecutionState &state) {
  allocateGlobalObjects(state);
  initializeGlobalAliases();
  makeGlobalsSymbolic(&state);
  //  llvm::outs() << "after initialize globals, dump the address space\n";
  //  state.addressSpace.dump();
}

void CTXCSExecutor::makeGlobalsSymbolic(ExecutionState *state) {
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
      mo->setName(name);
      globalsMod.push_back(&v);
      llvm::outs() << "make a global variable symbolic: " << name << "\n";
      llvm::outs() << "the memory object is: \n";
      mo->dump();
    }
  }
}

void CTXCSExecutor::makeArgsSymbolic(ExecutionState *state) {
  KFunction *kf = kmodule->functionMap[func];
  unsigned numArgs = func->arg_size();
  auto arg_it = func->arg_begin();
  for (unsigned i = 0; i < numArgs; ++i) {
    llvm::Value *arg = arg_it;
    llvm::Type *argTy = arg->getType();
    Expr::Width w = 0;

    if (isa<llvm::PointerType>(argTy)) {
      llvm::outs()
          << "in CTXCSExecutor::makeArgsSymbolic: function arg is a pointer\n";
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
      llvm::outs() << "this arg " << name
                   << " is a pointer, allocate some memory for its pointee\n";
    } else {
      llvm::outs() << "CTXCSExecutor::makeArgsSymbolic: creating new symbolic "
                      "array with size "
                   << w << "\n";
      std::string name = "arg_" + func->getName().str() + "_" + llvm::utostr(i);
      llvm::outs() << "the arg name is " << name << "\n";
      const Array *array =
          arrayCache.CreateArray(name, Expr::getMinBytesForWidth(w));
      res = Expr::createTempRead(array, w);
    }
    bindArgument(kf, i, *state, res);
    summary->addArg(arg, res);
    ++arg_it;
  }
}

void CTXCSExecutor::terminateState(ExecutionState &state) {
  interpreterHandler->incPathsExplored();

  std::vector<ExecutionState *>::iterator it =
      std::find(addedStates.begin(), addedStates.end(), &state);
  if (it == addedStates.end()) {
    state.pc = state.prevPC;

    removedStates.push_back(&state);
  } else {
    // never reached searcher, just delete immediately
    addedStates.erase(it);
    // processTree->remove(state.ptreeNode);
    delete &state;
  }
}

void CTXCSExecutor::terminateStateEarly(ExecutionState &state,
                                        const Twine &message) {
  // this state has been terminated early, it has no contricution to the summary
  // lib, just throw it.
  terminateState(state);
}

void CTXCSExecutor::terminateStateOnExit(ExecutionState &state) {
  // this state has been terminate normally because of encountering a ret
  // instruction, we should record its behaviour as summary.

  // we use prevPC because pc has been updated in stepInstruction
  KInstruction *ki = state.prevPC;
  Instruction *i = ki->inst;
  ReturnInst *ri = cast<ReturnInst>(i);
  bool isVoidReturn = (ri->getNumOperands() == 0);

  // construct a path summary
  NormalPathSummary ps;
  ps.setPreCond(state.constraints);
  ps.markVoidRet(isVoidReturn);

  ref<Expr> result = ConstantExpr::alloc(0, Expr::Bool);
  if (!isVoidReturn) {
    result = eval(ki, 0, state).value;
    ps.setRetValue(result);
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
    llvm::outs() << "adding a modified global: ";
    gVal->dump();
    ps.addGlobalsModified(g, gVal);
  }

  summary->addNormalPathSummary(ps);

  terminateState(state);
}

void CTXCSExecutor::terminateStateOnError(ExecutionState &state,
                                          const llvm::Twine &messaget,
                                          enum TerminateReason termReason,
                                          const char *suffix,
                                          const llvm::Twine &info) {
  llvm::outs() << "terminate this state because error: "
               << TerminateReasonNames[termReason] << "\n";
  // construct an error path summary
  ErrorPathSummary eps(state.constraints, (enum ErrorReason)termReason);

  // since the error path will be terminated immediately, we do not handle
  // globals for simplicity.

  summary->addErrorPathSummary(eps);

  terminateState(state);

  if (shouldExitOn(termReason))
    haltExecution = true;
}

void CTXCSExecutor::stepInstruction(ExecutionState &state) {
  KInstruction *ki = state.pc;
  unsigned &instCnter = state.stack.back().instCnterMap[ki];
  ++instCnter;

  Instruction *inst = ki->inst;
  outs() << "this instruction: " << *inst << "\n";
  outs() << "counter is: " << instCnter << "\n";

  if (instCnter >= MaxLoopUnroll) {
    outs() << "MaxLoopUnroll is " << MaxLoopUnroll << "\n";
    outs() << "reach max loop unroll\n";
    terminateState(state);
  }

  printDebugInstructions(state);
  if (statsTracker)
    statsTracker->stepInstruction(state);

  ++stats::instructions;
  ++state.steppedInstructions;
  state.prevPC = state.pc;
  ++state.pc;
}
