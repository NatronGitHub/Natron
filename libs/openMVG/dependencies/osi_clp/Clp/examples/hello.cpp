/* $Id$ */
/* Copyright (C) 2004, International Business Machines Corporation
   and others.  All Rights Reserved.

   This sample program is designed to illustrate programming
   techniques using CoinLP, has not been thoroughly tested
   and comes without any warranty whatsoever.

   You may copy, modify and distribute this sample program without
   any restrictions whatsoever and without any payment to anyone.
*/

/* This shows how to provide a simple picture of a matrix.
   The default matrix will print Hello World
*/

#include "ClpSimplex.hpp"

int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;
     // Keep names
     if (argc < 2) {
          status = model.readMps("hello.mps", true);
     } else {
          status = model.readMps(argv[1], true);
     }
     if (status)
          exit(10);

     int numberColumns = model.numberColumns();
     int numberRows = model.numberRows();

     if (numberColumns > 80 || numberRows > 80) {
          printf("model too large\n");
          exit(11);
     }
     printf("This prints x wherever a non-zero element exists in the matrix.\n\n\n");

     char x[81];

     int iRow;
     // get row copy
     CoinPackedMatrix rowCopy = *model.matrix();
     rowCopy.reverseOrdering();
     const int * column = rowCopy.getIndices();
     const int * rowLength = rowCopy.getVectorLengths();
     const CoinBigIndex * rowStart = rowCopy.getVectorStarts();

     x[numberColumns] = '\0';
     for (iRow = 0; iRow < numberRows; iRow++) {
          memset(x, ' ', numberColumns);
          for (int k = rowStart[iRow]; k < rowStart[iRow] + rowLength[iRow]; k++) {
               int iColumn = column[k];
               x[iColumn] = 'x';
          }
          printf("%s\n", x);
     }
     printf("\n\n");
     return 0;
}
