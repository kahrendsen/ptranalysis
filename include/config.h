/*  [config.h] Options for Anders.
 *  v. 0.16, 2008-11-27
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#ifndef __CONFIG_H
#define __CONFIG_H

//Enable (1) or disable (0): assert(), debug output, statistics collection,
//  points-to metrics, memory/time measurement, and Google profiler.
//Note that disabling debug may cause "empty body in an if statement" warnings
//  because it completely removes DEBUG() statements from the code.
#define USE_ASSERT 1
#define USE_DEBUG 1
#define USE_STATS 1
#define USE_METRICS 1
#define USE_MEM_TIME 1
#define USE_PROFILER 1
//Setting these to 0 will disable debug output for each stage, by changing
//  DEBUG_TYPE to ___disabled. For this to take effect, run opt with
//  -debug-only=anders; with -debug it will still output everything.
#define DEBUG_OCI 0
#define DEBUG_OPT 0
#define DEBUG_SOLVE 0

//Stop when the object/constraint identification is done?
#define OCI_ONLY 0
//Skip the solve phase?
#define NO_SOLVE 0
//Set to 0 to make the constraint generator field-insensitive.
//(not yet implemented)
#define FIELD_SENSITIVE 1
//Check for uninitialized global values? -- see obj_cons_id()
#define CHECK_GLOBAL_NULL 0
//Check for constraints with "undefined" sources?
//  -- see obj_cons_id.cpp/check_cons_undef()
#define CHECK_CONS_UNDEF 1
//Print unknown external functions used by the module?
//  -- see print.cpp/list_ext_unknown()
#define LIST_EXT_UNKNOWN 0

//NOTE: if USE_MEM_TIME is disabled above, these limits won't be checked.
//How much RAM (in MB) our process may use. This is only checked during solving.
#define SOLVE_RAM_LIMIT 3600
//How long (in sec) the solver may run.
#define SOLVE_TIME_LIMIT 200
//If either limit is exceeded, the solver will stop with a partial result,
//  and runOnModule() will complete normally.

//Order in which nodes are removed from the worklist:
#define WLO_FIFO 0      //pop the least recently pushed
#define WLO_LIFO 1      //pop the most recently pushed
#define WLO_ID 2        //pop in order of increasing node ID
#define WLO_PRIO 3      //pop the lowest priority first
#define WL_ORDER WLO_PRIO
//If set to 1, use 2 worklists, pushing onto next, popping from current,
//  and swapping when current is empty. The solver will make several passes
//  over the graph, using the above order within each pass.
#define DUAL_WL 1
//The type of priority to use (for WLO_PRIO):
#define WLP_LRF 0       //Least Recently Fired (lowest last-visited time)
#define WL_PRIO WLP_LRF

//Bits per sparse-bitmap element (must be a power of 2).
//128 is most commonly used, but 32, 64, or 256 may reduce
//  time and/or memory in some cases.
#define BM_ELSZ 128

//The BDD-to-vector cache is allowed to use at most (bvc_sz) MB, and
//  (bvc_remove) MB will be freed at once when it gets full. -- see util.cpp
//Do not increase bvc_max unless the "evicted and reused" number is a
//  significant fraction of the miss count.
const unsigned bvc_max= 128, bvc_remove= 8;

//Run LCD (lazy cycle detection) if there are at least (lcd_sz) candidate
//  nodes or if it has not run for (lcd_period) runs of solve_node().
//  -- see solve.cpp/solve()
//The curve of runtime vs. lcd_sz is irregularly shaped, but the interval
//  [20,300] usually works best.
const unsigned lcd_sz= 20, lcd_period= 50000;

//Size of the print buffer in bytes -- see printbuf.h
const unsigned printbuf_sz= 1<<18;

//Don't factor any constraint sets smaller than this (must be >1).
//  -- see cons_opt.cpp/factor_ls()
const unsigned factor_ls_min_sz= 2;

#endif
