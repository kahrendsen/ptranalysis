/*  [util.cpp] Various utility functions for Anders.
 *  v. 0.16, 2008-11-30
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#include "../include/common.h"

namespace{
//The CPU time at the last print_time() call.
u64 start_time;

//------------------------------------------------------------------------------
// BDD -> vector translation with cache
//------------------------------------------------------------------------------

//This vector holds the result of sat2vec().
vector<u32> bddexp;
//A map of BDD IDs to their last-used time and vector expansion.
//If (it) is an iterator into bv_cache, (it->second.second) points to
//  the vector.
hash_map<u32, pair<u32, vector<u32>*> > bv_cache;

//Total number of entries (4B each) in all vectors in the cache.
u32 bvc_sz= 0;
//Cache stats: hit, misses, total items evicted, and evicted items
//  requested again.
u32 bvc_hits, bvc_misses, bvc_evictions, bvc_evict_used;
//The BDD IDs that have been evicted from the cache.
hash_set<u32> evicted;
//The sequence number of the current bdd2vec call (for LRU).
u32 bv_time= 0;
//priority_queue puts the greatest element at the top, but we need the
//  lowest time at the top, so use the reverse comparator.
priority_queue<pair<u32, u32>, vector<pair<u32, u32> >,
    greater<pair<u32, u32> > > bv_lru;

//------------------------------------------------------------------------------
//This bdd_allsat callback appends all assignments of bit vector (v)
//  (in which -1 means don't care) to the global vector (bddexp).
//  Non-relevant positions in (v) are skipped.
//It assumes that domain 0 is the first set of variables to be allocated
//  and that domains 0 and 1 have their domains interleaved.
void sat2vec(char *v, int szv){
  //The number of bits in (v) used for domain 0
  //  and any other variables interleaved into it.
  static u32 fdd0_bits= 0;
  if(!fdd0_bits)
    fdd0_bits= (u32)fdd_varnum(0) * 2 - 1;
  //The result with all dont-cares at 0.
  u32 base= 0;
  //The list of dont-care masks and its size.
  static u32 dc[32];
  u32 ndc= 0;
  //v[0] represents bit 0 of the result, so the set bit in (m) moves left.
  //Odd values of (i) are skipped because they refer to domain 1.
  for(u32 i= 0, m= 1; i < fdd0_bits; i += 2, m <<= 1){
    switch(v[i]){
      case -1:
        dc[ndc++]= m;
        break;
      case 1:
        base |= m;
        break;
      default:
        ;
    }
  }
  //Speed up the handling of small cases (up to 2 dont-cares).
  switch(ndc){
    case 2:{
      u32 x= base|dc[1];
      bddexp.push_back(x);
      bddexp.push_back(x|dc[0]);
    }
    case 1:
      bddexp.push_back(base|dc[0]);
    case 0:
      bddexp.push_back(base);
      break;
    default:
      assert(ndc <= 25 && "too many assignments for BDD");
      //Use all subsets of dont-cares.
      for(u32 i= 0, ie= 1<<ndc; i < ie; ++i){
        u32 x= base;
        for(u32 j= 0, m= 1; j < ndc; ++j, m <<= 1){
          if(i&m){
            x |= dc[j];
          }
        }
        bddexp.push_back(x);
      }
  }
}

} //end anon namespace

//------------------------------------------------------------------------------
//Clear the bdd -> vector cache.
void clear_bdd2vec(){
  bddexp.clear();
  for(hash_map<u32, pair<u32, vector<u32>*> >::iterator it= bv_cache.begin(),
      ie= bv_cache.end(); it != ie; ++it)
    delete it->second.second;
  bv_cache.clear();
  bvc_sz= 0;
  evicted.clear();
  bvc_hits= bvc_misses= bvc_evictions= bvc_evict_used= 0;
  while(!bv_lru.empty())
    bv_lru.pop();
  bv_time= 0;
}

//------------------------------------------------------------------------------
//Print the cache stats.
void print_bdd2vec_stats(){
  fprintf(stderr, "bdd2vector cache: limit %uM, misses %u, hits %u,\n"
      "    evictions %u, evicted and reused %u\n",
      bvc_max, bvc_misses, bvc_hits, bvc_evictions, bvc_evict_used);
}

//------------------------------------------------------------------------------
//Return a pointer to a vector containing the set of integers
//  represented by BDD (B).
//Make sure you're finished with this pointer before calling bdd2vec again:
//  it can be deleted when the cache gets full.
const vector<u32>* bdd2vec(bdd B){
  assert(B != bddtrue);
  if(B == bddfalse){          //empty set
    static vector<u32> v_false;
    return &v_false;
  }
  ++bv_time;

  u32 idB= B.id();
  hash_map<u32, pair<u32, vector<u32>*> >::iterator i_bvc= bv_cache.find(idB);
  if(i_bvc != bv_cache.end()){
    //If this BDD is cached, update its time and return its bitmap.
    i_bvc->second.first= bv_time;
    bv_lru.push(make_pair(bv_time, idB));
    ++bvc_hits;
    return i_bvc->second.second;
  }
  ++bvc_misses;
  if(evicted.count(idB))
    ++bvc_evict_used;

  //If the cache size has reached (bvc_max) MB, remove the oldest items
  //  until at least (bvc_remove) MB is free.
  //Note that bvc_sz is in 4B units.
  if(bvc_sz >= bvc_max<<18){
    while(bvc_sz >= (bvc_max-bvc_remove)<<18){
      pair<u32, u32> x= bv_lru.top();
      bv_lru.pop();
      u32 t= x.first, id= x.second;
      i_bvc= bv_cache.find(id);
      assert(i_bvc != bv_cache.end());
      //Some LRU entries may have an older time than the cache entry.
      if(t == i_bvc->second.first){
        //Update the size if the vector was actually deleted.
        bvc_sz -= i_bvc->second.second->size();
        delete i_bvc->second.second;
        bv_cache.erase(i_bvc);
        evicted.insert(id);
        ++bvc_evictions;
      }
    }
  }

  //Fill in bddexp and copy it to the cache.
  //This is faster than passing a new blank vector to sat2vec()
  //  because bddexp doesn't need to grow every time.
  bddexp.clear();
  bdd_allsat(B, sat2vec);
  //This lets the solver iterate the points-to set in increasing order,
  //  which will usually save more time than the sorting uses.
  sort(bddexp.begin(), bddexp.end());
  vector<u32> *v= new vector<u32>(bddexp);
  bv_cache[idB]= make_pair(bv_time, v);
  bvc_sz += bddexp.size();
  bv_lru.push(make_pair(bv_time++, idB));
  return v;
}

//------------------------------------------------------------------------------
//Return the current size of our data segment (in MB),
//  by reading the line "VmData: # kB" from /proc/self/status.
u32 get_mem_usage(){
  u32 x= 0;
  ifstream in("/proc/self/status");
  while(in){
    string line, tag;
    getline(in, line);
    istringstream iss(line);
    iss>>tag;
    if(tag == "VmData:"){
      iss>>x;
      break;
    }
  }
  in.close();
  return (x+512)/1024;
}

//------------------------------------------------------------------------------
//Return the CPU time we have used so far, in microseconds.
u64 get_cpu_time(){
#define MICRO(x) (x.tv_sec*1000000LL + x.tv_usec)
  static struct rusage r;
  getrusage(RUSAGE_SELF, &r);
  return MICRO(r.ru_utime) + MICRO(r.ru_stime);
}

//------------------------------------------------------------------------------
//Print the CPU time used since the last print_time call, with the given label.
//If the label is null, update the start time but don't print anything.
void print_time(const char *label){
  u64 t= get_cpu_time();
  if(label)
    fprintf(stderr, "Time for %s: %0.3fs\n", label, (t-start_time)*1e-6);
  start_time= t;
}
