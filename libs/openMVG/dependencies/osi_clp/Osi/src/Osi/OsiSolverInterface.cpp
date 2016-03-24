// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stddef.h>
#include <iostream>

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinMpsIO.hpp"
#include "CoinMessage.hpp"
#include "CoinWarmStart.hpp"
#ifdef COIN_SNAPSHOT
#include "CoinSnapshot.hpp"
#endif

#include "OsiSolverInterface.hpp"
#ifdef CBC_NEXT_VERSION
#include "OsiSolverBranch.hpp"
#endif
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "OsiRowCutDebugger.hpp"
#include "OsiAuxInfo.hpp"
#include "OsiBranchingObject.hpp"
#include <cassert>
#include "CoinFinite.hpp"
#include "CoinBuild.hpp"
#include "CoinModel.hpp"
#include "CoinLpIO.hpp"
//#############################################################################
// Hotstart related methods (primarily used in strong branching)
// It is assumed that only bounds (on vars/constraints) can change between
// markHotStart() and unmarkHotStart()
//#############################################################################

void OsiSolverInterface::markHotStart()
{
  delete ws_;
  ws_ = getWarmStart();
}

void OsiSolverInterface::solveFromHotStart()
{
  setWarmStart(ws_);
  resolve();
}

void OsiSolverInterface::unmarkHotStart()
{
  delete ws_;
  ws_ = NULL;
}

//#############################################################################
// Get indices of solution vector which are integer variables presently at
// fractional values
//#############################################################################

OsiVectorInt
OsiSolverInterface::getFractionalIndices(const double etol) const
{
   const int colnum = getNumCols();
   OsiVectorInt frac;
   CoinAbsFltEq eq(etol);
   for (int i = 0; i < colnum; ++i) {
      if (isInteger(i)) {
	 const double ci = getColSolution()[i];
	 const double distanceFromInteger = ci - floor(ci + 0.5);
	 if (! eq(distanceFromInteger, 0.0))
	    frac.push_back(i);
      }
   }
   return frac;
}


int OsiSolverInterface::getNumElements() const
{
  return getMatrixByRow()->getNumElements();
}

//#############################################################################
// Methods for determining the type of column variable.
// The method isContinuous() is presently implemented in the derived classes.
// The methods below depend on isContinuous and the values
// stored in the column bounds.
//#############################################################################

bool 
OsiSolverInterface::isBinary(int colIndex) const
{
  if ( isContinuous(colIndex) ) return false; 
  const double * cu = getColUpper();
  const double * cl = getColLower();
  if (
    (cu[colIndex]== 1 || cu[colIndex]== 0) && 
    (cl[colIndex]== 0 || cl[colIndex]==1)
    ) return true;
  else return false;
}


//#############################################################################
// Method for getting a column solution that is guaranteed to be
// between the column lower and upper bounds. 
// 
// This method is was introduced for Cgl. Osi developers can
// improve performance for their favorite solvers by 
// overriding this method and providing an implementation that caches
// strictColSolution_.
//#############################################################################

const double * 
OsiSolverInterface::getStrictColSolution()
{
  const double * colSolution = getColSolution();
  const double * colLower = getColLower();
  const double * colUpper = getColUpper();
  const int numCols = getNumCols();

  strictColSolution_.clear();
  strictColSolution_.insert(strictColSolution_.end(),colSolution, colSolution+numCols);

  for (int i=numCols-1; i> 0; --i){
    if (colSolution[i] <= colUpper[i]){
      if (colSolution[i] >= colLower[i]){
	continue;
      } else {
	strictColSolution_[i] = colLower[i];
      }
    } else {
      strictColSolution_[i] = colUpper[i];
    }
  }
  return &strictColSolution_[0];
}


//-----------------------------------------------------------------------------
bool 
OsiSolverInterface::isInteger(int colIndex) const
{
   return !isContinuous(colIndex);
}
//-----------------------------------------------------------------------------
/*
  Return number of integer variables. OSI implementors really should replace
  this one, it's generally trivial from within the OSI, but this is the only
  safe way to do it generically.
*/
int
OsiSolverInterface::getNumIntegers () const
{
  if (numberIntegers_>=0) {
    // we already know
    return numberIntegers_;
  } else {
    // work out
    const int numCols = getNumCols() ;
    int numIntegers = 0 ;
    for (int i = 0 ; i < numCols ; ++i) {
      if (!isContinuous(i)) {
	numIntegers++ ;
      }
    }
    return (numIntegers) ;
  }
}
//-----------------------------------------------------------------------------
bool 
OsiSolverInterface::isIntegerNonBinary(int colIndex) const
{
  if ( isInteger(colIndex) && !isBinary(colIndex) )
    return true; 
  else return false;
}
//-----------------------------------------------------------------------------
bool 
OsiSolverInterface::isFreeBinary(int colIndex) const
{
  if ( isContinuous(colIndex) ) return false;
  const double * cu = getColUpper();
  const double * cl = getColLower();
  if (
    (cu[colIndex]== 1) &&
    (cl[colIndex]== 0)
    ) return true;
  else return false;
}
/*  Return array of column length
    0 - continuous
    1 - binary (may get fixed later)
    2 - general integer (may get fixed later)
*/
const char * 
OsiSolverInterface::getColType(bool refresh) const
{
  if (!columnType_||refresh) {
    const int numCols = getNumCols() ;
    if (!columnType_)
      columnType_ = new char [numCols];
    const double * cu = getColUpper();
    const double * cl = getColLower();
    for (int i = 0 ; i < numCols ; ++i) {
      if (!isContinuous(i)) {
	if ((cu[i]== 1 || cu[i]== 0) && 
	    (cl[i]== 0 || cl[i]==1))
	  columnType_[i]=1;
	else
	  columnType_[i]=2;
      } else {
	columnType_[i]=0;
      }
    }
  }
  return columnType_;
}

/*
###############################################################################

  Built-in methods for problem modification

  Some of these can surely be reimplemented more efficiently given knowledge of
  the derived OsiXXX data structures, but we can't make assumptions here. For
  others, it's less clear. Given standard vector data structures in the
  underlying OsiXXX, likely the only possible gain is avoiding a method call.

###############################################################################
*/

void
OsiSolverInterface::setObjCoeffSet(const int* indexFirst,
				  const int* indexLast,
				  const double* coeffList)
{
   const std::ptrdiff_t cnt = indexLast - indexFirst;
   for (std::ptrdiff_t i = 0; i < cnt; ++i) {
      setObjCoeff(indexFirst[i], coeffList[i]);
   }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setColSetBounds(const int* indexFirst,
				    const int* indexLast,
				    const double* boundList)
{
  while (indexFirst != indexLast) {
    setColBounds(*indexFirst, boundList[0], boundList[1]);
    ++indexFirst;
    boundList += 2;
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setRowSetBounds(const int* indexFirst,
				    const int* indexLast,
				    const double* boundList)
{
  while (indexFirst != indexLast) {
    setRowBounds(*indexFirst, boundList[0], boundList[1]);
    ++indexFirst;
    boundList += 2;
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setRowSetTypes(const int* indexFirst,
				   const int* indexLast,
				   const char* senseList,
				   const double* rhsList,
				   const double* rangeList)
{
  while (indexFirst != indexLast) {
    setRowType(*indexFirst++, *senseList++, *rhsList++, *rangeList++);
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setContinuous(const int* indices, int len)
{
  for (int i = 0; i < len; ++i) {
    setContinuous(indices[i]);
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setInteger(const int* indices, int len)
{
  for (int i = 0; i < len; ++i) {
    setInteger(indices[i]);
  }
}
//-----------------------------------------------------------------------------
/* Set the objective coefficients for all columns
    array [getNumCols()] is an array of values for the objective.
    This defaults to a series of set operations and is here for speed.
*/
void OsiSolverInterface::setObjective(const double * array)
{
  int n=getNumCols();
  for (int i=0;i<n;i++)
    setObjCoeff(i,array[i]);
}
//-----------------------------------------------------------------------------
/* Set the lower bounds for all columns
    array [getNumCols()] is an array of values for the objective.
    This defaults to a series of set operations and is here for speed.
*/
void OsiSolverInterface::setColLower(const double * array)
{
  int n=getNumCols();
  for (int i=0;i<n;i++)
    setColLower(i,array[i]);
}
//-----------------------------------------------------------------------------
/* Set the upper bounds for all columns
    array [getNumCols()] is an array of values for the objective.
    This defaults to a series of set operations and is here for speed.
*/
void OsiSolverInterface::setColUpper(const double * array)
{
  int n=getNumCols();
  for (int i=0;i<n;i++)
    setColUpper(i,array[i]);
}
//-----------------------------------------------------------------------------
/*
  Add a named column. Less than efficient, because the underlying solver almost
  surely generates an internal name when creating the column, which is then
  replaced.
*/
void OsiSolverInterface::addCol(const CoinPackedVectorBase& vec,
				const double collb, const double colub,
				const double obj, std::string name)
{
  int ndx = getNumCols() ;
  addCol(vec,collb,colub,obj) ;
  setColName(ndx,name) ;
}

void OsiSolverInterface::addCol(int numberElements,
				const int* rows, const double* elements,
				const double collb, const double colub,
				const double obj, std::string name)
{
  int ndx = getNumCols() ;
  addCol(numberElements,rows,elements,collb,colub,obj) ;
  setColName(ndx,name) ;
}

/* Convenience alias for addCol */
void 
OsiSolverInterface::addCol(int numberElements,
			   const int* rows, const double* elements,
			   const double collb, const double colub,   
			   const double obj) 
{
  CoinPackedVector column(numberElements, rows, elements);
  addCol(column,collb,colub,obj);
}
//-----------------------------------------------------------------------------
/* Add a set of columns (primal variables) to the problem.
   
  Default implementation simply makes repeated calls to addCol().
*/
void
OsiSolverInterface::addCols(const int numcols,
			    const CoinPackedVectorBase* const* cols,
			    const double* collb, const double* colub,   
			    const double* obj)
{
  for (int i = 0; i < numcols; ++i) {
    addCol(*cols[i], collb[i], colub[i], obj[i]);
  }
}

void OsiSolverInterface::addCols(const int numcols, const int* columnStarts,
				 const int* rows, const double* elements,
				 const double* collb, const double* colub,   
				 const double* obj)
{
  double infinity = getInfinity();
  for (int i = 0; i < numcols; ++i) {
    int start = columnStarts[i];
    int number = columnStarts[i+1]-start;
    assert (number>=0);
    addCol(number, rows+start, elements+start, collb ? collb[i] : 0.0, 
	   colub ? colub[i] : infinity, 
	   obj ? obj[i] : 0.0);
  }
}
//-----------------------------------------------------------------------------
// Add columns from a build object
void 
OsiSolverInterface::addCols(const CoinBuild & buildObject)
{
  assert (buildObject.type()==1); // check correct
  int number = buildObject.numberColumns();
  if (number) {
    CoinPackedVectorBase ** columns=
      new CoinPackedVectorBase * [number];
    int iColumn;
    double * objective = new double [number];
    double * lower = new double [number];
    double * upper = new double [number];
    for (iColumn=0;iColumn<number;iColumn++) {
      const int * rows;
      const double * elements;
      int numberElements = buildObject.column(iColumn,lower[iColumn],
                                              upper[iColumn],objective[iColumn],
                                              rows,elements);
      columns[iColumn] = 
	new CoinPackedVector(numberElements,
			     rows,elements);
    }
    addCols(number, columns, lower, upper,objective);
    for (iColumn=0;iColumn<number;iColumn++) 
      delete columns[iColumn];
    delete [] columns;
    delete [] objective;
    delete [] lower;
    delete [] upper;
  }
  return;
}
// Add columns from a model object
int 
OsiSolverInterface::addCols( CoinModel & modelObject)
{
  bool goodState=true;
  if (modelObject.rowLowerArray()) {
    // some row information exists
    int numberRows2 = modelObject.numberRows();
    const double * rowLower = modelObject.rowLowerArray();
    const double * rowUpper = modelObject.rowUpperArray();
    for (int i=0;i<numberRows2;i++) {
      if (rowLower[i]!=-COIN_DBL_MAX) 
        goodState=false;
      if (rowUpper[i]!=COIN_DBL_MAX) 
        goodState=false;
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
                                 objective, integerType,associated);
    }
    CoinPackedMatrix matrix;
    modelObject.createPackedMatrix(matrix,associated);
    int numberColumns = getNumCols(); // save number of columns
    int numberColumns2 = modelObject.numberColumns();
    if (numberColumns2&&!numberErrors) {
      // Clean up infinity
      double infinity = getInfinity();
      int iColumn;
      for (iColumn=0;iColumn<numberColumns2;iColumn++) {
	if (columnUpper[iColumn]>1.0e30)
	  columnUpper[iColumn]=infinity;
	if (columnLower[iColumn]<-1.0e30)
	  columnLower[iColumn]=-infinity;
      }
      const int * row = matrix.getIndices();
      const int * columnLength = matrix.getVectorLengths();
      const CoinBigIndex * columnStart = matrix.getVectorStarts();
      const double * element = matrix.getElements();
      CoinPackedVectorBase ** columns=
        new CoinPackedVectorBase * [numberColumns2];
      assert (columnLower);
      for (iColumn=0;iColumn<numberColumns2;iColumn++) {
        int start = columnStart[iColumn];
        columns[iColumn] = 
          new CoinPackedVector(columnLength[iColumn],
                               row+start,element+start);
      }
      addCols(numberColumns2, columns, columnLower, columnUpper,objective);
      for (iColumn=0;iColumn<numberColumns2;iColumn++) 
        delete columns[iColumn];
      delete [] columns;
      // Do integers if wanted
      assert(integerType);
      for (iColumn=0;iColumn<numberColumns2;iColumn++) {
        if (integerType[iColumn])
          setInteger(iColumn+numberColumns);
      }
    }
    if (columnLower!=modelObject.columnLowerArray()) {
      delete [] rowLower;
      delete [] rowUpper;
      delete [] columnLower;
      delete [] columnUpper;
      delete [] objective;
      delete [] integerType;
      delete [] associated;
      //if (numberErrors)
      //handler_->message(CLP_BAD_STRING_VALUES,messages_)
      //  <<numberErrors
      //  <<CoinMessageEol;
    }
    return numberErrors;
  } else {
    // not suitable for addColumns
    //handler_->message(CLP_COMPLICATED_MODEL,messages_)
    //<<modelObject.numberRows()
    //<<modelObject.numberColumns()
    //<<CoinMessageEol;
    return -1;
  }
}
//-----------------------------------------------------------------------------
/*
  Add a named row. Less than efficient, because the underlying solver almost
  surely generates an internal name when creating the row, which is then
  replaced.
*/
void OsiSolverInterface::addRow(const CoinPackedVectorBase& vec,
				const double rowlb, const double rowub,
				std::string name)
{
  int ndx = getNumRows() ;
  addRow(vec,rowlb,rowub) ;
  setRowName(ndx,name) ;
}

void OsiSolverInterface::addRow(const CoinPackedVectorBase& vec,
				const char rowsen, const double rowrhs,
				const double rowrng, std::string name)
{
  int ndx = getNumRows() ;
  addRow(vec,rowsen,rowrhs,rowrng) ;
  setRowName(ndx,name) ;
}

/* Convenience alias for addRow. */
void 
OsiSolverInterface::addRow(int numberElements,
			   const int *columns, const double *elements,
			   const double rowlb, const double rowub) 
{
  CoinPackedVector row(numberElements,columns,elements);
  addRow(row,rowlb,rowub);
}
//-----------------------------------------------------------------------------
/* Add a set of rows (constraints) to the problem.
   
  The default implementation simply makes repeated calls to addRow().
*/
void 
OsiSolverInterface::addRows(const int numrows, const int* rowStarts,
			    const int* columns, const double* elements,
			    const double* rowlb, const double* rowub)
{
  double infinity = getInfinity();
  for (int i = 0; i < numrows; ++i) {
    int start = rowStarts[i];
    int number = rowStarts[i+1]-start;
    assert (number>=0);
    addRow(number, columns+start, elements+start, rowlb ? rowlb[i] : -infinity, 
	   rowub ? rowub[i] : infinity);
  }
}

void
OsiSolverInterface::addRows(const int numrows,
			    const CoinPackedVectorBase* const* rows,
			    const double* rowlb, const double* rowub)
{
  for (int i = 0; i < numrows; ++i) {
    addRow(*rows[i], rowlb[i], rowub[i]);
  }
}

void
OsiSolverInterface::addRows(const int numrows,
			    const CoinPackedVectorBase* const* rows,
			    const char* rowsen, const double* rowrhs,   
			    const double* rowrng)
{
  for (int i = 0; i < numrows; ++i) {
    addRow(*rows[i], rowsen[i], rowrhs[i], rowrng[i]);
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::addRows(const CoinBuild & buildObject)
{
  int number = buildObject.numberRows();
  if (number) {
    CoinPackedVectorBase ** rows=
      new CoinPackedVectorBase * [number];
    int iRow;
    double * lower = new double [number];
    double * upper = new double [number];
    for (iRow=0;iRow<number;iRow++) {
      const int * columns;
      const double * elements;
      int numberElements = buildObject.row(iRow,lower[iRow],upper[iRow],
                                           columns,elements);
      rows[iRow] = 
	new CoinPackedVector(numberElements,
			     columns,elements);
    }
    addRows(number, rows, lower, upper);
    for (iRow=0;iRow<number;iRow++) 
      delete rows[iRow];
    delete [] rows;
    delete [] lower;
    delete [] upper;
  }
}
//-----------------------------------------------------------------------------
int
OsiSolverInterface::addRows( CoinModel & modelObject)
{
  bool goodState=true;
  if (modelObject.columnLowerArray()) {
    // some column information exists
    int numberColumns2 = modelObject.numberColumns();
    const double * columnLower = modelObject.columnLowerArray();
    const double * columnUpper = modelObject.columnUpperArray();
    const double * objective = modelObject.objectiveArray();
    const int * integerType = modelObject.integerTypeArray();
    for (int i=0;i<numberColumns2;i++) {
      if (columnLower[i]!=0.0) 
        goodState=false;
      if (columnUpper[i]!=COIN_DBL_MAX) 
        goodState=false;
      if (objective[i]!=0.0) 
        goodState=false;
      if (integerType[i]!=0)
        goodState=false;
    }
  }
  if (goodState) {
    // can do addRows
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
                                 objective, integerType,associated);
    }
    CoinPackedMatrix matrix;
    modelObject.createPackedMatrix(matrix,associated);
    int numberRows2 = modelObject.numberRows();
    if (numberRows2&&!numberErrors) {
      // Clean up infinity
      double infinity = getInfinity();
      int iRow;
      for (iRow=0;iRow<numberRows2;iRow++) {
	if (rowUpper[iRow]>1.0e30)
	  rowUpper[iRow]=infinity;
	if (rowLower[iRow]<-1.0e30)
	  rowLower[iRow]=-infinity;
      }
      // matrix by rows
      matrix.reverseOrdering();
      const int * column = matrix.getIndices();
      const int * rowLength = matrix.getVectorLengths();
      const CoinBigIndex * rowStart = matrix.getVectorStarts();
      const double * element = matrix.getElements();
      CoinPackedVectorBase ** rows=
        new CoinPackedVectorBase * [numberRows2];
      assert (rowLower);
      for (iRow=0;iRow<numberRows2;iRow++) {
        int start = rowStart[iRow];
        rows[iRow] = 
          new CoinPackedVector(rowLength[iRow],
                               column+start,element+start);
      }
      addRows(numberRows2, rows, rowLower, rowUpper);
      for (iRow=0;iRow<numberRows2;iRow++) 
        delete rows[iRow];
      delete [] rows;
    }
    if (rowLower!=modelObject.rowLowerArray()) {
      delete [] rowLower;
      delete [] rowUpper;
      delete [] columnLower;
      delete [] columnUpper;
      delete [] objective;
      delete [] integerType;
      delete [] associated;
      //if (numberErrors)
      //handler_->message(CLP_BAD_STRING_VALUES,messages_)
      //  <<numberErrors
      //  <<CoinMessageEol;
    }
    return numberErrors;
  } else {
    // not suitable for addRows
    //handler_->message(CLP_COMPLICATED_MODEL,messages_)
    //<<modelObject.numberRows()
    //<<modelObject.numberColumns()
    //<<CoinMessageEol;
    return -1;
  }
}
/*  Strip off rows to get to this number of rows.
    If solver wants it can restore a copy of "base" (continuous) model here
*/
void 
OsiSolverInterface::restoreBaseModel(int numberRows)
{
  int currentNumberCuts = getNumRows()-numberRows;
  int *which = new int[currentNumberCuts];
  for (int i = 0 ; i < currentNumberCuts ; i++)
    which[i] = i+numberRows;
  deleteRows(currentNumberCuts,which);
  delete [] which;
}
  
// This loads a model from a coinModel object - returns number of errors
int 
OsiSolverInterface::loadFromCoinModel (  CoinModel & modelObject, bool keepSolution)
{
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
                                            objective, integerType,associated);
  }
  CoinPackedMatrix matrix;
  modelObject.createPackedMatrix(matrix,associated);
  int numberRows = modelObject.numberRows();
  int numberColumns = modelObject.numberColumns();
  // Clean up infinity
  double infinity = getInfinity();
  int iColumn,iRow;
  for (iColumn=0;iColumn<numberColumns;iColumn++) {
    if (columnUpper[iColumn]>1.0e30)
      columnUpper[iColumn]=infinity;
    if (columnLower[iColumn]<-1.0e30)
      columnLower[iColumn]=-infinity;
  }
  for (iRow=0;iRow<numberRows;iRow++) {
    if (rowUpper[iRow]>1.0e30)
      rowUpper[iRow]=infinity;
    if (rowLower[iRow]<-1.0e30)
      rowLower[iRow]=-infinity;
  }
  CoinWarmStart * ws = getWarmStart();
  bool restoreBasis = keepSolution && numberRows&&numberRows==getNumRows()&&
    numberColumns==getNumCols();
  loadProblem(matrix, 
              columnLower, columnUpper, objective, rowLower, rowUpper);
  setRowColNames(modelObject) ;
  if (restoreBasis)
    setWarmStart(ws);
  delete ws;
  // Do integers if wanted
  assert(integerType);
  for (int iColumn=0;iColumn<numberColumns;iColumn++) {
    if (integerType[iColumn])
      setInteger(iColumn);
  }
  if (rowLower!=modelObject.rowLowerArray()||
      columnLower!=modelObject.columnLowerArray()) {
    delete [] rowLower;
    delete [] rowUpper;
    delete [] columnLower;
    delete [] columnUpper;
    delete [] objective;
    delete [] integerType;
    delete [] associated;
    //if (numberErrors)
    //  handler_->message(CLP_BAD_STRING_VALUES,messages_)
    //    <<numberErrors
    //    <<CoinMessageEol;
  }
  return numberErrors;
}
//-----------------------------------------------------------------------------

//#############################################################################
// Implement getObjValue in a simple way that the derived solver interfaces
// can use if the choose.
//#############################################################################
double OsiSolverInterface::getObjValue() const
{
  int nc = getNumCols();
  const double * objCoef = getObjCoefficients();
  const double * colSol  = getColSolution();
  double objOffset=0.0;
  getDblParam(OsiObjOffset,objOffset);
  
  // Compute dot product of objCoef and colSol and then adjust by offset
  // Jean-Sebastien pointed out this is overkill - lets just do it simply
  //double retVal = CoinPackedVector(nc,objCoef).dotProduct(colSol)-objOffset;
  double retVal = -objOffset;
  for ( int i=0 ; i<nc ; i++ )
    retVal += objCoef[i]*colSol[i];
  return retVal;
}

//#############################################################################
// Apply Cuts
//#############################################################################

OsiSolverInterface::ApplyCutsReturnCode
OsiSolverInterface::applyCuts( const OsiCuts & cs, double effectivenessLb ) 
{
  OsiSolverInterface::ApplyCutsReturnCode retVal;
  int i;

  // Loop once for each column cut
  for ( i=0; i<cs.sizeColCuts(); i ++ ) {
    if ( cs.colCut(i).effectiveness() < effectivenessLb ) {
      retVal.incrementIneffective();
      continue;
    }
    if ( !cs.colCut(i).consistent() ) {
      retVal.incrementInternallyInconsistent();
      continue;
    }
    if ( !cs.colCut(i).consistent(*this) ) {
      retVal.incrementExternallyInconsistent();
      continue;
    }
    if ( cs.colCut(i).infeasible(*this) ) {
      retVal.incrementInfeasible();
      continue;
    }
    applyColCut( cs.colCut(i) );
    retVal.incrementApplied();
  }

  // Loop once for each row cut
  for ( i=0; i<cs.sizeRowCuts(); i ++ ) {
    if ( cs.rowCut(i).effectiveness() < effectivenessLb ) {
      retVal.incrementIneffective();
      continue;
    }
    if ( !cs.rowCut(i).consistent() ) {
      retVal.incrementInternallyInconsistent();
      continue;
    }
    if ( !cs.rowCut(i).consistent(*this) ) {
      retVal.incrementExternallyInconsistent();
      continue;
    }
    if ( cs.rowCut(i).infeasible(*this) ) {
      retVal.incrementInfeasible();
      continue;
    }
    applyRowCut( cs.rowCut(i) );
    retVal.incrementApplied();
  }
  
  return retVal;
}
/* Apply a collection of row cuts which are all effective.
   applyCuts seems to do one at a time which seems inefficient.
   The default does slowly, but solvers can override.
*/
void 
OsiSolverInterface::applyRowCuts(int numberCuts, const OsiRowCut * cuts)
{
  int i;
  for (i=0;i<numberCuts;i++) {
    applyRowCut(cuts[i]);
  }
}
// And alternatively
void 
OsiSolverInterface::applyRowCuts(int numberCuts, const OsiRowCut ** cuts)
{
  int i;
  for (i=0;i<numberCuts;i++) {
    applyRowCut(*cuts[i]);
  }
}
//#############################################################################
// Set/Get Application Data
// This is a pointer that the application can store into and retrieve
// from the solverInterface.
// This field is the application to optionally define and use.
//#############################################################################

void OsiSolverInterface::setApplicationData(void * appData)
{
  delete appDataEtc_;
  appDataEtc_ = new OsiAuxInfo(appData);
}
//-----------------------------------------------------------------------------
void * OsiSolverInterface::getApplicationData() const
{
  return appDataEtc_->getApplicationData();
}
void 
OsiSolverInterface::setAuxiliaryInfo(OsiAuxInfo * auxiliaryInfo)
{ 
  delete appDataEtc_;
  appDataEtc_ = auxiliaryInfo->clone();
}
// Get pointer to auxiliary info object
OsiAuxInfo * 
OsiSolverInterface::getAuxiliaryInfo() const
{
  return appDataEtc_;
}

//#############################################################################
// Methods related to Row Cut Debuggers
//#############################################################################

//-------------------------------------------------------------------
// Activate Row Cut Debugger<br>
// If the model name passed is on list of known models
// then all cuts are checked to see that they do NOT cut
// off the known optimal solution.  

void OsiSolverInterface::activateRowCutDebugger (const char * modelName)
{
  delete rowCutDebugger_;
  rowCutDebugger_=NULL; // so won't use in new
  rowCutDebugger_ = new OsiRowCutDebugger(*this,modelName);
}
/* Activate debugger using full solution array.
   Only integer values need to be correct.
   Up to user to get it correct.
*/
void OsiSolverInterface::activateRowCutDebugger (const double * solution,
						 bool keepContinuous)
{
  delete rowCutDebugger_;
  rowCutDebugger_=NULL; // so won't use in new
  rowCutDebugger_ = new OsiRowCutDebugger(*this,solution,keepContinuous);
}
//-------------------------------------------------------------------
// Get Row Cut Debugger<br>
// If there is a row cut debugger object associated with
// model AND if the known optimal solution is within the
// current feasible region then a pointer to the object is
// returned which may be used to test validity of cuts.
// Otherwise NULL is returned

const OsiRowCutDebugger * OsiSolverInterface::getRowCutDebugger() const
{
  if (rowCutDebugger_&&rowCutDebugger_->onOptimalPath(*this)) {
    return rowCutDebugger_;
  } else {
    return NULL;
  }
}
// If you want to get debugger object even if not on optimal path then use this
OsiRowCutDebugger * OsiSolverInterface::getRowCutDebuggerAlways() const
{
  if (rowCutDebugger_&&rowCutDebugger_->active()) {
    return rowCutDebugger_;
  } else {
    return NULL;
  }
}

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiSolverInterface::OsiSolverInterface () :
  rowCutDebugger_(NULL),
  handler_(NULL),
  defaultHandler_(true),
  columnType_(NULL),
  appDataEtc_(NULL),
  ws_(NULL)
{
  setInitialData();
}
// Set data for default constructor
void 
OsiSolverInterface::setInitialData()
{
  delete rowCutDebugger_;
  rowCutDebugger_ = NULL;
  delete ws_;
  ws_ = NULL;
  delete appDataEtc_;
  appDataEtc_ = new OsiAuxInfo(); 
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL;
  }
  defaultHandler_=true;
  delete [] columnType_;
  columnType_ = NULL;
  intParam_[OsiMaxNumIteration] = 9999999;
  intParam_[OsiMaxNumIterationHotStart] = 9999999;
  intParam_[OsiNameDiscipline] = 0;

  // Dual objective limit is acceptable `badness'; for minimisation, COIN_DBL_MAX
  dblParam_[OsiDualObjectiveLimit] = COIN_DBL_MAX;
  // Primal objective limit is desired `goodness'; for minimisation, -COIN_DBL_MAX
  dblParam_[OsiPrimalObjectiveLimit] = -COIN_DBL_MAX;
  dblParam_[OsiDualTolerance] = 1e-6;
  dblParam_[OsiPrimalTolerance] = 1e-6;
  dblParam_[OsiObjOffset] = 0.0;

  strParam_[OsiProbName] = "OsiDefaultName";
  strParam_[OsiSolverName] = "Unknown Solver";
  handler_ = new CoinMessageHandler();
  messages_ = CoinMessage();

  // initialize all hints
  int hint;
  for (hint=OsiDoPresolveInInitial;hint<OsiLastHintParam;hint++) {
    hintParam_[hint] = false;
    hintStrength_[hint] = OsiHintIgnore;
  }
  // objects
  numberObjects_=0;
  numberIntegers_=-1;
  object_=NULL;

  // names
  rowNames_ = OsiNameVec(0) ;
  colNames_ = OsiNameVec(0) ;
  objName_ = "" ;
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiSolverInterface::OsiSolverInterface (const OsiSolverInterface & rhs) :
  rowCutDebugger_(NULL),
  ws_(NULL)
{  
  appDataEtc_ = rhs.appDataEtc_->clone();
  if ( rhs.rowCutDebugger_!=NULL )
    rowCutDebugger_ = new OsiRowCutDebugger(*rhs.rowCutDebugger_);
  defaultHandler_ = rhs.defaultHandler_;
  if (defaultHandler_) {
    handler_ = new CoinMessageHandler(*rhs.handler_);
  } else {
    handler_ = rhs.handler_;
  }
  messages_ = CoinMessages(rhs.messages_);
  CoinDisjointCopyN(rhs.intParam_, OsiLastIntParam, intParam_);
  CoinDisjointCopyN(rhs.dblParam_, OsiLastDblParam, dblParam_);
  CoinDisjointCopyN(rhs.strParam_, OsiLastStrParam, strParam_);
  CoinDisjointCopyN(rhs.hintParam_, OsiLastHintParam, hintParam_);
  CoinDisjointCopyN(rhs.hintStrength_, OsiLastHintParam, hintStrength_);
  // objects
  numberObjects_=rhs.numberObjects_;
  numberIntegers_=rhs.numberIntegers_;
  if (numberObjects_) {
    object_ = new OsiObject * [numberObjects_];
    for (int i=0;i<numberObjects_;i++) 
      object_[i] = rhs.object_[i]->clone();
  } else {
    object_=NULL;
  }
  // names
  rowNames_ = rhs.rowNames_ ;
  colNames_ = rhs.colNames_ ;
  objName_ = rhs.objName_ ;
  // NULL as number of columns not known
  columnType_ = NULL;
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiSolverInterface::~OsiSolverInterface ()
{
  // delete debugger - should be safe as only ever returned const
  delete rowCutDebugger_;
  rowCutDebugger_ = NULL;
  delete ws_;
  ws_ = NULL;
  delete appDataEtc_;
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL;
  }
  for (int i=0;i<numberObjects_;i++) 
    delete object_[i];
  delete [] object_;
  delete [] columnType_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiSolverInterface &
OsiSolverInterface::operator=(const OsiSolverInterface& rhs)
{
  if (this != &rhs) {
    delete appDataEtc_;
    appDataEtc_ = rhs.appDataEtc_->clone();
    delete rowCutDebugger_;
    if ( rhs.rowCutDebugger_!=NULL )
      rowCutDebugger_ = new OsiRowCutDebugger(*rhs.rowCutDebugger_);
    else
      rowCutDebugger_ = NULL;
    CoinDisjointCopyN(rhs.intParam_, OsiLastIntParam, intParam_);
    CoinDisjointCopyN(rhs.dblParam_, OsiLastDblParam, dblParam_);
    CoinDisjointCopyN(rhs.strParam_, OsiLastStrParam, strParam_);
    CoinDisjointCopyN(rhs.hintParam_, OsiLastHintParam, hintParam_);
    CoinDisjointCopyN(rhs.hintStrength_, OsiLastHintParam, hintStrength_);
    delete ws_;
    ws_ = NULL;
    if (defaultHandler_) {
      delete handler_;
      handler_ = NULL;
    }
    defaultHandler_ = rhs.defaultHandler_;
    if (defaultHandler_) {
      handler_ = new CoinMessageHandler(*rhs.handler_);
    } else {
      handler_ = rhs.handler_;
    }
    // objects
    int i;
    for (i=0;i<numberObjects_;i++) 
      delete object_[i];
    delete [] object_;
    numberObjects_=rhs.numberObjects_;
    numberIntegers_=rhs.numberIntegers_;
    if (numberObjects_) {
      object_ = new OsiObject * [numberObjects_];
      for (i=0;i<numberObjects_;i++) 
	object_[i] = rhs.object_[i]->clone();
    } else {
      object_=NULL;
    }
    // names
    rowNames_ = rhs.rowNames_ ;
    colNames_ = rhs.colNames_ ;
    objName_ = rhs.objName_ ;
    delete [] columnType_;
    // NULL as number of columns not known
    columnType_ = NULL;
  }
  return *this;
}

//-----------------------------------------------------------------------------
// Read mps files
//-----------------------------------------------------------------------------

int OsiSolverInterface::readMps(const char * filename,
    const char * extension)
{
  CoinMpsIO m;

  int logLvl = handler_->logLevel() ;
  if (logLvl > 1)
  { m.messageHandler()->setLogLevel(handler_->logLevel()) ; }
  else
  { m.messageHandler()->setLogLevel(0) ; }
  m.setInfinity(getInfinity());
  
  int numberErrors = m.readMps(filename,extension);
  handler_->message(COIN_SOLVER_MPS,messages_)
    <<m.getProblemName()<< numberErrors <<CoinMessageEol;
  if (!numberErrors) {

    // set objective function offest
    setDblParam(OsiObjOffset,m.objectiveOffset());

    // set problem name
    setStrParam(OsiProbName,m.getProblemName());

    // no errors --- load problem, set names and integrality
    loadProblem(*m.getMatrixByCol(),m.getColLower(),m.getColUpper(),
		m.getObjCoefficients(),m.getRowSense(),m.getRightHandSide(),
		m.getRowRange());
    setRowColNames(m) ;
    const char * integer = m.integerColumns();
    if (integer) {
      int i,n=0;
      int nCols=m.getNumCols();
      int * index = new int [nCols];
      for (i=0;i<nCols;i++) {
	if (integer[i]) {
	  index[n++]=i;
	}
      }
      setInteger(index,n);
      delete [] index;
    }
  }
  return numberErrors;
}
/* Read a problem in GMPL format from the given filenames.
   
  Will only work if glpk installed
*/
int 
OsiSolverInterface::readGMPL(const char *filename, const char * dataname)
{
  CoinMpsIO m;
  m.setInfinity(getInfinity());
  m.passInMessageHandler(handler_);

  int numberErrors = m.readGMPL(filename,dataname,false);
  handler_->message(COIN_SOLVER_MPS,messages_)
    <<m.getProblemName()<< numberErrors <<CoinMessageEol;
  if (!numberErrors) {

    // set objective function offest
    setDblParam(OsiObjOffset,m.objectiveOffset());

    // set problem name
    setStrParam(OsiProbName,m.getProblemName());

    // no errors --- load problem, set names and integrality
    loadProblem(*m.getMatrixByCol(),m.getColLower(),m.getColUpper(),
		m.getObjCoefficients(),m.getRowSense(),m.getRightHandSide(),
		m.getRowRange());
    setRowColNames(m) ;
    const char * integer = m.integerColumns();
    if (integer) {
      int i,n=0;
      int nCols=m.getNumCols();
      int * index = new int [nCols];
      for (i=0;i<nCols;i++) {
	if (integer[i]) {
	  index[n++]=i;
	}
      }
      setInteger(index,n);
      delete [] index;
    }
  }
  return numberErrors;
}
 /* Read a problem in MPS format from the given full filename.
   
This uses CoinMpsIO::readMps() to read
the MPS file and returns the number of errors encountered.
It also may return an array of set information
*/
int 
OsiSolverInterface::readMps(const char *filename, const char*extension,
			    int & numberSets, CoinSet ** & sets)
{
  CoinMpsIO m;
  m.setInfinity(getInfinity());
  
  int numberErrors = m.readMps(filename,extension,numberSets,sets);
  handler_->message(COIN_SOLVER_MPS,messages_)
    <<m.getProblemName()<< numberErrors <<CoinMessageEol;
  if (!numberErrors) {
    // set objective function offset
    setDblParam(OsiObjOffset,m.objectiveOffset());

    // set problem name
    setStrParam(OsiProbName,m.getProblemName());

    // no errors --- load problem, set names and integrality
    loadProblem(*m.getMatrixByCol(),m.getColLower(),m.getColUpper(),
		m.getObjCoefficients(),m.getRowSense(),m.getRightHandSide(),
		m.getRowRange());
    setRowColNames(m) ;
    const char * integer = m.integerColumns();
    if (integer) {
      int i,n=0;
      int nCols=m.getNumCols();
      int * index = new int [nCols];
      for (i=0;i<nCols;i++) {
	if (integer[i]) {
	  index[n++]=i;
	}
      }
      setInteger(index,n);
      delete [] index;
    }
  }
  return numberErrors;
}

int 
OsiSolverInterface::writeMpsNative(const char *filename, 
				   const char ** rowNames, 
				   const char ** columnNames,
				   int formatType,
				   int numberAcross,
				   double objSense,
				   int numberSOS,
				   const CoinSet * setInfo ) const
{
   const int numcols = getNumCols();
   char* integrality = new char[numcols];
   bool hasInteger = false;
   for (int i = 0; i < numcols; ++i) {
      if (isInteger(i)) {
	 integrality[i] = 1;
	 hasInteger = true;
      } else {
	 integrality[i] = 0;
      }
   }

   // Get multiplier for objective function - default 1.0
   double * objective = new double[numcols];
   memcpy(objective,getObjCoefficients(),numcols*sizeof(double));
   //if (objSense*getObjSense()<0.0) {
   double locObjSense = (objSense == 0 ? 1 : objSense);
   if(getObjSense() * locObjSense < 0.0) {
     for (int i = 0; i < numcols; ++i) 
       objective [i] = - objective[i];
   }

   CoinMpsIO writer;
   writer.setInfinity(getInfinity());
   writer.passInMessageHandler(handler_);
   writer.setMpsData(*getMatrixByCol(), getInfinity(),
		     getColLower(), getColUpper(),
		     objective, hasInteger ? integrality : 0,
		     getRowLower(), getRowUpper(),
		     columnNames,rowNames);
   double objOffset=0.0;
   getDblParam(OsiObjOffset,objOffset);
   writer.setObjectiveOffset(objOffset);
   delete [] objective;
   delete[] integrality;
   return writer.writeMps(filename, 1 /*gzip it*/, formatType, numberAcross,
			  NULL,numberSOS,setInfo);
}
/***********************************************************************/
void OsiSolverInterface::writeLp(const char * filename,
				 const char * extension,
				  double epsilon,
				  int numberAcross,
				  int decimals,
				  double objSense,
				  bool useRowNames) const
{
  std::string f(filename);
  std::string e(extension);
  std::string fullname;
  if (e!="") {
    fullname = f + "." + e;
  } else {
    // no extension so no trailing period
    fullname = f;
  }

	char** colnames;
	char** rownames;
  int nameDiscipline;
  if (!getIntParam(OsiNameDiscipline,nameDiscipline))
     nameDiscipline = 0;
	if (useRowNames && nameDiscipline==2) {
		colnames = new char*[getNumCols()];
		rownames = new char*[getNumRows()+1];
		for (int i = 0; i < getNumCols(); ++i)
			colnames[i] = strdup(getColName(i).c_str());
		for (int i = 0; i < getNumRows(); ++i)
			rownames[i] = strdup(getRowName(i).c_str());
		rownames[getNumRows()] = strdup(getObjName().c_str());
	} else {
		colnames = NULL;
		rownames = NULL;
	}

  // Fall back on Osi version
  OsiSolverInterface::writeLpNative(fullname.c_str(), 
				    rownames, colnames, epsilon, numberAcross,
				    decimals, objSense, useRowNames);

	if (useRowNames && nameDiscipline==2) {
		for (int i = 0; i < getNumCols(); ++i)
			free(colnames[i]);
		for (int i = 0; i < getNumRows()+1; ++i)
			free(rownames[i]);
		delete[] colnames;
		delete[] rownames;
	}
}

/*************************************************************************/
void OsiSolverInterface::writeLp(FILE *fp,
				  double epsilon,
				  int numberAcross,
				  int decimals,
				  double objSense,
				  bool useRowNames) const
{
	char** colnames;
	char** rownames;
  int nameDiscipline;
  getIntParam(OsiNameDiscipline,nameDiscipline) ;
	if (useRowNames && nameDiscipline==2) {
		colnames = new char*[getNumCols()];
		rownames = new char*[getNumRows()+1];
		for (int i = 0; i < getNumCols(); ++i)
			colnames[i] = strdup(getColName(i).c_str());
		for (int i = 0; i < getNumRows(); ++i)
			rownames[i] = strdup(getRowName(i).c_str());
      rownames[getNumRows()] = strdup(getObjName().c_str());
	} else {
		colnames = NULL;
		rownames = NULL;
	}

  // Fall back on Osi version
  OsiSolverInterface::writeLpNative(fp, 
				    rownames, colnames, epsilon, numberAcross,
				    decimals, objSense, useRowNames);

	if (useRowNames && nameDiscipline==2) {
		for (int i = 0; i < getNumCols(); ++i)
			free(colnames[i]);
		for (int i = 0; i < getNumRows()+1; ++i)
			free(rownames[i]);
		delete[] colnames;
		delete[] rownames;
	}
}

/***********************************************************************/
int
OsiSolverInterface::writeLpNative(const char *filename,
				  char const * const * const rowNames,
				  char const * const * const columnNames,
				  const double epsilon,
			 	  const int numberAcross,
				  const int decimals,
				  const double objSense,
				  const bool useRowNames) const
{
  FILE *fp = NULL;
  fp = fopen(filename,"w");
  if (!fp) {
    printf("### ERROR: in OsiSolverInterface::writeLpNative(): unable to open file %s\n",
	   filename);
    exit(1);
  }
  int nerr = writeLpNative(fp,rowNames, columnNames,
			   epsilon, numberAcross, decimals, 
			   objSense, useRowNames);
  fclose(fp);
  return(nerr);
}

/***********************************************************************/
int
OsiSolverInterface::writeLpNative(FILE *fp,
				  char const * const * const rowNames,
				  char const * const * const columnNames,
				  const double epsilon,
				  const int numberAcross,
				  const int decimals,
				  const double objSense,
				  const bool useRowNames) const
{
   const int numcols = getNumCols();
   char *integrality = new char[numcols];
   bool hasInteger = false;

   for (int i=0; i<numcols; i++) {
     if (isInteger(i)) {
       integrality[i] = 1;
       hasInteger = true;
     } else {
       integrality[i] = 0;
     }
   }

   // Get multiplier for objective function - default 1.0
   double *objective = new double[numcols];
   const double *curr_obj = getObjCoefficients();

   //if(getObjSense() * objSense < 0.0) {
   double locObjSense = (objSense == 0 ? 1 : objSense);
   if(getObjSense() * locObjSense < 0.0) {
     for (int i=0; i<numcols; i++) {
       objective[i] = - curr_obj[i];
     }
   }
   else {
     for (int i=0; i<numcols; i++) {
       objective[i] = curr_obj[i];
     }
   }

   CoinLpIO writer;
   writer.setInfinity(getInfinity());
   writer.setEpsilon(epsilon);
   writer.setNumberAcross(numberAcross);
   writer.setDecimals(decimals);

   writer.setLpDataWithoutRowAndColNames(*getMatrixByRow(),
		     getColLower(), getColUpper(),
		     objective, hasInteger ? integrality : 0,
		     getRowLower(), getRowUpper());

   writer.setLpDataRowAndColNames(rowNames, columnNames);

   //writer.print();
   delete [] objective;
   delete[] integrality;
   return writer.writeLp(fp, epsilon, numberAcross, decimals, 
			 useRowNames);

} /*writeLpNative */

/*************************************************************************/
int OsiSolverInterface::readLp(const char * filename, const double epsilon) {
  FILE *fp = fopen(filename, "r");

  if(!fp) {
    printf("### ERROR: OsiSolverInterface::readLp():  Unable to open file %s for reading\n",
	   filename);
    return(1);
  }

  int nerr = readLp(fp, epsilon);
  fclose(fp);
  return(nerr);
}

/*************************************************************************/
int OsiSolverInterface::readLp(FILE *fp, const double epsilon) {
  CoinLpIO m;
  m.readLp(fp, epsilon);

  // set objective function offest
  setDblParam(OsiObjOffset, 0);

  // set problem name
  setStrParam(OsiProbName, m.getProblemName());

  // no errors --- load problem, set names and integrality
  loadProblem(*m.getMatrixByRow(), m.getColLower(), m.getColUpper(),
	      m.getObjCoefficients(), m.getRowLower(), m.getRowUpper());
  setRowColNames(m) ;
  const char *integer = m.integerColumns();
  if (integer) {
    int i, n = 0;
    int nCols = m.getNumCols();
    int *index = new int [nCols];
    for (i=0; i<nCols; i++) {
      if (integer[i]) {
	index[n++] = i;
      }
    }
    setInteger(index, n);
    delete [] index;
  }
  setObjSense(1);
  return(0);
} /* readLp */

/*************************************************************************/

// Pass in Message handler (not deleted at end)
void 
OsiSolverInterface::passInMessageHandler(CoinMessageHandler * handler)
{
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL;
  }
  defaultHandler_=false;
  handler_=handler;
}
// Set language
void 
OsiSolverInterface::newLanguage(CoinMessages::Language language)
{
  messages_ = CoinMessage(language);
}
// Is the given primal objective limit reached?
bool
OsiSolverInterface::isPrimalObjectiveLimitReached() const
{
  double primalobjlimit;
  if (getDblParam(OsiPrimalObjectiveLimit, primalobjlimit))
    return getObjSense() * getObjValue() < getObjSense() * primalobjlimit;
  else
    return false;
}
// Is the given dual objective limit reached?
bool
OsiSolverInterface::isDualObjectiveLimitReached() const
{
  double dualobjlimit;
  if (getDblParam(OsiDualObjectiveLimit, dualobjlimit))
    return getObjSense() * getObjValue() > getObjSense() * dualobjlimit;
  else
    return false;
}
// copy all parameters in this section from one solver to another
void 
OsiSolverInterface::copyParameters(OsiSolverInterface & rhs)
{
  /*
    We should think about this block of code. appData, rowCutDebugger,
    and handler_ are not part of the standard parameter data. Arguably copy
    actions for them belong in the base Osi.clone() or as separate methods.
    -lh, 091021-
  */
  delete appDataEtc_;
  appDataEtc_ = rhs.appDataEtc_->clone();
  delete rowCutDebugger_;
  if ( rhs.rowCutDebugger_!=NULL )
    rowCutDebugger_ = new OsiRowCutDebugger(*rhs.rowCutDebugger_);
  else
    rowCutDebugger_ = NULL;
  if (defaultHandler_) {
    delete handler_;
  }
  /*
    Is this correct? Shouldn't we clone a non-default handler instead of
    simply assigning the pointer?  -lh, 091021-
  */
  defaultHandler_ = rhs.defaultHandler_;
  if (defaultHandler_) {
    handler_ = new CoinMessageHandler(*rhs.handler_);
  } else {
    handler_ = rhs.handler_;
  }
  CoinDisjointCopyN(rhs.intParam_, OsiLastIntParam, intParam_);
  CoinDisjointCopyN(rhs.dblParam_, OsiLastDblParam, dblParam_);
  CoinDisjointCopyN(rhs.strParam_, OsiLastStrParam, strParam_);
  CoinDisjointCopyN(rhs.hintParam_, OsiLastHintParam, hintParam_);
  CoinDisjointCopyN(rhs.hintStrength_, OsiLastHintParam, hintStrength_);
}
// Resets as if default constructor
void 
OsiSolverInterface::reset()
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "reset",
		  "OsiSolverInterface");
}
/*Enables normal operation of subsequent functions.
  This method is supposed to ensure that all typical things (like
  reduced costs, etc.) are updated when individual pivots are executed
  and can be queried by other methods.  says whether will be
  doing primal or dual
*/
void 
OsiSolverInterface::enableSimplexInterface(bool ) {}

//Undo whatever setting changes the above method had to make
void 
OsiSolverInterface::disableSimplexInterface() {}
/* Returns 1 if can just do getBInv etc
   2 if has all OsiSimplex methods
   and 0 if it has none */
int 
OsiSolverInterface::canDoSimplexInterface() const
{
  return 0;
}

/* Tells solver that calls to getBInv etc are about to take place.
   Underlying code may need mutable as this may be called from 
   CglCut:;generateCuts which is const.  If that is too horrific then
   each solver e.g. BCP or CBC will have to do something outside
   main loop.
*/
void 
OsiSolverInterface::enableFactorization() const
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "enableFactorization",
		  "OsiSolverInterface");
}
// and stop
void 
OsiSolverInterface::disableFactorization() const
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "disableFactorization",
		  "OsiSolverInterface");
}

//Returns true if a basis is available
bool 
OsiSolverInterface::basisIsAvailable() const 
{
  return false;
}

/* (JJF ?date?) The following two methods may be replaced by the
   methods of OsiSolverInterface using OsiWarmStartBasis if:
   1. OsiWarmStartBasis resize operation is implemented
   more efficiently and
   2. It is ensured that effects on the solver are the same
 
   (lh 100818)
   1. CoinWarmStartBasis is the relevant resize, and John's right, it needs
      to be reworked. The problem is that when new columns or rows are added,
      the new space is initialised two bits at a time. It needs to be done
      in bulk. There are other problems with CoinWarmStartBasis that should
      be addressed at the same time.
   2. setWarmStart followed by resolve should do it.
*/
void 
OsiSolverInterface::getBasisStatus(int* , int* ) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBasisStatus",
		  "OsiSolverInterface");
}

/* Set the status of structural/artificial variables and
   factorize, update solution etc 
   
   NOTE  artificials are treated as +1 elements so for <= rhs
   artificial will be at lower bound if constraint is tight
*/
int 
OsiSolverInterface::setBasisStatus(const int* , const int* ) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "setBasisStatus",
		  "OsiSolverInterface");
}

/* Perform a pivot by substituting a colIn for colOut in the basis. 
   The status of the leaving variable is given in statOut. Where
   1 is to upper bound, -1 to lower bound
*/
int 
OsiSolverInterface::pivot(int , int , int ) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "pivot",
		  "OsiSolverInterface");
}

/* Obtain a result of the primal pivot 
   Outputs: colOut -- leaving column, outStatus -- its status,
   t -- step size, and, if dx!=NULL, *dx -- primal ray direction.
   Inputs: colIn -- entering column, sign -- direction of its change (+/-1).
   Both for colIn and colOut, artificial variables are index by
   the negative of the row index minus 1.
   Return code (for now): 0 -- leaving variable found, 
   -1 -- everything else?
   Clearly, more informative set of return values is required 
   Primal and dual solutions are updated
*/
int 
OsiSolverInterface::primalPivotResult(int, int , 
                                      int& , int& , 
                                      double& , CoinPackedVector* )
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "primalPivotResult",
		  "OsiSolverInterface");
}

/* Obtain a result of the dual pivot (similar to the previous method)
   Differences: entering variable and a sign of its change are now
   the outputs, the leaving variable and its statuts -- the inputs
   If dx!=NULL, then *dx contains dual ray
   Return code: same
*/
int 
OsiSolverInterface::dualPivotResult(int& , int& , 
                                    int , int , 
                                    double& , CoinPackedVector* ) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "dualPivotResult",
		  "OsiSolverInterface");
}

//Get the reduced gradient for the cost vector c 
void 
OsiSolverInterface::getReducedGradient(double* , 
                                       double * ,
                                       const double * ) const
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getReducedGradient",
		  "OsiSolverInterface");
}

//Get a row of the tableau (slack part in slack if not NULL)
void 
OsiSolverInterface::getBInvARow(int , double* , double * ) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBInvARow",
		  "OsiSolverInterface");
}

//Get a row of the basis inverse
void OsiSolverInterface::getBInvRow(int , double* ) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBInvRow",
		  "OsiSolverInterface");
}

//Get a column of the tableau
void 
OsiSolverInterface::getBInvACol(int , double* ) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBInvACol",
		  "OsiSolverInterface");
}

//Get a column of the basis inverse
void 
OsiSolverInterface::getBInvCol(int , double* ) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBInvCol",
		  "OsiSolverInterface");
}
/* Get warm start information.
   Return warm start information for the current state of the solver
   interface. If there is no valid warm start information, an empty warm
   start object wil be returned.  This does not necessarily create an 
   object - may just point to one.  must Delete set true if user
   should delete returned object.
   OsiClp version always returns pointer and false.
*/
CoinWarmStart* 
OsiSolverInterface::getPointerToWarmStart(bool & mustDelete) 
{
  mustDelete = true;
  return getWarmStart();
}

/* Get basic indices (order of indices corresponds to the
   order of elements in a vector retured by getBInvACol() and
   getBInvCol()).
*/
void 
OsiSolverInterface::getBasics(int* ) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBasics",
		  "OsiSolverInterface");
}
#ifdef COIN_SNAPSHOT
// Fill in a CoinSnapshot
CoinSnapshot *
OsiSolverInterface::snapshot(bool createArrays) const
{
  CoinSnapshot * snap = new CoinSnapshot();
  // scalars
  snap->setObjSense(getObjSense());
  snap->setInfinity(getInfinity());
  snap->setObjValue(getObjValue());
  double objOffset=0.0;
  getDblParam(OsiObjOffset,objOffset);
  snap->setObjOffset(objOffset);
  snap->setDualTolerance(dblParam_[OsiDualTolerance]);
  snap->setPrimalTolerance(dblParam_[OsiPrimalTolerance]);
  snap->setIntegerTolerance(getIntegerTolerance());
  snap->setIntegerUpperBound(dblParam_[OsiDualObjectiveLimit]);
  if (createArrays) {
    snap->loadProblem(*getMatrixByCol(),getColLower(),getColUpper(),
		      getObjCoefficients(),getRowLower(),
		      getRowUpper());
    snap->createRightHandSide();
    // Solution 
    snap->setColSolution(getColSolution(),true);
    snap->setRowPrice(getRowPrice(),true);
    snap->setReducedCost(getReducedCost(),true);
    snap->setRowActivity(getRowActivity(),true);
  } else {
    snap->setNumCols(getNumCols());
    snap->setNumRows(getNumRows());
    snap->setNumElements(getNumElements());
    snap->setMatrixByCol(getMatrixByCol(),false);
    snap->setColLower(getColLower(),false);
    snap->setColUpper(getColUpper(),false);
    snap->setObjCoefficients(getObjCoefficients(),false);
    snap->setRowLower(getRowLower(),false);
    snap->setRowUpper(getRowUpper(),false);
    // Solution 
    snap->setColSolution(getColSolution(),false);
    snap->setRowPrice(getRowPrice(),false);
    snap->setReducedCost(getReducedCost(),false);
    snap->setRowActivity(getRowActivity(),false);
  }
  // If integer then create array (which snapshot has to own);
  int numberColumns = getNumCols();
  char * type = new char[numberColumns];
  int numberIntegers=0;
  for (int i=0;i<numberColumns;i++) {
    if (isInteger(i)) {
      numberIntegers++;
      if (isBinary(i))
	type[i]='B';
      else
	type[i]='I';
    } else {
      type[i]='C';
    }
  }
  if (numberIntegers) 
    snap->setColType(type,true);
  else
    snap->setNumIntegers(0);
  delete [] type;
  return snap;
}
#endif
/* Identify integer variables and create corresponding objects.
  
   Record integer variables and create an OsiSimpleInteger object for each
   one.  All existing OsiSimpleInteger objects will be destroyed.
   New 
*/
void 
OsiSolverInterface::findIntegers(bool justCount)
{
  numberIntegers_=0;
  int numberColumns = getNumCols();
  int iColumn;
  for (iColumn=0;iColumn<numberColumns;iColumn++) {
    if (isInteger(iColumn)) 
      numberIntegers_++;
  }
  if (justCount) {
    assert (!numberObjects_);
    assert (!object_);
    return;
  }
  int numberIntegers=0;
  int iObject;
  for (iObject = 0;iObject<numberObjects_;iObject++) {
    OsiSimpleInteger * obj =
      dynamic_cast <OsiSimpleInteger *>(object_[iObject]) ;
    if (obj) 
      numberIntegers++;
  }
  // if same number return
  if (numberIntegers_==numberIntegers)
    return;
  // space for integers
  int * marked = new int[numberColumns];
  for (iColumn=0;iColumn<numberColumns;iColumn++)
    marked[iColumn]=-1;
  // mark simple integers
  OsiObject ** oldObject = object_;
  int nObjects=numberObjects_;
  for (iObject = 0;iObject<nObjects;iObject++) {
    OsiSimpleInteger * obj =
      dynamic_cast <OsiSimpleInteger *>(oldObject[iObject]) ;
    if (obj) {
      iColumn = obj->columnNumber();
      assert (iColumn>=0&&iColumn<numberColumns);
      marked[iColumn]=iObject;
    }
  }
  // make a large enough array for new objects
  numberObjects_ += numberIntegers_-numberIntegers;
  if (numberObjects_)
    object_ = new OsiObject * [numberObjects_];
  else
    object_=NULL;
  /*
    Walk the variables again, filling in the indices and creating objects for
    the integer variables. Initially, the objects hold the index and upper &
    lower bounds.
  */
  numberObjects_=0;
  for (iColumn=0;iColumn<numberColumns;iColumn++) {
    if(isInteger(iColumn)) {
      iObject=marked[iColumn];
      if (iObject>=0) 
	object_[numberObjects_++] = oldObject[iObject];
      else
	object_[numberObjects_++] =
	  new OsiSimpleInteger(this,iColumn);
    }
  }
  // Now append other objects
  for (iObject = 0;iObject<nObjects;iObject++) {
    OsiSimpleInteger * obj =
      dynamic_cast <OsiSimpleInteger *>(oldObject[iObject]) ;
    if (!obj) 
      object_[numberObjects_++] = oldObject[iObject];
  }
  // Delete old array (just array)
  delete [] oldObject;
  delete [] marked;
}
/* Identify integer variables and SOS and create corresponding objects.
  
      Record integer variables and create an OsiSimpleInteger object for each
      one.  All existing OsiSimpleInteger objects will be destroyed.
      If the solver supports SOS then do the same for SOS.

      If justCount then no objects created and we just store numberIntegers_
      Returns number of SOS
*/
int 
OsiSolverInterface::findIntegersAndSOS(bool justCount)
{
  findIntegers(justCount);
  return 0;
}
// Delete all object information
void 
OsiSolverInterface::deleteObjects()
{
  for (int i=0;i<numberObjects_;i++) 
    delete object_[i];
  delete [] object_;
  object_=NULL;
  numberObjects_=0;
}

/* Add in object information.
  
   Objects are cloned; the owner can delete the originals.
*/
void 
OsiSolverInterface::addObjects(int numberObjects, OsiObject ** objects)
{
  // Create integers if first time
  if (!numberObjects_)
    findIntegers(false);
  /* But if incoming objects inherit from simple integer we just want
     to replace */
  int numberColumns = getNumCols();
  /** mark is -1 if not integer, >=0 if using existing simple integer and
      >=numberColumns if using new integer */
  int * mark = new int[numberColumns];
  int i;
  for (i=0;i<numberColumns;i++)
    mark[i]=-1;
  int newNumberObjects = numberObjects;
  int newIntegers=0;
  for (i=0;i<numberObjects;i++) { 
    OsiSimpleInteger * obj =
      dynamic_cast <OsiSimpleInteger *>(objects[i]) ;
    if (obj) {
      int iColumn = obj->columnNumber();
      mark[iColumn]=i+numberColumns;
      newIntegers++;
    }
  }
  // and existing
  for (i=0;i<numberObjects_;i++) { 
    OsiSimpleInteger * obj =
      dynamic_cast <OsiSimpleInteger *>(object_[i]) ;
    if (obj) {
      int iColumn = obj->columnNumber();
      if (mark[iColumn]<0) {
        newIntegers++;
        newNumberObjects++;
        mark[iColumn]=i;
      } else {
	// But delete existing
	delete object_[i];
	object_[i]=NULL;
      }
    } else {
      newNumberObjects++;
    }
  } 
  numberIntegers_ = newIntegers;
  OsiObject ** temp  = new OsiObject * [newNumberObjects];
  // Put integers first
  newIntegers=0;
  numberIntegers_=0;
  for (i=0;i<numberColumns;i++) {
    int which = mark[i];
    if (which>=0) {
      if (!isInteger(i)) {
        newIntegers++;
        setInteger(i);
      }
      if (which<numberColumns) {
        temp[numberIntegers_]=object_[which];
      } else {
        temp[numberIntegers_]=objects[which-numberColumns]->clone();
      }
      numberIntegers_++;
    }
  }
  int n=numberIntegers_;
  // Now rest of old
  for (i=0;i<numberObjects_;i++) { 
    if (object_[i]) {
      OsiSimpleInteger * obj =
        dynamic_cast <OsiSimpleInteger *>(object_[i]) ;
      if (!obj) {
        temp[n++]=object_[i];
      }
    }
  }
  // and rest of new
  for (i=0;i<numberObjects;i++) { 
    OsiSimpleInteger * obj =
      dynamic_cast <OsiSimpleInteger *>(objects[i]) ;
    if (!obj) {
      temp[n++]=objects[i]->clone();
    }
  }
  delete [] mark;
  delete [] object_;
  object_ = temp;
  numberObjects_ = newNumberObjects;
}
// Deletes branching information before columns deleted
void 
OsiSolverInterface::deleteBranchingInfo(int numberDeleted, const int * which)
{
  if (numberObjects_) {
    int numberColumns = getNumCols();
    // mark is -1 if deleted and new number if not deleted 
    int * mark = new int[numberColumns];
    int i;
    int iColumn;
    for (i=0;i<numberColumns;i++)
      mark[i]=0;
    for (i=0;i<numberDeleted;i++) {
      iColumn = which[i];
      if (iColumn>=0&&iColumn<numberColumns)
	mark[iColumn]=-1;
    }
    iColumn = 0;
    for (i=0;i<numberColumns;i++) {
      if (mark[i]>=0) {
	mark[i]=iColumn;
	iColumn++;
      }
    }
    int oldNumberObjects = numberObjects_;
    numberIntegers_=0;
    numberObjects_=0;
    for (i=0;i<oldNumberObjects;i++) { 
      OsiSimpleInteger * obj =
	dynamic_cast <OsiSimpleInteger *>(object_[i]) ;
      if (obj) {
	iColumn = obj->columnNumber();
	int jColumn = mark[iColumn];
	if (jColumn>=0) {
	  obj->setColumnNumber(jColumn);
	  object_[numberObjects_++]=obj;
	  numberIntegers_++;
	} else {
	  delete obj;
	}
      } else {
	// not integer - all I know about is SOS
	OsiSOS * obj =
	  dynamic_cast <OsiSOS *>(object_[i]) ;
	if (obj) {
	  int oldNumberMembers=obj->numberMembers();
	  int numberMembers=0;
	  double * weight = obj->mutableWeights();
	  int * members = obj->mutableMembers();
	  for (int k=0;k<oldNumberMembers;k++) {
	    iColumn = members[k];
	    int jColumn = mark[iColumn];
	    if (jColumn>=0) {
	      members[numberMembers]=jColumn;
	      weight[numberMembers++]=weight[k];
	    }
	  }
	  if (numberMembers) {
	    obj->setNumberMembers(numberMembers);
	    object_[numberObjects_++]=obj;
	  }
	}
      }
    }
    delete [] mark;
  } else {
    findIntegers(false);
  }
}
/* Use current solution to set bounds so current integer feasible solution will stay feasible.
   Only feasible bounds will be used, even if current solution outside bounds.  The amount of
   such violation will be returned (and if small can be ignored)
*/
double 
OsiSolverInterface::forceFeasible()
{
  /*
    Run through the objects and use feasibleRegion() to set variable bounds
    so as to fix the variables specified in the objects at their value in this
    solution. Since the object list contains (at least) one object for every
    integer variable, this has the effect of fixing all integer variables.
  */
  int i;
  // Can't guarantee has matrix
  const OsiBranchingInformation info(this,false,false);
  double infeasibility=0.0;
  for (i=0;i<numberObjects_;i++)
    infeasibility += object_[i]->feasibleRegion(this,&info);
  return infeasibility;
}
/* 
   For variables currently at bound, fix at bound if reduced cost >= gap
   Returns number fixed
*/
int 
OsiSolverInterface::reducedCostFix(double gap, bool justInteger)
{
  double direction = getObjSense() ;
  double tolerance;
  getDblParam(OsiPrimalTolerance,tolerance) ;
  if (gap<=0.0)
    return 0;

  const double *lower = getColLower() ;
  const double *upper = getColUpper() ;
  const double *solution = getColSolution() ;
  const double *reducedCost = getReducedCost() ;

  int numberFixed = 0 ;
  int numberColumns = getNumCols();

  for (int iColumn = 0 ; iColumn < numberColumns ; iColumn++) {
    if (isInteger(iColumn)||!justInteger) {
      double djValue = direction*reducedCost[iColumn] ;
      if (upper[iColumn]-lower[iColumn] > tolerance) {
	if (solution[iColumn] < lower[iColumn]+tolerance && djValue > gap) {
	  setColUpper(iColumn,lower[iColumn]) ;
	  numberFixed++ ; 
	} else if (solution[iColumn] > upper[iColumn]-tolerance && -djValue > gap) {
	  setColLower(iColumn,upper[iColumn]) ;
	  numberFixed++ ;
	}
      }
    }
  }
  
  return numberFixed;
}
#ifdef COIN_FACTORIZATION_INFO
// Return number of entries in L part of current factorization
CoinBigIndex 
OsiSolverInterface::getSizeL() const
{
  return -1;
}
// Return number of entries in U part of current factorization
CoinBigIndex 
OsiSolverInterface::getSizeU() const
{
  return -1;
}
#endif
#ifdef CBC_NEXT_VERSION
/*
  Solve 2**N (N==depth) problems and return solutions and bases.
  There are N branches each of which changes bounds on both sides
  as given by branch.  The user should provide an array of (empty)
  results which will be filled in.  See OsiSolveResult for more details
  (in OsiSolveBranch.?pp) but it will include a basis and primal solution.
  
  The order of results is left to right at feasible leaf nodes so first one
  is down, down, .....
  
  Returns number of feasible leaves.  Also sets number of solves done and number
  of iterations.
  
  This is provided so a solver can do faster.
  
  If forceBranch true then branch done even if satisfied
*/
int 
OsiSolverInterface::solveBranches(int depth,const OsiSolverBranch * branch,
                                  OsiSolverResult * result,
                                  int & numberSolves, int & numberIterations,
                                  bool forceBranch)
{
  int * stack = new int [depth];
  CoinWarmStart ** basis = new CoinWarmStart * [depth];
  int iDepth;
  for (iDepth=0;iDepth<depth;iDepth++) {
    stack[iDepth]=-1;
    basis[iDepth]=NULL;
  }
  //#define PRINTALL
#ifdef PRINTALL
  int seq[10];
  double val[10];
  assert (iDepth<=10);
  for (iDepth=0;iDepth<depth;iDepth++) {
    assert (branch[iDepth].starts()[4]==2);
    assert (branch[iDepth].which()[0]==branch[iDepth].which()[1]);
    assert (branch[iDepth].bounds()[0]==branch[iDepth].bounds()[1]-1.0);
    seq[iDepth]=branch[iDepth].which()[0];
    val[iDepth]=branch[iDepth].bounds()[0];
    printf("depth %d seq %d nominal value %g\n",iDepth,seq[iDepth],val[iDepth]+0.5);
  }
#endif  
  int numberColumns = getNumCols();
  double * lowerBefore = CoinCopyOfArray(getColLower(),numberColumns);
  double * upperBefore = CoinCopyOfArray(getColUpper(),numberColumns);
  iDepth=0;
  int numberFeasible=0;
  bool finished=false;
  bool backTrack=false;
  bool iterated=false;
  numberIterations=0;
  numberSolves=0;
  int nFeas=0;
  while (!finished) {
    bool feasible = true;
    if (stack[iDepth]==-1) {
      delete basis[iDepth];
      basis[iDepth]=getWarmStart();
    } else {
      setWarmStart(basis[iDepth]);
    }
    // may be a faster way
    setColLower(lowerBefore);
    setColUpper(upperBefore);
    for (int i=0;i<iDepth;i++) {
      // skip if values feasible and not forceBranch
      if (stack[i])
        branch[i].applyBounds(*this,stack[i]);
    }
    bool doBranch = true;
    if (!forceBranch&&!backTrack) {
      // see if feasible on one side
      if (!branch[iDepth].feasibleOneWay(*this)) {
        branch[iDepth].applyBounds(*this,stack[iDepth]);
      } else {
        doBranch=false;
        stack[iDepth]=0;
      }
    } else {
      branch[iDepth].applyBounds(*this,stack[iDepth]);
    }
    if (doBranch) {
      resolve();
      numberIterations += getIterationCount();
      numberSolves++;
      iterated=true;
      if (!isProvenOptimal()||isDualObjectiveLimitReached()) {
        feasible=false;
#ifdef PRINTALL
        const double * columnLower = getColLower();
        const double * columnUpper = getColUpper();
        const double * columnSolution = getColSolution();
        printf("infeas depth %d ",iDepth);
        for (int jDepth=0;jDepth<=iDepth;jDepth++) {
          int iColumn=seq[jDepth];
          printf(" (%d %g, %g, %g (nom %g))",iColumn,columnLower[iColumn],
                 columnSolution[iColumn],columnUpper[iColumn],val[jDepth]+0.5);
        }
        printf("\n");
#endif
      }
    } else {
      // must be feasible
      nFeas++;
#ifdef PRINTALL
      const double * columnLower = getColLower();
      const double * columnUpper = getColUpper();
      const double * columnSolution = getColSolution();
      printf("feas depth %d ",iDepth);
      int iColumn=seq[iDepth];
      printf(" (%d %g, %g, %g (nom %g))",iColumn,columnLower[iColumn],
             columnSolution[iColumn],columnUpper[iColumn],val[iDepth]+0.5);
      printf("\n");
#endif
    }
    backTrack=false;
    iDepth++;
    if (iDepth==depth||!feasible) {
      if (feasible&&iterated) {
        result[numberFeasible++]=OsiSolverResult(*this,lowerBefore,upperBefore);
#ifdef PRINTALL
        const double * columnLower = getColLower();
        const double * columnUpper = getColUpper();
        const double * columnSolution = getColSolution();
        printf("sol obj %g",getObjValue());
        for (int jDepth=0;jDepth<depth;jDepth++) {
          int iColumn=seq[jDepth];
          printf(" (%d %g, %g, %g (nom %g))",iColumn,columnLower[iColumn],
                 columnSolution[iColumn],columnUpper[iColumn],val[jDepth]+0.5);
        }
        printf("\n");
#endif
      }
      // on to next
      iDepth--;
      iterated=false;
      backTrack=true;
      while (stack[iDepth]>=0) {
        if (iDepth==0) {
          // finished
          finished=true;
          break;
        }
        stack[iDepth]=-1;
        iDepth--;
      }
      if (!finished) {
        stack[iDepth]=1;
      }
    }
  }
  delete [] stack;
  for (iDepth=0;iDepth<depth;iDepth++)
    delete basis[iDepth];
  delete [] basis;
  // restore bounds
  setColLower(lowerBefore);
  setColUpper(upperBefore);
  delete [] lowerBefore;
  delete [] upperBefore;
#if 0
  static int xxxxxx=0;
  static int yyyyyy=0;
  static int zzzzzz=0;
  zzzzzz += nFeas;
  for (int j=0;j<(1<<depth);j++) {
    xxxxxx++;
    if ((xxxxxx%10000)==0)
      printf("%d implicit %d feas %d sent back\n",xxxxxx,zzzzzz,yyyyyy);
  }
  for (int j=0;j<numberFeasible;j++) {
    yyyyyy++;
    if ((yyyyyy%10000)==0)
      printf("%d implicit %d feas %d sent back\n",xxxxxx,zzzzzz,yyyyyy);
  }
#endif
  return numberFeasible;
}
#endif
