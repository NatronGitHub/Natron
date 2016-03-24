/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpDynamicExampleMatrix_H
#define ClpDynamicExampleMatrix_H


#include "CoinPragma.hpp"

#include "ClpDynamicMatrix.hpp"
class ClpSimplex;
/** This implements a dynamic matrix when we have a limit on the number of
    "interesting rows". This version inherits from ClpDynamicMatrix and knows that
    the real matrix is gub.  This acts just like ClpDynamicMatrix but generates columns.
    This "generates" columns by choosing from stored set.  It is maent as a starting point
    as to how you could use shortest path to generate columns.

    So it has its own copy of all data needed.  It populates ClpDynamicWatrix with enough
    to allow for gub keys and active variables.  In turn ClpDynamicMatrix populates
    a CoinPackedMatrix with active columns and rows.

    As there is one copy here and one in ClpDynamicmatrix these names end in Gen_

    It is obviously more efficient to just use ClpDynamicMatrix but the ideas is to
    show how much code a user would have to write.

    This does not work very well with bounds

*/

class ClpDynamicExampleMatrix : public ClpDynamicMatrix {

public:
     /**@name Main functions provided */
     //@{
     /// Partial pricing
     virtual void partialPricing(ClpSimplex * model, double start, double end,
                                 int & bestSequence, int & numberWanted);

     /** Creates a variable.  This is called after partial pricing and will modify matrix.
         Will update bestSequence.
     */
     virtual void createVariable(ClpSimplex * model, int & bestSequence);
     /** If addColumn forces compression then this allows descendant to know what to do.
         If >= then entry stayed in, if -1 then entry went out to lower bound.of zero.
         Entries at upper bound (really nonzero) never go out (at present).
     */
     virtual void packDown(const int * in, int numberToPack);
     //@}



     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpDynamicExampleMatrix();
     /** This is the real constructor.
         It assumes factorization frequency will not be changed.
         This resizes model !!!!
         The contents of original matrix in model will be taken over and original matrix
         will be sanitized so can be deleted (to avoid a very small memory leak)
      */
     ClpDynamicExampleMatrix(ClpSimplex * model, int numberSets,
                             int numberColumns, const int * starts,
                             const double * lower, const double * upper,
                             const int * startColumn, const int * row,
                             const double * element, const double * cost,
                             const double * columnLower = NULL, const double * columnUpper = NULL,
                             const unsigned char * status = NULL,
                             const unsigned char * dynamicStatus = NULL,
                             int numberIds = 0, const int *ids = NULL);
#if 0
     /// This constructor just takes over ownership (except for lower, upper)
     ClpDynamicExampleMatrix(ClpSimplex * model, int numberSets,
                             int numberColumns, int * starts,
                             const double * lower, const double * upper,
                             int * startColumn, int * row,
                             double * element, double * cost,
                             double * columnLower = NULL, double * columnUpper = NULL,
                             const unsigned char * status = NULL,
                             const unsigned char * dynamicStatus = NULL,
                             int numberIds = 0, const int *ids = NULL);
#endif
     /** Destructor */
     virtual ~ClpDynamicExampleMatrix();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpDynamicExampleMatrix(const ClpDynamicExampleMatrix&);
     ClpDynamicExampleMatrix& operator=(const ClpDynamicExampleMatrix&);
     /// Clone
     virtual ClpMatrixBase * clone() const ;
     //@}
     /**@name gets and sets */
     //@{
     /// Starts of each column
     inline CoinBigIndex * startColumnGen() const {
          return startColumnGen_;
     }
     /// rows
     inline int * rowGen() const {
          return rowGen_;
     }
     /// elements
     inline double * elementGen() const {
          return elementGen_;
     }
     /// costs
     inline double * costGen() const {
          return costGen_;
     }
     /// full starts
     inline int * fullStartGen() const {
          return fullStartGen_;
     }
     /// ids in next level matrix
     inline int * idGen() const {
          return idGen_;
     }
     /// Optional lower bounds on columns
     inline double * columnLowerGen() const {
          return columnLowerGen_;
     }
     /// Optional upper bounds on columns
     inline double * columnUpperGen() const {
          return columnUpperGen_;
     }
     /// size
     inline int numberColumns() const {
          return numberColumns_;
     }
     inline void setDynamicStatusGen(int sequence, DynamicStatus status) {
          unsigned char & st_byte = dynamicStatusGen_[sequence];
          st_byte = static_cast<unsigned char>(st_byte & ~7);
          st_byte = static_cast<unsigned char>(st_byte | status);
     }
     inline DynamicStatus getDynamicStatusGen(int sequence) const {
          return static_cast<DynamicStatus> (dynamicStatusGen_[sequence] & 7);
     }
     /// Whether flagged
     inline bool flaggedGen(int i) const {
          return (dynamicStatusGen_[i] & 8) != 0;
     }
     inline void setFlaggedGen(int i) {
          dynamicStatusGen_[i] = static_cast<unsigned char>(dynamicStatusGen_[i] | 8);
     }
     inline void unsetFlagged(int i) {
          dynamicStatusGen_[i] = static_cast<unsigned char>(dynamicStatusGen_[i] & ~8);
     }
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// size
     int numberColumns_;
     /// Starts of each column
     CoinBigIndex * startColumnGen_;
     /// rows
     int * rowGen_;
     /// elements
     double * elementGen_;
     /// costs
     double * costGen_;
     /// start of each set
     int * fullStartGen_;
     /// for status and which bound
     unsigned char * dynamicStatusGen_;
     /** identifier for each variable up one level (startColumn_, etc).  This is
         of length maximumGubColumns_.  For this version it is just sequence number
         at this level */
     int * idGen_;
     /// Optional lower bounds on columns
     double * columnLowerGen_;
     /// Optional upper bounds on columns
     double * columnUpperGen_;
     //@}
};

#endif
