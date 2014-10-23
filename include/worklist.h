/*  [worklist.h] Implementations of the solver's worklist.
 *  v. 0.16, 2008-11-26
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#ifndef __WORKLIST_H
#define __WORKLIST_H

#include "common.h"
#include "heap.h"

//------------------------------------------------------------------------------
#if WL_ORDER == WLO_FIFO || WL_ORDER == WLO_LIFO
//Wrapper for the STL deque (used as a queue or stack).
//arg0 of constructor, arg1 of push(), and arg0 of pop()
//  are for compatibility with the priority worklist.
class Worklist{
private:
#if DUAL_WL == 0
  deque<u32> curr;
#else
  deque<u32> curr, next;
#endif

public:
  Worklist(u32 nn= 0) {}
  bool empty() const{
    return curr.empty();
  }
  //Switch to the next list, and return true,
  //  if nothing remains in the current one.
  bool swap_if_empty(){
#if DUAL_WL == 1
    if(curr.empty()){
      curr.swap(next);
      return 1;
    }
    return 0;
#else
    return curr.empty();
#endif
  }
  //Insert node (n) into the list; priority is ignored.
  void push(u32 n, u32 p= 0){
#if DUAL_WL == 1
    next.push_back(n);
#else
    curr.push_back(n);
#endif
  }
  //Return the first (FIFO) or last (LIFO) node from the deque,
  //  storing priority (0) into (pp) if provided.
  u32 pop(u32 *pp= 0){
    assert(!curr.empty() && "trying to pop empty worklist");
#if WL_PRIO == WLP_FIFO
    u32 x= curr.front();
    curr.pop_front();
#else                                   //LIFO
    u32 x= curr.back();
    curr.pop_back();
#endif
    if(pp)
      *pp= 0;
    return x;
  }
};

//------------------------------------------------------------------------------
#elif WL_ORDER == WLO_ID
//Wrapper for the STL tree set (pops lowest ID first).
class Worklist{
private:
#if DUAL_WL == 0
  set<u32> curr;
#else
  set<u32> curr, next;
#endif

public:
  Worklist(u32 nn= 0) {}
  bool empty() const{
    return curr.empty();
  }
  bool swap_if_empty(){
#if DUAL_WL == 1
    if(curr.empty()){
      curr.swap(next);
      return 1;
    }
    return 0;
#else
    return curr.empty();
#endif
  }
  void push(u32 n, u32 p= 0){
#if DUAL_WL == 1
    next.insert(n);
#else
    curr.insert(n);
#endif
  }
  //Return the lowest-numbered node from the list.
  u32 pop(u32 *pp= 0){
    assert(!curr.empty() && "trying to pop empty worklist");
    set<u32>::iterator it= curr.begin();
    u32 n= *it;
    curr.erase(it);
    if(pp)
      *pp= 0;
    return n;
  }
};

//------------------------------------------------------------------------------
#elif WL_ORDER == WLO_PRIO
//Wrapper for the heap (pops lowest priority first).
class Worklist{
private:
#if DUAL_WL == 0
  Heap curr;
#else
  Heap curr, next;
#endif

public:
  //The constructor requires the node count (nn) to make the heap.
#if DUAL_WL == 0
  Worklist(u32 nn) : curr(nn) {}
#else
  Worklist(u32 nn) : curr(nn), next(nn) {}
#endif
  bool empty() const{
    return curr.empty();
  }
  bool swap_if_empty(){
#if DUAL_WL == 1
    if(curr.empty()){
      curr.swap(next);
      return 1;
    }
    return 0;
#else
    return curr.empty();
#endif
  }
  //Insert node (n) with priority (p) into the list.
  void push(u32 n, u32 p){
#if DUAL_WL == 1
    next.push(n, p);
#else
    curr.push(n, p);
#endif
  }
  //Return the top-priority node from the list,
  //  storing the priority into (pp) if provided.
  u32 pop(u32 *pp= 0){
    assert(!curr.empty() && "trying to pop empty worklist");
    return curr.pop(pp);
  }
};
#else
#error "unknown WL_ORDER"
#endif

#endif
