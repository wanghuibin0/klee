#include "CSExecutor.h"
#include "CoreStats.h"
#include "MemoryManager.h"
#include "Searcher.h"
#include "StatsTracker.h"

#include "../Module/IntervalInfo/Env.h"
#include "klee/Core/SummaryManager.h"
#include "klee/Module/EnvContext.h"

#include "klee/Support/Debug.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace klee;
using namespace llvm;

namespace klee {
  extern cl::opt<bool> SimplifySymIndices;
  extern cl::opt<unsigned> MaxLoopUnroll;
  extern cl::opt<unsigned> MaxStatesInCse;
}

BUCSExecutor::BUCSExecutor(const Executor &proto, llvm::Function *f)
    : Executor(proto), func(f), summary(new Summary(f)) {}

void BUCSExecutor::run() {
  ExecutionState *es = createInitialState(func);

  states.insert(es);

  searcher = constructUserSearcher(*this);
  std::vector<ExecutionState *> newStates(states.begin(), states.end());
  searcher->update(0, newStates, std::vector<ExecutionState *>());

  while (!states.empty() && !gHaltExecution) {
    ExecutionState &state = searcher->selectState();
    KInstruction *ki = state.pc;
    stepInstruction(state);

    executeInstruction(state, ki);
    timers.invoke();
    updateStates(&state);

    if (summary->getNumOfSummaries() + states.size() >= MaxStatesInCse) {
      setInhibitForking(true);
    }
  }

  delete searcher;
  searcher = nullptr;

  llvm::errs() << "finished generating summary for function " << func->getName() << "\n\tdumping it:\n";
  summary->dump();
  llvm::errs() << "summary has been dumped!\n";

  return;
}

std::unique_ptr<Summary> BUCSExecutor::extractSummary() {
  return std::move(summary);
}

ExecutionState *BUCSExecutor::createInitialState(Function *f) {
  ExecutionState *state = new ExecutionState(kmodule->functionMap[f]);
  initializeGlobals(*state);
  makeArgsSymbolic(state);
  return state;
}

void BUCSExecutor::initializeGlobals(ExecutionState &state) {
  makeGlobalsSymbolic(&state);
}

void BUCSExecutor::makeGlobalsSymbolic(ExecutionState *state) {
  const Module *m = kmodule->module.get();

  for (const GlobalVariable &v : m->globals()) {
    MemoryObject *mo = globalObjects.find(&v)->second;

    llvm::errs() << "makeGlobalsSymbolic: " << mo->name << " at address " << mo->address << "\n";

    if (v.isConstant()) {
      assert(v.hasInitializer());
      ObjectState *os = bindObjectInState(*state, mo, false);
      initializeGlobalObject(*state, os, v.getInitializer(), 0);
    } else {
      std::string name = std::string("global_") + v.getName().data();
      executeMakeSymbolic(*state, mo, name);
      mo->setName(name);
      globalsMod.push_back(&v);
      const ObjectState *os = state->addressSpace.findObject(mo);
      Expr::Width width = getWidthForLLVMType(v.getType()->getElementType());
      summary->addFormalGlobals(&v, os->read(0, width));
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
      std::string name = "arg_" + func->getName().str() + "_" + llvm::utostr(i);
      const Array *array =
          arrayCache.CreateArray(name, Expr::getMinBytesForWidth(w));
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
  // this state has been terminated normally because of encountering a ret
  // instruction, we should record its behaviour as summary.

  // we use prevPC because pc has been updated in stepInstruction
  KInstruction *ki = state.prevPC;
  Instruction *i = ki->inst;
  // llvm::outs() << "Instruction is : " << *i << "\n";
  if (!isa<ReturnInst>(i)) {
    // must be a call to `exit`
    // construct an error summary
    ErrorPathSummary eps(state.constraints, Exit);
    summary->addErrorPathSummary(eps);
    terminateState(state);
    return;
  }
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
    ps.addGlobalsModified(g, gVal);
  }

  summary->addNormalPathSummary(ps);

  terminateState(state);
}

void BUCSExecutor::terminateStateOnError(ExecutionState &state,
                                         const llvm::Twine &messaget,
                                         enum TerminateReason termReason,
                                         const char *suffix,
                                         const llvm::Twine &info) {
  KLEE_DEBUG_WITH_TYPE("cse", llvm::errs()
                                  << "terminate this state because error: "
                                  << TerminateReasonNames[termReason] << "\n";);
  // construct an error path summary
  ErrorPathSummary eps(state.constraints, termReason);

  // since the error path will be terminated immediately, we do not handle
  // globals for simplicity.

  summary->addErrorPathSummary(eps);

  terminateState(state);

  if (shouldExitOn(termReason))
    gHaltExecution = true;
}

void BUCSExecutor::stepInstruction(ExecutionState &state) {
  KInstruction *ki = state.pc;
  unsigned &instCnter = state.stack.back().instCnterMap[ki];
  ++instCnter;

  /* if (instCnter >= MaxLoopUnroll) { */
  /*   llvm::errs() << "reaching max loop unroll " << MaxLoopUnroll << " when summarizing function " << this->func->getName() << "\n"; */
  /*   setInhibitForking(true); */
  /* } */

  printDebugInstructions(state);
  if (statsTracker)
    statsTracker->stepInstruction(state);

  ++stats::instructions;
  ++state.steppedInstructions;
  state.prevPC = state.pc;
  ++state.pc;
}

void BUCSExecutor::executeMemoryOperation(
    ExecutionState &state, bool isWrite, ref<Expr> address,
    ref<Expr> value /* undef if read */,
    KInstruction *target /* undef if write */) {
  /* KLEE_DEBUG_WITH_TYPE( */
  /*     "cse", */
  /*     llvm::errs() << "entering BUCSExecutor::executeMemoryOperation\n";); */
  Expr::Width type = (isWrite ? value->getWidth()
                              : getWidthForLLVMType(target->inst->getType()));
  unsigned bytes = Expr::getMinBytesForWidth(type);

  assert(SimplifySymIndices == false &&
         "SimplifySymIndices must be false in cse");
  address = optimizer.optimizeExpr(address, true);

  // fast path: single in-bounds resolution
  ObjectPair op;
  bool success;
  solver->setTimeout(coreSolverTimeout);
  if (!state.addressSpace.resolveOne(state, solver.get(), address, op,
                                     success)) {
    address = toConstant(state, address, "resolveOne failure");
    success = state.addressSpace.resolveOne(cast<ConstantExpr>(address), op);
  }
  solver->setTimeout(time::Span());

  if (success) {
    const MemoryObject *mo = op.first;

    /* KLEE_DEBUG_WITH_TYPE("cse", mo->dump();); */

    ref<Expr> offset = mo->getOffsetExpr(address);
    ref<Expr> check = mo->getBoundsCheckOffset(offset, bytes);
    check = optimizer.optimizeExpr(check, true);

    bool inBounds;
    solver->setTimeout(coreSolverTimeout);
    bool success = solver->mustBeTrue(state.constraints, check, inBounds,
                                      state.queryMetaData);
    solver->setTimeout(time::Span());
    if (!success) {
      state.pc = state.prevPC;
      terminateStateEarly(state, "Query timed out (bounds check).");
      return;
    }

    if (inBounds) {
      const ObjectState *os = op.second;
      if (isWrite) {
        if (os->readOnly) {
          terminateStateOnError(state, "memory error: object read only",
                                ReadOnly);
        } else {
          ObjectState *wos = state.addressSpace.getWriteable(mo, os);
          wos->write(offset, value);

          if (globalObjectsReversed.count(mo)) {
            objectsWritten.push_back(globalObjectsReversed.at(mo));
          }
        }
      } else {
        ref<Expr> result = os->read(offset, type);

        if (interpreterOpts.MakeConcreteSymbolic)
          result = replaceReadWithSymbolic(state, result);

        bindLocal(target, state, result);

        if (globalObjectsReversed.count(mo)) {
          objectsRead.push_back(globalObjectsReversed.at(mo));
        }
      }

      return;
    }
  }

  address = optimizer.optimizeExpr(address, true);
  ResolutionList rl;
  solver->setTimeout(coreSolverTimeout);
  bool incomplete = state.addressSpace.resolve(state, solver.get(), address, rl,
                                               0, coreSolverTimeout);
  solver->setTimeout(time::Span());

  // XXX there is some query wasteage here. who cares?
  ExecutionState *unbound = &state;

  for (ResolutionList::iterator i = rl.begin(), ie = rl.end(); i != ie; ++i) {
    const MemoryObject *mo = i->first;
    const ObjectState *os = i->second;
    ref<Expr> inBounds = mo->getBoundsCheckPointer(address, bytes);

    StatePair branches = fork(*unbound, inBounds, true);
    ExecutionState *bound = branches.first;

    // bound can be 0 on failure or overlapped
    if (bound) {
      if (isWrite) {
        if (os->readOnly) {
          terminateStateOnError(*bound, "memory error: object read only",
                                ReadOnly);
        } else {
          ObjectState *wos = bound->addressSpace.getWriteable(mo, os);
          wos->write(mo->getOffsetExpr(address), value);
          if (globalObjectsReversed.count(mo)) {
            objectsWritten.push_back(globalObjectsReversed.at(mo));
          }
        }
      } else {
        ref<Expr> result = os->read(mo->getOffsetExpr(address), type);
        bindLocal(target, *bound, result);
        if (globalObjectsReversed.count(mo)) {
          objectsRead.push_back(globalObjectsReversed.at(mo));
        }
      }
    }

    unbound = branches.second;
    if (!unbound)
      break;
  }

  // XXX should we distinguish out of bounds and overlapped cases?
  if (unbound) {
    if (incomplete) {
      terminateStateEarly(*unbound, "Query timed out (resolve).");
    } else {
      terminateStateOnError(*unbound, "memory error: out of bound pointer", Ptr,
                            NULL, getAddressInfo(*unbound, address));
    }
  }
}
