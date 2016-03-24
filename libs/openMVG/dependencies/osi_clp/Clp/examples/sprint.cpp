/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
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
       This driver implements what I called Sprint.  Cplex calls it
       "sifting" which is just as silly.  When I thought of this trivial idea
       it reminded me of an LP code of the 60's called sprint which after
       every factorization took a subset of the matrix into memory (all
       64K words!) and then iterated very fast on that subset.  On the
       problems of those days it did not work very well, but it worked very
       well on aircrew scheduling problems where there were very large numbers
       of columns all with the same flavor.
     */

     /* The idea works best if you can get feasible easily.  To make it
        more general we can add in costed slacks */

     int originalNumberColumns = model.numberColumns();
     int numberRows = model.numberRows();

     // We will need arrays to choose variables.  These are too big but ..
     double * weight = new double [numberRows+originalNumberColumns];
     int * sort = new int [numberRows+originalNumberColumns];
     int numberSort = 0;
     // Say we are going to add slacks - if you can get a feasible
     // solution then do that at the comment - Add in your own coding here
     bool addSlacks = true;

     if (addSlacks) {
          // initial list will just be artificials
          // first we will set all variables as close to zero as possible
          int iColumn;
          const double * columnLower = model.columnLower();
          const double * columnUpper = model.columnUpper();
          double * columnSolution = model.primalColumnSolution();

          for (iColumn = 0; iColumn < originalNumberColumns; iColumn++) {
               double value = 0.0;
               if (columnLower[iColumn] > 0.0)
                    value = columnLower[iColumn];
               else if (columnUpper[iColumn] < 0.0)
                    value = columnUpper[iColumn];
               columnSolution[iColumn] = value;
          }
          // now see what that does to row solution
          double * rowSolution = model.primalRowSolution();
          memset(rowSolution, 0, numberRows * sizeof(double));
          model.times(1.0, columnSolution, rowSolution);

          int * addStarts = new int [numberRows+1];
          int * addRow = new int[numberRows];
          double * addElement = new double[numberRows];
          const double * lower = model.rowLower();
          const double * upper = model.rowUpper();
          addStarts[0] = 0;
          int numberArtificials = 0;
          double * addCost = new double [numberRows];
          const double penalty = 1.0e8;
          int iRow;
          for (iRow = 0; iRow < numberRows; iRow++) {
               if (lower[iRow] > rowSolution[iRow]) {
                    addRow[numberArtificials] = iRow;
                    addElement[numberArtificials] = 1.0;
                    addCost[numberArtificials] = penalty;
                    numberArtificials++;
                    addStarts[numberArtificials] = numberArtificials;
               } else if (upper[iRow] < rowSolution[iRow]) {
                    addRow[numberArtificials] = iRow;
                    addElement[numberArtificials] = -1.0;
                    addCost[numberArtificials] = penalty;
                    numberArtificials++;
                    addStarts[numberArtificials] = numberArtificials;
               }
          }
          model.addColumns(numberArtificials, NULL, NULL, addCost,
                           addStarts, addRow, addElement);
          delete [] addStarts;
          delete [] addRow;
          delete [] addElement;
          delete [] addCost;
          // Set up initial list
          numberSort = numberArtificials;
          int i;
          for (i = 0; i < numberSort; i++)
               sort[i] = i + originalNumberColumns;
     } else {
          // Get initial list in some magical way
          // Add in your own coding here
          abort();
     }

     int numberColumns = model.numberColumns();
     const double * columnLower = model.columnLower();
     const double * columnUpper = model.columnUpper();
     double * fullSolution = model.primalColumnSolution();

     // Just do this number of passes
     int maxPass = 100;
     int iPass;
     double lastObjective = 1.0e31;

     // Just take this number of columns in small problem
     int smallNumberColumns = CoinMin(3 * numberRows, numberColumns);
     smallNumberColumns = CoinMax(smallNumberColumns, 3000);
     // To stop seg faults on unsuitable problems
     smallNumberColumns = CoinMin(smallNumberColumns,numberColumns);
     // We will be using all rows
     int * whichRows = new int [numberRows];
     for (int iRow = 0; iRow < numberRows; iRow++)
          whichRows[iRow] = iRow;
     double originalOffset;
     model.getDblParam(ClpObjOffset, originalOffset);

     for (iPass = 0; iPass < maxPass; iPass++) {
          printf("Start of pass %d\n", iPass);
          //printf("Bug until submodel new version\n");
          CoinSort_2(sort, sort + numberSort, weight);
          // Create small problem
          ClpSimplex small(&model, numberRows, whichRows, numberSort, sort);
          // now see what variables left out do to row solution
          double * rowSolution = model.primalRowSolution();
          memset(rowSolution, 0, numberRows * sizeof(double));
          int iRow, iColumn;
          // zero out ones in small problem
          for (iColumn = 0; iColumn < numberSort; iColumn++) {
               int kColumn = sort[iColumn];
               fullSolution[kColumn] = 0.0;
          }
          // Get objective offset
          double offset = 0.0;
          const double * objective = model.objective();
          for (iColumn = 0; iColumn < originalNumberColumns; iColumn++)
               offset += fullSolution[iColumn] * objective[iColumn];
          small.setDblParam(ClpObjOffset, originalOffset - offset);
          model.times(1.0, fullSolution, rowSolution);

          double * lower = small.rowLower();
          double * upper = small.rowUpper();
          for (iRow = 0; iRow < numberRows; iRow++) {
               if (lower[iRow] > -1.0e50)
                    lower[iRow] -= rowSolution[iRow];
               if (upper[iRow] < 1.0e50)
                    upper[iRow] -= rowSolution[iRow];
          }
          /* For some problems a useful variant is to presolve problem.
             In this case you need to adjust smallNumberColumns to get
             right size problem.  Also you can dispense with creating
             small problem and fix variables in large problem and do presolve
             on that. */
          // Solve
          small.primal();
          // move solution back
          const double * solution = small.primalColumnSolution();
          for (iColumn = 0; iColumn < numberSort; iColumn++) {
               int kColumn = sort[iColumn];
               model.setColumnStatus(kColumn, small.getColumnStatus(iColumn));
               fullSolution[kColumn] = solution[iColumn];
          }
          for (iRow = 0; iRow < numberRows; iRow++)
               model.setRowStatus(iRow, small.getRowStatus(iRow));
          memcpy(model.primalRowSolution(), small.primalRowSolution(),
                 numberRows * sizeof(double));
          if ((small.objectiveValue() > lastObjective - 1.0e-7 && iPass > 5) ||
                    !small.numberIterations() ||
                    iPass == maxPass - 1) {

               break; // finished
          } else {
               lastObjective = small.objectiveValue();
               // get reduced cost for large problem
               // this assumes minimization
               memcpy(weight, model.objective(), numberColumns * sizeof(double));
               model.transposeTimes(-1.0, small.dualRowSolution(), weight);
               // now massage weight so all basic in plus good djs
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    double dj = weight[iColumn];
                    double value = fullSolution[iColumn];
                    if (model.getColumnStatus(iColumn) == ClpSimplex::basic)
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
               numberSort = smallNumberColumns;
          }
     }
     if (addSlacks) {
          int i;
          int numberArtificials = numberColumns - originalNumberColumns;
          for (i = 0; i < numberArtificials; i++)
               sort[i] = i + originalNumberColumns;
          model.deleteColumns(numberArtificials, sort);
     }
     delete [] weight;
     delete [] sort;
     delete [] whichRows;
     model.primal(1);
     return 0;
}
