/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Tomlin (pdco)
   John Forrest (standard predictor-corrector)

   Note JJF has added arrays - this takes more memory but makes
   flow easier to understand and hopefully easier to extend

 */
#ifndef ClpInterior_H
#define ClpInterior_H

#include <iostream>
#include <cfloat>
#include "ClpModel.hpp"
#include "ClpMatrixBase.hpp"
#include "ClpSolve.hpp"
#include "CoinDenseVector.hpp"
class ClpLsqr;
class ClpPdcoBase;
/// ******** DATA to be moved into protected section of ClpInterior
typedef struct {
     double  atolmin;
     double  r3norm;
     double  LSdamp;
     double* deltay;
} Info;
/// ******** DATA to be moved into protected section of ClpInterior

typedef struct {
     double  atolold;
     double  atolnew;
     double  r3ratio;
     int   istop;
     int   itncg;
} Outfo;
/// ******** DATA to be moved into protected section of ClpInterior

typedef struct {
     double  gamma;
     double  delta;
     int MaxIter;
     double  FeaTol;
     double  OptTol;
     double  StepTol;
     double  x0min;
     double  z0min;
     double  mu0;
     int   LSmethod;   // 1=Cholesky    2=QR    3=LSQR
     int   LSproblem;  // See below
     int LSQRMaxIter;
     double  LSQRatol1; // Initial  atol
     double  LSQRatol2; // Smallest atol (unless atol1 is smaller)
     double  LSQRconlim;
     int  wait;
} Options;
class Lsqr;
class ClpCholeskyBase;
// ***** END
/** This solves LPs using interior point methods

    It inherits from ClpModel and all its arrays are created at
    algorithm time.

*/

class ClpInterior : public ClpModel {
     friend void ClpInteriorUnitTest(const std::string & mpsDir,
                                     const std::string & netlibDir);

public:

     /**@name Constructors and destructor and copy */
     //@{
     /// Default constructor
     ClpInterior (  );

     /// Copy constructor.
     ClpInterior(const ClpInterior &);
     /// Copy constructor from model.
     ClpInterior(const ClpModel &);
     /** Subproblem constructor.  A subset of whole model is created from the
         row and column lists given.  The new order is given by list order and
         duplicates are allowed.  Name and integer information can be dropped
     */
     ClpInterior (const ClpModel * wholeModel,
                  int numberRows, const int * whichRows,
                  int numberColumns, const int * whichColumns,
                  bool dropNames = true, bool dropIntegers = true);
     /// Assignment operator. This copies the data
     ClpInterior & operator=(const ClpInterior & rhs);
     /// Destructor
     ~ClpInterior (  );
     // Ones below are just ClpModel with some changes
     /** Loads a problem (the constraints on the
           rows are given by lower and upper bounds). If a pointer is 0 then the
           following values are the default:
           <ul>
             <li> <code>colub</code>: all columns have upper bound infinity
             <li> <code>collb</code>: all columns have lower bound 0
             <li> <code>rowub</code>: all rows have upper bound infinity
             <li> <code>rowlb</code>: all rows have lower bound -infinity
         <li> <code>obj</code>: all variables have 0 objective coefficient
           </ul>
       */
     void loadProblem (  const ClpMatrixBase& matrix,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     void loadProblem (  const CoinPackedMatrix& matrix,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);

     /** Just like the other loadProblem() method except that the matrix is
       given in a standard column major ordered format (without gaps). */
     void loadProblem (  const int numcols, const int numrows,
                         const CoinBigIndex* start, const int* index,
                         const double* value,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     /// This one is for after presolve to save memory
     void loadProblem (  const int numcols, const int numrows,
                         const CoinBigIndex* start, const int* index,
                         const double* value, const int * length,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     /// Read an mps file from the given filename
     int readMps(const char *filename,
                 bool keepNames = false,
                 bool ignoreErrors = false);
     /** Borrow model.  This is so we dont have to copy large amounts
         of data around.  It assumes a derived class wants to overwrite
         an empty model with a real one - while it does an algorithm.
         This is same as ClpModel one. */
     void borrowModel(ClpModel & otherModel);
     /** Return model - updates any scalars */
     void returnModel(ClpModel & otherModel);
     //@}

     /**@name Functions most useful to user */
     //@{
     /** Pdco algorithm - see ClpPdco.hpp for method */
     int pdco();
     // ** Temporary version
     int  pdco( ClpPdcoBase * stuff, Options &options, Info &info, Outfo &outfo);
     /// Primal-Dual Predictor-Corrector barrier
     int primalDual();
     //@}

     /**@name most useful gets and sets */
     //@{
     /// If problem is primal feasible
     inline bool primalFeasible() const {
          return (sumPrimalInfeasibilities_ <= 1.0e-5);
     }
     /// If problem is dual feasible
     inline bool dualFeasible() const {
          return (sumDualInfeasibilities_ <= 1.0e-5);
     }
     /// Current (or last) algorithm
     inline int algorithm() const {
          return algorithm_;
     }
     /// Set algorithm
     inline void setAlgorithm(int value) {
          algorithm_ = value;
     }
     /// Sum of dual infeasibilities
     inline CoinWorkDouble sumDualInfeasibilities() const {
          return sumDualInfeasibilities_;
     }
     /// Sum of primal infeasibilities
     inline CoinWorkDouble sumPrimalInfeasibilities() const {
          return sumPrimalInfeasibilities_;
     }
     /// dualObjective.
     inline CoinWorkDouble dualObjective() const {
          return dualObjective_;
     }
     /// primalObjective.
     inline CoinWorkDouble primalObjective() const {
          return primalObjective_;
     }
     /// diagonalNorm
     inline CoinWorkDouble diagonalNorm() const {
          return diagonalNorm_;
     }
     /// linearPerturbation
     inline CoinWorkDouble linearPerturbation() const {
          return linearPerturbation_;
     }
     inline void setLinearPerturbation(CoinWorkDouble value) {
          linearPerturbation_ = value;
     }
     /// projectionTolerance
     inline CoinWorkDouble projectionTolerance() const {
          return projectionTolerance_;
     }
     inline void setProjectionTolerance(CoinWorkDouble value) {
          projectionTolerance_ = value;
     }
     /// diagonalPerturbation
     inline CoinWorkDouble diagonalPerturbation() const {
          return diagonalPerturbation_;
     }
     inline void setDiagonalPerturbation(CoinWorkDouble value) {
          diagonalPerturbation_ = value;
     }
     /// gamma
     inline CoinWorkDouble gamma() const {
          return gamma_;
     }
     inline void setGamma(CoinWorkDouble value) {
          gamma_ = value;
     }
     /// delta
     inline CoinWorkDouble delta() const {
          return delta_;
     }
     inline void setDelta(CoinWorkDouble value) {
          delta_ = value;
     }
     /// ComplementarityGap
     inline CoinWorkDouble complementarityGap() const {
          return complementarityGap_;
     }
     //@}

     /**@name most useful gets and sets */
     //@{
     /// Largest error on Ax-b
     inline CoinWorkDouble largestPrimalError() const {
          return largestPrimalError_;
     }
     /// Largest error on basic duals
     inline CoinWorkDouble largestDualError() const {
          return largestDualError_;
     }
     /// Maximum iterations
     inline int maximumBarrierIterations() const {
          return maximumBarrierIterations_;
     }
     inline void setMaximumBarrierIterations(int value) {
          maximumBarrierIterations_ = value;
     }
     /// Set cholesky (and delete present one)
     void setCholesky(ClpCholeskyBase * cholesky);
     /// Return number fixed to see if worth presolving
     int numberFixed() const;
     /** fix variables interior says should be.  If reallyFix false then just
         set values to exact bounds */
     void fixFixed(bool reallyFix = true);
     /// Primal erturbation vector
     inline CoinWorkDouble * primalR() const {
          return primalR_;
     }
     /// Dual erturbation vector
     inline CoinWorkDouble * dualR() const {
          return dualR_;
     }
     //@}

protected:
     /**@name protected methods */
     //@{
     /// Does most of deletion
     void gutsOfDelete();
     /// Does most of copying
     void gutsOfCopy(const ClpInterior & rhs);
     /// Returns true if data looks okay, false if not
     bool createWorkingData();
     void deleteWorkingData();
     /// Sanity check on input rim data
     bool sanityCheck();
     ///  This does housekeeping
     int housekeeping();
     //@}
public:
     /**@name public methods */
     //@{
     /// Raw objective value (so always minimize)
     inline CoinWorkDouble rawObjectiveValue() const {
          return objectiveValue_;
     }
     /// Returns 1 if sequence indicates column
     inline int isColumn(int sequence) const {
          return sequence < numberColumns_ ? 1 : 0;
     }
     /// Returns sequence number within section
     inline int sequenceWithin(int sequence) const {
          return sequence < numberColumns_ ? sequence : sequence - numberColumns_;
     }
     /// Checks solution
     void checkSolution();
     /** Modifies djs to allow for quadratic.
         returns quadratic offset */
     CoinWorkDouble quadraticDjs(CoinWorkDouble * djRegion, const CoinWorkDouble * solution,
                                 CoinWorkDouble scaleFactor);

     /// To say a variable is fixed
     inline void setFixed( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] | 1) ;
     }
     inline void clearFixed( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~1) ;
     }
     inline bool fixed(int sequence) const {
          return ((status_[sequence] & 1) != 0);
     }

     /// To flag a variable
     inline void setFlagged( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] | 2) ;
     }
     inline void clearFlagged( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~2) ;
     }
     inline bool flagged(int sequence) const {
          return ((status_[sequence] & 2) != 0);
     }

     /// To say a variable is fixed OR free
     inline void setFixedOrFree( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] | 4) ;
     }
     inline void clearFixedOrFree( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~4) ;
     }
     inline bool fixedOrFree(int sequence) const {
          return ((status_[sequence] & 4) != 0);
     }

     /// To say a variable has lower bound
     inline void setLowerBound( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] | 8) ;
     }
     inline void clearLowerBound( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~8) ;
     }
     inline bool lowerBound(int sequence) const {
          return ((status_[sequence] & 8) != 0);
     }

     /// To say a variable has upper bound
     inline void setUpperBound( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] | 16) ;
     }
     inline void clearUpperBound( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~16) ;
     }
     inline bool upperBound(int sequence) const {
          return ((status_[sequence] & 16) != 0);
     }

     /// To say a variable has fake lower bound
     inline void setFakeLower( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] | 32) ;
     }
     inline void clearFakeLower( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~32) ;
     }
     inline bool fakeLower(int sequence) const {
          return ((status_[sequence] & 32) != 0);
     }

     /// To say a variable has fake upper bound
     inline void setFakeUpper( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] | 64) ;
     }
     inline void clearFakeUpper( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~64) ;
     }
     inline bool fakeUpper(int sequence) const {
          return ((status_[sequence] & 64) != 0);
     }
     //@}

////////////////// data //////////////////
protected:

     /**@name data.  Many arrays have a row part and a column part.
      There is a single array with both - columns then rows and
      then normally two arrays pointing to rows and columns.  The
      single array is the owner of memory
     */
     //@{
     /// Largest error on Ax-b
     CoinWorkDouble largestPrimalError_;
     /// Largest error on basic duals
     CoinWorkDouble largestDualError_;
     /// Sum of dual infeasibilities
     CoinWorkDouble sumDualInfeasibilities_;
     /// Sum of primal infeasibilities
     CoinWorkDouble sumPrimalInfeasibilities_;
     /// Worst complementarity
     CoinWorkDouble worstComplementarity_;
     ///
public:
     CoinWorkDouble xsize_;
     CoinWorkDouble zsize_;
protected:
     /// Working copy of lower bounds (Owner of arrays below)
     CoinWorkDouble * lower_;
     /// Row lower bounds - working copy
     CoinWorkDouble * rowLowerWork_;
     /// Column lower bounds - working copy
     CoinWorkDouble * columnLowerWork_;
     /// Working copy of upper bounds (Owner of arrays below)
     CoinWorkDouble * upper_;
     /// Row upper bounds - working copy
     CoinWorkDouble * rowUpperWork_;
     /// Column upper bounds - working copy
     CoinWorkDouble * columnUpperWork_;
     /// Working copy of objective
     CoinWorkDouble * cost_;
public:
     /// Rhs
     CoinWorkDouble * rhs_;
     CoinWorkDouble * x_;
     CoinWorkDouble * y_;
     CoinWorkDouble * dj_;
protected:
     /// Pointer to Lsqr object
     ClpLsqr * lsqrObject_;
     /// Pointer to stuff
     ClpPdcoBase * pdcoStuff_;
     /// Below here is standard barrier stuff
     /// mu.
     CoinWorkDouble mu_;
     /// objectiveNorm.
     CoinWorkDouble objectiveNorm_;
     /// rhsNorm.
     CoinWorkDouble rhsNorm_;
     /// solutionNorm.
     CoinWorkDouble solutionNorm_;
     /// dualObjective.
     CoinWorkDouble dualObjective_;
     /// primalObjective.
     CoinWorkDouble primalObjective_;
     /// diagonalNorm.
     CoinWorkDouble diagonalNorm_;
     /// stepLength
     CoinWorkDouble stepLength_;
     /// linearPerturbation
     CoinWorkDouble linearPerturbation_;
     /// diagonalPerturbation
     CoinWorkDouble diagonalPerturbation_;
     // gamma from Saunders and Tomlin regularized
     CoinWorkDouble gamma_;
     // delta from Saunders and Tomlin regularized
     CoinWorkDouble delta_;
     /// targetGap
     CoinWorkDouble targetGap_;
     /// projectionTolerance
     CoinWorkDouble projectionTolerance_;
     /// maximumRHSError.  maximum Ax
     CoinWorkDouble maximumRHSError_;
     /// maximumBoundInfeasibility.
     CoinWorkDouble maximumBoundInfeasibility_;
     /// maximumDualError.
     CoinWorkDouble maximumDualError_;
     /// diagonalScaleFactor.
     CoinWorkDouble diagonalScaleFactor_;
     /// scaleFactor.  For scaling objective
     CoinWorkDouble scaleFactor_;
     /// actualPrimalStep
     CoinWorkDouble actualPrimalStep_;
     /// actualDualStep
     CoinWorkDouble actualDualStep_;
     /// smallestInfeasibility
     CoinWorkDouble smallestInfeasibility_;
     /// historyInfeasibility.
#define LENGTH_HISTORY 5
     CoinWorkDouble historyInfeasibility_[LENGTH_HISTORY];
     /// complementarityGap.
     CoinWorkDouble complementarityGap_;
     /// baseObjectiveNorm
     CoinWorkDouble baseObjectiveNorm_;
     /// worstDirectionAccuracy
     CoinWorkDouble worstDirectionAccuracy_;
     /// maximumRHSChange
     CoinWorkDouble maximumRHSChange_;
     /// errorRegion. i.e. Ax
     CoinWorkDouble * errorRegion_;
     /// rhsFixRegion.
     CoinWorkDouble * rhsFixRegion_;
     /// upperSlack
     CoinWorkDouble * upperSlack_;
     /// lowerSlack
     CoinWorkDouble * lowerSlack_;
     /// diagonal
     CoinWorkDouble * diagonal_;
     /// solution
     CoinWorkDouble * solution_;
     /// work array
     CoinWorkDouble * workArray_;
     /// delta X
     CoinWorkDouble * deltaX_;
     /// delta Y
     CoinWorkDouble * deltaY_;
     /// deltaZ.
     CoinWorkDouble * deltaZ_;
     /// deltaW.
     CoinWorkDouble * deltaW_;
     /// deltaS.
     CoinWorkDouble * deltaSU_;
     CoinWorkDouble * deltaSL_;
     /// Primal regularization array
     CoinWorkDouble * primalR_;
     /// Dual regularization array
     CoinWorkDouble * dualR_;
     /// rhs B
     CoinWorkDouble * rhsB_;
     /// rhsU.
     CoinWorkDouble * rhsU_;
     /// rhsL.
     CoinWorkDouble * rhsL_;
     /// rhsZ.
     CoinWorkDouble * rhsZ_;
     /// rhsW.
     CoinWorkDouble * rhsW_;
     /// rhs C
     CoinWorkDouble * rhsC_;
     /// zVec
     CoinWorkDouble * zVec_;
     /// wVec
     CoinWorkDouble * wVec_;
     /// cholesky.
     ClpCholeskyBase * cholesky_;
     /// numberComplementarityPairs i.e. ones with lower and/or upper bounds (not fixed)
     int numberComplementarityPairs_;
     /// numberComplementarityItems_ i.e. number of active bounds
     int numberComplementarityItems_;
     /// Maximum iterations
     int maximumBarrierIterations_;
     /// gonePrimalFeasible.
     bool gonePrimalFeasible_;
     /// goneDualFeasible.
     bool goneDualFeasible_;
     /// Which algorithm being used
     int algorithm_;
     //@}
};
//#############################################################################
/** A function that tests the methods in the ClpInterior class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging.

    It also does some testing of ClpFactorization class
 */
void
ClpInteriorUnitTest(const std::string & mpsDir,
                    const std::string & netlibDir);


#endif
