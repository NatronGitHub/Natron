/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef AbcSimplexFactorization_H
#define AbcSimplexFactorization_H


#include "CoinPragma.hpp"

#include "CoinAbcCommon.hpp"
#include "CoinAbcFactorization.hpp"
#include "AbcMatrix.hpp"
//#include "CoinAbcAnyFactorization.hpp"
#include "AbcSimplex.hpp"
class ClpFactorization;

/** This just implements AbcFactorization when an AbcMatrix object
    is passed. 
*/
class AbcSimplexFactorization
{
  
public:
  /**@name factorization */
  //@{
  /** When part of LP - given by basic variables.
      Actually does factorization.
      Arrays passed in have non negative value to say basic.
      If status is okay, basic variables have pivot row - this is only needed
      if increasingRows_ >1.
      Allows scaling
      If status is singular, then basic variables have pivot row
      and ones thrown out have -1
      returns 0 -okay, -1 singular, -2 too many in basis, -99 memory */
  int factorize (AbcSimplex * model, int solveType, bool valuesPass);
#ifdef EARLY_FACTORIZE
  /// Returns -2 if can't, -1 if singular, -99 memory, 0 OK
  inline int factorize (AbcSimplex * model, CoinIndexedVector & stuff)
  { return coinAbcFactorization_->factorize(model,stuff);}
#endif
  //@}
  
  
  /**@name Constructors, destructor */
  //@{
  /** Default constructor. */
  AbcSimplexFactorization(int numberRows=0);
  /** Destructor */
  ~AbcSimplexFactorization();
  //@}
  
  /**@name Copy method */
  //@{
  /** The copy constructor. */
  AbcSimplexFactorization(const AbcSimplexFactorization&, int denseIfSmaller = 0);
  AbcSimplexFactorization& operator=(const AbcSimplexFactorization&);
  /// Sets factorization
  void setFactorization(AbcSimplexFactorization & rhs);
  //@}
  
  /*  **** below here is so can use networkish basis */
  /**@name rank one updates which do exist */
  //@{
  
  /** Checks if can replace one Column to basis,
      returns update alpha
      Fills in region for use later
      partial update already in U */
  inline
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
  double checkReplacePart1 ( CoinIndexedVector * regionSparse,
			     int pivotRow)
  {return coinAbcFactorization_->checkReplacePart1(regionSparse,pivotRow);}
  /** Checks if can replace one Column to basis,
      returns update alpha
      Fills in region for use later
      partial update in vector */
  inline 
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
  double checkReplacePart1 ( CoinIndexedVector * regionSparse,
				  CoinIndexedVector * partialUpdate,
				  int pivotRow)
  {return coinAbcFactorization_->checkReplacePart1(regionSparse,partialUpdate,pivotRow);}
  /** Checks if can replace one Column to basis,
      returns update alpha
      Fills in region for use later
      partial update already in U */
  inline void checkReplacePart1a ( CoinIndexedVector * regionSparse,
			     int pivotRow)
  {coinAbcFactorization_->checkReplacePart1a(regionSparse,pivotRow);}
  inline double checkReplacePart1b (CoinIndexedVector * regionSparse,
			     int pivotRow)
  {return coinAbcFactorization_->checkReplacePart1b(regionSparse,pivotRow);}
  /** Checks if can replace one Column to basis,
      returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots */
  inline int checkReplacePart2 ( int pivotRow,
				 double btranAlpha, 
				 double ftranAlpha, 
#ifdef ABC_LONG_FACTORIZATION 
				 long
#endif
				 double ftAlpha)
  {return coinAbcFactorization_->checkReplacePart2(pivotRow,btranAlpha,ftranAlpha,ftAlpha);}
#ifdef ABC_LONG_FACTORIZATION 
  /// Clear all hidden arrays
  inline void clearHiddenArrays()
  { coinAbcFactorization_->clearHiddenArrays();}
#endif
  /** Replaces one Column to basis,
      partial update already in U */
  void replaceColumnPart3 ( const AbcSimplex * model,
			    CoinIndexedVector * regionSparse,
			    CoinIndexedVector * tableauColumn,
			    int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
			    long
#endif
			    double alpha );
  /** Replaces one Column to basis,
      partial update in vector */
  void replaceColumnPart3 ( const AbcSimplex * model,
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
  inline int replaceColumns ( const AbcSimplex * model,
			CoinIndexedVector & stuff,
			      int firstPivot,int lastPivot,bool cleanUp)
  { return coinAbcFactorization_->replaceColumns(model,stuff,firstPivot,lastPivot,cleanUp);}
#endif
  //@}
  
  /**@name various uses of factorization (return code number elements)
     which user may want to know about */
  //@{
#if 0
  /** Updates one column (FTRAN) from region2
      Tries to do FT update
      number returned is negative if no room
      region1 starts as zero and is zero at end */
  int updateColumnFT ( CoinIndexedVector * regionSparse,
		       CoinIndexedVector * regionSparse2);
  /** Updates one column (FTRAN) from region2
      region1 starts as zero and is zero at end */
  int updateColumn ( CoinIndexedVector * regionSparse,
		     CoinIndexedVector * regionSparse2) const;
  /** Updates one column (FTRAN) from region2
      Tries to do FT update
      number returned is negative if no room.
      Also updates region3
      region1 starts as zero and is zero at end */
  int updateTwoColumnsFT ( CoinIndexedVector * regionSparse1,
			   CoinIndexedVector * regionSparse2,
			   CoinIndexedVector * regionSparse3) ;
  /** Updates one column (BTRAN) from region2
      region1 starts as zero and is zero at end */
  int updateColumnTranspose ( CoinIndexedVector * regionSparse,
			      CoinIndexedVector * regionSparse2) const;
#endif
  /** Updates one column (FTRAN)
      Tries to do FT update
      number returned is negative if no room */
  inline int updateColumnFT ( CoinIndexedVector & regionSparseFT)
  { return coinAbcFactorization_->updateColumnFT(regionSparseFT);}
  inline int updateColumnFTPart1 ( CoinIndexedVector & regionSparseFT)
  { return coinAbcFactorization_->updateColumnFTPart1(regionSparseFT);}
  inline void updateColumnFTPart2 ( CoinIndexedVector & regionSparseFT)
  { coinAbcFactorization_->updateColumnFTPart2(regionSparseFT);}
  /** Updates one column (FTRAN)
      Tries to do FT update
      puts partial update in vector */
  inline void updateColumnFT ( CoinIndexedVector & regionSparseFT,
			       CoinIndexedVector & partialUpdate,
			       int which)
  { coinAbcFactorization_->updateColumnFT(regionSparseFT,partialUpdate,which);}
  /** Updates one column (FTRAN) */
  inline int updateColumn ( CoinIndexedVector & regionSparse) const
  { return coinAbcFactorization_->updateColumn(regionSparse);}
  /** Updates one column (FTRAN) from regionFT
      Tries to do FT update
      number returned is negative if no room.
      Also updates regionOther */
  inline int updateTwoColumnsFT ( CoinIndexedVector & regionSparseFT,
			   CoinIndexedVector & regionSparseOther)
  { return coinAbcFactorization_->updateTwoColumnsFT(regionSparseFT,regionSparseOther);}
  /** Updates one column (BTRAN) */
  inline int updateColumnTranspose ( CoinIndexedVector & regionSparse) const
  { return coinAbcFactorization_->updateColumnTranspose(regionSparse);}
  /** Updates one column (FTRAN) */
  inline void updateColumnCpu ( CoinIndexedVector & regionSparse,int whichCpu) const
  { coinAbcFactorization_->updateColumnCpu(regionSparse,whichCpu);}
  /** Updates one column (BTRAN) */
  inline void updateColumnTransposeCpu ( CoinIndexedVector & regionSparse,int whichCpu) const
  { coinAbcFactorization_->updateColumnTransposeCpu(regionSparse,whichCpu);}
  /** Updates one full column (FTRAN) */
  inline void updateFullColumn ( CoinIndexedVector & regionSparse) const
  { coinAbcFactorization_->updateFullColumn(regionSparse);}
  /** Updates one full column (BTRAN) */
  inline void updateFullColumnTranspose ( CoinIndexedVector & regionSparse) const
  { coinAbcFactorization_->updateFullColumnTranspose(regionSparse);}
  /** Updates one column for dual steepest edge weights (FTRAN) */
  void updateWeights ( CoinIndexedVector & regionSparse) const
  { coinAbcFactorization_->updateWeights(regionSparse);}
  //@}
  /**@name Lifted from CoinFactorization */
  //@{
  /// Total number of elements in factorization
  inline int numberElements (  ) const {
    return coinAbcFactorization_->numberElements() ;
  }
  /// Maximum number of pivots between factorizations
  inline int maximumPivots (  ) const {
    return coinAbcFactorization_->maximumPivots() ;
  }
  /// Set maximum number of pivots between factorizations
  inline void maximumPivots (  int value) {
    coinAbcFactorization_->maximumPivots(value);
  }
  /// Returns true if doing FT
  inline bool usingFT() const
  { return !coinAbcFactorization_->wantsTableauColumn();}
  /// Returns number of pivots since factorization
  inline int pivots (  ) const {
    return coinAbcFactorization_->pivots() ;
  }
  /// Sets number of pivots since factorization 
  inline void setPivots ( int value ) const {
    coinAbcFactorization_->setPivots(value) ;
  }
  /// Whether larger areas needed
  inline double areaFactor (  ) const {
    return coinAbcFactorization_->areaFactor() ;
  }
  /// Set whether larger areas needed
  inline void areaFactor ( double value) {
    coinAbcFactorization_->areaFactor(value);
  }
  /// Zero tolerance
  inline double zeroTolerance (  ) const {
    return coinAbcFactorization_->zeroTolerance() ;
  }
  /// Set zero tolerance
  inline void zeroTolerance (  double value) {
    coinAbcFactorization_->zeroTolerance(value);
  }
  /// Set tolerances to safer of existing and given
  void saferTolerances (  double zeroTolerance, double pivotTolerance);
  /// Returns status
  inline int status (  ) const {
    return coinAbcFactorization_->status() ;
  }
  /// Sets status
  inline void setStatus (  int value) {
    coinAbcFactorization_->setStatus(value) ;
  }
#if ABC_PARALLEL==2
  /// Says parallel
  inline void setParallelMode(int value)
  {coinAbcFactorization_->setParallelMode(value);};
#endif
  /// Returns number of dense rows
  inline int numberDense() const {
    return coinAbcFactorization_->numberDense() ;
  }
  inline bool timeToRefactorize() const {
    return coinAbcFactorization_->pivots() > coinAbcFactorization_->numberRows() / 2.45 + 20;
  }
  /// Get rid of all memory
  inline void clearArrays() {
    coinAbcFactorization_->clearArrays();
  }
  /// Number of Rows after factorization
  inline int numberRows (  ) const {
    return coinAbcFactorization_->numberRows() ;
  }
  /// Number of slacks at last factorization
  inline int numberSlacks() const
  { return numberSlacks_;}
  /// Pivot tolerance
  inline double pivotTolerance (  ) const {
    return coinAbcFactorization_->pivotTolerance();
  }
  /// Set pivot tolerance
  inline void pivotTolerance (  double value) {
    coinAbcFactorization_->pivotTolerance(value);
  }
  /// Minimum pivot tolerance
  inline double minimumPivotTolerance (  ) const {
    return coinAbcFactorization_->minimumPivotTolerance();
  }
  /// Set minimum pivot tolerance
  inline void minimumPivotTolerance (  double value) {
    coinAbcFactorization_->minimumPivotTolerance(value);
  }
  /// pivot region
  inline double * pivotRegion() const
  { return coinAbcFactorization_->pivotRegion();}
  /// Allows change of pivot accuracy check 1.0 == none >1.0 relaxed
  //inline void relaxAccuracyCheck(double /*value*/) {
  //abort();
  //}
  /// Delete all stuff (leaves as after CoinFactorization())
  inline void almostDestructor() {
    coinAbcFactorization_->clearArrays();
  }
  /// So we can temporarily switch off dense
  void setDenseThreshold(int number);
  int getDenseThreshold() const;
  /// If nonzero force use of 1,dense 2,small 3,long
  void forceOtherFactorization(int which);
  /// Go over to dense code
  void goDenseOrSmall(int numberRows);
  /// Get switch to dense if number rows <= this
  inline int goDenseThreshold() const {
    return goDenseThreshold_;
  }
  /// Set switch to dense if number rows <= this
  inline void setGoDenseThreshold(int value) {
    goDenseThreshold_ = value;
  }
  /// Get switch to small if number rows <= this
  inline int goSmallThreshold() const {
    return goSmallThreshold_;
  }
  /// Set switch to small if number rows <= this
  inline void setGoSmallThreshold(int value) {
    goSmallThreshold_ = value;
  }
  /// Get switch to long/ordered if number rows >= this
  inline int goLongThreshold() const {
    return goLongThreshold_;
  }
  /// Set switch to long/ordered if number rows >= this
  inline void setGoLongThreshold(int value) {
    goLongThreshold_ = value;
  }
  /// Returns type
  inline int typeOfFactorization() const
  { return forceB_;}
  /// Synchronize stuff
  void synchronize(const ClpFactorization * otherFactorization,const AbcSimplex * model);
  //@}
  
  /**@name other stuff */
  //@{
  /** makes a row copy of L for speed and to allow very sparse problems */
  void goSparse();
#ifndef NDEBUG
  inline void checkMarkArrays() const
  { coinAbcFactorization_->checkMarkArrays();}
#endif
  /// Says whether to redo pivot order
  inline bool needToReorder() const {abort();return true;}
  /// Pointer to factorization
  CoinAbcAnyFactorization * factorization() const
  { return coinAbcFactorization_;}
  //@}
  
  ////////////////// data //////////////////
private:
  
  /**@name data */
  //@{
  /// Pointer to model
  AbcSimplex * model_;
  /// Pointer to factorization
  CoinAbcAnyFactorization * coinAbcFactorization_;
  /// If nonzero force use of 1,dense 2,small 3,long
  int forceB_;
  /// Switch to dense if number rows <= this
  int goDenseThreshold_;
  /// Switch to small if number rows <= this
  int goSmallThreshold_;
  /// Switch to long/ordered if number rows >= this
  int goLongThreshold_;
  /// Number of slacks at last factorization
  int numberSlacks_;
  //@}
};

#endif
