/* $Id: CoinModel.hpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinModel_H
#define CoinModel_H

#include "CoinModelUseful.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinFinite.hpp"
class CoinBaseModel {

public:


  /**@name Constructors, destructor */
   //@{
  /// Default Constructor 
  CoinBaseModel ();

  /// Copy constructor 
  CoinBaseModel ( const CoinBaseModel &rhs);
   
  /// Assignment operator 
  CoinBaseModel & operator=( const CoinBaseModel& rhs);

  /// Clone
  virtual CoinBaseModel * clone() const=0;

  /// Destructor 
  virtual ~CoinBaseModel () ;
   //@}

  /**@name For getting information */
   //@{
   /// Return number of rows
  inline int numberRows() const
  { return numberRows_;}
   /// Return number of columns
  inline int numberColumns() const
  { return numberColumns_;}
   /// Return number of elements
  virtual CoinBigIndex numberElements() const = 0;
  /** Returns the (constant) objective offset
      This is the RHS entry for the objective row
  */
  inline double objectiveOffset() const
  { return objectiveOffset_;}
  /// Set objective offset
  inline void setObjectiveOffset(double value)
  { objectiveOffset_=value;}
  /// Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  inline double optimizationDirection() const {
    return  optimizationDirection_;
  }
  /// Set direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  inline void setOptimizationDirection(double value)
  { optimizationDirection_=value;}
  /// Get print level 0 - off, 1 - errors, 2 - more
  inline int logLevel() const
  { return logLevel_;}
  /// Set print level 0 - off, 1 - errors, 2 - more
  void setLogLevel(int value);
  /// Return the problem name
  inline const char * getProblemName() const
  { return problemName_.c_str();}
  /// Set problem name
  void setProblemName(const char *name) ;
  /// Set problem name
  void setProblemName(const std::string &name) ;
  /// Return the row block name
  inline const std::string & getRowBlock() const
  { return rowBlockName_;}
  /// Set row block name
  inline void setRowBlock(const std::string &name) 
  { rowBlockName_ = name;}
  /// Return the column block name
  inline const std::string & getColumnBlock() const
  { return columnBlockName_;}
  /// Set column block name
  inline void setColumnBlock(const std::string &name) 
  { columnBlockName_ = name;}
   //@}
  
protected:
  /**@name Data members */
   //@{
  /// Current number of rows
  int numberRows_;
  /// Current number of columns
  int numberColumns_;
  /// Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  double optimizationDirection_;
  /// Objective offset to be passed on
  double objectiveOffset_;
  /// Problem name
  std::string problemName_;
  /// Rowblock name
  std::string rowBlockName_;
  /// Columnblock name
  std::string columnBlockName_;
  /** Print level.
      I could have gone for full message handling but this should normally
      be silent and lightweight.  I can always change.
      0 - no output
      1 - on errors
      2 - more detailed
  */
  int logLevel_;
   //@}
  /// data

};

/** 
    This is a simple minded model which is stored in a format which makes
    it easier to construct and modify but not efficient for algorithms.  It has
    to be passed across to ClpModel or OsiSolverInterface by addRows, addCol(umn)s
    or loadProblem.

    It may have up to four parts -
    1) A matrix of doubles (or strings - see note A)
    2) Column information including integer information and names
    3) Row information including names
    4) Quadratic objective (not implemented - but see A)

    This class is meant to make it more efficient to build a model.  It is at
    its most efficient when all additions are done as addRow or as addCol but
    not mixed.  If only 1 and 2 exist then solver.addColumns may be used to pass to solver,
    if only 1 and 3 exist then solver.addRows may be used.  Otherwise solver.loadProblem
    must be used.

    If addRows and addColumns are mixed or if individual elements are set then the
    speed will drop to some extent and more memory will be used.

    It is also possible to iterate over existing elements and to access columns and rows
    by name.  Again each of these use memory and cpu time.  However memory is unlikely
    to be critical as most algorithms will use much more.

    Notes:
    A)  Although this could be used to pass nonlinear information around the
        only use at present is to have named values e.g. value1 which can then be
        set to a value after model is created.  I have no idea whether that could
        be useful but I thought it might be fun.
	Quadratic terms are allowed in strings!  A solver could try and use this
	if so - the convention is that 0.5* quadratic is stored
	
    B)  This class could be useful for modeling.
*/

class CoinModel : public CoinBaseModel {
  
public:
  /**@name Useful methods for building model */
   //@{
   /** add a row -  numberInRow may be zero */
   void addRow(int numberInRow, const int * columns,
	       const double * elements, double rowLower=-COIN_DBL_MAX, 
              double rowUpper=COIN_DBL_MAX, const char * name=NULL);
  /// add a column - numberInColumn may be zero */
   void addColumn(int numberInColumn, const int * rows,
                  const double * elements, 
                  double columnLower=0.0, 
                  double columnUpper=COIN_DBL_MAX, double objectiveValue=0.0,
                  const char * name=NULL, bool isInteger=false);
  /// add a column - numberInColumn may be zero */
  inline void addCol(int numberInColumn, const int * rows,
                     const double * elements, 
                     double columnLower=0.0, 
                     double columnUpper=COIN_DBL_MAX, double objectiveValue=0.0,
                     const char * name=NULL, bool isInteger=false)
  { addColumn(numberInColumn, rows, elements, columnLower, columnUpper, objectiveValue,
              name,isInteger);}
  /// Sets value for row i and column j 
  inline void operator() (int i,int j,double value) 
  { setElement(i,j,value);}
  /// Sets value for row i and column j 
  void setElement(int i,int j,double value) ;
  /** Gets sorted row - user must provide enough space 
      (easiest is allocate number of columns).
      If column or element NULL then just returns number
      Returns number of elements
  */
  int getRow(int whichRow, int * column, double * element);
  /** Gets sorted column - user must provide enough space 
      (easiest is allocate number of rows).
      If row or element NULL then just returns number
      Returns number of elements
  */
  int getColumn(int whichColumn, int * column, double * element);
  /// Sets quadratic value for column i and j 
  void setQuadraticElement(int i,int j,double value) ;
  /// Sets value for row i and column j as string
  inline void operator() (int i,int j,const char * value) 
  { setElement(i,j,value);}
  /// Sets value for row i and column j as string
  void setElement(int i,int j,const char * value) ;
  /// Associates a string with a value.  Returns string id (or -1 if does not exist)
  int associateElement(const char * stringValue, double value);
  /** Sets rowLower (if row does not exist then
      all rows up to this are defined with default values and no elements)
  */
  void setRowLower(int whichRow,double rowLower); 
  /** Sets rowUpper (if row does not exist then
      all rows up to this are defined with default values and no elements)
  */
  void setRowUpper(int whichRow,double rowUpper); 
  /** Sets rowLower and rowUpper (if row does not exist then
      all rows up to this are defined with default values and no elements)
  */
  void setRowBounds(int whichRow,double rowLower,double rowUpper); 
  /** Sets name (if row does not exist then
      all rows up to this are defined with default values and no elements)
  */
  void setRowName(int whichRow,const char * rowName); 
  /** Sets columnLower (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnLower(int whichColumn,double columnLower); 
  /** Sets columnUpper (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnUpper(int whichColumn,double columnUpper); 
  /** Sets columnLower and columnUpper (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnBounds(int whichColumn,double columnLower,double columnUpper); 
  /** Sets columnObjective (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnObjective(int whichColumn,double columnObjective); 
  /** Sets name (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnName(int whichColumn,const char * columnName); 
  /** Sets integer state (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnIsInteger(int whichColumn,bool columnIsInteger); 
  /** Sets columnObjective (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setObjective(int whichColumn,double columnObjective) 
  { setColumnObjective( whichColumn, columnObjective);} 
  /** Sets integer state (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setIsInteger(int whichColumn,bool columnIsInteger) 
  { setColumnIsInteger( whichColumn, columnIsInteger);} 
  /** Sets integer (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setInteger(int whichColumn) 
  { setColumnIsInteger( whichColumn, true);} 
  /** Sets continuous (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setContinuous(int whichColumn) 
  { setColumnIsInteger( whichColumn, false);} 
  /** Sets columnLower (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setColLower(int whichColumn,double columnLower) 
  { setColumnLower( whichColumn, columnLower);} 
  /** Sets columnUpper (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setColUpper(int whichColumn,double columnUpper) 
  { setColumnUpper( whichColumn, columnUpper);} 
  /** Sets columnLower and columnUpper (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setColBounds(int whichColumn,double columnLower,double columnUpper) 
  { setColumnBounds( whichColumn, columnLower, columnUpper);} 
  /** Sets columnObjective (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setColObjective(int whichColumn,double columnObjective) 
  { setColumnObjective( whichColumn, columnObjective);} 
  /** Sets name (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setColName(int whichColumn,const char * columnName) 
  { setColumnName( whichColumn, columnName);} 
  /** Sets integer (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setColIsInteger(int whichColumn,bool columnIsInteger) 
  { setColumnIsInteger( whichColumn, columnIsInteger);} 
  /** Sets rowLower (if row does not exist then
      all rows up to this are defined with default values and no elements)
  */
  void setRowLower(int whichRow,const char * rowLower); 
  /** Sets rowUpper (if row does not exist then
      all rows up to this are defined with default values and no elements)
  */
  void setRowUpper(int whichRow,const char * rowUpper); 
  /** Sets columnLower (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnLower(int whichColumn,const char * columnLower); 
  /** Sets columnUpper (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnUpper(int whichColumn,const char * columnUpper); 
  /** Sets columnObjective (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnObjective(int whichColumn,const char * columnObjective); 
  /** Sets integer (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  void setColumnIsInteger(int whichColumn,const char * columnIsInteger); 
  /** Sets columnObjective (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setObjective(int whichColumn,const char * columnObjective) 
  { setColumnObjective( whichColumn, columnObjective);} 
  /** Sets integer (if column does not exist then
      all columns up to this are defined with default values and no elements)
  */
  inline void setIsInteger(int whichColumn,const char * columnIsInteger) 
  { setColumnIsInteger( whichColumn, columnIsInteger);} 
  /** Deletes all entries in row and bounds.  Will be ignored by
      writeMps etc and will be packed down if asked for. */
  void deleteRow(int whichRow);
  /** Deletes all entries in column and bounds and objective.  Will be ignored by
      writeMps etc and will be packed down if asked for. */
  void deleteColumn(int whichColumn);
  /** Deletes all entries in column and bounds.  If last column the number of columns
      will be decremented and true returned.  */
  inline void deleteCol(int whichColumn)
  { deleteColumn(whichColumn);}
  /// Takes element out of matrix - returning position (<0 if not there);
  int deleteElement(int row, int column);
  /// Takes element out of matrix when position known
  void deleteThisElement(int row, int column,int position);
  /** Packs down all rows i.e. removes empty rows permanently.  Empty rows
      have no elements and feasible bounds. returns number of rows deleted. */
  int packRows();
  /** Packs down all columns i.e. removes empty columns permanently.  Empty columns
      have no elements and no objective. returns number of columns deleted. */
  int packColumns();
  /** Packs down all columns i.e. removes empty columns permanently.  Empty columns
      have no elements and no objective. returns number of columns deleted. */
  inline int packCols()
  { return packColumns();}
  /** Packs down all rows and columns.  i.e. removes empty rows and columns permanently.
      Empty rows have no elements and feasible bounds.
      Empty columns have no elements and no objective.
      returns number of rows+columns deleted. */
  int pack();

  /** Sets columnObjective array
  */
  void setObjective(int numberColumns,const double * objective) ;
  /** Sets columnLower array
  */
  void setColumnLower(int numberColumns,const double * columnLower);
  /** Sets columnLower array
  */
  inline void setColLower(int numberColumns,const double * columnLower)
  { setColumnLower( numberColumns, columnLower);} 
  /** Sets columnUpper array
  */
  void setColumnUpper(int numberColumns,const double * columnUpper);
  /** Sets columnUpper array
  */
  inline void setColUpper(int numberColumns,const double * columnUpper)
  { setColumnUpper( numberColumns, columnUpper);} 
  /** Sets rowLower array
  */
  void setRowLower(int numberRows,const double * rowLower);
  /** Sets rowUpper array
  */
  void setRowUpper(int numberRows,const double * rowUpper);

  /** Write the problem in MPS format to a file with the given filename.
      
  \param compression can be set to three values to indicate what kind
  of file should be written
  <ul>
  <li> 0: plain text (default)
  <li> 1: gzip compressed (.gz is appended to \c filename)
  <li> 2: bzip2 compressed (.bz2 is appended to \c filename) (TODO)
  </ul>
  If the library was not compiled with the requested compression then
  writeMps falls back to writing a plain text file.
  
  \param formatType specifies the precision to used for values in the
  MPS file
  <ul>
  <li> 0: normal precision (default)
  <li> 1: extra accuracy
  <li> 2: IEEE hex
  </ul>
  
  \param numberAcross specifies whether 1 or 2 (default) values should be
  specified on every data line in the MPS file.
  
  not const as may change model e.g. fill in default bounds
  */
  int writeMps(const char *filename, int compression = 0,
               int formatType = 0, int numberAcross = 2, bool keepStrings=false) ;
  
  /** Check two models against each other.  Return nonzero if different.
      Ignore names if that set.
      May modify both models by cleaning up
  */
  int differentModel(CoinModel & other, bool ignoreNames);
   //@}


  /**@name For structured models */
   //@{
  /// Pass in CoinPackedMatrix (and switch off element updates)
  void passInMatrix(const CoinPackedMatrix & matrix);
  /** Convert elements to CoinPackedMatrix (and switch off element updates).
      Returns number of errors */
  int convertMatrix();
  /// Return a pointer to CoinPackedMatrix (or NULL)
  inline const CoinPackedMatrix * packedMatrix() const
  { return packedMatrix_;}
  /// Return pointers to original rows (for decomposition)
  inline const int * originalRows() const
  { return rowType_;}
  /// Return pointers to original columns (for decomposition)
  inline const int * originalColumns() const
  { return columnType_;}
   //@}


  /**@name For getting information */
   //@{
   /// Return number of elements
  inline CoinBigIndex numberElements() const
  { return numberElements_;}
   /// Return  elements as triples
  inline const CoinModelTriple * elements() const
  { return elements_;}
  /// Returns value for row i and column j
  inline double operator() (int i,int j) const
  { return getElement(i,j);}
  /// Returns value for row i and column j
  double getElement(int i,int j) const;
  /// Returns value for row rowName and column columnName
  inline double operator() (const char * rowName,const char * columnName) const
  { return getElement(rowName,columnName);}
  /// Returns value for row rowName and column columnName
  double getElement(const char * rowName,const char * columnName) const;
  /// Returns quadratic value for columns i and j
  double getQuadraticElement(int i,int j) const;
  /** Returns value for row i and column j as string.
      Returns NULL if does not exist.
      Returns "Numeric" if not a string
  */
  const char * getElementAsString(int i,int j) const;
  /** Returns pointer to element for row i column j.
      Only valid until next modification. 
      NULL if element does not exist */
  double * pointer (int i,int j) const;
  /** Returns position in elements for row i column j.
      Only valid until next modification. 
      -1 if element does not exist */
  int position (int i,int j) const;
  
  
  /** Returns first element in given row - index is -1 if none.
      Index is given by .index and value by .value
  */
  CoinModelLink firstInRow(int whichRow) const ;
  /** Returns last element in given row - index is -1 if none.
      Index is given by .index and value by .value
  */
  CoinModelLink lastInRow(int whichRow) const ;
  /** Returns first element in given column - index is -1 if none.
      Index is given by .index and value by .value
  */
  CoinModelLink firstInColumn(int whichColumn) const ;
  /** Returns last element in given column - index is -1 if none.
      Index is given by .index and value by .value
  */
  CoinModelLink lastInColumn(int whichColumn) const ;
  /** Returns next element in current row or column - index is -1 if none.
      Index is given by .index and value by .value.
      User could also tell because input.next would be NULL
  */
  CoinModelLink next(CoinModelLink & current) const ;
  /** Returns previous element in current row or column - index is -1 if none.
      Index is given by .index and value by .value.
      User could also tell because input.previous would be NULL
      May not be correct if matrix updated.
  */
  CoinModelLink previous(CoinModelLink & current) const ;
  /** Returns first element in given quadratic column - index is -1 if none.
      Index is given by .index and value by .value
      May not be correct if matrix updated.
  */
  CoinModelLink firstInQuadraticColumn(int whichColumn) const ;
  /** Returns last element in given quadratic column - index is -1 if none.
      Index is given by .index and value by .value
  */
  CoinModelLink lastInQuadraticColumn(int whichColumn) const ;
  /** Gets rowLower (if row does not exist then -COIN_DBL_MAX)
  */
  double  getRowLower(int whichRow) const ; 
  /** Gets rowUpper (if row does not exist then +COIN_DBL_MAX)
  */
  double  getRowUpper(int whichRow) const ; 
  /** Gets name (if row does not exist then NULL)
  */
  const char * getRowName(int whichRow) const ; 
  inline double  rowLower(int whichRow) const
  { return getRowLower(whichRow);}
  /** Gets rowUpper (if row does not exist then COIN_DBL_MAX)
  */
  inline double  rowUpper(int whichRow) const
  { return getRowUpper(whichRow) ;}
  /** Gets name (if row does not exist then NULL)
  */
  inline const char * rowName(int whichRow) const
  { return getRowName(whichRow);}
  /** Gets columnLower (if column does not exist then 0.0)
  */
  double  getColumnLower(int whichColumn) const ; 
  /** Gets columnUpper (if column does not exist then COIN_DBL_MAX)
  */
  double  getColumnUpper(int whichColumn) const ; 
  /** Gets columnObjective (if column does not exist then 0.0)
  */
  double  getColumnObjective(int whichColumn) const ; 
  /** Gets name (if column does not exist then NULL)
  */
  const char * getColumnName(int whichColumn) const ; 
  /** Gets if integer (if column does not exist then false)
  */
  bool getColumnIsInteger(int whichColumn) const ; 
  /** Gets columnLower (if column does not exist then 0.0)
  */
  inline double  columnLower(int whichColumn) const
  { return getColumnLower(whichColumn);}
  /** Gets columnUpper (if column does not exist then COIN_DBL_MAX)
  */
  inline double  columnUpper(int whichColumn) const
  { return getColumnUpper(whichColumn) ;}
  /** Gets columnObjective (if column does not exist then 0.0)
  */
  inline double  columnObjective(int whichColumn) const
  { return getColumnObjective(whichColumn);}
  /** Gets columnObjective (if column does not exist then 0.0)
  */
  inline double  objective(int whichColumn) const
  { return getColumnObjective(whichColumn);}
  /** Gets name (if column does not exist then NULL)
  */
  inline const char * columnName(int whichColumn) const
  { return getColumnName(whichColumn);}
  /** Gets if integer (if column does not exist then false)
  */
  inline bool columnIsInteger(int whichColumn) const
  { return getColumnIsInteger(whichColumn);}
  /** Gets if integer (if column does not exist then false)
  */
  inline bool isInteger(int whichColumn) const
  { return getColumnIsInteger(whichColumn);}
  /** Gets columnLower (if column does not exist then 0.0)
  */
  inline double  getColLower(int whichColumn) const
  { return getColumnLower(whichColumn);}
  /** Gets columnUpper (if column does not exist then COIN_DBL_MAX)
  */
  inline double  getColUpper(int whichColumn) const
  { return getColumnUpper(whichColumn) ;}
  /** Gets columnObjective (if column does not exist then 0.0)
  */
  inline double  getColObjective(int whichColumn) const
  { return getColumnObjective(whichColumn);}
  /** Gets name (if column does not exist then NULL)
  */
  inline const char * getColName(int whichColumn) const
  { return getColumnName(whichColumn);}
  /** Gets if integer (if column does not exist then false)
  */
  inline bool getColIsInteger(int whichColumn) const
  { return getColumnIsInteger(whichColumn);}
  /** Gets rowLower (if row does not exist then -COIN_DBL_MAX)
  */
  const char *  getRowLowerAsString(int whichRow) const ; 
  /** Gets rowUpper (if row does not exist then +COIN_DBL_MAX)
  */
  const char *  getRowUpperAsString(int whichRow) const ; 
  inline const char *  rowLowerAsString(int whichRow) const
  { return getRowLowerAsString(whichRow);}
  /** Gets rowUpper (if row does not exist then COIN_DBL_MAX)
  */
  inline const char *  rowUpperAsString(int whichRow) const
  { return getRowUpperAsString(whichRow) ;}
  /** Gets columnLower (if column does not exist then 0.0)
  */
  const char *  getColumnLowerAsString(int whichColumn) const ; 
  /** Gets columnUpper (if column does not exist then COIN_DBL_MAX)
  */
  const char *  getColumnUpperAsString(int whichColumn) const ; 
  /** Gets columnObjective (if column does not exist then 0.0)
  */
  const char *  getColumnObjectiveAsString(int whichColumn) const ; 
  /** Gets if integer (if column does not exist then false)
  */
  const char * getColumnIsIntegerAsString(int whichColumn) const ; 
  /** Gets columnLower (if column does not exist then 0.0)
  */
  inline const char *  columnLowerAsString(int whichColumn) const
  { return getColumnLowerAsString(whichColumn);}
  /** Gets columnUpper (if column does not exist then COIN_DBL_MAX)
  */
  inline const char *  columnUpperAsString(int whichColumn) const
  { return getColumnUpperAsString(whichColumn) ;}
  /** Gets columnObjective (if column does not exist then 0.0)
  */
  inline const char *  columnObjectiveAsString(int whichColumn) const
  { return getColumnObjectiveAsString(whichColumn);}
  /** Gets columnObjective (if column does not exist then 0.0)
  */
  inline const char *  objectiveAsString(int whichColumn) const
  { return getColumnObjectiveAsString(whichColumn);}
  /** Gets if integer (if column does not exist then false)
  */
  inline const char * columnIsIntegerAsString(int whichColumn) const
  { return getColumnIsIntegerAsString(whichColumn);}
  /** Gets if integer (if column does not exist then false)
  */
  inline const char * isIntegerAsString(int whichColumn) const
  { return getColumnIsIntegerAsString(whichColumn);}
  /// Row index from row name (-1 if no names or no match)
  int row(const char * rowName) const;
  /// Column index from column name (-1 if no names or no match)
  int column(const char * columnName) const;
  /// Returns type
  inline int type() const
  { return type_;}
  /// returns unset value
  inline double unsetValue() const
  { return -1.23456787654321e-97;}
  /// Creates a packed matrix - return number of errors
  int createPackedMatrix(CoinPackedMatrix & matrix, 
			 const double * associated);
  /** Fills in startPositive and startNegative with counts for +-1 matrix.
      If not +-1 then startPositive[0]==-1 otherwise counts and
      startPositive[numberColumns]== size
      - return number of errors
  */
  int countPlusMinusOne(CoinBigIndex * startPositive, CoinBigIndex * startNegative,
                        const double * associated);
  /** Creates +-1 matrix given startPositive and startNegative counts for +-1 matrix.
  */
  void createPlusMinusOne(CoinBigIndex * startPositive, CoinBigIndex * startNegative,
                         int * indices,
                         const double * associated);
  /// Creates copies of various arrays - return number of errors
  int createArrays(double * & rowLower, double * &  rowUpper,
                   double * & columnLower, double * & columnUpper,
                   double * & objective, int * & integerType,
                   double * & associated);
  /// Says if strings exist
  inline bool stringsExist() const
  { return string_.numberItems()!=0;}
  /// Return string array
  inline const CoinModelHash * stringArray() const
  { return &string_;}
  /// Returns associated array
  inline double * associatedArray() const
  { return associated_;}
  /// Return rowLower array
  inline double * rowLowerArray() const
  { return rowLower_;}
  /// Return rowUpper array
  inline double * rowUpperArray() const
  { return rowUpper_;}
  /// Return columnLower array
  inline double * columnLowerArray() const
  { return columnLower_;}
  /// Return columnUpper array
  inline double * columnUpperArray() const
  { return columnUpper_;}
  /// Return objective array
  inline double * objectiveArray() const
  { return objective_;}
  /// Return integerType array
  inline int * integerTypeArray() const
  { return integerType_;}
  /// Return row names array
  inline const CoinModelHash * rowNames() const
  { return &rowName_;}
  /// Return column names array
  inline const CoinModelHash * columnNames() const
  { return &columnName_;}
  /// Reset row names
  inline void zapRowNames()
  { rowName_=CoinModelHash();}
  /// Reset column names
  inline void zapColumnNames()
  { columnName_=CoinModelHash();}
  /// Returns array of 0 or nonzero if can be a cut (or returns NULL)
  inline const int * cutMarker() const
  { return cut_;}
  /// Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  inline double optimizationDirection() const {
    return  optimizationDirection_;
  }
  /// Set direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  inline void setOptimizationDirection(double value)
  { optimizationDirection_=value;}
  /// Return pointer to more information
  inline void * moreInfo() const
  { return moreInfo_;}
  /// Set pointer to more information
  inline void setMoreInfo(void * info)
  { moreInfo_ = info;}
  /** Returns which parts of model are set
      1 - matrix
      2 - rhs
      4 - row names
      8 - column bounds and/or objective
      16 - column names
      32 - integer types
  */
  int whatIsSet() const;
   //@}

  /**@name for block models - matrix will be CoinPackedMatrix */
   //@{
  /*! \brief Load in a problem by copying the arguments. The constraints on
    the rows are given by lower and upper bounds.
    
    If a pointer is 0 then the following values are the default:
    <ul>
    <li> <code>colub</code>: all columns have upper bound infinity
    <li> <code>collb</code>: all columns have lower bound 0 
    <li> <code>rowub</code>: all rows have upper bound infinity
    <li> <code>rowlb</code>: all rows have lower bound -infinity
    <li> <code>obj</code>: all variables have 0 objective coefficient
    </ul>
    
    Note that the default values for rowub and rowlb produce the
    constraint -infty <= ax <= infty. This is probably not what you want.
  */
  void loadBlock (const CoinPackedMatrix& matrix,
		  const double* collb, const double* colub,   
		  const double* obj,
		  const double* rowlb, const double* rowub) ;
  /*! \brief Load in a problem by copying the arguments.
    The constraints on the rows are given by sense/rhs/range triplets.
    
    If a pointer is 0 then the following values are the default:
    <ul>
    <li> <code>colub</code>: all columns have upper bound infinity
    <li> <code>collb</code>: all columns have lower bound 0 
    <li> <code>obj</code>: all variables have 0 objective coefficient
    <li> <code>rowsen</code>: all rows are >=
    <li> <code>rowrhs</code>: all right hand sides are 0
    <li> <code>rowrng</code>: 0 for the ranged rows
    </ul>
    
    Note that the default values for rowsen, rowrhs, and rowrng produce the
    constraint ax >= 0.
  */
  void loadBlock (const CoinPackedMatrix& matrix,
		  const double* collb, const double* colub,
		  const double* obj,
		  const char* rowsen, const double* rowrhs,   
		  const double* rowrng) ;
  
  /*! \brief Load in a problem by copying the arguments. The constraint
    matrix is is specified with standard column-major
    column starts / row indices / coefficients vectors. 
    The constraints on the rows are given by lower and upper bounds.
    
    The matrix vectors must be gap-free. Note that <code>start</code> must
    have <code>numcols+1</code> entries so that the length of the last column
    can be calculated as <code>start[numcols]-start[numcols-1]</code>.
    
    See the previous loadBlock method using rowlb and rowub for default
    argument values.
  */
  void loadBlock (const int numcols, const int numrows,
		  const CoinBigIndex * start, const int* index,
		  const double* value,
		  const double* collb, const double* colub,   
		  const double* obj,
		  const double* rowlb, const double* rowub) ;
  
  /*! \brief Load in a problem by copying the arguments. The constraint
    matrix is is specified with standard column-major
    column starts / row indices / coefficients vectors. 
    The constraints on the rows are given by sense/rhs/range triplets.
    
    The matrix vectors must be gap-free. Note that <code>start</code> must
    have <code>numcols+1</code> entries so that the length of the last column
    can be calculated as <code>start[numcols]-start[numcols-1]</code>.
    
    See the previous loadBlock method using sense/rhs/range for default
    argument values.
  */
  void loadBlock (const int numcols, const int numrows,
		  const CoinBigIndex * start, const int* index,
		  const double* value,
		  const double* collb, const double* colub,   
		  const double* obj,
		  const char* rowsen, const double* rowrhs,   
		  const double* rowrng) ;

   //@}

  /**@name Constructors, destructor */
   //@{
   /** Default constructor. */
   CoinModel();
   /** Constructor with sizes. */
   CoinModel(int firstRows, int firstColumns, int firstElements,bool noNames=false);
   /** Read a problem in MPS or GAMS format from the given filename.
   */
   CoinModel(const char *fileName, int allowStrings=0);
   /** Read a problem from AMPL nl file
       NOTE - as I can't work out configure etc the source code is in Cbc_ampl.cpp!
   */
   CoinModel( int nonLinear, const char * fileName,const void * info);
  /// From arrays
  CoinModel(int numberRows, int numberColumns,
	    const CoinPackedMatrix * matrix,
	    const double * rowLower, const double * rowUpper,
	    const double * columnLower, const double * columnUpper,
	    const double * objective);
  /// Clone
  virtual CoinBaseModel * clone() const;

   /** Destructor */
   virtual ~CoinModel();
   //@}

   /**@name Copy method */
   //@{
   /** The copy constructor. */
   CoinModel(const CoinModel&);
  /// =
   CoinModel& operator=(const CoinModel&);
   //@}

   /**@name For debug */
   //@{
  /// Checks that links are consistent
  void validateLinks() const;
   //@}
private:
  /// Resize
  void resize(int maximumRows, int maximumColumns, int maximumElements);
  /// Fill in default row information
  void fillRows(int which,bool forceCreation,bool fromAddRow=false);
  /// Fill in default column information
  void fillColumns(int which,bool forceCreation,bool fromAddColumn=false);
  /** Fill in default linked list information (1= row, 2 = column)
      Marked as const as list is mutable */
  void fillList(int which, CoinModelLinkedList & list,int type) const ;
  /** Create a linked list and synchronize free 
      type 1 for row 2 for column
      Marked as const as list is mutable */
  void createList(int type) const;
  /// Adds one string, returns index
  int addString(const char * string);
  /** Gets a double from a string possibly containing named strings,
      returns unset if not found
  */
  double getDoubleFromString(CoinYacc & info, const char * string);
  /// Frees value memory
  void freeStringMemory(CoinYacc & info);
public:
  /// Fills in all associated - returning number of errors
  int computeAssociated(double * associated);
  /** Gets correct form for a quadratic row - user to delete
      If row is not quadratic then returns which other variables are involved
      with tiny (1.0e-100) elements and count of total number of variables which could not
      be put in quadratic form
  */
  CoinPackedMatrix * quadraticRow(int rowNumber,double * linear,
				  int & numberBad) const;
  /// Replaces a quadratic row
  void replaceQuadraticRow(int rowNumber,const double * linear, const CoinPackedMatrix * quadraticPart);
  /** If possible return a model where if all variables marked nonzero are fixed
      the problem will be linear.  At present may only work if quadratic.
      Returns NULL if not possible
  */
  CoinModel * reorder(const char * mark) const;
  /** Expands out all possible combinations for a knapsack
      If buildObj NULL then just computes space needed - returns number elements
      On entry numberOutput is maximum allowed, on exit it is number needed or
      -1 (as will be number elements) if maximum exceeded.  numberOutput will have at
      least space to return values which reconstruct input.
      Rows returned will be original rows but no entries will be returned for
      any rows all of whose entries are in knapsack.  So up to user to allow for this.
      If reConstruct >=0 then returns number of entrie which make up item "reConstruct"
      in expanded knapsack.  Values in buildRow and buildElement;
  */
  int expandKnapsack(int knapsackRow, int & numberOutput,double * buildObj, CoinBigIndex * buildStart,
		     int * buildRow, double * buildElement,int reConstruct=-1) const;
  /// Sets cut marker array
  void setCutMarker(int size,const int * marker);
  /// Sets priority array
  void setPriorities(int size,const int * priorities);
  /// priorities (given for all columns (-1 if not integer)
  inline const int * priorities() const
  { return priority_;}
  /// For decomposition set original row and column indices
  void setOriginalIndices(const int * row, const int * column);
  
private:
  /** Read a problem from AMPL nl file
      so not constructor so gdb will work
   */
  void gdb( int nonLinear, const char * fileName, const void * info);
  /// returns jColumn (-2 if linear term, -1 if unknown) and coefficient
  int decodeBit(char * phrase, char * & nextPhrase, double & coefficient, bool ifFirst) const;
  /// Aborts with message about packedMatrix
  void badType() const;
  /**@name Data members */
   //@{
  /// Maximum number of rows
  int maximumRows_;
  /// Maximum number of columns
  int maximumColumns_;
  /// Current number of elements
  int numberElements_;
  /// Maximum number of elements
  int maximumElements_;
  /// Current number of quadratic elements
  int numberQuadraticElements_;
  /// Maximum number of quadratic elements
  int maximumQuadraticElements_;
  /// Row lower 
  double * rowLower_;
  /// Row upper 
  double * rowUpper_;
  /// Row names
  CoinModelHash rowName_;
  /** Row types.
      Has information - at present
      bit 0 - rowLower is a string
      bit 1 - rowUpper is a string
      NOTE - if converted to CoinPackedMatrix - may be indices of 
      original rows (i.e. when decomposed)
  */
  int * rowType_;
  /// Objective
  double * objective_;
  /// Column Lower
  double * columnLower_;
  /// Column Upper
  double * columnUpper_;
  /// Column names
  CoinModelHash columnName_;
  /// Integer information
  int * integerType_;
  /// Strings
  CoinModelHash string_;
  /** Column types.
      Has information - at present
      bit 0 - columnLower is a string
      bit 1 - columnUpper is a string
      bit 2 - objective is a string
      bit 3 - integer setting is a string
      NOTE - if converted to CoinPackedMatrix - may be indices of 
      original columns (i.e. when decomposed)
  */
  int * columnType_;
  /// If simple then start of each row/column
  int * start_;
  /// Actual elements
  CoinModelTriple * elements_;
  /// Actual elements as CoinPackedMatrix
  CoinPackedMatrix * packedMatrix_;
  /// Hash for elements
  mutable CoinModelHash2 hashElements_;
  /// Linked list for rows
  mutable CoinModelLinkedList rowList_;
  /// Linked list for columns
  mutable CoinModelLinkedList columnList_;
  /// Actual quadratic elements (always linked lists)
  CoinModelTriple * quadraticElements_;
  /// Hash for quadratic elements
  mutable CoinModelHash2 hashQuadraticElements_;
  /// Array for sorting indices
  int * sortIndices_;
  /// Array for sorting elements
  double * sortElements_;
  /// Size of sort arrays
  int sortSize_;
  /// Linked list for quadratic rows
  mutable CoinModelLinkedList quadraticRowList_;
  /// Linked list for quadratic columns
  mutable CoinModelLinkedList quadraticColumnList_;
  /// Size of associated values
  int sizeAssociated_;
  /// Associated values
  double * associated_;
  /// Number of SOS - all these are done in one go e.g. from ampl
  int numberSOS_;
  /// SOS starts
  int * startSOS_;
  /// SOS members
  int * memberSOS_;
  /// SOS type
  int * typeSOS_;
  /// SOS priority
  int * prioritySOS_;
  /// SOS reference
  double * referenceSOS_;
  /// priorities (given for all columns (-1 if not integer)
  int * priority_;
  /// Nonzero if row is cut - done in one go e.g. from ampl
  int * cut_;
  /// Pointer to more information
  void * moreInfo_;
  /** Type of build -
      -1 unset,
      0 for row, 
      1 for column,
      2 linked.
      3 matrix is CoinPackedMatrix (and at present can't be modified);
  */
  mutable int type_;
  /// True if no names EVER being used (for users who know what they are doing)
  bool noNames_;
  /** Links present (could be tested by sizes of objects)
      0 - none,
      1 - row links,
      2 - column links,
      3 - both
  */
  mutable int links_;
   //@}
};
/// Just function of single variable x
double getFunctionValueFromString(const char * string, const char * x, double xValue);
/// faster version
double getDoubleFromString(CoinYacc & info, const char * string, const char * x, double xValue);
#endif
