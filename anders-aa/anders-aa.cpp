/*  [anders-aa.cpp] Pass registration for AndersAA.
 *  v. 0.16, 2008-11-29
 *------------------------------------------------------------------------------
 *  Copyright 2008 Andrey Petrov (see ../COPYING)
 */

#include "anders-aa.h"

char AndersAA::ID= 0;
//Option -anders-aa is reserved for the built-in Andersen analysis.
RegisterPass<AndersAA> X("anders",
  "Andersen's interprocedural pointer analysis (field-sensitive)");
