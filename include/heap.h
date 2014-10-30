/*  [heap.h] Binary heap structure.
 *  v. 0.16, 2008-11-25
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#ifndef __HEAP_H
#define __HEAP_H

#include "common.h"

//This is a binary min-heap with keys in the range [0, UINT_MAX]
//  and values in the range [0, MV].
//No value may occur more than once; inserting a value that already exists
//  will change its key.
//The memory requirement is (4*MV + 8*S) B, where S = max. number of values
//  stored at the same time.
class Heap{
private:
  static const u32 NONE= 0xffffffff;
  //The keys in the actual heap and the corresponding values.
  //hk[0] is always empty; hk[1] is the min element.
  vector<u32> hk, hv;
  //The max value allowed in the heap and the current number of items.
  //(nv) is always equal to the pos. of the last item.
  u32 maxv, nv;
  //The position of each value in (hv), or NONE if it's not there.
  u32 *idxv;

  //Percolate up/down from pos. (i0), to restore the heap property.
  void perc_up(u32 i0){
    for(u32 i= i0; ; ){
      u32 ip= i/2;                    //parent pos.
      if(!ip)
        return;
      u32 k= hk[i], kp= hk[ip];
      //if the parent key is not greater, we're done
      if(kp <= k)
        return;
      //Swap key and value with the parent.
      hk[ip]= k, hk[i]= kp;
      u32 v= hv[i], vp= hv[ip];
      hv[ip]= v, hv[i]= vp;
      idxv[v]= ip, idxv[vp]= i;       //swap the values' indices
      i= ip;
    }
  }
  void perc_dn(u32 i0){
    for(u32 i= i0; ; ){
      u32 ic= i*2;                    //left child pos.
      if(ic > nv)                     //no left child
        return;
      u32 k= hk[i], kc= hk[ic];
      if(ic < nv){                    //right child exists
        u32 kr= hk[ic+1];
        if(kr < kc)                   //use the right child if it's smaller
          ++ic, kc= kr;
      }

      if(kc >= k)                     //if the child key is not less, we're done
        return;
      hk[ic]= k, hk[i]= kc;
      u32 v= hv[i], vc= hv[ic];
      hv[ic]= v, hv[i]= vc;
      idxv[v]= ic, idxv[vc]= i;
      i= ic;
    }
  }
public:
//------------------------------------------------------------------------------
  Heap(u32 mv) : maxv(mv) {
    nv= 0;
    //hk, hv have an extra word before the first item.
    hk.assign(1, 0);
    hv.assign(1, 0);
    idxv= (u32*)malloc((mv+1)*4);
    memset(idxv, 0xff, (mv+1)*4);     //set all words to NONE
  }
  ~Heap(){
    free(idxv);
  }
  void swap(Heap &b){
    u32 t, *p;
    hk.swap(b.hk);
    hv.swap(b.hv);
    p= idxv, idxv= b.idxv, b.idxv= p;
    t= maxv, maxv= b.maxv, b.maxv= t;
    t= nv, nv= b.nv, b.nv= t;
  }
  u32 size() const{
    return nv;
  }
  bool empty() const{
    return !nv;
  }
  //Insert the given value and key; returns true if (v) already existed.
  NOINLINE bool push(u32 v, u32 k){
    assert(v <= maxv);
    u32 i= idxv[v];
    if(i == NONE){
      //Append the new item and move up as needed.
      hk.push_back(k);
      hv.push_back(v);
      ++nv;
      idxv[v]= nv;
      perc_up(nv);
      return 0;
    }
    //Change the key in place and move up/down as needed.
    assert(i <= nv);
    u32 pk= hk[i];
    if(k == pk)
      return 1;
    hk[i]= k;
    if(k < pk)
      perc_up(i);
    else
      perc_dn(i);
    return 1;
  }
  //Delete and return the current min value, storing its key in (pk).
  NOINLINE u32 pop(u32 *pk= 0){
    assert(nv);
    u32 k= hk[1], v= hv[1];
    idxv[v]= NONE;
    --nv;
    //Move the last item down from the top, unless it's the one we returned,
    //  then delete it from both vectors.
    if(nv){
      hk[1]= hk[nv+1];
      u32 lv= hv[nv+1];
      hv[1]= lv;
      idxv[lv]= 1;
      perc_dn(1);
    }
    hk.pop_back();
    hv.pop_back();
    if(pk)
      *pk= k;
    return v;
  }
};

#endif
