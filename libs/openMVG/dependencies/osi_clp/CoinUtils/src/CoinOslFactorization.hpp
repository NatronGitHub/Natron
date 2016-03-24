/* $Id: CoinOslFactorization.hpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 1987, 2009, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* 
   Authors
   
   John Forrest

 */
#ifndef CoinOslFactorization_H
#define CoinOslFactorization_H
#include <iostream>
#include <string>
#include <cassert>
#include "CoinTypes.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinDenseFactorization.hpp"
class CoinPackedMatrix;
/** This deals with Factorization and Updates
    This is ripped off from OSL!!!!!!!!!

    I am assuming that 32 bits is enough for number of rows or columns, but CoinBigIndex
    may be redefined to get 64 bits.
 */

typedef struct {int suc, pre;} EKKHlink;
typedef struct _EKKfactinfo {
  double drtpiv;
  double demark;
  double zpivlu;
  double zeroTolerance;
  double areaFactor;
  int *xrsadr;
  int *xcsadr;
  int *xrnadr;
  int *xcnadr;
  int *krpadr;
  int *kcpadr;
  int *mpermu;
  int *bitArray;
  int * back;
  char * nonzero;
  double * trueStart;
  mutable double *kadrpm;
  int *R_etas_index;
  int *R_etas_start;
  double *R_etas_element;

  int *xecadr;
  int *xeradr;
  double *xeeadr;
  double *xe2adr;
  EKKHlink * kp1adr;
  EKKHlink * kp2adr;
  double * kw1adr;
  double * kw2adr;
  double * kw3adr;
  int * hpivcoR;
  int nrow;
  int nrowmx;
  int firstDoRow;
  int firstLRow;
  int maxinv;
  int nnetas;
  int iterin;
  int iter0;
  int invok;
  int nbfinv;
  int num_resets;
  int nnentl;
  int nnentu;
#ifdef CLP_REUSE_ETAS
  int save_nnentu;
#endif
  int ndenuc;
  int npivots; /* use as xpivsq in factorization */
  int kmxeta;
  int xnetal;
  int first_dense;
  int last_dense;
  int iterno;
  int numberSlacks;
  int lastSlack;
  int firstNonSlack;
  int xnetalval;
  int lstart;
  int if_sparse_update;
  mutable int packedMode;
  int switch_off_sparse_update;
  int nuspike;
  bool rows_ok;	/* replaces test using mrstrt[1] */
#ifdef CLP_REUSE_ETAS
  mutable int reintro;
#endif
  int nR_etas;
  int sortedEta; /* if vector for F-T is sorted */
  int lastEtaCount;
  int ifvsol;
  int eta_size;
  int last_eta_size;
  int maxNNetas;
} EKKfactinfo;

class CoinOslFactorization : public CoinOtherFactorization {
   friend void CoinOslFactorizationUnitTest( const std::string & mpsDir );

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
  CoinOslFactorization (  );
  /// Copy constructor 
  CoinOslFactorization ( const CoinOslFactorization &other);
  
  /// Destructor
  virtual ~CoinOslFactorization (  );
  /// = copy
  CoinOslFactorization & operator = ( const CoinOslFactorization & other );
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
  /** When part of LP - given by basic variables.
  Actually does factorization.
  Arrays passed in have non negative value to say basic.
  If status is okay, basic variables have pivot row - this is only needed
  If status is singular, then basic variables have pivot row
  and ones thrown out have -1
  returns 0 -okay, -1 singular, -2 too many in basis, -99 memory */
  int factorize ( const CoinPackedMatrix & matrix, 
		  int rowIsBasic[], int columnIsBasic[] , 
		  double areaFactor = 0.0 );
  //@}

  /**@name general stuff such as number of elements */
  //@{ 
  /// Total number of elements in factorization
  virtual inline int numberElements (  ) const {
    return numberRows_*(numberColumns_+numberPivots_);
  }
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
  /// Returns true if wants tableauColumn in replaceColumn
  virtual bool wantsTableauColumn() const;
  /** Useful information for factorization
      0 - iteration number
      whereFrom is 0 for factorize and 1 for replaceColumn
  */
  virtual void setUsefulInformation(const int * info,int whereFrom);
  /// Set maximum pivots
  virtual void maximumPivots (  int value );

  /// Returns maximum absolute value in factorization
  double maximumCoefficient() const;
  /// Condition number - product of pivots after factorization
  double conditionNumber() const;
  /// Get rid of all memory
  virtual void clearArrays();
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
  virtual int updateColumnFT ( CoinIndexedVector * regionSparse,
				      CoinIndexedVector * regionSparse2,
				      bool noPermute=false);
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
  //inline void clearArrays()
  //{ gutsOfDestructor();}
  /// Returns array to put basis indices in
  virtual int * indices() const;
  /// Returns permute in
  virtual inline int * permute() const
  { return NULL;/*pivotRow_*/;}
  //@}

  /// The real work of desstructor 
  void gutsOfDestructor(bool clearFact=true);
  /// The real work of constructor
  void gutsOfInitialize(bool zapFact=true);
  /// The real work of copy
  void gutsOfCopy(const CoinOslFactorization &other);

  //@}
protected:
  /** Returns accuracy status of replaceColumn
      returns 0=OK, 1=Probably OK, 2=singular */
  int checkPivot(double saveFromU, double oldPivot) const;
////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  /// Osl factorization data
  EKKfactinfo factInfo_;
  //@}
};
#endif
