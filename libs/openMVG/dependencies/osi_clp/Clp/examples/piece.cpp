/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* This simple example takes a matrix read in by CoinMpsIo,
   deletes every second column and solves the resulting problem */

#include "ClpSimplex.hpp"
#include "ClpNonLinearCost.hpp"
#include "CoinMpsIO.hpp"
#include <iomanip>

int main(int argc, const char *argv[])
{
     int status;
     CoinMpsIO m;
     if (argc < 2)
          status = m.readMps("model1.mps", "");
     else
          status = m.readMps(argv[1], "");

     if (status) {
          fprintf(stdout, "Bad readMps %s\n", argv[1]);
          exit(1);
     }

     // Load up model1 - so we can use known good solution
     ClpSimplex model1;
     model1.loadProblem(*m.getMatrixByCol(),
                        m.getColLower(), m.getColUpper(),
                        m.getObjCoefficients(),
                        m.getRowLower(), m.getRowUpper());
     model1.dual();
     // Get data arrays
     const CoinPackedMatrix * matrix1 = m.getMatrixByCol();
     const int * start1 = matrix1->getVectorStarts();
     const int * length1 = matrix1->getVectorLengths();
     const int * row1 = matrix1->getIndices();
     const double * element1 = matrix1->getElements();

     const double * columnLower1 = m.getColLower();
     const double * columnUpper1 = m.getColUpper();
     const double * rowLower1 = m.getRowLower();
     const double * rowUpper1 = m.getRowUpper();
     const double * objective1 = m.getObjCoefficients();

     int numberColumns = m.getNumCols();
     int numberRows = m.getNumRows();
     int numberElements = m.getNumElements();

     // Get new arrays
     int numberColumns2 = (numberColumns + 1);
     int * start2 = new int[numberColumns2+1];
     int * row2 = new int[numberElements];
     double * element2 = new double[numberElements];
     int * segstart = new int[numberColumns+1];
     double * breakpt  = new double[2*numberColumns];
     double * slope  = new double[2*numberColumns];

     double * objective2 = new double[numberColumns2];
     double * columnLower2 = new double[numberColumns2];
     double * columnUpper2 = new double[numberColumns2];
     double * rowLower2 = new double[numberRows];
     double * rowUpper2 = new double[numberRows];

     // We need to modify rhs
     memcpy(rowLower2, rowLower1, numberRows * sizeof(double));
     memcpy(rowUpper2, rowUpper1, numberRows * sizeof(double));
     double objectiveOffset = 0.0;

     // For new solution
     double * newSolution = new double [numberColumns];
     const double * oldSolution = model1.primalColumnSolution();

     int iColumn;
     for (iColumn = 0; iColumn < numberColumns; iColumn++)
          printf("%g ", oldSolution[iColumn]);
     printf("\n");

     numberColumns2 = 0;
     numberElements = 0;
     start2[0] = 0;
     int segptr = 0;

     segstart[0] = 0;

     // Now check for duplicates
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          // test if column identical to next column
          bool ifcopy = 1;
          if (iColumn < numberColumns - 1) {
               int  joff = length1[iColumn];
               for (int j = start1[iColumn]; j < start1[iColumn] + length1[iColumn]; j++) {
                    if (row1[j] != row1[j+joff]) {
                         ifcopy = 0;
                         break;
                    }
                    if (element1[j] != element1[j+joff]) {
                         ifcopy = 0;
                         break;
                    }
               }
          } else {
               ifcopy = 0;
          }
          //if (iColumn>47||iColumn<45)
          //ifcopy=0;
          if (ifcopy) {
               double lo1 = columnLower1[iColumn];
               double up1 = columnUpper1[iColumn];
               double obj1 = objective1[iColumn];
               double sol1 = oldSolution[iColumn];
               double lo2 = columnLower1[iColumn+1];
               double up2 = columnUpper1[iColumn+1];
               double obj2 = objective1[iColumn+1];
               double sol2 = oldSolution[iColumn+1];
               if (fabs(up1 - lo2) > 1.0e-8) {
                    // try other way
                    double temp;
                    temp = lo1;
                    lo1 = lo2;
                    lo2 = temp;
                    temp = up1;
                    up1 = up2;
                    up2 = temp;
                    temp = obj1;
                    obj1 = obj2;
                    obj2 = temp;
                    temp = sol1;
                    sol1 = sol2;
                    sol2 = temp;
                    assert(fabs(up1 - lo2) < 1.0e-8);
               }
               // subtract out from rhs
               double fixed = up1;
               // do offset
               objectiveOffset += fixed * obj2;
               for (int j = start1[iColumn]; j < start1[iColumn] + length1[iColumn]; j++) {
                    int iRow = row1[j];
                    double value = element1[j];
                    if (rowLower2[iRow] > -1.0e30)
                         rowLower2[iRow] -= value * fixed;
                    if (rowUpper2[iRow] < 1.0e30)
                         rowUpper2[iRow] -= value * fixed;
               }
               newSolution[numberColumns2] = fixed;
               if (fabs(sol1 - fixed) > 1.0e-8)
                    newSolution[numberColumns2] = sol1;
               if (fabs(sol2 - fixed) > 1.0e-8)
                    newSolution[numberColumns2] = sol2;
               columnLower2[numberColumns2] = lo1;
               columnUpper2[numberColumns2] = up2;
               objective2[numberColumns2] = 0.0;
               breakpt[segptr] = lo1;
               slope[segptr++] = obj1;
               breakpt[segptr] = lo2;
               slope[segptr++] = obj2;
               for (int j = start1[iColumn]; j < start1[iColumn] + length1[iColumn]; j++) {
                    row2[numberElements] = row1[j];
                    element2[numberElements++] = element1[j];
               }
               start2[++numberColumns2] = numberElements;
               breakpt[segptr] = up2;
               slope[segptr++] = COIN_DBL_MAX;
               segstart[numberColumns2] = segptr;
               iColumn++; // skip next column
          } else {
               // normal column
               columnLower2[numberColumns2] = columnLower1[iColumn];
               columnUpper2[numberColumns2] = columnUpper1[iColumn];
               objective2[numberColumns2] = objective1[iColumn];
               breakpt[segptr] = columnLower1[iColumn];
               slope[segptr++] = objective1[iColumn];
               for (int j = start1[iColumn]; j < start1[iColumn] + length1[iColumn]; j++) {
                    row2[numberElements] = row1[j];
                    element2[numberElements++] = element1[j];
               }
               newSolution[numberColumns2] = oldSolution[iColumn];
               start2[++numberColumns2] = numberElements;
               breakpt[segptr] = columnUpper1[iColumn];
               slope[segptr++] = COIN_DBL_MAX;
               segstart[numberColumns2] = segptr;
          }
     }

     // print new number of columns, elements
     printf("New number of columns  = %d\n", numberColumns2);
     printf("New number of elements = %d\n", numberElements);
     printf("Objective offset is %g\n", objectiveOffset);


     ClpSimplex  model;

     // load up
     model.loadProblem(numberColumns2, numberRows,
                       start2, row2, element2,
                       columnLower2, columnUpper2,
                       objective2,
                       rowLower2, rowUpper2);
     model.scaling(0);
     model.setDblParam(ClpObjOffset, -objectiveOffset);
     // Create nonlinear objective
     int returnCode = model.createPiecewiseLinearCosts(segstart, breakpt, slope);
     if( returnCode != 0 )
     {
        printf("Unexpected return code %d from model.createPiecewiseLinearCosts()\n", returnCode);
        return returnCode;
     }

     // delete
     delete [] segstart;
     delete [] breakpt;
     delete [] slope;
     delete [] start2;
     delete [] row2 ;
     delete [] element2;

     delete [] objective2;
     delete [] columnLower2;
     delete [] columnUpper2;
     delete [] rowLower2;
     delete [] rowUpper2;

     // copy in solution - (should be optimal)
     model.allSlackBasis();
     memcpy(model.primalColumnSolution(), newSolution, numberColumns2 * sizeof(double));
     //memcpy(model.columnLower(),newSolution,numberColumns2*sizeof(double));
     //memcpy(model.columnUpper(),newSolution,numberColumns2*sizeof(double));
     delete [] newSolution;
     //model.setLogLevel(63);

     const double * solution = model.primalColumnSolution();
     double * saveSol = new double[numberColumns2];
     memcpy(saveSol, solution, numberColumns2 * sizeof(double));
     for (iColumn = 0; iColumn < numberColumns2; iColumn++)
          printf("%g ", solution[iColumn]);
     printf("\n");
     // solve
     model.primal(1);
     for (iColumn = 0; iColumn < numberColumns2; iColumn++) {
          if (fabs(solution[iColumn] - saveSol[iColumn]) > 1.0e-3)
               printf(" ** was %g ", saveSol[iColumn]);
          printf("%g ", solution[iColumn]);
     }
     printf("\n");
     model.primal(1);
     for (iColumn = 0; iColumn < numberColumns2; iColumn++) {
          if (fabs(solution[iColumn] - saveSol[iColumn]) > 1.0e-3)
               printf(" ** was %g ", saveSol[iColumn]);
          printf("%g ", solution[iColumn]);
     }
     printf("\n");
     model.primal();
     for (iColumn = 0; iColumn < numberColumns2; iColumn++) {
          if (fabs(solution[iColumn] - saveSol[iColumn]) > 1.0e-3)
               printf(" ** was %g ", saveSol[iColumn]);
          printf("%g ", solution[iColumn]);
     }
     printf("\n");
     model.allSlackBasis();
     for (iColumn = 0; iColumn < numberColumns2; iColumn++) {
          if (fabs(solution[iColumn] - saveSol[iColumn]) > 1.0e-3)
               printf(" ** was %g ", saveSol[iColumn]);
          printf("%g ", solution[iColumn]);
     }
     printf("\n");
     model.setLogLevel(63);
     model.primal();
     for (iColumn = 0; iColumn < numberColumns2; iColumn++) {
          if (fabs(solution[iColumn] - saveSol[iColumn]) > 1.0e-3)
               printf(" ** was %g ", saveSol[iColumn]);
          printf("%g ", solution[iColumn]);
     }
     printf("\n");
     return 0;
}
