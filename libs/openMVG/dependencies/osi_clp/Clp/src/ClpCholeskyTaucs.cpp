/* $Id$ */
#ifdef TAUCS_BARRIER
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"

#include "ClpInterior.hpp"
#include "ClpCholeskyTaucs.hpp"
#include "ClpMessage.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpCholeskyTaucs::ClpCholeskyTaucs ()
     : ClpCholeskyBase(),
       matrix_(NULL),
       factorization_(NULL),
       sparseFactorT_(NULL),
       choleskyStartT_(NULL),
       choleskyRowT_(NULL),
       sizeFactorT_(0),
       rowCopyT_(NULL)
{
     type_ = 13;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpCholeskyTaucs::ClpCholeskyTaucs (const ClpCholeskyTaucs & rhs)
     : ClpCholeskyBase(rhs)
{
     type_ = rhs.type_;
     // For Taucs stuff is done by malloc
     matrix_ = rhs.matrix_;
     sizeFactorT_ = rhs.sizeFactorT_;
     if (matrix_) {
          choleskyStartT_ = (int *) malloc((numberRows_ + 1) * sizeof(int));
          CoinMemcpyN(rhs.choleskyStartT_, (numberRows_ + 1), choleskyStartT_);
          choleskyRowT_ = (int *) malloc(sizeFactorT_ * sizeof(int));
          CoinMemcpyN(rhs.choleskyRowT_, sizeFactorT_, choleskyRowT_);
          sparseFactorT_ = (double *) malloc(sizeFactorT_ * sizeof(double));
          CoinMemcpyN(rhs.sparseFactorT_, sizeFactorT_, sparseFactorT_);
          matrix_->colptr = choleskyStartT_;
          matrix_->rowind = choleskyRowT_;
          matrix_->values.d = sparseFactorT_;
     } else {
          sparseFactorT_ = NULL;
          choleskyStartT_ = NULL;
          choleskyRowT_ = NULL;
     }
     factorization_ = NULL,
     rowCopyT_ = rhs.rowCopyT_->clone();
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpCholeskyTaucs::~ClpCholeskyTaucs ()
{
     taucs_ccs_free(matrix_);
     if (factorization_)
          taucs_supernodal_factor_free(factorization_);
     delete rowCopyT_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpCholeskyTaucs &
ClpCholeskyTaucs::operator=(const ClpCholeskyTaucs& rhs)
{
     if (this != &rhs) {
          ClpCholeskyBase::operator=(rhs);
          taucs_ccs_free(matrix_);
          if (factorization_)
               taucs_supernodal_factor_free(factorization_);
          factorization_ = NULL;
          sizeFactorT_ = rhs.sizeFactorT_;
          matrix_ = rhs.matrix_;
          if (matrix_) {
               choleskyStartT_ = (int *) malloc((numberRows_ + 1) * sizeof(int));
               CoinMemcpyN(rhs.choleskyStartT_, (numberRows_ + 1), choleskyStartT_);
               choleskyRowT_ = (int *) malloc(sizeFactorT_ * sizeof(int));
               CoinMemcpyN(rhs.choleskyRowT_, sizeFactorT_, choleskyRowT_);
               sparseFactorT_ = (double *) malloc(sizeFactorT_ * sizeof(double));
               CoinMemcpyN(rhs.sparseFactorT_, sizeFactorT_, sparseFactorT_);
               matrix_->colptr = choleskyStartT_;
               matrix_->rowind = choleskyRowT_;
               matrix_->values.d = sparseFactorT_;
          } else {
               sparseFactorT_ = NULL;
               choleskyStartT_ = NULL;
               choleskyRowT_ = NULL;
          }
          delete rowCopyT_;
          rowCopyT_ = rhs.rowCopyT_->clone();
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpCholeskyBase * ClpCholeskyTaucs::clone() const
{
     return new ClpCholeskyTaucs(*this);
}
/* Orders rows and saves pointer to matrix.and model */
int
ClpCholeskyTaucs::order(ClpInterior * model)
{
     numberRows_ = model->numberRows();
     rowsDropped_ = new char [numberRows_];
     memset(rowsDropped_, 0, numberRows_);
     numberRowsDropped_ = 0;
     model_ = model;
     rowCopyT_ = model->clpMatrix()->reverseOrderedCopy();
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const CoinBigIndex * rowStart = rowCopyT_->getVectorStarts();
     const int * rowLength = rowCopyT_->getVectorLengths();
     const int * column = rowCopyT_->getIndices();
     // We need two arrays for counts
     int * which = new int [numberRows_];
     int * used = new int[numberRows_];
     CoinZeroN(used, numberRows_);
     int iRow;
     sizeFactorT_ = 0;
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
               sizeFactorT_ += number;
               int j;
               for (j = 0; j < number; j++)
                    used[which[j]] = 0;
          }
     }
     delete [] which;
     // Now we have size - create arrays and fill in
     matrix_ = taucs_ccs_create(numberRows_, numberRows_, sizeFactorT_,
                                TAUCS_DOUBLE | TAUCS_SYMMETRIC | TAUCS_LOWER);
     if (!matrix_)
          return 1;
     // Space for starts
     choleskyStartT_ = matrix_->colptr;
     choleskyRowT_ = matrix_->rowind;
     sparseFactorT_ = matrix_->values.d;
     sizeFactorT_ = 0;
     which = choleskyRowT_;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int number = 1;
          // make sure diagonal exists
          which[0] = iRow;
          used[iRow] = 1;
          choleskyStartT_[iRow] = sizeFactorT_;
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
               sizeFactorT_ += number;
               int j;
               for (j = 0; j < number; j++)
                    used[which[j]] = 0;
               // Sort
               std::sort(which, which + number);
               // move which on
               which += number;
          }
     }
     choleskyStartT_[numberRows_] = sizeFactorT_;
     delete [] used;
     permuteInverse_ = new int [numberRows_];
     permute_ = new int[numberRows_];
     int * perm, *invp;
     // There seem to be bugs in ordering if model too small
     if (numberRows_ > 10)
          taucs_ccs_order(matrix_, &perm, &invp, (const char *) "genmmd");
     else
          taucs_ccs_order(matrix_, &perm, &invp, (const char *) "identity");
     CoinMemcpyN(perm, numberRows_, permuteInverse_);
     free(perm);
     CoinMemcpyN(invp, numberRows_, permute_);
     free(invp);
     // need to permute
     taucs_ccs_matrix * permuted = taucs_ccs_permute_symmetrically(matrix_, permuteInverse_, permute_);
     // symbolic
     factorization_ = taucs_ccs_factor_llt_symbolic(permuted);
     taucs_ccs_free(permuted);
     return factorization_ ? 0 : 1;
}
/* Does Symbolic factorization given permutation.
   This is called immediately after order.  If user provides this then
   user must provide factorize and solve.  Otherwise the default factorization is used
   returns non-zero if not enough memory */
int
ClpCholeskyTaucs::symbolic()
{
     return 0;
}
/* Factorize - filling in rowsDropped and returning number dropped */
int
ClpCholeskyTaucs::factorize(const double * diagonal, int * rowsDropped)
{
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const double * element = model_->clpMatrix()->getElements();
     const CoinBigIndex * rowStart = rowCopyT_->getVectorStarts();
     const int * rowLength = rowCopyT_->getVectorLengths();
     const int * column = rowCopyT_->getIndices();
     const double * elementByRow = rowCopyT_->getElements();
     int numberColumns = model_->clpMatrix()->getNumCols();
     int iRow;
     double * work = new double[numberRows_];
     CoinZeroN(work, numberRows_);
     const double * diagonalSlack = diagonal + numberColumns;
     int newDropped = 0;
     double largest;
     //perturbation
     double perturbation = model_->diagonalPerturbation() * model_->diagonalNorm();
     perturbation = perturbation * perturbation;
     if (perturbation > 1.0) {
          //if (model_->model()->logLevel()&4)
          std::cout << "large perturbation " << perturbation << std::endl;
          perturbation = sqrt(perturbation);;
          perturbation = 1.0;
     }
     for (iRow = 0; iRow < numberRows_; iRow++) {
          double * put = sparseFactorT_ + choleskyStartT_[iRow];
          int * which = choleskyRowT_ + choleskyStartT_[iRow];
          int number = choleskyStartT_[iRow+1] - choleskyStartT_[iRow];
          if (!rowLength[iRow])
               rowsDropped_[iRow] = 1;
          if (!rowsDropped_[iRow]) {
               CoinBigIndex startRow = rowStart[iRow];
               CoinBigIndex endRow = rowStart[iRow] + rowLength[iRow];
               work[iRow] = diagonalSlack[iRow];
               for (CoinBigIndex k = startRow; k < endRow; k++) {
                    int iColumn = column[k];
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
     double largest2 = maximumAbsElement(sparseFactorT_, sizeFactorT_);
     largest2 *= 1.0e-19;
     largest = CoinMin(largest2, 1.0e-11);
     int numberDroppedBefore = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int dropped = rowsDropped_[iRow];
          // Move to int array
          rowsDropped[iRow] = dropped;
          if (!dropped) {
               CoinBigIndex start = choleskyStartT_[iRow];
               double diagonal = sparseFactorT_[start];
               if (diagonal > largest2) {
                    sparseFactorT_[start] = diagonal + perturbation;
               } else {
                    sparseFactorT_[start] = diagonal + perturbation;
                    rowsDropped[iRow] = 2;
                    numberDroppedBefore++;
               }
          }
     }
     taucs_supernodal_factor_free_numeric(factorization_);
     // need to permute
     taucs_ccs_matrix * permuted = taucs_ccs_permute_symmetrically(matrix_, permuteInverse_, permute_);
     int rCode = taucs_ccs_factor_llt_numeric(permuted, factorization_);
     taucs_ccs_free(permuted);
     if (rCode)
          printf("return code of %d from factor\n", rCode);
     delete [] work;
     choleskyCondition_ = 1.0;
     bool cleanCholesky;
     if (model_->numberIterations() < 200)
          cleanCholesky = true;
     else
          cleanCholesky = false;
     /*
       How do I find out where 1.0e100's are in cholesky?
     */
     if (cleanCholesky) {
          //drop fresh makes some formADAT easier
          int oldDropped = numberRowsDropped_;
          if (newDropped || numberRowsDropped_) {
               std::cout << "Rank " << numberRows_ - newDropped << " ( " <<
                         newDropped << " dropped)";
               if (newDropped > oldDropped)
                    std::cout << " ( " << newDropped - oldDropped << " dropped this time)";
               std::cout << std::endl;
               newDropped = 0;
               for (int i = 0; i < numberRows_; i++) {
                    char dropped = rowsDropped[i];
                    rowsDropped_[i] = dropped;
                    if (dropped == 2) {
                         //dropped this time
                         rowsDropped[newDropped++] = i;
                         rowsDropped_[i] = 0;
                    }
               }
               numberRowsDropped_ = newDropped;
               newDropped = -(1 + newDropped);
          }
     } else {
          if (newDropped) {
               newDropped = 0;
               for (int i = 0; i < numberRows_; i++) {
                    char dropped = rowsDropped[i];
                    int oldDropped = rowsDropped_[i];;
                    rowsDropped_[i] = dropped;
                    if (dropped == 2) {
                         assert (!oldDropped);
                         //dropped this time
                         rowsDropped[newDropped++] = i;
                         rowsDropped_[i] = 1;
                    }
               }
          }
          numberRowsDropped_ += newDropped;
          if (numberRowsDropped_) {
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
ClpCholeskyTaucs::solve (double * region)
{
     double * in = new double[numberRows_];
     double * out = new double[numberRows_];
     taucs_vec_permute(numberRows_, TAUCS_DOUBLE, region, in, permuteInverse_);
     int rCode = taucs_supernodal_solve_llt(factorization_, out, in);
     if (rCode)
          printf("return code of %d from solve\n", rCode);
     taucs_vec_permute(numberRows_, TAUCS_DOUBLE, out, region, permute_);
     delete [] out;
     delete [] in;
}
#endif
