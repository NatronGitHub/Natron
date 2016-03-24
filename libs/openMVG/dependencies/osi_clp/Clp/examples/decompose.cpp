/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "CoinMpsIO.hpp"
#include <iomanip>

int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;
     // Keep names
     if (argc < 2) {
#if defined(NETLIBDIR)
          status = model.readMps(NETLIBDIR "/czprob.mps", true);
#else
          fprintf(stderr, "Do not know where to find netlib MPS files.\n");
          return 1;
#endif
     } else {
          status = model.readMps(argv[1], true);
     }
     if (status)
          exit(10);
     /*
       This driver does a simple Dantzig Wolfe decomposition
     */
     // Get master rows in some mysterious way
     int numberRows = model.numberRows();
     int * rowBlock = new int[numberRows];
     int iRow;
     for (iRow = 0; iRow < numberRows; iRow++)
          rowBlock[iRow] = -2;
     // these are master rows
     if (numberRows == 105127) {
          // ken-18
          for (iRow = 104976; iRow < numberRows; iRow++)
               rowBlock[iRow] = -1;
     } else if (numberRows == 2426) {
          // ken-7
          for (iRow = 2401; iRow < numberRows; iRow++)
               rowBlock[iRow] = -1;
     } else if (numberRows == 810) {
          for (iRow = 81; iRow < 84; iRow++)
               rowBlock[iRow] = -1;
     } else if (numberRows == 5418) {
          for (iRow = 564; iRow < 603; iRow++)
               rowBlock[iRow] = -1;
     } else if (numberRows == 10280) {
          // osa-60
          for (iRow = 10198; iRow < 10280; iRow++)
               rowBlock[iRow] = -1;
     } else if (numberRows == 1503) {
          // degen3
          for (iRow = 0; iRow < 561; iRow++)
               rowBlock[iRow] = -1;
     } else if (numberRows == 929) {
          // czprob
          for (iRow = 0; iRow < 39; iRow++)
               rowBlock[iRow] = -1;
     }
     CoinPackedMatrix * matrix = model.matrix();
     // get row copy
     CoinPackedMatrix rowCopy = *matrix;
     rowCopy.reverseOrdering();
     const int * row = matrix->getIndices();
     const int * columnLength = matrix->getVectorLengths();
     const CoinBigIndex * columnStart = matrix->getVectorStarts();
     //const double * elementByColumn = matrix->getElements();
     const int * column = rowCopy.getIndices();
     const int * rowLength = rowCopy.getVectorLengths();
     const CoinBigIndex * rowStart = rowCopy.getVectorStarts();
     //const double * elementByRow = rowCopy.getElements();
     int numberBlocks = 0;
     int * stack = new int [numberRows];
     // to say if column looked at
     int numberColumns = model.numberColumns();
     int * columnBlock = new int[numberColumns];
     int iColumn;
     for (iColumn = 0; iColumn < numberColumns; iColumn++)
          columnBlock[iColumn] = -2;
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          int kstart = columnStart[iColumn];
          int kend = columnStart[iColumn] + columnLength[iColumn];
          if (columnBlock[iColumn] == -2) {
               // column not allocated
               int j;
               int nstack = 0;
               for (j = kstart; j < kend; j++) {
                    int iRow = row[j];
                    if (rowBlock[iRow] != -1) {
                         assert(rowBlock[iRow] == -2);
                         rowBlock[iRow] = numberBlocks; // mark
                         stack[nstack++] = iRow;
                    }
               }
               if (nstack) {
                    // new block - put all connected in
                    numberBlocks++;
                    columnBlock[iColumn] = numberBlocks - 1;
                    while (nstack) {
                         int iRow = stack[--nstack];
                         int k;
                         for (k = rowStart[iRow]; k < rowStart[iRow] + rowLength[iRow]; k++) {
                              int iColumn = column[k];
                              int kkstart = columnStart[iColumn];
                              int kkend = kkstart + columnLength[iColumn];
                              if (columnBlock[iColumn] == -2) {
                                   columnBlock[iColumn] = numberBlocks - 1; // mark
                                   // column not allocated
                                   int jj;
                                   for (jj = kkstart; jj < kkend; jj++) {
                                        int jRow = row[jj];
                                        if (rowBlock[jRow] == -2) {
                                             rowBlock[jRow] = numberBlocks - 1;
                                             stack[nstack++] = jRow;
                                        }
                                   }
                              } else {
                                   assert(columnBlock[iColumn] == numberBlocks - 1);
                              }
                         }
                    }
               } else {
                    // Only in master
                    columnBlock[iColumn] = -1;
               }
          }
     }
     printf("%d blocks found\n", numberBlocks);
     if (numberBlocks > 50) {
          int iBlock;
          for (iRow = 0; iRow < numberRows; iRow++) {
               iBlock = rowBlock[iRow];
               if (iBlock >= 0)
                    rowBlock[iRow] = iBlock % 50;
          }
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               iBlock = columnBlock[iColumn];
               if (iBlock >= 0)
                    columnBlock[iColumn] = iBlock % 50;
          }
          numberBlocks = 50;
     }
     delete [] stack;
     // make up problems
     CoinPackedMatrix * top = new CoinPackedMatrix [numberBlocks];
     ClpSimplex * sub = new ClpSimplex [numberBlocks];
     ClpSimplex master;
     // Create all sub problems
     // Could do much faster - but do that later
     int * whichRow = new int [numberRows];
     int * whichColumn = new int [numberColumns];
     // get top matrix
     CoinPackedMatrix topMatrix = *model.matrix();
     int numberRow2, numberColumn2;
     numberRow2 = 0;
     for (iRow = 0; iRow < numberRows; iRow++)
          if (rowBlock[iRow] >= 0)
               whichRow[numberRow2++] = iRow;
     topMatrix.deleteRows(numberRow2, whichRow);
     int iBlock;
     for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
          numberRow2 = 0;
          numberColumn2 = 0;
          for (iRow = 0; iRow < numberRows; iRow++)
               if (iBlock == rowBlock[iRow])
                    whichRow[numberRow2++] = iRow;
          for (iColumn = 0; iColumn < numberColumns; iColumn++)
               if (iBlock == columnBlock[iColumn])
                    whichColumn[numberColumn2++] = iColumn;
          sub[iBlock] = ClpSimplex(&model, numberRow2, whichRow,
                                   numberColumn2, whichColumn);
#if 0
          // temp
          double * upper = sub[iBlock].columnUpper();
          for (iColumn = 0; iColumn < numberColumn2; iColumn++)
               upper[iColumn] = 100.0;
#endif
          // and top matrix
          CoinPackedMatrix matrix = topMatrix;
          // and delete bits
          numberColumn2 = 0;
          for (iColumn = 0; iColumn < numberColumns; iColumn++)
               if (iBlock != columnBlock[iColumn])
                    whichColumn[numberColumn2++] = iColumn;
          matrix.deleteCols(numberColumn2, whichColumn);
          top[iBlock] = matrix;
     }
     // and master
     numberRow2 = 0;
     numberColumn2 = 0;
     for (iRow = 0; iRow < numberRows; iRow++)
          if (rowBlock[iRow] < 0)
               whichRow[numberRow2++] = iRow;
     for (iColumn = 0; iColumn < numberColumns; iColumn++)
          if (columnBlock[iColumn] == -1)
               whichColumn[numberColumn2++] = iColumn;
     ClpModel masterModel(&model, numberRow2, whichRow,
                          numberColumn2, whichColumn);
     master = ClpSimplex(masterModel);
     delete [] whichRow;
     delete [] whichColumn;

     // Overkill in terms of space
     int numberMasterRows = master.numberRows();
     int * columnAdd = new int[numberBlocks+1];
     int * rowAdd = new int[numberBlocks*(numberMasterRows+1)];
     double * elementAdd = new double[numberBlocks*(numberMasterRows+1)];
     double * objective = new double[numberBlocks];
     int maxPass = 500;
     int iPass;
     double lastObjective = 1.0e31;
     // Create convexity rows for proposals
     int numberMasterColumns = master.numberColumns();
     master.resize(numberMasterRows + numberBlocks, numberMasterColumns);
     // Arrays to say which block and when created
     int maximumColumns = 2 * numberMasterRows + 10 * numberBlocks;
     int * whichBlock = new int[maximumColumns];
     int * when = new int[maximumColumns];
     int numberColumnsGenerated = numberBlocks;
     // fill in rhs and add in artificials
     {
          double * rowLower = master.rowLower();
          double * rowUpper = master.rowUpper();
          int iBlock;
          columnAdd[0] = 0;
          for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
               int iRow = iBlock + numberMasterRows;;
               rowLower[iRow] = 1.0;
               rowUpper[iRow] = 1.0;
               rowAdd[iBlock] = iRow;
               elementAdd[iBlock] = 1.0;
               objective[iBlock] = 1.0e9;
               columnAdd[iBlock+1] = iBlock + 1;
               when[iBlock] = -1;
               whichBlock[iBlock] = iBlock;
          }
          master.addColumns(numberBlocks, NULL, NULL, objective,
                            columnAdd, rowAdd, elementAdd);
     }
     // and resize matrix to double check clp will be happy
     //master.matrix()->setDimensions(numberMasterRows+numberBlocks,
     //			 numberMasterColumns+numberBlocks);

     for (iPass = 0; iPass < maxPass; iPass++) {
          printf("Start of pass %d\n", iPass);
          // Solve master - may be infeasible
          master.scaling(false);
          if (0) {
               master.writeMps("yy.mps");
          }

          master.primal();
          int problemStatus = master.status(); // do here as can change (delcols)
          if (master.numberIterations() == 0 && iPass)
               break; // finished
          if (master.objectiveValue() > lastObjective - 1.0e-7 && iPass > 555)
               break; // finished
          lastObjective = master.objectiveValue();
          // mark basic ones and delete if necessary
          int iColumn;
          numberColumnsGenerated = master.numberColumns() - numberMasterColumns;
          for (iColumn = 0; iColumn < numberColumnsGenerated; iColumn++) {
               if (master.getStatus(iColumn + numberMasterColumns) == ClpSimplex::basic)
                    when[iColumn] = iPass;
          }
          if (numberColumnsGenerated + numberBlocks > maximumColumns) {
               // delete
               int numberKeep = 0;
               int numberDelete = 0;
               int * whichDelete = new int[numberColumns];
               for (iColumn = 0; iColumn < numberColumnsGenerated; iColumn++) {
                    if (when[iColumn] > iPass - 7) {
                         // keep
                         when[numberKeep] = when[iColumn];
                         whichBlock[numberKeep++] = whichBlock[iColumn];
                    } else {
                         // delete
                         whichDelete[numberDelete++] = iColumn + numberMasterColumns;
                    }
               }
               numberColumnsGenerated -= numberDelete;
               master.deleteColumns(numberDelete, whichDelete);
               delete [] whichDelete;
          }
          const double * dual = NULL;
          bool deleteDual = false;
          if (problemStatus == 0) {
               dual = master.dualRowSolution();
          } else if (problemStatus == 1) {
               // could do composite objective
               dual = master.infeasibilityRay();
               deleteDual = true;
               printf("The sum of infeasibilities is %g\n",
                      master.sumPrimalInfeasibilities());
          } else if (!master.numberColumns()) {
               assert(!iPass);
               dual = master.dualRowSolution();
               memset(master.dualRowSolution(),
                      0, (numberMasterRows + numberBlocks) *sizeof(double));
          } else {
               abort();
          }
          // Create objective for sub problems and solve
          columnAdd[0] = 0;
          int numberProposals = 0;
          for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
               int numberColumns2 = sub[iBlock].numberColumns();
               double * saveObj = new double [numberColumns2];
               double * objective2 = sub[iBlock].objective();
               memcpy(saveObj, objective2, numberColumns2 * sizeof(double));
               // new objective
               top[iBlock].transposeTimes(dual, objective2);
               int i;
               if (problemStatus == 0) {
                    for (i = 0; i < numberColumns2; i++)
                         objective2[i] = saveObj[i] - objective2[i];
               } else {
                    for (i = 0; i < numberColumns2; i++)
                         objective2[i] = -objective2[i];
               }
               sub[iBlock].primal();
               memcpy(objective2, saveObj, numberColumns2 * sizeof(double));
               // get proposal
               if (sub[iBlock].numberIterations() || !iPass) {
                    double objValue = 0.0;
                    int start = columnAdd[numberProposals];
                    // proposal
                    if (sub[iBlock].isProvenOptimal()) {
                         const double * solution = sub[iBlock].primalColumnSolution();
                         top[iBlock].times(solution, elementAdd + start);
                         for (i = 0; i < numberColumns2; i++)
                              objValue += solution[i] * saveObj[i];
                         // See if good dj and pack down
                         int number = start;
                         double dj = objValue;
                         if (problemStatus)
                              dj = 0.0;
                         double smallest = 1.0e100;
                         double largest = 0.0;
                         for (i = 0; i < numberMasterRows; i++) {
                              double value = elementAdd[start+i];
                              if (fabs(value) > 1.0e-15) {
                                   dj -= dual[i] * value;
                                   smallest = CoinMin(smallest, fabs(value));
                                   largest = CoinMax(largest, fabs(value));
                                   rowAdd[number] = i;
                                   elementAdd[number++] = value;
                              }
                         }
                         // and convexity
                         dj -= dual[numberMasterRows+iBlock];
                         rowAdd[number] = numberMasterRows + iBlock;
                         elementAdd[number++] = 1.0;
                         // if elements large then scale?
                         //if (largest>1.0e8||smallest<1.0e-8)
                         printf("For subproblem %d smallest - %g, largest %g - dj %g\n",
                                iBlock, smallest, largest, dj);
                         if (dj < -1.0e-6 || !iPass) {
                              // take
                              objective[numberProposals] = objValue;
                              columnAdd[++numberProposals] = number;
                              when[numberColumnsGenerated] = iPass;
                              whichBlock[numberColumnsGenerated++] = iBlock;
                         }
                    } else if (sub[iBlock].isProvenDualInfeasible()) {
                         // use ray
                         const double * solution = sub[iBlock].unboundedRay();
                         top[iBlock].times(solution, elementAdd + start);
                         for (i = 0; i < numberColumns2; i++)
                              objValue += solution[i] * saveObj[i];
                         // See if good dj and pack down
                         int number = start;
                         double dj = objValue;
                         double smallest = 1.0e100;
                         double largest = 0.0;
                         for (i = 0; i < numberMasterRows; i++) {
                              double value = elementAdd[start+i];
                              if (fabs(value) > 1.0e-15) {
                                   dj -= dual[i] * value;
                                   smallest = CoinMin(smallest, fabs(value));
                                   largest = CoinMax(largest, fabs(value));
                                   rowAdd[number] = i;
                                   elementAdd[number++] = value;
                              }
                         }
                         // if elements large or small then scale?
                         //if (largest>1.0e8||smallest<1.0e-8)
                         printf("For subproblem ray %d smallest - %g, largest %g - dj %g\n",
                                iBlock, smallest, largest, dj);
                         if (dj < -1.0e-6) {
                              // take
                              objective[numberProposals] = objValue;
                              columnAdd[++numberProposals] = number;
                              when[numberColumnsGenerated] = iPass;
                              whichBlock[numberColumnsGenerated++] = iBlock;
                         }
                    } else {
                         abort();
                    }
               }
               delete [] saveObj;
          }
          if (deleteDual)
               delete [] dual;
          if (numberProposals)
               master.addColumns(numberProposals, NULL, NULL, objective,
                                 columnAdd, rowAdd, elementAdd);
     }
     // now put back a good solution
     double * lower = new double[numberMasterRows+numberBlocks];
     double * upper = new double[numberMasterRows+numberBlocks];
     numberColumnsGenerated  += numberMasterColumns;
     double * sol = new double[numberColumnsGenerated];
     const double * solution = master.primalColumnSolution();
     const double * masterLower = master.rowLower();
     const double * masterUpper = master.rowUpper();
     double * fullSolution = model.primalColumnSolution();
     const double * fullLower = model.columnLower();
     const double * fullUpper = model.columnUpper();
     const double * rowSolution = master.primalRowSolution();
     double * fullRowSolution = model.primalRowSolution();
     int kRow = 0;
     for (iRow = 0; iRow < numberRows; iRow++) {
          if (rowBlock[iRow] == -1) {
               model.setRowStatus(iRow, master.getRowStatus(kRow));
               fullRowSolution[iRow] = rowSolution[kRow++];
          }
     }
     int kColumn = 0;
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          if (columnBlock[iColumn] == -1) {
               model.setStatus(iColumn, master.getStatus(kColumn));
               fullSolution[iColumn] = solution[kColumn++];
          }
     }
     for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
          // convert top bit to by rows
          top[iBlock].reverseOrdering();
          // zero solution
          memset(sol, 0, numberColumnsGenerated * sizeof(double));
          int i;
          for (i = numberMasterColumns; i < numberColumnsGenerated; i++)
               if (whichBlock[i-numberMasterColumns] == iBlock)
                    sol[i] = solution[i];
          memset(lower, 0, (numberMasterRows + numberBlocks) *sizeof(double));
          master.times(1.0, sol, lower);
          for (iRow = 0; iRow < numberMasterRows; iRow++) {
               double value = lower[iRow];
               if (masterUpper[iRow] < 1.0e20)
                    upper[iRow] = value;
               else
                    upper[iRow] = COIN_DBL_MAX;
               if (masterLower[iRow] > -1.0e20)
                    lower[iRow] = value;
               else
                    lower[iRow] = -COIN_DBL_MAX;
          }
          sub[iBlock].addRows(numberMasterRows, lower, upper,
                              top[iBlock].getVectorStarts(),
                              top[iBlock].getVectorLengths(),
                              top[iBlock].getIndices(),
                              top[iBlock].getElements());
          sub[iBlock].primal();
          const double * subSolution = sub[iBlock].primalColumnSolution();
          const double * subRowSolution = sub[iBlock].primalRowSolution();
          // move solution
          kColumn = 0;
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (columnBlock[iColumn] == iBlock) {
                    model.setStatus(iColumn, sub[iBlock].getStatus(kColumn));
                    fullSolution[iColumn] = subSolution[kColumn++];
               }
          }
          assert(kColumn == sub[iBlock].numberColumns());
          kRow = 0;
          for (iRow = 0; iRow < numberRows; iRow++) {
               if (rowBlock[iRow] == iBlock) {
                    model.setRowStatus(iRow, sub[iBlock].getRowStatus(kRow));
                    fullRowSolution[iRow] = subRowSolution[kRow++];
               }
          }
          assert(kRow == sub[iBlock].numberRows() - numberMasterRows);
     }
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          if (fullSolution[iColumn] < fullUpper[iColumn] - 1.0e-8 &&
                    fullSolution[iColumn] > fullLower[iColumn] + 1.0e-8) {
               assert(model.getStatus(iColumn) == ClpSimplex::basic);
          } else if (fullSolution[iColumn] >= fullUpper[iColumn] - 1.0e-8) {
               // may help to make rest non basic
               model.setStatus(iColumn, ClpSimplex::atUpperBound);
          } else if (fullSolution[iColumn] <= fullLower[iColumn] + 1.0e-8) {
               // may help to make rest non basic
               model.setStatus(iColumn, ClpSimplex::atLowerBound);
          }
     }
     for (iRow = 0; iRow < numberRows; iRow++)
          model.setRowStatus(iRow, ClpSimplex::superBasic);
     model.primal(1);
     delete [] sol;
     delete [] lower;
     delete [] upper;
     delete [] whichBlock;
     delete [] when;
     delete [] columnAdd;
     delete [] rowAdd;
     delete [] elementAdd;
     delete [] objective;
     delete [] top;
     delete [] sub;
     delete [] rowBlock;
     delete [] columnBlock;
     return 0;
}
