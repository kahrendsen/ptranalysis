/*  [anders-aa.h] LLVM ModulePass wrapper for running Anders.
 *  v. 0.16, 2008-11-29
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#ifndef __ANDERS_AA_H
#define __ANDERS_AA_H

#include "../include/anders.h"
#include "llvm/Analysis/FindUsedTypes.h"

class AndersAA : public ModulePass {
private:
  Anders *anders;

public:
  static char ID;

  AndersAA(): ModulePass((intptr_t)&ID), anders(0) {}

  ~AndersAA(){
    releaseMemory();
  }

  virtual bool runOnModule(Module &M){
    if(!anders)
      anders= new Anders;
    return anders->runOnModule(M);
  }

  virtual void releaseMemory(){
    if(anders){
      delete anders;
      anders= 0;
    }
  }

  //If -analyze option was given, print the points-to graph (sorted).
  virtual void print(std::ostream &O, const Module *M) const{
    assert(anders);
    anders->print_cons_graph(1, 1, &O);
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const{
    AU.setPreservesAll();
    AU.addRequired<FindUsedTypes>();
  }
};

#endif
