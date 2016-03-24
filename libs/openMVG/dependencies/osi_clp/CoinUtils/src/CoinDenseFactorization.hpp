/* $Id: CoinDenseFactorization.hpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


/* 
   Authors
   
   John Forrest

 */
#ifndef CoinDenseFactorization_H
#define CoinDenseFactorization_H

#include <iostream>
#include <string>
#include <cassert>
#include "CoinTypes.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinFactorization.hpp"
class CoinPackedMatrix;
/// Abstract base class which also has some scalars so can be used from Dense or Simp
class CoinOtherFactorization {

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
  CoinOtherFactorization (  );
  /// Copy constructor 
  CoinOtherFactorization ( const CoinOtherFactorization &other);
  
  /// Destructor
  virtual ~CoinOtherFactorization (  );
  /// = copy
  CoinOtherFactorization & operator = ( const CoinOtherFactorization & other );
 
  /// Clone
  virtual CoinOtherFactorization * clone() const = 0;
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
  /// Sets number of pivots since factorization
  inline void setPivots (  int value ) 
  { numberPivots_=value; }
  /// Set number of Rows after factorization
  inline void setNumberRows(int value)
  { numberRows_ = value; }
  /// Number of Rows after factorization
  inline int numberRows (  ) const {
    return numberRows_;
  }
  /// Total number of columns in factorization
  inline int numberColumns (  ) const {
    return numberColumns_;
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
  /// Zero tolerance
  inline double zeroTolerance (  ) const {
    return zeroTolerance_ ;
  }
  void zeroTolerance (  double value );
#ifndef COIN_FAST_CODE
  /// Whether slack value is +1 or -1
  inline double slackValue (  ) const {
    return slackValue_ ;
  }
  void slackValue (  double value );
#endif
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
  virtual int factor ( ) = 0;
  /// Does post processing on valid factorization - putting variables on correct rows
  virtual void postProcess(const int * sequence, int * pivotVariable) = 0;
  /// Makes a non-singular basis by replacing variables
  virtual void makeNonSingular(int * sequence, int numberColumns) = 0;
  //@}

  /**@name rank one updates which do exist */
  //@{

  /** Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
      If checkBeforeModifying is true will do all accuracy checks
      before modifying factorization.  Whether to set this depends on
      speed considerations.  You could just do this on first iteration
      after factorization and thereafter re-factorize
   partial update already in U */
  virtual int replaceColumn ( CoinIndexedVector * regionSparse,
		      int pivotRow,
		      double pivotCheck ,
			      bool checkBeforeModifying=false,
			      double acceptablePivot=1.0e-8)=0;
  //@}

  /**@name various uses of factorization (return code number elements) 
   which user may want to know about */
  //@{
  /** Updates one column (FTRAN) from regionSparse2
      Tries to do FT update
      number returned is negative if no room
      regionSparse starts as zero and is zero at end.
      Note - if regionSparse2 packed on input - will be packed on output
  */
  virtual int updateColumnFT ( CoinIndexedVector * regionSparse,
			       CoinIndexedVector * regionSparse2,
			       bool noPermute=false) = 0;
  /** This version has same effect as above with FTUpdate==false
      so number returned is always >=0 */
  virtual int updateColumn ( CoinIndexedVector * regionSparse,
		     CoinIndexedVector * regionSparse2,
		     bool noPermute=false) const = 0;
    /// does FTRAN on two columns
    virtual int updateTwoColumnsFT(CoinIndexedVector * regionSparse1,
			   CoinIndexedVector * regionSparse2,
			   CoinIndexedVector * regionSparse3,
			   bool noPermute=false) = 0;
  /** Updates one column (BTRAN) from regionSparse2
      regionSparse starts as zero and is zero at end 
      Note - if regionSparse2 packed on input - will be packed on output
  */
  virtual int updateColumnTranspose ( CoinIndexedVector * regionSparse,
			      CoinIndexedVector * regionSparse2) const = 0;
  //@}

////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  /// Pivot tolerance
  double pivotTolerance_;
  /// Zero tolerance
  double zeroTolerance_;
#ifndef COIN_FAST_CODE
  /// Whether slack value is  +1 or -1
  double slackValue_;
#else
#ifndef slackValue_
#define slackValue_ -1.0
#endif
#endif
  /// Relax check on accuracy in replaceColumn
  double relaxCheck_;
  /// Number of elements after factorization
  CoinBigIndex factorElements_;
  /// Number of Rows in factorization
  int numberRows_;
  /// Number of Columns in factorization
  int numberColumns_;
  /// Number factorized in U (not row singletons)
  int numberGoodU_;
  /// Maximum number of pivots before factorization
  int maximumPivots_;
  /// Number pivots since last factorization
  int numberPivots_;
  /// Status of factorization
  int status_;
  /// Maximum rows ever (i.e. use to copy arrays etc)
  int maximumRows_;
  /// Maximum length of iterating area
  CoinBigIndex maximumSpace_;
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



class CoinDenseFactorization : public CoinOtherFactorization {
   friend void CoinDenseFactorizationUnitTest( const std::string & mpsDir );

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
  CoinDenseFactorization (  );
  /// Copy constructor 
  CoinDenseFactorization ( const CoinDenseFactorization &other);
  
  /// Destructor
  virtual ~CoinDenseFactorization (  );
  /// = copy
  CoinDenseFactorization & operator = ( const CoinDenseFactorization & other );
  /// Clone
  virtual CoinOtherFactorization * clone() const ;
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
  virtual int factor ( );
  /// Does post processing on valid factorization - putting variables on correct rows
  virtual void postProcess(const int * sequence, int * pivotVariable);
  /// Makes a non-singular basis by replacing variables
  virtual void makeNonSingular(int * sequence, int numberColumns);
  //@}

  /**@name general stuff such as number of elements */
  //@{ 
  /// Total number of elements in factorization
  virtual inline int numberElements (  ) const {
    return numberRows_*(numberColumns_+numberPivots_);
  }
  /// Returns maximum absolute value in factorization
  double maximumCoefficient() const;
  //@}

  /**@name rank one updates which do exist */
  //@{

  /** Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
      If checkBeforeModifying is true will do all accuracy checks
      before modifying factorization.  Whether to set this depends on
      speed considerations.  You could just do this on first iteration
      after factorization and thereafter re-factorize
   partial update already in U */
  virtual int replaceColumn ( CoinIndexedVector * regionSparse,
		      int pivotRow,
		      double pivotCheck ,
			      bool checkBeforeModifying=false,
			      double acceptablePivot=1.0e-8);
  //@}

  /**@name various uses of factorization (return code number elements) 
   which user may want to know about */
  //@{
  /** Updates one column (FTRAN) from regionSparse2
      Tries to do FT update
      number returned is negative if no room
      regionSparse starts as zero and is zero at end.
      Note - if regionSparse2 packed on input - will be packed on output
  */
  virtual inline int updateColumnFT ( CoinIndexedVector * regionSparse,
				      CoinIndexedVector * regionSparse2,
				      bool = false)
  { return updateColumn(regionSparse,regionSparse2);}
  /** This version has same effect as above with FTUpdate==false
      so number returned is always >=0 */
  virtual int updateColumn ( CoinIndexedVector * regionSparse,
		     CoinIndexedVector * regionSparse2,
		     bool noPermute=false) const;
    /// does FTRAN on two columns
    virtual int updateTwoColumnsFT(CoinIndexedVector * regionSparse1,
			   CoinIndexedVector * regionSparse2,
			   CoinIndexedVector * regionSparse3,
			   bool noPermute=false);
  /** Updates one column (BTRAN) from regionSparse2
      regionSparse starts as zero and is zero at end 
      Note - if regionSparse2 packed on input - will be packed on output
  */
  virtual int updateColumnTranspose ( CoinIndexedVector * regionSparse,
			      CoinIndexedVector * regionSparse2) const;
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
  void gutsOfCopy(const CoinDenseFactorization &other);

  //@}
protected:
  /** Returns accuracy status of replaceColumn
      returns 0=OK, 1=Probably OK, 2=singular */
  int checkPivot(double saveFromU, double oldPivot) const;
////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  //@}
};
#endif
