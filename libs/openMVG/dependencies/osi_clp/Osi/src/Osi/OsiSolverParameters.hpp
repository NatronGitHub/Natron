// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiSolverParameters_H
#define OsiSolverParameters_H

enum OsiIntParam {
  /*! \brief Iteration limit for initial solve and resolve.
  
    The maximum number of iterations (whatever that means for the given
    solver) the solver can execute in the OsiSolverinterface::initialSolve()
    and OsiSolverinterface::resolve() methods before terminating.
  */
  OsiMaxNumIteration = 0,
  /*! \brief Iteration limit for hot start
  
    The maximum number of iterations (whatever that means for the given
    solver) the solver can execute in the
    OsiSolverinterface::solveFromHotStart() method before terminating.
  */
  OsiMaxNumIterationHotStart,
  /*! \brief Handling of row and column names.
  
    The name discipline specifies how the solver will handle row and column
    names:
    - 0: Auto names: Names cannot be set by the client. Names of the form
	 Rnnnnnnn or Cnnnnnnn are generated on demand when a name for a
	 specific row or column is requested; nnnnnnn is derived from the row
	 or column index. Requests for a vector of names return a vector with
	 zero entries.
    - 1: Lazy names: Names supplied by the client are retained. Names of the
	 form Rnnnnnnn or Cnnnnnnn are generated on demand if no name has been
	 supplied by the client. Requests for a vector of names return a
	 vector sized to the largest index of a name supplied by the client;
	 some entries in the vector may be null strings.
    - 2: Full names: Names supplied by the client are retained. Names of the
	 form Rnnnnnnn or Cnnnnnnn are generated on demand if no name has been
	 supplied by the client. Requests for a vector of names return a
	 vector sized to match the constraint system, and all entries will
	 contain either the name specified by the client or a generated name.
  */
  OsiNameDiscipline,
  /*! \brief End marker.
  
    Used by OsiSolverInterface to allocate a fixed-sized array to store
    integer parameters.
  */
  OsiLastIntParam
} ;

enum OsiDblParam {
  /*! \brief Dual objective limit.
  
    This is to be used as a termination criteria in algorithms where the dual
    objective changes monotonically (e.g., dual simplex, volume algorithm).
  */
  OsiDualObjectiveLimit = 0,
  /*! \brief Primal objective limit.
  
    This is to be used as a termination criteria in algorithms where the
    primal objective changes monotonically (e.g., primal simplex)
  */
  OsiPrimalObjectiveLimit,
  /*! \brief Dual feasibility tolerance.
  
    The maximum amount a dual constraint can be violated and still be
    considered feasible.
  */
  OsiDualTolerance,
  /*! \brief Primal feasibility tolerance.
  
    The maximum amount a primal constraint can be violated and still be
    considered feasible.
  */
  OsiPrimalTolerance,
  /** The value of any constant term in the objective function. */
  OsiObjOffset,
  /*! \brief End marker.
  
    Used by OsiSolverInterface to allocate a fixed-sized array to store
    double parameters.
  */
  OsiLastDblParam
};


enum OsiStrParam {
  /*! \brief The name of the loaded problem.
  
    This is the string specified on the Name card of an mps file.
  */
  OsiProbName = 0,
  /*! \brief The name of the solver.
  
    This parameter is read-only.
  */
  OsiSolverName,
  /*! \brief End marker.
  
    Used by OsiSolverInterface to allocate a fixed-sized array to store
    string parameters.
  */
  OsiLastStrParam
};

enum OsiHintParam {
  /** Whether to do a presolve in initialSolve */
  OsiDoPresolveInInitial = 0,
  /** Whether to use a dual algorithm in initialSolve.
      The reverse is to use a primal algorithm */
  OsiDoDualInInitial,
  /** Whether to do a presolve in resolve */
  OsiDoPresolveInResolve,
  /** Whether to use a dual algorithm in resolve.
      The reverse is to use a primal algorithm */
  OsiDoDualInResolve,
  /** Whether to scale problem */
  OsiDoScale,
  /** Whether to create a non-slack basis (only in initialSolve) */
  OsiDoCrash,
  /** Whether to reduce amount of printout, e.g., for branch and cut */
  OsiDoReducePrint,
  /** Whether we are in branch and cut - so can modify behavior */
  OsiDoInBranchAndCut,
  /** Just a marker, so that OsiSolverInterface can allocate a static sized
      array to store parameters. */
  OsiLastHintParam
};

enum OsiHintStrength {
  /** Ignore hint (default) */
  OsiHintIgnore = 0,
  /** This means it is only a hint */
  OsiHintTry,
  /** This means do hint if at all possible */
  OsiHintDo,
  /** And this means throw an exception if not possible */
  OsiForceDo
};

#endif
