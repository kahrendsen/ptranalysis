/*  [extinfo.h] Provides access to info about external functions.
 *  v. 0.16, 2008-11-25
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#ifndef __EXTINFO_H
#define __EXTINFO_H

#include "common.h"

//External function types, to approximate their effects.
//Assume a call in the form L= F(arg0, arg1, arg2, arg3).
enum extf_t{
  EFT_NOOP= 0,      //no effect on pointers
  EFT_ALLOC,        //L points to a newly allocated object
  EFT_REALLOC,      //like ALLOC if arg0 is a null ptr, else L_A0
  EFT_NOSTRUCT_ALLOC, //like ALLOC but only allocates non-struct data
  EFT_STAT,         //L points to an unknown static var X
  EFT_STAT2,        //L -> X (as above) and X -> Y (another static var)
  EFT_L_A0,         //Returns arg0, arg1, etc.
  EFT_L_A1,
  EFT_L_A2,
  EFT_L_A8,
  EFT_L_A0__A0R_A1R,  //copies the data that arg1 points to into the location
                      //  arg0 points to; note that several fields may be
                      //  copied at once if both point to structs.
                      //  Returns arg0.
  EFT_A1R_A0R,      //copies *arg0 into *arg1
  EFT_A3R_A1R_NS,   //copies *arg1 into *arg3 (cannot copy structs)
  EFT_A1R_A0,       //stores arg0 into *arg1
  EFT_A2R_A1,       //stores arg1 into *arg2
  EFT_A4R_A1,       //stores arg1 into *arg4
  EFT_L_A0__A2R_A0, //stores arg0 into *arg2 and returns it
  EFT_A0R_NEW,      //stores a pointer to an allocated object in *arg0
  EFT_A1R_NEW,      //as above, into *arg1, etc.
  EFT_A2R_NEW,
  EFT_A4R_NEW,
  EFT_A11R_NEW,
  EFT_OTHER         //not found in the list
};

//------------------------------------------------------------------------------
class ExtInfo{
private:

  //This maps each known function's name to its extf_t.
  //  Here StringMap is faster than DenseMap or STL map or hash_map.
  StringMap<extf_t> info;
  //A cache of is_ext results for all Function*'s (hash_map is fastest).
  hash_map<const Function*, bool> isext_cache;
  //see extinfo.cpp
  void init();

public:
//------------------------------------------------------------------------------
  ExtInfo(){
    init();
    isext_cache.clear();
  }

  //Return the extf_t of (F).
  extf_t get_type(const Function *F) const{
    assert(F);
    StringMap<extf_t>::const_iterator it= info.find(F->getName());
    if(it == info.end())
      return EFT_OTHER;
    else
      return it->second;
  }

  //Does (F) have a static var X (unavailable to us) that its return points to?
  bool has_static(const Function *F) const{
    extf_t t= get_type(F);
    return t==EFT_STAT || t==EFT_STAT2;
  }
  //Assuming hasStatic(F), does (F) have a second static Y where X -> Y?
  bool has_static2(const Function *F) const{
    return get_type(F) == EFT_STAT2;
  }
  //Does (F) allocate a new object?
  bool is_alloc(const Function *F) const{
    extf_t t= get_type(F);
    return t==EFT_ALLOC || t==EFT_NOSTRUCT_ALLOC;
  }
  //Does (F) allocate only non-struct objects?
  bool no_struct_alloc(const Function *F) const{
    return get_type(F) == EFT_NOSTRUCT_ALLOC;
  }
  //Does (F) not do anything with the known pointers?
  bool is_noop(const Function *F) const{
    return get_type(F) == EFT_NOOP;
  }

  //Should (F) be considered "external" (either not defined in the program
  //  or a user-defined version of a known alloc or no-op)?
  bool is_ext(const Function *F){
    assert(F);
    //Check the cache first; everything below is slower.
    hash_map<const Function*, bool>::iterator i_iec= isext_cache.find(F);
    if(i_iec != isext_cache.end())
      return i_iec->second;

    bool res;
    if(F->isDeclaration() || F->isIntrinsic()){
      res= 1;
    }else{
      extf_t t= get_type(F);
      res= t==EFT_ALLOC || t==EFT_REALLOC || t==EFT_NOSTRUCT_ALLOC
          || t==EFT_NOOP;
    }
    isext_cache[F]= res;
    return res;
  }
};

#endif
