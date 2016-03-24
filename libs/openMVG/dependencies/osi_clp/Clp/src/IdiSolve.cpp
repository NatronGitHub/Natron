/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include "CoinHelperFunctions.hpp"
#include "Idiot.hpp"
#define FIT
#ifdef FIT
#define HISTORY 8
#else
#define HISTORY 7
#endif
#define NSOLVE HISTORY-1
static void solveSmall(int nsolve, double **aIn, double **a, double * b)
{
     int i, j;
     /* copy */
     for (i = 0; i < nsolve; i++) {
          for (j = 0; j < nsolve; j++) {
               a[i][j] = aIn[i][j];
          }
     }
     for (i = 0; i < nsolve; i++) {
          /* update using all previous */
          double diagonal;
          int j;
          for (j = i; j < nsolve; j++) {
               int k;
               double value = a[i][j];
               for (k = 0; k < i; k++) {
                    value -= a[k][i] * a[k][j];
               }
               a[i][j] = value;
          }
          diagonal = a[i][i];
          if (diagonal < 1.0e-20) {
               diagonal = 0.0;
          } else {
               diagonal = 1.0 / sqrt(diagonal);
          }
          a[i][i] = diagonal;
          for (j = i + 1; j < nsolve; j++) {
               a[i][j] *= diagonal;
          }
     }
     /* update */
     for (i = 0; i < nsolve; i++) {
          int j;
          double value = b[i];
          for (j = 0; j < i; j++) {
               value -= b[j] * a[j][i];
          }
          value *= a[i][i];
          b[i] = value;
     }
     for (i = nsolve - 1; i >= 0; i--) {
          int j;
          double value = b[i];
          for (j = i + 1; j < nsolve; j++) {
               value -= b[j] * a[i][j];
          }
          value *= a[i][i];
          b[i] = value;
     }
}
IdiotResult
Idiot::objval(int nrows, int ncols, double * rowsol , double * colsol,
              double * pi, double * /*djs*/, const double * cost ,
              const double * /*rowlower*/,
              const double * rowupper, const double * /*lower*/,
              const double * /*upper*/, const double * elemnt,
              const int * row, const CoinBigIndex * columnStart,
              const int * length, int extraBlock, int * rowExtra,
              double * solExtra, double * elemExtra, double * /*upperExtra*/,
              double * costExtra, double weight)
{
     IdiotResult result;
     double objvalue = 0.0;
     double sum1 = 0.0, sum2 = 0.0;
     int i;
     for (i = 0; i < nrows; i++) {
          rowsol[i] = -rowupper[i];
     }
     for (i = 0; i < ncols; i++) {
          CoinBigIndex j;
          double value = colsol[i];
          if (value) {
               objvalue += value * cost[i];
               if (elemnt) {
                    for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                         int irow = row[j];
                         rowsol[irow] += elemnt[j] * value;
                    }
               } else {
                    for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                         int irow = row[j];
                         rowsol[irow] += value;
                    }
               }
          }
     }
     /* adjust to make as feasible as possible */
     /* no */
     if (extraBlock) {
          for (i = 0; i < extraBlock; i++) {
               double element = elemExtra[i];
               int irow = rowExtra[i];
               objvalue += solExtra[i] * costExtra[i];
               rowsol[irow] += solExtra[i] * element;
          }
     }
     for (i = 0; i < nrows; i++) {
          double value = rowsol[i];
          sum1 += fabs(value);
          sum2 += value * value;
          pi[i] = -2.0 * weight * value;
     }
     result.infeas = sum1;
     result.objval = objvalue;
     result.weighted = objvalue + weight * sum2;
     result.sumSquared = sum2;
     return result;
}
IdiotResult
Idiot::IdiSolve(
     int nrows, int ncols, double * COIN_RESTRICT rowsol , double * COIN_RESTRICT colsol,
     double * COIN_RESTRICT pi, double * COIN_RESTRICT djs, const double * COIN_RESTRICT origcost , double * COIN_RESTRICT rowlower,
     double * COIN_RESTRICT rowupper, const double * COIN_RESTRICT lower,
     const double * COIN_RESTRICT upper, const double * COIN_RESTRICT elemnt,
     const int * row, const CoinBigIndex * columnStart,
     const int * length, double * COIN_RESTRICT lambda,
     int maxIts, double mu, double drop,
     double maxmin, double offset,
     int strategy, double djTol, double djExit, double djFlag,
     CoinThreadRandom * randomNumberGenerator)
{
     IdiotResult result;
     int  i, j, k, iter;
     double value = 0.0, objvalue = 0.0, weightedObj = 0.0;
     double tolerance = 1.0e-8;
     double * history[HISTORY+1];
     int ncolx;
     int nChange;
     int extraBlock = 0;
     int * rowExtra = NULL;
     double * COIN_RESTRICT solExtra = NULL;
     double * COIN_RESTRICT elemExtra = NULL;
     double * COIN_RESTRICT upperExtra = NULL;
     double * COIN_RESTRICT costExtra = NULL;
     double * COIN_RESTRICT useCostExtra = NULL;
     double * COIN_RESTRICT saveExtra = NULL;
     double * COIN_RESTRICT cost = NULL;
     double saveValue = 1.0e30;
     double saveOffset = offset;
     double useOffset = offset;
     /*#define NULLVECTOR*/
#ifndef NULLVECTOR
     int nsolve = NSOLVE;
#else
     int nsolve = NSOLVE + 1; /* allow for null vector */
#endif
     int nflagged;
     double * COIN_RESTRICT thetaX;
     double * COIN_RESTRICT djX;
     double * COIN_RESTRICT bX;
     double * COIN_RESTRICT vX;
     double ** aX;
     double **aworkX;
     double ** allsum;
     double * COIN_RESTRICT saveSol = 0;
     const double * COIN_RESTRICT useCost = cost;
     double bestSol = 1.0e60;
     double weight = 0.5 / mu;
     char * statusSave = new char[2*ncols];
     char * statusWork = statusSave + ncols;
#define DJTEST 5
     double djSave[DJTEST];
     double largestDj = 0.0;
     double smallestDj = 1.0e60;
     double maxDj = 0.0;
     int doFull = 0;
#define SAVEHISTORY 10
#define EVERY (2*SAVEHISTORY)
#define AFTER SAVEHISTORY*(HISTORY+1)
#define DROP 5
     double after = AFTER;
     double obj[DROP];
     double kbad = 0, kgood = 0;
     if (strategy & 128) after = 999999; /* no acceleration at all */
     for (i = 0; i < DROP; i++) {
          obj[i] = 1.0e70;
     }
     //#define FOUR_GOES 2
#ifdef FOUR_GOES
     double * COIN_RESTRICT pi2 = new double [3*nrows];
     double * COIN_RESTRICT rowsol2 = new double [3*nrows];
     double * COIN_RESTRICT piX[4];
     double * COIN_RESTRICT rowsolX[4];
     int startsX[2][5];
     int nChangeX[4];
     double maxDjX[4];
     double objvalueX[4];
     int nflaggedX[4];
     piX[0]=pi;
     piX[1]=pi2;
     piX[2]=pi2+nrows;
     piX[3]=piX[2]+nrows;
     rowsolX[0]=rowsol;
     rowsolX[1]=rowsol2;
     rowsolX[2]=rowsol2+nrows;
     rowsolX[3]=rowsolX[2]+nrows;
#endif
     allsum = new double * [nsolve];
     aX = new double * [nsolve];
     aworkX = new double * [nsolve];
     thetaX = new double[nsolve];
     vX = new double[nsolve];
     bX = new double[nsolve];
     djX = new double[nsolve];
     allsum[0] = pi;
     for (i = 0; i < nsolve; i++) {
          if (i) allsum[i] = new double[nrows];
          aX[i] = new double[nsolve];
          aworkX[i] = new double[nsolve];
     }
     /* check = rows */
     for (i = 0; i < nrows; i++) {
          if (rowupper[i] - rowlower[i] > tolerance) {
               extraBlock++;
          }
     }
     cost = new double[ncols];
     memset(rowsol, 0, nrows * sizeof(double));
     for (i = 0; i < ncols; i++) {
          CoinBigIndex j;
          double value = origcost[i] * maxmin;
          double value2 = colsol[i];
          if (elemnt) {
               for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                    int irow = row[j];
                    value += elemnt[j] * lambda[irow];
                    rowsol[irow] += elemnt[j] * value2;
               }
          } else {
               for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                    int irow = row[j];
                    value += lambda[irow];
                    rowsol[irow] += value2;
               }
          }
          cost[i] = value;
     }
     if (extraBlock) {
          rowExtra = new int[extraBlock];
          solExtra = new double[extraBlock];
          elemExtra = new double[extraBlock];
          upperExtra = new double[extraBlock];
          costExtra = new double[extraBlock];
          saveExtra = new double[extraBlock];
          extraBlock = 0;
          int nbad = 0;
          for (i = 0; i < nrows; i++) {
               if (rowupper[i] - rowlower[i] > tolerance) {
                    double smaller, difference;
                    double value;
                    saveExtra[extraBlock] = rowupper[i];
                    if (fabs(rowupper[i]) > fabs(rowlower[i])) {
                         smaller = rowlower[i];
                         value = -1.0;
                    } else {
                         smaller = rowupper[i];
                         value = 1.0;
                    }
                    if (fabs(smaller) > 1.0e10) {
                         if (!nbad)
                              COIN_DETAIL_PRINT(printf("Can't handle rows where both bounds >1.0e10 %d %g\n",
						       i, smaller));
                         nbad++;
                         if (rowupper[i] < 0.0 || rowlower[i] > 0.0)
                              abort();
                         if (fabs(rowupper[i]) > fabs(rowlower[i])) {
                              rowlower[i] = -0.9e10;
                              smaller = rowlower[i];
                              value = -1.0;
                         } else {
                              rowupper[i] = 0.9e10;
                              saveExtra[extraBlock] = rowupper[i];
                              smaller = rowupper[i];
                              value = 1.0;
                         }
                    }
                    difference = rowupper[i] - rowlower[i];
                    difference = CoinMin(difference, 1.0e31);
                    rowupper[i] = smaller;
                    elemExtra[extraBlock] = value;
                    solExtra[extraBlock] = (rowupper[i] - rowsol[i]) / value;
                    if (solExtra[extraBlock] < 0.0) solExtra[extraBlock] = 0.0;
                    if (solExtra[extraBlock] > difference) solExtra[extraBlock] = difference;
                    costExtra[extraBlock] = lambda[i] * value;
                    upperExtra[extraBlock] = difference;
                    rowsol[i] += value * solExtra[extraBlock];
                    rowExtra[extraBlock++] = i;
               }
          }
          if (nbad)
	    COIN_DETAIL_PRINT(printf("%d bad values - results may be wrong\n", nbad));
     }
     for (i = 0; i < nrows; i++) {
          offset += lambda[i] * rowsol[i];
     }
     if ((strategy & 256) != 0) {
          /* save best solution */
          saveSol = new double[ncols];
          CoinMemcpyN(colsol, ncols, saveSol);
          if (extraBlock) {
               useCostExtra = new double[extraBlock];
               memset(useCostExtra, 0, extraBlock * sizeof(double));
          }
          useCost = origcost;
          useOffset = saveOffset;
     } else {
          useCostExtra = costExtra;
          useCost = cost;
          useOffset = offset;
     }
     ncolx = ncols + extraBlock;
     for (i = 0; i < HISTORY + 1; i++) {
          history[i] = new double[ncolx];
     }
     for (i = 0; i < DJTEST; i++) {
          djSave[i] = 1.0e30;
     }
     for (i = 0; i < ncols; i++) {
          if (upper[i] - lower[i]) {
               statusSave[i] = 0;
          } else {
               statusSave[i] = 1;
          }
     }
     // for two pass method
     int start[2];
     int stop[2];
     int direction = -1;
     start[0] = 0;
     stop[0] = ncols;
     start[1] = 0;
     stop[1] = 0;
     iter = 0;
     for (; iter < maxIts; iter++) {
          double sum1 = 0.0, sum2 = 0.0;
          double lastObj = 1.0e70;
          int good = 0, doScale = 0;
          if (strategy & 16) {
               int ii = iter / EVERY + 1;
               ii = ii * EVERY;
               if (iter > ii - HISTORY * 2 && (iter & 1) == 0) {
                    double * COIN_RESTRICT x = history[HISTORY-1];
                    for (i = HISTORY - 1; i > 0; i--) {
                         history[i] = history[i-1];
                    }
                    history[0] = x;
                    CoinMemcpyN(colsol, ncols, history[0]);
                    CoinMemcpyN(solExtra, extraBlock, history[0] + ncols);
               }
          }
          if ((iter % SAVEHISTORY) == 0 || doFull) {
               if ((strategy & 16) == 0) {
                    double * COIN_RESTRICT x = history[HISTORY-1];
                    for (i = HISTORY - 1; i > 0; i--) {
                         history[i] = history[i-1];
                    }
                    history[0] = x;
                    CoinMemcpyN(colsol, ncols, history[0]);
                    CoinMemcpyN(solExtra, extraBlock, history[0] + ncols);
               }
          }
          /* start full try */
          if ((iter % EVERY) == 0 || doFull) {
               // for next pass
               direction = - direction;
               // randomize.
               // The cast is to avoid gcc compiler warning
               int kcol = static_cast<int>(ncols * randomNumberGenerator->randomDouble());
               if (kcol == ncols)
                    kcol = ncols - 1;
               if (direction > 0) {
                    start[0] = kcol;
                    stop[0] = ncols;
                    start[1] = 0;
                    stop[1] = kcol;
#ifdef FOUR_GOES
		    for (int itry=0;itry<2;itry++) {
		      int chunk=(stop[itry]-start[itry]+FOUR_GOES-1)/FOUR_GOES;
		      startsX[itry][0]=start[itry];
		      for (int i=1;i<5;i++)
			startsX[itry][i]=CoinMin(stop[itry],startsX[itry][i-1]+chunk);
		    }
#endif
               } else {
                    start[0] = kcol;
                    stop[0] = -1;
                    start[1] = ncols - 1;
                    stop[1] = kcol;
#ifdef FOUR_GOES
		    for (int itry=0;itry<2;itry++) {
		      int chunk=(start[itry]-stop[itry]+FOUR_GOES-1)/FOUR_GOES;
		      startsX[itry][0]=start[itry];
		      for (int i=1;i<5;i++)
			startsX[itry][i]=CoinMax(stop[itry],startsX[itry][i-1]-chunk);
		    }
#endif
               }
               int itry = 0;
               /*if ((strategy&16)==0) {
               	double * COIN_RESTRICT x=history[HISTORY-1];
               	for (i=HISTORY-1;i>0;i--) {
               	history[i]=history[i-1];
               	}
               	history[0]=x;
               	CoinMemcpyN(colsol,ncols,history[0]);
                 CoinMemcpyN(solExtra,extraBlock,history[0]+ncols);
               	}*/
               while (!good) {
                    itry++;
#define MAXTRY 5
                    if (iter > after && doScale < 2 && itry < MAXTRY) {
                         /* now full one */
                         for (i = 0; i < nrows; i++) {
                              rowsol[i] = -rowupper[i];
                         }
                         sum2 = 0.0;
                         objvalue = 0.0;
                         memset(pi, 0, nrows * sizeof(double));
                         {
                              double * COIN_RESTRICT theta = thetaX;
                              double * COIN_RESTRICT dj = djX;
                              double * COIN_RESTRICT b = bX;
                              double ** a = aX;
                              double ** awork = aworkX;
                              double * COIN_RESTRICT v = vX;
                              double c;
#ifdef FIT
                              int ntot = 0, nsign = 0, ngood = 0, mgood[4] = {0, 0, 0, 0};
                              double diff1, diff2, val0, val1, val2, newValue;
                              CoinMemcpyN(colsol, ncols, history[HISTORY-1]);
                              CoinMemcpyN(solExtra, extraBlock, history[HISTORY-1] + ncols);
#endif
                              dj[0] = 0.0;
                              for (i = 1; i < nsolve; i++) {
                                   dj[i] = 0.0;
                                   memset(allsum[i], 0, nrows * sizeof(double));
                              }
                              for (i = 0; i < ncols; i++) {
                                   double value2 = colsol[i];
                                   if (value2 > lower[i] + tolerance) {
                                        if(value2 < (upper[i] - tolerance)) {
                                             int k;
                                             objvalue += value2 * cost[i];
#ifdef FIT
                                             ntot++;
                                             val0 = history[0][i];
                                             val1 = history[1][i];
                                             val2 = history[2][i];
                                             diff1 = val0 - val1;
                                             diff2 = val1 - val2;
                                             if (diff1*diff2 >= 0.0) {
                                                  nsign++;
                                                  if (fabs(diff1) < fabs(diff2)) {
                                                       int ii = static_cast<int>(fabs(4.0 * diff1 / diff2));
                                                       if (ii == 4) ii = 3;
                                                       mgood[ii]++;
                                                       ngood++;
                                                  }
                                                  if (fabs(diff1) < 0.75 * fabs(diff2)) {
                                                       newValue = val1 + (diff1 * diff2) / (diff2 - diff1);
                                                  } else {
                                                       newValue = val1 + 4.0 * diff1;
                                                  }
                                             } else {
                                                  newValue = 0.333333333 * (val0 + val1 + val2);
                                             }
                                             if (newValue > upper[i] - tolerance) {
                                                  newValue = upper[i];
                                             } else if (newValue < lower[i] + tolerance) {
                                                  newValue = lower[i];
                                             }
                                             history[HISTORY-1][i] = newValue;
#endif
                                             for (k = 0; k < HISTORY - 1; k++) {
                                                  value = history[k][i] - history[k+1][i];
                                                  dj[k] += value * cost[i];
                                                  v[k] = value;
                                             }
                                             if (elemnt) {
                                                  for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                                                       int irow = row[j];
                                                       for (k = 0; k < HISTORY - 1; k++) {
                                                            allsum[k][irow] += elemnt[j] * v[k];
                                                       }
                                                       rowsol[irow] += elemnt[j] * value2;
                                                  }
                                             } else {
                                                  for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                                                       int irow = row[j];
                                                       for (k = 0; k < HISTORY - 1; k++) {
                                                            allsum[k][irow] += v[k];
                                                       }
                                                       rowsol[irow] += value2;
                                                  }
                                             }
                                        } else {
                                             /* at ub */
                                             colsol[i] = upper[i];
                                             value2 = colsol[i];
                                             objvalue += value2 * cost[i];
                                             if (elemnt) {
                                                  for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                                                       int irow = row[j];
                                                       rowsol[irow] += elemnt[j] * value2;
                                                  }
                                             } else {
                                                  for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                                                       int irow = row[j];
                                                       rowsol[irow] += value2;
                                                  }
                                             }
                                        }
                                   } else {
                                        /* at lb */
                                        if (value2) {
                                             objvalue += value2 * cost[i];
                                             if (elemnt) {
                                                  for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                                                       int irow = row[j];
                                                       rowsol[irow] += elemnt[j] * value2;
                                                  }
                                             } else {
                                                  for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                                                       int irow = row[j];
                                                       rowsol[irow] += value2;
                                                  }
                                             }
                                        }
                                   }
                              }
#ifdef FIT
                              /*printf("total %d, same sign %d, good %d %d %d %d %d\n",
                                ntot,nsign,ngood,mgood[0],mgood[1],mgood[2],mgood[3]);*/
#endif
                              if (extraBlock) {
                                   for (i = 0; i < extraBlock; i++) {
                                        double element = elemExtra[i];
                                        int irow = rowExtra[i];
                                        objvalue += solExtra[i] * costExtra[i];
                                        if (solExtra[i] > tolerance
                                                  && solExtra[i] < (upperExtra[i] - tolerance)) {
                                             double value2 = solExtra[i];
                                             int k;
                                             for (k = 0; k < HISTORY - 1; k++) {
                                                  value = history[k][i+ncols] - history[k+1][i+ncols];
                                                  dj[k] += value * costExtra[i];
                                                  allsum[k][irow] += element * value;
                                             }
                                             rowsol[irow] += element * value2;
                                        } else {
                                             double value2 = solExtra[i];
                                             double element = elemExtra[i];
                                             int irow = rowExtra[i];
                                             rowsol[irow] += element * value2;
                                        }
                                   }
                              }
#ifdef NULLVECTOR
                              if ((strategy & 64)) {
                                   double djVal = dj[0];
                                   for (i = 0; i < ncols - nrows; i++) {
                                        double value2 = colsol[i];
                                        if (value2 > lower[i] + tolerance && value2 < upper[i] - tolerance) {
                                             value = history[0][i] - history[1][i];
                                        } else {
                                             value = 0.0;
                                        }
                                        history[HISTORY][i] = value;
                                   }
                                   for (; i < ncols; i++) {
                                        double value2 = colsol[i];
                                        double delta;
                                        int irow = i - (ncols - nrows);
                                        double oldSum = allsum[0][irow];;
                                        if (value2 > lower[i] + tolerance && value2 < upper[i] - tolerance) {
                                             delta = history[0][i] - history[1][i];
                                        } else {
                                             delta = 0.0;
                                        }
                                        djVal -= delta * cost[i];
                                        oldSum -= delta;
                                        delta = - oldSum;
                                        djVal += delta * cost[i];
                                        history[HISTORY][i] = delta;
                                   }
                                   dj[HISTORY-1] = djVal;
                                   djVal = 0.0;
                                   for (i = 0; i < ncols; i++) {
                                        double value2 = colsol[i];
                                        if (value2 > lower[i] + tolerance && value2 < upper[i] - tolerance ||
                                                  i >= ncols - nrows) {
                                             int k;
                                             value = history[HISTORY][i];
                                             djVal += value * cost[i];
                                             for (j = columnStart[i]; j < columnStart[i] + length[i]; j++) {
                                                  int irow = row[j];
                                                  allsum[nsolve-1][irow] += value;
                                             }
                                        }
                                   }
                                   printf("djs %g %g\n", dj[HISTORY-1], djVal);
                              }
#endif
                              for (i = 0; i < nsolve; i++) {
                                   int j;
                                   b[i] = 0.0;
                                   for (j = 0; j < nsolve; j++) {
                                        a[i][j] = 0.0;
                                   }
                              }
                              c = 0.0;
                              for (i = 0; i < nrows; i++) {
                                   double value = rowsol[i];
                                   for (k = 0; k < nsolve; k++) {
                                        v[k] = allsum[k][i];
                                        b[k] += v[k] * value;
                                   }
                                   c += value * value;
                                   for (k = 0; k < nsolve; k++) {
                                        for (j = k; j < nsolve; j++) {
                                             a[k][j] += v[k] * v[j];
                                        }
                                   }
                              }
                              sum2 = c;
                              if (itry == 1) {
                                   lastObj = objvalue + weight * sum2;
                              }
                              for (k = 0; k < nsolve; k++) {
                                   b[k] = - (weight * b[k] + 0.5 * dj[k]);
                                   for (j = k; j < nsolve; j++) {
                                        a[k][j] *= weight;
                                        a[j][k] = a[k][j];
                                   }
                              }
                              c *= weight;
                              for (k = 0; k < nsolve; k++) {
                                   theta[k] = b[k];
                              }
                              solveSmall(nsolve, a, awork, theta);
                              if ((strategy & 64) != 0) {
                                   value = 10.0;
                                   for (k = 0; k < nsolve; k++) {
                                        value  = CoinMax(value, fabs(theta[k]));
                                   }
                                   if (value > 10.0 && ((logLevel_ & 4) != 0)) {
                                        printf("theta %g %g %g\n", theta[0], theta[1], theta[2]);
                                   }
                                   value = 10.0 / value;
                                   for (k = 0; k < nsolve; k++) {
                                        theta[k] *= value;
                                   }
                              }
                              for (i = 0; i < ncolx; i++) {
                                   double valueh = 0.0;
                                   for (k = 0; k < HISTORY - 1; k++) {
                                        value = history[k][i] - history[k+1][i];
                                        valueh += value * theta[k];
                                   }
#ifdef NULLVECTOR
                                   value = history[HISTORY][i];
                                   valueh += value * theta[HISTORY-1];
#endif
                                   history[HISTORY][i] = valueh;
                              }
                         }
#ifdef NULLVECTOR
                         if ((strategy & 64)) {
                              for (i = 0; i < ncols - nrows; i++) {
                                   if (colsol[i] <= lower[i] + tolerance
                                             || colsol[i] >= (upper[i] - tolerance)) {
                                        history[HISTORY][i] = 0.0;;
                                   }
                              }
                              tolerance = -tolerance; /* switch off test */
                         }
#endif
                         if (!doScale) {
                              for (i = 0; i < ncols; i++) {
                                   if (colsol[i] > lower[i] + tolerance
                                             && colsol[i] < (upper[i] - tolerance)) {
                                        value = history[HISTORY][i];
                                        colsol[i] += value;
                                        if (colsol[i] < lower[i] + tolerance) {
                                             colsol[i] = lower[i];
                                        } else if (colsol[i] > upper[i] - tolerance) {
                                             colsol[i] = upper[i];
                                        }
                                   }
                              }
                              if (extraBlock) {
                                   for (i = 0; i < extraBlock; i++) {
                                        if (solExtra[i] > tolerance
                                                  && solExtra[i] < (upperExtra[i] - tolerance)) {
                                             value = history[HISTORY][i+ncols];
                                             solExtra[i] += value;
                                             if (solExtra[i] < 0.0) {
                                                  solExtra[i] = 0.0;
                                             } else if (solExtra[i] > upperExtra[i]) {
                                                  solExtra[i] = upperExtra[i];
                                             }
                                        }
                                   }
                              }
                         } else {
                              double theta = 1.0;
                              double saveTheta = theta;
                              for (i = 0; i < ncols; i++) {
                                   if (colsol[i] > lower[i] + tolerance
                                             && colsol[i] < (upper[i] - tolerance)) {
                                        value = history[HISTORY][i];
                                        if (value > 0) {
                                             if (theta * value + colsol[i] > upper[i]) {
                                                  theta = (upper[i] - colsol[i]) / value;
                                             }
                                        } else if (value < 0) {
                                             if (colsol[i] + theta * value < lower[i]) {
                                                  theta = (lower[i] - colsol[i]) / value;
                                             }
                                        }
                                   }
                              }
                              if (extraBlock) {
                                   for (i = 0; i < extraBlock; i++) {
                                        if (solExtra[i] > tolerance
                                                  && solExtra[i] < (upperExtra[i] - tolerance)) {
                                             value = history[HISTORY][i+ncols];
                                             if (value > 0) {
                                                  if (theta * value + solExtra[i] > upperExtra[i]) {
                                                       theta = (upperExtra[i] - solExtra[i]) / value;
                                                  }
                                             } else if (value < 0) {
                                                  if (solExtra[i] + theta * value < 0.0) {
                                                       theta = -solExtra[i] / value;
                                                  }
                                             }
                                        }
                                   }
                              }
                              if ((iter % 100 == 0) && (logLevel_ & 8) != 0) {
                                   if (theta < saveTheta) {
                                        printf(" - modified theta %g\n", theta);
                                   }
                              }
                              for (i = 0; i < ncols; i++) {
                                   if (colsol[i] > lower[i] + tolerance
                                             && colsol[i] < (upper[i] - tolerance)) {
                                        value = history[HISTORY][i];
                                        colsol[i] += value * theta;
                                   }
                              }
                              if (extraBlock) {
                                   for (i = 0; i < extraBlock; i++) {
                                        if (solExtra[i] > tolerance
                                                  && solExtra[i] < (upperExtra[i] - tolerance)) {
                                             value = history[HISTORY][i+ncols];
                                             solExtra[i] += value * theta;
                                        }
                                   }
                              }
                         }
#ifdef NULLVECTOR
                         tolerance = fabs(tolerance); /* switch back on */
#endif
                         if ((iter % 100) == 0 && (logLevel_ & 8) != 0) {
                              printf("\n");
                         }
                    }
                    good = 1;
                    result = objval(nrows, ncols, rowsol, colsol, pi, djs, useCost,
                                    rowlower, rowupper, lower, upper,
                                    elemnt, row, columnStart, length, extraBlock, rowExtra,
                                    solExtra, elemExtra, upperExtra, useCostExtra,
                                    weight);
                    weightedObj = result.weighted;
                    if (!iter) saveValue = weightedObj;
                    objvalue = result.objval;
                    sum1 = result.infeas;
                    if (saveSol) {
                         if (result.weighted < bestSol) {
                              COIN_DETAIL_PRINT(printf("%d %g better than %g\n", iter,
						       result.weighted * maxmin - useOffset, bestSol * maxmin - useOffset));
                              bestSol = result.weighted;
                              CoinMemcpyN(colsol, ncols, saveSol);
                         }
                    }
#ifdef FITz
                    if (iter > after) {
                         IdiotResult result2;
                         double ww, oo, ss;
                         if (extraBlock) abort();
                         result2 = objval(nrows, ncols, row2, sol2, pi2, djs, cost,
                                          rowlower, rowupper, lower, upper,
                                          elemnt, row, columnStart, extraBlock, rowExtra,
                                          solExtra, elemExtra, upperExtra, costExtra,
                                          weight);
                         ww = result2.weighted;
                         oo = result2.objval;
                         ss = result2.infeas;
                         printf("wobj %g obj %g inf %g last %g\n", ww, oo, ss, lastObj);
                         if (ww < weightedObj && ww < lastObj) {
                              printf(" taken");
                              ntaken++;
                              saving += weightedObj - ww;
                              weightedObj = ww;
                              objvalue = oo;
                              sum1 = ss;
                              CoinMemcpyN(row2, nrows, rowsol);
                              CoinMemcpyN(pi2, nrows, pi);
                              CoinMemcpyN(sol2, ncols, colsol);
                              result = objval(nrows, ncols, rowsol, colsol, pi, djs, cost,
                                              rowlower, rowupper, lower, upper,
                                              elemnt, row, columnStart, extraBlock, rowExtra,
                                              solExtra, elemExtra, upperExtra, costExtra,
                                              weight);
                              weightedObj = result.weighted;
                              objvalue = result.objval;
                              sum1 = result.infeas;
                              if (ww < weightedObj) abort();
                         } else {
                              printf(" not taken");
                              nottaken++;
                         }
                    }
#endif
                    /*printf("%d %g %g %g %g\n",itry,lastObj,weightedObj,objvalue,sum1);*/
                    if (weightedObj > lastObj + 1.0e-4 && itry < MAXTRY) {
                         if((logLevel_ & 16) != 0 && doScale) {
                              printf("Weighted objective from %g to %g **** bad move\n",
                                     lastObj, weightedObj);
                         }
                         if (doScale) {
                              good = 1;
                         }
                         if ((strategy & 3) == 1) {
                              good = 0;
                              if (weightedObj > lastObj + djExit) {
                                   if ((logLevel_ & 16) != 0) {
                                        printf("Weighted objective from %g to %g ?\n", lastObj, weightedObj);
                                   }
                                   CoinMemcpyN(history[0], ncols, colsol);
                                   CoinMemcpyN(history[0] + ncols, extraBlock, solExtra);
                                   good = 1;
                              }
                         } else if ((strategy & 3) == 2) {
                              if (weightedObj > lastObj + 0.1 * maxDj) {
                                   CoinMemcpyN(history[0], ncols, colsol);
                                   CoinMemcpyN(history[0] + ncols, extraBlock, solExtra);
                                   doScale++;
                                   good = 0;
                              }
                         } else if ((strategy & 3) == 3) {
                              if (weightedObj > lastObj + 0.001 * maxDj) {
                                   /*doScale++;*/
                                   good = 0;
                              }
                         }
                    }
               }
               if ((iter % checkFrequency_) == 0) {
                    double best = weightedObj;
                    double test = obj[0];
                    for (i = 1; i < DROP; i++) {
                         obj[i-1] = obj[i];
                         if (best > obj[i]) best = obj[i];
                    }
                    obj[DROP-1] = best;
                    if (test - best < drop && (strategy & 8) == 0) {
                         if ((logLevel_ & 8) != 0) {
                              printf("Exiting as drop in %d its is %g after %d iterations\n",
                                     DROP * checkFrequency_, test - best, iter);
                         }
                         goto RETURN;
                    }
               }
               if ((iter % logFreq_) == 0) {
                    double piSum = 0.0;
                    for (i = 0; i < nrows; i++) {
                         piSum += (rowsol[i] + rowupper[i]) * pi[i];
                    }
                    if ((logLevel_ & 2) != 0) {
                         printf("%d Infeas %g, obj %g - wtObj %g dual %g maxDj %g\n",
                                iter, sum1, objvalue * maxmin - useOffset, weightedObj - useOffset,
                                piSum * maxmin - useOffset, maxDj);
                    }
               }
               CoinMemcpyN(statusSave, ncols, statusWork);
               nflagged = 0;
          }
          nChange = 0;
          doFull = 0;
          maxDj = 0.0;
          // go through forwards or backwards and starting at odd places
#ifdef FOUR_GOES
	  for (int i=1;i<FOUR_GOES;i++) {
	    cilk_spawn memcpy(piX[i], pi, nrows * sizeof(double));
	    cilk_spawn memcpy(rowsolX[i], rowsol, nrows * sizeof(double));
	  }
	  for (int i=0;i<FOUR_GOES;i++) {
	    nChangeX[i]=0;
	    maxDjX[i]=0.0;
	    objvalueX[i]=0.0;
	    nflaggedX[i]=0;
	  }
	  cilk_sync;
#endif
	  //printf("PASS\n");
#ifdef FOUR_GOES
               cilk_for (int iPar = 0; iPar < FOUR_GOES; iPar++) {
                    double * COIN_RESTRICT pi=piX[iPar];
                    double * COIN_RESTRICT rowsol = rowsolX[iPar];
          for (int itry = 0; itry < 2; itry++) {
		    int istop;
		    int istart;
#if 0
		    int chunk = (start[itry]+stop[itry]+FOUR_GOES-1)/FOUR_GOES;
                    if (iPar == 0) {
		      istart=start[itry];
		      istop=(start[itry]+stop[itry])>>1;
                    } else {
		      istart=(start[itry]+stop[itry])>>1;
		      istop = stop[itry];
                    }
#endif
#if 0
		    printf("istart %d istop %d direction %d array %d %d new %d %d\n",
		    	   istart,istop,direction,start[itry],stop[itry],
		    	   startsX[itry][iPar],startsX[itry][iPar+1]);
#endif
		    istart=startsX[itry][iPar];
		    istop=startsX[itry][iPar+1];
#else
          for (int itry = 0; itry < 2; itry++) {
               int istart = start[itry];
               int istop = stop[itry];
#endif
                    for (int icol=istart; icol != istop; icol += direction) {
                         if (!statusWork[icol]) {
                              CoinBigIndex j;
                              double value = colsol[icol];
                              double djval = cost[icol];
                              double djval2, value2;
                              double theta, a, b, c;
                              if (elemnt) {
                                   for (j = columnStart[icol]; j < columnStart[icol] + length[icol]; j++) {
                                        int irow = row[j];
                                        djval -= elemnt[j] * pi[irow];
                                   }
                              } else {
                                   for (j = columnStart[icol]; j < columnStart[icol] + length[icol]; j++) {
                                        int irow = row[j];
                                        djval -= pi[irow];
                                   }
                              }
                              /*printf("xx iter %d seq %d djval %g value %g\n",
                                iter,i,djval,value);*/
                              if (djval > 1.0e-5) {
                                   value2 = (lower[icol] - value);
                              } else {
                                   value2 = (upper[icol] - value);
                              }
                              djval2 = djval * value2;
                              djval = fabs(djval);
                              if (djval > djTol) {
                                   if (djval2 < -1.0e-4) {
#ifndef FOUR_GOES
                                        nChange++;
                                        if (djval > maxDj) maxDj = djval;
#else
                                        nChangeX[iPar]++;
                                        if (djval > maxDjX[iPar]) maxDjX[iPar] = djval;
#endif
                                        /*if (djval>3.55e6) {
                                        		printf("big\n");
                                        		}*/
                                        a = 0.0;
                                        b = 0.0;
                                        c = 0.0;
                                        djval2 = cost[icol];
                                        if (elemnt) {
                                             for (j = columnStart[icol]; j < columnStart[icol] + length[icol]; j++) {
                                                  int irow = row[j];
                                                  double value = rowsol[irow];
                                                  c += value * value;
                                                  a += elemnt[j] * elemnt[j];
                                                  b += value * elemnt[j];
                                             }
                                        } else {
                                             for (j = columnStart[icol]; j < columnStart[icol] + length[icol]; j++) {
                                                  int irow = row[j];
                                                  double value = rowsol[irow];
                                                  c += value * value;
                                                  a += 1.0;
                                                  b += value;
                                             }
                                        }
                                        a *= weight;
                                        b = b * weight + 0.5 * djval2;
                                        c *= weight;
                                        /* solve */
                                        theta = -b / a;
#ifndef FOUR_GOES
                                        if ((strategy & 4) != 0) {
					  double valuep, thetap;
                                             value2 = a * theta * theta + 2.0 * b * theta;
                                             thetap = 2.0 * theta;
                                             valuep = a * thetap * thetap + 2.0 * b * thetap;
                                             if (valuep < value2 + djTol) {
                                                  theta = thetap;
                                                  kgood++;
                                             } else {
                                                  kbad++;
                                             }
                                        }
#endif
                                        if (theta > 0.0) {
                                             if (theta < upper[icol] - colsol[icol]) {
                                                  value2 = theta;
                                             } else {
                                                  value2 = upper[icol] - colsol[icol];
                                             }
                                        } else {
                                             if (theta > lower[icol] - colsol[icol]) {
                                                  value2 = theta;
                                             } else {
                                                  value2 = lower[icol] - colsol[icol];
                                             }
                                        }
                                        colsol[icol] += value2;
#ifndef FOUR_GOES
                                        objvalue += cost[icol] * value2;
#else
                                        objvalueX[iPar] += cost[icol] * value2;
#endif
                                        if (elemnt) {
                                             for (j = columnStart[icol]; j < columnStart[icol] + length[icol]; j++) {
                                                  int irow = row[j];
                                                  double value;
                                                  rowsol[irow] += elemnt[j] * value2;
                                                  value = rowsol[irow];
                                                  pi[irow] = -2.0 * weight * value;
                                             }
                                        } else {
                                             for (j = columnStart[icol]; j < columnStart[icol] + length[icol]; j++) {
                                                  int irow = row[j];
                                                  double value;
                                                  rowsol[irow] += value2;
                                                  value = rowsol[irow];
                                                  pi[irow] = -2.0 * weight * value;
                                             }
                                        }
                                   } else {
                                        /* dj but at bound */
                                        if (djval > djFlag) {
                                             statusWork[icol] = 1;
#ifndef FOUR_GOES
                                             nflagged++;
#else
                                             nflaggedX[iPar]++;
#endif
                                        }
                                   }
                              }
                         }
                    }
#ifdef FOUR_GOES
               }
#endif
          }
#ifdef FOUR_GOES
	  for (int i=0;i<FOUR_GOES;i++) {
	    nChange += nChangeX[i];
	    maxDj = CoinMax(maxDj,maxDjX[i]);
	    objvalue += objvalueX[i];
	    nflagged += nflaggedX[i];
	  }
          cilk_for (int i = 0; i < nrows; i++) {
#if FOUR_GOES==2
	    rowsol[i] = 0.5 * (rowsolX[0][i] + rowsolX[1][i]);
	    pi[i] = 0.5 * (piX[0][i] + piX[1][i]);
#elif FOUR_GOES==3
	    pi[i] = 0.33333333333333 * (piX[0][i] + piX[1][i]+piX[2][i]);
	    rowsol[i] = 0.3333333333333 * (rowsolX[0][i] + rowsolX[1][i]+rowsolX[2][i]);
#else
	    pi[i] = 0.25 * (piX[0][i] + piX[1][i]+piX[2][i]+piX[3][i]);
	    rowsol[i] = 0.25 * (rowsolX[0][i] + rowsolX[1][i]+rowsolX[2][i]+rowsolX[3][i]);
#endif
          }
#endif
          if (extraBlock) {
               for (int i = 0; i < extraBlock; i++) {
                    double value = solExtra[i];
                    double djval = costExtra[i];
                    double djval2, value2;
                    double element = elemExtra[i];
                    double theta, a, b, c;
                    int irow = rowExtra[i];
                    djval -= element * pi[irow];
                    /*printf("xxx iter %d extra %d djval %g value %g\n",
                    	  iter,irow,djval,value);*/
                    if (djval > 1.0e-5) {
                         value2 = -value;
                    } else {
                         value2 = (upperExtra[i] - value);
                    }
                    djval2 = djval * value2;
                    if (djval2 < -1.0e-4 && fabs(djval) > djTol) {
                         nChange++;
                         a = 0.0;
                         b = 0.0;
                         c = 0.0;
                         djval2 = costExtra[i];
                         value = rowsol[irow];
                         c += value * value;
                         a += element * element;
                         b += element * value;
                         a *= weight;
                         b = b * weight + 0.5 * djval2;
                         c *= weight;
                         /* solve */
                         theta = -b / a;
                         if (theta > 0.0) {
                              value2 = CoinMin(theta, upperExtra[i] - solExtra[i]);
                         } else {
                              value2 = CoinMax(theta, -solExtra[i]);
                         }
                         solExtra[i] += value2;
                         rowsol[irow] += element * value2;
                         value = rowsol[irow];
                         pi[irow] = -2.0 * weight * value;
                    }
               }
          }
          if ((iter % 10) == 2) {
               for (int i = DJTEST - 1; i > 0; i--) {
                    djSave[i] = djSave[i-1];
               }
               djSave[0] = maxDj;
               largestDj = CoinMax(largestDj, maxDj);
               smallestDj = CoinMin(smallestDj, maxDj);
               for (int i = DJTEST - 1; i > 0; i--) {
                    maxDj += djSave[i];
               }
               maxDj = maxDj / static_cast<double> (DJTEST);
               if (maxDj < djExit && iter > 50) {
                    //printf("Exiting on low dj %g after %d iterations\n",maxDj,iter);
                    break;
               }
               if (nChange < 100) {
                    djTol *= 0.5;
               }
          }
     }
RETURN:
     if (kgood || kbad) {
       COIN_DETAIL_PRINT(printf("%g good %g bad\n", kgood, kbad));
     }
     result = objval(nrows, ncols, rowsol, colsol, pi, djs, useCost,
                     rowlower, rowupper, lower, upper,
                     elemnt, row, columnStart, length, extraBlock, rowExtra,
                     solExtra, elemExtra, upperExtra, useCostExtra,
                     weight);
     result.djAtBeginning = largestDj;
     result.djAtEnd = smallestDj;
     result.dropThis = saveValue - result.weighted;
     if (saveSol) {
          if (result.weighted < bestSol) {
               bestSol = result.weighted;
               CoinMemcpyN(colsol, ncols, saveSol);
          } else {
               COIN_DETAIL_PRINT(printf("restoring previous - now %g best %g\n",
					result.weighted * maxmin - useOffset, bestSol * maxmin - useOffset));
          }
     }
     if (saveSol) {
          if (extraBlock) {
               delete [] useCostExtra;
          }
          CoinMemcpyN(saveSol, ncols, colsol);
          delete [] saveSol;
     }
     for (i = 0; i < nsolve; i++) {
          if (i) delete [] allsum[i];
          delete [] aX[i];
          delete [] aworkX[i];
     }
     delete [] thetaX;
     delete [] djX;
     delete [] bX;
     delete [] vX;
     delete [] aX;
     delete [] aworkX;
     delete [] allsum;
     delete [] cost;
#ifdef FOUR_GOES
     delete [] pi2 ;
     delete [] rowsol2 ;
#endif
     for (i = 0; i < HISTORY + 1; i++) {
          delete [] history[i];
     }
     delete [] statusSave;
     /* do original costs objvalue*/
     result.objval = 0.0;
     for (i = 0; i < ncols; i++) {
          result.objval += colsol[i] * origcost[i];
     }
     if (extraBlock) {
          for (i = 0; i < extraBlock; i++) {
               int irow = rowExtra[i];
               rowupper[irow] = saveExtra[i];
          }
          delete [] rowExtra;
          delete [] solExtra;
          delete [] elemExtra;
          delete [] upperExtra;
          delete [] costExtra;
          delete [] saveExtra;
     }
     result.iteration = iter;
     result.objval -= saveOffset;
     result.weighted = result.objval + weight * result.sumSquared;
     return result;
}
