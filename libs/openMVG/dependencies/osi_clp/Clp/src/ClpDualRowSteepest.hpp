/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpDualRowSteepest_H
#define ClpDualRowSteepest_H

#include "ClpDualRowPivot.hpp"
class CoinIndexedVector;


//#############################################################################

/** Dual Row Pivot Steepest Edge Algorithm Class

See Forrest-Goldfarb paper for algorithm

*/

class ClpDualRowSteepest : public ClpDualRowPivot {

public:

     ///@name Algorithmic methods
     //@{

     /// Returns pivot row, -1 if none
     virtual int pivotRow();

     /** Updates weights and returns pivot alpha.
         Also does FT update */
     virtual double updateWeights(CoinIndexedVector * input,
                                  CoinIndexedVector * spare,
                                  CoinIndexedVector * spare2,
                                  CoinIndexedVector * updatedColumn);

     /** Updates primal solution (and maybe list of candidates)
         Uses input vector which it deletes
         Computes change in objective function
     */
     virtual void updatePrimalSolution(CoinIndexedVector * input,
                                       double theta,
                                       double & changeInObjective);

     /** Saves any weights round factorization as pivot rows may change
         Save model
         May also recompute infeasibility stuff
         1) before factorization
         2) after good factorization (if weights empty may initialize)
         3) after something happened but no factorization
            (e.g. check for infeasible)
         4) as 2 but restore weights from previous snapshot
         5) for strong branching - initialize (uninitialized) , infeasibilities
     */
     virtual void saveWeights(ClpSimplex * model, int mode);
     /// Gets rid of last update
     virtual void unrollWeights();
     /// Gets rid of all arrays
     virtual void clearArrays();
     /// Returns true if would not find any row
     virtual bool looksOptimal() const;
     /// Called when maximum pivots changes
     virtual void maximumPivotsChanged();
     //@}

     /** enums for persistence
     */
     enum Persistence {
          normal = 0x00, // create (if necessary) and destroy
          keep = 0x01 // create (if necessary) and leave
     };

     ///@name Constructors and destructors
     //@{
     /** Default Constructor
         0 is uninitialized, 1 full, 2 is partial uninitialized,
         3 starts as 2 but may switch to 1.
         By partial is meant that the weights are updated as normal
         but only part of the infeasible basic variables are scanned.
         This can be faster on very easy problems.
     */
     ClpDualRowSteepest(int mode = 3);

     /// Copy constructor
     ClpDualRowSteepest(const ClpDualRowSteepest &);

     /// Assignment operator
     ClpDualRowSteepest & operator=(const ClpDualRowSteepest& rhs);

     /// Fill most values
     void fill(const ClpDualRowSteepest& rhs);

     /// Destructor
     virtual ~ClpDualRowSteepest ();

     /// Clone
     virtual ClpDualRowPivot * clone(bool copyData = true) const;

     //@}
     /**@name gets and sets */
     //@{
     /// Mode
     inline int mode() const {
          return mode_;
     }
     /// Set/ get persistence
     inline void setPersistence(Persistence life) {
          persistence_ = life;
     }
     inline Persistence persistence() const {
          return persistence_ ;
     }
//@}

     //---------------------------------------------------------------------------

private:
     ///@name Private member data
     /** Status
         0) Normal
         -1) Needs initialization
         1) Weights are stored by sequence number
     */
     int state_;
     /** If 0 then we are using uninitialized weights, 1 then full,
         if 2 then uninitialized partial, 3 switchable */
     int mode_;
     /// Life of weights
     Persistence persistence_;
     /// weight array
     double * weights_;
     /// square of infeasibility array (just for infeasible rows)
     CoinIndexedVector * infeasible_;
     /// alternate weight array (so we can unroll)
     CoinIndexedVector * alternateWeights_;
     /// save weight array (so we can use checkpoint)
     CoinIndexedVector * savedWeights_;
     /// Dubious weights
     int * dubiousWeights_;
     //@}
};

#endif
