/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include "CoinPragma.hpp"

#include "ClpConfig.h"

#define MPI_COMM_WORLD CLP_MPI_COMM_WORLD
#define JOB_INIT -1
#define JOB_END -2
#define USE_COMM_WORLD -987654
extern "C" {
#include "dmumps_c.h"
#include "mpi.h"
}

#include "ClpCholeskyMumps.hpp"
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
ClpCholeskyMumps::ClpCholeskyMumps (int denseThreshold)
     : ClpCholeskyBase(denseThreshold)
{
     mumps_ = (DMUMPS_STRUC_C*)malloc(sizeof(DMUMPS_STRUC_C));
     type_ = 16;
     mumps_->n = 0;
     mumps_->nz = 0;
     mumps_->a = NULL;
     mumps_->jcn = NULL;
     mumps_->irn = NULL;
     mumps_->job = JOB_INIT;//initialize mumps
     mumps_->par = 1;//working host for sequential version
     mumps_->sym = 2;//general symmetric matrix
     mumps_->comm_fortran = USE_COMM_WORLD;
     int myid;
     int justName;
     MPI_Init(&justName, NULL);
#ifndef NDEBUG
     int ierr = MPI_Comm_rank(MPI_COMM_WORLD, &myid);
     assert (!ierr);
#else
     MPI_Comm_rank(MPI_COMM_WORLD, &myid);
#endif
     dmumps_c(mumps_);
#define ICNTL(I) icntl[(I)-1] /* macro s.t. indices match documentation */
#define CNTL(I) cntl[(I)-1] /* macro s.t. indices match documentation */
     mumps_->ICNTL(5) = 1; // say compressed format
     mumps_->ICNTL(4) = 2; // log messages
     mumps_->ICNTL(24) = 1; // Deal with zeros on diagonal
     mumps_->CNTL(3) = 1.0e-20; // drop if diagonal less than this
     // output off
     mumps_->ICNTL(1) = -1;
     mumps_->ICNTL(2) = -1;
     mumps_->ICNTL(3) = -1;
     mumps_->ICNTL(4) = 0;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpCholeskyMumps::ClpCholeskyMumps (const ClpCholeskyMumps & rhs)
     : ClpCholeskyBase(rhs)
{
     abort();
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpCholeskyMumps::~ClpCholeskyMumps ()
{
     mumps_->job = JOB_END;
     dmumps_c(mumps_); /* Terminate instance */
     MPI_Finalize();
     free(mumps_);
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpCholeskyMumps &
ClpCholeskyMumps::operator=(const ClpCholeskyMumps& rhs)
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
ClpCholeskyBase * ClpCholeskyMumps::clone() const
{
     return new ClpCholeskyMumps(*this);
}
/* Orders rows and saves pointer to matrix.and model */
int
ClpCholeskyMumps::order(ClpInterior * model)
{
     numberRows_ = model->numberRows();
     if (doKKT_) {
          numberRows_ += numberRows_ + model->numberColumns();
          printf("finish coding MUMPS KKT!\n");
          abort();
     }
     rowsDropped_ = new char [numberRows_];
     memset(rowsDropped_, 0, numberRows_);
     numberRowsDropped_ = 0;
     model_ = model;
     rowCopy_ = model->clpMatrix()->reverseOrderedCopy();
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
     // NOT COMPRESSED FOR NOW ??? - Space for starts
     mumps_->ICNTL(5) = 0; // say NOT compressed format
     try {
          choleskyStart_ = new CoinBigIndex[numberRows_+1+sizeFactor_];
     } catch (...) {
          // no memory
          return -1;
     }
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
     // To Fortran and fake
     for (iRow = 0; iRow < numberRows_ + 1; iRow++) {
          int k = choleskyStart_[iRow];
          int kEnd = choleskyStart_[iRow+1];
          k += numberRows_ + 1;
          kEnd += numberRows_ + 1;
          for (; k < kEnd; k++)
               choleskyStart_[k] = iRow + 1;
          choleskyStart_[iRow]++;
     }
     mumps_->nz = sizeFactor_;
     mumps_->irn = choleskyStart_ + numberRows_ + 1;
     mumps_->jcn = choleskyRow_;
     mumps_->a = NULL;
     for (CoinBigIndex i = 0; i < sizeFactor_; i++) {
          choleskyRow_[i]++;
#ifndef NDEBUG
          assert (mumps_->irn[i] >= 1 && mumps_->irn[i] <= numberRows_);
          assert (mumps_->jcn[i] >= 1 && mumps_->jcn[i] <= numberRows_);
#endif
     }
     // validate
     //mumps code here
     mumps_->n = numberRows_;
     mumps_->nelt = numberRows_;
     mumps_->eltptr = choleskyStart_;
     mumps_->eltvar = choleskyRow_;
     mumps_->a_elt = NULL;
     mumps_->rhs = NULL;
     mumps_->job = 1; // order
     dmumps_c(mumps_);
     mumps_->a = sparseFactor_;
     if (mumps_->infog[0]) {
          COIN_DETAIL_PRINT(printf("MUMPS ordering failed -error %d %d\n",
				   mumps_->infog[0], mumps_->infog[1]));
          return 1;
     } else {
          double size = mumps_->infog[19];
          if (size < 0)
               size *= -1000000;
          COIN_DETAIL_PRINT(printf("%g nonzeros, flop count %g\n", size, mumps_->rinfog[0]));
     }
     for (iRow = 0; iRow < numberRows_; iRow++) {
          permuteInverse_[iRow] = iRow;
          permute_[iRow] = iRow;
     }
     return 0;
}
/* Does Symbolic factorization given permutation.
   This is called immediately after order.  If user provides this then
   user must provide factorize and solve.  Otherwise the default factorization is used
   returns non-zero if not enough memory */
int
ClpCholeskyMumps::symbolic()
{
     return 0;
}
/* Factorize - filling in rowsDropped and returning number dropped */
int
ClpCholeskyMumps::factorize(const double * diagonal, int * rowsDropped)
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
          double * put = sparseFactor_ + choleskyStart_[iRow] - 1; // Fortran
          int * which = choleskyRow_ + choleskyStart_[iRow] - 1; // Fortran
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
                    int jRow = which[j] - 1; // to Fortran
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
     int numberDroppedBefore = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int dropped = rowsDropped_[iRow];
          // Move to int array
          rowsDropped[iRow] = dropped;
          if (!dropped) {
               CoinBigIndex start = choleskyStart_[iRow] - 1; // to Fortran
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
     // code here
     mumps_->a_elt = sparseFactor_;
     mumps_->rhs = NULL;
     mumps_->job = 2; // factorize
     dmumps_c(mumps_);
     if (mumps_->infog[0]) {
          COIN_DETAIL_PRINT(printf("MUMPS factorization failed -error %d %d\n",
				   mumps_->infog[0], mumps_->infog[1]));
     }
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
/* Uses factorization to solve. */
void
ClpCholeskyMumps::solve (double * region)
{
     mumps_->rhs = region;
     mumps_->job = 3; // solve
     dmumps_c(mumps_);
}
