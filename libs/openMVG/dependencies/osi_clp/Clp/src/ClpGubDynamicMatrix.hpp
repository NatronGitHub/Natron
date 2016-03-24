/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpGubDynamicMatrix_H
#define ClpGubDynamicMatrix_H


#include "CoinPragma.hpp"

#include "ClpGubMatrix.hpp"
/** This implements Gub rows plus a ClpPackedMatrix.
    This a dynamic version which stores the gub part and dynamically creates matrix.
    All bounds are assumed to be zero and infinity

    This is just a simple example for real column generation
*/

class ClpGubDynamicMatrix : public ClpGubMatrix {

public:
     /**@name Main functions provided */
     //@{
     /// Partial pricing
     virtual void partialPricing(ClpSimplex * model, double start, double end,
                                 int & bestSequence, int & numberWanted);
     /** This is local to Gub to allow synchronization:
         mode=0 when status of basis is good
         mode=1 when variable is flagged
         mode=2 when all variables unflagged (returns number flagged)
         mode=3 just reset costs (primal)
         mode=4 correct number of dual infeasibilities
         mode=5 return 4 if time to re-factorize
         mode=8  - make sure set is clean
         mode=9  - adjust lower, upper on set by incoming
     */
     virtual int synchronize(ClpSimplex * model, int mode);
     /// Sets up an effective RHS and does gub crash if needed
     virtual void useEffectiveRhs(ClpSimplex * model, bool cheapest = true);
     /**
        update information for a pivot (and effective rhs)
     */
     virtual int updatePivot(ClpSimplex * model, double oldInValue, double oldOutValue);
     /// Add a new variable to a set
     void insertNonBasic(int sequence, int iSet);
     /** Returns effective RHS offset if it is being used.  This is used for long problems
         or big gub or anywhere where going through full columns is
         expensive.  This may re-compute */
     virtual double * rhsOffset(ClpSimplex * model, bool forceRefresh = false,
                                bool check = false);

     using ClpPackedMatrix::times ;
     /** Return <code>y + A * scalar *x</code> in <code>y</code>.
         @pre <code>x</code> must be of size <code>numColumns()</code>
         @pre <code>y</code> must be of size <code>numRows()</code> */
     virtual void times(double scalar,
                        const double * x, double * y) const;
     /** Just for debug
         Returns sum and number of primal infeasibilities. Recomputes keys
     */
     virtual int checkFeasible(ClpSimplex * model, double & sum) const;
     /// Cleans data after setWarmStart
     void cleanData(ClpSimplex * model);
     //@}



     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpGubDynamicMatrix();
     /** Destructor */
     virtual ~ClpGubDynamicMatrix();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpGubDynamicMatrix(const ClpGubDynamicMatrix&);
     /** This is the real constructor.
         It assumes factorization frequency will not be changed.
         This resizes model !!!!
      */
     ClpGubDynamicMatrix(ClpSimplex * model, int numberSets,
                         int numberColumns, const int * starts,
                         const double * lower, const double * upper,
                         const int * startColumn, const int * row,
                         const double * element, const double * cost,
                         const double * lowerColumn = NULL, const double * upperColumn = NULL,
                         const unsigned char * status = NULL);

     ClpGubDynamicMatrix& operator=(const ClpGubDynamicMatrix&);
     /// Clone
     virtual ClpMatrixBase * clone() const ;
     //@}
     /**@name gets and sets */
     //@{
     /// enums for status of various sorts
     enum DynamicStatus {
          inSmall = 0x01,
          atUpperBound = 0x02,
          atLowerBound = 0x03
     };
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
     /// full starts
     inline int * fullStart() const {
          return fullStart_;
     }
     /// ids of active columns (just index here)
     inline int * id() const {
          return id_;
     }
     /// Optional lower bounds on columns
     inline double * lowerColumn() const {
          return lowerColumn_;
     }
     /// Optional upper bounds on columns
     inline double * upperColumn() const {
          return upperColumn_;
     }
     /// Optional true lower bounds on sets
     inline double * lowerSet() const {
          return lowerSet_;
     }
     /// Optional true upper bounds on sets
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
     /// set first free
     inline void setFirstAvailable(int value) {
          firstAvailable_ = value;
     }
     /// first dynamic
     inline int firstDynamic() const {
          return firstDynamic_;
     }
     /// number of columns in dynamic model
     inline int lastDynamic() const {
          return lastDynamic_;
     }
     /// size of working matrix (max)
     inline int numberElements() const {
          return numberElements_;
     }
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
     /// Saved value of objective offset
     double objectiveOffset_;
     /// Starts of each column
     CoinBigIndex * startColumn_;
     /// rows
     int * row_;
     /// elements
     double * element_;
     /// costs
     double * cost_;
     /// full starts
     int * fullStart_;
     /// ids of active columns (just index here)
     int * id_;
     /// for status and which bound
     unsigned char * dynamicStatus_;
     /// Optional lower bounds on columns
     double * lowerColumn_;
     /// Optional upper bounds on columns
     double * upperColumn_;
     /// Optional true lower bounds on sets
     double * lowerSet_;
     /// Optional true upper bounds on sets
     double * upperSet_;
     /// size
     int numberGubColumns_;
     /// first free
     int firstAvailable_;
     /// saved first free
     int savedFirstAvailable_;
     /// first dynamic
     int firstDynamic_;
     /// number of columns in dynamic model
     int lastDynamic_;
     /// size of working matrix (max)
     int numberElements_;
     //@}
};

#endif
