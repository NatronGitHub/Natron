// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiRowCutDebugger_H
#define OsiRowCutDebugger_H

/*! \file OsiRowCutDebugger.hpp

  \brief Provides a facility to validate cut constraints to ensure that they
  	 do not cut off a given solution.
*/

#include <string>

#include "OsiCuts.hpp"
#include "OsiSolverInterface.hpp"

/*! \brief Validate cuts against a known solution

  OsiRowCutDebugger provides a facility for validating cuts against a known
  solution for a problem. The debugger knows an optimal solution for many of
  the miplib3 problems. Check the source for
  #activate(const OsiSolverInterface&,const char*)
  in OsiRowCutDebugger.cpp for the full set of known problems.

  A full solution vector can be supplied as a parameter with
  (#activate(const OsiSolverInterface&,const double*,bool)).
  Only the integer values need to be valid.
  The default behaviour is to solve an lp relaxation with the integer
  variables fixed to the specified values and use the optimal solution to fill
  in the continuous variables in the solution.
  The debugger can be instructed to preserve the continuous variables (useful
  when debugging solvers where the linear relaxation doesn't capture all the
  constraints).

  Note that the solution must match the problem held in the solver interface.
  If you want to use the row cut debugger on a problem after applying presolve
  transformations, your solution must match the presolved problem. (But see
  #redoSolution().)
*/
class OsiRowCutDebugger {
  friend void OsiRowCutDebuggerUnitTest(const OsiSolverInterface * siP,    
					const std::string & mpsDir);

public:
  
  /*! @name Validate Row Cuts
  
    Check that the specified cuts do not cut off the known solution.
  */
  //@{
  /*! \brief Check that the set of cuts does not cut off the solution known
  	     to the debugger.

    Check if any generated cuts cut off the solution known to the debugger!
    If so then print offending cuts.  Return the number of invalid cuts.
  */
  virtual int validateCuts(const OsiCuts & cs, int first, int last) const;

  /*! \brief Check that the cut does not cut off the solution known to the
  	     debugger.
  
    Return true if cut is invalid
  */
  virtual bool invalidCut(const OsiRowCut & rowcut) const;

  /*! \brief Returns true if the solution held in the solver is compatible
  	     with the known solution.

    More specifically, returns true if the known solution satisfies the column
    bounds held in the solver.
  */
  bool onOptimalPath(const OsiSolverInterface &si) const;
  //@}

  /*! @name Activate the Debugger
  
    The debugger is considered to be active when it holds a known solution.
  */
  //@{
  /*! \brief Activate a debugger using the name of a problem.

    The debugger knows an optimal solution for most of miplib3. Check the
    source code for the full list.  Returns true if the debugger is
    successfully activated.
  */
  bool activate(const OsiSolverInterface &si, const char *model) ;

  /*! \brief Activate a debugger using a full solution array.

    The solution must have one entry for every variable, but only the entries
    for integer values are used. By default the debugger will solve an lp
    relaxation with the integer variables fixed and fill in values for the
    continuous variables from this solution. If the debugger should preserve
    the given values for the continuous variables, set \p keepContinuous to
    \c true.

    Returns true if debugger activates successfully.
  */
  bool activate(const OsiSolverInterface &si, const double* solution,
  		bool keepContinuous = false) ;

  /// Returns true if the debugger is active 
  bool active() const;
  //@}

  /*! @name Query or Manipulate the Known Solution */
  //@{
  /// Return the known solution
  inline const double * optimalSolution() const
  { return knownSolution_;}

  /// Return the number of columns in the known solution
  inline int numberColumns() const { return (numberColumns_) ; }

  /// Return the value of the objective for the known solution
  inline double optimalValue() const { return knownValue_;}

  /*! \brief Edit the known solution to reflect column changes

    Given a translation array \p originalColumns[numberColumns] which can
    translate current column indices to original column indices, this method
    will edit the solution held in the debugger so that it matches the current
    set of columns.

    Useful when the original problem is preprocessed prior to cut generation.
    The debugger does keep a record of the changes.
  */
  void redoSolution(int numberColumns, const int *originalColumns);

  /// Print optimal solution (returns -1 bad debug, 0 on optimal, 1 not)
  int printOptimalSolution(const OsiSolverInterface & si) const;
  //@}

  /**@name Constructors and Destructors */
  //@{
  /// Default constructor - no checking 
  OsiRowCutDebugger ();

  /*! \brief Constructor with name of model.

    See #activate(const OsiSolverInterface&,const char*).
  */
  OsiRowCutDebugger(const OsiSolverInterface &si, const char *model) ;

  /*! \brief Constructor with full solution.

    See #activate(const OsiSolverInterface&,const double*,bool).
  */
  OsiRowCutDebugger(const OsiSolverInterface &si, const double *solution,
  		    bool enforceOptimality = false) ;
 
  /// Copy constructor 
  OsiRowCutDebugger(const OsiRowCutDebugger &);

  /// Assignment operator 
  OsiRowCutDebugger& operator=(const OsiRowCutDebugger& rhs);
  
  /// Destructor 
  virtual ~OsiRowCutDebugger ();
  //@}
      
private:
  
  // Private member data

  /**@name Private member data */
  //@{
  /// Value of known solution
  double knownValue_;

  /*! \brief Number of columns in known solution
  
    This must match the number of columns reported by the solver.
  */
  int numberColumns_;

  /// array specifying integer variables
  bool * integerVariable_;

  /// array specifying known solution
  double * knownSolution_;
  //@}
};
  
#endif
