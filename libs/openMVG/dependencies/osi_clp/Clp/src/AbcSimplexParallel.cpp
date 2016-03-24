/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
#include "CoinPragma.hpp"

#include <math.h>
#include "CoinHelperFunctions.hpp"
#include "CoinAbcHelperFunctions.hpp"
#include "AbcSimplexDual.hpp"
#include "ClpEventHandler.hpp"
#include "AbcSimplexFactorization.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinFloatEqual.hpp"
#include "AbcDualRowDantzig.hpp"
#include "ClpMessage.hpp"
#include "ClpLinearObjective.hpp"
#include <cfloat>
#include <cassert>
#include <string>
#include <stdio.h>
#include <iostream>
//#undef AVX2
#define _mm256_broadcast_sd(x) static_cast<__m256d> (__builtin_ia32_vbroadcastsd256 (x))
#define _mm256_load_pd(x) *(__m256d *)(x)
#define _mm256_store_pd (s, x)  *((__m256d *)s)=x
//#define ABC_DEBUG 2
/* Reasons to come out:
   -1 iterations etc
   -2 inaccuracy
   -3 slight inaccuracy (and done iterations)
   +0 looks optimal (might be unbounded - but we will investigate)
   +1 looks infeasible
   +3 max iterations
*/
int
AbcSimplexDual::whileIteratingSerial()
{
  /* arrays
     0 - to get tableau row and then for weights update
     1 - tableau column
     2 - for flip
     3 - actual tableau row
  */
#ifdef CLP_DEBUG
  int debugIteration = -1;
#endif
  // if can't trust much and long way from optimal then relax
  //if (largestPrimalError_ > 10.0)
  //abcFactorization_->relaxAccuracyCheck(CoinMin(1.0e2, largestPrimalError_ / 10.0));
  //else
  //abcFactorization_->relaxAccuracyCheck(1.0);
  // status stays at -1 while iterating, >=0 finished, -2 to invert
  // status -3 to go to top without an invert
  int returnCode = -1;
#define DELAYED_UPDATE
  arrayForBtran_=0;
  setUsedArray(arrayForBtran_);
  arrayForFtran_=1;
  setUsedArray(arrayForFtran_);
  arrayForFlipBounds_=2;
  setUsedArray(arrayForFlipBounds_);
  arrayForTableauRow_=3;
  setUsedArray(arrayForTableauRow_);
  arrayForDualColumn_=4;
  setUsedArray(arrayForDualColumn_);
  arrayForReplaceColumn_=4; //4;
  arrayForFlipRhs_=5;
  setUsedArray(arrayForFlipRhs_);
  dualPivotRow();
  lastPivotRow_=pivotRow_;
  if (pivotRow_ >= 0) {
    // we found a pivot row
    createDualPricingVectorSerial();
    do {
#if ABC_DEBUG
      checkArrays();
#endif
      /*
	-1 - just move dual in values pass
	0 - take iteration
	1 - don't take but continue
	2 - don't take and break
      */
      stateOfIteration_=0;
      returnCode=-1;
      // put row of tableau in usefulArray[arrayForTableauRow_]
      /*
	Could
	a) copy [arrayForBtran_] and start off updateWeights earlier
	b) break transposeTimes into two and do after slack part
	c) also think if cleaner to go all way with update but just don't do final part
      */
      //upperTheta=COIN_DBL_MAX;
      double saveAcceptable=acceptablePivot_;
      if (largestPrimalError_>1.0e-5||largestDualError_>1.0e-5) {
	if (!abcFactorization_->pivots())
	  acceptablePivot_*=1.0e2;
	else if (abcFactorization_->pivots()<5)
	  acceptablePivot_*=1.0e1;
      }
      dualColumn1();
      acceptablePivot_=saveAcceptable;
      if ((stateOfProblem_&VALUES_PASS)!=0) {
	// see if can just move dual
	if (fabs(upperTheta_-fabs(abcDj_[sequenceOut_]))<dualTolerance_) {
	  stateOfIteration_=-1;
	}
      }
      //usefulArray_[arrayForTableauRow_].sortPacked();
      //usefulArray_[arrayForTableauRow_].print();
      //usefulArray_[arrayForDualColumn_].setPackedMode(true);
      //usefulArray_[arrayForDualColumn].sortPacked();
      //usefulArray_[arrayForDualColumn].print();
      if (!stateOfIteration_) {
	// get sequenceIn_
	dualPivotColumn();
	if (sequenceIn_ >= 0) {
	  // normal iteration
	  // update the incoming column (and do weights)
	  getTableauColumnFlipAndStartReplaceSerial();
	} else if (stateOfIteration_!=-1) {
	  stateOfIteration_=2;
	}
      }
      if (!stateOfIteration_) {
	//	assert (stateOfIteration_!=1); 
	int whatNext=0;
	whatNext = housekeeping();
	if (whatNext == 1) {
	  problemStatus_ = -2; // refactorize
	} else if (whatNext == 2) {
	  // maximum iterations or equivalent
	  problemStatus_ = 3;
	  returnCode = 3;
	  stateOfIteration_=2;
	}
	if (problemStatus_==-1) { 
	  replaceColumnPart3();
	  //clearArrays(arrayForReplaceColumn_);
#if ABC_DEBUG
	  checkArrays();
#endif
	    updateDualsInDual();
	    abcDualRowPivot_->updatePrimalSolutionAndWeights(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_],
							     movement_);
#if ABC_DEBUG
	  checkArrays();
#endif
	} else {
	  abcDualRowPivot_->updateWeights2(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_]);
	  //clearArrays(arrayForBtran_);
	  //clearArrays(arrayForFtran_);
	}
      } else {
	if (stateOfIteration_<0) {
	  // move dual in dual values pass
	  theta_=abcDj_[sequenceOut_];
	  updateDualsInDual();
	  abcDj_[sequenceOut_]=0.0;
	  sequenceOut_=-1;
	}
	// clear all arrays
	clearArrays(-1);
	//sequenceIn_=-1;
	//sequenceOut_=-1;
      }
      // Check event
      {
	int status = eventHandler_->event(ClpEventHandler::endOfIteration);
	if (status >= 0) {
	  problemStatus_ = 5;
	  secondaryStatus_ = ClpEventHandler::endOfIteration;
	  returnCode = 4;
	  stateOfIteration_=2;
	}
      }
      // at this stage sequenceIn_ may be <0
      if (sequenceIn_<0&&sequenceOut_>=0) {
	// no incoming column is valid
	returnCode=noPivotColumn();
      }
      if (stateOfIteration_==2) {
	sequenceIn_=-1;
	break;
      }
      swapPrimalStuff();
      if (problemStatus_!=-1) {
	break;
      }
      // dualRow will go to virtual row pivot choice algorithm
      // make sure values pass off if it should be
      // use Btran array and clear inside dualPivotRow (if used)
      int lastSequenceOut=sequenceOut_;
      int lastDirectionOut=directionOut_;
      dualPivotRow();
      lastPivotRow_=pivotRow_;
      if (pivotRow_ >= 0) {
	// we found a pivot row
	// create as packed
	createDualPricingVectorSerial();
	swapDualStuff(lastSequenceOut,lastDirectionOut);
	// next pivot
      } else {
	// no pivot row
	clearArrays(-1);
	returnCode=noPivotRow();
	break;
      }
    } while (problemStatus_ == -1);
    usefulArray_[arrayForTableauRow_].compact();
#if ABC_DEBUG
    checkArrays();
#endif
  } else {
    // no pivot row on first try
    clearArrays(-1);
    returnCode=noPivotRow();
  }
  //printStuff();
  clearArrays(-1);
  //if (pivotRow_==-100)
  //returnCode=-100; // end of values pass
  return returnCode;
}
// Create dual pricing vector
void 
AbcSimplexDual::createDualPricingVectorSerial()
{
#ifndef NDEBUG
#if ABC_NORMAL_DEBUG>3
    checkArrays();
#endif
#endif
  // we found a pivot row
  if (handler_->detail(CLP_SIMPLEX_PIVOTROW, messages_) < 100) {
    handler_->message(CLP_SIMPLEX_PIVOTROW, messages_)
      << pivotRow_
      << CoinMessageEol;
  }
  // check accuracy of weights
  abcDualRowPivot_->checkAccuracy();
  // get sign for finding row of tableau
  // create as packed
  usefulArray_[arrayForBtran_].createOneUnpackedElement(pivotRow_, -directionOut_);
  abcFactorization_->updateColumnTranspose(usefulArray_[arrayForBtran_]);
  sequenceIn_ = -1;
}
void
AbcSimplexDual::getTableauColumnPart1Serial()
{
#if ABC_DEBUG
  {
    const double * work=usefulArray_[arrayForTableauRow_].denseVector();
    int number=usefulArray_[arrayForTableauRow_].getNumElements();
    const int * which=usefulArray_[arrayForTableauRow_].getIndices();
    for (int i=0;i<number;i++) {
      if (which[i]==sequenceIn_) {
	assert (alpha_==work[i]);
	break;
      }
    }
  }
#endif
  //int returnCode=0;
  // update the incoming column
  unpack(usefulArray_[arrayForFtran_]);
  // Array[0] may be needed until updateWeights2
  // and update dual weights (can do in parallel - with extra array)
  alpha_ = abcDualRowPivot_->updateWeights1(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_]);
}
int 
AbcSimplexDual::getTableauColumnFlipAndStartReplaceSerial()
{
  // move checking stuff down into called functions
  int numberFlipped;
  numberFlipped=flipBounds();
  // update the incoming column
  getTableauColumnPart1Serial();
  checkReplacePart1();
  //checkReplacePart1a();
  //checkReplacePart1b();
  getTableauColumnPart2();
  //usefulArray_[arrayForTableauRow_].compact();
  // returns 3 if skip this iteration and re-factorize
/*
  return code
  0 - OK
  2 - flag something and skip
  3 - break and re-factorize
  4 - break and say infeasible
 */
  int returnCode=0;
  // amount primal will move
  movement_ = -dualOut_ * directionOut_ / alpha_;
  // see if update stable
#if ABC_NORMAL_DEBUG>3
  if ((handler_->logLevel() & 32)) {
    double ft=ftAlpha_*abcFactorization_->pivotRegion()[pivotRow_];
    double diff1=fabs(alpha_-btranAlpha_);
    double diff2=fabs(fabs(alpha_)-fabs(ft));
    double diff3=fabs(fabs(ft)-fabs(btranAlpha_));
    double largest=CoinMax(CoinMax(diff1,diff2),diff3);
    printf("btran alpha %g, ftran alpha %g ftAlpha %g largest diff %g\n", 
	   btranAlpha_, alpha_,ft,largest);
    if (largest>0.001*fabs(alpha_)) {
      printf("bad\n");
    }
  }
#endif
  int numberPivots=abcFactorization_->pivots();
  //double checkValue = 1.0e-7; // numberPivots<5 ? 1.0e-7 : 1.0e-6;
  double checkValue = numberPivots ? 1.0e-7 : 1.0e-5;
  // if can't trust much and long way from optimal then relax
  if (largestPrimalError_ > 10.0)
    checkValue = CoinMin(1.0e-4, 1.0e-8 * largestPrimalError_);
  if (fabs(btranAlpha_) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
      fabs(btranAlpha_ - alpha_) > checkValue*(1.0 + fabs(alpha_))) {
    handler_->message(CLP_DUAL_CHECK, messages_)
      << btranAlpha_
      << alpha_
      << CoinMessageEol;
    // see with more relaxed criterion
    double test;
    if (fabs(btranAlpha_) < 1.0e-8 || fabs(alpha_) < 1.0e-8)
      test = 1.0e-1 * fabs(alpha_);
    else
      test = 10.0*checkValue;//1.0e-4 * (1.0 + fabs(alpha_));
    bool needFlag = (fabs(btranAlpha_) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
		     fabs(btranAlpha_ - alpha_) > test);
    double derror = CoinMin(fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_)),1.0)*0.9999e7;
    int error=0;
    while (derror>1.0) {
      error++;
      derror *= 0.1;
    }
    int frequency[8]={99999999,100,10,2,1,1,1,1};
    int newFactorFrequency=CoinMin(forceFactorization_,frequency[error]);
#if ABC_NORMAL_DEBUG>0
    if (newFactorFrequency<forceFactorization_)
      printf("Error of %g after %d pivots old forceFact %d now %d\n",
	     fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_)),numberPivots,
	     forceFactorization_,newFactorFrequency);
#endif
    if (!numberPivots&&fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_))>0.5e-6
	&&abcFactorization_->pivotTolerance()<0.5)
      abcFactorization_->saferTolerances (1.0, -1.03);
    forceFactorization_=newFactorFrequency;

    
#if ABC_NORMAL_DEBUG>0
    if (fabs(btranAlpha_ + alpha_) < 1.0e-5*(1.0 + fabs(alpha_))) {
      printf("diff (%g,%g) %g check %g\n",btranAlpha_,alpha_,fabs(btranAlpha_-alpha_),checkValue*(1.0+fabs(alpha_)));
    }
#endif
    if (numberPivots) {
      if (needFlag&&numberPivots<10) {
	// need to reject something
	assert (sequenceOut_>=0);
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>0
	printf("flag %d as alpha  %g %g - after %d pivots\n", sequenceOut_,
	       btranAlpha_, alpha_,numberPivots);
#endif
	// Make safer?
	abcFactorization_->saferTolerances (1.0, -1.03);
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	lastBadIteration_ = numberIterations_; // say be more cautious
      }
      // Make safer?
      //if (numberPivots<5)
      //abcFactorization_->saferTolerances (-0.99, -1.01);
      problemStatus_ = -2; // factorize now
      returnCode = -2;
      stateOfIteration_=2;
    } else {
      if (needFlag) {
	assert (sequenceOut_>=0);
	// need to reject something
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>3
	printf("flag a %g %g\n", btranAlpha_, alpha_);
#endif
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	//abcProgress_.clearBadTimes();
	lastBadIteration_ = numberIterations_; // say be more cautious
	if (fabs(alpha_) < 1.0e-10 && fabs(btranAlpha_) < 1.0e-8 && numberIterations_ > 100) {
	  //printf("I think should declare infeasible\n");
	  problemStatus_ = 1;
	  returnCode = 1;
	  stateOfIteration_=2;
	} else {
	  stateOfIteration_=1;
	}
	// Make safer?
	if (abcFactorization_->pivotTolerance()<0.999&&stateOfIteration_==1) {
	  // change tolerance and re-invert
	  abcFactorization_->saferTolerances (1.0, -1.03);
	  problemStatus_ = -6; // factorize now
	  returnCode = -2;
	  stateOfIteration_=2;
	}
      }
    }
  }
  if (!stateOfIteration_) {
    // check update
    //int updateStatus = 
/*
  returns 
  0 - OK
  1 - take iteration then re-factorize
  2 - flag something and skip
  3 - break and re-factorize
  5 - take iteration then re-factorize because of memory
 */
    int status=checkReplace();
    if (status&&!returnCode)
      returnCode=status;
  }
  //clearArrays(arrayForFlipRhs_);
  if (stateOfIteration_&&numberFlipped) {
    //usefulArray_[arrayForTableauRow_].compact();
    // move variables back
    flipBack(numberFlipped);
  }
  // could choose average of all three
  return returnCode;
}
#if ABC_PARALLEL==1
/* Reasons to come out:
   -1 iterations etc
   -2 inaccuracy
   -3 slight inaccuracy (and done iterations)
   +0 looks optimal (might be unbounded - but we will investigate)
   +1 looks infeasible
   +3 max iterations
*/
int
AbcSimplexDual::whileIteratingThread()
{
  /* arrays
     0 - to get tableau row and then for weights update
     1 - tableau column
     2 - for flip
     3 - actual tableau row
  */
#ifdef CLP_DEBUG
  int debugIteration = -1;
#endif
  // if can't trust much and long way from optimal then relax
  //if (largestPrimalError_ > 10.0)
  //abcFactorization_->relaxAccuracyCheck(CoinMin(1.0e2, largestPrimalError_ / 10.0));
  //else
  //abcFactorization_->relaxAccuracyCheck(1.0);
  // status stays at -1 while iterating, >=0 finished, -2 to invert
  // status -3 to go to top without an invert
  int returnCode = -1;
#define DELAYED_UPDATE
  arrayForBtran_=0;
  setUsedArray(arrayForBtran_);
  arrayForFtran_=1;
  setUsedArray(arrayForFtran_);
  arrayForFlipBounds_=2;
  setUsedArray(arrayForFlipBounds_);
  arrayForTableauRow_=3;
  setUsedArray(arrayForTableauRow_);
  arrayForDualColumn_=4;
  setUsedArray(arrayForDualColumn_);
  arrayForReplaceColumn_=4; //4;
  arrayForFlipRhs_=5;
  setUsedArray(arrayForFlipRhs_);
  dualPivotRow();
  lastPivotRow_=pivotRow_;
  if (pivotRow_ >= 0) {
    // we found a pivot row
    createDualPricingVectorThread();
    do {
#if ABC_DEBUG
      checkArrays();
#endif
      /*
	-1 - just move dual in values pass
	0 - take iteration
	1 - don't take but continue
	2 - don't take and break
      */
      stateOfIteration_=0;
      returnCode=-1;
      // put row of tableau in usefulArray[arrayForTableauRow_]
      /*
	Could
	a) copy [arrayForBtran_] and start off updateWeights earlier
	b) break transposeTimes into two and do after slack part
	c) also think if cleaner to go all way with update but just don't do final part
      */
      //upperTheta=COIN_DBL_MAX;
      double saveAcceptable=acceptablePivot_;
      if (largestPrimalError_>1.0e-5||largestDualError_>1.0e-5) {
	if (!abcFactorization_->pivots())
	  acceptablePivot_*=1.0e2;
	else if (abcFactorization_->pivots()<5)
	  acceptablePivot_*=1.0e1;
      }
      dualColumn1();
      acceptablePivot_=saveAcceptable;
      if ((stateOfProblem_&VALUES_PASS)!=0) {
	// see if can just move dual
	if (fabs(upperTheta_-fabs(abcDj_[sequenceOut_]))<dualTolerance_) {
	  stateOfIteration_=-1;
	}
      }
      //usefulArray_[arrayForTableauRow_].sortPacked();
      //usefulArray_[arrayForTableauRow_].print();
      //usefulArray_[arrayForDualColumn_].setPackedMode(true);
      //usefulArray_[arrayForDualColumn].sortPacked();
      //usefulArray_[arrayForDualColumn].print();
      if (parallelMode_!=0) {
	stopStart_=1+32*1; // Just first thread for updateweights
	startParallelStuff(2);
      }
      if (!stateOfIteration_) {
	// get sequenceIn_
	dualPivotColumn();
	if (sequenceIn_ >= 0) {
	  // normal iteration
	  // update the incoming column (and do weights if serial)
	  getTableauColumnFlipAndStartReplaceThread();
	  //usleep(1000);
	} else if (stateOfIteration_!=-1) {
	  stateOfIteration_=2;
	}
      }
      if (parallelMode_!=0) {
	// do sync here
	stopParallelStuff(2);
      }
      if (!stateOfIteration_) {
	//	assert (stateOfIteration_!=1); 
	int whatNext=0;
	whatNext = housekeeping();
	if (whatNext == 1) {
	  problemStatus_ = -2; // refactorize
	} else if (whatNext == 2) {
	  // maximum iterations or equivalent
	  problemStatus_ = 3;
	  returnCode = 3;
	  stateOfIteration_=2;
	}
      } else {
	if (stateOfIteration_<0) {
	  // move dual in dual values pass
	  theta_=abcDj_[sequenceOut_];
	  updateDualsInDual();
	  abcDj_[sequenceOut_]=0.0;
	  sequenceOut_=-1;
	}
	// clear all arrays
	clearArrays(-1);
      }
      // at this stage sequenceIn_ may be <0
      if (sequenceIn_<0&&sequenceOut_>=0) {
	// no incoming column is valid
	returnCode=noPivotColumn();
      }
      if (stateOfIteration_==2) {
	sequenceIn_=-1;
	break;
      }
      if (problemStatus_!=-1) {
	abcDualRowPivot_->updateWeights2(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_]);
	swapPrimalStuff();
	break;
      }
#if ABC_DEBUG
      checkArrays();
#endif
      if (stateOfIteration_!=-1) {
	assert (stateOfIteration_==0); // if 1 why are we here
	// can do these in parallel
	if (parallelMode_==0) {
	  replaceColumnPart3();
	  updateDualsInDual();
	  abcDualRowPivot_->updatePrimalSolutionAndWeights(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_],
							   movement_);
	} else {
	  stopStart_=1+32*1;
	  startParallelStuff(3);
	  if (parallelMode_>1) {
	    stopStart_=2+32*2;
	    startParallelStuff(9);
	  } else {
	    replaceColumnPart3();
	  }
	  abcDualRowPivot_->updatePrimalSolutionAndWeights(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_],
							   movement_);
	}
      }
#if ABC_DEBUG
      checkArrays();
#endif
      swapPrimalStuff();
      // dualRow will go to virtual row pivot choice algorithm
      // make sure values pass off if it should be
      // use Btran array and clear inside dualPivotRow (if used)
      int lastSequenceOut=sequenceOut_;
      int lastDirectionOut=directionOut_;
      // redo to test on parallelMode_
      if (parallelMode_>1) {
	stopStart_=2+32*2; // stop factorization update
	//stopStart_=3+32*3; // stop all
	stopParallelStuff(9);
      }
      dualPivotRow();
      lastPivotRow_=pivotRow_;
      if (pivotRow_ >= 0) {
	// we found a pivot row
	// create as packed
	createDualPricingVectorThread();
	swapDualStuff(lastSequenceOut,lastDirectionOut);
	// next pivot
	// redo to test on parallelMode_
	if (parallelMode_!=0) {
	  stopStart_=1+32*1; // stop dual update
	  stopParallelStuff(3);
	}
      } else {
	// no pivot row
	// redo to test on parallelMode_
	if (parallelMode_!=0) {
	  stopStart_=1+32*1; // stop dual update
	  stopParallelStuff(3);
	}
	clearArrays(-1);
	returnCode=noPivotRow();
	break;
      }
    } while (problemStatus_ == -1);
    usefulArray_[arrayForTableauRow_].compact();
#if ABC_DEBUG
    checkArrays();
#endif
  } else {
    // no pivot row on first try
    clearArrays(-1);
    returnCode=noPivotRow();
  }
  //printStuff();
  clearArrays(-1);
  //if (pivotRow_==-100)
  //returnCode=-100; // end of values pass
  return returnCode;
}
// Create dual pricing vector
void 
AbcSimplexDual::createDualPricingVectorThread()
{
#ifndef NDEBUG
#if ABC_NORMAL_DEBUG>3
    checkArrays();
#endif
#endif
  // we found a pivot row
  if (handler_->detail(CLP_SIMPLEX_PIVOTROW, messages_) < 100) {
    handler_->message(CLP_SIMPLEX_PIVOTROW, messages_)
      << pivotRow_
      << CoinMessageEol;
  }
  // check accuracy of weights
  abcDualRowPivot_->checkAccuracy();
  // get sign for finding row of tableau
  // create as packed
  usefulArray_[arrayForBtran_].createOneUnpackedElement(pivotRow_, -directionOut_);
  abcFactorization_->updateColumnTranspose(usefulArray_[arrayForBtran_]);
  sequenceIn_ = -1;
}
void
AbcSimplexDual::getTableauColumnPart1Thread()
{
#if ABC_DEBUG
  {
    const double * work=usefulArray_[arrayForTableauRow_].denseVector();
    int number=usefulArray_[arrayForTableauRow_].getNumElements();
    const int * which=usefulArray_[arrayForTableauRow_].getIndices();
    for (int i=0;i<number;i++) {
      if (which[i]==sequenceIn_) {
	assert (alpha_==work[i]);
	break;
      }
    }
  }
#endif
  //int returnCode=0;
  // update the incoming column
  unpack(usefulArray_[arrayForFtran_]);
  // Array[0] may be needed until updateWeights2
  // and update dual weights (can do in parallel - with extra array)
  if (parallelMode_!=0) {
    abcFactorization_->updateColumnFTPart1(usefulArray_[arrayForFtran_]);
    // pivot element
    //alpha_ = usefulArray_[arrayForFtran_].denseVector()[pivotRow_];
    } else {
    alpha_ = abcDualRowPivot_->updateWeights1(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_]);
  }
}
int 
AbcSimplexDual::getTableauColumnFlipAndStartReplaceThread()
{
  // move checking stuff down into called functions
  // threads 2 and 3 are available
  int numberFlipped;

  if (parallelMode_==0) {
    numberFlipped=flipBounds();
    // update the incoming column
    getTableauColumnPart1Thread();
    checkReplacePart1();
    //checkReplacePart1a();
    //checkReplacePart1b();
    getTableauColumnPart2();
  } else {
    // save stopStart
    int saveStopStart=stopStart_;
    if (parallelMode_>1) {
      // we can flip immediately
      stopStart_=2+32*2; // just thread 1
      startParallelStuff(8);
      // update the incoming column
      getTableauColumnPart1Thread();
      stopStart_=4+32*4; // just thread 2
      startParallelStuff(7);
      getTableauColumnPart2();
      stopStart_=6+32*6; // just threads 1 and 2
      stopParallelStuff(8);
      //ftAlpha_=threadInfo_[2].result;
    } else {
    // just one extra thread - do replace
    // update the incoming column
    getTableauColumnPart1Thread();
    stopStart_=2+32*2; // just thread 1
    startParallelStuff(7);
    getTableauColumnPart2();
    numberFlipped=flipBounds();
    stopParallelStuff(8);
    //ftAlpha_=threadInfo_[1].result;
    }
    stopStart_=saveStopStart;
  }
  //usefulArray_[arrayForTableauRow_].compact();
  // returns 3 if skip this iteration and re-factorize
/*
  return code
  0 - OK
  2 - flag something and skip
  3 - break and re-factorize
  4 - break and say infeasible
 */
  int returnCode=0;
  // amount primal will move
  movement_ = -dualOut_ * directionOut_ / alpha_;
  // see if update stable
#if ABC_NORMAL_DEBUG>3
  if ((handler_->logLevel() & 32)) {
    double ft=ftAlpha_*abcFactorization_->pivotRegion()[pivotRow_];
    double diff1=fabs(alpha_-btranAlpha_);
    double diff2=fabs(fabs(alpha_)-fabs(ft));
    double diff3=fabs(fabs(ft)-fabs(btranAlpha_));
    double largest=CoinMax(CoinMax(diff1,diff2),diff3);
    printf("btran alpha %g, ftran alpha %g ftAlpha %g largest diff %g\n", 
	   btranAlpha_, alpha_,ft,largest);
    if (largest>0.001*fabs(alpha_)) {
      printf("bad\n");
    }
  }
#endif
  int numberPivots=abcFactorization_->pivots();
  //double checkValue = 1.0e-7; // numberPivots<5 ? 1.0e-7 : 1.0e-6;
  double checkValue = numberPivots ? 1.0e-7 : 1.0e-5;
  // if can't trust much and long way from optimal then relax
  if (largestPrimalError_ > 10.0)
    checkValue = CoinMin(1.0e-4, 1.0e-8 * largestPrimalError_);
  if (fabs(btranAlpha_) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
      fabs(btranAlpha_ - alpha_) > checkValue*(1.0 + fabs(alpha_))) {
    handler_->message(CLP_DUAL_CHECK, messages_)
      << btranAlpha_
      << alpha_
      << CoinMessageEol;
    // see with more relaxed criterion
    double test;
    if (fabs(btranAlpha_) < 1.0e-8 || fabs(alpha_) < 1.0e-8)
      test = 1.0e-1 * fabs(alpha_);
    else
      test = 10.0*checkValue;//1.0e-4 * (1.0 + fabs(alpha_));
    bool needFlag = (fabs(btranAlpha_) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
		     fabs(btranAlpha_ - alpha_) > test);
    double derror = CoinMin(fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_)),1.0)*0.9999e7;
    int error=0;
    while (derror>1.0) {
      error++;
      derror *= 0.1;
    }
    int frequency[8]={99999999,100,10,2,1,1,1,1};
    int newFactorFrequency=CoinMin(forceFactorization_,frequency[error]);
#if ABC_NORMAL_DEBUG>0
    if (newFactorFrequency<forceFactorization_)
      printf("Error of %g after %d pivots old forceFact %d now %d\n",
	     fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_)),numberPivots,
	     forceFactorization_,newFactorFrequency);
#endif
    if (!numberPivots&&fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_))>0.5e-6
	&&abcFactorization_->pivotTolerance()<0.5)
      abcFactorization_->saferTolerances (1.0, -1.03);
    forceFactorization_=newFactorFrequency;

    
#if ABC_NORMAL_DEBUG>0
    if (fabs(btranAlpha_ + alpha_) < 1.0e-5*(1.0 + fabs(alpha_))) {
      printf("diff (%g,%g) %g check %g\n",btranAlpha_,alpha_,fabs(btranAlpha_-alpha_),checkValue*(1.0+fabs(alpha_)));
    }
#endif
    if (numberPivots) {
      if (needFlag&&numberPivots<10) {
	// need to reject something
	assert (sequenceOut_>=0);
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>0
	printf("flag %d as alpha  %g %g - after %d pivots\n", sequenceOut_,
	       btranAlpha_, alpha_,numberPivots);
#endif
	// Make safer?
	abcFactorization_->saferTolerances (1.0, -1.03);
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	lastBadIteration_ = numberIterations_; // say be more cautious
      }
      // Make safer?
      //if (numberPivots<5)
      //abcFactorization_->saferTolerances (-0.99, -1.01);
      problemStatus_ = -2; // factorize now
      returnCode = -2;
      stateOfIteration_=2;
    } else {
      if (needFlag) {
	assert (sequenceOut_>=0);
	// need to reject something
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>3
	printf("flag a %g %g\n", btranAlpha_, alpha_);
#endif
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	//abcProgress_.clearBadTimes();
	lastBadIteration_ = numberIterations_; // say be more cautious
	if (fabs(alpha_) < 1.0e-10 && fabs(btranAlpha_) < 1.0e-8 && numberIterations_ > 100) {
	  //printf("I think should declare infeasible\n");
	  problemStatus_ = 1;
	  returnCode = 1;
	  stateOfIteration_=2;
	} else {
	  stateOfIteration_=1;
	}
	// Make safer?
	if (abcFactorization_->pivotTolerance()<0.999&&stateOfIteration_==1) {
	  // change tolerance and re-invert
	  abcFactorization_->saferTolerances (1.0, -1.03);
	  problemStatus_ = -6; // factorize now
	  returnCode = -2;
	  stateOfIteration_=2;
	}
      }
    }
  }
  if (!stateOfIteration_) {
    // check update
    //int updateStatus = 
/*
  returns 
  0 - OK
  1 - take iteration then re-factorize
  2 - flag something and skip
  3 - break and re-factorize
  5 - take iteration then re-factorize because of memory
 */
    int status=checkReplace();
    if (status&&!returnCode)
      returnCode=status;
  }
  //clearArrays(arrayForFlipRhs_);
  if (stateOfIteration_&&numberFlipped) {
    //usefulArray_[arrayForTableauRow_].compact();
    // move variables back
    flipBack(numberFlipped);
  }
  // could choose average of all three
  return returnCode;
}
#endif
#if ABC_PARALLEL==2
#if ABC_NORMAL_DEBUG>0
// for conflicts
int cilk_conflict=0;
#endif
#ifdef EARLY_FACTORIZE
static int doEarlyFactorization(AbcSimplexDual * dual)
{
  int returnCode=cilk_spawn dual->whileIteratingParallel(123456789);
  CoinIndexedVector & vector = *dual->usefulArray(ABC_NUMBER_USEFUL-1);
  int status=dual->earlyFactorization()->factorize(dual,vector);
#if 0
  // Is this safe
  if (!status&&true) {
    // do pivots if there are any
    int capacity = vector.capacity();
    capacity--;
    int numberPivotsStored = vector.getIndices()[capacity];
    status = 
      dual->earlyFactorization()->replaceColumns(dual,vector,
						    0,numberPivotsStored,false);
  }
#endif
  if (status) {
    printf("bad early factorization in doEarly - switch off\n");
    vector.setNumElements(-1);
  }
  cilk_sync;
  return returnCode;
}
#endif
#define MOVE_UPDATE_WEIGHTS
/* Reasons to come out:
   -1 iterations etc
   -2 inaccuracy
   -3 slight inaccuracy (and done iterations)
   +0 looks optimal (might be unbounded - but we will investigate)
   +1 looks infeasible
   +3 max iterations
*/
int
AbcSimplexDual::whileIteratingCilk()
{
  /* arrays
     0 - to get tableau row and then for weights update
     1 - tableau column
     2 - for flip
     3 - actual tableau row
  */
#ifdef CLP_DEBUG
  int debugIteration = -1;
#endif
  // if can't trust much and long way from optimal then relax
  //if (largestPrimalError_ > 10.0)
  //abcFactorization_->relaxAccuracyCheck(CoinMin(1.0e2, largestPrimalError_ / 10.0));
  //else
  //abcFactorization_->relaxAccuracyCheck(1.0);
  // status stays at -1 while iterating, >=0 finished, -2 to invert
  // status -3 to go to top without an invert
  int returnCode = -1;
#define DELAYED_UPDATE
  arrayForBtran_=0;
  setUsedArray(arrayForBtran_);
  arrayForFtran_=1;
  setUsedArray(arrayForFtran_);
  arrayForFlipBounds_=2;
  setUsedArray(arrayForFlipBounds_);
  arrayForTableauRow_=3;
  setUsedArray(arrayForTableauRow_);
  arrayForDualColumn_=4;
  setUsedArray(arrayForDualColumn_);
  arrayForReplaceColumn_=6; //4;
  setUsedArray(arrayForReplaceColumn_);
#ifndef MOVE_UPDATE_WEIGHTS
  arrayForFlipRhs_=5;
  setUsedArray(arrayForFlipRhs_);
#else
  arrayForFlipRhs_=0;
  // use for weights
  setUsedArray(5);
#endif
  dualPivotRow();
  lastPivotRow_=pivotRow_;
  if (pivotRow_ >= 0) {
    // we found a pivot row
    createDualPricingVectorCilk();
    // ABC_PARALLEL 2
#if 0
    {
      for (int i=0;i<numberRows_;i++) {
	int iSequence=abcPivotVariable_[i];
	if (lowerBasic_[i]!=lowerSaved_[iSequence]||
	    upperBasic_[i]!=upperSaved_[iSequence])
	  printf("basic l %g,%g,%g u %g,%g\n",
		 abcLower_[iSequence],lowerSaved_[iSequence],lowerBasic_[i],
		 abcUpper_[iSequence],upperSaved_[iSequence],upperBasic_[i]);
      }
    }
#endif
#if 0
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      int bad=-1;
      if (getFakeBound(iSequence)==noFake) {
	if (abcLower_[iSequence]!=lowerSaved_[iSequence]||
	    abcUpper_[iSequence]!=upperSaved_[iSequence]) 
	  bad=0;
      } else if (getFakeBound(iSequence)==lowerFake) {
	if (abcLower_[iSequence]==lowerSaved_[iSequence]||
	    abcUpper_[iSequence]!=upperSaved_[iSequence]) 
	  bad=1;
      } else if (getFakeBound(iSequence)==upperFake) {
	if (abcLower_[iSequence]!=lowerSaved_[iSequence]||
	    abcUpper_[iSequence]==upperSaved_[iSequence]) 
	  bad=2;
      } else {
	if (abcLower_[iSequence]==lowerSaved_[iSequence]||
	    abcUpper_[iSequence]==upperSaved_[iSequence]) 
	  bad=3;
      }
      if (bad>=0) 
	printf("%d status %d l %g,%g u %g,%g\n",iSequence,internalStatus_[iSequence],
	       abcLower_[iSequence],lowerSaved_[iSequence],
	       abcUpper_[iSequence],upperSaved_[iSequence]);
    }
#endif
    int numberPivots = abcFactorization_->maximumPivots();
#ifdef EARLY_FACTORIZE
    int numberEarly=0;
    if (numberPivots>20&&(numberEarly_&0xffff)>5) {
      numberEarly=numberEarly_&0xffff;
      numberPivots=CoinMax(numberPivots-numberEarly-abcFactorization_->pivots(),numberPivots/2);
    }
    returnCode=whileIteratingParallel(numberPivots);
    if (returnCode==-99) {
      if (numberEarly) {
	if (!abcEarlyFactorization_)
	  abcEarlyFactorization_ = new AbcSimplexFactorization(*abcFactorization_);
	// create pivot list
	CoinIndexedVector & vector = usefulArray_[ABC_NUMBER_USEFUL-1];
	vector.checkClear();
	int * indices = vector.getIndices();
	int capacity = vector.capacity();
	int numberNeeded=numberRows_+2*numberEarly+1;
	if (capacity<numberNeeded) {
	  vector.reserve(numberNeeded);
	  capacity = vector.capacity();
	}
	int numberBasic=0;
	for (int i=0;i<numberRows_;i++) {
	  int iSequence = abcPivotVariable_[i];
	  if (iSequence<numberRows_)
	    indices[numberBasic++]=iSequence;
	}
	vector.setNumElements(numberRows_-numberBasic);
	for (int i=0;i<numberRows_;i++) {
	  int iSequence = abcPivotVariable_[i];
	  if (iSequence>=numberRows_)
	    indices[numberBasic++]=iSequence;
	}
	assert (numberBasic==numberRows_);
	indices[capacity-1]=0;
	// could set iterations to -1 for safety
#if 0
	cilk_spawn doEarlyFactorization(this);
	returnCode=whileIteratingParallel(123456789);
	cilk_sync;
#else
	returnCode=doEarlyFactorization(this);
#endif
      } else {
	returnCode=whileIteratingParallel(123456789);
      }
    }
#else
    returnCode=whileIteratingParallel(numberPivots);
#endif
    usefulArray_[arrayForTableauRow_].compact();
#if ABC_DEBUG
    checkArrays();
#endif
  } else {
    // no pivot row on first try
    clearArrays(-1);
    returnCode=noPivotRow();
  }
  //printStuff();
  clearArrays(-1);
  //if (pivotRow_==-100)
  //returnCode=-100; // end of values pass
  return returnCode;
}
// Create dual pricing vector
void 
AbcSimplexDual::createDualPricingVectorCilk()
{
#if CILK_CONFLICT>0
  if(cilk_conflict&15!=0) {
    printf("cilk_conflict %d!\n",cilk_conflict);
    cilk_conflict=0;
  }
#endif
#ifndef NDEBUG
#if ABC_NORMAL_DEBUG>3
    checkArrays();
#endif
#endif
  // we found a pivot row
  if (handler_->detail(CLP_SIMPLEX_PIVOTROW, messages_) < 100) {
    handler_->message(CLP_SIMPLEX_PIVOTROW, messages_)
      << pivotRow_
      << CoinMessageEol;
  }
  // check accuracy of weights
  abcDualRowPivot_->checkAccuracy();
  // get sign for finding row of tableau
  // create as packed
#if MOVE_REPLACE_PART1A > 0
  if (!abcFactorization_->usingFT()) {
#endif
    usefulArray_[arrayForBtran_].createOneUnpackedElement(pivotRow_, -directionOut_);
    abcFactorization_->updateColumnTranspose(usefulArray_[arrayForBtran_]);
#if MOVE_REPLACE_PART1A > 0
  } else {
    cilk_spawn abcFactorization_->checkReplacePart1a(&usefulArray_[arrayForReplaceColumn_],pivotRow_);
    usefulArray_[arrayForBtran_].createOneUnpackedElement(pivotRow_, -directionOut_);
    abcFactorization_->updateColumnTransposeCpu(usefulArray_[arrayForBtran_],1);
    cilk_sync;
  }
#endif
  sequenceIn_ = -1;
}
void
AbcSimplexDual::getTableauColumnPart1Cilk()
{
#if ABC_DEBUG
  {
    const double * work=usefulArray_[arrayForTableauRow_].denseVector();
    int number=usefulArray_[arrayForTableauRow_].getNumElements();
    const int * which=usefulArray_[arrayForTableauRow_].getIndices();
    for (int i=0;i<number;i++) {
      if (which[i]==sequenceIn_) {
	assert (alpha_==work[i]);
	break;
      }
    }
  }
#endif
  //int returnCode=0;
  // update the incoming column
  unpack(usefulArray_[arrayForFtran_]);
  // Array[0] may be needed until updateWeights2
  // and update dual weights (can do in parallel - with extra array)
  abcFactorization_->updateColumnFTPart1(usefulArray_[arrayForFtran_]);
}
int 
AbcSimplexDual::getTableauColumnFlipAndStartReplaceCilk()
{
  // move checking stuff down into called functions
  // threads 2 and 3 are available
  int numberFlipped;
  //cilk
  getTableauColumnPart1Cilk();
#if MOVE_REPLACE_PART1A <= 0
  cilk_spawn getTableauColumnPart2();
#if MOVE_REPLACE_PART1A == 0
  cilk_spawn checkReplacePart1();
#endif
  numberFlipped=flipBounds();
  cilk_sync;
#else
  if (abcFactorization_->usingFT()) {
    cilk_spawn getTableauColumnPart2();
    ftAlpha_ = cilk_spawn abcFactorization_->checkReplacePart1b(&usefulArray_[arrayForReplaceColumn_],pivotRow_);
    numberFlipped=flipBounds();
    cilk_sync;
  } else {
    cilk_spawn getTableauColumnPart2();
    numberFlipped=flipBounds();
    cilk_sync;
  }
#endif
  //usefulArray_[arrayForTableauRow_].compact();
  // returns 3 if skip this iteration and re-factorize
  /*
    return code
    0 - OK
    2 - flag something and skip
    3 - break and re-factorize
    4 - break and say infeasible
 */
  int returnCode=0;
  // amount primal will move
  movement_ = -dualOut_ * directionOut_ / alpha_;
  // see if update stable
#if ABC_NORMAL_DEBUG>3
  if ((handler_->logLevel() & 32)) {
    double ft=ftAlpha_*abcFactorization_->pivotRegion()[pivotRow_];
    double diff1=fabs(alpha_-btranAlpha_);
    double diff2=fabs(fabs(alpha_)-fabs(ft));
    double diff3=fabs(fabs(ft)-fabs(btranAlpha_));
    double largest=CoinMax(CoinMax(diff1,diff2),diff3);
    printf("btran alpha %g, ftran alpha %g ftAlpha %g largest diff %g\n", 
	   btranAlpha_, alpha_,ft,largest);
    if (largest>0.001*fabs(alpha_)) {
      printf("bad\n");
    }
  }
#endif
  int numberPivots=abcFactorization_->pivots();
  //double checkValue = 1.0e-7; // numberPivots<5 ? 1.0e-7 : 1.0e-6;
  double checkValue = numberPivots ? 1.0e-7 : 1.0e-5;
  // if can't trust much and long way from optimal then relax
  if (largestPrimalError_ > 10.0)
    checkValue = CoinMin(1.0e-4, 1.0e-8 * largestPrimalError_);
  if (fabs(btranAlpha_) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
      fabs(btranAlpha_ - alpha_) > checkValue*(1.0 + fabs(alpha_))) {
    handler_->message(CLP_DUAL_CHECK, messages_)
      << btranAlpha_
      << alpha_
      << CoinMessageEol;
    // see with more relaxed criterion
    double test;
    if (fabs(btranAlpha_) < 1.0e-8 || fabs(alpha_) < 1.0e-8)
      test = 1.0e-1 * fabs(alpha_);
    else
      test = CoinMin(10.0*checkValue,1.0e-4 * (1.0 + fabs(alpha_)));
    bool needFlag = (fabs(btranAlpha_) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
		     fabs(btranAlpha_ - alpha_) > test);
    double derror = CoinMin(fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_)),1.0)*0.9999e7;
    int error=0;
    while (derror>1.0) {
      error++;
      derror *= 0.1;
    }
    int frequency[8]={99999999,100,10,2,1,1,1,1};
    int newFactorFrequency=CoinMin(forceFactorization_,frequency[error]);
#if ABC_NORMAL_DEBUG>0
    if (newFactorFrequency<forceFactorization_)
      printf("Error of %g after %d pivots old forceFact %d now %d\n",
	     fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_)),numberPivots,
	     forceFactorization_,newFactorFrequency);
#endif
    if (!numberPivots&&fabs(btranAlpha_ - alpha_)/(1.0 + fabs(alpha_))>0.5e-6
	&&abcFactorization_->pivotTolerance()<0.5)
      abcFactorization_->saferTolerances (1.0, -1.03);
    forceFactorization_=newFactorFrequency;

    
#if ABC_NORMAL_DEBUG>0
    if (fabs(btranAlpha_ + alpha_) < 1.0e-5*(1.0 + fabs(alpha_))) {
      printf("diff (%g,%g) %g check %g\n",btranAlpha_,alpha_,fabs(btranAlpha_-alpha_),checkValue*(1.0+fabs(alpha_)));
    }
#endif
    if (numberPivots) {
      if (needFlag&&numberPivots<10) {
	// need to reject something
	assert (sequenceOut_>=0);
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>0
	printf("flag %d as alpha  %g %g - after %d pivots\n", sequenceOut_,
	       btranAlpha_, alpha_,numberPivots);
#endif
	// Make safer?
	abcFactorization_->saferTolerances (1.0, -1.03);
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	lastBadIteration_ = numberIterations_; // say be more cautious
      }
      // Make safer?
      //if (numberPivots<5)
      //abcFactorization_->saferTolerances (-0.99, -1.01);
      problemStatus_ = -2; // factorize now
      returnCode = -2;
      stateOfIteration_=2;
    } else {
      if (needFlag) {
	assert (sequenceOut_>=0);
	// need to reject something
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>3
	printf("flag a %g %g\n", btranAlpha_, alpha_);
#endif
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	//abcProgress_.clearBadTimes();
	lastBadIteration_ = numberIterations_; // say be more cautious
	if (fabs(alpha_) < 1.0e-10 && fabs(btranAlpha_) < 1.0e-8 && numberIterations_ > 100) {
	  //printf("I think should declare infeasible\n");
	  problemStatus_ = 1;
	  returnCode = 1;
	  stateOfIteration_=2;
	} else {
	  stateOfIteration_=1;
	}
	// Make safer?
	if (abcFactorization_->pivotTolerance()<0.999&&stateOfIteration_==1) {
	  // change tolerance and re-invert
	  abcFactorization_->saferTolerances (1.0, -1.03);
	  problemStatus_ = -6; // factorize now
	  returnCode = -2;
	  stateOfIteration_=2;
	}
      }
    }
  }
  if (!stateOfIteration_) {
    // check update
    //int updateStatus = 
/*
  returns 
  0 - OK
  1 - take iteration then re-factorize
  2 - flag something and skip
  3 - break and re-factorize
  5 - take iteration then re-factorize because of memory
 */
    int status=checkReplace();
    if (status&&!returnCode)
      returnCode=status;
  }
  //clearArrays(arrayForFlipRhs_);
  if (stateOfIteration_&&numberFlipped) {
    //usefulArray_[arrayForTableauRow_].compact();
    // move variables back
    flipBack(numberFlipped);
  }
  // could choose average of all three
  return returnCode;
}
int 
AbcSimplexDual::whileIteratingParallel(int numberIterations)
{
  int returnCode=-1;
#ifdef EARLY_FACTORIZE
  int savePivot=-1;
  CoinIndexedVector & early = usefulArray_[ABC_NUMBER_USEFUL-1];
  int * pivotIndices = early.getIndices();
  double * pivotValues = early.denseVector();
  int pivotCountPosition=early.capacity()-1;
  int numberSaved=0;
  if (numberIterations==123456789)
    savePivot=pivotCountPosition;;
#endif
  numberIterations += numberIterations_;
  do {
#if ABC_DEBUG
    checkArrays();
#endif
    /*
      -1 - just move dual in values pass
      0 - take iteration
      1 - don't take but continue
      2 - don't take and break
    */
    stateOfIteration_=0;
    returnCode=-1;
    // put row of tableau in usefulArray[arrayForTableauRow_]
    /*
      Could
      a) copy [arrayForBtran_] and start off updateWeights earlier
      b) break transposeTimes into two and do after slack part
      c) also think if cleaner to go all way with update but just don't do final part
    */
    //upperTheta=COIN_DBL_MAX;
    double saveAcceptable=acceptablePivot_;
    if (largestPrimalError_>1.0e-5||largestDualError_>1.0e-5) {
    //if ((largestPrimalError_>1.0e-5||largestDualError_>1.0e-5)&&false) {
      if (!abcFactorization_->pivots())
	acceptablePivot_*=1.0e2;
      else if (abcFactorization_->pivots()<5)
	acceptablePivot_*=1.0e1;
    }
#ifdef MOVE_UPDATE_WEIGHTS
    // copy btran across 
    usefulArray_[5].copy(usefulArray_[arrayForBtran_]);
    cilk_spawn abcDualRowPivot_->updateWeightsOnly(usefulArray_[5]);;
#endif
    dualColumn1();
    acceptablePivot_=saveAcceptable;
    if ((stateOfProblem_&VALUES_PASS)!=0) {
      // see if can just move dual
      if (fabs(upperTheta_-fabs(abcDj_[sequenceOut_]))<dualTolerance_) {
	stateOfIteration_=-1;
      }
    }
    if (!stateOfIteration_) {
#ifndef MOVE_UPDATE_WEIGHTS
      cilk_spawn abcDualRowPivot_->updateWeightsOnly(usefulArray_[arrayForBtran_]);;
#endif
      // get sequenceIn_
      dualPivotColumn();
      //stateOfIteration_=0;
      if (sequenceIn_ >= 0) {
	// normal iteration
	// update the incoming column
	//arrayForReplaceColumn_=getAvailableArray();
	getTableauColumnFlipAndStartReplaceCilk();
	//usleep(1000);
      } else if (stateOfIteration_!=-1) {
	stateOfIteration_=2;
      }
    }
    cilk_sync;
    // Check event
    {
      int status = eventHandler_->event(ClpEventHandler::endOfIteration);
      if (status >= 0) {
	problemStatus_ = 5;
	secondaryStatus_ = ClpEventHandler::endOfIteration;
	returnCode = 4;
	stateOfIteration_=2;
      }
    }
    if (!stateOfIteration_) {
      //	assert (stateOfIteration_!=1); 
      int whatNext=0;
      whatNext = housekeeping();
#ifdef EARLY_FACTORIZE
      if (savePivot>=0) {
	// save pivot
	pivotIndices[--savePivot]=sequenceOut_;
	pivotValues[savePivot]=alpha_;
	pivotIndices[--savePivot]=sequenceIn_;
	pivotValues[savePivot]=btranAlpha_;
	numberSaved++;
      }
#endif
      if (whatNext == 1) {
	problemStatus_ = -2; // refactorize
	usefulArray_[arrayForTableauRow_].compact();
      } else if (whatNext == 2) {
	// maximum iterations or equivalent
	problemStatus_ = 3;
	returnCode = 3;
	stateOfIteration_=2;
      }
#ifdef EARLY_FACTORIZE
      if (savePivot>=0) {
	// tell worker can update this
	pivotIndices[pivotCountPosition]=numberSaved;
      }
#endif
    } else {
      usefulArray_[arrayForTableauRow_].compact();
       if (stateOfIteration_<0) {
	// move dual in dual values pass
	theta_=abcDj_[sequenceOut_];
	updateDualsInDual();
	abcDj_[sequenceOut_]=0.0;
	sequenceOut_=-1;
      }
      // clear all arrays
      clearArrays(-1);
    }
    // at this stage sequenceIn_ may be <0
    if (sequenceIn_<0&&sequenceOut_>=0) {
      usefulArray_[arrayForTableauRow_].compact();
      // no incoming column is valid
      returnCode=noPivotColumn();
    }
    if (stateOfIteration_==2) {
      usefulArray_[arrayForTableauRow_].compact();
      sequenceIn_=-1;
#ifdef ABC_LONG_FACTORIZATION
      abcFactorization_->clearHiddenArrays();
#endif
      break;
    }
    if (problemStatus_!=-1) {
#ifndef MOVE_UPDATE_WEIGHTS
      abcDualRowPivot_->updateWeights2(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_]);
#else
      abcDualRowPivot_->updateWeights2(usefulArray_[5],usefulArray_[arrayForFtran_]);
#endif
#ifdef ABC_LONG_FACTORIZATION
      abcFactorization_->clearHiddenArrays();
#endif
      swapPrimalStuff();
      break;
    }
    if (stateOfIteration_==1) {
      // clear all arrays
      clearArrays(-2);
#ifdef ABC_LONG_FACTORIZATION
      abcFactorization_->clearHiddenArrays();
#endif
    }
    if (stateOfIteration_==0) {
      // can do these in parallel
      // No idea why I need this - but otherwise runs not repeatable (try again??)
      //usefulArray_[3].compact();
      cilk_spawn updateDualsInDual();
      int lastSequenceOut;
      int lastDirectionOut;
      if (firstFree_<0) {
	// can do in parallel
	cilk_spawn replaceColumnPart3();
	updatePrimalSolution();
	swapPrimalStuff();
	// dualRow will go to virtual row pivot choice algorithm
	// make sure values pass off if it should be
	// use Btran array and clear inside dualPivotRow (if used)
	lastSequenceOut=sequenceOut_;
	lastDirectionOut=directionOut_;
	dualPivotRow();
	cilk_sync;
      } else {
	// be more careful as dualPivotRow may do update
	cilk_spawn replaceColumnPart3();
	updatePrimalSolution();
	swapPrimalStuff();
	// dualRow will go to virtual row pivot choice algorithm
	// make sure values pass off if it should be
	// use Btran array and clear inside dualPivotRow (if used)
	lastSequenceOut=sequenceOut_;
	lastDirectionOut=directionOut_;
	cilk_sync;
	dualPivotRow();
      }
      lastPivotRow_=pivotRow_;
      if (pivotRow_ >= 0) {
	// we found a pivot row
	// create as packed
	// dual->checkReplacePart1a();
	createDualPricingVectorCilk();
	swapDualStuff(lastSequenceOut,lastDirectionOut);
      }
      cilk_sync;
    } else {
      // after moving dual in values pass
      dualPivotRow();
      lastPivotRow_=pivotRow_;
      if (pivotRow_ >= 0) {
	// we found a pivot row
	// create as packed
	createDualPricingVectorCilk();
      }
    }
    if (pivotRow_<0) {
      // no pivot row
      clearArrays(-1);
      returnCode=noPivotRow();
      break;
    }
    if (numberIterations == numberIterations_&&problemStatus_==-1) {
      returnCode=-99;
      break;
    }
  } while (problemStatus_ == -1);
  return returnCode;
}
#if 0
void
AbcSimplexDual::whileIterating2()
{
  // get sequenceIn_
  dualPivotColumn();
  //stateOfIteration_=0;
  if (sequenceIn_ >= 0) {
    // normal iteration
    // update the incoming column
    //arrayForReplaceColumn_=getAvailableArray();
    getTableauColumnFlipAndStartReplaceCilk();
    //usleep(1000);
  } else if (stateOfIteration_!=-1) {
    stateOfIteration_=2;
  }
}
#endif
#endif
void 
AbcSimplexDual::updatePrimalSolution()
{
  // finish doing weights
#ifndef MOVE_UPDATE_WEIGHTS
  abcDualRowPivot_->updatePrimalSolutionAndWeights(usefulArray_[arrayForBtran_],usefulArray_[arrayForFtran_],
					 movement_);
#else
  abcDualRowPivot_->updatePrimalSolutionAndWeights(usefulArray_[5],usefulArray_[arrayForFtran_],
					 movement_);
#endif
}
int xxInfo[6][8];
double parallelDual4(AbcSimplexDual * dual)
{
  int maximumRows=dual->maximumAbcNumberRows();
  //int numberTotal=dual->numberTotal();
  CoinIndexedVector & update = *dual->usefulArray(dual->arrayForBtran());
  CoinPartitionedVector & tableauRow= *dual->usefulArray(dual->arrayForTableauRow());
  CoinPartitionedVector & candidateList= *dual->usefulArray(dual->arrayForDualColumn());
  AbcMatrix * matrix = dual->abcMatrix();
  // do slacks first (move before sync)
  double upperTheta=dual->upperTheta();
  //assert (upperTheta>-100*dual->dualTolerance()||dual->sequenceIn()>=0||dual->lastFirstFree()>=0);
#if ABC_PARALLEL
#define NUMBER_BLOCKS NUMBER_ROW_BLOCKS
  int numberBlocks=CoinMin(NUMBER_BLOCKS,dual->numberCpus());
#else
#define NUMBER_BLOCKS 1
  int numberBlocks=NUMBER_BLOCKS;
#endif
#if ABC_PARALLEL
  if (dual->parallelMode()==0)
    numberBlocks=1;
#endif
  // see if worth using row copy
  bool gotRowCopy = matrix->gotRowCopy();
  int number=update.getNumElements();
  assert (number);
  bool useRowCopy = (gotRowCopy&&(number<<2)<maximumRows);
  assert (numberBlocks==matrix->numberRowBlocks());
#if ABC_PARALLEL==1
  // redo so can pass info in with void *
  CoinAbcThreadInfo * infoP = dual->threadInfoPointer();
  int cpuMask=((1<<dual->parallelMode())-1);
  cpuMask += cpuMask<<5;
  dual->setStopStart(cpuMask);
#endif
  CoinAbcThreadInfo info[NUMBER_BLOCKS];
  const int * starts;
  if (useRowCopy)
    starts=matrix->blockStart();
  else
    starts=matrix->startColumnBlock();
  tableauRow.setPartitions(numberBlocks,starts);
  candidateList.setPartitions(numberBlocks,starts);
  //printf("free sequence in %d\n",dual->freeSequenceIn());
  if (useRowCopy) {
    // using row copy
#if ABC_PARALLEL
    if (numberBlocks>1) {
#if ABC_PARALLEL==2
    for (int i=0;i<numberBlocks;i++) {
      info[i].stuff[1]=i;
      info[i].stuff[2]=-1;
      info[i].result=upperTheta;
      info[i].result= cilk_spawn 
	matrix->dualColumn1Row(info[i].stuff[1],COIN_DBL_MAX,info[i].stuff[2],
			       update,tableauRow,candidateList);
    }
    cilk_sync;
#else
      // parallel 1
    for (int i=0;i<numberBlocks;i++) {
      info[i].status=5;
      info[i].stuff[1]=i;
      info[i].result=upperTheta;
      if (i<numberBlocks-1) {
	infoP[i]=info[i];
	if (i==numberBlocks-2) 
	  dual->startParallelStuff(5);
      }
    }
    info[numberBlocks-1].stuff[1]=numberBlocks-1;
    info[numberBlocks-1].stuff[2]=-1;
    //double freeAlpha;
    info[numberBlocks-1].result =
      matrix->dualColumn1Row(info[numberBlocks-1].stuff[1],
			     upperTheta,info[numberBlocks-1].stuff[2],
			     update,tableauRow,candidateList);
    dual->stopParallelStuff(5);
    for (int i=0;i<numberBlocks-1;i++) 
      info[i]=infoP[i];
#endif
    } else {
#endif
      info[0].status=5;
      info[0].stuff[1]=0;
      info[0].result=upperTheta;
      info[0].stuff[1]=0;
      info[0].stuff[2]=-1;
      // worth using row copy
      //assert (number>2);
      info[0].result =
      matrix->dualColumn1Row(info[0].stuff[1],
			     upperTheta,info[0].stuff[2],
			     update,tableauRow,candidateList);
#if ABC_PARALLEL
    }
#endif
  } else { // end of useRowCopy
#if ABC_PARALLEL
    if (numberBlocks>1) {
#if ABC_PARALLEL==2
      // do by column
      for (int i=0;i<numberBlocks;i++) {
	info[i].stuff[1]=i;
	info[i].result=upperTheta;
	cilk_spawn
	  matrix->dualColumn1Part(info[i].stuff[1],info[i].stuff[2],
					     info[i].result,
					     update,tableauRow,candidateList);
      }
      cilk_sync;
#else
      // parallel 1
      // do by column
      for (int i=0;i<numberBlocks;i++) {
	info[i].status=4;
	info[i].stuff[1]=i;
	info[i].result=upperTheta;
	if (i<numberBlocks-1) {
	  infoP[i]=info[i];
	  if (i==numberBlocks-2) 
	    dual->startParallelStuff(4);
	}
      }
      matrix->dualColumn1Part(info[numberBlocks-1].stuff[1],info[numberBlocks-1].stuff[2],
							    info[numberBlocks-1].result,
							    update,tableauRow,candidateList);
      dual->stopParallelStuff(4);
      for (int i=0;i<numberBlocks-1;i++) 
	info[i]=infoP[i];
#endif
    } else {
#endif
      info[0].status=4;
      info[0].stuff[1]=0;
      info[0].result=upperTheta;
      info[0].stuff[1]=0;
      matrix->dualColumn1Part(info[0].stuff[1],info[0].stuff[2],
					       info[0].result,
					       update,tableauRow,candidateList);
#if ABC_PARALLEL
    }
#endif
  }
  int sequenceIn[NUMBER_BLOCKS];
  bool anyFree=false;
#if 0
  if (useRowCopy) {
    printf("Result at iteration %d slack seqIn %d upperTheta %g\n",
	   dual->numberIterations(),dual->freeSequenceIn(),upperTheta);
    double * arrayT = tableauRow.denseVector();
    int * indexT = tableauRow.getIndices();
    double * arrayC = candidateList.denseVector();
    int * indexC = candidateList.getIndices();
    for (int i=0;i<numberBlocks;i++) {
      printf("Block %d seqIn %d upperTheta %g first %d last %d firstIn %d nnz %d numrem %d\n",
	     i,info[i].stuff[2],info[i].result,
	     xxInfo[0][i],xxInfo[1][i],xxInfo[2][i],xxInfo[3][i],xxInfo[4][i]);
      if (xxInfo[3][i]<-35) {
	assert (xxInfo[3][i]==tableauRow.getNumElements(i));
	assert (xxInfo[4][i]==candidateList.getNumElements(i));
	for (int k=0;k<xxInfo[3][i];k++) 
	  printf("T %d %d %g\n",k,indexT[k+xxInfo[2][i]],arrayT[k+xxInfo[2][i]]);
	for (int k=0;k<xxInfo[4][i];k++) 
	  printf("C %d %d %g\n",k,indexC[k+xxInfo[2][i]],arrayC[k+xxInfo[2][i]]);
      }
    }
  }
#endif
  for (int i=0;i<numberBlocks;i++) {
    sequenceIn[i]=info[i].stuff[2];
    if (sequenceIn[i]>=0)
      anyFree=true;
    upperTheta=CoinMin(info[i].result,upperTheta);
    //assert (info[i].result>-100*dual->dualTolerance()||sequenceIn[i]>=0||dual->lastFirstFree()>=0);
  }
  if (anyFree) {
    int *  COIN_RESTRICT index = tableauRow.getIndices();
    double *  COIN_RESTRICT array = tableauRow.denseVector();
    // search for free coming in
    double bestFree=0.0;
    int bestSequence=dual->sequenceIn();
    if (bestSequence>=0)
      bestFree=dual->alpha();
    for (int i=0;i<numberBlocks;i++) {
      int iLook=sequenceIn[i];
      if (iLook>=0) {
	// free variable - search 
	int start=starts[i];
	int end=start+tableauRow.getNumElements(i);
	for (int k=start;k<end;k++) {
	  if (iLook==index[k]) {
	    if (fabs(bestFree)<fabs(array[k])) {
	      bestFree=array[k];
	      bestSequence=iLook;
	    }
	    break;
	  }
	}
      }
    }
    if (bestSequence>=0) {
      double oldValue = dual->djRegion()[bestSequence];
      dual->setSequenceIn(bestSequence);
      dual->setAlpha(bestFree);
      dual->setTheta(oldValue / bestFree);
    }
  }
  //tableauRow.compact();
  //candidateList.compact();
#if 0//ndef NDEBUG
  tableauRow.setPackedMode(true);
  tableauRow.sortPacked();
  candidateList.setPackedMode(true);
  candidateList.sortPacked();
#endif
  return upperTheta;
}
#if ABC_PARALLEL==2
static void
parallelDual5a(AbcSimplexFactorization * factorization,
	      CoinIndexedVector * whichVector,
	      int numberCpu,
	      int whichCpu,
	      double * weights)
{
  double *  COIN_RESTRICT array = whichVector->denseVector();
  int *  COIN_RESTRICT which = whichVector->getIndices();
  int numberRows=factorization->numberRows();
  for (int i = whichCpu; i < numberRows; i+=numberCpu) {
    double value = 0.0;
    array[i] = 1.0;
    which[0] = i;
    whichVector->setNumElements(1);
    factorization->updateColumnTransposeCpu(*whichVector,whichCpu);
    int number = whichVector->getNumElements();
    for (int j = 0; j < number; j++) {
      int k= which[j];
      value += array[k] * array[k];
      array[k] = 0.0;
    }
    weights[i] = value;
  }
  whichVector->setNumElements(0);
}
#endif
#if ABC_PARALLEL==2
void
parallelDual5(AbcSimplexFactorization * factorization,
	      CoinIndexedVector ** whichVector,
	      int numberCpu,
	      int whichCpu,
	      double * weights)
{
  if (whichCpu) {
    cilk_spawn parallelDual5(factorization,whichVector,numberCpu,whichCpu-1,weights);
    parallelDual5a(factorization,whichVector[whichCpu],numberCpu,whichCpu,weights);
    cilk_sync;
  } else {
    parallelDual5a(factorization,whichVector[whichCpu],numberCpu,whichCpu,weights);
  }
}
#endif
// cilk seems a bit fragile
#define CILK_FRAGILE 1
#if CILK_FRAGILE>1
#undef cilk_spawn
#undef cilk_sync
#define cilk_spawn
#define cilk_sync
#define ONWARD 0
#elif CILK_FRAGILE==1
#define ONWARD 0
#else
#define ONWARD 1
#endif
// Code below is just a translation of LAPACK
#define BLOCKING1 8 // factorization strip
#define BLOCKING2 8 // dgemm recursive
#define BLOCKING3 16 // dgemm parallel
/* type
   0 Left Lower NoTranspose Unit
   1 Left Upper NoTranspose NonUnit
   2 Left Lower Transpose Unit
   3 Left Upper Transpose NonUnit
*/
static void CoinAbcDtrsmFactor(int m, int n, double * COIN_RESTRICT a,int lda)
{
  assert ((m&(BLOCKING8-1))==0&&(n&(BLOCKING8-1))==0);
  assert (m==BLOCKING8);
  // 0 Left Lower NoTranspose Unit
  /* entry for column j and row i (when multiple of BLOCKING8)
     is at aBlocked+j*m+i*BLOCKING8
  */
  double * COIN_RESTRICT aBase2 = a;
  double * COIN_RESTRICT bBase2 = aBase2+lda*BLOCKING8;
  for (int jj=0;jj<n;jj+=BLOCKING8) {
    double * COIN_RESTRICT bBase = bBase2;
    for (int j=jj;j<jj+BLOCKING8;j++) {
#if 0
      double * COIN_RESTRICT aBase = aBase2;
      for (int k=0;k<BLOCKING8;k++) {
	double bValue = bBase[k];
	if (bValue) {
	  for (int i=k+1;i<BLOCKING8;i++) {
	    bBase[i]-=bValue*aBase[i];
	  }
	}
	aBase+=BLOCKING8;
      }
#else
      // unroll - stay in registers - don't test for zeros
      assert (BLOCKING8==8);
      double bValue0 = bBase[0];
      double bValue1 = bBase[1];
      double bValue2 = bBase[2];
      double bValue3 = bBase[3];
      double bValue4 = bBase[4];
      double bValue5 = bBase[5];
      double bValue6 = bBase[6];
      double bValue7 = bBase[7];
      bValue1-=bValue0*a[1+0*BLOCKING8];
      bBase[1] = bValue1;
      bValue2-=bValue0*a[2+0*BLOCKING8];
      bValue3-=bValue0*a[3+0*BLOCKING8];
      bValue4-=bValue0*a[4+0*BLOCKING8];
      bValue5-=bValue0*a[5+0*BLOCKING8];
      bValue6-=bValue0*a[6+0*BLOCKING8];
      bValue7-=bValue0*a[7+0*BLOCKING8];
      bValue2-=bValue1*a[2+1*BLOCKING8];
      bBase[2] = bValue2;
      bValue3-=bValue1*a[3+1*BLOCKING8];
      bValue4-=bValue1*a[4+1*BLOCKING8];
      bValue5-=bValue1*a[5+1*BLOCKING8];
      bValue6-=bValue1*a[6+1*BLOCKING8];
      bValue7-=bValue1*a[7+1*BLOCKING8];
      bValue3-=bValue2*a[3+2*BLOCKING8];
      bBase[3] = bValue3;
      bValue4-=bValue2*a[4+2*BLOCKING8];
      bValue5-=bValue2*a[5+2*BLOCKING8];
      bValue6-=bValue2*a[6+2*BLOCKING8];
      bValue7-=bValue2*a[7+2*BLOCKING8];
      bValue4-=bValue3*a[4+3*BLOCKING8];
      bBase[4] = bValue4;
      bValue5-=bValue3*a[5+3*BLOCKING8];
      bValue6-=bValue3*a[6+3*BLOCKING8];
      bValue7-=bValue3*a[7+3*BLOCKING8];
      bValue5-=bValue4*a[5+4*BLOCKING8];
      bBase[5] = bValue5;
      bValue6-=bValue4*a[6+4*BLOCKING8];
      bValue7-=bValue4*a[7+4*BLOCKING8];
      bValue6-=bValue5*a[6+5*BLOCKING8];
      bBase[6] = bValue6;
      bValue7-=bValue5*a[7+5*BLOCKING8];
      bValue7-=bValue6*a[7+6*BLOCKING8];
      bBase[7] = bValue7;
#endif
      bBase += BLOCKING8;
    }
    bBase2 +=lda*BLOCKING8; 
  }
}
#define UNROLL_DTRSM 16
#define CILK_DTRSM 32
static void dtrsm0(int kkk, int first, int last,
		   int m, double * COIN_RESTRICT a,double * COIN_RESTRICT b)
{
  int mm=CoinMin(kkk+UNROLL_DTRSM*BLOCKING8,m);
  assert ((last-first)%BLOCKING8==0);
  if (last-first>CILK_DTRSM) {
    int mid=((first+last)>>4)<<3;
    cilk_spawn dtrsm0(kkk,first,mid,m,a,b);
    dtrsm0(kkk,mid,last,m,a,b);
    cilk_sync;
  } else {
    const double * COIN_RESTRICT aBaseA = a+UNROLL_DTRSM*BLOCKING8X8+kkk*BLOCKING8;
    aBaseA += (first-mm)*BLOCKING8-BLOCKING8X8;
    aBaseA += m*kkk;
    for (int ii=first;ii<last;ii+=BLOCKING8) {
      aBaseA += BLOCKING8X8;
      const double * COIN_RESTRICT aBaseB=aBaseA;
      double * COIN_RESTRICT bBaseI = b+ii;
      for (int kk=kkk;kk<mm;kk+=BLOCKING8) {
	double * COIN_RESTRICT bBase = b+kk;
	const double * COIN_RESTRICT aBase2 = aBaseB;//a+UNROLL_DTRSM*BLOCKING8X8+m*kk+kkk*BLOCKING8;
	//aBase2 += (ii-mm)*BLOCKING8;
	//assert (aBase2==aBaseB);
	aBaseB+=m*BLOCKING8;
#if AVX2 !=2
#define ALTERNATE_INNER
#ifndef ALTERNATE_INNER
	for (int k=0;k<BLOCKING8;k++) {
	  //coin_prefetch_const(aBase2+BLOCKING8);		
	  for (int i=0;i<BLOCKING8;i++) {
	    bBaseI[i] -= bBase[k]*aBase2[i];
	  }
	  aBase2 += BLOCKING8;
	}
#else
	double b0=bBaseI[0];
	double b1=bBaseI[1];
	double b2=bBaseI[2];
	double b3=bBaseI[3];
	double b4=bBaseI[4];
	double b5=bBaseI[5];
	double b6=bBaseI[6];
	double b7=bBaseI[7];
	for (int k=0;k<BLOCKING8;k++) {
	  //coin_prefetch_const(aBase2+BLOCKING8);		
	  double bValue=bBase[k];
	  b0 -= bValue * aBase2[0];
	  b1 -= bValue * aBase2[1];
	  b2 -= bValue * aBase2[2];
	  b3 -= bValue * aBase2[3];
	  b4 -= bValue * aBase2[4];
	  b5 -= bValue * aBase2[5];
	  b6 -= bValue * aBase2[6];
	  b7 -= bValue * aBase2[7];
	  aBase2 += BLOCKING8;
	}
	bBaseI[0]=b0;
	bBaseI[1]=b1;
	bBaseI[2]=b2;
	bBaseI[3]=b3;
	bBaseI[4]=b4;
	bBaseI[5]=b5;
	bBaseI[6]=b6;
	bBaseI[7]=b7;
#endif
#else
	__m256d b0=_mm256_load_pd(bBaseI);
	__m256d b1=_mm256_load_pd(bBaseI+4);
	for (int k=0;k<BLOCKING8;k++) {
	  //coin_prefetch_const(aBase2+BLOCKING8);		
	  __m256d bb = _mm256_broadcast_sd(bBase+k);
	  __m256d a0 = _mm256_load_pd(aBase2);
	  b0 -= a0*bb;
	  __m256d a1 = _mm256_load_pd(aBase2+4);
	  b1 -= a1*bb;
	  aBase2 += BLOCKING8;
	}
	//_mm256_store_pd ((bBaseI), (b0));
	*reinterpret_cast<__m256d *>(bBaseI)=b0;
	//_mm256_store_pd (bBaseI+4, b1);
	*reinterpret_cast<__m256d *>(bBaseI+4)=b1;
#endif
      }
    }
  }
}
void CoinAbcDtrsm0(int m, double * COIN_RESTRICT a,double * COIN_RESTRICT b)
{
  assert ((m&(BLOCKING8-1))==0);
  // 0 Left Lower NoTranspose Unit
  for (int kkk=0;kkk<m;kkk+=UNROLL_DTRSM*BLOCKING8) {
    int mm=CoinMin(m,kkk+UNROLL_DTRSM*BLOCKING8);
    for (int kk=kkk;kk<mm;kk+=BLOCKING8) {
      const double * COIN_RESTRICT aBase2 = a+kk*(m+BLOCKING8);
      double * COIN_RESTRICT bBase = b+kk;
      for (int k=0;k<BLOCKING8;k++) {
	for (int i=k+1;i<BLOCKING8;i++) {
	  bBase[i] -= bBase[k]*aBase2[i];
	}
	aBase2 += BLOCKING8;
      }
      for (int ii=kk+BLOCKING8;ii<mm;ii+=BLOCKING8) {
	double * COIN_RESTRICT bBaseI = b+ii;
	for (int k=0;k<BLOCKING8;k++) {
	  //coin_prefetch_const(aBase2+BLOCKING8);		
	  for (int i=0;i<BLOCKING8;i++) {
	    bBaseI[i] -= bBase[k]*aBase2[i];
	  }
	  aBase2 += BLOCKING8;
	}
      }
    }
    dtrsm0(kkk,mm,m,m,a,b);
  }
}
static void dtrsm1(int kkk, int first, int last,
		   int m, double * COIN_RESTRICT a,double * COIN_RESTRICT b)
{
  int mm=CoinMax(0,kkk-(UNROLL_DTRSM-1)*BLOCKING8);
  assert ((last-first)%BLOCKING8==0);
  if (last-first>CILK_DTRSM) {
    int mid=((first+last)>>4)<<3;
    cilk_spawn dtrsm1(kkk,first,mid,m,a,b);
    dtrsm1(kkk,mid,last,m,a,b);
    cilk_sync;
  } else {
    for (int iii=last-BLOCKING8;iii>=first;iii-=BLOCKING8) {
      double * COIN_RESTRICT bBase2 = b+iii;
      const double * COIN_RESTRICT aBaseA=
	a+BLOCKING8X8+BLOCKING8*iii;
      for (int ii=kkk;ii>=mm;ii-=BLOCKING8) {
	double * COIN_RESTRICT bBase = b+ii;
	const double * COIN_RESTRICT aBase=aBaseA+m*ii;
#if AVX2 !=2
#ifndef ALTERNATE_INNER
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase -= BLOCKING8;
	  //coin_prefetch_const(aBase-BLOCKING8);		
	  for (int k=BLOCKING8-1;k>=0;k--) {
	    bBase2[k] -= bBase[i]*aBase[k];
	  }
	}
#else
	double b0=bBase2[0];
	double b1=bBase2[1];
	double b2=bBase2[2];
	double b3=bBase2[3];
	double b4=bBase2[4];
	double b5=bBase2[5];
	double b6=bBase2[6];
	double b7=bBase2[7];
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase -= BLOCKING8;
	  //coin_prefetch_const(aBase-BLOCKING8);		
	  double bValue=bBase[i];
	  b0 -= bValue * aBase[0];
	  b1 -= bValue * aBase[1];
	  b2 -= bValue * aBase[2];
	  b3 -= bValue * aBase[3];
	  b4 -= bValue * aBase[4];
	  b5 -= bValue * aBase[5];
	  b6 -= bValue * aBase[6];
	  b7 -= bValue * aBase[7];
	}
	bBase2[0]=b0;
	bBase2[1]=b1;
	bBase2[2]=b2;
	bBase2[3]=b3;
	bBase2[4]=b4;
	bBase2[5]=b5;
	bBase2[6]=b6;
	bBase2[7]=b7;
#endif
#else
	__m256d b0=_mm256_load_pd(bBase2);
	__m256d b1=_mm256_load_pd(bBase2+4);
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase -= BLOCKING8;
	  //coin_prefetch_const(aBase5-BLOCKING8);		
	  __m256d bb = _mm256_broadcast_sd(bBase+i);
	  __m256d a0 = _mm256_load_pd(aBase);
	  b0 -= a0*bb;
	  __m256d a1 = _mm256_load_pd(aBase+4);
	  b1 -= a1*bb;
	}
	//_mm256_store_pd (bBase2, b0);
	*reinterpret_cast<__m256d *>(bBase2)=b0;
	//_mm256_store_pd (bBase2+4, b1);
	*reinterpret_cast<__m256d *>(bBase2+4)=b1;
#endif
      }
    }
  }
}
void CoinAbcDtrsm1(int m, double * COIN_RESTRICT a,double * COIN_RESTRICT b)
{
  assert ((m&(BLOCKING8-1))==0);
  // 1 Left Upper NoTranspose NonUnit
  for (int kkk=m-BLOCKING8;kkk>=0;kkk-=UNROLL_DTRSM*BLOCKING8) {
    int mm=CoinMax(0,kkk-(UNROLL_DTRSM-1)*BLOCKING8);
    for (int ii=kkk;ii>=mm;ii-=BLOCKING8) {
      const double * COIN_RESTRICT aBase = a+m*ii+ii*BLOCKING8;
      double * COIN_RESTRICT bBase = b+ii;
      for (int i=BLOCKING8-1;i>=0;i--) {
	bBase[i] *= aBase[i*(BLOCKING8+1)];
	for (int k=i-1;k>=0;k--) {
	  bBase[k] -= bBase[i]*aBase[k+i*BLOCKING8];
	}
      }
      double * COIN_RESTRICT bBase2 = bBase;
      for (int iii=ii;iii>mm;iii-=BLOCKING8) {
	bBase2 -= BLOCKING8;
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase -= BLOCKING8;
	  coin_prefetch_const(aBase-BLOCKING8);		
	  for (int k=BLOCKING8-1;k>=0;k--) {
	    bBase2[k] -= bBase[i]*aBase[k];
	  }
	}
      }
    }
    dtrsm1(kkk, 0, mm,  m, a, b);
  }
}
static void dtrsm2(int kkk, int first, int last,
		   int m, double * COIN_RESTRICT a,double * COIN_RESTRICT b)
{
  int mm=CoinMax(0,kkk-(UNROLL_DTRSM-1)*BLOCKING8);
  assert ((last-first)%BLOCKING8==0);
  if (last-first>CILK_DTRSM) {
    int mid=((first+last)>>4)<<3;
    cilk_spawn dtrsm2(kkk,first,mid,m,a,b);
    dtrsm2(kkk,mid,last,m,a,b);
    cilk_sync;
  } else {
    for (int iii=last-BLOCKING8;iii>=first;iii-=BLOCKING8) {
      for (int ii=kkk;ii>=mm;ii-=BLOCKING8) {
	const double * COIN_RESTRICT bBase = b+ii;
	double * COIN_RESTRICT bBase2=b+iii;
	const double * COIN_RESTRICT aBase=a+ii*BLOCKING8+m*iii+BLOCKING8X8;
	for (int j=BLOCKING8-1;j>=0;j-=4) {
	  double bValue0=bBase2[j];
	  double bValue1=bBase2[j-1];
	  double bValue2=bBase2[j-2];
	  double bValue3=bBase2[j-3];
	  bValue0 -= aBase[-1*BLOCKING8+7]*bBase[7];
	  bValue1 -= aBase[-2*BLOCKING8+7]*bBase[7];
	  bValue2 -= aBase[-3*BLOCKING8+7]*bBase[7];
	  bValue3 -= aBase[-4*BLOCKING8+7]*bBase[7];
	  bValue0 -= aBase[-1*BLOCKING8+6]*bBase[6];
	  bValue1 -= aBase[-2*BLOCKING8+6]*bBase[6];
	  bValue2 -= aBase[-3*BLOCKING8+6]*bBase[6];
	  bValue3 -= aBase[-4*BLOCKING8+6]*bBase[6];
	  bValue0 -= aBase[-1*BLOCKING8+5]*bBase[5];
	  bValue1 -= aBase[-2*BLOCKING8+5]*bBase[5];
	  bValue2 -= aBase[-3*BLOCKING8+5]*bBase[5];
	  bValue3 -= aBase[-4*BLOCKING8+5]*bBase[5];
	  bValue0 -= aBase[-1*BLOCKING8+4]*bBase[4];
	  bValue1 -= aBase[-2*BLOCKING8+4]*bBase[4];
	  bValue2 -= aBase[-3*BLOCKING8+4]*bBase[4];
	  bValue3 -= aBase[-4*BLOCKING8+4]*bBase[4];
	  bValue0 -= aBase[-1*BLOCKING8+3]*bBase[3];
	  bValue1 -= aBase[-2*BLOCKING8+3]*bBase[3];
	  bValue2 -= aBase[-3*BLOCKING8+3]*bBase[3];
	  bValue3 -= aBase[-4*BLOCKING8+3]*bBase[3];
	  bValue0 -= aBase[-1*BLOCKING8+2]*bBase[2];
	  bValue1 -= aBase[-2*BLOCKING8+2]*bBase[2];
	  bValue2 -= aBase[-3*BLOCKING8+2]*bBase[2];
	  bValue3 -= aBase[-4*BLOCKING8+2]*bBase[2];
	  bValue0 -= aBase[-1*BLOCKING8+1]*bBase[1];
	  bValue1 -= aBase[-2*BLOCKING8+1]*bBase[1];
	  bValue2 -= aBase[-3*BLOCKING8+1]*bBase[1];
	  bValue3 -= aBase[-4*BLOCKING8+1]*bBase[1];
	  bValue0 -= aBase[-1*BLOCKING8+0]*bBase[0];
	  bValue1 -= aBase[-2*BLOCKING8+0]*bBase[0];
	  bValue2 -= aBase[-3*BLOCKING8+0]*bBase[0];
	  bValue3 -= aBase[-4*BLOCKING8+0]*bBase[0];
	  bBase2[j]=bValue0;
	  bBase2[j-1]=bValue1;
	  bBase2[j-2]=bValue2;
	  bBase2[j-3]=bValue3;
	  aBase -= 4*BLOCKING8;
	}
      }
    }
  }
}
void CoinAbcDtrsm2(int m, double * COIN_RESTRICT a,double * COIN_RESTRICT b)
{
  assert ((m&(BLOCKING8-1))==0);
  // 2 Left Lower Transpose Unit
  for (int kkk=m-BLOCKING8;kkk>=0;kkk-=UNROLL_DTRSM*BLOCKING8) {
    int mm=CoinMax(0,kkk-(UNROLL_DTRSM-1)*BLOCKING8);
    for (int ii=kkk;ii>=mm;ii-=BLOCKING8) {
      double * COIN_RESTRICT bBase = b+ii;
      double * COIN_RESTRICT bBase2 = bBase;
      const double * COIN_RESTRICT aBase=a+m*ii+(ii+BLOCKING8)*BLOCKING8;
      for (int i=BLOCKING8-1;i>=0;i--) {
	aBase -= BLOCKING8;
	for (int k=i+1;k<BLOCKING8;k++) {
	  bBase2[i] -= aBase[k]*bBase2[k];
	}
      }
      for (int iii=ii-BLOCKING8;iii>=mm;iii-=BLOCKING8) {
	bBase2 -= BLOCKING8;
	assert (bBase2==b+iii);
	aBase -= m*BLOCKING8;
	const double * COIN_RESTRICT aBase2 = aBase+BLOCKING8X8;
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase2 -= BLOCKING8;
	  for (int k=BLOCKING8-1;k>=0;k--) {
	    bBase2[i] -= aBase2[k]*bBase[k];
	  }
	}
      }
    }
    dtrsm2(kkk, 0, mm,  m, a, b);
  }
}
#define UNROLL_DTRSM3 16
#define CILK_DTRSM3 32
static void dtrsm3(int kkk, int first, int last,
		   int m, double * COIN_RESTRICT a,double * COIN_RESTRICT b)
{
  //int mm=CoinMin(kkk+UNROLL_DTRSM3*BLOCKING8,m);
  assert ((last-first)%BLOCKING8==0);
  if (last-first>CILK_DTRSM3) {
    int mid=((first+last)>>4)<<3;
    cilk_spawn dtrsm3(kkk,first,mid,m,a,b);
    dtrsm3(kkk,mid,last,m,a,b);
    cilk_sync;
  } else {
    for (int kk=0;kk<kkk;kk+=BLOCKING8) {
      for (int ii=first;ii<last;ii+=BLOCKING8) {
	double * COIN_RESTRICT bBase = b+ii;
	double * COIN_RESTRICT bBase2 = b+kk;
	const double * COIN_RESTRICT aBase=a+ii*m+kk*BLOCKING8;
	for (int j=0;j<BLOCKING8;j+=4) {
	  double bValue0=bBase[j];
	  double bValue1=bBase[j+1];
	  double bValue2=bBase[j+2];
	  double bValue3=bBase[j+3];
	  bValue0 -= aBase[0]*bBase2[0];
	  bValue1 -= aBase[1*BLOCKING8+0]*bBase2[0];
	  bValue2 -= aBase[2*BLOCKING8+0]*bBase2[0];
	  bValue3 -= aBase[3*BLOCKING8+0]*bBase2[0];
	  bValue0 -= aBase[1]*bBase2[1];
	  bValue1 -= aBase[1*BLOCKING8+1]*bBase2[1];
	  bValue2 -= aBase[2*BLOCKING8+1]*bBase2[1];
	  bValue3 -= aBase[3*BLOCKING8+1]*bBase2[1];
	  bValue0 -= aBase[2]*bBase2[2];
	  bValue1 -= aBase[1*BLOCKING8+2]*bBase2[2];
	  bValue2 -= aBase[2*BLOCKING8+2]*bBase2[2];
	  bValue3 -= aBase[3*BLOCKING8+2]*bBase2[2];
	  bValue0 -= aBase[3]*bBase2[3];
	  bValue1 -= aBase[1*BLOCKING8+3]*bBase2[3];
	  bValue2 -= aBase[2*BLOCKING8+3]*bBase2[3];
	  bValue3 -= aBase[3*BLOCKING8+3]*bBase2[3];
	  bValue0 -= aBase[4]*bBase2[4];
	  bValue1 -= aBase[1*BLOCKING8+4]*bBase2[4];
	  bValue2 -= aBase[2*BLOCKING8+4]*bBase2[4];
	  bValue3 -= aBase[3*BLOCKING8+4]*bBase2[4];
	  bValue0 -= aBase[5]*bBase2[5];
	  bValue1 -= aBase[1*BLOCKING8+5]*bBase2[5];
	  bValue2 -= aBase[2*BLOCKING8+5]*bBase2[5];
	  bValue3 -= aBase[3*BLOCKING8+5]*bBase2[5];
	  bValue0 -= aBase[6]*bBase2[6];
	  bValue1 -= aBase[1*BLOCKING8+6]*bBase2[6];
	  bValue2 -= aBase[2*BLOCKING8+7]*bBase2[7];
	  bValue3 -= aBase[3*BLOCKING8+6]*bBase2[6];
	  bValue0 -= aBase[7]*bBase2[7];
	  bValue1 -= aBase[1*BLOCKING8+7]*bBase2[7];
	  bValue2 -= aBase[2*BLOCKING8+6]*bBase2[6];
	  bValue3 -= aBase[3*BLOCKING8+7]*bBase2[7];
	  bBase[j]=bValue0;
	  bBase[j+1]=bValue1;
	  bBase[j+2]=bValue2;
	  bBase[j+3]=bValue3;
	  aBase += 4*BLOCKING8;
	}
      }
    }
  }
}
void CoinAbcDtrsm3(int m, double * COIN_RESTRICT a,double * COIN_RESTRICT b)
{
  assert ((m&(BLOCKING8-1))==0);
  // 3 Left Upper Transpose NonUnit
  for (int kkk=0;kkk<m;kkk+=UNROLL_DTRSM3*BLOCKING8) {
    int mm=CoinMin(m,kkk+UNROLL_DTRSM3*BLOCKING8);
    dtrsm3(kkk, kkk, mm,  m, a, b);
    for (int ii=kkk;ii<mm;ii+=BLOCKING8) {
      double * COIN_RESTRICT bBase = b+ii;
      for (int kk=kkk;kk<ii;kk+=BLOCKING8) {
	double * COIN_RESTRICT bBase2 = b+kk;
	const double * COIN_RESTRICT aBase=a+ii*m+kk*BLOCKING8;
	for (int i=0;i<BLOCKING8;i++) {
	  for (int k=0;k<BLOCKING8;k++) {
	    bBase[i] -= aBase[k]*bBase2[k];
	  }
	  aBase += BLOCKING8;
	}
      }
      const double * COIN_RESTRICT aBase=a+ii*m+ii*BLOCKING8;
      for (int i=0;i<BLOCKING8;i++) {
	for (int k=0;k<i;k++) {
	  bBase[i] -= aBase[k]*bBase[k];
	}
	bBase[i] = bBase[i]*aBase[i];
	aBase += BLOCKING8;
      }
    }
  }
}
static void 
CoinAbcDlaswp(int n, double * COIN_RESTRICT a,int lda,int start,int end, int * ipiv)
{
  assert ((n&(BLOCKING8-1))==0);
  double * COIN_RESTRICT aThis3 = a;
  for (int j=0;j<n;j+=BLOCKING8) {
    for (int i=start;i<end;i++) {
      int ip=ipiv[i];
      if (ip!=i) {
	double * COIN_RESTRICT aTop=aThis3+j*lda;
	int iBias = ip/BLOCKING8;
	ip-=iBias*BLOCKING8;
	double * COIN_RESTRICT aNotTop=aTop+iBias*BLOCKING8X8;
	iBias = i/BLOCKING8;
	int i2=i-iBias*BLOCKING8;
	aTop += iBias*BLOCKING8X8;
	for (int k=0;k<BLOCKING8;k++) {
	  double temp=aTop[i2];
	  aTop[i2]=aNotTop[ip];
	  aNotTop[ip]=temp;
	  aTop += BLOCKING8;
	  aNotTop += BLOCKING8;
	}
      }
    }
  }
}
extern void CoinAbcDgemm(int m, int n, int k, double * COIN_RESTRICT a,int lda,
			  double * COIN_RESTRICT b,double * COIN_RESTRICT c
#if ABC_PARALLEL==2
			  ,int parallelMode
#endif
			 );
static int CoinAbcDgetrf2(int m, int n, double * COIN_RESTRICT a, int * ipiv)
{
  assert ((m&(BLOCKING8-1))==0&&(n&(BLOCKING8-1))==0);
  assert (n==BLOCKING8);
  int dimension=CoinMin(m,n);
  /* entry for column j and row i (when multiple of BLOCKING8)
     is at aBlocked+j*m+i*BLOCKING8
  */
  assert (dimension==BLOCKING8);
  //printf("Dgetrf2 m=%d n=%d\n",m,n);
  double * COIN_RESTRICT aThis3 = a;
  for (int j=0;j<dimension;j++) {
    int pivotRow=-1;
    double largest=0.0;
    double * COIN_RESTRICT aThis2 = aThis3+j*BLOCKING8;
    // this block
    int pivotRow2=-1;
    for (int i=j;i<BLOCKING8;i++) {
      double value=fabs(aThis2[i]);
      if (value>largest) {
	largest=value;
	pivotRow2=i;
      }
    }
    // other blocks
    int iBias=0;
    for (int ii=BLOCKING8;ii<m;ii+=BLOCKING8) {
      aThis2+=BLOCKING8*BLOCKING8;
      for (int i=0;i<BLOCKING8;i++) {
	double value=fabs(aThis2[i]);
	if (value>largest) {
	  largest=value;
	  pivotRow2=i;
	  iBias=ii;
	}
      }
    }
    pivotRow=pivotRow2+iBias;
    ipiv[j]=pivotRow;
    if (largest) {
      if (j!=pivotRow) {
	double * COIN_RESTRICT aTop=aThis3;
	double * COIN_RESTRICT aNotTop=aThis3+iBias*BLOCKING8;
	for (int i=0;i<n;i++) {
	  double value = aTop[j];
	  aTop[j]=aNotTop[pivotRow2];
	  aNotTop[pivotRow2]=value;
	  aTop += BLOCKING8;
	  aNotTop += BLOCKING8;
	}
      }
      aThis2 = aThis3+j*BLOCKING8;
      double pivotMultiplier = 1.0 / aThis2[j];
      aThis2[j] = pivotMultiplier;
      // Do L
      // this block
      for (int i=j+1;i<BLOCKING8;i++) 
	aThis2[i] *= pivotMultiplier;
      // other blocks
      for (int ii=BLOCKING8;ii<m;ii+=BLOCKING8) {
	aThis2+=BLOCKING8*BLOCKING8;
	for (int i=0;i<BLOCKING8;i++) 
	  aThis2[i] *= pivotMultiplier;
      }
      // update rest
      double * COIN_RESTRICT aPut;
      aThis2 = aThis3+j*BLOCKING8;
      aPut = aThis2+BLOCKING8;
      double * COIN_RESTRICT aPut2 = aPut;
      // this block
      for (int i=j+1;i<BLOCKING8;i++) {
	double value = aPut[j];
	for (int k=j+1;k<BLOCKING8;k++) 
	  aPut[k] -= value*aThis2[k];
	aPut += BLOCKING8;
      }
      // other blocks
      for (int ii=BLOCKING8;ii<m;ii+=BLOCKING8) {
	aThis2 += BLOCKING8*BLOCKING8;
	aPut = aThis2+BLOCKING8;
	double * COIN_RESTRICT aPut2a = aPut2;
	for (int i=j+1;i<BLOCKING8;i++) {
	  double value = aPut2a[j];
	  for (int k=0;k<BLOCKING8;k++) 
	    aPut[k] -= value*aThis2[k];
	  aPut += BLOCKING8;
	  aPut2a += BLOCKING8;
	}
      }
    } else {
      return 1;
    }
  }
  return 0;
}

int 
CoinAbcDgetrf(int m, int n, double * COIN_RESTRICT a, int lda, int * ipiv
#if ABC_PARALLEL==2
			  ,int parallelMode
#endif
)
{
  assert (m==n);
  assert ((m&(BLOCKING8-1))==0&&(n&(BLOCKING8-1))==0);
  if (m<BLOCKING8) {
    return CoinAbcDgetrf2(m,n,a,ipiv);
  } else {
    for (int j=0;j<n;j+=BLOCKING8) {
      int start=j;
      int newSize=CoinMin(BLOCKING8,n-j);
      int end=j+newSize;
      int returnCode=CoinAbcDgetrf2(m-start,newSize,a+(start*lda+start*BLOCKING8),
				     ipiv+start);
      if (!returnCode) {
	// adjust
	for (int k=start;k<end;k++)
	  ipiv[k]+=start;
	// swap 0<start
	CoinAbcDlaswp(start,a,lda,start,end,ipiv);
	if (end<n) {
	  // swap >=end
	  CoinAbcDlaswp(n-end,a+end*lda,lda,start,end,ipiv);
          CoinAbcDtrsmFactor(newSize,n-end,a+(start*lda+start*BLOCKING8),lda);
          CoinAbcDgemm(n-end,n-end,newSize,
			a+start*lda+end*BLOCKING8,lda,
			a+end*lda+start*BLOCKING8,a+end*lda+end*BLOCKING8
#if ABC_PARALLEL==2
			  ,parallelMode
#endif
			);
	}
      } else {
	return returnCode;
      }
    }
  }
  return 0;
}
void
CoinAbcDgetrs(char trans,int m, double * COIN_RESTRICT a, double * COIN_RESTRICT work)
{
  assert ((m&(BLOCKING8-1))==0);
  if (trans=='N') {
    //CoinAbcDlaswp1(work,m,ipiv);
    CoinAbcDtrsm0(m,a,work);
    CoinAbcDtrsm1(m,a,work);
  } else {
    assert (trans=='T');
    CoinAbcDtrsm3(m,a,work);
    CoinAbcDtrsm2(m,a,work);
    //CoinAbcDlaswp1Backwards(work,m,ipiv);
  }
}
#ifdef ABC_LONG_FACTORIZATION
/// ****** Start long double version
/* type
   0 Left Lower NoTranspose Unit
   1 Left Upper NoTranspose NonUnit
   2 Left Lower Transpose Unit
   3 Left Upper Transpose NonUnit
*/
static void CoinAbcDtrsmFactor(int m, int n, long double * COIN_RESTRICT a,int lda)
{
  assert ((m&(BLOCKING8-1))==0&&(n&(BLOCKING8-1))==0);
  assert (m==BLOCKING8);
  // 0 Left Lower NoTranspose Unit
  /* entry for column j and row i (when multiple of BLOCKING8)
     is at aBlocked+j*m+i*BLOCKING8
  */
  long double * COIN_RESTRICT aBase2 = a;
  long double * COIN_RESTRICT bBase2 = aBase2+lda*BLOCKING8;
  for (int jj=0;jj<n;jj+=BLOCKING8) {
    long double * COIN_RESTRICT bBase = bBase2;
    for (int j=jj;j<jj+BLOCKING8;j++) {
#if 0
      long double * COIN_RESTRICT aBase = aBase2;
      for (int k=0;k<BLOCKING8;k++) {
	long double bValue = bBase[k];
	if (bValue) {
	  for (int i=k+1;i<BLOCKING8;i++) {
	    bBase[i]-=bValue*aBase[i];
	  }
	}
	aBase+=BLOCKING8;
      }
#else
      // unroll - stay in registers - don't test for zeros
      assert (BLOCKING8==8);
      long double bValue0 = bBase[0];
      long double bValue1 = bBase[1];
      long double bValue2 = bBase[2];
      long double bValue3 = bBase[3];
      long double bValue4 = bBase[4];
      long double bValue5 = bBase[5];
      long double bValue6 = bBase[6];
      long double bValue7 = bBase[7];
      bValue1-=bValue0*a[1+0*BLOCKING8];
      bBase[1] = bValue1;
      bValue2-=bValue0*a[2+0*BLOCKING8];
      bValue3-=bValue0*a[3+0*BLOCKING8];
      bValue4-=bValue0*a[4+0*BLOCKING8];
      bValue5-=bValue0*a[5+0*BLOCKING8];
      bValue6-=bValue0*a[6+0*BLOCKING8];
      bValue7-=bValue0*a[7+0*BLOCKING8];
      bValue2-=bValue1*a[2+1*BLOCKING8];
      bBase[2] = bValue2;
      bValue3-=bValue1*a[3+1*BLOCKING8];
      bValue4-=bValue1*a[4+1*BLOCKING8];
      bValue5-=bValue1*a[5+1*BLOCKING8];
      bValue6-=bValue1*a[6+1*BLOCKING8];
      bValue7-=bValue1*a[7+1*BLOCKING8];
      bValue3-=bValue2*a[3+2*BLOCKING8];
      bBase[3] = bValue3;
      bValue4-=bValue2*a[4+2*BLOCKING8];
      bValue5-=bValue2*a[5+2*BLOCKING8];
      bValue6-=bValue2*a[6+2*BLOCKING8];
      bValue7-=bValue2*a[7+2*BLOCKING8];
      bValue4-=bValue3*a[4+3*BLOCKING8];
      bBase[4] = bValue4;
      bValue5-=bValue3*a[5+3*BLOCKING8];
      bValue6-=bValue3*a[6+3*BLOCKING8];
      bValue7-=bValue3*a[7+3*BLOCKING8];
      bValue5-=bValue4*a[5+4*BLOCKING8];
      bBase[5] = bValue5;
      bValue6-=bValue4*a[6+4*BLOCKING8];
      bValue7-=bValue4*a[7+4*BLOCKING8];
      bValue6-=bValue5*a[6+5*BLOCKING8];
      bBase[6] = bValue6;
      bValue7-=bValue5*a[7+5*BLOCKING8];
      bValue7-=bValue6*a[7+6*BLOCKING8];
      bBase[7] = bValue7;
#endif
      bBase += BLOCKING8;
    }
    bBase2 +=lda*BLOCKING8; 
  }
}
#define UNROLL_DTRSM 16
#define CILK_DTRSM 32
static void dtrsm0(int kkk, int first, int last,
		   int m, long double * COIN_RESTRICT a,long double * COIN_RESTRICT b)
{
  int mm=CoinMin(kkk+UNROLL_DTRSM*BLOCKING8,m);
  assert ((last-first)%BLOCKING8==0);
  if (last-first>CILK_DTRSM) {
    int mid=((first+last)>>4)<<3;
    cilk_spawn dtrsm0(kkk,first,mid,m,a,b);
    dtrsm0(kkk,mid,last,m,a,b);
    cilk_sync;
  } else {
    const long double * COIN_RESTRICT aBaseA = a+UNROLL_DTRSM*BLOCKING8X8+kkk*BLOCKING8;
    aBaseA += (first-mm)*BLOCKING8-BLOCKING8X8;
    aBaseA += m*kkk;
    for (int ii=first;ii<last;ii+=BLOCKING8) {
      aBaseA += BLOCKING8X8;
      const long double * COIN_RESTRICT aBaseB=aBaseA;
      long double * COIN_RESTRICT bBaseI = b+ii;
      for (int kk=kkk;kk<mm;kk+=BLOCKING8) {
	long double * COIN_RESTRICT bBase = b+kk;
	const long double * COIN_RESTRICT aBase2 = aBaseB;//a+UNROLL_DTRSM*BLOCKING8X8+m*kk+kkk*BLOCKING8;
	//aBase2 += (ii-mm)*BLOCKING8;
	//assert (aBase2==aBaseB);
	aBaseB+=m*BLOCKING8;
#if AVX2 !=2
#define ALTERNATE_INNER
#ifndef ALTERNATE_INNER
	for (int k=0;k<BLOCKING8;k++) {
	  //coin_prefetch_const(aBase2+BLOCKING8);		
	  for (int i=0;i<BLOCKING8;i++) {
	    bBaseI[i] -= bBase[k]*aBase2[i];
	  }
	  aBase2 += BLOCKING8;
	}
#else
	long double b0=bBaseI[0];
	long double b1=bBaseI[1];
	long double b2=bBaseI[2];
	long double b3=bBaseI[3];
	long double b4=bBaseI[4];
	long double b5=bBaseI[5];
	long double b6=bBaseI[6];
	long double b7=bBaseI[7];
	for (int k=0;k<BLOCKING8;k++) {
	  //coin_prefetch_const(aBase2+BLOCKING8);		
	  long double bValue=bBase[k];
	  b0 -= bValue * aBase2[0];
	  b1 -= bValue * aBase2[1];
	  b2 -= bValue * aBase2[2];
	  b3 -= bValue * aBase2[3];
	  b4 -= bValue * aBase2[4];
	  b5 -= bValue * aBase2[5];
	  b6 -= bValue * aBase2[6];
	  b7 -= bValue * aBase2[7];
	  aBase2 += BLOCKING8;
	}
	bBaseI[0]=b0;
	bBaseI[1]=b1;
	bBaseI[2]=b2;
	bBaseI[3]=b3;
	bBaseI[4]=b4;
	bBaseI[5]=b5;
	bBaseI[6]=b6;
	bBaseI[7]=b7;
#endif
#else
	__m256d b0=_mm256_load_pd(bBaseI);
	__m256d b1=_mm256_load_pd(bBaseI+4);
	for (int k=0;k<BLOCKING8;k++) {
	  //coin_prefetch_const(aBase2+BLOCKING8);		
	  __m256d bb = _mm256_broadcast_sd(bBase+k);
	  __m256d a0 = _mm256_load_pd(aBase2);
	  b0 -= a0*bb;
	  __m256d a1 = _mm256_load_pd(aBase2+4);
	  b1 -= a1*bb;
	  aBase2 += BLOCKING8;
	}
	//_mm256_store_pd ((bBaseI), (b0));
	*reinterpret_cast<__m256d *>(bBaseI)=b0;
	//_mm256_store_pd (bBaseI+4, b1);
	*reinterpret_cast<__m256d *>(bBaseI+4)=b1;
#endif
      }
    }
  }
}
void CoinAbcDtrsm0(int m, long double * COIN_RESTRICT a,long double * COIN_RESTRICT b)
{
  assert ((m&(BLOCKING8-1))==0);
  // 0 Left Lower NoTranspose Unit
  for (int kkk=0;kkk<m;kkk+=UNROLL_DTRSM*BLOCKING8) {
    int mm=CoinMin(m,kkk+UNROLL_DTRSM*BLOCKING8);
    for (int kk=kkk;kk<mm;kk+=BLOCKING8) {
      const long double * COIN_RESTRICT aBase2 = a+kk*(m+BLOCKING8);
      long double * COIN_RESTRICT bBase = b+kk;
      for (int k=0;k<BLOCKING8;k++) {
	for (int i=k+1;i<BLOCKING8;i++) {
	  bBase[i] -= bBase[k]*aBase2[i];
	}
	aBase2 += BLOCKING8;
      }
      for (int ii=kk+BLOCKING8;ii<mm;ii+=BLOCKING8) {
	long double * COIN_RESTRICT bBaseI = b+ii;
	for (int k=0;k<BLOCKING8;k++) {
	  //coin_prefetch_const(aBase2+BLOCKING8);		
	  for (int i=0;i<BLOCKING8;i++) {
	    bBaseI[i] -= bBase[k]*aBase2[i];
	  }
	  aBase2 += BLOCKING8;
	}
      }
    }
    dtrsm0(kkk,mm,m,m,a,b);
  }
}
static void dtrsm1(int kkk, int first, int last,
		   int m, long double * COIN_RESTRICT a,long double * COIN_RESTRICT b)
{
  int mm=CoinMax(0,kkk-(UNROLL_DTRSM-1)*BLOCKING8);
  assert ((last-first)%BLOCKING8==0);
  if (last-first>CILK_DTRSM) {
    int mid=((first+last)>>4)<<3;
    cilk_spawn dtrsm1(kkk,first,mid,m,a,b);
    dtrsm1(kkk,mid,last,m,a,b);
    cilk_sync;
  } else {
    for (int iii=last-BLOCKING8;iii>=first;iii-=BLOCKING8) {
      long double * COIN_RESTRICT bBase2 = b+iii;
      const long double * COIN_RESTRICT aBaseA=
	a+BLOCKING8X8+BLOCKING8*iii;
      for (int ii=kkk;ii>=mm;ii-=BLOCKING8) {
	long double * COIN_RESTRICT bBase = b+ii;
	const long double * COIN_RESTRICT aBase=aBaseA+m*ii;
#if AVX2 !=2
#ifndef ALTERNATE_INNER
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase -= BLOCKING8;
	  //coin_prefetch_const(aBase-BLOCKING8);		
	  for (int k=BLOCKING8-1;k>=0;k--) {
	    bBase2[k] -= bBase[i]*aBase[k];
	  }
	}
#else
	long double b0=bBase2[0];
	long double b1=bBase2[1];
	long double b2=bBase2[2];
	long double b3=bBase2[3];
	long double b4=bBase2[4];
	long double b5=bBase2[5];
	long double b6=bBase2[6];
	long double b7=bBase2[7];
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase -= BLOCKING8;
	  //coin_prefetch_const(aBase-BLOCKING8);		
	  long double bValue=bBase[i];
	  b0 -= bValue * aBase[0];
	  b1 -= bValue * aBase[1];
	  b2 -= bValue * aBase[2];
	  b3 -= bValue * aBase[3];
	  b4 -= bValue * aBase[4];
	  b5 -= bValue * aBase[5];
	  b6 -= bValue * aBase[6];
	  b7 -= bValue * aBase[7];
	}
	bBase2[0]=b0;
	bBase2[1]=b1;
	bBase2[2]=b2;
	bBase2[3]=b3;
	bBase2[4]=b4;
	bBase2[5]=b5;
	bBase2[6]=b6;
	bBase2[7]=b7;
#endif
#else
	__m256d b0=_mm256_load_pd(bBase2);
	__m256d b1=_mm256_load_pd(bBase2+4);
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase -= BLOCKING8;
	  //coin_prefetch_const(aBase5-BLOCKING8);		
	  __m256d bb = _mm256_broadcast_sd(bBase+i);
	  __m256d a0 = _mm256_load_pd(aBase);
	  b0 -= a0*bb;
	  __m256d a1 = _mm256_load_pd(aBase+4);
	  b1 -= a1*bb;
	}
	//_mm256_store_pd (bBase2, b0);
	*reinterpret_cast<__m256d *>(bBase2)=b0;
	//_mm256_store_pd (bBase2+4, b1);
	*reinterpret_cast<__m256d *>(bBase2+4)=b1;
#endif
      }
    }
  }
}
void CoinAbcDtrsm1(int m, long double * COIN_RESTRICT a,long double * COIN_RESTRICT b)
{
  assert ((m&(BLOCKING8-1))==0);
  // 1 Left Upper NoTranspose NonUnit
  for (int kkk=m-BLOCKING8;kkk>=0;kkk-=UNROLL_DTRSM*BLOCKING8) {
    int mm=CoinMax(0,kkk-(UNROLL_DTRSM-1)*BLOCKING8);
    for (int ii=kkk;ii>=mm;ii-=BLOCKING8) {
      const long double * COIN_RESTRICT aBase = a+m*ii+ii*BLOCKING8;
      long double * COIN_RESTRICT bBase = b+ii;
      for (int i=BLOCKING8-1;i>=0;i--) {
	bBase[i] *= aBase[i*(BLOCKING8+1)];
	for (int k=i-1;k>=0;k--) {
	  bBase[k] -= bBase[i]*aBase[k+i*BLOCKING8];
	}
      }
      long double * COIN_RESTRICT bBase2 = bBase;
      for (int iii=ii;iii>mm;iii-=BLOCKING8) {
	bBase2 -= BLOCKING8;
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase -= BLOCKING8;
	  coin_prefetch_const(aBase-BLOCKING8);		
	  for (int k=BLOCKING8-1;k>=0;k--) {
	    bBase2[k] -= bBase[i]*aBase[k];
	  }
	}
      }
    }
    dtrsm1(kkk, 0, mm,  m, a, b);
  }
}
static void dtrsm2(int kkk, int first, int last,
		   int m, long double * COIN_RESTRICT a,long double * COIN_RESTRICT b)
{
  int mm=CoinMax(0,kkk-(UNROLL_DTRSM-1)*BLOCKING8);
  assert ((last-first)%BLOCKING8==0);
  if (last-first>CILK_DTRSM) {
    int mid=((first+last)>>4)<<3;
    cilk_spawn dtrsm2(kkk,first,mid,m,a,b);
    dtrsm2(kkk,mid,last,m,a,b);
    cilk_sync;
  } else {
    for (int iii=last-BLOCKING8;iii>=first;iii-=BLOCKING8) {
      for (int ii=kkk;ii>=mm;ii-=BLOCKING8) {
	const long double * COIN_RESTRICT bBase = b+ii;
	long double * COIN_RESTRICT bBase2=b+iii;
	const long double * COIN_RESTRICT aBase=a+ii*BLOCKING8+m*iii+BLOCKING8X8;
	for (int j=BLOCKING8-1;j>=0;j-=4) {
	  long double bValue0=bBase2[j];
	  long double bValue1=bBase2[j-1];
	  long double bValue2=bBase2[j-2];
	  long double bValue3=bBase2[j-3];
	  bValue0 -= aBase[-1*BLOCKING8+7]*bBase[7];
	  bValue1 -= aBase[-2*BLOCKING8+7]*bBase[7];
	  bValue2 -= aBase[-3*BLOCKING8+7]*bBase[7];
	  bValue3 -= aBase[-4*BLOCKING8+7]*bBase[7];
	  bValue0 -= aBase[-1*BLOCKING8+6]*bBase[6];
	  bValue1 -= aBase[-2*BLOCKING8+6]*bBase[6];
	  bValue2 -= aBase[-3*BLOCKING8+6]*bBase[6];
	  bValue3 -= aBase[-4*BLOCKING8+6]*bBase[6];
	  bValue0 -= aBase[-1*BLOCKING8+5]*bBase[5];
	  bValue1 -= aBase[-2*BLOCKING8+5]*bBase[5];
	  bValue2 -= aBase[-3*BLOCKING8+5]*bBase[5];
	  bValue3 -= aBase[-4*BLOCKING8+5]*bBase[5];
	  bValue0 -= aBase[-1*BLOCKING8+4]*bBase[4];
	  bValue1 -= aBase[-2*BLOCKING8+4]*bBase[4];
	  bValue2 -= aBase[-3*BLOCKING8+4]*bBase[4];
	  bValue3 -= aBase[-4*BLOCKING8+4]*bBase[4];
	  bValue0 -= aBase[-1*BLOCKING8+3]*bBase[3];
	  bValue1 -= aBase[-2*BLOCKING8+3]*bBase[3];
	  bValue2 -= aBase[-3*BLOCKING8+3]*bBase[3];
	  bValue3 -= aBase[-4*BLOCKING8+3]*bBase[3];
	  bValue0 -= aBase[-1*BLOCKING8+2]*bBase[2];
	  bValue1 -= aBase[-2*BLOCKING8+2]*bBase[2];
	  bValue2 -= aBase[-3*BLOCKING8+2]*bBase[2];
	  bValue3 -= aBase[-4*BLOCKING8+2]*bBase[2];
	  bValue0 -= aBase[-1*BLOCKING8+1]*bBase[1];
	  bValue1 -= aBase[-2*BLOCKING8+1]*bBase[1];
	  bValue2 -= aBase[-3*BLOCKING8+1]*bBase[1];
	  bValue3 -= aBase[-4*BLOCKING8+1]*bBase[1];
	  bValue0 -= aBase[-1*BLOCKING8+0]*bBase[0];
	  bValue1 -= aBase[-2*BLOCKING8+0]*bBase[0];
	  bValue2 -= aBase[-3*BLOCKING8+0]*bBase[0];
	  bValue3 -= aBase[-4*BLOCKING8+0]*bBase[0];
	  bBase2[j]=bValue0;
	  bBase2[j-1]=bValue1;
	  bBase2[j-2]=bValue2;
	  bBase2[j-3]=bValue3;
	  aBase -= 4*BLOCKING8;
	}
      }
    }
  }
}
void CoinAbcDtrsm2(int m, long double * COIN_RESTRICT a,long double * COIN_RESTRICT b)
{
  assert ((m&(BLOCKING8-1))==0);
  // 2 Left Lower Transpose Unit
  for (int kkk=m-BLOCKING8;kkk>=0;kkk-=UNROLL_DTRSM*BLOCKING8) {
    int mm=CoinMax(0,kkk-(UNROLL_DTRSM-1)*BLOCKING8);
    for (int ii=kkk;ii>=mm;ii-=BLOCKING8) {
      long double * COIN_RESTRICT bBase = b+ii;
      long double * COIN_RESTRICT bBase2 = bBase;
      const long double * COIN_RESTRICT aBase=a+m*ii+(ii+BLOCKING8)*BLOCKING8;
      for (int i=BLOCKING8-1;i>=0;i--) {
	aBase -= BLOCKING8;
	for (int k=i+1;k<BLOCKING8;k++) {
	  bBase2[i] -= aBase[k]*bBase2[k];
	}
      }
      for (int iii=ii-BLOCKING8;iii>=mm;iii-=BLOCKING8) {
	bBase2 -= BLOCKING8;
	assert (bBase2==b+iii);
	aBase -= m*BLOCKING8;
	const long double * COIN_RESTRICT aBase2 = aBase+BLOCKING8X8;
	for (int i=BLOCKING8-1;i>=0;i--) {
	  aBase2 -= BLOCKING8;
	  for (int k=BLOCKING8-1;k>=0;k--) {
	    bBase2[i] -= aBase2[k]*bBase[k];
	  }
	}
      }
    }
    dtrsm2(kkk, 0, mm,  m, a, b);
  }
}
#define UNROLL_DTRSM3 16
#define CILK_DTRSM3 32
static void dtrsm3(int kkk, int first, int last,
		   int m, long double * COIN_RESTRICT a,long double * COIN_RESTRICT b)
{
  //int mm=CoinMin(kkk+UNROLL_DTRSM3*BLOCKING8,m);
  assert ((last-first)%BLOCKING8==0);
  if (last-first>CILK_DTRSM3) {
    int mid=((first+last)>>4)<<3;
    cilk_spawn dtrsm3(kkk,first,mid,m,a,b);
    dtrsm3(kkk,mid,last,m,a,b);
    cilk_sync;
  } else {
    for (int kk=0;kk<kkk;kk+=BLOCKING8) {
      for (int ii=first;ii<last;ii+=BLOCKING8) {
	long double * COIN_RESTRICT bBase = b+ii;
	long double * COIN_RESTRICT bBase2 = b+kk;
	const long double * COIN_RESTRICT aBase=a+ii*m+kk*BLOCKING8;
	for (int j=0;j<BLOCKING8;j+=4) {
	  long double bValue0=bBase[j];
	  long double bValue1=bBase[j+1];
	  long double bValue2=bBase[j+2];
	  long double bValue3=bBase[j+3];
	  bValue0 -= aBase[0]*bBase2[0];
	  bValue1 -= aBase[1*BLOCKING8+0]*bBase2[0];
	  bValue2 -= aBase[2*BLOCKING8+0]*bBase2[0];
	  bValue3 -= aBase[3*BLOCKING8+0]*bBase2[0];
	  bValue0 -= aBase[1]*bBase2[1];
	  bValue1 -= aBase[1*BLOCKING8+1]*bBase2[1];
	  bValue2 -= aBase[2*BLOCKING8+1]*bBase2[1];
	  bValue3 -= aBase[3*BLOCKING8+1]*bBase2[1];
	  bValue0 -= aBase[2]*bBase2[2];
	  bValue1 -= aBase[1*BLOCKING8+2]*bBase2[2];
	  bValue2 -= aBase[2*BLOCKING8+2]*bBase2[2];
	  bValue3 -= aBase[3*BLOCKING8+2]*bBase2[2];
	  bValue0 -= aBase[3]*bBase2[3];
	  bValue1 -= aBase[1*BLOCKING8+3]*bBase2[3];
	  bValue2 -= aBase[2*BLOCKING8+3]*bBase2[3];
	  bValue3 -= aBase[3*BLOCKING8+3]*bBase2[3];
	  bValue0 -= aBase[4]*bBase2[4];
	  bValue1 -= aBase[1*BLOCKING8+4]*bBase2[4];
	  bValue2 -= aBase[2*BLOCKING8+4]*bBase2[4];
	  bValue3 -= aBase[3*BLOCKING8+4]*bBase2[4];
	  bValue0 -= aBase[5]*bBase2[5];
	  bValue1 -= aBase[1*BLOCKING8+5]*bBase2[5];
	  bValue2 -= aBase[2*BLOCKING8+5]*bBase2[5];
	  bValue3 -= aBase[3*BLOCKING8+5]*bBase2[5];
	  bValue0 -= aBase[6]*bBase2[6];
	  bValue1 -= aBase[1*BLOCKING8+6]*bBase2[6];
	  bValue2 -= aBase[2*BLOCKING8+7]*bBase2[7];
	  bValue3 -= aBase[3*BLOCKING8+6]*bBase2[6];
	  bValue0 -= aBase[7]*bBase2[7];
	  bValue1 -= aBase[1*BLOCKING8+7]*bBase2[7];
	  bValue2 -= aBase[2*BLOCKING8+6]*bBase2[6];
	  bValue3 -= aBase[3*BLOCKING8+7]*bBase2[7];
	  bBase[j]=bValue0;
	  bBase[j+1]=bValue1;
	  bBase[j+2]=bValue2;
	  bBase[j+3]=bValue3;
	  aBase += 4*BLOCKING8;
	}
      }
    }
  }
}
void CoinAbcDtrsm3(int m, long double * COIN_RESTRICT a,long double * COIN_RESTRICT b)
{
  assert ((m&(BLOCKING8-1))==0);
  // 3 Left Upper Transpose NonUnit
  for (int kkk=0;kkk<m;kkk+=UNROLL_DTRSM3*BLOCKING8) {
    int mm=CoinMin(m,kkk+UNROLL_DTRSM3*BLOCKING8);
    dtrsm3(kkk, kkk, mm,  m, a, b);
    for (int ii=kkk;ii<mm;ii+=BLOCKING8) {
      long double * COIN_RESTRICT bBase = b+ii;
      for (int kk=kkk;kk<ii;kk+=BLOCKING8) {
	long double * COIN_RESTRICT bBase2 = b+kk;
	const long double * COIN_RESTRICT aBase=a+ii*m+kk*BLOCKING8;
	for (int i=0;i<BLOCKING8;i++) {
	  for (int k=0;k<BLOCKING8;k++) {
	    bBase[i] -= aBase[k]*bBase2[k];
	  }
	  aBase += BLOCKING8;
	}
      }
      const long double * COIN_RESTRICT aBase=a+ii*m+ii*BLOCKING8;
      for (int i=0;i<BLOCKING8;i++) {
	for (int k=0;k<i;k++) {
	  bBase[i] -= aBase[k]*bBase[k];
	}
	bBase[i] = bBase[i]*aBase[i];
	aBase += BLOCKING8;
      }
    }
  }
}
static void 
CoinAbcDlaswp(int n, long double * COIN_RESTRICT a,int lda,int start,int end, int * ipiv)
{
  assert ((n&(BLOCKING8-1))==0);
  long double * COIN_RESTRICT aThis3 = a;
  for (int j=0;j<n;j+=BLOCKING8) {
    for (int i=start;i<end;i++) {
      int ip=ipiv[i];
      if (ip!=i) {
	long double * COIN_RESTRICT aTop=aThis3+j*lda;
	int iBias = ip/BLOCKING8;
	ip-=iBias*BLOCKING8;
	long double * COIN_RESTRICT aNotTop=aTop+iBias*BLOCKING8X8;
	iBias = i/BLOCKING8;
	int i2=i-iBias*BLOCKING8;
	aTop += iBias*BLOCKING8X8;
	for (int k=0;k<BLOCKING8;k++) {
	  long double temp=aTop[i2];
	  aTop[i2]=aNotTop[ip];
	  aNotTop[ip]=temp;
	  aTop += BLOCKING8;
	  aNotTop += BLOCKING8;
	}
      }
    }
  }
}
extern void CoinAbcDgemm(int m, int n, int k, long double * COIN_RESTRICT a,int lda,
			  long double * COIN_RESTRICT b,long double * COIN_RESTRICT c
#if ABC_PARALLEL==2
			  ,int parallelMode
#endif
			 );
static int CoinAbcDgetrf2(int m, int n, long double * COIN_RESTRICT a, int * ipiv)
{
  assert ((m&(BLOCKING8-1))==0&&(n&(BLOCKING8-1))==0);
  assert (n==BLOCKING8);
  int dimension=CoinMin(m,n);
  /* entry for column j and row i (when multiple of BLOCKING8)
     is at aBlocked+j*m+i*BLOCKING8
  */
  assert (dimension==BLOCKING8);
  //printf("Dgetrf2 m=%d n=%d\n",m,n);
  long double * COIN_RESTRICT aThis3 = a;
  for (int j=0;j<dimension;j++) {
    int pivotRow=-1;
    long double largest=0.0;
    long double * COIN_RESTRICT aThis2 = aThis3+j*BLOCKING8;
    // this block
    int pivotRow2=-1;
    for (int i=j;i<BLOCKING8;i++) {
      long double value=fabsl(aThis2[i]);
      if (value>largest) {
	largest=value;
	pivotRow2=i;
      }
    }
    // other blocks
    int iBias=0;
    for (int ii=BLOCKING8;ii<m;ii+=BLOCKING8) {
      aThis2+=BLOCKING8*BLOCKING8;
      for (int i=0;i<BLOCKING8;i++) {
	long double value=fabsl(aThis2[i]);
	if (value>largest) {
	  largest=value;
	  pivotRow2=i;
	  iBias=ii;
	}
      }
    }
    pivotRow=pivotRow2+iBias;
    ipiv[j]=pivotRow;
    if (largest) {
      if (j!=pivotRow) {
	long double * COIN_RESTRICT aTop=aThis3;
	long double * COIN_RESTRICT aNotTop=aThis3+iBias*BLOCKING8;
	for (int i=0;i<n;i++) {
	  long double value = aTop[j];
	  aTop[j]=aNotTop[pivotRow2];
	  aNotTop[pivotRow2]=value;
	  aTop += BLOCKING8;
	  aNotTop += BLOCKING8;
	}
      }
      aThis2 = aThis3+j*BLOCKING8;
      long double pivotMultiplier = 1.0 / aThis2[j];
      aThis2[j] = pivotMultiplier;
      // Do L
      // this block
      for (int i=j+1;i<BLOCKING8;i++) 
	aThis2[i] *= pivotMultiplier;
      // other blocks
      for (int ii=BLOCKING8;ii<m;ii+=BLOCKING8) {
	aThis2+=BLOCKING8*BLOCKING8;
	for (int i=0;i<BLOCKING8;i++) 
	  aThis2[i] *= pivotMultiplier;
      }
      // update rest
      long double * COIN_RESTRICT aPut;
      aThis2 = aThis3+j*BLOCKING8;
      aPut = aThis2+BLOCKING8;
      long double * COIN_RESTRICT aPut2 = aPut;
      // this block
      for (int i=j+1;i<BLOCKING8;i++) {
	long double value = aPut[j];
	for (int k=j+1;k<BLOCKING8;k++) 
	  aPut[k] -= value*aThis2[k];
	aPut += BLOCKING8;
      }
      // other blocks
      for (int ii=BLOCKING8;ii<m;ii+=BLOCKING8) {
	aThis2 += BLOCKING8*BLOCKING8;
	aPut = aThis2+BLOCKING8;
	long double * COIN_RESTRICT aPut2a = aPut2;
	for (int i=j+1;i<BLOCKING8;i++) {
	  long double value = aPut2a[j];
	  for (int k=0;k<BLOCKING8;k++) 
	    aPut[k] -= value*aThis2[k];
	  aPut += BLOCKING8;
	  aPut2a += BLOCKING8;
	}
      }
    } else {
      return 1;
    }
  }
  return 0;
}

int 
CoinAbcDgetrf(int m, int n, long double * COIN_RESTRICT a, int lda, int * ipiv
#if ABC_PARALLEL==2
			  ,int parallelMode
#endif
)
{
  assert (m==n);
  assert ((m&(BLOCKING8-1))==0&&(n&(BLOCKING8-1))==0);
  if (m<BLOCKING8) {
    return CoinAbcDgetrf2(m,n,a,ipiv);
  } else {
    for (int j=0;j<n;j+=BLOCKING8) {
      int start=j;
      int newSize=CoinMin(BLOCKING8,n-j);
      int end=j+newSize;
      int returnCode=CoinAbcDgetrf2(m-start,newSize,a+(start*lda+start*BLOCKING8),
				     ipiv+start);
      if (!returnCode) {
	// adjust
	for (int k=start;k<end;k++)
	  ipiv[k]+=start;
	// swap 0<start
	CoinAbcDlaswp(start,a,lda,start,end,ipiv);
	if (end<n) {
	  // swap >=end
	  CoinAbcDlaswp(n-end,a+end*lda,lda,start,end,ipiv);
          CoinAbcDtrsmFactor(newSize,n-end,a+(start*lda+start*BLOCKING8),lda);
          CoinAbcDgemm(n-end,n-end,newSize,
			a+start*lda+end*BLOCKING8,lda,
			a+end*lda+start*BLOCKING8,a+end*lda+end*BLOCKING8
#if ABC_PARALLEL==2
			  ,parallelMode
#endif
			);
	}
      } else {
	return returnCode;
      }
    }
  }
  return 0;
}
void
CoinAbcDgetrs(char trans,int m, long double * COIN_RESTRICT a, long double * COIN_RESTRICT work)
{
  assert ((m&(BLOCKING8-1))==0);
  if (trans=='N') {
    //CoinAbcDlaswp1(work,m,ipiv);
    CoinAbcDtrsm0(m,a,work);
    CoinAbcDtrsm1(m,a,work);
  } else {
    assert (trans=='T');
    CoinAbcDtrsm3(m,a,work);
    CoinAbcDtrsm2(m,a,work);
    //CoinAbcDlaswp1Backwards(work,m,ipiv);
  }
}
#endif
