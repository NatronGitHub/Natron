/* $Id$ */
// Copyright (C) 2008, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


/* 
   Authors
   
   John Forrest

 */
#ifndef CoinAbcDenseFactorization_H
#define CoinAbcDenseFactorization_H

#include <iostream>
#include <string>
#include <cassert>
#include "CoinTypes.hpp"
#include "CoinAbcCommon.hpp"
#include "CoinIndexedVector.hpp"
class CoinPackedMatrix;
/// Abstract base class which also has some scalars so can be used from Dense or Simp
class CoinAbcAnyFactorization {

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
  CoinAbcAnyFactorization (  );
  /// Copy constructor 
  CoinAbcAnyFactorization ( const CoinAbcAnyFactorization &other);
  
  /// Destructor
  virtual ~CoinAbcAnyFactorization (  );
  /// = copy
  CoinAbcAnyFactorization & operator = ( const CoinAbcAnyFactorization & other );
 
  /// Clone
  virtual CoinAbcAnyFactorization * clone() const = 0;
  //@}

  /**@name general stuff such as status */
  //@{ 
  /// Returns status
  inline int status (  ) const {
    return status_;
  }
  /// Sets status
  inline void setStatus (  int value)
  {  status_=value;  }
  /// Returns number of pivots since factorization
  inline int pivots (  ) const {
    return numberPivots_;
  }
#if ABC_PARALLEL==2
  /// Says parallel
  inline void setParallelMode(int value)
  {parallelMode_=value;};
#endif
  /// Sets number of pivots since factorization
  inline void setPivots (  int value ) 
  { numberPivots_=value; }
  /// Returns number of slacks
  inline int numberSlacks (  ) const {
    return numberSlacks_;
  }
  /// Sets number of slacks
  inline void setNumberSlacks (  int value ) 
  { numberSlacks_=value; }
  /// Set number of Rows after factorization
  inline void setNumberRows(int value)
  { numberRows_ = value; }
  /// Number of Rows after factorization
  inline int numberRows (  ) const {
    return numberRows_;
  }
  /// Number of dense rows after factorization
  inline CoinSimplexInt numberDense (  ) const {
    return numberDense_;
  }
  /// Number of good columns in factorization
  inline int numberGoodColumns (  ) const {
    return numberGoodU_;
  }
  /// Allows change of pivot accuracy check 1.0 == none >1.0 relaxed
  inline void relaxAccuracyCheck(double value)
  { relaxCheck_ = value;}
  inline double getAccuracyCheck() const
  { return relaxCheck_;}
  /// Maximum number of pivots between factorizations
  inline int maximumPivots (  ) const {
    return maximumPivots_ ;
  }
  /// Set maximum pivots
  virtual void maximumPivots (  int value );

  /// Pivot tolerance
  inline double pivotTolerance (  ) const {
    return pivotTolerance_ ;
  }
  void pivotTolerance (  double value );
  /// Minimum pivot tolerance
  inline double minimumPivotTolerance (  ) const {
    return minimumPivotTolerance_ ;
  }
  void minimumPivotTolerance (  double value );
  virtual CoinFactorizationDouble * pivotRegion() const
  { return NULL;}
  /// Area factor
  inline double areaFactor (  ) const {
    return areaFactor_ ;
  }
  inline void areaFactor ( CoinSimplexDouble value ) {
    areaFactor_=value;
  }
  /// Zero tolerance
  inline double zeroTolerance (  ) const {
    return zeroTolerance_ ;
  }
  void zeroTolerance (  double value );
  /// Returns array to put basis elements in
  virtual CoinFactorizationDouble * elements() const;
  /// Returns pivot row 
  virtual int * pivotRow() const;
  /// Returns work area
  virtual CoinFactorizationDouble * workArea() const;
  /// Returns int work area
  virtual int * intWorkArea() const;
  /// Number of entries in each row
  virtual int * numberInRow() const;
  /// Number of entries in each column
  virtual int * numberInColumn() const;
  /// Returns array to put basis starts in
  virtual CoinBigIndex * starts() const;
  /// Returns permute back
  virtual int * permuteBack() const;
  /// Sees whether to go sparse
  virtual void goSparse() {}
#ifndef NDEBUG
  virtual inline void checkMarkArrays() const {}
#endif
  /** Get solve mode e.g. 0 C++ code, 1 Lapack, 2 choose
      If 4 set then values pass
      if 8 set then has iterated
  */
  inline int solveMode() const
  { return solveMode_ ;}
  /** Set solve mode e.g. 0 C++ code, 1 Lapack, 2 choose
      If 4 set then values pass
      if 8 set then has iterated
  */
  inline void setSolveMode(int value)
  { solveMode_ = value;}
  /// Returns true if wants tableauColumn in replaceColumn
  virtual bool wantsTableauColumn() const;
  /** Useful information for factorization
      0 - iteration number
      whereFrom is 0 for factorize and 1 for replaceColumn
  */
  virtual void setUsefulInformation(const int * info,int whereFrom);
  /// Get rid of all memory
  virtual void clearArrays() {}
  //@}
  /**@name virtual general stuff such as permutation */
  //@{ 
  /// Returns array to put basis indices in
  virtual int * indices() const  = 0;
  /// Returns permute in
  virtual int * permute() const = 0;
  /// Returns pivotColumn or permute
  virtual int * pivotColumn() const;
  /// Total number of elements in factorization
  virtual int numberElements (  ) const = 0;
  //@}
  /**@name Do factorization - public */
  //@{
  /// Gets space for a factorization
  virtual void getAreas ( int numberRows,
		  int numberColumns,
		  CoinBigIndex maximumL,
		  CoinBigIndex maximumU ) = 0;
  
  /// PreProcesses column ordered copy of basis
  virtual void preProcess ( ) = 0;
  /** Does most of factorization returning status
      0 - OK
      -99 - needs more memory
      -1 - singular - use numberGoodColumns and redo
  */
  virtual int factor (AbcSimplex * model) = 0;
#ifdef EARLY_FACTORIZE
  /// Returns -2 if can't, -1 if singular, -99 memory, 0 OK
  virtual int factorize (AbcSimplex * /*model*/, CoinIndexedVector & /*stuff*/)
  { return -2;}
#endif
  /// Does post processing on valid factorization - putting variables on correct rows
  virtual void postProcess(const int * sequence, int * pivotVariable) = 0;
  /// Makes a non-singular basis by replacing variables
  virtual void makeNonSingular(int * sequence) = 0;
  //@}

  /**@name rank one updates which do exist */
  //@{
#if 0
  /** Checks if can replace one Column to basis,
      returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots
      Fills in region for use later
      partial update already in U */
  virtual int checkReplace ( CoinIndexedVector * /*regionSparse*/,
			     int /*pivotRow*/,
			     double & /*pivotCheck*/,
			     double /*acceptablePivot = 1.0e-8*/)
  {return 0;}
  /** Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
      If skipBtranU is false will do btran part
   partial update already in U */
  virtual int replaceColumn ( CoinIndexedVector * regionSparse,
		      int pivotRow,
		      double pivotCheck ,
			      bool skipBtranU=false,
			      double acceptablePivot=1.0e-8)=0;
#endif
#ifdef EARLY_FACTORIZE
  /// 0 success, -1 can't +1 accuracy problems
  virtual int replaceColumns ( const AbcSimplex * /*model*/,
			       CoinIndexedVector & /*stuff*/,
			       int /*firstPivot*/,int /*lastPivot*/,bool /*cleanUp*/)
  { return -1;}
#endif
#ifdef ABC_LONG_FACTORIZATION 
  /// Clear all hidden arrays
  virtual void clearHiddenArrays() {}
#endif
  /** Checks if can replace one Column to basis,
      returns update alpha
      Fills in region for use later
      partial update already in U */
  virtual 
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
  double checkReplacePart1 ( CoinIndexedVector * /*regionSparse*/,
				     int /*pivotRow*/)
  {return 0.0;}
  virtual 
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
  double checkReplacePart1 ( CoinIndexedVector * /*regionSparse*/,
				     CoinIndexedVector * /*partialUpdate*/,
				     int /*pivotRow*/)
  {return 0.0;}
  virtual void checkReplacePart1a ( CoinIndexedVector * /* regionSparse */,
				    int /*pivotRow*/)
  {}
  virtual double checkReplacePart1b (CoinIndexedVector * /*regionSparse*/,
				     int /*pivotRow*/)
  {return 0.0;}
  /** Checks if can replace one Column to basis,
      returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots */
  virtual int checkReplacePart2 ( int pivotRow,
				  double btranAlpha, 
				  double ftranAlpha, 
#ifdef ABC_LONG_FACTORIZATION 
				  long
#endif
				  double ftAlpha,
				  double acceptablePivot = 1.0e-8) = 0;
  /** Replaces one Column to basis,
      partial update already in U */
  virtual void replaceColumnPart3 ( const AbcSimplex * model,
		      CoinIndexedVector * regionSparse,
		      CoinIndexedVector * tableauColumn,
		      int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
				    long
#endif
		       double alpha ) = 0;
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
				    double alpha )=0;
  //@}

  /**@name various uses of factorization (return code number elements) 
   which user may want to know about */
  //@{
  /** Updates one column (FTRAN) from unpacked regionSparse
      Tries to do FT update
      number returned is negative if no room
  */
  virtual int updateColumnFT ( CoinIndexedVector & regionSparse) = 0;
  virtual int updateColumnFTPart1 ( CoinIndexedVector & regionSparse) = 0;
  virtual void updateColumnFTPart2 ( CoinIndexedVector & regionSparse) = 0;
  virtual void updateColumnFT ( CoinIndexedVector & regionSparseFT,
				CoinIndexedVector & partialUpdate,
				int which)=0;
  /** This version has same effect as above with FTUpdate==false
      so number returned is always >=0 */
  virtual int updateColumn ( CoinIndexedVector & regionSparse) const = 0;
  /// does FTRAN on two unpacked columns
  virtual int updateTwoColumnsFT(CoinIndexedVector & regionFT,
				    CoinIndexedVector & regionOther) = 0;
  /** Updates one column (BTRAN) from unpacked regionSparse
  */
  virtual int updateColumnTranspose ( CoinIndexedVector & regionSparse) const = 0;
  /** This version does FTRAN on array when indices not set up */
  virtual void updateFullColumn ( CoinIndexedVector & regionSparse) const = 0;
  /** Updates one column (BTRAN) from unpacked regionSparse
  */
  virtual void updateFullColumnTranspose ( CoinIndexedVector & regionSparse) const = 0;
  /** Updates one column for dual steepest edge weights (FTRAN) */
  virtual void updateWeights ( CoinIndexedVector & regionSparse) const=0;
  /** Updates one column (FTRAN) */
  virtual void updateColumnCpu ( CoinIndexedVector & regionSparse,int whichCpu) const;
  /** Updates one column (BTRAN) */
  virtual void updateColumnTransposeCpu ( CoinIndexedVector & regionSparse,int whichCpu) const;
  //@}

////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  /// Pivot tolerance
  double pivotTolerance_;
  /// Minimum pivot tolerance
  double minimumPivotTolerance_;
  /// Area factor
  double areaFactor_;
  /// Zero tolerance
  double zeroTolerance_;
  //#ifndef slackValue_ 
#define slackValue2_ 1.0
  //#endif
  /// Relax check on accuracy in replaceColumn
  double relaxCheck_;
  /// Number of elements after factorization
  CoinBigIndex factorElements_;
  /// Number of Rows in factorization
  int numberRows_;
  /// Number of dense rows in factorization
  int numberDense_;
  /// Number factorized in U (not row singletons)
  int numberGoodU_;
  /// Maximum number of pivots before factorization
  int maximumPivots_;
  /// Number pivots since last factorization
  int numberPivots_;
  /// Number slacks
  int numberSlacks_;
  /// Status of factorization
  int status_;
  /// Maximum rows ever (i.e. use to copy arrays etc)
  int maximumRows_;
#if ABC_PARALLEL==2
  int parallelMode_;
#endif
  /// Pivot row 
  int * pivotRow_;
  /** Elements of factorization and updates
      length is maxR*maxR+maxSpace
      will always be long enough so can have nR*nR ints in maxSpace 
  */
  CoinFactorizationDouble * elements_;
  /// Work area of numberRows_ 
  CoinFactorizationDouble * workArea_;
  /** Solve mode e.g. 0 C++ code, 1 Lapack, 2 choose
      If 4 set then values pass
      if 8 set then has iterated
  */
  int solveMode_;
  //@}
};
/** This deals with Factorization and Updates
    This is a simple dense version so other people can write a better one

    I am assuming that 32 bits is enough for number of rows or columns, but CoinBigIndex
    may be redefined to get 64 bits.
 */



class CoinAbcDenseFactorization : public CoinAbcAnyFactorization {
   friend void CoinAbcDenseFactorizationUnitTest( const std::string & mpsDir );

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
  CoinAbcDenseFactorization (  );
  /// Copy constructor 
  CoinAbcDenseFactorization ( const CoinAbcDenseFactorization &other);
  
  /// Destructor
  virtual ~CoinAbcDenseFactorization (  );
  /// = copy
  CoinAbcDenseFactorization & operator = ( const CoinAbcDenseFactorization & other );
  /// Clone
  virtual CoinAbcAnyFactorization * clone() const ;
  //@}

  /**@name Do factorization - public */
  //@{
  /// Gets space for a factorization
  virtual void getAreas ( int numberRows,
		  int numberColumns,
		  CoinBigIndex maximumL,
		  CoinBigIndex maximumU );
  
  /// PreProcesses column ordered copy of basis
  virtual void preProcess ( );
  /** Does most of factorization returning status
      0 - OK
      -99 - needs more memory
      -1 - singular - use numberGoodColumns and redo
  */
  virtual int factor (AbcSimplex * model);
  /// Does post processing on valid factorization - putting variables on correct rows
  virtual void postProcess(const int * sequence, int * pivotVariable);
  /// Makes a non-singular basis by replacing variables
  virtual void makeNonSingular(int * sequence);
  //@}

  /**@name general stuff such as number of elements */
  //@{ 
  /// Total number of elements in factorization
  virtual inline int numberElements (  ) const {
    return numberRows_*(numberRows_+numberPivots_);
  }
  /// Returns maximum absolute value in factorization
  double maximumCoefficient() const;
  //@}

  /**@name rank one updates which do exist */
  //@{

  /** Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
      If skipBtranU is false will do btran part
   partial update already in U */
  virtual int replaceColumn ( CoinIndexedVector * regionSparse,
		      int pivotRow,
		      double pivotCheck ,
			      bool skipBtranU=false,
			      double acceptablePivot=1.0e-8);
  /** Checks if can replace one Column to basis,
      returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots */
  virtual int checkReplacePart2 ( int pivotRow,
				  double btranAlpha, 
				  double ftranAlpha, 
#ifdef ABC_LONG_FACTORIZATION 
				  long
#endif
				  double ftAlpha,
				  double acceptablePivot = 1.0e-8) ;
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
				    CoinIndexedVector * /*partialUpdate*/,
				    int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
				    long
#endif
				    double alpha )
  { replaceColumnPart3(model,regionSparse,tableauColumn,pivotRow,alpha);}
  //@}

  /**@name various uses of factorization (return code number elements) 
   which user may want to know about */
  //@{
  /** Updates one column (FTRAN) from unpacked regionSparse
      Tries to do FT update
      number returned is negative if no room
  */
  virtual int updateColumnFT ( CoinIndexedVector & regionSparse)
  {return updateColumn(regionSparse);}
  virtual int updateColumnFTPart1 ( CoinIndexedVector & regionSparse)
  {return updateColumn(regionSparse);}
  virtual void updateColumnFTPart2 ( CoinIndexedVector & /*regionSparse*/)
  {}
  virtual void updateColumnFT ( CoinIndexedVector & regionSparseFT,CoinIndexedVector & /*partialUpdate*/,int /*which*/)
  { updateColumnFT(regionSparseFT);}
  /** This version has same effect as above with FTUpdate==false
      so number returned is always >=0 */
  virtual int updateColumn ( CoinIndexedVector & regionSparse) const;
  /// does FTRAN on two unpacked columns
  virtual int updateTwoColumnsFT(CoinIndexedVector & regionFT,
				    CoinIndexedVector & regionOther);
  /** Updates one column (BTRAN) from unpacked regionSparse
  */
  virtual int updateColumnTranspose ( CoinIndexedVector & regionSparse) const;
  /** This version does FTRAN on array when indices not set up */
  virtual void updateFullColumn ( CoinIndexedVector & regionSparse) const
  {updateColumn(regionSparse);}
  /** Updates one column (BTRAN) from unpacked regionSparse
  */
  virtual void updateFullColumnTranspose ( CoinIndexedVector & regionSparse) const
  {updateColumnTranspose(regionSparse);}
  /** Updates one column for dual steepest edge weights (FTRAN) */
  virtual void updateWeights ( CoinIndexedVector & regionSparse) const;
  //@}
  /// *** Below this user may not want to know about

  /**@name various uses of factorization
   which user may not want to know about (left over from my LP code) */
  //@{
  /// Get rid of all memory
  inline void clearArrays()
  { gutsOfDestructor();}
  /// Returns array to put basis indices in
  virtual inline int * indices() const
  { return reinterpret_cast<int *> (elements_+numberRows_*numberRows_);}
  /// Returns permute in
  virtual inline int * permute() const
  { return NULL;/*pivotRow_*/;}
  //@}

  /// The real work of desstructor 
  void gutsOfDestructor();
  /// The real work of constructor
  void gutsOfInitialize();
  /// The real work of copy
  void gutsOfCopy(const CoinAbcDenseFactorization &other);

  //@}
protected:
  /** Returns accuracy status of replaceColumn
      returns 0=OK, 1=Probably OK, 2=singular */
  int checkPivot(double saveFromU, double oldPivot) const;
////////////////// data //////////////////
protected:
  /// Maximum length of iterating area
  CoinBigIndex maximumSpace_;
  /// Use for array size to get multiple of 8
  CoinSimplexInt maximumRowsAdjusted_;

  /**@name data */
  //@{
  //@}
};
#endif
