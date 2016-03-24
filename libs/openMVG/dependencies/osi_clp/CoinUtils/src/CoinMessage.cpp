/* $Id: CoinMessage.cpp 1540 2012-06-28 10:31:24Z forrest $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinPragma.hpp"
#include "CoinMessage.hpp"
#include "CoinError.hpp"

typedef struct {
  COIN_Message internalNumber;
  int externalNumber; // or continuation
  char detail;
  const char * message;
} Coin_message;
static Coin_message us_english[]=
{
  {COIN_MPS_LINE,1,1,"At line %d %s"},
  {COIN_MPS_STATS,2,1,"Problem %s has %d rows, %d columns and %d elements"},
  {COIN_MPS_ILLEGAL,3001,0,"Illegal value for %s of %g"},
  {COIN_MPS_BADIMAGE,3002,0,"Bad image at line %d < %s >"},
  {COIN_MPS_DUPOBJ,3003,0,"Duplicate objective at line %d < %s >"},
  {COIN_MPS_DUPROW,3004,0,"Duplicate row %s at line %d < %s >"},
  {COIN_MPS_NOMATCHROW,3005,0,"No match for row %s at line %d < %s >"},
  {COIN_MPS_NOMATCHCOL,3006,0,"No match for column %s at line %d < %s >"},
  {COIN_MPS_FILE,6001,0,"Unable to open mps input file %s"},
  {COIN_MPS_BADFILE1,6002,0,"Unknown image %s at line %d of file %s"},
  {COIN_MPS_BADFILE2,6003,0,"Consider the possibility of a compressed file\
 which %s is unable to read"},
  {COIN_MPS_EOF,6004,0,"EOF on file %s"},
  {COIN_MPS_RETURNING,6005,0,"Returning as too many errors"},
  {COIN_MPS_CHANGED,3007,1,"Generated %s names had duplicates - %d changed"},
  {COIN_SOLVER_MPS,8,1,"%s read with %d errors"},
  {COIN_PRESOLVE_COLINFEAS,501,2,"Problem is infeasible due to column %d, %.16g %.16g"},
  {COIN_PRESOLVE_ROWINFEAS,502,2,"Problem is infeasible due to row %d, %.16g %.16g"},
  {COIN_PRESOLVE_COLUMNBOUNDA,503,2,"Problem looks unbounded above due to column %d, %g %g"},
  {COIN_PRESOLVE_COLUMNBOUNDB,504,2,"Problem looks unbounded below due to column %d, %g %g"},
  {COIN_PRESOLVE_NONOPTIMAL,505,1,"Presolved problem not optimal, resolve after postsolve"},
  {COIN_PRESOLVE_STATS,506,1,"Presolve %d (%d) rows, %d (%d) columns and %d (%d) elements"},
  {COIN_PRESOLVE_INFEAS,507,1,"Presolve determined that the problem was infeasible with tolerance of %g"},
  {COIN_PRESOLVE_UNBOUND,508,1,"Presolve thinks problem is unbounded"},
  {COIN_PRESOLVE_INFEASUNBOUND,509,1,"Presolve thinks problem is infeasible AND unbounded???"},
  {COIN_PRESOLVE_INTEGERMODS,510,1,"Presolve is modifying %d integer bounds and re-presolving"},
  {COIN_PRESOLVE_POSTSOLVE,511,1,"After Postsolve, objective %g, infeasibilities - dual %g (%d), primal %g (%d)"},
  {COIN_PRESOLVE_NEEDS_CLEANING,512,1,"Presolved model was optimal, full model needs cleaning up"},
  {COIN_PRESOLVE_PASS,513,3,"%d rows dropped after presolve pass %d"},
# if PRESOLVE_DEBUG
  { COIN_PRESOLDBG_FIRSTCHECK,514,3,"First occurrence of %s checks." },
  { COIN_PRESOLDBG_RCOSTACC,515,3,
    "rcost[%d] = %g should be %g |diff| = %g." },
  { COIN_PRESOLDBG_RCOSTSTAT,516,3,
    "rcost[%d] = %g inconsistent with status %s (%s)." },
  { COIN_PRESOLDBG_STATSB,517,3,
    "status[%d] = %s rcost = %g lb = %g val = %g ub = %g." },
  { COIN_PRESOLDBG_DUALSTAT,518,3,
    "dual[%d] = %g inconsistent with status %s (%s)." },
# endif
  {COIN_GENERAL_INFO,9,1,"%s"},
  {COIN_GENERAL_WARNING,3007,1,"%s"},
  {COIN_DUMMY_END,999999,0,""}
};
// **** aiutami!
static Coin_message italian[]=
{
  {COIN_MPS_LINE,1,1,"al numero %d %s"},
  {COIN_MPS_STATS,2,1,"matrice %s ha %d file, %d colonne and %d elementi (diverso da zero)"},
  {COIN_DUMMY_END,999999,0,""}
};
/* Constructor */
CoinMessage::CoinMessage(Language language) :
  CoinMessages(sizeof(us_english)/sizeof(Coin_message))
{
  language_=language;
  strcpy(source_,"Coin");
  class_= 2; // Coin
  Coin_message * message = us_english;

  while (message->internalNumber!=COIN_DUMMY_END) {
    CoinOneMessage oneMessage(message->externalNumber,message->detail,
		message->message);
    addMessage(message->internalNumber,oneMessage);
    message ++;
  }
  // Put into compact form
  toCompact();
  // now override any language ones

  switch (language) {
  case it:
    message = italian;
    break;

  default:
    message=NULL;
    break;
  }

  // replace if any found
  if (message) {
    while (message->internalNumber!=COIN_DUMMY_END) {
      replaceMessage(message->internalNumber,message->message);
      message ++;
    }
  }
}
