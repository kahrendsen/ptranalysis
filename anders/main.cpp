/*  [main.cpp] Cleanup functions, runOnModule(), and implementation
 *    of the Anders client interface.
 *  v. 0.16, 2008-11-30
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#include "../include/anders.h"

//------------------------------------------------------------------------------
//Initialize all data before starting the run.
void Anders::run_init(Module *M){
  releaseMemory();
  assert(!extinfo && !stats);
  curr_module= M;
  extinfo= new ExtInfo;
  if(USE_STATS){
    stats= (u32*)malloc(num_stats*4);
    memset(stats, 0, num_stats*4);
  }
}

//------------------------------------------------------------------------------
//Delete what the optimizations and solver won't need.
void Anders::pre_opt_cleanup(){
  struct_info.clear();
  gep_ce.clear();
  global_init_done.clear();
  stat_ret_node.clear();
  at_args.clear();
}

//------------------------------------------------------------------------------
//Delete anything not needed to get the analysis results.
void Anders::run_cleanup(){
  pre_opt_cleanup();
  curr_module= 0;
  constraints.clear();
  ind_calls.clear();
  icall_cons.clear();
  hcd_var.clear();
  off_mask.clear();
  cplx_cons.clear();
  if(extinfo){
    delete extinfo;
    extinfo= 0;
  }
  //Delete the constraint graph and prev_points_to.
  FORN(i, nodes.size()){
    Node *N= nodes[i];
    N->prev_points_to= bddfalse;
    N->copy_to.clear();
    N->load_to.clear();
    N->store_from.clear();
    N->gep_to.clear();
  }
}

//------------------------------------------------------------------------------
//Delete the points-to sets not needed by the clients.
void Anders::pts_cleanup(){
  //BDD id -> first ptr-eq. node.
  hash_map<u32, u32> eq;

  FORN(i, nodes.size()){
    Node *N= nodes[i];
    if(N->obj_sz){
      //Skip Argument objects, which contain top-level pointers
      //  (i.e. the parameters used directly in the function body).
      //Note that an obj. node may have no value if it was merged into
      //  an artificial node.
      if(!dyn_cast_or_null<Argument>(N->get_val())){
        N->points_to= bddfalse;
        continue;
      }
    }

    if(N->points_to != bddfalse){
      u32 pts= N->points_to.id();
      hash_map<u32, u32>::iterator j= eq.find(pts);
      if(j == eq.end()){
        eq[pts]= i;
      }else{
        merge_nodes(get_node_rep(i), get_node_rep(j->second));
      }
    }
  }
}

//------------------------------------------------------------------------------
//This is what LLVM calls to actually run the pass.
bool Anders::runOnModule(Module &M){
  DPUTS("***** Anders::runOnModule starting\n");
#if USE_PROFILER == 1
  ProfilerStart("/tmp/anders.prof");
#endif
  if(USE_MEM_TIME){
    print_time(0);            //set the start time
    fprintf(stderr, "Memory baseline: %uM\n", get_mem_usage());
  }
  run_init(&M);
  if(LIST_EXT_UNKNOWN)
    list_ext_unknown(M);
  obj_cons_id();
  if(USE_MEM_TIME)
    print_time("obj_cons_id");
  pre_opt_cleanup();

  if(!OCI_ONLY){
    cons_opt();
    if(USE_MEM_TIME){
      print_time("cons_opt");
      fprintf(stderr, "Memory used: %uM\n", get_mem_usage());
    }
    if(!NO_SOLVE){
      pts_init();
      if(USE_MEM_TIME){
        print_time("pts_init");
        fprintf(stderr, "Memory used: %uM\n", get_mem_usage());
      }
      solve_init();
      solve();
      if(USE_MEM_TIME){
        print_time("solve");
        fprintf(stderr, "Memory used: %uM\n", get_mem_usage());
      }
      //Points-to graph only, not sorted (sorting may run out of memory).
      if(DEBUG_SOLVE)
        DEBUG(print_cons_graph(1, 0));
    }
  }

  if(USE_STATS){
    print_stats();
    print_bdd2vec_stats();
  }
  if(USE_METRICS)
    print_metrics();
  run_cleanup();
  pts_cleanup();
#if USE_PROFILER == 1
  ProfilerStop();
#endif
  DPUTS("***** Anders::runOnModule exiting\n");
  return 0;                           //module not changed
}

//------------------------------------------------------------------------------
//Delete all remaining data when our results are no longer needed.
void Anders::releaseMemory(){
  run_cleanup();
  FORN(i, nodes.size())
    delete nodes[i];
  nodes.clear();
  val_node.clear();
  obj_node.clear();
  ret_node.clear();
  vararg_node.clear();
  tmp_num.clear();
  pts_dom= bddfalse;
  if(gep2pts){
    bdd_freepair(gep2pts);
    gep2pts= 0;
  }
  //If we delete any BDD implicitly, its destructor will decrease the refcount.
  geps.clear();
  _pb_free();
  if(stats){
    free(stats);
    stats= 0;
  }
  clear_bdd2vec();
  //We should not use bdd_done() here because the clients may still be
  //  using the BDD system.
}

//------------------------------------------------------------------------------
// Client interface
//------------------------------------------------------------------------------

//Return the points-to set of node n, with offset off,
//  as a pointer to a vector in the cache.
const vector<u32>* Anders::pointsToSet(u32 n, u32 off){
  assert(n && n < nodes.size() && "node ID out of range");
  if(!off)
    return bdd2vec(nodes[get_node_rep(n)]->points_to);
  assert(off < geps.size() && geps[off] != bddfalse);
  bdd gep= bdd_replace(bdd_relprod(nodes[get_node_rep(n)]->points_to,
      geps[off], pts_dom), gep2pts);
  return bdd2vec(gep);
}

//Return the points-to set of V's node.
const vector<u32>* Anders::pointsToSet(Value* V, u32 off){
  return pointsToSet(get_val_node(V), off);
}

//Get the rep node of V, or MAX_U32 if it has no node.
u32 Anders::PE(Value* V){
  u32 n= get_val_node(V, 1);
  if(!n)
    return MAX_U32;
  return get_node_rep(n);
}

//Get the rep node of node n
u32 Anders::PE(u32 n){
  assert(n && n < nodes.size() && "node ID out of range");
  return get_node_rep(n);
}

bool Anders::is_null(u32 n, u32 off)
{
  assert(n && n < nodes.size() && "node ID out of range");
  bdd pts = nodes[get_node_rep(n)]->points_to;

  if (!off) { return (pts == bddfalse); }
  else {
    assert(off < geps.size() && geps[off] != bddfalse);
    bdd gep= bdd_replace(bdd_relprod(pts, geps[off], pts_dom), gep2pts);
    return (gep == bddfalse);
  }
}

bool Anders::is_single(u32 n, u32 off)
{
  assert(n && n < nodes.size() && "node ID out of range");
  bdd pts = nodes[get_node_rep(n)]->points_to;

  if (!off) { return (bdd_satcountset(pts,pts_dom) == 1); }
  else {
    assert(off < geps.size() && geps[off] != bddfalse);
    bdd gep= bdd_replace(bdd_relprod(pts, geps[off], pts_dom), gep2pts);
    return (bdd_satcountset(gep,pts_dom) == 1);
  }
}

vector<bdd>& Anders::get_geps()
{
  return geps;
}
