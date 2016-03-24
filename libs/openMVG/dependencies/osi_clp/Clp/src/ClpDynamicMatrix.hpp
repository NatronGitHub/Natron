/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpDynamicMatrix_H
#define ClpDynamicMatrix_H


#include "CoinPragma.hpp"

#include "ClpPackedMatrix.hpp"
class ClpSimplex;
/** This implements  a dynamic matrix when we have a limit on the number of
    "interesting rows". This version inherits from ClpPackedMatrix and knows that
    the real matrix is gub.  A later version could use shortest path to generate columns.

*/

class ClpDynamicMatrix : public ClpPackedMatrix {

public:
     /// enums for status of various sorts
     enum DynamicStatus {
          soloKey = 0x00,
          inSmall = 0x01,
          atUpperBound = 0x02,
          atLowerBound = 0x03
     };
     /**@name Main functions provided */
     //@{
     /// Partial pricing
     virtual void partialPricing(ClpSimplex * model, double start, double end,
                                 int & bestSequence, int & numberWanted);

     /**
        update information for a pivot (and effective rhs)
     */
     virtual int updatePivot(ClpSimplex * model, double oldInValue, double oldOutValue);
     /** Returns effective RHS offset if it is being used.  This is used for long problems
         or big dynamic or anywhere where going through full columns is
         expensive.  This may re-compute */
     virtual double * rhsOffset(ClpSimplex * model, bool forceRefresh = false,
                                bool check = false);

     using ClpPackedMatrix::times ;
     /** Return <code>y + A * scalar *x</code> in <code>y</code>.
         @pre <code>x</code> must be of size <code>numColumns()</code>
         @pre <code>y</code> must be of size <code>numRows()</code> */
     virtual void times(double scalar,
                        const double * x, double * y) const;
     /// Modifies rhs offset
     void modifyOffset(int sequence, double amount);
     /// Gets key value when none in small
     double keyValue(int iSet) const;
     /**
         mode=0  - Set up before "updateTranspose" and "transposeTimes" for duals using extended
                   updates array (and may use other if dual values pass)
         mode=1  - Update dual solution after "transposeTimes" using extended rows.
         mode=2  - Compute all djs and compute key dual infeasibilities
         mode=3  - Report on key dual infeasibilities
         mode=4  - Modify before updateTranspose in partial pricing
     */
     virtual void dualExpanded(ClpSimplex * model, CoinIndexedVector * array,
                               double * other, int mode);
     /**
         mode=0  - Create list of non-key basics in pivotVariable_ using
                   number as numberBasic in and out
         mode=1  - Set all key variables as basic
         mode=2  - return number extra rows needed, number gives maximum number basic
         mode=3  - before replaceColumn
         mode=4  - return 1 if can do primal, 2 if dual, 3 if both
         mode=5  - save any status stuff (when in good state)
         mode=6  - restore status stuff
         mode=7  - flag given variable (normally sequenceIn)
         mode=8  - unflag all variables
         mode=9  - synchronize costs
         mode=10  - return 1 if there may be changing bounds on variable (column generation)
         mode=11  - make sure set is clean (used when a variable rejected - but not flagged)
         mode=12  - after factorize but before permute stuff
         mode=13  - at end of simplex to delete stuff
     */
     virtual int generalExpanded(ClpSimplex * model, int mode, int & number);
     /** Purely for column generation and similar ideas.  Allows
         matrix and any bounds or costs to be updated (sensibly).
         Returns non-zero if any changes.
     */
     virtual int refresh(ClpSimplex * model);
     /** Creates a variable.  This is called after partial pricing and will modify matrix.
         Will update bestSequence.
     */
     virtual void createVariable(ClpSimplex * model, int & bestSequence);
     /// Returns reduced cost of a variable
     virtual double reducedCost( ClpSimplex * model, int sequence) const;
     /// Does gub crash
     void gubCrash();
     /// Writes out model (without names)
     void writeMps(const char * name);
     /// Populates initial matrix from dynamic status
     void initialProblem();
     /** Adds in a column to gub structure (called from descendant) and returns sequence */
     int addColumn(int numberEntries, const int * row, const double * element,
                   double cost, double lower, double upper, int iSet,
                   DynamicStatus status);
     /** If addColumn forces compression then this allows descendant to know what to do.
         If >=0 then entry stayed in, if -1 then entry went out to lower bound.of zero.
         Entries at upper bound (really nonzero) never go out (at present).
     */
     virtual void packDown(const int * , int ) {}
     /// Gets lower bound (to simplify coding)
     inline double columnLower(int sequence) const {
          if (columnLower_) return columnLower_[sequence];
          else return 0.0;
     }
     /// Gets upper bound (to simplify coding)
     inline double columnUpper(int sequence) const {
          if (columnUpper_) return columnUpper_[sequence];
          else return COIN_DBL_MAX;
     }

     //@}



     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpDynamicMatrix();
     /** This is the real constructor.
         It assumes factorization frequency will not be changed.
         This resizes model !!!!
         The contents of original matrix in model will be taken over and original matrix
         will be sanitized so can be deleted (to avoid a very small memory leak)
      */
     ClpDynamicMatrix(ClpSimplex * model, int numberSets,
                      int numberColumns, const int * starts,
                      const double * lower, const double * upper,
                      const CoinBigIndex * startColumn, const int * row,
                      const double * element, const double * cost,
                      const double * columnLower = NULL, const double * columnUpper = NULL,
                      const unsigned char * status = NULL,
                      const unsigned char * dynamicStatus = NULL);

     /** Destructor */
     virtual ~ClpDynamicMatrix();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpDynamicMatrix(const ClpDynamicMatrix&);
     /** The copy constructor from an CoinPackedMatrix. */
     ClpDynamicMatrix(const CoinPackedMatrix&);

     ClpDynamicMatrix& operator=(const ClpDynamicMatrix&);
     /// Clone
     virtual ClpMatrixBase * clone() const ;
     //@}
     /**@name gets and sets */
     //@{
     /// Status of row slacks
     inline ClpSimplex::Status getStatus(int sequence) const {
          return static_cast<ClpSimplex::Status> (status_[sequence] & 7);
     }
     inline void setStatus(int sequence, ClpSimplex::Status status) {
          unsigned char & st_byte = status_[sequence];
          st_byte = static_cast<unsigned char>(st_byte & ~7);
          st_byte = static_cast<unsigned char>(st_byte | status);
     }
     /// Whether flagged slack
     inline bool flaggedSlack(int i) const {
          return (status_[i] & 8) != 0;
     }
     inline void setFlaggedSlack(int i) {
          status_[i] = static_cast<unsigned char>(status_[i] | 8);
     }
     inline void unsetFlaggedSlack(int i) {
          status_[i] = static_cast<unsigned char>(status_[i] & ~8);
     }
     /// Number of sets (dynamic rows)
     inline int numberSets() const {
          return numberSets_;
     }
     /// Number of possible gub variables
     inline int numberGubEntries() const
     { return startSet_[numberSets_];}
     /// Sets
     inline int * startSets() const
     { return startSet_;}
     /// Whether flagged
     inline bool flagged(int i) const {
          return (dynamicStatus_[i] & 8) != 0;
     }
     inline void setFlagged(int i) {
          dynamicStatus_[i] = static_cast<unsigned char>(dynamicStatus_[i] | 8);
     }
     inline void unsetFlagged(int i) {
          dynamicStatus_[i] = static_cast<unsigned char>(dynamicStatus_[i] & ~8);
     }
     inline void setDynamicStatus(int sequence, DynamicStatus status) {
          unsigned char & st_byte = dynamicStatus_[sequence];
          st_byte = static_cast<unsigned char>(st_byte & ~7);
          st_byte = static_cast<unsigned char>(st_byte | status);
     }
     inline DynamicStatus getDynamicStatus(int sequence) const {
          return static_cast<DynamicStatus> (dynamicStatus_[sequence] & 7);
     }
     /// Saved value of objective offset
     inline double objectiveOffset() const {
          return objectiveOffset_;
     }
     /// Starts of each column
     inline CoinBigIndex * startColumn() const {
          return startColumn_;
     }
     /// rows
     inline int * row() const {
          return row_;
     }
     /// elements
     inline double * element() const {
          return element_;
     }
     /// costs
     inline double * cost() const {
          return cost_;
     }
     /// ids of active columns (just index here)
     inline int * id() const {
          return id_;
     }
     /// Optional lower bounds on columns
     inline double * columnLower() const {
          return columnLower_;
     }
     /// Optional upper bounds on columns
     inline double * columnUpper() const {
          return columnUpper_;
     }
     /// Lower bounds on sets
     inline double * lowerSet() const {
          return lowerSet_;
     }
     /// Upper bounds on sets
     inline double * upperSet() const {
          return upperSet_;
     }
     /// size
     inline int numberGubColumns() const {
          return numberGubColumns_;
     }
     /// first free
     inline int firstAvailable() const {
          return firstAvailable_;
     }
     /// first dynamic
     inline int firstDynamic() const {
          return firstDynamic_;
     }
     /// number of columns in dynamic model
     inline int lastDynamic() const {
          return lastDynamic_;
     }
     /// number of rows in original model
     inline int numberStaticRows() const {
          return numberStaticRows_;
     }
     /// size of working matrix (max)
     inline int numberElements() const {
          return numberElements_;
     }
     inline int * keyVariable() const {
          return keyVariable_;
     }
     /// Switches off dj checking each factorization (for BIG models)
     void switchOffCheck();
     /// Status region for gub slacks
     inline unsigned char * gubRowStatus() const {
          return status_;
     }
     /// Status region for gub variables
     inline unsigned char * dynamicStatus() const {
          return dynamicStatus_;
     }
     /// Returns which set a variable is in
     int whichSet (int sequence) const;
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Sum of dual infeasibilities
     double sumDualInfeasibilities_;
     /// Sum of primal infeasibilities
     double sumPrimalInfeasibilities_;
     /// Sum of Dual infeasibilities using tolerance based on error in duals
     double sumOfRelaxedDualInfeasibilities_;
     /// Sum of Primal infeasibilities using tolerance based on error in primals
     double sumOfRelaxedPrimalInfeasibilities_;
     /// Saved best dual on gub row in pricing
     double savedBestGubDual_;
     /// Saved best set in pricing
     int savedBestSet_;
     /// Backward pointer to pivot row !!!
     int * backToPivotRow_;
     /// Key variable of set (only accurate if none in small problem)
     mutable int * keyVariable_;
     /// Backward pointer to extra row
     int * toIndex_;
     // Reverse pointer from index to set
     int * fromIndex_;
     /// Number of sets (dynamic rows)
     int numberSets_;
     /// Number of active sets
     int numberActiveSets_;
     /// Saved value of objective offset
     double objectiveOffset_;
     /// Lower bounds on sets
     double * lowerSet_;
     /// Upper bounds on sets
     double * upperSet_;
     /// Status of slack on set
     unsigned char * status_;
     /// Pointer back to model
     ClpSimplex * model_;
     /// first free
     int firstAvailable_;
     /// first free when iteration started
     int firstAvailableBefore_;
     /// first dynamic
     int firstDynamic_;
     /// number of columns in dynamic model
     int lastDynamic_;
     /// number of rows in original model
     int numberStaticRows_;
     /// size of working matrix (max)
     int numberElements_;
     /// Number of dual infeasibilities
     int numberDualInfeasibilities_;
     /// Number of primal infeasibilities
     int numberPrimalInfeasibilities_;
     /** If pricing will declare victory (i.e. no check every factorization).
         -1 - always check
         0  - don't check
         1  - in don't check mode but looks optimal
     */
     int noCheck_;
     /// Infeasibility weight when last full pass done
     double infeasibilityWeight_;
     /// size
     int numberGubColumns_;
     /// current maximum number of columns (then compress)
     int maximumGubColumns_;
     /// current maximum number of elemnts (then compress)
     int maximumElements_;
     /// Start of each set
     int * startSet_;
     /// next in chain
     int * next_;
     /// Starts of each column
     CoinBigIndex * startColumn_;
     /// rows
     int * row_;
     /// elements
     double * element_;
     /// costs
     double * cost_;
     /// ids of active columns (just index here)
     int * id_;
     /// for status and which bound
     unsigned char * dynamicStatus_;
     /// Optional lower bounds on columns
     double * columnLower_;
     /// Optional upper bounds on columns
     double * columnUpper_;
     //@}
};

#endif
