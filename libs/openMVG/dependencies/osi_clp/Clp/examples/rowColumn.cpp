/* $Id$ */
/* Copyright (C) 2004, International Business Machines Corporation
   and others.  All Rights Reserved.

   This sample program is designed to illustrate programming
   techniques using CoinLP, has not been thoroughly tested
   and comes without any warranty whatsoever.

   You may copy, modify and distribute this sample program without
   any restrictions whatsoever and without any payment to anyone.
*/

#include "ClpSimplex.hpp"
#include <cstdio>
#include <cassert>

int main(int argc, const char *argv[])
{
     ClpSimplex  modelByRow, modelByColumn;

     // This very simple example shows how to create a model by row and by column
     int numberRows = 3;
     int numberColumns = 5;
     // Rim of problem is same in both cases
     double objective [] = {1000.0, 400.0, 500.0, 10000.0, 10000.0};
     double columnLower[] = {0.0, 0.0, 0.0, 0.0, 0.0};
     double columnUpper[] = {COIN_DBL_MAX, COIN_DBL_MAX, COIN_DBL_MAX, 20.0, 20.0};
     double rowLower[] = {20.0, -COIN_DBL_MAX, 8.0};
     double rowUpper[] = {COIN_DBL_MAX, 30.0, 8.0};
     // Matrix by row
     int rowStart[] = {0, 5, 10, 13};
     int column[] = {0, 1, 2, 3, 4,
                     0, 1, 2, 3, 4,
                     0, 1, 2
                    };
     double elementByRow[] = {8.0, 5.0, 4.0, 4.0, -4.0,
                              8.0, 4.0, 5.0, 5.0, -5.0,
                              1.0, -1.0, -1.0
                             };
     // Matrix by column
     int columnStart[] = {0, 3, 6, 9, 11, 13};
     int row[] = {0, 1, 2,
                  0, 1, 2,
                  0, 1, 2,
                  0, 1,
                  0, 1
                 };
     double elementByColumn[] = {8.0, 8.0, 1.0,
                                 5.0, 4.0, -1.0,
                                 4.0, 5.0, -1.0,
                                 4.0, 5.0,
                                 -4.0, -5.0
                                };
     int numberElements;
     // Do column version first as it can be done two ways
     // a) As one step using matrix as stored
     modelByColumn.loadProblem(numberColumns, numberRows, columnStart, row, elementByColumn,
                               columnLower, columnUpper, objective,
                               rowLower, rowUpper);
     // Solve
     modelByColumn.dual();
     // check value of objective
     assert(fabs(modelByColumn.objectiveValue() - 76000.0) < 1.0e-7);
     // b) As two steps - first creating a CoinPackedMatrix
     // NULL for column lengths indicate they are stored without gaps
     // Look at CoinPackedMatrix.hpp for other ways to create a matrix
     numberElements = columnStart[numberColumns];
     CoinPackedMatrix byColumn(true, numberRows, numberColumns, numberElements,
                               elementByColumn, row, columnStart, NULL);
     // now load matrix and rim
     modelByColumn.loadProblem(byColumn,
                               columnLower, columnUpper, objective,
                               rowLower, rowUpper);
     // Solve
     modelByColumn.dual();
     // check value of objective
     assert(fabs(modelByColumn.objectiveValue() - 76000.0) < 1.0e-7);
     // Now do by row
     // The false says row ordered so numberRows and numberColumns swapped - see CoinPackedMatrix.hpp
     assert(numberElements == rowStart[numberRows]);    // check same number of elements in each copy
     CoinPackedMatrix byRow(false, numberColumns, numberRows, numberElements,
                            elementByRow, column, rowStart, NULL);
     // now load matrix and rim
     modelByRow.loadProblem(byRow,
                            columnLower, columnUpper, objective,
                            rowLower, rowUpper);
     // Solve
     modelByRow.dual();
     // check value of objective
     assert(fabs(modelByRow.objectiveValue() - 76000.0) < 1.0e-7);
     // write solution
     const double * solution = modelByRow.primalColumnSolution();
     for (int i = 0; i < numberColumns; i++) {
          if (solution[i])
               printf("Column %d has value %g\n",
                      i, solution[i]);
     }
     modelByRow.writeMps("Tiny.mps");
     return 0;
}
