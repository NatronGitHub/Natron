/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*  This can be used to compare OSL and Clp.  The interface functions
    below (in ekk_interface.cpp) were written so that calls to osl
    in the middle of complex algorithms could easily be swapped
    for Clp code.  This was to stress test Clp (and Osl :-)).

    With the addition of ekk_crash it may be used to see if we need
    a crash in Clp.  With "both" set it can also be used to see which
    gives better behavior after postSolve.

    However they may be useful as sample code.  Virtuous people would
    add return code checking.

    This main could just as easily be C code.

*/

#include "ekk_c_api.h"
// These interface functions are needed
// Note - This example wastes memory as it has several copies of matrix
/* As ekk_primalSimplex + postsolve instructions:
   presolve - 0 , no presolve, 1 presolve but no primal after postsolve,
                  2 do primal if any infeasibilities,
		  3 always do primal.
*/
extern "C" int ekk_primalClp(EKKModel * model, int  startup, int presolve);

/* As ekk_dualSimplex + postsolve instructions:
   presolve - 0 , no presolve, 1 presolve but no primal after postsolve,
                  2 do primal if any infeasibilities,
		  3 always do primal.
*/
extern "C" int ekk_dualClp(EKKModel * model, int presolve);

/* rather like ekk_preSolve (3) plus:
   keepIntegers - false to treat as if continuous
   pass   - do this many passes (0==default(5))

   returns 1 if infeasible
*/
extern "C" int ekk_preSolveClp(EKKModel * model, bool keepIntegers,
                               int pass);

#include "ClpSimplex.hpp"
#include <stdio.h>
#include <stdarg.h>

int main(int argc, const char *argv[])
{
     const char * name;
     // problem is actually scaled for osl, dynamically for clp (slows clp)
     // default is primal, no presolve, minimise and use clp
     bool    primal = true, presolve = false;
     int useosl = 0;
     bool freeFormat = false;
     bool exportIt = false;

     EKKModel * model;
     EKKContext * context;

     if (argc > 1) {
          name = argv[1];
     } else {
#if defined(SAMPLEDIR)
          name = (SAMPLEDIR "/p0033.mps";
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
             }

             /* initialize OSL environment */
             context = ekk_initializeContext();
     model = ekk_newModel(context, "");

     int i;
     printf("*** Options ");
     for (i = 2; i < argc; i++) {
          printf("%s ", argv[i]);
     }
     printf("\n");

     // see if free format needed

     for (i = 2; i < argc; i++) {
          if (!strncmp(argv[i], "free", 4)) {
               freeFormat = true;
          }
     }

     // create model from MPS file

     if (!freeFormat) {
          ekk_importModel(model, name);
     } else {
          ekk_importModelFree(model, name);
     }

     // other options
     for (i = 2; i < argc; i++) {
          if (!strncmp(argv[i], "max", 3)) {
               if (!strncmp(argv[i], "max2", 4)) {
                    // This is for testing - it just reverses signs and maximizes
                    int i, n = ekk_getInumcols(model);
                    double * objective = ekk_getObjective(model);
                    for (i = 0; i < n; i++) {
                         objective[i] = -objective[i];
                    }
                    ekk_setObjective(model, objective);
                    ekk_setMaximize(model);
               } else {
                    // maximize
                    ekk_setMaximize(model);
               }
          }
          if (!strncmp(argv[i], "dual", 4)) {
               primal = false;
          }
          if (!strncmp(argv[i], "presol", 6)) {
               presolve = true;
          }
          if (!strncmp(argv[i], "osl", 3)) {
               useosl = 1;
          }
          if (!strncmp(argv[i], "both", 4)) {
               useosl = 2;
          }
          if (!strncmp(argv[i], "export", 6)) {
               exportIt = true;
          }
     }
     if (useosl) {
          // OSL
          if (presolve)
               ekk_preSolve(model, 3, NULL);
          ekk_scale(model);

          if (primal)
               ekk_primalSimplex(model, 1);
          else
               ekk_dualSimplex(model);
          if (presolve) {
               ekk_postSolve(model, NULL);
               ekk_primalSimplex(model, 3);
          }
          if (useosl == 2)
               ekk_allSlackBasis(model);    // otherwise it would be easy
     }
     if ((useosl & 2) == 0) {
          // CLP
          if (presolve)
               ekk_preSolveClp(model, true, 5);
          /* 3 is because it is ignored if no presolve, and we
             are forcing Clp to re-optimize */
          if (primal)
               ekk_primalClp(model, 1, 3);
          else
               ekk_dualClp(model, 3);
     }
     if (exportIt) {
          ClpSimplex * clp = new ClpSimplex();;
          int numberRows = ekk_getInumrows(model);
          int numberColumns = ekk_getInumcols(model);
          clp->loadProblem(numberColumns, numberRows, ekk_blockColumn(model, 0),
                           ekk_blockRow(model, 0), ekk_blockElement(model, 0),
                           ekk_collower(model), ekk_colupper(model),
                           ekk_objective(model),
                           ekk_rowlower(model), ekk_rowupper(model));
          // Do integer stuff
          int * which =  ekk_listOfIntegers(model);
          int numberIntegers =  ekk_getInumints(model);
          for (int i = 0; i < numberIntegers; i++)
               clp->setInteger(which[i]);
          ekk_free(which);
          clp->writeMps("try1.mps");
          delete clp;
     }
     ekk_deleteModel(model);
     ekk_endContext(context);
     return 0;
}
