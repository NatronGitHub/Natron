/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpFactorization_H
#define ClpFactorization_H


#include "CoinPragma.hpp"

#include "CoinFactorization.hpp"
class ClpMatrixBase;
class ClpSimplex;
class ClpNetworkBasis;
class CoinOtherFactorization;
#ifndef CLP_MULTIPLE_FACTORIZATIONS
#define CLP_MULTIPLE_FACTORIZATIONS 4
#endif
#ifdef CLP_MULTIPLE_FACTORIZATIONS
#include "CoinDenseFactorization.hpp"
#include "ClpSimplex.hpp"
#endif
#ifndef COIN_FAST_CODE
#define COIN_FAST_CODE
#endif

/** This just implements CoinFactorization when an ClpMatrixBase object
    is passed.  If a network then has a dummy CoinFactorization and
    a genuine ClpNetworkBasis object
*/
class ClpFactorization
#ifndef CLP_MULTIPLE_FACTORIZATIONS
     : public CoinFactorization
#endif
{

     //friend class CoinFactorization;

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
     int factorize (ClpSimplex * model, int solveType, bool valuesPass);
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpFactorization();
     /** Destructor */
     ~ClpFactorization();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor from an CoinFactorization. */
     ClpFactorization(const CoinFactorization&);
     /** The copy constructor. */
     ClpFactorization(const ClpFactorization&, int denseIfSmaller = 0);
#ifdef CLP_MULTIPLE_FACTORIZATIONS
     /** The copy constructor from an CoinOtherFactorization. */
     ClpFactorization(const CoinOtherFactorization&);
#endif
     ClpFactorization& operator=(const ClpFactorization&);
     //@}

     /*  **** below here is so can use networkish basis */
     /**@name rank one updates which do exist */
     //@{

     /** Replaces one Column to basis,
      returns 0=OK, 1=Probably OK, 2=singular, 3=no room
         If checkBeforeModifying is true will do all accuracy checks
         before modifying factorization.  Whether to set this depends on
         speed considerations.  You could just do this on first iteration
         after factorization and thereafter re-factorize
      partial update already in U */
     int replaceColumn ( const ClpSimplex * model,
                         CoinIndexedVector * regionSparse,
                         CoinIndexedVector * tableauColumn,
                         int pivotRow,
                         double pivotCheck ,
                         bool checkBeforeModifying = false,
                         double acceptablePivot = 1.0e-8);
     //@}

     /**@name various uses of factorization (return code number elements)
      which user may want to know about */
     //@{
     /** Updates one column (FTRAN) from region2
         Tries to do FT update
         number returned is negative if no room
         region1 starts as zero and is zero at end */
     int updateColumnFT ( CoinIndexedVector * regionSparse,
                          CoinIndexedVector * regionSparse2);
     /** Updates one column (FTRAN) from region2
         region1 starts as zero and is zero at end */
     int updateColumn ( CoinIndexedVector * regionSparse,
                        CoinIndexedVector * regionSparse2,
                        bool noPermute = false) const;
     /** Updates one column (FTRAN) from region2
         Tries to do FT update
         number returned is negative if no room.
         Also updates region3
         region1 starts as zero and is zero at end */
     int updateTwoColumnsFT ( CoinIndexedVector * regionSparse1,
                              CoinIndexedVector * regionSparse2,
                              CoinIndexedVector * regionSparse3,
                              bool noPermuteRegion3 = false) ;
     /// For debug (no statistics update)
     int updateColumnForDebug ( CoinIndexedVector * regionSparse,
                                CoinIndexedVector * regionSparse2,
                                bool noPermute = false) const;
     /** Updates one column (BTRAN) from region2
         region1 starts as zero and is zero at end */
     int updateColumnTranspose ( CoinIndexedVector * regionSparse,
                                 CoinIndexedVector * regionSparse2) const;
     //@}
#ifdef CLP_MULTIPLE_FACTORIZATIONS
     /**@name Lifted from CoinFactorization */
     //@{
     /// Total number of elements in factorization
     inline int numberElements (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->numberElements();
          else return coinFactorizationB_->numberElements() ;
     }
     /// Returns address of permute region
     inline int *permute (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->permute();
          else return coinFactorizationB_->permute() ;
     }
     /// Returns address of pivotColumn region (also used for permuting)
     inline int *pivotColumn (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->pivotColumn();
          else return coinFactorizationB_->permute() ;
     }
     /// Maximum number of pivots between factorizations
     inline int maximumPivots (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->maximumPivots();
          else return coinFactorizationB_->maximumPivots() ;
     }
     /// Set maximum number of pivots between factorizations
     inline void maximumPivots (  int value) {
          if (coinFactorizationA_) coinFactorizationA_->maximumPivots(value);
          else coinFactorizationB_->maximumPivots(value);
     }
     /// Returns number of pivots since factorization
     inline int pivots (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->pivots();
          else return coinFactorizationB_->pivots() ;
     }
     /// Whether larger areas needed
     inline double areaFactor (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->areaFactor();
          else return 0.0 ;
     }
     /// Set whether larger areas needed
     inline void areaFactor ( double value) {
          if (coinFactorizationA_) coinFactorizationA_->areaFactor(value);
     }
     /// Zero tolerance
     inline double zeroTolerance (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->zeroTolerance();
          else return coinFactorizationB_->zeroTolerance() ;
     }
     /// Set zero tolerance
     inline void zeroTolerance (  double value) {
          if (coinFactorizationA_) coinFactorizationA_->zeroTolerance(value);
          else coinFactorizationB_->zeroTolerance(value);
     }
     /// Set tolerances to safer of existing and given
     void saferTolerances (  double zeroTolerance, double pivotTolerance);
     /**  get sparse threshold */
     inline int sparseThreshold ( ) const {
          if (coinFactorizationA_) return coinFactorizationA_->sparseThreshold();
          else return 0 ;
     }
     /**  Set sparse threshold */
     inline void sparseThreshold ( int value) {
          if (coinFactorizationA_) coinFactorizationA_->sparseThreshold(value);
     }
     /// Returns status
     inline int status (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->status();
          else return coinFactorizationB_->status() ;
     }
     /// Sets status
     inline void setStatus (  int value) {
          if (coinFactorizationA_) coinFactorizationA_->setStatus(value);
          else coinFactorizationB_->setStatus(value) ;
     }
     /// Returns number of dense rows
     inline int numberDense() const {
          if (coinFactorizationA_) return coinFactorizationA_->numberDense();
          else return 0 ;
     }
#if 1
     /// Returns number in U area
     inline CoinBigIndex numberElementsU (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->numberElementsU();
          else return -1 ;
     }
     /// Returns number in L area
     inline CoinBigIndex numberElementsL (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->numberElementsL();
          else return -1 ;
     }
     /// Returns number in R area
     inline CoinBigIndex numberElementsR (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->numberElementsR();
          else return 0 ;
     }
#endif
     inline bool timeToRefactorize() const {
          if (coinFactorizationA_) {
               return (coinFactorizationA_->pivots() * 3 > coinFactorizationA_->maximumPivots() * 2 &&
                       coinFactorizationA_->numberElementsR() * 3 > (coinFactorizationA_->numberElementsL() +
                                 coinFactorizationA_->numberElementsU()) * 2 + 1000 &&
                       !coinFactorizationA_->numberDense());
          } else {
               return coinFactorizationB_->pivots() > coinFactorizationB_->numberRows() / 2.45 + 20;
          }
     }
     /// Level of detail of messages
     inline int messageLevel (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->messageLevel();
          else return 1 ;
     }
     /// Set level of detail of messages
     inline void messageLevel (  int value) {
          if (coinFactorizationA_) coinFactorizationA_->messageLevel(value);
     }
     /// Get rid of all memory
     inline void clearArrays() {
          if (coinFactorizationA_)
               coinFactorizationA_->clearArrays();
          else if (coinFactorizationB_)
               coinFactorizationB_->clearArrays();
     }
     /// Number of Rows after factorization
     inline int numberRows (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->numberRows();
          else return coinFactorizationB_->numberRows() ;
     }
     /// Gets dense threshold
     inline int denseThreshold() const {
          if (coinFactorizationA_) return coinFactorizationA_->denseThreshold();
          else return 0 ;
     }
     /// Sets dense threshold
     inline void setDenseThreshold(int value) {
          if (coinFactorizationA_) coinFactorizationA_->setDenseThreshold(value);
     }
     /// Pivot tolerance
     inline double pivotTolerance (  ) const {
          if (coinFactorizationA_) return coinFactorizationA_->pivotTolerance();
          else if (coinFactorizationB_) return coinFactorizationB_->pivotTolerance();
          return 1.0e-8 ;
     }
     /// Set pivot tolerance
     inline void pivotTolerance (  double value) {
          if (coinFactorizationA_) coinFactorizationA_->pivotTolerance(value);
          else if (coinFactorizationB_) coinFactorizationB_->pivotTolerance(value);
     }
     /// Allows change of pivot accuracy check 1.0 == none >1.0 relaxed
     inline void relaxAccuracyCheck(double value) {
          if (coinFactorizationA_) coinFactorizationA_->relaxAccuracyCheck(value);
     }
     /** Array persistence flag
         If 0 then as now (delete/new)
         1 then only do arrays if bigger needed
         2 as 1 but give a bit extra if bigger needed
     */
     inline int persistenceFlag() const {
          if (coinFactorizationA_) return coinFactorizationA_->persistenceFlag();
          else return 0 ;
     }
     inline void setPersistenceFlag(int value) {
          if (coinFactorizationA_) coinFactorizationA_->setPersistenceFlag(value);
     }
     /// Delete all stuff (leaves as after CoinFactorization())
     inline void almostDestructor() {
          if (coinFactorizationA_)
               coinFactorizationA_->almostDestructor();
          else if (coinFactorizationB_)
               coinFactorizationB_->clearArrays();
     }
     /// Returns areaFactor but adjusted for dense
     inline double adjustedAreaFactor() const {
          if (coinFactorizationA_) return coinFactorizationA_->adjustedAreaFactor();
          else return 0.0 ;
     }
     inline void setBiasLU(int value) {
          if (coinFactorizationA_) coinFactorizationA_->setBiasLU(value);
     }
     /// true if Forrest Tomlin update, false if PFI
     inline void setForrestTomlin(bool value) {
          if (coinFactorizationA_) coinFactorizationA_->setForrestTomlin(value);
     }
     /// Sets default values
     inline void setDefaultValues() {
          if (coinFactorizationA_) {
               // row activities have negative sign
#ifndef COIN_FAST_CODE
               coinFactorizationA_->slackValue(-1.0);
#endif
               coinFactorizationA_->zeroTolerance(1.0e-13);
          }
     }
     /// If nonzero force use of 1,dense 2,small 3,osl
     void forceOtherFactorization(int which);
     /// Get switch to osl if number rows <= this
     inline int goOslThreshold() const {
          return goOslThreshold_;
     }
     /// Set switch to osl if number rows <= this
     inline void setGoOslThreshold(int value) {
          goOslThreshold_ = value;
     }
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
     /// Go over to dense or small code if small enough
     void goDenseOrSmall(int numberRows) ;
     /// Sets factorization
     void setFactorization(ClpFactorization & factorization);
     /// Return 1 if dense code
     inline int isDenseOrSmall() const {
          return coinFactorizationB_ ? 1 : 0;
     }
#else
     inline bool timeToRefactorize() const {
          return (pivots() * 3 > maximumPivots() * 2 &&
                  numberElementsR() * 3 > (numberElementsL() + numberElementsU()) * 2 + 1000 &&
                  !numberDense());
     }
     /// Sets default values
     inline void setDefaultValues() {
          // row activities have negative sign
#ifndef COIN_FAST_CODE
          slackValue(-1.0);
#endif
          zeroTolerance(1.0e-13);
     }
     /// Go over to dense code
     inline void goDense() {}
#endif
     //@}

     /**@name other stuff */
     //@{
     /** makes a row copy of L for speed and to allow very sparse problems */
     void goSparse();
     /// Cleans up i.e. gets rid of network basis
     void cleanUp();
     /// Says whether to redo pivot order
     bool needToReorder() const;
#ifndef SLIM_CLP
     /// Says if a network basis
     inline bool networkBasis() const {
          return (networkBasis_ != NULL);
     }
#else
     /// Says if a network basis
     inline bool networkBasis() const {
          return false;
     }
#endif
     /// Fills weighted row list
     void getWeights(int * weights) const;
     //@}

////////////////// data //////////////////
private:

     /**@name data */
     //@{
     /// Pointer to network basis
#ifndef SLIM_CLP
     ClpNetworkBasis * networkBasis_;
#endif
#ifdef CLP_MULTIPLE_FACTORIZATIONS
     /// Pointer to CoinFactorization
     CoinFactorization * coinFactorizationA_;
     /// Pointer to CoinOtherFactorization
     CoinOtherFactorization * coinFactorizationB_;
#ifdef CLP_REUSE_ETAS
     /// Pointer to model
     ClpSimplex * model_;
#endif
     /// If nonzero force use of 1,dense 2,small 3,osl
     int forceB_;
     /// Switch to osl if number rows <= this
     int goOslThreshold_;
     /// Switch to small if number rows <= this
     int goSmallThreshold_;
     /// Switch to dense if number rows <= this
     int goDenseThreshold_;
#endif
     //@}
};

#endif
