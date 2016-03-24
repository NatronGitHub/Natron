/* $Id$ */
/*
  Copyright (C) 2003, International Business Machines Corporation and others.
  All Rights Reserved.

  This sample program is designed to illustrate programming techniques
  using CoinLP, has not been thoroughly tested and comes without any
  warranty whatsoever.

  You may copy, modify and distribute this sample program without any
  restrictions whatsoever and without any payment to anyone.
*/

#include "ClpSimplex.hpp"
#include "ClpPresolve.hpp"
#include "ClpFactorization.hpp"
#include "CoinSort.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "CoinMpsIO.hpp"
#include <iomanip>

int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;
     // Keep names
     if (argc < 2) {
          status = model.readMps("small.mps", true);
     } else {
          status = model.readMps(argv[1], false);
     }
     if (status)
          exit(10);
     /*
       This driver implements a method of treating a problem as all cuts.
       So it adds in all E rows, solves and then adds in violated rows.
     */

     double time1 = CoinCpuTime();
     ClpSimplex * model2;
     ClpPresolve pinfo;
     int numberPasses = 5; // can change this
     /* Use a tolerance of 1.0e-8 for feasibility, treat problem as
        not being integer, do "numberpasses" passes and throw away names
        in presolved model */
     model2 = pinfo.presolvedModel(model, 1.0e-8, false, numberPasses, false);
     if (!model2) {
          fprintf(stderr, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                  argv[1], 1.0e-8);
          fprintf(stdout, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                  argv[1], 1.0e-8);
          // model was infeasible - maybe try again with looser tolerances
          model2 = pinfo.presolvedModel(model, 1.0e-7, false, numberPasses, false);
          if (!model2) {
               fprintf(stderr, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                       argv[1], 1.0e-7);
               fprintf(stdout, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                       argv[1], 1.0e-7);
               exit(2);
          }
     }
     // change factorization frequency from 200
     model2->setFactorizationFrequency(100 + model2->numberRows() / 50);

     int numberColumns = model2->numberColumns();
     int originalNumberRows = model2->numberRows();

     // We will need arrays to choose rows to add
     double * weight = new double [originalNumberRows];
     int * sort = new int [originalNumberRows];
     int numberSort = 0;
     char * take = new char [originalNumberRows];

     const double * rowLower = model2->rowLower();
     const double * rowUpper = model2->rowUpper();
     int iRow, iColumn;
     // Set up initial list
     numberSort = 0;
     for (iRow = 0; iRow < originalNumberRows; iRow++) {
          weight[iRow] = 1.123e50;
          if (rowLower[iRow] == rowUpper[iRow]) {
               sort[numberSort++] = iRow;
               weight[iRow] = 0.0;
          }
     }
     numberSort /= 2;
     // Just add this number of rows each time in small problem
     int smallNumberRows = 2 * numberColumns;
     smallNumberRows = CoinMin(smallNumberRows, originalNumberRows / 20);
     // and pad out with random rows
     double ratio = ((double)(smallNumberRows - numberSort)) / ((double) originalNumberRows);
     for (iRow = 0; iRow < originalNumberRows; iRow++) {
          if (weight[iRow] == 1.123e50 && CoinDrand48() < ratio)
               sort[numberSort++] = iRow;
     }
     /* This is optional.
        The best thing to do is to miss out random rows and do a set which makes dual feasible.
        If that is not possible then make sure variables have bounds.

        One way that normally works is to automatically tighten bounds.
     */
#if 0
     // However for some we need to do anyway
     double * columnLower = model2->columnLower();
     double * columnUpper = model2->columnUpper();
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          columnLower[iColumn] = CoinMax(-1.0e6, columnLower[iColumn]);
          columnUpper[iColumn] = CoinMin(1.0e6, columnUpper[iColumn]);
     }
#endif
     model2->tightenPrimalBounds(-1.0e4, true);
     printf("%d rows in initial problem\n", numberSort);
     double * fullSolution = model2->primalRowSolution();

     // Just do this number of passes
     int maxPass = 50;
     // And take out slack rows until this pass
     int takeOutPass = 30;
     int iPass;

     const int * start = model2->clpMatrix()->getVectorStarts();
     const int * length = model2->clpMatrix()->getVectorLengths();
     const int * row = model2->clpMatrix()->getIndices();
     int * whichColumns = new int [numberColumns];
     for (iColumn = 0; iColumn < numberColumns; iColumn++)
          whichColumns[iColumn] = iColumn;
     int numberSmallColumns = numberColumns;
     for (iPass = 0; iPass < maxPass; iPass++) {
          printf("Start of pass %d\n", iPass);
          // Cleaner this way
          std::sort(sort, sort + numberSort);
          // Create small problem
          ClpSimplex small(model2, numberSort, sort, numberSmallColumns, whichColumns);
          small.setFactorizationFrequency(100 + numberSort / 200);
          //small.setPerturbation(50);
          //small.setLogLevel(63);
          // A variation is to just do N iterations
          //if (iPass)
          //small.setMaximumIterations(100);
          // Solve
          small.factorization()->messageLevel(8);
          if (iPass) {
               small.dual();
          } else {
               small.writeMps("continf.mps");
               ClpSolve solveOptions;
               solveOptions.setSolveType(ClpSolve::useDual);
               //solveOptions.setSolveType(ClpSolve::usePrimalorSprint);
               //solveOptions.setSpecialOption(1,2,200); // idiot
               small.initialSolve(solveOptions);
          }
          bool dualInfeasible = (small.status() == 2);
          // move solution back
          double * solution = model2->primalColumnSolution();
          const double * smallSolution = small.primalColumnSolution();
          for (int j = 0; j < numberSmallColumns; j++) {
               iColumn = whichColumns[j];
               solution[iColumn] = smallSolution[j];
               model2->setColumnStatus(iColumn, small.getColumnStatus(j));
          }
          for (iRow = 0; iRow < numberSort; iRow++) {
               int kRow = sort[iRow];
               model2->setRowStatus(kRow, small.getRowStatus(iRow));
          }
          // compute full solution
          memset(fullSolution, 0, originalNumberRows * sizeof(double));
          model2->times(1.0, model2->primalColumnSolution(), fullSolution);
          if (iPass != maxPass - 1) {
               // Mark row as not looked at
               for (iRow = 0; iRow < originalNumberRows; iRow++)
                    weight[iRow] = 1.123e50;
               // Look at rows already in small problem
               int iSort;
               int numberDropped = 0;
               int numberKept = 0;
               int numberBinding = 0;
               int numberInfeasibilities = 0;
               double sumInfeasibilities = 0.0;
               for (iSort = 0; iSort < numberSort; iSort++) {
                    iRow = sort[iSort];
                    //printf("%d %g %g\n",iRow,fullSolution[iRow],small.primalRowSolution()[iSort]);
                    if (model2->getRowStatus(iRow) == ClpSimplex::basic) {
                         // Basic - we can get rid of if early on
                         if (iPass < takeOutPass && !dualInfeasible) {
                              // may have hit max iterations so check
                              double infeasibility = CoinMax(fullSolution[iRow] - rowUpper[iRow],
                                                             rowLower[iRow] - fullSolution[iRow]);
                              weight[iRow] = -infeasibility;
                              if (infeasibility > 1.0e-8) {
                                   numberInfeasibilities++;
                                   sumInfeasibilities += infeasibility;
                              } else {
                                   weight[iRow] = 1.0;
                                   numberDropped++;
                              }
                         } else {
                              // keep
                              weight[iRow] = -1.0e40;
                              numberKept++;
                         }
                    } else {
                         // keep
                         weight[iRow] = -1.0e50;
                         numberKept++;
                         numberBinding++;
                    }
               }
               // Now rest
               for (iRow = 0; iRow < originalNumberRows; iRow++) {
                    sort[iRow] = iRow;
                    if (weight[iRow] == 1.123e50) {
                         // not looked at yet
                         double infeasibility = CoinMax(fullSolution[iRow] - rowUpper[iRow],
                                                        rowLower[iRow] - fullSolution[iRow]);
                         weight[iRow] = -infeasibility;
                         if (infeasibility > 1.0e-8) {
                              numberInfeasibilities++;
                              sumInfeasibilities += infeasibility;
                         }
                    }
               }
               // sort
               CoinSort_2(weight, weight + originalNumberRows, sort);
               numberSort = CoinMin(originalNumberRows, smallNumberRows + numberKept);
               memset(take, 0, originalNumberRows);
               for (iRow = 0; iRow < numberSort; iRow++)
                    take[sort[iRow]] = 1;
               numberSmallColumns = 0;
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    int n = 0;
                    for (int j = start[iColumn]; j < start[iColumn] + length[iColumn]; j++) {
                         int iRow = row[j];
                         if (take[iRow])
                              n++;
                    }
                    if (n)
                         whichColumns[numberSmallColumns++] = iColumn;
               }
               printf("%d rows binding, %d rows kept, %d rows dropped - new size %d rows, %d columns\n",
                      numberBinding, numberKept, numberDropped, numberSort, numberSmallColumns);
               printf("%d rows are infeasible - sum is %g\n", numberInfeasibilities,
                      sumInfeasibilities);
               if (!numberInfeasibilities) {
                    printf("Exiting as looks optimal\n");
                    break;
               }
               numberInfeasibilities = 0;
               sumInfeasibilities = 0.0;
               for (iSort = 0; iSort < numberSort; iSort++) {
                    if (weight[iSort] > -1.0e30 && weight[iSort] < -1.0e-8) {
                         numberInfeasibilities++;
                         sumInfeasibilities += -weight[iSort];
                    }
               }
               printf("in small model %d rows are infeasible - sum is %g\n", numberInfeasibilities,
                      sumInfeasibilities);
          }
     }
     delete [] weight;
     delete [] sort;
     delete [] whichColumns;
     delete [] take;
     // If problem is big you may wish to skip this
     model2->dual();
     int numberBinding = 0;
     for (iRow = 0; iRow < originalNumberRows; iRow++) {
          if (model2->getRowStatus(iRow) != ClpSimplex::basic)
               numberBinding++;
     }
     printf("%d binding rows at end\n", numberBinding);
     pinfo.postsolve(true);

     int numberIterations = model2->numberIterations();;
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
     // But resolve for safety
     model.primal(1);

     numberIterations += model.numberIterations();;
     printf("Solve took %g seconds\n", CoinCpuTime() - time1);
     return 0;
}
