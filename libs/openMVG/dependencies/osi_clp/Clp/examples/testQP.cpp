/* $Id$ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinMpsIO.hpp"
#include "ClpInterior.hpp"
#include "ClpSimplex.hpp"
#include "ClpCholeskyBase.hpp"
#include "ClpQuadraticObjective.hpp"
#include <cassert>
int main(int argc, const char *argv[])
{
     /* Read quadratic model in two stages to test loadQuadraticObjective.

        And is also possible to just read into ClpSimplex/Interior which sets it all up in one go.
        But this is only if it is in QUADOBJ format.

        If no arguments does share2qp using ClpInterior (also creates quad.mps which is in QUADOBJ format)
        If one argument uses simplex e.g. testit quad.mps
        If > one uses barrier via ClpSimplex input and then ClpInterior borrow
     */
     if (argc < 2) {
          CoinMpsIO  m;
#if defined(SAMPLEDIR)
          int status = m.readMps(SAMPLEDIR "/share2qp", "mps");
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
          if (status) {
               printf("errors on input\n");
               exit(77);
          }
          ClpInterior model;
          model.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                            m.getObjCoefficients(),
                            m.getRowLower(), m.getRowUpper());
          // get quadratic part
          int * start = NULL;
          int * column = NULL;
          double * element = NULL;
          m.readQuadraticMps(NULL, start, column, element, 2);
          int j;
          for (j = 0; j < 79; j++) {
               if (start[j] < start[j+1]) {
                    int i;
                    printf("Column %d ", j);
                    for (i = start[j]; i < start[j+1]; i++) {
                         printf("( %d, %g) ", column[i], element[i]);
                    }
                    printf("\n");
               }
          }
          model.loadQuadraticObjective(model.numberColumns(), start, column, element);
          // share2qp is in old style qp format - convert to new so other options can use
          model.writeMps("quad.mps");
          ClpCholeskyBase * cholesky = new ClpCholeskyBase();
          cholesky->setKKT(true);
          model.setCholesky(cholesky);
          model.primalDual();
          double *primal;
          double *dual;
          primal = model.primalColumnSolution();
          dual = model.dualRowSolution();
          int i;
          int numberColumns = model.numberColumns();
          int numberRows = model.numberRows();
          for (i = 0; i < numberColumns; i++) {
               if (fabs(primal[i]) > 1.0e-8)
                    printf("%d primal %g\n", i, primal[i]);
          }
          for (i = 0; i < numberRows; i++) {
               if (fabs(dual[i]) > 1.0e-8)
                    printf("%d dual %g\n", i, dual[i]);
          }
     } else {
          // Could read into ClpInterior
          ClpSimplex model;
          if (model.readMps(argv[1])) {
               printf("errors on input\n");
               exit(77);
          }
          model.writeMps("quad");
          if (argc < 3) {
               // simplex - just primal as dual does not work
               // also I need to fix scaling of duals on output
               // (Was okay in first place - can't mix and match scaling techniques)
               // model.scaling(0);
               model.primal();
          } else {
               // barrier
               ClpInterior barrier;
               barrier.borrowModel(model);
               ClpCholeskyBase * cholesky = new ClpCholeskyBase();
               cholesky->setKKT(true);
               barrier.setCholesky(cholesky);
               barrier.primalDual();
               barrier.returnModel(model);
          }
          // Just check if share2qp (quad.mps here)
          // this is because I am not checking if variables at ub
          if (model.numberColumns() == 79) {
               double *primal;
               double *dual;
               primal = model.primalColumnSolution();
               dual = model.dualRowSolution();
               // Check duals by hand
               const ClpQuadraticObjective * quadraticObj =
                    (dynamic_cast<const ClpQuadraticObjective*>(model.objectiveAsObject()));
               assert(quadraticObj);
               CoinPackedMatrix * quad = quadraticObj->quadraticObjective();
               const int * columnQuadratic = quad->getIndices();
               const CoinBigIndex * columnQuadraticStart = quad->getVectorStarts();
               const int * columnQuadraticLength = quad->getVectorLengths();
               const double * quadraticElement = quad->getElements();
               int numberColumns = model.numberColumns();
               int numberRows = model.numberRows();
               double * gradient = new double [numberColumns];
               // move linear objective
               memcpy(gradient, quadraticObj->linearObjective(), numberColumns * sizeof(double));
               int iColumn;
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    double valueI = primal[iColumn];
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = primal[jColumn];
                         double elementValue = quadraticElement[j];
                         if (iColumn != jColumn) {
                              double gradientI = valueJ * elementValue;
                              double gradientJ = valueI * elementValue;
                              gradient[iColumn] += gradientI;
                              gradient[jColumn] += gradientJ;
                         } else {
                              double gradientI = valueI * elementValue;
                              gradient[iColumn] += gradientI;
                         }
                    }
                    if (fabs(primal[iColumn]) > 1.0e-8)
                         printf("%d primal %g\n", iColumn, primal[iColumn]);
               }
               for (int i = 0; i < numberRows; i++) {
                    if (fabs(dual[i]) > 1.0e-8)
                         printf("%d dual %g\n", i, dual[i]);
               }
               // Now use duals to get reduced costs
               // Can't use this as will try and use scaling
               // model.transposeTimes(-1.0,dual,gradient);
               // So ...
               CoinPackedMatrix * matrix = model.matrix();
               const int * row = matrix->getIndices();
               const CoinBigIndex * columnStart = matrix->getVectorStarts();
               const int * columnLength = matrix->getVectorLengths();
               const double * element = matrix->getElements();
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    double dj = gradient[iColumn];
                    CoinBigIndex j;
                    for (j = columnStart[iColumn];
                              j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                         int jRow = row[j];
                         dj -= element[j] * dual[jRow];
                    }
                    if (model.getColumnStatus(iColumn) == ClpSimplex::basic) {
                         assert(fabs(dj) < 1.0e-5);
                    } else {
                         assert(dj > -1.0e-5);
                    }
               }
               delete [] gradient;
          }
     }
     return 0;
}
