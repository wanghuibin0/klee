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

TDCSExecutor::TDCSExecutor(const Executor &proto, llvm::Function *f)
    : Executor(proto), func(f), summary(new Summary(f)) {
      errs() << "TDCSE for function " << f->getName() << "\n";
    }

void TDCSExecutor::run() {
  llvm::errs() << "TDCSExecutor::run: starting to run TDCSExecutor for function " << func->getName() << "\n";
  ExecutionState *es = createInitialState(func);

  bindModuleConstants();

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

  llvm::errs() << "TDCSExecutor::run: finished running TDCSExecutor for function " << func->getName() << "\n";
  llvm::errs() << "======================================================================================\n";
  llvm::errs() << "finished generating summary for function " << func->getName() << "\n\tdumping it:\n";
  summary->dump();
  llvm::errs() << "summary has been dumped!\n";
  llvm::errs() << "======================================================================================\n";

  return;
}

std::unique_ptr<Summary> TDCSExecutor::extractSummary() {
  return std::move(summary);
}

ExecutionState *TDCSExecutor::createInitialState(Function *f) {
  ExecutionState *state = new ExecutionState(kmodule->functionMap[f]);
  initializeGlobals(*state);
  makeArgsSymbolic(state);
  return state;
}

void TDCSExecutor::initializeGlobals(ExecutionState &state) {
  allocateGlobalObjects(state);
  makeGlobalsSymbolic(&state);
}

void TDCSExecutor::makeGlobalsSymbolic(ExecutionState *state) {
  llvm::errs() << "TDCSExecutor::makeGlobalsSymbolic\n";
  const Module *m = kmodule->module.get();

  for (const GlobalVariable &v : m->globals()) {
    MemoryObject *mo = globalObjects.find(&v)->second;

    //llvm::errs() << "makeGlobalsSymbolic: " << mo->name << " at address " << mo->address << "\n";

    if (v.isConstant()) {
      assert(v.hasInitializer());
      ObjectState *os = bindObjectInState(*state, mo, false);
      initializeGlobalObject(*state, os, v.getInitializer(), 0);
    } else {
      std::string name = std::string("global_") + v.getName().data();
      executeMakeSymbolic(*state, mo, name);
      mo->setName(name);
      const ObjectState *os = state->addressSpace.findObject(mo);
      Expr::Width width = getWidthForLLVMType(v.getType()->getElementType());
      summary->addFormalGlobals(&v, os->read(0, width));
    }
  }
}

void TDCSExecutor::makeArgsSymbolic(ExecutionState *state) {
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
      mo2Val.emplace(mo, arg);
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

void TDCSExecutor::terminateState(ExecutionState &state) {
  //interpreterHandler->incPathsExplored();

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

void TDCSExecutor::terminateStateEarly(ExecutionState &state,
                                       const Twine &message) {
  // this state has been terminated early, it has no contribution to the summary
  // lib, just throw it.
  terminateState(state);
}

void TDCSExecutor::terminateStateOnRet(ExecutionState &state) {
  // this state has been terminated normally because of encountering a ret
  // instruction, we should record its behaviour as summary.

  // we use prevPC because pc has been updated in stepInstruction
  KInstruction *ki = state.prevPC;
  Instruction *i = ki->inst;
  // llvm::outs() << "Instruction is : " << *i << "\n";
  assert(isa<ReturnInst>(i) && "must be reaching a ret inst");

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

  summarizeGlobalsAndPtrArgs(state, ps);

  summary->addNormalPathSummary(ps);

  terminateState(state);
}

void TDCSExecutor::terminateStateOnExit(ExecutionState &state) {
  // this state has been terminated normally because of encountering a exit
  // instruction, we should record its behaviour as an error summary.

  // we use prevPC because pc has been updated in stepInstruction
  KInstruction *ki = state.prevPC;
  Instruction *i = ki->inst;
  // llvm::outs() << "Instruction is : " << *i << "\n";
  assert(!isa<ReturnInst>(i) && "must not be a return inst");
  // must be a call to `exit`
  // construct an error summary
  ErrorPathSummary ps(state.constraints, Exit);

  summarizeGlobalsAndPtrArgs(state, ps);
  
  summary->addErrorPathSummary(ps);
  terminateState(state);
  return;
}

void TDCSExecutor::terminateStateOnError(ExecutionState &state,
                                         const llvm::Twine &messaget,
                                         enum TerminateReason termReason,
                                         const char *suffix,
                                         const llvm::Twine &info) {
  KLEE_DEBUG_WITH_TYPE("cse", llvm::errs()
                                  << "terminate this state because error: "
                                  << TerminateReasonNames[termReason] << "\n";);
  // construct an error path summary
  ErrorPathSummary ps(state.constraints, termReason);

  for (const auto &mo : state.modified) {
    const ObjectState *os = state.addressSpace.findObject(mo);
    os->flushForRead();
    UpdateList ul = os->getUpdates();

    // skip what is not global or pointer arguments
    if (!mo2Val.count(mo)) continue;
    const Value *v = mo2Val[mo];
    assert((isa<GlobalValue>(v) || isa<Argument>(v)) &&
           "must be a globalValue or argument");
    if (isa<GlobalValue>(v)) {
      ps.globalsUpdated.emplace(cast<GlobalValue>(v), ul);
    } else {
      ps.argsUpdated.emplace(cast<Argument>(v), ul);
    }
  }

  summary->addErrorPathSummary(ps);

  terminateState(state);

  if (shouldExitOn(termReason))
    gHaltExecution = true;
}

void TDCSExecutor::stepInstruction(ExecutionState &state) {
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

void TDCSExecutor::executeMemoryOperation(
    ExecutionState &state, bool isWrite, ref<Expr> address,
    ref<Expr> value /* undef if read */,
    KInstruction *target /* undef if write */) {
  /* KLEE_DEBUG_WITH_TYPE( */
  /*     "cse", */
  /*     llvm::errs() << "entering TDCSExecutor::executeMemoryOperation\n";); */
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
          state.modified.insert(mo);
        }
      } else {
        ref<Expr> result = os->read(offset, type);

        if (interpreterOpts.MakeConcreteSymbolic)
          result = replaceReadWithSymbolic(state, result);

        bindLocal(target, state, result);
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
          state.modified.insert(mo);
        }
      } else {
        ref<Expr> result = os->read(mo->getOffsetExpr(address), type);
        bindLocal(target, *bound, result);
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
