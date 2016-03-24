/* $Id: CoinSnapshot.hpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinSnapshot_H
#define CoinSnapshot_H

class CoinPackedMatrix;
#include "CoinTypes.hpp"

//#############################################################################

/** NON Abstract Base Class for interfacing with cut generators or branching code or ..
    It is designed to be snapshot of a problem at a node in tree
    
  The class may or may not own the arrays - see owned_


  Querying a problem that has no data associated with it will result in
  zeros for the number of rows and columns, and NULL pointers from
  the methods that return arrays.
*/

class CoinSnapshot  {
  
public:
  
  //---------------------------------------------------------------------------
  /**@name Problem query methods
     
     The Matrix pointers may be NULL
  */
  //@{
  /// Get number of columns
  inline int getNumCols() const
  { return numCols_;}
  
  /// Get number of rows
  inline int getNumRows() const
  { return numRows_;}
  
  /// Get number of nonzero elements
  inline int getNumElements() const
  { return numElements_;}
  
  /// Get number of integer variables
  inline int getNumIntegers() const
  { return numIntegers_;}
  
  /// Get pointer to array[getNumCols()] of column lower bounds
  inline const double * getColLower() const
  { return colLower_;}
  
  /// Get pointer to array[getNumCols()] of column upper bounds
  inline const double * getColUpper() const
  { return colUpper_;}
  
  /// Get pointer to array[getNumRows()] of row lower bounds
  inline const double * getRowLower() const
  { return rowLower_;}
  
  /// Get pointer to array[getNumRows()] of row upper bounds
  inline const double * getRowUpper() const
  { return rowUpper_;}
  
  /** Get pointer to array[getNumRows()] of row right-hand sides
      This gives same results as OsiSolverInterface for useful cases
      If getRowUpper()[i] != infinity then
        getRightHandSide()[i] == getRowUpper()[i]
      else
        getRightHandSide()[i] == getRowLower()[i]
  */
  inline const double * getRightHandSide() const
  { return rightHandSide_;}

  /// Get pointer to array[getNumCols()] of objective function coefficients
  inline const double * getObjCoefficients() const
  { return objCoefficients_;}
  
  /// Get objective function sense (1 for min (default), -1 for max)
  inline double getObjSense() const
  { return objSense_;}
  
  /// Return true if variable is continuous
  inline bool isContinuous(int colIndex) const
  { return colType_[colIndex]=='C';}
  
  /// Return true if variable is binary
  inline bool isBinary(int colIndex) const
  { return colType_[colIndex]=='B';}
  
  /// Return true if column is integer.
  inline bool isInteger(int colIndex) const
  { return colType_[colIndex]=='B'||colType_[colIndex]=='I';}
  
  /// Return true if variable is general integer
  inline bool isIntegerNonBinary(int colIndex) const
  { return colType_[colIndex]=='I';}
  
  /// Return true if variable is binary and not fixed at either bound
  inline bool isFreeBinary(int colIndex) const
  { return colType_[colIndex]=='B'&&colUpper_[colIndex]>colLower_[colIndex];}

  /// Get colType array ('B', 'I', or 'C' for Binary, Integer and Continuous) 
  inline const char * getColType() const
  { return colType_;}
  
  /// Get pointer to row-wise copy of current matrix
  inline const CoinPackedMatrix * getMatrixByRow() const
  { return matrixByRow_;}
  
  /// Get pointer to column-wise copy of current matrix
  inline const CoinPackedMatrix * getMatrixByCol() const
  { return matrixByCol_;}
  
  /// Get pointer to row-wise copy of "original" matrix
  inline const CoinPackedMatrix * getOriginalMatrixByRow() const
  { return originalMatrixByRow_;}
  
  /// Get pointer to column-wise copy of "original" matrix
  inline const CoinPackedMatrix * getOriginalMatrixByCol() const
  { return originalMatrixByCol_;}
  //@}
  
  /**@name Solution query methods */
  //@{
  /// Get pointer to array[getNumCols()] of primal variable values
  inline const double * getColSolution() const
  { return colSolution_;}
  
  /// Get pointer to array[getNumRows()] of dual variable values
  inline const double * getRowPrice() const
  { return rowPrice_;}
  
  /// Get a pointer to array[getNumCols()] of reduced costs
  inline const double * getReducedCost() const
  { return reducedCost_;}
  
  /// Get pointer to array[getNumRows()] of row activity levels (constraint matrix times the solution vector). 
  inline const double * getRowActivity() const
  { return rowActivity_;}
  
  /// Get pointer to array[getNumCols()] of primal variable values which should not be separated (for debug)
  inline const double * getDoNotSeparateThis() const
  { return doNotSeparateThis_;}
  //@}
  
  /**@name Other scalar get methods */
  //@{
  /// Get solver's value for infinity
  inline double getInfinity() const
  { return infinity_;}
  
  /** Get objective function value - includinbg any offset i.e.
      sum c sub j * x subj - objValue = objOffset */
  inline double getObjValue() const
  { return objValue_;}

  /// Get objective offset i.e. sum c sub j * x subj -objValue = objOffset 
  inline double getObjOffset() const
  { return objOffset_;}

  /// Get dual tolerance
  inline double getDualTolerance() const
  { return dualTolerance_;}

  /// Get primal tolerance
  inline double getPrimalTolerance() const
  { return primalTolerance_;}

  /// Get integer tolerance
  inline double getIntegerTolerance() const
  { return integerTolerance_;}

  /// Get integer upper bound i.e. best solution * getObjSense
  inline double getIntegerUpperBound() const
  { return integerUpperBound_;}

  /// Get integer lower bound i.e. best possible solution * getObjSense
  inline double getIntegerLowerBound() const
  { return integerLowerBound_;}
  //@}
  
  //---------------------------------------------------------------------------
  
  /**@name Method to input a problem */
  //@{
  /** Load in an problem by copying the arguments (the constraints on the
      rows are given by lower and upper bounds). If a pointer is NULL then the
      following values are the default:
      <ul>
      <li> <code>colub</code>: all columns have upper bound infinity
      <li> <code>collb</code>: all columns have lower bound 0 
      <li> <code>rowub</code>: all rows have upper bound infinity
      <li> <code>rowlb</code>: all rows have lower bound -infinity
      <li> <code>obj</code>: all variables have 0 objective coefficient
      </ul>
      All solution type arrays will be deleted
  */
  void loadProblem(const CoinPackedMatrix& matrix,
		   const double* collb, const double* colub,   
		   const double* obj,
		   const double* rowlb, const double* rowub,
		   bool makeRowCopy=false);
  
  //@}
  
  //---------------------------------------------------------------------------
  
  /**@name Methods to set data */
  //@{
  /// Set number of columns
  inline void setNumCols(int value)
  { numCols_ = value;}
  
  /// Set number of rows
  inline void setNumRows(int value)
  { numRows_ = value;}
  
  /// Set number of nonzero elements
  inline void setNumElements(int value)
  { numElements_ = value;}
  
  /// Set number of integer variables
  inline void setNumIntegers(int value)
  { numIntegers_ = value;}
  
  /// Set pointer to array[getNumCols()] of column lower bounds
  void setColLower(const double * array, bool copyIn=true);
  
  /// Set pointer to array[getNumCols()] of column upper bounds
  void setColUpper(const double * array, bool copyIn=true);
  
  /// Set pointer to array[getNumRows()] of row lower bounds
  void setRowLower(const double * array, bool copyIn=true);
  
  /// Set pointer to array[getNumRows()] of row upper bounds
  void setRowUpper(const double * array, bool copyIn=true);
  
  /** Set pointer to array[getNumRows()] of row right-hand sides
      This gives same results as OsiSolverInterface for useful cases
      If getRowUpper()[i] != infinity then
        getRightHandSide()[i] == getRowUpper()[i]
      else
        getRightHandSide()[i] == getRowLower()[i]
  */
  void setRightHandSide(const double * array, bool copyIn=true);

  /** Create array[getNumRows()] of row right-hand sides
      using existing information
      This gives same results as OsiSolverInterface for useful cases
      If getRowUpper()[i] != infinity then
        getRightHandSide()[i] == getRowUpper()[i]
      else
        getRightHandSide()[i] == getRowLower()[i]
  */
  void createRightHandSide();

  /// Set pointer to array[getNumCols()] of objective function coefficients
  void setObjCoefficients(const double * array, bool copyIn=true);
  
  /// Set objective function sense (1 for min (default), -1 for max)
  inline void setObjSense(double value)
  { objSense_ = value;}
  
  /// Set colType array ('B', 'I', or 'C' for Binary, Integer and Continuous) 
  void setColType(const char *array, bool copyIn=true);
  
  /// Set pointer to row-wise copy of current matrix
  void setMatrixByRow(const CoinPackedMatrix * matrix, bool copyIn=true);
  
  /// Create row-wise copy from MatrixByCol
  void createMatrixByRow();
  
  /// Set pointer to column-wise copy of current matrix
  void setMatrixByCol(const CoinPackedMatrix * matrix, bool copyIn=true);
  
  /// Set pointer to row-wise copy of "original" matrix
  void setOriginalMatrixByRow(const CoinPackedMatrix * matrix, bool copyIn=true);
  
  /// Set pointer to column-wise copy of "original" matrix
  void setOriginalMatrixByCol(const CoinPackedMatrix * matrix, bool copyIn=true);

  /// Set pointer to array[getNumCols()] of primal variable values
  void setColSolution(const double * array, bool copyIn=true);
  
  /// Set pointer to array[getNumRows()] of dual variable values
  void setRowPrice(const double * array, bool copyIn=true);
  
  /// Set a pointer to array[getNumCols()] of reduced costs
  void setReducedCost(const double * array, bool copyIn=true);
  
  /// Set pointer to array[getNumRows()] of row activity levels (constraint matrix times the solution vector). 
  void setRowActivity(const double * array, bool copyIn=true);
  
  /// Set pointer to array[getNumCols()] of primal variable values which should not be separated (for debug)
  void setDoNotSeparateThis(const double * array, bool copyIn=true);

  /// Set solver's value for infinity
  inline void setInfinity(double value)
  { infinity_ = value;}
  
  /// Set objective function value (including any rhs offset)
  inline void setObjValue(double value)
  { objValue_ = value;}

  /// Set objective offset i.e. sum c sub j * x subj -objValue = objOffset 
  inline void setObjOffset(double value)
  { objOffset_ = value;}

  /// Set dual tolerance
  inline void setDualTolerance(double value)
  { dualTolerance_ = value;}

  /// Set primal tolerance
  inline void setPrimalTolerance(double value)
  { primalTolerance_ = value;}

  /// Set integer tolerance
  inline void setIntegerTolerance(double value)
  { integerTolerance_ = value;}

  /// Set integer upper bound i.e. best solution * getObjSense
  inline void setIntegerUpperBound(double value)
  { integerUpperBound_ = value;}

  /// Set integer lower bound i.e. best possible solution * getObjSense
  inline void setIntegerLowerBound(double value)
  { integerLowerBound_ = value;}
  //@}
  
  //---------------------------------------------------------------------------
  
  ///@name Constructors and destructors
  //@{
  /// Default Constructor
  CoinSnapshot(); 
    
  /// Copy constructor 
  CoinSnapshot(const CoinSnapshot &);
  
  /// Assignment operator 
  CoinSnapshot & operator=(const CoinSnapshot& rhs);
  
  /// Destructor 
  virtual ~CoinSnapshot ();
  
  //@}

private:
  ///@name private functions
  //@{
  /** Does main work of destructor - type (or'ed)
      1 - NULLify pointers
      2 - delete pointers
      4 - initialize scalars (tolerances etc)
      8 - initialize scalars (objValue etc0
  */
  void gutsOfDestructor(int type);
  /// Does main work of copy
  void gutsOfCopy(const CoinSnapshot & rhs);
  //@}

  ///@name Private member data 

  /// objective function sense (1 for min (default), -1 for max)
  double objSense_;
  
  /// solver's value for infinity
  double infinity_;
  
  /// objective function value (including any rhs offset)
  double objValue_;

  /// objective offset i.e. sum c sub j * x subj -objValue = objOffset 
  double objOffset_;

  /// dual tolerance
  double dualTolerance_;

  /// primal tolerance
  double primalTolerance_;

  /// integer tolerance
  double integerTolerance_;

  /// integer upper bound i.e. best solution * getObjSense
  double integerUpperBound_;

  /// integer lower bound i.e. best possible solution * getObjSense
  double integerLowerBound_;

  /// pointer to array[getNumCols()] of column lower bounds
  const double * colLower_;
  
  /// pointer to array[getNumCols()] of column upper bounds
  const double * colUpper_;
  
  /// pointer to array[getNumRows()] of row lower bounds
  const double * rowLower_;
  
  /// pointer to array[getNumRows()] of row upper bounds
  const double * rowUpper_;
  
  /// pointer to array[getNumRows()] of rhs side values
  const double * rightHandSide_;
  
  /// pointer to array[getNumCols()] of objective function coefficients
  const double * objCoefficients_;
  
  /// colType array ('B', 'I', or 'C' for Binary, Integer and Continuous) 
  const char * colType_;
  
  /// pointer to row-wise copy of current matrix
  const CoinPackedMatrix * matrixByRow_;
  
  /// pointer to column-wise copy of current matrix
  const CoinPackedMatrix * matrixByCol_;
  
  /// pointer to row-wise copy of "original" matrix
  const CoinPackedMatrix * originalMatrixByRow_;
  
  /// pointer to column-wise copy of "original" matrix
  const CoinPackedMatrix * originalMatrixByCol_;

  /// pointer to array[getNumCols()] of primal variable values
  const double * colSolution_;
  
  /// pointer to array[getNumRows()] of dual variable values
  const double * rowPrice_;
  
  /// a pointer to array[getNumCols()] of reduced costs
  const double * reducedCost_;
  
  /// pointer to array[getNumRows()] of row activity levels (constraint matrix times the solution vector). 
  const double * rowActivity_;
  
  /// pointer to array[getNumCols()] of primal variable values which should not be separated (for debug)
  const double * doNotSeparateThis_;

  /// number of columns
  int numCols_;
  
  /// number of rows
  int numRows_;
  
  /// number of nonzero elements
  int numElements_;
  
  /// number of integer variables
  int numIntegers_;

  /// To say whether arrays etc are owned by CoinSnapshot
    typedef struct {
      unsigned int colLower:1;
      unsigned int colUpper:1;
      unsigned int rowLower:1;
      unsigned int rowUpper:1;
      unsigned int rightHandSide:1;
      unsigned int objCoefficients:1;
      unsigned int colType:1;
      unsigned int matrixByRow:1;
      unsigned int matrixByCol:1;
      unsigned int originalMatrixByRow:1;
      unsigned int originalMatrixByCol:1;
      unsigned int colSolution:1;
      unsigned int rowPrice:1;
      unsigned int reducedCost:1;
      unsigned int rowActivity:1;
      unsigned int doNotSeparateThis:1;
  } coinOwned;
  coinOwned owned_;
  //@}
};  
#endif
