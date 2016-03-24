/* $Id$ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*
  This is a simple example to create a model using CoinModel.
  For even simpler methods see addRows.cpp and addColumns.cpp

  This reads in one model and then creates another one:
  a) Row bounds
  b) Column bounds and objective
  c) Adds elements one by one by row.

  Solve

  It then repeats the exercise:
  a) Column bounds
  b) Objective - half the columns as is and half with multiplier of "1.0+multiplier"
  c) It then adds rows one by one but for half the rows sets their values
          with multiplier of "1.0+1.5*multiplier" where column affected

  It then loops with multiplier going from 0.0 to 2.0 in increments of 0.1

  (you can have as many different strings as you want)
*/
#include "ClpSimplex.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "CoinModel.hpp"
#include <iomanip>
#include <cassert>

int main(int argc, const char *argv[])
{
     // Empty model
     ClpSimplex  model;
     std::string mpsFileName;
     if (argc >= 2) mpsFileName = argv[1];
     else {
#if defined(NETLIBDIR)
          mpsFileName = NETLIBDIR "/25fv47.mps";
#else
          fprintf(stderr, "Do not know where to find netlib MPS files.\n");
          exit(1);
#endif
     }
     int status = model.readMps(mpsFileName.c_str(), true);

     if (status) {
          fprintf(stderr, "Bad readMps %s\n", mpsFileName.c_str());
          fprintf(stdout, "Bad readMps %s\n", mpsFileName.c_str());
          exit(1);
     }
     // Point to data
     int numberRows = model.numberRows();
     const double * rowLower = model.rowLower();
     const double * rowUpper = model.rowUpper();
     int numberColumns = model.numberColumns();
     const double * columnLower = model.columnLower();
     const double * columnUpper = model.columnUpper();
     const double * columnObjective = model.objective();
     CoinPackedMatrix * matrix = model.matrix();
     // get row copy
     CoinPackedMatrix rowCopy = *matrix;
     rowCopy.reverseOrdering();
     const int * column = rowCopy.getIndices();
     const int * rowLength = rowCopy.getVectorLengths();
     const CoinBigIndex * rowStart = rowCopy.getVectorStarts();
     const double * element = rowCopy.getElements();
     //const int * row = matrix->getIndices();
     //const int * columnLength = matrix->getVectorLengths();
     //const CoinBigIndex * columnStart = matrix->getVectorStarts();
     //const double * elementByColumn = matrix->getElements();

     // solve
     model.dual();
     // Now build new model
     CoinModel build;
     double time1 = CoinCpuTime();
     // Row bounds
     int iRow;
     for (iRow = 0; iRow < numberRows; iRow++) {
          build.setRowBounds(iRow, rowLower[iRow], rowUpper[iRow]);
          // optional name
          build.setRowName(iRow, model.rowName(iRow).c_str());
     }
     // Column bounds and objective
     int iColumn;
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          build.setColumnLower(iColumn, columnLower[iColumn]);
          build.setColumnUpper(iColumn, columnUpper[iColumn]);
          build.setObjective(iColumn, columnObjective[iColumn]);
          // optional name
          build.setColumnName(iColumn, model.columnName(iColumn).c_str());
     }
     // Adds elements one by one by row (backwards by row)
     for (iRow = numberRows - 1; iRow >= 0; iRow--) {
          int start = rowStart[iRow];
          for (int j = start; j < start + rowLength[iRow]; j++)
               build(iRow, column[j], element[j]);
     }
     double time2 = CoinCpuTime();
     // Now create clpsimplex
     ClpSimplex model2;
     model2.loadProblem(build);
     double time3 = CoinCpuTime();
     printf("Time for build using CoinModel is %g (%g for loadproblem)\n", time3 - time1,
            time3 - time2);
     model2.dual();
     // Now do with strings attached
     // Save build to show how to go over rows
     CoinModel saveBuild = build;
     build = CoinModel();
     time1 = CoinCpuTime();
     // Column bounds
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          build.setColumnLower(iColumn, columnLower[iColumn]);
          build.setColumnUpper(iColumn, columnUpper[iColumn]);
     }
     // Objective - half the columns as is and half with multiplier of "1.0+multiplier"
     // Pick up from saveBuild (for no reason at all)
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          double value = saveBuild.objective(iColumn);
          if (iColumn * 2 < numberColumns) {
               build.setObjective(iColumn, columnObjective[iColumn]);
          } else {
               // create as string
               char temp[100];
               sprintf(temp, "%g + abs(%g*multiplier)", value, value);
               build.setObjective(iColumn, temp);
          }
     }
     // It then adds rows one by one but for half the rows sets their values
     //      with multiplier of "1.0+1.5*multiplier"
     for (iRow = 0; iRow < numberRows; iRow++) {
          if (iRow * 2 < numberRows) {
               // add row in simple way
               int start = rowStart[iRow];
               build.addRow(rowLength[iRow], column + start, element + start,
                            rowLower[iRow], rowUpper[iRow]);
          } else {
               // As we have to add one by one let's get from saveBuild
               CoinModelLink triple = saveBuild.firstInRow(iRow);
               while (triple.column() >= 0) {
                    int iColumn = triple.column();
                    if (iColumn * 2 < numberColumns) {
                         // just value as normal
                         build(iRow, triple.column(), triple.value());
                    } else {
                         // create as string
                         char temp[100];
                         sprintf(temp, "%g + (1.5*%g*multiplier)", triple.value(), triple.value());
                         build(iRow, iColumn, temp);
                    }
                    triple = saveBuild.next(triple);
               }
               // but remember to do rhs
               build.setRowLower(iRow, rowLower[iRow]);
               build.setRowUpper(iRow, rowUpper[iRow]);
          }
     }
     time2 = CoinCpuTime();
     // Now create ClpSimplex
     // If small switch on error printing
     if (numberColumns < 50)
          build.setLogLevel(1);
     // should fail as we never set multiplier
     int numberErrors = model2.loadProblem(build);
     if( numberErrors == 0 )
     {
        printf("%d errors from model2.loadProblem(build), but we expected some", numberErrors);
        return 1;
     }
     time3 = CoinCpuTime() - time2;
     // subtract out unsuccessful times
     time1 += time3;
     time2 += time3;
     build.associateElement("multiplier", 0.0);
     numberErrors = model2.loadProblem(build);
     assert(!numberErrors);
     time3 = CoinCpuTime();
     printf("Time for build using CoinModel is %g (%g for successful loadproblem)\n", time3 - time1,
            time3 - time2);
     build.writeMps("zero.mps");
     // It then loops with multiplier going from 0.0 to 2.0 in increments of 0.1
     for (double multiplier = 0.0; multiplier < 2.0; multiplier += 0.1) {
          build.associateElement("multiplier", multiplier);
          numberErrors = model2.loadProblem(build, true);
          assert(!numberErrors);
          model2.dual();
     }

     return 0;
}
