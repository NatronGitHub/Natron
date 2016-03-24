/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "ClpPresolve.hpp"
#include "CoinSort.hpp"
#include <iomanip>

int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;
     // Keep names
     if (argc < 2) {
          status = model.readMps("small.mps", true);
     } else {
          status = model.readMps(argv[1], true);
     }
     if (status)
          exit(10);
     /*
       This driver implements the presolve variation of Sprint.
       This assumes we can get feasible easily
     */

     int numberRows = model.numberRows();
     int numberColumns = model.numberColumns();

     // We will need arrays to choose variables.  These are too big but ..
     double * weight = new double [numberRows+numberColumns];
     int * sort = new int [numberRows+numberColumns];

     double * columnLower = model.columnLower();
     double * columnUpper = model.columnUpper();
     double * saveLower = new double [numberColumns];
     memcpy(saveLower, columnLower, numberColumns * sizeof(double));
     double * saveUpper = new double [numberColumns];
     memcpy(saveUpper, columnUpper, numberColumns * sizeof(double));
     double * solution = model.primalColumnSolution();
     // Fix in some magical way so remaining problem is easy
#if 0
     // This is from a real-world problem
     for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
          char firstCharacter = model.columnName(iColumn)[0];
          if (firstCharacter == 'F' || firstCharacter == 'P'
                    || firstCharacter == 'L' || firstCharacter == 'T') {
               columnUpper[iColumn] = columnLower[iColumn];
          }
     }
#else
     double * obj = model.objective();
     double * saveObj = new double [numberColumns];
     memcpy(saveObj, obj, numberColumns * sizeof(double));
     memset(obj, 0, numberColumns * sizeof(double));
     model.dual();
     memcpy(obj, saveObj, numberColumns * sizeof(double));
     delete [] saveObj;
     for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
         if (solution[iColumn]<columnLower[iColumn]+1.0e-8) {
	     columnUpper[iColumn] = columnLower[iColumn];
       }
     }
#endif

     // Just do this number of passes
     int maxPass = 100;
     int iPass;
     double lastObjective = 1.0e31;

     // Just take this number of columns in small problem
     int smallNumberColumns = 3 * numberRows;
     // To stop seg faults on unsuitable problems
     smallNumberColumns = CoinMin(smallNumberColumns,numberColumns);
     // And we want number of rows to be this
     int smallNumberRows = numberRows / 4;

     for (iPass = 0; iPass < maxPass; iPass++) {
          printf("Start of pass %d\n", iPass);
          ClpSimplex * model2;
          ClpPresolve pinfo;
          int numberPasses = 1; // can change this
          model2 = pinfo.presolvedModel(model, 1.0e-8, false, numberPasses, false);
          if (!model2) {
               fprintf(stdout, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                       argv[1], 1.0e-8);
               // model was infeasible - maybe try again with looser tolerances
               model2 = pinfo.presolvedModel(model, 1.0e-7, false, numberPasses, false);
               if (!model2) {
                    fprintf(stdout, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                            argv[1], 1.0e-7);
                    exit(2);
               }
          }
          // change factorization frequency from 200
          model2->setFactorizationFrequency(100 + model2->numberRows() / 50);
          model2->primal();
          pinfo.postsolve(true);

          // adjust smallNumberColumns if necessary
          if (iPass) {
               double ratio = ((double) smallNumberRows) / ((double) model2->numberRows());
               smallNumberColumns = (int)(smallNumberColumns * ratio);
	       // deal with pathological case
	       smallNumberColumns = CoinMax(smallNumberColumns,0);
          }
          delete model2;
          /* After this postsolve model should be optimal.
             We can use checkSolution and test feasibility */
          model.checkSolution();
          if (model.numberDualInfeasibilities() ||
                    model.numberPrimalInfeasibilities())
               printf("%g dual %g(%d) Primal %g(%d)\n",
                      model.objectiveValue(),
                      model.sumDualInfeasibilities(),
                      model.numberDualInfeasibilities(),
                      model.sumPrimalInfeasibilities(),
                      model.numberPrimalInfeasibilities());
          // Put back true bounds
          memcpy(columnLower, saveLower, numberColumns * sizeof(double));
          memcpy(columnUpper, saveUpper, numberColumns * sizeof(double));
          if ((model.objectiveValue() > lastObjective - 1.0e-7 && iPass > 5) ||
                    iPass == maxPass - 1) {
               break; // finished
          } else {
               lastObjective = model.objectiveValue();
               // now massage weight so all basic in plus good djs
	       const double * djs = model.dualColumnSolution();
               for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
                    double dj = djs[iColumn];
                    double value = solution[iColumn];
                    if (model.getStatus(iColumn) == ClpSimplex::basic)
                         dj = -1.0e50;
                    else if (dj < 0.0 && value < columnUpper[iColumn])
                         dj = dj;
                    else if (dj > 0.0 && value > columnLower[iColumn])
                         dj = -dj;
                    else if (columnUpper[iColumn] > columnLower[iColumn])
                         dj = fabs(dj);
                    else
                         dj = 1.0e50;
                    weight[iColumn] = dj;
                    sort[iColumn] = iColumn;
               }
               // sort
               CoinSort_2(weight, weight + numberColumns, sort);
               // and fix others
               for (int iColumn = smallNumberColumns; iColumn < numberColumns; iColumn++) {
                    int kColumn = sort[iColumn];
                    double value = solution[kColumn];
                    columnLower[kColumn] = value;
                    columnUpper[kColumn] = value;
               }
          }
     }
     delete [] weight;
     delete [] sort;
     delete [] saveLower;
     delete [] saveUpper;
     model.primal(1);
     return 0;
}
