/* $Id$ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinBuild.hpp"
int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;
     if (argc < 2) {
#if defined(SAMPLEDIR)
          status = model.readMps(SAMPLEDIR "/p0033.mps", true);
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
     } else
          status = model.readMps(argv[1]);
     if (status) {
          printf("errors on input\n");
          exit(77);
     }
     {
          // check if we need to change bounds to rows
          int numberColumns = model.numberColumns();
          const double * columnLower = model.columnLower();
          const double * columnUpper = model.columnUpper();
          int iColumn;
          CoinBuild build;
          double one = 1.0;
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (columnUpper[iColumn] < 1.0e20 &&
                         columnLower[iColumn] > -1.0e20) {
                    if (fabs(columnLower[iColumn]) < fabs(columnUpper[iColumn])) {
                         double value = columnUpper[iColumn];
                         model.setColumnUpper(iColumn, COIN_DBL_MAX);
                         build.addRow(1, &iColumn, &one, -COIN_DBL_MAX, value);
                    } else {
                         double value = columnLower[iColumn];
                         model.setColumnLower(iColumn, -COIN_DBL_MAX);
                         build.addRow(1, &iColumn, &one, value, COIN_DBL_MAX);
                    }
               }
          }
          if (build.numberRows())
               model.addRows(build);
     }
     int numberColumns = model.numberColumns();
     const double * columnLower = model.columnLower();
     const double * columnUpper = model.columnUpper();
     int numberRows = model.numberRows();
     double * rowLower = CoinCopyOfArray(model.rowLower(), numberRows);
     double * rowUpper = CoinCopyOfArray(model.rowUpper(), numberRows);

     const double * objective = model.objective();
     CoinPackedMatrix * matrix = model.matrix();
     // get transpose
     CoinPackedMatrix rowCopy = *matrix;
     int iRow, iColumn;
     int numberExtraRows = 0;
     for (iRow = 0; iRow < numberRows; iRow++) {
          if (rowLower[iRow] <= -1.0e20) {
          } else if (rowUpper[iRow] >= 1.0e20) {
          } else {
               if (rowUpper[iRow] != rowLower[iRow])
                    numberExtraRows++;
          }
     }
     const int * row = matrix->getIndices();
     const int * columnLength = matrix->getVectorLengths();
     const CoinBigIndex * columnStart = matrix->getVectorStarts();
     const double * elementByColumn = matrix->getElements();
     double objOffset = 0.0;
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          double offset = 0.0;
          if (columnUpper[iColumn] >= 1.0e20) {
               if (columnLower[iColumn] > -1.0e20)
                    offset = columnLower[iColumn];
          } else if (columnLower[iColumn] <= -1.0e20) {
               offset = columnUpper[iColumn];
          } else {
               // taken care of before
               abort();
          }
          if (offset) {
               objOffset += offset * objective[iColumn];
               for (CoinBigIndex j = columnStart[iColumn];
                         j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                    int iRow = row[j];
                    if (rowLower[iRow] > -1.0e20)
                         rowLower[iRow] -= offset * elementByColumn[j];
                    if (rowUpper[iRow] < 1.0e20)
                         rowUpper[iRow] -= offset * elementByColumn[j];
               }
          }
     }
     int * which = new int[numberRows+numberExtraRows];
     rowCopy.reverseOrdering();
     rowCopy.transpose();
     double * fromRowsLower = new double[numberRows+numberExtraRows];
     double * fromRowsUpper = new double[numberRows+numberExtraRows];
     double * newObjective = new double[numberRows+numberExtraRows];
     double * fromColumnsLower = new double[numberColumns];
     double * fromColumnsUpper = new double[numberColumns];
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          // Offset is already in
          if (columnUpper[iColumn] >= 1.0e20) {
               if (columnLower[iColumn] > -1.0e20) {
                    fromColumnsLower[iColumn] = -COIN_DBL_MAX;
                    fromColumnsUpper[iColumn] = objective[iColumn];
               } else {
                    // free
                    fromColumnsLower[iColumn] = objective[iColumn];
                    fromColumnsUpper[iColumn] = objective[iColumn];
               }
          } else if (columnLower[iColumn] <= -1.0e20) {
               fromColumnsLower[iColumn] = objective[iColumn];
               fromColumnsUpper[iColumn] = COIN_DBL_MAX;
          } else {
               abort();
          }
     }
     int kRow = 0;
     for (iRow = 0; iRow < numberRows; iRow++) {
          if (rowLower[iRow] <= -1.0e20) {
               assert(rowUpper[iRow] < 1.0e20);
               newObjective[kRow] = -rowUpper[iRow];
               fromRowsLower[kRow] = -COIN_DBL_MAX;
               fromRowsUpper[kRow] = 0.0;
               which[kRow] = iRow;
               kRow++;
          } else if (rowUpper[iRow] >= 1.0e20) {
               newObjective[kRow] = -rowLower[iRow];
               fromRowsLower[kRow] = 0.0;
               fromRowsUpper[kRow] = COIN_DBL_MAX;
               which[kRow] = iRow;
               kRow++;
          } else {
               if (rowUpper[iRow] == rowLower[iRow]) {
                    newObjective[kRow] = -rowLower[iRow];
                    fromRowsLower[kRow] = -COIN_DBL_MAX;;
                    fromRowsUpper[kRow] = COIN_DBL_MAX;
                    which[kRow] = iRow;
                    kRow++;
               } else {
                    // range
                    newObjective[kRow] = -rowUpper[iRow];
                    fromRowsLower[kRow] = -COIN_DBL_MAX;
                    fromRowsUpper[kRow] = 0.0;
                    which[kRow] = iRow;
                    kRow++;
                    newObjective[kRow] = -rowLower[iRow];
                    fromRowsLower[kRow] = 0.0;
                    fromRowsUpper[kRow] = COIN_DBL_MAX;
                    which[kRow] = iRow;
                    kRow++;
               }
          }
     }
     if (numberExtraRows) {
          CoinPackedMatrix newCopy;
          newCopy.submatrixOfWithDuplicates(rowCopy, kRow, which);
          rowCopy = newCopy;
     }
     ClpSimplex modelDual;
     modelDual.loadProblem(rowCopy, fromRowsLower, fromRowsUpper, newObjective,
                           fromColumnsLower, fromColumnsUpper);
     modelDual.setObjectiveOffset(objOffset);
     delete [] fromRowsLower;
     delete [] fromRowsUpper;
     delete [] fromColumnsLower;
     delete [] fromColumnsUpper;
     delete [] newObjective;
     delete [] which;
     delete [] rowLower;
     delete [] rowUpper;
     modelDual.writeMps("dual.mps");
     return 0;
}
