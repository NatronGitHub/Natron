/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef AbcMatrix_H
#define AbcMatrix_H

#include "CoinPragma.hpp"

#include "ClpMatrixBase.hpp"
#include "AbcSimplex.hpp"
#include "CoinAbcHelperFunctions.hpp"
/** This implements a scaled version of CoinPackedMatrix
    It may have THREE! copies
    1) scaled CoinPackedMatrix without gaps
    2) row copy non-basic,basic, fixed
    3) vector copy
*/
class AbcMatrix2;
class AbcMatrix3;
class AbcMatrix {
  
public:
  /**@name Useful methods */
  //@{
  /// Return a complete CoinPackedMatrix
  inline CoinPackedMatrix * getPackedMatrix() const {
    return matrix_;
  }
  /** Whether the packed matrix is column major ordered or not. */
  inline bool isColOrdered() const {
    return true;
  }
  /** Number of entries in the packed matrix. */
  inline CoinBigIndex getNumElements() const {
    return matrix_->getNumElements();
  }
  /** Number of columns. */
  inline int getNumCols() const {
    assert(matrix_->getNumCols()==model_->numberColumns());return matrix_->getNumCols();
  }
  /** Number of rows. */
  inline int getNumRows() const {
    assert(matrix_->getNumRows()==model_->numberRows());return matrix_->getNumRows();
  }
  /// Sets model
  void setModel(AbcSimplex * model);
  /// A vector containing the elements in the packed matrix.
  inline const double * getElements() const {
    return matrix_->getElements();
  }
  /// Mutable elements
  inline double * getMutableElements() const {
    return matrix_->getMutableElements();
  }
  /// A vector containing the minor indices of the elements in the packed matrix. 
  inline const int * getIndices() const {
    return matrix_->getIndices();
  }
  /// A vector containing the minor indices of the elements in the packed matrix. 
  inline int * getMutableIndices() const {
    return matrix_->getMutableIndices();
  }
  /// Starts
  inline const CoinBigIndex * getVectorStarts() const {
    return matrix_->getVectorStarts();
  }
  inline  CoinBigIndex * getMutableVectorStarts() const {
    return matrix_->getMutableVectorStarts();
  }
  /** The lengths of the major-dimension vectors. */
  inline const int * getVectorLengths() const {
    return matrix_->getVectorLengths();
  }
  /** The lengths of the major-dimension vectors. */
  inline int * getMutableVectorLengths() const {
    return matrix_->getMutableVectorLengths();
  }
  /// Row starts
  CoinBigIndex * rowStart() const;
  /// Row ends
  CoinBigIndex * rowEnd() const;
  /// Row elements
  double * rowElements() const;
  /// Row columns
  CoinSimplexInt * rowColumns() const;
  /** Returns a new matrix in reverse order without gaps */
  CoinPackedMatrix * reverseOrderedCopy() const;
  /// Returns number of elements in column part of basis
  CoinBigIndex countBasis(const int * whichColumn,
			  int & numberColumnBasic);
  /// Fills in column part of basis
  void fillBasis(const int * whichColumn,
		 int & numberColumnBasic,
		 int * row, int * start,
		 int * rowCount, int * columnCount,
		 CoinSimplexDouble * element);
  /// Fills in column part of basis
  void fillBasis(const int * whichColumn,
		 int & numberColumnBasic,
		 int * row, int * start,
		 int * rowCount, int * columnCount,
		 long double * element);
  /** Scales and creates row copy
  */
  void scale(int numberRowsAlreadyScaled);
  /// Creates row copy
  void createRowCopy();
  /// Take out of useful
  void takeOutOfUseful(int sequence,CoinIndexedVector & spare);
  /// Put into useful
  void putIntofUseful(int sequence,CoinIndexedVector & spare);
  /// Put in and out for useful
  void inOutUseful(int sequenceIn,int sequenceOut);
  /// Make all useful
  void makeAllUseful(CoinIndexedVector & spare);
  /// Sort into useful
  void sortUseful(CoinIndexedVector & spare);
  /// Move largest in column to beginning (not used as doesn't help factorization)
  void moveLargestToStart();
  
  /** Unpacks a column into an CoinIndexedVector
   */
  void unpack(CoinIndexedVector & rowArray,
	      int column) const ;
  /** Adds multiple of a column (or slack) into an CoinIndexedvector
      You can use quickAdd to add to vector */
  void add(CoinIndexedVector & rowArray, int column, double multiplier) const ;
  //@}
  
  /**@name Matrix times vector methods */
  //@{
  /** Return <code>y + A * scalar *x</code> in <code>y</code>.
      @pre <code>x</code> must be of size <code>numColumns()</code>
      @pre <code>y</code> must be of size <code>numRows()</code> */
  void timesModifyExcludingSlacks(double scalar,
	     const double * x, double * y) const;
  /** Return <code>y + A * scalar(+-1) *x</code> in <code>y</code>.
      @pre <code>x</code> must be of size <code>numColumns()+numRows()</code>
      @pre <code>y</code> must be of size <code>numRows()</code> */
  void timesModifyIncludingSlacks(double scalar,
	     const double * x, double * y) const;
  /** Return <code>A * scalar(+-1) *x</code> in <code>y</code>.
      @pre <code>x</code> must be of size <code>numColumns()+numRows()</code>
      @pre <code>y</code> must be of size <code>numRows()</code> */
  void timesIncludingSlacks(double scalar,
	     const double * x, double * y) const;
  /** Return A * scalar(+-1) *x + y</code> in <code>y</code>.
      @pre <code>x</code> must be of size <code>numRows()</code>
      @pre <code>y</code> must be of size <code>numRows()+numColumns()</code> */
  void transposeTimesNonBasic(double scalar,
			    const double * x, double * y) const;
  /** Return y - A * x</code> in <code>y</code>.
      @pre <code>x</code> must be of size <code>numRows()</code>
      @pre <code>y</code> must be of size <code>numRows()+numColumns()</code> */
  void transposeTimesAll(const double * x, double * y) const;
  /** Return y + A * scalar(+-1) *x</code> in <code>y</code>.
      @pre <code>x</code> must be of size <code>numRows()</code>
      @pre <code>y</code> must be of size <code>numRows()</code> */
  void transposeTimesBasic(double scalar,
			    const double * x, double * y) const;
  /** Return <code>x * scalar * A/code> in <code>z</code>.
      Note - x unpacked mode - z packed mode including slacks
      All these return atLo/atUp first then free/superbasic
      number of first set returned
      pivotVariable is extended to have that order
      reversePivotVariable used to update that list
      free/superbasic only stored in normal format
      can use spare array to get this effect
      may put djs alongside atLo/atUp
      Squashes small elements and knows about AbcSimplex */
  int transposeTimesNonBasic(double scalar,
		      const CoinIndexedVector & x,
		      CoinIndexedVector & z) const;
  /// gets sorted tableau row and a possible value of theta
  double dualColumn1(const CoinIndexedVector & update,
		     CoinPartitionedVector & tableauRow,
		     CoinPartitionedVector & candidateList) const;
  /// gets sorted tableau row and a possible value of theta
  double dualColumn1Row(int iBlock, double upperThetaSlack, int & freeSequence,
			const CoinIndexedVector & update,
			CoinPartitionedVector & tableauRow,
			CoinPartitionedVector & candidateList) const;
  /// gets sorted tableau row and a possible value of theta
  double dualColumn1RowFew(int iBlock, double upperThetaSlack, int & freeSequence,
			 const CoinIndexedVector & update,
			 CoinPartitionedVector & tableauRow,
			 CoinPartitionedVector & candidateList) const;
  /// gets sorted tableau row and a possible value of theta
  double dualColumn1Row2(double upperThetaSlack, int & freeSequence,
			 const CoinIndexedVector & update,
			 CoinPartitionedVector & tableauRow,
			 CoinPartitionedVector & candidateList) const;
  /// gets sorted tableau row and a possible value of theta
  double dualColumn1Row1(double upperThetaSlack, int & freeSequence,
			 const CoinIndexedVector & update,
			 CoinPartitionedVector & tableauRow,
			 CoinPartitionedVector & candidateList) const;
  /** gets sorted tableau row and a possible value of theta
      On input first,last give what to scan 
      On output is number in tableauRow and candidateList */
  void dualColumn1Part(int iBlock,int & sequenceIn, double & upperTheta,
			 const CoinIndexedVector & update,
			 CoinPartitionedVector & tableauRow,
			 CoinPartitionedVector & candidateList) const;
  /// rebalance for parallel
  void rebalance() const;
  /// Get sequenceIn when Dantzig
  int pivotColumnDantzig(const CoinIndexedVector & updates,
			 CoinPartitionedVector & spare) const; 
  /// Get sequenceIn when Dantzig (One block)
  int pivotColumnDantzig(int iBlock,bool doByRow,const CoinIndexedVector & updates,
			 CoinPartitionedVector & spare,
			 double & bestValue) const; 
  /// gets tableau row - returns number of slacks in block 
  int primalColumnRow(int iBlock,bool doByRow,const CoinIndexedVector & update,
			  CoinPartitionedVector & tableauRow) const;
  /// gets tableau row and dj row - returns number of slacks in block 
  int primalColumnRowAndDjs(int iBlock,const CoinIndexedVector & updateTableau,
			  const CoinIndexedVector & updateDjs,
			    CoinPartitionedVector & tableauRow) const;
  /** Chooses best weighted dj
   */
  int chooseBestDj(int iBlock,const CoinIndexedVector & infeasibilities, 
		   const double * weights) const;
  /** does steepest edge double or triple update
      If scaleFactor!=0 then use with tableau row to update djs
      otherwise use updateForDjs
      Returns best sequence
   */
  int primalColumnDouble(int iBlock,CoinPartitionedVector & updateForTableauRow,
			  CoinPartitionedVector & updateForDjs,
			  const CoinIndexedVector & updateForWeights,
			  CoinPartitionedVector & spareColumn1,
			  double * infeasibilities, 
			  double referenceIn, double devex,
			  // Array for exact devex to say what is in reference framework
			  unsigned int * reference,
			  double * weights, double scaleFactor) const;
  /** does steepest edge double or triple update
      If scaleFactor!=0 then use with tableau row to update djs
      otherwise use updateForDjs
      Returns best sequence
   */
  int primalColumnSparseDouble(int iBlock,CoinPartitionedVector & updateForTableauRow,
			  CoinPartitionedVector & updateForDjs,
			  const CoinIndexedVector & updateForWeights,
			  CoinPartitionedVector & spareColumn1,
			  double * infeasibilities, 
			  double referenceIn, double devex,
			  // Array for exact devex to say what is in reference framework
			  unsigned int * reference,
			  double * weights, double scaleFactor) const;
  /** does steepest edge double or triple update
      If scaleFactor!=0 then use with tableau row to update djs
      otherwise use updateForDjs
      Returns best sequence
   */
  int primalColumnDouble(CoinPartitionedVector & updateForTableauRow,
			 CoinPartitionedVector & updateForDjs,
			 const CoinIndexedVector & updateForWeights,
			 CoinPartitionedVector & spareColumn1,
			 CoinIndexedVector & infeasible, 
			 double referenceIn, double devex,
			 // Array for exact devex to say what is in reference framework
			 unsigned int * reference,
			 double * weights, double scaleFactor) const;
  /// gets subset updates
  void primalColumnSubset(int iBlock,const CoinIndexedVector & update,
			  const CoinPartitionedVector & tableauRow,
			  CoinPartitionedVector & weights) const;
  /// Partial pricing
  void partialPricing(double startFraction, double endFraction,
		      int & bestSequence, int & numberWanted);
  /** Return <code>x *A</code> in <code>z</code> but
      just for indices Already in z.
      Note - z always packed mode */
  void subsetTransposeTimes(const CoinIndexedVector & x,
			    CoinIndexedVector & z) const;
  /// Return <code>-x *A</code> in <code>z</code>
  void transposeTimes(const CoinIndexedVector & x,
			    CoinIndexedVector & z) const;
  //@}
  
  /**@name Other */
  //@{
  /// Returns CoinPackedMatrix (non const)
  inline CoinPackedMatrix * matrix() const {
    return matrix_;
  }
  /** Partial pricing tuning parameter - minimum number of "objects" to scan.
      e.g. number of Gub sets but could be number of variables */
  inline int minimumObjectsScan() const {
    return minimumObjectsScan_;
  }
  inline void setMinimumObjectsScan(int value) {
    minimumObjectsScan_ = value;
  }
  /// Partial pricing tuning parameter - minimum number of negative reduced costs to get
  inline int minimumGoodReducedCosts() const {
    return minimumGoodReducedCosts_;
  }
  inline void setMinimumGoodReducedCosts(int value) {
    minimumGoodReducedCosts_ = value;
  }
  /// Current start of search space in matrix (as fraction)
  inline double startFraction() const {
    return startFraction_;
  }
  inline void setStartFraction(double value) {
    startFraction_ = value;
  }
  /// Current end of search space in matrix (as fraction)
  inline double endFraction() const {
    return endFraction_;
  }
  inline void setEndFraction(double value) {
    endFraction_ = value;
  }
  /// Current best reduced cost
  inline double savedBestDj() const {
    return savedBestDj_;
  }
  inline void setSavedBestDj(double value) {
    savedBestDj_ = value;
  }
  /// Initial number of negative reduced costs wanted
  inline int originalWanted() const {
    return originalWanted_;
  }
  inline void setOriginalWanted(int value) {
    originalWanted_ = value;
  }
  /// Current number of negative reduced costs which we still need
  inline int currentWanted() const {
    return currentWanted_;
  }
  inline void setCurrentWanted(int value) {
    currentWanted_ = value;
  }
  /// Current best sequence
  inline int savedBestSequence() const {
    return savedBestSequence_;
  }
  inline void setSavedBestSequence(int value) {
    savedBestSequence_ = value;
  }
  /// Start of each column block
  inline int * startColumnBlock() const
  {return startColumnBlock_;}
  /// Start of each block (in stored)
  inline const int * blockStart() const
  { return blockStart_;}
  inline bool gotRowCopy() const
  { return rowStart_!=0;}
  /// Start of each block (in stored)
  inline int blockStart(int block) const
  { return blockStart_[block];}
  /// Number of actual column blocks
  inline int numberColumnBlocks() const
  { return numberColumnBlocks_;}
  /// Number of actual row blocks
  inline int numberRowBlocks() const
  { return numberRowBlocks_;}
  //@}
  
  
  /**@name Constructors, destructor */
  //@{
  /** Default constructor. */
  AbcMatrix();
  /** Destructor */
  ~AbcMatrix();
  //@}
  
  /**@name Copy method */
  //@{
  /** The copy constructor. */
  AbcMatrix(const AbcMatrix&);
  /** The copy constructor from an CoinPackedMatrix. */
  AbcMatrix(const CoinPackedMatrix&);
  /** Subset constructor (without gaps).  Duplicates are allowed
      and order is as given */
  AbcMatrix (const AbcMatrix & wholeModel,
	     int numberRows, const int * whichRows,
	     int numberColumns, const int * whichColumns);
  AbcMatrix (const CoinPackedMatrix & wholeModel,
	     int numberRows, const int * whichRows,
	     int numberColumns, const int * whichColumns);
  
  AbcMatrix& operator=(const AbcMatrix&);
  /// Copy contents - resizing if necessary - otherwise re-use memory
  void copy(const AbcMatrix * from);
  //@}
private:
  
protected:
  /**@name Data members
     The data members are protected to allow access for derived classes. */
  //@{
  /// Data
  CoinPackedMatrix * matrix_;
  /// Model
  mutable AbcSimplex * model_;
#if ABC_PARALLEL==0
#define NUMBER_ROW_BLOCKS 1
#define NUMBER_COLUMN_BLOCKS 1
#elif ABC_PARALLEL==1
#define NUMBER_ROW_BLOCKS 4
#define NUMBER_COLUMN_BLOCKS 4
#else
#define NUMBER_ROW_BLOCKS 8
#define NUMBER_COLUMN_BLOCKS 8
#endif
  /** Start of each row (per block) - last lot are useless
      first all row starts for block 0, then for block2
      so NUMBER_ROW_BLOCKS+2 times number rows */
  CoinBigIndex * rowStart_;
  /// Values by row
  double * element_;
  /// Columns
  int * column_;
  /// Start of each column block
  mutable int startColumnBlock_[NUMBER_COLUMN_BLOCKS+1];
  /// Start of each block (in stored)
  int blockStart_[NUMBER_ROW_BLOCKS+1];
  /// Number of actual column blocks
  mutable int numberColumnBlocks_;
  /// Number of actual row blocks
  int numberRowBlocks_;
  //#define COUNT_COPY
#ifdef COUNT_COPY
#define MAX_COUNT 13
  /// Start in elements etc
  CoinBigIndex countStart_[MAX_COUNT+1];
  /// First column
  int countFirst_[MAX_COUNT+1];
  // later int countEndUseful_[MAX_COUNT+1];
  int * countRealColumn_;
  // later int * countInverseRealColumn_;
  CoinBigIndex * countStartLarge_;
  int * countRow_;
  double * countElement_;
  int smallestCount_;
  int largestCount_;
#endif 
  /// Special row copy
  //AbcMatrix2 * rowCopy_;
  /// Special column copy
  //AbcMatrix3 * columnCopy_;
  /// Current start of search space in matrix (as fraction)
  double startFraction_;
  /// Current end of search space in matrix (as fraction)
  double endFraction_;
  /// Best reduced cost so far
  double savedBestDj_;
  /// Initial number of negative reduced costs wanted
  int originalWanted_;
  /// Current number of negative reduced costs which we still need
  int currentWanted_;
  /// Saved best sequence in pricing
  int savedBestSequence_;
  /// Partial pricing tuning parameter - minimum number of "objects" to scan
  int minimumObjectsScan_;
  /// Partial pricing tuning parameter - minimum number of negative reduced costs to get
  int minimumGoodReducedCosts_;
  //@}
};
#ifdef THREAD
#include <pthread.h>
typedef struct {
  double acceptablePivot;
  const AbcSimplex * model;
  double * spare;
  int * spareIndex;
  double * arrayTemp;
  int * indexTemp;
  int * numberInPtr;
  double * bestPossiblePtr;
  double * upperThetaPtr;
  int * posFreePtr;
  double * freePivotPtr;
  int * numberOutPtr;
  const unsigned short * count;
  const double * pi;
  const CoinBigIndex * rowStart;
  const double * element;
  const unsigned short * column;
  int offset;
  int numberInRowArray;
  int numberLook;
} dualColumn0Struct;
#endif
class AbcMatrix2 {
  
public:
  /**@name Useful methods */
  //@{
  /** Return <code>x * -1 * A in <code>z</code>.
      Note - x packed and z will be packed mode
      Squashes small elements and knows about AbcSimplex */
  void transposeTimes(const AbcSimplex * model,
		      const CoinPackedMatrix * rowCopy,
		      const CoinIndexedVector & x,
		      CoinIndexedVector & spareArray,
		      CoinIndexedVector & z) const;
  /// Returns true if copy has useful information
  inline bool usefulInfo() const {
    return rowStart_ != NULL;
  }
  //@}
  
  
  /**@name Constructors, destructor */
  //@{
  /** Default constructor. */
  AbcMatrix2();
  /** Constructor from copy. */
  AbcMatrix2(AbcSimplex * model, const CoinPackedMatrix * rowCopy);
  /** Destructor */
  ~AbcMatrix2();
  //@}
  
  /**@name Copy method */
  //@{
  /** The copy constructor. */
  AbcMatrix2(const AbcMatrix2&);
  AbcMatrix2& operator=(const AbcMatrix2&);
  //@}
  
  
protected:
  /**@name Data members
     The data members are protected to allow access for derived classes. */
  //@{
  /// Number of blocks
  int numberBlocks_;
  /// Number of rows
  int numberRows_;
  /// Column offset for each block (plus one at end)
  int * offset_;
  /// Counts of elements in each part of row
  mutable unsigned short * count_;
  /// Row starts
  mutable CoinBigIndex * rowStart_;
  /// columns within block
  unsigned short * column_;
  /// work arrays
  double * work_;
#ifdef THREAD
  pthread_t * threadId_;
  dualColumn0Struct * info_;
#endif
  //@}
};
typedef struct {
  CoinBigIndex startElements_; // point to data
  int startIndices_; // point to column_
  int numberInBlock_;
  int numberPrice_; // at beginning
  int numberElements_; // number elements per column
} blockStruct3;
class AbcMatrix3 {
  
public:
  /**@name Useful methods */
  //@{
  /** Return <code>x * -1 * A in <code>z</code>.
      Note - x packed and z will be packed mode
      Squashes small elements and knows about AbcSimplex */
  void transposeTimes(const AbcSimplex * model,
		      const double * pi,
		      CoinIndexedVector & output) const;
  /// Updates two arrays for steepest
  void transposeTimes2(const AbcSimplex * model,
		       const double * pi, CoinIndexedVector & dj1,
		       const double * piWeight,
		       double referenceIn, double devex,
		       // Array for exact devex to say what is in reference framework
		       unsigned int * reference,
		       double * weights, double scaleFactor);
  //@}
  
  
  /**@name Constructors, destructor */
  //@{
  /** Default constructor. */
  AbcMatrix3();
  /** Constructor from copy. */
  AbcMatrix3(AbcSimplex * model, const CoinPackedMatrix * columnCopy);
  /** Destructor */
  ~AbcMatrix3();
  //@}
  
  /**@name Copy method */
  //@{
  /** The copy constructor. */
  AbcMatrix3(const AbcMatrix3&);
  AbcMatrix3& operator=(const AbcMatrix3&);
  //@}
  /**@name Sort methods */
  //@{
  /** Sort blocks */
  void sortBlocks(const AbcSimplex * model);
  /// Swap one variable
  void swapOne(const AbcSimplex * model, const AbcMatrix * matrix,
	       int iColumn);
  //@}
  
  
protected:
  /**@name Data members
     The data members are protected to allow access for derived classes. */
  //@{
  /// Number of blocks
  int numberBlocks_;
  /// Number of columns
  int numberColumns_;
  /// Column indices and reverse lookup (within block)
  int * column_;
  /// Starts for odd/long vectors
  CoinBigIndex * start_;
  /// Rows
  int * row_;
  /// Elements
  double * element_;
  /// Blocks (ordinary start at 0 and go to first block)
  blockStruct * block_;
  //@}
};

#endif
