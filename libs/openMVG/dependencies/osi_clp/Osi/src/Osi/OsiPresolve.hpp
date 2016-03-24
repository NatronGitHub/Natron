// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiPresolve_H
#define OsiPresolve_H
#include "OsiSolverInterface.hpp"

class CoinPresolveAction;
#include "CoinPresolveMatrix.hpp"


/*! \class OsiPresolve
    \brief OSI interface to COIN problem simplification capabilities

  COIN provides a number of classes which implement problem simplification
  algorithms (CoinPresolveAction, CoinPrePostsolveMatrix, and derived
  classes). The model of operation is as follows:
  <ul>
    <li>
      Create a copy of the original problem.
    </li>
    <li>
      Subject the copy to a series of transformations (the <i>presolve</i>
      methods) to produce a presolved model. Each transformation is also
      expected to provide a method to reverse the transformation (the
      <i>postsolve</i> method). The postsolve methods are collected in a
      linked list; the postsolve method for the final presolve transformation
      is at the head of the list.
    </li>
    <li>
      Hand the presolved problem to the solver for optimization.
    </li>
    <li>
      Apply the collected postsolve methods to the presolved problem
      and solution, restating the solution in terms of the original problem.
    </li>
  </ul>

  The COIN presolve algorithms are unaware of OSI. The OsiPresolve class takes
  care of the interface. Given an OsiSolverInterface \c origModel, it will take
  care of creating a clone properly loaded with the presolved problem and ready
  for optimization. After optimization, it will apply postsolve
  transformations and load the result back into \c origModel.
  
  Assuming a problem has been loaded into an
  \c OsiSolverInterface \c origModel, a bare-bones application looks like this:
  \code
  OsiPresolve pinfo ;
  OsiSolverInterface *presolvedModel ;
  // Return an OsiSolverInterface loaded with the presolved problem.
  presolvedModel = pinfo.presolvedModel(*origModel,1.0e-8,false,numberPasses) ;
  presolvedModel->initialSolve() ;
  // Restate the solution and load it back into origModel.
  pinfo.postsolve(true) ;
  delete presolvedModel ;
  \endcode
*/



class OsiPresolve {
public:
  /// Default constructor (empty object)
  OsiPresolve();

  /// Virtual destructor
  virtual ~OsiPresolve();

  /*! \brief Create a new OsiSolverInterface loaded with the presolved problem.

    This method implements the first two steps described in the class
    documentation.  It clones \c origModel and applies presolve
    transformations, storing the resulting list of postsolve
    transformations.  It returns a pointer to a new OsiSolverInterface loaded
    with the presolved problem, or NULL if the problem is infeasible or
    unbounded.  If \c keepIntegers is true then bounds may be tightened in
    the original. Bounds will be moved by up to \c feasibilityTolerance to
    try and stay feasible. When \c doStatus is true, the current solution will
    be transformed to match the presolved model.

    This should be paired with postsolve(). It is up to the client to
    destroy the returned OsiSolverInterface, <i>after</i> calling postsolve().
    
    This method is virtual. Override this method if you need to customize
    the steps of creating a model to apply presolve transformations.

    In some sense, a wrapper for presolve(CoinPresolveMatrix*).
  */
  virtual OsiSolverInterface *presolvedModel(OsiSolverInterface & origModel,
					     double feasibilityTolerance=0.0,
					     bool keepIntegers=true,
					     int numberPasses=5,
                                             const char * prohibited=NULL,
					     bool doStatus=true,
					     const char * rowProhibited=NULL);

  /*! \brief Restate the solution to the presolved problem in terms of the
	     original problem and load it into the original model.
  
    postsolve() restates the solution in terms of the original problem and
    updates the original OsiSolverInterface supplied to presolvedModel().  If
    the problem has not been solved to optimality, there are no guarantees.
    If you are using an algorithm like simplex that has a concept of a basic
    solution, then set updateStatus

    The advantage of going back to the original problem is that it
    will be exactly as it was, <i>i.e.</i>, 0.0 will not become 1.0e-19.

    Note that if you modified the original problem after presolving, then you
    must ``undo'' these modifications before calling postsolve().

    In some sense, a wrapper for postsolve(CoinPostsolveMatrix&).
  */
  virtual void postsolve(bool updateStatus=true);

  /*! \brief Return a pointer to the presolved model. */
  OsiSolverInterface * model() const;

  /// Return a pointer to the original model
  OsiSolverInterface * originalModel() const;

  /// Set the pointer to the original model
  void setOriginalModel(OsiSolverInterface *model);

  /// Return a pointer to the original columns
  const int * originalColumns() const;

  /// Return a pointer to the original rows
  const int * originalRows() const;

  /// Return number of rows in original model
  inline int getNumRows() const
  { return nrows_;}

  /// Return number of columns in original model
  inline int getNumCols() const
  { return ncols_;}

  /** "Magic" number. If this is non-zero then any elements with this value
      may change and so presolve is very limited in what can be done
      to the row and column.  This is for non-linear problems.
  */
  inline void setNonLinearValue(double value)
  { nonLinearValue_ = value;}
  inline double nonLinearValue() const
    { return nonLinearValue_;}
  /*! \brief Fine control over presolve actions
  
    Set/clear the following bits to allow or suppress actions:
      - 0x01 allow duplicate column processing on integer columns
	     and dual stuff on integers
      - 0x02 switch off actions which can change +1 to something else
      	     (doubleton, tripleton, implied free)
      - 0x04 allow transfer of costs from singletons and between integer
      	     variables (when advantageous)
      - 0x08 do not allow x+y+z=1 transform
      - 0x10 allow actions that don't easily unroll
      - 0x20 allow dubious gub element reduction

    GUB element reduction is only partially implemented in CoinPresolve (see
    gubrow_action) and willl cause an abort at postsolve. It's not clear
    what's meant by `dual stuff on integers'.
    -- lh, 110605 --
  */
  inline void setPresolveActions(int action)
  { presolveActions_  = (presolveActions_&0xffff0000)|(action&0xffff);}

private:
  /*! Original model (solver interface loaded with the original problem).

      Must not be destroyed until after postsolve().
  */
  OsiSolverInterface * originalModel_;

  /*! Presolved  model (solver interface loaded with the presolved problem)
  
    Must be destroyed by the client (using delete) after postsolve().
  */
  OsiSolverInterface * presolvedModel_;

  /*! "Magic" number. If this is non-zero then any elements with this value
      may change and so presolve is very limited in what can be done
      to the row and column.  This is for non-linear problems.
      One could also allow for cases where sign of coefficient is known.
  */
  double nonLinearValue_;

  /// Original column numbers
  int * originalColumn_;

  /// Original row numbers
  int * originalRow_;

  /// The list of transformations applied.
  const CoinPresolveAction *paction_;

  /*! \brief Number of columns in original model.

    The problem will expand back to its former size as postsolve
    transformations are applied. It is efficient to allocate data structures
    for the final size of the problem rather than expand them as needed.
  */
  int ncols_;

  /*! \brief Number of rows in original model. */
  int nrows_;

  /*! \brief Number of nonzero matrix coefficients in the original model. */
  CoinBigIndex nelems_;

  /** Whether we want to skip dual part of presolve etc.
      1 bit allows duplicate column processing on integer columns
      and dual stuff on integers
      4 transfers costs to integer variables
  */
  int presolveActions_;
  /// Number of major passes
  int numberPasses_;

protected:
  /*! \brief Apply presolve transformations to the problem.
  
    Handles the core activity of applying presolve transformations.
    
    If you want to apply the individual presolve routines differently, or
    perhaps add your own to the mix, define a derived class and override
    this method
  */
  virtual const CoinPresolveAction *presolve(CoinPresolveMatrix *prob);

  /*! \brief Reverse presolve transformations to recover the solution
	     to the original problem.

    Handles the core activity of applying postsolve transformations.

    Postsolving is pretty generic; just apply the transformations in reverse
    order. You will probably only be interested in overriding this method if
    you want to add code to test for consistency while debugging new presolve
    techniques.
  */
  virtual void postsolve(CoinPostsolveMatrix &prob);

  /*! \brief Destroys queued postsolve actions.
  
    <i>E.g.</i>, when presolve() determines the problem is infeasible, so that
    it will not be necessary to actually solve the presolved problem and
    convert the result back to the original problem.
  */
  void gutsOfDestroy();
};
#endif
