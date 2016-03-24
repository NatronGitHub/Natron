/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef AbcDualRowDantzig_H
#define AbcDualRowDantzig_H

#include "AbcDualRowPivot.hpp"

//#############################################################################

/** Dual Row Pivot Dantzig Algorithm Class
    
    This is simplest choice - choose largest infeasibility
    
*/

class AbcDualRowDantzig : public AbcDualRowPivot {
  
public:
  
  ///@name Algorithmic methods
  //@{
  
  /// Returns pivot row, -1 if none
  virtual int pivotRow();
  
  /** Updates weights and returns pivot alpha.
      Also does FT update */
  virtual double updateWeights(CoinIndexedVector & input,CoinIndexedVector & updatedColumn);
  virtual double updateWeights1(CoinIndexedVector & input,CoinIndexedVector & updateColumn);
  virtual void updateWeightsOnly(CoinIndexedVector & /*input*/) {};
  /// Actually updates weights
  virtual void updateWeights2(CoinIndexedVector & input,CoinIndexedVector & /*updateColumn*/) {input.clear();};
  /** Updates primal solution (and maybe list of candidates)
      Uses input vector which it deletes
      Computes change in objective function
  */
  virtual void updatePrimalSolution(CoinIndexedVector & input,
				    double theta);
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
  virtual void saveWeights(AbcSimplex * model, int mode);
  /// Recompute infeasibilities
  virtual void recomputeInfeasibilities();
  //@}
  
  
  ///@name Constructors and destructors
  //@{
  /// Default Constructor
  AbcDualRowDantzig();
  
  /// Copy constructor
  AbcDualRowDantzig(const AbcDualRowDantzig &);
  
  /// Assignment operator
  AbcDualRowDantzig & operator=(const AbcDualRowDantzig& rhs);
  
  /// Destructor
  virtual ~AbcDualRowDantzig ();
  
  /// Clone
  virtual AbcDualRowPivot * clone(bool copyData = true) const;
  
  //@}
  
  //---------------------------------------------------------------------------
  
private:
  ///@name Private member data
  /// infeasibility array (just for infeasible rows)
  CoinIndexedVector * infeasible_;
  //@}
};

#endif
