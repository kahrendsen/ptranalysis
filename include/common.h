/*  [common.h] Includes & definitions used by Anders.
 *  v. 0.16, 2008-11-27
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#ifndef __COMMON_H
#define __COMMON_H

#include "bitmap.h"
#include "bvec.h"
#include "config.h"
#include "fdd.h"
#include "profiler.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
//#include "llvm/TypeSymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringMap.h"
#include <hash_set>
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/InstIterator.h"

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <list>
#include <sstream>
#include <map>
#include <math.h>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <stack>
#include <time.h>
#include <vector>
#include <ext/hash_map>
#include <ext/hash_set>
#include <sys/resource.h>
#include <sys/time.h>

//Disable assert() if requested (must go above assert.h)
#if USE_ASSERT == 1
  #undef NDEBUG
#else
  #define NDEBUG 1
#endif
#include <assert.h>

using namespace std;
using namespace llvm;
using namespace __gnu_cxx;

//------------------------------------------------------------------------------
#if USE_DEBUG == 1
  #undef DEBUG
  #define DEBUG(x)
#else
  #undef DEBUG
  #define DEBUG(x)
#endif

#define MAX_U32 (u32)-1
#define NOINLINE __attribute__((noinline))
#define FORN(i,n)  for(u32 i= 0, i##e= n; i < i##e; ++i)
//Print the C-str (s) to stderr.
#define PUTS(s) fputs(s, stderr)
#define DPUTS(s) DEBUG(PUTS(s))
#define DEOL DEBUG(putc('\n', stderr))

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef MySparseBitVector<BM_ELSZ> bitmap;

//------------------------------------------------------------------------------
//DenseMapInfo for different types
//------------------------------------------------------------------------------

namespace llvm{
  template<> struct DenseMapInfo<pair<u32, u32> >{
    static pair<u32, u32> getEmptyKey(){
      //Assume that these special values will never be inserted into the map.
      return make_pair(0xffef123a, 0xffef123b);
    }
    static pair<u32, u32> getTombstoneKey(){
      return make_pair(0xffef123c, 0xffef123d);
    }
    static unsigned getHashValue(const pair<u32, u32> &X){
      return (X.first<<16) ^ (X.first>>16) ^ X.second;
    }
    static unsigned isEqual(const pair<u32, u32> &X, const pair<u32, u32> &Y){
      return X == Y;
    }
  };
  /*
  //DenseSet needs this for some reason.
  template<> struct DenseMapInfo<char>{
    static char getEmptyKey(){ return (char)0xf0; }
    static char getTombstoneKey(){ return (char)0xf1; }
    static unsigned getHashValue(u32 x){ return x; }
    static unsigned isEqual(u32 x, u32 y){ return x == y; }
    static bool isPod(){ return true; }
  };
  */
  //Note: don't use DenseMap<bitmap>, hash_map is much faster.
  //DenseMapInfo<Constraint> is in anders.h.
}

//------------------------------------------------------------------------------
//GNU STL hashes/comparators for different types
namespace __gnu_cxx{
  template<> struct hash<pair<u32, u32> >{
    size_t operator () (const pair<u32, u32> &X) const{
      return (X.first<<16) ^ (X.first>>16) ^ X.second;
    }
  };
  template<> struct hash<bitmap>{
    size_t operator () (const bitmap& s) const{
      return (u32)s.getHashValue();
    }
  };
  template<> struct hash<const llvm::Function*>{
    size_t operator () (const llvm::Function* f) const {
        //return (u32)llvm::hash_value(&f);
        return 2;
    }
  };
#if 1
  template<> struct hash<std::basic_string<char> > {
    size_t operator () (std::basic_string<char> bs) const {
        return 43;
    }
  };
#endif
  }
/*
namespace std{
   template<> struct modulus<llvm::Function>{
    size_t operator () (const llvm::Function &f1, const llvm::Function &f2) const{
        return ((u32)&f1)%((u32)&f2);
    }
  };
 

}
*/
//see ../anders/util.cpp
void clear_bdd2vec();
void print_bdd2vec_stats();
const vector<u32>* bdd2vec(bdd x);
u32 get_mem_usage();
u64 get_cpu_time();
void print_time(const char *label);

#endif
