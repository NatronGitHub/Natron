/* $Id: CoinOslFactorization.cpp 1585 2013-04-06 20:42:02Z stefan $ */
// Copyright (C) 1987, 2009, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinUtilsConfig.h"

#include <cassert>
#include "CoinPragma.hpp"
#include "CoinOslFactorization.hpp"
#include "CoinOslC.h"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinTypes.hpp"
#include "CoinFinite.hpp"
#include <stdio.h>
static void c_ekksmem(EKKfactinfo *fact,int numberRows,int maximumPivots);
static void c_ekksmem_copy(EKKfactinfo *fact,const EKKfactinfo * rhsFact);
static void c_ekksmem_delete(EKKfactinfo *fact);
//:class CoinOslFactorization.  Deals with Factorization and Updates
//  CoinOslFactorization.  Constructor
CoinOslFactorization::CoinOslFactorization (  )
  : CoinOtherFactorization()
{
  gutsOfInitialize();
}

/// Copy constructor 
CoinOslFactorization::CoinOslFactorization ( const CoinOslFactorization &other)
  : CoinOtherFactorization(other)
{
  gutsOfInitialize();
  gutsOfCopy(other);
}
// Clone
CoinOtherFactorization * 
CoinOslFactorization::clone() const 
{
  return new CoinOslFactorization(*this);
}
/// The real work of constructors etc
void CoinOslFactorization::gutsOfDestructor(bool clearFact)
{
  delete [] elements_;
  delete [] pivotRow_;
  delete [] workArea_;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
  numberRows_ = 0;
  numberColumns_ = 0;
  numberGoodU_ = 0;
  status_ = -1;
  maximumRows_=0;
  maximumSpace_=0;
  solveMode_=0;
  if (clearFact)
    c_ekksmem_delete(&factInfo_);
}
void CoinOslFactorization::gutsOfInitialize(bool zapFact)
{
  pivotTolerance_ = 1.0e-1;
  zeroTolerance_ = 1.0e-13;
#ifndef COIN_FAST_CODE
  slackValue_ = -1.0;
#endif
  maximumPivots_=200;
  relaxCheck_=1.0;
  numberRows_ = 0;
  numberColumns_ = 0;
  numberGoodU_ = 0;
  status_ = -1;
  numberPivots_ = 0;
  maximumRows_=0;
  maximumSpace_=0;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
  solveMode_=0;
  if (zapFact) {
    memset(&factInfo_,0,sizeof(factInfo_));
    factInfo_.maxinv=100;
    factInfo_.drtpiv=1.0e-10;
    factInfo_.zeroTolerance=1.0e-12;
    factInfo_.zpivlu=0.1;
    factInfo_.areaFactor=1.0;
    factInfo_.nbfinv=100;
  }
}
//  ~CoinOslFactorization.  Destructor
CoinOslFactorization::~CoinOslFactorization (  )
{
  gutsOfDestructor();
}
//  =
CoinOslFactorization & CoinOslFactorization::operator = ( const CoinOslFactorization & other ) {
  if (this != &other) {    
    bool noGood = factInfo_.nrowmx!=other.factInfo_.nrowmx&&
      factInfo_.eta_size!=other.factInfo_.eta_size;
    gutsOfDestructor(noGood);
    gutsOfInitialize(noGood);
    gutsOfCopy(other);
  }
  return *this;
}
#define WORK_MULT 2
void CoinOslFactorization::gutsOfCopy(const CoinOslFactorization &other)
{
  pivotTolerance_ = other.pivotTolerance_;
  zeroTolerance_ = other.zeroTolerance_;
#ifndef COIN_FAST_CODE
  slackValue_ = other.slackValue_;
#endif
  relaxCheck_ = other.relaxCheck_;
  numberRows_ = other.numberRows_;
  numberColumns_ = other.numberColumns_;
  maximumRows_ = other.maximumRows_;
  maximumSpace_ = other.maximumSpace_;
  solveMode_ = other.solveMode_;
  numberGoodU_ = other.numberGoodU_;
  maximumPivots_ = other.maximumPivots_;
  numberPivots_ = other.numberPivots_;
  factorElements_ = other.factorElements_;
  status_ = other.status_;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
  c_ekksmem_copy(&factInfo_,&other.factInfo_);
}

//  getAreas.  Gets space for a factorization
//called by constructors
void
CoinOslFactorization::getAreas ( int numberOfRows,
			 int numberOfColumns,
			 CoinBigIndex maximumL,
			 CoinBigIndex maximumU )
{

  numberRows_ = numberOfRows;
  numberColumns_ = numberOfColumns;
  CoinBigIndex size = static_cast<CoinBigIndex>(factInfo_.areaFactor*
						(maximumL+maximumU));
  factInfo_.zeroTolerance=zeroTolerance_;
  // If wildly out redo
  if (maximumRows_>numberRows_+1000) {
    maximumRows_=0;
    maximumSpace_=0;
    factInfo_.last_eta_size=0;
  }
  if (size>maximumSpace_) {
    //delete [] elements_;
    //elements_ = new CoinFactorizationDouble [size];
    maximumSpace_ = size;
  }
  factInfo_.lastEtaCount = factInfo_.nnentu+factInfo_.nnentl;
  int oldnnetas=factInfo_.last_eta_size;
  // If we are going to increase then be on safe side
  if (size>oldnnetas) 
    size = static_cast<int>(1.1*size);
  factInfo_.eta_size=CoinMax(size,oldnnetas);
  //printf("clp size %d, old %d now %d - iteration %d - last count %d - rows %d,%d,%d\n",
  // size,oldnnetas,factInfo_.eta_size,factInfo_.iterno,factInfo_.lastEtaCount,
  //numberRows_,factInfo_.nrowmx,factInfo_.nrow);
  //if (!factInfo_.iterno) {
  //printf("here\n");
  //}
  /** Get solve mode e.g. 0 C++ code, 1 Lapack, 2 choose
      If 4 set then values pass
      if 8 set then has iterated
  */
  solveMode_ &= 4+8; // clear bottom bits
  factInfo_.ifvsol= ((solveMode_&4)!=0) ? 1 : 0;
  if ((solveMode_&8)!=0) {
    factInfo_.ifvsol=0;
    factInfo_.invok=1; 
  } else {
    factInfo_.iter0=factInfo_.iterno;
    factInfo_.invok=-1;
    factInfo_.if_sparse_update=0;
  }
#if 0
  if (!factInfo_.if_sparse_update &&
      factInfo_.iterno>factInfo_.iter0 &&
      numberRows_>=C_EKK_GO_SPARSE) {
    printf("count %d rows %d etasize %d\n",
	   factInfo_.lastEtaCount,factInfo_.nrow,factInfo_.eta_size);
    
  }
#endif 
  if (!factInfo_.if_sparse_update &&
      factInfo_.iterno>factInfo_.iter0 &&
      numberRows_>=C_EKK_GO_SPARSE &&
      (factInfo_.lastEtaCount>>2)<factInfo_.nrow&&
      !factInfo_.switch_off_sparse_update) {
#if PRINT_DEBUG
    printf("**** Switching on sparse update - etacount\n");
#endif
    /* I suspect this can go into c_ekksslvf;
     * if c_ekkshff decides to switch sparse_update off,
     * then it problably always switches it off (?)
     */
    factInfo_.if_sparse_update=2;
  }
  c_ekksmem(&factInfo_,numberRows_,maximumPivots_);
  if (numberRows_>maximumRows_) { 
    maximumRows_ = numberRows_;
    //delete [] pivotRow_;
    //delete [] workArea_; 
    //pivotRow_ = new int [2*maximumRows_+maximumPivots_];
    //workArea_ = new CoinFactorizationDouble [maximumRows_*WORK_MULT];
  }
}  

//  preProcess.  
void
CoinOslFactorization::preProcess ()
{
  factInfo_.zpivlu=pivotTolerance_;
  // Go to Fortran  
  int * hcoli=factInfo_.xecadr+1;
  int * indexRowU = factInfo_.xeradr+1;
  CoinBigIndex * startColumnU=factInfo_.xcsadr+1;
  for (int i=0;i<numberRows_;i++) {
    int start = startColumnU[i];
    startColumnU[i]++; // to Fortran
    for (int j=start;j<startColumnU[i+1];j++) {
      indexRowU[j]++; // to Fortran
      hcoli[j]=i+1; // to Fortran     
    }
  }
  startColumnU[numberRows_]++; // to Fortran

  /* can do in column order - no zeros or duplicates */
#ifndef NDEBUG
  int ninbas =
#endif 
    c_ekkslcf(&factInfo_);
  assert (ninbas>0);
}

//Does factorization
int
CoinOslFactorization::factor ( )
{
  /*     Uwe's factorization (sort of) */
  int irtcod = c_ekklfct(&factInfo_);
  
  /*       Check return code */
  /*       0 - Fine , 1 - Backtrack, 2 - Singularities on initial, 3-Fatal */
  /*       now 5-Need more memory */
  
  status_= 0;
  if (factInfo_.eta_size>factInfo_.last_eta_size) {
    factInfo_.areaFactor *= factInfo_.eta_size;
    factInfo_.areaFactor /= factInfo_.last_eta_size;
#ifdef CLP_INVESTIGATE
    printf("areaFactor increased to %g\n",factInfo_.areaFactor);
#endif
  }
  if (irtcod==5) {
    status_=-99;
    assert (factInfo_.eta_size>factInfo_.last_eta_size) ;
#ifdef CLP_INVESTIGATE
    printf("need more memory\n");
#endif
  } else if (irtcod) {
    status_=-1;
    //printf("singular %d\n",irtcod);
  }
  return status_;
}
// Makes a non-singular basis by replacing variables
void 
CoinOslFactorization::makeNonSingular(int * sequence, int numberColumns)
{
  const EKKHlink *rlink	= factInfo_.kp1adr;
  const EKKHlink *clink	= factInfo_.kp2adr;
  int nextRow=0;
  //int * mark = reinterpret_cast<int *>(factInfo_.kw1adr);
  //int nr=0;
  //int nc=0;
#if 0  
  for (int i=0;i<numberRows_;i++) {
    //mark[i]=-1;
    if (rlink[i].pre>=0||rlink[i].pre==-(numberRows_+1)) {
      nr++;
      printf("%d rl %d cl %d\n",i,rlink[i].pre,clink[i].pre);
    }
    if (clink[i].pre>=0||clink[i].pre==-(numberRows_+1)) {
      nc++;
      printf("%d rl %d cl %d\n",i,rlink[i].pre,clink[i].pre);
    }
  }
#endif
  //printf("nr %d nc %d\n",nr,nc);
#ifndef NDEBUG
  bool goodPass=true;
#endif
  int numberDone=0;
  for (int i=0;i<numberRows_;i++) {
    int cRow =(-clink[i].pre)-1;
    if (cRow==numberRows_||cRow<0) {
      // throw out
      for (;nextRow<numberRows_;nextRow++) {
	int rRow =(-rlink[nextRow].pre)-1;
	if (rRow==numberRows_||rRow<0)
	  break;  
      }
      if (nextRow<numberRows_) {
	sequence[i]=nextRow+numberColumns;
	nextRow++;
	numberDone++;
      } else {
#ifndef NDEBUG
	goodPass=false;
#endif
	assert(numberDone);
	//printf("BAD singular at row %d\n",i);
	break;
      }
    }
  }
#ifndef NDEBUG
  if (goodPass) { 
    for (;nextRow<numberRows_;nextRow++) {
      int rRow =(-rlink[nextRow].pre)-1;
      assert (!(rRow==numberRows_||rRow<0)); 
    }
  }
#endif
}
// Does post processing on valid factorization - putting variables on correct rows  
void 
CoinOslFactorization::postProcess(const int * sequence, int * pivotVariable)
{
  factInfo_.iterin=factInfo_.iterno;
  factInfo_.npivots=0;
  numberPivots_=0;
  const int * permute3 = factInfo_.mpermu+1;
  assert (permute3==reinterpret_cast<const int *> 
	  (factInfo_.kadrpm+numberRows_+1));
  // this is ridiculous - must be better way
  int * permute2 = reinterpret_cast<int *>(factInfo_.kw1adr);
  const int * permute = reinterpret_cast<const int *>(factInfo_.kp2adr);
  for (int i=0;i<numberRows_;i++) {
    permute2[permute[i]-1]=i;
  }
  for (int i=0;i<numberRows_;i++) {
    // the row is i
    // the column is whatever matches k3[i] in k1
    int look=permute3[i]-1;
    int j=permute2[look];
    int k = sequence[j];
    pivotVariable[i]=k;
  }
#ifdef CLP_REUSE_ETAS
  int * start = factInfo_.xcsadr+1;
  int * putSeq = factInfo_.xrsadr+2*factInfo_.nrowmx+2;
  int * position = putSeq+factInfo_.maxinv;
  int * putStart = position+factInfo_.maxinv;
  memcpy(putStart,start,numberRows_*sizeof(int));
  int iLast=start[numberRows_-1];
  putStart[numberRows_]=iLast+factInfo_.xeradr[iLast]+1;
  factInfo_.save_nnentu=factInfo_.nnentu;
#endif
#ifndef NDEBUG
  {
    int lstart=numberRows_+factInfo_.maxinv+5;
    int ndo = factInfo_.xnetal-lstart;
    double * dluval=factInfo_.xeeadr;
    int * mcstrt = factInfo_.xcsadr+lstart;
    if (ndo)
      assert (dluval[mcstrt[ndo]+1]<1.0e50);
  }
#endif
}
/* Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
   If checkBeforeModifying is true will do all accuracy checks
   before modifying factorization.  Whether to set this depends on
   speed considerations.  You could just do this on first iteration
   after factorization and thereafter re-factorize
   partial update already in U */
int 
CoinOslFactorization::replaceColumn ( CoinIndexedVector * regionSparse,
					int pivotRow,
					double pivotCheck ,
				      bool /*checkBeforeModifying*/,
				       double acceptablePivot)
{
  if (numberPivots_+1==maximumPivots_)
    return 3;
  int *regionIndex = regionSparse->getIndices (  );
  double *region = regionSparse->denseVector (  );
  int orig_nincol=0;
  double saveTolerance = factInfo_.drtpiv;
  factInfo_.drtpiv=acceptablePivot;
  int returnCode=c_ekketsj(&factInfo_,region-1,
			 regionIndex,
			 pivotCheck,orig_nincol,
			 numberPivots_,&factInfo_.nuspike,
			 pivotRow+1,
			 reinterpret_cast<int *>(factInfo_.kw1adr));
  factInfo_.drtpiv=saveTolerance;
  if (returnCode!=2)
    numberPivots_++;
#ifndef NDEBUG
  {
    int lstart=numberRows_+factInfo_.maxinv+5;
    int ndo = factInfo_.xnetal-lstart;
    double * dluval=factInfo_.xeeadr;
    int * mcstrt = factInfo_.xcsadr+lstart;
    if (ndo)
      assert (dluval[mcstrt[ndo]+1]<1.0e50);
  }
#endif
  return returnCode;
}
/* This version has same effect as above with FTUpdate==false
   so number returned is always >=0 */
int 
CoinOslFactorization::updateColumn ( CoinIndexedVector * regionSparse,
				       CoinIndexedVector * regionSparse2,
				     bool /*noPermute*/) const
{
#ifndef NDEBUG
  {
    int lstart=numberRows_+factInfo_.maxinv+5;
    int ndo = factInfo_.xnetal-lstart;
    double * dluval=factInfo_.xeeadr;
    int * mcstrt = factInfo_.xcsadr+lstart;
    if (ndo)
      assert (dluval[mcstrt[ndo]+1]<1.0e50);
  }
#endif
  assert (numberRows_==numberColumns_);
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex2 = regionSparse2->getIndices (  );
  int numberNonZero = regionSparse2->getNumElements (  );
  double *region = regionSparse->denseVector (  );
  //const int * permuteIn = factInfo_.mpermu+1;
  // Stuff is put one up so won't get illegal read
  assert (!region[numberRows_]);
  assert (!regionSparse2->packedMode());
#if 0
  int first=numberRows_;
  for (int j=0;j<numberNonZero;j++) {
    int jRow = regionIndex2[j];
    int iRow = permuteIn[jRow];
    region[iRow]=region2[jRow];
    first=CoinMin(first,iRow);
    region2[jRow]=0.0;
  }
#endif
  numberNonZero=c_ekkftrn(&factInfo_,
			region2-1,region,regionIndex2,numberNonZero);
  regionSparse2->setNumElements(numberNonZero);
  return 0;
}
/* Updates one column (FTRAN) from regionSparse2
   Tries to do FT update
   number returned is negative if no room
   regionSparse starts as zero and is zero at end.
   Note - if regionSparse2 packed on input - will be packed on output
*/
int 
CoinOslFactorization::updateColumnFT ( CoinIndexedVector * regionSparse,
				      CoinIndexedVector * regionSparse2,
				       bool /*noPermute*/)
{
  assert (numberRows_==numberColumns_);
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex2 = regionSparse2->getIndices (  );
  int numberNonZero = regionSparse2->getNumElements (  );
  assert (regionSparse2->packedMode());
  // packed mode
  //int numberNonInOriginal=numberNonZero;
  //double *dpermu = factInfo_.kadrpm;
  // Use region instead of dpermu
  double * save =factInfo_.kadrpm;
  factInfo_.kadrpm=regionSparse->denseVector()-1;
  int nuspike=c_ekkftrn_ft(&factInfo_, region2,regionIndex2,
			 &numberNonZero);
  factInfo_.kadrpm=save;
  regionSparse2->setNumElements(numberNonZero);
  //regionSparse2->print();
  factInfo_.nuspike=nuspike;
  return nuspike;
}


int 
CoinOslFactorization::updateTwoColumnsFT(CoinIndexedVector * regionSparse1,
					  CoinIndexedVector * regionSparse2,
					  CoinIndexedVector * regionSparse3,
					 bool /*noPermute*/)
{
#if 1
  // probably best to merge on a LU part by part
  // but can try full merge
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex2 = regionSparse2->getIndices (  );
  int numberNonZero2 = regionSparse2->getNumElements (  );
  assert (regionSparse2->packedMode());

  assert (numberRows_==numberColumns_);
  double *region3 = regionSparse3->denseVector (  );
  int *regionIndex3 = regionSparse3->getIndices (  );
  int numberNonZero3 = regionSparse3->getNumElements (  );
  double *region = regionSparse1->denseVector (  );
  // Stuff is put one up so won't get illegal read
  assert (!region[numberRows_]);
  assert (!regionSparse3->packedMode());
  // packed mode
  //double *dpermu = factInfo_.kadrpm;
#if 0
  factInfo_.nuspike=c_ekkftrn_ft(&factInfo_, region2,regionIndex2,
			       &numberNonZero2);
  numberNonZero3=c_ekkftrn(&factInfo_,
			region3-1,region,regionIndex3,numberNonZero3);
#else
  c_ekkftrn2(&factInfo_,region3-1,region,regionIndex3,&numberNonZero3,
  region2,regionIndex2,&numberNonZero2);
#endif
  regionSparse2->setNumElements(numberNonZero2);
  regionSparse3->setNumElements(numberNonZero3);
  return factInfo_.nuspike;
#else
  // probably best to merge on a LU part by part
  // but can try full merge
  int returnCode= updateColumnFT(regionSparse1,
				 regionSparse2);
  updateColumn(regionSparse1,
	       regionSparse3,
	       noPermute);
  return returnCode;
#endif
}

/* Updates one column (BTRAN) from regionSparse2
   regionSparse starts as zero and is zero at end 
   Note - if regionSparse2 packed on input - will be packed on output
*/
int  
CoinOslFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
						CoinIndexedVector * regionSparse2) const
{
  assert (numberRows_==numberColumns_);
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex2 = regionSparse2->getIndices (  );
  int numberNonZero = regionSparse2->getNumElements (  );
  //double *region = regionSparse->denseVector (  );
  /*int *regionIndex = regionSparse->getIndices (  );*/
  const int * permuteIn = factInfo_.mpermu+1;
  factInfo_.packedMode = regionSparse2->packedMode() ? 1 : 0;
  // Use region instead of dpermu
  double * save =factInfo_.kadrpm;
  factInfo_.kadrpm=regionSparse->denseVector()-1;
  // use internal one for now (address is one off)
  double * region = factInfo_.kadrpm;
  if (numberNonZero<2) {
    if (numberNonZero) {
      int ipivrw=regionIndex2[0];
      if (factInfo_.packedMode) {
	double value=region2[0];
	region2[0]=0.0;
	region2[ipivrw]=value;
      }
      numberNonZero=c_ekkbtrn_ipivrw(&factInfo_, region2-1,
				   regionIndex2-1,ipivrw+1,
				   reinterpret_cast<int *>(factInfo_.kp1adr));
    }
  } else {
#ifndef NDEBUG    
    {
      int *mcstrt	= factInfo_.xcsadr;
      int * hpivco_new=factInfo_.kcpadr+1;
      int nrow=factInfo_.nrow;
      int i;
      int ipiv = hpivco_new[0];
      int last = mcstrt[ipiv];
      for (i=0;i<nrow-1;i++) {
	ipiv=hpivco_new[ipiv];
	assert (mcstrt[ipiv]>last);
	last=mcstrt[ipiv];
      }
    }
#endif
    int iSmallest = COIN_INT_MAX;
    int iPiv=0;
    const int *mcstrt	= factInfo_.xcsadr;
    // permute and save where nonzeros are
    if (!factInfo_.packedMode) {
      if ((numberRows_<200||(numberNonZero<<4)>numberRows_)) {
	for (int j=0;j<numberNonZero;j++) {
	  int jRow = regionIndex2[j];
	  int iRow = permuteIn[jRow];
	  regionIndex2[j]=iRow; 
	  region[iRow]=region2[jRow];
	  region2[jRow]=0.0;
	}
      } else {
	for (int j=0;j<numberNonZero;j++) {
	  int jRow = regionIndex2[j];
	  int iRow = permuteIn[jRow];
	  regionIndex2[j]=iRow; 
	  region[iRow]=region2[jRow];
	  if (mcstrt[iRow]<iSmallest) {
	    iPiv=iRow;
	    iSmallest=mcstrt[iRow];
	  }
	  region2[jRow]=0.0;
	}
      }
    } else {
      for (int j=0;j<numberNonZero;j++) {
	int jRow = regionIndex2[j];
	int iRow = permuteIn[jRow];
	regionIndex2[j]=iRow;
	region[iRow]=region2[j];
	region2[j]=0.0;
      }
    }
    assert (iPiv>=0);
    numberNonZero=c_ekkbtrn(&factInfo_, region2-1,regionIndex2-1,iPiv);
  }
  factInfo_.kadrpm=save;
  factInfo_.packedMode=0;
  regionSparse2->setNumElements(numberNonZero);
  return 0;
}
// Number of entries in each row
int * 
CoinOslFactorization::numberInRow() const
{ return reinterpret_cast<int *> (factInfo_.xrnadr+1);}
// Number of entries in each column
int * 
CoinOslFactorization::numberInColumn() const
{ return reinterpret_cast<int *> (factInfo_.xcnadr+1);}
// Returns array to put basis starts in
CoinBigIndex * 
CoinOslFactorization::starts() const
{ return reinterpret_cast<CoinBigIndex *> (factInfo_.xcsadr+1);}
// Returns array to put basis elements in
CoinFactorizationDouble * 
CoinOslFactorization::elements() const
{ return factInfo_.xeeadr+1;}
// Returns pivot row 
int * 
CoinOslFactorization::pivotRow() const
{ return factInfo_.krpadr+1;}
// Returns work area
CoinFactorizationDouble * 
CoinOslFactorization::workArea() const
{ return factInfo_.kw1adr;}
// Returns int work area
int * 
CoinOslFactorization::intWorkArea() const
{ return reinterpret_cast<int *> (factInfo_.kw1adr);}
// Returns permute back
int * 
CoinOslFactorization::permuteBack() const
{ return factInfo_.kcpadr+1;}
// Returns array to put basis indices in
int * 
CoinOslFactorization::indices() const
{ return factInfo_.xeradr+1;}
// Returns true if wants tableauColumn in replaceColumn
bool
CoinOslFactorization::wantsTableauColumn() const
{ return false;}
/* Useful information for factorization
   0 - iteration number
   whereFrom is 0 for factorize and 1 for replaceColumn
*/
#ifdef CLP_REUSE_ETAS
void 
CoinOslFactorization::setUsefulInformation(const int * info,int whereFrom)
{ 
  factInfo_.iterno=info[0]; 
  if (whereFrom) {
    factInfo_.reintro=-1;
    if( factInfo_.first_dense>=factInfo_.last_dense) {
      int * putSeq = factInfo_.xrsadr+2*factInfo_.nrowmx+2;
      int * position = putSeq+factInfo_.maxinv;
      //int * putStart = position+factInfo_.maxinv;
      int iSequence=info[1];
      if (whereFrom==1) {
	putSeq[factInfo_.npivots]=iSequence;
      } else {
	int i;
	for (i=factInfo_.npivots-1;i>=0;i--) {
	  if (putSeq[i]==iSequence)
	    break;
	}
	if (i>=0) {
	  factInfo_.reintro=position[i];
	} else {
	  factInfo_.reintro=-1;
	}
	factInfo_.nnentu=factInfo_.save_nnentu;
      }
    }
  }
}
#else
void
CoinOslFactorization::setUsefulInformation(const int * info,int /*whereFrom*/)
{ factInfo_.iterno=info[0]; }
#endif

// Get rid of all memory
void 
CoinOslFactorization::clearArrays()
{
  factInfo_.nR_etas=0;
  factInfo_.nnentu=0;
  factInfo_.nnentl=0;
  maximumRows_=0;
  maximumSpace_=0;
  factInfo_.last_eta_size=0;
  gutsOfDestructor(false);
}
void 
CoinOslFactorization::maximumPivots (  int value )
{
  maximumPivots_ = value;
}
#define CLP_FILL 15
/*#undef NDEBUG*/
//#define CLP_DEBUG_MALLOC 1000000
#if CLP_DEBUG_MALLOC
static int malloc_number=0;
static int malloc_check=-1;
static int malloc_counts_on=0;
struct malloc_struct {
  void * previous;
  void * next;
  int size;
  int when;
  int type;
};
static double malloc_times=0.0;
static double malloc_total=0.0;
static double malloc_current=0.0;
static double malloc_max=0.0;
static int malloc_amount[]={0,32,128,256,1024,4096,16384,65536,262144,
			    2000000000};
static int malloc_n=10;
double malloc_counts[10]={0,0,0,0,0,0,0,0,0,0};
typedef struct malloc_struct malloc_struct;
static malloc_struct startM = {0,0,0,0};
static malloc_struct endM = {0,0,0,0};
static int extra=4;
void clp_memory(int type)
{
  if (type==0) {
    /* switch on */
    malloc_counts_on=1;
    startM.next=&endM;
    endM.previous=&startM;
  } else {
    /* summary */
    double average = malloc_total/malloc_times;
    int i;
    malloc_struct * previous = (malloc_struct *) endM.previous;
    printf("count %g bytes %g - average %g\n",malloc_times,malloc_total,average);
    printf("current bytes %g - maximum %g\n",malloc_current,malloc_max);
    
    for ( i=0;i<malloc_n;i++) 
      printf("%g ",malloc_counts[i]);
    printf("\n");
    malloc_counts_on=0;
    if (previous->previous!=&startM) {
      int n=0;
      printf("Allocated blocks\n");
      while (previous->previous!=&startM) {
	printf("(%d at %d) ",previous->size,previous->when);
	n++;
	if ((n%5)==0)
	  printf("\n");
	previous = (malloc_struct *) previous->previous;
      }
      printf("\n - total %d\n",n);
    }
  }
  malloc_number=0;
  malloc_times=0.0;
  malloc_total=0.0;
  malloc_current=0.0;
  malloc_max=0.0;
  memset(malloc_counts,0,sizeof(malloc_counts));
}
static void clp_adjust(void * temp,int size,int type)
{
  malloc_struct * itemp = (malloc_struct *) temp;
  malloc_struct * first = (malloc_struct *) startM.next;
  int i;
  startM.next=temp;
  first->previous=temp;
  itemp->previous=&startM;
  itemp->next=first;
  malloc_number++;
  if (malloc_number==malloc_check) {
    printf("Allocation of %d bytes at %d (type %d) not freed\n",
	   size,malloc_number,type);
  }
  itemp->when=malloc_number;
  itemp->size=size;
  itemp->type=type;
  malloc_times ++;
  malloc_total += size;
  malloc_current += size;
  malloc_max=CoinMax(malloc_max,malloc_current);
  for (i=0;i<malloc_n;i++) {
    if ((int) size<=malloc_amount[i]) {
      malloc_counts[i]++;
      break;
    }
  }
}
#endif
/* covers */
double * clp_double(int number_entries)
{
#if CLP_DEBUG_MALLOC==0
  return reinterpret_cast<double *>( malloc(number_entries*sizeof(double)));
#else
  double * temp = reinterpret_cast<double *>( malloc((number_entries+extra)*sizeof(double)));
  clp_adjust(temp,number_entries*sizeof(double),1);
#if CLP_DEBUG_MALLOC>1
  if (number_entries*sizeof(double)>=CLP_DEBUG_MALLOC)
    printf("WWW %x malloced by double %d - size %d\n",
	   temp+extra,malloc_number,number_entries);
#endif
  return temp+extra;
#endif
}
int * clp_int(int number_entries)
{
#if CLP_DEBUG_MALLOC==0
  return reinterpret_cast<int *>( malloc(number_entries*sizeof(int)));
#else
  double * temp = reinterpret_cast<double *>( malloc(((number_entries+1)/2+extra)*sizeof(double)));
  clp_adjust(temp,number_entries*sizeof(int),2);
#if CLP_DEBUG_MALLOC>1
  if (number_entries*sizeof(int)>=CLP_DEBUG_MALLOC)
    printf("WWW %x malloced by int %d - size %d\n",
	   temp+extra,malloc_number,number_entries);
#endif
  return reinterpret_cast<int *>( (temp+extra));
#endif
}
void * clp_malloc(int number_entries)
{
#if CLP_DEBUG_MALLOC==0
  return malloc(number_entries);
#else
  double * temp = reinterpret_cast<double *>( malloc(number_entries+extra*sizeof(double)));
  clp_adjust(temp,number_entries,0);
#if CLP_DEBUG_MALLOC>1
  if (number_entries>=CLP_DEBUG_MALLOC)
    printf("WWW %x malloced by void %d - size %d\n",
	   temp+extra,malloc_number,number_entries);
#endif
  return (void *) (temp+extra);
#endif
}
void clp_free(void * oldArray)
{
#if CLP_DEBUG_MALLOC==0
  free(oldArray);
#else
  if (oldArray) {
    double * temp = (reinterpret_cast<double *>( oldArray)-extra);
    malloc_struct * itemp = (malloc_struct *) temp;
    malloc_struct * next = (malloc_struct *) itemp->next;
    malloc_struct * previous = (malloc_struct *) itemp->previous;
    previous->next=next;
    next->previous=previous;
    malloc_current -= itemp->size;
#if CLP_DEBUG_MALLOC>1
    if (itemp->size>=CLP_DEBUG_MALLOC)
    printf("WWW %x freed by free %d - old length %d - type %d\n",
	   oldArray,itemp->when,itemp->size,itemp->type);
#endif
    free(temp);
  }
#endif
}
/*#define FIX_ADD 4*nrowmx+5
  #define FIX_ADD2 4*nrowmx+5*/
#define FIX_ADD 1*nrowmx+5
#define FIX_ADD2 1*nrowmx+5
#define ALIGNMENT 32
static void * clp_align (void * memory)
{
  if (sizeof(int)==sizeof(void *)&&ALIGNMENT) {
    CoinInt64 k = reinterpret_cast<CoinInt64> (memory);
    if ((k&(ALIGNMENT-1))!=0) {
      k &= ~(ALIGNMENT-1);
      k += ALIGNMENT;
      memory = reinterpret_cast<void *> (k);
    }
    return memory;
  } else {
    return memory;
  }
}
void clp_setup_pointers(EKKfactinfo * fact)
{
  /* do extra stuff */
  int nrow=fact->nrow;
  int nrowmx=fact->nrowmx;
  int maxinv=fact->maxinv;
  fact->lstart	= nrow + maxinv + 5;
  /* this is the number of L transforms */
  fact->xnetalval = fact->xnetal - fact->lstart;
  fact->mpermu	= (reinterpret_cast<int*> (fact->kadrpm+nrow))+1;
  fact->bitArray = fact->krpadr + ( nrowmx+2);
  fact->back	= fact->kcpadr+2*nrow + maxinv + 4;
  fact->hpivcoR = fact->kcpadr+nrow+3;
  fact->nonzero = (reinterpret_cast<char *>( &fact->mpermu[nrow+1]))-1;
}
#ifndef NDEBUG
int ets_count=0;
int ets_check=-1;
//static int adjust_count=0;
//static int adjust_check=-1;
#endif
static void clp_adjust_pointers(EKKfactinfo * fact, int adjust)
{
#if 0 //ndef NDEBUG
  adjust_count++;
  if (adjust_check>=0&&adjust_count>=adjust_check) {
    printf("trouble\n");
  }
#endif
  if (fact->trueStart) {
    fact->kadrpm += adjust;
    fact->krpadr += adjust;
    fact->kcpadr += adjust;
    fact->xrsadr += adjust;
    fact->xcsadr += adjust;
    fact->xrnadr += adjust;
    fact->xcnadr += adjust;
  }
  if (fact->xeradr) {
    fact->xeradr += adjust;
    fact->xecadr += adjust;
    fact->xeeadr += adjust;
  }
}
/* deals with memory for complicated array 
   0 just do addresses 
   1 just get memory */
static double *
clp_alloc_memory(EKKfactinfo * fact,int type, int * length)
{
  int nDouble=0;
  int nInt=0;
  int nrowmxp;
  int ntot1;
  int ntot2;
  int ntot3;
  int nrowmx;
  int * tempI;
  double * tempD;
  nrowmx=fact->nrowmx;
  nrowmxp = nrowmx + 2;
  ntot1 = nrowmxp;
  ntot2 = 3*nrowmx+5; /* space for three lists */
  ntot3 = 2*nrowmx;
  if ((ntot1<<1)<ntot2) {
    ntot1=ntot2>>1;
  }
  ntot3=CoinMax(ntot3,ntot1);
  /*   Row work regions */
  /* must be contiguous so allocate as one chunk */
  /* may only need 2.5 */
  /* now doing all at once - far too much - reduce later */
  tempD=fact->kw1adr;
  tempD+=nrowmxp;
  tempD = reinterpret_cast<double *>( clp_align(tempD));
  fact->kw2adr=tempD;
  tempD+=nrowmxp;
  tempD = reinterpret_cast<double *>( clp_align(tempD));
  fact->kw3adr=tempD-1;
  tempD+=nrowmxp;
  tempD = reinterpret_cast<double *>( clp_align(tempD));
  fact->kp1adr=reinterpret_cast<EKKHlink *>(tempD);
  tempD+=nrowmxp;
  tempD = reinterpret_cast<double *>( clp_align(tempD));
  fact->kp2adr=reinterpret_cast<EKKHlink *>(tempD);
  //tempD+=ntot3;
  tempD+=nrowmxp;
  tempD = reinterpret_cast<double *>( clp_align(tempD));
  /*printf("zz %x %x\n",tempD,fact->kadrpm);*/
  fact->kadrpm = tempD;
  /* seems a lot */
  tempD += ((6*nrowmx +8)*(sizeof(int))/sizeof(double));
  /* integer arrays */
  tempI = reinterpret_cast<int *>( tempD);
  tempI = reinterpret_cast<int *>( clp_align(tempI));
  fact->xrsadr = tempI;
#ifdef CLP_REUSE_ETAS
  tempI +=( 3*(nrowmx+fact->maxinv+1));
#else
  tempI +=( (nrowmx<<1)+fact->maxinv+1);
#endif
  tempI = reinterpret_cast<int *>( clp_align(tempI));
  fact->xcsadr = tempI;
#if 1 //def CLP_REUSE_ETAS
  tempI += ( 2*nrowmx+8+2*fact->maxinv);
#else
  tempI += ( 2*nrowmx+8+fact->maxinv);
#endif
  tempI += FIX_ADD+FIX_ADD2;
  tempI = reinterpret_cast<int *>( clp_align(tempI));
  fact->xrnadr = tempI;
  tempI += nrowmx;
  tempI = reinterpret_cast<int *>( clp_align(tempI));
  fact->xcnadr = tempI;
  tempI += nrowmx;
  tempI = reinterpret_cast<int *>( clp_align(tempI));
  fact->krpadr = tempI;
  tempI += ( nrowmx+1) +((nrowmx+33)>>5);
  /*printf("zzz %x %x\n",tempI,fact->kcpadr);*/
  tempI = reinterpret_cast<int *>( clp_align(tempI));
  fact->kcpadr = tempI;
  tempI += 3*nrowmx+8+fact->maxinv;
  fact->R_etas_start = fact->xcsadr+nrowmx+fact->maxinv+4;
  fact->R_etas_start += FIX_ADD;
  nInt = static_cast<int>(tempI-(reinterpret_cast<int *>( fact->trueStart)));
  nDouble = static_cast<int>(sizeof(int)*(nInt+1)/sizeof(double));
  *length = nDouble;
  /*printf("nDouble %d - type %d\n",nDouble,type);*/
  nDouble += static_cast<int>((2*ALIGNMENT)/sizeof(double));
  if (type) {
    /*printf("%d allocated\n",nDouble);*/
    tempD = reinterpret_cast<double *>( clp_double(nDouble));
#ifndef NDEBUG
    memset(tempD,CLP_FILL,nDouble*sizeof(double));
#endif
  }
  return tempD;
}
static void c_ekksmem(EKKfactinfo *fact,int nrow,int maximumPivots)
{
  /* space for invert */
  int nnetas=fact->eta_size;
  fact->nrow=nrow;
  if (!(nnetas>fact->last_eta_size||(!fact->xe2adr&&fact->if_sparse_update)||
	nrow>fact->nrowmx||maximumPivots>fact->maxinv))
    return;
  clp_adjust_pointers(fact, +1);
  if (nrow>fact->nrowmx||maximumPivots>fact->maxinv) {
    int length;
    fact->nrowmx=CoinMax(nrow,fact->nrowmx);
    fact->maxinv=CoinMax(maximumPivots,fact->maxinv);
    clp_free(fact->trueStart);
    fact->trueStart=0;
    fact->kw1adr=0;
    fact->trueStart=clp_alloc_memory(fact,1,&length);
    fact->kw1adr=reinterpret_cast<double *>( clp_align(fact->trueStart));
    clp_alloc_memory(fact,0,&length);
  }
  /*if (!fact->iterno) fact->eta_size+=1000000;*//* TEMP*/
  if (nnetas>fact->last_eta_size||(!fact->xe2adr&&fact->if_sparse_update)) {
    fact->last_eta_size = nnetas;
    clp_free(reinterpret_cast<char *>(fact->xe2adr));
    /* if malloc fails - we have lost memory - start again */
    if (!fact->ndenuc &&fact->if_sparse_update) {
      /* allow second copy of elements */
      fact->xe2adr = clp_double(nnetas);
#ifndef NDEBUG
      memset(fact->xe2adr,CLP_FILL,nnetas*sizeof(double));
#endif
      if (!fact->xe2adr) {
	fact->maxNNetas=fact->last_eta_size; /* dont allow any increase */
	nnetas=fact->last_eta_size;
	fact->eta_size=nnetas;
#ifdef PRINT_DEBUG
	if (fact->if_sparse_update) {
	  printf("*** Sparse update off due to memory\n");
	}
#endif
	fact->if_sparse_update=0;
	fact->switch_off_sparse_update=1;
      }
    } else {
      fact->xe2adr = 0;
      fact->if_sparse_update=0;
    }
    clp_free(fact->xeradr);
    fact->xeradr= clp_int( nnetas);
#ifndef NDEBUG
      memset(fact->xeradr,CLP_FILL,nnetas*sizeof(int));
#endif
    if (!fact->xeradr) {
      nnetas=0;
    }
    if (nnetas) {
      clp_free(fact->xecadr);
      fact->xecadr= clp_int( nnetas);
#ifndef NDEBUG
      memset(fact->xecadr,CLP_FILL,nnetas*sizeof(int));
#endif
      if (!fact->xecadr) {
	nnetas=0;
      }
    }
    if (nnetas) {
      clp_free(fact->xeeadr);
      fact->xeeadr= clp_double(nnetas);
#ifndef NDEBUG
      memset(fact->xeeadr,CLP_FILL,nnetas*sizeof(double));
#endif
      if (!fact->xeeadr) {
	nnetas=0;
      }
    }
  }
  if (!nnetas) {
    char msg[100];
    sprintf(msg,"Unable to allocate factorization memory for %d elements",
	   nnetas);
    throw(msg);
  }
  /*c_ekklplp->nnetas=nnetas;*/
  fact->nnetas=nnetas;
  clp_adjust_pointers(fact, -1);
}
static void c_ekksmem_copy(EKKfactinfo *fact,const EKKfactinfo * rhsFact)
{
  /* space for invert */
  int nrowmx=rhsFact->nrowmx,nnetas=rhsFact->nnetas;
  int canReuseEtas= (fact->eta_size==rhsFact->eta_size) ? 1 : 0;
  int canReuseArrays = (fact->nrowmx==rhsFact->nrowmx) ? 1 : 0;
  clp_adjust_pointers(fact, +1);
  clp_adjust_pointers(const_cast<EKKfactinfo *>(rhsFact), +1);
  /*memset(fact,0,sizeof(EKKfactinfo));*/
  /* copy scalars */
  memcpy(&fact->drtpiv,&rhsFact->drtpiv,5*sizeof(double));
  memcpy(&fact->nrow,&rhsFact->nrow,((&fact->maxNNetas-&fact->nrow)+1)*
	 sizeof(int));
  if (nrowmx) {
    int length;
    int kCopyEnd,nCopyEnd,nCopyStart;
    if (!canReuseEtas) {
      clp_free(fact->xeradr);
      clp_free(fact->xecadr);
      clp_free(fact->xeeadr);
      clp_free(fact->xe2adr);
      fact->xeradr = 0;
      fact->xecadr = 0;
      fact->xeeadr = 0;
      fact->xe2adr = 0;
    }
    if (!canReuseArrays) {
      clp_free(fact->trueStart);
      fact->trueStart=0;
      fact->kw1adr=0;
      fact->trueStart=clp_alloc_memory(fact,1,&length);
      fact->kw1adr=reinterpret_cast<double *>( clp_align(fact->trueStart));
    }
    clp_alloc_memory(fact,0,&length);
    nnetas=fact->eta_size;
    assert (nnetas);
    {
      int n2 = rhsFact->nR_etas;
      int n3 = n2 ? rhsFact->R_etas_start[1+n2]: 0;
      int * startR = rhsFact->R_etas_index+n3; 
      nCopyEnd=static_cast<int>((rhsFact->xeradr+nnetas)-startR);
      nCopyStart=rhsFact->nnentu;
      nCopyEnd = CoinMin(nCopyEnd+20,nnetas);
      kCopyEnd = nnetas-nCopyEnd;
      nCopyStart = CoinMin(nCopyStart+20,nnetas);
      if (!n2&&!rhsFact->nnentu&&!rhsFact->nnentl) {
	nCopyStart=nCopyEnd=0;
      }
    }
    /* copy */
    if(nCopyStart||nCopyEnd||true) {
#if 1
      memcpy(fact->kw1adr,rhsFact->kw1adr,length*sizeof(double));
#else
      c_ekkscpy((length*sizeof(double))/sizeof(int),
	      reinterpret_cast<int *>( rhsFact->kw1adr,reinterpret_cast<int *>( fact->kw1adr));
#endif
    }
    /* if malloc fails - we have lost memory - start again */
    if (!fact->ndenuc &&fact->if_sparse_update) {
      /* allow second copy of elements */
      if (!canReuseEtas) 
	fact->xe2adr = clp_double(nnetas);
      if (!fact->xe2adr) {
	fact->maxNNetas=nnetas; /* dont allow any increase */
#ifdef PRINT_DEBUG
	if (fact->if_sparse_update) {
	  printf("*** Sparse update off due to memory\n");
	}
#endif
	fact->if_sparse_update=0;
      } else {
#ifndef NDEBUG
	memset(fact->xe2adr,CLP_FILL,nnetas*sizeof(double));
#endif
      }
    } else {
      clp_free(fact->xe2adr);
      fact->xe2adr = 0;
      fact->if_sparse_update=0;
    }
    
    if (!canReuseEtas) 
      fact->xeradr= clp_int(nnetas);
    if (!fact->xeradr) {
      nnetas=0;
    } else {
#ifndef NDEBUG
      memset(fact->xeradr,CLP_FILL,nnetas*sizeof(int));
#endif
      
      /* copy */
      if(nCopyStart||nCopyEnd) {
#if 0
	memcpy(fact->xeradr,rhsFact->xeradr,nCopyStart*sizeof(int));
	memcpy(fact->xeradr+kCopyEnd,rhsFact->xeradr+kCopyEnd,nCopyEnd*sizeof(int));
#else
	c_ekkscpy(nCopyStart,rhsFact->xeradr,fact->xeradr);
	c_ekkscpy(nCopyEnd,rhsFact->xeradr+kCopyEnd,fact->xeradr+kCopyEnd);
#endif
      }
    }
    if (nnetas) {
      if (!canReuseEtas) 
	fact->xecadr= clp_int(nnetas);
      if (!fact->xecadr) {
	nnetas=0;
      } else {
#ifndef NDEBUG
	memset(fact->xecadr,CLP_FILL,nnetas*sizeof(int));
#endif
	/* copy */
	if (fact->rows_ok&&(nCopyStart||nCopyEnd)) {
	  int i;
	  int * hcoliR=rhsFact->xecadr-1;
	  int * hcoli=fact->xecadr-1;
	  int * mrstrt=fact->xrsadr;
	  int * hinrow=fact->xrnadr;
#if 0
	  memcpy(fact->xecadr+kCopyEnd,rhsFact->xecadr+kCopyEnd,
		 nCopyEnd*sizeof(int));
#else
	  c_ekkscpy(nCopyEnd,rhsFact->xecadr+kCopyEnd,fact->xecadr+kCopyEnd);
#endif
	  if (!fact->xe2adr) {
	    for (i=0;i<fact->nrow;i++) {
	      int istart = mrstrt[i];
	      assert (istart>0&&istart<=nnetas);
	      assert (hinrow[i]>=0&&hinrow[i]<=fact->nrow);
	      memcpy(hcoli+istart,hcoliR+istart,hinrow[i]*sizeof(int));
	    }
	  } else {
	    double * de2valR=rhsFact->xe2adr-1;
	    double * de2val=fact->xe2adr-1;
#if 0
	    memcpy(fact->xe2adr+kCopyEnd,rhsFact->xe2adr+kCopyEnd,
		   nCopyEnd*sizeof(double));
#else
	    c_ekkdcpy(nCopyEnd,rhsFact->xe2adr+kCopyEnd
		     ,fact->xe2adr+kCopyEnd);
#endif
	    for (i=0;i<fact->nrow;i++) {
	      int istart = mrstrt[i];
	      assert (istart>0&&istart<=nnetas);
	      assert (hinrow[i]>=0&&hinrow[i]<=fact->nrow);
	      memcpy(hcoli+istart,hcoliR+istart,hinrow[i]*sizeof(int));
	      memcpy(de2val+istart,de2valR+istart,hinrow[i]*sizeof(double));
#ifndef NDEBUG
	      {
		int j;
		for (j=istart;j<istart+hinrow[i];j++)
		  assert (fabs(de2val[j])<1.0e50);
	      }
#endif
	    }
	  }
	}
      }
    }
    if (nnetas) {
      if (!canReuseEtas) 
	fact->xeeadr= clp_double(nnetas);
      if (!fact->xeeadr) {
	nnetas=0;
      } else {
#ifndef NDEBUG
	memset(fact->xeeadr,CLP_FILL,nnetas*sizeof(double));
#endif
	/* copy */
	if(nCopyStart||nCopyEnd) {
#if 0
	  memcpy(fact->xeeadr,rhsFact->xeeadr,nCopyStart*sizeof(double));
	  memcpy(fact->xeeadr+kCopyEnd,rhsFact->xeeadr+kCopyEnd,nCopyEnd*sizeof(double));
#else
	  c_ekkdcpy(nCopyStart,
		  rhsFact->xeeadr,fact->xeeadr);
	  c_ekkdcpy(nCopyEnd,
		   rhsFact->xeeadr+kCopyEnd,
		   fact->xeeadr+kCopyEnd);
#endif
	}
	/*fact->R_etas_index = &XERADR1()[kdnspt - 1];
	  fact->R_etas_element = &XEEADR1()[kdnspt - 1];*/
	fact->R_etas_start = fact->xcsadr+
	  (rhsFact->R_etas_start-rhsFact->xcsadr);
	fact->R_etas_index = fact->xeradr+
	  (rhsFact->R_etas_index-rhsFact->xeradr);
	fact->R_etas_element = fact->xeeadr+
	  (rhsFact->R_etas_element-rhsFact->xeeadr);
      }
    }
  }
  assert (nnetas||!nrowmx);
  fact->nnetas=nnetas;
  clp_adjust_pointers(fact, -1);
  clp_setup_pointers(fact);
  clp_adjust_pointers(const_cast<EKKfactinfo *>(rhsFact), -1);
}
static void c_ekksmem_delete(EKKfactinfo *fact)
{
  clp_adjust_pointers(fact, +1);
  clp_free(fact->trueStart);
  clp_free(fact->xe2adr);
  clp_free(fact->xecadr);
  clp_free(fact->xeradr);
  clp_free(fact->xeeadr);
  fact->eta_size=0;
  fact->xrsadr = 0;
  fact->xcsadr = 0;
  fact->xrnadr = 0;
  fact->xcnadr = 0;
  fact->krpadr = 0;
  fact->kcpadr = 0;
  fact->xeradr = 0;
  fact->xecadr = 0;
  fact->xeeadr = 0;
  fact->xe2adr = 0;
  fact->trueStart = 0;
  fact->kw2adr = 0;
  fact->kw3adr = 0;
  fact->kp1adr = 0;
  fact->kp2adr = 0;
  fact->kadrpm = 0;
  fact->kw1adr = 0;
}
int c_ekk_IsSet(const int * array,int bit);
void c_ekk_Set(int * array,int bit);
void c_ekk_Unset(int * array,int bit);
int c_ekk_IsSet(const int * array,int bit)
{
  int iWord = bit>>5;
  int iBit = bit&31;
  int word = array[iWord];
  return (word&(1<<iBit))!=0;
}
void c_ekk_Set(int * array,int bit)
{
  int iWord = bit>>5;
  int iBit = bit&31;
  array[iWord] |= (1<<iBit);
}
void c_ekk_Unset(int * array,int bit)
{
  int iWord = bit>>5;
  int iBit = bit&31;
  array[iWord] &= ~(1<<iBit);
}
int CoinOslFactorization::factorize (
				 const CoinPackedMatrix & matrix,
				 int rowIsBasic[],
				 int columnIsBasic[],
				 double areaFactor )
{
  setSolveMode(10);
  if (areaFactor)
    factInfo_.areaFactor = areaFactor;
  const int * row = matrix.getIndices();
  const CoinBigIndex * columnStart = matrix.getVectorStarts();
  const int * columnLength = matrix.getVectorLengths(); 
  const double * element = matrix.getElements();
  int numberRows=matrix.getNumRows();
  int numberColumns=matrix.getNumCols();
  int numberBasic = 0;
  CoinBigIndex numberElements=0;
  int numberRowBasic=0;
  
  // compute how much in basis
  
  int i;
  // Move pivot variables across if they look good
  int * pivotTemp = new int [numberRows];
  
  for (i=0;i<numberRows;i++) {
    if (rowIsBasic[i]>=0) 
      pivotTemp[numberRowBasic++]=i;
  }
  
  numberBasic = numberRowBasic;
  
  for (i=0;i<numberColumns;i++) {
    if (columnIsBasic[i]>=0) {
      pivotTemp[numberBasic++]=i;
      numberElements += columnLength[i];
    }
  }
  if ( numberBasic > numberRows ) {
    return -2; // say too many in basis
  }
  numberElements = 3 * numberRows + 3 * numberElements + 20000;
  setUsefulInformation(&numberRows,0);
  getAreas ( numberRows, numberRows, numberElements,
	     2 * numberElements );
  //fill
  numberBasic=0;
  numberElements=0;
  // Fill in counts so we can skip part of preProcess
  double * elementU=elements();
  int * indexRowU=indices();
  int * startColumnU=starts();
  int * numberInRow=this->numberInRow();
  int * numberInColumn=this->numberInColumn();
  CoinZeroN ( numberInRow, numberRows  );
  CoinZeroN ( numberInColumn, numberRows );
  for (i=0;i<numberRowBasic;i++) {
    int iRow = pivotTemp[i];
    // Change pivotTemp to correct sequence
    pivotTemp[i]=iRow+numberColumns;
    indexRowU[i]=iRow;
    startColumnU[i]=i;
    elementU[i]=-1.0;
    numberInRow[iRow]=1;
    numberInColumn[i]=1;
  }
  startColumnU[numberRowBasic]=numberRowBasic;
  numberElements=numberRowBasic;
  numberBasic=numberRowBasic;
  for (i=0;i<numberColumns;i++) {
    if (columnIsBasic[i]>=0) {
      CoinBigIndex j;
      for (j=columnStart[i];j<columnStart[i]+columnLength[i];j++) {
	int iRow=row[j];
	numberInRow[iRow]++;
	indexRowU[numberElements]=iRow;
	elementU[numberElements++]=element[j];
      }
      numberInColumn[numberBasic]=columnLength[i];
      numberBasic++;
      startColumnU[numberBasic]=numberElements;
    }
  }
  preProcess ( );
  factor (  );
  if (status() == 0) {
    int * pivotVariable = new int [numberRows];
    postProcess(pivotTemp,pivotVariable);
    for (i=0;i<numberRows;i++) {
      int iPivot=pivotVariable[i];
      if (iPivot<numberColumns) {
	assert (columnIsBasic[iPivot]>=0);
	columnIsBasic[iPivot]=i;
      } else {
	iPivot-=numberColumns;
	assert (rowIsBasic[iPivot]>=0);
	rowIsBasic[iPivot]=i;
      }
    }
    delete [] pivotVariable;
  }
  delete [] pivotTemp;
  return status_;
}
// Condition number - product of pivots after factorization
double 
CoinOslFactorization::conditionNumber() const
{
  double condition = 1.0;
  const double *dluval	= factInfo_.xeeadr+1-1; // stored before
  const int *mcstrt	= factInfo_.xcsadr+1;
  for (int i=0;i<numberRows_;i++) {
    const int kx = mcstrt[i];
    const double dpiv = dluval[kx];
    condition *= dpiv;
  }
  condition = CoinMax(fabs(condition),1.0e-50);
  return 1.0/condition;
}
