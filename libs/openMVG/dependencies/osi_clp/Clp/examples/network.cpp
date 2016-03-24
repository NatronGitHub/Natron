/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


// The point of this example is to show how to create a model without
// using mps files.

// This reads a network problem created by netgen which can be
// downloaded from www.netlib.org/lp/generators/netgen
// This is a generator due to Darwin Klingman

#include "ClpSimplex.hpp"
#include "ClpFactorization.hpp"
#include "ClpNetworkMatrix.hpp"
#include <stdio.h>
#include <assert.h>
#include <cmath>

int main(int argc, const char *argv[])
{
     int numberColumns;
     int numberRows;

     FILE * fp;
     if (argc > 1) {
          fp = fopen(argv[1], "r");
          if (!fp) {
               fprintf(stderr, "Unable to open file %s\n", argv[1]);
               exit(1);
          }
     } else {
          fp = fopen("input.130", "r");
          if (!fp) {
               fprintf(stderr, "Unable to open file input.l30 in Samples directory\n");
               exit(1);
          }
     }
     int problem;
     char temp[100];
     // read and skip
     fscanf(fp, "%s", temp);
     assert(!strcmp(temp, "BEGIN"));
     fscanf(fp, "%*s %*s %d %d %*s %*s %d %*s", &problem, &numberRows,
            &numberColumns);
     // scan down to SUPPLY
     while (fgets(temp, 100, fp)) {
          if (!strncmp(temp, "SUPPLY", 6))
               break;
     }
     if (strncmp(temp, "SUPPLY", 6)) {
          fprintf(stderr, "Unable to find SUPPLY\n");
          exit(2);
     }
     // get space for rhs
     double * lower = new double[numberRows];
     double * upper = new double[numberRows];
     int i;
     for (i = 0; i < numberRows; i++) {
          lower[i] = 0.0;
          upper[i] = 0.0;
     }
     // ***** Remember to convert to C notation
     while (fgets(temp, 100, fp)) {
          int row;
          int value;
          if (!strncmp(temp, "ARCS", 4))
               break;
          sscanf(temp, "%d %d", &row, &value);
          upper[row-1] = -value;
          lower[row-1] = -value;
     }
     if (strncmp(temp, "ARCS", 4)) {
          fprintf(stderr, "Unable to find ARCS\n");
          exit(2);
     }
     // number of columns may be underestimate
     int * head = new int[2*numberColumns];
     int * tail = new int[2*numberColumns];
     int * ub = new int[2*numberColumns];
     int * cost = new int[2*numberColumns];
     // ***** Remember to convert to C notation
     numberColumns = 0;
     while (fgets(temp, 100, fp)) {
          int iHead;
          int iTail;
          int iUb;
          int iCost;
          if (!strncmp(temp, "DEMAND", 6))
               break;
          sscanf(temp, "%d %d %d %d", &iHead, &iTail, &iCost, &iUb);
          iHead--;
          iTail--;
          head[numberColumns] = iHead;
          tail[numberColumns] = iTail;
          ub[numberColumns] = iUb;
          cost[numberColumns] = iCost;
          numberColumns++;
     }
     if (strncmp(temp, "DEMAND", 6)) {
          fprintf(stderr, "Unable to find DEMAND\n");
          exit(2);
     }
     // ***** Remember to convert to C notation
     while (fgets(temp, 100, fp)) {
          int row;
          int value;
          if (!strncmp(temp, "END", 3))
               break;
          sscanf(temp, "%d %d", &row, &value);
          upper[row-1] = value;
          lower[row-1] = value;
     }
     if (strncmp(temp, "END", 3)) {
          fprintf(stderr, "Unable to find END\n");
          exit(2);
     }
     printf("Problem %d has %d rows and %d columns\n", problem,
            numberRows, numberColumns);
     fclose(fp);
     ClpSimplex  model;
     // now build model - we have rhs so build columns - two elements
     // per column

     double * objective = new double[numberColumns];
     double * lowerColumn = new double[numberColumns];
     double * upperColumn = new double[numberColumns];

     double * element = new double [2*numberColumns];
     int * start = new int[numberColumns+1];
     int * row = new int[2*numberColumns];
     start[numberColumns] = 2 * numberColumns;
     for (i = 0; i < numberColumns; i++) {
          start[i] = 2 * i;
          element[2*i] = -1.0;
          element[2*i+1] = 1.0;
          row[2*i] = head[i];
          row[2*i+1] = tail[i];
          lowerColumn[i] = 0.0;
          upperColumn[i] = ub[i];
          objective[i] = cost[i];
     }
     // Create Packed Matrix
     CoinPackedMatrix matrix;
     int * lengths = NULL;
     matrix.assignMatrix(true, numberRows, numberColumns,
                         2 * numberColumns, element, row, start, lengths);
     ClpNetworkMatrix network(matrix);
     // load model

     model.loadProblem(network,
                       lowerColumn, upperColumn, objective,
                       lower, upper);

     delete [] lower;
     delete [] upper;
     delete [] head;
     delete [] tail;
     delete [] ub;
     delete [] cost;
     delete [] objective;
     delete [] lowerColumn;
     delete [] upperColumn;
     delete [] element;
     delete [] start;
     delete [] row;

     /* The confusing flow below is in to exercise both dual and primal
        when ClpNetworkMatrix storage used.

        For practical use just one call e.g. model.dual(); would be used.

        If network then factorization scheme is changed
        to be much faster.

        Still not as fast as a real network code, but more flexible
     */
     model.factorization()->maximumPivots(200 + model.numberRows() / 100);
     model.factorization()->maximumPivots(1000);
     //model.factorization()->maximumPivots(1);
     if (model.numberRows() < 50)
          model.messageHandler()->setLogLevel(63);
     model.dual();
     model.setOptimizationDirection(-1);
     //model.messageHandler()->setLogLevel(63);
     model.primal();
     model.setOptimizationDirection(1);
     model.primal();
     return 0;
}
