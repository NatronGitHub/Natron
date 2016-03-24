/* $Id: CoinPresolveSingleton.cpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"

#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveSingleton.hpp"
#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif
#include "CoinMessage.hpp"
#include "CoinFinite.hpp"


/*
 * Original comment:
 *
 * Transfers singleton row bound information to the corresponding column bounds.
 * What I refer to as a row singleton would be called a doubleton
 * in the paper, since my terminology doesn't refer to the slacks.
 * In terms of the paper, we transfer the bounds of the slack onto
 * the variable (vii) and then "substitute" the slack out of the problem 
 * (which is a noop).
 */
/*
  Given blow(i) <= a(ij)x(j) <= b(i), we can transfer the bounds enforced by
  the constraint to the column bounds l(j) and u(j) on x(j) and delete the
  row.

  You can think of this as a specialised instance of doubleton_action, where
  the target variable is the logical that transforms an inequality to an
  equality. Since the system doesn't have logicals at this point, the row is a
  singleton.

  At some time in the past, the main loop was written to scan all rows but
  was limited in the number of rows it could process in one call. The
  notFinished parameter is the only remaining vestige of this behaviour and
  should probably be removed. For now, make sure it's forced to false for the
  benefit of code that looks at the returned value.  -- lh, 121015 --
*/
const CoinPresolveAction *
slack_doubleton_action::presolve(CoinPresolveMatrix *prob,
				 const CoinPresolveAction *next,
				 bool &notFinished)
{
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout << "Entering slack_doubleton_action::presolve." << std::endl ;
# endif
# if PRESOLVE_CONSISTENCY > 0
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# endif

# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  int startEmptyRows = prob->countEmptyRows() ;
  int startEmptyColumns = prob->countEmptyCols() ;
# if COIN_PRESOLVE_TUNING > 0
  double startTime = 0.0 ;
  if (prob->tuning_) {
    startTime = CoinCpuTime() ;
  }
# endif
# endif

  notFinished = false ;

/*
  Unpack the problem representation.
*/
  double *colels = prob->colels_ ;
  int *hrow = prob->hrow_ ;
  CoinBigIndex *mcstrt = prob->mcstrt_ ;
  int *hincol = prob->hincol_ ;

  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;

  double *rowels = prob->rowels_ ;
  const int *hcol = prob->hcol_ ;
  const CoinBigIndex *mrstrt = prob->mrstrt_ ;
  int *hinrow = prob->hinrow_ ;

  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;

/*
  Rowstat is used to decide if the solution is present.
*/
  unsigned char *rowstat = prob->rowstat_ ;
  double *acts = prob->acts_ ;
  double *sol = prob->sol_ ;

  const unsigned char *integerType = prob->integerType_ ;

  const double ztolzb	= prob->ztolzb_ ;

  int numberLook = prob->numberRowsToDo_ ;
  int *look = prob->rowsToDo_ ;
  bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;

  action *actions = new action[numberLook] ;
  int nactions = 0 ;

  int *fixed_cols = prob->usefulColumnInt_ ;
  int nfixed_cols = 0 ;

  bool infeas = false ;

/*
  Walk the rows looking for singletons.
*/
  for (int iLook = 0 ; iLook < numberLook ; iLook++) {
    int i = look[iLook] ;

    if (hinrow[i] != 1) continue ;
    int j = hcol[mrstrt[i]] ;
    double aij = rowels[mrstrt[i]] ;
    double lo = rlo[i] ;
    double up = rup[i] ;
    double abs_aij = fabs(aij) ;
/*
  A tiny value of a(ij) invites numerical error, since the new bound will be
  (something)/a(ij). Columns that are already fixed are also uninteresting.
*/
    if (abs_aij < ZTOLDP2) continue ;
    if (fabs(cup[j]-clo[j]) < ztolzb) continue ;

    PRESOLVE_DETAIL_PRINT(printf("pre_singleton %dC %dR E\n",j,i)) ;

/*
  Get down to work. First create the postsolve action for row i / x(j).
*/
    action *s = &actions[nactions] ;
    nactions++ ;
    s->col = j ;
    s->clo = clo[j] ;
    s->cup = cup[j] ;
    s->row = i ;
    s->rlo = rlo[i] ;
    s->rup = rup[i] ;
    s->coeff = aij ;

#   if PRESOLVE_DEBUG > 1
    std::cout
      << "  removing row " << i << ": " << rlo[i] << " <=  " << aij
      << "*x(" << j << ") <= " << rup[i] << std::endl ;
#   endif

/*
  Do the work of bounds transfer. Starting with
    blow(i) <= a(ij)x(j) <= b(i),
  we end up with
    blow(i)/a(ij) <= x(j) <= b(i)/a(ij)		a(ij) > 0
    blow(i)/a(ij) >= x(j) >= b(i)/a(ij)		a(ij) < 0
  The code deals with a(ij) < 0 by swapping and negating the row bounds and
  calculating with |a(ij)|. Be careful not to convert finite infinity to
  finite, or vice versa.
*/
    if (aij < 0.0) {
      CoinSwap(lo,up) ;
      lo = -lo ;
      up = -up ;
    }
    if (lo <= -PRESOLVE_INF)
      lo = -PRESOLVE_INF ;
    else {
      lo /= abs_aij ;
      if (lo <= -PRESOLVE_INF)
	lo = -PRESOLVE_INF ;
    }
    if (up > PRESOLVE_INF)
      up = PRESOLVE_INF ;
    else {
      up /= abs_aij ;
      if (up > PRESOLVE_INF)
	up = PRESOLVE_INF ;
    }
#   if PRESOLVE_DEBUG > 2
    std::cout
      << "    l(" << j << ") = " << clo[j] << " ==> " << lo << ", delta "
      << (lo-clo[j]) << std::endl ;
    std::cout
      << "    u(" << j << ") = " << cup[j] << " ==> " << up << ", delta "
      << (cup[j]-up) << std::endl ;
#   endif
/*
  lo and up are now the new l(j) and u(j), respectively. If they're better than
  the existing bounds, update. Have a care with integer variables --- don't let
  numerical inaccuracy pull us off an integral bound.
*/
    if (clo[j] < lo) {
      // If integer be careful
      if (integerType[j]) {
	if (fabs(lo-floor(lo+0.5)) < 0.000001) lo = floor(lo+0.5) ;
	if (clo[j] < lo) clo[j] = lo ;
      } else {
	clo[j] = lo ;
      }
    }
    if (cup[j] > up) {
      if (integerType[j]) {
	if (fabs(up-floor(up+0.5)) < 0.000001) up = floor(up+0.5) ;
	if (cup[j] > up) cup[j] = up ;
      } else {
	cup[j] = up ;
      }
    }
/*
  Is x(j) now fixed? Remember it for later.
*/
    if (fabs(cup[j] - clo[j]) < ZTOLDP) {
      fixed_cols[nfixed_cols++] = j ;
    }
/*
  Is x(j) infeasible? Fix it if we're within the feasibility tolerance, or if
  the user was so foolish as to request repair of infeasibility. Integer values
  are preferred, if close enough.

  If the infeasibility is too large to ignore, mark the problem infeasible and
  head for the exit.
*/
    if (lo > up) {
      if (lo <= up+prob->feasibilityTolerance_ || fixInfeasibility) {
	double nearest = floor(lo+0.5) ;
	if (fabs(nearest-lo)<2.0*prob->feasibilityTolerance_) {
	  lo = nearest ;
	  up = nearest ;
	} else {
	  lo = up ;
	}
	clo[j] = lo ;
	cup[j] = up ;
      } else {
	prob->status_ |= 1 ;
	prob->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
					prob->messages())
	    << j << lo << up << CoinMessageEol ;
	infeas = true ;
	break ;
      }
    }

#   if PRESOLVE_DEBUG > 1
    printf("SINGLETON R-%d C-%d\n", i, j) ;
#   endif

/*
  Remove the row from the row-major representation.
*/
    hinrow[i] = 0 ;
    PRESOLVE_REMOVE_LINK(prob->rlink_,i) ;
    rlo[i] = 0.0 ;
    rup[i] = 0.0 ;
/*
  Remove the row from this col in the column-major representation. It can
  happen that this will empty the column, in which case we can delink it.
  If the column isn't empty, queue it for further processing.
*/
    presolve_delete_from_col(i,j,mcstrt,hincol,hrow,colels) ;
    if (hincol[j] == 0) {
      PRESOLVE_REMOVE_LINK(prob->clink_,j) ;
    } else {
      prob->addCol(j) ;
    }
/*
  Update the solution, if it's present. The trick is maintaining the right
  number of basic variables. We've deleted a row, so we need to reduce the
  basis by one.

  There's a corner case that doesn't seem to be covered. What happens if
  both x(j) and s(i) are nonbasic? The number of basic variables will not
  be reduced.  This is admittedly a pathological situation: It implies
  that there's an existing bound l(j) or u(j) exactly equal to the bound
  imposed by this constraint, so that x(j) can be nonbasic at bound and
  the constraint can be simultaneously tight.   -- lh, 121115 --
*/
    if (rowstat) {
      int basisChoice = 0 ;
      int numberBasic = 0 ;
      double movement = 0 ;
      if (prob->columnIsBasic(j)) {
	numberBasic++ ;
	basisChoice = 2 ; // move to row to keep consistent
      }
      if (prob->rowIsBasic(i))
	numberBasic++ ;
      PRESOLVEASSERT(numberBasic > 0) ;
      if (sol[j] <= clo[j]+ztolzb) {
	movement = clo[j]-sol[j] ;
	sol[j] = clo[j] ;
	prob->setColumnStatus(j,CoinPrePostsolveMatrix::atLowerBound) ;
      } else if (sol[j] >= cup[j]-ztolzb) {
	movement = cup[j]-sol[j] ;
	sol[j] = cup[j] ;
	prob->setColumnStatus(j,CoinPrePostsolveMatrix::atUpperBound) ;
      } else {
	basisChoice = 1 ;
      }
      if (numberBasic > 1 || basisChoice == 1)
	prob->setColumnStatus(j,CoinPrePostsolveMatrix::basic) ;
      else if (basisChoice==2)
	prob->setRowStatus(i,CoinPrePostsolveMatrix::basic) ;
      if (movement) {
	const CoinBigIndex &kcs = mcstrt[j] ;
	const CoinBigIndex kce = kcs+hincol[j] ;
	for (CoinBigIndex kcol = kcs ; kcol < kce ; kcol++) {
	  int k = hrow[kcol] ;
	  PRESOLVEASSERT(hinrow[k] > 0) ;
	  acts[k] += movement*colels[kcol] ;
	}
      }
    }
  }
/*
  Done with processing. Time to deal with the results. First add the postsolve
  actions for the singletons to the postsolve list. Then call
  remove_fixed_action to handle variables that were fixed during the loop.
  (We've already adjusted the solution, so make_fixed_action is not needed.)
*/
  if (!infeas && nactions) {
#   if PRESOLVE_SUMMARY
    std::cout
      << "SINGLETON ROWS: " << nactions << std::endl ;
#   endif
    action *save_actions = new action[nactions] ;
    CoinMemcpyN(actions, nactions, save_actions) ;
    next = new slack_doubleton_action(nactions,save_actions,next) ;

    if (nfixed_cols)
      next = remove_fixed_action::presolve(prob,fixed_cols,nfixed_cols,next) ;
  }
  delete[] actions ;

# if COIN_PRESOLVE_TUNING > 0
  double thisTime ;
  if (prob->tuning_) double thisTime = CoinCpuTime() ;
# endif
# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  int droppedRows = prob->countEmptyRows()-startEmptyRows ;
  int droppedColumns = prob->countEmptyCols()-startEmptyColumns ;
  std::cout
    << "Leaving slack_doubleton_action::presolve, "
    << droppedRows << " rows, " << droppedColumns
    << " columns dropped" ;
#if COIN_PRESOLVE_TUNING > 0
  std::cout
    << " in " << (thisTime-startTime) << "s, total "
    << (thisTime-prob->startTime_) ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next) ;
}

void slack_doubleton_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_ ;
  const int nactions = nactions_ ;

  double *colels = prob->colels_ ;
  int *hrow = prob->hrow_ ;
  CoinBigIndex *mcstrt = prob->mcstrt_ ;
  int *hincol = prob->hincol_ ;
  int *link = prob->link_ ;

  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  double *sol = prob->sol_ ;
  double *rcosts = prob->rcosts_ ;
  unsigned char *colstat = prob->colstat_ ;

  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;
  double *acts = prob->acts_ ;
  double *rowduals = prob->rowduals_ ;


# if PRESOLVE_DEBUG
  char *rdone = prob->rdone_ ;
  std::cout
    << "Entering slack_doubleton_action::postsolve, "
    << nactions << " constraints to process." << std::endl ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# endif

  CoinBigIndex &free_list = prob->free_list_ ;

  const double ztolzb	= prob->ztolzb_ ;

  for (const action *f = &actions[nactions-1] ; actions <= f ; f--) {
    int irow = f->row ;
    double lo0 = f->clo ;
    double up0 = f->cup ;
    double coeff = f->coeff ;
    int jcol = f->col ;

    rlo[irow] = f->rlo ;
    rup[irow] = f->rup ;

    clo[jcol] = lo0 ;
    cup[jcol] = up0 ;

    acts[irow] = coeff*sol[jcol] ;
    /*
      Create the row and restore the single coefficient, linking the new
      coefficient at the start of the column.
    */
    {
      CoinBigIndex k = free_list ;
      assert(k >= 0 && k < prob->bulk0_) ;
      free_list = link[free_list] ;
      hrow[k] = irow ;
      colels[k] = coeff ;
      link[k] = mcstrt[jcol] ;
      mcstrt[jcol] = k ;
      hincol[jcol]++ ;
    }

    /*
      Since we are adding a row, we have to set the row status and dual
      to satisfy complimentary slackness.  We may also have to modify
      the column status and reduced cost if bounds have been relaxed.
     */
    if (!colstat) {
      // ????
      rowduals[irow] = 0.0 ;
    } else {
      if (prob->columnIsBasic(jcol)) {
	/*
	  The variable is basic, hence the slack must be basic, hence the dual
	  for the row is zero.  Relaxing the bounds on a basic variable
	  doesn't change anything.
	*/
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::basic) ;
	rowduals[irow] = 0.0 ;
      } else if ((fabs(sol[jcol]-lo0) <= ztolzb && rcosts[jcol] >= 0) ||
		 (fabs(sol[jcol]-up0) <= ztolzb && rcosts[jcol] <= 0)) {
	/*
	  The variable is nonbasic and the sign of the reduced cost is correct
	  for the bound. Again, the slack will be basic and the dual zero.
	*/
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::basic) ;
	rowduals[irow] = 0.0 ;
      } else if (!(fabs(sol[jcol]-lo0) <= ztolzb) &&
		 !(fabs(sol[jcol]-up0) <= ztolzb)) {
	/*
	  The variable was not basic but transferring bounds back to the
	  constraint has relaxed the column bounds. The variable will need to
	  be made basic. The constraint must then be tight and the dual must
	  be set so that the reduced cost of the variable becomes zero.
	*/
	prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::basic) ;
	prob->setRowStatusUsingValue(irow) ;
	rowduals[irow] = rcosts[jcol]/coeff ;
	rcosts[jcol] = 0.0 ;
      } else {
	/*
	  The variable is at bound, but the reduced cost is wrong. Again
	  set the row dual to bring the reduced cost to zero. This implies
	  that the constraint is tight and the slack will be nonbasic.
	*/
	prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::basic) ;
	prob->setRowStatusUsingValue(irow) ;
	rowduals[irow] = rcosts[jcol]/coeff ;
	rcosts[jcol] = 0.0 ;
      }
    }

#   if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
    rdone[irow] = SLACK_DOUBLETON ;
#   endif
  }

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# endif
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving slack_doubleton_action::postsolve." << std::endl ;
# endif

  return ; 
}
/*
    If we have a variable with one entry and no cost then we can
    transform the row from E to G etc.
    If there is a row objective region then we may be able to do
    this even with a cost.
*/
const CoinPresolveAction *
slack_singleton_action::presolve(CoinPresolveMatrix *prob,
				 const CoinPresolveAction *next,
                                 double * rowObjective)
{
  double startTime = 0.0 ;
  int startEmptyRows=0 ;
  int startEmptyColumns = 0 ;
  if (prob->tuning_) {
    startTime = CoinCpuTime() ;
    startEmptyRows = prob->countEmptyRows() ;
    startEmptyColumns = prob->countEmptyCols() ;
  }
  double *colels	= prob->colels_ ;
  int *hrow		= prob->hrow_ ;
  CoinBigIndex *mcstrt	= prob->mcstrt_ ;
  int *hincol		= prob->hincol_ ;
  //int ncols		= prob->ncols_ ;

  double *clo		= prob->clo_ ;
  double *cup		= prob->cup_ ;

  double *rowels	= prob->rowels_ ;
  int *hcol	= prob->hcol_ ;
  CoinBigIndex *mrstrt	= prob->mrstrt_ ;
  int *hinrow		= prob->hinrow_ ;
  int nrows		= prob->nrows_ ;

  double *rlo		= prob->rlo_ ;
  double *rup		= prob->rup_ ;

  // Existence of 
  unsigned char *rowstat	= prob->rowstat_ ;
  double *acts	= prob->acts_ ;
  double * sol = prob->sol_ ;

  const unsigned char *integerType = prob->integerType_ ;

  const double ztolzb	= prob->ztolzb_ ;
  double *dcost	= prob->cost_ ;
  //const double maxmin	= prob->maxmin_ ;

# if PRESOLVE_DEBUG
  std::cout << "Entering slack_singleton_action::presolve." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  int numberLook = prob->numberColsToDo_ ;
  int iLook ;
  int * look = prob->colsToDo_ ;
  // Make sure we allocate at least one action
  int maxActions = CoinMin(numberLook,nrows/10)+1 ;
  action * actions = new action[maxActions] ;
  int nactions = 0 ;
  int * fixed_cols = new int [numberLook] ;
  int nfixed_cols=0 ;
  int nWithCosts=0 ;
  double costOffset=0.0 ;
  for (iLook=0;iLook<numberLook;iLook++) {
    int iCol = look[iLook] ;
    if (dcost[iCol])
      continue ;
    if (hincol[iCol] == 1) {
      int iRow=hrow[mcstrt[iCol]] ;
      double coeff = colels[mcstrt[iCol]] ;
      double acoeff = fabs(coeff) ;
      if (acoeff < ZTOLDP2)
	continue ;
      // don't bother with fixed cols
      if (fabs(cup[iCol] - clo[iCol]) < ztolzb)
	continue ;
      if (integerType&&integerType[iCol]) {
	// only possible if everything else integer and unit coefficient
	// check everything else a bit later
	if (acoeff!=1.0)
	  continue ;
        double currentLower = rlo[iRow] ;
        double currentUpper = rup[iRow] ;
	if (coeff==1.0&&currentLower==1.0&&currentUpper==1.0) {
	  // leave if integer slack on sum x == 1
	  bool allInt=true ;
	  for (CoinBigIndex j=mrstrt[iRow] ;
	       j<mrstrt[iRow]+hinrow[iRow];j++) {
	    int iColumn = hcol[j] ;
	    double value = fabs(rowels[j]) ;
	    if (!integerType[iColumn]||value!=1.0) {
	      allInt=false ;
	      break ;
	    }
	  }
	  if (allInt)
	    continue; // leave as may help search
	}
      }
      if (!prob->colProhibited(iCol)) {
        double currentLower = rlo[iRow] ;
        double currentUpper = rup[iRow] ;
        if (!rowObjective) {
          if (dcost[iCol])
            continue ;
        } else if ((dcost[iCol]&&currentLower!=currentUpper)||rowObjective[iRow]) {
          continue ;
        }
        double newLower=currentLower ;
        double newUpper=currentUpper ;
        if (coeff<0.0) {
          if (currentUpper>1.0e20||cup[iCol]>1.0e20) {
            newUpper=COIN_DBL_MAX ;
          } else {
            newUpper -= coeff*cup[iCol] ;
            if (newUpper>1.0e20)
              newUpper=COIN_DBL_MAX ;
          }
          if (currentLower<-1.0e20||clo[iCol]<-1.0e20) {
            newLower=-COIN_DBL_MAX ;
          } else {
            newLower -= coeff*clo[iCol] ;
            if (newLower<-1.0e20)
              newLower=-COIN_DBL_MAX ;
          }
        } else {
          if (currentUpper>1.0e20||clo[iCol]<-1.0e20) {
            newUpper=COIN_DBL_MAX ;
          } else {
            newUpper -= coeff*clo[iCol] ;
            if (newUpper>1.0e20)
              newUpper=COIN_DBL_MAX ;
          }
          if (currentLower<-1.0e20||cup[iCol]>1.0e20) {
            newLower=-COIN_DBL_MAX ;
          } else {
            newLower -= coeff*cup[iCol] ;
            if (newLower<-1.0e20)
              newLower=-COIN_DBL_MAX ;
          }
        }
	if (integerType&&integerType[iCol]) {
	  // only possible if everything else integer
	  if (newLower>-1.0e30) {
	    if (newLower!=floor(newLower+0.5))
	      continue ;
	  }
	  if (newUpper<1.0e30) {
	    if (newUpper!=floor(newUpper+0.5))
	      continue ;
	  }
	  bool allInt=true ;
	  for (CoinBigIndex j=mrstrt[iRow] ;
	       j<mrstrt[iRow]+hinrow[iRow];j++) {
	    int iColumn = hcol[j] ;
	    double value = fabs(rowels[j]) ;
	    if (!integerType[iColumn]||value!=floor(value+0.5)) {
	      allInt=false ;
	      break ;
	    }
	  }
	  if (!allInt)
	    continue; // no good
	}
        if (nactions>=maxActions) {
          maxActions += CoinMin(numberLook-iLook,maxActions) ;
          action * temp = new action[maxActions] ;
	  memcpy(temp,actions,nactions*sizeof(action)) ;
          // changed as 4.6 compiler bug! CoinMemcpyN(actions,nactions,temp) ;
          delete [] actions ;
          actions=temp ;
        }
          
	action *s = &actions[nactions] ;
	nactions++ ;

	s->col = iCol ;
	s->clo = clo[iCol] ;
	s->cup = cup[iCol] ;

	s->row = iRow ;
	s->rlo = rlo[iRow] ;
	s->rup = rup[iRow] ;

	s->coeff = coeff ;

        presolve_delete_from_row(iRow,iCol,mrstrt,hinrow,hcol,rowels) ;
        if (!hinrow[iRow])
          PRESOLVE_REMOVE_LINK(prob->rlink_,iRow) ;
        // put row on stack of things to do next time
        prob->addRow(iRow) ;
#ifdef PRINTCOST        
        if (rowObjective&&dcost[iCol]) {
          printf("Singleton %d had coeff of %g in row %d - bounds %g %g - cost %g\n",
                 iCol,coeff,iRow,clo[iCol],cup[iCol],dcost[iCol]) ;
          printf("Row bounds were %g %g now %g %g\n",
                 rlo[iRow],rup[iRow],newLower,newUpper) ;
        }
#endif
        // Row may be redundant but let someone else do that
        rlo[iRow]=newLower ;
        rup[iRow]=newUpper ;
        if (rowstat&&sol) {
          // update solution and basis
          if ((sol[iCol] < cup[iCol]-ztolzb&&
	       sol[iCol] > clo[iCol]+ztolzb)||prob->columnIsBasic(iCol))
            prob->setRowStatus(iRow,CoinPrePostsolveMatrix::basic) ;
          prob->setColumnStatusUsingValue(iCol) ;
        }
        // Force column to zero
        clo[iCol]=0.0 ;
        cup[iCol]=0.0 ;
        if (rowObjective&&dcost[iCol]) {
          rowObjective[iRow]=-dcost[iCol]/coeff ;
            nWithCosts++ ;
          // adjust offset
          costOffset += currentLower*rowObjective[iRow] ;
          prob->dobias_ -= currentLower*rowObjective[iRow] ;
        }
	if (sol) {
	  double movement ;
	  if (fabs(sol[iCol]-clo[iCol])<fabs(sol[iCol]-cup[iCol])) {
	    movement = clo[iCol]-sol[iCol] ;
	    sol[iCol]=clo[iCol] ;
	  } else {
	    movement = cup[iCol]-sol[iCol] ;
	    sol[iCol]=cup[iCol] ;
	  }
	  if (movement) 
	    acts[iRow] += movement*coeff ;
	}
        /*
          Remove the row from this col in the col rep.and delink it.
        */
        presolve_delete_from_col(iRow,iCol,mcstrt,hincol,hrow,colels) ;
        assert (hincol[iCol] == 0) ;
        PRESOLVE_REMOVE_LINK(prob->clink_,iCol) ; 
	//clo[iCol] = 0.0 ;
	//cup[iCol] = 0.0 ;
	fixed_cols[nfixed_cols++] = iCol ;
        //presolve_consistent(prob) ;
      }
    }   
  }
  
  if (nactions) {
#   if PRESOLVE_SUMMARY
    printf("SINGLETON COLS:  %d\n", nactions) ;
#   endif
#ifdef COIN_DEVELOP
    printf("%d singletons, %d with costs - offset %g\n",nactions,
           nWithCosts, costOffset) ;
#endif
    action *save_actions = new action[nactions] ;
    CoinMemcpyN(actions, nactions, save_actions) ;
    next = new slack_singleton_action(nactions, save_actions, next) ;

    if (nfixed_cols)
      next = make_fixed_action::presolve(prob, fixed_cols, nfixed_cols,
					 true, // arbitrary
					 next) ;
  }
  delete [] actions ;
  delete [] fixed_cols ;
  if (prob->tuning_) {
    double thisTime=CoinCpuTime() ;
    int droppedRows = prob->countEmptyRows() - startEmptyRows ;
    int droppedColumns =  prob->countEmptyCols() - startEmptyColumns ;
    printf("CoinPresolveSingleton(3) - %d rows, %d columns dropped in time %g, total %g\n",
	   droppedRows,droppedColumns,thisTime-startTime,thisTime-prob->startTime_) ;
  }

# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
  std::cout << "Leaving slack_singleton_action::presolve." << std::endl ;
# endif

  return (next) ;
}

void slack_singleton_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_ ;
  const int nactions = nactions_ ;

  double *colels	= prob->colels_ ;
  int *hrow		= prob->hrow_ ;
  CoinBigIndex *mcstrt		= prob->mcstrt_ ;
  int *hincol		= prob->hincol_ ;
  int *link		= prob->link_ ;
  //  int ncols		= prob->ncols_ ;

  //double *rowels	= prob->rowels_ ;
  //int *hcol	= prob->hcol_ ;
  //CoinBigIndex *mrstrt	= prob->mrstrt_ ;
  //int *hinrow		= prob->hinrow_ ;

  double *clo		= prob->clo_ ;
  double *cup		= prob->cup_ ;

  double *rlo		= prob->rlo_ ;
  double *rup		= prob->rup_ ;

  double *sol		= prob->sol_ ;
  double *rcosts	= prob->rcosts_ ;

  double *acts		= prob->acts_ ;
  double *rowduals 	= prob->rowduals_ ;
  double *dcost	= prob->cost_ ;
  //const double maxmin	= prob->maxmin_ ;

  unsigned char *colstat		= prob->colstat_ ;
  //  unsigned char *rowstat		= prob->rowstat_ ;

# if PRESOLVE_DEBUG
  char *rdone		= prob->rdone_ ;

  std::cout << "Entering slack_singleton_action::postsolve." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  CoinBigIndex &free_list		= prob->free_list_ ;

  const double ztolzb	= prob->ztolzb_ ;
#ifdef CHECK_ONE_ROW
  {
    double act=0.0 ;
    for (int i=0;i<prob->ncols_;i++) {
      double solV = sol[i] ;
      assert (solV>=clo[i]-ztolzb&&solV<=cup[i]+ztolzb) ;
      int j=mcstrt[i] ;
      for (int k=0;k<hincol[i];k++) {
	if (hrow[j]==CHECK_ONE_ROW) {
	  act += colels[j]*solV ;
	}
	j=link[j] ;
      }
    }
    assert (act>=rlo[CHECK_ONE_ROW]-ztolzb&&act<=rup[CHECK_ONE_ROW]+ztolzb) ;
    printf("start %g %g %g %g\n",rlo[CHECK_ONE_ROW],act,acts[CHECK_ONE_ROW],rup[CHECK_ONE_ROW]) ;
  }
#endif
  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int iRow = f->row ;
    double lo0 = f->clo ;
    double up0 = f->cup ;
    double coeff = f->coeff ;
    int iCol = f->col ;
    assert (!hincol[iCol]) ;
#ifdef CHECK_ONE_ROW
    if (iRow==CHECK_ONE_ROW)
      printf("Col %d coeff %g old bounds %g,%g new %g,%g - new rhs %g,%g - act %g\n",
	     iCol,coeff,clo[iCol],cup[iCol],lo0,up0,f->rlo,f->rup,acts[CHECK_ONE_ROW]) ;
#endif
    rlo[iRow] = f->rlo ;
    rup[iRow] = f->rup ;

    clo[iCol] = lo0 ;
    cup[iCol] = up0 ;
    double movement=0.0 ;
    // acts was without coefficient - adjust
    acts[iRow] += coeff*sol[iCol] ;
    if (acts[iRow]<rlo[iRow]-ztolzb) 
      movement = rlo[iRow]-acts[iRow] ;
    else if (acts[iRow]>rup[iRow]+ztolzb)
      movement = rup[iRow]-acts[iRow] ;
    double cMove = movement/coeff ;
    sol[iCol] += cMove ;
    acts[iRow] += movement ;
    if (!dcost[iCol]) {
      // and to get column feasible
      cMove=0.0 ;
      if (sol[iCol]>cup[iCol]+ztolzb) 
        cMove = cup[iCol]-sol[iCol] ;
      else if (sol[iCol]<clo[iCol]-ztolzb) 
        cMove = clo[iCol]-sol[iCol] ;
      sol[iCol] += cMove ;
      acts[iRow] += cMove*coeff ;
      /*
       * Have to compute status.  At most one can be basic. It's possible that
	 both are nonbasic and nonbasic status must change.
       */
      if (colstat) {
        int numberBasic =0 ;
        if (prob->columnIsBasic(iCol)) 
          numberBasic++ ;
        if (prob->rowIsBasic(iRow)) 
          numberBasic++ ;
#ifdef COIN_DEVELOP
        if (numberBasic>1)
          printf("odd in singleton\n") ;
#endif
        if (sol[iCol]>clo[iCol]+ztolzb&&sol[iCol]<cup[iCol]-ztolzb) {
          prob->setColumnStatus(iCol,CoinPrePostsolveMatrix::basic) ;
          prob->setRowStatusUsingValue(iRow) ;
        } else if (acts[iRow]>rlo[iRow]+ztolzb&&acts[iRow]<rup[iRow]-ztolzb) {
          prob->setRowStatus(iRow,CoinPrePostsolveMatrix::basic) ;
          prob->setColumnStatusUsingValue(iCol) ;
        } else if (numberBasic) {
          prob->setRowStatus(iRow,CoinPrePostsolveMatrix::basic) ;
          prob->setColumnStatusUsingValue(iCol) ;
        } else {
          prob->setRowStatusUsingValue(iRow) ;
          prob->setColumnStatusUsingValue(iCol) ;
	}
      }
#     if PRESOLVE_DEBUG > 1
      printf("SLKSING: %d = %g restored %d lb = %g ub = %g.\n",
	     iCol,sol[iCol],prob->getColumnStatus(iCol),clo[iCol],cup[iCol]) ;
#     endif
    } else {
      // must have been equality row
      assert (rlo[iRow]==rup[iRow]) ;
      double cost = rcosts[iCol] ;
      // adjust for coefficient
      cost -= rowduals[iRow]*coeff ;
      bool basic=true ;
      if (fabs(sol[iCol]-cup[iCol])<ztolzb&&cost<-1.0e-6) 
        basic=false ;
      else if (fabs(sol[iCol]-clo[iCol])<ztolzb&&cost>1.0e-6) 
        basic=false ;
      //printf("Singleton %d had coeff of %g in row %d (dual %g) - bounds %g %g - cost %g, (dj %g)\n",
      //     iCol,coeff,iRow,rowduals[iRow],clo[iCol],cup[iCol],dcost[iCol],rcosts[iCol]) ;
      //if (prob->columnIsBasic(iCol)) 
      //printf("column basic! ") ;
      //if (prob->rowIsBasic(iRow)) 
      //printf("row basic ") ;
      //printf("- make column basic %s\n",basic ? "yes" : "no") ;
      if (basic&&!prob->rowIsBasic(iRow)) {
#ifdef PRINTCOST        
        printf("Singleton %d had coeff of %g in row %d (dual %g) - bounds %g %g - cost %g, (dj %g - new %g)\n",
               iCol,coeff,iRow,rowduals[iRow],clo[iCol],cup[iCol],dcost[iCol],rcosts[iCol],cost) ;
#endif
#ifdef COIN_DEVELOP
        if (prob->columnIsBasic(iCol)) 
          printf("column basic!\n") ;
#endif
        basic=false ;
      }
      if (fabs(rowduals[iRow])>1.0e-6&&prob->rowIsBasic(iRow))
        basic=true ;
      if (basic) {
        // Make basic have zero reduced cost 
	rowduals[iRow] = rcosts[iCol] / coeff ;
	rcosts[iCol] = 0.0 ;
      } else {
        rcosts[iCol]=cost ;
        //rowduals[iRow]=0.0 ;
      }
      if (colstat) {
        if (basic) {
          if (!prob->rowIsBasic(iRow)) {
#if 0
            // find column in row
            int jCol=-1 ;
            //for (CoinBigIndex j=mrstrt[iRow];j<mrstrt
            for (int k=0;k<prob->ncols0_;k++) {
              CoinBigIndex j=mcstrt[k] ;
              for (int i=0;i<hincol[k];i++) {
                if (hrow[k]==iRow) {
                  break ;
                }
                k=link[k] ;
              }
            }
#endif
          } else {
            prob->setColumnStatus(iCol,CoinPrePostsolveMatrix::basic) ;
          }
          prob->setRowStatusUsingValue(iRow) ;
        } else {
          //prob->setRowStatus(iRow,CoinPrePostsolveMatrix::basic) ;
          prob->setColumnStatusUsingValue(iCol) ;
        }
      }
    }
#if 0
    int nb=0 ;
    int kk ;
    for (kk=0;kk<prob->nrows_;kk++)
      if (prob->rowIsBasic(kk))
        nb++ ;
    for (kk=0;kk<prob->ncols_;kk++)
      if (prob->columnIsBasic(kk))
        nb++ ;
    assert (nb==prob->nrows_) ;
#endif
    // add new element
    {
      CoinBigIndex k = free_list ;
      assert(k >= 0 && k < prob->bulk0_) ;
      free_list = link[free_list] ;
      hrow[k] = iRow ;
      colels[k] = coeff ;
      link[k] = mcstrt[iCol] ;
      mcstrt[iCol] = k ;
    }
    hincol[iCol]++;	// right?
#ifdef CHECK_ONE_ROW
    {
      double act=0.0 ;
      for (int i=0;i<prob->ncols_;i++) {
	double solV = sol[i] ;
	assert (solV>=clo[i]-ztolzb&&solV<=cup[i]+ztolzb) ;
	int j=mcstrt[i] ;
	for (int k=0;k<hincol[i];k++) {
	  if (hrow[j]==CHECK_ONE_ROW) {
	    //printf("c %d el %g sol %g old act %g new %g\n",
	    //   i,colels[j],solV,act, act+colels[j]*solV) ;
	    act += colels[j]*solV ;
	  }
	  j=link[j] ;
	}
      }
      assert (act>=rlo[CHECK_ONE_ROW]-ztolzb&&act<=rup[CHECK_ONE_ROW]+ztolzb) ;
      printf("rhs now %g %g %g %g\n",rlo[CHECK_ONE_ROW],act,acts[CHECK_ONE_ROW],rup[CHECK_ONE_ROW]) ;
    }
#endif

#   if PRESOLVE_DEBUG
    rdone[iRow] = SLACK_SINGLETON ;
#   endif
  }

# if PRESOLVE_DEBUG
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
  std::cout << "Leaving slack_singleton_action::postsolve." << std::endl ;
# endif

  return ; 
}

