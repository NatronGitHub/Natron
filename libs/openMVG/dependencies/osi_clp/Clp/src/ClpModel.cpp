/* $Id$ */
// copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cfloat>
#include <string>
#include <cstdio>
#include <iostream>

/*
  CLP_NO_VECTOR

  There's no hint of the motivation for this, so here's a bit of speculation.
  CLP_NO_VECTOR excises CoinPackedVector from the code. Looking over
  affected code here, and the much more numerous occurrences of affected
  code in CoinUtils, it looks to be an efficiency issue.

  One good example is CoinPackedMatrix.isEquivalent. The default version
  tests equivalence of major dimension vectors by retrieving them as
  CPVs and invoking CPV.isEquivalent. As pointed out in the documention,
  CPV.isEquivalent implicitly sorts the nonzeros of each vector (insertion in
  a map) prior to comparison. As a one-off, this is arguably more efficient
  than allocating and clearing a full vector, then dropping in the nonzeros,
  then checking against the second vector (in fact, this is the algorithm
  for testing a packed vector against a full vector).

  In CPM.isEquivalent, we have a whole sequence of vectors to compare. Better
  to allocate a full vector sized to match, clear it (one time cost), then
  edit nonzeros in and out while doing comparisons. The cost of allocating
  and clearing the full vector is amortised over all columns.
*/


#include "CoinPragma.hpp"

#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "ClpModel.hpp"
#include "ClpEventHandler.hpp"
#include "ClpPackedMatrix.hpp"
#ifndef SLIM_CLP
#include "ClpPlusMinusOneMatrix.hpp"
#endif
#ifndef CLP_NO_VECTOR
#include "CoinPackedVector.hpp"
#endif
#include "CoinIndexedVector.hpp"
#if SLIM_CLP==2
#define SLIM_NOIO
#endif
#ifndef SLIM_NOIO
#include "CoinMpsIO.hpp"
#include "CoinFileIO.hpp"
#include "CoinModel.hpp"
#endif
#include "ClpMessage.hpp"
#include "CoinMessage.hpp"
#include "ClpLinearObjective.hpp"
#ifndef SLIM_CLP
#include "ClpQuadraticObjective.hpp"
#include "CoinBuild.hpp"
#endif

//#############################################################################
ClpModel::ClpModel (bool emptyMessages) :

     optimizationDirection_(1),
     objectiveValue_(0.0),
     smallElement_(1.0e-20),
     objectiveScale_(1.0),
     rhsScale_(1.0),
     numberRows_(0),
     numberColumns_(0),
     rowActivity_(NULL),
     columnActivity_(NULL),
     dual_(NULL),
     reducedCost_(NULL),
     rowLower_(NULL),
     rowUpper_(NULL),
     objective_(NULL),
     rowObjective_(NULL),
     columnLower_(NULL),
     columnUpper_(NULL),
     matrix_(NULL),
     rowCopy_(NULL),
     scaledMatrix_(NULL),
     ray_(NULL),
     rowScale_(NULL),
     columnScale_(NULL),
     inverseRowScale_(NULL),
     inverseColumnScale_(NULL),
     scalingFlag_(3),
     status_(NULL),
     integerType_(NULL),
     userPointer_(NULL),
     trustedUserPointer_(NULL),
     numberIterations_(0),
     solveType_(0),
     whatsChanged_(0),
     problemStatus_(-1),
     secondaryStatus_(0),
     lengthNames_(0),
     numberThreads_(0),
     specialOptions_(0),
#ifndef CLP_NO_STD
     defaultHandler_(true),
     rowNames_(),
     columnNames_(),
#else
     defaultHandler_(true),
#endif
     maximumColumns_(-1),
     maximumRows_(-1),
     maximumInternalColumns_(-1),
     maximumInternalRows_(-1),
     savedRowScale_(NULL),
     savedColumnScale_(NULL)
{
     intParam_[ClpMaxNumIteration] = 2147483647;
     intParam_[ClpMaxNumIterationHotStart] = 9999999;
     intParam_[ClpNameDiscipline] = 1;

     dblParam_[ClpDualObjectiveLimit] = COIN_DBL_MAX;
     dblParam_[ClpPrimalObjectiveLimit] = COIN_DBL_MAX;
     dblParam_[ClpDualTolerance] = 1e-7;
     dblParam_[ClpPrimalTolerance] = 1e-7;
     dblParam_[ClpObjOffset] = 0.0;
     dblParam_[ClpMaxSeconds] = -1.0;
     dblParam_[ClpMaxWallSeconds] = -1.0;
     dblParam_[ClpPresolveTolerance] = 1.0e-8;

#ifndef CLP_NO_STD
     strParam_[ClpProbName] = "ClpDefaultName";
#endif
     handler_ = new CoinMessageHandler();
     handler_->setLogLevel(1);
     eventHandler_ = new ClpEventHandler();
     if (!emptyMessages) {
          messages_ = ClpMessage();
          coinMessages_ = CoinMessage();
     }
     randomNumberGenerator_.setSeed(1234567);
}

//-----------------------------------------------------------------------------

ClpModel::~ClpModel ()
{
     if (defaultHandler_) {
          delete handler_;
          handler_ = NULL;
     }
     gutsOfDelete(0);
}
// Does most of deletion (0 = all, 1 = most)
void
ClpModel::gutsOfDelete(int type)
{
     if (!type || !permanentArrays()) {
          maximumRows_ = -1;
          maximumColumns_ = -1;
          delete [] rowActivity_;
          rowActivity_ = NULL;
          delete [] columnActivity_;
          columnActivity_ = NULL;
          delete [] dual_;
          dual_ = NULL;
          delete [] reducedCost_;
          reducedCost_ = NULL;
          delete [] rowLower_;
          delete [] rowUpper_;
          delete [] rowObjective_;
          rowLower_ = NULL;
          rowUpper_ = NULL;
          rowObjective_ = NULL;
          delete [] columnLower_;
          delete [] columnUpper_;
          delete objective_;
          columnLower_ = NULL;
          columnUpper_ = NULL;
          objective_ = NULL;
          delete [] savedRowScale_;
          if (rowScale_ == savedRowScale_)
               rowScale_ = NULL;
          savedRowScale_ = NULL;
          delete [] savedColumnScale_;
          if (columnScale_ == savedColumnScale_)
               columnScale_ = NULL;
          savedColumnScale_ = NULL;
          delete [] rowScale_;
          rowScale_ = NULL;
          delete [] columnScale_;
          columnScale_ = NULL;
          delete [] integerType_;
          integerType_ = NULL;
          delete [] status_;
          status_ = NULL;
          delete eventHandler_;
          eventHandler_ = NULL;
     }
     whatsChanged_ = 0;
     delete matrix_;
     matrix_ = NULL;
     delete rowCopy_;
     rowCopy_ = NULL;
     delete scaledMatrix_;
     scaledMatrix_ = NULL,
     delete [] ray_;
     ray_ = NULL;
     specialOptions_ = 0;
}
void
ClpModel::setRowScale(double * scale)
{
     if (!savedRowScale_) {
          delete [] reinterpret_cast<double *> (rowScale_);
          rowScale_ = scale;
     } else {
          assert (!scale);
          rowScale_ = NULL;
     }
}
void
ClpModel::setColumnScale(double * scale)
{
     if (!savedColumnScale_) {
          delete [] reinterpret_cast<double *> (columnScale_);
          columnScale_ = scale;
     } else {
          assert (!scale);
          columnScale_ = NULL;
     }
}
//#############################################################################
void ClpModel::setPrimalTolerance( double value)
{
     if (value > 0.0 && value < 1.0e10)
          dblParam_[ClpPrimalTolerance] = value;
}
void ClpModel::setDualTolerance( double value)
{
     if (value > 0.0 && value < 1.0e10)
          dblParam_[ClpDualTolerance] = value;
}
void ClpModel::setOptimizationDirection( double value)
{
     optimizationDirection_ = value;
}
void
ClpModel::gutsOfLoadModel (int numberRows, int numberColumns,
                           const double* collb, const double* colub,
                           const double* obj,
                           const double* rowlb, const double* rowub,
                           const double * rowObjective)
{
     // save event handler in case already set
     ClpEventHandler * handler = eventHandler_->clone();
     // Save specialOptions
     int saveOptions = specialOptions_;
     gutsOfDelete(0);
     specialOptions_ = saveOptions;
     eventHandler_ = handler;
     numberRows_ = numberRows;
     numberColumns_ = numberColumns;
     rowActivity_ = new double[numberRows_];
     columnActivity_ = new double[numberColumns_];
     dual_ = new double[numberRows_];
     reducedCost_ = new double[numberColumns_];

     CoinZeroN(dual_, numberRows_);
     CoinZeroN(reducedCost_, numberColumns_);
     int iRow, iColumn;

     rowLower_ = ClpCopyOfArray(rowlb, numberRows_, -COIN_DBL_MAX);
     rowUpper_ = ClpCopyOfArray(rowub, numberRows_, COIN_DBL_MAX);
     double * objective = ClpCopyOfArray(obj, numberColumns_, 0.0);
     objective_ = new ClpLinearObjective(objective, numberColumns_);
     delete [] objective;
     rowObjective_ = ClpCopyOfArray(rowObjective, numberRows_);
     columnLower_ = ClpCopyOfArray(collb, numberColumns_, 0.0);
     columnUpper_ = ClpCopyOfArray(colub, numberColumns_, COIN_DBL_MAX);
     // set default solution and clean bounds
     for (iRow = 0; iRow < numberRows_; iRow++) {
          if (rowLower_[iRow] > 0.0) {
               rowActivity_[iRow] = rowLower_[iRow];
          } else if (rowUpper_[iRow] < 0.0) {
               rowActivity_[iRow] = rowUpper_[iRow];
          } else {
               rowActivity_[iRow] = 0.0;
          }
          if (rowLower_[iRow] < -1.0e27)
               rowLower_[iRow] = -COIN_DBL_MAX;
          if (rowUpper_[iRow] > 1.0e27)
               rowUpper_[iRow] = COIN_DBL_MAX;
     }
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          if (columnLower_[iColumn] > 0.0) {
               columnActivity_[iColumn] = columnLower_[iColumn];
          } else if (columnUpper_[iColumn] < 0.0) {
               columnActivity_[iColumn] = columnUpper_[iColumn];
          } else {
               columnActivity_[iColumn] = 0.0;
          }
          if (columnLower_[iColumn] < -1.0e27)
               columnLower_[iColumn] = -COIN_DBL_MAX;
          if (columnUpper_[iColumn] > 1.0e27)
               columnUpper_[iColumn] = COIN_DBL_MAX;
     }
}
// This just loads up a row objective
void ClpModel::setRowObjective(const double * rowObjective)
{
     delete [] rowObjective_;
     rowObjective_ = ClpCopyOfArray(rowObjective, numberRows_);
     whatsChanged_ = 0;
}
void
ClpModel::loadProblem (  const ClpMatrixBase& matrix,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective)
{
     gutsOfLoadModel(matrix.getNumRows(), matrix.getNumCols(),
                     collb, colub, obj, rowlb, rowub, rowObjective);
     if (matrix.isColOrdered()) {
          matrix_ = matrix.clone();
     } else {
          // later may want to keep as unknown class
          CoinPackedMatrix matrix2;
          matrix2.setExtraGap(0.0);
          matrix2.setExtraMajor(0.0);
          matrix2.reverseOrderedCopyOf(*matrix.getPackedMatrix());
          matrix.releasePackedMatrix();
          matrix_ = new ClpPackedMatrix(matrix2);
     }
     matrix_->setDimensions(numberRows_, numberColumns_);
}
void
ClpModel::loadProblem (  const CoinPackedMatrix& matrix,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective)
{
     ClpPackedMatrix* clpMatrix =
          dynamic_cast< ClpPackedMatrix*>(matrix_);
     bool special = (clpMatrix) ? clpMatrix->wantsSpecialColumnCopy() : false;
     gutsOfLoadModel(matrix.getNumRows(), matrix.getNumCols(),
                     collb, colub, obj, rowlb, rowub, rowObjective);
     if (matrix.isColOrdered()) {
          matrix_ = new ClpPackedMatrix(matrix);
          if (special) {
               clpMatrix = static_cast< ClpPackedMatrix*>(matrix_);
               clpMatrix->makeSpecialColumnCopy();
          }
     } else {
          CoinPackedMatrix matrix2;
          matrix2.setExtraGap(0.0);
          matrix2.setExtraMajor(0.0);
          matrix2.reverseOrderedCopyOf(matrix);
          matrix_ = new ClpPackedMatrix(matrix2);
     }
     matrix_->setDimensions(numberRows_, numberColumns_);
}
void
ClpModel::loadProblem (
     const int numcols, const int numrows,
     const CoinBigIndex* start, const int* index,
     const double* value,
     const double* collb, const double* colub,
     const double* obj,
     const double* rowlb, const double* rowub,
     const double * rowObjective)
{
     gutsOfLoadModel(numrows, numcols,
                     collb, colub, obj, rowlb, rowub, rowObjective);
     int numberElements = start ? start[numcols] : 0;
     CoinPackedMatrix matrix(true, numrows, numrows ? numcols : 0, numberElements,
                             value, index, start, NULL);
     matrix_ = new ClpPackedMatrix(matrix);
     matrix_->setDimensions(numberRows_, numberColumns_);
}
void
ClpModel::loadProblem (
     const int numcols, const int numrows,
     const CoinBigIndex* start, const int* index,
     const double* value, const int* length,
     const double* collb, const double* colub,
     const double* obj,
     const double* rowlb, const double* rowub,
     const double * rowObjective)
{
     gutsOfLoadModel(numrows, numcols,
                     collb, colub, obj, rowlb, rowub, rowObjective);
     // Compute number of elements
     int numberElements = 0;
     int i;
     for (i = 0; i < numcols; i++)
          numberElements += length[i];
     CoinPackedMatrix matrix(true, numrows, numcols, numberElements,
                             value, index, start, length);
     matrix_ = new ClpPackedMatrix(matrix);
}
#ifndef SLIM_NOIO
// This loads a model from a coinModel object - returns number of errors
int
ClpModel::loadProblem (  CoinModel & modelObject, bool tryPlusMinusOne)
{
     if (modelObject.numberColumns() == 0 && modelObject.numberRows() == 0)
          return 0;
     int numberErrors = 0;
     // Set arrays for normal use
     double * rowLower = modelObject.rowLowerArray();
     double * rowUpper = modelObject.rowUpperArray();
     double * columnLower = modelObject.columnLowerArray();
     double * columnUpper = modelObject.columnUpperArray();
     double * objective = modelObject.objectiveArray();
     int * integerType = modelObject.integerTypeArray();
     double * associated = modelObject.associatedArray();
     // If strings then do copies
     if (modelObject.stringsExist()) {
          numberErrors = modelObject.createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                                  objective, integerType, associated);
     }
     int numberRows = modelObject.numberRows();
     int numberColumns = modelObject.numberColumns();
     gutsOfLoadModel(numberRows, numberColumns,
                     columnLower, columnUpper, objective, rowLower, rowUpper, NULL);
     setObjectiveOffset(modelObject.objectiveOffset());
     CoinBigIndex * startPositive = NULL;
     CoinBigIndex * startNegative = NULL;
     delete matrix_;
     if (tryPlusMinusOne) {
          startPositive = new CoinBigIndex[numberColumns+1];
          startNegative = new CoinBigIndex[numberColumns];
          modelObject.countPlusMinusOne(startPositive, startNegative, associated);
          if (startPositive[0] < 0) {
               // no good
               tryPlusMinusOne = false;
               delete [] startPositive;
               delete [] startNegative;
          }
     }
#ifndef SLIM_CLP
     if (!tryPlusMinusOne) {
#endif
          CoinPackedMatrix matrix;
          modelObject.createPackedMatrix(matrix, associated);
          matrix_ = new ClpPackedMatrix(matrix);
#ifndef SLIM_CLP
     } else {
          // create +-1 matrix
          CoinBigIndex size = startPositive[numberColumns];
          int * indices = new int[size];
          modelObject.createPlusMinusOne(startPositive, startNegative, indices,
                                         associated);
          // Get good object
          ClpPlusMinusOneMatrix * matrix = new ClpPlusMinusOneMatrix();
          matrix->passInCopy(numberRows, numberColumns,
                             true, indices, startPositive, startNegative);
          matrix_ = matrix;
     }
#endif
#ifndef CLP_NO_STD
     // Do names if wanted
     int numberItems;
     numberItems = modelObject.rowNames()->numberItems();
     if (numberItems) {
          const char *const * rowNames = modelObject.rowNames()->names();
          copyRowNames(rowNames, 0, numberItems);
     }
     numberItems = modelObject.columnNames()->numberItems();
     if (numberItems) {
          const char *const * columnNames = modelObject.columnNames()->names();
          copyColumnNames(columnNames, 0, numberItems);
     }
#endif
     // Do integers if wanted
     assert(integerType);
     for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
          if (integerType[iColumn])
               setInteger(iColumn);
     }
     if (rowLower != modelObject.rowLowerArray() ||
               columnLower != modelObject.columnLowerArray()) {
          delete [] rowLower;
          delete [] rowUpper;
          delete [] columnLower;
          delete [] columnUpper;
          delete [] objective;
          delete [] integerType;
          delete [] associated;
          if (numberErrors)
               handler_->message(CLP_BAD_STRING_VALUES, messages_)
                         << numberErrors
                         << CoinMessageEol;
     }
     matrix_->setDimensions(numberRows_, numberColumns_);
     return numberErrors;
}
#endif
void
ClpModel::getRowBound(int iRow, double& lower, double& upper) const
{
     lower = -COIN_DBL_MAX;
     upper = COIN_DBL_MAX;
     if (rowUpper_)
          upper = rowUpper_[iRow];
     if (rowLower_)
          lower = rowLower_[iRow];
}
//------------------------------------------------------------------
#ifndef NDEBUG
// For errors to make sure print to screen
// only called in debug mode
static void indexError(int index,
                       std::string methodName)
{
     std::cerr << "Illegal index " << index << " in ClpModel::" << methodName << std::endl;
     throw CoinError("Illegal index", methodName, "ClpModel");
}
#endif
/* Set an objective function coefficient */
void
ClpModel::setObjectiveCoefficient( int elementIndex, double elementValue )
{
#ifndef NDEBUG
     if (elementIndex < 0 || elementIndex >= numberColumns_) {
          indexError(elementIndex, "setObjectiveCoefficient");
     }
#endif
     objective()[elementIndex] = elementValue;
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
}
/* Set a single row lower bound<br>
   Use -DBL_MAX for -infinity. */
void
ClpModel::setRowLower( int elementIndex, double elementValue )
{
     if (elementValue < -1.0e27)
          elementValue = -COIN_DBL_MAX;
     rowLower_[elementIndex] = elementValue;
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
}

/* Set a single row upper bound<br>
   Use DBL_MAX for infinity. */
void
ClpModel::setRowUpper( int elementIndex, double elementValue )
{
     if (elementValue > 1.0e27)
          elementValue = COIN_DBL_MAX;
     rowUpper_[elementIndex] = elementValue;
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
}

/* Set a single row lower and upper bound */
void
ClpModel::setRowBounds( int elementIndex,
                        double lower, double upper )
{
     if (lower < -1.0e27)
          lower = -COIN_DBL_MAX;
     if (upper > 1.0e27)
          upper = COIN_DBL_MAX;
     CoinAssert (upper >= lower);
     rowLower_[elementIndex] = lower;
     rowUpper_[elementIndex] = upper;
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
}
void ClpModel::setRowSetBounds(const int* indexFirst,
                               const int* indexLast,
                               const double* boundList)
{
#ifndef NDEBUG
     int n = numberRows_;
#endif
     double * lower = rowLower_;
     double * upper = rowUpper_;
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
     while (indexFirst != indexLast) {
          const int iRow = *indexFirst++;
#ifndef NDEBUG
          if (iRow < 0 || iRow >= n) {
               indexError(iRow, "setRowSetBounds");
          }
#endif
          lower[iRow] = *boundList++;
          upper[iRow] = *boundList++;
          if (lower[iRow] < -1.0e27)
               lower[iRow] = -COIN_DBL_MAX;
          if (upper[iRow] > 1.0e27)
               upper[iRow] = COIN_DBL_MAX;
          CoinAssert (upper[iRow] >= lower[iRow]);
     }
}
//-----------------------------------------------------------------------------
/* Set a single column lower bound<br>
   Use -DBL_MAX for -infinity. */
void
ClpModel::setColumnLower( int elementIndex, double elementValue )
{
#ifndef NDEBUG
     int n = numberColumns_;
     if (elementIndex < 0 || elementIndex >= n) {
          indexError(elementIndex, "setColumnLower");
     }
#endif
     if (elementValue < -1.0e27)
          elementValue = -COIN_DBL_MAX;
     columnLower_[elementIndex] = elementValue;
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
}

/* Set a single column upper bound<br>
   Use DBL_MAX for infinity. */
void
ClpModel::setColumnUpper( int elementIndex, double elementValue )
{
#ifndef NDEBUG
     int n = numberColumns_;
     if (elementIndex < 0 || elementIndex >= n) {
          indexError(elementIndex, "setColumnUpper");
     }
#endif
     if (elementValue > 1.0e27)
          elementValue = COIN_DBL_MAX;
     columnUpper_[elementIndex] = elementValue;
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
}

/* Set a single column lower and upper bound */
void
ClpModel::setColumnBounds( int elementIndex,
                           double lower, double upper )
{
#ifndef NDEBUG
     int n = numberColumns_;
     if (elementIndex < 0 || elementIndex >= n) {
          indexError(elementIndex, "setColumnBounds");
     }
#endif
     if (lower < -1.0e27)
          lower = -COIN_DBL_MAX;
     if (upper > 1.0e27)
          upper = COIN_DBL_MAX;
     columnLower_[elementIndex] = lower;
     columnUpper_[elementIndex] = upper;
     CoinAssert (upper >= lower);
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
}
void ClpModel::setColumnSetBounds(const int* indexFirst,
                                  const int* indexLast,
                                  const double* boundList)
{
     double * lower = columnLower_;
     double * upper = columnUpper_;
     whatsChanged_ = 0; // Can't be sure (use ClpSimplex to keep)
#ifndef NDEBUG
     int n = numberColumns_;
#endif
     while (indexFirst != indexLast) {
          const int iColumn = *indexFirst++;
#ifndef NDEBUG
          if (iColumn < 0 || iColumn >= n) {
               indexError(iColumn, "setColumnSetBounds");
          }
#endif
          lower[iColumn] = *boundList++;
          upper[iColumn] = *boundList++;
          CoinAssert (upper[iColumn] >= lower[iColumn]);
          if (lower[iColumn] < -1.0e27)
               lower[iColumn] = -COIN_DBL_MAX;
          if (upper[iColumn] > 1.0e27)
               upper[iColumn] = COIN_DBL_MAX;
     }
}
//-----------------------------------------------------------------------------
//#############################################################################
// Copy constructor.
ClpModel::ClpModel(const ClpModel &rhs, int scalingMode) :
     optimizationDirection_(rhs.optimizationDirection_),
     numberRows_(rhs.numberRows_),
     numberColumns_(rhs.numberColumns_),
     specialOptions_(rhs.specialOptions_),
     maximumColumns_(-1),
     maximumRows_(-1),
     maximumInternalColumns_(-1),
     maximumInternalRows_(-1),
     savedRowScale_(NULL),
     savedColumnScale_(NULL)
{
     gutsOfCopy(rhs);
     if (scalingMode >= 0 && matrix_ &&
               matrix_->allElementsInRange(this, smallElement_, 1.0e20)) {
          // really do scaling
          scalingFlag_ = scalingMode;
          setRowScale(NULL);
          setColumnScale(NULL);
          delete rowCopy_; // in case odd
          rowCopy_ = NULL;
          delete scaledMatrix_;
          scaledMatrix_ = NULL;
          if (scalingMode && !matrix_->scale(this)) {
               // scaling worked - now apply
               inverseRowScale_ = rowScale_ + numberRows_;
               inverseColumnScale_ = columnScale_ + numberColumns_;
               gutsOfScaling();
               // pretend not scaled
               scalingFlag_ = -scalingFlag_;
          } else {
               // not scaled
               scalingFlag_ = 0;
          }
     }
     //randomNumberGenerator_.setSeed(1234567);
}
// Assignment operator. This copies the data
ClpModel &
ClpModel::operator=(const ClpModel & rhs)
{
     if (this != &rhs) {
          if (defaultHandler_) {
               //delete handler_;
               //handler_ = NULL;
          }
          gutsOfDelete(1);
          optimizationDirection_ = rhs.optimizationDirection_;
          numberRows_ = rhs.numberRows_;
          numberColumns_ = rhs.numberColumns_;
          gutsOfCopy(rhs, -1);
     }
     return *this;
}
/* Does most of copying
   If trueCopy 0 then just points to arrays
   If -1 leaves as much as possible */
void
ClpModel::gutsOfCopy(const ClpModel & rhs, int trueCopy)
{
     defaultHandler_ = rhs.defaultHandler_;
     randomNumberGenerator_ = rhs.randomNumberGenerator_;
     if (trueCopy >= 0) {
          if (defaultHandler_)
               handler_ = new CoinMessageHandler(*rhs.handler_);
          else
               handler_ = rhs.handler_;
          eventHandler_ = rhs.eventHandler_->clone();
          messages_ = rhs.messages_;
          coinMessages_ = rhs.coinMessages_;
     } else {
          if (!eventHandler_ && rhs.eventHandler_)
               eventHandler_ = rhs.eventHandler_->clone();
     }
     intParam_[ClpMaxNumIteration] = rhs.intParam_[ClpMaxNumIteration];
     intParam_[ClpMaxNumIterationHotStart] =
          rhs.intParam_[ClpMaxNumIterationHotStart];
     intParam_[ClpNameDiscipline] = rhs.intParam_[ClpNameDiscipline] ;

     dblParam_[ClpDualObjectiveLimit] = rhs.dblParam_[ClpDualObjectiveLimit];
     dblParam_[ClpPrimalObjectiveLimit] = rhs.dblParam_[ClpPrimalObjectiveLimit];
     dblParam_[ClpDualTolerance] = rhs.dblParam_[ClpDualTolerance];
     dblParam_[ClpPrimalTolerance] = rhs.dblParam_[ClpPrimalTolerance];
     dblParam_[ClpObjOffset] = rhs.dblParam_[ClpObjOffset];
     dblParam_[ClpMaxSeconds] = rhs.dblParam_[ClpMaxSeconds];
     dblParam_[ClpMaxWallSeconds] = rhs.dblParam_[ClpMaxWallSeconds];
     dblParam_[ClpPresolveTolerance] = rhs.dblParam_[ClpPresolveTolerance];
#ifndef CLP_NO_STD

     strParam_[ClpProbName] = rhs.strParam_[ClpProbName];
#endif

     optimizationDirection_ = rhs.optimizationDirection_;
     objectiveValue_ = rhs.objectiveValue_;
     smallElement_ = rhs.smallElement_;
     objectiveScale_ = rhs.objectiveScale_;
     rhsScale_ = rhs.rhsScale_;
     numberIterations_ = rhs.numberIterations_;
     solveType_ = rhs.solveType_;
     whatsChanged_ = rhs.whatsChanged_;
     problemStatus_ = rhs.problemStatus_;
     secondaryStatus_ = rhs.secondaryStatus_;
     numberRows_ = rhs.numberRows_;
     numberColumns_ = rhs.numberColumns_;
     userPointer_ = rhs.userPointer_;
     trustedUserPointer_ = rhs.trustedUserPointer_;
     scalingFlag_ = rhs.scalingFlag_;
     specialOptions_ = rhs.specialOptions_;
     if (trueCopy) {
#ifndef CLP_NO_STD
          lengthNames_ = rhs.lengthNames_;
          if (lengthNames_) {
               rowNames_ = rhs.rowNames_;
               columnNames_ = rhs.columnNames_;
          }
#endif
          numberThreads_ = rhs.numberThreads_;
          if (maximumRows_ < 0) {
               specialOptions_ &= ~65536;
               savedRowScale_ = NULL;
               savedColumnScale_ = NULL;
               integerType_ = CoinCopyOfArray(rhs.integerType_, numberColumns_);
               rowActivity_ = ClpCopyOfArray(rhs.rowActivity_, numberRows_);
               columnActivity_ = ClpCopyOfArray(rhs.columnActivity_, numberColumns_);
               dual_ = ClpCopyOfArray(rhs.dual_, numberRows_);
               reducedCost_ = ClpCopyOfArray(rhs.reducedCost_, numberColumns_);
               rowLower_ = ClpCopyOfArray ( rhs.rowLower_, numberRows_ );
               rowUpper_ = ClpCopyOfArray ( rhs.rowUpper_, numberRows_ );
               columnLower_ = ClpCopyOfArray ( rhs.columnLower_, numberColumns_ );
               columnUpper_ = ClpCopyOfArray ( rhs.columnUpper_, numberColumns_ );
               rowScale_ = ClpCopyOfArray(rhs.rowScale_, numberRows_ * 2);
               columnScale_ = ClpCopyOfArray(rhs.columnScale_, numberColumns_ * 2);
               if (rhs.objective_)
                    objective_  = rhs.objective_->clone();
               else
                    objective_ = NULL;
               rowObjective_ = ClpCopyOfArray ( rhs.rowObjective_, numberRows_ );
               status_ = ClpCopyOfArray( rhs.status_, numberColumns_ + numberRows_);
               ray_ = NULL;
               if (problemStatus_ == 1)
                    ray_ = ClpCopyOfArray (rhs.ray_, numberRows_);
               else if (problemStatus_ == 2)
                    ray_ = ClpCopyOfArray (rhs.ray_, numberColumns_);
               if (rhs.rowCopy_) {
                    rowCopy_ = rhs.rowCopy_->clone();
               } else {
                    rowCopy_ = NULL;
               }
               if (rhs.scaledMatrix_) {
                    scaledMatrix_ = new ClpPackedMatrix(*rhs.scaledMatrix_);
               } else {
                    scaledMatrix_ = NULL;
               }
               matrix_ = NULL;
               if (rhs.matrix_) {
                    matrix_ = rhs.matrix_->clone();
               }
          } else {
               // This already has arrays - just copy
               savedRowScale_ = NULL;
               savedColumnScale_ = NULL;
               startPermanentArrays();
               if (rhs.integerType_) {
                    assert (integerType_);
                    ClpDisjointCopyN(rhs.integerType_, numberColumns_, integerType_);
               } else {
                    integerType_ = NULL;
               }
               if (rhs.rowActivity_) {
                    ClpDisjointCopyN ( rhs.rowActivity_, numberRows_ ,
                                       rowActivity_);
                    ClpDisjointCopyN ( rhs.columnActivity_, numberColumns_ ,
                                       columnActivity_);
                    ClpDisjointCopyN ( rhs.dual_, numberRows_ ,
                                       dual_);
                    ClpDisjointCopyN ( rhs.reducedCost_, numberColumns_ ,
                                       reducedCost_);
               } else {
                    rowActivity_ = NULL;
                    columnActivity_ = NULL;
                    dual_ = NULL;
                    reducedCost_ = NULL;
               }
               ClpDisjointCopyN ( rhs.rowLower_, numberRows_, rowLower_ );
               ClpDisjointCopyN ( rhs.rowUpper_, numberRows_, rowUpper_ );
               ClpDisjointCopyN ( rhs.columnLower_, numberColumns_, columnLower_ );
               assert ((specialOptions_ & 131072) == 0);
               abort();
               ClpDisjointCopyN ( rhs.columnUpper_, numberColumns_, columnUpper_ );
               if (rhs.objective_) {
                    abort(); //check if same
                    objective_  = rhs.objective_->clone();
               } else {
                    objective_ = NULL;
               }
               assert (!rhs.rowObjective_);
               ClpDisjointCopyN( rhs.status_, numberColumns_ + numberRows_, status_);
               ray_ = NULL;
               if (problemStatus_ == 1)
                    ray_ = ClpCopyOfArray (rhs.ray_, numberRows_);
               else if (problemStatus_ == 2)
                    ray_ = ClpCopyOfArray (rhs.ray_, numberColumns_);
               assert (!ray_);
               delete rowCopy_;
               if (rhs.rowCopy_) {
                    rowCopy_ = rhs.rowCopy_->clone();
               } else {
                    rowCopy_ = NULL;
               }
               delete scaledMatrix_;
               if (rhs.scaledMatrix_) {
                    scaledMatrix_ = new ClpPackedMatrix(*rhs.scaledMatrix_);
               } else {
                    scaledMatrix_ = NULL;
               }
               delete matrix_;
               matrix_ = NULL;
               if (rhs.matrix_) {
                    matrix_ = rhs.matrix_->clone();
               }
               if (rhs.savedRowScale_) {
                    assert (savedRowScale_);
                    assert (!rowScale_);
                    ClpDisjointCopyN(rhs.savedRowScale_, 4 * maximumInternalRows_, savedRowScale_);
                    ClpDisjointCopyN(rhs.savedColumnScale_, 4 * maximumInternalColumns_, savedColumnScale_);
               } else {
                    assert (!savedRowScale_);
                    if (rowScale_) {
                         ClpDisjointCopyN(rhs.rowScale_, numberRows_, rowScale_);
                         ClpDisjointCopyN(rhs.columnScale_, numberColumns_, columnScale_);
                    } else {
                         rowScale_ = NULL;
                         columnScale_ = NULL;
                    }
               }
               abort(); // look at resizeDouble and resize
          }
     } else {
          savedRowScale_ = rhs.savedRowScale_;
          assert (!savedRowScale_);
          savedColumnScale_ = rhs.savedColumnScale_;
          rowActivity_ = rhs.rowActivity_;
          columnActivity_ = rhs.columnActivity_;
          dual_ = rhs.dual_;
          reducedCost_ = rhs.reducedCost_;
          rowLower_ = rhs.rowLower_;
          rowUpper_ = rhs.rowUpper_;
          objective_ = rhs.objective_;
          rowObjective_ = rhs.rowObjective_;
          columnLower_ = rhs.columnLower_;
          columnUpper_ = rhs.columnUpper_;
          matrix_ = rhs.matrix_;
          rowCopy_ = NULL;
          scaledMatrix_ = NULL;
          ray_ = rhs.ray_;
          //rowScale_ = rhs.rowScale_;
          //columnScale_ = rhs.columnScale_;
          lengthNames_ = 0;
          numberThreads_ = rhs.numberThreads_;
#ifndef CLP_NO_STD
          rowNames_ = std::vector<std::string> ();
          columnNames_ = std::vector<std::string> ();
#endif
          integerType_ = NULL;
          status_ = rhs.status_;
     }
     inverseRowScale_ = NULL;
     inverseColumnScale_ = NULL;
}
// Copy contents - resizing if necessary - otherwise re-use memory
void
ClpModel::copy(const ClpMatrixBase * from, ClpMatrixBase * & to)
{
     assert (from);
     const ClpPackedMatrix * matrixFrom = (dynamic_cast<const ClpPackedMatrix*>(from));
     ClpPackedMatrix * matrixTo = (dynamic_cast< ClpPackedMatrix*>(to));
     if (matrixFrom && matrixTo) {
          matrixTo->copy(matrixFrom);
     } else {
          delete to;
          to =  from->clone();
     }
#if 0
     delete modelPtr_->matrix_;
     if (continuousModel_->matrix_) {
          modelPtr_->matrix_ = continuousModel_->matrix_->clone();
     } else {
          modelPtr_->matrix_ = NULL;
     }
#endif
}
/* Borrow model.  This is so we dont have to copy large amounts
   of data around.  It assumes a derived class wants to overwrite
   an empty model with a real one - while it does an algorithm */
void
ClpModel::borrowModel(ClpModel & rhs)
{
     if (defaultHandler_) {
          delete handler_;
          handler_ = NULL;
     }
     gutsOfDelete(1);
     optimizationDirection_ = rhs.optimizationDirection_;
     numberRows_ = rhs.numberRows_;
     numberColumns_ = rhs.numberColumns_;
     delete [] rhs.ray_;
     rhs.ray_ = NULL;
     // make sure scaled matrix not copied
     ClpPackedMatrix * save = rhs.scaledMatrix_;
     rhs.scaledMatrix_ = NULL;
     delete scaledMatrix_;
     scaledMatrix_ = NULL;
     gutsOfCopy(rhs, 0);
     rhs.scaledMatrix_ = save;
     specialOptions_ = rhs.specialOptions_ & ~65536;
     savedRowScale_ = NULL;
     savedColumnScale_ = NULL;
     inverseRowScale_ = NULL;
     inverseColumnScale_ = NULL;
}
// Return model - nulls all arrays so can be deleted safely
void
ClpModel::returnModel(ClpModel & otherModel)
{
     otherModel.objectiveValue_ = objectiveValue_;
     otherModel.numberIterations_ = numberIterations_;
     otherModel.problemStatus_ = problemStatus_;
     otherModel.secondaryStatus_ = secondaryStatus_;
     rowActivity_ = NULL;
     columnActivity_ = NULL;
     dual_ = NULL;
     reducedCost_ = NULL;
     rowLower_ = NULL;
     rowUpper_ = NULL;
     objective_ = NULL;
     rowObjective_ = NULL;
     columnLower_ = NULL;
     columnUpper_ = NULL;
     matrix_ = NULL;
     if (rowCopy_ != otherModel.rowCopy_)
       delete rowCopy_;
     rowCopy_ = NULL;
     delete scaledMatrix_;
     scaledMatrix_ = NULL;
     delete [] otherModel.ray_;
     otherModel.ray_ = ray_;
     ray_ = NULL;
     if (rowScale_ && otherModel.rowScale_ != rowScale_) {
          delete [] rowScale_;
          delete [] columnScale_;
     }
     rowScale_ = NULL;
     columnScale_ = NULL;
     //rowScale_=NULL;
     //columnScale_=NULL;
     // do status
     if (otherModel.status_ != status_) {
          delete [] otherModel.status_;
          otherModel.status_ = status_;
     }
     status_ = NULL;
     if (defaultHandler_) {
          delete handler_;
          handler_ = NULL;
     }
     inverseRowScale_ = NULL;
     inverseColumnScale_ = NULL;
}
//#############################################################################
// Parameter related methods
//#############################################################################

bool
ClpModel::setIntParam(ClpIntParam key, int value)
{
     switch (key) {
     case ClpMaxNumIteration:
          if (value < 0)
               return false;
          break;
     case ClpMaxNumIterationHotStart:
          if (value < 0)
               return false;
          break;
     case ClpNameDiscipline:
          if (value < 0)
               return false;
          break;
     default:
          return false;
     }
     intParam_[key] = value;
     return true;
}

//-----------------------------------------------------------------------------

bool
ClpModel::setDblParam(ClpDblParam key, double value)
{

     switch (key) {
     case ClpDualObjectiveLimit:
          break;

     case ClpPrimalObjectiveLimit:
          break;

     case ClpDualTolerance:
          if (value <= 0.0 || value > 1.0e10)
               return false;
          break;

     case ClpPrimalTolerance:
          if (value <= 0.0 || value > 1.0e10)
               return false;
          break;

     case ClpObjOffset:
          break;

     case ClpMaxSeconds:
          if(value >= 0)
               value += CoinCpuTime();
          else
               value = -1.0;
          break;

     case ClpMaxWallSeconds:
          if(value >= 0)
               value += CoinWallclockTime();
          else
               value = -1.0;
          break;

     case ClpPresolveTolerance:
          if (value <= 0.0 || value > 1.0e10)
               return false;
          break;

     default:
          return false;
     }
     dblParam_[key] = value;
     return true;
}

//-----------------------------------------------------------------------------
#ifndef CLP_NO_STD

bool
ClpModel::setStrParam(ClpStrParam key, const std::string & value)
{

     switch (key) {
     case ClpProbName:
          break;

     default:
          return false;
     }
     strParam_[key] = value;
     return true;
}
#endif
// Useful routines
// Returns resized array and deletes incoming
double * resizeDouble(double * array , int size, int newSize, double fill,
                      bool createArray)
{
     if ((array || createArray) && size < newSize) {
          int i;
          double * newArray = new double[newSize];
          if (array)
               CoinMemcpyN(array, CoinMin(newSize, size), newArray);
          delete [] array;
          array = newArray;
          for (i = size; i < newSize; i++)
               array[i] = fill;
     }
     return array;
}
// Returns resized array and updates size
double * deleteDouble(double * array , int size,
                      int number, const int * which, int & newSize)
{
     if (array) {
          int i ;
          char * deleted = new char[size];
          int numberDeleted = 0;
          CoinZeroN(deleted, size);
          for (i = 0; i < number; i++) {
               int j = which[i];
               if (j >= 0 && j < size && !deleted[j]) {
                    numberDeleted++;
                    deleted[j] = 1;
               }
          }
          newSize = size - numberDeleted;
          double * newArray = new double[newSize];
          int put = 0;
          for (i = 0; i < size; i++) {
               if (!deleted[i]) {
                    newArray[put++] = array[i];
               }
          }
          delete [] array;
          array = newArray;
          delete [] deleted;
     }
     return array;
}
char * deleteChar(char * array , int size,
                  int number, const int * which, int & newSize,
                  bool ifDelete)
{
     if (array) {
          int i ;
          char * deleted = new char[size];
          int numberDeleted = 0;
          CoinZeroN(deleted, size);
          for (i = 0; i < number; i++) {
               int j = which[i];
               if (j >= 0 && j < size && !deleted[j]) {
                    numberDeleted++;
                    deleted[j] = 1;
               }
          }
          newSize = size - numberDeleted;
          char * newArray = new char[newSize];
          int put = 0;
          for (i = 0; i < size; i++) {
               if (!deleted[i]) {
                    newArray[put++] = array[i];
               }
          }
          if (ifDelete)
               delete [] array;
          array = newArray;
          delete [] deleted;
     }
     return array;
}
// Create empty ClpPackedMatrix
void
ClpModel::createEmptyMatrix()
{
     delete matrix_;
     whatsChanged_ = 0;
     CoinPackedMatrix matrix2;
     matrix_ = new ClpPackedMatrix(matrix2);
}
/* Really clean up matrix.
   a) eliminate all duplicate AND small elements in matrix
   b) remove all gaps and set extraGap_ and extraMajor_ to 0.0
   c) reallocate arrays and make max lengths equal to lengths
   d) orders elements
   returns number of elements eliminated or -1 if not ClpMatrix
*/
int
ClpModel::cleanMatrix(double threshold)
{
     ClpPackedMatrix * matrix = (dynamic_cast< ClpPackedMatrix*>(matrix_));
     if (matrix) {
          return matrix->getPackedMatrix()->cleanMatrix(threshold);
     } else {
          return -1;
     }
}
// Resizes
void
ClpModel::resize (int newNumberRows, int newNumberColumns)
{
     if (newNumberRows == numberRows_ &&
               newNumberColumns == numberColumns_)
          return; // nothing to do
     whatsChanged_ = 0;
     int numberRows2 = newNumberRows;
     int numberColumns2 = newNumberColumns;
     if (numberRows2 < maximumRows_)
          numberRows2 = maximumRows_;
     if (numberColumns2 < maximumColumns_)
          numberColumns2 = maximumColumns_;
     if (numberRows2 > maximumRows_) {
          rowActivity_ = resizeDouble(rowActivity_, numberRows_,
                                      newNumberRows, 0.0, true);
          dual_ = resizeDouble(dual_, numberRows_,
                               newNumberRows, 0.0, true);
          rowObjective_ = resizeDouble(rowObjective_, numberRows_,
                                       newNumberRows, 0.0, false);
          rowLower_ = resizeDouble(rowLower_, numberRows_,
                                   newNumberRows, -COIN_DBL_MAX, true);
          rowUpper_ = resizeDouble(rowUpper_, numberRows_,
                                   newNumberRows, COIN_DBL_MAX, true);
     }
     if (numberColumns2 > maximumColumns_) {
          columnActivity_ = resizeDouble(columnActivity_, numberColumns_,
                                         newNumberColumns, 0.0, true);
          reducedCost_ = resizeDouble(reducedCost_, numberColumns_,
                                      newNumberColumns, 0.0, true);
     }
     if (savedRowScale_ && numberRows2 > maximumInternalRows_) {
          double * temp;
          temp = new double [4*newNumberRows];
          CoinFillN(temp, 4 * newNumberRows, 1.0);
          CoinMemcpyN(savedRowScale_, numberRows_, temp);
          CoinMemcpyN(savedRowScale_ + maximumInternalRows_, numberRows_, temp + newNumberRows);
          CoinMemcpyN(savedRowScale_ + 2 * maximumInternalRows_, numberRows_, temp + 2 * newNumberRows);
          CoinMemcpyN(savedRowScale_ + 3 * maximumInternalRows_, numberRows_, temp + 3 * newNumberRows);
          delete [] savedRowScale_;
          savedRowScale_ = temp;
     }
     if (savedColumnScale_ && numberColumns2 > maximumInternalColumns_) {
          double * temp;
          temp = new double [4*newNumberColumns];
          CoinFillN(temp, 4 * newNumberColumns, 1.0);
          CoinMemcpyN(savedColumnScale_, numberColumns_, temp);
          CoinMemcpyN(savedColumnScale_ + maximumInternalColumns_, numberColumns_, temp + newNumberColumns);
          CoinMemcpyN(savedColumnScale_ + 2 * maximumInternalColumns_, numberColumns_, temp + 2 * newNumberColumns);
          CoinMemcpyN(savedColumnScale_ + 3 * maximumInternalColumns_, numberColumns_, temp + 3 * newNumberColumns);
          delete [] savedColumnScale_;
          savedColumnScale_ = temp;
     }
     if (objective_ && numberColumns2 > maximumColumns_)
          objective_->resize(newNumberColumns);
     else if (!objective_)
          objective_ = new ClpLinearObjective(NULL, newNumberColumns);
     if (numberColumns2 > maximumColumns_) {
          columnLower_ = resizeDouble(columnLower_, numberColumns_,
                                      newNumberColumns, 0.0, true);
          columnUpper_ = resizeDouble(columnUpper_, numberColumns_,
                                      newNumberColumns, COIN_DBL_MAX, true);
     }
     if (newNumberRows < numberRows_) {
          int * which = new int[numberRows_-newNumberRows];
          int i;
          for (i = newNumberRows; i < numberRows_; i++)
               which[i-newNumberRows] = i;
          matrix_->deleteRows(numberRows_ - newNumberRows, which);
          delete [] which;
     }
     if (numberRows_ != newNumberRows || numberColumns_ != newNumberColumns) {
          // set state back to unknown
          problemStatus_ = -1;
          secondaryStatus_ = 0;
          delete [] ray_;
          ray_ = NULL;
     }
     setRowScale(NULL);
     setColumnScale(NULL);
     if (status_) {
          if (newNumberColumns + newNumberRows) {
               if (newNumberColumns + newNumberRows > maximumRows_ + maximumColumns_) {
                    unsigned char * tempC = new unsigned char [newNumberColumns+newNumberRows];
                    unsigned char * tempR = tempC + newNumberColumns;
                    memset(tempC, 3, newNumberColumns * sizeof(unsigned char));
                    memset(tempR, 1, newNumberRows * sizeof(unsigned char));
                    CoinMemcpyN(status_, CoinMin(newNumberColumns, numberColumns_), tempC);
                    CoinMemcpyN(status_ + numberColumns_, CoinMin(newNumberRows, numberRows_), tempR);
                    delete [] status_;
                    status_ = tempC;
               } else if (newNumberColumns < numberColumns_) {
                    memmove(status_ + newNumberColumns, status_ + numberColumns_,
                            newNumberRows);
               } else if (newNumberColumns > numberColumns_) {
                    memset(status_ + numberColumns_, 3, newNumberColumns - numberColumns_);
                    memmove(status_ + newNumberColumns, status_ + numberColumns_,
                            newNumberRows);
               }
          } else {
               // empty model - some systems don't like new [0]
               delete [] status_;
               status_ = NULL;
          }
     }
#ifndef CLP_NO_STD
     if (lengthNames_) {
          // redo row and column names (make sure clean)
          int numberRowNames = 
	    CoinMin(static_cast<int>(rowNames_.size()),numberRows_);
          if (numberRowNames < newNumberRows) {
               rowNames_.resize(newNumberRows);
               lengthNames_ = CoinMax(lengthNames_, 8);
               char name[9];
               for (int iRow = numberRowNames; iRow < newNumberRows; iRow++) {
                    sprintf(name, "R%7.7d", iRow);
                    rowNames_[iRow] = name;
               }
          }
          int numberColumnNames = 
	    CoinMin(static_cast<int>(columnNames_.size()),numberColumns_);
          if (numberColumnNames < newNumberColumns) {
               columnNames_.resize(newNumberColumns);
               lengthNames_ = CoinMax(lengthNames_, 8);
               char name[9];
               for (int iColumn = numberColumnNames; 
		    iColumn < newNumberColumns; iColumn++) {
                    sprintf(name, "C%7.7d", iColumn);
                    columnNames_[iColumn] = name;
               }
          }
     }
#endif
     numberRows_ = newNumberRows;
     if (newNumberColumns < numberColumns_ && matrix_->getNumCols()) {
          int * which = new int[numberColumns_-newNumberColumns];
          int i;
          for (i = newNumberColumns; i < numberColumns_; i++)
               which[i-newNumberColumns] = i;
          matrix_->deleteCols(numberColumns_ - newNumberColumns, which);
          delete [] which;
     }
     if (integerType_ && numberColumns2 > maximumColumns_) {
          char * temp = new char [newNumberColumns];
          CoinZeroN(temp, newNumberColumns);
          CoinMemcpyN(integerType_,
                      CoinMin(newNumberColumns, numberColumns_), temp);
          delete [] integerType_;
          integerType_ = temp;
     }
     numberColumns_ = newNumberColumns;
     if ((specialOptions_ & 65536) != 0) {
          // leave until next create rim to up numbers
     }
     if (maximumRows_ >= 0) {
          if (numberRows_ > maximumRows_)
               COIN_DETAIL_PRINT(printf("resize %d rows, %d old maximum rows\n",
					numberRows_, maximumRows_));
          maximumRows_ = CoinMax(maximumRows_, numberRows_);
          maximumColumns_ = CoinMax(maximumColumns_, numberColumns_);
     }
}
// Deletes rows
void
ClpModel::deleteRows(int number, const int * which)
{
     if (!number)
          return; // nothing to do
     whatsChanged_ &= ~(1 + 2 + 4 + 8 + 16 + 32); // all except columns changed
     int newSize = 0;
     if (maximumRows_ < 0) {
          rowActivity_ = deleteDouble(rowActivity_, numberRows_,
                                      number, which, newSize);
          dual_ = deleteDouble(dual_, numberRows_,
                               number, which, newSize);
          rowObjective_ = deleteDouble(rowObjective_, numberRows_,
                                       number, which, newSize);
          rowLower_ = deleteDouble(rowLower_, numberRows_,
                                   number, which, newSize);
          rowUpper_ = deleteDouble(rowUpper_, numberRows_,
                                   number, which, newSize);
          if (matrix_->getNumRows())
               matrix_->deleteRows(number, which);
          //matrix_->removeGaps();
          // status
          if (status_) {
               if (numberColumns_ + newSize) {
                    unsigned char * tempR  = reinterpret_cast<unsigned char *>
                                             (deleteChar(reinterpret_cast<char *>(status_) + numberColumns_,
                                                         numberRows_,
                                                         number, which, newSize, false));
                    unsigned char * tempC = new unsigned char [numberColumns_+newSize];
                    CoinMemcpyN(status_, numberColumns_, tempC);
                    CoinMemcpyN(tempR, newSize, tempC + numberColumns_);
                    delete [] tempR;
                    delete [] status_;
                    status_ = tempC;
               } else {
                    // empty model - some systems don't like new [0]
                    delete [] status_;
                    status_ = NULL;
               }
          }
     } else {
          char * deleted = new char [numberRows_];
          int i;
          int numberDeleted = 0;
          CoinZeroN(deleted, numberRows_);
          for (i = 0; i < number; i++) {
               int j = which[i];
               if (j >= 0 && j < numberRows_ && !deleted[j]) {
                    numberDeleted++;
                    deleted[j] = 1;
               }
          }
          assert (!rowObjective_);
          unsigned char * status2 = status_ + numberColumns_;
          for (i = 0; i < numberRows_; i++) {
               if (!deleted[i]) {
                    rowActivity_[newSize] = rowActivity_[i];
                    dual_[newSize] = dual_[i];
                    rowLower_[newSize] = rowLower_[i];
                    rowUpper_[newSize] = rowUpper_[i];
                    status2[newSize] = status2[i];
                    newSize++;
               }
          }
          if (matrix_->getNumRows())
               matrix_->deleteRows(number, which);
          //matrix_->removeGaps();
          delete [] deleted;
     }
#ifndef CLP_NO_STD
     // Now works if which out of order
     if (lengthNames_) {
          char * mark = new char [numberRows_];
          CoinZeroN(mark, numberRows_);
          int i;
          for (i = 0; i < number; i++)
               mark[which[i]] = 1;
          int k = 0;
          for ( i = 0; i < numberRows_; ++i) {
               if (!mark[i])
                    rowNames_[k++] = rowNames_[i];
          }
          rowNames_.erase(rowNames_.begin() + k, rowNames_.end());
          delete [] mark;
     }
#endif
     numberRows_ = newSize;
     // set state back to unknown
     problemStatus_ = -1;
     secondaryStatus_ = 0;
     delete [] ray_;
     ray_ = NULL;
     if (savedRowScale_ != rowScale_) {
          delete [] rowScale_;
          delete [] columnScale_;
     }
     rowScale_ = NULL;
     columnScale_ = NULL;
     delete scaledMatrix_;
     scaledMatrix_ = NULL;
}
// Deletes columns
void
ClpModel::deleteColumns(int number, const int * which)
{
     if (!number)
          return; // nothing to do
     assert (maximumColumns_ < 0);
     whatsChanged_ &= ~(1 + 2 + 4 + 8 + 64 + 128 + 256); // all except rows changed
     int newSize = 0;
     columnActivity_ = deleteDouble(columnActivity_, numberColumns_,
                                    number, which, newSize);
     reducedCost_ = deleteDouble(reducedCost_, numberColumns_,
                                 number, which, newSize);
     objective_->deleteSome(number, which);
     columnLower_ = deleteDouble(columnLower_, numberColumns_,
                                 number, which, newSize);
     columnUpper_ = deleteDouble(columnUpper_, numberColumns_,
                                 number, which, newSize);
     // possible matrix is not full
     if (matrix_->getNumCols() < numberColumns_) {
          int * which2 = new int [number];
          int n = 0;
          int nMatrix = matrix_->getNumCols();
          for (int i = 0; i < number; i++) {
               if (which[i] < nMatrix)
                    which2[n++] = which[i];
          }
          matrix_->deleteCols(n, which2);
          delete [] which2;
     } else {
          matrix_->deleteCols(number, which);
     }
     //matrix_->removeGaps();
     // status
     if (status_) {
          if (numberRows_ + newSize) {
               unsigned char * tempC  = reinterpret_cast<unsigned char *>
                                        (deleteChar(reinterpret_cast<char *>(status_),
                                                    numberColumns_,
                                                    number, which, newSize, false));
               unsigned char * temp = new unsigned char [numberRows_+newSize];
               CoinMemcpyN(tempC, newSize, temp);
               CoinMemcpyN(status_ + numberColumns_,	numberRows_, temp + newSize);
               delete [] tempC;
               delete [] status_;
               status_ = temp;
          } else {
               // empty model - some systems don't like new [0]
               delete [] status_;
               status_ = NULL;
          }
     }
     integerType_ = deleteChar(integerType_, numberColumns_,
                               number, which, newSize, true);
#ifndef CLP_NO_STD
     // Now works if which out of order
     if (lengthNames_) {
          char * mark = new char [numberColumns_];
          CoinZeroN(mark, numberColumns_);
          int i;
          for (i = 0; i < number; i++)
               mark[which[i]] = 1;
          int k = 0;
          for ( i = 0; i < numberColumns_; ++i) {
               if (!mark[i])
                    columnNames_[k++] = columnNames_[i];
          }
          columnNames_.erase(columnNames_.begin() + k, columnNames_.end());
          delete [] mark;
     }
#endif
     numberColumns_ = newSize;
     // set state back to unknown
     problemStatus_ = -1;
     secondaryStatus_ = 0;
     delete [] ray_;
     ray_ = NULL;
     setRowScale(NULL);
     setColumnScale(NULL);
}
// Deletes rows AND columns (does not reallocate)
void 
ClpModel::deleteRowsAndColumns(int numberRows, const int * whichRows,
			       int numberColumns, const int * whichColumns)
{
  if (!numberColumns) {
    deleteRows(numberRows,whichRows);
  } else if (!numberRows) {
    deleteColumns(numberColumns,whichColumns);
  } else {
    whatsChanged_ &= ~511; // all changed
    bool doStatus = status_!=NULL;
    int numberTotal=numberRows_+numberColumns_;
    int * backRows = new int [numberTotal];
    int * backColumns = backRows+numberRows_;
    memset(backRows,0,numberTotal*sizeof(int));
    int newNumberColumns=0;
    for (int i=0;i<numberColumns;i++) {
      int iColumn=whichColumns[i];
      if (iColumn>=0&&iColumn<numberColumns_)
	backColumns[iColumn]=-1;
    }
    assert (objective_->type()==1);
    double * obj = objective(); 
    for (int i=0;i<numberColumns_;i++) {
      if (!backColumns[i]) {
	columnActivity_[newNumberColumns] = columnActivity_[i];
	reducedCost_[newNumberColumns] = reducedCost_[i];
	obj[newNumberColumns] = obj[i];
	columnLower_[newNumberColumns] = columnLower_[i];
	columnUpper_[newNumberColumns] = columnUpper_[i];
	if (doStatus)
	  status_[newNumberColumns] = status_[i];
	backColumns[i]=newNumberColumns++;
      }
    }
    integerType_ = deleteChar(integerType_, numberColumns_,
			      numberColumns, whichColumns, newNumberColumns, true);
#ifndef CLP_NO_STD
    // Now works if which out of order
    if (lengthNames_) {
      for (int i=0;i<numberColumns_;i++) {
	int iColumn=backColumns[i];
	if (iColumn) 
	  columnNames_[iColumn] = columnNames_[i];
      }
      columnNames_.erase(columnNames_.begin() + newNumberColumns, columnNames_.end());
    }
#endif
    int newNumberRows=0;
    assert (!rowObjective_);
    unsigned char * status2 = status_ + numberColumns_;
    unsigned char * status2a = status_ + newNumberColumns;
    for (int i=0;i<numberRows;i++) {
      int iRow=whichRows[i];
      if (iRow>=0&&iRow<numberRows_)
	backRows[iRow]=-1;
    }
    for (int i=0;i<numberRows_;i++) {
      if (!backRows[i]) {
	rowActivity_[newNumberRows] = rowActivity_[i];
	dual_[newNumberRows] = dual_[i];
	rowLower_[newNumberRows] = rowLower_[i];
	rowUpper_[newNumberRows] = rowUpper_[i];
	if (doStatus)
	  status2a[newNumberRows] = status2[i];
	backRows[i]=newNumberRows++;
      }
    }
#ifndef CLP_NO_STD
    // Now works if which out of order
    if (lengthNames_) {
      for (int i=0;i<numberRows_;i++) {
	int iRow=backRows[i];
	if (iRow) 
	  rowNames_[iRow] = rowNames_[i];
      }
      rowNames_.erase(rowNames_.begin() + newNumberRows, rowNames_.end());
    }
#endif
    // possible matrix is not full
    ClpPackedMatrix * clpMatrix = dynamic_cast<ClpPackedMatrix *>(matrix_);
    CoinPackedMatrix * matrix = clpMatrix ? clpMatrix->matrix() : NULL;
    if (matrix_->getNumCols() < numberColumns_) {
      assert (matrix);
      CoinBigIndex nel=matrix->getNumElements();
      int n=matrix->getNumCols();
      matrix->reserve(numberColumns_,nel);
      CoinBigIndex * columnStart = matrix->getMutableVectorStarts();
      int * columnLength = matrix->getMutableVectorLengths();
      for (int i=n;i<numberColumns_;i++) {
	columnStart[i]=nel;
	columnLength[i]=0;
      }
    }
    if (matrix) {
      matrix->setExtraMajor(0.1);
      //CoinPackedMatrix temp(*matrix);
      matrix->setExtraGap(0.0);
      matrix->setExtraMajor(0.0);
      int * row = matrix->getMutableIndices();
      CoinBigIndex * columnStart = matrix->getMutableVectorStarts();
      int * columnLength = matrix->getMutableVectorLengths();
      double * element = matrix->getMutableElements();
      newNumberColumns=0;
      CoinBigIndex n=0;
      for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
	if (backColumns[iColumn]>=0) {
	  CoinBigIndex start = columnStart[iColumn];
	  int nSave=n;
	  columnStart[newNumberColumns]=n;
	  for (CoinBigIndex j=start;j<start+columnLength[iColumn];j++) {
	    int iRow=row[j];
	    iRow = backRows[iRow];
	    if (iRow>=0) {
	      row[n]=iRow;
	      element[n++]=element[j];
	    }
	  }
	  columnLength[newNumberColumns++]=n-nSave;
	}
      }
      columnStart[newNumberColumns]=n;
      matrix->setNumElements(n);
      matrix->setMajorDim(newNumberColumns);
      matrix->setMinorDim(newNumberRows);
      clpMatrix->setNumberActiveColumns(newNumberColumns);
      //temp.deleteRows(numberRows, whichRows);
      //temp.deleteCols(numberColumns, whichColumns);
      //assert(matrix->isEquivalent2(temp));
      //*matrix=temp;
    } else {
      matrix_->deleteRows(numberRows, whichRows);
      matrix_->deleteCols(numberColumns, whichColumns);
    }
    numberColumns_ = newNumberColumns;
    numberRows_ = newNumberRows;
    delete [] backRows;
    // set state back to unknown
    problemStatus_ = -1;
    secondaryStatus_ = 0;
    delete [] ray_;
    ray_ = NULL;
    if (savedRowScale_ != rowScale_) {
      delete [] rowScale_;
      delete [] columnScale_;
    }
    rowScale_ = NULL;
    columnScale_ = NULL;
    delete scaledMatrix_;
    scaledMatrix_ = NULL;
    delete rowCopy_;
    rowCopy_ = NULL;
  }
}
// Add one row
void
ClpModel::addRow(int numberInRow, const int * columns,
                 const double * elements, double rowLower, double rowUpper)
{
     CoinBigIndex starts[2];
     starts[0] = 0;
     starts[1] = numberInRow;
     addRows(1, &rowLower, &rowUpper, starts, columns, elements);
}
// Add rows
void
ClpModel::addRows(int number, const double * rowLower,
                  const double * rowUpper,
                  const CoinBigIndex * rowStarts, const int * columns,
                  const double * elements)
{
     if (number) {
          whatsChanged_ &= ~(1 + 2 + 8 + 16 + 32); // all except columns changed
          int numberRowsNow = numberRows_;
          resize(numberRowsNow + number, numberColumns_);
          double * lower = rowLower_ + numberRowsNow;
          double * upper = rowUpper_ + numberRowsNow;
          int iRow;
          if (rowLower) {
               for (iRow = 0; iRow < number; iRow++) {
                    double value = rowLower[iRow];
                    if (value < -1.0e20)
                         value = -COIN_DBL_MAX;
                    lower[iRow] = value;
               }
          } else {
               for (iRow = 0; iRow < number; iRow++) {
                    lower[iRow] = -COIN_DBL_MAX;
               }
          }
          if (rowUpper) {
               for (iRow = 0; iRow < number; iRow++) {
                    double value = rowUpper[iRow];
                    if (value > 1.0e20)
                         value = COIN_DBL_MAX;
                    upper[iRow] = value;
               }
          } else {
               for (iRow = 0; iRow < number; iRow++) {
                    upper[iRow] = COIN_DBL_MAX;
               }
          }
          // Deal with matrix

          delete rowCopy_;
          rowCopy_ = NULL;
          delete scaledMatrix_;
          scaledMatrix_ = NULL;
          if (!matrix_)
               createEmptyMatrix();
          setRowScale(NULL);
          setColumnScale(NULL);
#ifndef CLP_NO_STD
          if (lengthNames_) {
               rowNames_.resize(numberRows_);
          }
#endif
          if (rowStarts) {
	       // Make sure matrix has correct number of columns
	       matrix_->getPackedMatrix()->reserve(numberColumns_, 0, true);
               matrix_->appendMatrix(number, 0, rowStarts, columns, elements);
          }
     }
}
// Add rows
void
ClpModel::addRows(int number, const double * rowLower,
                  const double * rowUpper,
                  const CoinBigIndex * rowStarts,
                  const int * rowLengths, const int * columns,
                  const double * elements)
{
     if (number) {
          CoinBigIndex numberElements = 0;
          int iRow;
          for (iRow = 0; iRow < number; iRow++)
               numberElements += rowLengths[iRow];
          int * newStarts = new int[number+1];
          int * newIndex = new int[numberElements];
          double * newElements = new double[numberElements];
          numberElements = 0;
          newStarts[0] = 0;
          for (iRow = 0; iRow < number; iRow++) {
               int iStart = rowStarts[iRow];
               int length = rowLengths[iRow];
               CoinMemcpyN(columns + iStart, length, newIndex + numberElements);
               CoinMemcpyN(elements + iStart, length, newElements + numberElements);
               numberElements += length;
               newStarts[iRow+1] = numberElements;
          }
          addRows(number, rowLower, rowUpper,
                  newStarts, newIndex, newElements);
          delete [] newStarts;
          delete [] newIndex;
          delete [] newElements;
     }
}
#ifndef CLP_NO_VECTOR
void
ClpModel::addRows(int number, const double * rowLower,
                  const double * rowUpper,
                  const CoinPackedVectorBase * const * rows)
{
     if (!number)
          return;
     whatsChanged_ &= ~(1 + 2 + 8 + 16 + 32); // all except columns changed
     int numberRowsNow = numberRows_;
     resize(numberRowsNow + number, numberColumns_);
     double * lower = rowLower_ + numberRowsNow;
     double * upper = rowUpper_ + numberRowsNow;
     int iRow;
     if (rowLower) {
          for (iRow = 0; iRow < number; iRow++) {
               double value = rowLower[iRow];
               if (value < -1.0e20)
                    value = -COIN_DBL_MAX;
               lower[iRow] = value;
          }
     } else {
          for (iRow = 0; iRow < number; iRow++) {
               lower[iRow] = -COIN_DBL_MAX;
          }
     }
     if (rowUpper) {
          for (iRow = 0; iRow < number; iRow++) {
               double value = rowUpper[iRow];
               if (value > 1.0e20)
                    value = COIN_DBL_MAX;
               upper[iRow] = value;
          }
     } else {
          for (iRow = 0; iRow < number; iRow++) {
               upper[iRow] = COIN_DBL_MAX;
          }
     }
     // Deal with matrix

     delete rowCopy_;
     rowCopy_ = NULL;
     delete scaledMatrix_;
     scaledMatrix_ = NULL;
     if (!matrix_)
          createEmptyMatrix();
     if (rows)
          matrix_->appendRows(number, rows);
     setRowScale(NULL);
     setColumnScale(NULL);
     if (lengthNames_) {
          rowNames_.resize(numberRows_);
     }
}
#endif
#ifndef SLIM_CLP
// Add rows from a build object
int
ClpModel::addRows(const CoinBuild & buildObject, bool tryPlusMinusOne, bool checkDuplicates)
{
     CoinAssertHint (buildObject.type() == 0, "Looks as if both addRows and addCols being used"); // check correct
     int number = buildObject.numberRows();
     int numberErrors = 0;
     if (number) {
          CoinBigIndex size = 0;
          int iRow;
          double * lower = new double [number];
          double * upper = new double [number];
          if ((!matrix_ || !matrix_->getNumElements()) && tryPlusMinusOne) {
               // See if can be +-1
               for (iRow = 0; iRow < number; iRow++) {
                    const int * columns;
                    const double * elements;
                    int numberElements = buildObject.row(iRow, lower[iRow],
                                                         upper[iRow],
                                                         columns, elements);
                    for (int i = 0; i < numberElements; i++) {
                         // allow for zero elements
                         if (elements[i]) {
                              if (fabs(elements[i]) == 1.0) {
                                   size++;
                              } else {
                                   // bad
                                   tryPlusMinusOne = false;
                              }
                         }
                    }
                    if (!tryPlusMinusOne)
                         break;
               }
          } else {
               // Will add to whatever sort of matrix exists
               tryPlusMinusOne = false;
          }
          if (!tryPlusMinusOne) {
               CoinBigIndex numberElements = buildObject.numberElements();
               CoinBigIndex * starts = new CoinBigIndex [number+1];
               int * column = new int[numberElements];
               double * element = new double[numberElements];
               starts[0] = 0;
               numberElements = 0;
               for (iRow = 0; iRow < number; iRow++) {
                    const int * columns;
                    const double * elements;
                    int numberElementsThis = buildObject.row(iRow, lower[iRow], upper[iRow],
                                             columns, elements);
                    CoinMemcpyN(columns, numberElementsThis, column + numberElements);
                    CoinMemcpyN(elements, numberElementsThis, element + numberElements);
                    numberElements += numberElementsThis;
                    starts[iRow+1] = numberElements;
               }
               addRows(number, lower, upper, NULL);
               // make sure matrix has enough columns
               matrix_->setDimensions(-1, numberColumns_);
               numberErrors = matrix_->appendMatrix(number, 0, starts, column, element,
                                                    checkDuplicates ? numberColumns_ : -1);
               delete [] starts;
               delete [] column;
               delete [] element;
          } else {
               char * which = NULL; // for duplicates
               if (checkDuplicates) {
                    which = new char[numberColumns_];
                    CoinZeroN(which, numberColumns_);
               }
               // build +-1 matrix
               // arrays already filled in
               addRows(number, lower, upper, NULL);
               CoinBigIndex * startPositive = new CoinBigIndex [numberColumns_+1];
               CoinBigIndex * startNegative = new CoinBigIndex [numberColumns_];
               int * indices = new int [size];
               CoinZeroN(startPositive, numberColumns_);
               CoinZeroN(startNegative, numberColumns_);
               int maxColumn = -1;
               // need two passes
               for (iRow = 0; iRow < number; iRow++) {
                    const int * columns;
                    const double * elements;
                    int numberElements = buildObject.row(iRow, lower[iRow],
                                                         upper[iRow],
                                                         columns, elements);
                    for (int i = 0; i < numberElements; i++) {
                         int iColumn = columns[i];
                         if (checkDuplicates) {
                              if (iColumn >= numberColumns_) {
                                   if(which[iColumn])
                                        numberErrors++;
                                   else
                                        which[iColumn] = 1;
                              } else {
                                   numberErrors++;
                                   // and may as well switch off
                                   checkDuplicates = false;
                              }
                         }
                         maxColumn = CoinMax(maxColumn, iColumn);
                         if (elements[i] == 1.0) {
                              startPositive[iColumn]++;
                         } else if (elements[i] == -1.0) {
                              startNegative[iColumn]++;
                         }
                    }
                    if (checkDuplicates) {
                         for (int i = 0; i < numberElements; i++) {
                              int iColumn = columns[i];
                              which[iColumn] = 0;
                         }
                    }
               }
               // check size
               int numberColumns = maxColumn + 1;
               CoinAssertHint (numberColumns <= numberColumns_,
                               "rows having column indices >= numberColumns_");
               size = 0;
               int iColumn;
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    CoinBigIndex n = startPositive[iColumn];
                    startPositive[iColumn] = size;
                    size += n;
                    n = startNegative[iColumn];
                    startNegative[iColumn] = size;
                    size += n;
               }
               startPositive[numberColumns_] = size;
               for (iRow = 0; iRow < number; iRow++) {
                    const int * columns;
                    const double * elements;
                    int numberElements = buildObject.row(iRow, lower[iRow],
                                                         upper[iRow],
                                                         columns, elements);
                    for (int i = 0; i < numberElements; i++) {
                         int iColumn = columns[i];
                         maxColumn = CoinMax(maxColumn, iColumn);
                         if (elements[i] == 1.0) {
                              CoinBigIndex position = startPositive[iColumn];
                              indices[position] = iRow;
                              startPositive[iColumn]++;
                         } else if (elements[i] == -1.0) {
                              CoinBigIndex position = startNegative[iColumn];
                              indices[position] = iRow;
                              startNegative[iColumn]++;
                         }
                    }
               }
               // and now redo starts
               for (iColumn = numberColumns_ - 1; iColumn >= 0; iColumn--) {
                    startPositive[iColumn+1] = startNegative[iColumn];
                    startNegative[iColumn] = startPositive[iColumn];
               }
               startPositive[0] = 0;
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    CoinBigIndex start = startPositive[iColumn];
                    CoinBigIndex end = startNegative[iColumn];
                    std::sort(indices + start, indices + end);
                    start = startNegative[iColumn];
                    end = startPositive[iColumn+1];
                    std::sort(indices + start, indices + end);
               }
               // Get good object
               delete matrix_;
               ClpPlusMinusOneMatrix * matrix = new ClpPlusMinusOneMatrix();
               matrix->passInCopy(numberRows_, numberColumns,
                                  true, indices, startPositive, startNegative);
               matrix_ = matrix;
               delete [] which;
          }
          delete [] lower;
          delete [] upper;
          // make sure matrix correct size
          matrix_->setDimensions(numberRows_, numberColumns_);
     }
     return numberErrors;
}
#endif
#ifndef SLIM_NOIO
// Add rows from a model object
int
ClpModel::addRows( CoinModel & modelObject, bool tryPlusMinusOne, bool checkDuplicates)
{
     if (modelObject.numberElements() == 0)
          return 0;
     bool goodState = true;
     int numberErrors = 0;
     if (modelObject.columnLowerArray()) {
          // some column information exists
          int numberColumns2 = modelObject.numberColumns();
          const double * columnLower = modelObject.columnLowerArray();
          const double * columnUpper = modelObject.columnUpperArray();
          const double * objective = modelObject.objectiveArray();
          const int * integerType = modelObject.integerTypeArray();
          for (int i = 0; i < numberColumns2; i++) {
               if (columnLower[i] != 0.0)
                    goodState = false;
               if (columnUpper[i] != COIN_DBL_MAX)
                    goodState = false;
               if (objective[i] != 0.0)
                    goodState = false;
               if (integerType[i] != 0)
                    goodState = false;
          }
     }
     if (goodState) {
          // can do addRows
          // Set arrays for normal use
          double * rowLower = modelObject.rowLowerArray();
          double * rowUpper = modelObject.rowUpperArray();
          double * columnLower = modelObject.columnLowerArray();
          double * columnUpper = modelObject.columnUpperArray();
          double * objective = modelObject.objectiveArray();
          int * integerType = modelObject.integerTypeArray();
          double * associated = modelObject.associatedArray();
          // If strings then do copies
          if (modelObject.stringsExist()) {
               numberErrors = modelObject.createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                                       objective, integerType, associated);
          }
          int numberRows = numberRows_; // save number of rows
          int numberRows2 = modelObject.numberRows();
          if (numberRows2 && !numberErrors) {
               CoinBigIndex * startPositive = NULL;
               CoinBigIndex * startNegative = NULL;
               int numberColumns = modelObject.numberColumns();
               if ((!matrix_ || !matrix_->getNumElements()) && !numberRows && tryPlusMinusOne) {
                    startPositive = new CoinBigIndex[numberColumns+1];
                    startNegative = new CoinBigIndex[numberColumns];
                    modelObject.countPlusMinusOne(startPositive, startNegative, associated);
                    if (startPositive[0] < 0) {
                         // no good
                         tryPlusMinusOne = false;
                         delete [] startPositive;
                         delete [] startNegative;
                    }
               } else {
                    // Will add to whatever sort of matrix exists
                    tryPlusMinusOne = false;
               }
               assert (rowLower);
               addRows(numberRows2, rowLower, rowUpper, NULL, NULL, NULL);
#ifndef SLIM_CLP
               if (!tryPlusMinusOne) {
#endif
                    CoinPackedMatrix matrix;
                    modelObject.createPackedMatrix(matrix, associated);
                    assert (!matrix.getExtraGap());
                    if (matrix_->getNumRows()) {
                         // matrix by rows
                         matrix.reverseOrdering();
                         assert (!matrix.getExtraGap());
                         const int * column = matrix.getIndices();
                         //const int * rowLength = matrix.getVectorLengths();
                         const CoinBigIndex * rowStart = matrix.getVectorStarts();
                         const double * element = matrix.getElements();
                         // make sure matrix has enough columns
                         matrix_->setDimensions(-1, numberColumns_);
                         numberErrors += matrix_->appendMatrix(numberRows2, 0, rowStart, column, element,
                                                               checkDuplicates ? numberColumns_ : -1);
                    } else {
                         delete matrix_;
                         matrix_ = new ClpPackedMatrix(matrix);
                    }
#ifndef SLIM_CLP
               } else {
                    // create +-1 matrix
                    CoinBigIndex size = startPositive[numberColumns];
                    int * indices = new int[size];
                    modelObject.createPlusMinusOne(startPositive, startNegative, indices,
                                                   associated);
                    // Get good object
                    ClpPlusMinusOneMatrix * matrix = new ClpPlusMinusOneMatrix();
                    matrix->passInCopy(numberRows2, numberColumns,
                                       true, indices, startPositive, startNegative);
                    delete matrix_;
                    matrix_ = matrix;
               }
               // Do names if wanted
               if (modelObject.rowNames()->numberItems()) {
                    const char *const * rowNames = modelObject.rowNames()->names();
                    copyRowNames(rowNames, numberRows, numberRows_);
               }
#endif
          }
          if (rowLower != modelObject.rowLowerArray()) {
               delete [] rowLower;
               delete [] rowUpper;
               delete [] columnLower;
               delete [] columnUpper;
               delete [] objective;
               delete [] integerType;
               delete [] associated;
               if (numberErrors)
                    handler_->message(CLP_BAD_STRING_VALUES, messages_)
                              << numberErrors
                              << CoinMessageEol;
          }
          return numberErrors;
     } else {
          // not suitable for addRows
          handler_->message(CLP_COMPLICATED_MODEL, messages_)
                    << modelObject.numberRows()
                    << modelObject.numberColumns()
                    << CoinMessageEol;
          return -1;
     }
}
#endif
// Add one column
void
ClpModel::addColumn(int numberInColumn,
                    const int * rows,
                    const double * elements,
                    double columnLower,
                    double  columnUpper,
                    double  objective)
{
     CoinBigIndex starts[2];
     starts[0] = 0;
     starts[1] = numberInColumn;
     addColumns(1, &columnLower, &columnUpper, &objective, starts, rows, elements);
}
// Add columns
void
ClpModel::addColumns(int number, const double * columnLower,
                     const double * columnUpper,
                     const double * objIn,
                     const int * columnStarts, const int * rows,
                     const double * elements)
{
     // Create a list of CoinPackedVectors
     if (number) {
          whatsChanged_ &= ~(1 + 2 + 4 + 64 + 128 + 256); // all except rows changed
          int numberColumnsNow = numberColumns_;
          resize(numberRows_, numberColumnsNow + number);
          double * lower = columnLower_ + numberColumnsNow;
          double * upper = columnUpper_ + numberColumnsNow;
          double * obj = objective() + numberColumnsNow;
          int iColumn;
          if (columnLower) {
               for (iColumn = 0; iColumn < number; iColumn++) {
                    double value = columnLower[iColumn];
                    if (value < -1.0e20)
                         value = -COIN_DBL_MAX;
                    lower[iColumn] = value;
               }
          } else {
               for (iColumn = 0; iColumn < number; iColumn++) {
                    lower[iColumn] = 0.0;
               }
          }
          if (columnUpper) {
               for (iColumn = 0; iColumn < number; iColumn++) {
                    double value = columnUpper[iColumn];
                    if (value > 1.0e20)
                         value = COIN_DBL_MAX;
                    upper[iColumn] = value;
               }
          } else {
               for (iColumn = 0; iColumn < number; iColumn++) {
                    upper[iColumn] = COIN_DBL_MAX;
               }
          }
          if (objIn) {
               for (iColumn = 0; iColumn < number; iColumn++) {
                    obj[iColumn] = objIn[iColumn];
               }
          } else {
               for (iColumn = 0; iColumn < number; iColumn++) {
                    obj[iColumn] = 0.0;
               }
          }
          // Deal with matrix

          delete rowCopy_;
          rowCopy_ = NULL;
          delete scaledMatrix_;
          scaledMatrix_ = NULL;
          if (!matrix_)
               createEmptyMatrix();
          setRowScale(NULL);
          setColumnScale(NULL);
#ifndef CLP_NO_STD
          if (lengthNames_) {
               columnNames_.resize(numberColumns_);
          }
#endif
          // Do even if elements NULL (to resize)
	  matrix_->appendMatrix(number, 1, columnStarts, rows, elements);
     }
}
// Add columns
void
ClpModel::addColumns(int number, const double * columnLower,
                     const double * columnUpper,
                     const double * objIn,
                     const int * columnStarts,
                     const int * columnLengths, const int * rows,
                     const double * elements)
{
     if (number) {
          CoinBigIndex numberElements = 0;
          int iColumn;
          for (iColumn = 0; iColumn < number; iColumn++)
               numberElements += columnLengths[iColumn];
          int * newStarts = new int[number+1];
          int * newIndex = new int[numberElements];
          double * newElements = new double[numberElements];
          numberElements = 0;
          newStarts[0] = 0;
          for (iColumn = 0; iColumn < number; iColumn++) {
               int iStart = columnStarts[iColumn];
               int length = columnLengths[iColumn];
               CoinMemcpyN(rows + iStart, length, newIndex + numberElements);
               CoinMemcpyN(elements + iStart, length, newElements + numberElements);
               numberElements += length;
               newStarts[iColumn+1] = numberElements;
          }
          addColumns(number, columnLower, columnUpper, objIn,
                     newStarts, newIndex, newElements);
          delete [] newStarts;
          delete [] newIndex;
          delete [] newElements;
     }
}
#ifndef CLP_NO_VECTOR
void
ClpModel::addColumns(int number, const double * columnLower,
                     const double * columnUpper,
                     const double * objIn,
                     const CoinPackedVectorBase * const * columns)
{
     if (!number)
          return;
     whatsChanged_ &= ~(1 + 2 + 4 + 64 + 128 + 256); // all except rows changed
     int numberColumnsNow = numberColumns_;
     resize(numberRows_, numberColumnsNow + number);
     double * lower = columnLower_ + numberColumnsNow;
     double * upper = columnUpper_ + numberColumnsNow;
     double * obj = objective() + numberColumnsNow;
     int iColumn;
     if (columnLower) {
          for (iColumn = 0; iColumn < number; iColumn++) {
               double value = columnLower[iColumn];
               if (value < -1.0e20)
                    value = -COIN_DBL_MAX;
               lower[iColumn] = value;
          }
     } else {
          for (iColumn = 0; iColumn < number; iColumn++) {
               lower[iColumn] = 0.0;
          }
     }
     if (columnUpper) {
          for (iColumn = 0; iColumn < number; iColumn++) {
               double value = columnUpper[iColumn];
               if (value > 1.0e20)
                    value = COIN_DBL_MAX;
               upper[iColumn] = value;
          }
     } else {
          for (iColumn = 0; iColumn < number; iColumn++) {
               upper[iColumn] = COIN_DBL_MAX;
          }
     }
     if (objIn) {
          for (iColumn = 0; iColumn < number; iColumn++) {
               obj[iColumn] = objIn[iColumn];
          }
     } else {
          for (iColumn = 0; iColumn < number; iColumn++) {
               obj[iColumn] = 0.0;
          }
     }
     // Deal with matrix

     delete rowCopy_;
     rowCopy_ = NULL;
     delete scaledMatrix_;
     scaledMatrix_ = NULL;
     if (!matrix_)
          createEmptyMatrix();
     if (columns)
          matrix_->appendCols(number, columns);
     setRowScale(NULL);
     setColumnScale(NULL);
     if (lengthNames_) {
          columnNames_.resize(numberColumns_);
     }
}
#endif
#ifndef SLIM_CLP
// Add columns from a build object
int
ClpModel::addColumns(const CoinBuild & buildObject, bool tryPlusMinusOne, bool checkDuplicates)
{
     CoinAssertHint (buildObject.type() == 1, "Looks as if both addRows and addCols being used"); // check correct
     int number = buildObject.numberColumns();
     int numberErrors = 0;
     if (number) {
          CoinBigIndex size = 0;
          int maximumLength = 0;
          double * lower = new double [number];
          double * upper = new double [number];
          int iColumn;
          double * objective = new double [number];
          if ((!matrix_ || !matrix_->getNumElements()) && tryPlusMinusOne) {
               // See if can be +-1
               for (iColumn = 0; iColumn < number; iColumn++) {
                    const int * rows;
                    const double * elements;
                    int numberElements = buildObject.column(iColumn, lower[iColumn],
                                                            upper[iColumn], objective[iColumn],
                                                            rows, elements);
                    maximumLength = CoinMax(maximumLength, numberElements);
                    for (int i = 0; i < numberElements; i++) {
                         // allow for zero elements
                         if (elements[i]) {
                              if (fabs(elements[i]) == 1.0) {
                                   size++;
                              } else {
                                   // bad
                                   tryPlusMinusOne = false;
                              }
                         }
                    }
                    if (!tryPlusMinusOne)
                         break;
               }
          } else {
               // Will add to whatever sort of matrix exists
               tryPlusMinusOne = false;
          }
          if (!tryPlusMinusOne) {
               CoinBigIndex numberElements = buildObject.numberElements();
               CoinBigIndex * starts = new CoinBigIndex [number+1];
               int * row = new int[numberElements];
               double * element = new double[numberElements];
               starts[0] = 0;
               numberElements = 0;
               for (iColumn = 0; iColumn < number; iColumn++) {
                    const int * rows;
                    const double * elements;
                    int numberElementsThis = buildObject.column(iColumn, lower[iColumn], upper[iColumn],
                                             objective[iColumn], rows, elements);
                    CoinMemcpyN(rows, numberElementsThis, row + numberElements);
                    CoinMemcpyN(elements, numberElementsThis, element + numberElements);
                    numberElements += numberElementsThis;
                    starts[iColumn+1] = numberElements;
               }
               addColumns(number, lower, upper, objective, NULL);
               // make sure matrix has enough rows
               matrix_->setDimensions(numberRows_, -1);
               numberErrors = matrix_->appendMatrix(number, 1, starts, row, element,
                                                    checkDuplicates ? numberRows_ : -1);
               delete [] starts;
               delete [] row;
               delete [] element;
          } else {
               // arrays already filled in
               addColumns(number, lower, upper, objective, NULL);
               char * which = NULL; // for duplicates
               if (checkDuplicates) {
                    which = new char[numberRows_];
                    CoinZeroN(which, numberRows_);
               }
               // build +-1 matrix
               CoinBigIndex * startPositive = new CoinBigIndex [number+1];
               CoinBigIndex * startNegative = new CoinBigIndex [number];
               int * indices = new int [size];
               int * neg = new int[maximumLength];
               startPositive[0] = 0;
               size = 0;
               int maxRow = -1;
               for (iColumn = 0; iColumn < number; iColumn++) {
                    const int * rows;
                    const double * elements;
                    int numberElements = buildObject.column(iColumn, lower[iColumn],
                                                            upper[iColumn], objective[iColumn],
                                                            rows, elements);
                    int nNeg = 0;
                    CoinBigIndex start = size;
                    for (int i = 0; i < numberElements; i++) {
                         int iRow = rows[i];
                         if (checkDuplicates) {
                              if (iRow < numberRows_) {
                                   if(which[iRow])
                                        numberErrors++;
                                   else
                                        which[iRow] = 1;
                              } else {
                                   numberErrors++;
                                   // and may as well switch off
                                   checkDuplicates = false;
                              }
                         }
                         maxRow = CoinMax(maxRow, iRow);
                         if (elements[i] == 1.0) {
                              indices[size++] = iRow;
                         } else if (elements[i] == -1.0) {
                              neg[nNeg++] = iRow;
                         }
                    }
                    std::sort(indices + start, indices + size);
                    std::sort(neg, neg + nNeg);
                    startNegative[iColumn] = size;
                    CoinMemcpyN(neg, nNeg, indices + size);
                    size += nNeg;
                    startPositive[iColumn+1] = size;
               }
               delete [] neg;
               // check size
               assert (maxRow + 1 <= numberRows_);
               // Get good object
               delete matrix_;
               ClpPlusMinusOneMatrix * matrix = new ClpPlusMinusOneMatrix();
               matrix->passInCopy(numberRows_, number, true, indices, startPositive, startNegative);
               matrix_ = matrix;
               delete [] which;
          }
          delete [] objective;
          delete [] lower;
          delete [] upper;
     }
     return 0;
}
#endif
#ifndef SLIM_NOIO
// Add columns from a model object
int
ClpModel::addColumns( CoinModel & modelObject, bool tryPlusMinusOne, bool checkDuplicates)
{
     if (modelObject.numberElements() == 0)
          return 0;
     bool goodState = true;
     if (modelObject.rowLowerArray()) {
          // some row information exists
          int numberRows2 = modelObject.numberRows();
          const double * rowLower = modelObject.rowLowerArray();
          const double * rowUpper = modelObject.rowUpperArray();
          for (int i = 0; i < numberRows2; i++) {
               if (rowLower[i] != -COIN_DBL_MAX)
                    goodState = false;
               if (rowUpper[i] != COIN_DBL_MAX)
                    goodState = false;
          }
     }
     if (goodState) {
          // can do addColumns
          int numberErrors = 0;
          // Set arrays for normal use
          double * rowLower = modelObject.rowLowerArray();
          double * rowUpper = modelObject.rowUpperArray();
          double * columnLower = modelObject.columnLowerArray();
          double * columnUpper = modelObject.columnUpperArray();
          double * objective = modelObject.objectiveArray();
          int * integerType = modelObject.integerTypeArray();
          double * associated = modelObject.associatedArray();
          // If strings then do copies
          if (modelObject.stringsExist()) {
               numberErrors = modelObject.createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                                       objective, integerType, associated);
          }
          int numberColumns = numberColumns_; // save number of columns
          int numberColumns2 = modelObject.numberColumns();
          if (numberColumns2 && !numberErrors) {
               CoinBigIndex * startPositive = NULL;
               CoinBigIndex * startNegative = NULL;
               if ((!matrix_ || !matrix_->getNumElements()) && !numberColumns && tryPlusMinusOne) {
                    startPositive = new CoinBigIndex[numberColumns2+1];
                    startNegative = new CoinBigIndex[numberColumns2];
                    modelObject.countPlusMinusOne(startPositive, startNegative, associated);
                    if (startPositive[0] < 0) {
                         // no good
                         tryPlusMinusOne = false;
                         delete [] startPositive;
                         delete [] startNegative;
                    }
               } else {
                    // Will add to whatever sort of matrix exists
                    tryPlusMinusOne = false;
               }
               assert (columnLower);
               addColumns(numberColumns2, columnLower, columnUpper, objective, NULL, NULL, NULL);
#ifndef SLIM_CLP
               if (!tryPlusMinusOne) {
#endif
		    /* addColumns just above extended matrix - to keep
		       things clean - take off again.  I know it is a bit
		       clumsy but won't break anything */
		    {
		      int * which = new int [numberColumns2];
		      for (int i=0;i<numberColumns2;i++)
			which[i]=i+numberColumns;
		      matrix_->deleteCols(numberColumns2,which);
		      delete [] which;
		    }
                    CoinPackedMatrix matrix;
                    modelObject.createPackedMatrix(matrix, associated);
                    assert (!matrix.getExtraGap());
                    if (matrix_->getNumCols()) {
                         const int * row = matrix.getIndices();
                         //const int * columnLength = matrix.getVectorLengths();
                         const CoinBigIndex * columnStart = matrix.getVectorStarts();
                         const double * element = matrix.getElements();
                         // make sure matrix has enough rows
                         matrix_->setDimensions(numberRows_, -1);
                         numberErrors += matrix_->appendMatrix(numberColumns2, 1, columnStart, row, element,
                                                               checkDuplicates ? numberRows_ : -1);
                    } else {
                         delete matrix_;
                         matrix_ = new ClpPackedMatrix(matrix);
                    }
#ifndef SLIM_CLP
               } else {
                    // create +-1 matrix
                    CoinBigIndex size = startPositive[numberColumns2];
                    int * indices = new int[size];
                    modelObject.createPlusMinusOne(startPositive, startNegative, indices,
                                                   associated);
                    // Get good object
                    ClpPlusMinusOneMatrix * matrix = new ClpPlusMinusOneMatrix();
                    matrix->passInCopy(numberRows_, numberColumns2,
                                       true, indices, startPositive, startNegative);
                    delete matrix_;
                    matrix_ = matrix;
               }
#endif
#ifndef CLP_NO_STD
               // Do names if wanted
               if (modelObject.columnNames()->numberItems()) {
                    const char *const * columnNames = modelObject.columnNames()->names();
                    copyColumnNames(columnNames, numberColumns, numberColumns_);
               }
#endif
               // Do integers if wanted
               assert(integerType);
               for (int iColumn = 0; iColumn < numberColumns2; iColumn++) {
                    if (integerType[iColumn])
                         setInteger(iColumn + numberColumns);
               }
          }
          if (columnLower != modelObject.columnLowerArray()) {
               delete [] rowLower;
               delete [] rowUpper;
               delete [] columnLower;
               delete [] columnUpper;
               delete [] objective;
               delete [] integerType;
               delete [] associated;
               if (numberErrors)
                    handler_->message(CLP_BAD_STRING_VALUES, messages_)
                              << numberErrors
                              << CoinMessageEol;
          }
          return numberErrors;
     } else {
          // not suitable for addColumns
          handler_->message(CLP_COMPLICATED_MODEL, messages_)
                    << modelObject.numberRows()
                    << modelObject.numberColumns()
                    << CoinMessageEol;
          return -1;
     }
}
#endif
// chgRowLower
void
ClpModel::chgRowLower(const double * rowLower)
{
     int numberRows = numberRows_;
     int iRow;
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     if (rowLower) {
          for (iRow = 0; iRow < numberRows; iRow++) {
               double value = rowLower[iRow];
               if (value < -1.0e20)
                    value = -COIN_DBL_MAX;
               rowLower_[iRow] = value;
          }
     } else {
          for (iRow = 0; iRow < numberRows; iRow++) {
               rowLower_[iRow] = -COIN_DBL_MAX;
          }
     }
}
// chgRowUpper
void
ClpModel::chgRowUpper(const double * rowUpper)
{
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     int numberRows = numberRows_;
     int iRow;
     if (rowUpper) {
          for (iRow = 0; iRow < numberRows; iRow++) {
               double value = rowUpper[iRow];
               if (value > 1.0e20)
                    value = COIN_DBL_MAX;
               rowUpper_[iRow] = value;
          }
     } else {
          for (iRow = 0; iRow < numberRows; iRow++) {
               rowUpper_[iRow] = COIN_DBL_MAX;;
          }
     }
}
// chgColumnLower
void
ClpModel::chgColumnLower(const double * columnLower)
{
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     int numberColumns = numberColumns_;
     int iColumn;
     if (columnLower) {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               double value = columnLower[iColumn];
               if (value < -1.0e20)
                    value = -COIN_DBL_MAX;
               columnLower_[iColumn] = value;
          }
     } else {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               columnLower_[iColumn] = 0.0;
          }
     }
}
// chgColumnUpper
void
ClpModel::chgColumnUpper(const double * columnUpper)
{
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     int numberColumns = numberColumns_;
     int iColumn;
     if (columnUpper) {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               double value = columnUpper[iColumn];
               if (value > 1.0e20)
                    value = COIN_DBL_MAX;
               columnUpper_[iColumn] = value;
          }
     } else {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               columnUpper_[iColumn] = COIN_DBL_MAX;;
          }
     }
}
// chgObjCoefficients
void
ClpModel::chgObjCoefficients(const double * objIn)
{
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     double * obj = objective();
     int numberColumns = numberColumns_;
     int iColumn;
     if (objIn) {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               obj[iColumn] = objIn[iColumn];
          }
     } else {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               obj[iColumn] = 0.0;
          }
     }
}
// Infeasibility/unbounded ray (NULL returned if none/wrong)
double *
ClpModel::infeasibilityRay(bool fullRay) const
{
     double * array = NULL;
     if (problemStatus_ == 1 && ray_) {
       if (!fullRay) {
	 array = ClpCopyOfArray(ray_, numberRows_);
       } else {
	 array = new double [numberRows_+numberColumns_];
	 memcpy(array,ray_,numberRows_*sizeof(double));
	 memset(array+numberRows_,0,numberColumns_*sizeof(double));
	 transposeTimes(-1.0,array,array+numberRows_);
       }
     }
     return array;
}
double *
ClpModel::unboundedRay() const
{
     double * array = NULL;
     if (problemStatus_ == 2)
          array = ClpCopyOfArray(ray_, numberColumns_);
     return array;
}
void
ClpModel::setMaximumIterations(int value)
{
     if(value >= 0)
          intParam_[ClpMaxNumIteration] = value;
}
void
ClpModel::setMaximumSeconds(double value)
{
     if(value >= 0)
          dblParam_[ClpMaxSeconds] = value + CoinCpuTime();
     else
          dblParam_[ClpMaxSeconds] = -1.0;
}
void
ClpModel::setMaximumWallSeconds(double value)
{
     if(value >= 0)
          dblParam_[ClpMaxWallSeconds] = value + CoinWallclockTime();
     else
          dblParam_[ClpMaxWallSeconds] = -1.0;
}
// Returns true if hit maximum iterations (or time)
bool
ClpModel::hitMaximumIterations() const
{
     // replaced - compiler error? bool hitMax= (numberIterations_>=maximumIterations());
     bool hitMax = (numberIterations_ >= intParam_[ClpMaxNumIteration]);
     if (dblParam_[ClpMaxSeconds] >= 0.0 && !hitMax) {
          hitMax = (CoinCpuTime() >= dblParam_[ClpMaxSeconds]);
     }
     if (dblParam_[ClpMaxWallSeconds] >= 0.0 && !hitMax) {
          hitMax = (CoinWallclockTime() >= dblParam_[ClpMaxWallSeconds]);
     }
     return hitMax;
}
// On stopped - sets secondary status
void
ClpModel::onStopped()
{
     if (problemStatus_ == 3) {
          secondaryStatus_ = 0;
          if ((CoinCpuTime() >= dblParam_[ClpMaxSeconds] && dblParam_[ClpMaxSeconds] >= 0.0) ||
	      (CoinWallclockTime() >= dblParam_[ClpMaxWallSeconds] && dblParam_[ClpMaxWallSeconds] >= 0.0))
               secondaryStatus_ = 9;
     }
}
// Pass in Message handler (not deleted at end)
void
ClpModel::passInMessageHandler(CoinMessageHandler * handler)
{
     if (defaultHandler_)
          delete handler_;
     defaultHandler_ = false;
     handler_ = handler;
}
// Pass in Message handler (not deleted at end) and return current
CoinMessageHandler *
ClpModel::pushMessageHandler(CoinMessageHandler * handler,
                             bool & oldDefault)
{
     CoinMessageHandler * returnValue = handler_;
     oldDefault = defaultHandler_;
     defaultHandler_ = false;
     handler_ = handler;
     return returnValue;
}
// back to previous message handler
void
ClpModel::popMessageHandler(CoinMessageHandler * oldHandler, bool oldDefault)
{
     if (defaultHandler_)
          delete handler_;
     defaultHandler_ = oldDefault;
     handler_ = oldHandler;
}
// Overrides message handler with a default one
void 
ClpModel::setDefaultMessageHandler()
{
     int logLevel = handler_->logLevel();
     if (defaultHandler_)
          delete handler_;
     defaultHandler_ = true;
     handler_ = new CoinMessageHandler();
     handler_->setLogLevel(logLevel);
}
// Set language
void
ClpModel::newLanguage(CoinMessages::Language language)
{
     messages_ = ClpMessage(language);
}
#ifndef SLIM_NOIO
// Read an mps file from the given filename
int
ClpModel::readMps(const char *fileName,
                  bool keepNames,
                  bool ignoreErrors)
{
     if (!strcmp(fileName, "-") || !strcmp(fileName, "stdin")) {
          // stdin
     } else {
          std::string name = fileName;
          bool readable = fileCoinReadable(name);
          if (!readable) {
               handler_->message(CLP_UNABLE_OPEN, messages_)
                         << fileName << CoinMessageEol;
               return -1;
          }
     }
     CoinMpsIO m;
     m.passInMessageHandler(handler_);
     *m.messagesPointer() = coinMessages();
     bool savePrefix = m.messageHandler()->prefix();
     m.messageHandler()->setPrefix(handler_->prefix());
     m.setSmallElementValue(CoinMax(smallElement_, m.getSmallElementValue()));
     double time1 = CoinCpuTime(), time2;
     int status = 0;
     try {
          status = m.readMps(fileName, "");
     } catch (CoinError e) {
          e.print();
          status = -1;
     }
     m.messageHandler()->setPrefix(savePrefix);
     if (!status || (ignoreErrors && (status > 0 && status < 100000))) {
          loadProblem(*m.getMatrixByCol(),
                      m.getColLower(), m.getColUpper(),
                      m.getObjCoefficients(),
                      m.getRowLower(), m.getRowUpper());
          if (m.integerColumns()) {
               integerType_ = new char[numberColumns_];
               CoinMemcpyN(m.integerColumns(), numberColumns_, integerType_);
          } else {
               integerType_ = NULL;
          }
#ifndef SLIM_CLP
          // get quadratic part
          if (m.reader()->whichSection (  ) == COIN_QUAD_SECTION ) {
               int * start = NULL;
               int * column = NULL;
               double * element = NULL;
               status = m.readQuadraticMps(NULL, start, column, element, 2);
               if (!status || ignoreErrors)
                    loadQuadraticObjective(numberColumns_, start, column, element);
               delete [] start;
               delete [] column;
               delete [] element;
          }
#endif
#ifndef CLP_NO_STD
          // set problem name
          setStrParam(ClpProbName, m.getProblemName());
          // do names
          if (keepNames) {
               unsigned int maxLength = 0;
               int iRow;
               rowNames_ = std::vector<std::string> ();
               columnNames_ = std::vector<std::string> ();
               rowNames_.reserve(numberRows_);
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    const char * name = m.rowName(iRow);
                    maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(name)));
                    rowNames_.push_back(name);
               }

               int iColumn;
               columnNames_.reserve(numberColumns_);
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    const char * name = m.columnName(iColumn);
                    maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(name)));
                    columnNames_.push_back(name);
               }
               lengthNames_ = static_cast<int> (maxLength);
          } else {
               lengthNames_ = 0;
          }
#endif
          setDblParam(ClpObjOffset, m.objectiveOffset());
          time2 = CoinCpuTime();
          handler_->message(CLP_IMPORT_RESULT, messages_)
                    << fileName
                    << time2 - time1 << CoinMessageEol;
     } else {
          // errors
          handler_->message(CLP_IMPORT_ERRORS, messages_)
                    << status << fileName << CoinMessageEol;
     }

     return status;
}
// Read GMPL files from the given filenames
int
ClpModel::readGMPL(const char *fileName, const char * dataName,
                   bool keepNames)
{
     FILE *fp = fopen(fileName, "r");
     if (fp) {
          // can open - lets go for it
          fclose(fp);
          if (dataName) {
               fp = fopen(dataName, "r");
               if (fp) {
                    fclose(fp);
               } else {
                    handler_->message(CLP_UNABLE_OPEN, messages_)
                              << dataName << CoinMessageEol;
                    return -1;
               }
          }
     } else {
          handler_->message(CLP_UNABLE_OPEN, messages_)
                    << fileName << CoinMessageEol;
          return -1;
     }
     CoinMpsIO m;
     m.passInMessageHandler(handler_);
     *m.messagesPointer() = coinMessages();
     bool savePrefix = m.messageHandler()->prefix();
     m.messageHandler()->setPrefix(handler_->prefix());
     double time1 = CoinCpuTime(), time2;
     int status = m.readGMPL(fileName, dataName, keepNames);
     m.messageHandler()->setPrefix(savePrefix);
     if (!status) {
          loadProblem(*m.getMatrixByCol(),
                      m.getColLower(), m.getColUpper(),
                      m.getObjCoefficients(),
                      m.getRowLower(), m.getRowUpper());
          if (m.integerColumns()) {
               integerType_ = new char[numberColumns_];
               CoinMemcpyN(m.integerColumns(), numberColumns_, integerType_);
          } else {
               integerType_ = NULL;
          }
#ifndef CLP_NO_STD
          // set problem name
          setStrParam(ClpProbName, m.getProblemName());
          // do names
          if (keepNames) {
               unsigned int maxLength = 0;
               int iRow;
               rowNames_ = std::vector<std::string> ();
               columnNames_ = std::vector<std::string> ();
               rowNames_.reserve(numberRows_);
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    const char * name = m.rowName(iRow);
                    maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(name)));
                    rowNames_.push_back(name);
               }

               int iColumn;
               columnNames_.reserve(numberColumns_);
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    const char * name = m.columnName(iColumn);
                    maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(name)));
                    columnNames_.push_back(name);
               }
               lengthNames_ = static_cast<int> (maxLength);
          } else {
               lengthNames_ = 0;
          }
#endif
          setDblParam(ClpObjOffset, m.objectiveOffset());
          time2 = CoinCpuTime();
          handler_->message(CLP_IMPORT_RESULT, messages_)
                    << fileName
                    << time2 - time1 << CoinMessageEol;
     } else {
          // errors
          handler_->message(CLP_IMPORT_ERRORS, messages_)
                    << status << fileName << CoinMessageEol;
     }
     return status;
}
#endif
bool ClpModel::isPrimalObjectiveLimitReached() const
{
     double limit = 0.0;
     getDblParam(ClpPrimalObjectiveLimit, limit);
     if (limit > 1e30) {
          // was not ever set
          return false;
     }

     const double obj = objectiveValue();
     const double maxmin = optimizationDirection();

     if (problemStatus_ == 0) // optimal
          return maxmin > 0 ? (obj < limit) /*minim*/ : (-obj < limit) /*maxim*/;
     else if (problemStatus_ == 2)
          return true;
     else
          return false;
}

bool ClpModel::isDualObjectiveLimitReached() const
{

     double limit = 0.0;
     getDblParam(ClpDualObjectiveLimit, limit);
     if (limit > 1e30) {
          // was not ever set
          return false;
     }

     const double obj = objectiveValue();
     const double maxmin = optimizationDirection();

     if (problemStatus_ == 0) // optimal
          return maxmin > 0 ? (obj > limit) /*minim*/ : (-obj > limit) /*maxim*/;
     else if (problemStatus_ == 1)
          return true;
     else
          return false;

}
void
ClpModel::copyInIntegerInformation(const char * information)
{
     delete [] integerType_;
     if (information) {
          integerType_ = new char[numberColumns_];
          CoinMemcpyN(information, numberColumns_, integerType_);
     } else {
          integerType_ = NULL;
     }
}
void
ClpModel::setContinuous(int index)
{

     if (integerType_) {
#ifndef NDEBUG
          if (index < 0 || index >= numberColumns_) {
               indexError(index, "setContinuous");
          }
#endif
          integerType_[index] = 0;
     }
}
//-----------------------------------------------------------------------------
void
ClpModel::setInteger(int index)
{
     if (!integerType_) {
          integerType_ = new char[numberColumns_];
          CoinZeroN ( integerType_, numberColumns_);
     }
#ifndef NDEBUG
     if (index < 0 || index >= numberColumns_) {
          indexError(index, "setInteger");
     }
#endif
     integerType_[index] = 1;
}
/* Return true if the index-th variable is an integer variable */
bool
ClpModel::isInteger(int index) const
{
     if (!integerType_) {
          return false;
     } else {
#ifndef NDEBUG
          if (index < 0 || index >= numberColumns_) {
               indexError(index, "isInteger");
          }
#endif
          return (integerType_[index] != 0);
     }
}
#ifndef CLP_NO_STD
// Drops names - makes lengthnames 0 and names empty
void
ClpModel::dropNames()
{
     lengthNames_ = 0;
     rowNames_ = std::vector<std::string> ();
     columnNames_ = std::vector<std::string> ();
}
#endif
// Drop integer informations
void
ClpModel::deleteIntegerInformation()
{
     delete [] integerType_;
     integerType_ = NULL;
}
/* Return copy of status array (char[numberRows+numberColumns]),
   use delete [] */
unsigned char *
ClpModel::statusCopy() const
{
     return ClpCopyOfArray(status_, numberRows_ + numberColumns_);
}
// Copy in status vector
void
ClpModel::copyinStatus(const unsigned char * statusArray)
{
     delete [] status_;
     if (statusArray) {
          status_ = new unsigned char [numberRows_+numberColumns_];
          CoinMemcpyN(statusArray, (numberRows_ + numberColumns_), status_);
     } else {
          status_ = NULL;
     }
}
#ifndef SLIM_CLP
// Load up quadratic objective
void
ClpModel::loadQuadraticObjective(const int numberColumns, const CoinBigIndex * start,
                                 const int * column, const double * element)
{
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     CoinAssert (numberColumns == numberColumns_);
     assert ((dynamic_cast< ClpLinearObjective*>(objective_)));
     double offset;
     ClpObjective * obj = new ClpQuadraticObjective(objective_->gradient(NULL, NULL, offset, false),
               numberColumns,
               start, column, element);
     delete objective_;
     objective_ = obj;

}
void
ClpModel::loadQuadraticObjective (  const CoinPackedMatrix& matrix)
{
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     CoinAssert (matrix.getNumCols() == numberColumns_);
     assert ((dynamic_cast< ClpLinearObjective*>(objective_)));
     double offset;
     ClpQuadraticObjective * obj =
          new ClpQuadraticObjective(objective_->gradient(NULL, NULL, offset, false),
                                    numberColumns_,
                                    NULL, NULL, NULL);
     delete objective_;
     objective_ = obj;
     obj->loadQuadraticObjective(matrix);
}
// Get rid of quadratic objective
void
ClpModel::deleteQuadraticObjective()
{
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     ClpQuadraticObjective * obj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
     if (obj)
          obj->deleteQuadraticObjective();
}
#endif
void
ClpModel::setObjective(ClpObjective * objective)
{
     whatsChanged_ = 0; // Use ClpSimplex stuff to keep
     delete objective_;
     objective_ = objective->clone();
}
// Returns resized array and updates size
double * whichDouble(double * array , int number, const int * which)
{
     double * newArray = NULL;
     if (array && number) {
          int i ;
          newArray = new double[number];
          for (i = 0; i < number; i++)
               newArray[i] = array[which[i]];
     }
     return newArray;
}
char * whichChar(char * array , int number, const int * which)
{
     char * newArray = NULL;
     if (array && number) {
          int i ;
          newArray = new char[number];
          for (i = 0; i < number; i++)
               newArray[i] = array[which[i]];
     }
     return newArray;
}
unsigned char * whichUnsignedChar(unsigned char * array ,
                                  int number, const int * which)
{
     unsigned char * newArray = NULL;
     if (array && number) {
          int i ;
          newArray = new unsigned char[number];
          for (i = 0; i < number; i++)
               newArray[i] = array[which[i]];
     }
     return newArray;
}
// Replace Clp Matrix (current is not deleted)
void
ClpModel::replaceMatrix( ClpMatrixBase * matrix, bool deleteCurrent)
{
     if (deleteCurrent)
          delete matrix_;
     matrix_ = matrix;
     whatsChanged_ = 0; // Too big a change
}
// Subproblem constructor
ClpModel::ClpModel ( const ClpModel * rhs,
                     int numberRows, const int * whichRow,
                     int numberColumns, const int * whichColumn,
                     bool dropNames, bool dropIntegers)
     :  specialOptions_(rhs->specialOptions_),
        maximumColumns_(-1),
        maximumRows_(-1),
        maximumInternalColumns_(-1),
        maximumInternalRows_(-1),
        savedRowScale_(NULL),
        savedColumnScale_(NULL)
{
     defaultHandler_ = rhs->defaultHandler_;
     if (defaultHandler_)
          handler_ = new CoinMessageHandler(*rhs->handler_);
     else
          handler_ = rhs->handler_;
     eventHandler_ = rhs->eventHandler_->clone();
     randomNumberGenerator_ = rhs->randomNumberGenerator_;
     messages_ = rhs->messages_;
     coinMessages_ = rhs->coinMessages_;
     maximumColumns_ = -1;
     maximumRows_ = -1;
     maximumInternalColumns_ = -1;
     maximumInternalRows_ = -1;
     savedRowScale_ = NULL;
     savedColumnScale_ = NULL;
     intParam_[ClpMaxNumIteration] = rhs->intParam_[ClpMaxNumIteration];
     intParam_[ClpMaxNumIterationHotStart] =
          rhs->intParam_[ClpMaxNumIterationHotStart];
     intParam_[ClpNameDiscipline] = rhs->intParam_[ClpNameDiscipline] ;

     dblParam_[ClpDualObjectiveLimit] = rhs->dblParam_[ClpDualObjectiveLimit];
     dblParam_[ClpPrimalObjectiveLimit] = rhs->dblParam_[ClpPrimalObjectiveLimit];
     dblParam_[ClpDualTolerance] = rhs->dblParam_[ClpDualTolerance];
     dblParam_[ClpPrimalTolerance] = rhs->dblParam_[ClpPrimalTolerance];
     dblParam_[ClpObjOffset] = rhs->dblParam_[ClpObjOffset];
     dblParam_[ClpMaxSeconds] = rhs->dblParam_[ClpMaxSeconds];
     dblParam_[ClpMaxWallSeconds] = rhs->dblParam_[ClpMaxWallSeconds];
     dblParam_[ClpPresolveTolerance] = rhs->dblParam_[ClpPresolveTolerance];
#ifndef CLP_NO_STD
     strParam_[ClpProbName] = rhs->strParam_[ClpProbName];
#endif
     specialOptions_ = rhs->specialOptions_;
     optimizationDirection_ = rhs->optimizationDirection_;
     objectiveValue_ = rhs->objectiveValue_;
     smallElement_ = rhs->smallElement_;
     objectiveScale_ = rhs->objectiveScale_;
     rhsScale_ = rhs->rhsScale_;
     numberIterations_ = rhs->numberIterations_;
     solveType_ = rhs->solveType_;
     whatsChanged_ = 0; // Too big a change
     problemStatus_ = rhs->problemStatus_;
     secondaryStatus_ = rhs->secondaryStatus_;
     // check valid lists
     int numberBad = 0;
     int i;
     for (i = 0; i < numberRows; i++)
          if (whichRow[i] < 0 || whichRow[i] >= rhs->numberRows_)
               numberBad++;
     CoinAssertHint(!numberBad, "Bad row list for subproblem constructor");
     numberBad = 0;
     for (i = 0; i < numberColumns; i++)
          if (whichColumn[i] < 0 || whichColumn[i] >= rhs->numberColumns_)
               numberBad++;
     CoinAssertHint(!numberBad, "Bad Column list for subproblem constructor");
     numberRows_ = numberRows;
     numberColumns_ = numberColumns;
     userPointer_ = rhs->userPointer_;
     trustedUserPointer_ = rhs->trustedUserPointer_;
     numberThreads_ = 0;
#ifndef CLP_NO_STD
     if (!dropNames) {
          unsigned int maxLength = 0;
          int iRow;
          rowNames_ = std::vector<std::string> ();
          columnNames_ = std::vector<std::string> ();
          rowNames_.reserve(numberRows_);
          for (iRow = 0; iRow < numberRows_; iRow++) {
               rowNames_.push_back(rhs->rowNames_[whichRow[iRow]]);
               maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(rowNames_[iRow].c_str())));
          }
          int iColumn;
          columnNames_.reserve(numberColumns_);
          for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
               columnNames_.push_back(rhs->columnNames_[whichColumn[iColumn]]);
               maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(columnNames_[iColumn].c_str())));
          }
          lengthNames_ = static_cast<int> (maxLength);
     } else {
          lengthNames_ = 0;
          rowNames_ = std::vector<std::string> ();
          columnNames_ = std::vector<std::string> ();
     }
#endif
     if (rhs->integerType_ && !dropIntegers) {
          integerType_ = whichChar(rhs->integerType_, numberColumns, whichColumn);
     } else {
          integerType_ = NULL;
     }
     if (rhs->rowActivity_) {
          rowActivity_ = whichDouble(rhs->rowActivity_, numberRows, whichRow);
          dual_ = whichDouble(rhs->dual_, numberRows, whichRow);
          columnActivity_ = whichDouble(rhs->columnActivity_, numberColumns,
                                        whichColumn);
          reducedCost_ = whichDouble(rhs->reducedCost_, numberColumns,
                                     whichColumn);
     } else {
          rowActivity_ = NULL;
          columnActivity_ = NULL;
          dual_ = NULL;
          reducedCost_ = NULL;
     }
     rowLower_ = whichDouble(rhs->rowLower_, numberRows, whichRow);
     rowUpper_ = whichDouble(rhs->rowUpper_, numberRows, whichRow);
     columnLower_ = whichDouble(rhs->columnLower_, numberColumns, whichColumn);
     columnUpper_ = whichDouble(rhs->columnUpper_, numberColumns, whichColumn);
     if (rhs->objective_)
          objective_  = rhs->objective_->subsetClone(numberColumns, whichColumn);
     else
          objective_ = NULL;
     rowObjective_ = whichDouble(rhs->rowObjective_, numberRows, whichRow);
     // status has to be done in two stages
     if (rhs->status_) {
       status_ = new unsigned char[numberColumns_+numberRows_];
       unsigned char * rowStatus = whichUnsignedChar(rhs->status_ + rhs->numberColumns_,
						     numberRows_, whichRow);
       unsigned char * columnStatus = whichUnsignedChar(rhs->status_,
							numberColumns_, whichColumn);
       CoinMemcpyN(rowStatus, numberRows_, status_ + numberColumns_);
       delete [] rowStatus;
       CoinMemcpyN(columnStatus, numberColumns_, status_);
       delete [] columnStatus;
     } else {
       status_=NULL;
     }
     ray_ = NULL;
     if (problemStatus_ == 1)
          ray_ = whichDouble (rhs->ray_, numberRows, whichRow);
     else if (problemStatus_ == 2)
          ray_ = whichDouble (rhs->ray_, numberColumns, whichColumn);
     rowScale_ = NULL;
     columnScale_ = NULL;
     inverseRowScale_ = NULL;
     inverseColumnScale_ = NULL;
     scalingFlag_ = rhs->scalingFlag_;
     rowCopy_ = NULL;
     scaledMatrix_ = NULL;
     matrix_ = NULL;
     if (rhs->matrix_) {
          matrix_ = rhs->matrix_->subsetClone(numberRows, whichRow,
                                              numberColumns, whichColumn);
     }
     randomNumberGenerator_ = rhs->randomNumberGenerator_;
}
#ifndef CLP_NO_STD
// Copies in names
void
ClpModel::copyNames(const std::vector<std::string> & rowNames,
                    const std::vector<std::string> & columnNames)
{
     unsigned int maxLength = 0;
     int iRow;
     rowNames_ = std::vector<std::string> ();
     columnNames_ = std::vector<std::string> ();
     rowNames_.reserve(numberRows_);
     for (iRow = 0; iRow < numberRows_; iRow++) {
          rowNames_.push_back(rowNames[iRow]);
          maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(rowNames_[iRow].c_str())));
     }
     int iColumn;
     columnNames_.reserve(numberColumns_);
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          columnNames_.push_back(columnNames[iColumn]);
          maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(columnNames_[iColumn].c_str())));
     }
     lengthNames_ = static_cast<int> (maxLength);
}
// Return name or Rnnnnnnn
std::string
ClpModel::getRowName(int iRow) const
{
#ifndef NDEBUG
     if (iRow < 0 || iRow >= numberRows_) {
          indexError(iRow, "getRowName");
     }
#endif
     int size = static_cast<int>(rowNames_.size());
     if (size > iRow) {
          return rowNames_[iRow];
     } else {
          char name[9];
          sprintf(name, "R%7.7d", iRow);
          std::string rowName(name);
          return rowName;
     }
}
// Set row name
void
ClpModel::setRowName(int iRow, std::string &name)
{
#ifndef NDEBUG
     if (iRow < 0 || iRow >= numberRows_) {
          indexError(iRow, "setRowName");
     }
#endif
     unsigned int maxLength = lengthNames_;
     int size = static_cast<int>(rowNames_.size());
     if (size <= iRow)
          rowNames_.resize(iRow + 1);
     rowNames_[iRow] = name;
     maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(name.c_str())));
     // May be too big - but we would have to check both rows and columns to be exact
     lengthNames_ = static_cast<int> (maxLength);
}
// Return name or Cnnnnnnn
std::string
ClpModel::getColumnName(int iColumn) const
{
#ifndef NDEBUG
     if (iColumn < 0 || iColumn >= numberColumns_) {
          indexError(iColumn, "getColumnName");
     }
#endif
     int size = static_cast<int>(columnNames_.size());
     if (size > iColumn) {
          return columnNames_[iColumn];
     } else {
          char name[9];
          sprintf(name, "C%7.7d", iColumn);
          std::string columnName(name);
          return columnName;
     }
}
// Set column name
void
ClpModel::setColumnName(int iColumn, std::string &name)
{
#ifndef NDEBUG
     if (iColumn < 0 || iColumn >= numberColumns_) {
          indexError(iColumn, "setColumnName");
     }
#endif
     unsigned int maxLength = lengthNames_;
     int size = static_cast<int>(columnNames_.size());
     if (size <= iColumn)
          columnNames_.resize(iColumn + 1);
     columnNames_[iColumn] = name;
     maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(name.c_str())));
     // May be too big - but we would have to check both columns and columns to be exact
     lengthNames_ = static_cast<int> (maxLength);
}
// Copies in Row names - modifies names first .. last-1
void
ClpModel::copyRowNames(const std::vector<std::string> & rowNames, int first, int last)
{
     // Do column names if necessary
     if (!lengthNames_&&numberColumns_) {
       lengthNames_=8;
       copyColumnNames(NULL,0,numberColumns_);
     }
     unsigned int maxLength = lengthNames_;
     int size = static_cast<int>(rowNames_.size());
     if (size != numberRows_)
          rowNames_.resize(numberRows_);
     int iRow;
     for (iRow = first; iRow < last; iRow++) {
          rowNames_[iRow] = rowNames[iRow-first];
          maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(rowNames_[iRow-first].c_str())));
     }
     // May be too big - but we would have to check both rows and columns to be exact
     lengthNames_ = static_cast<int> (maxLength);
}
// Copies in Column names - modifies names first .. last-1
void
ClpModel::copyColumnNames(const std::vector<std::string> & columnNames, int first, int last)
{
     // Do row names if necessary
     if (!lengthNames_&&numberRows_) {
       lengthNames_=8;
       copyRowNames(NULL,0,numberRows_);
     }
     unsigned int maxLength = lengthNames_;
     int size = static_cast<int>(columnNames_.size());
     if (size != numberColumns_)
          columnNames_.resize(numberColumns_);
     int iColumn;
     for (iColumn = first; iColumn < last; iColumn++) {
          columnNames_[iColumn] = columnNames[iColumn-first];
          maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(columnNames_[iColumn-first].c_str())));
     }
     // May be too big - but we would have to check both rows and columns to be exact
     lengthNames_ = static_cast<int> (maxLength);
}
// Copies in Row names - modifies names first .. last-1
void
ClpModel::copyRowNames(const char * const * rowNames, int first, int last)
{
     // Do column names if necessary
     if (!lengthNames_&&numberColumns_) {
       lengthNames_=8;
       copyColumnNames(NULL,0,numberColumns_);
     }
     unsigned int maxLength = lengthNames_;
     int size = static_cast<int>(rowNames_.size());
     if (size != numberRows_)
          rowNames_.resize(numberRows_);
     int iRow;
     for (iRow = first; iRow < last; iRow++) {
          if (rowNames && rowNames[iRow-first] && strlen(rowNames[iRow-first])) {
               rowNames_[iRow] = rowNames[iRow-first];
               maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(rowNames[iRow-first])));
          } else {
               maxLength = CoinMax(maxLength, static_cast<unsigned int> (8));
               char name[9];
               sprintf(name, "R%7.7d", iRow);
               rowNames_[iRow] = name;
          }
     }
     // May be too big - but we would have to check both rows and columns to be exact
     lengthNames_ = static_cast<int> (maxLength);
}
// Copies in Column names - modifies names first .. last-1
void
ClpModel::copyColumnNames(const char * const * columnNames, int first, int last)
{
     // Do row names if necessary
     if (!lengthNames_&&numberRows_) {
       lengthNames_=8;
       copyRowNames(NULL,0,numberRows_);
     }
     unsigned int maxLength = lengthNames_;
     int size = static_cast<int>(columnNames_.size());
     if (size != numberColumns_)
          columnNames_.resize(numberColumns_);
     int iColumn;
     for (iColumn = first; iColumn < last; iColumn++) {
          if (columnNames && columnNames[iColumn-first] && strlen(columnNames[iColumn-first])) {
               columnNames_[iColumn] = columnNames[iColumn-first];
               maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(columnNames[iColumn-first])));
          } else {
               maxLength = CoinMax(maxLength, static_cast<unsigned int> (8));
               char name[9];
               sprintf(name, "C%7.7d", iColumn);
               columnNames_[iColumn] = name;
          }
     }
     // May be too big - but we would have to check both rows and columns to be exact
     lengthNames_ = static_cast<int> (maxLength);
}
#endif
// Primal objective limit
void
ClpModel::setPrimalObjectiveLimit(double value)
{
     dblParam_[ClpPrimalObjectiveLimit] = value;
}
// Dual objective limit
void
ClpModel::setDualObjectiveLimit(double value)
{
     dblParam_[ClpDualObjectiveLimit] = value;
}
// Objective offset
void
ClpModel::setObjectiveOffset(double value)
{
     dblParam_[ClpObjOffset] = value;
}
// Solve a problem with no elements - return status
int ClpModel::emptyProblem(int * infeasNumber, double * infeasSum, bool printMessage)
{
     secondaryStatus_ = 6; // so user can see something odd
     if (printMessage)
          handler_->message(CLP_EMPTY_PROBLEM, messages_)
                    << numberRows_
                    << numberColumns_
                    << 0
                    << CoinMessageEol;
     int returnCode = 0;
     if (numberRows_ || numberColumns_) {
          if (!status_) {
               status_ = new unsigned char[numberRows_+numberColumns_];
               CoinZeroN(status_, numberRows_ + numberColumns_);
          }
     }
     // status is set directly (as can be used by Interior methods)
     // check feasible
     int numberPrimalInfeasibilities = 0;
     double sumPrimalInfeasibilities = 0.0;
     int numberDualInfeasibilities = 0;
     double sumDualInfeasibilities = 0.0;
     if (numberRows_) {
          for (int i = 0; i < numberRows_; i++) {
               dual_[i] = 0.0;
               if (rowLower_[i] <= rowUpper_[i]) {
                    if (rowLower_[i] > -1.0e30 || rowUpper_[i] < 1.0e30) {
                         if (rowLower_[i] <= 0.0 && rowUpper_[i] >= 0.0) {
                              if (fabs(rowLower_[i]) < fabs(rowUpper_[i]))
                                   rowActivity_[i] = rowLower_[i];
                              else
                                   rowActivity_[i] = rowUpper_[i];
                         } else {
                              rowActivity_[i] = 0.0;
                              numberPrimalInfeasibilities++;
                              sumPrimalInfeasibilities += CoinMin(rowLower_[i], -rowUpper_[i]);
                              returnCode = 1;
                         }
                    } else {
                         rowActivity_[i] = 0.0;
                    }
               } else {
                    rowActivity_[i] = 0.0;
                    numberPrimalInfeasibilities++;
                    sumPrimalInfeasibilities += rowLower_[i] - rowUpper_[i];
                    returnCode = 1;
               }
               status_[i+numberColumns_] = 1;
          }
     }
     objectiveValue_ = 0.0;
     if (numberColumns_) {
          const double * cost = objective();
          for (int i = 0; i < numberColumns_; i++) {
               reducedCost_[i] = cost[i];
               double objValue = cost[i] * optimizationDirection_;
               if (columnLower_[i] <= columnUpper_[i]) {
                    if (columnLower_[i] > -1.0e30 || columnUpper_[i] < 1.0e30) {
                         if (!objValue) {
                              if (fabs(columnLower_[i]) < fabs(columnUpper_[i])) {
                                   columnActivity_[i] = columnLower_[i];
                                   status_[i] = 3;
                              } else {
                                   columnActivity_[i] = columnUpper_[i];
                                   status_[i] = 2;
                              }
                         } else if (objValue > 0.0) {
                              if (columnLower_[i] > -1.0e30) {
                                   columnActivity_[i] = columnLower_[i];
                                   status_[i] = 3;
                              } else {
                                   columnActivity_[i] = columnUpper_[i];
                                   status_[i] = 2;
                                   numberDualInfeasibilities++;;
                                   sumDualInfeasibilities += fabs(objValue);
                                   returnCode |= 2;
                              }
                              objectiveValue_ += columnActivity_[i] * objValue;
                         } else {
                              if (columnUpper_[i] < 1.0e30) {
                                   columnActivity_[i] = columnUpper_[i];
                                   status_[i] = 2;
                              } else {
                                   columnActivity_[i] = columnLower_[i];
                                   status_[i] = 3;
                                   numberDualInfeasibilities++;;
                                   sumDualInfeasibilities += fabs(objValue);
                                   returnCode |= 2;
                              }
                              objectiveValue_ += columnActivity_[i] * objValue;
                         }
                    } else {
                         columnActivity_[i] = 0.0;
                         if (objValue) {
                              numberDualInfeasibilities++;;
                              sumDualInfeasibilities += fabs(objValue);
                              returnCode |= 2;
                         }
                         status_[i] = 0;
                    }
               } else {
                    if (fabs(columnLower_[i]) < fabs(columnUpper_[i])) {
                         columnActivity_[i] = columnLower_[i];
                         status_[i] = 3;
                    } else {
                         columnActivity_[i] = columnUpper_[i];
                         status_[i] = 2;
                    }
                    numberPrimalInfeasibilities++;
                    sumPrimalInfeasibilities += columnLower_[i] - columnUpper_[i];
                    returnCode |= 1;
               }
          }
     }
     objectiveValue_ /= (objectiveScale_ * rhsScale_);
     if (infeasNumber) {
          infeasNumber[0] = numberDualInfeasibilities;
          infeasSum[0] = sumDualInfeasibilities;
          infeasNumber[1] = numberPrimalInfeasibilities;
          infeasSum[1] = sumPrimalInfeasibilities;
     }
     if (returnCode == 3)
          returnCode = 4;
     return returnCode;
}
#ifndef SLIM_NOIO
/* Write the problem in MPS format to the specified file.

Row and column names may be null.
formatType is
<ul>
<li> 0 - normal
<li> 1 - extra accuracy
<li> 2 - IEEE hex (later)
</ul>

Returns non-zero on I/O error
*/
int
ClpModel::writeMps(const char *filename,
                   int formatType, int numberAcross,
                   double objSense) const
{
     matrix_->setDimensions(numberRows_, numberColumns_);

     // Get multiplier for objective function - default 1.0
     double * objective = new double[numberColumns_];
     CoinMemcpyN(getObjCoefficients(), numberColumns_, objective);
     if (objSense * getObjSense() < 0.0) {
          for (int i = 0; i < numberColumns_; ++i)
               objective [i] = - objective[i];
     }
     // get names
     const char * const * const rowNames = rowNamesAsChar();
     const char * const * const columnNames = columnNamesAsChar();
     CoinMpsIO writer;
     writer.passInMessageHandler(handler_);
     *writer.messagesPointer() = coinMessages();
     writer.setMpsData(*(matrix_->getPackedMatrix()), COIN_DBL_MAX,
                       getColLower(), getColUpper(),
                       objective,
                       reinterpret_cast<const char*> (NULL) /*integrality*/,
                       getRowLower(), getRowUpper(),
                       columnNames, rowNames);
     // Pass in array saying if each variable integer
     writer.copyInIntegerInformation(integerInformation());
     writer.setObjectiveOffset(objectiveOffset());
     delete [] objective;
     CoinPackedMatrix * quadratic = NULL;
#ifndef SLIM_CLP
     // allow for quadratic objective
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (objective_->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
     if (quadraticObj)
          quadratic = quadraticObj->quadraticObjective();
#endif
     int returnCode = writer.writeMps(filename, 0 /* do not gzip it*/, formatType, numberAcross,
                                      quadratic);
     if (rowNames) {
          deleteNamesAsChar(rowNames, numberRows_ + 1);
          deleteNamesAsChar(columnNames, numberColumns_);
     }
     return returnCode;
}
#ifndef CLP_NO_STD
// Create row names as char **
const char * const *
ClpModel::rowNamesAsChar() const
{
     char ** rowNames = NULL;
     if (lengthNames()) {
          rowNames = new char * [numberRows_+1];
          int numberNames = static_cast<int>(rowNames_.size());
          numberNames = CoinMin(numberRows_, numberNames);
          int iRow;
          for (iRow = 0; iRow < numberNames; iRow++) {
               if (rowName(iRow) != "") {
                    rowNames[iRow] =
                         CoinStrdup(rowName(iRow).c_str());
               } else {
                    char name[9];
                    sprintf(name, "R%7.7d", iRow);
                    rowNames[iRow] = CoinStrdup(name);
               }
#ifdef STRIPBLANKS
               char * xx = rowNames[iRow];
               int i;
               int length = strlen(xx);
               int n = 0;
               for (i = 0; i < length; i++) {
                    if (xx[i] != ' ')
                         xx[n++] = xx[i];
               }
               xx[n] = '\0';
#endif
          }
          char name[9];
          for ( ; iRow < numberRows_; iRow++) {
               sprintf(name, "R%7.7d", iRow);
               rowNames[iRow] = CoinStrdup(name);
          }
          rowNames[numberRows_] = CoinStrdup("OBJROW");
     }
     return reinterpret_cast<const char * const *>(rowNames);
}
// Create column names as char **
const char * const *
ClpModel::columnNamesAsChar() const
{
     char ** columnNames = NULL;
     if (lengthNames()) {
          columnNames = new char * [numberColumns_];
          int numberNames = static_cast<int>(columnNames_.size());
          numberNames = CoinMin(numberColumns_, numberNames);
          int iColumn;
          for (iColumn = 0; iColumn < numberNames; iColumn++) {
               if (columnName(iColumn) != "") {
                    columnNames[iColumn] =
                         CoinStrdup(columnName(iColumn).c_str());
               } else {
                    char name[9];
                    sprintf(name, "C%7.7d", iColumn);
                    columnNames[iColumn] = CoinStrdup(name);
               }
#ifdef STRIPBLANKS
               char * xx = columnNames[iColumn];
               int i;
               int length = strlen(xx);
               int n = 0;
               for (i = 0; i < length; i++) {
                    if (xx[i] != ' ')
                         xx[n++] = xx[i];
               }
               xx[n] = '\0';
#endif
          }
          char name[9];
          for ( ; iColumn < numberColumns_; iColumn++) {
               sprintf(name, "C%7.7d", iColumn);
               columnNames[iColumn] = CoinStrdup(name);
          }
     }
     return /*reinterpret_cast<const char * const *>*/(columnNames);
}
// Delete char * version of names
void
ClpModel::deleteNamesAsChar(const char * const * names, int number) const
{
     for (int i = 0; i < number; i++) {
          free(const_cast<char *>(names[i]));
     }
     delete [] const_cast<char **>(names);
}
#endif
#endif
// Pass in Event handler (cloned and deleted at end)
void
ClpModel::passInEventHandler(const ClpEventHandler * eventHandler)
{
     delete eventHandler_;
     eventHandler_ = eventHandler->clone();
}
// Sets or unsets scaling, 0 -off, 1 on, 2 dynamic(later)
void
ClpModel::scaling(int mode)
{
     // If mode changes then we treat as new matrix (need new row copy)
     if (mode != scalingFlag_) {
          whatsChanged_ &= ~(2 + 4 + 8);
	  // Get rid of scaled matrix
	  setClpScaledMatrix(NULL);
     }
     if (mode > 0 && mode < 6) {
          scalingFlag_ = mode;
     } else if (!mode) {
          scalingFlag_ = 0;
          setRowScale(NULL);
          setColumnScale(NULL);
     }
}
void
ClpModel::times(double scalar,
                const double * x, double * y) const
{
     if (!scaledMatrix_ || !rowScale_) {
          if (rowScale_)
               matrix_->times(scalar, x, y, rowScale_, columnScale_);
          else
               matrix_->times(scalar, x, y);
     } else {
          scaledMatrix_->times(scalar, x, y);
     }
}
void
ClpModel::transposeTimes(double scalar,
                         const double * x, double * y) const
{
     if (!scaledMatrix_ || !rowScale_) {
          if (rowScale_)
               matrix_->transposeTimes(scalar, x, y, rowScale_, columnScale_, NULL);
          else
               matrix_->transposeTimes(scalar, x, y);
     } else {
          scaledMatrix_->transposeTimes(scalar, x, y);
     }
}
// Does much of scaling
void
ClpModel::gutsOfScaling()
{
     int i;
     if (rowObjective_) {
          for (i = 0; i < numberRows_; i++)
               rowObjective_[i] /= rowScale_[i];
     }
     for (i = 0; i < numberRows_; i++) {
          double multiplier = rowScale_[i];
          double inverseMultiplier = 1.0 / multiplier;
          rowActivity_[i] *= multiplier;
          dual_[i] *= inverseMultiplier;
          if (rowLower_[i] > -1.0e30)
               rowLower_[i] *= multiplier;
          else
               rowLower_[i] = -COIN_DBL_MAX;
          if (rowUpper_[i] < 1.0e30)
               rowUpper_[i] *= multiplier;
          else
               rowUpper_[i] = COIN_DBL_MAX;
     }
     for (i = 0; i < numberColumns_; i++) {
          double multiplier = 1.0 * inverseColumnScale_[i];
          columnActivity_[i] *= multiplier;
          reducedCost_[i] *= columnScale_[i];
          if (columnLower_[i] > -1.0e30)
               columnLower_[i] *= multiplier;
          else
               columnLower_[i] = -COIN_DBL_MAX;
          if (columnUpper_[i] < 1.0e30)
               columnUpper_[i] *= multiplier;
          else
               columnUpper_[i] = COIN_DBL_MAX;

     }
     //now replace matrix
     //and objective
     matrix_->reallyScale(rowScale_, columnScale_);
     objective_->reallyScale(columnScale_);
}
/* If we constructed a "really" scaled model then this reverses the operation.
      Quantities may not be exactly as they were before due to rounding errors */
void
ClpModel::unscale()
{
     if (rowScale_) {
          int i;
          // reverse scaling
          for (i = 0; i < numberRows_; i++)
               rowScale_[i] = 1.0 * inverseRowScale_[i];
          for (i = 0; i < numberColumns_; i++)
               columnScale_[i] = 1.0 * inverseColumnScale_[i];
          gutsOfScaling();
     }

     scalingFlag_ = 0;
     setRowScale(NULL);
     setColumnScale(NULL);
}
void
ClpModel::setSpecialOptions(unsigned int value)
{
     specialOptions_ = value;
}
/* This creates a coinModel object
 */
CoinModel *
ClpModel::createCoinModel() const
{
     CoinModel * coinModel = new CoinModel();
     CoinPackedMatrix matrixByRow;
     matrixByRow.setExtraGap(0.0);
     matrixByRow.setExtraMajor(0.0);
     matrixByRow.reverseOrderedCopyOf(*matrix());
     coinModel->setObjectiveOffset(objectiveOffset());
     coinModel->setProblemName(problemName().c_str());

     // Build by row from scratch
     const double * element = matrixByRow.getElements();
     const int * column = matrixByRow.getIndices();
     const CoinBigIndex * rowStart = matrixByRow.getVectorStarts();
     const int * rowLength = matrixByRow.getVectorLengths();
     int i;
     for (i = 0; i < numberRows_; i++) {
          coinModel->addRow(rowLength[i], column + rowStart[i],
                            element + rowStart[i], rowLower_[i], rowUpper_[i]);
     }
     // Now do column part
     const double * objective = this->objective();
     for (i = 0; i < numberColumns_; i++) {
          coinModel->setColumnBounds(i, columnLower_[i], columnUpper_[i]);
          coinModel->setColumnObjective(i, objective[i]);
     }
     for ( i = 0; i < numberColumns_; i++) {
          if (isInteger(i))
               coinModel->setColumnIsInteger(i, true);
     }
     // do names - clear out
     coinModel->zapRowNames();
     coinModel->zapColumnNames();
     for (i = 0; i < numberRows_; i++) {
          char temp[30];
          strcpy(temp, rowName(i).c_str());
          size_t length = strlen(temp);
          for (size_t j = 0; j < length; j++) {
               if (temp[j] == '-')
                    temp[j] = '_';
          }
          coinModel->setRowName(i, temp);
     }
     for (i = 0; i < numberColumns_; i++) {
          char temp[30];
          strcpy(temp, columnName(i).c_str());
          size_t length = strlen(temp);
          for (size_t j = 0; j < length; j++) {
               if (temp[j] == '-')
                    temp[j] = '_';
          }
          coinModel->setColumnName(i, temp);
     }
     ClpQuadraticObjective * obj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
     if (obj) {
          const CoinPackedMatrix * quadObj = obj->quadraticObjective();
          // add in quadratic
          const double * element = quadObj->getElements();
          const int * row = quadObj->getIndices();
          const CoinBigIndex * columnStart = quadObj->getVectorStarts();
          const int * columnLength = quadObj->getVectorLengths();
          for (i = 0; i < numberColumns_; i++) {
               int nels = columnLength[i];
               if (nels) {
                    CoinBigIndex start = columnStart[i];
                    double constant = coinModel->getColumnObjective(i);
                    char temp[100000];
                    char temp2[30];
                    sprintf(temp, "%g", constant);
                    for (CoinBigIndex k = start; k < start + nels; k++) {
                         int kColumn = row[k];
                         double value = element[k];
#if 1
                         // ampl gives twice with assumed 0.5
                         if (kColumn < i)
                              continue;
                         else if (kColumn == i)
                              value *= 0.5;
#endif
                         if (value == 1.0)
                              sprintf(temp2, "+%s", coinModel->getColumnName(kColumn));
                         else if (value == -1.0)
                              sprintf(temp2, "-%s", coinModel->getColumnName(kColumn));
                         else if (value > 0.0)
                              sprintf(temp2, "+%g*%s", value, coinModel->getColumnName(kColumn));
                         else
                              sprintf(temp2, "%g*%s", value, coinModel->getColumnName(kColumn));
                         strcat(temp, temp2);
                         assert (strlen(temp) < 100000);
                    }
                    coinModel->setObjective(i, temp);
                    if (logLevel() > 2)
                         printf("el for objective column %s is %s\n", coinModel->getColumnName(i), temp);
               }
          }
     }
     return coinModel;
}
// Start or reset using maximumRows_ and Columns_
void
ClpModel::startPermanentArrays()
{
     COIN_DETAIL_PRINT(printf("startperm a %d rows, %d maximum rows\n",
			      numberRows_, maximumRows_));
     if ((specialOptions_ & 65536) != 0) {
          if (numberRows_ > maximumRows_ || numberColumns_ > maximumColumns_) {
               if (numberRows_ > maximumRows_) {
                    if (maximumRows_ > 0)
                         maximumRows_ = numberRows_ + 10 + numberRows_ / 100;
                    else
                         maximumRows_ = numberRows_;
               }
               if (numberColumns_ > maximumColumns_) {
                    if (maximumColumns_ > 0)
                         maximumColumns_ = numberColumns_ + 10 + numberColumns_ / 100;
                    else
                         maximumColumns_ = numberColumns_;
               }
               // need to make sure numberRows_ OK and size of matrices
               resize(maximumRows_, maximumColumns_);
               COIN_DETAIL_PRINT(printf("startperm b %d rows, %d maximum rows\n",
					numberRows_, maximumRows_));
          } else {
               return;
          }
     } else {
          specialOptions_ |= 65536;
          maximumRows_ = numberRows_;
          maximumColumns_ = numberColumns_;
          baseMatrix_ = *matrix();
          baseMatrix_.cleanMatrix();
          baseRowCopy_.setExtraGap(0.0);
          baseRowCopy_.setExtraMajor(0.0);
          baseRowCopy_.reverseOrderedCopyOf(baseMatrix_);
          COIN_DETAIL_PRINT(printf("startperm c %d rows, %d maximum rows\n",
				   numberRows_, maximumRows_));
     }
}
// Stop using maximumRows_ and Columns_
void
ClpModel::stopPermanentArrays()
{
     specialOptions_ &= ~65536;
     maximumRows_ = -1;
     maximumColumns_ = -1;
     if (rowScale_ != savedRowScale_) {
          delete [] savedRowScale_;
          delete [] savedColumnScale_;
     }
     savedRowScale_ = NULL;
     savedColumnScale_ = NULL;
}
// Set new row matrix
void
ClpModel::setNewRowCopy(ClpMatrixBase * newCopy)
{
     delete rowCopy_;
     rowCopy_ = newCopy;
}
/* Find a network subset.
   rotate array should be numberRows.  On output
   -1 not in network
   0 in network as is
   1 in network with signs swapped
  Returns number of network rows (positive if exact network, negative if needs extra row)
  From Gulpinar, Gutin, Maros and Mitra
*/
int
ClpModel::findNetwork(char * rotate, double fractionNeeded)
{
     int * mapping = new int [numberRows_];
     // Get column copy
     CoinPackedMatrix * columnCopy = matrix();
     // Get a row copy in standard format
     CoinPackedMatrix * copy = new CoinPackedMatrix();
     copy->setExtraGap(0.0);
     copy->setExtraMajor(0.0);
     copy->reverseOrderedCopyOf(*columnCopy);
     // make sure ordered and no gaps
     copy->cleanMatrix();
     // get matrix data pointers
     const int * columnIn = copy->getIndices();
     const CoinBigIndex * rowStartIn = copy->getVectorStarts();
     const int * rowLength = copy->getVectorLengths();
     const double * elementByRowIn = copy->getElements();
     int iRow, iColumn;
     int numberEligible = 0;
     int numberIn = 0;
     int numberElements = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          bool possible = true;
          mapping[iRow] = -1;
          rotate[iRow] = -1;
          for (CoinBigIndex j = rowStartIn[iRow]; j < rowStartIn[iRow] + rowLength[iRow]; j++) {
               //int iColumn = column[j];
               double value = elementByRowIn[j];
               if (fabs(value) != 1.0) {
                    possible = false;
                    break;
               }
          }
          if (rowLength[iRow] && possible) {
               mapping[iRow] = numberEligible;
               numberEligible++;
               numberElements += rowLength[iRow];
          }
     }
     if (numberEligible < fractionNeeded * numberRows_) {
          delete [] mapping;
          delete copy;
          return 0;
     }
     // create arrays
     int * eligible = new int [numberRows_];
     int * column = new int [numberElements];
     CoinBigIndex * rowStart = new CoinBigIndex [numberEligible+1];
     char * elementByRow = new char [numberElements];
     numberEligible = 0;
     numberElements = 0;
     rowStart[0] = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          if (mapping[iRow] < 0)
               continue;
          assert (numberEligible == mapping[iRow]);
          rotate[numberEligible] = 0;
          for (CoinBigIndex j = rowStartIn[iRow]; j < rowStartIn[iRow] + rowLength[iRow]; j++) {
               column[numberElements] = columnIn[j];
               double value = elementByRowIn[j];
               if (value == 1.0)
                    elementByRow[numberElements++] = 1;
               else
                    elementByRow[numberElements++] = -1;
          }
          numberEligible++;
          rowStart[numberEligible] = numberElements;
     }
     // get rid of copy to save space
     delete copy;
     const int * rowIn = columnCopy->getIndices();
     const CoinBigIndex * columnStartIn = columnCopy->getVectorStarts();
     const int * columnLengthIn = columnCopy->getVectorLengths();
     const double * elementByColumnIn = columnCopy->getElements();
     int * columnLength = new int [numberColumns_];
     // May just be that is a network - worth checking
     bool isNetworkAlready = true;
     bool trueNetwork = true;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          double product = 1.0;
          int n = 0;
          for (CoinBigIndex j = columnStartIn[iColumn]; j < columnStartIn[iColumn] + columnLengthIn[iColumn]; j++) {
               iRow = mapping[rowIn[j]];
               if (iRow >= 0) {
                    n++;
                    product *= elementByColumnIn[j];
               }
          }
          if (n >= 2) {
               if (product != -1.0 || n > 2)
                    isNetworkAlready = false;
          } else if (n == 1) {
               trueNetwork = false;
          }
          columnLength[iColumn] = n;
     }
     if (!isNetworkAlready) {
          // For sorting
          double * count = new double [numberRows_];
          int * which = new int [numberRows_];
          int numberLast = -1;
          // Count for columns
          char * columnCount = new char[numberColumns_];
          memset(columnCount, 0, numberColumns_);
          char * currentColumnCount = new char[numberColumns_];
          // Now do main loop
          while (numberIn > numberLast) {
               numberLast = numberIn;
               int numberLeft = 0;
               for (iRow = 0; iRow < numberEligible; iRow++) {
                    if (rotate[iRow] == 0 && rowStart[iRow+1] > rowStart[iRow]) {
                         which[numberLeft] = iRow;
                         int merit = 0;
                         bool OK = true;
                         bool reflectionOK = true;
                         for (CoinBigIndex j = rowStart[iRow]; j < rowStart[iRow+1]; j++) {
                              iColumn = column[j];
                              int iCount = columnCount[iColumn];
                              int absCount = CoinAbs(iCount);
                              if (absCount < 2) {
                                   merit = CoinMax(columnLength[iColumn] - absCount - 1, merit);
                                   if (elementByRow[j] == iCount)
                                        OK = false;
                                   else if (elementByRow[j] == -iCount)
                                        reflectionOK = false;
                              } else {
                                   merit = -2;
                                   break;
                              }
                         }
                         if (merit > -2 && (OK || reflectionOK) &&
                                   (!OK || !reflectionOK || !numberIn)) {
                              //if (!numberLast) merit=1;
                              count[numberLeft++] = (rowStart[iRow+1] - rowStart[iRow] - 1) *
                                                    (static_cast<double>(merit));
                              if (OK)
                                   rotate[iRow] = 0;
                              else
                                   rotate[iRow] = 1;
                         } else {
                              // no good
                              rotate[iRow] = -1;
                         }
                    }
               }
               CoinSort_2(count, count + numberLeft, which);
               // Get G
               memset(currentColumnCount, 0, numberColumns_);
               for (iRow = 0; iRow < numberLeft; iRow++) {
                    int jRow = which[iRow];
                    bool possible = true;
                    for (int i = 0; i < numberIn; i++) {
                         for (CoinBigIndex j = rowStart[jRow]; j < rowStart[jRow+1]; j++) {
                              if (currentColumnCount[column[j]]) {
                                   possible = false;
                                   break;
                              }
                         }
                    }
                    if (possible) {
                         rotate[jRow] = static_cast<char>(rotate[jRow] + 2);
                         eligible[numberIn++] = jRow;
                         char multiplier = static_cast<char>((rotate[jRow] == 2) ? 1 : -1);
                         for (CoinBigIndex j = rowStart[jRow]; j < rowStart[jRow+1]; j++) {
                              iColumn = column[j];
                              currentColumnCount[iColumn]++;
                              int iCount = columnCount[iColumn];
                              int absCount = CoinAbs(iCount);
                              if (!absCount) {
                                   columnCount[iColumn] = static_cast<char>(elementByRow[j] * multiplier);
                              } else {
                                   columnCount[iColumn] = 2;
                              }
                         }
                    }
               }
          }
#ifndef NDEBUG
          for (iRow = 0; iRow < numberIn; iRow++) {
               int kRow = eligible[iRow];
               assert (rotate[kRow] >= 2);
          }
#endif
          trueNetwork = true;
          for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
               if (CoinAbs(static_cast<int>(columnCount[iColumn])) == 1) {
                    trueNetwork = false;
                    break;
               }
          }
          delete [] currentColumnCount;
          delete [] columnCount;
          delete [] which;
          delete [] count;
     } else {
          numberIn = numberEligible;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int kRow = mapping[iRow];
               if (kRow >= 0) {
                    rotate[kRow] = 2;
               }
          }
     }
     if (!trueNetwork)
          numberIn = - numberIn;
     delete [] column;
     delete [] rowStart;
     delete [] elementByRow;
     delete [] columnLength;
     // redo rotate
     char * rotate2 = CoinCopyOfArray(rotate, numberEligible);
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int kRow = mapping[iRow];
          if (kRow >= 0) {
               int iState = rotate2[kRow];
               if (iState > 1)
                    iState -= 2;
               else
                    iState = -1;
               rotate[iRow] = static_cast<char>(iState);
          } else {
               rotate[iRow] = -1;
          }
     }
     delete [] rotate2;
     delete [] eligible;
     delete [] mapping;
     return numberIn;
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpDataSave::ClpDataSave ()
{
     dualBound_ = 0.0;
     infeasibilityCost_ = 0.0;
     sparseThreshold_ = 0;
     pivotTolerance_ = 0.0;
     zeroFactorizationTolerance_ = 1.0e13;
     zeroSimplexTolerance_ = 1.0e-13;
     acceptablePivot_ = 0.0;
     objectiveScale_ = 1.0;
     perturbation_ = 0;
     forceFactorization_ = -1;
     scalingFlag_ = 0;
     specialOptions_ = 0;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpDataSave::ClpDataSave (const ClpDataSave & rhs)
{
     dualBound_ = rhs.dualBound_;
     infeasibilityCost_ = rhs.infeasibilityCost_;
     pivotTolerance_ = rhs.pivotTolerance_;
     zeroFactorizationTolerance_ = rhs.zeroFactorizationTolerance_;
     zeroSimplexTolerance_ = rhs.zeroSimplexTolerance_;
     acceptablePivot_ = rhs.acceptablePivot_;
     objectiveScale_ = rhs.objectiveScale_;
     sparseThreshold_ = rhs.sparseThreshold_;
     perturbation_ = rhs.perturbation_;
     forceFactorization_ = rhs.forceFactorization_;
     scalingFlag_ = rhs.scalingFlag_;
     specialOptions_ = rhs.specialOptions_;
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpDataSave::~ClpDataSave ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpDataSave &
ClpDataSave::operator=(const ClpDataSave& rhs)
{
     if (this != &rhs) {
          dualBound_ = rhs.dualBound_;
          infeasibilityCost_ = rhs.infeasibilityCost_;
          pivotTolerance_ = rhs.pivotTolerance_;
          zeroFactorizationTolerance_ = rhs.zeroFactorizationTolerance_;
          zeroSimplexTolerance_ = rhs.zeroSimplexTolerance_;
          acceptablePivot_ = rhs.acceptablePivot_;
          objectiveScale_ = rhs.objectiveScale_;
          sparseThreshold_ = rhs.sparseThreshold_;
          perturbation_ = rhs.perturbation_;
          forceFactorization_ = rhs.forceFactorization_;
          scalingFlag_ = rhs.scalingFlag_;
          specialOptions_ = rhs.specialOptions_;
     }
     return *this;
}
// Create C++ lines to get to current state
void
ClpModel::generateCpp( FILE * fp)
{
     // Stuff that can't be done easily
     if (!lengthNames_) {
          // no names
          fprintf(fp, "  clpModel->dropNames();\n");
     }
     ClpModel defaultModel;
     ClpModel * other = &defaultModel;
     int iValue1, iValue2;
     double dValue1, dValue2;
     iValue1 = this->maximumIterations();
     iValue2 = other->maximumIterations();
     fprintf(fp, "%d  int save_maximumIterations = clpModel->maximumIterations();\n", iValue1 == iValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setMaximumIterations(%d);\n", iValue1 == iValue2 ? 4 : 3, iValue1);
     fprintf(fp, "%d  clpModel->setMaximumIterations(save_maximumIterations);\n", iValue1 == iValue2 ? 7 : 6);
     dValue1 = this->primalTolerance();
     dValue2 = other->primalTolerance();
     fprintf(fp, "%d  double save_primalTolerance = clpModel->primalTolerance();\n", dValue1 == dValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setPrimalTolerance(%g);\n", dValue1 == dValue2 ? 4 : 3, dValue1);
     fprintf(fp, "%d  clpModel->setPrimalTolerance(save_primalTolerance);\n", dValue1 == dValue2 ? 7 : 6);
     dValue1 = this->dualTolerance();
     dValue2 = other->dualTolerance();
     fprintf(fp, "%d  double save_dualTolerance = clpModel->dualTolerance();\n", dValue1 == dValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setDualTolerance(%g);\n", dValue1 == dValue2 ? 4 : 3, dValue1);
     fprintf(fp, "%d  clpModel->setDualTolerance(save_dualTolerance);\n", dValue1 == dValue2 ? 7 : 6);
     iValue1 = this->numberIterations();
     iValue2 = other->numberIterations();
     fprintf(fp, "%d  int save_numberIterations = clpModel->numberIterations();\n", iValue1 == iValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setNumberIterations(%d);\n", iValue1 == iValue2 ? 4 : 3, iValue1);
     fprintf(fp, "%d  clpModel->setNumberIterations(save_numberIterations);\n", iValue1 == iValue2 ? 7 : 6);
     dValue1 = this->maximumSeconds();
     dValue2 = other->maximumSeconds();
     fprintf(fp, "%d  double save_maximumSeconds = clpModel->maximumSeconds();\n", dValue1 == dValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setMaximumSeconds(%g);\n", dValue1 == dValue2 ? 4 : 3, dValue1);
     fprintf(fp, "%d  clpModel->setMaximumSeconds(save_maximumSeconds);\n", dValue1 == dValue2 ? 7 : 6);
     dValue1 = this->optimizationDirection();
     dValue2 = other->optimizationDirection();
     fprintf(fp, "%d  double save_optimizationDirection = clpModel->optimizationDirection();\n", dValue1 == dValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setOptimizationDirection(%g);\n", dValue1 == dValue2 ? 4 : 3, dValue1);
     fprintf(fp, "%d  clpModel->setOptimizationDirection(save_optimizationDirection);\n", dValue1 == dValue2 ? 7 : 6);
     dValue1 = this->objectiveScale();
     dValue2 = other->objectiveScale();
     fprintf(fp, "%d  double save_objectiveScale = clpModel->objectiveScale();\n", dValue1 == dValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setObjectiveScale(%g);\n", dValue1 == dValue2 ? 4 : 3, dValue1);
     fprintf(fp, "%d  clpModel->setObjectiveScale(save_objectiveScale);\n", dValue1 == dValue2 ? 7 : 6);
     dValue1 = this->rhsScale();
     dValue2 = other->rhsScale();
     fprintf(fp, "%d  double save_rhsScale = clpModel->rhsScale();\n", dValue1 == dValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setRhsScale(%g);\n", dValue1 == dValue2 ? 4 : 3, dValue1);
     fprintf(fp, "%d  clpModel->setRhsScale(save_rhsScale);\n", dValue1 == dValue2 ? 7 : 6);
     iValue1 = this->scalingFlag();
     iValue2 = other->scalingFlag();
     fprintf(fp, "%d  int save_scalingFlag = clpModel->scalingFlag();\n", iValue1 == iValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->scaling(%d);\n", iValue1 == iValue2 ? 4 : 3, iValue1);
     fprintf(fp, "%d  clpModel->scaling(save_scalingFlag);\n", iValue1 == iValue2 ? 7 : 6);
     dValue1 = this->getSmallElementValue();
     dValue2 = other->getSmallElementValue();
     fprintf(fp, "%d  double save_getSmallElementValue = clpModel->getSmallElementValue();\n", dValue1 == dValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setSmallElementValue(%g);\n", dValue1 == dValue2 ? 4 : 3, dValue1);
     fprintf(fp, "%d  clpModel->setSmallElementValue(save_getSmallElementValue);\n", dValue1 == dValue2 ? 7 : 6);
     iValue1 = this->logLevel();
     iValue2 = other->logLevel();
     fprintf(fp, "%d  int save_logLevel = clpModel->logLevel();\n", iValue1 == iValue2 ? 2 : 1);
     fprintf(fp, "%d  clpModel->setLogLevel(%d);\n", iValue1 == iValue2 ? 4 : 3, iValue1);
     fprintf(fp, "%d  clpModel->setLogLevel(save_logLevel);\n", iValue1 == iValue2 ? 7 : 6);
}
