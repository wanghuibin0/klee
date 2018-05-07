#ifndef HEADERS_H
#define HEADERS_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
//#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <queue>
#include <deque>
#include <set>
//#include <cassert>//not working
#include <cstdlib>
#include <iostream>
#include <algorithm>

using std::cout;
using std::endl;
using std::map;
using std::unordered_map;
using std::string;
using std::vector;
using std::queue;
using std::deque;
using std::set;
using namespace llvm;

void myassert(bool x, string msg) {
  if (!x) {
    errs() << msg << " assertion failed\n";
    abort();
  }
}

#endif
