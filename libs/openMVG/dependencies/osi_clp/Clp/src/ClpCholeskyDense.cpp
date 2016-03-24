/* $Id$ */
/*
  Copyright (C) 2003, International Business Machines Corporation
  and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/
#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpConfig.h"
#include "ClpHelperFunctions.hpp"
#include "ClpInterior.hpp"
#include "ClpCholeskyDense.hpp"
#include "ClpMessage.hpp"
#include "ClpQuadraticObjective.hpp"
#if CLP_HAS_ABC
#include "CoinAbcCommon.hpp"
#endif
#if CLP_HAS_ABC<3
#undef cilk_for 
#undef cilk_spawn
#undef cilk_sync
#define cilk_for for 
#define cilk_spawn
#define cilk_sync
#endif

/*#############################################################################*/
/* Constructors / Destructor / Assignment*/
/*#############################################################################*/

/*-------------------------------------------------------------------*/
/* Default Constructor */
/*-------------------------------------------------------------------*/
ClpCholeskyDense::ClpCholeskyDense ()
     : ClpCholeskyBase(),
       borrowSpace_(false)
{
     type_ = 11;;
}

/*-------------------------------------------------------------------*/
/* Copy constructor */
/*-------------------------------------------------------------------*/
ClpCholeskyDense::ClpCholeskyDense (const ClpCholeskyDense & rhs)
     : ClpCholeskyBase(rhs),
       borrowSpace_(rhs.borrowSpace_)
{
     assert(!rhs.borrowSpace_ || !rhs.sizeFactor_); /* can't do if borrowing space*/
}


/*-------------------------------------------------------------------*/
/* Destructor */
/*-------------------------------------------------------------------*/
ClpCholeskyDense::~ClpCholeskyDense ()
{
     if (borrowSpace_) {
          /* set NULL*/
          sparseFactor_ = NULL;
          workDouble_ = NULL;
          diagonal_ = NULL;
     }
}

/*----------------------------------------------------------------*/
/* Assignment operator */
/*-------------------------------------------------------------------*/
ClpCholeskyDense &
ClpCholeskyDense::operator=(const ClpCholeskyDense& rhs)
{
     if (this != &rhs) {
          assert(!rhs.borrowSpace_ || !rhs.sizeFactor_); /* can't do if borrowing space*/
          ClpCholeskyBase::operator=(rhs);
          borrowSpace_ = rhs.borrowSpace_;
     }
     return *this;
}
/*-------------------------------------------------------------------*/
/* Clone*/
/*-------------------------------------------------------------------*/
ClpCholeskyBase * ClpCholeskyDense::clone() const
{
     return new ClpCholeskyDense(*this);
}
/* If not power of 2 then need to redo a bit*/
#define BLOCK 16
#define BLOCKSHIFT 4
/* Block unroll if power of 2 and at least 8*/
#define BLOCKUNROLL

#define BLOCKSQ ( BLOCK*BLOCK )
#define BLOCKSQSHIFT ( BLOCKSHIFT+BLOCKSHIFT )
#define number_blocks(x) (((x)+BLOCK-1)>>BLOCKSHIFT)
#define number_rows(x) ((x)<<BLOCKSHIFT)
#define number_entries(x) ((x)<<BLOCKSQSHIFT)
/* Gets space */
int
ClpCholeskyDense::reserveSpace(const ClpCholeskyBase * factor, int numberRows)
{
     numberRows_ = numberRows;
     int numberBlocks = (numberRows_ + BLOCK - 1) >> BLOCKSHIFT;
     /* allow one stripe extra*/
     numberBlocks = numberBlocks + ((numberBlocks * (numberBlocks + 1)) / 2);
     sizeFactor_ = numberBlocks * BLOCKSQ;
     /*#define CHOL_COMPARE*/
#ifdef CHOL_COMPARE
     sizeFactor_ += 95000;
#endif
     if (!factor) {
          sparseFactor_ = new longDouble [sizeFactor_];
          rowsDropped_ = new char [numberRows_];
          memset(rowsDropped_, 0, numberRows_);
          workDouble_ = new longDouble[numberRows_];
          diagonal_ = new longDouble[numberRows_];
     } else {
          borrowSpace_ = true;
          int numberFull = factor->numberRows();
          sparseFactor_ = factor->sparseFactor() + (factor->size() - sizeFactor_);
          workDouble_ = factor->workDouble() + (numberFull - numberRows_);
          diagonal_ = factor->diagonal() + (numberFull - numberRows_);
     }
     numberRowsDropped_ = 0;
     return 0;
}
/* Returns space needed */
CoinBigIndex
ClpCholeskyDense::space( int numberRows) const
{
     int numberBlocks = (numberRows + BLOCK - 1) >> BLOCKSHIFT;
     /* allow one stripe extra*/
     numberBlocks = numberBlocks + ((numberBlocks * (numberBlocks + 1)) / 2);
     CoinBigIndex sizeFactor = numberBlocks * BLOCKSQ;
#ifdef CHOL_COMPARE
     sizeFactor += 95000;
#endif
     return sizeFactor;
}
/* Orders rows and saves pointer to matrix.and model */
int
ClpCholeskyDense::order(ClpInterior * model)
{
     model_ = model;
     int numberRows;
     int numberRowsModel = model_->numberRows();
     int numberColumns = model_->numberColumns();
     if (!doKKT_) {
          numberRows = numberRowsModel;
     } else {
          numberRows = 2 * numberRowsModel + numberColumns;
     }
     reserveSpace(NULL, numberRows);
     rowCopy_ = model->clpMatrix()->reverseOrderedCopy();
     return 0;
}
/* Does Symbolic factorization given permutation.
   This is called immediately after order.  If user provides this then
   user must provide factorize and solve.  Otherwise the default factorization is used
   returns non-zero if not enough memory */
int
ClpCholeskyDense::symbolic()
{
     return 0;
}
/* Factorize - filling in rowsDropped and returning number dropped */
int
ClpCholeskyDense::factorize(const CoinWorkDouble * diagonal, int * rowsDropped)
{
     assert (!borrowSpace_);
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const double * element = model_->clpMatrix()->getElements();
     const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
     const int * rowLength = rowCopy_->getVectorLengths();
     const int * column = rowCopy_->getIndices();
     const double * elementByRow = rowCopy_->getElements();
     int numberColumns = model_->clpMatrix()->getNumCols();
     CoinZeroN(sparseFactor_, sizeFactor_);
     /*perturbation*/
     CoinWorkDouble perturbation = model_->diagonalPerturbation() * model_->diagonalNorm();
     perturbation = perturbation * perturbation;
     if (perturbation > 1.0) {
#ifdef COIN_DEVELOP
          /*if (model_->model()->logLevel()&4) */
          std::cout << "large perturbation " << perturbation << std::endl;
#endif
          perturbation = CoinSqrt(perturbation);;
          perturbation = 1.0;
     }
     int iRow;
     int newDropped = 0;
     CoinWorkDouble largest = 1.0;
     CoinWorkDouble smallest = COIN_DBL_MAX;
     CoinWorkDouble delta2 = model_->delta(); /* add delta*delta to diagonal*/
     delta2 *= delta2;
     if (!doKKT_) {
          longDouble * work = sparseFactor_;
          work--; /* skip diagonal*/
          int addOffset = numberRows_ - 1;
          const CoinWorkDouble * diagonalSlack = diagonal + numberColumns;
          /* largest in initial matrix*/
          CoinWorkDouble largest2 = 1.0e-20;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               if (!rowsDropped_[iRow]) {
                    CoinBigIndex startRow = rowStart[iRow];
                    CoinBigIndex endRow = rowStart[iRow] + rowLength[iRow];
                    CoinWorkDouble diagonalValue = diagonalSlack[iRow] + delta2;
                    for (CoinBigIndex k = startRow; k < endRow; k++) {
                         int iColumn = column[k];
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         CoinWorkDouble multiplier = diagonal[iColumn] * elementByRow[k];
                         for (CoinBigIndex j = start; j < end; j++) {
                              int jRow = row[j];
                              if (!rowsDropped_[jRow]) {
                                   if (jRow > iRow) {
                                        CoinWorkDouble value = element[j] * multiplier;
                                        work[jRow] += value;
                                   } else if (jRow == iRow) {
                                        CoinWorkDouble value = element[j] * multiplier;
                                        diagonalValue += value;
                                   }
                              }
                         }
                    }
                    for (int j = iRow + 1; j < numberRows_; j++)
                         largest2 = CoinMax(largest2, CoinAbs(work[j]));
                    diagonal_[iRow] = diagonalValue;
                    largest2 = CoinMax(largest2, CoinAbs(diagonalValue));
               } else {
                    /* dropped*/
                    diagonal_[iRow] = 1.0;
               }
               addOffset--;
               work += addOffset;
          }
          /*check sizes*/
          largest2 *= 1.0e-20;
          largest = CoinMin(largest2, CHOL_SMALL_VALUE);
          int numberDroppedBefore = 0;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int dropped = rowsDropped_[iRow];
               /* Move to int array*/
               rowsDropped[iRow] = dropped;
               if (!dropped) {
                    CoinWorkDouble diagonal = diagonal_[iRow];
                    if (diagonal > largest2) {
                         diagonal_[iRow] = diagonal + perturbation;
                    } else {
                         diagonal_[iRow] = diagonal + perturbation;
                         rowsDropped[iRow] = 2;
                         numberDroppedBefore++;
                    }
               }
          }
          doubleParameters_[10] = CoinMax(1.0e-20, largest);
          integerParameters_[20] = 0;
          doubleParameters_[3] = 0.0;
          doubleParameters_[4] = COIN_DBL_MAX;
          integerParameters_[34] = 0; /* say all must be positive*/
#ifdef CHOL_COMPARE
          if (numberRows_ < 200)
               factorizePart3(rowsDropped);
#endif
          factorizePart2(rowsDropped);
          newDropped = integerParameters_[20] + numberDroppedBefore;
          largest = doubleParameters_[3];
          smallest = doubleParameters_[4];
          if (model_->messageHandler()->logLevel() > 1)
               std::cout << "Cholesky - largest " << largest << " smallest " << smallest << std::endl;
          choleskyCondition_ = largest / smallest;
          /*drop fresh makes some formADAT easier*/
          if (newDropped || numberRowsDropped_) {
               newDropped = 0;
               for (int i = 0; i < numberRows_; i++) {
                    char dropped = static_cast<char>(rowsDropped[i]);
                    rowsDropped_[i] = dropped;
                    if (dropped == 2) {
                         /*dropped this time*/
                         rowsDropped[newDropped++] = i;
                         rowsDropped_[i] = 0;
                    }
               }
               numberRowsDropped_ = newDropped;
               newDropped = -(2 + newDropped);
          }
     } else {
          /* KKT*/
          CoinPackedMatrix * quadratic = NULL;
          ClpQuadraticObjective * quadraticObj =
               (dynamic_cast< ClpQuadraticObjective*>(model_->objectiveAsObject()));
          if (quadraticObj)
               quadratic = quadraticObj->quadraticObjective();
          /* matrix*/
          int numberRowsModel = model_->numberRows();
          int numberColumns = model_->numberColumns();
          int numberTotal = numberColumns + numberRowsModel;
          longDouble * work = sparseFactor_;
          work--; /* skip diagonal*/
          int addOffset = numberRows_ - 1;
          int iColumn;
          if (!quadratic) {
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    CoinWorkDouble value = diagonal[iColumn];
                    if (CoinAbs(value) > 1.0e-100) {
                         value = 1.0 / value;
                         largest = CoinMax(largest, CoinAbs(value));
                         diagonal_[iColumn] = -value;
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         for (CoinBigIndex j = start; j < end; j++) {
                              /*choleskyRow_[numberElements]=row[j]+numberTotal;*/
                              /*sparseFactor_[numberElements++]=element[j];*/
                              work[row[j] + numberTotal] = element[j];
                              largest = CoinMax(largest, CoinAbs(element[j]));
                         }
                    } else {
                         diagonal_[iColumn] = -value;
                    }
                    addOffset--;
                    work += addOffset;
               }
          } else {
               /* Quadratic*/
               const int * columnQuadratic = quadratic->getIndices();
               const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
               const int * columnQuadraticLength = quadratic->getVectorLengths();
               const double * quadraticElement = quadratic->getElements();
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    CoinWorkDouble value = diagonal[iColumn];
                    CoinBigIndex j;
                    if (CoinAbs(value) > 1.0e-100) {
                         value = 1.0 / value;
                         for (j = columnQuadraticStart[iColumn];
                                   j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                              int jColumn = columnQuadratic[j];
                              if (jColumn > iColumn) {
                                   work[jColumn] = -quadraticElement[j];
                              } else if (iColumn == jColumn) {
                                   value += quadraticElement[j];
                              }
                         }
                         largest = CoinMax(largest, CoinAbs(value));
                         diagonal_[iColumn] = -value;
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         for (j = start; j < end; j++) {
                              work[row[j] + numberTotal] = element[j];
                              largest = CoinMax(largest, CoinAbs(element[j]));
                         }
                    } else {
                         value = 1.0e100;
                         diagonal_[iColumn] = -value;
                    }
                    addOffset--;
                    work += addOffset;
               }
          }
          /* slacks*/
          for (iColumn = numberColumns; iColumn < numberTotal; iColumn++) {
               CoinWorkDouble value = diagonal[iColumn];
               if (CoinAbs(value) > 1.0e-100) {
                    value = 1.0 / value;
                    largest = CoinMax(largest, CoinAbs(value));
               } else {
                    value = 1.0e100;
               }
               diagonal_[iColumn] = -value;
               work[iColumn-numberColumns+numberTotal] = -1.0;
               addOffset--;
               work += addOffset;
          }
          /* Finish diagonal*/
          for (iRow = 0; iRow < numberRowsModel; iRow++) {
               diagonal_[iRow+numberTotal] = delta2;
          }
          /*check sizes*/
          largest *= 1.0e-20;
          largest = CoinMin(largest, CHOL_SMALL_VALUE);
          doubleParameters_[10] = CoinMax(1.0e-20, largest);
          integerParameters_[20] = 0;
          doubleParameters_[3] = 0.0;
          doubleParameters_[4] = COIN_DBL_MAX;
          /* Set up LDL cutoff*/
          integerParameters_[34] = numberTotal;
          /* KKT*/
          int * rowsDropped2 = new int[numberRows_];
          CoinZeroN(rowsDropped2, numberRows_);
#ifdef CHOL_COMPARE
          if (numberRows_ < 200)
               factorizePart3(rowsDropped2);
#endif
          factorizePart2(rowsDropped2);
          newDropped = integerParameters_[20];
          largest = doubleParameters_[3];
          smallest = doubleParameters_[4];
          if (model_->messageHandler()->logLevel() > 1)
	    COIN_DETAIL_PRINT(std::cout << "Cholesky - largest " << largest << " smallest " << smallest << std::endl);
          choleskyCondition_ = largest / smallest;
          /* Should save adjustments in ..R_*/
          int n1 = 0, n2 = 0;
          CoinWorkDouble * primalR = model_->primalR();
          CoinWorkDouble * dualR = model_->dualR();
          for (iRow = 0; iRow < numberTotal; iRow++) {
               if (rowsDropped2[iRow]) {
                    n1++;
                    /*printf("row region1 %d dropped\n",iRow);*/
                    /*rowsDropped_[iRow]=1;*/
                    rowsDropped_[iRow] = 0;
                    primalR[iRow] = doubleParameters_[20];
               } else {
                    rowsDropped_[iRow] = 0;
                    primalR[iRow] = 0.0;
               }
          }
          for (; iRow < numberRows_; iRow++) {
               if (rowsDropped2[iRow]) {
                    n2++;
                    /*printf("row region2 %d dropped\n",iRow);*/
                    /*rowsDropped_[iRow]=1;*/
                    rowsDropped_[iRow] = 0;
                    dualR[iRow-numberTotal] = doubleParameters_[34];
               } else {
                    rowsDropped_[iRow] = 0;
                    dualR[iRow-numberTotal] = 0.0;
               }
          }
     }
     return 0;
}
/* Factorize - filling in rowsDropped and returning number dropped */
void
ClpCholeskyDense::factorizePart3( int * rowsDropped)
{
     int iColumn;
     longDouble * xx = sparseFactor_;
     longDouble * yy = diagonal_;
     diagonal_ = sparseFactor_ + 40000;
     sparseFactor_ = diagonal_ + numberRows_;
     /*memcpy(sparseFactor_,xx,sizeFactor_*sizeof(double));*/
     CoinMemcpyN(xx, 40000, sparseFactor_);
     CoinMemcpyN(yy, numberRows_, diagonal_);
     int numberDropped = 0;
     CoinWorkDouble largest = 0.0;
     CoinWorkDouble smallest = COIN_DBL_MAX;
     double dropValue = doubleParameters_[10];
     int firstPositive = integerParameters_[34];
     longDouble * work = sparseFactor_;
     /* Allow for triangular*/
     int addOffset = numberRows_ - 1;
     work--;
     for (iColumn = 0; iColumn < numberRows_; iColumn++) {
          int iRow;
          int addOffsetNow = numberRows_ - 1;;
          longDouble * workNow = sparseFactor_ - 1 + iColumn;
          CoinWorkDouble diagonalValue = diagonal_[iColumn];
          for (iRow = 0; iRow < iColumn; iRow++) {
               double aj = *workNow;
               addOffsetNow--;
               workNow += addOffsetNow;
               diagonalValue -= aj * aj * workDouble_[iRow];
          }
          bool dropColumn = false;
          if (iColumn < firstPositive) {
               /* must be negative*/
               if (diagonalValue <= -dropValue) {
                    smallest = CoinMin(smallest, -diagonalValue);
                    largest = CoinMax(largest, -diagonalValue);
                    workDouble_[iColumn] = diagonalValue;
                    diagonalValue = 1.0 / diagonalValue;
               } else {
                    dropColumn = true;
                    workDouble_[iColumn] = -1.0e100;
                    diagonalValue = 0.0;
                    integerParameters_[20]++;
               }
          } else {
               /* must be positive*/
               if (diagonalValue >= dropValue) {
                    smallest = CoinMin(smallest, diagonalValue);
                    largest = CoinMax(largest, diagonalValue);
                    workDouble_[iColumn] = diagonalValue;
                    diagonalValue = 1.0 / diagonalValue;
               } else {
                    dropColumn = true;
                    workDouble_[iColumn] = 1.0e100;
                    diagonalValue = 0.0;
                    integerParameters_[20]++;
               }
          }
          if (!dropColumn) {
               diagonal_[iColumn] = diagonalValue;
               for (iRow = iColumn + 1; iRow < numberRows_; iRow++) {
                    double value = work[iRow];
                    workNow = sparseFactor_ - 1;
                    int addOffsetNow = numberRows_ - 1;;
                    for (int jColumn = 0; jColumn < iColumn; jColumn++) {
                         double aj = workNow[iColumn];
                         double multiplier = workDouble_[jColumn];
                         double ai = workNow[iRow];
                         addOffsetNow--;
                         workNow += addOffsetNow;
                         value -= aj * ai * multiplier;
                    }
                    work[iRow] = value * diagonalValue;
               }
          } else {
               /* drop column*/
               rowsDropped[iColumn] = 2;
               numberDropped++;
               diagonal_[iColumn] = 0.0;
               for (iRow = iColumn + 1; iRow < numberRows_; iRow++) {
                    work[iRow] = 0.0;
               }
          }
          addOffset--;
          work += addOffset;
     }
     doubleParameters_[3] = largest;
     doubleParameters_[4] = smallest;
     integerParameters_[20] = numberDropped;
     sparseFactor_ = xx;
     diagonal_ = yy;
}
/*#define POS_DEBUG*/
#ifdef POS_DEBUG
static int counter = 0;
int ClpCholeskyDense::bNumber(const longDouble * array, int &iRow, int &iCol)
{
     int numberBlocks = (numberRows_ + BLOCK - 1) >> BLOCKSHIFT;
     longDouble * a = sparseFactor_ + BLOCKSQ * numberBlocks;
     int k = array - a;
     assert ((k % BLOCKSQ) == 0);
     iCol = 0;
     int kk = k >> BLOCKSQSHIFT;
     /*printf("%d %d %d %d\n",k,kk,BLOCKSQ,BLOCKSQSHIFT);*/
     k = kk;
     while (k >= numberBlocks) {
          iCol++;
          k -= numberBlocks;
          numberBlocks--;
     }
     iRow = iCol + k;
     counter++;
     if(counter > 100000)
          exit(77);
     return kk;
}
#endif
/* Factorize - filling in rowsDropped and returning number dropped */
void
ClpCholeskyDense::factorizePart2( int * rowsDropped)
{
     int iColumn;
     /*longDouble * xx = sparseFactor_;*/
     /*longDouble * yy = diagonal_;*/
     /*diagonal_ = sparseFactor_ + 40000;*/
     /*sparseFactor_=diagonal_ + numberRows_;*/
     /*memcpy(sparseFactor_,xx,sizeFactor_*sizeof(double));*/
     /*memcpy(sparseFactor_,xx,40000*sizeof(double));*/
     /*memcpy(diagonal_,yy,numberRows_*sizeof(double));*/
     int numberBlocks = (numberRows_ + BLOCK - 1) >> BLOCKSHIFT;
     /* later align on boundary*/
     longDouble * a = sparseFactor_ + BLOCKSQ * numberBlocks;
     int n = numberRows_;
     int nRound = numberRows_ & (~(BLOCK - 1));
     /* adjust if exact*/
     if (nRound == n)
          nRound -= BLOCK;
     int sizeLastBlock = n - nRound;
     int get = n * (n - 1) / 2; /* ? as no diagonal*/
     int block = numberBlocks * (numberBlocks + 1) / 2;
     int ifOdd;
     int rowLast;
     if (sizeLastBlock != BLOCK) {
          longDouble * aa = &a[(block-1)*BLOCKSQ];
          rowLast = nRound - 1;
          ifOdd = 1;
          int put = BLOCKSQ;
          /* do last separately*/
          put -= (BLOCK - sizeLastBlock) * (BLOCK + 1);
          for (iColumn = numberRows_ - 1; iColumn >= nRound; iColumn--) {
               int put2 = put;
               put -= BLOCK;
               for (int iRow = numberRows_ - 1; iRow > iColumn; iRow--) {
                    aa[--put2] = sparseFactor_[--get];
                    assert (aa + put2 >= sparseFactor_ + get);
               }
               /* save diagonal as well*/
               aa[--put2] = diagonal_[iColumn];
          }
          n = nRound;
          block--;
     } else {
          /* exact fit*/
          rowLast = numberRows_ - 1;
          ifOdd = 0;
     }
     /* Now main loop*/
     int nBlock = 0;
     for (; n > 0; n -= BLOCK) {
          longDouble * aa = &a[(block-1)*BLOCKSQ];
          longDouble * aaLast = NULL;
          int put = BLOCKSQ;
          int putLast = 0;
          /* see if we have small block*/
          if (ifOdd) {
               aaLast = &a[(block-1)*BLOCKSQ];
               aa = aaLast - BLOCKSQ;
               putLast = BLOCKSQ - BLOCK + sizeLastBlock;
          }
          for (iColumn = n - 1; iColumn >= n - BLOCK; iColumn--) {
               if (aaLast) {
                    /* last bit*/
                    for (int iRow = numberRows_ - 1; iRow > rowLast; iRow--) {
                         aaLast[--putLast] = sparseFactor_[--get];
                         assert (aaLast + putLast >= sparseFactor_ + get);
                    }
                    putLast -= BLOCK - sizeLastBlock;
               }
               longDouble * aPut = aa;
               int j = rowLast;
               for (int jBlock = 0; jBlock <= nBlock; jBlock++) {
                    int put2 = put;
                    int last = CoinMax(j - BLOCK, iColumn);
                    for (int iRow = j; iRow > last; iRow--) {
                         aPut[--put2] = sparseFactor_[--get];
                         assert (aPut + put2 >= sparseFactor_ + get);
                    }
                    if (j - BLOCK < iColumn) {
                         /* save diagonal as well*/
                         aPut[--put2] = diagonal_[iColumn];
                    }
                    j -= BLOCK;
                    aPut -= BLOCKSQ;
               }
               put -= BLOCK;
          }
          nBlock++;
          block -= nBlock + ifOdd;
     }
     ClpCholeskyDenseC  info;
     info.diagonal_ = diagonal_;
     info.doubleParameters_[0] = doubleParameters_[10];
     info.integerParameters_[0] = integerParameters_[34];
#ifndef CLP_CILK
     ClpCholeskyCfactor(&info, a, numberRows_, numberBlocks,
                        diagonal_, workDouble_, rowsDropped);
#else
     info.a = a;
     info.n = numberRows_;
     info.numberBlocks = numberBlocks;
     info.work = workDouble_;
     info.rowsDropped = rowsDropped;
     info.integerParameters_[1] = model_->numberThreads();
     ClpCholeskySpawn(&info);
#endif
     double largest = 0.0;
     double smallest = COIN_DBL_MAX;
     int numberDropped = 0;
     for (int i = 0; i < numberRows_; i++) {
          if (diagonal_[i]) {
               largest = CoinMax(largest, CoinAbs(diagonal_[i]));
               smallest = CoinMin(smallest, CoinAbs(diagonal_[i]));
          } else {
               numberDropped++;
          }
     }
     doubleParameters_[3] = CoinMax(doubleParameters_[3], 1.0 / smallest);
     doubleParameters_[4] = CoinMin(doubleParameters_[4], 1.0 / largest);
     integerParameters_[20] += numberDropped;
}
/* Non leaf recursive factor*/
void
ClpCholeskyCfactor(ClpCholeskyDenseC * thisStruct, longDouble * a, int n, int numberBlocks,
                   longDouble * diagonal, longDouble * work, int * rowsDropped)
{
     if (n <= BLOCK) {
          ClpCholeskyCfactorLeaf(thisStruct, a, n, diagonal, work, rowsDropped);
     } else {
          int nb = number_blocks((n + 1) >> 1);
          int nThis = number_rows(nb);
          longDouble * aother;
          int nLeft = n - nThis;
          int nintri = (nb * (nb + 1)) >> 1;
          int nbelow = (numberBlocks - nb) * nb;
          ClpCholeskyCfactor(thisStruct, a, nThis, numberBlocks, diagonal, work, rowsDropped);
          ClpCholeskyCtriRec(thisStruct, a, nThis, a + number_entries(nb), diagonal, work, nLeft, nb, 0, numberBlocks);
          aother = a + number_entries(nintri + nbelow);
          ClpCholeskyCrecTri(thisStruct, a + number_entries(nb), nLeft, nThis, nb, 0, aother, diagonal, work, numberBlocks);
          ClpCholeskyCfactor(thisStruct, aother, nLeft,
                             numberBlocks - nb, diagonal + nThis, work + nThis, rowsDropped);
     }
}
/* Non leaf recursive triangle rectangle update*/
void
ClpCholeskyCtriRec(ClpCholeskyDenseC * thisStruct, longDouble * aTri, int nThis, longDouble * aUnder,
                   longDouble * diagonal, longDouble * work,
                   int nLeft, int iBlock, int jBlock,
                   int numberBlocks)
{
     if (nThis <= BLOCK && nLeft <= BLOCK) {
          ClpCholeskyCtriRecLeaf(/*thisStruct,*/ aTri, aUnder, diagonal, work, nLeft);
     } else if (nThis < nLeft) {
          int nb = number_blocks((nLeft + 1) >> 1);
          int nLeft2 = number_rows(nb);
          cilk_spawn ClpCholeskyCtriRec(thisStruct, aTri, nThis, aUnder, diagonal, work, nLeft2, iBlock, jBlock, numberBlocks);
          ClpCholeskyCtriRec(thisStruct, aTri, nThis, aUnder + number_entries(nb), diagonal, work, nLeft - nLeft2,
                             iBlock + nb, jBlock, numberBlocks);
	  cilk_sync;
     } else {
          int nb = number_blocks((nThis + 1) >> 1);
          int nThis2 = number_rows(nb);
          longDouble * aother;
          int kBlock = jBlock + nb;
          int i;
          int nintri = (nb * (nb + 1)) >> 1;
          int nbelow = (numberBlocks - nb) * nb;
          ClpCholeskyCtriRec(thisStruct, aTri, nThis2, aUnder, diagonal, work, nLeft, iBlock, jBlock, numberBlocks);
          /* and rectangular update */
          i = ((numberBlocks - jBlock) * (numberBlocks - jBlock - 1) -
               (numberBlocks - jBlock - nb) * (numberBlocks - jBlock - nb - 1)) >> 1;
          aother = aUnder + number_entries(i);
          ClpCholeskyCrecRec(thisStruct, aTri + number_entries(nb), nThis - nThis2, nLeft, nThis2, aUnder, aother,
                             work, kBlock, jBlock, numberBlocks);
          ClpCholeskyCtriRec(thisStruct, aTri + number_entries(nintri + nbelow), nThis - nThis2, aother, diagonal + nThis2,
                             work + nThis2, nLeft,
                             iBlock - nb, kBlock - nb, numberBlocks - nb);
     }
}
/* Non leaf recursive rectangle triangle update*/
void
ClpCholeskyCrecTri(ClpCholeskyDenseC * thisStruct, longDouble * aUnder, int nTri, int nDo,
                   int iBlock, int jBlock, longDouble * aTri,
                   longDouble * diagonal, longDouble * work,
                   int numberBlocks)
{
     if (nTri <= BLOCK && nDo <= BLOCK) {
          ClpCholeskyCrecTriLeaf(/*thisStruct,*/ aUnder, aTri,/*diagonal,*/work, nTri);
     } else if (nTri < nDo) {
          int nb = number_blocks((nDo + 1) >> 1);
          int nDo2 = number_rows(nb);
          longDouble * aother;
          int i;
          ClpCholeskyCrecTri(thisStruct, aUnder, nTri, nDo2, iBlock, jBlock, aTri, diagonal, work, numberBlocks);
          i = ((numberBlocks - jBlock) * (numberBlocks - jBlock - 1) -
               (numberBlocks - jBlock - nb) * (numberBlocks - jBlock - nb - 1)) >> 1;
          aother = aUnder + number_entries(i);
          ClpCholeskyCrecTri(thisStruct, aother, nTri, nDo - nDo2, iBlock - nb, jBlock, aTri, diagonal + nDo2,
                             work + nDo2, numberBlocks - nb);
     } else {
          int nb = number_blocks((nTri + 1) >> 1);
          int nTri2 = number_rows(nb);
          longDouble * aother;
          int i;
          cilk_spawn ClpCholeskyCrecTri(thisStruct, aUnder, nTri2, nDo, iBlock, jBlock, aTri, diagonal, work, numberBlocks);
          /* and rectangular update */
          i = ((numberBlocks - iBlock) * (numberBlocks - iBlock + 1) -
               (numberBlocks - iBlock - nb) * (numberBlocks - iBlock - nb + 1)) >> 1;
          aother = aTri + number_entries(nb);
          ClpCholeskyCrecRec(thisStruct, aUnder, nTri2, nTri - nTri2, nDo, aUnder + number_entries(nb), aother,
                             work, iBlock, jBlock, numberBlocks);
          ClpCholeskyCrecTri(thisStruct, aUnder + number_entries(nb), nTri - nTri2, nDo, iBlock + nb, jBlock,
                             aTri + number_entries(i), diagonal, work, numberBlocks);
	  cilk_sync;
     }
}
/* Non leaf recursive rectangle rectangle update,
   nUnder is number of rows in iBlock,
   nUnderK is number of rows in kBlock
*/
void
ClpCholeskyCrecRec(ClpCholeskyDenseC * thisStruct, longDouble * above, int nUnder, int nUnderK,
                   int nDo, longDouble * aUnder, longDouble *aOther,
                   longDouble * work,
                   int iBlock, int jBlock,
                   int numberBlocks)
{
     if (nDo <= BLOCK && nUnder <= BLOCK && nUnderK <= BLOCK) {
          assert (nDo == BLOCK && nUnder == BLOCK);
          ClpCholeskyCrecRecLeaf(/*thisStruct,*/ above , aUnder ,  aOther, work, nUnderK);
     } else if (nDo <= nUnderK && nUnder <= nUnderK) {
          int nb = number_blocks((nUnderK + 1) >> 1);
          int nUnder2 = number_rows(nb);
          cilk_spawn ClpCholeskyCrecRec(thisStruct, above, nUnder, nUnder2, nDo, aUnder, aOther, work,
                             iBlock, jBlock, numberBlocks);
          ClpCholeskyCrecRec(thisStruct, above, nUnder, nUnderK - nUnder2, nDo, aUnder + number_entries(nb),
                             aOther + number_entries(nb), work, iBlock, jBlock, numberBlocks);
	  cilk_sync;
     } else if (nUnderK <= nDo && nUnder <= nDo) {
          int nb = number_blocks((nDo + 1) >> 1);
          int nDo2 = number_rows(nb);
          int i;
          ClpCholeskyCrecRec(thisStruct, above, nUnder, nUnderK, nDo2, aUnder, aOther, work,
                             iBlock, jBlock, numberBlocks);
          i = ((numberBlocks - jBlock) * (numberBlocks - jBlock - 1) -
               (numberBlocks - jBlock - nb) * (numberBlocks - jBlock - nb - 1)) >> 1;
          ClpCholeskyCrecRec(thisStruct, above + number_entries(i), nUnder, nUnderK, nDo - nDo2,
                             aUnder + number_entries(i),
                             aOther, work + nDo2,
                             iBlock - nb, jBlock, numberBlocks - nb);
     } else {
          int nb = number_blocks((nUnder + 1) >> 1);
          int nUnder2 = number_rows(nb);
          int i;
          cilk_spawn ClpCholeskyCrecRec(thisStruct, above, nUnder2, nUnderK, nDo, aUnder, aOther, work,
                             iBlock, jBlock, numberBlocks);
          i = ((numberBlocks - iBlock) * (numberBlocks - iBlock - 1) -
               (numberBlocks - iBlock - nb) * (numberBlocks - iBlock - nb - 1)) >> 1;
          ClpCholeskyCrecRec(thisStruct, above + number_entries(nb), nUnder - nUnder2, nUnderK, nDo, aUnder,
                             aOther + number_entries(i), work, iBlock + nb, jBlock, numberBlocks);
	  cilk_sync;
     }
}
/* Leaf recursive factor*/
void
ClpCholeskyCfactorLeaf(ClpCholeskyDenseC * thisStruct, longDouble * a, int n,
                       longDouble * diagonal, longDouble * work, int * rowsDropped)
{
     double dropValue = thisStruct->doubleParameters_[0];
     int firstPositive = thisStruct->integerParameters_[0];
     int rowOffset = static_cast<int>(diagonal - thisStruct->diagonal_);
     int i, j, k;
     CoinWorkDouble t00, temp1;
     longDouble * aa;
     aa = a - BLOCK;
     for (j = 0; j < n; j ++) {
          bool dropColumn;
          CoinWorkDouble useT00;
          aa += BLOCK;
          t00 = aa[j];
          for (k = 0; k < j; ++k) {
               CoinWorkDouble multiplier = work[k];
               t00 -= a[j + k * BLOCK] * a[j + k * BLOCK] * multiplier;
          }
          dropColumn = false;
          useT00 = t00;
          if (j + rowOffset < firstPositive) {
               /* must be negative*/
               if (t00 <= -dropValue) {
                    /*aa[j]=t00;*/
                    t00 = 1.0 / t00;
               } else {
                    dropColumn = true;
                    /*aa[j]=-1.0e100;*/
                    useT00 = -1.0e-100;
                    t00 = 0.0;
               }
          } else {
               /* must be positive*/
               if (t00 >= dropValue) {
                    /*aa[j]=t00;*/
                    t00 = 1.0 / t00;
               } else {
                    dropColumn = true;
                    /*aa[j]=1.0e100;*/
                    useT00 = 1.0e-100;
                    t00 = 0.0;
               }
          }
          if (!dropColumn) {
               diagonal[j] = t00;
               work[j] = useT00;
               temp1 = t00;
               for (i = j + 1; i < n; i++) {
                    t00 = aa[i];
                    for (k = 0; k < j; ++k) {
                         CoinWorkDouble multiplier = work[k];
                         t00 -= a[i + k * BLOCK] * a[j + k * BLOCK] * multiplier;
                    }
                    aa[i] = t00 * temp1;
               }
          } else {
               /* drop column*/
               rowsDropped[j+rowOffset] = 2;
               diagonal[j] = 0.0;
               /*aa[j]=1.0e100;*/
               work[j] = 1.0e100;
               for (i = j + 1; i < n; i++) {
                    aa[i] = 0.0;
               }
          }
     }
}
/* Leaf recursive triangle rectangle update*/
void
ClpCholeskyCtriRecLeaf(/*ClpCholeskyDenseC * thisStruct,*/ longDouble * aTri, longDouble * aUnder, longDouble * diagonal, longDouble * work,
          int nUnder)
{
#ifdef POS_DEBUG
     int iru, icu;
     int iu = bNumber(aUnder, iru, icu);
     int irt, ict;
     int it = bNumber(aTri, irt, ict);
     /*printf("%d %d\n",iu,it);*/
     printf("trirecleaf  under (%d,%d), tri (%d,%d)\n",
            iru, icu, irt, ict);
     assert (diagonal == thisStruct->diagonal_ + ict * BLOCK);
#endif
     int j;
     longDouble * aa;
#ifdef BLOCKUNROLL
     if (nUnder == BLOCK) {
          aa = aTri - 2 * BLOCK;
          for (j = 0; j < BLOCK; j += 2) {
               int i;
               CoinWorkDouble temp0 = diagonal[j];
               CoinWorkDouble temp1 = diagonal[j+1];
               aa += 2 * BLOCK;
               for ( i = 0; i < BLOCK; i += 2) {
                    CoinWorkDouble at1;
                    CoinWorkDouble t00 = aUnder[i+j*BLOCK];
                    CoinWorkDouble t10 = aUnder[i+ BLOCK + j*BLOCK];
                    CoinWorkDouble t01 = aUnder[i+1+j*BLOCK];
                    CoinWorkDouble t11 = aUnder[i+1+ BLOCK + j*BLOCK];
                    int k;
                    for (k = 0; k < j; ++k) {
                         CoinWorkDouble multiplier = work[k];
                         CoinWorkDouble au0 = aUnder[i + k * BLOCK] * multiplier;
                         CoinWorkDouble au1 = aUnder[i + 1 + k * BLOCK] * multiplier;
                         CoinWorkDouble at0 = aTri[j + k * BLOCK];
                         CoinWorkDouble at1 = aTri[j + 1 + k * BLOCK];
                         t00 -= au0 * at0;
                         t10 -= au0 * at1;
                         t01 -= au1 * at0;
                         t11 -= au1 * at1;
                    }
                    t00 *= temp0;
                    at1 = aTri[j + 1 + j*BLOCK] * work[j];
                    t10 -= t00 * at1;
                    t01 *= temp0;
                    t11 -= t01 * at1;
                    aUnder[i+j*BLOCK] = t00;
                    aUnder[i+1+j*BLOCK] = t01;
                    aUnder[i+ BLOCK + j*BLOCK] = t10 * temp1;
                    aUnder[i+1+ BLOCK + j*BLOCK] = t11 * temp1;
               }
          }
     } else {
#endif
          aa = aTri - BLOCK;
          for (j = 0; j < BLOCK; j ++) {
               int i;
               CoinWorkDouble temp1 = diagonal[j];
               aa += BLOCK;
               for (i = 0; i < nUnder; i++) {
                    int k;
                    CoinWorkDouble t00 = aUnder[i+j*BLOCK];
                    for ( k = 0; k < j; ++k) {
                         CoinWorkDouble multiplier = work[k];
                         t00 -= aUnder[i + k * BLOCK] * aTri[j + k * BLOCK] * multiplier;
                    }
                    aUnder[i+j*BLOCK] = t00 * temp1;
               }
          }
#ifdef BLOCKUNROLL
     }
#endif
}
/* Leaf recursive rectangle triangle update*/
void ClpCholeskyCrecTriLeaf(/*ClpCholeskyDenseC * thisStruct,*/ longDouble * aUnder, longDouble * aTri,
          /*longDouble * diagonal,*/ longDouble * work, int nUnder)
{
#ifdef POS_DEBUG
     int iru, icu;
     int iu = bNumber(aUnder, iru, icu);
     int irt, ict;
     int it = bNumber(aTri, irt, ict);
     /*printf("%d %d\n",iu,it);*/
     printf("rectrileaf  under (%d,%d), tri (%d,%d)\n",
            iru, icu, irt, ict);
     assert (diagonal == thisStruct->diagonal_ + icu * BLOCK);
#endif
     int i, j, k;
     CoinWorkDouble t00;
     longDouble * aa;
#ifdef BLOCKUNROLL
     if (nUnder == BLOCK) {
          longDouble * aUnder2 = aUnder - 2;
          aa = aTri - 2 * BLOCK;
          for (j = 0; j < BLOCK; j += 2) {
               CoinWorkDouble t00, t01, t10, t11;
               aa += 2 * BLOCK;
               aUnder2 += 2;
               t00 = aa[j];
               t01 = aa[j+1];
               t10 = aa[j+1+BLOCK];
               for (k = 0; k < BLOCK; ++k) {
                    CoinWorkDouble multiplier = work[k];
                    CoinWorkDouble a0 = aUnder2[k * BLOCK];
                    CoinWorkDouble a1 = aUnder2[1 + k * BLOCK];
                    CoinWorkDouble x0 = a0 * multiplier;
                    CoinWorkDouble x1 = a1 * multiplier;
                    t00 -= a0 * x0;
                    t01 -= a1 * x0;
                    t10 -= a1 * x1;
               }
               aa[j] = t00;
               aa[j+1] = t01;
               aa[j+1+BLOCK] = t10;
               for (i = j + 2; i < BLOCK; i += 2) {
                    t00 = aa[i];
                    t01 = aa[i+BLOCK];
                    t10 = aa[i+1];
                    t11 = aa[i+1+BLOCK];
                    for (k = 0; k < BLOCK; ++k) {
                         CoinWorkDouble multiplier = work[k];
                         CoinWorkDouble a0 = aUnder2[k * BLOCK] * multiplier;
                         CoinWorkDouble a1 = aUnder2[1 + k * BLOCK] * multiplier;
                         t00 -= aUnder[i + k * BLOCK] * a0;
                         t01 -= aUnder[i + k * BLOCK] * a1;
                         t10 -= aUnder[i + 1 + k * BLOCK] * a0;
                         t11 -= aUnder[i + 1 + k * BLOCK] * a1;
                    }
                    aa[i] = t00;
                    aa[i+BLOCK] = t01;
                    aa[i+1] = t10;
                    aa[i+1+BLOCK] = t11;
               }
          }
     } else {
#endif
          aa = aTri - BLOCK;
          for (j = 0; j < nUnder; j ++) {
               aa += BLOCK;
               for (i = j; i < nUnder; i++) {
                    t00 = aa[i];
                    for (k = 0; k < BLOCK; ++k) {
                         CoinWorkDouble multiplier = work[k];
                         t00 -= aUnder[i + k * BLOCK] * aUnder[j + k * BLOCK] * multiplier;
                    }
                    aa[i] = t00;
               }
          }
#ifdef BLOCKUNROLL
     }
#endif
}
/* Leaf recursive rectangle rectangle update,
   nUnder is number of rows in iBlock,
   nUnderK is number of rows in kBlock
*/
void
ClpCholeskyCrecRecLeaf(/*ClpCholeskyDenseC * thisStruct,*/
     const longDouble * COIN_RESTRICT above,
     const longDouble * COIN_RESTRICT aUnder,
     longDouble * COIN_RESTRICT aOther,
     const longDouble * COIN_RESTRICT work,
     int nUnder)
{
#ifdef POS_DEBUG
     int ira, ica;
     int ia = bNumber(above, ira, ica);
     int iru, icu;
     int iu = bNumber(aUnder, iru, icu);
     int iro, ico;
     int io = bNumber(aOther, iro, ico);
     /*printf("%d %d %d\n",ia,iu,io);*/
     printf("recrecleaf above (%d,%d), under (%d,%d), other (%d,%d)\n",
            ira, ica, iru, icu, iro, ico);
#endif
     int i, j, k;
     longDouble * aa;
#ifdef BLOCKUNROLL
     aa = aOther - 4 * BLOCK;
     if (nUnder == BLOCK) {
          /*#define INTEL*/
#ifdef INTEL
          aa += 2 * BLOCK;
          for (j = 0; j < BLOCK; j += 2) {
               aa += 2 * BLOCK;
               for (i = 0; i < BLOCK; i += 2) {
                    CoinWorkDouble t00 = aa[i+0*BLOCK];
                    CoinWorkDouble t10 = aa[i+1*BLOCK];
                    CoinWorkDouble t01 = aa[i+1+0*BLOCK];
                    CoinWorkDouble t11 = aa[i+1+1*BLOCK];
                    for (k = 0; k < BLOCK; k++) {
                         CoinWorkDouble multiplier = work[k];
                         CoinWorkDouble a00 = aUnder[i+k*BLOCK] * multiplier;
                         CoinWorkDouble a01 = aUnder[i+1+k*BLOCK] * multiplier;
                         t00 -= a00 * above[j + 0 + k * BLOCK];
                         t10 -= a00 * above[j + 1 + k * BLOCK];
                         t01 -= a01 * above[j + 0 + k * BLOCK];
                         t11 -= a01 * above[j + 1 + k * BLOCK];
                    }
                    aa[i+0*BLOCK] = t00;
                    aa[i+1*BLOCK] = t10;
                    aa[i+1+0*BLOCK] = t01;
                    aa[i+1+1*BLOCK] = t11;
               }
          }
#else
          for (j = 0; j < BLOCK; j += 4) {
               aa += 4 * BLOCK;
               for (i = 0; i < BLOCK; i += 4) {
                    CoinWorkDouble t00 = aa[i+0+0*BLOCK];
                    CoinWorkDouble t10 = aa[i+0+1*BLOCK];
                    CoinWorkDouble t20 = aa[i+0+2*BLOCK];
                    CoinWorkDouble t30 = aa[i+0+3*BLOCK];
                    CoinWorkDouble t01 = aa[i+1+0*BLOCK];
                    CoinWorkDouble t11 = aa[i+1+1*BLOCK];
                    CoinWorkDouble t21 = aa[i+1+2*BLOCK];
                    CoinWorkDouble t31 = aa[i+1+3*BLOCK];
                    CoinWorkDouble t02 = aa[i+2+0*BLOCK];
                    CoinWorkDouble t12 = aa[i+2+1*BLOCK];
                    CoinWorkDouble t22 = aa[i+2+2*BLOCK];
                    CoinWorkDouble t32 = aa[i+2+3*BLOCK];
                    CoinWorkDouble t03 = aa[i+3+0*BLOCK];
                    CoinWorkDouble t13 = aa[i+3+1*BLOCK];
                    CoinWorkDouble t23 = aa[i+3+2*BLOCK];
                    CoinWorkDouble t33 = aa[i+3+3*BLOCK];
                    const longDouble * COIN_RESTRICT aUnderNow = aUnder + i;
                    const longDouble * COIN_RESTRICT aboveNow = above + j;
                    for (k = 0; k < BLOCK; k++) {
                         CoinWorkDouble multiplier = work[k];
                         CoinWorkDouble a00 = aUnderNow[0] * multiplier;
                         CoinWorkDouble a01 = aUnderNow[1] * multiplier;
                         CoinWorkDouble a02 = aUnderNow[2] * multiplier;
                         CoinWorkDouble a03 = aUnderNow[3] * multiplier;
                         t00 -= a00 * aboveNow[0];
                         t10 -= a00 * aboveNow[1];
                         t20 -= a00 * aboveNow[2];
                         t30 -= a00 * aboveNow[3];
                         t01 -= a01 * aboveNow[0];
                         t11 -= a01 * aboveNow[1];
                         t21 -= a01 * aboveNow[2];
                         t31 -= a01 * aboveNow[3];
                         t02 -= a02 * aboveNow[0];
                         t12 -= a02 * aboveNow[1];
                         t22 -= a02 * aboveNow[2];
                         t32 -= a02 * aboveNow[3];
                         t03 -= a03 * aboveNow[0];
                         t13 -= a03 * aboveNow[1];
                         t23 -= a03 * aboveNow[2];
                         t33 -= a03 * aboveNow[3];
                         aUnderNow += BLOCK;
                         aboveNow += BLOCK;
                    }
                    aa[i+0+0*BLOCK] = t00;
                    aa[i+0+1*BLOCK] = t10;
                    aa[i+0+2*BLOCK] = t20;
                    aa[i+0+3*BLOCK] = t30;
                    aa[i+1+0*BLOCK] = t01;
                    aa[i+1+1*BLOCK] = t11;
                    aa[i+1+2*BLOCK] = t21;
                    aa[i+1+3*BLOCK] = t31;
                    aa[i+2+0*BLOCK] = t02;
                    aa[i+2+1*BLOCK] = t12;
                    aa[i+2+2*BLOCK] = t22;
                    aa[i+2+3*BLOCK] = t32;
                    aa[i+3+0*BLOCK] = t03;
                    aa[i+3+1*BLOCK] = t13;
                    aa[i+3+2*BLOCK] = t23;
                    aa[i+3+3*BLOCK] = t33;
               }
          }
#endif
     } else {
          int odd = nUnder & 1;
          int n = nUnder - odd;
          for (j = 0; j < BLOCK; j += 4) {
               aa += 4 * BLOCK;
               for (i = 0; i < n; i += 2) {
                    CoinWorkDouble t00 = aa[i+0*BLOCK];
                    CoinWorkDouble t10 = aa[i+1*BLOCK];
                    CoinWorkDouble t20 = aa[i+2*BLOCK];
                    CoinWorkDouble t30 = aa[i+3*BLOCK];
                    CoinWorkDouble t01 = aa[i+1+0*BLOCK];
                    CoinWorkDouble t11 = aa[i+1+1*BLOCK];
                    CoinWorkDouble t21 = aa[i+1+2*BLOCK];
                    CoinWorkDouble t31 = aa[i+1+3*BLOCK];
                    const longDouble * COIN_RESTRICT aUnderNow = aUnder + i;
                    const longDouble * COIN_RESTRICT aboveNow = above + j;
                    for (k = 0; k < BLOCK; k++) {
                         CoinWorkDouble multiplier = work[k];
                         CoinWorkDouble a00 = aUnderNow[0] * multiplier;
                         CoinWorkDouble a01 = aUnderNow[1] * multiplier;
                         t00 -= a00 * aboveNow[0];
                         t10 -= a00 * aboveNow[1];
                         t20 -= a00 * aboveNow[2];
                         t30 -= a00 * aboveNow[3];
                         t01 -= a01 * aboveNow[0];
                         t11 -= a01 * aboveNow[1];
                         t21 -= a01 * aboveNow[2];
                         t31 -= a01 * aboveNow[3];
                         aUnderNow += BLOCK;
                         aboveNow += BLOCK;
                    }
                    aa[i+0*BLOCK] = t00;
                    aa[i+1*BLOCK] = t10;
                    aa[i+2*BLOCK] = t20;
                    aa[i+3*BLOCK] = t30;
                    aa[i+1+0*BLOCK] = t01;
                    aa[i+1+1*BLOCK] = t11;
                    aa[i+1+2*BLOCK] = t21;
                    aa[i+1+3*BLOCK] = t31;
               }
               if (odd) {
                    CoinWorkDouble t0 = aa[n+0*BLOCK];
                    CoinWorkDouble t1 = aa[n+1*BLOCK];
                    CoinWorkDouble t2 = aa[n+2*BLOCK];
                    CoinWorkDouble t3 = aa[n+3*BLOCK];
                    CoinWorkDouble a0;
                    for (k = 0; k < BLOCK; k++) {
                         a0 = aUnder[n+k*BLOCK] * work[k];
                         t0 -= a0 * above[j + 0 + k * BLOCK];
                         t1 -= a0 * above[j + 1 + k * BLOCK];
                         t2 -= a0 * above[j + 2 + k * BLOCK];
                         t3 -= a0 * above[j + 3 + k * BLOCK];
                    }
                    aa[n+0*BLOCK] = t0;
                    aa[n+1*BLOCK] = t1;
                    aa[n+2*BLOCK] = t2;
                    aa[n+3*BLOCK] = t3;
               }
          }
     }
#else
     aa = aOther - BLOCK;
     for (j = 0; j < BLOCK; j ++) {
          aa += BLOCK;
          for (i = 0; i < nUnder; i++) {
               CoinWorkDouble t00 = aa[i+0*BLOCK];
               for (k = 0; k < BLOCK; k++) {
                    CoinWorkDouble a00 = aUnder[i+k*BLOCK] * work[k];
                    t00 -= a00 * above[j + k * BLOCK];
               }
               aa[i] = t00;
          }
     }
#endif
}
/* Uses factorization to solve. */
void
ClpCholeskyDense::solve (CoinWorkDouble * region)
{
#ifdef CHOL_COMPARE
     double * region2 = NULL;
     if (numberRows_ < 200) {
          longDouble * xx = sparseFactor_;
          longDouble * yy = diagonal_;
          diagonal_ = sparseFactor_ + 40000;
          sparseFactor_ = diagonal_ + numberRows_;
          region2 = ClpCopyOfArray(region, numberRows_);
          int iRow, iColumn;
          int addOffset = numberRows_ - 1;
          longDouble * work = sparseFactor_ - 1;
          for (iColumn = 0; iColumn < numberRows_; iColumn++) {
               double value = region2[iColumn];
               for (iRow = iColumn + 1; iRow < numberRows_; iRow++)
                    region2[iRow] -= value * work[iRow];
               addOffset--;
               work += addOffset;
          }
          for (iColumn = numberRows_ - 1; iColumn >= 0; iColumn--) {
               double value = region2[iColumn] * diagonal_[iColumn];
               work -= addOffset;
               addOffset++;
               for (iRow = iColumn + 1; iRow < numberRows_; iRow++)
                    value -= region2[iRow] * work[iRow];
               region2[iColumn] = value;
          }
          sparseFactor_ = xx;
          diagonal_ = yy;
     }
#endif
     /*longDouble * xx = sparseFactor_;*/
     /*longDouble * yy = diagonal_;*/
     /*diagonal_ = sparseFactor_ + 40000;*/
     /*sparseFactor_=diagonal_ + numberRows_;*/
     int numberBlocks = (numberRows_ + BLOCK - 1) >> BLOCKSHIFT;
     /* later align on boundary*/
     longDouble * a = sparseFactor_ + BLOCKSQ * numberBlocks;
     int iBlock;
     longDouble * aa = a;
     int iColumn;
     for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
          int nChunk;
          int jBlock;
          int iDo = iBlock * BLOCK;
          int base = iDo;
          if (iDo + BLOCK > numberRows_) {
               nChunk = numberRows_ - iDo;
          } else {
               nChunk = BLOCK;
          }
          solveF1(aa, nChunk, region + iDo);
          for (jBlock = iBlock + 1; jBlock < numberBlocks; jBlock++) {
               base += BLOCK;
               aa += BLOCKSQ;
               if (base + BLOCK > numberRows_) {
                    nChunk = numberRows_ - base;
               } else {
                    nChunk = BLOCK;
               }
               solveF2(aa, nChunk, region + iDo, region + base);
          }
          aa += BLOCKSQ;
     }
     /* do diagonal outside*/
     for (iColumn = 0; iColumn < numberRows_; iColumn++)
          region[iColumn] *= diagonal_[iColumn];
     int offset = ((numberBlocks * (numberBlocks + 1)) >> 1);
     aa = a + number_entries(offset - 1);
     int lBase = (numberBlocks - 1) * BLOCK;
     for (iBlock = numberBlocks - 1; iBlock >= 0; iBlock--) {
          int nChunk;
          int jBlock;
          int triBase = iBlock * BLOCK;
          int iBase = lBase;
          for (jBlock = iBlock + 1; jBlock < numberBlocks; jBlock++) {
               if (iBase + BLOCK > numberRows_) {
                    nChunk = numberRows_ - iBase;
               } else {
                    nChunk = BLOCK;
               }
               solveB2(aa, nChunk, region + triBase, region + iBase);
               iBase -= BLOCK;
               aa -= BLOCKSQ;
          }
          if (triBase + BLOCK > numberRows_) {
               nChunk = numberRows_ - triBase;
          } else {
               nChunk = BLOCK;
          }
          solveB1(aa, nChunk, region + triBase);
          aa -= BLOCKSQ;
     }
#ifdef CHOL_COMPARE
     if (numberRows_ < 200) {
          for (int i = 0; i < numberRows_; i++) {
               assert(CoinAbs(region[i] - region2[i]) < 1.0e-3);
          }
          delete [] region2;
     }
#endif
}
/* Forward part of solve 1*/
void
ClpCholeskyDense::solveF1(longDouble * a, int n, CoinWorkDouble * region)
{
     int j, k;
     CoinWorkDouble t00;
     for (j = 0; j < n; j ++) {
          t00 = region[j];
          for (k = 0; k < j; ++k) {
               t00 -= region[k] * a[j + k * BLOCK];
          }
          /*t00*=a[j + j * BLOCK];*/
          region[j] = t00;
     }
}
/* Forward part of solve 2*/
void
ClpCholeskyDense::solveF2(longDouble * a, int n, CoinWorkDouble * region, CoinWorkDouble * region2)
{
     int j, k;
#ifdef BLOCKUNROLL
     if (n == BLOCK) {
          for (k = 0; k < BLOCK; k += 4) {
               CoinWorkDouble t0 = region2[0];
               CoinWorkDouble t1 = region2[1];
               CoinWorkDouble t2 = region2[2];
               CoinWorkDouble t3 = region2[3];
               t0 -= region[0] * a[0 + 0 * BLOCK];
               t1 -= region[0] * a[1 + 0 * BLOCK];
               t2 -= region[0] * a[2 + 0 * BLOCK];
               t3 -= region[0] * a[3 + 0 * BLOCK];

               t0 -= region[1] * a[0 + 1 * BLOCK];
               t1 -= region[1] * a[1 + 1 * BLOCK];
               t2 -= region[1] * a[2 + 1 * BLOCK];
               t3 -= region[1] * a[3 + 1 * BLOCK];

               t0 -= region[2] * a[0 + 2 * BLOCK];
               t1 -= region[2] * a[1 + 2 * BLOCK];
               t2 -= region[2] * a[2 + 2 * BLOCK];
               t3 -= region[2] * a[3 + 2 * BLOCK];

               t0 -= region[3] * a[0 + 3 * BLOCK];
               t1 -= region[3] * a[1 + 3 * BLOCK];
               t2 -= region[3] * a[2 + 3 * BLOCK];
               t3 -= region[3] * a[3 + 3 * BLOCK];

               t0 -= region[4] * a[0 + 4 * BLOCK];
               t1 -= region[4] * a[1 + 4 * BLOCK];
               t2 -= region[4] * a[2 + 4 * BLOCK];
               t3 -= region[4] * a[3 + 4 * BLOCK];

               t0 -= region[5] * a[0 + 5 * BLOCK];
               t1 -= region[5] * a[1 + 5 * BLOCK];
               t2 -= region[5] * a[2 + 5 * BLOCK];
               t3 -= region[5] * a[3 + 5 * BLOCK];

               t0 -= region[6] * a[0 + 6 * BLOCK];
               t1 -= region[6] * a[1 + 6 * BLOCK];
               t2 -= region[6] * a[2 + 6 * BLOCK];
               t3 -= region[6] * a[3 + 6 * BLOCK];

               t0 -= region[7] * a[0 + 7 * BLOCK];
               t1 -= region[7] * a[1 + 7 * BLOCK];
               t2 -= region[7] * a[2 + 7 * BLOCK];
               t3 -= region[7] * a[3 + 7 * BLOCK];
#if BLOCK>8
               t0 -= region[8] * a[0 + 8 * BLOCK];
               t1 -= region[8] * a[1 + 8 * BLOCK];
               t2 -= region[8] * a[2 + 8 * BLOCK];
               t3 -= region[8] * a[3 + 8 * BLOCK];

               t0 -= region[9] * a[0 + 9 * BLOCK];
               t1 -= region[9] * a[1 + 9 * BLOCK];
               t2 -= region[9] * a[2 + 9 * BLOCK];
               t3 -= region[9] * a[3 + 9 * BLOCK];

               t0 -= region[10] * a[0 + 10 * BLOCK];
               t1 -= region[10] * a[1 + 10 * BLOCK];
               t2 -= region[10] * a[2 + 10 * BLOCK];
               t3 -= region[10] * a[3 + 10 * BLOCK];

               t0 -= region[11] * a[0 + 11 * BLOCK];
               t1 -= region[11] * a[1 + 11 * BLOCK];
               t2 -= region[11] * a[2 + 11 * BLOCK];
               t3 -= region[11] * a[3 + 11 * BLOCK];

               t0 -= region[12] * a[0 + 12 * BLOCK];
               t1 -= region[12] * a[1 + 12 * BLOCK];
               t2 -= region[12] * a[2 + 12 * BLOCK];
               t3 -= region[12] * a[3 + 12 * BLOCK];

               t0 -= region[13] * a[0 + 13 * BLOCK];
               t1 -= region[13] * a[1 + 13 * BLOCK];
               t2 -= region[13] * a[2 + 13 * BLOCK];
               t3 -= region[13] * a[3 + 13 * BLOCK];

               t0 -= region[14] * a[0 + 14 * BLOCK];
               t1 -= region[14] * a[1 + 14 * BLOCK];
               t2 -= region[14] * a[2 + 14 * BLOCK];
               t3 -= region[14] * a[3 + 14 * BLOCK];

               t0 -= region[15] * a[0 + 15 * BLOCK];
               t1 -= region[15] * a[1 + 15 * BLOCK];
               t2 -= region[15] * a[2 + 15 * BLOCK];
               t3 -= region[15] * a[3 + 15 * BLOCK];
#endif
               region2[0] = t0;
               region2[1] = t1;
               region2[2] = t2;
               region2[3] = t3;
               region2 += 4;
               a += 4;
          }
     } else {
#endif
          for (k = 0; k < n; ++k) {
               CoinWorkDouble t00 = region2[k];
               for (j = 0; j < BLOCK; j ++) {
                    t00 -= region[j] * a[k + j * BLOCK];
               }
               region2[k] = t00;
          }
#ifdef BLOCKUNROLL
     }
#endif
}
/* Backward part of solve 1*/
void
ClpCholeskyDense::solveB1(longDouble * a, int n, CoinWorkDouble * region)
{
     int j, k;
     CoinWorkDouble t00;
     for (j = n - 1; j >= 0; j --) {
          t00 = region[j];
          for (k = j + 1; k < n; ++k) {
               t00 -= region[k] * a[k + j * BLOCK];
          }
          /*t00*=a[j + j * BLOCK];*/
          region[j] = t00;
     }
}
/* Backward part of solve 2*/
void
ClpCholeskyDense::solveB2(longDouble * a, int n, CoinWorkDouble * region, CoinWorkDouble * region2)
{
     int j, k;
#ifdef BLOCKUNROLL
     if (n == BLOCK) {
          for (j = 0; j < BLOCK; j += 4) {
               CoinWorkDouble t0 = region[0];
               CoinWorkDouble t1 = region[1];
               CoinWorkDouble t2 = region[2];
               CoinWorkDouble t3 = region[3];
               t0 -= region2[0] * a[0 + 0*BLOCK];
               t1 -= region2[0] * a[0 + 1*BLOCK];
               t2 -= region2[0] * a[0 + 2*BLOCK];
               t3 -= region2[0] * a[0 + 3*BLOCK];

               t0 -= region2[1] * a[1 + 0*BLOCK];
               t1 -= region2[1] * a[1 + 1*BLOCK];
               t2 -= region2[1] * a[1 + 2*BLOCK];
               t3 -= region2[1] * a[1 + 3*BLOCK];

               t0 -= region2[2] * a[2 + 0*BLOCK];
               t1 -= region2[2] * a[2 + 1*BLOCK];
               t2 -= region2[2] * a[2 + 2*BLOCK];
               t3 -= region2[2] * a[2 + 3*BLOCK];

               t0 -= region2[3] * a[3 + 0*BLOCK];
               t1 -= region2[3] * a[3 + 1*BLOCK];
               t2 -= region2[3] * a[3 + 2*BLOCK];
               t3 -= region2[3] * a[3 + 3*BLOCK];

               t0 -= region2[4] * a[4 + 0*BLOCK];
               t1 -= region2[4] * a[4 + 1*BLOCK];
               t2 -= region2[4] * a[4 + 2*BLOCK];
               t3 -= region2[4] * a[4 + 3*BLOCK];

               t0 -= region2[5] * a[5 + 0*BLOCK];
               t1 -= region2[5] * a[5 + 1*BLOCK];
               t2 -= region2[5] * a[5 + 2*BLOCK];
               t3 -= region2[5] * a[5 + 3*BLOCK];

               t0 -= region2[6] * a[6 + 0*BLOCK];
               t1 -= region2[6] * a[6 + 1*BLOCK];
               t2 -= region2[6] * a[6 + 2*BLOCK];
               t3 -= region2[6] * a[6 + 3*BLOCK];

               t0 -= region2[7] * a[7 + 0*BLOCK];
               t1 -= region2[7] * a[7 + 1*BLOCK];
               t2 -= region2[7] * a[7 + 2*BLOCK];
               t3 -= region2[7] * a[7 + 3*BLOCK];
#if BLOCK>8

               t0 -= region2[8] * a[8 + 0*BLOCK];
               t1 -= region2[8] * a[8 + 1*BLOCK];
               t2 -= region2[8] * a[8 + 2*BLOCK];
               t3 -= region2[8] * a[8 + 3*BLOCK];

               t0 -= region2[9] * a[9 + 0*BLOCK];
               t1 -= region2[9] * a[9 + 1*BLOCK];
               t2 -= region2[9] * a[9 + 2*BLOCK];
               t3 -= region2[9] * a[9 + 3*BLOCK];

               t0 -= region2[10] * a[10 + 0*BLOCK];
               t1 -= region2[10] * a[10 + 1*BLOCK];
               t2 -= region2[10] * a[10 + 2*BLOCK];
               t3 -= region2[10] * a[10 + 3*BLOCK];

               t0 -= region2[11] * a[11 + 0*BLOCK];
               t1 -= region2[11] * a[11 + 1*BLOCK];
               t2 -= region2[11] * a[11 + 2*BLOCK];
               t3 -= region2[11] * a[11 + 3*BLOCK];

               t0 -= region2[12] * a[12 + 0*BLOCK];
               t1 -= region2[12] * a[12 + 1*BLOCK];
               t2 -= region2[12] * a[12 + 2*BLOCK];
               t3 -= region2[12] * a[12 + 3*BLOCK];

               t0 -= region2[13] * a[13 + 0*BLOCK];
               t1 -= region2[13] * a[13 + 1*BLOCK];
               t2 -= region2[13] * a[13 + 2*BLOCK];
               t3 -= region2[13] * a[13 + 3*BLOCK];

               t0 -= region2[14] * a[14 + 0*BLOCK];
               t1 -= region2[14] * a[14 + 1*BLOCK];
               t2 -= region2[14] * a[14 + 2*BLOCK];
               t3 -= region2[14] * a[14 + 3*BLOCK];

               t0 -= region2[15] * a[15 + 0*BLOCK];
               t1 -= region2[15] * a[15 + 1*BLOCK];
               t2 -= region2[15] * a[15 + 2*BLOCK];
               t3 -= region2[15] * a[15 + 3*BLOCK];
#endif
               region[0] = t0;
               region[1] = t1;
               region[2] = t2;
               region[3] = t3;
               a += 4 * BLOCK;
               region += 4;
          }
     } else {
#endif
          for (j = 0; j < BLOCK; j ++) {
               CoinWorkDouble t00 = region[j];
               for (k = 0; k < n; ++k) {
                    t00 -= region2[k] * a[k + j * BLOCK];
               }
               region[j] = t00;
          }
#ifdef BLOCKUNROLL
     }
#endif
}
