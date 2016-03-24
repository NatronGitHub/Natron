/* $Id$ */
/*
  Copyright (C) 2003 International Business Machines
  Corporation and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/

/* This example shows the use of the "C" interface */

#include "Clp_C_Interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Call back function - just says whenever it gets Clp0005 or Coin0002 */
static void callBack(Clp_Simplex * model, int messageNumber,
                     int nDouble, const double * vDouble,
                     int nInt, const int * vInt,
                     int nString, char ** vString)
{
     if (messageNumber == 1000002) {
          /* Coin0002 */
          assert(nString == 1 && nInt == 3);
          printf("Name of problem is %s\n", vString[0]);
          printf("row %d col %d el %d\n", vInt[0], vInt[1], vInt[2]);
     } else if (messageNumber == 5) {
          /* Clp0005 */
          int i;
          assert(nInt == 4 && nDouble == 3);    /* they may not all print */
          for (i = 0; i < 3; i++)
               printf("%d %g\n", vInt[i], vDouble[i]);
     }
}



int main(int argc, const char *argv[])
{
     /* Get default model */
     Clp_Simplex  * model = Clp_newModel();
     int status;
     /* register callback */
     Clp_registerCallBack(model, callBack);
     /* Keep names when reading an mps file */
     if (argc < 2) {
#if defined(SAMPLEDIR)
          status = Clp_readMps(model, SAMPLEDIR "/p0033.mps", 1, 0);
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
     } else
          status = Clp_readMps(model, argv[1], 1, 0);

     if (status) {
          fprintf(stderr, "Bad readMps %s\n", argv[1]);
          fprintf(stdout, "Bad readMps %s\n", argv[1]);
          exit(1);
     }

     if (argc < 3 || !strstr(argv[2], "primal")) {
          /* Use the dual algorithm unless user said "primal" */
          Clp_initialDualSolve(model);
     } else {
          Clp_initialPrimalSolve(model);
     }

     {
          char modelName[80];
          Clp_problemName(model, 80, modelName);
          printf("Model %s has %d rows and %d columns\n",
                 modelName, Clp_numberRows(model), Clp_numberColumns(model));
     }

     /* remove this to print solution */

     /*exit(0); */

     {
          /*
            Now to print out solution.  The methods used return modifiable
            arrays while the alternative names return const pointers -
            which is of course much more virtuous.

            This version just does non-zero columns

          */

          /* Columns */

          int numberColumns = Clp_numberColumns(model);
          int iColumn;


          /* Alternatively getColSolution(model) */
          double * columnPrimal = Clp_primalColumnSolution(model);
          /* Alternatively getReducedCost(model) */
          double * columnDual = Clp_dualColumnSolution(model);
          /* Alternatively getColLower(model) */
          double * columnLower = Clp_columnLower(model);
          /* Alternatively getColUpper(model) */
          double * columnUpper = Clp_columnUpper(model);
          /* Alternatively getObjCoefficients(model) */
          double * columnObjective = Clp_objective(model);

          printf("--------------------------------------\n");
          /* If we have not kept names (parameter to readMps) this will be 0 */
          assert(Clp_lengthNames(model));

          printf("                       Primal          Dual         Lower         Upper          Cost\n");

          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               double value;
               value = columnPrimal[iColumn];
               if (value > 1.0e-8 || value < -1.0e-8) {
                    char name[20];
                    Clp_columnName(model, iColumn, name);
                    printf("%6d %8s", iColumn, name);
                    printf(" %13g", columnPrimal[iColumn]);
                    printf(" %13g", columnDual[iColumn]);
                    printf(" %13g", columnLower[iColumn]);
                    printf(" %13g", columnUpper[iColumn]);
                    printf(" %13g", columnObjective[iColumn]);
                    printf("\n");
               }
          }
          printf("--------------------------------------\n");
     }
     return 0;
}
