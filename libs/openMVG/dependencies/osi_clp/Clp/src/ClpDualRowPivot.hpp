/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpDualRowPivot_H
#define ClpDualRowPivot_H

class ClpSimplex;
class CoinIndexedVector;

//#############################################################################

/** Dual Row Pivot Abstract Base Class

Abstract Base Class for describing an interface to an algorithm
to choose row pivot in dual simplex algorithm.  For some algorithms
e.g. Dantzig choice then some functions may be null.

*/

class ClpDualRowPivot  {

public:

     ///@name Algorithmic methods
     //@{

     /// Returns pivot row, -1 if none
     virtual int pivotRow() = 0;

     /** Updates weights and returns pivot alpha.
         Also does FT update */
     virtual double updateWeights(CoinIndexedVector * input,
                                  CoinIndexedVector * spare,
                                  CoinIndexedVector * spare2,
                                  CoinIndexedVector * updatedColumn) = 0;

     /** Updates primal solution (and maybe list of candidates)
         Uses input vector which it deletes
         Computes change in objective function
         Would be faster if we kept basic regions, but on other hand it
         means everything is always in sync
     */
     /* FIXME: this was pure virtul (=0). Why? */
     virtual void updatePrimalSolution(CoinIndexedVector * input,
                                       double theta,
                                       double & changeInObjective) = 0;
     /** Saves any weights round factorization as pivot rows may change
         Will be empty unless steepest edge (will save model)
         May also recompute infeasibility stuff
         1) before factorization
         2) after good factorization (if weights empty may initialize)
         3) after something happened but no factorization
            (e.g. check for infeasible)
         4) as 2 but restore weights from previous snapshot
         5) for strong branching - initialize  , infeasibilities
     */
     virtual void saveWeights(ClpSimplex * model, int mode);
     /// checks accuracy and may re-initialize (may be empty)
     virtual void checkAccuracy();
     /// Gets rid of last update (may be empty)
     virtual void unrollWeights();
     /// Gets rid of all arrays (may be empty)
     virtual void clearArrays();
     /// Returns true if would not find any row
     virtual bool looksOptimal() const {
          return false;
     }
     /// Called when maximum pivots changes
     virtual void maximumPivotsChanged() {}
     //@}


     ///@name Constructors and destructors
     //@{
     /// Default Constructor
     ClpDualRowPivot();

     /// Copy constructor
     ClpDualRowPivot(const ClpDualRowPivot &);

     /// Assignment operator
     ClpDualRowPivot & operator=(const ClpDualRowPivot& rhs);

     /// Destructor
     virtual ~ClpDualRowPivot ();

     /// Clone
     virtual ClpDualRowPivot * clone(bool copyData = true) const = 0;

     //@}

     ///@name Other
     //@{
     /// Returns model
     inline ClpSimplex * model() {
          return model_;
     }

     /// Sets model (normally to NULL)
     inline void setModel(ClpSimplex * newmodel) {
          model_ = newmodel;
     }

     /// Returns type (above 63 is extra information)
     inline int type() {
          return type_;
     }

     //@}

     //---------------------------------------------------------------------------

protected:
     ///@name Protected member data
     //@{
     /// Pointer to model
     ClpSimplex * model_;
     /// Type of row pivot algorithm
     int type_;
     //@}
};
#ifndef CLP_DUAL_COLUMN_MULTIPLIER
//#define CLP_DUAL_COLUMN_MULTIPLIER 0.99999
#endif
#endif
