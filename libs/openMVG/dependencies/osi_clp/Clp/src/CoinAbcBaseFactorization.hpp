/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* 
   Authors
   
   John Forrest

 */
/** This deals with Factorization and Updates

    I am assuming that 32 bits is enough for number of rows or columns, but CoinBigIndex
    may be redefined to get 64 bits.
 */

#include "AbcCommon.hpp"
#include "CoinAbcHelperFunctions.hpp"
#if ABC_PARALLEL
#define FACTOR_CPU 4
#else
#define FACTOR_CPU 1
#endif
#define LARGE_SET COIN_INT_MAX-10
#define LARGE_UNSET (LARGE_SET+1)

class CoinAbcTypeFactorization  : public 
CoinAbcAnyFactorization 
{
   friend void CoinAbcFactorizationUnitTest( const std::string & mpsDir );

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
    CoinAbcTypeFactorization (  );
  /// Copy constructor 
  CoinAbcTypeFactorization ( const CoinAbcTypeFactorization &other);
  /// Copy constructor 
  CoinAbcTypeFactorization ( const CoinFactorization &other);

  /// Destructor
  virtual ~CoinAbcTypeFactorization (  );
  /// Clone
  virtual CoinAbcAnyFactorization * clone() const ;
  /// Delete all stuff (leaves as after CoinAbcFactorization())
  void almostDestructor();
  /// Debug show object (shows one representation)
  void show_self (  ) const;
  /// Debug - sort so can compare
  void sort (  ) const;
  /// = copy
    CoinAbcTypeFactorization & operator = ( const CoinAbcTypeFactorization & other );
  //@}

  /**@name Do factorization */
  //@{
  /// Condition number - product of pivots after factorization
  CoinSimplexDouble conditionNumber() const;
  
  //@}

  /**@name general stuff such as permutation or status */
  //@{ 
  /// Returns address of permute region
  inline CoinSimplexInt *permute (  ) const {
    return NULL; //permute_.array();
  }
  /// Returns array to put basis indices in
  virtual inline CoinSimplexInt * indices() const
  { return indexRowU_.array();}
  /// Returns address of pivotColumn region (also used for permuting)
  virtual inline CoinSimplexInt *pivotColumn (  ) const {
    return pivotColumn_.array();
  }
  /// Returns address of pivot region
  virtual inline CoinFactorizationDouble *pivotRegion (  ) const {
    return pivotRegionAddress_;
  }
#if ABC_SMALL<2
  /// Start of each row in L
  inline CoinBigIndex * startRowL() const
  { return startRowL_.array();}
#endif

  /// Start of each column in L
  inline CoinBigIndex * startColumnL() const
  { return startColumnL_.array();}

#if ABC_SMALL<2
  /// Index of column in row for L
  inline CoinSimplexInt * indexColumnL() const
  { return indexColumnL_.array();}
#endif

  /// Row indices of L
  inline CoinSimplexInt * indexRowL() const
  { return indexRowL_.array();}

#if ABC_SMALL<2
  /// Elements in L (row copy)
  inline CoinFactorizationDouble * elementByRowL() const
  { return elementByRowL_.array();}
#endif
  /**
     Forward and backward linked lists (numberRows_+2)
   **/
  inline CoinSimplexInt * pivotLinkedBackwards() const
  { return firstCount_.array()+numberRows_+1;}
  inline CoinSimplexInt * pivotLinkedForwards() const
  { return firstCount_.array()+2*numberRows_+3;}
  inline CoinSimplexInt * pivotLOrder() const
  { return firstCount_.array();}
#if ABC_SMALL<0
#define ABC_USE_FUNCTION_POINTERS 0
#define SMALL_PERMUTE
#endif
#ifdef ABC_USE_FUNCTION_POINTERS
  typedef void (*scatterUpdate) (int,CoinFactorizationDouble,const CoinFactorizationDouble *, CoinFactorizationDouble *);
#if ABC_USE_FUNCTION_POINTERS
  typedef struct {
    scatterUpdate functionPointer;
    CoinBigIndex offset;
    int number;
  } scatterStruct;
#else
  typedef struct {
    CoinBigIndex offset;
    int number;
  } scatterStruct;
#endif
  /// Array of function pointers PLUS for U Column
  inline scatterStruct * scatterUColumn() const
  { return scatterPointersUColumnAddress_;}
#endif

  /// For equal counts in factorization
  /** First Row/Column with count of k,
      can tell which by offset - Rows then Columns 
      actually comes before nextCount*/
  inline CoinSimplexInt * firstCount() const
  { return firstCount_.array();} 

  /// Next Row/Column with count
  inline CoinSimplexInt * nextCount() const
  { return firstCount_.array()+numberRows_+2;} 

  /// Previous Row/Column with count
  inline CoinSimplexInt * lastCount() const
  { return firstCount_.array()+3*numberRows_+2;} 

  /// Number of Rows after iterating
  inline CoinSimplexInt numberRowsExtra (  ) const {
    return numberRowsExtra_;
  }
  /// Number in L
  inline CoinBigIndex numberL() const
  { return numberL_;}

  /// Base of L
  inline CoinBigIndex baseL() const
  { return baseL_;}
  /// Maximum of Rows after iterating
  inline CoinSimplexInt maximumRowsExtra (  ) const {
    return maximumRowsExtra_;
  }
  /// Total number of elements in factorization
  virtual inline CoinBigIndex numberElements (  ) const {
    return totalElements_;
  }
  /// Length of FT vector
  inline CoinSimplexInt numberForrestTomlin (  ) const {
    return numberInColumn_.array()[numberRowsExtra_];
  }
  /// Returns areaFactor but adjusted for dense
  CoinSimplexDouble adjustedAreaFactor() const;
  /// Level of detail of messages
  inline CoinSimplexInt messageLevel (  ) const {
    return messageLevel_ ;
  }
  void messageLevel (  CoinSimplexInt value );
  /// Set maximum pivots
  virtual void maximumPivots (  CoinSimplexInt value );

#if ABC_SMALL<4
  /// Gets dense threshold
  inline CoinSimplexInt denseThreshold() const 
    { return denseThreshold_;}
  /// Sets dense threshold
  inline void setDenseThreshold(CoinSimplexInt value)
    { denseThreshold_ = value;}
#endif
  /// Returns maximum absolute value in factorization
  CoinSimplexDouble maximumCoefficient() const;
#if 0
  /// true if Forrest Tomlin update, false if PFI 
  inline bool forrestTomlin() const
  { return doForrestTomlin_;}
  inline void setForrestTomlin(bool value)
  { doForrestTomlin_=value;}
#endif
  /// True if FT update and space
  inline bool spaceForForrestTomlin() const
  {
    CoinBigIndex start = lastEntryByColumnU_;
    CoinBigIndex space = lengthAreaU_ - ( start + numberRowsExtra_ );
    return (space>=0); //&&doForrestTomlin_;
  }
  //@}

  /**@name some simple stuff */
  //@{


  /// Returns number in U area
  inline CoinBigIndex numberElementsU (  ) const {
    return lengthU_;
  }
  /// Setss number in U area
  inline void setNumberElementsU(CoinBigIndex value)
  { lengthU_ = value; }
  /// Returns length of U area
  inline CoinBigIndex lengthAreaU (  ) const {
    return lengthAreaU_;
  }
  /// Returns number in L area
  inline CoinBigIndex numberElementsL (  ) const {
    return lengthL_;
  }
  /// Returns length of L area
  inline CoinBigIndex lengthAreaL (  ) const {
    return lengthAreaL_;
  }
  /// Returns number in R area
  inline CoinBigIndex numberElementsR (  ) const {
    return lengthR_;
  }
  /// Number of compressions done
  inline CoinBigIndex numberCompressions() const
  { return numberCompressions_;}
  /// Returns pivot row 
  //virtual CoinSimplexInt * pivotRow() const;
  /// Returns work area
  //virtual CoinFactorizationDouble * workArea() const;
  /// Returns CoinSimplexInt work area
  //virtual CoinSimplexInt * intWorkArea() const;
  /// Returns array to put basis starts in
  virtual inline CoinBigIndex * starts() const
  { return startColumnU_.array();}
  /// Number of entries in each row
  virtual inline CoinSimplexInt * numberInRow() const
  { return numberInRow_.array();}
  /// Number of entries in each column
  virtual inline CoinSimplexInt * numberInColumn() const
  { return numberInColumn_.array();}
  /// Returns array to put basis elements in
  virtual inline CoinFactorizationDouble * elements() const
  { return elementU_.array();}
  /// Start of columns for R
  inline CoinBigIndex * startColumnR() const
  {return reinterpret_cast<CoinBigIndex *>(firstCount_.array()+3*numberRows_+4);}
  /// Elements of U
  inline CoinFactorizationDouble * elementU() const
  { return elementU_.array();}
  /// Row indices of U
  inline CoinSimplexInt * indexRowU() const
  { return indexRowU_.array();}
  /// Start of each column in U
  inline CoinBigIndex * startColumnU() const
  { return startColumnU_.array();}
#if COIN_BIG_DOUBLE==1
  /// To a work array and associate vector
  void toLongArray(CoinIndexedVector * vector,int which) const;
  /// From a work array and dis-associate vector
  void fromLongArray(CoinIndexedVector * vector) const;
  /// From a work array and dis-associate vector
  void fromLongArray(int which) const;
  /// Returns long double * associated with vector
  long double * denseVector(CoinIndexedVector * vector) const;
  /// Returns long double * associated with vector
  long double * denseVector(CoinIndexedVector & vector) const;
  /// Returns long double * associated with vector
  const long double * denseVector(const CoinIndexedVector * vector) const;
  /// Returns long double * associated with vector
  const long double * denseVector(const CoinIndexedVector & vector) const;
  /// Scans region to find nonzeros
  void scan(CoinIndexedVector * vector) const;
  /// Clear all hidden arrays
  void clearHiddenArrays();
#else
  /// Returns double * associated with vector
  inline double * denseVector(CoinIndexedVector * vector) const
  {return vector->denseVector();}
  inline double * denseVector(CoinIndexedVector & vector) const
  {return vector.denseVector();}
  /// Returns double * associated with vector
  inline const double * denseVector(const CoinIndexedVector * vector) const
  {return vector->denseVector();}
  inline const double * denseVector(const CoinIndexedVector & vector) const
  {return vector.denseVector();}
  /// To a work array and associate vector
  inline void toLongArray(CoinIndexedVector * vector,int which) const {}
  /// From a work array and dis-associate vector
  inline void fromLongArray(CoinIndexedVector * vector) const {}
  /// From a work array and dis-associate vector
  inline void fromLongArray(int which) const {}
  /// Scans region to find nonzeros
  inline void scan(CoinIndexedVector * vector) const
  {vector->scan(0,numberRows_,zeroTolerance_);}
#endif
#ifdef ABC_ORDERED_FACTORIZATION
  /// Permute in for Ftran
  void permuteInForFtran(CoinIndexedVector & regionSparse,bool full=false) const ;
  /// Permute in for Btran and multiply by pivot Region
  void permuteInForBtranAndMultiply(CoinIndexedVector & regionSparse, bool full=false) const ;
  /// Permute out for Btran
  void permuteOutForBtran(CoinIndexedVector & regionSparse) const ;
#endif
  /** Array persistence flag
      If 0 then as now (delete/new)
      1 then only do arrays if bigger needed
      2 as 1 but give a bit extra if bigger needed
  */
  //inline CoinSimplexInt persistenceFlag() const
  //{ return persistenceFlag_;}
  //@}

  /**@name rank one updates which do exist */
  //@{
#if 0
  /** Checks if can replace one Column to basis,
      returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots
      Fills in region for use later
      partial update already in U */
  virtual int checkReplace ( const AbcSimplex * model,
		      CoinIndexedVector * regionSparse,
		      int pivotRow,
		      CoinSimplexDouble & pivotCheck,
			     double acceptablePivot = 1.0e-8);
  /** Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
      If skipBtranU is false will do btran part
   partial update already in U */
  virtual CoinSimplexInt replaceColumn ( CoinIndexedVector * regionSparse,
		      CoinSimplexInt pivotRow,
		      CoinSimplexDouble pivotCheck ,
		      bool skipBtranU=false,
		      CoinSimplexDouble acceptablePivot=1.0e-8);
#endif
  /** Checks if can replace one Column to basis,
      returns update alpha
      Fills in region for use later
      partial update already in U */
  virtual 
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
  double checkReplacePart1 (  CoinIndexedVector * regionSparse,
				     int pivotRow);
  /** Checks if can replace one Column to basis,
      returns update alpha
      Fills in region for use later
      partial update in vector */
  virtual 
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
  double checkReplacePart1 (  CoinIndexedVector * regionSparse,
				      CoinIndexedVector * partialUpdate,
				     int pivotRow);
#ifdef MOVE_REPLACE_PART1A
  /** Checks if can replace one Column to basis,
      returns update alpha
      Fills in region for use later
      partial update already in U */
  virtual void checkReplacePart1a (  CoinIndexedVector * regionSparse,
				     int pivotRow);
  virtual 
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
  double checkReplacePart1b (  CoinIndexedVector * regionSparse,
				     int pivotRow);
#endif
  /** Checks if can replace one Column to basis,
      returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots */
  virtual int checkReplacePart2 ( int pivotRow,
				  CoinSimplexDouble btranAlpha, 
				  double ftranAlpha, 
#ifdef ABC_LONG_FACTORIZATION 
				  long
#endif
				  double ftAlpha,
				  double acceptablePivot = 1.0e-8);
  /** Replaces one Column to basis,
      partial update already in U */
  virtual void replaceColumnPart3 ( const AbcSimplex * model,
		      CoinIndexedVector * regionSparse,
		      CoinIndexedVector * tableauColumn,
		      int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
				    long
#endif
		       double alpha );
  /** Replaces one Column to basis,
      partial update in vector */
  virtual void replaceColumnPart3 ( const AbcSimplex * model,
				    CoinIndexedVector * regionSparse,
				    CoinIndexedVector * tableauColumn,
				    CoinIndexedVector * partialUpdate,
				    int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
				  long
#endif
				    double alpha );
#ifdef EARLY_FACTORIZE
  /// 0 success, -1 can't +1 accuracy problems
  virtual int replaceColumns ( const AbcSimplex * model,
			       CoinIndexedVector & stuff,
			       int firstPivot,int lastPivot,bool cleanUp);
#endif
  /// Update partial Ftran by R update
  void updatePartialUpdate(CoinIndexedVector & partialUpdate);
  /// Returns true if wants tableauColumn in replaceColumn
  inline virtual bool wantsTableauColumn() const
  {return false;}
  /** Combines BtranU and store which elements are to be deleted
      returns number to be deleted
  */
  int replaceColumnU ( CoinIndexedVector * regionSparse,
		       CoinBigIndex * deletedPosition,
		       CoinSimplexInt * deletedColumns,
		       CoinSimplexInt pivotRow);
  //@}

  /**@name various uses of factorization (return code number elements) 
   which user may want to know about */
  /// Later take out return codes (apart from +- 1 on FT) 
  //@{
  /** Updates one column (FTRAN) from regionSparse2
      Tries to do FT update
      number returned is negative if no room
      regionSparse starts as zero and is zero at end.
      Note - if regionSparse2 packed on input - will be packed on output
  */
  virtual CoinSimplexInt updateColumnFT ( CoinIndexedVector & regionSparse);
  virtual int updateColumnFTPart1 ( CoinIndexedVector & regionSparse) ;
  virtual void updateColumnFTPart2 ( CoinIndexedVector & regionSparse) ;
  /** Updates one column (FTRAN)
      Tries to do FT update
      puts partial update in vector */
  virtual void updateColumnFT ( CoinIndexedVector & regionSparseFT,
				CoinIndexedVector & partialUpdate,
				int which);
  /** This version has same effect as above with FTUpdate==false
      so number returned is always >=0 */
  virtual CoinSimplexInt updateColumn ( CoinIndexedVector & regionSparse) const;
  /** Updates one column (FTRAN) from region2
      Tries to do FT update
      number returned is negative if no room.
      Also updates region3
      region1 starts as zero and is zero at end */
 virtual CoinSimplexInt updateTwoColumnsFT ( CoinIndexedVector & regionFT,
				     CoinIndexedVector & regionOther);
  /** Updates one column (BTRAN) from regionSparse2
      regionSparse starts as zero and is zero at end 
      Note - if regionSparse2 packed on input - will be packed on output
  */
  virtual CoinSimplexInt updateColumnTranspose ( CoinIndexedVector & regionSparse) const;
  /** Updates one full column (FTRAN) */
  virtual void updateFullColumn ( CoinIndexedVector & regionSparse) const;
  /** Updates one full column (BTRAN) */
  virtual void updateFullColumnTranspose ( CoinIndexedVector & regionSparse) const;
  /** Updates one column for dual steepest edge weights (FTRAN) */
  virtual void updateWeights ( CoinIndexedVector & regionSparse) const;
  /** Updates one column (FTRAN) */
  virtual void updateColumnCpu ( CoinIndexedVector & regionSparse,int whichCpu) const;
  /** Updates one column (BTRAN) */
  virtual void updateColumnTransposeCpu ( CoinIndexedVector & regionSparse,int whichCpu) const;
  void unpack ( CoinIndexedVector * regionFrom,
		CoinIndexedVector * regionTo) const;
  void pack ( CoinIndexedVector * regionFrom,
		CoinIndexedVector * regionTo) const;
  /** makes a row copy of L for speed and to allow very sparse problems */
  inline void goSparse() {}
  void goSparse2();
#ifndef NDEBUG
  virtual void checkMarkArrays() const;
#endif
#if ABC_SMALL<2
  /**  get sparse threshold */
  inline CoinSimplexInt sparseThreshold ( ) const
  { return sparseThreshold_;}
#endif
  /**  set sparse threshold */
  void sparseThreshold ( CoinSimplexInt value );
  //@}
  /// *** Below this user may not want to know about

  /**@name various uses of factorization (return code number elements) 
   which user may not want to know about (left over from my LP code) */
  //@{
  /// Get rid of all memory
  inline void clearArrays()
  { gutsOfDestructor();}
  //@}
  /**@name used by ClpFactorization */
  /// See if worth going sparse
  void checkSparse();
  /// The real work of constructors etc 0 just scalars, 1 bit normal 
  void gutsOfDestructor(CoinSimplexInt type=1);
  /// 1 bit - tolerances etc, 2 more, 4 dummy arrays
  void gutsOfInitialize(CoinSimplexInt type);
  void gutsOfCopy(const CoinAbcTypeFactorization &other);

  /// Reset all sparsity etc statistics
  void resetStatistics();
  void printRegion(const CoinIndexedVector & vector, const char * where) const;

  //@}

  /**@name used by factorization */
  /// Gets space for a factorization, called by constructors
  virtual void getAreas ( CoinSimplexInt numberRows,
		  CoinSimplexInt numberColumns,
		  CoinBigIndex maximumL,
		  CoinBigIndex maximumU );

  /// PreProcesses column ordered copy of basis
  virtual void preProcess ( );
  void preProcess (CoinSimplexInt );
  /// Return largest element
  double preProcess3 ( );
  void preProcess4 ( );
  /// Does most of factorization
  virtual CoinSimplexInt factor (AbcSimplex * model);
#ifdef EARLY_FACTORIZE
  /// Returns -2 if can't, -1 if singular, -99 memory, 0 OK
  virtual int factorize (AbcSimplex * model, CoinIndexedVector & stuff);
#endif
  /// Does post processing on valid factorization - putting variables on correct rows
  virtual void postProcess(const CoinSimplexInt * sequence, CoinSimplexInt * pivotVariable);
  /// Makes a non-singular basis by replacing variables
  virtual void makeNonSingular(CoinSimplexInt * sequence);
protected:
  /** Does sparse phase of factorization
      return code is <0 error, 0= finished */
  CoinSimplexInt factorSparse (  );
  /** Does dense phase of factorization
      return code is <0 error, 0= finished */
  CoinSimplexInt factorDense (  );

  /// Pivots when just one other row so faster?
  bool pivotOneOtherRow ( CoinSimplexInt pivotRow,
			  CoinSimplexInt pivotColumn );
  /// Does one pivot on Row Singleton in factorization
  bool pivotRowSingleton ( CoinSimplexInt pivotRow,
			   CoinSimplexInt pivotColumn );
  /// Does one pivot on Column Singleton in factorization (can't return false)
  void pivotColumnSingleton ( CoinSimplexInt pivotRow,
			      CoinSimplexInt pivotColumn );
  /// After pivoting
  void afterPivot( CoinSimplexInt pivotRow,
			      CoinSimplexInt pivotColumn );
  /// After pivoting - returns true if need to go dense
  int wantToGoDense();

  /** Gets space for one Column with given length,
   may have to do compression  (returns True if successful),
   also moves existing vector,
   extraNeeded is over and above present */
  bool getColumnSpace ( CoinSimplexInt iColumn,
			CoinSimplexInt extraNeeded );

  /** Reorders U so contiguous and in order (if there is space)
      Returns true if it could */
  bool reorderU();
  /**  getColumnSpaceIterateR.  Gets space for one extra R element in Column
       may have to do compression  (returns true)
       also moves existing vector */
  bool getColumnSpaceIterateR ( CoinSimplexInt iColumn, CoinFactorizationDouble value,
			       CoinSimplexInt iRow);
  /**  getColumnSpaceIterate.  Gets space for one extra U element in Column
       may have to do compression  (returns true)
       also moves existing vector.
       Returns -1 if no memory or where element was put
       Used by replaceRow (turns off R version) */
  CoinBigIndex getColumnSpaceIterate ( CoinSimplexInt iColumn, CoinFactorizationDouble value,
			       CoinSimplexInt iRow);
  /** Gets space for one Row with given length,
  may have to do compression  (returns True if successful),
  also moves existing vector */
  bool getRowSpace ( CoinSimplexInt iRow, CoinSimplexInt extraNeeded );

  /** Gets space for one Row with given length while iterating,
  may have to do compression  (returns True if successful),
  also moves existing vector */
  bool getRowSpaceIterate ( CoinSimplexInt iRow,
			    CoinSimplexInt extraNeeded );
  /// Checks that row and column copies look OK
  void checkConsistency (  );
//#define CHECK_LINKS
#ifdef CHECK_LINKS
  void checkLinks(int x=0);
#else
#  define checkLinks(x) 
#endif
  /// Adds a link in chain of equal counts
  inline void addLink ( CoinSimplexInt index, CoinSimplexInt count ) {
    CoinSimplexInt * COIN_RESTRICT nextCount = nextCountAddress_;
    CoinSimplexInt * COIN_RESTRICT firstCount = this->firstCount();
    CoinSimplexInt * COIN_RESTRICT lastCount = lastCountAddress_;
    CoinSimplexInt next = firstCount[count];
    firstCount[count] = index;
    nextCount[index] = next;
    lastCount[index] = count-numberRows_-2; // points to firstCount[count] 
    if (next>=0)
      lastCount[next] = index;
  }
  /// Deletes a link in chain of equal counts
  inline void deleteLink ( CoinSimplexInt index ) {
    CoinSimplexInt * COIN_RESTRICT nextCount = nextCountAddress_;
    CoinSimplexInt * COIN_RESTRICT lastCount = lastCountAddress_;
    CoinSimplexInt next = nextCount[index];
    CoinSimplexInt last = lastCount[index];
    assert (next!=index);
    assert (last!=index);
    if (next>=0)
      lastCount[next] = last;
    if (last>=0) {
      nextCount[last] = next;
    } else {
      int count=last+numberRows_+2;
      CoinSimplexInt * COIN_RESTRICT firstCount = this->firstCount();
      firstCount[count]=next;
    }
  }
  /// Modifies links in chain of equal counts
  inline void modifyLink ( CoinSimplexInt index, CoinSimplexInt count ) {
    CoinSimplexInt * COIN_RESTRICT nextCount = nextCountAddress_;
    CoinSimplexInt * COIN_RESTRICT lastCount = lastCountAddress_;
    CoinSimplexInt * COIN_RESTRICT firstCount = this->firstCount();
    CoinSimplexInt next2 = firstCount[count];
    if (next2==index)
      return;
    firstCount[count] = index;
    CoinSimplexInt next = nextCount[index];
    CoinSimplexInt last = lastCount[index];
    assert (next!=index);
    assert (last!=index);
    nextCount[index] = next2;
    lastCount[index] = count-numberRows_-2; // points to firstCount[count] 
    if (next>=0)
      lastCount[next] = last;
    if (next2>=0)
      lastCount[next2] = index;
    if (last>=0) {
      nextCount[last] = next;
    } else {
      int count=last+numberRows_+2;
      firstCount[count]=next;
    }
  }
  /// Separate out links with same row/column count
  void separateLinks();
  void separateLinks(CoinSimplexInt,CoinSimplexInt);
  /// Cleans up at end of factorization
  void cleanup (  );
  /// Set up addresses from arrays
  void doAddresses();

  /// Updates part of column (FTRANL)
  void updateColumnL ( CoinIndexedVector * region
#if ABC_SMALL<2
		       , CoinAbcStatistics & statistics
#endif
#if ABC_PARALLEL
		       ,int whichSparse=0
#endif
		       ) const;
  /// Updates part of column (FTRANL) when densish
  void updateColumnLDensish ( CoinIndexedVector * region ) const;
  /// Updates part of column (FTRANL) when dense (i.e. do as inner products)
  void updateColumnLDense ( CoinIndexedVector * region ) const;
  /// Updates part of column (FTRANL) when sparse
  void updateColumnLSparse ( CoinIndexedVector * region 
#if ABC_PARALLEL
			     ,int whichSparse
#endif
			     ) const;

  /// Updates part of column (FTRANR) without FT update
  void updateColumnR ( CoinIndexedVector * region
#if ABC_SMALL<2
		       , CoinAbcStatistics & statistics 
#endif
#if ABC_PARALLEL
		       ,int whichSparse=0
#endif
		       ) const;
  /// Store update after doing L and R - retuns false if no room
  bool storeFT(
#if ABC_SMALL<3
	       const 
#endif
	       CoinIndexedVector * regionFT);
  /// Updates part of column (FTRANU)
  void updateColumnU ( CoinIndexedVector * region
#if ABC_SMALL<2
		       , CoinAbcStatistics & statistics
#endif
#if ABC_PARALLEL
		       ,int whichSparse=0
#endif
		       ) const;

  /// Updates part of column (FTRANU) when sparse
  void updateColumnUSparse ( CoinIndexedVector * regionSparse
#if ABC_PARALLEL
			     ,int whichSparse
#endif
			     ) const;
  /// Updates part of column (FTRANU)
  void updateColumnUDensish (  CoinIndexedVector * regionSparse) const;
  /// Updates part of column (FTRANU) when dense (i.e. do as inner products)
  void updateColumnUDense (  CoinIndexedVector * regionSparse) const;
  /// Updates part of 2 columns (FTRANU) real work
  void updateTwoColumnsUDensish (
				 CoinSimplexInt & numberNonZero1,
				 CoinFactorizationDouble * COIN_RESTRICT region1, 
				 CoinSimplexInt * COIN_RESTRICT index1,
				 CoinSimplexInt & numberNonZero2,
				 CoinFactorizationDouble * COIN_RESTRICT region2, 
				 CoinSimplexInt * COIN_RESTRICT index2) const;
  /// Updates part of column PFI (FTRAN) (after rest)
  void updateColumnPFI ( CoinIndexedVector * regionSparse) const; 
  /// Updates part of column transpose PFI (BTRAN) (before rest)
  void updateColumnTransposePFI ( CoinIndexedVector * region) const;
  /** Updates part of column transpose (BTRANU),
      assumes index is sorted i.e. region is correct */
  void updateColumnTransposeU ( CoinIndexedVector * region,
				CoinSimplexInt smallestIndex
#if ABC_SMALL<2
		       , CoinAbcStatistics & statistics
#endif
#if ABC_PARALLEL
		  ,int whichCpu
#endif
				) const;
  /** Updates part of column transpose (BTRANU) when densish,
      assumes index is sorted i.e. region is correct */
  void updateColumnTransposeUDensish ( CoinIndexedVector * region,
				       CoinSimplexInt smallestIndex) const;
  /** Updates part of column transpose (BTRANU) when sparse,
      assumes index is sorted i.e. region is correct */
  void updateColumnTransposeUSparse ( CoinIndexedVector * region
#if ABC_PARALLEL
						,int whichSparse
#endif
				      ) const;
  /** Updates part of column transpose (BTRANU) by column
      assumes index is sorted i.e. region is correct */
  void updateColumnTransposeUByColumn ( CoinIndexedVector * region,
					CoinSimplexInt smallestIndex) const;

  /// Updates part of column transpose (BTRANR)
  void updateColumnTransposeR ( CoinIndexedVector * region 
#if ABC_SMALL<2
		       , CoinAbcStatistics & statistics
#endif
				) const;
  /// Updates part of column transpose (BTRANR) when dense
  void updateColumnTransposeRDensish ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANR) when sparse
  void updateColumnTransposeRSparse ( CoinIndexedVector * region ) const;

  /// Updates part of column transpose (BTRANL)
  void updateColumnTransposeL ( CoinIndexedVector * region 
#if ABC_SMALL<2
		       , CoinAbcStatistics & statistics
#endif
#if ABC_PARALLEL
				      ,int whichSparse
#endif
				) const;
  /// Updates part of column transpose (BTRANL) when densish by column
  void updateColumnTransposeLDensish ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANL) when densish by row
  void updateColumnTransposeLByRow ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANL) when sparse (by Row)
  void updateColumnTransposeLSparse ( CoinIndexedVector * region 
#if ABC_PARALLEL
						,int whichSparse
#endif
				      ) const;
public:
  /** Replaces one Column to basis for PFI
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room.
   In this case region is not empty - it is incoming variable (updated)
  */
  CoinSimplexInt replaceColumnPFI ( CoinIndexedVector * regionSparse,
			 CoinSimplexInt pivotRow, CoinSimplexDouble alpha);
protected:
  /** Returns accuracy status of replaceColumn
      returns 0=OK, 1=Probably OK, 2=singular */
  CoinSimplexInt checkPivot(CoinSimplexDouble saveFromU, CoinSimplexDouble oldPivot) const;
  /// 0 fine, -99 singular, 2 dense
  int pivot ( CoinSimplexInt pivotRow,
	       CoinSimplexInt pivotColumn,
	       CoinBigIndex pivotRowPosition,
	       CoinBigIndex pivotColumnPosition,
	       CoinFactorizationDouble * COIN_RESTRICT work,
	       CoinSimplexUnsignedInt * COIN_RESTRICT workArea2,
	       CoinSimplexInt increment2,
	       int * COIN_RESTRICT markRow );
  int pivot ( CoinSimplexInt & pivotRow,
	       CoinSimplexInt & pivotColumn,
	       CoinBigIndex pivotRowPosition,
	       CoinBigIndex pivotColumnPosition,
	       int * COIN_RESTRICT markRow );
#if ABC_SMALL<2
#define CONVERTROW 2
#elif ABC_SMALL<4
#else
#undef ABC_DENSE_CODE
#define ABC_DENSE_CODE 0
#endif

  //@}
////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  CoinSimplexInt * pivotColumnAddress_;
  CoinSimplexInt * permuteAddress_;
  CoinFactorizationDouble * pivotRegionAddress_;
  CoinFactorizationDouble * elementUAddress_;
  CoinSimplexInt * indexRowUAddress_;
  CoinSimplexInt * numberInColumnAddress_;
  CoinSimplexInt * numberInColumnPlusAddress_;
#ifdef ABC_USE_FUNCTION_POINTERS
  /// Array of function pointers
  scatterStruct * scatterPointersUColumnAddress_;
  CoinFactorizationDouble * elementUColumnPlusAddress_;
#endif
  CoinBigIndex * startColumnUAddress_;
#if CONVERTROW
  CoinBigIndex * convertRowToColumnUAddress_;
#if CONVERTROW>1
  CoinBigIndex * convertColumnToRowUAddress_;
#endif
#endif
#if ABC_SMALL<2
  CoinFactorizationDouble * elementRowUAddress_;
#endif
  CoinBigIndex * startRowUAddress_;
  CoinSimplexInt * numberInRowAddress_;
  CoinSimplexInt * indexColumnUAddress_;
  CoinSimplexInt * firstCountAddress_;
  /// Next Row/Column with count
  CoinSimplexInt * nextCountAddress_;
  /// Previous Row/Column with count
  CoinSimplexInt * lastCountAddress_;
  CoinSimplexInt * nextColumnAddress_;
  CoinSimplexInt * lastColumnAddress_;
  CoinSimplexInt * nextRowAddress_;
  CoinSimplexInt * lastRowAddress_;
  CoinSimplexInt * saveColumnAddress_;
  //CoinSimplexInt * saveColumnAddress2_;
  CoinCheckZero * markRowAddress_;
  CoinSimplexInt * listAddress_;
  CoinFactorizationDouble * elementLAddress_;
  CoinSimplexInt * indexRowLAddress_;
  CoinBigIndex * startColumnLAddress_;
#if ABC_SMALL<2
  CoinBigIndex * startRowLAddress_;
#endif
  CoinSimplexInt * pivotLinkedBackwardsAddress_;
  CoinSimplexInt * pivotLinkedForwardsAddress_;
  CoinSimplexInt * pivotLOrderAddress_;
  CoinBigIndex * startColumnRAddress_;
  /// Elements of R
  CoinFactorizationDouble *elementRAddress_;
  /// Row indices for R
  CoinSimplexInt *indexRowRAddress_;
  CoinSimplexInt * indexColumnLAddress_;
  CoinFactorizationDouble * elementByRowLAddress_;
#if ABC_SMALL<4
  CoinFactorizationDouble * denseAreaAddress_;
#endif
  CoinFactorizationDouble * workAreaAddress_;
  CoinSimplexUnsignedInt * workArea2Address_;
  mutable CoinSimplexInt * sparseAddress_;
#ifdef SMALL_PERMUTE
  CoinSimplexInt * fromSmallToBigRow_;
  CoinSimplexInt * fromSmallToBigColumn_;
#endif	  
  /// Number of Rows after iterating
  CoinSimplexInt numberRowsExtra_;
  /// Maximum number of Rows after iterating
  CoinSimplexInt maximumRowsExtra_;
  /// Size of small inverse
  CoinSimplexInt numberRowsSmall_;
  /// Number factorized in L
  CoinSimplexInt numberGoodL_;
  /// Number Rows left (numberRows-numberGood)
  CoinSimplexInt numberRowsLeft_;
  /// Number of elements in U (to go)
  ///       or while iterating total overall
  CoinBigIndex totalElements_;
  /// First place in funny copy zeroed out
  CoinBigIndex firstZeroed_;
#if ABC_SMALL<2
  /// Below this use sparse technology - if 0 then no L row copy
  CoinSimplexInt sparseThreshold_;
#endif
  /// Number in R
  CoinSimplexInt numberR_;
  /// Length of R stuff
  CoinBigIndex lengthR_;
  /// length of area reserved for R
  CoinBigIndex lengthAreaR_;
  /// Number in L
  CoinBigIndex numberL_;
  /// Base of L
  CoinBigIndex baseL_;
  /// Length of L
  CoinBigIndex lengthL_;
  /// Length of area reserved for L
  CoinBigIndex lengthAreaL_;
  /// Number in U
  CoinSimplexInt numberU_;
  /// Maximum space used in U
  CoinBigIndex maximumU_;
  /// Length of U
  CoinBigIndex lengthU_;
  /// Length of area reserved for U
  CoinBigIndex lengthAreaU_;
  /// Last entry by column for U
  CoinBigIndex lastEntryByColumnU_;
#ifdef ABC_USE_FUNCTION_POINTERS
  /// Last entry by column for U
  CoinBigIndex lastEntryByColumnUPlus_;
  /// Length of U
  CoinBigIndex lengthAreaUPlus_;
#endif
  /// Last entry by row for U
  CoinBigIndex lastEntryByRowU_;
  /// Number of trials before rejection
  CoinSimplexInt numberTrials_;
#if ABC_SMALL<4
  /// Leading dimension for dense
  CoinSimplexInt leadingDimension_;
#endif
#if COIN_BIG_DOUBLE==1
  /// Work arrays
  mutable CoinFactorizationLongDoubleArrayWithLength longArray_[FACTOR_CPU];
  /// Associated CoinIndexedVector
  mutable CoinIndexedVector * associatedVector_[FACTOR_CPU];
#endif
  /// Pivot order for each Column
  CoinIntArrayWithLength pivotColumn_;
  /// Permutation vector for pivot row order
  CoinIntArrayWithLength permute_;
  /// Start of each Row as pointer
  CoinBigIndexArrayWithLength startRowU_;
  /// Number in each Row
  CoinIntArrayWithLength numberInRow_;
  /// Number in each Column
  CoinIntArrayWithLength numberInColumn_;
  /// Number in each Column including pivoted
  CoinIntArrayWithLength numberInColumnPlus_;
  /** First Row/Column with count of k,
      can tell which by offset - Rows then Columns */
  CoinIntArrayWithLength firstCount_;
  /// Next Column in memory order
  CoinIntArrayWithLength nextColumn_;
  /// Previous Column in memory order
  CoinIntArrayWithLength lastColumn_;
  /// Next Row in memory order
  CoinIntArrayWithLength nextRow_;
  /// Previous Row in memory order
  CoinIntArrayWithLength lastRow_;
  /// Columns left to do in a single pivot
  CoinIntArrayWithLength saveColumn_;
  /// Marks rows to be updated
  CoinIntArrayWithLength markRow_;
  /// Base address for U (may change)
  CoinIntArrayWithLength indexColumnU_;
  /// Inverses of pivot values
  CoinFactorizationDoubleArrayWithLength pivotRegion_;
  /// Elements of U
  CoinFactorizationDoubleArrayWithLength elementU_;
  /// Row indices of U
  CoinIntArrayWithLength indexRowU_;
  /// Start of each column in U
  CoinBigIndexArrayWithLength startColumnU_;
#ifdef ABC_USE_FUNCTION_POINTERS
  /// Array of structs for U Column
  CoinArbitraryArrayWithLength scatterUColumn_;
#endif
#if CONVERTROW
  /// Converts rows to columns in U 
  CoinBigIndexArrayWithLength convertRowToColumnU_;
#if CONVERTROW>1
  /// Converts columns to rows in U 
  CoinBigIndexArrayWithLength convertColumnToRowU_;
#endif
#endif
#if ABC_SMALL<2
  /// Elements of U by row
  CoinFactorizationDoubleArrayWithLength elementRowU_;
#endif
  /// Elements of L
  CoinFactorizationDoubleArrayWithLength elementL_;
  /// Row indices of L
  CoinIntArrayWithLength indexRowL_;
  /// Start of each column in L
  CoinBigIndexArrayWithLength startColumnL_;
#if ABC_SMALL<4
  /// Dense area
  CoinFactorizationDoubleArrayWithLength denseArea_;
#endif
  /// First work area
  CoinFactorizationDoubleArrayWithLength workArea_;
  /// Second work area
  CoinUnsignedIntArrayWithLength workArea2_;
#if ABC_SMALL<2
  /// Start of each row in L
  CoinBigIndexArrayWithLength startRowL_;
  /// Index of column in row for L
  CoinIntArrayWithLength indexColumnL_;
  /// Elements in L (row copy)
  CoinFactorizationDoubleArrayWithLength elementByRowL_;
  /// Sparse regions
  mutable CoinIntArrayWithLength sparse_;
#endif
  /// Detail in messages
  CoinSimplexInt messageLevel_;
  /// Number of compressions done
  CoinBigIndex numberCompressions_;
  // last slack pivot row
  CoinSimplexInt lastSlack_;
#if ABC_SMALL<2
  /// To decide how to solve
  mutable double ftranCountInput_;
  mutable double ftranCountAfterL_;
  mutable double ftranCountAfterR_;
  mutable double ftranCountAfterU_;
  double ftranAverageAfterL_;
  double ftranAverageAfterR_;
  double ftranAverageAfterU_;
#if FACTORIZATION_STATISTICS
  double ftranTwiddleFactor1_;
  double ftranTwiddleFactor2_;
#endif  
  mutable CoinSimplexInt numberFtranCounts_;
#endif
  /// Maximum rows (ever) (here to use double alignment)
  CoinSimplexInt maximumRows_;
#if ABC_SMALL<2
  mutable double ftranFTCountInput_;
  mutable double ftranFTCountAfterL_;
  mutable double ftranFTCountAfterR_;
  mutable double ftranFTCountAfterU_;
  double ftranFTAverageAfterL_;
  double ftranFTAverageAfterR_;
  double ftranFTAverageAfterU_;
#if FACTORIZATION_STATISTICS
  double ftranFTTwiddleFactor1_;
  double ftranFTTwiddleFactor2_;
#endif  
  mutable CoinSimplexInt numberFtranFTCounts_;
#endif
#if ABC_SMALL<4
  /// Dense threshold (here to use double alignment)
  CoinSimplexInt denseThreshold_;
#endif
#if ABC_SMALL<2
  mutable double btranCountInput_;
  mutable double btranCountAfterU_;
  mutable double btranCountAfterR_;
  mutable double btranCountAfterL_;
  double btranAverageAfterU_;
  double btranAverageAfterR_;
  double btranAverageAfterL_;
#if FACTORIZATION_STATISTICS
  double btranTwiddleFactor1_;
  double btranTwiddleFactor2_;
#endif  
  mutable CoinSimplexInt numberBtranCounts_;
#endif
  /// Maximum maximum pivots
  CoinSimplexInt maximumMaximumPivots_;
#if ABC_SMALL<2
  /// To decide how to solve
  mutable double ftranFullCountInput_;
  mutable double ftranFullCountAfterL_;
  mutable double ftranFullCountAfterR_;
  mutable double ftranFullCountAfterU_;
  double ftranFullAverageAfterL_;
  double ftranFullAverageAfterR_;
  double ftranFullAverageAfterU_;
#if FACTORIZATION_STATISTICS
  double ftranFullTwiddleFactor1_;
  double ftranFullTwiddleFactor2_;
#endif  
  mutable CoinSimplexInt numberFtranFullCounts_;
#endif
  /// Rows first time nonzero
  CoinSimplexInt initialNumberRows_;
#if ABC_SMALL<2
  /// To decide how to solve
  mutable double btranFullCountInput_;
  mutable double btranFullCountAfterL_;
  mutable double btranFullCountAfterR_;
  mutable double btranFullCountAfterU_;
  double btranFullAverageAfterL_;
  double btranFullAverageAfterR_;
  double btranFullAverageAfterU_;
#if FACTORIZATION_STATISTICS
  double btranFullTwiddleFactor1_;
  double btranFullTwiddleFactor2_;
#endif  
  mutable CoinSimplexInt numberBtranFullCounts_;
#endif
  /** State of saved version and what can be done
      0 - nothing saved
      1 - saved and can go back to previous save by unwinding
      2 - saved - getting on for a full copy
      higher bits - see ABC_FAC....
  */
  CoinSimplexInt state_;
  /// Size in bytes of a sparseArray
  CoinBigIndex sizeSparseArray_;
public:
#if ABC_SMALL<2
#if ABC_SMALL>=0
  inline bool gotLCopy() const {return ((state_&ABC_FAC_GOT_LCOPY)!=0);}
  inline void setNoGotLCopy() {state_ &= ~ABC_FAC_GOT_LCOPY;}
  inline void setYesGotLCopy() {state_ |= ABC_FAC_GOT_LCOPY;}
  inline bool gotRCopy() const {return ((state_&ABC_FAC_GOT_RCOPY)!=0);}
  inline void setNoGotRCopy() {state_ &= ~ABC_FAC_GOT_RCOPY;}
  inline void setYesGotRCopy() {state_ |= ABC_FAC_GOT_RCOPY;}
  inline bool gotUCopy() const {return ((state_&ABC_FAC_GOT_UCOPY)!=0);}
  inline void setNoGotUCopy() {state_ &= ~ABC_FAC_GOT_UCOPY;}
  inline void setYesGotUCopy() {state_ |= ABC_FAC_GOT_UCOPY;}
  inline bool gotSparse() const {return ((state_&ABC_FAC_GOT_SPARSE)!=0);}
  inline void setNoGotSparse() {state_ &= ~ABC_FAC_GOT_SPARSE;}
  inline void setYesGotSparse() {state_ |= ABC_FAC_GOT_SPARSE;}
#else
  // force use of copies
  inline bool gotLCopy() const {return true;}
  inline void setNoGotLCopy() {}
  inline void setYesGotLCopy() {}
  inline bool gotRCopy() const {return true;}
  inline void setNoGotRCopy() {}
  inline void setYesGotRCopy() {}
  inline bool gotUCopy() const {return true;}
  inline void setNoGotUCopy() {}
  inline void setYesGotUCopy() {}
  inline bool gotSparse() const {return true;}
  inline void setNoGotSparse() {}
  inline void setYesGotSparse() {}
#endif
#else
  // force no use of copies
  inline bool gotLCopy() const {return false;}
  inline void setNoGotLCopy() {}
  inline void setYesGotLCopy() {}
  inline bool gotRCopy() const {return false;}
  inline void setNoGotRCopy() {}
  inline void setYesGotRCopy() {}
  inline bool gotUCopy() const {return false;}
  inline void setNoGotUCopy() {}
  inline void setYesGotUCopy() {}
  inline bool gotSparse() const {return false;}
  inline void setNoGotSparse() {}
  inline void setYesGotSparse() {}
#endif
  /** Array persistence flag
      If 0 then as now (delete/new)
      1 then only do arrays if bigger needed
      2 as 1 but give a bit extra if bigger needed
  */
  //CoinSimplexInt persistenceFlag_;
  //@}
};


