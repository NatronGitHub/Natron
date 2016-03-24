/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef AbcDualRowSteepest_H
#define AbcDualRowSteepest_H

#include "AbcDualRowPivot.hpp"
class CoinIndexedVector;


//#############################################################################

/** Dual Row Pivot Steepest Edge Algorithm Class
    
    See Forrest-Goldfarb paper for algorithm
    
*/

class AbcDualRowSteepest : public AbcDualRowPivot {
  
public:
  
  ///@name Algorithmic methods
  //@{
  
  /// Returns pivot row, -1 if none
  virtual int pivotRow();
  
  /** Updates weights and returns pivot alpha.
      Also does FT update */
  virtual double updateWeights(CoinIndexedVector & input,CoinIndexedVector & updatedColumn);
  virtual double updateWeights1(CoinIndexedVector & input,CoinIndexedVector & updateColumn);
  virtual void updateWeightsOnly(CoinIndexedVector & input);
  /// Actually updates weights
  virtual void updateWeights2(CoinIndexedVector & input,CoinIndexedVector & updateColumn);
  
  /** Updates primal solution (and maybe list of candidates)
      Uses input vector which it deletes
  */
  virtual void updatePrimalSolution(CoinIndexedVector & input,
				    double theta);
  
  virtual void updatePrimalSolutionAndWeights(CoinIndexedVector & weightsVector,
					      CoinIndexedVector & updateColumn,
					      double theta);
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
  virtual void saveWeights(AbcSimplex * model, int mode);
  /// Recompute infeasibilities
  virtual void recomputeInfeasibilities();
  /// Gets rid of all arrays
  virtual void clearArrays();
  /// Returns true if would not find any row
  virtual bool looksOptimal() const;
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
  AbcDualRowSteepest(int mode = 3);
  
  /// Copy constructor
  AbcDualRowSteepest(const AbcDualRowSteepest &);
  
  /// Assignment operator
  AbcDualRowSteepest & operator=(const AbcDualRowSteepest& rhs);
  
  /// Fill most values
  void fill(const AbcDualRowSteepest& rhs);
  
  /// Destructor
  virtual ~AbcDualRowSteepest ();
  
  /// Clone
  virtual AbcDualRowPivot * clone(bool copyData = true) const;
  
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
  /// Infeasible vector
  inline CoinIndexedVector * infeasible() const
  { return infeasible_;}
  /// Weights vector
  inline CoinIndexedVector * weights() const
  { return weights_;}
  /// Model
  inline AbcSimplex * model() const
  { return model_;}
  //@}
  
  //---------------------------------------------------------------------------
  
private:
  ///@name Private member data
  /// norm saved before going into update
  double norm_;
  /// Ratio of size of factorization to number of rows
  double factorizationRatio_;
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
  CoinIndexedVector * weights_;
  /// square of infeasibility array (just for infeasible rows)
  CoinIndexedVector * infeasible_;
  /// save weight array (so we can use checkpoint)
  CoinIndexedVector * savedWeights_;
  //@}
};

// For Devex stuff
#undef DEVEX_TRY_NORM
#define DEVEX_TRY_NORM 1.0e-8
#define DEVEX_ADD_ONE 1.0
#endif
