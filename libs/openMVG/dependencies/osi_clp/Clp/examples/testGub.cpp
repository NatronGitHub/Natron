/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "ClpDynamicExampleMatrix.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include "CoinSort.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "CoinMpsIO.hpp"
int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;
     int maxFactor = 100;
     if (argc < 2) {
#if defined(NETLIBDIR)
          status = model.readMps(NETLIBDIR "/czprob.mps");
          if (status) {
               printf("Unable to read matrix - trying gzipped version\n");
               status = model.readMps(NETLIBDIR "/czprob.mps.gz");
          }
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
     } else {
          status = model.readMps(argv[1]);
     }
     if (status) {
          printf("errors on input\n");
          exit(77);
     }
     if (argc > 2) {
          maxFactor = atoi(argv[2]);
          printf("max factor %d\n", maxFactor);
     }
     if (argc > 3) {
          printf("Using ClpDynamicMatrix\n");
     } else {
          printf("Using ClpDynamicExampleMatrix\n");
     }
     // find gub
     int numberRows = model.numberRows();
     int * gubStart = new int[numberRows+1];
     int * gubEnd = new int[numberRows];
     int * which = new int[numberRows];
     int * whichGub = new int[numberRows];
     int numberColumns = model.numberColumns();
     int * mark = new int[numberColumns];
     int iRow, iColumn;
     // delete variables fixed to zero
     const double * columnLower = model.columnLower();
     const double * columnUpper = model.columnUpper();
     int numberDelete = 0;
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          if (columnUpper[iColumn] == 0.0 && columnLower[iColumn] == 0.0)
               mark[numberDelete++] = iColumn;
     }
     if (numberDelete) {
          model.deleteColumns(numberDelete, mark);
          numberColumns -= numberDelete;
          columnLower = model.columnLower();
          columnUpper = model.columnUpper();
     }
     double * lower = new double[numberRows];
     double * upper = new double[numberRows];
     const double * rowLower = model.rowLower();
     const double * rowUpper = model.rowUpper();
     for (iColumn = 0; iColumn < numberColumns; iColumn++)
          mark[iColumn] = -1;
     CoinPackedMatrix * matrix = model.matrix();
     // get row copy
     CoinPackedMatrix rowCopy = *matrix;
     rowCopy.reverseOrdering();
     const int * column = rowCopy.getIndices();
     const int * rowLength = rowCopy.getVectorLengths();
     const CoinBigIndex * rowStart = rowCopy.getVectorStarts();
     const double * element = rowCopy.getElements();
     int putGub = numberRows;
     int putNonGub = numberRows;
     int * rowIsGub = new int [numberRows];
     for (iRow = numberRows - 1; iRow >= 0; iRow--) {
          bool gubRow = true;
          int first = numberColumns + 1;
          int last = -1;
          for (int j = rowStart[iRow]; j < rowStart[iRow] + rowLength[iRow]; j++) {
               if (element[j] != 1.0) {
                    gubRow = false;
                    break;
               } else {
                    int iColumn = column[j];
                    if (mark[iColumn] >= 0) {
                         gubRow = false;
                         break;
                    } else {
                         last = CoinMax(last, iColumn);
                         first = CoinMin(first, iColumn);
                    }
               }
          }
          if (last - first + 1 != rowLength[iRow] || !gubRow) {
               which[--putNonGub] = iRow;
               rowIsGub[iRow] = 0;
          } else {
               for (int j = rowStart[iRow]; j < rowStart[iRow] + rowLength[iRow]; j++) {
                    int iColumn = column[j];
                    mark[iColumn] = iRow;
               }
               rowIsGub[iRow] = -1;
               putGub--;
               gubStart[putGub] = first;
               gubEnd[putGub] = last + 1;
               lower[putGub] = rowLower[iRow];
               upper[putGub] = rowUpper[iRow];
               whichGub[putGub] = iRow;
          }
     }
     int numberNonGub = numberRows - putNonGub;
     int numberGub = numberRows - putGub;
     if (numberGub > 0) {
          printf("** %d gub rows\n", numberGub);
          int numberNormal = 0;
          const int * row = matrix->getIndices();
          const int * columnLength = matrix->getVectorLengths();
          const CoinBigIndex * columnStart = matrix->getVectorStarts();
          const double * elementByColumn = matrix->getElements();
          int numberElements = 0;
          bool doLower = false;
          bool doUpper = false;
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (mark[iColumn] < 0) {
                    mark[numberNormal++] = iColumn;
               } else {
                    numberElements += columnLength[iColumn];
                    if (columnLower[iColumn] != 0.0)
                         doLower = true;
                    if (columnUpper[iColumn] < 1.0e20)
                         doUpper = true;
               }
          }
          if (!numberNormal) {
               printf("Putting back one gub row to make non-empty\n");
               for (iColumn = gubStart[putGub]; iColumn < gubEnd[putGub]; iColumn++)
                    mark[numberNormal++] = iColumn;
               putGub++;
               numberGub--;
          }
          ClpSimplex model2(&model, numberNonGub, which + putNonGub, numberNormal, mark);
          // and copy for restart test
          ClpSimplex model3 = model2;
          int numberGubColumns = numberColumns - numberNormal;
          // sort gubs so monotonic
          int * which = new int[numberGub];
          int i;
          for (i = 0; i < numberGub; i++)
               which[i] = i;
          CoinSort_2(gubStart + putGub, gubStart + putGub + numberGub, which);
          // move to bottom if we want to use later
          memmove(gubStart, gubStart + putGub, numberGub * sizeof(int));
          int * temp1 = new int [numberGub];
          for (i = 0; i < numberGub; i++) {
               int k = which[i];
               temp1[i] = gubEnd[putGub+k];
          }
          memcpy(gubEnd, temp1, numberGub * sizeof(int));
          delete [] temp1;
          double * temp2 = new double [numberGub];
          for (i = 0; i < numberGub; i++) {
               int k = which[i];
               temp2[i] = lower[putGub+k];
          }
          memcpy(lower, temp2, numberGub * sizeof(double));
          for (i = 0; i < numberGub; i++) {
               int k = which[i];
               temp2[i] = upper[putGub+k];
          }
          memcpy(upper, temp2, numberGub * sizeof(double));
          delete [] temp2;
          delete [] which;
          numberElements -= numberGubColumns;
          int * start2 = new int[numberGubColumns+1];
          int * row2 = new int[numberElements];
          double * element2 = new double[numberElements];
          double * cost2 = new double [numberGubColumns];
          double * lowerColumn2 = NULL;
          if (doLower) {
               lowerColumn2 = new double [numberGubColumns];
               CoinFillN(lowerColumn2, numberGubColumns, 0.0);
          }
          double * upperColumn2 = NULL;
          if (doUpper) {
               upperColumn2 = new double [numberGubColumns];
               CoinFillN(upperColumn2, numberGubColumns, COIN_DBL_MAX);
          }
          numberElements = 0;
          int numberNonGubRows = 0;
          for (iRow = 0; iRow < numberRows; iRow++) {
               if (!rowIsGub[iRow])
                    rowIsGub[iRow] = numberNonGubRows++;
          }
          numberColumns = 0;
          int iStart = gubStart[0];
          gubStart[0] = 0;
          start2[0] = 0;
          const double * cost = model.objective();
          for (int iSet = 0; iSet < numberGub; iSet++) {
               int iEnd = gubEnd[iSet];
               for (int k = iStart; k < iEnd; k++) {
                    cost2[numberColumns] = cost[k];
                    if (columnLower[k])
                         lowerColumn2[numberColumns] = columnLower[k];
                    if (columnUpper[k] < 1.0e20)
                         upperColumn2[numberColumns] = columnUpper[k];
                    for (int j = columnStart[k]; j < columnStart[k] + columnLength[k]; j++) {
                         int iRow = rowIsGub[row[j]];
                         if (iRow >= 0) {
                              row2[numberElements] = iRow;
                              element2[numberElements++] = elementByColumn[j];
                         }
                    }
                    start2[++numberColumns] = numberElements;
               }
               if (iSet < numberGub - 1)
                    iStart = gubStart[iSet+1];
               gubStart[iSet+1] = numberColumns;
          }
          printf("** Before adding matrix there are %d rows and %d columns\n",
                 model2.numberRows(), model2.numberColumns());
          if (argc > 3) {
               ClpDynamicMatrix * newMatrix = new ClpDynamicMatrix(&model2, numberGub,
                         numberColumns, gubStart,
                         lower, upper,
                         start2, row2, element2, cost2,
                         lowerColumn2, upperColumn2);
               model2.replaceMatrix(newMatrix);
               newMatrix->switchOffCheck();
               newMatrix->setRefreshFrequency(1000);
          } else {
               ClpDynamicExampleMatrix * newMatrix = new ClpDynamicExampleMatrix(&model2, numberGub,
                         numberColumns, gubStart,
                         lower, upper,
                         start2, row2, element2, cost2,
                         lowerColumn2, upperColumn2);
               model2.replaceMatrix(newMatrix,true);
               newMatrix->switchOffCheck();
               newMatrix->setRefreshFrequency(1000);
          }
          printf("** While after adding matrix there are %d rows and %d columns\n",
                 model2.numberRows(), model2.numberColumns());
          model2.setSpecialOptions(4);    // exactly to bound
          // For now scaling off
          model2.scaling(0);
          ClpPrimalColumnSteepest steepest(5);
          model2.setPrimalColumnPivotAlgorithm(steepest);
          //model2.messageHandler()->setLogLevel(63);
          model2.setFactorizationFrequency(maxFactor);
          model2.setMaximumIterations(4000000);
          double time1 = CoinCpuTime();
          model2.primal();
          // can't use values pass
          model2.primal(0);
          // test proper restart
          if (argc > 3) {
               ClpDynamicMatrix * oldMatrix =
                    dynamic_cast< ClpDynamicMatrix*>(model2.clpMatrix());
               assert(oldMatrix);
               ClpDynamicMatrix * newMatrix = new
               ClpDynamicMatrix(&model3, numberGub,
                                numberColumns, gubStart,
                                lower, upper,
                                start2, row2, element2, cost2,
                                lowerColumn2, upperColumn2,
                                oldMatrix->gubRowStatus(), oldMatrix->dynamicStatus());
               model3.replaceMatrix(newMatrix,true);
               // and ordinary status (but only NON gub rows)
               memcpy(model3.statusArray(), model2.statusArray(),
                      (newMatrix->numberStaticRows() + model3.numberColumns()) *sizeof(unsigned char));
               newMatrix->switchOffCheck();
               newMatrix->setRefreshFrequency(1000);
          } else {
               ClpDynamicExampleMatrix * oldMatrix =
                    dynamic_cast< ClpDynamicExampleMatrix*>(model2.clpMatrix());
               assert(oldMatrix);
               ClpDynamicExampleMatrix * newMatrix = new
               ClpDynamicExampleMatrix(&model3, numberGub,
                                       numberColumns, gubStart,
                                       lower, upper,
                                       start2, row2, element2, cost2,
                                       lowerColumn2, upperColumn2,
                                       oldMatrix->gubRowStatus(), oldMatrix->dynamicStatus(),
                                       oldMatrix->numberGubColumns(), oldMatrix->idGen());
               model3.replaceMatrix(newMatrix,true);
               // and ordinary status (but only NON gub rows)
               memcpy(model3.statusArray(), model2.statusArray(),
                      (newMatrix->numberStaticRows() + model3.numberColumns()) *sizeof(unsigned char));
               newMatrix->switchOffCheck();
               newMatrix->setRefreshFrequency(1000);
          }
          model3.setSpecialOptions(4);    // exactly to bound
          // For now scaling off
          model3.scaling(0);
          model3.setPrimalColumnPivotAlgorithm(steepest);
          model3.messageHandler()->setLogLevel(63);
          model3.setFactorizationFrequency(maxFactor);
          model3.setMaximumIterations(4000000);
          delete [] rowIsGub;
          delete [] start2;
          delete [] row2;
          delete [] element2;
          delete [] cost2;
          delete [] lowerColumn2;
          delete [] upperColumn2;
          model3.primal();
          // this code expects non gub first in original matrix
          // and only works at present for ClpDynamicMatrix
          ClpDynamicMatrix * gubMatrix =
               dynamic_cast< ClpDynamicMatrix*>(model2.clpMatrix());
          assert(gubMatrix);
          ClpDynamicExampleMatrix * gubMatrix2 =
               dynamic_cast< ClpDynamicExampleMatrix*>(model2.clpMatrix());
          if (!gubMatrix2) {
               const double * solution = model2.primalColumnSolution();
               const double * cost = model.objective();
               int numberGubColumns = gubMatrix->numberGubColumns();
               int firstOdd = gubMatrix->firstDynamic();
               int lastOdd = gubMatrix->firstAvailable();
               int numberTotalColumns = firstOdd + numberGubColumns;
               int originalNumberRows = model.numberRows();
               int numberStaticRows = gubMatrix->numberStaticRows();
               char * status = new char [numberTotalColumns];
               double * gubSolution = new double [numberTotalColumns];
               int numberSets = gubMatrix->numberSets();
               const int * id = gubMatrix->id();
               int i;
               const double * columnLower = gubMatrix->columnLower();
               const double * columnUpper = gubMatrix->columnUpper();
               for (i = 0; i < numberGubColumns; i++) {
                    if (gubMatrix->getDynamicStatus(i) == ClpDynamicMatrix::atUpperBound) {
                         gubSolution[i+firstOdd] = columnUpper[i];
                         status[i+firstOdd] = 2;
                    } else if (gubMatrix->getDynamicStatus(i) == ClpDynamicMatrix::atLowerBound && columnLower) {
                         gubSolution[i+firstOdd] = columnLower[i];
                         status[i+firstOdd] = 1;
                    } else if (gubMatrix->getDynamicStatus(i) == ClpDynamicMatrix::soloKey) {
                         int iSet = gubMatrix->whichSet(i);
                         gubSolution[i+firstOdd] = gubMatrix->keyValue(iSet);
                         status[i+firstOdd] = 0;
                    } else {
                         gubSolution[i+firstOdd] = 0.0;
                         status[i+firstOdd] = 1;
                    }
               }
               for (i = 0; i < firstOdd; i++) {
                    ClpSimplex::Status thisStatus = model2.getStatus(i);
                    if (thisStatus == ClpSimplex::basic)
                         status[i] = 0;
                    else if (thisStatus == ClpSimplex::atLowerBound)
                         status[i] = 1;
                    else if (thisStatus == ClpSimplex::atUpperBound)
                         status[i] = 2;
                    else if (thisStatus == ClpSimplex::isFixed)
                         status[i] = 3;
                    else
                         abort();
                    gubSolution[i] = solution[i];
               }
               for (i = firstOdd; i < lastOdd; i++) {
                    int iBig = id[i-firstOdd] + firstOdd;
                    ClpSimplex::Status thisStatus = model2.getStatus(i);
                    if (thisStatus == ClpSimplex::basic)
                         status[iBig] = 0;
                    else if (thisStatus == ClpSimplex::atLowerBound)
                         status[iBig] = 1;
                    else if (thisStatus == ClpSimplex::atUpperBound)
                         status[iBig] = 2;
                    else if (thisStatus == ClpSimplex::isFixed)
                         status[iBig] = 3;
                    else
                         abort();
                    gubSolution[iBig] = solution[i];
               }
               char * rowStatus = new char[originalNumberRows];
               for (i = 0; i < numberStaticRows; i++) {
                    ClpSimplex::Status thisStatus = model2.getRowStatus(i);
                    if (thisStatus == ClpSimplex::basic)
                         rowStatus[i] = 0;
                    else if (thisStatus == ClpSimplex::atLowerBound)
                         rowStatus[i] = 1;
                    else if (thisStatus == ClpSimplex::atUpperBound)
                         rowStatus[i] = 2;
                    else if (thisStatus == ClpSimplex::isFixed)
                         rowStatus[i] = 3;
                    else
                         abort();
               }
               double objValue = 0.0;
               for (i = 0; i < numberTotalColumns; i++)
                    objValue += cost[i] * gubSolution[i];
               printf("objective value is %g\n", objValue);
               for (i = 0; i < numberSets; i++) {
                    ClpSimplex::Status thisStatus = gubMatrix->getStatus(i);
                    if (thisStatus == ClpSimplex::basic)
                         rowStatus[numberStaticRows+i] = 0;
                    else if (thisStatus == ClpSimplex::atLowerBound)
                         rowStatus[numberStaticRows+i] = 1;
                    else if (thisStatus == ClpSimplex::atUpperBound)
                         rowStatus[numberStaticRows+i] = 2;
                    else if (thisStatus == ClpSimplex::isFixed)
                         rowStatus[numberStaticRows+i] = 3;
                    else
                         abort();
               }
               // Coding below may not work if gub rows not at end
               FILE * fp = fopen("xx.sol", "w");
               fwrite(gubSolution, sizeof(double), numberTotalColumns, fp);
               fwrite(status, sizeof(char), numberTotalColumns, fp);
#ifndef NDEBUG
               const double * rowsol = model2.primalRowSolution();
#endif
               double * rowsol2 = new double[originalNumberRows];
               memset(rowsol2, 0, originalNumberRows * sizeof(double));
               model.times(1.0, gubSolution, rowsol2);
               for (i = 0; i < numberStaticRows; i++)
                    assert(fabs(rowsol[i] - rowsol2[i]) < 1.0e-3);
               for (; i < originalNumberRows; i++)
                    assert(rowsol2[i] > lower[i-numberStaticRows] - 1.0e-3 &&
                           rowsol2[i] < upper[i-numberStaticRows] + 1.0e-3);
               //for (;i<originalNumberRows;i++)
               //printf("%d %g\n",i,rowsol2[i]);
               fwrite(rowsol2, sizeof(double), originalNumberRows, fp);
               delete [] rowsol2;
               fwrite(rowStatus, sizeof(char), originalNumberRows, fp);
               fclose(fp);
               delete [] status;
               delete [] gubSolution;
               // ** if going to rstart as dynamic need id_
               // also copy coding in useEf.. from ClpGubMatrix (i.e. test for basis)
          }
          printf("obj offset is %g\n", model2.objectiveOffset());
          printf("Primal took %g seconds\n", CoinCpuTime() - time1);
     }
     delete [] mark;
     delete [] gubStart;
     delete [] gubEnd;
     delete [] which;
     delete [] whichGub;
     delete [] lower;
     delete [] upper;
     return 0;
}
