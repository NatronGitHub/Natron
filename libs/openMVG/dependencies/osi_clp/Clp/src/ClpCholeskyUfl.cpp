/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpConfig.h"

extern "C" {
#ifndef COIN_HAS_CHOLMOD
#ifndef COIN_HAS_AMD
#error "Need to have AMD or CHOLMOD to compile ClpCholeskyUfl."
#else
#include "amd.h"
#endif
#else
#include "cholmod.h"
#endif
}

#include "CoinPragma.hpp"
#include "ClpCholeskyUfl.hpp"
#include "ClpMessage.hpp"
#include "ClpInterior.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpCholeskyUfl::ClpCholeskyUfl (int denseThreshold)
     : ClpCholeskyBase(denseThreshold)
{
     type_ = 14;
     L_ = NULL;
     c_ = NULL;
     
#ifdef COIN_HAS_CHOLMOD
     c_ = (cholmod_common*) malloc(sizeof(cholmod_common));
     cholmod_start (c_) ;
     // Can't use supernodal as may not be positive definite
     c_->supernodal = 0;
#endif
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpCholeskyUfl::ClpCholeskyUfl (const ClpCholeskyUfl & rhs)
     : ClpCholeskyBase(rhs)
{
     abort();
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpCholeskyUfl::~ClpCholeskyUfl ()
{
#ifdef COIN_HAS_CHOLMOD
     cholmod_free_factor (&L_, c_) ;
     cholmod_finish (c_) ;
     free(c_);
#endif
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpCholeskyUfl &
ClpCholeskyUfl::operator=(const ClpCholeskyUfl& rhs)
{
     if (this != &rhs) {
          ClpCholeskyBase::operator=(rhs);
          abort();
     }
     return *this;
}

//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpCholeskyBase * ClpCholeskyUfl::clone() const
{
     return new ClpCholeskyUfl(*this);
}

#ifndef COIN_HAS_CHOLMOD
/* Orders rows and saves pointer to matrix.and model */
int
ClpCholeskyUfl::order(ClpInterior * model)
{
     int iRow;
     model_ = model;
     if (preOrder(false, true, doKKT_))
          return -1;
     permuteInverse_ = new int [numberRows_];
     permute_ = new int[numberRows_];
     double Control[AMD_CONTROL];
     double Info[AMD_INFO];

     amd_defaults(Control);
     //amd_control(Control);

     int returnCode = amd_order(numberRows_, choleskyStart_, choleskyRow_,
                                permute_, Control, Info);
     delete [] choleskyRow_;
     choleskyRow_ = NULL;
     delete [] choleskyStart_;
     choleskyStart_ = NULL;
     //amd_info(Info);

     if (returnCode != AMD_OK) {
          std::cout << "AMD ordering failed" << std::endl;
          return 1;
     }
     for (iRow = 0; iRow < numberRows_; iRow++) {
          permuteInverse_[permute_[iRow]] = iRow;
     }
     return 0;
}
#else
/* Orders rows and saves pointer to matrix.and model */
int
ClpCholeskyUfl::order(ClpInterior * model)
{
     numberRows_ = model->numberRows();
     if (doKKT_) {
          numberRows_ += numberRows_ + model->numberColumns();
          printf("finish coding UFL KKT!\n");
          abort();
     }
     rowsDropped_ = new char [numberRows_];
     memset(rowsDropped_, 0, numberRows_);
     numberRowsDropped_ = 0;
     model_ = model;
     rowCopy_ = model->clpMatrix()->reverseOrderedCopy();
     // Space for starts
     choleskyStart_ = new CoinBigIndex[numberRows_+1];
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
     const int * rowLength = rowCopy_->getVectorLengths();
     const int * column = rowCopy_->getIndices();
     // We need two arrays for counts
     int * which = new int [numberRows_];
     int * used = new int[numberRows_+1];
     CoinZeroN(used, numberRows_);
     int iRow;
     sizeFactor_ = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int number = 1;
          // make sure diagonal exists
          which[0] = iRow;
          used[iRow] = 1;
          if (!rowsDropped_[iRow]) {
               CoinBigIndex startRow = rowStart[iRow];
               CoinBigIndex endRow = rowStart[iRow] + rowLength[iRow];
               for (CoinBigIndex k = startRow; k < endRow; k++) {
                    int iColumn = column[k];
                    CoinBigIndex start = columnStart[iColumn];
                    CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                    for (CoinBigIndex j = start; j < end; j++) {
                         int jRow = row[j];
                         if (jRow >= iRow && !rowsDropped_[jRow]) {
                              if (!used[jRow]) {
                                   used[jRow] = 1;
                                   which[number++] = jRow;
                              }
                         }
                    }
               }
               sizeFactor_ += number;
               int j;
               for (j = 0; j < number; j++)
                    used[which[j]] = 0;
          }
     }
     delete [] which;
     // Now we have size - create arrays and fill in
     try {
          choleskyRow_ = new int [sizeFactor_];
     } catch (...) {
          // no memory
          delete [] choleskyStart_;
          choleskyStart_ = NULL;
          return -1;
     }
     try {
          sparseFactor_ = new double[sizeFactor_];
     } catch (...) {
          // no memory
          delete [] choleskyRow_;
          choleskyRow_ = NULL;
          delete [] choleskyStart_;
          choleskyStart_ = NULL;
          return -1;
     }

     sizeFactor_ = 0;
     which = choleskyRow_;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int number = 1;
          // make sure diagonal exists
          which[0] = iRow;
          used[iRow] = 1;
          choleskyStart_[iRow] = sizeFactor_;
          if (!rowsDropped_[iRow]) {
               CoinBigIndex startRow = rowStart[iRow];
               CoinBigIndex endRow = rowStart[iRow] + rowLength[iRow];
               for (CoinBigIndex k = startRow; k < endRow; k++) {
                    int iColumn = column[k];
                    CoinBigIndex start = columnStart[iColumn];
                    CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                    for (CoinBigIndex j = start; j < end; j++) {
                         int jRow = row[j];
                         if (jRow >= iRow && !rowsDropped_[jRow]) {
                              if (!used[jRow]) {
                                   used[jRow] = 1;
                                   which[number++] = jRow;
                              }
                         }
                    }
               }
               sizeFactor_ += number;
               int j;
               for (j = 0; j < number; j++)
                    used[which[j]] = 0;
               // Sort
               std::sort(which, which + number);
               // move which on
               which += number;
          }
     }
     choleskyStart_[numberRows_] = sizeFactor_;
     delete [] used;
     permuteInverse_ = new int [numberRows_];
     permute_ = new int[numberRows_];
     cholmod_sparse A ;
     A.nrow = numberRows_;
     A.ncol = numberRows_;
     A.nzmax = choleskyStart_[numberRows_];
     A.p = choleskyStart_;
     A.i = choleskyRow_;
     A.x = NULL;
     A.stype = -1;
     A.itype = CHOLMOD_INT;
     A.xtype = CHOLMOD_PATTERN;
     A.dtype = CHOLMOD_DOUBLE;
     A.sorted = 1;
     A.packed = 1;
     c_->nmethods = 9;
     c_->postorder = true;
     //c_->dbound=1.0e-20;
     L_ = cholmod_analyze (&A, c_) ;
     if (c_->status) {
       COIN_DETAIL_PRINT(std::cout << "CHOLMOD ordering failed" << std::endl);
          return 1;
     } else {
       COIN_DETAIL_PRINT(printf("%g nonzeros, flop count %g\n", c_->lnz, c_->fl));
     }
     for (iRow = 0; iRow < numberRows_; iRow++) {
          permuteInverse_[iRow] = iRow;
          permute_[iRow] = iRow;
     }
     return 0;
}
#endif

/* Does Symbolic factorization given permutation.
   This is called immediately after order.  If user provides this then
   user must provide factorize and solve.  Otherwise the default factorization is used
   returns non-zero if not enough memory */
int
ClpCholeskyUfl::symbolic()
{
#ifdef COIN_HAS_CHOLMOD
     return 0;
#else
     return ClpCholeskyBase::symbolic();
#endif
}

#ifdef COIN_HAS_CHOLMOD
/* Factorize - filling in rowsDropped and returning number dropped */
int
ClpCholeskyUfl::factorize(const double * diagonal, int * rowsDropped)
{
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const double * element = model_->clpMatrix()->getElements();
     const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
     const int * rowLength = rowCopy_->getVectorLengths();
     const int * column = rowCopy_->getIndices();
     const double * elementByRow = rowCopy_->getElements();
     int numberColumns = model_->clpMatrix()->getNumCols();
     int iRow;
     double * work = new double[numberRows_];
     CoinZeroN(work, numberRows_);
     const double * diagonalSlack = diagonal + numberColumns;
     int newDropped = 0;
     double largest;
     //double smallest;
     //perturbation
     double perturbation = model_->diagonalPerturbation() * model_->diagonalNorm();
     perturbation = 0.0;
     perturbation = perturbation * perturbation;
     if (perturbation > 1.0) {
#ifdef COIN_DEVELOP
          //if (model_->model()->logLevel()&4)
          std::cout << "large perturbation " << perturbation << std::endl;
#endif
          perturbation = sqrt(perturbation);;
          perturbation = 1.0;
     }
     double delta2 = model_->delta(); // add delta*delta to diagonal
     delta2 *= delta2;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          double * put = sparseFactor_ + choleskyStart_[iRow];
          int * which = choleskyRow_ + choleskyStart_[iRow];
          int number = choleskyStart_[iRow+1] - choleskyStart_[iRow];
          if (!rowLength[iRow])
               rowsDropped_[iRow] = 1;
          if (!rowsDropped_[iRow]) {
               CoinBigIndex startRow = rowStart[iRow];
               CoinBigIndex endRow = rowStart[iRow] + rowLength[iRow];
               work[iRow] = diagonalSlack[iRow] + delta2;
               for (CoinBigIndex k = startRow; k < endRow; k++) {
                    int iColumn = column[k];
                    if (!whichDense_ || !whichDense_[iColumn]) {
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         double multiplier = diagonal[iColumn] * elementByRow[k];
                         for (CoinBigIndex j = start; j < end; j++) {
                              int jRow = row[j];
                              if (jRow >= iRow && !rowsDropped_[jRow]) {
                                   double value = element[j] * multiplier;
                                   work[jRow] += value;
                              }
                         }
                    }
               }
               int j;
               for (j = 0; j < number; j++) {
                    int jRow = which[j];
                    put[j] = work[jRow];
                    work[jRow] = 0.0;
               }
          } else {
               // dropped
               int j;
               for (j = 1; j < number; j++) {
                    put[j] = 0.0;
               }
               put[0] = 1.0;
          }
     }
     //check sizes
     double largest2 = maximumAbsElement(sparseFactor_, sizeFactor_);
     largest2 *= 1.0e-20;
     largest = CoinMin(largest2, 1.0e-11);
     int numberDroppedBefore = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int dropped = rowsDropped_[iRow];
          // Move to int array
          rowsDropped[iRow] = dropped;
          if (!dropped) {
               CoinBigIndex start = choleskyStart_[iRow];
               double diagonal = sparseFactor_[start];
               if (diagonal > largest2) {
                    sparseFactor_[start] = CoinMax(diagonal, 1.0e-10);
               } else {
                    sparseFactor_[start] = CoinMax(diagonal, 1.0e-10);
                    rowsDropped[iRow] = 2;
                    numberDroppedBefore++;
               }
          }
     }
     delete [] work;
     cholmod_sparse A ;
     A.nrow = numberRows_;
     A.ncol = numberRows_;
     A.nzmax = choleskyStart_[numberRows_];
     A.p = choleskyStart_;
     A.i = choleskyRow_;
     A.x = sparseFactor_;
     A.stype = -1;
     A.itype = CHOLMOD_INT;
     A.xtype = CHOLMOD_REAL;
     A.dtype = CHOLMOD_DOUBLE;
     A.sorted = 1;
     A.packed = 1;
     cholmod_factorize (&A, L_, c_) ;		    /* factorize */
     choleskyCondition_ = 1.0;
     bool cleanCholesky;
     if (model_->numberIterations() < 2000)
          cleanCholesky = true;
     else
          cleanCholesky = false;
     if (cleanCholesky) {
          //drop fresh makes some formADAT easier
          //int oldDropped=numberRowsDropped_;
          if (newDropped || numberRowsDropped_) {
               //std::cout <<"Rank "<<numberRows_-newDropped<<" ( "<<
               //  newDropped<<" dropped)";
               //if (newDropped>oldDropped)
               //std::cout<<" ( "<<newDropped-oldDropped<<" dropped this time)";
               //std::cout<<std::endl;
               newDropped = 0;
               for (int i = 0; i < numberRows_; i++) {
                    int dropped = rowsDropped[i];
                    rowsDropped_[i] = (char)dropped;
                    if (dropped == 2) {
                         //dropped this time
                         rowsDropped[newDropped++] = i;
                         rowsDropped_[i] = 0;
                    }
               }
               numberRowsDropped_ = newDropped;
               newDropped = -(2 + newDropped);
          }
     } else {
          if (newDropped) {
               newDropped = 0;
               for (int i = 0; i < numberRows_; i++) {
                    int dropped = rowsDropped[i];
                    rowsDropped_[i] = (char)dropped;
                    if (dropped == 2) {
                         //dropped this time
                         rowsDropped[newDropped++] = i;
                         rowsDropped_[i] = 1;
                    }
               }
          }
          numberRowsDropped_ += newDropped;
          if (numberRowsDropped_ && 0) {
               std::cout << "Rank " << numberRows_ - numberRowsDropped_ << " ( " <<
                         numberRowsDropped_ << " dropped)";
               if (newDropped) {
                    std::cout << " ( " << newDropped << " dropped this time)";
               }
               std::cout << std::endl;
          }
     }
     status_ = 0;
     return newDropped;
}
#else
/* Factorize - filling in rowsDropped and returning number dropped */
int
ClpCholeskyUfl::factorize(const double * diagonal, int * rowsDropped)
{
  return ClpCholeskyBase::factorize(diagonal, rowsDropped);
}
#endif

#ifdef COIN_HAS_CHOLMOD
/* Uses factorization to solve. */
void
ClpCholeskyUfl::solve (double * region)
{
     cholmod_dense *x, *b;
     b = cholmod_allocate_dense (numberRows_, 1, numberRows_, CHOLMOD_REAL, c_) ;
     CoinMemcpyN(region, numberRows_, (double *) b->x);
     x = cholmod_solve (CHOLMOD_A, L_, b, c_) ;
     CoinMemcpyN((double *) x->x, numberRows_, region);
     cholmod_free_dense (&x, c_) ;
     cholmod_free_dense (&b, c_) ;
}
#else
void
ClpCholeskyUfl::solve (double * region)
{
  ClpCholeskyBase::solve(region);
}
#endif
