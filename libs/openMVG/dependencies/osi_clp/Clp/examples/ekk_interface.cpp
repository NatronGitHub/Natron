/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cassert>

#include "ClpSimplexPrimal.hpp"
#include "ClpFactorization.hpp"
#include "ClpPresolve.hpp"
#include "ekk_c_api.h"
//#include "ekk_c_api_undoc.h"
extern "C" {
     OSLLIBAPI void * OSLLINKAGE ekk_compressModel(EKKModel * model);
     OSLLIBAPI void OSLLINKAGE ekk_decompressModel(EKKModel * model,
               void * compressInfo);
}

static ClpPresolve * presolveInfo = NULL;

static ClpSimplex * clpmodel(EKKModel * model, int startup)
{
     ClpSimplex * clp = new ClpSimplex();;
     int numberRows = ekk_getInumrows(model);
     int numberColumns = ekk_getInumcols(model);
     clp->loadProblem(numberColumns, numberRows, ekk_blockColumn(model, 0),
                      ekk_blockRow(model, 0), ekk_blockElement(model, 0),
                      ekk_collower(model), ekk_colupper(model),
                      ekk_objective(model),
                      ekk_rowlower(model), ekk_rowupper(model));
     clp->setOptimizationDirection((int) ekk_getRmaxmin(model));
     clp->setPrimalTolerance(ekk_getRtolpinf(model));
     if (ekk_getRpweight(model) != 0.1)
          clp->setInfeasibilityCost(1.0 / ekk_getRpweight(model));
     clp->setDualTolerance(ekk_getRtoldinf(model));
     if (ekk_getRdweight(model) != 0.1)
          clp->setDualBound(1.0 / ekk_getRdweight(model));
     clp->setDblParam(ClpObjOffset, ekk_getRobjectiveOffset(model));
     const int * rowStatus =  ekk_rowstat(model);
     const double * rowSolution = ekk_rowacts(model);
     int i;
     clp->createStatus();
     double * clpSolution;
     clpSolution = clp->primalRowSolution();
     memcpy(clpSolution, rowSolution, numberRows * sizeof(double));
     const double * rowLower = ekk_rowlower(model);
     const double * rowUpper = ekk_rowupper(model);
     for (i = 0; i < numberRows; i++) {
          ClpSimplex::Status status;
          if ((rowStatus[i] & 0x80000000) != 0) {
               status = ClpSimplex::basic;
          } else {
               if (!startup) {
                    // believe bits
                    int ikey = rowStatus[i] & 0x60000000;
                    if (ikey == 0x40000000) {
                         // at ub
                         status = ClpSimplex::atUpperBound;
                         clpSolution[i] = rowUpper[i];
                    } else if (ikey == 0x20000000) {
                         // at lb
                         status = ClpSimplex::atLowerBound;
                         clpSolution[i] = rowLower[i];
                    } else if (ikey == 0x60000000) {
                         // free
                         status = ClpSimplex::isFree;
                         clpSolution[i] = 0.0;
                    } else {
                         // fixed
                         status = ClpSimplex::atLowerBound;
                         clpSolution[i] = rowLower[i];
                    }
               } else {
                    status = ClpSimplex::superBasic;
               }
          }
          clp->setRowStatus(i, status);
     }

     const int * columnStatus = ekk_colstat(model);
     const double * columnSolution =  ekk_colsol(model);
     clpSolution = clp->primalColumnSolution();
     memcpy(clpSolution, columnSolution, numberColumns * sizeof(double));
     const double * columnLower = ekk_collower(model);
     const double * columnUpper = ekk_colupper(model);
     for (i = 0; i < numberColumns; i++) {
          ClpSimplex::Status status;
          if ((columnStatus[i] & 0x80000000) != 0) {
               status = ClpSimplex::basic;
          } else {
               if (!startup) {
                    // believe bits
                    int ikey = columnStatus[i] & 0x60000000;
                    if (ikey == 0x40000000) {
                         // at ub
                         status = ClpSimplex::atUpperBound;
                         clpSolution[i] = columnUpper[i];
                    } else if (ikey == 0x20000000) {
                         // at lb
                         status = ClpSimplex::atLowerBound;
                         clpSolution[i] = columnLower[i];
                    } else if (ikey == 0x60000000) {
                         // free
                         status = ClpSimplex::isFree;
                         clpSolution[i] = 0.0;
                    } else {
                         // fixed
                         status = ClpSimplex::atLowerBound;
                         clpSolution[i] = columnLower[i];
                    }
               } else {
                    status = ClpSimplex::superBasic;
               }
          }
          clp->setColumnStatus(i, status);
     }
     return clp;
}

static int solve(EKKModel * model, int  startup, int algorithm,
                 int presolve)
{
     // values pass or not
     if (startup)
          startup = 1;
     // if scaled then be careful
     bool scaled = ekk_scaling(model) == 1;
     if (scaled)
          ekk_scaleRim(model, 1);
     void * compressInfo = NULL;
     ClpSimplex * clp;
     if (!presolve || !presolveInfo) {
          // no presolve or osl presolve - compact columns
          compressInfo = ekk_compressModel(model);
          clp = clpmodel(model, startup);;
     } else {
          // pick up clp model
          clp = presolveInfo->model();
     }

     // don't scale if alreday scaled
     if (scaled)
          clp->scaling(false);
     if (clp->numberRows() > 10000)
          clp->factorization()->maximumPivots(100 + clp->numberRows() / 100);
     if (algorithm > 0)
          clp->primal(startup);
     else
          clp->dual();

     int numberIterations = clp->numberIterations();
     if (presolve && presolveInfo) {
          // very wasteful - create a clp copy of osl model
          ClpSimplex * clpOriginal = clpmodel(model, 0);
          presolveInfo->setOriginalModel(clpOriginal);
          // do postsolve
          presolveInfo->postsolve(true);
          delete clp;
          delete presolveInfo;
          presolveInfo = NULL;
          clp = clpOriginal;
          if (presolve == 3 || (presolve == 2 && clp->status())) {
               printf("Resolving from postsolved model\n");
               clp->primal(1);
               numberIterations += clp->numberIterations();
          }
     }

     // put back solution

     double * rowDual = (double *) ekk_rowduals(model);
     int numberRows = ekk_getInumrows(model);
     int numberColumns = ekk_getInumcols(model);
     int * rowStatus = (int *) ekk_rowstat(model);
     double * rowSolution = (double *) ekk_rowacts(model);
     int i;
     int * columnStatus = (int *) ekk_colstat(model);
     double * columnSolution = (double *) ekk_colsol(model);
     memcpy(rowSolution, clp->primalRowSolution(), numberRows * sizeof(double));
     memcpy(rowDual, clp->dualRowSolution(), numberRows * sizeof(double));
     for (i = 0; i < numberRows; i++) {
          if (clp->getRowStatus(i) == ClpSimplex::basic)
               rowStatus[i] = 0x80000000;
          else
               rowStatus[i] = 0;
     }

     double * columnDual = (double *) ekk_colrcosts(model);
     memcpy(columnSolution, clp->primalColumnSolution(),
            numberColumns * sizeof(double));
     memcpy(columnDual, clp->dualColumnSolution(), numberColumns * sizeof(double));
     for (i = 0; i < numberColumns; i++) {
          if (clp->getColumnStatus(i) == ClpSimplex::basic)
               columnStatus[i] = 0x80000000;
          else
               columnStatus[i] = 0;
     }
     ekk_setIprobstat(model, clp->status());
     ekk_setRobjvalue(model, clp->objectiveValue());
     ekk_setInumpinf(model, clp->numberPrimalInfeasibilities());
     ekk_setInumdinf(model, clp->numberDualInfeasibilities());
     ekk_setIiternum(model, numberIterations);
     ekk_setRsumpinf(model, clp->sumPrimalInfeasibilities());
     ekk_setRsumdinf(model, clp->sumDualInfeasibilities());
     delete clp;
     if (compressInfo)
          ekk_decompressModel(model, compressInfo);

     if (scaled)
          ekk_scaleRim(model, 2);
     return 0;
}
/* As ekk_primalSimplex + postsolve instructions:
   presolve - 0 , no presolve, 1 presolve but no primal after postsolve,
                  2 do primal if any infeasibilities,
		  3 always do primal.
*/
extern "C" int ekk_primalClp(EKKModel * model, int  startup, int presolve)
{
     if (presolveInfo) {
          if (!presolve)
               presolve = 3;
          return solve(model, startup, 1, presolve);
     } else {
          return solve(model, startup, 1, 0);
     }
}

/* As ekk_dualSimplex + postsolve instructions:
   presolve - 0 , no presolve, 1 presolve but no primal after postsolve,
                  2 do primal if any infeasibilities,
		  3 always do primal.
*/
extern "C" int ekk_dualClp(EKKModel * model, int presolve)
{
     if (presolveInfo) {
          if (!presolve)
               presolve = 3;
          return solve(model, 0, -1, presolve);
     } else {
          return solve(model, 0, -1, 0);
     }
}
/* rather like ekk_preSolve (3) plus:
   keepIntegers - false to treat as if continuous
   pass   - do this many passes (0==default(5))

   returns 1 if infeasible
*/
extern "C" int ekk_preSolveClp(EKKModel * model, bool keepIntegers,
                               int pass)
{
     delete presolveInfo;
     presolveInfo = new ClpPresolve();
     bool scaled = ekk_scaling(model) == 1;
     if (scaled)
          ekk_scaleRim(model, 1);
     if (!pass)
          pass = 5;
     // very wasteful - create a clp copy of osl model
     // 1 to keep solution as is
     ClpSimplex * clp = clpmodel(model, 1);
     // could get round with tailored version of ClpPresolve.cpp
     ClpSimplex * newModel =
          presolveInfo->presolvedModel(*clp,
                                       ekk_getRtolpinf(model),
                                       keepIntegers, pass, true);
     delete clp;
     presolveInfo->setOriginalModel(NULL);
     if (scaled)
          ekk_scaleRim(model, 2);
     if (newModel) {
          return 0;
     } else {
          delete presolveInfo;
          presolveInfo = NULL;
          return 1;
     }
}
