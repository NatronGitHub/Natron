/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"

#include "ClpInterior.hpp"
#include "ClpCholeskyWssmp.hpp"
#include "ClpCholeskyDense.hpp"
#include "ClpMessage.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpCholeskyWssmp::ClpCholeskyWssmp (int denseThreshold)
     : ClpCholeskyBase(denseThreshold)
{
     type_ = 12;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpCholeskyWssmp::ClpCholeskyWssmp (const ClpCholeskyWssmp & rhs)
     : ClpCholeskyBase(rhs)
{
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpCholeskyWssmp::~ClpCholeskyWssmp ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpCholeskyWssmp &
ClpCholeskyWssmp::operator=(const ClpCholeskyWssmp& rhs)
{
     if (this != &rhs) {
          ClpCholeskyBase::operator=(rhs);
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpCholeskyBase * ClpCholeskyWssmp::clone() const
{
     return new ClpCholeskyWssmp(*this);
}
// At present I can't get wssmp to work as my libraries seem to be out of sync
// so I have linked in ekkwssmp which is an older version
#ifndef USE_EKKWSSMP
extern "C" {
void F77_FUNC(wsetmaxthrds,WSETMAXTHRDS)(const int* NTHREADS);

void F77_FUNC(wssmp,WSSMP)(const int* N, const int* IA,
                           const int* JA, const double* AVALS,
                           double* DIAG,  int* PERM,
                           int* INVP,  double* B,   
                           const int* LDB, const int* NRHS,
                           double* AUX, const int* NAUX,
                           int* MRP, int* IPARM,
                           double* DPARM);
void F77_FUNC_(wsmp_clear,WSMP_CLEAR)(void);
}
#else
/* minimum needed for user */
typedef struct EKKModel EKKModel;
typedef struct EKKContext EKKContext;


extern "C" {
     EKKContext *  ekk_initializeContext();
     void ekk_endContext(EKKContext * context);
     EKKModel *  ekk_newModel(EKKContext * env, const char * name);
     int ekk_deleteModel(EKKModel * model);
}
static  EKKModel * model = NULL;
static  EKKContext * context = NULL;
extern "C" void ekkwssmp(EKKModel *, int * n,
                         int * columnStart , int * rowIndex , double * element,
                         double * diagonal , int * perm , int * invp ,
                         double * rhs , int * ldb , int * nrhs ,
                         double * aux , int * naux ,
                         int   * mrp , int * iparm , double * dparm);
static void F77_FUNC(wssmp,WSSMP)( int *n, int *ia, int *ja,
                   double *avals, double *diag, int *perm, int *invp,
                   double *b, int *ldb, int *nrhs, double *aux, int *
                   naux, int *mrp, int *iparm, double *dparm)
{
     if (!context) {
          /* initialize OSL environment */
          context = ekk_initializeContext();
          model = ekk_newModel(context, "");
     }
     ekkwssmp(model, n, ia, ja,
              avals, diag, perm, invp,
              b, ldb, nrhs, aux,
              naux, mrp, iparm, dparm);
     //ekk_deleteModel(model);
     //ekk_endContext(context);
}
#endif
/* Orders rows and saves pointer to matrix.and model */
int
ClpCholeskyWssmp::order(ClpInterior * model)
{
     numberRows_ = model->numberRows();
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
     int numberColumns = model->numberColumns();
     int numberDense = 0;
     if (denseThreshold_ > 0) {
          delete [] whichDense_;
          delete [] denseColumn_;
          delete dense_;
          whichDense_ = new char[numberColumns];
          int iColumn;
          used[numberRows_] = 0;
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               int length = columnLength[iColumn];
               used[length] += 1;
          }
          int nLong = 0;
          int stop = CoinMax(denseThreshold_ / 2, 100);
          for (iRow = numberRows_; iRow >= stop; iRow--) {
               if (used[iRow])
		 COIN_DETAIL_PRINT(printf("%d columns are of length %d\n", used[iRow], iRow));
               nLong += used[iRow];
               if (nLong > 50 || nLong > (numberColumns >> 2))
                    break;
          }
          CoinZeroN(used, numberRows_);
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (columnLength[iColumn] < denseThreshold_) {
                    whichDense_[iColumn] = 0;
               } else {
                    whichDense_[iColumn] = 1;
                    numberDense++;
               }
          }
          if (!numberDense || numberDense > 100) {
               // free
               delete [] whichDense_;
               whichDense_ = NULL;
               denseColumn_ = NULL;
               dense_ = NULL;
          } else {
               // space for dense columns
               denseColumn_ = new double [numberDense*numberRows_];
               // dense cholesky
               dense_ = new ClpCholeskyDense();
               dense_->reserveSpace(NULL, numberDense);
               COIN_DETAIL_PRINT(printf("Taking %d columns as dense\n", numberDense));
          }
     }
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
                    if (!whichDense_ || !whichDense_[iColumn]) {
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
                    if (!whichDense_ || !whichDense_[iColumn]) {
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
     integerParameters_[0] = 0;
     int i0 = 0;
     int i1 = 1;
#ifndef USE_EKKWSSMP
     int i2 = 1;
     if (model->numberThreads() <= 0)
          i2 = 1;
     else
          i2 = model->numberThreads();
     F77_FUNC(wsetmaxthrds,WSETMAXTHRDS)(&i2);
#endif
     F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
           NULL, permute_, permuteInverse_, 0, &numberRows_, &i1,
           NULL, &i0, NULL, integerParameters_, doubleParameters_);
     integerParameters_[1] = 1; //order and symbolic
     integerParameters_[2] = 2;
     integerParameters_[3] = 0; //CSR
     integerParameters_[4] = 0; //C style
     integerParameters_[13] = 1; //reuse initial factorization space
     integerParameters_[15+0] = 1; //ordering
     integerParameters_[15+1] = 0;
     integerParameters_[15+2] = 1;
     integerParameters_[15+3] = 0;
     integerParameters_[15+4] = 1;
     doubleParameters_[10] = 1.0e-20;
     doubleParameters_[11] = 1.0e-15;
     F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
           NULL, permute_, permuteInverse_, NULL, &numberRows_, &i1,
           NULL, &i0, NULL, integerParameters_, doubleParameters_);
     //std::cout<<"Ordering and symbolic factorization took "<<doubleParameters_[0]<<std::endl;
     if (integerParameters_[63]) {
          std::cout << "wssmp returning error code of " << integerParameters_[63] << std::endl;
          return 1;
     }
     // Modify gamma and delta if 0.0
     if (!model->gamma() && !model->delta()) {
          model->setGamma(5.0e-5);
          model->setDelta(5.0e-5);
     }
     std::cout << integerParameters_[23] << " elements in sparse Cholesky" << std::endl;
     if (!integerParameters_[23]) {
          for (int iRow = 0; iRow < numberRows_; iRow++) {
               permuteInverse_[iRow] = iRow;
               permute_[iRow] = iRow;
          }
          std::cout << "wssmp says no elements - fully dense? - switching to dense" << std::endl;
          integerParameters_[1] = 2;
          integerParameters_[2] = 2;
          integerParameters_[7] = 1; // no permute
          F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
                NULL, permute_, permuteInverse_, NULL, &numberRows_, &i1,
                NULL, &i0, NULL, integerParameters_, doubleParameters_);
          std::cout << integerParameters_[23] << " elements in dense Cholesky" << std::endl;
     }
     return 0;
}
/* Does Symbolic factorization given permutation.
   This is called immediately after order.  If user provides this then
   user must provide factorize and solve.  Otherwise the default factorization is used
   returns non-zero if not enough memory */
int
ClpCholeskyWssmp::symbolic()
{
     return 0;
}
/* Factorize - filling in rowsDropped and returning number dropped */
int
ClpCholeskyWssmp::factorize(const double * diagonal, int * rowsDropped)
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
     double smallest;
     int numberDense = 0;
     if (dense_)
          numberDense = dense_->numberRows();
     //perturbation
     double perturbation = model_->diagonalPerturbation() * model_->diagonalNorm();
     perturbation = perturbation * perturbation;
     if (perturbation > 1.0) {
#ifdef COIN_DEVELOP
          //if (model_->model()->logLevel()&4)
          std::cout << "large perturbation " << perturbation << std::endl;
#endif
          perturbation = sqrt(perturbation);;
          perturbation = 1.0;
     }
     if (whichDense_) {
          double * denseDiagonal = dense_->diagonal();
          double * dense = denseColumn_;
          int iDense = 0;
          for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (whichDense_[iColumn]) {
                    denseDiagonal[iDense++] = 1.0 / diagonal[iColumn];
                    CoinZeroN(dense, numberRows_);
                    CoinBigIndex start = columnStart[iColumn];
                    CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                    for (CoinBigIndex j = start; j < end; j++) {
                         int jRow = row[j];
                         dense[jRow] = element[j];
                    }
                    dense += numberRows_;
               }
          }
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
                    sparseFactor_[start] = diagonal + perturbation;
               } else {
                    sparseFactor_[start] = diagonal + perturbation;
                    rowsDropped[iRow] = 2;
                    numberDroppedBefore++;
               }
          }
     }
     int i1 = 1;
     int i0 = 0;
     integerParameters_[1] = 3;
     integerParameters_[2] = 3;
     integerParameters_[10] = 2;
     //integerParameters_[11]=1;
     integerParameters_[12] = 2;
     doubleParameters_[10] = CoinMax(1.0e-20, largest);
     if (doubleParameters_[10] > 1.0e-3)
          integerParameters_[9] = 1;
     else
          integerParameters_[9] = 0;
     F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
           NULL, permute_, permuteInverse_, NULL, &numberRows_, &i1,
           NULL, &i0, rowsDropped, integerParameters_, doubleParameters_);
     //std::cout<<"factorization took "<<doubleParameters_[0]<<std::endl;
     if (integerParameters_[9]) {
          std::cout << "scaling applied" << std::endl;
     }
     newDropped = integerParameters_[20] + numberDroppedBefore;
     //if (integerParameters_[20])
     //std::cout<<integerParameters_[20]<<" rows dropped"<<std::endl;
     largest = doubleParameters_[3];
     smallest = doubleParameters_[4];
     delete [] work;
     if (model_->messageHandler()->logLevel() > 1)
          std::cout << "Cholesky - largest " << largest << " smallest " << smallest << std::endl;
     choleskyCondition_ = largest / smallest;
     if (integerParameters_[63] < 0)
          return -1; // out of memory
     if (whichDense_) {
          // Update dense columns (just L)
          // Zero out dropped rows
          int i;
          for (i = 0; i < numberDense; i++) {
               double * a = denseColumn_ + i * numberRows_;
               for (int j = 0; j < numberRows_; j++) {
                    if (rowsDropped[j])
                         a[j] = 0.0;
               }
          }
          integerParameters_[29] = 1;
          int i0 = 0;
          integerParameters_[1] = 4;
          integerParameters_[2] = 4;
          F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
                NULL, permute_, permuteInverse_, denseColumn_, &numberRows_, &numberDense,
                NULL, &i0, NULL, integerParameters_, doubleParameters_);
          integerParameters_[29] = 0;
          dense_->resetRowsDropped();
          longDouble * denseBlob = dense_->aMatrix();
          double * denseDiagonal = dense_->diagonal();
          // Update dense matrix
          for (i = 0; i < numberDense; i++) {
               const double * a = denseColumn_ + i * numberRows_;
               // do diagonal
               double value = denseDiagonal[i];
               const double * b = denseColumn_ + i * numberRows_;
               for (int k = 0; k < numberRows_; k++)
                    value += a[k] * b[k];
               denseDiagonal[i] = value;
               for (int j = i + 1; j < numberDense; j++) {
                    double value = 0.0;
                    const double * b = denseColumn_ + j * numberRows_;
                    for (int k = 0; k < numberRows_; k++)
                         value += a[k] * b[k];
                    *denseBlob = value;
                    denseBlob++;
               }
          }
          // dense cholesky (? long double)
          int * dropped = new int [numberDense];
          dense_->factorizePart2(dropped);
          delete [] dropped;
     }
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
                    char dropped = static_cast<char>(rowsDropped[i]);
                    rowsDropped_[i] = dropped;
                    rowsDropped_[i] = 0;
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
                    char dropped = static_cast<char>(rowsDropped[i]);
                    rowsDropped_[i] = dropped;
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
ClpCholeskyWssmp::solve (double * region)
{
     int i1 = 1;
     int i0 = 0;
     integerParameters_[1] = 4;
     integerParameters_[2] = 4;
#if 1
     integerParameters_[5] = 3;
     doubleParameters_[5] = 1.0e-10;
     integerParameters_[6] = 6;
#endif
     if (!whichDense_) {
          F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
                NULL, permute_, permuteInverse_, region, &numberRows_, &i1,
                NULL, &i0, NULL, integerParameters_, doubleParameters_);
     } else {
          // dense columns
          integerParameters_[29] = 1;
          F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
                NULL, permute_, permuteInverse_, region, &numberRows_, &i1,
                NULL, &i0, NULL, integerParameters_, doubleParameters_);
          // do change;
          int numberDense = dense_->numberRows();
          double * change = new double[numberDense];
          int i;
          for (i = 0; i < numberDense; i++) {
               const double * a = denseColumn_ + i * numberRows_;
               double value = 0.0;
               for (int iRow = 0; iRow < numberRows_; iRow++)
                    value += a[iRow] * region[iRow];
               change[i] = value;
          }
          // solve
          dense_->solve(change);
          for (i = 0; i < numberDense; i++) {
               const double * a = denseColumn_ + i * numberRows_;
               double value = change[i];
               for (int iRow = 0; iRow < numberRows_; iRow++)
                    region[iRow] -= value * a[iRow];
          }
          delete [] change;
          // and finish off
          integerParameters_[29] = 2;
          integerParameters_[1] = 4;
          F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
                NULL, permute_, permuteInverse_, region, &numberRows_, &i1,
                NULL, &i0, NULL, integerParameters_, doubleParameters_);
          integerParameters_[29] = 0;
     }
#if 0
     if (integerParameters_[5]) {
          std::cout << integerParameters_[5] << " refinements ";
     }
     std::cout << doubleParameters_[6] << std::endl;
#endif
}
