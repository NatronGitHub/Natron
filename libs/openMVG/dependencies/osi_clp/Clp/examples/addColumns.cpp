/* $Id$ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


// This is a simple example to create a model by column
#include "ClpSimplex.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "CoinBuild.hpp"
#include "CoinModel.hpp"
#include <iomanip>
#include <cassert>

int main(int argc, const char *argv[])
{
     {
          // Empty model
          ClpSimplex  model;

          // Bounds on rows - as dense vector
          double lower[] = {2.0, 1.0};
          double upper[] = {COIN_DBL_MAX, 1.0};

          // Create space for 2 rows
          model.resize(2, 0);
          // Fill in
          int i;
          // Now row bounds
          for (i = 0; i < 2; i++) {
               model.setRowLower(i, lower[i]);
               model.setRowUpper(i, upper[i]);
          }
          // Now add column 1
          int column1Index[] = {0, 1};
          double column1Value[] = {1.0, 1.0};
          model.addColumn(2, column1Index, column1Value,
                          0.0, 2, 1.0);
          // Now add column 2
          int column2Index[] = {1};
          double column2Value[] = { -5.0};
          model.addColumn(1, column2Index, column2Value,
                          0.0, COIN_DBL_MAX, 0.0);
          // Now add column 3
          int column3Index[] = {0, 1};
          double column3Value[] = {1.0, 1.0};
          model.addColumn(2, column3Index, column3Value,
                          0.0, 4.0, 4.0);
          // solve
          model.dual();

          /*
            Adding one column at a time has a significant overhead so let's
            try a more complicated but faster way

            First time adding in 10000 columns one by one

          */
          model.allSlackBasis();
          ClpSimplex modelSave = model;
          double time1 = CoinCpuTime();
          int k;
          for (k = 0; k < 10000; k++) {
               int column2Index[] = {0, 1};
               double column2Value[] = {1.0, -5.0};
               model.addColumn(2, column2Index, column2Value,
                               0.0, 1.0, 10000.0);
          }
          printf("Time for 10000 addColumn is %g\n", CoinCpuTime() - time1);
          model.dual();
          model = modelSave;
          // Now use build
          CoinBuild buildObject;
          time1 = CoinCpuTime();
          for (k = 0; k < 100000; k++) {
               int column2Index[] = {0, 1};
               double column2Value[] = {1.0, -5.0};
               buildObject.addColumn(2, column2Index, column2Value,
                                     0.0, 1.0, 10000.0);
          }
          model.addColumns(buildObject);
          printf("Time for 100000 addColumn using CoinBuild is %g\n", CoinCpuTime() - time1);
          model.dual();
          model = modelSave;
          // Now use build +-1
          int del[] = {0, 1, 2};
          model.deleteColumns(3, del);
          CoinBuild buildObject2;
          time1 = CoinCpuTime();
          for (k = 0; k < 10000; k++) {
               int column2Index[] = {0, 1};
               double column2Value[] = {1.0, 1.0, -1.0};
               int bias = k & 1;
               buildObject2.addColumn(2, column2Index, column2Value + bias,
                                      0.0, 1.0, 10000.0);
          }
          model.addColumns(buildObject2, true);
          printf("Time for 10000 addColumn using CoinBuild+-1 is %g\n", CoinCpuTime() - time1);
          model.dual();
          model = modelSave;
          // Now use build +-1
          model.deleteColumns(3, del);
          CoinModel modelObject2;
          time1 = CoinCpuTime();
          for (k = 0; k < 10000; k++) {
               int column2Index[] = {0, 1};
               double column2Value[] = {1.0, 1.0, -1.0};
               int bias = k & 1;
               modelObject2.addColumn(2, column2Index, column2Value + bias,
                                      0.0, 1.0, 10000.0);
          }
          model.addColumns(modelObject2, true);
          printf("Time for 10000 addColumn using CoinModel+-1 is %g\n", CoinCpuTime() - time1);
          //model.writeMps("xx.mps");
          model.dual();
          model = modelSave;
          // Now use model
          CoinModel modelObject;
          time1 = CoinCpuTime();
          for (k = 0; k < 100000; k++) {
               int column2Index[] = {0, 1};
               double column2Value[] = {1.0, -5.0};
               modelObject.addColumn(2, column2Index, column2Value,
                                     0.0, 1.0, 10000.0);
          }
          model.addColumns(modelObject);
          printf("Time for 100000 addColumn using CoinModel is %g\n", CoinCpuTime() - time1);
          model.dual();
          // Print column solution Just first 3 columns
          int numberColumns = model.numberColumns();
          numberColumns = CoinMin(3, numberColumns);

          // Alternatively getColSolution()
          double * columnPrimal = model.primalColumnSolution();
          // Alternatively getReducedCost()
          double * columnDual = model.dualColumnSolution();
          // Alternatively getColLower()
          double * columnLower = model.columnLower();  // Alternatively getColUpper()
          double * columnUpper = model.columnUpper();
          // Alternatively getObjCoefficients()
          double * columnObjective = model.objective();

          int iColumn;

          std::cout << "               Primal          Dual         Lower         Upper          Cost"
                    << std::endl;

          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               double value;
               std::cout << std::setw(6) << iColumn << " ";
               value = columnPrimal[iColumn];
               if (fabs(value) < 1.0e5)
                    std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
               else
                    std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
               value = columnDual[iColumn];
               if (fabs(value) < 1.0e5)
                    std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
               else
                    std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
               value = columnLower[iColumn];
               if (fabs(value) < 1.0e5)
                    std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
               else
                    std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
               value = columnUpper[iColumn];
               if (fabs(value) < 1.0e5)
                    std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
               else
                    std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
               value = columnObjective[iColumn];
               if (fabs(value) < 1.0e5)
                    std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
               else
                    std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;

               std::cout << std::endl;
          }
          std::cout << "--------------------------------------" << std::endl;
     }
     {
          // Now copy a model
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
          model.initialSolve();
          int numberRows = model.numberRows();
          int numberColumns = model.numberColumns();
          const double * rowLower = model.rowLower();
          const double * rowUpper = model.rowUpper();

          // Start off model2
          ClpSimplex model2;
          model2.addRows(numberRows, rowLower, rowUpper, NULL);

          // Build object
          CoinBuild buildObject;
          // Add columns
          const double * columnLower = model.columnLower();
          const double * columnUpper = model.columnUpper();
          const double * objective = model.objective();
          CoinPackedMatrix * matrix = model.matrix();
          const int * row = matrix->getIndices();
          const int * columnLength = matrix->getVectorLengths();
          const CoinBigIndex * columnStart = matrix->getVectorStarts();
          const double * elementByColumn = matrix->getElements();
          for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
               CoinBigIndex start = columnStart[iColumn];
               buildObject.addColumn(columnLength[iColumn], row + start, elementByColumn + start,
                                     columnLower[iColumn], columnUpper[iColumn],
                                     objective[iColumn]);
          }

          // add in
          model2.addColumns(buildObject);
          model2.initialSolve();
     }
     {
          // and again
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
          model.initialSolve();
          int numberRows = model.numberRows();
          int numberColumns = model.numberColumns();
          const double * rowLower = model.rowLower();
          const double * rowUpper = model.rowUpper();

          // Build object
          CoinModel buildObject;
          for (int iRow = 0; iRow < numberRows; iRow++)
               buildObject.setRowBounds(iRow, rowLower[iRow], rowUpper[iRow]);

          // Add columns
          const double * columnLower = model.columnLower();
          const double * columnUpper = model.columnUpper();
          const double * objective = model.objective();
          CoinPackedMatrix * matrix = model.matrix();
          const int * row = matrix->getIndices();
          const int * columnLength = matrix->getVectorLengths();
          const CoinBigIndex * columnStart = matrix->getVectorStarts();
          const double * elementByColumn = matrix->getElements();
          for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
               CoinBigIndex start = columnStart[iColumn];
               buildObject.addColumn(columnLength[iColumn], row + start, elementByColumn + start,
                                     columnLower[iColumn], columnUpper[iColumn],
                                     objective[iColumn]);
          }

          // add in
          ClpSimplex model2;
          model2.loadProblem(buildObject);
          model2.initialSolve();
     }
     return 0;
}
