/* $Id: CoinSnapshot.cpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include "CoinUtilsConfig.h"
#include "CoinHelperFunctions.hpp"
#include "CoinSnapshot.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinFinite.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinSnapshot::CoinSnapshot () 
{
  gutsOfDestructor(13);
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinSnapshot::CoinSnapshot (const CoinSnapshot & rhs) 
{
  gutsOfDestructor(13);
  gutsOfCopy(rhs);
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinSnapshot::~CoinSnapshot ()
{
  gutsOfDestructor(15);
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinSnapshot &
CoinSnapshot::operator=(const CoinSnapshot& rhs)
{
  if (this != &rhs) {
    gutsOfDestructor(15);
    gutsOfCopy(rhs);
  }
  return *this;
}
// Does main work of destructor
void 
CoinSnapshot::gutsOfDestructor(int type)
{
  if ((type&2)!=0) {
    if (owned_.colLower)
      delete [] colLower_;
    if (owned_.colUpper)
      delete [] colUpper_;
    if (owned_.rowLower)
      delete [] rowLower_;
    if (owned_.rowUpper)
      delete [] rowUpper_;
    if (owned_.rightHandSide)
      delete [] rightHandSide_;
    if (owned_.objCoefficients)
      delete [] objCoefficients_;
    if (owned_.colType)
      delete [] colType_;
    if (owned_.matrixByRow)
      delete matrixByRow_;
    if (owned_.matrixByCol)
      delete matrixByCol_;
    if (owned_.originalMatrixByRow)
      delete originalMatrixByRow_;
    if (owned_.originalMatrixByCol)
      delete originalMatrixByCol_;
    if (owned_.colSolution)
      delete [] colSolution_;
    if (owned_.rowPrice)
      delete [] rowPrice_;
    if (owned_.reducedCost)
      delete [] reducedCost_;
    if (owned_.rowActivity)
      delete [] rowActivity_;
    if (owned_.doNotSeparateThis)
      delete [] doNotSeparateThis_;
  }
  if ((type&4)!=0) {
    objSense_ = 1.0;
    infinity_ = COIN_DBL_MAX;
    dualTolerance_ = 1.0e-7;
    primalTolerance_ = 1.0e-7;
    integerTolerance_ = 1.0e-7;
  }
  if ((type&8)!=0) {
    objValue_ = COIN_DBL_MAX;
    objOffset_ = 0.0;
    integerUpperBound_ = COIN_DBL_MAX;
    integerLowerBound_ = -COIN_DBL_MAX;
  }
  if ((type&1)!=0) {
    colLower_ = NULL;
    colUpper_ = NULL;
    rowLower_ = NULL;
    rowUpper_ = NULL;
    rightHandSide_ = NULL;
    objCoefficients_ = NULL;
    colType_ = NULL;
    matrixByRow_ = NULL;
    matrixByCol_ = NULL;
    originalMatrixByRow_ = NULL;
    originalMatrixByCol_ = NULL;
    colSolution_ = NULL;
    rowPrice_ = NULL;
    reducedCost_ = NULL;
    rowActivity_ = NULL;
    doNotSeparateThis_ = NULL;
    numCols_ = 0;
    numRows_ = 0;
    numElements_ = 0;
    numIntegers_ = 0;
    // say nothing owned
    memset(&owned_,0,sizeof(owned_));
  }
}
// Does main work of copy
void 
CoinSnapshot::gutsOfCopy(const CoinSnapshot & rhs)
{
  objSense_ = rhs.objSense_;
  infinity_ = rhs.infinity_;
  objValue_ = rhs.objValue_;
  objOffset_ = rhs.objOffset_;
  dualTolerance_ = rhs.dualTolerance_;
  primalTolerance_ = rhs.primalTolerance_;
  integerTolerance_ = rhs.integerTolerance_;
  integerUpperBound_ = rhs.integerUpperBound_;
  integerLowerBound_ = rhs.integerLowerBound_;
  numCols_ = rhs.numCols_;
  numRows_ = rhs.numRows_;
  numElements_ = rhs.numElements_;
  numIntegers_ = rhs.numIntegers_;
  owned_ = rhs.owned_;
  if (owned_.colLower)
    colLower_ = CoinCopyOfArray(rhs.colLower_,numCols_);
  else
    colLower_ = rhs.colLower_;
  if (owned_.colUpper)
    colUpper_ = CoinCopyOfArray(rhs.colUpper_,numCols_);
  else
    colUpper_ = rhs.colUpper_;
  if (owned_.rowLower)
    rowLower_ = CoinCopyOfArray(rhs.rowLower_,numRows_);
  else
    rowLower_ = rhs.rowLower_;
  if (owned_.rowUpper)
    rowUpper_ = CoinCopyOfArray(rhs.rowUpper_,numRows_);
  else
    rowUpper_ = rhs.rowUpper_;
  if (owned_.rightHandSide)
    rightHandSide_ = CoinCopyOfArray(rhs.rightHandSide_,numRows_);
  else
    rightHandSide_ = rhs.rightHandSide_;
  if (owned_.objCoefficients)
    objCoefficients_ = CoinCopyOfArray(rhs.objCoefficients_,numCols_);
  else
    objCoefficients_ = rhs.objCoefficients_;
  if (owned_.colType)
    colType_ = CoinCopyOfArray(rhs.colType_,numCols_);
  else
    colType_ = rhs.colType_;
  if (owned_.colSolution)
    colSolution_ = CoinCopyOfArray(rhs.colSolution_,numCols_);
  else
    colSolution_ = rhs.colSolution_;
  if (owned_.rowPrice)
    rowPrice_ = CoinCopyOfArray(rhs.rowPrice_,numRows_);
  else
    rowPrice_ = rhs.rowPrice_;
  if (owned_.reducedCost)
    reducedCost_ = CoinCopyOfArray(rhs.reducedCost_,numCols_);
  else
    reducedCost_ = rhs.reducedCost_;
  if (owned_.rowActivity)
    rowActivity_ = CoinCopyOfArray(rhs.rowActivity_,numRows_);
  else
    rowActivity_ = rhs.rowActivity_;
  if (owned_.doNotSeparateThis)
    doNotSeparateThis_ = CoinCopyOfArray(rhs.doNotSeparateThis_,numCols_);
  else
    doNotSeparateThis_ = rhs.doNotSeparateThis_;
  if (owned_.matrixByRow)
    matrixByRow_ = new CoinPackedMatrix(*rhs.matrixByRow_);
  else
    matrixByRow_ = rhs.matrixByRow_;
  if (owned_.matrixByCol)
    matrixByCol_ = new CoinPackedMatrix(*rhs.matrixByCol_);
  else
    matrixByCol_ = rhs.matrixByCol_;
  if (owned_.originalMatrixByRow)
    originalMatrixByRow_ = new CoinPackedMatrix(*rhs.originalMatrixByRow_);
  else
    originalMatrixByRow_ = rhs.originalMatrixByRow_;
  if (owned_.originalMatrixByCol)
    originalMatrixByCol_ = new CoinPackedMatrix(*rhs.originalMatrixByCol_);
  else
    originalMatrixByCol_ = rhs.originalMatrixByCol_;
}
/* Load in an problem by copying the arguments (the constraints on the
   rows are given by lower and upper bounds). If a pointer is NULL then the
   following values are the default:
   <ul>
   <li> <code>colub</code>: all columns have upper bound infinity
   <li> <code>collb</code>: all columns have lower bound 0 
   <li> <code>rowub</code>: all rows have upper bound infinity
   <li> <code>rowlb</code>: all rows have lower bound -infinity
   <li> <code>obj</code>: all variables have 0 objective coefficient
      </ul>
*/
void 
CoinSnapshot::loadProblem(const CoinPackedMatrix& matrix,
			  const double* collb, const double* colub,   
			  const double* obj,
			  const double* rowlb, const double* rowub,
			  bool makeRowCopy)
{
  // Keep scalars (apart from objective value etc)
  gutsOfDestructor(3+8);
  numRows_ = matrix.getNumRows();
  numCols_ = matrix.getNumCols();
  numElements_ = matrix.getNumElements();
  owned_.matrixByCol = 1;
  matrixByCol_ = new CoinPackedMatrix(matrix);
  if (makeRowCopy) {
    owned_.matrixByRow = 1;
    CoinPackedMatrix * matrixByRow = new CoinPackedMatrix(matrix);
    matrixByRow->reverseOrdering();
    matrixByRow_ = matrixByRow;
  }
  colLower_ = CoinCopyOfArray(collb,numCols_,0.0);
  colUpper_ = CoinCopyOfArray(colub,numCols_,infinity_);
  objCoefficients_ = CoinCopyOfArray(obj,numCols_,0.0);
  rowLower_ = CoinCopyOfArray(rowlb,numRows_,-infinity_);
  rowUpper_ = CoinCopyOfArray(rowub,numRows_,infinity_);
  // do rhs as well
  createRightHandSide();
}
  
// Set pointer to array[getNumCols()] of column lower bounds
void 
CoinSnapshot::setColLower(const double * array, bool copyIn)
{
  if (owned_.colLower)
    delete [] colLower_;
  if (copyIn) {
    owned_.colLower=1;
    colLower_ = CoinCopyOfArray(array,numCols_);
  } else {
    owned_.colLower=0;
    colLower_ = array;
  }
}
// Set pointer to array[getNumCols()] of column upper bounds
void 
CoinSnapshot::setColUpper(const double * array, bool copyIn)
{
  if (owned_.colUpper)
    delete [] colUpper_;
  if (copyIn) {
    owned_.colUpper=1;
    colUpper_ = CoinCopyOfArray(array,numCols_);
  } else {
    owned_.colUpper=0;
    colUpper_ = array;
  }
}
// Set pointer to array[getNumRows()] of row lower bounds
void 
CoinSnapshot::setRowLower(const double * array, bool copyIn)
{
  if (owned_.rowLower)
    delete [] rowLower_;
  if (copyIn) {
    owned_.rowLower=1;
    rowLower_ = CoinCopyOfArray(array,numRows_);
  } else {
    owned_.rowLower=0;
    rowLower_ = array;
  }
}
// Set pointer to array[getNumRows()] of row upper bounds
void 
CoinSnapshot::setRowUpper(const double * array, bool copyIn)
{
  if (owned_.rowUpper)
    delete [] rowUpper_;
  if (copyIn) {
    owned_.rowUpper=1;
    rowUpper_ = CoinCopyOfArray(array,numRows_);
  } else {
    owned_.rowUpper=0;
    rowUpper_ = array;
  }
}
/* Set pointer to array[getNumRows()] of rhs side values
   This gives same results as OsiSolverInterface for useful cases
   If getRowUpper()[i] != infinity then
     getRightHandSide()[i] == getRowUpper()[i]
   else
     getRightHandSide()[i] == getRowLower()[i]
*/
void 
CoinSnapshot::setRightHandSide(const double * array, bool copyIn)
{
  if (owned_.rightHandSide)
    delete [] rightHandSide_;
  if (copyIn) {
    owned_.rightHandSide=1;
    rightHandSide_ = CoinCopyOfArray(array,numRows_);
  } else {
    owned_.rightHandSide=0;
    rightHandSide_ = array;
  }
}
/* Create array[getNumRows()] of rhs side values
   This gives same results as OsiSolverInterface for useful cases
   If getRowUpper()[i] != infinity then
     getRightHandSide()[i] == getRowUpper()[i]
   else
     getRightHandSide()[i] == getRowLower()[i]
*/
void 
CoinSnapshot::createRightHandSide()
{
  if (owned_.rightHandSide)
    delete [] rightHandSide_;
  owned_.rightHandSide=1;
  assert (rowUpper_);
  assert (rowLower_);
  double * rightHandSide = CoinCopyOfArray(rowUpper_,numRows_);
  for (int i=0;i<numRows_;i++) {
    if (rightHandSide[i]==infinity_)
      rightHandSide[i] = rowLower_[i];
  }
  rightHandSide_ = rightHandSide;
}
// Set pointer to array[getNumCols()] of objective function coefficients
void 
CoinSnapshot::setObjCoefficients(const double * array, bool copyIn)
{
  if (owned_.objCoefficients)
    delete [] objCoefficients_;
  if (copyIn) {
    owned_.objCoefficients=1;
    objCoefficients_ = CoinCopyOfArray(array,numCols_);
  } else {
    owned_.objCoefficients=0;
    objCoefficients_ = array;
  }
}
// Set colType array ('B', 'I', or 'C' for Binary, Integer and Continuous) 
void 
CoinSnapshot::setColType(const char *array, bool copyIn)
{
  if (owned_.colType)
    delete [] colType_;
  if (copyIn) {
    owned_.colType=1;
    colType_ = CoinCopyOfArray(array,numCols_);
  } else {
    owned_.colType=0;
    colType_ = array;
  }
  int i;
  numIntegers_=0;
  for (i=0;i<numCols_;i++) {
    if (colType_[i]=='B'||colType_[i]=='I')
      numIntegers_++;
  }
}
// Set pointer to row-wise copy of current matrix
void 
CoinSnapshot::setMatrixByRow(const CoinPackedMatrix * matrix, bool copyIn)
{
  if (owned_.matrixByRow)
    delete matrixByRow_;
  if (copyIn) {
    owned_.matrixByRow=1;
    matrixByRow_ = new CoinPackedMatrix(*matrix);
  } else {
    owned_.matrixByRow=0;
    matrixByRow_ = matrix;
  }
  assert (matrixByRow_->getNumCols()==numCols_);
  assert (matrixByRow_->getNumRows()==numRows_);
}
// Create row-wise copy from MatrixByCol
void 
CoinSnapshot::createMatrixByRow()
{
  if (owned_.matrixByRow)
    delete matrixByRow_;
  assert (matrixByCol_);
  owned_.matrixByRow = 1;
  CoinPackedMatrix * matrixByRow = new CoinPackedMatrix(*matrixByCol_);
  matrixByRow->reverseOrdering();
  matrixByRow_ = matrixByRow;
}
// Set pointer to column-wise copy of current matrix
void 
CoinSnapshot::setMatrixByCol(const CoinPackedMatrix * matrix, bool copyIn)
{
  if (owned_.matrixByCol)
    delete matrixByCol_;
  if (copyIn) {
    owned_.matrixByCol=1;
    matrixByCol_ = new CoinPackedMatrix(*matrix);
  } else {
    owned_.matrixByCol=0;
    matrixByCol_ = matrix;
  }
  assert (matrixByCol_->getNumCols()==numCols_);
  assert (matrixByCol_->getNumRows()==numRows_);
}
// Set pointer to row-wise copy of "original" matrix
void 
CoinSnapshot::setOriginalMatrixByRow(const CoinPackedMatrix * matrix, bool copyIn)
{
  if (owned_.originalMatrixByRow)
    delete originalMatrixByRow_;
  if (copyIn) {
    owned_.originalMatrixByRow=1;
    originalMatrixByRow_ = new CoinPackedMatrix(*matrix);
  } else {
    owned_.originalMatrixByRow=0;
    originalMatrixByRow_ = matrix;
  }
  assert (matrixByRow_->getNumCols()==numCols_);
}
// Set pointer to column-wise copy of "original" matrix
void 
CoinSnapshot::setOriginalMatrixByCol(const CoinPackedMatrix * matrix, bool copyIn)
{
  if (owned_.originalMatrixByCol)
    delete originalMatrixByCol_;
  if (copyIn) {
    owned_.originalMatrixByCol=1;
    originalMatrixByCol_ = new CoinPackedMatrix(*matrix);
  } else {
    owned_.originalMatrixByCol=0;
    originalMatrixByCol_ = matrix;
  }
  assert (matrixByCol_->getNumCols()==numCols_);
}
// Set pointer to array[getNumCols()] of primal variable values
void 
CoinSnapshot::setColSolution(const double * array, bool copyIn)
{
  if (owned_.colSolution)
    delete [] colSolution_;
  if (copyIn) {
    owned_.colSolution=1;
    colSolution_ = CoinCopyOfArray(array,numCols_);
  } else {
    owned_.colSolution=0;
    colSolution_ = array;
  }
}
// Set pointer to array[getNumRows()] of dual variable values
void 
CoinSnapshot::setRowPrice(const double * array, bool copyIn)
{
  if (owned_.rowPrice)
    delete [] rowPrice_;
  if (copyIn) {
    owned_.rowPrice=1;
    rowPrice_ = CoinCopyOfArray(array,numRows_);
  } else {
    owned_.rowPrice=0;
    rowPrice_ = array;
  }
}
// Set a pointer to array[getNumCols()] of reduced costs
void 
CoinSnapshot::setReducedCost(const double * array, bool copyIn)
{
  if (owned_.reducedCost)
    delete [] reducedCost_;
  if (copyIn) {
    owned_.reducedCost=1;
    reducedCost_ = CoinCopyOfArray(array,numCols_);
  } else {
    owned_.reducedCost=0;
    reducedCost_ = array;
  }
}
// Set pointer to array[getNumRows()] of row activity levels (constraint matrix times the solution vector). 
void 
CoinSnapshot::setRowActivity(const double * array, bool copyIn)
{
  if (owned_.rowActivity)
    delete [] rowActivity_;
  if (copyIn) {
    owned_.rowActivity=1;
    rowActivity_ = CoinCopyOfArray(array,numRows_);
  } else {
    owned_.rowActivity=0;
    rowActivity_ = array;
  }
}
// Set pointer to array[getNumCols()] of primal variable values which should not be separated (for debug)
void 
CoinSnapshot::setDoNotSeparateThis(const double * array, bool copyIn)
{
  if (owned_.doNotSeparateThis)
    delete [] doNotSeparateThis_;
  if (copyIn) {
    owned_.doNotSeparateThis=1;
    doNotSeparateThis_ = CoinCopyOfArray(array,numCols_);
  } else {
    owned_.doNotSeparateThis=0;
    doNotSeparateThis_ = array;
  }
}
