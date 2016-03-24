/* $Id: CoinSimpFactorization.hpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* 
   This is a simple factorization of the LP Basis
 */
#ifndef CoinSimpFactorization_H
#define CoinSimpFactorization_H

#include <iostream>
#include <string>
#include <cassert>
#include "CoinTypes.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinDenseFactorization.hpp"
class CoinPackedMatrix;


/// pointers used during factorization
class FactorPointers{
public:
    double *rowMax;
    int *firstRowKnonzeros;
    int *prevRow;
    int *nextRow;
    int *firstColKnonzeros;
    int *prevColumn;
    int *nextColumn;
    int *newCols;
    //constructor
    FactorPointers( int numRows, int numCols, int *UrowLengths_, int *UcolLengths_ );
    // destructor
    ~ FactorPointers();
};

class CoinSimpFactorization : public CoinOtherFactorization {
   friend void CoinSimpFactorizationUnitTest( const std::string & mpsDir );

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
  CoinSimpFactorization (  );
  /// Copy constructor 
  CoinSimpFactorization ( const CoinSimpFactorization &other);
  
  /// Destructor
  virtual ~CoinSimpFactorization (  );
  /// = copy
  CoinSimpFactorization & operator = ( const CoinSimpFactorization & other );
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

  /**@name general stuff such as status */
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
    /// does updatecolumn if save==true keeps column for replace column
    int upColumn ( CoinIndexedVector * regionSparse,
		   CoinIndexedVector * regionSparse2,
		   bool noPermute=false, bool save=false) const;
    /** Updates one column (BTRAN) from regionSparse2
	regionSparse starts as zero and is zero at end 
	Note - if regionSparse2 packed on input - will be packed on output
    */
    virtual int updateColumnTranspose ( CoinIndexedVector * regionSparse,
				CoinIndexedVector * regionSparse2) const;
    /// does updateColumnTranspose, the other is a wrapper
    int upColumnTranspose ( CoinIndexedVector * regionSparse,
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
  inline int * indices() const
  { return reinterpret_cast<int *> (elements_+numberRows_*numberRows_);}
  /// Returns permute in
  virtual inline int * permute() const
  { return pivotRow_;}
  //@}

  /// The real work of destructor 
  void gutsOfDestructor();
  /// The real work of constructor
  void gutsOfInitialize();
  /// The real work of copy
  void gutsOfCopy(const CoinSimpFactorization &other);

    
    /// calls factorization
  void factorize(int numberOfRows,
		 int numberOfColumns,
		 const int colStarts[],
		 const int indicesRow[],
		 const double elements[]);
    /// main loop of factorization
    int mainLoopFactor (FactorPointers &pointers );
    /// copies L by rows
    void copyLbyRows();
    /// copies U by columns
    void copyUbyColumns();
    /// finds a pivot element using Markowitz count
    int findPivot(FactorPointers &pointers, int &r, int &s, bool &ifSlack);
    /// finds a pivot in a shortest column
    int findPivotShCol(FactorPointers &pointers, int &r, int &s);
    /// finds a pivot in the first column available
    int findPivotSimp(FactorPointers &pointers, int &r, int &s);
    /// does Gauss elimination
    void GaussEliminate(FactorPointers &pointers, int &r, int &s);
    /// finds short row that intersects a given column
    int findShortRow(const int column, const int length, int &minRow, 
		     int &minRowLength, FactorPointers &pointers);
    /// finds short column that intersects a given row
    int findShortColumn(const int row, const int length, int &minCol, 
			int &minColLength, FactorPointers &pointers);
    /// finds maximum absolute value in a row
    double findMaxInRrow(const int row, FactorPointers &pointers);
    /// does pivoting
    void pivoting(const int pivotRow, const int pivotColumn,
		  const double invPivot, FactorPointers &pointers);
    /// part of pivoting
    void updateCurrentRow(const int pivotRow, const int row, 
			  const double multiplier, FactorPointers &pointers,
			  int &newNonZeros);
    /// allocates more space for L
    void increaseLsize();
    /// allocates more space for a row of U
    void increaseRowSize(const int row, const int newSize);
    /// allocates more space for a column of U
    void increaseColSize(const int column, const int newSize, const bool b);
    /// allocates more space for rows of U
    void enlargeUrow(const int numNewElements);
    /// allocates more space for columns of U
    void enlargeUcol(const int numNewElements, const bool b);
    /// finds a given row in a column
    int findInRow(const int row, const int column);
    /// finds a given column in a row
    int findInColumn(const int column, const int row);
    /// declares a row inactive
    void removeRowFromActSet(const int row, FactorPointers &pointers);
    /// declares a column inactive
    void removeColumnFromActSet(const int column, FactorPointers &pointers);
    /// allocates space for U
    void allocateSpaceForU();
    /// allocates several working arrays
    void allocateSomeArrays();
    /// initializes some numbers
    void initialSomeNumbers();
    /// solves L x = b
    void Lxeqb(double *b) const;
    /// same as above but with two rhs
    void Lxeqb2(double *b1, double *b2) const;
    /// solves U x = b
    void Uxeqb(double *b, double *sol) const;
    /// same as above but with two rhs
    void Uxeqb2(double *b1, double *sol1, double *sol2, double *b2) const;
    /// solves x L = b
    void xLeqb(double *b) const;
    /// solves x U = b
    void xUeqb(double *b, double *sol) const;
    /// updates factorization after a Simplex iteration
    int LUupdate(int newBasicCol);
    /// creates a new eta vector
    void newEta(int row, int numNewElements);
    /// makes a copy of row permutations
    void copyRowPermutations();
    /// solves H x = b, where H is a product of eta matrices
    void Hxeqb(double *b) const;
    /// same as above but with two rhs
    void Hxeqb2(double *b1, double *b2) const;
    /// solves x H = b
    void xHeqb(double *b) const;
    /// does FTRAN
    void ftran(double *b, double *sol, bool save) const;
    /// same as above but with two columns
    void ftran2(double *b1, double *sol1, double *b2, double *sol2) const;
    /// does BTRAN
    void btran(double *b, double *sol) const;
   ///---------------------------------------



  //@}
protected:
  /** Returns accuracy status of replaceColumn
      returns 0=OK, 1=Probably OK, 2=singular */
  int checkPivot(double saveFromU, double oldPivot) const;
////////////////// data //////////////////
protected:

  /**@name data */
  //@{
    /// work array (should be initialized to zero)
    double *denseVector_;
    /// work array 
    double *workArea2_; 
    /// work array 
    double *workArea3_;
    /// array of labels (should be initialized to zero)
    int *vecLabels_;
    /// array of indices
    int *indVector_;

    /// auxiliary vector 
    double *auxVector_;
    /// auxiliary vector 
    int *auxInd_;

    /// vector to keep for LUupdate
    double *vecKeep_;
    /// indices of this vector
    int *indKeep_;
    /// number of nonzeros
    mutable int keepSize_;

    

    /// Starts of the rows of L
    int *LrowStarts_;
    /// Lengths of the rows of L
    int *LrowLengths_;
    /// L by rows
    double *Lrows_;
    /// indices in the rows of L
    int *LrowInd_;
    /// Size of Lrows_;
    int LrowSize_; 
    /// Capacity of Lrows_
    int LrowCap_;

    /// Starts of the columns of L
    int *LcolStarts_;
    /// Lengths of the columns of L
    int *LcolLengths_;
    /// L by columns
    double *Lcolumns_;
    /// indices in the columns of L
    int *LcolInd_;
    /// numbers of elements in L
    int LcolSize_;
    /// maximum capacity of L
    int LcolCap_;
   

    /// Starts of the rows of U
    int *UrowStarts_;
    /// Lengths of the rows of U
    int *UrowLengths_;
#ifdef COIN_SIMP_CAPACITY
    /// Capacities of the rows of U
    int *UrowCapacities_;
#endif
    /// U by rows
    double *Urows_;
    /// Indices in the rows of U
    int *UrowInd_;
    /// maximum capacity of Urows
    int UrowMaxCap_;
    /// number of used places in Urows
    int UrowEnd_;
    /// first row in U
    int firstRowInU_;
    /// last row in U
    int lastRowInU_;
    /// previous row in U
    int *prevRowInU_;
    /// next row in U
    int *nextRowInU_;

    /// Starts of the columns of U
    int *UcolStarts_;
    /// Lengths of the columns of U
    int *UcolLengths_;
#ifdef COIN_SIMP_CAPACITY
    /// Capacities of the columns of U
    int *UcolCapacities_;
#endif
    /// U by columns
    double *Ucolumns_;
    /// Indices in the columns of U
    int *UcolInd_;
    /// previous column in U
    int *prevColInU_;
    /// next column in U
    int *nextColInU_;
    /// first column in U
    int firstColInU_;
    /// last column in U
    int lastColInU_;
    /// maximum capacity of Ucolumns_
    int UcolMaxCap_;
    /// last used position in Ucolumns_
    int UcolEnd_;    
    /// indicator of slack variables
    int *colSlack_;
 
    /// inverse values of the elements of diagonal of U
    double *invOfPivots_;

    /// permutation of columns 
    int *colOfU_;
    /// position of column after permutation
    int *colPosition_;
    /// permutations of rows
    int *rowOfU_;
    /// position of row after permutation
    int *rowPosition_;
    /// permutations of rows during LUupdate
    int *secRowOfU_;
    /// position of row after permutation during LUupdate
    int *secRowPosition_;
    
    /// position of Eta vector
    int *EtaPosition_;
    /// Starts of eta vectors
    int *EtaStarts_;
    /// Lengths of eta vectors
    int *EtaLengths_;
    /// columns of eta vectors
    int *EtaInd_;
    /// elements of eta vectors
    double *Eta_;
    /// number of elements in Eta_
    int EtaSize_;
    /// last eta row
    int lastEtaRow_;
    /// maximum number of eta vectors
    int maxEtaRows_;
    /// Capacity of Eta_
    int EtaMaxCap_;
    
    /// minimum storage increase
    int minIncrease_;
    /// maximum size for the diagonal of U after update
    double updateTol_;
    /// do Shul heuristic
    bool doSuhlHeuristic_;
    /// maximum of U
    double maxU_;
    /// bound on the growth rate
    double maxGrowth_;
    /// maximum of A
    double maxA_;
    /// maximum number of candidates for pivot
    int pivotCandLimit_;    
    /// number of slacks in basis
    int numberSlacks_;    
    /// number of slacks in irst basis
    int firstNumberSlacks_;
    //@}
};
#endif
