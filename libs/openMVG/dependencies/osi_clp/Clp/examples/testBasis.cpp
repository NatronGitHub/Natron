/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "CoinHelperFunctions.hpp"
#include <iomanip>
#include <cassert>

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
       This driver turns a problem into all equalities, solves it and
       then creates optimal basis.
     */

     // copy of original
     ClpSimplex model2(model);
     // And another
     ClpSimplex model3(model);

     int originalNumberColumns = model.numberColumns();
     int numberRows = model.numberRows();

     int * addStarts = new int [numberRows+1];
     int * addRow = new int[numberRows];
     double * addElement = new double[numberRows];
     double * newUpper = new double[numberRows];
     double * newLower = new double[numberRows];

     double * lower = model2.rowLower();
     double * upper = model2.rowUpper();
     int iRow;
     // Simplest is to change all rhs to zero
     // One should skip E rows but this is simpler coding
     for (iRow = 0; iRow < numberRows; iRow++) {
          newUpper[iRow] = upper[iRow];
          upper[iRow] = 0.0;
          newLower[iRow] = lower[iRow];
          lower[iRow] = 0.0;
          addRow[iRow] = iRow;
          addElement[iRow] = -1.0;
          addStarts[iRow] = iRow;
     }
     addStarts[numberRows] = numberRows;
     model2.addColumns(numberRows, newLower, newUpper, NULL,
                       addStarts, addRow, addElement);
     delete [] addStarts;
     delete [] addRow;
     delete [] addElement;
     delete [] newLower;
     delete [] newUpper;

     // Modify costs
     double * randomArray = new double[numberRows];
     for (iRow = 0; iRow < numberRows; iRow++)
          randomArray[iRow] = CoinDrand48();

     model2.transposeTimes(1.0, randomArray, model2.objective());
     delete [] randomArray;
     // solve
     model2.primal();
     // first check okay if solution values back
     memcpy(model.primalColumnSolution(), model2.primalColumnSolution(),
            originalNumberColumns * sizeof(double));
     memcpy(model.primalRowSolution(), model2.primalRowSolution(),
            numberRows * sizeof(double));
     int iColumn;
     for (iColumn = 0; iColumn < originalNumberColumns; iColumn++)
          model.setColumnStatus(iColumn, model2.getColumnStatus(iColumn));

     for (iRow = 0; iRow < numberRows; iRow++) {
          if (model2.getRowStatus(iRow) == ClpSimplex::basic) {
               model.setRowStatus(iRow, ClpSimplex::basic);
          } else {
               model.setRowStatus(iRow, model2.getColumnStatus(iRow + originalNumberColumns));
          }
     }
     model.primal(0);
     // and now without solution values

     for (iColumn = 0; iColumn < originalNumberColumns; iColumn++)
          model3.setColumnStatus(iColumn, model2.getColumnStatus(iColumn));

     for (iRow = 0; iRow < numberRows; iRow++)
          model3.setRowStatus(iRow, model2.getColumnStatus(iRow + originalNumberColumns));
     model3.primal(0);
     return 0;
}
