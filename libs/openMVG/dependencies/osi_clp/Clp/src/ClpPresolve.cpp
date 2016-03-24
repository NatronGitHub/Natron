/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

//#define	PRESOLVE_CONSISTENCY	1
//#define	PRESOLVE_DEBUG	1

#include <stdio.h>

#include <cassert>
#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "ClpConfig.h"
#ifdef CLP_HAS_ABC
#include "CoinAbcCommon.hpp"
#endif

#include "CoinPackedMatrix.hpp"
#include "ClpPackedMatrix.hpp"
#include "ClpSimplex.hpp"
#include "ClpSimplexOther.hpp"
#ifndef SLIM_CLP
#include "ClpQuadraticObjective.hpp"
#endif

#include "ClpPresolve.hpp"
#include "CoinPresolveMatrix.hpp"

#include "CoinPresolveEmpty.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolvePsdebug.hpp"
#include "CoinPresolveSingleton.hpp"
#include "CoinPresolveDoubleton.hpp"
#include "CoinPresolveTripleton.hpp"
#include "CoinPresolveZeros.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinPresolveForcing.hpp"
#include "CoinPresolveDual.hpp"
#include "CoinPresolveTighten.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinPresolveDupcol.hpp"
#include "CoinPresolveImpliedFree.hpp"
#include "CoinPresolveIsolated.hpp"
#include "CoinMessage.hpp"



ClpPresolve::ClpPresolve() :
     originalModel_(NULL),
     presolvedModel_(NULL),
     nonLinearValue_(0.0),
     originalColumn_(NULL),
     originalRow_(NULL),
     rowObjective_(NULL),
     paction_(0),
     ncols_(0),
     nrows_(0),
     nelems_(0),
#ifdef ABC_INHERIT
     numberPasses_(20),
#else
     numberPasses_(5),
#endif
     substitution_(3),
#ifndef CLP_NO_STD
     saveFile_(""),
#endif
     presolveActions_(0)
{
}

ClpPresolve::~ClpPresolve()
{
     destroyPresolve();
}
// Gets rid of presolve actions (e.g.when infeasible)
void
ClpPresolve::destroyPresolve()
{
     const CoinPresolveAction *paction = paction_;
     while (paction) {
          const CoinPresolveAction *next = paction->next;
          delete paction;
          paction = next;
     }
     delete [] originalColumn_;
     delete [] originalRow_;
     paction_ = NULL;
     originalColumn_ = NULL;
     originalRow_ = NULL;
     delete [] rowObjective_;
     rowObjective_ = NULL;
}

/* This version of presolve returns a pointer to a new presolved
   model.  NULL if infeasible
*/
ClpSimplex *
ClpPresolve::presolvedModel(ClpSimplex & si,
                            double feasibilityTolerance,
                            bool keepIntegers,
                            int numberPasses,
                            bool dropNames,
                            bool doRowObjective,
			    const char * prohibitedRows,
			    const char * prohibitedColumns)
{
     // Check matrix
     int checkType = ((si.specialOptions() & 128) != 0) ? 14 : 15;
     if (!si.clpMatrix()->allElementsInRange(&si, si.getSmallElementValue(),
                                             1.0e20,checkType))
          return NULL;
     else
          return gutsOfPresolvedModel(&si, feasibilityTolerance, keepIntegers, numberPasses, dropNames,
                                      doRowObjective,
				      prohibitedRows,
				      prohibitedColumns);
}
#ifndef CLP_NO_STD
/* This version of presolve updates
   model and saves original data to file.  Returns non-zero if infeasible
*/
int
ClpPresolve::presolvedModelToFile(ClpSimplex &si, std::string fileName,
                                  double feasibilityTolerance,
                                  bool keepIntegers,
                                  int numberPasses,
				  bool dropNames,
                                  bool doRowObjective)
{
     // Check matrix
     if (!si.clpMatrix()->allElementsInRange(&si, si.getSmallElementValue(),
                                             1.0e20))
          return 2;
     saveFile_ = fileName;
     si.saveModel(saveFile_.c_str());
     ClpSimplex * model = gutsOfPresolvedModel(&si, feasibilityTolerance, keepIntegers, numberPasses, dropNames,
                          doRowObjective);
     if (model == &si) {
          return 0;
     } else {
          si.restoreModel(saveFile_.c_str());
          remove(saveFile_.c_str());
          return 1;
     }
}
#endif
// Return pointer to presolved model
ClpSimplex *
ClpPresolve::model() const
{
     return presolvedModel_;
}
// Return pointer to original model
ClpSimplex *
ClpPresolve::originalModel() const
{
     return originalModel_;
}
// Return presolve status (0,1,2)
int 
ClpPresolve::presolveStatus() const
{
  if (nelems_>=0) {
    // feasible (or not done yet)
    return 0;
  } else {
    int presolveStatus = - nelems_;
    // If both infeasible and unbounded - say infeasible
    if (presolveStatus>2)
      presolveStatus = 1;
    return presolveStatus;
  }
}
void
ClpPresolve::postsolve(bool updateStatus)
{
     // Return at once if no presolved model
     if (!presolvedModel_)
          return;
     // Messages
     CoinMessages messages = originalModel_->coinMessages();
     if (!presolvedModel_->isProvenOptimal()) {
          presolvedModel_->messageHandler()->message(COIN_PRESOLVE_NONOPTIMAL,
                    messages)
                    << CoinMessageEol;
     }

     // this is the size of the original problem
     const int ncols0  = ncols_;
     const int nrows0  = nrows_;
     const CoinBigIndex nelems0 = nelems_;

     // this is the reduced problem
     int ncols = presolvedModel_->getNumCols();
     int nrows = presolvedModel_->getNumRows();

     double * acts = NULL;
     double * sol = NULL;
     unsigned char * rowstat = NULL;
     unsigned char * colstat = NULL;
#ifndef CLP_NO_STD
     if (saveFile_ == "") {
#endif
          // reality check
          assert(ncols0 == originalModel_->getNumCols());
          assert(nrows0 == originalModel_->getNumRows());
          acts = originalModel_->primalRowSolution();
          sol  = originalModel_->primalColumnSolution();
          if (updateStatus) {
               // postsolve does not know about fixed
               int i;
               for (i = 0; i < nrows + ncols; i++) {
                    if (presolvedModel_->getColumnStatus(i) == ClpSimplex::isFixed)
                         presolvedModel_->setColumnStatus(i, ClpSimplex::atLowerBound);
               }
               unsigned char *status = originalModel_->statusArray();
               if (!status) {
                    originalModel_->createStatus();
                    status = originalModel_->statusArray();
               }
               rowstat = status + ncols0;
               colstat = status;
               CoinMemcpyN( presolvedModel_->statusArray(), ncols, colstat);
               CoinMemcpyN( presolvedModel_->statusArray() + ncols, nrows, rowstat);
          }
#ifndef CLP_NO_STD
     } else {
          // from file
          acts = new double[nrows0];
          sol  = new double[ncols0];
          CoinZeroN(acts, nrows0);
          CoinZeroN(sol, ncols0);
          if (updateStatus) {
               unsigned char *status = new unsigned char [nrows0+ncols0];
               rowstat = status + ncols0;
               colstat = status;
               CoinMemcpyN( presolvedModel_->statusArray(), ncols, colstat);
               CoinMemcpyN( presolvedModel_->statusArray() + ncols, nrows, rowstat);
          }
     }
#endif

     // CoinPostsolveMatrix object assumes ownership of sol, acts, colstat;
     // will be deleted by ~CoinPostsolveMatrix. delete[] operations below
     // cause duplicate free. In case where saveFile == "", as best I can see
     // arrays are owned by originalModel_. fix is to
     // clear fields in prob to avoid delete[] in ~CoinPostsolveMatrix.
     CoinPostsolveMatrix prob(presolvedModel_,
                              ncols0,
                              nrows0,
                              nelems0,
                              presolvedModel_->getObjSense(),
                              // end prepost

                              sol, acts,
                              colstat, rowstat);

     postsolve(prob);

#ifndef CLP_NO_STD
     if (saveFile_ != "") {
          // From file
          assert (originalModel_ == presolvedModel_);
          originalModel_->restoreModel(saveFile_.c_str());
          remove(saveFile_.c_str());
          CoinMemcpyN(acts, nrows0, originalModel_->primalRowSolution());
          // delete [] acts;
          CoinMemcpyN(sol, ncols0, originalModel_->primalColumnSolution());
          // delete [] sol;
          if (updateStatus) {
               CoinMemcpyN(colstat, nrows0 + ncols0, originalModel_->statusArray());
               // delete [] colstat;
          }
     } else {
#endif
          prob.sol_ = 0 ;
          prob.acts_ = 0 ;
          prob.colstat_ = 0 ;
#ifndef CLP_NO_STD
     }
#endif 
     // put back duals
     CoinMemcpyN(prob.rowduals_,	nrows_, originalModel_->dualRowSolution());
     double maxmin = originalModel_->getObjSense();
     if (maxmin < 0.0) {
          // swap signs
          int i;
          double * pi = originalModel_->dualRowSolution();
          for (i = 0; i < nrows_; i++)
               pi[i] = -pi[i];
     }
     // Now check solution
     double offset;
     CoinMemcpyN(originalModel_->objectiveAsObject()->gradient(originalModel_,
                 originalModel_->primalColumnSolution(), offset, true),
                 ncols_, originalModel_->dualColumnSolution());
     originalModel_->clpMatrix()->transposeTimes(-1.0,
                                    originalModel_->dualRowSolution(),
                                    originalModel_->dualColumnSolution());
     memset(originalModel_->primalRowSolution(), 0, nrows_ * sizeof(double));
     originalModel_->clpMatrix()->times(1.0, 
					originalModel_->primalColumnSolution(),
					originalModel_->primalRowSolution());
     originalModel_->checkSolutionInternal();
     if (originalModel_->sumDualInfeasibilities() > 1.0e-1) {
          // See if we can fix easily
          static_cast<ClpSimplexOther *> (originalModel_)->cleanupAfterPostsolve();
     }
     // Messages
     presolvedModel_->messageHandler()->message(COIN_PRESOLVE_POSTSOLVE,
               messages)
               << originalModel_->objectiveValue()
               << originalModel_->sumDualInfeasibilities()
               << originalModel_->numberDualInfeasibilities()
               << originalModel_->sumPrimalInfeasibilities()
               << originalModel_->numberPrimalInfeasibilities()
               << CoinMessageEol;

     //originalModel_->objectiveValue_=objectiveValue_;
     originalModel_->setNumberIterations(presolvedModel_->numberIterations());
     if (!presolvedModel_->status()) {
          if (!originalModel_->numberDualInfeasibilities() &&
                    !originalModel_->numberPrimalInfeasibilities()) {
               originalModel_->setProblemStatus( 0);
          } else {
               originalModel_->setProblemStatus( -1);
               // Say not optimal after presolve
               originalModel_->setSecondaryStatus(7);
               presolvedModel_->messageHandler()->message(COIN_PRESOLVE_NEEDS_CLEANING,
                         messages)
                         << CoinMessageEol;
          }
     } else {
          originalModel_->setProblemStatus( presolvedModel_->status());
	  // but not if close to feasible
	  if( originalModel_->sumPrimalInfeasibilities()<1.0e-1) {
               originalModel_->setProblemStatus( -1);
               // Say not optimal after presolve
               originalModel_->setSecondaryStatus(7);
	  }
     }
#ifndef CLP_NO_STD
     if (saveFile_ != "")
          presolvedModel_ = NULL;
#endif
}

// return pointer to original columns
const int *
ClpPresolve::originalColumns() const
{
     return originalColumn_;
}
// return pointer to original rows
const int *
ClpPresolve::originalRows() const
{
     return originalRow_;
}
// Set pointer to original model
void
ClpPresolve::setOriginalModel(ClpSimplex * model)
{
     originalModel_ = model;
}
#if 0
// A lazy way to restrict which transformations are applied
// during debugging.
static int ATOI(const char *name)
{
     return true;
#if	PRESOLVE_DEBUG || PRESOLVE_SUMMARY
     if (getenv(name)) {
          int val = atoi(getenv(name));
          printf("%s = %d\n", name, val);
          return (val);
     } else {
          if (strcmp(name, "off"))
               return (true);
          else
               return (false);
     }
#else
     return (true);
#endif
}
#endif
//#define PRESOLVE_DEBUG 1
#if PRESOLVE_DEBUG
void check_sol(CoinPresolveMatrix *prob, double tol)
{
     double *colels	= prob->colels_;
     int *hrow		= prob->hrow_;
     int *mcstrt		= prob->mcstrt_;
     int *hincol		= prob->hincol_;
     int *hinrow		= prob->hinrow_;
     int ncols		= prob->ncols_;


     double * csol = prob->sol_;
     double * acts = prob->acts_;
     double * clo = prob->clo_;
     double * cup = prob->cup_;
     int nrows = prob->nrows_;
     double * rlo = prob->rlo_;
     double * rup = prob->rup_;

     int colx;

     double * rsol = new double[nrows];
     memset(rsol, 0, nrows * sizeof(double));

     for (colx = 0; colx < ncols; ++colx) {
          if (1) {
               CoinBigIndex k = mcstrt[colx];
               int nx = hincol[colx];
               double solutionValue = csol[colx];
               for (int i = 0; i < nx; ++i) {
                    int row = hrow[k];
                    double coeff = colels[k];
                    k++;
                    rsol[row] += solutionValue * coeff;
               }
               if (csol[colx] < clo[colx] - tol) {
                    printf("low CSOL:  %d  - %g %g %g\n",
                           colx, clo[colx], csol[colx], cup[colx]);
               } else if (csol[colx] > cup[colx] + tol) {
                    printf("high CSOL:  %d  - %g %g %g\n",
                           colx, clo[colx], csol[colx], cup[colx]);
               }
          }
     }
     int rowx;
     for (rowx = 0; rowx < nrows; ++rowx) {
          if (hinrow[rowx]) {
               if (fabs(rsol[rowx] - acts[rowx]) > tol)
                    printf("inacc RSOL:  %d - %g %g (acts_ %g) %g\n",
                           rowx,  rlo[rowx], rsol[rowx], acts[rowx], rup[rowx]);
               if (rsol[rowx] < rlo[rowx] - tol) {
                    printf("low RSOL:  %d - %g %g %g\n",
                           rowx,  rlo[rowx], rsol[rowx], rup[rowx]);
               } else if (rsol[rowx] > rup[rowx] + tol ) {
                    printf("high RSOL:  %d - %g %g %g\n",
                           rowx,  rlo[rowx], rsol[rowx], rup[rowx]);
               }
          }
     }
     delete [] rsol;
}
#endif
static int tightenDoubletons2(CoinPresolveMatrix * prob)
{
  // column-major representation
  const int ncols = prob->ncols_ ;
  const CoinBigIndex *const mcstrt = prob->mcstrt_ ;
  const int *const hincol = prob->hincol_ ;
  const int *const hrow = prob->hrow_ ;
  double * colels = prob->colels_ ;
  double * cost = prob->cost_ ;

  // column type, bounds, solution, and status
  const unsigned char *const integerType = prob->integerType_ ;
  double * clo = prob->clo_ ;
  double * cup = prob->cup_ ;
  // row-major representation
  //const int nrows = prob->nrows_ ;
  const CoinBigIndex *const mrstrt = prob->mrstrt_ ;
  const int *const hinrow = prob->hinrow_ ;
  const int *const hcol = prob->hcol_ ;
  double * rowels = prob->rowels_ ;

  // row bounds
  double *const rlo = prob->rlo_ ;
  double *const rup = prob->rup_ ;

  // tolerances
  //const double ekkinf2 = PRESOLVE_SMALL_INF ;
  //const double ekkinf = ekkinf2*1.0e8 ;
  //const double ztolcbarj = prob->ztoldj_ ;
  //const CoinRelFltEq relEq(prob->ztolzb_) ;
  int numberChanged=0;
  double bound[2];
  double alpha[2]={0.0,0.0};
  double offset=0.0;

  for (int icol=0;icol<ncols;icol++) {
    if (hincol[icol]==2) {
      CoinBigIndex start=mcstrt[icol];
      int row0 = hrow[start];
      if (hinrow[row0]!=2)
	continue;
      int row1 = hrow[start+1];
      if (hinrow[row1]!=2)
	continue;
      double element0 = colels[start];
      double rowUpper0=rup[row0];
      bool swapSigns0=false;
      if (rlo[row0]>-1.0e30) {
	if (rup[row0]>1.0e30) {
	  swapSigns0=true;
	  rowUpper0=-rlo[row0];
	  element0=-element0;
	} else {
	  // range or equality
	  continue;
	}
      } else if (rup[row0]>1.0e30) {
	// free
	continue;
      }
#if 0
      // skip here for speed
      // skip if no cost (should be able to get rid of)
      if (!cost[icol]) {
	printf("should be able to get rid of %d with no cost\n",icol);
	continue;
      }
      // skip if negative cost for now
      if (cost[icol]<0.0) {
	printf("code for negative cost\n");
	continue;
      }
#endif
      double element1 = colels[start+1];
      double rowUpper1=rup[row1];
      bool swapSigns1=false;
      if (rlo[row1]>-1.0e30) {
	if (rup[row1]>1.0e30) {
	  swapSigns1=true;
	  rowUpper1=-rlo[row1];
	  element1=-element1;
	} else {
	  // range or equality
	  continue;
	}
      } else if (rup[row1]>1.0e30) {
	// free
	continue;
      }
      double lowerX=clo[icol];
      double upperX=cup[icol];
      int otherCol=-1;
      CoinBigIndex startRow=mrstrt[row0];
      for (CoinBigIndex j=startRow;j<startRow+2;j++) {
	int jcol=hcol[j];
	if (jcol!=icol) {
	  alpha[0]=swapSigns0 ? -rowels[j] :rowels[j];
	  otherCol=jcol;
	}
      }
      startRow=mrstrt[row1];
      bool possible=true;
      for (CoinBigIndex j=startRow;j<startRow+2;j++) {
	int jcol=hcol[j];
	if (jcol!=icol) {
	  if (jcol==otherCol) {
	    alpha[1]=swapSigns1 ? -rowels[j] :rowels[j];
	  } else {
	    possible=false;
	  }
	}
      }
      if (possible) {
	// skip if no cost (should be able to get rid of)
	if (!cost[icol]) {
	  PRESOLVE_DETAIL_PRINT(printf("should be able to get rid of %d with no cost\n",icol));
	  continue;
	}
	// skip if negative cost for now
	if (cost[icol]<0.0) {
	  PRESOLVE_DETAIL_PRINT(printf("code for negative cost\n"));
	  continue;
	}
	bound[0]=clo[otherCol];
	bound[1]=cup[otherCol];
	double lowestLowest=COIN_DBL_MAX;
	double highestLowest=-COIN_DBL_MAX;
	double lowestHighest=COIN_DBL_MAX;
	double highestHighest=-COIN_DBL_MAX;
	int binding0=0;
	int binding1=0;
	for (int k=0;k<2;k++) {
	  bool infLow0=false;
	  bool infLow1=false;
	  double sum0=0.0;
	  double sum1=0.0;
	  double value=bound[k];
	  if (fabs(value)<1.0e30) {
	    sum0+=alpha[0]*value;
	    sum1+=alpha[1]*value;
	  } else {
	    if (alpha[0]>0.0) {
	      if (value<0.0)
		infLow0 =true;
	    } else if (alpha[0]<0.0) {
	      if (value>0.0)
		infLow0 =true;
	    }
	    if (alpha[1]>0.0) {
	      if (value<0.0)
		infLow1 =true;
	    } else if (alpha[1]<0.0) {
	      if (value>0.0)
		infLow1 =true;
	    }
	  }
	  /* Got sums
	   */
	  double thisLowest0=-COIN_DBL_MAX;
	  double thisHighest0=COIN_DBL_MAX;
	  if (element0>0.0) {
	    // upper bound unless inf&2 !=0
	    if (!infLow0)
	      thisHighest0 = (rowUpper0-sum0)/element0;
	  } else {
	    // lower bound unless inf&2 !=0
	    if (!infLow0)
	      thisLowest0 = (rowUpper0-sum0)/element0;
	  }
	  double thisLowest1=-COIN_DBL_MAX;
	  double thisHighest1=COIN_DBL_MAX;
	  if (element1>0.0) {
	    // upper bound unless inf&2 !=0
	    if (!infLow1)
	      thisHighest1 = (rowUpper1-sum1)/element1;
	  } else {
	    // lower bound unless inf&2 !=0
	    if (!infLow1)
	      thisLowest1 = (rowUpper1-sum1)/element1;
	  }
	  if (thisLowest0>thisLowest1+1.0e-12) {
	    if (thisLowest0>lowerX+1.0e-12)
	      binding0|= 1<<k;
	  } else if (thisLowest1>thisLowest0+1.0e-12) {
	    if (thisLowest1>lowerX+1.0e-12)
	      binding1|= 1<<k;
	    thisLowest0=thisLowest1;
	  }
	  if (thisHighest0<thisHighest1-1.0e-12) {
	    if (thisHighest0<upperX-1.0e-12)
	      binding0|= 1<<k;
	  } else if (thisHighest1<thisHighest0-1.0e-12) {
	    if (thisHighest1<upperX-1.0e-12)
	      binding1|= 1<<k;
	    thisHighest0=thisHighest1;
	  }
	  lowestLowest=CoinMin(lowestLowest,thisLowest0);
	  highestHighest=CoinMax(highestHighest,thisHighest0);
	  lowestHighest=CoinMin(lowestHighest,thisHighest0);
	  highestLowest=CoinMax(highestLowest,thisLowest0);
	}
	// see if any good
	//#define PRINT_VALUES
	if (!binding0||!binding1) {
	  PRESOLVE_DETAIL_PRINT(printf("Row redundant for column %d\n",icol));
	} else {
#ifdef PRINT_VALUES
	  printf("Column %d bounds %g,%g lowest %g,%g highest %g,%g\n",
		 icol,lowerX,upperX,lowestLowest,lowestHighest,
		 highestLowest,highestHighest);
#endif
	  // if integer adjust
	  if (integerType[icol]) {
	    lowestLowest=ceil(lowestLowest-1.0e-5);
	    highestLowest=ceil(highestLowest-1.0e-5);
	    lowestHighest=floor(lowestHighest+1.0e-5);
	    highestHighest=floor(highestHighest+1.0e-5);
	  }
	  // if costed may be able to adjust
	  if (cost[icol]>=0.0) {
	    if (highestLowest<upperX&&highestLowest>=lowerX&&highestHighest<1.0e30) {
	      highestHighest=CoinMin(highestHighest,highestLowest);
	    }
	  }
	  if (cost[icol]<=0.0) {
	    if (lowestHighest>lowerX&&lowestHighest<=upperX&&lowestHighest>-1.0e30) {
	      lowestLowest=CoinMax(lowestLowest,lowestHighest);
	    }
	  }
#if 1
	  if (lowestLowest>lowerX+1.0e-8) {
#ifdef PRINT_VALUES
	    printf("Can increase lower bound on %d from %g to %g\n",
		   icol,lowerX,lowestLowest);
#endif
	    lowerX=lowestLowest;
	  }
	  if (highestHighest<upperX-1.0e-8) {
#ifdef PRINT_VALUES
	    printf("Can decrease upper bound on %d from %g to %g\n",
		   icol,upperX,highestHighest);
#endif
	    upperX=highestHighest;
	    
	  }
#endif
	  // see if we can move costs
	  double xValue;
	  double yValue0;
	  double yValue1;
	  double newLower=COIN_DBL_MAX;
	  double newUpper=-COIN_DBL_MAX;
#ifdef PRINT_VALUES
	  double ranges0[2];
	  double ranges1[2];
#endif
	  double costEqual;
	  double slope[2];
	  assert (binding0+binding1==3);
	  // get where equal
	  xValue=(rowUpper0*element1-rowUpper1*element0)/(alpha[0]*element1-alpha[1]*element0);
	  yValue0=(rowUpper0-xValue*alpha[0])/element0;
	  yValue1=(rowUpper1-xValue*alpha[1])/element1;
	  newLower=CoinMin(newLower,CoinMax(yValue0,yValue1));
	  newUpper=CoinMax(newUpper,CoinMax(yValue0,yValue1));
	  double xValueEqual=xValue;
	  double yValueEqual=yValue0;
	  costEqual = xValue*cost[otherCol]+yValueEqual*cost[icol];
	  if (binding0==1) {
#ifdef PRINT_VALUES
	    ranges0[0]=bound[0];
	    ranges0[1]=yValue0;
	    ranges1[0]=yValue0;
	    ranges1[1]=bound[1];
#endif
	    // take x 1.0 down
	    double x=xValue-1.0;
	    double y=(rowUpper0-x*alpha[0])/element0;
	    double costTotal = x*cost[otherCol]+y*cost[icol];
	    slope[0] = costEqual-costTotal;
	    // take x 1.0 up
	    x=xValue+1.0;
	    y=(rowUpper1-x*alpha[1])/element0;
	    costTotal = x*cost[otherCol]+y*cost[icol];
	    slope[1] = costTotal-costEqual;
	  } else {
#ifdef PRINT_VALUES
	    ranges1[0]=bound[0];
	    ranges1[1]=yValue0;
	    ranges0[0]=yValue0;
	    ranges0[1]=bound[1];
#endif
	    // take x 1.0 down
	    double x=xValue-1.0;
	    double y=(rowUpper1-x*alpha[1])/element0;
	    double costTotal = x*cost[otherCol]+y*cost[icol];
	    slope[1] = costEqual-costTotal;
	    // take x 1.0 up
	    x=xValue+1.0;
	    y=(rowUpper0-x*alpha[0])/element0;
	    costTotal = x*cost[otherCol]+y*cost[icol];
	    slope[0] = costTotal-costEqual;
	  }
#ifdef PRINT_VALUES
	  printf("equal value of %d is %g, value of %d is max(%g,%g) - %g\n",
		 otherCol,xValue,icol,yValue0,yValue1,CoinMax(yValue0,yValue1));
	  printf("Cost at equality %g for constraint 0 ranges %g -> %g slope %g for constraint 1 ranges %g -> %g slope %g\n",
		 costEqual,ranges0[0],ranges0[1],slope[0],ranges1[0],ranges1[1],slope[1]);
#endif
	  xValue=bound[0];
	  yValue0=(rowUpper0-xValue*alpha[0])/element0;
	  yValue1=(rowUpper1-xValue*alpha[1])/element1;
#ifdef PRINT_VALUES
	  printf("value of %d is %g, value of %d is max(%g,%g) - %g\n",
		 otherCol,xValue,icol,yValue0,yValue1,CoinMax(yValue0,yValue1));
#endif
	  newLower=CoinMin(newLower,CoinMax(yValue0,yValue1));
	  // cost>0 so will be at lower
	  //double yValueAtBound0=newLower;
	  newUpper=CoinMax(newUpper,CoinMax(yValue0,yValue1));
	  xValue=bound[1];
	  yValue0=(rowUpper0-xValue*alpha[0])/element0;
	  yValue1=(rowUpper1-xValue*alpha[1])/element1;
#ifdef PRINT_VALUES
	  printf("value of %d is %g, value of %d is max(%g,%g) - %g\n",
		 otherCol,xValue,icol,yValue0,yValue1,CoinMax(yValue0,yValue1));
#endif
	  newLower=CoinMin(newLower,CoinMax(yValue0,yValue1));
	  // cost>0 so will be at lower
	  //double yValueAtBound1=newLower;
	  newUpper=CoinMax(newUpper,CoinMax(yValue0,yValue1));
	  lowerX=CoinMax(lowerX,newLower-1.0e-12*fabs(newLower));
	  upperX=CoinMin(upperX,newUpper+1.0e-12*fabs(newUpper));
	  // Now make duplicate row
	  // keep row 0 so need to adjust costs so same
#ifdef PRINT_VALUES
	  printf("Costs for x %g,%g,%g are %g,%g,%g\n",
		 xValueEqual-1.0,xValueEqual,xValueEqual+1.0,
		 costEqual-slope[0],costEqual,costEqual+slope[1]);
#endif
	  double costOther=cost[otherCol]+slope[1];
	  double costThis=cost[icol]+slope[1]*(element0/alpha[0]);
	  xValue=xValueEqual;
	  yValue0=CoinMax((rowUpper0-xValue*alpha[0])/element0,lowerX);
	  double thisOffset=costEqual-(costOther*xValue+costThis*yValue0);
	  offset += thisOffset;
#ifdef PRINT_VALUES
	  printf("new cost at equal %g\n",costOther*xValue+costThis*yValue0+thisOffset);
#endif
	  xValue=xValueEqual-1.0;
	  yValue0=CoinMax((rowUpper0-xValue*alpha[0])/element0,lowerX);
#ifdef PRINT_VALUES
	  printf("new cost at -1 %g\n",costOther*xValue+costThis*yValue0+thisOffset);
#endif
	  assert(fabs((costOther*xValue+costThis*yValue0+thisOffset)-(costEqual-slope[0]))<1.0e-5);
	  xValue=xValueEqual+1.0;
	  yValue0=CoinMax((rowUpper0-xValue*alpha[0])/element0,lowerX);
#ifdef PRINT_VALUES
	  printf("new cost at +1 %g\n",costOther*xValue+costThis*yValue0+thisOffset);
#endif
	  assert(fabs((costOther*xValue+costThis*yValue0+thisOffset)-(costEqual+slope[1]))<1.0e-5);
	  numberChanged++;
	  //	  continue;
	  cost[otherCol] = costOther;
	  cost[icol] = costThis;
	  clo[icol]=lowerX;
	  cup[icol]=upperX;
	  int startCol[2];
	  int endCol[2];
	  startCol[0]=mcstrt[icol];
	  endCol[0]=startCol[0]+2;
	  startCol[1]=mcstrt[otherCol];
	  endCol[1]=startCol[1]+hincol[otherCol];
	  double values[2]={0.0,0.0};
	  for (int k=0;k<2;k++) {
	    for (CoinBigIndex i=startCol[k];i<endCol[k];i++) {
	      if (hrow[i]==row0)
		values[k]=colels[i];
	    }
	    for (CoinBigIndex i=startCol[k];i<endCol[k];i++) {
	      if (hrow[i]==row1)
		colels[i]=values[k];
	    }
	  }
	  for (CoinBigIndex i=mrstrt[row1];i<mrstrt[row1]+2;i++) {
	    if (hcol[i]==icol)
	      rowels[i]=values[0];
	    else
	      rowels[i]=values[1];
	  }
	}
      }
    }
  }
#if ABC_NORMAL_DEBUG>0
  if (offset)
    printf("Cost offset %g\n",offset);
#endif
  return numberChanged;
}
//#define COIN_PRESOLVE_BUG
#ifdef COIN_PRESOLVE_BUG
static int counter=1000000;
static int startEmptyRows=0;
static int startEmptyColumns=0;
static bool break2(CoinPresolveMatrix *prob)
{
  int droppedRows = prob->countEmptyRows() - startEmptyRows ;
  int droppedColumns =  prob->countEmptyCols() - startEmptyColumns;
  startEmptyRows=prob->countEmptyRows();
  startEmptyColumns=prob->countEmptyCols();
  printf("Dropped %d rows and %d columns - current empty %d, %d\n",droppedRows,
	 droppedColumns,startEmptyRows,startEmptyColumns);
  counter--;
  if (!counter) {
    printf("skipping next and all\n");
  }
  return (counter<=0);
}
#define possibleBreak if (break2(prob)) break
#define possibleSkip  if (!break2(prob)) 
#else
#define possibleBreak
#define possibleSkip
#endif
#define SOME_PRESOLVE_DETAIL
#ifndef SOME_PRESOLVE_DETAIL
#define printProgress(x,y) {}
#else
#define printProgress(x,y) {if ((presolveActions_ & 0x80000000) != 0)	\
      printf("%c loop %d %d empty rows, %d empty columns\n",x,y,prob->countEmptyRows(), \
	   prob->countEmptyCols());}
#endif
// This is the presolve loop.
// It is a separate virtual function so that it can be easily
// customized by subclassing CoinPresolve.
const CoinPresolveAction *ClpPresolve::presolve(CoinPresolveMatrix *prob)
{
     // Messages
     CoinMessages messages = CoinMessage(prob->messages().language());
     paction_ = 0;
     prob->maxSubstLevel_ = 3 ;
#ifndef PRESOLVE_DETAIL
     if (prob->tuning_) {
#endif
       int numberEmptyRows=0;
       for ( int i=0;i<prob->nrows_;i++) {
	 if (!prob->hinrow_[i]) {
	   PRESOLVE_DETAIL_PRINT(printf("pre_empty row %d\n",i));
	   //printf("pre_empty row %d\n",i);
	   numberEmptyRows++;
	 }
       }
       int numberEmptyCols=0;
       for ( int i=0;i<prob->ncols_;i++) {
	 if (!prob->hincol_[i]) {
	   PRESOLVE_DETAIL_PRINT(printf("pre_empty col %d\n",i));
	   //printf("pre_empty col %d\n",i);
	   numberEmptyCols++;
	 }
       }
       printf("CoinPresolve initial state %d empty rows and %d empty columns\n",
	      numberEmptyRows,numberEmptyCols);
#ifndef PRESOLVE_DETAIL
     }
#endif
     prob->status_ = 0; // say feasible
     printProgress('A',0);
     paction_ = make_fixed(prob, paction_);
     paction_ = testRedundant(prob,paction_) ;
     printProgress('B',0);
     // if integers then switch off dual stuff
     // later just do individually
     bool doDualStuff = (presolvedModel_->integerInformation() == NULL);
     // but allow in some cases
     if ((presolveActions_ & 512) != 0)
          doDualStuff = true;
     if (prob->anyProhibited())
          doDualStuff = false;
     if (!doDual())
          doDualStuff = false;
#if	PRESOLVE_CONSISTENCY
//  presolve_links_ok(prob->rlink_, prob->mrstrt_, prob->hinrow_, prob->nrows_);
     presolve_links_ok(prob, false, true) ;
#endif

     if (!prob->status_) {
          bool slackSingleton = doSingletonColumn();
          slackSingleton = true;
          const bool slackd = doSingleton();
          const bool doubleton = doDoubleton();
          const bool tripleton = doTripleton();
          //#define NO_FORCING
#ifndef NO_FORCING
          const bool forcing = doForcing();
#endif
          const bool ifree = doImpliedFree();
          const bool zerocost = doTighten();
          const bool dupcol = doDupcol();
          const bool duprow = doDuprow();
          const bool dual = doDualStuff;

          // some things are expensive so just do once (normally)

          int i;
          // say look at all
          if (!prob->anyProhibited()) {
               for (i = 0; i < nrows_; i++)
                    prob->rowsToDo_[i] = i;
               prob->numberRowsToDo_ = nrows_;
               for (i = 0; i < ncols_; i++)
                    prob->colsToDo_[i] = i;
               prob->numberColsToDo_ = ncols_;
          } else {
               // some stuff must be left alone
               prob->numberRowsToDo_ = 0;
               for (i = 0; i < nrows_; i++)
                    if (!prob->rowProhibited(i))
                         prob->rowsToDo_[prob->numberRowsToDo_++] = i;
               prob->numberColsToDo_ = 0;
               for (i = 0; i < ncols_; i++)
                    if (!prob->colProhibited(i))
                         prob->colsToDo_[prob->numberColsToDo_++] = i;
          }

	    // transfer costs (may want to do it in OsiPresolve)
	    // need a transfer back at end of postsolve transferCosts(prob);

          int iLoop;
#if	PRESOLVE_DEBUG
          check_sol(prob, 1.0e0);
#endif
          if (dupcol) {
               // maybe allow integer columns to be checked
               if ((presolveActions_ & 512) != 0)
                    prob->setPresolveOptions(prob->presolveOptions() | 1);
	       possibleSkip;
               paction_ = dupcol_action::presolve(prob, paction_);
	       printProgress('C',0);
          }
#ifdef ABC_INHERIT
          if (doTwoxTwo()) {
	    possibleSkip;
	    paction_ = twoxtwo_action::presolve(prob, paction_);
          }
#endif
          if (duprow) {
	    possibleSkip;
	    if (doTwoxTwo()) {
	      int nTightened=tightenDoubletons2(prob);
	      if (nTightened)
		PRESOLVE_DETAIL_PRINT(printf("%d doubletons tightened\n",
					     nTightened));
	    }
	    paction_ = duprow_action::presolve(prob, paction_);
	    printProgress('D',0);
          }
          if (doGubrow()) {
	    possibleSkip;
               paction_ = gubrow_action::presolve(prob, paction_);
	       printProgress('E',0);
          }

          if ((presolveActions_ & 16384) != 0)
               prob->setPresolveOptions(prob->presolveOptions() | 16384);
	  // For inaccurate data in implied free
          if ((presolveActions_ & 1024) != 0)
               prob->setPresolveOptions(prob->presolveOptions() | 0x20000);
          // Check number rows dropped
          int lastDropped = 0;
          prob->pass_ = 0;
#ifdef ABC_INHERIT
	  int numberRowsStart=nrows_-prob->countEmptyRows();
	  int numberColumnsStart=ncols_-prob->countEmptyCols();
	  int numberRowsLeft=numberRowsStart;
	  int numberColumnsLeft=numberColumnsStart;
	  bool lastPassWasGood=true;
#if ABC_NORMAL_DEBUG
	  printf("Original rows,columns %d,%d starting first pass with %d,%d\n", 
		 nrows_,ncols_,numberRowsLeft,numberColumnsLeft);
#endif
#endif
	  if (numberPasses_<=5)
	      prob->presolveOptions_ |= 0x10000; // say more lightweight
          for (iLoop = 0; iLoop < numberPasses_; iLoop++) {
               // See if we want statistics
               if ((presolveActions_ & 0x80000000) != 0)
		 printf("Starting major pass %d after %g seconds with %d rows, %d columns\n", iLoop + 1, CoinCpuTime() - prob->startTime_,
			nrows_-prob->countEmptyRows(),
			ncols_-prob->countEmptyCols());
#ifdef PRESOLVE_SUMMARY
               printf("Starting major pass %d\n", iLoop + 1);
#endif
               const CoinPresolveAction * const paction0 = paction_;
               // look for substitutions with no fill
               //#define IMPLIED 3
#ifdef IMPLIED
               int fill_level = 3;
#define IMPLIED2 99
#if IMPLIED!=3
#if IMPLIED>2&&IMPLIED<11
               fill_level = IMPLIED;
               COIN_DETAIL_PRINT(printf("** fill_level == %d !\n", fill_level));
#endif
#if IMPLIED>11&&IMPLIED<21
               fill_level = -(IMPLIED - 10);
               COIN_DETAIL_PRINT(printf("** fill_level == %d !\n", fill_level));
#endif
#endif
#else
               int fill_level = prob->maxSubstLevel_;
#endif
               int whichPass = 0;
               while (1) {
                    whichPass++;
                    prob->pass_++;
                    const CoinPresolveAction * const paction1 = paction_;

                    if (slackd) {
                         bool notFinished = true;
                         while (notFinished) {
			   possibleBreak;
                              paction_ = slack_doubleton_action::presolve(prob, paction_,
                                         notFinished);
			 }
			 printProgress('F',iLoop+1);
                         if (prob->status_)
                              break;
                    }
                    if (dual && whichPass == 1) {
                         // this can also make E rows so do one bit here
		      possibleBreak;
                         paction_ = remove_dual_action::presolve(prob, paction_);
                         if (prob->status_)
                              break;
			 printProgress('G',iLoop+1);
                    }

                    if (doubleton) {
		      possibleBreak;
                         paction_ = doubleton_action::presolve(prob, paction_);
                         if (prob->status_)
                              break;
			 printProgress('H',iLoop+1);
                    }
                    if (tripleton) {
		      possibleBreak;
                         paction_ = tripleton_action::presolve(prob, paction_);
                         if (prob->status_)
                              break;
			 printProgress('I',iLoop+1);
                    }

                    if (zerocost) {
		      possibleBreak;
                         paction_ = do_tighten_action::presolve(prob, paction_);
                         if (prob->status_)
                              break;
			 printProgress('J',iLoop+1);
                    }
#ifndef NO_FORCING
                    if (forcing) {
		      possibleBreak;
                         paction_ = forcing_constraint_action::presolve(prob, paction_);
                         if (prob->status_)
                              break;
			 printProgress('K',iLoop+1);
                    }
#endif

                    if (ifree && (whichPass % 5) == 1) {
		      possibleBreak;
                         paction_ = implied_free_action::presolve(prob, paction_, fill_level);
                         if (prob->status_)
                              break;
			 printProgress('L',iLoop+1);
                    }

#if	PRESOLVE_DEBUG
                    check_sol(prob, 1.0e0);
#endif

#if	PRESOLVE_CONSISTENCY
//	presolve_links_ok(prob->rlink_, prob->mrstrt_, prob->hinrow_,
//			  prob->nrows_);
                    presolve_links_ok(prob, false, true) ;
#endif

//#if	PRESOLVE_DEBUG
//	presolve_no_zeros(prob->mcstrt_, prob->colels_, prob->hincol_,
//			  prob->ncols_);
//#endif
//#if	PRESOLVE_CONSISTENCY
//	prob->consistent();
//#endif
#if	PRESOLVE_CONSISTENCY
                    presolve_no_zeros(prob, true, false) ;
                    presolve_consistent(prob, true) ;
#endif

		    {
		      // set up for next pass
		      // later do faster if many changes i.e. memset and memcpy
		      const int * count = prob->hinrow_;
		      const int * nextToDo = prob->nextRowsToDo_;
		      int * toDo = prob->rowsToDo_;
		      int nNext = prob->numberNextRowsToDo_;
		      int n = 0;
		      for (int i = 0; i < nNext; i++) {
			int index = nextToDo[i];
			prob->unsetRowChanged(index);
			if (count[index]) 
			  toDo[n++] = index;
		      }
		      prob->numberRowsToDo_ = n;
		      prob->numberNextRowsToDo_ = 0;
		      count = prob->hincol_;
		      nextToDo = prob->nextColsToDo_;
		      toDo = prob->colsToDo_;
		      nNext = prob->numberNextColsToDo_;
		      n = 0;
		      for (int i = 0; i < nNext; i++) {
			int index = nextToDo[i];
			prob->unsetColChanged(index);
			if (count[index]) 
			  toDo[n++] = index;
		      }
		      prob->numberColsToDo_ = n;
		      prob->numberNextColsToDo_ = 0;
		    }
                    if (paction_ == paction1 && fill_level > 0)
                         break;
               }
               // say look at all
               int i;
               if (!prob->anyProhibited()) {
		 const int * count = prob->hinrow_;
		 int * toDo = prob->rowsToDo_;
		 int n = 0;
		 for (int i = 0; i < nrows_; i++) {
		   prob->unsetRowChanged(i);
		   if (count[i]) 
		     toDo[n++] = i;
		 }
		 prob->numberRowsToDo_ = n;
		 prob->numberNextRowsToDo_ = 0;
		 count = prob->hincol_;
		 toDo = prob->colsToDo_;
		 n = 0;
		 for (int i = 0; i < ncols_; i++) {
		   prob->unsetColChanged(i);
		   if (count[i]) 
		     toDo[n++] = i;
		 }
		 prob->numberColsToDo_ = n;
		 prob->numberNextColsToDo_ = 0;
               } else {
                    // some stuff must be left alone
                    prob->numberRowsToDo_ = 0;
                    for (i = 0; i < nrows_; i++)
                         if (!prob->rowProhibited(i))
                              prob->rowsToDo_[prob->numberRowsToDo_++] = i;
                    prob->numberColsToDo_ = 0;
                    for (i = 0; i < ncols_; i++)
                         if (!prob->colProhibited(i))
                              prob->colsToDo_[prob->numberColsToDo_++] = i;
               }
               // now expensive things
               // this caused world.mps to run into numerical difficulties
#ifdef PRESOLVE_SUMMARY
               printf("Starting expensive\n");
#endif

               if (dual) {
                    int itry;
                    for (itry = 0; itry < 5; itry++) {
		      possibleBreak;
                         paction_ = remove_dual_action::presolve(prob, paction_);
                         if (prob->status_)
                              break;
			 printProgress('M',iLoop+1);
                         const CoinPresolveAction * const paction2 = paction_;
                         if (ifree) {
#ifdef IMPLIED
#if IMPLIED2 ==0
                              int fill_level = 0; // switches off substitution
#elif IMPLIED2!=99
                              int fill_level = IMPLIED2;
#endif
#endif
                              if ((itry & 1) == 0) {
				possibleBreak;
                                   paction_ = implied_free_action::presolve(prob, paction_, fill_level);
			      }
                              if (prob->status_)
                                   break;
			      printProgress('N',iLoop+1);
                         }
                         if (paction_ == paction2)
                              break;
                    }
               } else if (ifree) {
                    // just free
#ifdef IMPLIED
#if IMPLIED2 ==0
                    int fill_level = 0; // switches off substitution
#elif IMPLIED2!=99
                    int fill_level = IMPLIED2;
#endif
#endif
		    possibleBreak;
                    paction_ = implied_free_action::presolve(prob, paction_, fill_level);
                    if (prob->status_)
                         break;
		    printProgress('O',iLoop+1);
               }
#if	PRESOLVE_DEBUG
               check_sol(prob, 1.0e0);
#endif
               if (dupcol) {
                    // maybe allow integer columns to be checked
                    if ((presolveActions_ & 512) != 0)
                         prob->setPresolveOptions(prob->presolveOptions() | 1);
		    possibleBreak;
                    paction_ = dupcol_action::presolve(prob, paction_);
                    if (prob->status_)
                         break;
		    printProgress('P',iLoop+1);
               }
#if	PRESOLVE_DEBUG
               check_sol(prob, 1.0e0);
#endif

               if (duprow) {
		 possibleBreak;
                    paction_ = duprow_action::presolve(prob, paction_);
                    if (prob->status_)
                         break;
		    printProgress('Q',iLoop+1);
               }
	       // Marginally slower on netlib if this call is enabled.
	       // paction_ = testRedundant(prob,paction_) ;
#if	PRESOLVE_DEBUG
               check_sol(prob, 1.0e0);
#endif
               bool stopLoop = false;
               {
                    int * hinrow = prob->hinrow_;
                    int numberDropped = 0;
                    for (i = 0; i < nrows_; i++)
                         if (!hinrow[i])
                              numberDropped++;

                    prob->messageHandler()->message(COIN_PRESOLVE_PASS,
                                                    messages)
                              << numberDropped << iLoop + 1
                              << CoinMessageEol;
                    //printf("%d rows dropped after pass %d\n",numberDropped,
                    //     iLoop+1);
                    if (numberDropped == lastDropped)
                         stopLoop = true;
                    else
                         lastDropped = numberDropped;
               }
               // Do this here as not very loopy
               if (slackSingleton) {
                    // On most passes do not touch costed slacks
                    if (paction_ != paction0 && !stopLoop) {
		      possibleBreak;
                         paction_ = slack_singleton_action::presolve(prob, paction_, NULL);
                    } else {
                         // do costed if Clp (at end as ruins rest of presolve)
		      possibleBreak;
                         paction_ = slack_singleton_action::presolve(prob, paction_, rowObjective_);
                         stopLoop = true;
                    }
		    printProgress('R',iLoop+1);
               }
#if	PRESOLVE_DEBUG
               check_sol(prob, 1.0e0);
#endif
               if (paction_ == paction0 || stopLoop)
                    break;
#ifdef ABC_INHERIT
	       // see whether to stop anyway
	       int numberRowsNow=nrows_-prob->countEmptyRows();
	       int numberColumnsNow=ncols_-prob->countEmptyCols();
#if ABC_NORMAL_DEBUG
	       printf("Original rows,columns %d,%d - last %d,%d end of pass %d has %d,%d\n", 
		      nrows_,ncols_,numberRowsLeft,numberColumnsLeft,iLoop+1,numberRowsNow,
		      numberColumnsNow);
#endif
	       int rowsDeleted=numberRowsLeft-numberRowsNow;
	       int columnsDeleted=numberColumnsLeft-numberColumnsNow;
 	       if (iLoop>15) {
		 if (rowsDeleted*100<numberRowsStart&&
		     columnsDeleted*100<numberColumnsStart)
		   break;
		 lastPassWasGood=true;
	       } else if (rowsDeleted*100<numberRowsStart&&rowsDeleted<500&&
			  columnsDeleted*100<numberColumnsStart&&columnsDeleted<500) {
		 if (!lastPassWasGood)
		   break;
		 else
		   lastPassWasGood=false;
	       } else {
		 lastPassWasGood=true;
	       }
	       numberRowsLeft=numberRowsNow;
	       numberColumnsLeft=numberColumnsNow;
#endif
          }
     }
     prob->presolveOptions_ &= ~0x10000;
     if (!prob->status_) {
          paction_ = drop_zero_coefficients(prob, paction_);
#if	PRESOLVE_DEBUG
          check_sol(prob, 1.0e0);
#endif

          paction_ = drop_empty_cols_action::presolve(prob, paction_);
          paction_ = drop_empty_rows_action::presolve(prob, paction_);
#if	PRESOLVE_DEBUG
          check_sol(prob, 1.0e0);
#endif
     }

     if (prob->status_) {
          if (prob->status_ == 1)
               prob->messageHandler()->message(COIN_PRESOLVE_INFEAS,
                                               messages)
                         << prob->feasibilityTolerance_
                         << CoinMessageEol;
          else if (prob->status_ == 2)
               prob->messageHandler()->message(COIN_PRESOLVE_UNBOUND,
                                               messages)
                         << CoinMessageEol;
          else
               prob->messageHandler()->message(COIN_PRESOLVE_INFEASUNBOUND,
                                               messages)
                         << CoinMessageEol;
          // get rid of data
          destroyPresolve();
     }
     return (paction_);
}

void check_djs(CoinPostsolveMatrix *prob);


// We could have implemented this by having each postsolve routine
// directly call the next one, but this may make it easier to add debugging checks.
void ClpPresolve::postsolve(CoinPostsolveMatrix &prob)
{
     {
          // Check activities
          double *colels	= prob.colels_;
          int *hrow		= prob.hrow_;
          CoinBigIndex *mcstrt		= prob.mcstrt_;
          int *hincol		= prob.hincol_;
          int *link		= prob.link_;
          int ncols		= prob.ncols_;

          char *cdone	= prob.cdone_;

          double * csol = prob.sol_;
          int nrows = prob.nrows_;

          int colx;

          double * rsol = prob.acts_;
          memset(rsol, 0, nrows * sizeof(double));

          for (colx = 0; colx < ncols; ++colx) {
               if (cdone[colx]) {
                    CoinBigIndex k = mcstrt[colx];
                    int nx = hincol[colx];
                    double solutionValue = csol[colx];
                    for (int i = 0; i < nx; ++i) {
                         int row = hrow[k];
                         double coeff = colels[k];
                         k = link[k];
                         rsol[row] += solutionValue * coeff;
                    }
               }
          }
     }
     if (prob.maxmin_<0) {
       //for (int i=0;i<presolvedModel_->numberRows();i++)
       //prob.rowduals_[i]=-prob.rowduals_[i];
       for (int i=0;i<ncols_;i++) {
	 prob.cost_[i]=-prob.cost_[i];
	 //prob.rcosts_[i]=-prob.rcosts_[i];
       }
       prob.maxmin_=1.0;
     }
     const CoinPresolveAction *paction = paction_;
     //#define PRESOLVE_DEBUG 1
#if	PRESOLVE_DEBUG
     // Check only works after first one
     int checkit = -1;
#endif

     while (paction) {
#if PRESOLVE_DEBUG
          printf("POSTSOLVING %s\n", paction->name());
#endif

          paction->postsolve(&prob);

#if	PRESOLVE_DEBUG
#         if 0
          /*
	    This check fails (on exmip1 (!) in osiUnitTest) because clp
	    enters postsolve with a solution that seems to have incorrect
	    status for a logical. You can see similar behaviour with
	    column status --- incorrect entering postsolve.
	    -- lh, 111207 --
	  */
          {
               int nr = 0;
               int i;
               for (i = 0; i < prob.nrows_; i++) {
                    if ((prob.rowstat_[i] & 7) == 1) {
                         nr++;
                    } else if ((prob.rowstat_[i] & 7) == 2) {
                         // at ub
                         assert (prob.acts_[i] > prob.rup_[i] - 1.0e-6);
                    } else if ((prob.rowstat_[i] & 7) == 3) {
                         // at lb
                         assert (prob.acts_[i] < prob.rlo_[i] + 1.0e-6);
                    }
               }
               int nc = 0;
               for (i = 0; i < prob.ncols_; i++) {
                    if ((prob.colstat_[i] & 7) == 1)
                         nc++;
               }
               printf("%d rows (%d basic), %d cols (%d basic)\n", prob.nrows_, nr, prob.ncols_, nc);
          }
#         endif   // if 0
          checkit++;
          if (prob.colstat_ && checkit > 0) {
               presolve_check_nbasic(&prob) ;
               presolve_check_sol(&prob, 2, 2, 1) ;
          }
#endif
          paction = paction->next;
#if	PRESOLVE_DEBUG
//  check_djs(&prob);
          if (checkit > 0)
               presolve_check_reduced_costs(&prob) ;
#endif
     }
#if	PRESOLVE_DEBUG
     if (prob.colstat_) {
          presolve_check_nbasic(&prob) ;
          presolve_check_sol(&prob, 2, 2, 1) ;
     }
#endif
#undef PRESOLVE_DEBUG

#if	0 && PRESOLVE_DEBUG
     for (i = 0; i < ncols0; i++) {
          if (!cdone[i]) {
               printf("!cdone[%d]\n", i);
               abort();
          }
     }

     for (i = 0; i < nrows0; i++) {
          if (!rdone[i]) {
               printf("!rdone[%d]\n", i);
               abort();
          }
     }


     for (i = 0; i < ncols0; i++) {
          if (sol[i] < -1e10 || sol[i] > 1e10)
               printf("!!!%d %g\n", i, sol[i]);

     }


#endif

#if	0 && PRESOLVE_DEBUG
     // debug check:  make sure we ended up with same original matrix
     {
          int identical = 1;

          for (int i = 0; i < ncols0; i++) {
               PRESOLVEASSERT(hincol[i] == &prob->mcstrt0[i+1] - &prob->mcstrt0[i]);
               CoinBigIndex kcs0 = &prob->mcstrt0[i];
               CoinBigIndex kcs = mcstrt[i];
               int n = hincol[i];
               for (int k = 0; k < n; k++) {
                    CoinBigIndex k1 = presolve_find_row1(&prob->hrow0[kcs0+k], kcs, kcs + n, hrow);

                    if (k1 == kcs + n) {
                         printf("ROW %d NOT IN COL %d\n", &prob->hrow0[kcs0+k], i);
                         abort();
                    }

                    if (colels[k1] != &prob->dels0[kcs0+k])
                         printf("BAD COLEL[%d %d %d]:  %g\n",
                                k1, i, &prob->hrow0[kcs0+k], colels[k1] - &prob->dels0[kcs0+k]);

                    if (kcs0 + k != k1)
                         identical = 0;
               }
          }
          printf("identical? %d\n", identical);
     }
#endif
}








static inline double getTolerance(const ClpSimplex  *si, ClpDblParam key)
{
     double tol;
     if (! si->getDblParam(key, tol)) {
          CoinPresolveAction::throwCoinError("getDblParam failed",
                                             "CoinPrePostsolveMatrix::CoinPrePostsolveMatrix");
     }
     return (tol);
}


// Assumptions:
// 1. nrows>=m.getNumRows()
// 2. ncols>=m.getNumCols()
//
// In presolve, these values are equal.
// In postsolve, they may be inequal, since the reduced problem
// may be smaller, but we need room for the large problem.
// ncols may be larger than si.getNumCols() in postsolve,
// this at that point si will be the reduced problem,
// but we need to reserve enough space for the original problem.
CoinPrePostsolveMatrix::CoinPrePostsolveMatrix(const ClpSimplex * si,
          int ncols_in,
          int nrows_in,
          CoinBigIndex nelems_in,
          double bulkRatio)
     : ncols_(si->getNumCols()),
       nrows_(si->getNumRows()),
       nelems_(si->getNumElements()),
       ncols0_(ncols_in),
       nrows0_(nrows_in),
       bulkRatio_(bulkRatio),
       mcstrt_(new CoinBigIndex[ncols_in+1]),
       hincol_(new int[ncols_in+1]),
       cost_(new double[ncols_in]),
       clo_(new double[ncols_in]),
       cup_(new double[ncols_in]),
       rlo_(new double[nrows_in]),
       rup_(new double[nrows_in]),
       originalColumn_(new int[ncols_in]),
       originalRow_(new int[nrows_in]),
       ztolzb_(getTolerance(si, ClpPrimalTolerance)),
       ztoldj_(getTolerance(si, ClpDualTolerance)),
       maxmin_(si->getObjSense()),
       sol_(NULL),
       rowduals_(NULL),
       acts_(NULL),
       rcosts_(NULL),
       colstat_(NULL),
       rowstat_(NULL),
       handler_(NULL),
       defaultHandler_(false),
       messages_()

{
     bulk0_ = static_cast<CoinBigIndex> (bulkRatio_ * nelems_in);
     hrow_  = new int   [bulk0_];
     colels_ = new double[bulk0_];
     si->getDblParam(ClpObjOffset, originalOffset_);
     int ncols = si->getNumCols();
     int nrows = si->getNumRows();

     setMessageHandler(si->messageHandler()) ;

     ClpDisjointCopyN(si->getColLower(), ncols, clo_);
     ClpDisjointCopyN(si->getColUpper(), ncols, cup_);
     //ClpDisjointCopyN(si->getObjCoefficients(), ncols, cost_);
     double offset;
     ClpDisjointCopyN(si->objectiveAsObject()->gradient(si, si->getColSolution(), offset, true), ncols, cost_);
     ClpDisjointCopyN(si->getRowLower(), nrows,  rlo_);
     ClpDisjointCopyN(si->getRowUpper(), nrows,  rup_);
     int i;
     for (i = 0; i < ncols_in; i++)
          originalColumn_[i] = i;
     for (i = 0; i < nrows_in; i++)
          originalRow_[i] = i;
     sol_ = NULL;
     rowduals_ = NULL;
     acts_ = NULL;

     rcosts_ = NULL;
     colstat_ = NULL;
     rowstat_ = NULL;
}

// I am not familiar enough with CoinPackedMatrix to be confident
// that I will implement a row-ordered version of toColumnOrderedGapFree
// properly.
static bool isGapFree(const CoinPackedMatrix& matrix)
{
     const CoinBigIndex * start = matrix.getVectorStarts();
     const int * length = matrix.getVectorLengths();
     int i = matrix.getSizeVectorLengths() - 1;
     // Quick check
     if (matrix.getNumElements() == start[i]) {
          return true;
     } else {
          for (i = matrix.getSizeVectorLengths() - 1; i >= 0; --i) {
               if (start[i+1] - start[i] != length[i])
                    break;
          }
          return (! (i >= 0));
     }
}
#if	PRESOLVE_DEBUG
static void matrix_bounds_ok(const double *lo, const double *up, int n)
{
     int i;
     for (i = 0; i < n; i++) {
          PRESOLVEASSERT(lo[i] <= up[i]);
          PRESOLVEASSERT(lo[i] < PRESOLVE_INF);
          PRESOLVEASSERT(-PRESOLVE_INF < up[i]);
     }
}
#endif
CoinPresolveMatrix::CoinPresolveMatrix(int ncols0_in,
                                       double /*maxmin*/,
                                       // end prepost members

                                       ClpSimplex * si,

                                       // rowrep
                                       int nrows_in,
                                       CoinBigIndex nelems_in,
                                       bool doStatus,
                                       double nonLinearValue,
                                       double bulkRatio) :

     CoinPrePostsolveMatrix(si,
                            ncols0_in, nrows_in, nelems_in, bulkRatio),
     clink_(new presolvehlink[ncols0_in+1]),
     rlink_(new presolvehlink[nrows_in+1]),

     dobias_(0.0),


     // temporary init
     integerType_(new unsigned char[ncols0_in]),
     tuning_(false),
     startTime_(0.0),
     feasibilityTolerance_(0.0),
     status_(-1),
     colsToDo_(new int [ncols0_in]),
     numberColsToDo_(0),
     nextColsToDo_(new int[ncols0_in]),
     numberNextColsToDo_(0),
     rowsToDo_(new int [nrows_in]),
     numberRowsToDo_(0),
     nextRowsToDo_(new int[nrows_in]),
     numberNextRowsToDo_(0),
     presolveOptions_(0)
{
     const int bufsize = bulk0_;

     nrows_ = si->getNumRows() ;

     // Set up change bits etc
     rowChanged_ = new unsigned char[nrows_];
     memset(rowChanged_, 0, nrows_);
     colChanged_ = new unsigned char[ncols_];
     memset(colChanged_, 0, ncols_);
     CoinPackedMatrix * m = si->matrix();

     // The coefficient matrix is a big hunk of stuff.
     // Do the copy here to try to avoid running out of memory.

     const CoinBigIndex * start = m->getVectorStarts();
     const int * row = m->getIndices();
     const double * element = m->getElements();
     int icol, nel = 0;
     mcstrt_[0] = 0;
     ClpDisjointCopyN(m->getVectorLengths(), ncols_,  hincol_);
     if (si->getObjSense() < 0.0) {
       for (int i=0;i<ncols_;i++)
	 cost_[i]=-cost_[i];
       maxmin_=1.0;
     }
     for (icol = 0; icol < ncols_; icol++) {
          CoinBigIndex j;
          for (j = start[icol]; j < start[icol] + hincol_[icol]; j++) {
               hrow_[nel] = row[j];
               if (fabs(element[j]) > ZTOLDP)
                    colels_[nel++] = element[j];
          }
          mcstrt_[icol+1] = nel;
          hincol_[icol] = nel - mcstrt_[icol];
     }

     // same thing for row rep
     CoinPackedMatrix * mRow = new CoinPackedMatrix();
     mRow->setExtraGap(0.0);
     mRow->setExtraMajor(0.0);
     mRow->reverseOrderedCopyOf(*m);
     //mRow->removeGaps();
     //mRow->setExtraGap(0.0);

     // Now get rid of matrix
     si->createEmptyMatrix();

     double * el = mRow->getMutableElements();
     int * ind = mRow->getMutableIndices();
     CoinBigIndex * strt = mRow->getMutableVectorStarts();
     int * len = mRow->getMutableVectorLengths();
     // Do carefully to save memory
     rowels_ = new double[bulk0_];
     ClpDisjointCopyN(el,      nelems_, rowels_);
     mRow->nullElementArray();
     delete [] el;
     hcol_ = new int[bulk0_];
     ClpDisjointCopyN(ind,       nelems_, hcol_);
     mRow->nullIndexArray();
     delete [] ind;
     mrstrt_ = new CoinBigIndex[nrows_in+1];
     ClpDisjointCopyN(strt,  nrows_,  mrstrt_);
     mRow->nullStartArray();
     mrstrt_[nrows_] = nelems_;
     delete [] strt;
     hinrow_ = new int[nrows_in+1];
     ClpDisjointCopyN(len, nrows_,  hinrow_);
     if (nelems_ > nel) {
          nelems_ = nel;
          // Clean any small elements
          int irow;
          nel = 0;
          CoinBigIndex start = 0;
          for (irow = 0; irow < nrows_; irow++) {
               CoinBigIndex j;
               for (j = start; j < start + hinrow_[irow]; j++) {
                    hcol_[nel] = hcol_[j];
                    if (fabs(rowels_[j]) > ZTOLDP)
                         rowels_[nel++] = rowels_[j];
               }
               start = mrstrt_[irow+1];
               mrstrt_[irow+1] = nel;
               hinrow_[irow] = nel - mrstrt_[irow];
          }
     }

     delete mRow;
     if (si->integerInformation()) {
          CoinMemcpyN(reinterpret_cast<unsigned char *> (si->integerInformation()), ncols_, integerType_);
     } else {
          ClpFillN<unsigned char>(integerType_, ncols_, static_cast<unsigned char> (0));
     }

#ifndef SLIM_CLP
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(si->objectiveAsObject()));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (si->objectiveAsObject()->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(si->objectiveAsObject()));
#endif
#endif
     // Set up prohibited bits if needed
     if (nonLinearValue) {
          anyProhibited_ = true;
          for (icol = 0; icol < ncols_; icol++) {
               int j;
               bool nonLinearColumn = false;
               if (cost_[icol] == nonLinearValue)
                    nonLinearColumn = true;
               for (j = mcstrt_[icol]; j < mcstrt_[icol+1]; j++) {
                    if (colels_[j] == nonLinearValue) {
                         nonLinearColumn = true;
                         setRowProhibited(hrow_[j]);
                    }
               }
               if (nonLinearColumn)
                    setColProhibited(icol);
          }
#ifndef SLIM_CLP
     } else if (quadraticObj) {
          CoinPackedMatrix * quadratic = quadraticObj->quadraticObjective();
          //const int * columnQuadratic = quadratic->getIndices();
          //const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
          const int * columnQuadraticLength = quadratic->getVectorLengths();
          //double * quadraticElement = quadratic->getMutableElements();
          int numberColumns = quadratic->getNumCols();
          anyProhibited_ = true;
          for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (columnQuadraticLength[iColumn]) {
                    setColProhibited(iColumn);
                    //printf("%d prohib\n",iColumn);
               }
          }
#endif
     } else {
          anyProhibited_ = false;
     }

     if (doStatus) {
          // allow for status and solution
          sol_ = new double[ncols_];
          CoinMemcpyN(si->primalColumnSolution(), ncols_, sol_);;
          acts_ = new double [nrows_];
          CoinMemcpyN(si->primalRowSolution(), nrows_, acts_);
          if (!si->statusArray())
               si->createStatus();
          colstat_ = new unsigned char [nrows_+ncols_];
          CoinMemcpyN(si->statusArray(),	(nrows_ + ncols_), colstat_);
          rowstat_ = colstat_ + ncols_;
     }

     // the original model's fields are now unneeded - free them

     si->resize(0, 0);

#if	PRESOLVE_DEBUG
     matrix_bounds_ok(rlo_, rup_, nrows_);
     matrix_bounds_ok(clo_, cup_, ncols_);
#endif

#if 0
     for (i = 0; i < nrows; ++i)
          printf("NR: %6d\n", hinrow[i]);
     for (int i = 0; i < ncols; ++i)
          printf("NC: %6d\n", hincol[i]);
#endif

     presolve_make_memlists(/*mcstrt_,*/ hincol_, clink_, ncols_);
     presolve_make_memlists(/*mrstrt_,*/ hinrow_, rlink_, nrows_);

     // this allows last col/row to expand up to bufsize-1 (22);
     // this must come after the calls to presolve_prefix
     mcstrt_[ncols_] = bufsize - 1;
     mrstrt_[nrows_] = bufsize - 1;
     // Allocate useful arrays
     initializeStuff();

#if	PRESOLVE_CONSISTENCY
//consistent(false);
     presolve_consistent(this, false) ;
#endif
}

// avoid compiler warnings
#if PRESOLVE_SUMMARY > 0
void CoinPresolveMatrix::update_model(ClpSimplex * si,
                                      int nrows0, int ncols0,
                                      CoinBigIndex nelems0)
#else
void CoinPresolveMatrix::update_model(ClpSimplex * si,
                                      int /*nrows0*/,
                                      int /*ncols0*/,
                                      CoinBigIndex /*nelems0*/)
#endif
{
     if (si->getObjSense() < 0.0) {
       for (int i=0;i<ncols_;i++)
	 cost_[i]=-cost_[i];
       dobias_=-dobias_;
     }
     si->loadProblem(ncols_, nrows_, mcstrt_, hrow_, colels_, hincol_,
                     clo_, cup_, cost_, rlo_, rup_);
     //delete [] si->integerInformation();
     int numberIntegers = 0;
     for (int i = 0; i < ncols_; i++) {
          if (integerType_[i])
               numberIntegers++;
     }
     if (numberIntegers)
          si->copyInIntegerInformation(reinterpret_cast<const char *> (integerType_));
     else
          si->copyInIntegerInformation(NULL);

#if	PRESOLVE_SUMMARY
     printf("NEW NCOL/NROW/NELS:  %d(-%d) %d(-%d) %d(-%d)\n",
            ncols_, ncols0 - ncols_,
            nrows_, nrows0 - nrows_,
            si->getNumElements(), nelems0 - si->getNumElements());
#endif
     si->setDblParam(ClpObjOffset, originalOffset_ - dobias_);
     if (si->getObjSense() < 0.0) {
       // put back
       for (int i=0;i<ncols_;i++)
	 cost_[i]=-cost_[i];
       dobias_=-dobias_;
       maxmin_=-1.0;
     }

}











////////////////  POSTSOLVE

CoinPostsolveMatrix::CoinPostsolveMatrix(ClpSimplex*  si,
          int ncols0_in,
          int nrows0_in,
          CoinBigIndex nelems0,

          double maxmin,
          // end prepost members

          double *sol_in,
          double *acts_in,

          unsigned char *colstat_in,
          unsigned char *rowstat_in) :
     CoinPrePostsolveMatrix(si,
                            ncols0_in, nrows0_in, nelems0, 2.0),

     free_list_(0),
     // link, free_list, maxlink
     maxlink_(bulk0_),
     link_(new int[/*maxlink*/ bulk0_]),

     cdone_(new char[ncols0_]),
     rdone_(new char[nrows0_in])

{
     bulk0_ = maxlink_ ;
     nrows_ = si->getNumRows() ;
     ncols_ = si->getNumCols() ;

     sol_ = sol_in;
     rowduals_ = NULL;
     acts_ = acts_in;

     rcosts_ = NULL;
     colstat_ = colstat_in;
     rowstat_ = rowstat_in;

     // this is the *reduced* model, which is probably smaller
     int ncols1 = ncols_ ;
     int nrows1 = nrows_ ;

     const CoinPackedMatrix * m = si->matrix();

     const CoinBigIndex nelemsr = m->getNumElements();
     if (m->getNumElements() && !isGapFree(*m)) {
          // Odd - gaps
          CoinPackedMatrix mm(*m);
          mm.removeGaps();
          mm.setExtraGap(0.0);

          ClpDisjointCopyN(mm.getVectorStarts(), ncols1, mcstrt_);
          CoinZeroN(mcstrt_ + ncols1, ncols0_ - ncols1);
          mcstrt_[ncols1] = nelems0;	// ??    (should point to end of bulk store   -- lh --)
          ClpDisjointCopyN(mm.getVectorLengths(), ncols1,  hincol_);
          ClpDisjointCopyN(mm.getIndices(),      nelemsr, hrow_);
          ClpDisjointCopyN(mm.getElements(),     nelemsr, colels_);
     } else {
          // No gaps

          ClpDisjointCopyN(m->getVectorStarts(), ncols1, mcstrt_);
          CoinZeroN(mcstrt_ + ncols1, ncols0_ - ncols1);
          mcstrt_[ncols1] = nelems0;	// ??    (should point to end of bulk store   -- lh --)
          ClpDisjointCopyN(m->getVectorLengths(), ncols1,  hincol_);
          ClpDisjointCopyN(m->getIndices(),      nelemsr, hrow_);
          ClpDisjointCopyN(m->getElements(),     nelemsr, colels_);
     }



#if	0 && PRESOLVE_DEBUG
     presolve_check_costs(model, &colcopy);
#endif

     // This determines the size of the data structure that contains
     // the matrix being postsolved.  Links are taken from the free_list
     // to recreate matrix entries that were presolved away,
     // and links are added to the free_list when entries created during
     // presolve are discarded.  There is never a need to gc this list.
     // Naturally, it should contain
     // exactly nelems0 entries "in use" when postsolving is done,
     // but I don't know whether the matrix could temporarily get
     // larger during postsolving.  Substitution into more than two
     // rows could do that, in principle.  I am being very conservative
     // here by reserving much more than the amount of space I probably need.
     // If this guess is wrong, check_free_list may be called.
     //  int bufsize = 2*nelems0;

     memset(cdone_, -1, ncols0_);
     memset(rdone_, -1, nrows0_);

     rowduals_ = new double[nrows0_];
     ClpDisjointCopyN(si->getRowPrice(), nrows1, rowduals_);

     rcosts_ = new double[ncols0_];
     ClpDisjointCopyN(si->getReducedCost(), ncols1, rcosts_);
     if (maxmin < 0.0) {
          // change so will look as if minimize
          int i;
          for (i = 0; i < nrows1; i++)
               rowduals_[i] = - rowduals_[i];
          for (i = 0; i < ncols1; i++) {
               rcosts_[i] = - rcosts_[i];
          }
     }

     //ClpDisjointCopyN(si->getRowUpper(), nrows1, rup_);
     //ClpDisjointCopyN(si->getRowLower(), nrows1, rlo_);

     ClpDisjointCopyN(si->getColSolution(), ncols1, sol_);
     si->setDblParam(ClpObjOffset, originalOffset_);

     for (int j = 0; j < ncols1; j++) {
#ifdef COIN_SLOW_PRESOLVE
       if (hincol_[j]) {
#endif
          CoinBigIndex kcs = mcstrt_[j];
          CoinBigIndex kce = kcs + hincol_[j];
          for (CoinBigIndex k = kcs; k < kce; ++k) {
               link_[k] = k + 1;
          }
          link_[kce-1] = NO_LINK ;
#ifdef COIN_SLOW_PRESOLVE
       }
#endif
     }
     {
          int ml = maxlink_;
          for (CoinBigIndex k = nelemsr; k < ml; ++k)
               link_[k] = k + 1;
          if (ml)
               link_[ml-1] = NO_LINK;
     }
     free_list_ = nelemsr;
# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
     /*
       These are used to track the action of postsolve transforms during debugging.
     */
     CoinFillN(cdone_, ncols1, PRESENT_IN_REDUCED) ;
     CoinZeroN(cdone_ + ncols1, ncols0_in - ncols1) ;
     CoinFillN(rdone_, nrows1, PRESENT_IN_REDUCED) ;
     CoinZeroN(rdone_ + nrows1, nrows0_in - nrows1) ;
# endif
}
/* This is main part of Presolve */
ClpSimplex *
ClpPresolve::gutsOfPresolvedModel(ClpSimplex * originalModel,
                                  double feasibilityTolerance,
                                  bool keepIntegers,
                                  int numberPasses,
                                  bool dropNames,
                                  bool doRowObjective,
				  const char * prohibitedRows,
				  const char * prohibitedColumns)
{
     ncols_ = originalModel->getNumCols();
     nrows_ = originalModel->getNumRows();
     nelems_ = originalModel->getNumElements();
     numberPasses_ = numberPasses;

     double maxmin = originalModel->getObjSense();
     originalModel_ = originalModel;
     delete [] originalColumn_;
     originalColumn_ = new int[ncols_];
     delete [] originalRow_;
     originalRow_ = new int[nrows_];
     // and fill in case returns early
     int i;
     for (i = 0; i < ncols_; i++)
          originalColumn_[i] = i;
     for (i = 0; i < nrows_; i++)
          originalRow_[i] = i;
     delete [] rowObjective_;
     if (doRowObjective) {
          rowObjective_ = new double [nrows_];
          memset(rowObjective_, 0, nrows_ * sizeof(double));
     } else {
          rowObjective_ = NULL;
     }

     // result is 0 - okay, 1 infeasible, -1 go round again, 2 - original model
     int result = -1;

     // User may have deleted - its their responsibility
     presolvedModel_ = NULL;
     // Messages
     CoinMessages messages = originalModel->coinMessages();
     // Only go round 100 times even if integer preprocessing
     int totalPasses = 100;
     while (result == -1) {

#ifndef CLP_NO_STD
          // make new copy
          if (saveFile_ == "") {
#endif
               delete presolvedModel_;
#ifndef CLP_NO_STD
               // So won't get names
               int lengthNames = originalModel->lengthNames();
               originalModel->setLengthNames(0);
#endif
               presolvedModel_ = new ClpSimplex(*originalModel);
#ifndef CLP_NO_STD
               originalModel->setLengthNames(lengthNames);
	       presolvedModel_->dropNames();
          } else {
               presolvedModel_ = originalModel;
	       if (dropNames)
		 presolvedModel_->dropNames();
          }
#endif

          // drop integer information if wanted
          if (!keepIntegers)
               presolvedModel_->deleteIntegerInformation();
          totalPasses--;

          double ratio = 2.0;
          if (substitution_ > 3)
               ratio = substitution_;
          else if (substitution_ == 2)
               ratio = 1.5;
          CoinPresolveMatrix prob(ncols_,
                                  maxmin,
                                  presolvedModel_,
                                  nrows_, nelems_, true, nonLinearValue_, ratio);
	  if (prohibitedRows) {
	    prob.setAnyProhibited();
	    for (int i=0;i<nrows_;i++) {
	      if (prohibitedRows[i])
		prob.setRowProhibited(i);
	    }
	  }
	  if (prohibitedColumns) {
	    prob.setAnyProhibited();
	    for (int i=0;i<ncols_;i++) {
	      if (prohibitedColumns[i])
		prob.setColProhibited(i);
	    }
	  }
          prob.setMaximumSubstitutionLevel(substitution_);
          if (doRowObjective)
               memset(rowObjective_, 0, nrows_ * sizeof(double));
          // See if we want statistics
          if ((presolveActions_ & 0x80000000) != 0)
               prob.statistics();
          // make sure row solution correct
          {
               double *colels	= prob.colels_;
               int *hrow		= prob.hrow_;
               CoinBigIndex *mcstrt		= prob.mcstrt_;
               int *hincol		= prob.hincol_;
               int ncols		= prob.ncols_;


               double * csol = prob.sol_;
               double * acts = prob.acts_;
               int nrows = prob.nrows_;

               int colx;

               memset(acts, 0, nrows * sizeof(double));

               for (colx = 0; colx < ncols; ++colx) {
                    double solutionValue = csol[colx];
                    for (int i = mcstrt[colx]; i < mcstrt[colx] + hincol[colx]; ++i) {
                         int row = hrow[i];
                         double coeff = colels[i];
                         acts[row] += solutionValue * coeff;
                    }
               }
          }

          // move across feasibility tolerance
          prob.feasibilityTolerance_ = feasibilityTolerance;

          // Do presolve
          paction_ = presolve(&prob);
          // Get rid of useful arrays
          prob.deleteStuff();

          result = 0;

	  bool fixInfeasibility = (prob.presolveOptions_&16384)!=0;
	  bool hasSolution = (prob.presolveOptions_&32768)!=0;
          if (prob.status_ == 0 && paction_ && (!hasSolution || !fixInfeasibility)) {
               // Looks feasible but double check to see if anything slipped through
               int n		= prob.ncols_;
               double * lo = prob.clo_;
               double * up = prob.cup_;
               int i;

               for (i = 0; i < n; i++) {
                    if (up[i] < lo[i]) {
                         if (up[i] < lo[i] - feasibilityTolerance && !fixInfeasibility) {
                              // infeasible
                              prob.status_ = 1;
                         } else {
                              up[i] = lo[i];
                         }
                    }
               }

               n = prob.nrows_;
               lo = prob.rlo_;
               up = prob.rup_;

               for (i = 0; i < n; i++) {
                    if (up[i] < lo[i]) {
                         if (up[i] < lo[i] - feasibilityTolerance && !fixInfeasibility) {
                              // infeasible
                              prob.status_ = 1;
                         } else {
                              up[i] = lo[i];
                         }
                    }
               }
          }
          if (prob.status_ == 0 && paction_) {
               // feasible

               prob.update_model(presolvedModel_, nrows_, ncols_, nelems_);
               // copy status and solution
               CoinMemcpyN(	     prob.sol_, prob.ncols_, presolvedModel_->primalColumnSolution());
               CoinMemcpyN(	     prob.acts_, prob.nrows_, presolvedModel_->primalRowSolution());
               CoinMemcpyN(	     prob.colstat_, prob.ncols_, presolvedModel_->statusArray());
               CoinMemcpyN(	     prob.rowstat_, prob.nrows_, presolvedModel_->statusArray() + prob.ncols_);
	       if (fixInfeasibility && hasSolution) {
		 // Looks feasible but double check to see if anything slipped through
		 int n		= prob.ncols_;
		 double * lo = prob.clo_;
		 double * up = prob.cup_;
		 double * rsol = prob.acts_;
		 //memset(prob.acts_,0,prob.nrows_*sizeof(double));
		 presolvedModel_->matrix()->times(prob.sol_,rsol);
		 int i;
		 
		 for (i = 0; i < n; i++) {
		   double gap=up[i]-lo[i];
		   if (rsol[i]<lo[i]-feasibilityTolerance&&fabs(rsol[i]-lo[i])<1.0e-3) {
		     lo[i]=rsol[i];
		     if (gap<1.0e5)
		       up[i]=lo[i]+gap;
		   } else if (rsol[i]>up[i]+feasibilityTolerance&&fabs(rsol[i]-up[i])<1.0e-3) {
		     up[i]=rsol[i];
		     if (gap<1.0e5)
		       lo[i]=up[i]-gap;
		   }
		   if (up[i] < lo[i]) {
		     up[i] = lo[i];
		   }
		 }
	       }

               int n = prob.nrows_;
               double * lo = prob.rlo_;
               double * up = prob.rup_;

               for (i = 0; i < n; i++) {
                    if (up[i] < lo[i]) {
                         if (up[i] < lo[i] - feasibilityTolerance && !fixInfeasibility) {
                              // infeasible
                              prob.status_ = 1;
                         } else {
                              up[i] = lo[i];
                         }
                    }
               }
               delete [] prob.sol_;
               delete [] prob.acts_;
               delete [] prob.colstat_;
               prob.sol_ = NULL;
               prob.acts_ = NULL;
               prob.colstat_ = NULL;

               int ncolsNow = presolvedModel_->getNumCols();
               CoinMemcpyN(prob.originalColumn_, ncolsNow, originalColumn_);
#ifndef SLIM_CLP
#ifndef NO_RTTI
               ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(originalModel->objectiveAsObject()));
#else
               ClpQuadraticObjective * quadraticObj = NULL;
               if (originalModel->objectiveAsObject()->type() == 2)
                    quadraticObj = (static_cast< ClpQuadraticObjective*>(originalModel->objectiveAsObject()));
#endif
               if (quadraticObj) {
                    // set up for subset
                    char * mark = new char [ncols_];
                    memset(mark, 0, ncols_);
                    CoinPackedMatrix * quadratic = quadraticObj->quadraticObjective();
                    //const int * columnQuadratic = quadratic->getIndices();
                    //const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
                    const int * columnQuadraticLength = quadratic->getVectorLengths();
                    //double * quadraticElement = quadratic->getMutableElements();
                    int numberColumns = quadratic->getNumCols();
                    ClpQuadraticObjective * newObj = new ClpQuadraticObjective(*quadraticObj,
                              ncolsNow,
                              originalColumn_);
                    // and modify linear and check
                    double * linear = newObj->linearObjective();
                    CoinMemcpyN(presolvedModel_->objective(), ncolsNow, linear);
                    int iColumn;
                    for ( iColumn = 0; iColumn < numberColumns; iColumn++) {
                         if (columnQuadraticLength[iColumn])
                              mark[iColumn] = 1;
                    }
                    // and new
                    quadratic = newObj->quadraticObjective();
                    columnQuadraticLength = quadratic->getVectorLengths();
                    int numberColumns2 = quadratic->getNumCols();
                    for ( iColumn = 0; iColumn < numberColumns2; iColumn++) {
                         if (columnQuadraticLength[iColumn])
                              mark[originalColumn_[iColumn]] = 0;
                    }
                    presolvedModel_->setObjective(newObj);
                    delete newObj;
                    // final check
                    for ( iColumn = 0; iColumn < numberColumns; iColumn++)
                         if (mark[iColumn])
                              printf("Quadratic column %d modified - may be okay\n", iColumn);
                    delete [] mark;
               }
#endif
               delete [] prob.originalColumn_;
               prob.originalColumn_ = NULL;
               int nrowsNow = presolvedModel_->getNumRows();
               CoinMemcpyN(prob.originalRow_, nrowsNow, originalRow_);
               delete [] prob.originalRow_;
               prob.originalRow_ = NULL;
#ifndef CLP_NO_STD
               if (!dropNames && originalModel->lengthNames()) {
                    // Redo names
                    int iRow;
                    std::vector<std::string> rowNames;
                    rowNames.reserve(nrowsNow);
                    for (iRow = 0; iRow < nrowsNow; iRow++) {
                         int kRow = originalRow_[iRow];
                         rowNames.push_back(originalModel->rowName(kRow));
                    }

                    int iColumn;
                    std::vector<std::string> columnNames;
                    columnNames.reserve(ncolsNow);
                    for (iColumn = 0; iColumn < ncolsNow; iColumn++) {
                         int kColumn = originalColumn_[iColumn];
                         columnNames.push_back(originalModel->columnName(kColumn));
                    }
                    presolvedModel_->copyNames(rowNames, columnNames);
               } else {
                    presolvedModel_->setLengthNames(0);
               }
#endif
               if (rowObjective_) {
                    int iRow;
#ifndef NDEBUG
                    int k = -1;
#endif
                    int nObj = 0;
                    for (iRow = 0; iRow < nrowsNow; iRow++) {
                         int kRow = originalRow_[iRow];
#ifndef NDEBUG
                         assert (kRow > k);
                         k = kRow;
#endif
                         rowObjective_[iRow] = rowObjective_[kRow];
                         if (rowObjective_[iRow])
                              nObj++;
                    }
                    if (nObj) {
                         printf("%d costed slacks\n", nObj);
                         presolvedModel_->setRowObjective(rowObjective_);
                    }
               }
               /* now clean up integer variables.  This can modify original
               		  Don't do if dupcol added columns together */
               int i;
               const char * information = presolvedModel_->integerInformation();
               if ((prob.presolveOptions_ & 0x80000000) == 0 && information) {
                    int numberChanges = 0;
                    double * lower0 = originalModel_->columnLower();
                    double * upper0 = originalModel_->columnUpper();
                    double * lower = presolvedModel_->columnLower();
                    double * upper = presolvedModel_->columnUpper();
                    for (i = 0; i < ncolsNow; i++) {
                         if (!information[i])
                              continue;
                         int iOriginal = originalColumn_[i];
                         double lowerValue0 = lower0[iOriginal];
                         double upperValue0 = upper0[iOriginal];
                         double lowerValue = ceil(lower[i] - 1.0e-5);
                         double upperValue = floor(upper[i] + 1.0e-5);
                         lower[i] = lowerValue;
                         upper[i] = upperValue;
                         if (lowerValue > upperValue) {
                              numberChanges++;
                              presolvedModel_->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
                                        messages)
                                        << iOriginal
                                        << lowerValue
                                        << upperValue
                                        << CoinMessageEol;
                              result = 1;
                         } else {
                              if (lowerValue > lowerValue0 + 1.0e-8) {
                                   lower0[iOriginal] = lowerValue;
                                   numberChanges++;
                              }
                              if (upperValue < upperValue0 - 1.0e-8) {
                                   upper0[iOriginal] = upperValue;
                                   numberChanges++;
                              }
                         }
                    }
                    if (numberChanges) {
                         presolvedModel_->messageHandler()->message(COIN_PRESOLVE_INTEGERMODS,
                                   messages)
                                   << numberChanges
                                   << CoinMessageEol;
                         if (!result && totalPasses > 0) {
                              result = -1; // round again
                              const CoinPresolveAction *paction = paction_;
                              while (paction) {
                                   const CoinPresolveAction *next = paction->next;
                                   delete paction;
                                   paction = next;
                              }
                              paction_ = NULL;
                         }
                    }
               }
          } else if (prob.status_) {
               // infeasible or unbounded
               result = 1;
	       // Put status in nelems_!
	       nelems_ = - prob.status_;
               originalModel->setProblemStatus(prob.status_);
          } else {
               // no changes - model needs restoring after Lou's changes
#ifndef CLP_NO_STD
               if (saveFile_ == "") {
#endif
                    delete presolvedModel_;
                    presolvedModel_ = new ClpSimplex(*originalModel);
                    // but we need to remove gaps
                    ClpPackedMatrix* clpMatrix =
                         dynamic_cast< ClpPackedMatrix*>(presolvedModel_->clpMatrix());
                    if (clpMatrix) {
                         clpMatrix->getPackedMatrix()->removeGaps();
                    }
#ifndef CLP_NO_STD
               } else {
                    presolvedModel_ = originalModel;
               }
               presolvedModel_->dropNames();
#endif

               // drop integer information if wanted
               if (!keepIntegers)
                    presolvedModel_->deleteIntegerInformation();
               result = 2;
          }
     }
     if (result == 0 || result == 2) {
          int nrowsAfter = presolvedModel_->getNumRows();
          int ncolsAfter = presolvedModel_->getNumCols();
          CoinBigIndex nelsAfter = presolvedModel_->getNumElements();
          presolvedModel_->messageHandler()->message(COIN_PRESOLVE_STATS,
                    messages)
                    << nrowsAfter << -(nrows_ - nrowsAfter)
                    << ncolsAfter << -(ncols_ - ncolsAfter)
                    << nelsAfter << -(nelems_ - nelsAfter)
                    << CoinMessageEol;
     } else {
          destroyPresolve();
          if (presolvedModel_ != originalModel_)
               delete presolvedModel_;
          presolvedModel_ = NULL;
     }
     return presolvedModel_;
}


