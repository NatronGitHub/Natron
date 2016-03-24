/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

// This file has higher level solve functions

#include "CoinPragma.hpp"
#include "ClpConfig.h"

// check already here if COIN_HAS_GLPK is defined, since we do not want to get confused by a COIN_HAS_GLPK in config_coinutils.h
#if defined(COIN_HAS_AMD) || defined(COIN_HAS_CHOLMOD) || defined(COIN_HAS_GLPK)
#define UFL_BARRIER
#endif

#include <math.h>

#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "ClpFactorization.hpp"
#include "ClpSimplex.hpp"
#include "ClpSimplexOther.hpp"
#include "ClpSimplexDual.hpp"
#ifndef SLIM_CLP
#include "ClpQuadraticObjective.hpp"
#include "ClpInterior.hpp"
#include "ClpCholeskyDense.hpp"
#include "ClpCholeskyBase.hpp"
#include "ClpPlusMinusOneMatrix.hpp"
#include "ClpNetworkMatrix.hpp"
#endif
#include "ClpEventHandler.hpp"
#include "ClpLinearObjective.hpp"
#include "ClpSolve.hpp"
#include "ClpPackedMatrix.hpp"
#include "ClpMessage.hpp"
#include "CoinTime.hpp"
#if CLP_HAS_ABC
#include "CoinAbcCommon.hpp"
#endif
#ifdef ABC_INHERIT
#include "AbcSimplex.hpp"
#include "AbcSimplexFactorization.hpp"
#endif
double zz_slack_value=0.0;

#include "ClpPresolve.hpp"
#ifndef SLIM_CLP
#include "Idiot.hpp"
#ifdef COIN_HAS_WSMP
#include "ClpCholeskyWssmp.hpp"
#include "ClpCholeskyWssmpKKT.hpp"
#endif
#include "ClpCholeskyUfl.hpp"
#ifdef TAUCS_BARRIER
#include "ClpCholeskyTaucs.hpp"
#endif
#include "ClpCholeskyMumps.hpp"
#ifdef COIN_HAS_VOL
#include "VolVolume.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinMpsIO.hpp"
//#############################################################################

class lpHook : public VOL_user_hooks {
private:
     lpHook(const lpHook&);
     lpHook& operator=(const lpHook&);
private:
     /// Pointer to dense vector of structural variable upper bounds
     double  *colupper_;
     /// Pointer to dense vector of structural variable lower bounds
     double  *collower_;
     /// Pointer to dense vector of objective coefficients
     double  *objcoeffs_;
     /// Pointer to dense vector of right hand sides
     double  *rhs_;
     /// Pointer to dense vector of senses
     char    *sense_;

     /// The problem matrix in a row ordered form
     CoinPackedMatrix rowMatrix_;
     /// The problem matrix in a column ordered form
     CoinPackedMatrix colMatrix_;

public:
     lpHook(double* clb, double* cub, double* obj,
            double* rhs, char* sense, const CoinPackedMatrix& mat);
     virtual ~lpHook();

public:
     // for all hooks: return value of -1 means that volume should quit
     /** compute reduced costs
         @param u (IN) the dual variables
         @param rc (OUT) the reduced cost with respect to the dual values
     */
     virtual int compute_rc(const VOL_dvector& u, VOL_dvector& rc);

     /** Solve the subproblem for the subgradient step.
         @param dual (IN) the dual variables
         @param rc (IN) the reduced cost with respect to the dual values
         @param lcost (OUT) the lagrangean cost with respect to the dual values
         @param x (OUT) the primal result of solving the subproblem
         @param v (OUT) b-Ax for the relaxed constraints
         @param pcost (OUT) the primal objective value of <code>x</code>
     */
     virtual int solve_subproblem(const VOL_dvector& dual, const VOL_dvector& rc,
                                  double& lcost, VOL_dvector& x, VOL_dvector& v,
                                  double& pcost);
     /** Starting from the primal vector x, run a heuristic to produce
         an integer solution
         @param x (IN) the primal vector
         @param heur_val (OUT) the value of the integer solution (return
         <code>DBL_MAX</code> here if no feas sol was found
     */
     virtual int heuristics(const VOL_problem& p,
                            const VOL_dvector& x, double& heur_val) {
          return 0;
     }
};

//#############################################################################

lpHook::lpHook(double* clb, double* cub, double* obj,
               double* rhs, char* sense,
               const CoinPackedMatrix& mat)
{
     colupper_ = cub;
     collower_ = clb;
     objcoeffs_ = obj;
     rhs_ = rhs;
     sense_ = sense;
     assert (mat.isColOrdered());
     colMatrix_.copyOf(mat);
     rowMatrix_.reverseOrderedCopyOf(mat);
}

//-----------------------------------------------------------------------------

lpHook::~lpHook()
{
}

//#############################################################################

int
lpHook::compute_rc(const VOL_dvector& u, VOL_dvector& rc)
{
     rowMatrix_.transposeTimes(u.v, rc.v);
     const int psize = rowMatrix_.getNumCols();

     for (int i = 0; i < psize; ++i)
          rc[i] = objcoeffs_[i] - rc[i];
     return 0;
}

//-----------------------------------------------------------------------------

int
lpHook::solve_subproblem(const VOL_dvector& dual, const VOL_dvector& rc,
                         double& lcost, VOL_dvector& x, VOL_dvector& v,
                         double& pcost)
{
     int i;
     const int psize = x.size();
     const int dsize = v.size();

     // compute the lagrangean solution corresponding to the reduced costs
     for (i = 0; i < psize; ++i)
          x[i] = (rc[i] >= 0.0) ? collower_[i] : colupper_[i];

     // compute the lagrangean value (rhs*dual + primal*rc)
     lcost = 0;
     for (i = 0; i < dsize; ++i)
          lcost += rhs_[i] * dual[i];
     for (i = 0; i < psize; ++i)
          lcost += x[i] * rc[i];

     // compute the rhs - lhs
     colMatrix_.times(x.v, v.v);
     for (i = 0; i < dsize; ++i)
          v[i] = rhs_[i] - v[i];

     // compute the lagrangean primal objective
     pcost = 0;
     for (i = 0; i < psize; ++i)
          pcost += x[i] * objcoeffs_[i];

     return 0;
}

//#############################################################################
/** A quick inlined function to convert from lb/ub style constraint
    definition to sense/rhs/range style */
inline void
convertBoundToSense(const double lower, const double upper,
                    char& sense, double& right,
                    double& range)
{
     range = 0.0;
     if (lower > -1.0e20) {
          if (upper < 1.0e20) {
               right = upper;
               if (upper == lower) {
                    sense = 'E';
               } else {
                    sense = 'R';
                    range = upper - lower;
               }
          } else {
               sense = 'G';
               right = lower;
          }
     } else {
          if (upper < 1.0e20) {
               sense = 'L';
               right = upper;
          } else {
               sense = 'N';
               right = 0.0;
          }
     }
}

static int
solveWithVolume(ClpSimplex * model, int numberPasses, int doIdiot)
{
     VOL_problem volprob;
     volprob.parm.gap_rel_precision = 0.00001;
     volprob.parm.maxsgriters = 3000;
     if(numberPasses > 3000) {
          volprob.parm.maxsgriters = numberPasses;
          volprob.parm.primal_abs_precision = 0.0;
          volprob.parm.minimum_rel_ascent = 0.00001;
     } else if (doIdiot > 0) {
          volprob.parm.maxsgriters = doIdiot;
     }
     if (model->logLevel() < 2)
          volprob.parm.printflag = 0;
     else
          volprob.parm.printflag = 3;
     const CoinPackedMatrix* mat = model->matrix();
     int psize = model->numberColumns();
     int dsize = model->numberRows();
     char * sense = new char[dsize];
     double * rhs = new double [dsize];

     // Set the lb/ub on the duals
     volprob.dsize = dsize;
     volprob.psize = psize;
     volprob.dual_lb.allocate(dsize);
     volprob.dual_ub.allocate(dsize);
     int i;
     const double * rowLower = model->rowLower();
     const double * rowUpper = model->rowUpper();
     for (i = 0; i < dsize; ++i) {
          double range;
          convertBoundToSense(rowLower[i], rowUpper[i],
                              sense[i], rhs[i], range);
          switch (sense[i]) {
          case 'E':
               volprob.dual_lb[i] = -1.0e31;
               volprob.dual_ub[i] = 1.0e31;
               break;
          case 'L':
               volprob.dual_lb[i] = -1.0e31;
               volprob.dual_ub[i] = 0.0;
               break;
          case 'G':
               volprob.dual_lb[i] = 0.0;
               volprob.dual_ub[i] = 1.0e31;
               break;
          default:
               printf("Volume Algorithm can't work if there is a non ELG row\n");
               return 1;
          }
     }
     // Check bounds
     double * saveLower = model->columnLower();
     double * saveUpper = model->columnUpper();
     bool good = true;
     for (i = 0; i < psize; i++) {
          if (saveLower[i] < -1.0e20 || saveUpper[i] > 1.0e20) {
               good = false;
               break;
          }
     }
     if (!good) {
          saveLower = CoinCopyOfArray(model->columnLower(), psize);
          saveUpper = CoinCopyOfArray(model->columnUpper(), psize);
          for (i = 0; i < psize; i++) {
               if (saveLower[i] < -1.0e20)
                    saveLower[i] = -1.0e20;
               if(saveUpper[i] > 1.0e20)
                    saveUpper[i] = 1.0e20;
          }
     }
     lpHook myHook(saveLower, saveUpper,
                   model->objective(),
                   rhs, sense, *mat);

     volprob.solve(myHook, false /* no warmstart */);

     if (saveLower != model->columnLower()) {
          delete [] saveLower;
          delete [] saveUpper;
     }
     //------------- extract the solution ---------------------------

     //printf("Best lagrangean value: %f\n", volprob.value);

     double avg = 0;
     for (i = 0; i < dsize; ++i) {
          switch (sense[i]) {
          case 'E':
               avg += CoinAbs(volprob.viol[i]);
               break;
          case 'L':
               if (volprob.viol[i] < 0)
                    avg +=  (-volprob.viol[i]);
               break;
          case 'G':
               if (volprob.viol[i] > 0)
                    avg +=  volprob.viol[i];
               break;
          }
     }

     //printf("Average primal constraint violation: %f\n", avg/dsize);

     // volprob.dsol contains the dual solution (dual feasible)
     // volprob.psol contains the primal solution
     //              (NOT necessarily primal feasible)
     CoinMemcpyN(volprob.dsol.v, dsize, model->dualRowSolution());
     CoinMemcpyN(volprob.psol.v, psize, model->primalColumnSolution());
     return 0;
}
#endif
static ClpInterior * currentModel2 = NULL;
#endif
//#############################################################################
// Allow for interrupts
// But is this threadsafe ? (so switched off by option)

#include "CoinSignal.hpp"
static ClpSimplex * currentModel = NULL;
#ifdef ABC_INHERIT
static AbcSimplex * currentAbcModel = NULL;
#endif

extern "C" {
     static void
#if defined(_MSC_VER)
     __cdecl
#endif // _MSC_VER
     signal_handler(int /*whichSignal*/)
     {
          if (currentModel != NULL)
               currentModel->setMaximumIterations(0); // stop at next iterations
#ifdef ABC_INHERIT
          if (currentAbcModel != NULL)
               currentAbcModel->setMaximumIterations(0); // stop at next iterations
#endif
#ifndef SLIM_CLP
          if (currentModel2 != NULL)
               currentModel2->setMaximumBarrierIterations(0); // stop at next iterations
#endif
          return;
     }
}
#if ABC_INSTRUMENT>1
int abcPricing[20];
int abcPricingDense[20];
static int trueNumberRows;
static int numberTypes;
#define MAX_TYPES 25
#define MAX_COUNT 20
#define MAX_FRACTION 101
static char * types[MAX_TYPES];
static double counts[MAX_TYPES][MAX_COUNT];
static double countsFraction[MAX_TYPES][MAX_FRACTION];
static double * currentCounts;
static double * currentCountsFraction;
static int currentType;
static double workMultiplier[MAX_TYPES];
static double work[MAX_TYPES];
static double currentWork;
static double otherWork[MAX_TYPES];
static int timesCalled[MAX_TYPES];
static int timesStarted[MAX_TYPES];
static int fractionDivider;
void instrument_initialize(int numberRows)
{
  trueNumberRows=numberRows;
  numberTypes=0;
  memset(counts,0,sizeof(counts));
  currentCounts=NULL;
  memset(countsFraction,0,sizeof(countsFraction));
  currentCountsFraction=NULL;
  memset(workMultiplier,0,sizeof(workMultiplier));
  memset(work,0,sizeof(work));
  memset(otherWork,0,sizeof(otherWork));
  memset(timesCalled,0,sizeof(timesCalled));
  memset(timesStarted,0,sizeof(timesStarted));
  currentType=-1;
  fractionDivider=(numberRows+MAX_FRACTION-2)/(MAX_FRACTION-1);
}
void instrument_start(const char * type,int numberRowsEtc)
{
  if (currentType>=0)
    instrument_end();
  currentType=-1;
  currentWork=0.0;
  for (int i=0;i<numberTypes;i++) {
    if (!strcmp(types[i],type)) {
      currentType=i;
      break;
    }
  }
  if (currentType==-1) {
    assert (numberTypes<MAX_TYPES);
    currentType=numberTypes;
    types[numberTypes++]=strdup(type);
  }
  currentCounts = &counts[currentType][0];
  currentCountsFraction = &countsFraction[currentType][0];
  timesStarted[currentType]++;
  assert (trueNumberRows);
  workMultiplier[currentType]+=static_cast<double>(numberRowsEtc)/static_cast<double>(trueNumberRows);
}
void instrument_add(int count)
{
  assert (currentType>=0);
  currentWork+=count;
  timesCalled[currentType]++;
  if (count<MAX_COUNT-1)
    currentCounts[count]++;
  else
    currentCounts[MAX_COUNT-1]++;
  assert(count/fractionDivider>=0&&count/fractionDivider<MAX_FRACTION);
  currentCountsFraction[count/fractionDivider]++;
}
void instrument_do(const char * type,double count)
{
  int iType=-1;
  for (int i=0;i<numberTypes;i++) {
    if (!strcmp(types[i],type)) {
      iType=i;
      break;
    }
  }
  if (iType==-1) {
    assert (numberTypes<MAX_TYPES);
    iType=numberTypes;
    types[numberTypes++]=strdup(type);
  }
  timesStarted[iType]++;
  otherWork[iType]+=count;
}
void instrument_end()
{
  work[currentType]+=currentWork;
  currentType=-1;
}
void instrument_end_and_adjust(double factor)
{
  work[currentType]+=currentWork*factor;
  currentType=-1;
}
void instrument_print()
{
  for (int iType=0;iType<numberTypes;iType++) {
    currentCounts = &counts[iType][0];
    currentCountsFraction = &countsFraction[iType][0];
    if (!otherWork[iType]) {
      printf("%s started %d times, used %d times, work %g (average length %.1f) multiplier %g\n",
	     types[iType],timesStarted[iType],timesCalled[iType],
	     work[iType],work[iType]/(timesCalled[iType]+1.0e-100),workMultiplier[iType]/(timesStarted[iType]+1.0e-100));
      int n=0;
      for (int i=0;i<MAX_COUNT-1;i++) {
	if (currentCounts[i]) {
	  if (n==5) {
	    n=0;
	    printf("\n");
	  }
	  n++;
	  printf("(%d els,%.0f times) ",i,currentCounts[i]);
	}
      }
      if (currentCounts[MAX_COUNT-1]) {
	if (n==5) {
	  n=0;
	  printf("\n");
	}
	n++;
	printf("(>=%d els,%.0f times) ",MAX_COUNT-1,currentCounts[MAX_COUNT-1]);
      }
      printf("\n");
      int largestFraction;
      int nBig=0;
      for (largestFraction=MAX_FRACTION-1;largestFraction>=10;largestFraction--) {
	double count = currentCountsFraction[largestFraction];
	if (count&&largestFraction>10)
	  nBig++;
	if (nBig>4)
	  break;
      }
      int chunk=(largestFraction+5)/10;
      int lo=0;
      for (int iChunk=0;iChunk<largestFraction;iChunk+=chunk) {
	int hi=CoinMin(lo+chunk*fractionDivider,trueNumberRows);
	double sum=0.0;
	for (int i=iChunk;i<CoinMin(iChunk+chunk,MAX_FRACTION);i++)
	  sum += currentCountsFraction[i];
	if (sum)
	  printf("(%d-%d %.0f) ",lo,hi,sum);
	lo=hi;
      }
      for (int i=lo/fractionDivider;i<MAX_FRACTION;i++) {
	if (currentCountsFraction[i])
	  printf("(%d %.0f) ",i*fractionDivider,currentCountsFraction[i]);
      }
      printf("\n");
    } else {
      printf("%s started %d times, used %d times, work %g multiplier %g other work %g\n",
	     types[iType],timesStarted[iType],timesCalled[iType],
	     work[iType],workMultiplier[iType],otherWork[iType]);
    }
    free(types[iType]);
  }
}
#endif
#if ABC_PARALLEL==2
#ifndef FAKE_CILK
int number_cilk_workers=0;
#include <cilk/cilk_api.h>
#endif
#endif
#ifdef ABC_INHERIT
void 
ClpSimplex::dealWithAbc(int solveType, int startUp,
			bool interrupt)
{
  if (!this->abcState()||!numberRows_||!numberColumns_) {
    if (!solveType)
      this->dual(0);
    else
      this->primal(startUp ? 1 : 0);
  } else {
    AbcSimplex * abcModel2=new AbcSimplex(*this);
    if (interrupt)
      currentAbcModel = abcModel2;
    //if (abcSimplex_) {
    // move factorization stuff
    abcModel2->factorization()->synchronize(this->factorization(),abcModel2);
    //}
    //abcModel2->startPermanentArrays();
    int crashState=abcModel2->abcState()&(256+512+1024);
    abcModel2->setAbcState(CLP_ABC_WANTED|crashState);
    int ifValuesPass=startUp ? 1 : 0;
    // temp
    if (fabs(this->primalTolerance()-1.001e-6)<0.999e-9) {
      int type=1;
      double diff=this->primalTolerance()-1.001e-6;
      printf("Diff %g\n",diff);
      if (fabs(diff-1.0e-10)<1.0e-13)
	type=2;
      else if (fabs(diff-1.0e-11)<1.0e-13)
	type=3;
#if 0
      ClpSimplex * thisModel = static_cast<ClpSimplexOther *> (this)->dualOfModel(1.0,1.0);
      if (thisModel) {
	printf("Dual of model has %d rows and %d columns\n",
	       thisModel->numberRows(), thisModel->numberColumns());
	thisModel->setOptimizationDirection(1.0);
	Idiot info(*thisModel);
	info.setStrategy(512 | info.getStrategy());
	// Allow for scaling
	info.setStrategy(32 | info.getStrategy());
	info.setStartingWeight(1.0e3);
	info.setReduceIterations(6);
	info.crash(50, this->messageHandler(), this->messagesPointer(),false);
	// make sure later that primal solution in correct place
	// and has correct sign
	abcModel2->setupDualValuesPass(thisModel->primalColumnSolution(),
				       thisModel->dualRowSolution(),type);
	//thisModel->dual();
	delete thisModel;
      }
#else
      if (!solveType) {
	this->dual(0);
	abcModel2->setupDualValuesPass(this->dualRowSolution(),
				       this->primalColumnSolution(),type);
      } else {
	ifValuesPass=1;
	abcModel2->setStateOfProblem(abcModel2->stateOfProblem() | VALUES_PASS);
	Idiot info(*abcModel2);
	info.setStrategy(512 | info.getStrategy());
	// Allow for scaling
	info.setStrategy(32 | info.getStrategy());
	info.setStartingWeight(1.0e3);
	info.setReduceIterations(6);
	info.crash(200, abcModel2->messageHandler(), abcModel2->messagesPointer(),false);
	//memcpy(abcModel2->primalColumnSolution(),this->primalColumnSolution(),
	//     this->numberColumns()*sizeof(double));
      }
#endif
    }
    char line[200];
#if ABC_PARALLEL
#if ABC_PARALLEL==2
#ifndef FAKE_CILK
    if (!number_cilk_workers) {
      number_cilk_workers=__cilkrts_get_nworkers();
      sprintf(line,"%d cilk workers",number_cilk_workers);
      handler_->message(CLP_GENERAL, messages_)
	<< line
	<< CoinMessageEol;
    }
#endif
#endif
    int numberCpu=this->abcState()&15;
    if (numberCpu==9) {
      numberCpu=1;
#if ABC_PARALLEL==2
#ifndef FAKE_CILK
      if (number_cilk_workers>1)
      numberCpu=CoinMin(2*number_cilk_workers,8);
#endif
#endif
    } else if (numberCpu==10) {
      // maximum
      numberCpu=4;
    } else if (numberCpu==10) {
      // decide
      if (abcModel2->getNumElements()<5000)
	numberCpu=1;
#if ABC_PARALLEL==2
#ifndef FAKE_CILK
      else if (number_cilk_workers>1)
	numberCpu=CoinMin(2*number_cilk_workers,8);
#endif
#endif
      else
	numberCpu=1;
    } else {
#if ABC_PARALLEL==2
#if 0 //ndef FAKE_CILK
      char temp[3];
      sprintf(temp,"%d",numberCpu);
      __cilkrts_set_param("nworkers",temp);
#endif
#endif
    }
    abcModel2->setParallelMode(numberCpu-1);
#endif
    //if (abcState()==3||abcState()==4) {
    //abcModel2->setMoreSpecialOptions((65536*2)|abcModel2->moreSpecialOptions());
    //}
    //if (processTune>0&&processTune<8)
    //abcModel2->setMoreSpecialOptions(abcModel2->moreSpecialOptions()|65536*processTune);
#if ABC_INSTRUMENT
    double startTimeCpu=CoinCpuTime();
    double startTimeElapsed=CoinGetTimeOfDay();
#if ABC_INSTRUMENT>1
    memset(abcPricing,0,sizeof(abcPricing));
    memset(abcPricingDense,0,sizeof(abcPricing));
    instrument_initialize(abcModel2->numberRows());
#endif
#endif
    if (!solveType) {
      abcModel2->ClpSimplex::doAbcDual();
    } else {
      int saveOptions=abcModel2->specialOptions();
      if (startUp==2)
	abcModel2->setSpecialOptions(8192|saveOptions);
      abcModel2->ClpSimplex::doAbcPrimal(ifValuesPass);
      abcModel2->setSpecialOptions(saveOptions);
    }
#if ABC_INSTRUMENT
    double timeCpu=CoinCpuTime()-startTimeCpu;
    double timeElapsed=CoinGetTimeOfDay()-startTimeElapsed;
    sprintf(line,"Cpu time for %s (%d rows, %d columns %d elements) %g elapsed %g ratio %g - %d iterations",
	    this->problemName().c_str(),this->numberRows(),this->numberColumns(),
	    this->getNumElements(),
	    timeCpu,timeElapsed,timeElapsed ? timeCpu/timeElapsed : 1.0,abcModel2->numberIterations());
    handler_->message(CLP_GENERAL, messages_)
      << line
      << CoinMessageEol;
#if ABC_INSTRUMENT>1
    {
      int n;
      n=0;
      for (int i=0;i<20;i++) 
	n+= abcPricing[i];
      printf("CCSparse pricing done %d times",n);
      int n2=0;
      for (int i=0;i<20;i++) 
	n2+= abcPricingDense[i];
      if (n2) 
	printf(" and dense pricing done %d times\n",n2);
      else
	printf("\n");
      n=0;
      printf ("CCS");
      for (int i=0;i<19;i++) {
	if (abcPricing[i]) {
	  if (n==5) {
	    n=0;
	    printf("\nCCS");
	  }
	  n++;
	  printf("(%d els,%d times) ",i,abcPricing[i]);
	}
      }
      if (abcPricing[19]) {
	if (n==5) {
	  n=0;
	  printf("\nCCS");
	}
	n++;
	printf("(>=19 els,%d times) ",abcPricing[19]);
      }
      if (n2) {
	printf ("CCD");
	for (int i=0;i<19;i++) {
	  if (abcPricingDense[i]) {
	    if (n==5) {
	      n=0;
	      printf("\nCCD");
	    }
	    n++;
	    int k1=(numberRows_/16)*i;;
	    int k2=CoinMin(numberRows_,k1+(numberRows_/16)-1);
	    printf("(%d-%d els,%d times) ",k1,k2,abcPricingDense[i]);
	  }
	}
      }
      printf("\n");
    }
    instrument_print();
#endif
#endif
    abcModel2->moveStatusToClp(this);
    //ClpModel::stopPermanentArrays();
    this->setSpecialOptions(this->specialOptions()&~65536);
#if 0
    this->writeBasis("a.bas",true);
    for (int i=0;i<this->numberRows();i++)
      printf("%d %g\n",i,this->dualRowSolution()[i]);
    this->dual();
    this->writeBasis("b.bas",true);
    for (int i=0;i<this->numberRows();i++)
      printf("%d %g\n",i,this->dualRowSolution()[i]);
#endif
    // switch off initialSolve flag
    moreSpecialOptions_ &= ~16384;
    //this->setNumberIterations(abcModel2->numberIterations()+this->numberIterations());
    delete abcModel2;
  }
}
#endif
/** General solve algorithm which can do presolve
    special options (bits)
    1 - do not perturb
    2 - do not scale
    4 - use crash (default allslack in dual, idiot in primal)
    8 - all slack basis in primal
    16 - switch off interrupt handling
    32 - do not try and make plus minus one matrix
    64 - do not use sprint even if problem looks good
 */
int
ClpSimplex::initialSolve(ClpSolve & options)
{
     ClpSolve::SolveType method = options.getSolveType();
     //ClpSolve::SolveType originalMethod=method;
     ClpSolve::PresolveType presolve = options.getPresolveType();
     int saveMaxIterations = maximumIterations();
     int finalStatus = -1;
     int numberIterations = 0;
     double time1 = CoinCpuTime();
     double timeX = time1;
     double time2;
     ClpMatrixBase * saveMatrix = NULL;
     ClpObjective * savedObjective = NULL;
     if (!objective_ || !matrix_) {
          // totally empty
          handler_->message(CLP_EMPTY_PROBLEM, messages_)
                    << 0
                    << 0
                    << 0
                    << CoinMessageEol;
          return -1;
     } else if (!numberRows_ || !numberColumns_ || !getNumElements()) {
          presolve = ClpSolve::presolveOff;
     }
     if (objective_->type() >= 2 && optimizationDirection_ == 0) {
          // pretend linear
          savedObjective = objective_;
          // make up objective
          double * obj = new double[numberColumns_];
          for (int i = 0; i < numberColumns_; i++) {
               double l = fabs(columnLower_[i]);
               double u = fabs(columnUpper_[i]);
               obj[i] = 0.0;
               if (CoinMin(l, u) < 1.0e20) {
                    if (l < u)
                         obj[i] = 1.0 + randomNumberGenerator_.randomDouble() * 1.0e-2;
                    else
                         obj[i] = -1.0 - randomNumberGenerator_.randomDouble() * 1.0e-2;
               }
          }
          objective_ = new ClpLinearObjective(obj, numberColumns_);
          delete [] obj;
     }
     ClpSimplex * model2 = this;
     bool interrupt = (options.getSpecialOption(2) == 0);
     CoinSighandler_t saveSignal = static_cast<CoinSighandler_t> (0);
     if (interrupt) {
          currentModel = model2;
          // register signal handler
          saveSignal = signal(SIGINT, signal_handler);
     }
     // If no status array - set up basis
     if (!status_)
          allSlackBasis();
     ClpPresolve * pinfo = new ClpPresolve();
     pinfo->setSubstitution(options.substitution());
     int presolveOptions = options.presolveActions();
     bool presolveToFile = (presolveOptions & 0x40000000) != 0;
     presolveOptions &= ~0x40000000;
     if ((presolveOptions & 0xffff) != 0)
          pinfo->setPresolveActions(presolveOptions);
     // switch off singletons to slacks
     //pinfo->setDoSingletonColumn(false); // done by bits
     int printOptions = options.getSpecialOption(5);
     if ((printOptions & 1) != 0)
          pinfo->statistics();
     double timePresolve = 0.0;
     double timeIdiot = 0.0;
     double timeCore = 0.0;
     eventHandler()->event(ClpEventHandler::presolveStart);
     int savePerturbation = perturbation_;
     int saveScaling = scalingFlag_;
#ifndef SLIM_CLP
#ifndef NO_RTTI
     if (dynamic_cast< ClpNetworkMatrix*>(matrix_)) {
          // network - switch off stuff
          presolve = ClpSolve::presolveOff;
     }
#else
     if (matrix_->type() == 11) {
          // network - switch off stuff
          presolve = ClpSolve::presolveOff;
     }
#endif
#endif
     if (presolve != ClpSolve::presolveOff) {
          bool costedSlacks = false;
#ifdef ABC_INHERIT
          int numberPasses = 20;
#else
          int numberPasses = 5;
#endif
          if (presolve == ClpSolve::presolveNumber) {
               numberPasses = options.getPresolvePasses();
               presolve = ClpSolve::presolveOn;
          } else if (presolve == ClpSolve::presolveNumberCost) {
               numberPasses = options.getPresolvePasses();
               presolve = ClpSolve::presolveOn;
               costedSlacks = true;
               // switch on singletons to slacks
               pinfo->setDoSingletonColumn(true);
               // gub stuff for testing
               //pinfo->setDoGubrow(true);
          }
#ifndef CLP_NO_STD
          if (presolveToFile) {
               // PreSolve to file - not fully tested
               printf("Presolving to file - presolve.save\n");
               pinfo->presolvedModelToFile(*this, "presolve.save", dblParam_[ClpPresolveTolerance],
                                          false, numberPasses);
               model2 = this;
          } else {
#endif
               model2 = pinfo->presolvedModel(*this, dblParam_[ClpPresolveTolerance],
                                             false, numberPasses, true, costedSlacks);
#ifndef CLP_NO_STD
          }
#endif
          time2 = CoinCpuTime();
          timePresolve = time2 - timeX;
          handler_->message(CLP_INTERVAL_TIMING, messages_)
                    << "Presolve" << timePresolve << time2 - time1
                    << CoinMessageEol;
          timeX = time2;
          if (!model2) {
               handler_->message(CLP_INFEASIBLE, messages_)
                         << CoinMessageEol;
               model2 = this;
	       eventHandler()->event(ClpEventHandler::presolveInfeasible);
               problemStatus_ = pinfo->presolveStatus(); 
               if (options.infeasibleReturn() || (moreSpecialOptions_ & 1) != 0) {
		 delete pinfo;
                    return -1;
               }
               presolve = ClpSolve::presolveOff;
	  } else {
#if 0 //def ABC_INHERIT
	    {
	      AbcSimplex * abcModel2=new AbcSimplex(*model2);
	      delete model2;
	      model2=abcModel2;
	      pinfo->setPresolvedModel(model2);
	    }
#else
	    //ClpModel::stopPermanentArrays();
	    //setSpecialOptions(specialOptions()&~65536);
#endif
	      model2->eventHandler()->setSimplex(model2);
	      int rcode=model2->eventHandler()->event(ClpEventHandler::presolveSize);
	      // see if too big or small
	      if (rcode==2) {
		  delete model2;
		 delete pinfo;
		  return -2;
	      } else if (rcode==3) {
		  delete model2;
		 delete pinfo;
		  return -3;
	      }
          }
	  model2->setMoreSpecialOptions(model2->moreSpecialOptions()&(~1024));
	  model2->eventHandler()->setSimplex(model2);
          // We may be better off using original (but if dual leave because of bounds)
          if (presolve != ClpSolve::presolveOff &&
                    numberRows_ < 1.01 * model2->numberRows_ && numberColumns_ < 1.01 * model2->numberColumns_
                    && model2 != this) {
               if(method != ClpSolve::useDual ||
                         (numberRows_ == model2->numberRows_ && numberColumns_ == model2->numberColumns_)) {
                    delete model2;
                    model2 = this;
                    presolve = ClpSolve::presolveOff;
               }
          }
     }
     if (interrupt)
          currentModel = model2;
     // For below >0 overrides
     // 0 means no, -1 means maybe
     int doIdiot = 0;
     int doCrash = 0;
     int doSprint = 0;
     int doSlp = 0;
     int primalStartup = 1;
     model2->eventHandler()->event(ClpEventHandler::presolveBeforeSolve);
     bool tryItSave = false;
     // switch to primal from automatic if just one cost entry
     if (method == ClpSolve::automatic && model2->numberColumns() > 5000 &&
               (specialOptions_ & 1024) != 0) {
          int numberColumns = model2->numberColumns();
          int numberRows = model2->numberRows();
          const double * obj = model2->objective();
          int nNon = 0;
          for (int i = 0; i < numberColumns; i++) {
               if (obj[i])
                    nNon++;
          }
          if (nNon == 1) {
#ifdef COIN_DEVELOP
               printf("Forcing primal\n");
#endif
               method = ClpSolve::usePrimal;
               tryItSave = numberRows > 200 && numberColumns > 2000 &&
                           (numberColumns > 2 * numberRows || (specialOptions_ & 1024) != 0);
          }
     }
     if (method != ClpSolve::useDual && method != ClpSolve::useBarrier
               && method != ClpSolve::useBarrierNoCross) {
          switch (options.getSpecialOption(1)) {
          case 0:
               doIdiot = -1;
               doCrash = -1;
               doSprint = -1;
               break;
          case 1:
               doIdiot = 0;
               doCrash = 1;
               if (options.getExtraInfo(1) > 0)
                    doCrash = options.getExtraInfo(1);
               doSprint = 0;
               break;
          case 2:
               doIdiot = 1;
               if (options.getExtraInfo(1) > 0)
                    doIdiot = options.getExtraInfo(1);
               doCrash = 0;
               doSprint = 0;
               break;
          case 3:
               doIdiot = 0;
               doCrash = 0;
               doSprint = 1;
               break;
          case 4:
               doIdiot = 0;
               doCrash = 0;
               doSprint = 0;
               break;
          case 5:
               doIdiot = 0;
               doCrash = -1;
               doSprint = -1;
               break;
          case 6:
               doIdiot = -1;
               doCrash = -1;
               doSprint = 0;
               break;
          case 7:
               doIdiot = -1;
               doCrash = 0;
               doSprint = -1;
               break;
          case 8:
               doIdiot = -1;
               doCrash = 0;
               doSprint = 0;
               break;
          case 9:
               doIdiot = 0;
               doCrash = 0;
               doSprint = -1;
               break;
          case 10:
               doIdiot = 0;
               doCrash = 0;
               doSprint = 0;
               if (options.getExtraInfo(1) > 0)
                    doSlp = options.getExtraInfo(1);
               break;
          case 11:
               doIdiot = 0;
               doCrash = 0;
               doSprint = 0;
               primalStartup = 0;
               break;
          default:
               abort();
          }
     } else {
          // Dual
          switch (options.getSpecialOption(0)) {
          case 0:
               doIdiot = 0;
               doCrash = 0;
               doSprint = 0;
               break;
          case 1:
               doIdiot = 0;
               doCrash = 1;
               if (options.getExtraInfo(0) > 0)
                    doCrash = options.getExtraInfo(0);
               doSprint = 0;
               break;
          case 2:
               doIdiot = -1;
               if (options.getExtraInfo(0) > 0)
                    doIdiot = options.getExtraInfo(0);
               doCrash = 0;
               doSprint = 0;
               break;
          default:
               abort();
          }
     }
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objectiveAsObject()));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (objective_->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
     // If quadratic then primal or barrier or slp
     if (quadraticObj) {
          doSprint = 0;
          doIdiot = 0;
          // off
          if (method == ClpSolve::useBarrier)
               method = ClpSolve::useBarrierNoCross;
          else if (method != ClpSolve::useBarrierNoCross)
               method = ClpSolve::usePrimal;
     }
#ifdef COIN_HAS_VOL
     // Save number of idiot
     int saveDoIdiot = doIdiot;
#endif
     // Just do this number of passes in Sprint
     int maxSprintPass = 100;
     // See if worth trying +- one matrix
     bool plusMinus = false;
     int numberElements = model2->getNumElements();
#ifndef SLIM_CLP
#ifndef NO_RTTI
     if (dynamic_cast< ClpNetworkMatrix*>(matrix_)) {
          // network - switch off stuff
          doIdiot = 0;
          if (doSprint < 0)
               doSprint = 0;
     }
#else
     if (matrix_->type() == 11) {
          // network - switch off stuff
          doIdiot = 0;
          //doSprint=0;
     }
#endif
#endif
     int numberColumns = model2->numberColumns();
     int numberRows = model2->numberRows();
     // If not all slack basis - switch off all except sprint
     int numberRowsBasic = 0;
     int iRow;
     for (iRow = 0; iRow < numberRows; iRow++)
          if (model2->getRowStatus(iRow) == basic)
               numberRowsBasic++;
     if (numberRowsBasic < numberRows) {
          doIdiot = 0;
          doCrash = 0;
          //doSprint=0;
     }
     if (options.getSpecialOption(3) == 0) {
          if(numberElements > 100000)
               plusMinus = true;
          if(numberElements > 10000 && (doIdiot || doSprint))
               plusMinus = true;
     } else if ((specialOptions_ & 1024) != 0) {
          plusMinus = true;
     }
#ifndef SLIM_CLP
     // Statistics (+1,-1, other) - used to decide on strategy if not +-1
     CoinBigIndex statistics[3] = { -1, 0, 0};
     if (plusMinus) {
          saveMatrix = model2->clpMatrix();
#ifndef NO_RTTI
          ClpPackedMatrix* clpMatrix =
               dynamic_cast< ClpPackedMatrix*>(saveMatrix);
#else
          ClpPackedMatrix* clpMatrix = NULL;
          if (saveMatrix->type() == 1)
               clpMatrix =
                    static_cast< ClpPackedMatrix*>(saveMatrix);
#endif
          if (clpMatrix) {
               ClpPlusMinusOneMatrix * newMatrix = new ClpPlusMinusOneMatrix(*(clpMatrix->matrix()));
               if (newMatrix->getIndices()) {
                  // CHECKME This test of specialOptions and the one above
                  // don't seem compatible.
#ifndef ABC_INHERIT
                    if ((specialOptions_ & 1024) == 0) {
                         model2->replaceMatrix(newMatrix);
                    } else {
#endif
                         // in integer (or abc) - just use for sprint/idiot
                         saveMatrix = NULL;
                         delete newMatrix;
#ifndef ABC_INHERIT
                    }
#endif
               } else {
                    handler_->message(CLP_MATRIX_CHANGE, messages_)
                              << "+- 1"
                              << CoinMessageEol;
                    CoinMemcpyN(newMatrix->startPositive(), 3, statistics);
                    saveMatrix = NULL;
                    plusMinus = false;
                    delete newMatrix;
               }
          } else {
               saveMatrix = NULL;
               plusMinus = false;
          }
     }
#endif
     if (this->factorizationFrequency() == 200) {
          // User did not touch preset
          model2->defaultFactorizationFrequency();
     } else if (model2 != this) {
          // make sure model2 has correct value
          model2->setFactorizationFrequency(this->factorizationFrequency());
     }
     if (method == ClpSolve::automatic) {
          if (doSprint == 0 && doIdiot == 0) {
               // off
               method = ClpSolve::useDual;
          } else {
               // only do primal if sprint or idiot
               if (doSprint > 0) {
                    method = ClpSolve::usePrimalorSprint;
               } else if (doIdiot > 0) {
                    method = ClpSolve::usePrimal;
               } else {
                    if (numberElements < 500000) {
                         // Small problem
                         if(numberRows * 10 > numberColumns || numberColumns < 6000
                                   || (numberRows * 20 > numberColumns && !plusMinus))
                              doSprint = 0; // switch off sprint
                    } else {
                         // larger problem
                         if(numberRows * 8 > numberColumns)
                              doSprint = 0; // switch off sprint
                    }
                    // switch off idiot or sprint if any free variable
		    // switch off sprint if very few with costs
                    int iColumn;
                    const double * columnLower = model2->columnLower();
                    const double * columnUpper = model2->columnUpper();
		    const double * objective = model2->objective();
		    int nObj=0;
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         if (columnLower[iColumn] < -1.0e10 && columnUpper[iColumn] > 1.0e10) {
			      doSprint = 0;
                              doIdiot = 0;
                              break;
			 } else if (objective[iColumn]) {
			   nObj++;
                         }
                    }
		    if (nObj*10<numberColumns)
		      doSprint=0;
                    int nPasses = 0;
                    // look at rhs
                    int iRow;
                    double largest = 0.0;
                    double smallest = 1.0e30;
                    double largestGap = 0.0;
                    int numberNotE = 0;
                    bool notInteger = false;
                    for (iRow = 0; iRow < numberRows; iRow++) {
                         double value1 = model2->rowLower_[iRow];
                         if (value1 && value1 > -1.0e31) {
                              largest = CoinMax(largest, fabs(value1));
                              smallest = CoinMin(smallest, fabs(value1));
                              if (fabs(value1 - floor(value1 + 0.5)) > 1.0e-8) {
                                   notInteger = true;
                                   break;
                              }
                         }
                         double value2 = model2->rowUpper_[iRow];
                         if (value2 && value2 < 1.0e31) {
                              largest = CoinMax(largest, fabs(value2));
                              smallest = CoinMin(smallest, fabs(value2));
                              if (fabs(value2 - floor(value2 + 0.5)) > 1.0e-8) {
                                   notInteger = true;
                                   break;
                              }
                         }
                         // CHECKME This next bit can't be right...
                         if (value2 > value1) {
                              numberNotE++;
                              //if (value2 > 1.0e31 || value1 < -1.0e31)
			      //   largestGap = COIN_DBL_MAX;
                              //else
			      //   largestGap = value2 - value1;
                         }
                    }
                    bool tryIt = numberRows > 200 && numberColumns > 2000 &&
                                 (numberColumns > 2 * numberRows || (method != ClpSolve::useDual && (specialOptions_ & 1024) != 0));
                    tryItSave = tryIt;
                    if (numberRows < 1000 && numberColumns < 3000)
                         tryIt = false;
                    if (notInteger)
                         tryIt = false;
                    if (largest / smallest > 10 || (largest / smallest > 2.0 && largest > 50))
                         tryIt = false;
                    if (tryIt) {
                         if (largest / smallest > 2.0) {
                              nPasses = 10 + numberColumns / 100000;
                              nPasses = CoinMin(nPasses, 50);
                              nPasses = CoinMax(nPasses, 15);
                              if (numberRows > 20000 && nPasses > 5) {
                                   // Might as well go for it
                                   nPasses = CoinMax(nPasses, 71);
                              } else if (numberRows > 2000 && nPasses > 5) {
                                   nPasses = CoinMax(nPasses, 50);
                              } else if (numberElements < 3 * numberColumns) {
                                   nPasses = CoinMin(nPasses, 10); // probably not worh it
                              }
                         } else if (largest / smallest > 1.01 || numberElements <= 3 * numberColumns) {
                              nPasses = 10 + numberColumns / 1000;
                              nPasses = CoinMin(nPasses, 100);
                              nPasses = CoinMax(nPasses, 30);
                              if (numberRows > 25000) {
                                   // Might as well go for it
                                   nPasses = CoinMax(nPasses, 71);
                              }
                              if (!largestGap)
                                   nPasses *= 2;
                         } else {
                              nPasses = 10 + numberColumns / 1000;
                              nPasses = CoinMin(nPasses, 200);
                              nPasses = CoinMax(nPasses, 100);
                              if (!largestGap)
                                   nPasses *= 2;
                         }
                    }
                    //printf("%d rows %d cols plus %c tryIt %c largest %g smallest %g largestGap %g npasses %d sprint %c\n",
                    //     numberRows,numberColumns,plusMinus ? 'Y' : 'N',
                    //     tryIt ? 'Y' :'N',largest,smallest,largestGap,nPasses,doSprint ? 'Y' :'N');
                    //exit(0);
                    if (!tryIt || nPasses <= 5)
                         doIdiot = 0;
                    if (doSprint) {
                         method = ClpSolve::usePrimalorSprint;
                    } else if (doIdiot) {
                         method = ClpSolve::usePrimal;
                    } else {
                         method = ClpSolve::useDual;
                    }
               }
          }
     }
     if (method == ClpSolve::usePrimalorSprint) {
          if (doSprint < 0) {
               if (numberElements < 500000) {
                    // Small problem
                    if(numberRows * 10 > numberColumns || numberColumns < 6000
                              || (numberRows * 20 > numberColumns && !plusMinus))
                         method = ClpSolve::usePrimal; // switch off sprint
               } else {
                    // larger problem
                    if(numberRows * 8 > numberColumns)
                         method = ClpSolve::usePrimal; // switch off sprint
                    // but make lightweight
                    if(numberRows * 10 > numberColumns || numberColumns < 6000
                              || (numberRows * 20 > numberColumns && !plusMinus))
                         maxSprintPass = 10;
               }
          } else if (doSprint == 0) {
               method = ClpSolve::usePrimal; // switch off sprint
          }
     }
     if (method == ClpSolve::useDual) {
          double * saveLower = NULL;
          double * saveUpper = NULL;
          if (presolve == ClpSolve::presolveOn) {
               int numberInfeasibilities = model2->tightenPrimalBounds(0.0, 0);
               if (numberInfeasibilities) {
                    handler_->message(CLP_INFEASIBLE, messages_)
                              << CoinMessageEol;
                    delete model2;
                    model2 = this;
                    presolve = ClpSolve::presolveOff;
               }
          } else if (numberRows_ + numberColumns_ > 5000) {
               // do anyway
               saveLower = new double[numberRows_+numberColumns_];
               CoinMemcpyN(model2->columnLower(), numberColumns_, saveLower);
               CoinMemcpyN(model2->rowLower(), numberRows_, saveLower + numberColumns_);
               saveUpper = new double[numberRows_+numberColumns_];
               CoinMemcpyN(model2->columnUpper(), numberColumns_, saveUpper);
               CoinMemcpyN(model2->rowUpper(), numberRows_, saveUpper + numberColumns_);
               int numberInfeasibilities = model2->tightenPrimalBounds();
               if (numberInfeasibilities) {
                    handler_->message(CLP_INFEASIBLE, messages_)
                              << CoinMessageEol;
                    CoinMemcpyN(saveLower, numberColumns_, model2->columnLower());
                    CoinMemcpyN(saveLower + numberColumns_, numberRows_, model2->rowLower());
                    delete [] saveLower;
                    saveLower = NULL;
                    CoinMemcpyN(saveUpper, numberColumns_, model2->columnUpper());
                    CoinMemcpyN(saveUpper + numberColumns_, numberRows_, model2->rowUpper());
                    delete [] saveUpper;
                    saveUpper = NULL;
               }
          }
#ifndef COIN_HAS_VOL
          // switch off idiot and volume for now
          doIdiot = 0;
#endif
          // pick up number passes
          int nPasses = 0;
          int numberNotE = 0;
#ifndef SLIM_CLP
          if ((doIdiot < 0 && plusMinus) || doIdiot > 0) {
               // See if candidate for idiot
               nPasses = 0;
               Idiot info(*model2);
               // Get average number of elements per column
               double ratio  = static_cast<double> (numberElements) / static_cast<double> (numberColumns);
               // look at rhs
               int iRow;
               double largest = 0.0;
               double smallest = 1.0e30;
               for (iRow = 0; iRow < numberRows; iRow++) {
                    double value1 = model2->rowLower_[iRow];
                    if (value1 && value1 > -1.0e31) {
                         largest = CoinMax(largest, fabs(value1));
                         smallest = CoinMin(smallest, fabs(value1));
                    }
                    double value2 = model2->rowUpper_[iRow];
                    if (value2 && value2 < 1.0e31) {
                         largest = CoinMax(largest, fabs(value2));
                         smallest = CoinMin(smallest, fabs(value2));
                    }
                    if (value2 > value1) {
                         numberNotE++;
                    }
               }
               if (doIdiot < 0) {
                    if (numberRows > 200 && numberColumns > 5000 && ratio >= 3.0 &&
                              largest / smallest < 1.1 && !numberNotE) {
                         nPasses = 71;
                    }
               }
               if (doIdiot > 0) {
                    nPasses = CoinMax(nPasses, doIdiot);
                    if (nPasses > 70) {
                         info.setStartingWeight(1.0e3);
                         info.setDropEnoughFeasibility(0.01);
                    }
               }
               if (nPasses > 20) {
#ifdef COIN_HAS_VOL
                    int returnCode = solveWithVolume(model2, nPasses, saveDoIdiot);
                    if (!returnCode) {
                         time2 = CoinCpuTime();
                         timeIdiot = time2 - timeX;
                         handler_->message(CLP_INTERVAL_TIMING, messages_)
                                   << "Idiot Crash" << timeIdiot << time2 - time1
                                   << CoinMessageEol;
                         timeX = time2;
                    } else {
                         nPasses = 0;
                    }
#else
                    nPasses = 0;
#endif
               } else {
                    nPasses = 0;
               }
          }
#endif
          if (doCrash) {
#ifdef ABC_INHERIT
	    if (!model2->abcState()) {
#endif
               switch(doCrash) {
                    // standard
               case 1:
                    model2->crash(1000, 1);
                    break;
                    // As in paper by Solow and Halim (approx)
               case 2:
               case 3:
                    model2->crash(model2->dualBound(), 0);
                    break;
                    // Just put free in basis
               case 4:
                    model2->crash(0.0, 3);
                    break;
               }
#ifdef ABC_INHERIT
	    } else if (doCrash>=0) {
	       model2->setAbcState(model2->abcState()|256*doCrash);
	    }
#endif
          }
          if (!nPasses) {
               int saveOptions = model2->specialOptions();
               if (model2->numberRows() > 100)
                    model2->setSpecialOptions(saveOptions | 64); // go as far as possible
               //int numberRows = model2->numberRows();
               //int numberColumns = model2->numberColumns();
               if (dynamic_cast< ClpPackedMatrix*>(matrix_)) {
                    // See if original wanted vector
                    ClpPackedMatrix * clpMatrixO = dynamic_cast< ClpPackedMatrix*>(matrix_);
                    ClpMatrixBase * matrix = model2->clpMatrix();
                    if (dynamic_cast< ClpPackedMatrix*>(matrix) && clpMatrixO->wantsSpecialColumnCopy()) {
                         ClpPackedMatrix * clpMatrix = dynamic_cast< ClpPackedMatrix*>(matrix);
                         clpMatrix->makeSpecialColumnCopy();
                         //model2->setSpecialOptions(model2->specialOptions()|256); // to say no row copy for comparisons
                         model2->dual(0);
                         clpMatrix->releaseSpecialColumnCopy();
                    } else {
#ifndef ABC_INHERIT
		      model2->dual(0);
#else
		      model2->dealWithAbc(0,0,interrupt);
#endif
                    }
               } else {
                    model2->dual(0);
               }
          } else if (!numberNotE && 0) {
               // E so we can do in another way
               double * pi = model2->dualRowSolution();
               int i;
               int numberColumns = model2->numberColumns();
               int numberRows = model2->numberRows();
               double * saveObj = new double[numberColumns];
               CoinMemcpyN(model2->objective(), numberColumns, saveObj);
               CoinMemcpyN(model2->objective(),
                           numberColumns, model2->dualColumnSolution());
               model2->clpMatrix()->transposeTimes(-1.0, pi, model2->dualColumnSolution());
               CoinMemcpyN(model2->dualColumnSolution(),
                           numberColumns, model2->objective());
               const double * rowsol = model2->primalRowSolution();
               double offset = 0.0;
               for (i = 0; i < numberRows; i++) {
                    offset += pi[i] * rowsol[i];
               }
               double value2;
               model2->getDblParam(ClpObjOffset, value2);
               //printf("Offset %g %g\n",offset,value2);
               model2->setDblParam(ClpObjOffset, value2 - offset);
               model2->setPerturbation(51);
               //model2->setRowObjective(pi);
               // zero out pi
               //memset(pi,0,numberRows*sizeof(double));
               // Could put some in basis - only partially tested
               model2->allSlackBasis();
               //model2->factorization()->maximumPivots(200);
               //model2->setLogLevel(63);
               // solve
               model2->dual(0);
               model2->setDblParam(ClpObjOffset, value2);
               CoinMemcpyN(saveObj, numberColumns, model2->objective());
               // zero out pi
               //memset(pi,0,numberRows*sizeof(double));
               //model2->setRowObjective(pi);
               delete [] saveObj;
               //model2->dual(0);
               model2->setPerturbation(50);
               model2->primal();
          } else {
               // solve
               model2->setPerturbation(100);
               model2->dual(2);
               model2->setPerturbation(50);
               model2->dual(0);
          }
          if (saveLower) {
               CoinMemcpyN(saveLower, numberColumns_, model2->columnLower());
               CoinMemcpyN(saveLower + numberColumns_, numberRows_, model2->rowLower());
               delete [] saveLower;
               saveLower = NULL;
               CoinMemcpyN(saveUpper, numberColumns_, model2->columnUpper());
               CoinMemcpyN(saveUpper + numberColumns_, numberRows_, model2->rowUpper());
               delete [] saveUpper;
               saveUpper = NULL;
          }
          time2 = CoinCpuTime();
          timeCore = time2 - timeX;
          handler_->message(CLP_INTERVAL_TIMING, messages_)
                    << "Dual" << timeCore << time2 - time1
                    << CoinMessageEol;
          timeX = time2;
     } else if (method == ClpSolve::usePrimal) {
#ifndef SLIM_CLP
          if (doIdiot) {
               int nPasses = 0;
               Idiot info(*model2);
               // Get average number of elements per column
               double ratio  = static_cast<double> (numberElements) / static_cast<double> (numberColumns);
               // look at rhs
               int iRow;
               double largest = 0.0;
               double smallest = 1.0e30;
               double largestGap = 0.0;
               int numberNotE = 0;
               for (iRow = 0; iRow < numberRows; iRow++) {
                    double value1 = model2->rowLower_[iRow];
                    if (value1 && value1 > -1.0e31) {
                         largest = CoinMax(largest, fabs(value1));
                         smallest = CoinMin(smallest, fabs(value1));
                    }
                    double value2 = model2->rowUpper_[iRow];
                    if (value2 && value2 < 1.0e31) {
                         largest = CoinMax(largest, fabs(value2));
                         smallest = CoinMin(smallest, fabs(value2));
                    }
                    if (value2 > value1) {
                         numberNotE++;
                         if (value2 > 1.0e31 || value1 < -1.0e31)
                              largestGap = COIN_DBL_MAX;
                         else
                              largestGap = value2 - value1;
                    }
               }
               bool increaseSprint = plusMinus;
               if ((specialOptions_ & 1024) != 0)
                    increaseSprint = false;
               if (!plusMinus) {
                    // If 90% +- 1 then go for sprint
                    if (statistics[0] >= 0 && 10 * statistics[2] < statistics[0] + statistics[1])
                         increaseSprint = true;
               }
               bool tryIt = tryItSave;
               if (numberRows < 1000 && numberColumns < 3000)
                    tryIt = false;
               if (tryIt) {
                    if (increaseSprint) {
                         info.setStartingWeight(1.0e3);
                         info.setReduceIterations(6);
                         // also be more lenient on infeasibilities
                         info.setDropEnoughFeasibility(0.5 * info.getDropEnoughFeasibility());
                         info.setDropEnoughWeighted(-2.0);
                         if (largest / smallest > 2.0) {
                              nPasses = 10 + numberColumns / 100000;
                              nPasses = CoinMin(nPasses, 50);
                              nPasses = CoinMax(nPasses, 15);
                              if (numberRows > 20000 && nPasses > 5) {
                                   // Might as well go for it
                                   nPasses = CoinMax(nPasses, 71);
                              } else if (numberRows > 2000 && nPasses > 5) {
                                   nPasses = CoinMax(nPasses, 50);
                              } else if (numberElements < 3 * numberColumns) {
                                   nPasses = CoinMin(nPasses, 10); // probably not worh it
                                   if (doIdiot < 0)
                                        info.setLightweight(1); // say lightweight idiot
                              } else {
                                   if (doIdiot < 0)
                                        info.setLightweight(1); // say lightweight idiot
                              }
                         } else if (largest / smallest > 1.01 || numberElements <= 3 * numberColumns) {
                              nPasses = 10 + numberColumns / 1000;
                              nPasses = CoinMin(nPasses, 100);
                              nPasses = CoinMax(nPasses, 30);
                              if (numberRows > 25000) {
                                   // Might as well go for it
                                   nPasses = CoinMax(nPasses, 71);
                              }
                              if (!largestGap)
                                   nPasses *= 2;
                         } else {
                              nPasses = 10 + numberColumns / 1000;
                              nPasses = CoinMin(nPasses, 200);
                              nPasses = CoinMax(nPasses, 100);
                              info.setStartingWeight(1.0e-1);
                              info.setReduceIterations(6);
                              if (!largestGap)
                                   nPasses *= 2;
                              //info.setFeasibilityTolerance(1.0e-7);
                         }
                         // If few passes - don't bother
                         if (nPasses <= 5 && !plusMinus)
                              nPasses = 0;
                    } else {
                         if (doIdiot < 0)
                              info.setLightweight(1); // say lightweight idiot
                         if (largest / smallest > 1.01 || numberNotE || statistics[2] > statistics[0] + statistics[1]) {
                              if (numberRows > 25000 || numberColumns > 5 * numberRows) {
                                   nPasses = 50;
                              } else if (numberColumns > 4 * numberRows) {
                                   nPasses = 20;
                              } else {
                                   nPasses = 5;
                              }
                         } else {
                              if (numberRows > 25000 || numberColumns > 5 * numberRows) {
                                   nPasses = 50;
                                   info.setLightweight(0); // say not lightweight idiot
                              } else if (numberColumns > 4 * numberRows) {
                                   nPasses = 20;
                              } else {
                                   nPasses = 15;
                              }
                         }
                         if (ratio < 3.0) {
                              nPasses = static_cast<int> (ratio * static_cast<double> (nPasses) / 4.0); // probably not worth it
                         } else {
                              nPasses = CoinMax(nPasses, 5);
                         }
                         if (numberRows > 25000 && nPasses > 5) {
                              // Might as well go for it
                              nPasses = CoinMax(nPasses, 71);
                         } else if (increaseSprint) {
                              nPasses *= 2;
                              nPasses = CoinMin(nPasses, 71);
                         } else if (nPasses == 5 && ratio > 5.0) {
                              nPasses = static_cast<int> (static_cast<double> (nPasses) * (ratio / 5.0)); // increase if lots of elements per column
                         }
                         if (nPasses <= 5 && !plusMinus)
                              nPasses = 0;
                         //info.setStartingWeight(1.0e-1);
                    }
               }
               if (doIdiot > 0) {
                    // pick up number passes
                    nPasses = options.getExtraInfo(1) % 1000000;
#ifdef COIN_HAS_VOL
                    int returnCode = solveWithVolume(model2, nPasses, saveDoIdiot);
		    nPasses=0;
                    if (!returnCode) {
		      time2 = CoinCpuTime();
		      timeIdiot = time2 - timeX;
		      handler_->message(CLP_INTERVAL_TIMING, messages_)
			<< "Idiot Crash" << timeIdiot << time2 - time1
			<< CoinMessageEol;
		      timeX = time2;
                    }
#endif
                    if (nPasses > 70) {
                         info.setStartingWeight(1.0e3);
                         info.setReduceIterations(6);
			 //if (nPasses > 200) 
			 //info.setFeasibilityTolerance(1.0e-9);
			 //if (nPasses > 1900) 
			 //info.setWeightFactor(0.93);
			 if (nPasses > 900) {
			   double reductions=nPasses/6.0;
			   if (nPasses<5000) {
			     reductions /= 12.0;
			   } else {
			     reductions /= 13.0;
			     info.setStartingWeight(1.0e4);
			   }
			   double ratio=1.0/std::pow(10.0,(1.0/reductions));
			   printf("%d passes reduction factor %g\n",nPasses,ratio);
			   info.setWeightFactor(ratio);
			 } else if (nPasses > 500) {
			   info.setWeightFactor(0.7);
			 } else if (nPasses > 200) {
			   info.setWeightFactor(0.5);
			 }
			 if (maximumIterations()<nPasses) {
			   printf("Presuming maximumIterations is just for Idiot\n");
			   nPasses=maximumIterations();
			   setMaximumIterations(COIN_INT_MAX);
			   model2->setMaximumIterations(COIN_INT_MAX);
			 }
                         if (nPasses >= 10000&&nPasses<100000) {
                              int k = nPasses % 100;
                              nPasses /= 200;
                              info.setReduceIterations(3);
                              if (k)
                                   info.setStartingWeight(1.0e2);
                         }
                         // also be more lenient on infeasibilities
                         info.setDropEnoughFeasibility(0.5 * info.getDropEnoughFeasibility());
                         info.setDropEnoughWeighted(-2.0);
                    } else if (nPasses >= 50) {
                         info.setStartingWeight(1.0e3);
                         //info.setReduceIterations(6);
                    }
                    // For experimenting
                    if (nPasses < 70 && (nPasses % 10) > 0 && (nPasses % 10) < 4) {
                         info.setStartingWeight(1.0e3);
                         info.setLightweight(nPasses % 10); // special testing
#ifdef COIN_DEVELOP
                         printf("warning - odd lightweight %d\n", nPasses % 10);
                         //info.setReduceIterations(6);
#endif
                    }
               }
               if (options.getExtraInfo(1) > 1000000)
                    nPasses += 1000000;
               if (nPasses) {
                    doCrash = 0;
#if 0
                    double * solution = model2->primalColumnSolution();
                    int iColumn;
                    double * saveLower = new double[numberColumns];
                    CoinMemcpyN(model2->columnLower(), numberColumns, saveLower);
                    double * saveUpper = new double[numberColumns];
                    CoinMemcpyN(model2->columnUpper(), numberColumns, saveUpper);
                    printf("doing tighten before idiot\n");
                    model2->tightenPrimalBounds();
                    // Move solution
                    double * columnLower = model2->columnLower();
                    double * columnUpper = model2->columnUpper();
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         if (columnLower[iColumn] > 0.0)
                              solution[iColumn] = columnLower[iColumn];
                         else if (columnUpper[iColumn] < 0.0)
                              solution[iColumn] = columnUpper[iColumn];
                         else
                              solution[iColumn] = 0.0;
                    }
                    CoinMemcpyN(saveLower, numberColumns, columnLower);
                    CoinMemcpyN(saveUpper, numberColumns, columnUpper);
                    delete [] saveLower;
                    delete [] saveUpper;
#else
                    // Allow for crossover
                    //#define LACI_TRY
#ifndef LACI_TRY
                    //if (doIdiot>0)
#ifdef ABC_INHERIT
		    if (!model2->abcState()) 
#endif
                    info.setStrategy(512 | info.getStrategy());
#endif
                    // Allow for scaling
                    info.setStrategy(32 | info.getStrategy());
                    info.crash(nPasses, model2->messageHandler(), model2->messagesPointer());
#endif
                    time2 = CoinCpuTime();
                    timeIdiot = time2 - timeX;
                    handler_->message(CLP_INTERVAL_TIMING, messages_)
                              << "Idiot Crash" << timeIdiot << time2 - time1
                              << CoinMessageEol;
                    timeX = time2;
		    if (nPasses>100000&&nPasses<100500) {
		      // make sure no status left
		      model2->createStatus();
		      // solve
		      if (model2->factorizationFrequency() == 200) {
			// User did not touch preset
			model2->defaultFactorizationFrequency();
		      }
		      //int numberRows = model2->numberRows();
		      int numberColumns = model2->numberColumns();
		      // save duals
		      //double * saveDuals = CoinCopyOfArray(model2->dualRowSolution(),numberRows);
		      // for moment this only works on nug etc (i.e. all ==)
		      // needs row objective
		      double * saveObj = CoinCopyOfArray(model2->objective(),numberColumns);
		      double * pi = model2->dualRowSolution();
		      model2->clpMatrix()->transposeTimes(-1.0, pi, model2->objective());
		      // just primal values pass
		      double saveScale = model2->objectiveScale();
		      model2->setObjectiveScale(1.0e-3);
		      model2->primal(2);
		      model2->writeMps("xx.mps");
		      double * solution = model2->primalColumnSolution();
		      double * upper = model2->columnUpper();
		      for (int i=0;i<numberColumns;i++) {
			if (solution[i]<100.0)
			  upper[i]=1000.0;
		      }
		      model2->setProblemStatus(-1);
		      model2->setObjectiveScale(saveScale);
#ifdef ABC_INHERIT
		      AbcSimplex * abcModel2=new AbcSimplex(*model2);
		      if (interrupt)
			currentAbcModel = abcModel2;
		      if (abcSimplex_) {
			// move factorization stuff
			abcModel2->factorization()->synchronize(model2->factorization(),abcModel2);
		      }
		      abcModel2->startPermanentArrays();
		      abcModel2->setAbcState(CLP_ABC_WANTED);
#if ABC_PARALLEL
		      int parallelMode=1;
		      printf("Parallel mode %d\n",parallelMode);
		      abcModel2->setParallelMode(parallelMode);
#endif
		      //if (processTune>0&&processTune<8)
		      //abcModel2->setMoreSpecialOptions(abcModel2->moreSpecialOptions()|65536*processTune);
		      abcModel2->doAbcDual();
		      abcModel2->moveStatusToClp(model2);
		      //ClpModel::stopPermanentArrays();
		      model2->setSpecialOptions(model2->specialOptions()&~65536);
		      //model2->dual();
		      //model2->setNumberIterations(abcModel2->numberIterations()+model2->numberIterations());
		      delete abcModel2;
#endif
		      memcpy(model2->objective(),saveObj,numberColumns*sizeof(double));
		      //delete [] saveDuals;
		      delete [] saveObj;
		      model2->dual(2);
		    } // end dubious idiot
               }
          }
#endif
          // ?
          if (doCrash) {
               switch(doCrash) {
                    // standard
               case 1:
                    model2->crash(1000, 1);
                    break;
                    // As in paper by Solow and Halim (approx)
               case 2:
                    model2->crash(model2->dualBound(), 0);
                    break;
                    // My take on it
               case 3:
                    model2->crash(model2->dualBound(), -1);
                    break;
                    // Just put free in basis
               case 4:
                    model2->crash(0.0, 3);
                    break;
               }
          }
#ifndef SLIM_CLP
          if (doSlp > 0 && objective_->type() == 2) {
               model2->nonlinearSLP(doSlp, 1.0e-5);
          }
#endif
#ifndef LACI_TRY
          if (options.getSpecialOption(1) != 2 ||
                    options.getExtraInfo(1) < 1000000) {
               if (dynamic_cast< ClpPackedMatrix*>(matrix_)) {
                    // See if original wanted vector
                    ClpPackedMatrix * clpMatrixO = dynamic_cast< ClpPackedMatrix*>(matrix_);
                    ClpMatrixBase * matrix = model2->clpMatrix();
                    if (dynamic_cast< ClpPackedMatrix*>(matrix) && clpMatrixO->wantsSpecialColumnCopy()) {
                         ClpPackedMatrix * clpMatrix = dynamic_cast< ClpPackedMatrix*>(matrix);
                         clpMatrix->makeSpecialColumnCopy();
                         //model2->setSpecialOptions(model2->specialOptions()|256); // to say no row copy for comparisons
                         model2->primal(primalStartup);
                         clpMatrix->releaseSpecialColumnCopy();
                    } else {
#ifndef ABC_INHERIT
		        model2->primal(primalStartup);
#else
			model2->dealWithAbc(1,primalStartup,interrupt);
#endif
                    }
               } else {
#ifndef ABC_INHERIT
                    model2->primal(primalStartup);
#else
		    model2->dealWithAbc(1,primalStartup,interrupt);
#endif
               }
          }
#endif
          time2 = CoinCpuTime();
          timeCore = time2 - timeX;
          handler_->message(CLP_INTERVAL_TIMING, messages_)
                    << "Primal" << timeCore << time2 - time1
                    << CoinMessageEol;
          timeX = time2;
     } else if (method == ClpSolve::usePrimalorSprint) {
          // Sprint
          /*
            This driver implements what I called Sprint when I introduced the idea
            many years ago.  Cplex calls it "sifting" which I think is just as silly.
            When I thought of this trivial idea
            it reminded me of an LP code of the 60's called sprint which after
            every factorization took a subset of the matrix into memory (all
            64K words!) and then iterated very fast on that subset.  On the
            problems of those days it did not work very well, but it worked very
            well on aircrew scheduling problems where there were very large numbers
            of columns all with the same flavor.
          */

          /* The idea works best if you can get feasible easily.  To make it
             more general we can add in costed slacks */

          int originalNumberColumns = model2->numberColumns();
          int numberRows = model2->numberRows();
          ClpSimplex * originalModel2 = model2;

          // We will need arrays to choose variables.  These are too big but ..
          double * weight = new double [numberRows+originalNumberColumns];
          int * sort = new int [numberRows+originalNumberColumns];
          int numberSort = 0;
          // We are going to add slacks to get feasible.
          // initial list will just be artificials
          int iColumn;
          const double * columnLower = model2->columnLower();
          const double * columnUpper = model2->columnUpper();
          double * columnSolution = model2->primalColumnSolution();

          // See if we have costed slacks
          int * negSlack = new int[numberRows];
          int * posSlack = new int[numberRows];
          int iRow;
          for (iRow = 0; iRow < numberRows; iRow++) {
               negSlack[iRow] = -1;
               posSlack[iRow] = -1;
          }
          const double * element = model2->matrix()->getElements();
          const int * row = model2->matrix()->getIndices();
          const CoinBigIndex * columnStart = model2->matrix()->getVectorStarts();
          const int * columnLength = model2->matrix()->getVectorLengths();
          //bool allSlack = (numberRowsBasic==numberRows);
          for (iColumn = 0; iColumn < originalNumberColumns; iColumn++) {
               if (!columnSolution[iColumn] || fabs(columnSolution[iColumn]) > 1.0e20) {
                    double value = 0.0;
                    if (columnLower[iColumn] > 0.0)
                         value = columnLower[iColumn];
                    else if (columnUpper[iColumn] < 0.0)
                         value = columnUpper[iColumn];
                    columnSolution[iColumn] = value;
               }
               if (columnLength[iColumn] == 1) {
                    int jRow = row[columnStart[iColumn]];
                    if (!columnLower[iColumn]) {
                         if (element[columnStart[iColumn]] > 0.0 && posSlack[jRow] < 0)
                              posSlack[jRow] = iColumn;
                         else if (element[columnStart[iColumn]] < 0.0 && negSlack[jRow] < 0)
                              negSlack[jRow] = iColumn;
                    } else if (!columnUpper[iColumn]) {
                         if (element[columnStart[iColumn]] < 0.0 && posSlack[jRow] < 0)
                              posSlack[jRow] = iColumn;
                         else if (element[columnStart[iColumn]] > 0.0 && negSlack[jRow] < 0)
                              negSlack[jRow] = iColumn;
                    }
               }
          }
          // now see what that does to row solution
          double * rowSolution = model2->primalRowSolution();
          CoinZeroN (rowSolution, numberRows);
          model2->clpMatrix()->times(1.0, columnSolution, rowSolution);
          // See if we can adjust using costed slacks
          double penalty = CoinMax(1.0e5, CoinMin(infeasibilityCost_ * 0.01, 1.0e10)) * optimizationDirection_;
          const double * lower = model2->rowLower();
          const double * upper = model2->rowUpper();
          for (iRow = 0; iRow < numberRows; iRow++) {
               if (lower[iRow] > rowSolution[iRow] + 1.0e-8) {
                    int jColumn = posSlack[iRow];
                    if (jColumn >= 0) {
                         if (columnSolution[jColumn])
                              continue;
                         double difference = lower[iRow] - rowSolution[iRow];
                         double elementValue = element[columnStart[jColumn]];
                         if (elementValue > 0.0) {
                              double movement = CoinMin(difference / elementValue, columnUpper[jColumn]);
                              columnSolution[jColumn] = movement;
                              rowSolution[iRow] += movement * elementValue;
                         } else {
                              double movement = CoinMax(difference / elementValue, columnLower[jColumn]);
                              columnSolution[jColumn] = movement;
                              rowSolution[iRow] += movement * elementValue;
                         }
                    }
               } else if (upper[iRow] < rowSolution[iRow] - 1.0e-8) {
                    int jColumn = negSlack[iRow];
                    if (jColumn >= 0) {
                         if (columnSolution[jColumn])
                              continue;
                         double difference = upper[iRow] - rowSolution[iRow];
                         double elementValue = element[columnStart[jColumn]];
                         if (elementValue < 0.0) {
                              double movement = CoinMin(difference / elementValue, columnUpper[jColumn]);
                              columnSolution[jColumn] = movement;
                              rowSolution[iRow] += movement * elementValue;
                         } else {
                              double movement = CoinMax(difference / elementValue, columnLower[jColumn]);
                              columnSolution[jColumn] = movement;
                              rowSolution[iRow] += movement * elementValue;
                         }
                    }
               }
          }
          delete [] negSlack;
          delete [] posSlack;
          int nRow = numberRows;
          bool network = false;
          if (dynamic_cast< ClpNetworkMatrix*>(matrix_)) {
               network = true;
               nRow *= 2;
          }
          int * addStarts = new int [nRow+1];
          int * addRow = new int[nRow];
          double * addElement = new double[nRow];
          addStarts[0] = 0;
          int numberArtificials = 0;
          int numberAdd = 0;
          double * addCost = new double [numberRows];
          for (iRow = 0; iRow < numberRows; iRow++) {
               if (lower[iRow] > rowSolution[iRow] + 1.0e-8) {
                    addRow[numberAdd] = iRow;
                    addElement[numberAdd++] = 1.0;
                    if (network) {
                         addRow[numberAdd] = numberRows;
                         addElement[numberAdd++] = -1.0;
                    }
                    addCost[numberArtificials] = penalty;
                    numberArtificials++;
                    addStarts[numberArtificials] = numberAdd;
               } else if (upper[iRow] < rowSolution[iRow] - 1.0e-8) {
                    addRow[numberAdd] = iRow;
                    addElement[numberAdd++] = -1.0;
                    if (network) {
                         addRow[numberAdd] = numberRows;
                         addElement[numberAdd++] = 1.0;
                    }
                    addCost[numberArtificials] = penalty;
                    numberArtificials++;
                    addStarts[numberArtificials] = numberAdd;
               }
          }
          if (numberArtificials) {
               // need copy so as not to disturb original
               model2 = new ClpSimplex(*model2);
               if (network) {
                    // network - add a null row
                    model2->addRow(0, NULL, NULL, -COIN_DBL_MAX, COIN_DBL_MAX);
                    numberRows++;
               }
               model2->addColumns(numberArtificials, NULL, NULL, addCost,
                                  addStarts, addRow, addElement);
          }
          delete [] addStarts;
          delete [] addRow;
          delete [] addElement;
          delete [] addCost;
          // look at rhs to see if to perturb
          double largest = 0.0;
          double smallest = 1.0e30;
          for (iRow = 0; iRow < numberRows; iRow++) {
               double value;
               value = fabs(model2->rowLower_[iRow]);
               if (value && value < 1.0e30) {
                    largest = CoinMax(largest, value);
                    smallest = CoinMin(smallest, value);
               }
               value = fabs(model2->rowUpper_[iRow]);
               if (value && value < 1.0e30) {
                    largest = CoinMax(largest, value);
                    smallest = CoinMin(smallest, value);
               }
          }
          double * saveLower = NULL;
          double * saveUpper = NULL;
          if (largest < 2.01 * smallest) {
               // perturb - so switch off standard
               model2->setPerturbation(100);
               saveLower = new double[numberRows];
               CoinMemcpyN(model2->rowLower_, numberRows, saveLower);
               saveUpper = new double[numberRows];
               CoinMemcpyN(model2->rowUpper_, numberRows, saveUpper);
               double * lower = model2->rowLower();
               double * upper = model2->rowUpper();
               for (iRow = 0; iRow < numberRows; iRow++) {
                    double lowerValue = lower[iRow], upperValue = upper[iRow];
                    double value = randomNumberGenerator_.randomDouble();
                    if (upperValue > lowerValue + primalTolerance_) {
                         if (lowerValue > -1.0e20 && lowerValue)
                              lowerValue -= value * 1.0e-4 * fabs(lowerValue);
                         if (upperValue < 1.0e20 && upperValue)
                              upperValue += value * 1.0e-4 * fabs(upperValue);
                    } else if (upperValue > 0.0) {
                         upperValue -= value * 1.0e-4 * fabs(lowerValue);
                         lowerValue -= value * 1.0e-4 * fabs(lowerValue);
                    } else if (upperValue < 0.0) {
                         upperValue += value * 1.0e-4 * fabs(lowerValue);
                         lowerValue += value * 1.0e-4 * fabs(lowerValue);
                    } else {
                    }
                    lower[iRow] = lowerValue;
                    upper[iRow] = upperValue;
               }
          }
          int i;
          // Just do this number of passes in Sprint
          if (doSprint > 0)
               maxSprintPass = options.getExtraInfo(1);
          // but if big use to get ratio
          double ratio = 3;
          if (maxSprintPass > 1000) {
               ratio = static_cast<double> (maxSprintPass) * 0.0001;
               ratio = CoinMax(ratio, 1.1);
               maxSprintPass = maxSprintPass % 1000;
#ifdef COIN_DEVELOP
               printf("%d passes wanted with ratio of %g\n", maxSprintPass, ratio);
#endif
          }
          // Just take this number of columns in small problem
          int smallNumberColumns = static_cast<int> (CoinMin(ratio * numberRows, static_cast<double> (numberColumns)));
          smallNumberColumns = CoinMax(smallNumberColumns, 3000);
          smallNumberColumns = CoinMin(smallNumberColumns, numberColumns);
          //int smallNumberColumns = CoinMin(12*numberRows/10,numberColumns);
          //smallNumberColumns = CoinMax(smallNumberColumns,3000);
          //smallNumberColumns = CoinMax(smallNumberColumns,numberRows+1000);
          // redo as may have changed
          columnLower = model2->columnLower();
          columnUpper = model2->columnUpper();
          columnSolution = model2->primalColumnSolution();
          // Set up initial list
          numberSort = 0;
          if (numberArtificials) {
               numberSort = numberArtificials;
               for (i = 0; i < numberSort; i++)
                    sort[i] = i + originalNumberColumns;
          }
          // maybe a solution there already
          for (iColumn = 0; iColumn < originalNumberColumns; iColumn++) {
               if (model2->getColumnStatus(iColumn) == basic)
                    sort[numberSort++] = iColumn;
          }
          for (iColumn = 0; iColumn < originalNumberColumns; iColumn++) {
               if (model2->getColumnStatus(iColumn) != basic) {
                    if (columnSolution[iColumn] > columnLower[iColumn] &&
                              columnSolution[iColumn] < columnUpper[iColumn] &&
                              columnSolution[iColumn])
                         sort[numberSort++] = iColumn;
               }
          }
          numberSort = CoinMin(numberSort, smallNumberColumns);

          int numberColumns = model2->numberColumns();
          double * fullSolution = model2->primalColumnSolution();


          int iPass;
          double lastObjective[] = {1.0e31,1.0e31};
          // It will be safe to allow dense
          model2->setInitialDenseFactorization(true);

          // We will be using all rows
          int * whichRows = new int [numberRows];
          for (iRow = 0; iRow < numberRows; iRow++)
               whichRows[iRow] = iRow;
          double originalOffset;
          model2->getDblParam(ClpObjOffset, originalOffset);
          int totalIterations = 0;
          double lastSumArtificials = COIN_DBL_MAX;
          int originalMaxSprintPass = maxSprintPass;
          maxSprintPass = 20; // so we do that many if infeasible
          for (iPass = 0; iPass < maxSprintPass; iPass++) {
               //printf("Bug until submodel new version\n");
               //CoinSort_2(sort,sort+numberSort,weight);
               // Create small problem
               ClpSimplex small(model2, numberRows, whichRows, numberSort, sort);
               small.setPerturbation(model2->perturbation());
               small.setInfeasibilityCost(model2->infeasibilityCost());
               if (model2->factorizationFrequency() == 200) {
                    // User did not touch preset
                    small.defaultFactorizationFrequency();
               }
               // now see what variables left out do to row solution
               double * rowSolution = model2->primalRowSolution();
               double * sumFixed = new double[numberRows];
               CoinZeroN (sumFixed, numberRows);
               int iRow, iColumn;
               // zero out ones in small problem
               for (iColumn = 0; iColumn < numberSort; iColumn++) {
                    int kColumn = sort[iColumn];
                    fullSolution[kColumn] = 0.0;
               }
               // Get objective offset
               const double * objective = model2->objective();
               double offset = 0.0;
               for (iColumn = 0; iColumn < originalNumberColumns; iColumn++)
                    offset += fullSolution[iColumn] * objective[iColumn];
#if 0
	       // Set artificials to zero if first time close to zero
               for (iColumn = originalNumberColumns; iColumn < numberColumns; iColumn++) {
		 if (fullSolution[iColumn]<primalTolerance_&&objective[iColumn]==penalty) {
		   model2->objective()[iColumn]=2.0*penalty;
		   fullSolution[iColumn]=0.0;
		 }
	       }
#endif
               small.setDblParam(ClpObjOffset, originalOffset - offset);
               model2->clpMatrix()->times(1.0, fullSolution, sumFixed);

               double * lower = small.rowLower();
               double * upper = small.rowUpper();
               for (iRow = 0; iRow < numberRows; iRow++) {
                    if (lower[iRow] > -1.0e50)
                         lower[iRow] -= sumFixed[iRow];
                    if (upper[iRow] < 1.0e50)
                         upper[iRow] -= sumFixed[iRow];
                    rowSolution[iRow] -= sumFixed[iRow];
               }
               delete [] sumFixed;
               // Solve
               if (interrupt)
                    currentModel = &small;
               small.defaultFactorizationFrequency();
               if (dynamic_cast< ClpPackedMatrix*>(matrix_)) {
                    // See if original wanted vector
                    ClpPackedMatrix * clpMatrixO = dynamic_cast< ClpPackedMatrix*>(matrix_);
                    ClpMatrixBase * matrix = small.clpMatrix();
                    if (dynamic_cast< ClpPackedMatrix*>(matrix) && clpMatrixO->wantsSpecialColumnCopy()) {
                         ClpPackedMatrix * clpMatrix = dynamic_cast< ClpPackedMatrix*>(matrix);
                         clpMatrix->makeSpecialColumnCopy();
                         small.primal(1);
                         clpMatrix->releaseSpecialColumnCopy();
                    } else {
#if 1
#ifdef ABC_INHERIT
		      //small.writeMps("try.mps");
		      if (iPass||!numberArtificials) 
		         small.dealWithAbc(1,1);
		       else 
		         small.dealWithAbc(0,0);
#else
		      if (iPass||!numberArtificials) 
		         small.primal(1);
		      else
		         small.dual(0);
#endif
		      if (small.problemStatus())
			small.dual(0);
#else
                         int numberColumns = small.numberColumns();
                         int numberRows = small.numberRows();
                         // Use dual region
                         double * rhs = small.dualRowSolution();
                         int * whichRow = new int[3*numberRows];
                         int * whichColumn = new int[2*numberColumns];
                         int nBound;
                         ClpSimplex * small2 = ((ClpSimplexOther *) (&small))->crunch(rhs, whichRow, whichColumn,
                                               nBound, false, false);
                         if (small2) {
#ifdef ABC_INHERIT
                              small2->dealWithAbc(1,1);
#else
			      small.primal(1);
#endif
                              if (small2->problemStatus() == 0) {
                                   small.setProblemStatus(0);
                                   ((ClpSimplexOther *) (&small))->afterCrunch(*small2, whichRow, whichColumn, nBound);
                              } else {
#ifdef ABC_INHERIT
                                   small2->dealWithAbc(1,1);
#else
				   small.primal(1);
#endif
                                   if (small2->problemStatus())
                                        small.primal(1);
                              }
                              delete small2;
                         } else {
                              small.primal(1);
                         }
                         delete [] whichRow;
                         delete [] whichColumn;
#endif
                    }
               } else {
                    small.primal(1);
               }
               totalIterations += small.numberIterations();
               // move solution back
               const double * solution = small.primalColumnSolution();
               for (iColumn = 0; iColumn < numberSort; iColumn++) {
                    int kColumn = sort[iColumn];
                    model2->setColumnStatus(kColumn, small.getColumnStatus(iColumn));
                    fullSolution[kColumn] = solution[iColumn];
               }
               for (iRow = 0; iRow < numberRows; iRow++)
                    model2->setRowStatus(iRow, small.getRowStatus(iRow));
               CoinMemcpyN(small.primalRowSolution(),
                           numberRows, model2->primalRowSolution());
               double sumArtificials = 0.0;
               for (i = 0; i < numberArtificials; i++)
                    sumArtificials += fullSolution[i + originalNumberColumns];
               if (sumArtificials && iPass > 5 && sumArtificials >= lastSumArtificials) {
                    // increase costs
                    double * cost = model2->objective() + originalNumberColumns;
                    double newCost = CoinMin(1.0e10, cost[0] * 1.5);
                    for (i = 0; i < numberArtificials; i++)
                         cost[i] = newCost;
               }
               lastSumArtificials = sumArtificials;
               // get reduced cost for large problem
               double * djs = model2->dualColumnSolution();
               CoinMemcpyN(model2->objective(), numberColumns, djs);
               model2->clpMatrix()->transposeTimes(-1.0, small.dualRowSolution(), djs);
               int numberNegative = 0;
               double sumNegative = 0.0;
               // now massage weight so all basic in plus good djs
               // first count and do basic
               numberSort = 0;
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    double dj = djs[iColumn] * optimizationDirection_;
                    double value = fullSolution[iColumn];
                    if (model2->getColumnStatus(iColumn) == ClpSimplex::basic) {
                         sort[numberSort++] = iColumn;
                    } else if (dj < -dualTolerance_ && value < columnUpper[iColumn]) {
                         numberNegative++;
                         sumNegative -= dj;
                    } else if (dj > dualTolerance_ && value > columnLower[iColumn]) {
                         numberNegative++;
                         sumNegative += dj;
                    }
               }
               handler_->message(CLP_SPRINT, messages_)
                         << iPass + 1 << small.numberIterations() << small.objectiveValue() << sumNegative
                         << numberNegative
                         << CoinMessageEol;
               if (sumArtificials < 1.0e-8 && originalMaxSprintPass >= 0) {
                    maxSprintPass = iPass + originalMaxSprintPass;
                    originalMaxSprintPass = -1;
               }
               if (iPass > 20)
                    sumArtificials = 0.0;
               if ((small.objectiveValue()*optimizationDirection_ > lastObjective[1] - 1.0e-7 && iPass > 5 && sumArtificials < 1.0e-8) ||
                         (!small.numberIterations() && iPass) ||
                         iPass == maxSprintPass - 1 || small.status() == 3) {

                    break; // finished
               } else {
		    lastObjective[1] = lastObjective[0];
                    lastObjective[0] = small.objectiveValue() * optimizationDirection_;
                    double tolerance;
                    double averageNegDj = sumNegative / static_cast<double> (numberNegative + 1);
                    if (numberNegative + numberSort > smallNumberColumns)
                         tolerance = -dualTolerance_;
                    else
                         tolerance = 10.0 * averageNegDj;
                    int saveN = numberSort;
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         double dj = djs[iColumn] * optimizationDirection_;
                         double value = fullSolution[iColumn];
                         if (model2->getColumnStatus(iColumn) != ClpSimplex::basic) {
                              if (dj < -dualTolerance_ && value < columnUpper[iColumn])
                                   dj = dj;
                              else if (dj > dualTolerance_ && value > columnLower[iColumn])
                                   dj = -dj;
                              else if (columnUpper[iColumn] > columnLower[iColumn])
                                   dj = fabs(dj);
                              else
                                   dj = 1.0e50;
                              if (dj < tolerance) {
                                   weight[numberSort] = dj;
                                   sort[numberSort++] = iColumn;
                              }
                         }
                    }
                    // sort
                    CoinSort_2(weight + saveN, weight + numberSort, sort + saveN);
                    numberSort = CoinMin(smallNumberColumns, numberSort);
               }
          }
          if (interrupt)
               currentModel = model2;
          for (i = 0; i < numberArtificials; i++)
               sort[i] = i + originalNumberColumns;
          model2->deleteColumns(numberArtificials, sort);
          if (network) {
               int iRow = numberRows - 1;
               model2->deleteRows(1, &iRow);
          }
          delete [] weight;
          delete [] sort;
          delete [] whichRows;
          if (saveLower) {
               // unperturb and clean
               for (iRow = 0; iRow < numberRows; iRow++) {
                    double diffLower = saveLower[iRow] - model2->rowLower_[iRow];
                    double diffUpper = saveUpper[iRow] - model2->rowUpper_[iRow];
                    model2->rowLower_[iRow] = saveLower[iRow];
                    model2->rowUpper_[iRow] = saveUpper[iRow];
                    if (diffLower)
                         assert (!diffUpper || fabs(diffLower - diffUpper) < 1.0e-5);
                    else
                         diffLower = diffUpper;
                    model2->rowActivity_[iRow] += diffLower;
               }
               delete [] saveLower;
               delete [] saveUpper;
          }
#ifdef ABC_INHERIT
          model2->dealWithAbc(1,1);
#else
          model2->primal(1);
#endif
          model2->setPerturbation(savePerturbation);
          if (model2 != originalModel2) {
               originalModel2->moveInfo(*model2);
               delete model2;
               model2 = originalModel2;
          }
          time2 = CoinCpuTime();
          timeCore = time2 - timeX;
          handler_->message(CLP_INTERVAL_TIMING, messages_)
                    << "Sprint" << timeCore << time2 - time1
                    << CoinMessageEol;
          timeX = time2;
          model2->setNumberIterations(model2->numberIterations() + totalIterations);
     } else if (method == ClpSolve::useBarrier || method == ClpSolve::useBarrierNoCross) {
#ifndef SLIM_CLP
          //printf("***** experimental pretty crude barrier\n");
          //#define SAVEIT 2
#ifndef SAVEIT
#define BORROW
#endif
#ifdef BORROW
          ClpInterior barrier;
          barrier.borrowModel(*model2);
#else
          ClpInterior barrier(*model2);
#endif
          if (interrupt)
               currentModel2 = &barrier;
	  if (barrier.numberRows()+barrier.numberColumns()>10000)
	    barrier.setMaximumBarrierIterations(1000);
          int barrierOptions = options.getSpecialOption(4);
          int aggressiveGamma = 0;
          bool presolveInCrossover = false;
          bool scale = false;
          bool doKKT = false;
          bool forceFixing = false;
          int speed = 0;
          if (barrierOptions & 16) {
               barrierOptions &= ~16;
               doKKT = true;
          }
          if (barrierOptions&(32 + 64 + 128)) {
               aggressiveGamma = (barrierOptions & (32 + 64 + 128)) >> 5;
               barrierOptions &= ~(32 + 64 + 128);
          }
          if (barrierOptions & 256) {
               barrierOptions &= ~256;
               presolveInCrossover = true;
          }
          if (barrierOptions & 512) {
               barrierOptions &= ~512;
               forceFixing = true;
          }
          if (barrierOptions & 1024) {
               barrierOptions &= ~1024;
               barrier.setProjectionTolerance(1.0e-9);
          }
          if (barrierOptions&(2048 | 4096)) {
               speed = (barrierOptions & (2048 | 4096)) >> 11;
               barrierOptions &= ~(2048 | 4096);
          }
          if (barrierOptions & 8) {
               barrierOptions &= ~8;
               scale = true;
          }
          // If quadratic force KKT
          if (quadraticObj) {
               doKKT = true;
          }
          switch (barrierOptions) {
          case 0:
          default:
               if (!doKKT) {
                    ClpCholeskyBase * cholesky = new ClpCholeskyBase(options.getExtraInfo(1));
                    cholesky->setIntegerParameter(0, speed);
                    barrier.setCholesky(cholesky);
               } else {
                    ClpCholeskyBase * cholesky = new ClpCholeskyBase();
                    cholesky->setKKT(true);
                    barrier.setCholesky(cholesky);
               }
               break;
          case 1:
               if (!doKKT) {
                    ClpCholeskyDense * cholesky = new ClpCholeskyDense();
                    barrier.setCholesky(cholesky);
               } else {
                    ClpCholeskyDense * cholesky = new ClpCholeskyDense();
                    cholesky->setKKT(true);
                    barrier.setCholesky(cholesky);
               }
               break;
#ifdef COIN_HAS_WSMP
          case 2: {
               ClpCholeskyWssmp * cholesky = new ClpCholeskyWssmp(CoinMax(100, model2->numberRows() / 10));
               barrier.setCholesky(cholesky);
               assert (!doKKT);
          }
          break;
          case 3:
               if (!doKKT) {
                    ClpCholeskyWssmp * cholesky = new ClpCholeskyWssmp();
                    barrier.setCholesky(cholesky);
               } else {
                    ClpCholeskyWssmpKKT * cholesky = new ClpCholeskyWssmpKKT(CoinMax(100, model2->numberRows() / 10));
                    barrier.setCholesky(cholesky);
               }
               break;
#endif
#ifdef UFL_BARRIER
          case 4:
               if (!doKKT) {
                    ClpCholeskyUfl * cholesky = new ClpCholeskyUfl();
                    barrier.setCholesky(cholesky);
               } else {
                    ClpCholeskyUfl * cholesky = new ClpCholeskyUfl();
                    cholesky->setKKT(true);
                    barrier.setCholesky(cholesky);
               }
               break;
#endif
#ifdef TAUCS_BARRIER
          case 5: {
               ClpCholeskyTaucs * cholesky = new ClpCholeskyTaucs();
               barrier.setCholesky(cholesky);
               assert (!doKKT);
          }
          break;
#endif
#ifdef COIN_HAS_MUMPS
          case 6: {
               ClpCholeskyMumps * cholesky = new ClpCholeskyMumps();
               barrier.setCholesky(cholesky);
               assert (!doKKT);
          }
          break;
#endif
          }
          int numberRows = model2->numberRows();
          int numberColumns = model2->numberColumns();
          int saveMaxIts = model2->maximumIterations();
          if (saveMaxIts < 1000) {
               barrier.setMaximumBarrierIterations(saveMaxIts);
               model2->setMaximumIterations(10000000);
          }
#ifndef SAVEIT
          //barrier.setDiagonalPerturbation(1.0e-25);
          if (aggressiveGamma) {
               switch (aggressiveGamma) {
               case 1:
                    barrier.setGamma(1.0e-5);
                    barrier.setDelta(1.0e-5);
                    break;
               case 2:
                    barrier.setGamma(1.0e-7);
                    break;
               case 3:
                    barrier.setDelta(1.0e-5);
                    break;
               case 4:
                    barrier.setGamma(1.0e-3);
                    barrier.setDelta(1.0e-3);
                    break;
               case 5:
                    barrier.setGamma(1.0e-3);
                    break;
               case 6:
                    barrier.setDelta(1.0e-3);
                    break;
               }
          }
          if (scale)
               barrier.scaling(1);
          else
               barrier.scaling(0);
          barrier.primalDual();
#elif SAVEIT==1
          barrier.primalDual();
#else
          model2->restoreModel("xx.save");
          // move solutions
          CoinMemcpyN(model2->primalRowSolution(),
                      numberRows, barrier.primalRowSolution());
          CoinMemcpyN(model2->dualRowSolution(),
                      numberRows, barrier.dualRowSolution());
          CoinMemcpyN(model2->primalColumnSolution(),
                      numberColumns, barrier.primalColumnSolution());
          CoinMemcpyN(model2->dualColumnSolution(),
                      numberColumns, barrier.dualColumnSolution());
#endif
          time2 = CoinCpuTime();
          timeCore = time2 - timeX;
          handler_->message(CLP_INTERVAL_TIMING, messages_)
                    << "Barrier" << timeCore << time2 - time1
                    << CoinMessageEol;
          timeX = time2;
          int maxIts = barrier.maximumBarrierIterations();
          int barrierStatus = barrier.status();
          double gap = barrier.complementarityGap();
          // get which variables are fixed
          double * saveLower = NULL;
          double * saveUpper = NULL;
          ClpPresolve pinfo2;
          ClpSimplex * saveModel2 = NULL;
          bool extraPresolve = false;
          int numberFixed = barrier.numberFixed();
          if (numberFixed) {
               int numberRows = barrier.numberRows();
               int numberColumns = barrier.numberColumns();
               int numberTotal = numberRows + numberColumns;
               saveLower = new double [numberTotal];
               saveUpper = new double [numberTotal];
               CoinMemcpyN(barrier.columnLower(), numberColumns, saveLower);
               CoinMemcpyN(barrier.rowLower(), numberRows, saveLower + numberColumns);
               CoinMemcpyN(barrier.columnUpper(), numberColumns, saveUpper);
               CoinMemcpyN(barrier.rowUpper(), numberRows, saveUpper + numberColumns);
          }
          if (((numberFixed * 20 > barrier.numberRows() && numberFixed > 5000) || forceFixing) &&
                    presolveInCrossover) {
               // may as well do presolve
               if (!forceFixing) {
                    barrier.fixFixed();
               } else {
                    // Fix
                    int n = barrier.numberColumns();
                    double * lower = barrier.columnLower();
                    double * upper = barrier.columnUpper();
                    double * solution = barrier.primalColumnSolution();
                    int nFix = 0;
                    for (int i = 0; i < n; i++) {
                         if (barrier.fixedOrFree(i) && lower[i] < upper[i]) {
                              double value = solution[i];
                              if (value < lower[i] + 1.0e-6 && value - lower[i] < upper[i] - value) {
                                   solution[i] = lower[i];
                                   upper[i] = lower[i];
                                   nFix++;
                              } else if (value > upper[i] - 1.0e-6 && value - lower[i] > upper[i] - value) {
                                   solution[i] = upper[i];
                                   lower[i] = upper[i];
                                   nFix++;
                              }
                         }
                    }
#ifdef CLP_INVESTIGATE
                    printf("%d columns fixed\n", nFix);
#endif
                    int nr = barrier.numberRows();
                    lower = barrier.rowLower();
                    upper = barrier.rowUpper();
                    solution = barrier.primalRowSolution();
                    nFix = 0;
                    for (int i = 0; i < nr; i++) {
                         if (barrier.fixedOrFree(i + n) && lower[i] < upper[i]) {
                              double value = solution[i];
                              if (value < lower[i] + 1.0e-6 && value - lower[i] < upper[i] - value) {
                                   solution[i] = lower[i];
                                   upper[i] = lower[i];
                                   nFix++;
                              } else if (value > upper[i] - 1.0e-6 && value - lower[i] > upper[i] - value) {
                                   solution[i] = upper[i];
                                   lower[i] = upper[i];
                                   nFix++;
                              }
                         }
                    }
#ifdef CLP_INVESTIGATE
                    printf("%d row slacks fixed\n", nFix);
#endif
               }
               saveModel2 = model2;
               extraPresolve = true;
          } else if (numberFixed) {
               // Set fixed to bounds (may have restored earlier solution)
               if (!forceFixing) {
                    barrier.fixFixed(false);
               } else {
                    // Fix
                    int n = barrier.numberColumns();
                    double * lower = barrier.columnLower();
                    double * upper = barrier.columnUpper();
                    double * solution = barrier.primalColumnSolution();
                    int nFix = 0;
                    for (int i = 0; i < n; i++) {
                         if (barrier.fixedOrFree(i) && lower[i] < upper[i]) {
                              double value = solution[i];
                              if (value < lower[i] + 1.0e-8 && value - lower[i] < upper[i] - value) {
                                   solution[i] = lower[i];
                                   upper[i] = lower[i];
                                   nFix++;
                              } else if (value > upper[i] - 1.0e-8 && value - lower[i] > upper[i] - value) {
                                   solution[i] = upper[i];
                                   lower[i] = upper[i];
                                   nFix++;
                              } else {
                                   //printf("fixcol %d %g <= %g <= %g\n",
                                   //     i,lower[i],solution[i],upper[i]);
                              }
                         }
                    }
#ifdef CLP_INVESTIGATE
                    printf("%d columns fixed\n", nFix);
#endif
                    int nr = barrier.numberRows();
                    lower = barrier.rowLower();
                    upper = barrier.rowUpper();
                    solution = barrier.primalRowSolution();
                    nFix = 0;
                    for (int i = 0; i < nr; i++) {
                         if (barrier.fixedOrFree(i + n) && lower[i] < upper[i]) {
                              double value = solution[i];
                              if (value < lower[i] + 1.0e-5 && value - lower[i] < upper[i] - value) {
                                   solution[i] = lower[i];
                                   upper[i] = lower[i];
                                   nFix++;
                              } else if (value > upper[i] - 1.0e-5 && value - lower[i] > upper[i] - value) {
                                   solution[i] = upper[i];
                                   lower[i] = upper[i];
                                   nFix++;
                              } else {
                                   //printf("fixrow %d %g <= %g <= %g\n",
                                   //     i,lower[i],solution[i],upper[i]);
                              }
                         }
                    }
#ifdef CLP_INVESTIGATE
                    printf("%d row slacks fixed\n", nFix);
#endif
               }
          }
#ifdef BORROW
	  int saveNumberIterations = barrier.numberIterations();
          barrier.returnModel(*model2);
          double * rowPrimal = new double [numberRows];
          double * columnPrimal = new double [numberColumns];
          double * rowDual = new double [numberRows];
          double * columnDual = new double [numberColumns];
          // move solutions other way
          CoinMemcpyN(model2->primalRowSolution(),
                      numberRows, rowPrimal);
          CoinMemcpyN(model2->dualRowSolution(),
                      numberRows, rowDual);
          CoinMemcpyN(model2->primalColumnSolution(),
                      numberColumns, columnPrimal);
          CoinMemcpyN(model2->dualColumnSolution(),
                      numberColumns, columnDual);
#else
          double * rowPrimal = barrier.primalRowSolution();
          double * columnPrimal = barrier.primalColumnSolution();
          double * rowDual = barrier.dualRowSolution();
          double * columnDual = barrier.dualColumnSolution();
          // move solutions
          CoinMemcpyN(rowPrimal,
                      numberRows, model2->primalRowSolution());
          CoinMemcpyN(rowDual,
                      numberRows, model2->dualRowSolution());
          CoinMemcpyN(columnPrimal,
                      numberColumns, model2->primalColumnSolution());
          CoinMemcpyN(columnDual,
                      numberColumns, model2->dualColumnSolution());
#endif
          if (saveModel2) {
               // do presolve
               model2 = pinfo2.presolvedModel(*model2, dblParam_[ClpPresolveTolerance],
                                              false, 5, true);
               if (!model2) {
                    model2 = saveModel2;
                    saveModel2 = NULL;
                    int numberRows = model2->numberRows();
                    int numberColumns = model2->numberColumns();
                    CoinMemcpyN(saveLower, numberColumns, model2->columnLower());
                    CoinMemcpyN(saveLower + numberColumns, numberRows, model2->rowLower());
                    delete [] saveLower;
                    CoinMemcpyN(saveUpper, numberColumns, model2->columnUpper());
                    CoinMemcpyN(saveUpper + numberColumns, numberRows, model2->rowUpper());
                    delete [] saveUpper;
                    saveLower = NULL;
                    saveUpper = NULL;
               }
          }
          if (method == ClpSolve::useBarrier || barrierStatus < 0) {
               if (maxIts && barrierStatus < 4 && !quadraticObj) {
                    //printf("***** crossover - needs more thought on difficult models\n");
#if SAVEIT==1
                    model2->ClpSimplex::saveModel("xx.save");
#endif
                    // make sure no status left
                    model2->createStatus();
                    // solve
                    if (!forceFixing)
                         model2->setPerturbation(100);
                    if (model2->factorizationFrequency() == 200) {
                         // User did not touch preset
                         model2->defaultFactorizationFrequency();
                    }
#if 1 //ndef ABC_INHERIT //#if 1
                    // throw some into basis
                    if(!forceFixing) {
                         int numberRows = model2->numberRows();
                         int numberColumns = model2->numberColumns();
                         double * dsort = new double[numberColumns];
                         int * sort = new int[numberColumns];
                         int n = 0;
                         const double * columnLower = model2->columnLower();
                         const double * columnUpper = model2->columnUpper();
                         double * primalSolution = model2->primalColumnSolution();
                         const double * dualSolution = model2->dualColumnSolution();
                         double tolerance = 10.0 * primalTolerance_;
                         int i;
                         for ( i = 0; i < numberRows; i++)
                              model2->setRowStatus(i, superBasic);
                         for ( i = 0; i < numberColumns; i++) {
                              double distance = CoinMin(columnUpper[i] - primalSolution[i],
                                                        primalSolution[i] - columnLower[i]);
                              if (distance > tolerance) {
                                   if (fabs(dualSolution[i]) < 1.0e-5)
                                        distance *= 100.0;
                                   dsort[n] = -distance;
                                   sort[n++] = i;
                                   model2->setStatus(i, superBasic);
                              } else if (distance > primalTolerance_) {
                                   model2->setStatus(i, superBasic);
                              } else if (primalSolution[i] <= columnLower[i] + primalTolerance_) {
                                   model2->setStatus(i, atLowerBound);
                                   primalSolution[i] = columnLower[i];
                              } else {
                                   model2->setStatus(i, atUpperBound);
                                   primalSolution[i] = columnUpper[i];
                              }
                         }
                         CoinSort_2(dsort, dsort + n, sort);
                         n = CoinMin(numberRows, n);
                         for ( i = 0; i < n; i++) {
                              int iColumn = sort[i];
                              model2->setStatus(iColumn, basic);
                         }
                         delete [] sort;
                         delete [] dsort;
                         // model2->allSlackBasis();
                         if (gap < 1.0e-3 * static_cast<double> (numberRows + numberColumns)) {
                              if (saveUpper) {
                                   int numberRows = model2->numberRows();
                                   int numberColumns = model2->numberColumns();
                                   CoinMemcpyN(saveLower, numberColumns, model2->columnLower());
                                   CoinMemcpyN(saveLower + numberColumns, numberRows, model2->rowLower());
                                   CoinMemcpyN(saveUpper, numberColumns, model2->columnUpper());
                                   CoinMemcpyN(saveUpper + numberColumns, numberRows, model2->rowUpper());
                                   delete [] saveLower;
                                   delete [] saveUpper;
                                   saveLower = NULL;
                                   saveUpper = NULL;
                              }
                              int numberRows = model2->numberRows();
                              int numberColumns = model2->numberColumns();
#ifdef ABC_INHERIT
			      model2->checkSolution(0);
			      printf("%d primal infeasibilities summing to %g\n",
				     model2->numberPrimalInfeasibilities(),
				     model2->sumPrimalInfeasibilities());
			      model2->dealWithAbc(1,1);
			 }
		    }
#else
                              // just primal values pass
                              double saveScale = model2->objectiveScale();
                              model2->setObjectiveScale(1.0e-3);
                              model2->primal(2);
                              model2->setObjectiveScale(saveScale);
                              // save primal solution and copy back dual
                              CoinMemcpyN(model2->primalRowSolution(),
                                          numberRows, rowPrimal);
                              CoinMemcpyN(rowDual,
                                          numberRows, model2->dualRowSolution());
                              CoinMemcpyN(model2->primalColumnSolution(),
                                          numberColumns, columnPrimal);
                              CoinMemcpyN(columnDual,
                                          numberColumns, model2->dualColumnSolution());
                              //model2->primal(1);
                              // clean up reduced costs and flag variables
                              {
                                   double * dj = model2->dualColumnSolution();
                                   double * cost = model2->objective();
                                   double * saveCost = new double[numberColumns];
                                   CoinMemcpyN(cost, numberColumns, saveCost);
                                   double * saveLower = new double[numberColumns];
                                   double * lower = model2->columnLower();
                                   CoinMemcpyN(lower, numberColumns, saveLower);
                                   double * saveUpper = new double[numberColumns];
                                   double * upper = model2->columnUpper();
                                   CoinMemcpyN(upper, numberColumns, saveUpper);
                                   int i;
                                   double tolerance = 10.0 * dualTolerance_;
                                   for ( i = 0; i < numberColumns; i++) {
                                        if (model2->getStatus(i) == basic) {
                                             dj[i] = 0.0;
                                        } else if (model2->getStatus(i) == atLowerBound) {
                                             if (optimizationDirection_ * dj[i] < tolerance) {
                                                  if (optimizationDirection_ * dj[i] < 0.0) {
                                                       //if (dj[i]<-1.0e-3)
                                                       //printf("bad dj at lb %d %g\n",i,dj[i]);
                                                       cost[i] -= dj[i];
                                                       dj[i] = 0.0;
                                                  }
                                             } else {
                                                  upper[i] = lower[i];
                                             }
                                        } else if (model2->getStatus(i) == atUpperBound) {
                                             if (optimizationDirection_ * dj[i] > tolerance) {
                                                  if (optimizationDirection_ * dj[i] > 0.0) {
                                                       //if (dj[i]>1.0e-3)
                                                       //printf("bad dj at ub %d %g\n",i,dj[i]);
                                                       cost[i] -= dj[i];
                                                       dj[i] = 0.0;
                                                  }
                                             } else {
                                                  lower[i] = upper[i];
                                             }
                                        }
                                   }
                                   // just dual values pass
                                   //model2->setLogLevel(63);
                                   //model2->setFactorizationFrequency(1);
                                   model2->dual(2);
                                   CoinMemcpyN(saveCost, numberColumns, cost);
                                   delete [] saveCost;
                                   CoinMemcpyN(saveLower, numberColumns, lower);
                                   delete [] saveLower;
                                   CoinMemcpyN(saveUpper, numberColumns, upper);
                                   delete [] saveUpper;
                              }
                         }
                         // and finish
                         // move solutions
                         CoinMemcpyN(rowPrimal,
                                     numberRows, model2->primalRowSolution());
                         CoinMemcpyN(columnPrimal,
                                     numberColumns, model2->primalColumnSolution());
                    }
                    double saveScale = model2->objectiveScale();
                    model2->setObjectiveScale(1.0e-3);
                    model2->primal(2);
                    model2->setObjectiveScale(saveScale);
                    model2->primal(1);
#endif
#else
                    // just primal
#ifdef ABC_INHERIT
		    model2->checkSolution(0);
		    printf("%d primal infeasibilities summing to %g\n",
			   model2->numberPrimalInfeasibilities(),
			   model2->sumPrimalInfeasibilities());
		    model2->dealWithAbc(1,1);
#else
		    model2->primal(1);
#endif
		    //model2->primal(1);
#endif
               } else if (barrierStatus == 4) {
                    // memory problems
                    model2->setPerturbation(savePerturbation);
                    model2->createStatus();
                    model2->dual();
               } else if (maxIts && quadraticObj) {
                    // make sure no status left
                    model2->createStatus();
                    // solve
                    model2->setPerturbation(100);
                    model2->reducedGradient(1);
               }
          }

          //model2->setMaximumIterations(saveMaxIts);
#ifdef BORROW
          model2->setNumberIterations(model2->numberIterations()+saveNumberIterations);
          delete [] rowPrimal;
          delete [] columnPrimal;
          delete [] rowDual;
          delete [] columnDual;
#endif
          if (extraPresolve) {
               pinfo2.postsolve(true);
               delete model2;
               model2 = saveModel2;
          }
          if (saveUpper) {
               if (!forceFixing) {
                    int numberRows = model2->numberRows();
                    int numberColumns = model2->numberColumns();
                    CoinMemcpyN(saveLower, numberColumns, model2->columnLower());
                    CoinMemcpyN(saveLower + numberColumns, numberRows, model2->rowLower());
                    CoinMemcpyN(saveUpper, numberColumns, model2->columnUpper());
                    CoinMemcpyN(saveUpper + numberColumns, numberRows, model2->rowUpper());
               }
               delete [] saveLower;
               delete [] saveUpper;
               saveLower = NULL;
               saveUpper = NULL;
               if (method != ClpSolve::useBarrierNoCross)
                    model2->primal(1);
          }
          model2->setPerturbation(savePerturbation);
          time2 = CoinCpuTime();
          timeCore = time2 - timeX;
          handler_->message(CLP_INTERVAL_TIMING, messages_)
                    << "Crossover" << timeCore << time2 - time1
                    << CoinMessageEol;
          timeX = time2;
#else
          abort();
#endif
     } else {
          assert (method != ClpSolve::automatic); // later
          time2 = 0.0;
     }
     if (saveMatrix) {
          if (model2 == this) {
               // delete and replace
               delete model2->clpMatrix();
               model2->replaceMatrix(saveMatrix);
          } else {
               delete saveMatrix;
          }
     }
     numberIterations = model2->numberIterations();
     finalStatus = model2->status();
     int finalSecondaryStatus = model2->secondaryStatus();
     if (presolve == ClpSolve::presolveOn) {
          int saveLevel = logLevel();
          if ((specialOptions_ & 1024) == 0)
               setLogLevel(CoinMin(1, saveLevel));
          else
               setLogLevel(CoinMin(0, saveLevel));
          pinfo->postsolve(true);
	  numberIterations_ = 0;
	  delete pinfo;
	  pinfo = NULL;
          factorization_->areaFactor(model2->factorization()->adjustedAreaFactor());
          time2 = CoinCpuTime();
          timePresolve += time2 - timeX;
          handler_->message(CLP_INTERVAL_TIMING, messages_)
                    << "Postsolve" << time2 - timeX << time2 - time1
                    << CoinMessageEol;
          timeX = time2;
          if (!presolveToFile) {
#if 1 //ndef ABC_INHERIT
               delete model2;
#else
               if (model2->abcSimplex())
		 delete model2->abcSimplex();
	       else
		 delete model2;
#endif
	  }
          if (interrupt)
               currentModel = this;
          // checkSolution(); already done by postSolve
          setLogLevel(saveLevel);
	  int oldStatus=problemStatus_;
	  setProblemStatus(finalStatus);
	  setSecondaryStatus(finalSecondaryStatus);
	  int rcode=eventHandler()->event(ClpEventHandler::presolveAfterFirstSolve);
          if (finalStatus != 3 && rcode < 0 && (finalStatus || oldStatus == -1)) {
               int savePerturbation = perturbation();
               if (!finalStatus || (moreSpecialOptions_ & 2) == 0 ||
		   fabs(sumDualInfeasibilities_)+
		   fabs(sumPrimalInfeasibilities_)<1.0e-3) {
                    if (finalStatus == 2) {
                         // unbounded - get feasible first
                         double save = optimizationDirection_;
                         optimizationDirection_ = 0.0;
                         primal(1);
                         optimizationDirection_ = save;
                         primal(1);
                    } else if (finalStatus == 1) {
                         dual();
                    } else {
		        if ((moreSpecialOptions_&65536)==0) {
			  if (numberRows_<10000) 
			    setPerturbation(100); // probably better to perturb after n its
			  else if (savePerturbation<100)
			    setPerturbation(51); // probably better to perturb after n its
			}
#ifndef ABC_INHERIT
		        primal(1);
#else
			dealWithAbc(1,2,interrupt);
#endif
                    }
               } else {
                    // just set status
                    problemStatus_ = finalStatus;
               }
               setPerturbation(savePerturbation);
               numberIterations += numberIterations_;
               numberIterations_ = numberIterations;
               finalStatus = status();
               time2 = CoinCpuTime();
               handler_->message(CLP_INTERVAL_TIMING, messages_)
                         << "Cleanup" << time2 - timeX << time2 - time1
                         << CoinMessageEol;
               timeX = time2;
          } else if (rcode >= 0) {
#ifdef ABC_INHERIT
	    dealWithAbc(1,2,true);
#else
	    primal(1);
#endif
	  } else {
               secondaryStatus_ = finalSecondaryStatus;
          }
     } else if (model2 != this) {
          // not presolved - but different model used (sprint probably)
          CoinMemcpyN(model2->primalRowSolution(),
                      numberRows_, this->primalRowSolution());
          CoinMemcpyN(model2->dualRowSolution(),
                      numberRows_, this->dualRowSolution());
          CoinMemcpyN(model2->primalColumnSolution(),
                      numberColumns_, this->primalColumnSolution());
          CoinMemcpyN(model2->dualColumnSolution(),
                      numberColumns_, this->dualColumnSolution());
          CoinMemcpyN(model2->statusArray(),
                      numberColumns_ + numberRows_, this->statusArray());
          objectiveValue_ = model2->objectiveValue_;
          numberIterations_ = model2->numberIterations_;
          problemStatus_ = model2->problemStatus_;
          secondaryStatus_ = model2->secondaryStatus_;
          delete model2;
     }
     if (method != ClpSolve::useBarrierNoCross &&
               method != ClpSolve::useBarrier)
          setMaximumIterations(saveMaxIterations);
     std::string statusMessage[] = {"Unknown", "Optimal", "PrimalInfeasible", "DualInfeasible", "Stopped",
                                    "Errors", "User stopped"
                                   };
     assert (finalStatus >= -1 && finalStatus <= 5);
     numberIterations_ = numberIterations;
     handler_->message(CLP_TIMING, messages_)
               << statusMessage[finalStatus+1] << objectiveValue() << numberIterations << time2 - time1;
     handler_->printing(presolve == ClpSolve::presolveOn)
               << timePresolve;
     handler_->printing(timeIdiot != 0.0)
               << timeIdiot;
     handler_->message() << CoinMessageEol;
     if (interrupt)
          signal(SIGINT, saveSignal);
     perturbation_ = savePerturbation;
     scalingFlag_ = saveScaling;
     // If faking objective - put back correct one
     if (savedObjective) {
          delete objective_;
          objective_ = savedObjective;
     }
     if (options.getSpecialOption(1) == 2 &&
               options.getExtraInfo(1) > 1000000) {
          ClpObjective * savedObjective = objective_;
          // make up zero objective
          double * obj = new double[numberColumns_];
          for (int i = 0; i < numberColumns_; i++)
               obj[i] = 0.0;
          objective_ = new ClpLinearObjective(obj, numberColumns_);
          delete [] obj;
          primal(1);
          delete objective_;
          objective_ = savedObjective;
          finalStatus = status();
     }
     eventHandler()->event(ClpEventHandler::presolveEnd);
     delete pinfo;
     return finalStatus;
}
// General solve
int
ClpSimplex::initialSolve()
{
     // Default so use dual
     ClpSolve options;
     return initialSolve(options);
}
// General dual solve
int
ClpSimplex::initialDualSolve()
{
     ClpSolve options;
     // Use dual
     options.setSolveType(ClpSolve::useDual);
     return initialSolve(options);
}
// General primal solve
int
ClpSimplex::initialPrimalSolve()
{
     ClpSolve options;
     // Use primal
     options.setSolveType(ClpSolve::usePrimal);
     return initialSolve(options);
}
// barrier solve, not to be followed by crossover
int
ClpSimplex::initialBarrierNoCrossSolve()
{
     ClpSolve options;
     // Use primal
     options.setSolveType(ClpSolve::useBarrierNoCross);
     return initialSolve(options);
}

// General barrier solve
int
ClpSimplex::initialBarrierSolve()
{
     ClpSolve options;
     // Use primal
     options.setSolveType(ClpSolve::useBarrier);
     return initialSolve(options);
}

// Default constructor
ClpSolve::ClpSolve (  )
{
     method_ = automatic;
     presolveType_ = presolveOn;
     numberPasses_ = 5;
     int i;
     for (i = 0; i < 7; i++)
          options_[i] = 0;
     // say no +-1 matrix
     options_[3] = 1;
     for (i = 0; i < 7; i++)
          extraInfo_[i] = -1;
     independentOptions_[0] = 0;
     // But switch off slacks
     independentOptions_[1] = 512;
     // Substitute up to 3
     independentOptions_[2] = 3;

}
// Constructor when you really know what you are doing
ClpSolve::ClpSolve ( SolveType method, PresolveType presolveType,
                     int numberPasses, int options[6],
                     int extraInfo[6], int independentOptions[3])
{
     method_ = method;
     presolveType_ = presolveType;
     numberPasses_ = numberPasses;
     int i;
     for (i = 0; i < 6; i++)
          options_[i] = options[i];
     options_[6] = 0;
     for (i = 0; i < 6; i++)
          extraInfo_[i] = extraInfo[i];
     extraInfo_[6] = 0;
     for (i = 0; i < 3; i++)
          independentOptions_[i] = independentOptions[i];
}

// Copy constructor.
ClpSolve::ClpSolve(const ClpSolve & rhs)
{
     method_ = rhs.method_;
     presolveType_ = rhs.presolveType_;
     numberPasses_ = rhs.numberPasses_;
     int i;
     for ( i = 0; i < 7; i++)
          options_[i] = rhs.options_[i];
     for ( i = 0; i < 7; i++)
          extraInfo_[i] = rhs.extraInfo_[i];
     for (i = 0; i < 3; i++)
          independentOptions_[i] = rhs.independentOptions_[i];
}
// Assignment operator. This copies the data
ClpSolve &
ClpSolve::operator=(const ClpSolve & rhs)
{
     if (this != &rhs) {
          method_ = rhs.method_;
          presolveType_ = rhs.presolveType_;
          numberPasses_ = rhs.numberPasses_;
          int i;
          for (i = 0; i < 7; i++)
               options_[i] = rhs.options_[i];
          for (i = 0; i < 7; i++)
               extraInfo_[i] = rhs.extraInfo_[i];
          for (i = 0; i < 3; i++)
               independentOptions_[i] = rhs.independentOptions_[i];
     }
     return *this;

}
// Destructor
ClpSolve::~ClpSolve (  )
{
}
// See header file for details
void
ClpSolve::setSpecialOption(int which, int value, int extraInfo)
{
     options_[which] = value;
     extraInfo_[which] = extraInfo;
}
int
ClpSolve::getSpecialOption(int which) const
{
     return options_[which];
}

// Solve types
void
ClpSolve::setSolveType(SolveType method, int /*extraInfo*/)
{
     method_ = method;
}

ClpSolve::SolveType
ClpSolve::getSolveType()
{
     return method_;
}

// Presolve types
void
ClpSolve::setPresolveType(PresolveType amount, int extraInfo)
{
     presolveType_ = amount;
     numberPasses_ = extraInfo;
}
ClpSolve::PresolveType
ClpSolve::getPresolveType()
{
     return presolveType_;
}
// Extra info for idiot (or sprint)
int
ClpSolve::getExtraInfo(int which) const
{
     return extraInfo_[which];
}
int
ClpSolve::getPresolvePasses() const
{
     return numberPasses_;
}
/* Say to return at once if infeasible,
   default is to solve */
void
ClpSolve::setInfeasibleReturn(bool trueFalse)
{
     independentOptions_[0] = trueFalse ? 1 : 0;
}
#include <string>
// Generates code for above constructor
void
ClpSolve::generateCpp(FILE * fp)
{
     std::string solveType[] = {
          "ClpSolve::useDual",
          "ClpSolve::usePrimal",
          "ClpSolve::usePrimalorSprint",
          "ClpSolve::useBarrier",
          "ClpSolve::useBarrierNoCross",
          "ClpSolve::automatic",
          "ClpSolve::notImplemented"
     };
     std::string presolveType[] =  {
          "ClpSolve::presolveOn",
          "ClpSolve::presolveOff",
          "ClpSolve::presolveNumber",
          "ClpSolve::presolveNumberCost"
     };
     fprintf(fp, "3  ClpSolve::SolveType method = %s;\n", solveType[method_].c_str());
     fprintf(fp, "3  ClpSolve::PresolveType presolveType = %s;\n",
             presolveType[presolveType_].c_str());
     fprintf(fp, "3  int numberPasses = %d;\n", numberPasses_);
     fprintf(fp, "3  int options[] = {%d,%d,%d,%d,%d,%d};\n",
             options_[0], options_[1], options_[2],
             options_[3], options_[4], options_[5]);
     fprintf(fp, "3  int extraInfo[] = {%d,%d,%d,%d,%d,%d};\n",
             extraInfo_[0], extraInfo_[1], extraInfo_[2],
             extraInfo_[3], extraInfo_[4], extraInfo_[5]);
     fprintf(fp, "3  int independentOptions[] = {%d,%d,%d};\n",
             independentOptions_[0], independentOptions_[1], independentOptions_[2]);
     fprintf(fp, "3  ClpSolve clpSolve(method,presolveType,numberPasses,\n");
     fprintf(fp, "3                    options,extraInfo,independentOptions);\n");
}
//#############################################################################
#include "ClpNonLinearCost.hpp"

ClpSimplexProgress::ClpSimplexProgress ()
{
     int i;
     for (i = 0; i < CLP_PROGRESS; i++) {
          objective_[i] = COIN_DBL_MAX;
          infeasibility_[i] = -1.0; // set to an impossible value
          realInfeasibility_[i] = COIN_DBL_MAX;
          numberInfeasibilities_[i] = -1;
          iterationNumber_[i] = -1;
     }
#ifdef CLP_PROGRESS_WEIGHT
     for (i = 0; i < CLP_PROGRESS_WEIGHT; i++) {
          objectiveWeight_[i] = COIN_DBL_MAX;
          infeasibilityWeight_[i] = -1.0; // set to an impossible value
          realInfeasibilityWeight_[i] = COIN_DBL_MAX;
          numberInfeasibilitiesWeight_[i] = -1;
          iterationNumberWeight_[i] = -1;
     }
     drop_ = 0.0;
     best_ = 0.0;
#endif
     initialWeight_ = 0.0;
     for (i = 0; i < CLP_CYCLE; i++) {
          //obj_[i]=COIN_DBL_MAX;
          in_[i] = -1;
          out_[i] = -1;
          way_[i] = 0;
     }
     numberTimes_ = 0;
     numberBadTimes_ = 0;
     numberReallyBadTimes_ = 0;
     numberTimesFlagged_ = 0;
     model_ = NULL;
     oddState_ = 0;
}


//-----------------------------------------------------------------------------

ClpSimplexProgress::~ClpSimplexProgress ()
{
}
// Copy constructor.
ClpSimplexProgress::ClpSimplexProgress(const ClpSimplexProgress &rhs)
{
     int i;
     for (i = 0; i < CLP_PROGRESS; i++) {
          objective_[i] = rhs.objective_[i];
          infeasibility_[i] = rhs.infeasibility_[i];
          realInfeasibility_[i] = rhs.realInfeasibility_[i];
          numberInfeasibilities_[i] = rhs.numberInfeasibilities_[i];
          iterationNumber_[i] = rhs.iterationNumber_[i];
     }
#ifdef CLP_PROGRESS_WEIGHT
     for (i = 0; i < CLP_PROGRESS_WEIGHT; i++) {
          objectiveWeight_[i] = rhs.objectiveWeight_[i];
          infeasibilityWeight_[i] = rhs.infeasibilityWeight_[i];
          realInfeasibilityWeight_[i] = rhs.realInfeasibilityWeight_[i];
          numberInfeasibilitiesWeight_[i] = rhs.numberInfeasibilitiesWeight_[i];
          iterationNumberWeight_[i] = rhs.iterationNumberWeight_[i];
     }
     drop_ = rhs.drop_;
     best_ = rhs.best_;
#endif
     initialWeight_ = rhs.initialWeight_;
     for (i = 0; i < CLP_CYCLE; i++) {
          //obj_[i]=rhs.obj_[i];
          in_[i] = rhs.in_[i];
          out_[i] = rhs.out_[i];
          way_[i] = rhs.way_[i];
     }
     numberTimes_ = rhs.numberTimes_;
     numberBadTimes_ = rhs.numberBadTimes_;
     numberReallyBadTimes_ = rhs.numberReallyBadTimes_;
     numberTimesFlagged_ = rhs.numberTimesFlagged_;
     model_ = rhs.model_;
     oddState_ = rhs.oddState_;
}
// Copy constructor.from model
ClpSimplexProgress::ClpSimplexProgress(ClpSimplex * model)
{
     model_ = model;
     reset();
     initialWeight_ = 0.0;
}
// Fill from model
void
ClpSimplexProgress::fillFromModel ( ClpSimplex * model )
{
     model_ = model;
     reset();
     initialWeight_ = 0.0;
}
// Assignment operator. This copies the data
ClpSimplexProgress &
ClpSimplexProgress::operator=(const ClpSimplexProgress & rhs)
{
     if (this != &rhs) {
          int i;
          for (i = 0; i < CLP_PROGRESS; i++) {
               objective_[i] = rhs.objective_[i];
               infeasibility_[i] = rhs.infeasibility_[i];
               realInfeasibility_[i] = rhs.realInfeasibility_[i];
               numberInfeasibilities_[i] = rhs.numberInfeasibilities_[i];
               iterationNumber_[i] = rhs.iterationNumber_[i];
          }
#ifdef CLP_PROGRESS_WEIGHT
          for (i = 0; i < CLP_PROGRESS_WEIGHT; i++) {
               objectiveWeight_[i] = rhs.objectiveWeight_[i];
               infeasibilityWeight_[i] = rhs.infeasibilityWeight_[i];
               realInfeasibilityWeight_[i] = rhs.realInfeasibilityWeight_[i];
               numberInfeasibilitiesWeight_[i] = rhs.numberInfeasibilitiesWeight_[i];
               iterationNumberWeight_[i] = rhs.iterationNumberWeight_[i];
          }
          drop_ = rhs.drop_;
          best_ = rhs.best_;
#endif
          initialWeight_ = rhs.initialWeight_;
          for (i = 0; i < CLP_CYCLE; i++) {
               //obj_[i]=rhs.obj_[i];
               in_[i] = rhs.in_[i];
               out_[i] = rhs.out_[i];
               way_[i] = rhs.way_[i];
          }
          numberTimes_ = rhs.numberTimes_;
          numberBadTimes_ = rhs.numberBadTimes_;
          numberReallyBadTimes_ = rhs.numberReallyBadTimes_;
          numberTimesFlagged_ = rhs.numberTimesFlagged_;
          model_ = rhs.model_;
          oddState_ = rhs.oddState_;
     }
     return *this;
}
// Seems to be something odd about exact comparison of doubles on linux
static bool equalDouble(double value1, double value2)
{

     union {
          double d;
          int i[2];
     } v1, v2;
     v1.d = value1;
     v2.d = value2;
     if (sizeof(int) * 2 == sizeof(double))
          return (v1.i[0] == v2.i[0] && v1.i[1] == v2.i[1]);
     else
          return (v1.i[0] == v2.i[0]);
}
int
ClpSimplexProgress::looping()
{
     if (!model_)
          return -1;
     double objective;
     if (model_->algorithm() < 0) {
       objective = model_->rawObjectiveValue();
       objective -= model_->bestPossibleImprovement();
     } else {
       objective = model_->nonLinearCost()->feasibleReportCost();
     }
     double infeasibility;
     double realInfeasibility = 0.0;
     int numberInfeasibilities;
     int iterationNumber = model_->numberIterations();
     //numberTimesFlagged_ = 0;
     if (model_->algorithm() < 0) {
          // dual
          infeasibility = model_->sumPrimalInfeasibilities();
          numberInfeasibilities = model_->numberPrimalInfeasibilities();
     } else {
          //primal
          infeasibility = model_->sumDualInfeasibilities();
          realInfeasibility = model_->nonLinearCost()->sumInfeasibilities();
          numberInfeasibilities = model_->numberDualInfeasibilities();
     }
     int i;
     int numberMatched = 0;
     int matched = 0;
     int nsame = 0;
     for (i = 0; i < CLP_PROGRESS; i++) {
          bool matchedOnObjective = equalDouble(objective, objective_[i]);
          bool matchedOnInfeasibility = equalDouble(infeasibility, infeasibility_[i]);
          bool matchedOnInfeasibilities =
               (numberInfeasibilities == numberInfeasibilities_[i]);

          if (matchedOnObjective && matchedOnInfeasibility && matchedOnInfeasibilities) {
               matched |= (1 << i);
               // Check not same iteration
               if (iterationNumber != iterationNumber_[i]) {
                    numberMatched++;
                    // here mainly to get over compiler bug?
                    if (model_->messageHandler()->logLevel() > 10)
                         printf("%d %d %d %d %d loop check\n", i, numberMatched,
                                matchedOnObjective, matchedOnInfeasibility,
                                matchedOnInfeasibilities);
               } else {
                    // stuck but code should notice
                    nsame++;
               }
          }
          if (i) {
               objective_[i-1] = objective_[i];
               infeasibility_[i-1] = infeasibility_[i];
               realInfeasibility_[i-1] = realInfeasibility_[i];
               numberInfeasibilities_[i-1] = numberInfeasibilities_[i];
               iterationNumber_[i-1] = iterationNumber_[i];
          }
     }
     objective_[CLP_PROGRESS-1] = objective;
     infeasibility_[CLP_PROGRESS-1] = infeasibility;
     realInfeasibility_[CLP_PROGRESS-1] = realInfeasibility;
     numberInfeasibilities_[CLP_PROGRESS-1] = numberInfeasibilities;
     iterationNumber_[CLP_PROGRESS-1] = iterationNumber;
     if (nsame == CLP_PROGRESS)
          numberMatched = CLP_PROGRESS; // really stuck
     if (model_->progressFlag())
          numberMatched = 0;
     numberTimes_++;
     if (numberTimes_ < 10)
          numberMatched = 0;
     // skip if just last time as may be checking something
     if (matched == (1 << (CLP_PROGRESS - 1)))
          numberMatched = 0;
     if (numberMatched && model_->clpMatrix()->type() < 15) {
          model_->messageHandler()->message(CLP_POSSIBLELOOP, model_->messages())
                    << numberMatched
                    << matched
                    << numberTimes_
                    << CoinMessageEol;
          numberBadTimes_++;
          if (numberBadTimes_ < 10) {
               // make factorize every iteration
               model_->forceFactorization(1);
               if (numberBadTimes_ < 2) {
                    startCheck(); // clear other loop check
                    if (model_->algorithm() < 0) {
                         // dual - change tolerance
                         model_->setCurrentDualTolerance(model_->currentDualTolerance() * 1.05);
                         // if infeasible increase dual bound
                         if (model_->dualBound() < 1.0e17) {
                              model_->setDualBound(model_->dualBound() * 1.1);
                              static_cast<ClpSimplexDual *>(model_)->resetFakeBounds(0);
                         }
                    } else {
                         // primal - change tolerance
                         if (numberBadTimes_ > 3)
                              model_->setCurrentPrimalTolerance(model_->currentPrimalTolerance() * 1.05);
                         // if infeasible increase infeasibility cost
                         if (model_->nonLinearCost()->numberInfeasibilities() &&
                                   model_->infeasibilityCost() < 1.0e17) {
                              model_->setInfeasibilityCost(model_->infeasibilityCost() * 1.1);
                         }
                    }
               } else {
                    // flag
                    int iSequence;
                    if (model_->algorithm() < 0) {
                         // dual
                         if (model_->dualBound() > 1.0e14)
                              model_->setDualBound(1.0e14);
                         iSequence = in_[CLP_CYCLE-1];
                    } else {
                         // primal
                         if (model_->infeasibilityCost() > 1.0e14)
                              model_->setInfeasibilityCost(1.0e14);
                         iSequence = out_[CLP_CYCLE-1];
                    }
                    if (iSequence >= 0) {
                         char x = model_->isColumn(iSequence) ? 'C' : 'R';
                         if (model_->messageHandler()->logLevel() >= 63)
                              model_->messageHandler()->message(CLP_SIMPLEX_FLAG, model_->messages())
                                        << x << model_->sequenceWithin(iSequence)
                                        << CoinMessageEol;
                         // if Gub then needs to be sequenceIn_
                         int save = model_->sequenceIn();
                         model_->setSequenceIn(iSequence);
                         model_->setFlagged(iSequence);
                         model_->setSequenceIn(save);
                         //printf("flagging %d from loop\n",iSequence);
                         startCheck();
                    } else {
                         // Give up
                         if (model_->messageHandler()->logLevel() >= 63)
                              printf("***** All flagged?\n");
                         return 4;
                    }
                    // reset
                    numberBadTimes_ = 2;
               }
               return -2;
          } else {
               // look at solution and maybe declare victory
               if (infeasibility < 1.0e-4) {
                    return 0;
               } else {
                    model_->messageHandler()->message(CLP_LOOP, model_->messages())
                              << CoinMessageEol;
#ifndef NDEBUG
                    printf("debug loop ClpSimplex A\n");
                    abort();
#endif
                    return 3;
               }
          }
     }
     return -1;
}
// Resets as much as possible
void
ClpSimplexProgress::reset()
{
     int i;
     for (i = 0; i < CLP_PROGRESS; i++) {
          if (model_->algorithm() >= 0)
               objective_[i] = COIN_DBL_MAX;
          else
               objective_[i] = -COIN_DBL_MAX;
          infeasibility_[i] = -1.0; // set to an impossible value
          realInfeasibility_[i] = COIN_DBL_MAX;
          numberInfeasibilities_[i] = -1;
          iterationNumber_[i] = -1;
     }
#ifdef CLP_PROGRESS_WEIGHT
     for (i = 0; i < CLP_PROGRESS_WEIGHT; i++) {
          objectiveWeight_[i] = COIN_DBL_MAX;
          infeasibilityWeight_[i] = -1.0; // set to an impossible value
          realInfeasibilityWeight_[i] = COIN_DBL_MAX;
          numberInfeasibilitiesWeight_[i] = -1;
          iterationNumberWeight_[i] = -1;
     }
     drop_ = 0.0;
     best_ = 0.0;
#endif
     for (i = 0; i < CLP_CYCLE; i++) {
          //obj_[i]=COIN_DBL_MAX;
          in_[i] = -1;
          out_[i] = -1;
          way_[i] = 0;
     }
     numberTimes_ = 0;
     numberBadTimes_ = 0;
     numberReallyBadTimes_ = 0;
     numberTimesFlagged_ = 0;
     oddState_ = 0;
}
// Returns previous objective (if -1) - current if (0)
double
ClpSimplexProgress::lastObjective(int back) const
{
     return objective_[CLP_PROGRESS-1-back];
}
// Returns previous infeasibility (if -1) - current if (0)
double
ClpSimplexProgress::lastInfeasibility(int back) const
{
     return realInfeasibility_[CLP_PROGRESS-1-back];
}
// Sets real primal infeasibility
void
ClpSimplexProgress::setInfeasibility(double value)
{
     for (int i = 1; i < CLP_PROGRESS; i++)
          realInfeasibility_[i-1] = realInfeasibility_[i];
     realInfeasibility_[CLP_PROGRESS-1] = value;
}
// Modify objective e.g. if dual infeasible in dual
void
ClpSimplexProgress::modifyObjective(double value)
{
     objective_[CLP_PROGRESS-1] = value;
}
// Returns previous iteration number (if -1) - current if (0)
int
ClpSimplexProgress::lastIterationNumber(int back) const
{
     return iterationNumber_[CLP_PROGRESS-1-back];
}
// clears iteration numbers (to switch off panic)
void
ClpSimplexProgress::clearIterationNumbers()
{
     for (int i = 0; i < CLP_PROGRESS; i++)
          iterationNumber_[i] = -1;
}
// Start check at beginning of whileIterating
void
ClpSimplexProgress::startCheck()
{
     int i;
     for (i = 0; i < CLP_CYCLE; i++) {
          //obj_[i]=COIN_DBL_MAX;
          in_[i] = -1;
          out_[i] = -1;
          way_[i] = 0;
     }
}
// Returns cycle length in whileIterating
int
ClpSimplexProgress::cycle(int in, int out, int wayIn, int wayOut)
{
     int i;
#if 0
     if (model_->numberIterations() > 206571) {
          printf("in %d out %d\n", in, out);
          for (i = 0; i < CLP_CYCLE; i++)
               printf("cy %d in %d out %d\n", i, in_[i], out_[i]);
     }
#endif
     int matched = 0;
     // first see if in matches any out
     for (i = 1; i < CLP_CYCLE; i++) {
          if (in == out_[i]) {
               // even if flip then suspicious
               matched = -1;
               break;
          }
     }
#if 0
     if (!matched || in_[0] < 0) {
          // can't be cycle
          for (i = 0; i < CLP_CYCLE - 1; i++) {
               //obj_[i]=obj_[i+1];
               in_[i] = in_[i+1];
               out_[i] = out_[i+1];
               way_[i] = way_[i+1];
          }
     } else {
          // possible cycle
          matched = 0;
          for (i = 0; i < CLP_CYCLE - 1; i++) {
               int k;
               char wayThis = way_[i];
               int inThis = in_[i];
               int outThis = out_[i];
               //double objThis = obj_[i];
               for(k = i + 1; k < CLP_CYCLE; k++) {
                    if (inThis == in_[k] && outThis == out_[k] && wayThis == way_[k]) {
                         int distance = k - i;
                         if (k + distance < CLP_CYCLE) {
                              // See if repeats
                              int j = k + distance;
                              if (inThis == in_[j] && outThis == out_[j] && wayThis == way_[j]) {
                                   matched = distance;
                                   break;
                              }
                         } else {
                              matched = distance;
                              break;
                         }
                    }
               }
               //obj_[i]=obj_[i+1];
               in_[i] = in_[i+1];
               out_[i] = out_[i+1];
               way_[i] = way_[i+1];
          }
     }
#else
     if (matched && in_[0] >= 0) {
          // possible cycle - only check [0] against all
          matched = 0;
          int nMatched = 0;
          char way0 = way_[0];
          int in0 = in_[0];
          int out0 = out_[0];
          //double obj0 = obj_[i];
          for(int k = 1; k < CLP_CYCLE - 4; k++) {
               if (in0 == in_[k] && out0 == out_[k] && way0 == way_[k]) {
                    nMatched++;
                    // See if repeats
                    int end = CLP_CYCLE - k;
                    int j;
                    for ( j = 1; j < end; j++) {
                         if (in_[j+k] != in_[j] || out_[j+k] != out_[j] || way_[j+k] != way_[j])
                              break;
                    }
                    if (j == end) {
                         matched = k;
                         break;
                    }
               }
          }
          // If three times then that is too much even if not regular
          if (matched <= 0 && nMatched > 1)
               matched = 100;
     }
     for (i = 0; i < CLP_CYCLE - 1; i++) {
          //obj_[i]=obj_[i+1];
          in_[i] = in_[i+1];
          out_[i] = out_[i+1];
          way_[i] = way_[i+1];
     }
#endif
     int way = 1 - wayIn + 4 * (1 - wayOut);
     //obj_[i]=model_->objectiveValue();
     in_[CLP_CYCLE-1] = in;
     out_[CLP_CYCLE-1] = out;
     way_[CLP_CYCLE-1] = static_cast<char>(way);
     return matched;
}
#include "CoinStructuredModel.hpp"
// Solve using structure of model and maybe in parallel
int
ClpSimplex::solve(CoinStructuredModel * model)
{
     // analyze structure
     int numberRowBlocks = model->numberRowBlocks();
     int numberColumnBlocks = model->numberColumnBlocks();
     int numberElementBlocks = model->numberElementBlocks();
     if (numberElementBlocks == 1) {
          loadProblem(*model, false);
          return dual();
     }
     // For now just get top level structure
     CoinModelBlockInfo * blockInfo = new CoinModelBlockInfo [numberElementBlocks];
     for (int i = 0; i < numberElementBlocks; i++) {
          CoinStructuredModel * subModel =
               dynamic_cast<CoinStructuredModel *>(model->block(i));
          CoinModel * thisBlock;
          if (subModel) {
               thisBlock = subModel->coinModelBlock(blockInfo[i]);
               model->setCoinModel(thisBlock, i);
          } else {
               thisBlock = dynamic_cast<CoinModel *>(model->block(i));
               assert (thisBlock);
               // just fill in info
               CoinModelBlockInfo info = CoinModelBlockInfo();
               int whatsSet = thisBlock->whatIsSet();
               info.matrix = static_cast<char>(((whatsSet & 1) != 0) ? 1 : 0);
               info.rhs = static_cast<char>(((whatsSet & 2) != 0) ? 1 : 0);
               info.rowName = static_cast<char>(((whatsSet & 4) != 0) ? 1 : 0);
               info.integer = static_cast<char>(((whatsSet & 32) != 0) ? 1 : 0);
               info.bounds = static_cast<char>(((whatsSet & 8) != 0) ? 1 : 0);
               info.columnName = static_cast<char>(((whatsSet & 16) != 0) ? 1 : 0);
               // Which block
               int iRowBlock = model->rowBlock(thisBlock->getRowBlock());
               info.rowBlock = iRowBlock;
               int iColumnBlock = model->columnBlock(thisBlock->getColumnBlock());
               info.columnBlock = iColumnBlock;
               blockInfo[i] = info;
          }
     }
     int * rowCounts = new int [numberRowBlocks];
     CoinZeroN(rowCounts, numberRowBlocks);
     int * columnCounts = new int [numberColumnBlocks+1];
     CoinZeroN(columnCounts, numberColumnBlocks);
     int decomposeType = 0;
     for (int i = 0; i < numberElementBlocks; i++) {
          int iRowBlock = blockInfo[i].rowBlock;
          int iColumnBlock = blockInfo[i].columnBlock;
          rowCounts[iRowBlock]++;
          columnCounts[iColumnBlock]++;
     }
     if (numberRowBlocks == numberColumnBlocks ||
               numberRowBlocks == numberColumnBlocks + 1) {
          // could be Dantzig-Wolfe
          int numberG1 = 0;
          for (int i = 0; i < numberRowBlocks; i++) {
               if (rowCounts[i] > 1)
                    numberG1++;
          }
          bool masterColumns = (numberColumnBlocks == numberRowBlocks);
          if ((masterColumns && numberElementBlocks == 2 * numberRowBlocks - 1)
                    || (!masterColumns && numberElementBlocks == 2 * numberRowBlocks)) {
               if (numberG1 < 2)
                    decomposeType = 1;
          }
     }
     if (!decomposeType && (numberRowBlocks == numberColumnBlocks ||
                            numberRowBlocks == numberColumnBlocks - 1)) {
          // could be Benders
          int numberG1 = 0;
          for (int i = 0; i < numberColumnBlocks; i++) {
               if (columnCounts[i] > 1)
                    numberG1++;
          }
          bool masterRows = (numberColumnBlocks == numberRowBlocks);
          if ((masterRows && numberElementBlocks == 2 * numberColumnBlocks - 1)
                    || (!masterRows && numberElementBlocks == 2 * numberColumnBlocks)) {
               if (numberG1 < 2)
                    decomposeType = 2;
          }
     }
     delete [] rowCounts;
     delete [] columnCounts;
     delete [] blockInfo;
     // decide what to do
     switch (decomposeType) {
          // No good
     case 0:
          loadProblem(*model, false);
          return dual();
          // DW
     case 1:
          return solveDW(model);
          // Benders
     case 2:
          return solveBenders(model);
     }
     return 0; // to stop compiler warning
}
/* This loads a model from a CoinStructuredModel object - returns number of errors.
   If originalOrder then keep to order stored in blocks,
   otherwise first column/rows correspond to first block - etc.
   If keepSolution true and size is same as current then
   keeps current status and solution
*/
int
ClpSimplex::loadProblem (  CoinStructuredModel & coinModel,
                           bool originalOrder,
                           bool keepSolution)
{
     unsigned char * status = NULL;
     double * psol = NULL;
     double * dsol = NULL;
     int numberRows = coinModel.numberRows();
     int numberColumns = coinModel.numberColumns();
     int numberRowBlocks = coinModel.numberRowBlocks();
     int numberColumnBlocks = coinModel.numberColumnBlocks();
     int numberElementBlocks = coinModel.numberElementBlocks();
     if (status_ && numberRows_ && numberRows_ == numberRows &&
               numberColumns_ == numberColumns && keepSolution) {
          status = new unsigned char [numberRows_+numberColumns_];
          CoinMemcpyN(status_, numberRows_ + numberColumns_, status);
          psol = new double [numberRows_+numberColumns_];
          CoinMemcpyN(columnActivity_, numberColumns_, psol);
          CoinMemcpyN(rowActivity_, numberRows_, psol + numberColumns_);
          dsol = new double [numberRows_+numberColumns_];
          CoinMemcpyN(reducedCost_, numberColumns_, dsol);
          CoinMemcpyN(dual_, numberRows_, dsol + numberColumns_);
     }
     int returnCode = 0;
     double * rowLower = new double [numberRows];
     double * rowUpper = new double [numberRows];
     double * columnLower = new double [numberColumns];
     double * columnUpper = new double [numberColumns];
     double * objective = new double [numberColumns];
     int * integerType = new int [numberColumns];
     CoinBigIndex numberElements = 0;
     // Bases for blocks
     int * rowBase = new int[numberRowBlocks];
     CoinFillN(rowBase, numberRowBlocks, -1);
     // And row to put it
     int * whichRow = new int [numberRows];
     int * columnBase = new int[numberColumnBlocks];
     CoinFillN(columnBase, numberColumnBlocks, -1);
     // And column to put it
     int * whichColumn = new int [numberColumns];
     for (int iBlock = 0; iBlock < numberElementBlocks; iBlock++) {
          CoinModel * block = coinModel.coinBlock(iBlock);
          numberElements += block->numberElements();
          //and set up elements etc
          double * associated = block->associatedArray();
          // If strings then do copies
          if (block->stringsExist())
               returnCode += block->createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                                 objective, integerType, associated);
          const CoinModelBlockInfo & info = coinModel.blockType(iBlock);
          int iRowBlock = info.rowBlock;
          int iColumnBlock = info.columnBlock;
          if (rowBase[iRowBlock] < 0) {
               rowBase[iRowBlock] = block->numberRows();
               // Save block number
               whichRow[numberRows-numberRowBlocks+iRowBlock] = iBlock;
          } else {
               assert(rowBase[iRowBlock] == block->numberRows());
          }
          if (columnBase[iColumnBlock] < 0) {
               columnBase[iColumnBlock] = block->numberColumns();
               // Save block number
               whichColumn[numberColumns-numberColumnBlocks+iColumnBlock] = iBlock;
          } else {
               assert(columnBase[iColumnBlock] == block->numberColumns());
          }
     }
     // Fill arrays with defaults
     CoinFillN(rowLower, numberRows, -COIN_DBL_MAX);
     CoinFillN(rowUpper, numberRows, COIN_DBL_MAX);
     CoinFillN(columnLower, numberColumns, 0.0);
     CoinFillN(columnUpper, numberColumns, COIN_DBL_MAX);
     CoinFillN(objective, numberColumns, 0.0);
     CoinFillN(integerType, numberColumns, 0);
     int n = 0;
     for (int iBlock = 0; iBlock < numberRowBlocks; iBlock++) {
          int k = rowBase[iBlock];
          rowBase[iBlock] = n;
          assert (k >= 0);
          // block number
          int jBlock = whichRow[numberRows-numberRowBlocks+iBlock];
          if (originalOrder) {
               memcpy(whichRow + n, coinModel.coinBlock(jBlock)->originalRows(), k * sizeof(int));
          } else {
               CoinIotaN(whichRow + n, k, n);
          }
          n += k;
     }
     assert (n == numberRows);
     n = 0;
     for (int iBlock = 0; iBlock < numberColumnBlocks; iBlock++) {
          int k = columnBase[iBlock];
          columnBase[iBlock] = n;
          assert (k >= 0);
          if (k) {
               // block number
               int jBlock = whichColumn[numberColumns-numberColumnBlocks+iBlock];
               if (originalOrder) {
                    memcpy(whichColumn + n, coinModel.coinBlock(jBlock)->originalColumns(),
                           k * sizeof(int));
               } else {
                    CoinIotaN(whichColumn + n, k, n);
               }
               n += k;
          }
     }
     assert (n == numberColumns);
     bool gotIntegers = false;
     for (int iBlock = 0; iBlock < numberElementBlocks; iBlock++) {
          CoinModel * block = coinModel.coinBlock(iBlock);
          const CoinModelBlockInfo & info = coinModel.blockType(iBlock);
          int iRowBlock = info.rowBlock;
          int iRowBase = rowBase[iRowBlock];
          int iColumnBlock = info.columnBlock;
          int iColumnBase = columnBase[iColumnBlock];
          if (info.rhs) {
               int nRows = block->numberRows();
               const double * lower = block->rowLowerArray();
               const double * upper = block->rowUpperArray();
               for (int i = 0; i < nRows; i++) {
                    int put = whichRow[i+iRowBase];
                    rowLower[put] = lower[i];
                    rowUpper[put] = upper[i];
               }
          }
          if (info.bounds) {
               int nColumns = block->numberColumns();
               const double * lower = block->columnLowerArray();
               const double * upper = block->columnUpperArray();
               const double * obj = block->objectiveArray();
               for (int i = 0; i < nColumns; i++) {
                    int put = whichColumn[i+iColumnBase];
                    columnLower[put] = lower[i];
                    columnUpper[put] = upper[i];
                    objective[put] = obj[i];
               }
          }
          if (info.integer) {
               gotIntegers = true;
               int nColumns = block->numberColumns();
               const int * type = block->integerTypeArray();
               for (int i = 0; i < nColumns; i++) {
                    int put = whichColumn[i+iColumnBase];
                    integerType[put] = type[i];
               }
          }
     }
     gutsOfLoadModel(numberRows, numberColumns,
                     columnLower, columnUpper, objective, rowLower, rowUpper, NULL);
     delete [] rowLower;
     delete [] rowUpper;
     delete [] columnLower;
     delete [] columnUpper;
     delete [] objective;
     // Do integers if wanted
     if (gotIntegers) {
          for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (integerType[iColumn])
                    setInteger(iColumn);
          }
     }
     delete [] integerType;
     setObjectiveOffset(coinModel.objectiveOffset());
     // Space for elements
     int * row = new int[numberElements];
     int * column = new int[numberElements];
     double * element = new double[numberElements];
     numberElements = 0;
     for (int iBlock = 0; iBlock < numberElementBlocks; iBlock++) {
          CoinModel * block = coinModel.coinBlock(iBlock);
          const CoinModelBlockInfo & info = coinModel.blockType(iBlock);
          int iRowBlock = info.rowBlock;
          int iRowBase = rowBase[iRowBlock];
          int iColumnBlock = info.columnBlock;
          int iColumnBase = columnBase[iColumnBlock];
          if (info.rowName) {
               int numberItems = block->rowNames()->numberItems();
               assert( block->numberRows() >= numberItems);
               if (numberItems) {
                    const char *const * rowNames = block->rowNames()->names();
                    for (int i = 0; i < numberItems; i++) {
                         int put = whichRow[i+iRowBase];
                         std::string name = rowNames[i];
                         setRowName(put, name);
                    }
               }
          }
          if (info.columnName) {
               int numberItems = block->columnNames()->numberItems();
               assert( block->numberColumns() >= numberItems);
               if (numberItems) {
                    const char *const * columnNames = block->columnNames()->names();
                    for (int i = 0; i < numberItems; i++) {
                         int put = whichColumn[i+iColumnBase];
                         std::string name = columnNames[i];
                         setColumnName(put, name);
                    }
               }
          }
          if (info.matrix) {
               CoinPackedMatrix matrix2;
               const CoinPackedMatrix * matrix = block->packedMatrix();
               if (!matrix) {
                    double * associated = block->associatedArray();
                    block->createPackedMatrix(matrix2, associated);
                    matrix = &matrix2;
               }
               // get matrix data pointers
               const int * row2 = matrix->getIndices();
               const CoinBigIndex * columnStart = matrix->getVectorStarts();
               const double * elementByColumn = matrix->getElements();
               const int * columnLength = matrix->getVectorLengths();
               int n = matrix->getNumCols();
               assert (matrix->isColOrdered());
               for (int iColumn = 0; iColumn < n; iColumn++) {
                    CoinBigIndex j;
                    int jColumn = whichColumn[iColumn+iColumnBase];
                    for (j = columnStart[iColumn];
                              j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                         row[numberElements] = whichRow[row2[j] + iRowBase];
                         column[numberElements] = jColumn;
                         element[numberElements++] = elementByColumn[j];
                    }
               }
          }
     }
     delete [] whichRow;
     delete [] whichColumn;
     delete [] rowBase;
     delete [] columnBase;
     CoinPackedMatrix * matrix =
          new CoinPackedMatrix (true, row, column, element, numberElements);
     matrix_ = new ClpPackedMatrix(matrix);
     matrix_->setDimensions(numberRows, numberColumns);
     delete [] row;
     delete [] column;
     delete [] element;
     createStatus();
     if (status) {
          // copy back
          CoinMemcpyN(status, numberRows_ + numberColumns_, status_);
          CoinMemcpyN(psol, numberColumns_, columnActivity_);
          CoinMemcpyN(psol + numberColumns_, numberRows_, rowActivity_);
          CoinMemcpyN(dsol, numberColumns_, reducedCost_);
          CoinMemcpyN(dsol + numberColumns_, numberRows_, dual_);
          delete [] status;
          delete [] psol;
          delete [] dsol;
     }
     optimizationDirection_ = coinModel.optimizationDirection();
     return returnCode;
}
/*  If input negative scales objective so maximum <= -value
    and returns scale factor used.  If positive unscales and also
    redoes dual stuff
*/
double
ClpSimplex::scaleObjective(double value)
{
     double * obj = objective();
     double largest = 0.0;
     if (value < 0.0) {
          value = - value;
          for (int i = 0; i < numberColumns_; i++) {
               largest = CoinMax(largest, fabs(obj[i]));
          }
          if (largest > value) {
               double scaleFactor = value / largest;
               for (int i = 0; i < numberColumns_; i++) {
                    obj[i] *= scaleFactor;
                    reducedCost_[i] *= scaleFactor;
               }
               for (int i = 0; i < numberRows_; i++) {
                    dual_[i] *= scaleFactor;
               }
               largest /= value;
          } else {
               // no need
               largest = 1.0;
          }
     } else {
          // at end
          if (value != 1.0) {
               for (int i = 0; i < numberColumns_; i++) {
                    obj[i] *= value;
                    reducedCost_[i] *= value;
               }
               for (int i = 0; i < numberRows_; i++) {
                    dual_[i] *= value;
               }
               computeObjectiveValue();
          }
     }
     return largest;
}
// Solve using Dantzig-Wolfe decomposition and maybe in parallel
int
ClpSimplex::solveDW(CoinStructuredModel * model)
{
     double time1 = CoinCpuTime();
     int numberColumns = model->numberColumns();
     int numberRowBlocks = model->numberRowBlocks();
     int numberColumnBlocks = model->numberColumnBlocks();
     int numberElementBlocks = model->numberElementBlocks();
     // We already have top level structure
     CoinModelBlockInfo * blockInfo = new CoinModelBlockInfo [numberElementBlocks];
     for (int i = 0; i < numberElementBlocks; i++) {
          CoinModel * thisBlock = model->coinBlock(i);
          assert (thisBlock);
          // just fill in info
          CoinModelBlockInfo info = CoinModelBlockInfo();
          int whatsSet = thisBlock->whatIsSet();
          info.matrix = static_cast<char>(((whatsSet & 1) != 0) ? 1 : 0);
          info.rhs = static_cast<char>(((whatsSet & 2) != 0) ? 1 : 0);
          info.rowName = static_cast<char>(((whatsSet & 4) != 0) ? 1 : 0);
          info.integer = static_cast<char>(((whatsSet & 32) != 0) ? 1 : 0);
          info.bounds = static_cast<char>(((whatsSet & 8) != 0) ? 1 : 0);
          info.columnName = static_cast<char>(((whatsSet & 16) != 0) ? 1 : 0);
          // Which block
          int iRowBlock = model->rowBlock(thisBlock->getRowBlock());
          info.rowBlock = iRowBlock;
          int iColumnBlock = model->columnBlock(thisBlock->getColumnBlock());
          info.columnBlock = iColumnBlock;
          blockInfo[i] = info;
     }
     // make up problems
     int numberBlocks = numberRowBlocks - 1;
     // Find master rows and columns
     int * rowCounts = new int [numberRowBlocks];
     CoinZeroN(rowCounts, numberRowBlocks);
     int * columnCounts = new int [numberColumnBlocks+1];
     CoinZeroN(columnCounts, numberColumnBlocks);
     int iBlock;
     for (iBlock = 0; iBlock < numberElementBlocks; iBlock++) {
          int iRowBlock = blockInfo[iBlock].rowBlock;
          rowCounts[iRowBlock]++;
          int iColumnBlock = blockInfo[iBlock].columnBlock;
          columnCounts[iColumnBlock]++;
     }
     int * whichBlock = new int [numberElementBlocks];
     int masterRowBlock = -1;
     for (iBlock = 0; iBlock < numberRowBlocks; iBlock++) {
          if (rowCounts[iBlock] > 1) {
               if (masterRowBlock == -1) {
                    masterRowBlock = iBlock;
               } else {
                    // Can't decode
                    masterRowBlock = -2;
                    break;
               }
          }
     }
     int masterColumnBlock = -1;
     int kBlock = 0;
     for (iBlock = 0; iBlock < numberColumnBlocks; iBlock++) {
          int count = columnCounts[iBlock];
          columnCounts[iBlock] = kBlock;
          kBlock += count;
     }
     for (iBlock = 0; iBlock < numberElementBlocks; iBlock++) {
          int iColumnBlock = blockInfo[iBlock].columnBlock;
          whichBlock[columnCounts[iColumnBlock]] = iBlock;
          columnCounts[iColumnBlock]++;
     }
     for (iBlock = numberColumnBlocks - 1; iBlock >= 0; iBlock--)
          columnCounts[iBlock+1] = columnCounts[iBlock];
     columnCounts[0] = 0;
     for (iBlock = 0; iBlock < numberColumnBlocks; iBlock++) {
          int count = columnCounts[iBlock+1] - columnCounts[iBlock];
          if (count == 1) {
               int kBlock = whichBlock[columnCounts[iBlock]];
               int iRowBlock = blockInfo[kBlock].rowBlock;
               if (iRowBlock == masterRowBlock) {
                    if (masterColumnBlock == -1) {
                         masterColumnBlock = iBlock;
                    } else {
                         // Can't decode
                         masterColumnBlock = -2;
                         break;
                    }
               }
          }
     }
     if (masterRowBlock < 0 || masterColumnBlock == -2) {
          // What now
          abort();
     }
     delete [] rowCounts;
     // create all data
     const CoinPackedMatrix ** top = new const CoinPackedMatrix * [numberColumnBlocks];
     ClpSimplex * sub = new ClpSimplex [numberBlocks];
     ClpSimplex master;
     // Set offset
     master.setObjectiveOffset(model->objectiveOffset());
     kBlock = 0;
     int masterBlock = -1;
     for (iBlock = 0; iBlock < numberColumnBlocks; iBlock++) {
          top[kBlock] = NULL;
          int start = columnCounts[iBlock];
          int end = columnCounts[iBlock+1];
          assert (end - start <= 2);
          for (int j = start; j < end; j++) {
               int jBlock = whichBlock[j];
               int iRowBlock = blockInfo[jBlock].rowBlock;
               int iColumnBlock = blockInfo[jBlock].columnBlock;
               assert (iColumnBlock == iBlock);
               if (iColumnBlock != masterColumnBlock && iRowBlock == masterRowBlock) {
                    // top matrix
                    top[kBlock] = model->coinBlock(jBlock)->packedMatrix();
               } else {
                    const CoinPackedMatrix * matrix
                    = model->coinBlock(jBlock)->packedMatrix();
                    // Get pointers to arrays
                    const double * rowLower;
                    const double * rowUpper;
                    const double * columnLower;
                    const double * columnUpper;
                    const double * objective;
                    model->block(iRowBlock, iColumnBlock, rowLower, rowUpper,
                                 columnLower, columnUpper, objective);
                    if (iColumnBlock != masterColumnBlock) {
                         // diagonal block
                         sub[kBlock].loadProblem(*matrix, columnLower, columnUpper,
                                                 objective, rowLower, rowUpper);
                         if (true) {
                              double * lower = sub[kBlock].columnLower();
                              double * upper = sub[kBlock].columnUpper();
                              int n = sub[kBlock].numberColumns();
                              for (int i = 0; i < n; i++) {
                                   lower[i] = CoinMax(-1.0e8, lower[i]);
                                   upper[i] = CoinMin(1.0e8, upper[i]);
                              }
                         }
                         if (optimizationDirection_ < 0.0) {
                              double * obj = sub[kBlock].objective();
                              int n = sub[kBlock].numberColumns();
                              for (int i = 0; i < n; i++)
                                   obj[i] = - obj[i];
                         }
                         if (this->factorizationFrequency() == 200) {
                              // User did not touch preset
                              sub[kBlock].defaultFactorizationFrequency();
                         } else {
                              // make sure model has correct value
                              sub[kBlock].setFactorizationFrequency(this->factorizationFrequency());
                         }
                         sub[kBlock].setPerturbation(50);
                         // Set columnCounts to be diagonal block index for cleanup
                         columnCounts[kBlock] = jBlock;
                    } else {
                         // master
                         masterBlock = jBlock;
                         master.loadProblem(*matrix, columnLower, columnUpper,
                                            objective, rowLower, rowUpper);
                         if (optimizationDirection_ < 0.0) {
                              double * obj = master.objective();
                              int n = master.numberColumns();
                              for (int i = 0; i < n; i++)
                                   obj[i] = - obj[i];
                         }
                    }
               }
          }
          if (iBlock != masterColumnBlock)
               kBlock++;
     }
     delete [] whichBlock;
     delete [] blockInfo;
     // For now master must have been defined (does not have to have columns)
     assert (master.numberRows());
     assert (masterBlock >= 0);
     int numberMasterRows = master.numberRows();
     // Overkill in terms of space
     int spaceNeeded = CoinMax(numberBlocks * (numberMasterRows + 1),
                               2 * numberMasterRows);
     int * rowAdd = new int[spaceNeeded];
     double * elementAdd = new double[spaceNeeded];
     spaceNeeded = numberBlocks;
     int * columnAdd = new int[spaceNeeded+1];
     double * objective = new double[spaceNeeded];
     // Add in costed slacks
     int firstArtificial = master.numberColumns();
     int lastArtificial = firstArtificial;
     if (true) {
          const double * lower = master.rowLower();
          const double * upper = master.rowUpper();
          int kCol = 0;
          for (int iRow = 0; iRow < numberMasterRows; iRow++) {
               if (lower[iRow] > -1.0e10) {
                    rowAdd[kCol] = iRow;
                    elementAdd[kCol++] = 1.0;
               }
               if (upper[iRow] < 1.0e10) {
                    rowAdd[kCol] = iRow;
                    elementAdd[kCol++] = -1.0;
               }
          }
          if (kCol > spaceNeeded) {
               spaceNeeded = kCol;
               delete [] columnAdd;
               delete [] objective;
               columnAdd = new int[spaceNeeded+1];
               objective = new double[spaceNeeded];
          }
          for (int i = 0; i < kCol; i++) {
               columnAdd[i] = i;
               objective[i] = 1.0e13;
          }
          columnAdd[kCol] = kCol;
          master.addColumns(kCol, NULL, NULL, objective,
                            columnAdd, rowAdd, elementAdd);
          lastArtificial = master.numberColumns();
     }
     int maxPass = 500;
     int iPass;
     double lastObjective = 1.0e31;
     // Create convexity rows for proposals
     int numberMasterColumns = master.numberColumns();
     master.resize(numberMasterRows + numberBlocks, numberMasterColumns);
     if (this->factorizationFrequency() == 200) {
          // User did not touch preset
          master.defaultFactorizationFrequency();
     } else {
          // make sure model has correct value
          master.setFactorizationFrequency(this->factorizationFrequency());
     }
     master.setPerturbation(50);
     // Arrays to say which block and when created
     int maximumColumns = 2 * numberMasterRows + 10 * numberBlocks;
     whichBlock = new int[maximumColumns];
     int * when = new int[maximumColumns];
     int numberColumnsGenerated = numberBlocks;
     // fill in rhs and add in artificials
     {
          double * rowLower = master.rowLower();
          double * rowUpper = master.rowUpper();
          int iBlock;
          columnAdd[0] = 0;
          for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
               int iRow = iBlock + numberMasterRows;;
               rowLower[iRow] = 1.0;
               rowUpper[iRow] = 1.0;
               rowAdd[iBlock] = iRow;
               elementAdd[iBlock] = 1.0;
               objective[iBlock] = 1.0e13;
               columnAdd[iBlock+1] = iBlock + 1;
               when[iBlock] = -1;
               whichBlock[iBlock] = iBlock;
          }
          master.addColumns(numberBlocks, NULL, NULL, objective,
                            columnAdd, rowAdd, elementAdd);
     }
     // and resize matrix to double check clp will be happy
     //master.matrix()->setDimensions(numberMasterRows+numberBlocks,
     //			 numberMasterColumns+numberBlocks);
     std::cout << "Time to decompose " << CoinCpuTime() - time1 << " seconds" << std::endl;
     for (iPass = 0; iPass < maxPass; iPass++) {
          printf("Start of pass %d\n", iPass);
          // Solve master - may be infeasible
          //master.scaling(0);
          if (0) {
               master.writeMps("yy.mps");
          }
          // Correct artificials
          double sumArtificials = 0.0;
          if (iPass) {
               double * upper = master.columnUpper();
               double * solution = master.primalColumnSolution();
               double * obj = master.objective();
               sumArtificials = 0.0;
               for (int i = firstArtificial; i < lastArtificial; i++) {
                    sumArtificials += solution[i];
                    //assert (solution[i]>-1.0e-2);
                    if (solution[i] < 1.0e-6) {
#if 0
                         // Could take out
                         obj[i] = 0.0;
                         upper[i] = 0.0;
#else
                         obj[i] = 1.0e7;
                         upper[i] = 1.0e-1;
#endif
                         solution[i] = 0.0;
                         master.setColumnStatus(i, isFixed);
                    } else {
                         upper[i] = solution[i] + 1.0e-5 * (1.0 + solution[i]);
                    }
               }
               printf("Sum of artificials before solve is %g\n", sumArtificials);
          }
          // scale objective to be reasonable
          double scaleFactor = master.scaleObjective(-1.0e9);
          {
               double * dual = master.dualRowSolution();
               int n = master.numberRows();
               memset(dual, 0, n * sizeof(double));
               double * solution = master.primalColumnSolution();
               master.clpMatrix()->times(1.0, solution, dual);
               double sum = 0.0;
               double * lower = master.rowLower();
               double * upper = master.rowUpper();
               for (int iRow = 0; iRow < n; iRow++) {
                    double value = dual[iRow];
                    if (value > upper[iRow])
                         sum += value - upper[iRow];
                    else if (value < lower[iRow])
                         sum -= value - lower[iRow];
               }
               printf("suminf %g\n", sum);
               lower = master.columnLower();
               upper = master.columnUpper();
               n = master.numberColumns();
               for (int iColumn = 0; iColumn < n; iColumn++) {
                    double value = solution[iColumn];
                    if (value > upper[iColumn] + 1.0e-5)
                         sum += value - upper[iColumn];
                    else if (value < lower[iColumn] - 1.0e-5)
                         sum -= value - lower[iColumn];
               }
               printf("suminf %g\n", sum);
          }
          master.primal(1);
          // Correct artificials
          sumArtificials = 0.0;
          {
               double * solution = master.primalColumnSolution();
               for (int i = firstArtificial; i < lastArtificial; i++) {
                    sumArtificials += solution[i];
               }
               printf("Sum of artificials after solve is %g\n", sumArtificials);
          }
          master.scaleObjective(scaleFactor);
          int problemStatus = master.status(); // do here as can change (delcols)
          if (master.numberIterations() == 0 && iPass)
               break; // finished
          if (master.objectiveValue() > lastObjective - 1.0e-7 && iPass > 555)
               break; // finished
          lastObjective = master.objectiveValue();
          // mark basic ones and delete if necessary
          int iColumn;
          numberColumnsGenerated = master.numberColumns() - numberMasterColumns;
          for (iColumn = 0; iColumn < numberColumnsGenerated; iColumn++) {
               if (master.getStatus(iColumn + numberMasterColumns) == ClpSimplex::basic)
                    when[iColumn] = iPass;
          }
          if (numberColumnsGenerated + numberBlocks > maximumColumns) {
               // delete
               int numberKeep = 0;
               int numberDelete = 0;
               int * whichDelete = new int[numberColumnsGenerated];
               for (iColumn = 0; iColumn < numberColumnsGenerated; iColumn++) {
                    if (when[iColumn] > iPass - 7) {
                         // keep
                         when[numberKeep] = when[iColumn];
                         whichBlock[numberKeep++] = whichBlock[iColumn];
                    } else {
                         // delete
                         whichDelete[numberDelete++] = iColumn + numberMasterColumns;
                    }
               }
               numberColumnsGenerated -= numberDelete;
               master.deleteColumns(numberDelete, whichDelete);
               delete [] whichDelete;
          }
          const double * dual = NULL;
          bool deleteDual = false;
          if (problemStatus == 0) {
               dual = master.dualRowSolution();
          } else if (problemStatus == 1) {
               // could do composite objective
               dual = master.infeasibilityRay();
               deleteDual = true;
               printf("The sum of infeasibilities is %g\n",
                      master.sumPrimalInfeasibilities());
          } else if (!master.numberColumns()) {
               assert(!iPass);
               dual = master.dualRowSolution();
               memset(master.dualRowSolution(),
                      0, (numberMasterRows + numberBlocks)*sizeof(double));
          } else {
               abort();
          }
          // Scale back on first time
          if (!iPass) {
               double * dual2 = master.dualRowSolution();
               for (int i = 0; i < numberMasterRows + numberBlocks; i++) {
                    dual2[i] *= 1.0e-7;
               }
               dual = master.dualRowSolution();
          }
          // Create objective for sub problems and solve
          columnAdd[0] = 0;
          int numberProposals = 0;
          for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
               int numberColumns2 = sub[iBlock].numberColumns();
               double * saveObj = new double [numberColumns2];
               double * objective2 = sub[iBlock].objective();
               memcpy(saveObj, objective2, numberColumns2 * sizeof(double));
               // new objective
               top[iBlock]->transposeTimes(dual, objective2);
               int i;
               if (problemStatus == 0) {
                    for (i = 0; i < numberColumns2; i++)
                         objective2[i] = saveObj[i] - objective2[i];
               } else {
                    for (i = 0; i < numberColumns2; i++)
                         objective2[i] = -objective2[i];
               }
               // scale objective to be reasonable
               double scaleFactor =
                    sub[iBlock].scaleObjective((sumArtificials > 1.0e-5) ? -1.0e-4 : -1.0e9);
               if (iPass) {
                    sub[iBlock].primal();
               } else {
                    sub[iBlock].dual();
               }
               sub[iBlock].scaleObjective(scaleFactor);
               if (!sub[iBlock].isProvenOptimal() &&
                         !sub[iBlock].isProvenDualInfeasible()) {
                    memset(objective2, 0, numberColumns2 * sizeof(double));
                    sub[iBlock].primal();
                    if (problemStatus == 0) {
                         for (i = 0; i < numberColumns2; i++)
                              objective2[i] = saveObj[i] - objective2[i];
                    } else {
                         for (i = 0; i < numberColumns2; i++)
                              objective2[i] = -objective2[i];
                    }
                    double scaleFactor = sub[iBlock].scaleObjective(-1.0e9);
                    sub[iBlock].primal(1);
                    sub[iBlock].scaleObjective(scaleFactor);
               }
               memcpy(objective2, saveObj, numberColumns2 * sizeof(double));
               // get proposal
               if (sub[iBlock].numberIterations() || !iPass) {
                    double objValue = 0.0;
                    int start = columnAdd[numberProposals];
                    // proposal
                    if (sub[iBlock].isProvenOptimal()) {
                         const double * solution = sub[iBlock].primalColumnSolution();
                         top[iBlock]->times(solution, elementAdd + start);
                         for (i = 0; i < numberColumns2; i++)
                              objValue += solution[i] * saveObj[i];
                         // See if good dj and pack down
                         int number = start;
                         double dj = objValue;
                         if (problemStatus)
                              dj = 0.0;
                         double smallest = 1.0e100;
                         double largest = 0.0;
                         for (i = 0; i < numberMasterRows; i++) {
                              double value = elementAdd[start+i];
                              if (fabs(value) > 1.0e-15) {
                                   dj -= dual[i] * value;
                                   smallest = CoinMin(smallest, fabs(value));
                                   largest = CoinMax(largest, fabs(value));
                                   rowAdd[number] = i;
                                   elementAdd[number++] = value;
                              }
                         }
                         // and convexity
                         dj -= dual[numberMasterRows+iBlock];
                         rowAdd[number] = numberMasterRows + iBlock;
                         elementAdd[number++] = 1.0;
                         // if elements large then scale?
                         //if (largest>1.0e8||smallest<1.0e-8)
                         printf("For subproblem %d smallest - %g, largest %g - dj %g\n",
                                iBlock, smallest, largest, dj);
                         if (dj < -1.0e-6 || !iPass) {
                              // take
                              objective[numberProposals] = objValue;
                              columnAdd[++numberProposals] = number;
                              when[numberColumnsGenerated] = iPass;
                              whichBlock[numberColumnsGenerated++] = iBlock;
                         }
                    } else if (sub[iBlock].isProvenDualInfeasible()) {
                         // use ray
                         const double * solution = sub[iBlock].unboundedRay();
                         top[iBlock]->times(solution, elementAdd + start);
                         for (i = 0; i < numberColumns2; i++)
                              objValue += solution[i] * saveObj[i];
                         // See if good dj and pack down
                         int number = start;
                         double dj = objValue;
                         double smallest = 1.0e100;
                         double largest = 0.0;
                         for (i = 0; i < numberMasterRows; i++) {
                              double value = elementAdd[start+i];
                              if (fabs(value) > 1.0e-15) {
                                   dj -= dual[i] * value;
                                   smallest = CoinMin(smallest, fabs(value));
                                   largest = CoinMax(largest, fabs(value));
                                   rowAdd[number] = i;
                                   elementAdd[number++] = value;
                              }
                         }
                         // if elements large or small then scale?
                         //if (largest>1.0e8||smallest<1.0e-8)
                         printf("For subproblem ray %d smallest - %g, largest %g - dj %g\n",
                                iBlock, smallest, largest, dj);
                         if (dj < -1.0e-6) {
                              // take
                              objective[numberProposals] = objValue;
                              columnAdd[++numberProposals] = number;
                              when[numberColumnsGenerated] = iPass;
                              whichBlock[numberColumnsGenerated++] = iBlock;
                         }
                    } else {
                         abort();
                    }
               }
               delete [] saveObj;
          }
          if (deleteDual)
               delete [] dual;
          if (numberProposals)
               master.addColumns(numberProposals, NULL, NULL, objective,
                                 columnAdd, rowAdd, elementAdd);
     }
     std::cout << "Time at end of D-W " << CoinCpuTime() - time1 << " seconds" << std::endl;
     //master.scaling(0);
     //master.primal(1);
     loadProblem(*model);
     // now put back a good solution
     double * lower = new double[numberMasterRows+numberBlocks];
     double * upper = new double[numberMasterRows+numberBlocks];
     numberColumnsGenerated  += numberMasterColumns;
     double * sol = new double[numberColumnsGenerated];
     const double * solution = master.primalColumnSolution();
     const double * masterLower = master.rowLower();
     const double * masterUpper = master.rowUpper();
     double * fullSolution = primalColumnSolution();
     const double * fullLower = columnLower();
     const double * fullUpper = columnUpper();
     const double * rowSolution = master.primalRowSolution();
     double * fullRowSolution = primalRowSolution();
     const int * rowBack = model->coinBlock(masterBlock)->originalRows();
     int numberRows2 = model->coinBlock(masterBlock)->numberRows();
     const int * columnBack = model->coinBlock(masterBlock)->originalColumns();
     int numberColumns2 = model->coinBlock(masterBlock)->numberColumns();
     for (int iRow = 0; iRow < numberRows2; iRow++) {
          int kRow = rowBack[iRow];
          setRowStatus(kRow, master.getRowStatus(iRow));
          fullRowSolution[kRow] = rowSolution[iRow];
     }
     for (int iColumn = 0; iColumn < numberColumns2; iColumn++) {
          int kColumn = columnBack[iColumn];
          setStatus(kColumn, master.getStatus(iColumn));
          fullSolution[kColumn] = solution[iColumn];
     }
     for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
          // move basis
          int kBlock = columnCounts[iBlock];
          const int * rowBack = model->coinBlock(kBlock)->originalRows();
          int numberRows2 = model->coinBlock(kBlock)->numberRows();
          const int * columnBack = model->coinBlock(kBlock)->originalColumns();
          int numberColumns2 = model->coinBlock(kBlock)->numberColumns();
          for (int iRow = 0; iRow < numberRows2; iRow++) {
               int kRow = rowBack[iRow];
               setRowStatus(kRow, sub[iBlock].getRowStatus(iRow));
          }
          for (int iColumn = 0; iColumn < numberColumns2; iColumn++) {
               int kColumn = columnBack[iColumn];
               setStatus(kColumn, sub[iBlock].getStatus(iColumn));
          }
          // convert top bit to by rows
          CoinPackedMatrix topMatrix = *top[iBlock];
          topMatrix.reverseOrdering();
          // zero solution
          memset(sol, 0, numberColumnsGenerated * sizeof(double));

          for (int i = numberMasterColumns; i < numberColumnsGenerated; i++) {
               if (whichBlock[i-numberMasterColumns] == iBlock)
                    sol[i] = solution[i];
          }
          memset(lower, 0, (numberMasterRows + numberBlocks)*sizeof(double));
          master.clpMatrix()->times(1.0, sol, lower);
          for (int iRow = 0; iRow < numberMasterRows; iRow++) {
               double value = lower[iRow];
               if (masterUpper[iRow] < 1.0e20)
                    upper[iRow] = value;
               else
                    upper[iRow] = COIN_DBL_MAX;
               if (masterLower[iRow] > -1.0e20)
                    lower[iRow] = value;
               else
                    lower[iRow] = -COIN_DBL_MAX;
          }
          sub[iBlock].addRows(numberMasterRows, lower, upper,
                              topMatrix.getVectorStarts(),
                              topMatrix.getVectorLengths(),
                              topMatrix.getIndices(),
                              topMatrix.getElements());
          sub[iBlock].primal(1);
          const double * subSolution = sub[iBlock].primalColumnSolution();
          const double * subRowSolution = sub[iBlock].primalRowSolution();
          // move solution
          for (int iRow = 0; iRow < numberRows2; iRow++) {
               int kRow = rowBack[iRow];
               fullRowSolution[kRow] = subRowSolution[iRow];
          }
          for (int iColumn = 0; iColumn < numberColumns2; iColumn++) {
               int kColumn = columnBack[iColumn];
               fullSolution[kColumn] = subSolution[iColumn];
          }
     }
     for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
          if (fullSolution[iColumn] < fullUpper[iColumn] - 1.0e-8 &&
                    fullSolution[iColumn] > fullLower[iColumn] + 1.0e-8) {
               if (getStatus(iColumn) != ClpSimplex::basic) {
                    if (columnLower_[iColumn] > -1.0e30 ||
                              columnUpper_[iColumn] < 1.0e30)
                         setStatus(iColumn, ClpSimplex::superBasic);
                    else
                         setStatus(iColumn, ClpSimplex::isFree);
               }
          } else if (fullSolution[iColumn] >= fullUpper[iColumn] - 1.0e-8) {
               // may help to make rest non basic
               if (getStatus(iColumn) != ClpSimplex::basic)
                    setStatus(iColumn, ClpSimplex::atUpperBound);
          } else if (fullSolution[iColumn] <= fullLower[iColumn] + 1.0e-8) {
               // may help to make rest non basic
               if (getStatus(iColumn) != ClpSimplex::basic)
                    setStatus(iColumn, ClpSimplex::atLowerBound);
          }
     }
     //int numberRows=model->numberRows();
     //for (int iRow=0;iRow<numberRows;iRow++)
     //setRowStatus(iRow,ClpSimplex::superBasic);
     std::cout << "Time before cleanup of full model " << CoinCpuTime() - time1 << " seconds" << std::endl;
     primal(1);
     std::cout << "Total time " << CoinCpuTime() - time1 << " seconds" << std::endl;
     delete [] columnCounts;
     delete [] sol;
     delete [] lower;
     delete [] upper;
     delete [] whichBlock;
     delete [] when;
     delete [] columnAdd;
     delete [] rowAdd;
     delete [] elementAdd;
     delete [] objective;
     delete [] top;
     delete [] sub;
     return 0;
}
// Solve using Benders decomposition and maybe in parallel
int
ClpSimplex::solveBenders(CoinStructuredModel * model)
{
     double time1 = CoinCpuTime();
     //int numberColumns = model->numberColumns();
     int numberRowBlocks = model->numberRowBlocks();
     int numberColumnBlocks = model->numberColumnBlocks();
     int numberElementBlocks = model->numberElementBlocks();
     // We already have top level structure
     CoinModelBlockInfo * blockInfo = new CoinModelBlockInfo [numberElementBlocks];
     for (int i = 0; i < numberElementBlocks; i++) {
          CoinModel * thisBlock = model->coinBlock(i);
          assert (thisBlock);
          // just fill in info
          CoinModelBlockInfo info = CoinModelBlockInfo();
          int whatsSet = thisBlock->whatIsSet();
          info.matrix = static_cast<char>(((whatsSet & 1) != 0) ? 1 : 0);
          info.rhs = static_cast<char>(((whatsSet & 2) != 0) ? 1 : 0);
          info.rowName = static_cast<char>(((whatsSet & 4) != 0) ? 1 : 0);
          info.integer = static_cast<char>(((whatsSet & 32) != 0) ? 1 : 0);
          info.bounds = static_cast<char>(((whatsSet & 8) != 0) ? 1 : 0);
          info.columnName = static_cast<char>(((whatsSet & 16) != 0) ? 1 : 0);
          // Which block
          int iRowBlock = model->rowBlock(thisBlock->getRowBlock());
          info.rowBlock = iRowBlock;
          int iColumnBlock = model->columnBlock(thisBlock->getColumnBlock());
          info.columnBlock = iColumnBlock;
          blockInfo[i] = info;
     }
     // make up problems
     int numberBlocks = numberColumnBlocks - 1;
     // Find master columns and rows
     int * columnCounts = new int [numberColumnBlocks];
     CoinZeroN(columnCounts, numberColumnBlocks);
     int * rowCounts = new int [numberRowBlocks+1];
     CoinZeroN(rowCounts, numberRowBlocks);
     int iBlock;
     for (iBlock = 0; iBlock < numberElementBlocks; iBlock++) {
          int iColumnBlock = blockInfo[iBlock].columnBlock;
          columnCounts[iColumnBlock]++;
          int iRowBlock = blockInfo[iBlock].rowBlock;
          rowCounts[iRowBlock]++;
     }
     int * whichBlock = new int [numberElementBlocks];
     int masterColumnBlock = -1;
     for (iBlock = 0; iBlock < numberColumnBlocks; iBlock++) {
          if (columnCounts[iBlock] > 1) {
               if (masterColumnBlock == -1) {
                    masterColumnBlock = iBlock;
               } else {
                    // Can't decode
                    masterColumnBlock = -2;
                    break;
               }
          }
     }
     int masterRowBlock = -1;
     int kBlock = 0;
     for (iBlock = 0; iBlock < numberRowBlocks; iBlock++) {
          int count = rowCounts[iBlock];
          rowCounts[iBlock] = kBlock;
          kBlock += count;
     }
     for (iBlock = 0; iBlock < numberElementBlocks; iBlock++) {
          int iRowBlock = blockInfo[iBlock].rowBlock;
          whichBlock[rowCounts[iRowBlock]] = iBlock;
          rowCounts[iRowBlock]++;
     }
     for (iBlock = numberRowBlocks - 1; iBlock >= 0; iBlock--)
          rowCounts[iBlock+1] = rowCounts[iBlock];
     rowCounts[0] = 0;
     for (iBlock = 0; iBlock < numberRowBlocks; iBlock++) {
          int count = rowCounts[iBlock+1] - rowCounts[iBlock];
          if (count == 1) {
               int kBlock = whichBlock[rowCounts[iBlock]];
               int iColumnBlock = blockInfo[kBlock].columnBlock;
               if (iColumnBlock == masterColumnBlock) {
                    if (masterRowBlock == -1) {
                         masterRowBlock = iBlock;
                    } else {
                         // Can't decode
                         masterRowBlock = -2;
                         break;
                    }
               }
          }
     }
     if (masterColumnBlock < 0 || masterRowBlock == -2) {
          // What now
          abort();
     }
     delete [] columnCounts;
     // create all data
     const CoinPackedMatrix ** first = new const CoinPackedMatrix * [numberRowBlocks];
     ClpSimplex * sub = new ClpSimplex [numberBlocks];
     ClpSimplex master;
     // Set offset
     master.setObjectiveOffset(model->objectiveOffset());
     kBlock = 0;
     int masterBlock = -1;
     for (iBlock = 0; iBlock < numberRowBlocks; iBlock++) {
          first[kBlock] = NULL;
          int start = rowCounts[iBlock];
          int end = rowCounts[iBlock+1];
          assert (end - start <= 2);
          for (int j = start; j < end; j++) {
               int jBlock = whichBlock[j];
               int iColumnBlock = blockInfo[jBlock].columnBlock;
               int iRowBlock = blockInfo[jBlock].rowBlock;
               assert (iRowBlock == iBlock);
               if (iRowBlock != masterRowBlock && iColumnBlock == masterColumnBlock) {
                    // first matrix
                    first[kBlock] = model->coinBlock(jBlock)->packedMatrix();
               } else {
                    const CoinPackedMatrix * matrix
                    = model->coinBlock(jBlock)->packedMatrix();
                    // Get pointers to arrays
                    const double * columnLower;
                    const double * columnUpper;
                    const double * rowLower;
                    const double * rowUpper;
                    const double * objective;
                    model->block(iRowBlock, iColumnBlock, rowLower, rowUpper,
                                 columnLower, columnUpper, objective);
                    if (iRowBlock != masterRowBlock) {
                         // diagonal block
                         sub[kBlock].loadProblem(*matrix, columnLower, columnUpper,
                                                 objective, rowLower, rowUpper);
                         if (optimizationDirection_ < 0.0) {
                              double * obj = sub[kBlock].objective();
                              int n = sub[kBlock].numberColumns();
                              for (int i = 0; i < n; i++)
                                   obj[i] = - obj[i];
                         }
                         if (this->factorizationFrequency() == 200) {
                              // User did not touch preset
                              sub[kBlock].defaultFactorizationFrequency();
                         } else {
                              // make sure model has correct value
                              sub[kBlock].setFactorizationFrequency(this->factorizationFrequency());
                         }
                         sub[kBlock].setPerturbation(50);
                         // Set rowCounts to be diagonal block index for cleanup
                         rowCounts[kBlock] = jBlock;
                    } else {
                         // master
                         masterBlock = jBlock;
                         master.loadProblem(*matrix, columnLower, columnUpper,
                                            objective, rowLower, rowUpper);
                         if (optimizationDirection_ < 0.0) {
                              double * obj = master.objective();
                              int n = master.numberColumns();
                              for (int i = 0; i < n; i++)
                                   obj[i] = - obj[i];
                         }
                    }
               }
          }
          if (iBlock != masterRowBlock)
               kBlock++;
     }
     delete [] whichBlock;
     delete [] blockInfo;
     // For now master must have been defined (does not have to have rows)
     assert (master.numberColumns());
     assert (masterBlock >= 0);
     int numberMasterColumns = master.numberColumns();
     // Overkill in terms of space
     int spaceNeeded = CoinMax(numberBlocks * (numberMasterColumns + 1),
                               2 * numberMasterColumns);
     int * columnAdd = new int[spaceNeeded];
     double * elementAdd = new double[spaceNeeded];
     spaceNeeded = numberBlocks;
     int * rowAdd = new int[spaceNeeded+1];
     double * objective = new double[spaceNeeded];
     int maxPass = 500;
     int iPass;
     double lastObjective = -1.0e31;
     // Create columns for proposals
     int numberMasterRows = master.numberRows();
     master.resize(numberMasterColumns + numberBlocks, numberMasterRows);
     if (this->factorizationFrequency() == 200) {
          // User did not touch preset
          master.defaultFactorizationFrequency();
     } else {
          // make sure model has correct value
          master.setFactorizationFrequency(this->factorizationFrequency());
     }
     master.setPerturbation(50);
     // Arrays to say which block and when created
     int maximumRows = 2 * numberMasterColumns + 10 * numberBlocks;
     whichBlock = new int[maximumRows];
     int * when = new int[maximumRows];
     int numberRowsGenerated = numberBlocks;
     // Add extra variables
     {
          int iBlock;
          columnAdd[0] = 0;
          for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
               objective[iBlock] = 1.0;
               columnAdd[iBlock+1] = 0;
               when[iBlock] = -1;
               whichBlock[iBlock] = iBlock;
          }
          master.addColumns(numberBlocks, NULL, NULL, objective,
                            columnAdd, rowAdd, elementAdd);
     }
     std::cout << "Time to decompose " << CoinCpuTime() - time1 << " seconds" << std::endl;
     for (iPass = 0; iPass < maxPass; iPass++) {
          printf("Start of pass %d\n", iPass);
          // Solve master - may be unbounded
          //master.scaling(0);
          if (1) {
               master.writeMps("yy.mps");
          }
          master.dual();
          int problemStatus = master.status(); // do here as can change (delcols)
          if (master.numberIterations() == 0 && iPass)
               break; // finished
          if (master.objectiveValue() < lastObjective + 1.0e-7 && iPass > 555)
               break; // finished
          lastObjective = master.objectiveValue();
          // mark non-basic rows and delete if necessary
          int iRow;
          numberRowsGenerated = master.numberRows() - numberMasterRows;
          for (iRow = 0; iRow < numberRowsGenerated; iRow++) {
               if (master.getStatus(iRow + numberMasterRows) != ClpSimplex::basic)
                    when[iRow] = iPass;
          }
          if (numberRowsGenerated > maximumRows) {
               // delete
               int numberKeep = 0;
               int numberDelete = 0;
               int * whichDelete = new int[numberRowsGenerated];
               for (iRow = 0; iRow < numberRowsGenerated; iRow++) {
                    if (when[iRow] > iPass - 7) {
                         // keep
                         when[numberKeep] = when[iRow];
                         whichBlock[numberKeep++] = whichBlock[iRow];
                    } else {
                         // delete
                         whichDelete[numberDelete++] = iRow + numberMasterRows;
                    }
               }
               numberRowsGenerated -= numberDelete;
               master.deleteRows(numberDelete, whichDelete);
               delete [] whichDelete;
          }
          const double * primal = NULL;
          bool deletePrimal = false;
          if (problemStatus == 0) {
               primal = master.primalColumnSolution();
          } else if (problemStatus == 2) {
               // could do composite objective
               primal = master.unboundedRay();
               deletePrimal = true;
               printf("The sum of infeasibilities is %g\n",
                      master.sumPrimalInfeasibilities());
          } else if (!master.numberRows()) {
               assert(!iPass);
               primal = master.primalColumnSolution();
               memset(master.primalColumnSolution(),
                      0, numberMasterColumns * sizeof(double));
          } else {
               abort();
          }
          // Create rhs for sub problems and solve
          rowAdd[0] = 0;
          int numberProposals = 0;
          for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
               int numberRows2 = sub[iBlock].numberRows();
               double * saveLower = new double [numberRows2];
               double * lower2 = sub[iBlock].rowLower();
               double * saveUpper = new double [numberRows2];
               double * upper2 = sub[iBlock].rowUpper();
               // new rhs
               CoinZeroN(saveUpper, numberRows2);
               first[iBlock]->times(primal, saveUpper);
               for (int i = 0; i < numberRows2; i++) {
                    double value = saveUpper[i];
                    saveLower[i] = lower2[i];
                    saveUpper[i] = upper2[i];
                    if (saveLower[i] > -1.0e30)
                         lower2[i] -= value;
                    if (saveUpper[i] < 1.0e30)
                         upper2[i] -= value;
               }
               sub[iBlock].dual();
               memcpy(lower2, saveLower, numberRows2 * sizeof(double));
               memcpy(upper2, saveUpper, numberRows2 * sizeof(double));
               // get proposal
               if (sub[iBlock].numberIterations() || !iPass) {
                    double objValue = 0.0;
                    int start = rowAdd[numberProposals];
                    // proposal
                    if (sub[iBlock].isProvenOptimal()) {
                         const double * solution = sub[iBlock].dualRowSolution();
                         first[iBlock]->transposeTimes(solution, elementAdd + start);
                         for (int i = 0; i < numberRows2; i++) {
                              if (solution[i] < -dualTolerance_) {
                                   // at upper
                                   assert (saveUpper[i] < 1.0e30);
                                   objValue += solution[i] * saveUpper[i];
                              } else if (solution[i] > dualTolerance_) {
                                   // at lower
                                   assert (saveLower[i] > -1.0e30);
                                   objValue += solution[i] * saveLower[i];
                              }
                         }

                         // See if cuts off and pack down
                         int number = start;
                         double infeas = objValue;
                         double smallest = 1.0e100;
                         double largest = 0.0;
                         for (int i = 0; i < numberMasterColumns; i++) {
                              double value = elementAdd[start+i];
                              if (fabs(value) > 1.0e-15) {
                                   infeas -= primal[i] * value;
                                   smallest = CoinMin(smallest, fabs(value));
                                   largest = CoinMax(largest, fabs(value));
                                   columnAdd[number] = i;
                                   elementAdd[number++] = -value;
                              }
                         }
                         columnAdd[number] = numberMasterColumns + iBlock;
                         elementAdd[number++] = -1.0;
                         // if elements large then scale?
                         //if (largest>1.0e8||smallest<1.0e-8)
                         printf("For subproblem %d smallest - %g, largest %g - infeas %g\n",
                                iBlock, smallest, largest, infeas);
                         if (infeas < -1.0e-6 || !iPass) {
                              // take
                              objective[numberProposals] = objValue;
                              rowAdd[++numberProposals] = number;
                              when[numberRowsGenerated] = iPass;
                              whichBlock[numberRowsGenerated++] = iBlock;
                         }
                    } else if (sub[iBlock].isProvenPrimalInfeasible()) {
                         // use ray
                         const double * solution = sub[iBlock].infeasibilityRay();
                         first[iBlock]->transposeTimes(solution, elementAdd + start);
                         for (int i = 0; i < numberRows2; i++) {
                              if (solution[i] < -dualTolerance_) {
                                   // at upper
                                   assert (saveUpper[i] < 1.0e30);
                                   objValue += solution[i] * saveUpper[i];
                              } else if (solution[i] > dualTolerance_) {
                                   // at lower
                                   assert (saveLower[i] > -1.0e30);
                                   objValue += solution[i] * saveLower[i];
                              }
                         }
                         // See if good infeas and pack down
                         int number = start;
                         double infeas = objValue;
                         double smallest = 1.0e100;
                         double largest = 0.0;
                         for (int i = 0; i < numberMasterColumns; i++) {
                              double value = elementAdd[start+i];
                              if (fabs(value) > 1.0e-15) {
                                   infeas -= primal[i] * value;
                                   smallest = CoinMin(smallest, fabs(value));
                                   largest = CoinMax(largest, fabs(value));
                                   columnAdd[number] = i;
                                   elementAdd[number++] = -value;
                              }
                         }
                         // if elements large or small then scale?
                         //if (largest>1.0e8||smallest<1.0e-8)
                         printf("For subproblem ray %d smallest - %g, largest %g - infeas %g\n",
                                iBlock, smallest, largest, infeas);
                         if (infeas < -1.0e-6) {
                              // take
                              objective[numberProposals] = objValue;
                              rowAdd[++numberProposals] = number;
                              when[numberRowsGenerated] = iPass;
                              whichBlock[numberRowsGenerated++] = iBlock;
                         }
                    } else {
                         abort();
                    }
               }
               delete [] saveLower;
               delete [] saveUpper;
          }
          if (deletePrimal)
               delete [] primal;
          if (numberProposals) {
               master.addRows(numberProposals, NULL, objective,
                              rowAdd, columnAdd, elementAdd);
          }
     }
     std::cout << "Time at end of Benders " << CoinCpuTime() - time1 << " seconds" << std::endl;
     //master.scaling(0);
     //master.primal(1);
     loadProblem(*model);
     // now put back a good solution
     const double * columnSolution = master.primalColumnSolution();
     double * fullColumnSolution = primalColumnSolution();
     const int * columnBack = model->coinBlock(masterBlock)->originalColumns();
     int numberColumns2 = model->coinBlock(masterBlock)->numberColumns();
     const int * rowBack = model->coinBlock(masterBlock)->originalRows();
     int numberRows2 = model->coinBlock(masterBlock)->numberRows();
     for (int iColumn = 0; iColumn < numberColumns2; iColumn++) {
          int kColumn = columnBack[iColumn];
          setColumnStatus(kColumn, master.getColumnStatus(iColumn));
          fullColumnSolution[kColumn] = columnSolution[iColumn];
     }
     for (int iRow = 0; iRow < numberRows2; iRow++) {
          int kRow = rowBack[iRow];
          setStatus(kRow, master.getStatus(iRow));
          //fullSolution[kRow]=solution[iRow];
     }
     for (iBlock = 0; iBlock < numberBlocks; iBlock++) {
          // move basis
          int kBlock = rowCounts[iBlock];
          const int * columnBack = model->coinBlock(kBlock)->originalColumns();
          int numberColumns2 = model->coinBlock(kBlock)->numberColumns();
          const int * rowBack = model->coinBlock(kBlock)->originalRows();
          int numberRows2 = model->coinBlock(kBlock)->numberRows();
          const double * subColumnSolution = sub[iBlock].primalColumnSolution();
          for (int iColumn = 0; iColumn < numberColumns2; iColumn++) {
               int kColumn = columnBack[iColumn];
               setColumnStatus(kColumn, sub[iBlock].getColumnStatus(iColumn));
               fullColumnSolution[kColumn] = subColumnSolution[iColumn];
          }
          for (int iRow = 0; iRow < numberRows2; iRow++) {
               int kRow = rowBack[iRow];
               setStatus(kRow, sub[iBlock].getStatus(iRow));
               setStatus(kRow, atLowerBound);
          }
     }
     double * fullSolution = primalRowSolution();
     CoinZeroN(fullSolution, numberRows_);
     times(1.0, fullColumnSolution, fullSolution);
     //int numberColumns=model->numberColumns();
     //for (int iColumn=0;iColumn<numberColumns;iColumn++)
     //setColumnStatus(iColumn,ClpSimplex::superBasic);
     std::cout << "Time before cleanup of full model " << CoinCpuTime() - time1 << " seconds" << std::endl;
     this->primal(1);
     std::cout << "Total time " << CoinCpuTime() - time1 << " seconds" << std::endl;
     delete [] rowCounts;
     //delete [] sol;
     //delete [] lower;
     //delete [] upper;
     delete [] whichBlock;
     delete [] when;
     delete [] rowAdd;
     delete [] columnAdd;
     delete [] elementAdd;
     delete [] objective;
     delete [] first;
     delete [] sub;
     return 0;
}
