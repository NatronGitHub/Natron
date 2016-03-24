/* $Id$ */
// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*
  This example shows the creation of a model from arrays, solution
  and then changes to objective and adding a row
*/
#include "ClpSimplex.hpp"
#include "CoinHelperFunctions.hpp"
int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     // model is as exmip1.mps from Data/samples
     int numberRows = 5;
     int numberColumns = 8;
     int numberElements = 14;
     // matrix data - column ordered
     CoinBigIndex start[9] = {0, 2, 4, 6, 8, 10, 11, 12, 14};
     int length[8] = {2, 2, 2, 2, 2, 1, 1, 2};
     int rows[14] = {0, 4, 0, 1, 1, 2, 0, 3, 0, 4, 2, 3, 0, 4};
     double elements[14] = {3, 5.6, 1, 2, 1.1, 1, -2, 2.8, -1, 1, 1, -1.2, -1, 1.9};
     CoinPackedMatrix matrix(true, numberRows, numberColumns, numberElements, elements, rows, start, length);

     // rim data
     double objective[8] = {1, 0, 0, 0, 2, 0, 0, -1};
     double rowLower[5] = {2.5, -COIN_DBL_MAX, 4, 1.8, 3};
     double rowUpper[5] = {COIN_DBL_MAX, 2.1, 4, 5, 15};
     double colLower[8] = {2.5, 0, 0, 0, 0.5, 0, 0, 0};
     double colUpper[8] = {COIN_DBL_MAX, 4.1, 1, 1, 4, COIN_DBL_MAX, COIN_DBL_MAX, 4.3};
     // load problem
     model.loadProblem(matrix, colLower, colUpper, objective,
                       rowLower, rowUpper);
     // mark integer (really for Cbc/examples/modify.cpp
     model.setInteger(2);
     model.setInteger(3);

     // Solve
     model.initialSolve();

     // Solution
     const double * solution = model.primalColumnSolution();
     int i;
     for (i = 0; i < numberColumns; i++)
          if (solution[i])
               printf("Column %d has value %g\n", i, solution[i]);

     // Change objective
     double * objective2 = model.objective();
     objective2[0] = -100.0;

     // Solve - primal as primal feasible
     model.primal(1);

     // Solution (array won't have changed)
     for (i = 0; i < numberColumns; i++)
          if (solution[i])
               printf("Column %d has value %g\n", i, solution[i]);

     // Add constraint
     int column[8] = {0, 1, 2, 3, 4, 5, 6, 7};
     double element2[8] = {1, 1, 1, 1, 1, 1, 1, 1};
     model.addRow(8, column, element2, 7.8, COIN_DBL_MAX);

     // Solve - dual as dual feasible
     model.dual();

     /* Solution
        This time we have changed arrays of solver so -
        array won't have changed as column array and we added a row
        - but be on safe side
     */
     solution = model.primalColumnSolution();
     for (i = 0; i < numberColumns; i++)
          if (solution[i])
               printf("Column %d has value %g\n", i, solution[i]);

     return 0;
}
