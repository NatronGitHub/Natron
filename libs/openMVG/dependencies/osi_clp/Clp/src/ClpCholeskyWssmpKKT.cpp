/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"

#include "ClpInterior.hpp"
#include "ClpCholeskyWssmpKKT.hpp"
#include "ClpQuadraticObjective.hpp"
#include "ClpMessage.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpCholeskyWssmpKKT::ClpCholeskyWssmpKKT (int denseThreshold)
     : ClpCholeskyBase(denseThreshold)
{
     type_ = 21;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpCholeskyWssmpKKT::ClpCholeskyWssmpKKT (const ClpCholeskyWssmpKKT & rhs)
     : ClpCholeskyBase(rhs)
{
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpCholeskyWssmpKKT::~ClpCholeskyWssmpKKT ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpCholeskyWssmpKKT &
ClpCholeskyWssmpKKT::operator=(const ClpCholeskyWssmpKKT& rhs)
{
     if (this != &rhs) {
          ClpCholeskyBase::operator=(rhs);
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpCholeskyBase * ClpCholeskyWssmpKKT::clone() const
{
     return new ClpCholeskyWssmpKKT(*this);
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

/* Orders rows and saves pointer to model */
int
ClpCholeskyWssmpKKT::order(ClpInterior * model)
{
     int numberRowsModel = model->numberRows();
     int numberColumns = model->numberColumns();
     int numberTotal = numberColumns + numberRowsModel;
     numberRows_ = 2 * numberRowsModel + numberColumns;
     rowsDropped_ = new char [numberRows_];
     memset(rowsDropped_, 0, numberRows_);
     numberRowsDropped_ = 0;
     model_ = model;
     CoinPackedMatrix * quadratic = NULL;
     ClpQuadraticObjective * quadraticObj =
          (dynamic_cast< ClpQuadraticObjective*>(model_->objectiveAsObject()));
     if (quadraticObj)
          quadratic = quadraticObj->quadraticObjective();
     int numberElements = model_->clpMatrix()->getNumElements();
     numberElements = numberElements + 2 * numberRowsModel + numberTotal;
     if (quadratic)
          numberElements += quadratic->getNumElements();
     // Space for starts
     choleskyStart_ = new CoinBigIndex[numberRows_+1];
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     //const double * element = model_->clpMatrix()->getElements();
     // Now we have size - create arrays and fill in
     try {
          choleskyRow_ = new int [numberElements];
     } catch (...) {
          // no memory
          delete [] choleskyStart_;
          choleskyStart_ = NULL;
          return -1;
     }
     try {
          sparseFactor_ = new double[numberElements];
     } catch (...) {
          // no memory
          delete [] choleskyRow_;
          choleskyRow_ = NULL;
          delete [] choleskyStart_;
          choleskyStart_ = NULL;
          return -1;
     }
     int iRow, iColumn;

     sizeFactor_ = 0;
     // matrix
     if (!quadratic) {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               choleskyStart_[iColumn] = sizeFactor_;
               choleskyRow_[sizeFactor_++] = iColumn;
               CoinBigIndex start = columnStart[iColumn];
               CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
               for (CoinBigIndex j = start; j < end; j++) {
                    choleskyRow_[sizeFactor_++] = row[j] + numberTotal;
               }
          }
     } else {
          // Quadratic
          const int * columnQuadratic = quadratic->getIndices();
          const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
          const int * columnQuadraticLength = quadratic->getVectorLengths();
          //const double * quadraticElement = quadratic->getElements();
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               choleskyStart_[iColumn] = sizeFactor_;
               choleskyRow_[sizeFactor_++] = iColumn;
               for (CoinBigIndex j = columnQuadraticStart[iColumn];
                         j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                    int jColumn = columnQuadratic[j];
                    if (jColumn > iColumn)
                         choleskyRow_[sizeFactor_++] = jColumn;
               }
               CoinBigIndex start = columnStart[iColumn];
               CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
               for (CoinBigIndex j = start; j < end; j++) {
                    choleskyRow_[sizeFactor_++] = row[j] + numberTotal;
               }
          }
     }
     // slacks
     for (; iColumn < numberTotal; iColumn++) {
          choleskyStart_[iColumn] = sizeFactor_;
          choleskyRow_[sizeFactor_++] = iColumn;
          choleskyRow_[sizeFactor_++] = iColumn - numberColumns + numberTotal;
     }
     // Transpose - nonzero diagonal (may regularize)
     for (iRow = 0; iRow < numberRowsModel; iRow++) {
          choleskyStart_[iRow+numberTotal] = sizeFactor_;
          // diagonal
          choleskyRow_[sizeFactor_++] = iRow + numberTotal;
     }
     choleskyStart_[numberRows_] = sizeFactor_;
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
#if 1
     integerParameters_[1] = 2; //just symbolic
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          permuteInverse_[iRow] = iRow;
          permute_[iRow] = iRow;
     }
#endif
     F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
           NULL, permute_, permuteInverse_, NULL, &numberRows_, &i1,
           NULL, &i0, NULL, integerParameters_, doubleParameters_);
     //std::cout<<"Ordering and symbolic factorization took "<<doubleParameters_[0]<<std::endl;
     if (integerParameters_[63]) {
          std::cout << "wssmp returning error code of " << integerParameters_[63] << std::endl;
          return 1;
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
ClpCholeskyWssmpKKT::symbolic()
{
     return 0;
}
/* Factorize - filling in rowsDropped and returning number dropped */
int
ClpCholeskyWssmpKKT::factorize(const double * diagonal, int * rowsDropped)
{
     int numberRowsModel = model_->numberRows();
     int numberColumns = model_->numberColumns();
     int numberTotal = numberColumns + numberRowsModel;
     int newDropped = 0;
     double largest = 0.0;
     double smallest;
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
     // need to recreate every time
     int iRow, iColumn;
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const double * element = model_->clpMatrix()->getElements();

     CoinBigIndex numberElements = 0;
     CoinPackedMatrix * quadratic = NULL;
     ClpQuadraticObjective * quadraticObj =
          (dynamic_cast< ClpQuadraticObjective*>(model_->objectiveAsObject()));
     if (quadraticObj)
          quadratic = quadraticObj->quadraticObjective();
     // matrix
     if (!quadratic) {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               choleskyStart_[iColumn] = numberElements;
               double value = diagonal[iColumn];
               if (fabs(value) > 1.0e-100) {
                    value = 1.0 / value;
                    largest = CoinMax(largest, fabs(value));
                    sparseFactor_[numberElements] = -value;
                    choleskyRow_[numberElements++] = iColumn;
                    CoinBigIndex start = columnStart[iColumn];
                    CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                    for (CoinBigIndex j = start; j < end; j++) {
                         choleskyRow_[numberElements] = row[j] + numberTotal;
                         sparseFactor_[numberElements++] = element[j];
                         largest = CoinMax(largest, fabs(element[j]));
                    }
               } else {
                    sparseFactor_[numberElements] = -1.0e100;
                    choleskyRow_[numberElements++] = iColumn;
               }
          }
     } else {
          // Quadratic
          const int * columnQuadratic = quadratic->getIndices();
          const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
          const int * columnQuadraticLength = quadratic->getVectorLengths();
          const double * quadraticElement = quadratic->getElements();
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               choleskyStart_[iColumn] = numberElements;
               CoinBigIndex savePosition = numberElements;
               choleskyRow_[numberElements++] = iColumn;
               double value = diagonal[iColumn];
               if (fabs(value) > 1.0e-100) {
                    value = 1.0 / value;
                    for (CoinBigIndex j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         if (jColumn > iColumn) {
                              sparseFactor_[numberElements] = -quadraticElement[j];
                              choleskyRow_[numberElements++] = jColumn;
                         } else if (iColumn == jColumn) {
                              value += quadraticElement[j];
                         }
                    }
                    largest = CoinMax(largest, fabs(value));
                    sparseFactor_[savePosition] = -value;
                    CoinBigIndex start = columnStart[iColumn];
                    CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                    for (CoinBigIndex j = start; j < end; j++) {
                         choleskyRow_[numberElements] = row[j] + numberTotal;
                         sparseFactor_[numberElements++] = element[j];
                         largest = CoinMax(largest, fabs(element[j]));
                    }
               } else {
                    value = 1.0e100;
                    sparseFactor_[savePosition] = -value;
               }
          }
     }
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          assert (sparseFactor_[choleskyStart_[iColumn]] < 0.0);
     }
     // slacks
     for (iColumn = numberColumns; iColumn < numberTotal; iColumn++) {
          choleskyStart_[iColumn] = numberElements;
          double value = diagonal[iColumn];
          if (fabs(value) > 1.0e-100) {
               value = 1.0 / value;
               largest = CoinMax(largest, fabs(value));
          } else {
               value = 1.0e100;
          }
          sparseFactor_[numberElements] = -value;
          choleskyRow_[numberElements++] = iColumn;
          choleskyRow_[numberElements] = iColumn - numberColumns + numberTotal;
          sparseFactor_[numberElements++] = -1.0;
     }
     // Finish diagonal
     double delta2 = model_->delta(); // add delta*delta to bottom
     delta2 *= delta2;
     for (iRow = 0; iRow < numberRowsModel; iRow++) {
          choleskyStart_[iRow+numberTotal] = numberElements;
          choleskyRow_[numberElements] = iRow + numberTotal;
          sparseFactor_[numberElements++] = delta2;
     }
     choleskyStart_[numberRows_] = numberElements;
     int i1 = 1;
     int i0 = 0;
     integerParameters_[1] = 3;
     integerParameters_[2] = 3;
     integerParameters_[10] = 2;
     //integerParameters_[11]=1;
     integerParameters_[12] = 2;
     // LDLT
     integerParameters_[30] = 1;
     doubleParameters_[20] = 1.0e100;
     double largest2 = largest * 1.0e-20;
     largest = CoinMin(largest2, 1.0e-11);
     doubleParameters_[10] = CoinMax(1.0e-20, largest);
     if (doubleParameters_[10] > 1.0e-3)
          integerParameters_[9] = 1;
     else
          integerParameters_[9] = 0;
#ifndef WSMP
     // Set up LDL cutoff
     integerParameters_[34] = numberTotal;
     doubleParameters_[20] = 1.0e-15;
     doubleParameters_[34] = 1.0e-12;
     //printf("tol is %g\n",doubleParameters_[10]);
     //doubleParameters_[10]=1.0e-17;
#endif
     int * rowsDropped2 = new int[numberRows_];
     CoinZeroN(rowsDropped2, numberRows_);
     F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
           NULL, permute_, permuteInverse_, NULL, &numberRows_, &i1,
           NULL, &i0, rowsDropped2, integerParameters_, doubleParameters_);
     //std::cout<<"factorization took "<<doubleParameters_[0]<<std::endl;
     if (integerParameters_[9]) {
          std::cout << "scaling applied" << std::endl;
     }
     newDropped = integerParameters_[20];
#if 1
     // Should save adjustments in ..R_
     int n1 = 0, n2 = 0;
     double * primalR = model_->primalR();
     double * dualR = model_->dualR();
     for (iRow = 0; iRow < numberTotal; iRow++) {
          if (rowsDropped2[iRow]) {
               n1++;
               //printf("row region1 %d dropped\n",iRow);
               //rowsDropped_[iRow]=1;
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
               //printf("row region2 %d dropped\n",iRow);
               //rowsDropped_[iRow]=1;
               rowsDropped_[iRow] = 0;
               dualR[iRow-numberTotal] = doubleParameters_[34];
          } else {
               rowsDropped_[iRow] = 0;
               dualR[iRow-numberTotal] = 0.0;
          }
     }
     //printf("%d rows dropped in region1, %d in region2\n",n1,n2);
#endif
     delete [] rowsDropped2;
     //if (integerParameters_[20])
     //std::cout<<integerParameters_[20]<<" rows dropped"<<std::endl;
     largest = doubleParameters_[3];
     smallest = doubleParameters_[4];
     if (model_->messageHandler()->logLevel() > 1)
          std::cout << "Cholesky - largest " << largest << " smallest " << smallest << std::endl;
     choleskyCondition_ = largest / smallest;
     if (integerParameters_[63] < 0)
          return -1; // out of memory
     status_ = 0;
     return 0;
}
/* Uses factorization to solve. */
void
ClpCholeskyWssmpKKT::solve (double * region)
{
     abort();
}
/* Uses factorization to solve. */
void
ClpCholeskyWssmpKKT::solveKKT (double * region1, double * region2, const double * diagonal,
                               double diagonalScaleFactor)
{
     int numberRowsModel = model_->numberRows();
     int numberColumns = model_->numberColumns();
     int numberTotal = numberColumns + numberRowsModel;
     double * array = new double [numberRows_];
     CoinMemcpyN(region1, numberTotal, array);
     CoinMemcpyN(region2, numberRowsModel, array + numberTotal);
     int i1 = 1;
     int i0 = 0;
     integerParameters_[1] = 4;
     integerParameters_[2] = 4;
#if 0
     integerParameters_[5] = 3;
     doubleParameters_[5] = 1.0e-10;
     integerParameters_[6] = 6;
#endif
     F77_FUNC(wssmp,WSSMP)(&numberRows_, choleskyStart_, choleskyRow_, sparseFactor_,
           NULL, permute_, permuteInverse_, array, &numberRows_, &i1,
           NULL, &i0, NULL, integerParameters_, doubleParameters_);
#if 0
     int iRow;
     for (iRow = 0; iRow < numberTotal; iRow++) {
          if (rowsDropped_[iRow] && fabs(array[iRow]) > 1.0e-8) {
               printf("row region1 %d dropped %g\n", iRow, array[iRow]);
          }
     }
     for (; iRow < numberRows_; iRow++) {
          if (rowsDropped_[iRow] && fabs(array[iRow]) > 1.0e-8) {
               printf("row region2 %d dropped %g\n", iRow, array[iRow]);
          }
     }
#endif
     CoinMemcpyN(array + numberTotal, numberRowsModel, region2);
#if 1
     CoinMemcpyN(array, numberTotal, region1);
#else
     multiplyAdd(region2, numberRowsModel, -1.0, array + numberColumns, 0.0);
     CoinZeroN(array, numberColumns);
     model_->clpMatrix()->transposeTimes(1.0, region2, array);
     for (int iColumn = 0; iColumn < numberTotal; iColumn++)
          region1[iColumn] = diagonal[iColumn] * (array[iColumn] - region1[iColumn]);
#endif
     delete [] array;
#if 0
     if (integerParameters_[5]) {
          std::cout << integerParameters_[5] << " refinements ";
     }
     std::cout << doubleParameters_[6] << std::endl;
#endif
}
