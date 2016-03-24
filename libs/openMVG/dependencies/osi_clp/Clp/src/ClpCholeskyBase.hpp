/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpCholeskyBase_H
#define ClpCholeskyBase_H

#include "CoinPragma.hpp"
#include "CoinTypes.hpp"
//#define CLP_LONG_CHOLESKY 0
#ifndef CLP_LONG_CHOLESKY
#define CLP_LONG_CHOLESKY 0
#endif
/* valid combinations are
   CLP_LONG_CHOLESKY 0 and COIN_LONG_WORK 0
   CLP_LONG_CHOLESKY 1 and COIN_LONG_WORK 1
   CLP_LONG_CHOLESKY 2 and COIN_LONG_WORK 1
*/
#if COIN_LONG_WORK==0
#if CLP_LONG_CHOLESKY>0
#define CHOLESKY_BAD_COMBINATION
#endif
#else
#if CLP_LONG_CHOLESKY==0
#define CHOLESKY_BAD_COMBINATION
#endif
#endif
#ifdef CHOLESKY_BAD_COMBINATION
#  warning("Bad combination of CLP_LONG_CHOLESKY and COIN_BIG_DOUBLE/COIN_LONG_WORK");
"Bad combination of CLP_LONG_CHOLESKY and COIN_LONG_WORK"
#endif
#if CLP_LONG_CHOLESKY>1
typedef long double longDouble;
#define CHOL_SMALL_VALUE 1.0e-15
#elif CLP_LONG_CHOLESKY==1
typedef double longDouble;
#define CHOL_SMALL_VALUE 1.0e-11
#else
typedef double longDouble;
#define CHOL_SMALL_VALUE 1.0e-11
#endif
class ClpInterior;
class ClpCholeskyDense;
class ClpMatrixBase;

/** Base class for Clp Cholesky factorization
    Will do better factorization.  very crude ordering

    Derived classes may be using more sophisticated methods
*/

class ClpCholeskyBase  {

public:
     /**@name Virtual methods that the derived classes may provide  */
     //@{
     /** Orders rows and saves pointer to matrix.and model.
      returns non-zero if not enough memory.
      You can use preOrder to set up ADAT
      If using default symbolic etc then must set sizeFactor_ to
      size of input matrix to order (and to symbolic).
      Also just permute_ and permuteInverse_ should be created */
     virtual int order(ClpInterior * model);
     /** Does Symbolic factorization given permutation.
         This is called immediately after order.  If user provides this then
         user must provide factorize and solve.  Otherwise the default factorization is used
         returns non-zero if not enough memory */
     virtual int symbolic();
     /** Factorize - filling in rowsDropped and returning number dropped.
         If return code negative then out of memory */
     virtual int factorize(const CoinWorkDouble * diagonal, int * rowsDropped) ;
     /** Uses factorization to solve. */
     virtual void solve (CoinWorkDouble * region) ;
     /** Uses factorization to solve. - given as if KKT.
      region1 is rows+columns, region2 is rows */
     virtual void solveKKT (CoinWorkDouble * region1, CoinWorkDouble * region2, const CoinWorkDouble * diagonal,
                            CoinWorkDouble diagonalScaleFactor);
private:
     /// AMD ordering
     int orderAMD();
public:
     //@}

     /**@name Gets */
     //@{
     /// status.  Returns status
     inline int status() const {
          return status_;
     }
     /// numberRowsDropped.  Number of rows gone
     inline int numberRowsDropped() const {
          return numberRowsDropped_;
     }
     /// reset numberRowsDropped and rowsDropped.
     void resetRowsDropped();
     /// rowsDropped - which rows are gone
     inline char * rowsDropped() const {
          return rowsDropped_;
     }
     /// choleskyCondition.
     inline double choleskyCondition() const {
          return choleskyCondition_;
     }
     /// goDense i.e. use dense factoriaztion if > this (default 0.7).
     inline double goDense() const {
          return goDense_;
     }
     /// goDense i.e. use dense factoriaztion if > this (default 0.7).
     inline void setGoDense(double value) {
          goDense_ = value;
     }
     /// rank.  Returns rank
     inline int rank() const {
          return numberRows_ - numberRowsDropped_;
     }
     /// Return number of rows
     inline int numberRows() const {
          return numberRows_;
     }
     /// Return size
     inline CoinBigIndex size() const {
          return sizeFactor_;
     }
     /// Return sparseFactor
     inline longDouble * sparseFactor() const {
          return sparseFactor_;
     }
     /// Return diagonal
     inline longDouble * diagonal() const {
          return diagonal_;
     }
     /// Return workDouble
     inline longDouble * workDouble() const {
          return workDouble_;
     }
     /// If KKT on
     inline bool kkt() const {
          return doKKT_;
     }
     /// Set KKT
     inline void setKKT(bool yesNo) {
          doKKT_ = yesNo;
     }
     /// Set integer parameter
     inline void setIntegerParameter(int i, int value) {
          integerParameters_[i] = value;
     }
     /// get integer parameter
     inline int getIntegerParameter(int i) {
          return integerParameters_[i];
     }
     /// Set double parameter
     inline void setDoubleParameter(int i, double value) {
          doubleParameters_[i] = value;
     }
     /// get double parameter
     inline double getDoubleParameter(int i) {
          return doubleParameters_[i];
     }
     //@}


public:

     /**@name Constructors, destructor
      */
     //@{
     /** Constructor which has dense columns activated.
         Default is off. */
     ClpCholeskyBase(int denseThreshold = -1);
     /** Destructor (has to be public) */
     virtual ~ClpCholeskyBase();
     /// Copy
     ClpCholeskyBase(const ClpCholeskyBase&);
     /// Assignment
     ClpCholeskyBase& operator=(const ClpCholeskyBase&);
     //@}
     //@{
     ///@name Other
     /// Clone
     virtual ClpCholeskyBase * clone() const;

     /// Returns type
     inline int type() const {
          if (doKKT_) return 100;
          else return type_;
     }
protected:
     /// Sets type
     inline void setType(int type) {
          type_ = type;
     }
     /// model.
     inline void setModel(ClpInterior * model) {
          model_ = model;
     }
     //@}

     /**@name Symbolic, factor and solve */
     //@{
     /** Symbolic1  - works out size without clever stuff.
         Uses upper triangular as much easier.
         Returns size
      */
     int symbolic1(const CoinBigIndex * Astart, const int * Arow);
     /** Symbolic2  - Fills in indices
         Uses lower triangular so can do cliques etc
      */
     void symbolic2(const CoinBigIndex * Astart, const int * Arow);
     /** Factorize - filling in rowsDropped and returning number dropped
         in integerParam.
      */
     void factorizePart2(int * rowsDropped) ;
     /** solve - 1 just first half, 2 just second half - 3 both.
     If 1 and 2 then diagonal has sqrt of inverse otherwise inverse
     */
     void solve(CoinWorkDouble * region, int type);
     /// Forms ADAT - returns nonzero if not enough memory
     int preOrder(bool lowerTriangular, bool includeDiagonal, bool doKKT);
     /// Updates dense part (broken out for profiling)
     void updateDense(longDouble * d, /*longDouble * work,*/ int * first);
     //@}

protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// type (may be useful) if > 20 do KKT
     int type_;
     /// Doing full KKT (only used if default symbolic and factorization)
     bool doKKT_;
     /// Go dense at this fraction
     double goDense_;
     /// choleskyCondition.
     double choleskyCondition_;
     /// model.
     ClpInterior * model_;
     /// numberTrials.  Number of trials before rejection
     int numberTrials_;
     /// numberRows.  Number of Rows in factorization
     int numberRows_;
     /// status.  Status of factorization
     int status_;
     /// rowsDropped
     char * rowsDropped_;
     /// permute inverse.
     int * permuteInverse_;
     /// main permute.
     int * permute_;
     /// numberRowsDropped.  Number of rows gone
     int numberRowsDropped_;
     /// sparseFactor.
     longDouble * sparseFactor_;
     /// choleskyStart - element starts
     CoinBigIndex * choleskyStart_;
     /// choleskyRow (can be shorter than sparsefactor)
     int * choleskyRow_;
     /// Index starts
     CoinBigIndex * indexStart_;
     /// Diagonal
     longDouble * diagonal_;
     /// double work array
     longDouble * workDouble_;
     /// link array
     int * link_;
     // Integer work array
     CoinBigIndex * workInteger_;
     // Clique information
     int * clique_;
     /// sizeFactor.
     CoinBigIndex sizeFactor_;
     /// Size of index array
     CoinBigIndex sizeIndex_;
     /// First dense row
     int firstDense_;
     /// integerParameters
     int integerParameters_[64];
     /// doubleParameters;
     double doubleParameters_[64];
     /// Row copy of matrix
     ClpMatrixBase * rowCopy_;
     /// Dense indicators
     char * whichDense_;
     /// Dense columns (updated)
     longDouble * denseColumn_;
     /// Dense cholesky
     ClpCholeskyDense * dense_;
     /// Dense threshold (for taking out of Cholesky)
     int denseThreshold_;
     //@}
};

#endif
