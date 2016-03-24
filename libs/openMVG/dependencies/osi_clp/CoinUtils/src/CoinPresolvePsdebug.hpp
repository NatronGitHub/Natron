/* $Id: CoinPresolvePsdebug.hpp 1560 2012-11-24 00:29:01Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolvePsdebug_H
#define CoinPresolvePsdebug_H

/*
  The current idea of the relation between PRESOLVE_DEBUG and
  PRESOLVE_CONSISTENCY is that PRESOLVE_CONSISTENCY triggers the consistency
  checks and PRESOLVE_DEBUG triggers consistency checks and output.
  This isn't always true in the code, but that's the goal.  Really,
  the whole compile-time scheme should be replaced with something more
  user-friendly (control variables that can be changed during the run).

  Also floating about are PRESOLVE_SUMMARY and COIN_PRESOLVE_TUNING.
  -- lh, 111208 --
*/
/*! \defgroup PresolveDebugFunctions Presolve Debug Functions

  These functions implement consistency checks on data structures involved
  in presolve and postsolve and on the components of the lp solution.

  To use these functions, include CoinPresolvePsdebug.hpp in your file and
  define the compile-time constants PRESOLVE_SUMMARY, PRESOLVE_DEBUG, and
  PRESOLVE_CONSISTENCY. A value is needed (<i>i.e.</i>, PRESOLVE_DEBUG=1).
  In a few places, higher values will get you a bit more output.

                        ********

  Define the symbols PRESOLVE_DEBUG and PRESOLVE_CONSISTENCY on the configure
  command line (use ADD_CXXFLAGS), in a Makefile, or similar and do a full
  rebuild (including any presolve driver code). If the symbols are not
  consistently nonzero across *all* presolve code, you'll get something
  between garbage and a core dump! Debugging adds messages to CoinMessage
  and allocates and maintains arrays that hold debug information.

  That said, given that you've configured and built with PRESOLVE_DEBUG and
  PRESOLVE_CONSISTENCY nonzero everywhere, it's safe to adjust PRESOLVE_DEBUG
  to values in the range 1..n in individual files to increase or decrease the
  amount of output.

  The suggested approach for PRESOLVE_DEBUG is to define it to 1 in the build
  and then increase it in individual presolve code files to get more detail.

                        ********
*/
//@{

/*! \relates CoinPresolveMatrix
    \brief Check column-major and/or row-major matrices for duplicate
	   entries in the major vectors.

  By default, scans both the column- and row-major matrices. Set doCol (doRow)
  to false to suppress the column (row) scan.
*/
void presolve_no_dups(const CoinPresolveMatrix *preObj,
		      bool doCol = true, bool doRow = true) ;

/*! \relates CoinPresolveMatrix
    \brief Check the links which track storage order for major vectors in
    the bulk storage area.

  By default, scans both the column- and row-major matrix. Set doCol = false to
  suppress the column-major scan. Set doRow = false to suppres the row-major
  scan. 
*/
void presolve_links_ok(const CoinPresolveMatrix *preObj,
		       bool doCol = true, bool doRow = true) ;

/*! \relates CoinPresolveMatrix
    \brief Check for explicit zeros in the column- and/or row-major matrices.

  By default, scans both the column- and row-major matrices. Set doCol (doRow)
  to false to suppress the column (row) scan.
*/
void presolve_no_zeros(const CoinPresolveMatrix *preObj,
		       bool doCol = true, bool doRow = true) ;

/*! \relates CoinPresolveMatrix
    \brief Checks for equivalence of the column- and row-major matrices.

  Normally the routine will test for coefficient presence and value. Set
  \p chkvals to false to suppress the check for equal value.
*/
void presolve_consistent(const CoinPresolveMatrix *preObj,
			 bool chkvals = true) ;

/*! \relates CoinPostsolveMatrix
    \brief Checks that column threads agree with column lengths
*/
void presolve_check_threads(const CoinPostsolveMatrix *obj) ;

/*! \relates CoinPostsolveMatrix
    \brief Checks the free list

    Scans the thread of free locations in the bulk store and checks that all
    entries are reasonable (0 <= index < bulk0_). If chkElemCnt is true, it
    also checks that the total number of entries in the matrix plus the
    locations on the free list total to the size of the bulk store. Postsolve
    routines do not maintain an accurate element count, but this is useful
    for checking a newly constructed postsolve matrix.
*/
void presolve_check_free_list(const CoinPostsolveMatrix *obj,
			      bool chkElemCnt = false) ;

/*! \relates CoinPostsolveMatrix
    \brief Check stored reduced costs for accuracy and consistency with
	   variable status.

  The routine will check the value of the reduced costs for architectural
  variables (CoinPrePostsolveMatrix::rcosts_). It performs an accuracy check
  by recalculating the reduced cost from scratch. It will also check the
  value for consistency with the status information in
  CoinPrePostsolveMatrix::colstat_.
*/
void presolve_check_reduced_costs(const CoinPostsolveMatrix *obj) ;

/*! \relates CoinPostsolveMatrix
    \brief Check the dual variables for consistency with row activity.

  The routine checks that the value of the dual variable is consistent
  with the state of the constraint (loose, tight at lower bound, or tight at
  upper bound).
*/
void presolve_check_duals(const CoinPostsolveMatrix *postObj) ;

/*! \relates CoinPresolveMatrix
    \brief Check primal solution and architectural variable status.

    The architectural variables can be checked for bogus values, feasibility,
    and valid status. The row activity is checked for bogus values, accuracy,
    and feasibility.  By default, row activity is not checked (presolve is
    sloppy about maintaining it). See the definitions in
    CoinPresolvePsdebug.cpp for more information.
*/
void presolve_check_sol(const CoinPresolveMatrix *preObj,
			int chkColSol = 2, int chkRowAct = 1,
			int chkStatus = 1) ;

/*! \relates CoinPostsolveMatrix
    \brief Check primal solution and architectural variable status.

    The architectural variables can be checked for bogus values, feasibility,
    and valid status. The row activity is checked for bogus values, accuracy,
    and feasibility. See the definitions in CoinPresolvePsdebug.cpp for more
    information.
*/
void presolve_check_sol(const CoinPostsolveMatrix *postObj,
			int chkColSol = 2, int chkRowAct = 2,
			int chkStatus = 1) ;

/*! \relates CoinPresolveMatrix
    \brief Check for the proper number of basic variables.
*/
void presolve_check_nbasic(const CoinPresolveMatrix *preObj) ;

/*! \relates CoinPostsolveMatrix
    \brief Check for the proper number of basic variables.
*/
void presolve_check_nbasic(const CoinPostsolveMatrix *postObj) ;

//@}

#endif
