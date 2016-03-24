/* $Id: CoinSimpFactorization.cpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinUtilsConfig.h"

#include <cassert>
#include "CoinPragma.hpp"
#include "CoinSimpFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinFinite.hpp"
#include <stdio.h>



#define ARRAY 0



FactorPointers::FactorPointers( int numRows, int numColumns,
				int *UrowLengths_,
				int *UcolLengths_ ){

    rowMax = new double[numRows];
    double *current=rowMax;
    const double *end=current+numRows;
    for ( ; current!=end; ++current ) *current=-1.0;

    firstRowKnonzeros = new int[numRows+1];
    CoinFillN(firstRowKnonzeros, numRows+1, -1 );

    prevRow = new int[numRows];
    nextRow = new int[numRows];
    firstColKnonzeros = new int[numRows+1];
    memset(firstColKnonzeros, -1, (numRows+1)*sizeof(int) );

    prevColumn = new int[numColumns];
    nextColumn = new int[numColumns];
    newCols = new int[numRows];


    for ( int i=numRows-1; i>=0; --i ){
	int length=UrowLengths_[i];
	prevRow[i]=-1;
	nextRow[i]=firstRowKnonzeros[length];
	if ( nextRow[i]!=-1 ) prevRow[nextRow[i]]=i;
	firstRowKnonzeros[length]=i;
    }
    for ( int i=numColumns-1; i>=0; --i ){
	int length=UcolLengths_[i];
	prevColumn[i]=-1;
	nextColumn[i]=firstColKnonzeros[length];
	if ( nextColumn[i]!=-1 ) prevColumn[nextColumn[i]]=i;
	firstColKnonzeros[length]=i;
    }
}
FactorPointers::~ FactorPointers(){
    delete [] rowMax;
    delete [] firstRowKnonzeros;
    delete [] prevRow;
    delete [] nextRow;
    delete [] firstColKnonzeros;
    delete [] prevColumn;
    delete [] nextColumn;
    delete [] newCols;
}


//:class CoinSimpFactorization.  Deals with Factorization and Updates
//  CoinSimpFactorization.  Constructor
CoinSimpFactorization::CoinSimpFactorization (  )
  : CoinOtherFactorization()
{
  gutsOfInitialize();
}

/// Copy constructor
CoinSimpFactorization::CoinSimpFactorization ( const CoinSimpFactorization &other)
  : CoinOtherFactorization(other)
{
  gutsOfInitialize();
  gutsOfCopy(other);
}
// Clone
CoinOtherFactorization *
CoinSimpFactorization::clone() const
{
  return new CoinSimpFactorization(*this);
}
/// The real work of constructors etc
void CoinSimpFactorization::gutsOfDestructor()
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
  numberSlacks_=0;
  firstNumberSlacks_=0;
  //
  delete [] denseVector_;
  delete [] workArea2_;
  delete [] workArea3_;
  delete [] vecLabels_;
  delete [] indVector_;

  delete [] auxVector_;
  delete [] auxInd_;

  delete [] vecKeep_;
  delete [] indKeep_;

  delete [] LrowStarts_;
  delete [] LrowLengths_;
  delete [] Lrows_;
  delete [] LrowInd_;


  delete [] LcolStarts_;
  delete [] LcolLengths_;
  delete [] Lcolumns_;
  delete [] LcolInd_;

  delete [] UrowStarts_;
  delete [] UrowLengths_;
#ifdef COIN_SIMP_CAPACITY
  delete [] UrowCapacities_;
#endif
  delete [] Urows_;
  delete [] UrowInd_;

  delete [] prevRowInU_;
  delete [] nextRowInU_;

  delete [] UcolStarts_;
  delete [] UcolLengths_;
#ifdef COIN_SIMP_CAPACITY
  delete [] UcolCapacities_;
#endif
  delete [] Ucolumns_;
  delete [] UcolInd_;
  delete [] prevColInU_;
  delete [] nextColInU_;
  delete [] colSlack_;

  delete [] invOfPivots_;

  delete [] colOfU_;
  delete [] colPosition_;
  delete [] rowOfU_;
  delete [] rowPosition_;
  delete [] secRowOfU_;
  delete [] secRowPosition_;

  delete [] EtaPosition_;
  delete [] EtaStarts_;
  delete [] EtaLengths_;
  delete [] EtaInd_;
  delete [] Eta_;

  denseVector_=NULL;
  workArea2_=NULL;
  workArea3_=NULL;
  vecLabels_=NULL;
  indVector_=NULL;

  auxVector_=NULL;
  auxInd_=NULL;

  vecKeep_=NULL;
  indKeep_=NULL;

  LrowStarts_=NULL;
  LrowLengths_=NULL;
  Lrows_=NULL;
  LrowInd_=NULL;


  LcolStarts_=NULL;
  LcolLengths_=NULL;
  Lcolumns_=NULL;
  LcolInd_=NULL;

  UrowStarts_=NULL;
  UrowLengths_=NULL;
#ifdef COIN_SIMP_CAPACITY
  UrowCapacities_=NULL;
#endif
  Urows_=NULL;
  UrowInd_=NULL;

  prevRowInU_=NULL;
  nextRowInU_=NULL;

  UcolStarts_=NULL;
  UcolLengths_=NULL;
#ifdef COIN_SIMP_CAPACITY
  UcolCapacities_=NULL;
#endif
  Ucolumns_=NULL;
  UcolInd_=NULL;
  prevColInU_=NULL;
  nextColInU_=NULL;
  colSlack_=NULL;

  invOfPivots_=NULL;

  colOfU_=NULL;
  colPosition_=NULL;
  rowOfU_=NULL;
  rowPosition_=NULL;
  secRowOfU_=NULL;
  secRowPosition_=NULL;

  EtaPosition_=NULL;
  EtaStarts_=NULL;
  EtaLengths_=NULL;
  EtaInd_=NULL;
  Eta_=NULL;
}
void CoinSimpFactorization::gutsOfInitialize()
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
  numberSlacks_=0;
  firstNumberSlacks_=0;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;

  denseVector_=NULL;
  workArea2_=NULL;
  workArea3_=NULL;
  vecLabels_=NULL;
  indVector_=NULL;

  auxVector_=NULL;
  auxInd_=NULL;

  vecKeep_=NULL;
  indKeep_=NULL;

  LrowStarts_=NULL;
  LrowLengths_=NULL;
  Lrows_=NULL;
  LrowInd_=NULL;


  LcolStarts_=NULL;
  LcolLengths_=NULL;
  Lcolumns_=NULL;
  LcolInd_=NULL;

  UrowStarts_=NULL;
  UrowLengths_=NULL;
#ifdef COIN_SIMP_CAPACITY
  UrowCapacities_=NULL;
#endif
  Urows_=NULL;
  UrowInd_=NULL;

  prevRowInU_=NULL;
  nextRowInU_=NULL;

  UcolStarts_=NULL;
  UcolLengths_=NULL;
#ifdef COIN_SIMP_CAPACITY
  UcolCapacities_=NULL;
#endif
  Ucolumns_=NULL;
  UcolInd_=NULL;
  prevColInU_=NULL;
  nextColInU_=NULL;
  colSlack_=NULL;

  invOfPivots_=NULL;

  colOfU_=NULL;
  colPosition_=NULL;
  rowOfU_=NULL;
  rowPosition_=NULL;
  secRowOfU_=NULL;
  secRowPosition_=NULL;

  EtaPosition_=NULL;
  EtaStarts_=NULL;
  EtaLengths_=NULL;
  EtaInd_=NULL;
  Eta_=NULL;
}
void CoinSimpFactorization::initialSomeNumbers(){
    keepSize_=-1;
    LrowSize_=-1;
    // LrowCap_ in allocateSomeArrays
    LcolSize_=-1;
    // LcolCap_ in allocateSomeArrays
    // UrowMaxCap_ in allocateSomeArrays
    UrowEnd_=-1;
    firstRowInU_=-1;
    lastRowInU_=-1;
    firstColInU_=-1;
    lastColInU_=-1;
    //UcolMaxCap_ in allocateSomeArrays
    UcolEnd_=-1;


    EtaSize_=0;
    lastEtaRow_=-1;
    //maxEtaRows_ in allocateSomeArrays
    //EtaMaxCap_ in allocateSomeArrays

    // minIncrease_ in allocateSomeArrays
    updateTol_=1.0e12;

    doSuhlHeuristic_=true;
    maxU_=-1.0;
    maxGrowth_=1.e12;
    maxA_=-1.0;
    pivotCandLimit_=4;
    minIncrease_=10;

}

//  ~CoinSimpFactorization.  Destructor
CoinSimpFactorization::~CoinSimpFactorization (  )
{
  gutsOfDestructor();
}
//  =
CoinSimpFactorization & CoinSimpFactorization::operator = ( const CoinSimpFactorization & other ) {
  if (this != &other) {
    gutsOfDestructor();
    gutsOfInitialize();
    gutsOfCopy(other);
  }
  return *this;
}
void CoinSimpFactorization::gutsOfCopy(const CoinSimpFactorization &other)
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
  numberGoodU_ = other.numberGoodU_;
  maximumPivots_ = other.maximumPivots_;
  numberPivots_ = other.numberPivots_;
  factorElements_ = other.factorElements_;
  status_ = other.status_;
  numberSlacks_ = other.numberSlacks_;
  firstNumberSlacks_ = other.firstNumberSlacks_;
  if (other.pivotRow_) {
    pivotRow_ = new int [2*maximumRows_+maximumPivots_];
    memcpy(pivotRow_,other.pivotRow_,(2*maximumRows_+numberPivots_)*sizeof(int));
    elements_ = new CoinFactorizationDouble [maximumSpace_];
    memcpy(elements_,other.elements_,(maximumRows_+numberPivots_)*maximumRows_*sizeof(CoinFactorizationDouble));
    workArea_ = new CoinFactorizationDouble [maximumRows_];
  } else {
    elements_ = NULL;
    pivotRow_ = NULL;
    workArea_ = NULL;
  }

  keepSize_ = other.keepSize_;

  LrowSize_ = other.LrowSize_;
  LrowCap_ = other.LrowCap_;

  LcolSize_ = other.LcolSize_;
  LcolCap_ = other.LcolCap_;

  UrowMaxCap_ = other.UrowMaxCap_;
  UrowEnd_ = other.UrowEnd_;
  firstRowInU_ = other.firstRowInU_;
  lastRowInU_ = other.lastRowInU_;

  firstColInU_ = other.firstColInU_;
  lastColInU_ = other.lastColInU_;
  UcolMaxCap_ = other.UcolMaxCap_;
  UcolEnd_ = other.UcolEnd_;

  EtaSize_ = other.EtaSize_;
  lastEtaRow_ = other.lastEtaRow_;
  maxEtaRows_ = other.maxEtaRows_;
  EtaMaxCap_ = other.EtaMaxCap_;


  minIncrease_ = other.minIncrease_;
  updateTol_ = other.updateTol_;



  if (other.denseVector_)
      {
	  denseVector_ = new double[maximumRows_];
	  memcpy(denseVector_,other.denseVector_,maximumRows_*sizeof(double));
      } else denseVector_=NULL;
  if (other.workArea2_)
      {
	  workArea2_ = new double[maximumRows_];
	  memcpy(workArea2_,other.workArea2_,maximumRows_*sizeof(double));
      } else workArea2_=NULL;
  if (other.workArea3_)
      {
	  workArea3_ = new double[maximumRows_];
	  memcpy(workArea3_,other.workArea3_,maximumRows_*sizeof(double));
      } else workArea3_=NULL;
  if (other.vecLabels_)
      {
	  vecLabels_ = new int[maximumRows_];
	  memcpy(vecLabels_,other.vecLabels_,maximumRows_*sizeof(int));
      } else vecLabels_=NULL;
  if (other.indVector_)
      {
	  indVector_ = new int[maximumRows_];
	  memcpy(indVector_ ,other.indVector_ , maximumRows_ *sizeof(int ));
      } else indVector_=NULL;

  if (other.auxVector_)
      {
	  auxVector_ = new double[maximumRows_];
	  memcpy(auxVector_ ,other.auxVector_ , maximumRows_ *sizeof(double ));
      } else auxVector_=NULL;
  if (other.auxInd_)
      {
	  auxInd_ = new int[maximumRows_];
	  memcpy(auxInd_ , other.auxInd_, maximumRows_ *sizeof(int));
      } else auxInd_=NULL;

  if (other.vecKeep_)
      {
	  vecKeep_ = new double[maximumRows_];
	  memcpy(vecKeep_ ,other.vecKeep_ , maximumRows_ *sizeof(double));
      } else vecKeep_=NULL;
  if (other.indKeep_)
      {
	  indKeep_ = new int[maximumRows_];
	  memcpy(indKeep_ , other.indKeep_, maximumRows_ *sizeof(int));
      } else indKeep_=NULL;
  if (other.LrowStarts_)
      {
	  LrowStarts_ = new int[maximumRows_];
	  memcpy(LrowStarts_ , other.LrowStarts_, maximumRows_ *sizeof(int));
      } else LrowStarts_=NULL;
  if (other.LrowLengths_)
      {
	  LrowLengths_ = new int[maximumRows_];
	  memcpy(LrowLengths_ , other.LrowLengths_ , maximumRows_ *sizeof(int));
      } else LrowLengths_=NULL;
  if (other.Lrows_)
      {
	  Lrows_ = new double[other.LrowCap_];
	  memcpy(Lrows_ , other.Lrows_, other.LrowCap_*sizeof(double));
      } else Lrows_=NULL;
  if (other.LrowInd_)
      {
	  LrowInd_ = new int[other.LrowCap_];
	  memcpy(LrowInd_, other.LrowInd_,  other.LrowCap_*sizeof(int));
      } else LrowInd_=NULL;

  if (other.LcolStarts_)
      {
	  LcolStarts_ = new int[maximumRows_];
	  memcpy(LcolStarts_ ,other.LcolStarts_ , maximumRows_*sizeof(int));
      } else LcolStarts_=NULL;
  if (other.LcolLengths_)
      {
	  LcolLengths_ = new int[maximumRows_];
	  memcpy(LcolLengths_ , other.LcolLengths_ , maximumRows_ *sizeof(int));
      } else LcolLengths_=NULL;
  if (other.Lcolumns_)
      {
	  Lcolumns_ = new double[other.LcolCap_];
	  memcpy(Lcolumns_ ,other.Lcolumns_ , other.LcolCap_ *sizeof(double));
      } else Lcolumns_=NULL;
  if (other.LcolInd_)
      {
	  LcolInd_ = new int[other.LcolCap_];
	  memcpy(LcolInd_ , other.LcolInd_, other.LcolCap_*sizeof(int));
      } else LcolInd_=NULL;

  if (other.UrowStarts_)
      {
	  UrowStarts_ = new int[maximumRows_];
	  memcpy(UrowStarts_ ,other.UrowStarts_ , maximumRows_ *sizeof(int));
      } else UrowStarts_=NULL;
  if (other.UrowLengths_)
      {
	  UrowLengths_ = new int[maximumRows_];
	  memcpy(UrowLengths_ ,other.UrowLengths_ , maximumRows_ *sizeof(int));
      } else UrowLengths_=NULL;
#ifdef COIN_SIMP_CAPACITY
  if (other.UrowCapacities_)
      {
	  UrowCapacities_ = new int[maximumRows_];
	  memcpy(UrowCapacities_ ,other.UrowCapacities_ , maximumRows_ *sizeof(int));
      } else UrowCapacities_=NULL;
#endif
  if (other.Urows_)
      {
	  Urows_ = new double[other.UrowMaxCap_];
	  memcpy(Urows_ ,other.Urows_ , other.UrowMaxCap_ *sizeof(double));
      } else Urows_=NULL;
  if (other.UrowInd_)
      {
	  UrowInd_ = new int[other.UrowMaxCap_];
	  memcpy(UrowInd_,other.UrowInd_, other.UrowMaxCap_*sizeof(int));
      } else UrowInd_=NULL;

  if (other.prevRowInU_)
      {
	  prevRowInU_ = new int[maximumRows_];
	  memcpy(prevRowInU_ , other.prevRowInU_, maximumRows_*sizeof(int));
      } else prevRowInU_=NULL;
  if (other.nextRowInU_)
      {
	  nextRowInU_ = new int[maximumRows_];
	  memcpy(nextRowInU_, other.nextRowInU_, maximumRows_*sizeof(int));
      } else nextRowInU_=NULL;

  if (other.UcolStarts_)
      {
	  UcolStarts_ = new int[maximumRows_];
	  memcpy(UcolStarts_ , other.UcolStarts_, maximumRows_*sizeof(int));
      } else UcolStarts_=NULL;
  if (other.UcolLengths_)
      {
	  UcolLengths_ = new int[maximumRows_];
	  memcpy(UcolLengths_ , other.UcolLengths_,  maximumRows_*sizeof(int));
      } else UcolLengths_=NULL;
#ifdef COIN_SIMP_CAPACITY
  if (other.UcolCapacities_)
      {
	  UcolCapacities_ = new int[maximumRows_];
	  memcpy(UcolCapacities_ ,other.UcolCapacities_ , maximumRows_*sizeof(int));
      } else UcolCapacities_=NULL;
#endif
  if (other.Ucolumns_)
      {
	  Ucolumns_ = new double[other.UcolMaxCap_];
	  memcpy(Ucolumns_ ,other.Ucolumns_ , other.UcolMaxCap_*sizeof(double));
      } else Ucolumns_=NULL;
  if (other.UcolInd_)
      {
	  UcolInd_ = new int[other.UcolMaxCap_];
	  memcpy(UcolInd_ , other.UcolInd_ , other.UcolMaxCap_*sizeof(int));
      } else UcolInd_=NULL;
  if (other.prevColInU_)
      {
	  prevColInU_ = new int[maximumRows_];
	  memcpy(prevColInU_ , other.prevColInU_ , maximumRows_*sizeof(int));
      } else prevColInU_=NULL;
  if (other.nextColInU_)
      {
	  nextColInU_ = new int[maximumRows_];
	  memcpy(nextColInU_ ,other.nextColInU_ , maximumRows_*sizeof(int));
      } else nextColInU_=NULL;
  if (other.colSlack_)
      {
	  colSlack_ = new int[maximumRows_];
	  memcpy(colSlack_, other.colSlack_, maximumRows_*sizeof(int));
      }


  if (other.invOfPivots_)
      {
	  invOfPivots_ = new double[maximumRows_];
	  memcpy(invOfPivots_ , other.invOfPivots_,  maximumRows_*sizeof(double));
      } else invOfPivots_=NULL;

  if (other.colOfU_)
      {
	  colOfU_ = new int[maximumRows_];
	  memcpy(colOfU_ , other.colOfU_, maximumRows_*sizeof(int));
      } else colOfU_=NULL;
  if (other.colPosition_)
      {
	  colPosition_ = new int[maximumRows_];
	  memcpy(colPosition_, other.colPosition_,  maximumRows_*sizeof(int));
      } else colPosition_=NULL;
  if (other.rowOfU_)
      {
	  rowOfU_ = new int[maximumRows_];
	  memcpy(rowOfU_ , other.rowOfU_, maximumRows_*sizeof(int));
      } else rowOfU_=NULL;
  if (other.rowPosition_)
      {
	  rowPosition_ = new int[maximumRows_];
	  memcpy(rowPosition_ , other.rowPosition_,  maximumRows_*sizeof(int));
      } else rowPosition_=NULL;
  if (other.secRowOfU_)
      {
	  secRowOfU_ = new int[maximumRows_];
	  memcpy(secRowOfU_ , other.secRowOfU_,  maximumRows_*sizeof(int));
      } else secRowOfU_=NULL;
  if (other.secRowPosition_)
      {
	  secRowPosition_ = new int[maximumRows_];
	  memcpy(secRowPosition_ , other.secRowPosition_, maximumRows_*sizeof(int));
      } else secRowPosition_=NULL;

  if (other.EtaPosition_)
      {
	  EtaPosition_ = new int[other.maxEtaRows_];
	  memcpy(EtaPosition_ ,other.EtaPosition_ , other.maxEtaRows_ *sizeof(int));
      } else EtaPosition_=NULL;
  if (other.EtaStarts_)
      {
	  EtaStarts_ = new int[other.maxEtaRows_];
	  memcpy(EtaStarts_, other.EtaStarts_, other.maxEtaRows_*sizeof(int));
      } else EtaStarts_=NULL;
  if (other.EtaLengths_)
      {
	  EtaLengths_ = new int[other.maxEtaRows_];
	  memcpy(EtaLengths_, other.EtaLengths_, other.maxEtaRows_*sizeof(int));
      } else EtaLengths_=NULL;
  if (other.EtaInd_)
      {
	  EtaInd_	= new int[other.EtaMaxCap_];
	  memcpy(EtaInd_, other.EtaInd_, other.EtaMaxCap_*sizeof(int));
      } else EtaInd_=NULL;
  if (other.Eta_)
      {
	  Eta_ = new double[other.EtaMaxCap_];
	  memcpy(Eta_ , other.Eta_,  other.EtaMaxCap_*sizeof(double));
      } else Eta_=NULL;



  doSuhlHeuristic_ = other.doSuhlHeuristic_;
  maxU_ = other.maxU_;
  maxGrowth_ = other.maxGrowth_;
  maxA_ = other.maxA_;
  pivotCandLimit_ = other.pivotCandLimit_;
}

//  getAreas.  Gets space for a factorization
//called by constructors
void
CoinSimpFactorization::getAreas ( int numberOfRows,
			 int numberOfColumns,
			 CoinBigIndex ,
			 CoinBigIndex  )
{

  numberRows_ = numberOfRows;
  numberColumns_ = numberOfColumns;
  CoinBigIndex size = numberRows_*(numberRows_+CoinMax(maximumPivots_,(numberRows_+1)>>1));
  if (size>maximumSpace_) {
    delete [] elements_;
    elements_ = new CoinFactorizationDouble [size];
    maximumSpace_ = size;
  }
  if (numberRows_>maximumRows_) {
    maximumRows_ = numberRows_;
    delete [] pivotRow_;
    delete [] workArea_;
    pivotRow_ = new int [2*maximumRows_+maximumPivots_];
    workArea_ = new CoinFactorizationDouble [maximumRows_];
    allocateSomeArrays();
  }
}

//  preProcess.
void
CoinSimpFactorization::preProcess ()
{
    CoinBigIndex put = numberRows_*numberRows_;
    int *indexRow = reinterpret_cast<int *> (elements_+put);
    CoinBigIndex * starts = reinterpret_cast<CoinBigIndex *> (pivotRow_);
    initialSomeNumbers();

    // compute sizes for Urows_ and Ucolumns_
    //for ( int row=0; row < numberRows_; ++row )
    //UrowLengths_[row]=0;
    int k=0;
    for ( int column=0; column < numberColumns_; ++column ){
	UcolStarts_[column]=k;
 	//for (CoinBigIndex j=starts[column];j<starts[column+1];j++) {
	//  int iRow = indexRow[j];
	//  ++UrowLengths_[iRow];
	//	}
	UcolLengths_[column]=starts[column+1]-starts[column];
#ifdef COIN_SIMP_CAPACITY
	UcolCapacities_[column]=numberRows_;
#endif
	//k+=UcolLengths_[column]+minIncrease_;
	k+=numberRows_;
    }

    // create space for Urows_
    k=0;
    for ( int row=0; row < numberRows_; ++row ){
	prevRowInU_[row]=row-1;
	nextRowInU_[row]=row+1;
	UrowStarts_[row]=k;
	//k+=UrowLengths_[row]+minIncrease_;
	k+=numberRows_;
	UrowLengths_[row]=0;
#ifdef COIN_SIMP_CAPACITY
	UrowCapacities_[row]=numberRows_;
#endif
    }
    UrowEnd_=k;
    nextRowInU_[numberRows_-1]=-1;
    firstRowInU_=0;
    lastRowInU_=numberRows_-1;
    maxA_=-1.0;
    // build Ucolumns_ and Urows_
    int colBeg, colEnd;
    for ( int column=0; column < numberColumns_; ++column ){
	prevColInU_[column]=column-1;
	nextColInU_[column]=column+1;
	k=0;
	colBeg=starts[column];
	colEnd=starts[column+1];
	// identify slacks
	if ( colEnd == colBeg+1 && elements_[colBeg]==slackValue_ )
	    colSlack_[column]=1;
	else colSlack_[column]=0;
	//
	for (int j=colBeg; j < colEnd; j++) {
	    // Ucolumns_
	    int iRow = indexRow[j];
	    UcolInd_[UcolStarts_[column] + k]=iRow;
	    ++k;
	    // Urow_
	    int ind=UrowStarts_[iRow]+UrowLengths_[iRow];
	    UrowInd_[ind]=column;
	    Urows_[ind]=elements_[j];
	    //maxA_=CoinMax( maxA_, fabs(Urows_[ind]) );
	    ++UrowLengths_[iRow];
	}
    }
    nextColInU_[numberColumns_-1]=-1;
    firstColInU_=0;
    lastColInU_=numberColumns_-1;

    // Initialize L
    LcolSize_=0;
    //LcolCap_=numberRows_*minIncrease_;
    memset(LrowStarts_,-1,numberRows_ * sizeof(int));
    memset(LrowLengths_,0,numberRows_ * sizeof(int));
    memset(LcolStarts_,-1,numberRows_ * sizeof(int));
    memset(LcolLengths_,0,numberRows_ * sizeof(int));

    // initialize permutations
    for ( int i=0; i<numberRows_; ++i ){
	rowOfU_[i]=i;
	rowPosition_[i]=i;
    }
    for ( int i=0; i<numberColumns_; ++i ){
	colOfU_[i]=i;
	colPosition_[i]=i;
    }
    //

    doSuhlHeuristic_=true;

}

void CoinSimpFactorization::factorize(int numberOfRows,
				     int numberOfColumns,
				     const int colStarts[],
				     const int indicesRow[],
				     const double elements[])
{
    CoinBigIndex maximumL=0;
    CoinBigIndex maximumU=0;
    getAreas ( numberOfRows, numberOfColumns, maximumL, maximumU );

    CoinBigIndex put = numberRows_*numberRows_;
    int *indexRow = reinterpret_cast<int *> (elements_+put);
    CoinBigIndex * starts = reinterpret_cast<CoinBigIndex *> (pivotRow_);
    for ( int column=0; column <= numberColumns_; ++column ){
	starts[column]=colStarts[column];
    }
    const int limit=colStarts[numberColumns_];
    for ( int i=0; i<limit; ++i ){
	indexRow[i]=indicesRow[i];
	elements_[i]=elements[i];
    }

    preProcess();
    factor();
}

//Does factorization
int
CoinSimpFactorization::factor ( )
{
  numberPivots_=0;
  status_= 0;

  FactorPointers pointers(numberRows_, numberColumns_, UrowLengths_, UcolLengths_);
  int rc=mainLoopFactor (pointers);
  // rc=0 success
  if ( rc != 0 ) { // failure
      status_ = -1;
      //return status_; // failure
  }
  //if ( rc == -3 ) {  // numerical instability
  //    status_ = -1;
  //    return status_;
  //}

  //copyLbyRows();
  copyUbyColumns();
  copyRowPermutations();
  firstNumberSlacks_=numberSlacks_;
  // row permutations
  if ( status_==-1 || numberColumns_ < numberRows_ ){
      for (int j=0;j<numberRows_;j++)
	  pivotRow_[j+numberRows_]=rowOfU_[j];
      for (int j=0;j<numberRows_;j++) {
	  int k = pivotRow_[j+numberRows_];
	  pivotRow_[k]=j;
      }
  }
  else // no permutations
      for (int j=0;j<numberRows_;j++){
	  pivotRow_[j]=j;
	  pivotRow_[j+numberRows_]=j;
      }

  return status_;
}
//



// Makes a non-singular basis by replacing variables
void
CoinSimpFactorization::makeNonSingular(int * sequence, int numberColumns)
{
  // Replace bad ones by correct slack
  int * workArea= reinterpret_cast<int *> (workArea_);
  int i;
  for ( i=0;i<numberRows_;i++)
    workArea[i]=-1;
  for ( i=0;i<numberGoodU_;i++) {
    int iOriginal = pivotRow_[i+numberRows_];
    workArea[iOriginal]=i;
    //workArea[i]=iOriginal;
  }
  int lastRow=-1;
  for ( i=0;i<numberRows_;i++) {
    if (workArea[i]==-1) {
      lastRow=i;
      break;
    }
  }
  assert (lastRow>=0);
  for ( i=numberGoodU_;i<numberRows_;i++) {
    assert (lastRow<numberRows_);
    // Put slack in basis
    sequence[i]=lastRow+numberColumns;
    lastRow++;
    for (;lastRow<numberRows_;lastRow++) {
      if (workArea[lastRow]==-1)
	break;
    }
  }
}
// Does post processing on valid factorization - putting variables on correct rows
void
CoinSimpFactorization::postProcess(const int * sequence, int * pivotVariable)
{
  for (int i=0;i<numberRows_;i++) {
    int k = sequence[i];
    pivotVariable[pivotRow_[i+numberRows_]]=k;
  }
}
/* Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
   If checkBeforeModifying is true will do all accuracy checks
   before modifying factorization.  Whether to set this depends on
   speed considerations.  You could just do this on first iteration
   after factorization and thereafter re-factorize
   partial update already in U */
int
CoinSimpFactorization::replaceColumn ( CoinIndexedVector * ,
				      int pivotRow,
				      double pivotCheck ,
				       bool ,
				       double )
{
    if (numberPivots_==maximumPivots_)
	return 3;

    double pivotValue = pivotCheck;
    if (fabs(pivotValue)<zeroTolerance_)
	return 2;
    int realPivotRow = pivotRow_[pivotRow];

    LUupdate(pivotRow);

    pivotRow_[2*numberRows_+numberPivots_]=realPivotRow;
    numberPivots_++;

    return 0;
}
int
CoinSimpFactorization::updateColumn ( CoinIndexedVector * regionSparse,
				       CoinIndexedVector * regionSparse2,
				       bool noPermute) const
{
    return upColumn(regionSparse, regionSparse2, noPermute, false);
}
int
CoinSimpFactorization::updateColumnFT( CoinIndexedVector * regionSparse,
				       CoinIndexedVector * regionSparse2,
				       bool noPermute)
{

    int rc=upColumn(regionSparse, regionSparse2, noPermute, true);
    return rc;
}


int
CoinSimpFactorization::upColumn( CoinIndexedVector * regionSparse,
				  CoinIndexedVector * regionSparse2,
				  bool , bool save) const
{
    assert (numberRows_==numberColumns_);
    double *region2 = regionSparse2->denseVector (  );
    int *regionIndex = regionSparse2->getIndices (  );
    int numberNonZero = regionSparse2->getNumElements (  );
    double *region=regionSparse->denseVector (  );
    if (!regionSparse2->packedMode()) {
	region=regionSparse2->denseVector (  );
    }
    else { // packed mode
	for (int j=0;j<numberNonZero;j++) {
	    region[regionIndex[j]]=region2[j];
	    region2[j]=0.0;
	}
    }

    double *solution=workArea2_;
    ftran(region, solution, save);

    // get nonzeros
    numberNonZero=0;
    if (!regionSparse2->packedMode()) {
	for (int i=0;i<numberRows_;i++) {
	    const double value=solution[i];
	    if ( fabs(value) > zeroTolerance_ ){
		region[i]=value;
		regionIndex[numberNonZero++]=i;
	    }
	    else
		region[i]=0.0;
	}
    }
    else { // packed mode
	memset(region,0,numberRows_*sizeof(double));
	for (int i=0;i<numberRows_;i++) {
	    const double value=solution[i];
	    if ( fabs(value) > zeroTolerance_ ){
		region2[numberNonZero] = value;
		regionIndex[numberNonZero++]=i;
	    }
	}
    }
    regionSparse2->setNumElements(numberNonZero);
    return 0;
}


int
CoinSimpFactorization::updateTwoColumnsFT(CoinIndexedVector * regionSparse1,
					  CoinIndexedVector * regionSparse2,
					  CoinIndexedVector * regionSparse3,
					  bool )
{
    assert (numberRows_==numberColumns_);

    double *region2 = regionSparse2->denseVector (  );
    int *regionIndex2 = regionSparse2->getIndices (  );
    int numberNonZero2 = regionSparse2->getNumElements (  );

    double *vec1=regionSparse1->denseVector (  );
    if (!regionSparse2->packedMode()) {
	vec1=regionSparse2->denseVector (  );
    }
    else { // packed mode
	for (int j=0;j<numberNonZero2;j++) {
	    vec1[regionIndex2[j]]=region2[j];
	    region2[j]=0.0;
	}
    }
    //
    double *region3 = regionSparse3->denseVector (  );
    int *regionIndex3 = regionSparse3->getIndices (  );
    int numberNonZero3 = regionSparse3->getNumElements (  );
    double *vec2=auxVector_;
    if (!regionSparse3->packedMode()) {
	vec2=regionSparse3->denseVector (  );
    }
    else { // packed mode
	memset(vec2,0,numberRows_*sizeof(double));
	for (int j=0;j<numberNonZero3;j++) {
	    vec2[regionIndex3[j]]=region3[j];
	    region3[j]=0.0;
	}
    }

    double *solution1=workArea2_;
    double *solution2=workArea3_;
    ftran2(vec1, solution1, vec2, solution2);

    // get nonzeros
    numberNonZero2=0;
    if (!regionSparse2->packedMode()) {
	double value;
	for (int i=0;i<numberRows_;i++) {
	    value=solution1[i];
	    if ( fabs(value) > zeroTolerance_ ){
		vec1[i]=value;
		regionIndex2[numberNonZero2++]=i;
	    }
	    else
		vec1[i]=0.0;
	}
    }
    else { // packed mode
	double value;
	for (int i=0;i<numberRows_;i++) {
	    vec1[i]=0.0;
	    value=solution1[i];
	    if ( fabs(value) > zeroTolerance_ ){
		region2[numberNonZero2] = value;
		regionIndex2[numberNonZero2++]=i;
	    }
	}
    }
    regionSparse2->setNumElements(numberNonZero2);
    //
    numberNonZero3=0;
    if (!regionSparse3->packedMode()) {
	double value;
	for (int i=0;i<numberRows_;i++) {
	    value=solution2[i];
	    if ( fabs(value) > zeroTolerance_ ){
		vec2[i]=value;
		regionIndex3[numberNonZero3++]=i;
	    }
	    else
		vec2[i]=0.0;
	}
    }
    else { // packed mode
	double value;
	for (int i=0;i<numberRows_;i++) {
	    value=solution2[i];
	    if ( fabs(value) > zeroTolerance_ ){
		region3[numberNonZero3] = value;
		regionIndex3[numberNonZero3++]=i;
	    }
	}
    }
    regionSparse3->setNumElements(numberNonZero3);
    return 0;
}



int
CoinSimpFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
					       CoinIndexedVector * regionSparse2) const
{
    upColumnTranspose(regionSparse, regionSparse2);
    return 0;
}

int
CoinSimpFactorization::upColumnTranspose ( CoinIndexedVector * regionSparse,
					       CoinIndexedVector * regionSparse2) const
{
    assert (numberRows_==numberColumns_);
    double *region2 = regionSparse2->denseVector (  );
    int *regionIndex = regionSparse2->getIndices (  );
    int numberNonZero = regionSparse2->getNumElements (  );
    double *region = regionSparse->denseVector (  );
    if (!regionSparse2->packedMode()) {
	region=regionSparse2->denseVector (  );
    }
    else { // packed
	for (int j=0;j<numberNonZero;j++) {
	    region[regionIndex[j]]=region2[j];
	    region2[j]=0.0;
	}
    }
    double *solution=workArea2_;
    btran(region, solution);
    // get nonzeros
    numberNonZero=0;
    if (!regionSparse2->packedMode()) {
	double value;
	for (int i=0;i<numberRows_;i++) {
	    value=solution[i];
	    if ( fabs(value) > zeroTolerance_ ){
		region[i]=value;
		regionIndex[numberNonZero++]=i;
	    }
	    else
		region[i]=0.0;
	}
    }
    else { // packed mode
	memset(region,0,numberRows_*sizeof(double));
	for (int i=0;i<numberRows_;i++) {
	    const double value=solution[i];
	    if ( fabs(value) > zeroTolerance_ ){
		region2[numberNonZero] = value;
		regionIndex[numberNonZero++]=i;
	    }
	}
    }
    regionSparse2->setNumElements(numberNonZero);
    return 0;
}



int
CoinSimpFactorization::mainLoopFactor (FactorPointers &pointers )
{
    numberGoodU_=0;
    numberSlacks_=0;
    bool ifSlack=true;
    for ( int i=0; i<numberColumns_; ++i ){
	int r, s;
	//s=i;
	if ( findPivot(pointers,r,s,ifSlack) ){
	    return -1;
	}
	if ( ifSlack ) ++numberSlacks_;
	const int rowPos=rowPosition_[r];
	const int colPos=colPosition_[s];
	assert( i <= rowPos && rowPos < numberRows_);
	assert( i <= colPos && colPos < numberColumns_);
	// permute columns
	int j=colOfU_[i];
	colOfU_[i]=colOfU_[colPos];
	colOfU_[colPos]=j;
	colPosition_[colOfU_[i]]=i;
	colPosition_[colOfU_[colPos]]=colPos;
	// permute rows
	j=rowOfU_[i];
	rowOfU_[i]=rowOfU_[rowPos];
	rowOfU_[rowPos]=j;
	rowPosition_[rowOfU_[i]]=i;
	rowPosition_[rowOfU_[rowPos]]=rowPos;
	GaussEliminate(pointers,r,s);
	//if ( maxU_ > maxGrowth_ * maxA_  ){
	//  return -3;
	//}
	++numberGoodU_;
    }
    return 0;
}
/// find a pivot in the active part of U
int CoinSimpFactorization::findPivot(FactorPointers &pointers, int &r,
				     int &s, bool &ifSlack){
    int *firstRowKnonzeros=pointers.firstRowKnonzeros;
    int *nextRow=pointers.nextRow;
    int *firstColKnonzeros=pointers.firstColKnonzeros;
    int *prevColumn=pointers.prevColumn;
    int *nextColumn=pointers.nextColumn;
    r=s=-1;
    int numCandidates=0;
    double bestMarkowitzCount=COIN_DBL_MAX;
    // if there is a column with one element choose it as pivot
    int column=firstColKnonzeros[1];
    if ( column!=-1 ){
	assert( UcolLengths_[column] == 1 );
	r=UcolInd_[UcolStarts_[column]];
	s=column;
	if ( !colSlack_[column] )
	    ifSlack=false;
	return 0;
    }
    // from now on no more slacks
    ifSlack=false;
    // if there is a  row with one element choose it
    int row=firstRowKnonzeros[1];
    if ( row!=-1 ){
	assert( UrowLengths_[row] == 1 );
	s=UrowInd_[UrowStarts_[row]];
	r=row;
	return 0;
    }
    // consider other rows and columns
    for ( int length=2; length <=numberRows_; ++length){
	int nextCol=-1;
	for ( column=firstColKnonzeros[length]; column!=-1; column=nextCol ){
	    nextCol=nextColumn[column];
	    int minRow, minRowLength;
	    int rc=findShortRow(column, length, minRow, minRowLength, pointers);
	    if ( rc== 0 ){
		r=minRow;
		s=column;
		return 0;
	    }
	    if ( minRow != -1 ){
		++numCandidates;
		double MarkowitzCount=static_cast<double>(minRowLength-1)*(length-1);
		if ( MarkowitzCount < bestMarkowitzCount ){
		    r=minRow; s=column;
		    bestMarkowitzCount=MarkowitzCount;
		}
		if ( numCandidates == pivotCandLimit_ ) return 0;
	    }
	    else {
		if ( doSuhlHeuristic_ ){
		    // this column did not give a candidate, it will be
		    // removed until it becomes a singleton
		    removeColumnFromActSet(column, pointers);
		    prevColumn[column]=nextColumn[column]=column;
		}
	    }
	} // end for ( column= ....
	// now rows
	for ( row=firstRowKnonzeros[length]; row!=-1; row=nextRow[row] ){
	    int minCol, minColLength;
	    int rc=findShortColumn(row, length, minCol, minColLength, pointers);
	    if ( rc==0 ){
		r=row;
		s=minCol;
		return 0;
	    }
	    if ( minCol != -1 ){
		++numCandidates;
		double MarkowitzCount=static_cast<double>(minColLength-1)*(length-1);
		if ( MarkowitzCount < bestMarkowitzCount ){
		    r=row; s=minCol;
		    bestMarkowitzCount=MarkowitzCount;
		}
		if ( numCandidates == pivotCandLimit_ ) return 0;
	    }
	    //else abort();
	}// end for ( row= ...
    }// end for ( int length= ...
    if ( r== -1 || s==-1 ) return 1;
    else return 0;
}
//
int CoinSimpFactorization::findPivotShCol(FactorPointers &pointers, int &r, int &s)
{
    int *firstColKnonzeros=pointers.firstColKnonzeros;
    r=s=-1;
    // if there is a column with one element choose it as pivot
    int column=firstColKnonzeros[1];
    if ( column!=-1 ){
	assert( UcolLengths_[column] == 1 );
	r=UcolInd_[UcolStarts_[column]];
	s=column;
	return 0;
    }
    // consider other columns
    for ( int length=2; length <=numberRows_; ++length){
	column=firstColKnonzeros[length];
	if ( column != -1 ) break;
    }
    if ( column == -1 ) return 1;
    // find largest element
    const int colBeg=UcolStarts_[column];
    const int colEnd=colBeg+UcolLengths_[column];
    double largest=0.0;
    int rowLargest=-1;
    for ( int j=colBeg; j<colEnd; ++j ){
	const int row=UcolInd_[j];
	const int columnIndx=findInRow(row,column);
	assert(columnIndx!=-1);
	double coeff=fabs(Urows_[columnIndx]);
	if ( coeff < largest ) continue;
	largest=coeff;
	rowLargest=row;
    }
    assert(rowLargest != -1);
    s=column;
    r=rowLargest;
    return 0;
}

int CoinSimpFactorization::findPivotSimp(FactorPointers &, int &r, int &s){
    r=-1;
    int column=s;
    const int colBeg=UcolStarts_[column];
    const int colEnd=colBeg+UcolLengths_[column];
    double largest=0.0;
    int rowLargest=-1;
    for ( int j=colBeg; j<colEnd; ++j ){
	const int row=UcolInd_[j];
	const int columnIndx=findInRow(row,column);
	assert(columnIndx!=-1);
	double coeff=fabs(Urows_[columnIndx]);
	if ( coeff < largest ) continue;
	largest=coeff;
	rowLargest=row;
    }
    if ( rowLargest != -1 ){
	r=rowLargest;
	return 0;
    }
    else return 1;
}


int CoinSimpFactorization::findShortRow(const int column,
				       const int length,
				       int &minRow,
				       int &minRowLength,
				       FactorPointers &pointers)
{
    const int colBeg=UcolStarts_[column];
    const int colEnd=colBeg+UcolLengths_[column];
    minRow=-1;
    minRowLength= COIN_INT_MAX;
    for ( int j=colBeg; j<colEnd; ++j ){
	int row=UcolInd_[j];
	if ( UrowLengths_[row] >= minRowLength ) continue;
	double largestInRow=findMaxInRrow(row,pointers);
	// find column in row
	int columnIndx=findInRow(row,column);
	assert(columnIndx!=-1);
	double coeff=Urows_[columnIndx];
	if ( fabs(coeff) < pivotTolerance_ * largestInRow ) continue;
	minRow=row; minRowLength=UrowLengths_[row];
	if ( UrowLengths_[row] <= length ) return 0;
    }
    return 1;
}
int CoinSimpFactorization::findShortColumn(const int row,
					  const int length,
					  int &minCol,
					  int &minColLength,
					  FactorPointers &pointers)
{
    const int rowBeg=UrowStarts_[row];
    const int rowEnd=rowBeg+UrowLengths_[row];
    minCol=-1;
    minColLength=COIN_INT_MAX;
    double largestInRow=findMaxInRrow(row,pointers);
    for ( int i=rowBeg; i<rowEnd; ++i ){
	int column=UrowInd_[i];
	if ( UcolLengths_[column] >= minColLength ) continue;
	double coeff=Urows_[i];
	if ( fabs(coeff) < pivotTolerance_ * largestInRow ) continue;
	minCol=column;
	minColLength=UcolLengths_[column];
	if ( minColLength <= length ) return 0;
    }
    return 1;
}
// Gaussian elimination
void CoinSimpFactorization::GaussEliminate(FactorPointers &pointers, int &pivotRow, int &pivotCol)
{
    assert( pivotRow >= 0 && pivotRow < numberRows_ );
    assert( pivotCol >= 0 && pivotCol < numberRows_ );
    int *firstColKnonzeros=pointers.firstColKnonzeros;
    int *prevColumn=pointers.prevColumn;
    int *nextColumn=pointers.nextColumn;
    int *colLabels=vecLabels_;
    double *denseRow=denseVector_;
    removeRowFromActSet(pivotRow, pointers);
    removeColumnFromActSet(pivotCol, pointers);
    // find column s
    int indxColS=findInRow(pivotRow, pivotCol);
    assert( indxColS >= 0 );
    // store the inverse of the pivot and remove it from row
    double invPivot=1.0/Urows_[indxColS];
    invOfPivots_[pivotRow]=invPivot;
    int rowBeg=UrowStarts_[pivotRow];
    int rowEnd=rowBeg+UrowLengths_[pivotRow];
    Urows_[indxColS]=Urows_[rowEnd-1];
    UrowInd_[indxColS]=UrowInd_[rowEnd-1];
    --UrowLengths_[pivotRow];
    --rowEnd;
    // now remove pivot from column
    int indxRowR=findInColumn(pivotCol,pivotRow);
    assert( indxRowR >= 0 );
    const int pivColEnd=UcolStarts_[pivotCol]+UcolLengths_[pivotCol];
    UcolInd_[indxRowR]=UcolInd_[pivColEnd-1];
    --UcolLengths_[pivotCol];
    // go through pivot row
    for ( int i=rowBeg; i<rowEnd; ++i ){
	int column=UrowInd_[i];
	colLabels[column]=1;
	denseRow[column]=Urows_[i];
	// remove this column from bucket because it will change
	removeColumnFromActSet(column, pointers);
	// remove element (pivotRow, column) from column
	int indxRow=findInColumn(column,pivotRow);
	assert( indxRow>=0 );
	const int colEnd=UcolStarts_[column]+UcolLengths_[column];
	UcolInd_[indxRow]=UcolInd_[colEnd-1];
	--UcolLengths_[column];
    }
    //
    pivoting(pivotRow, pivotCol, invPivot, pointers);
    //
    rowBeg=UrowStarts_[pivotRow];
    rowEnd=rowBeg+UrowLengths_[pivotRow];
    for ( int i=rowBeg; i<rowEnd; ++i ){
	int column=UrowInd_[i];
	// clean back these two arrays
	colLabels[column]=0;
	denseRow[column]=0.0;
	// column goes into a bucket, if Suhl' heuristic had removed it, it
	// can go back only if it is a singleton
	if ( UcolLengths_[column] != 1 ||
	     prevColumn[column]!=column || nextColumn[column]!=column )
	    {
		prevColumn[column]=-1;
		nextColumn[column]=firstColKnonzeros[UcolLengths_[column]];
		if ( nextColumn[column] != -1 )
		    prevColumn[nextColumn[column]]=column;
		firstColKnonzeros[UcolLengths_[column]]=column;
	    }
    }
}
void CoinSimpFactorization::pivoting(const int pivotRow,
				    const int pivotColumn,
				    const double invPivot,
				    FactorPointers &pointers)
{
    // initialize the new column of L
    LcolStarts_[pivotRow]=LcolSize_;
    // go trough pivot column
    const int colBeg=UcolStarts_[pivotColumn];
    int colEnd=colBeg+UcolLengths_[pivotColumn];
    for ( int i=colBeg; i<colEnd; ++i ){
	int row=UcolInd_[i];
	// remove row from bucket because it will change
	removeRowFromActSet(row, pointers);
	// find pivot column
	int pivotColInRow=findInRow(row,pivotColumn);
	assert(pivotColInRow >= 0);
	const double multiplier=Urows_[pivotColInRow]*invPivot;
	// remove element (row,pivotColumn) from row
	const int currentRowEnd=UrowStarts_[row]+UrowLengths_[row];
	Urows_[pivotColInRow]=Urows_[currentRowEnd-1];
	UrowInd_[pivotColInRow]=UrowInd_[currentRowEnd-1];
	--UrowLengths_[row];
	int newNonZeros=UrowLengths_[pivotRow];
	updateCurrentRow(pivotRow, row, multiplier, pointers,
			 newNonZeros);
	// store multiplier
	if ( LcolSize_ == LcolCap_ ) increaseLsize();
	Lcolumns_[LcolSize_]=multiplier;
	LcolInd_[LcolSize_++]=row;
	++LcolLengths_[pivotRow];
    }
    // remove elements of pivot column
    UcolLengths_[pivotColumn]=0;
    // remove pivot column from Ucol_
    if ( prevColInU_[pivotColumn]==-1 )
	firstColInU_=nextColInU_[pivotColumn];
    else{
	nextColInU_[prevColInU_[pivotColumn]]=nextColInU_[pivotColumn];
#ifdef COIN_SIMP_CAPACITY
	UcolCapacities_[prevColInU_[pivotColumn]]+=UcolCapacities_[pivotColumn];
	UcolCapacities_[pivotColumn]=0;
#endif
    }
    if ( nextColInU_[pivotColumn] == -1 )
	lastColInU_=prevColInU_[pivotColumn];
    else
	prevColInU_[nextColInU_[pivotColumn]]=prevColInU_[pivotColumn];
}



void CoinSimpFactorization::updateCurrentRow(const int pivotRow,
					    const int row,
					    const double multiplier,
					    FactorPointers &pointers,
					    int &newNonZeros)
{
    double *rowMax=pointers.rowMax;
    int *firstRowKnonzeros=pointers.firstRowKnonzeros;
    int *prevRow=pointers.prevRow;
    int *nextRow=pointers.nextRow;
    int *colLabels=vecLabels_;
    double *denseRow=denseVector_;
    const int rowBeg=UrowStarts_[row];
    int rowEnd=rowBeg+UrowLengths_[row];
    // treat old nonzeros
    for ( int i=rowBeg; i<rowEnd; ++i ){
	const int column=UrowInd_[i];
	if ( colLabels[column] ){
	    Urows_[i]-= multiplier*denseRow[column];
	    const double absNewCoeff=fabs(Urows_[i]);
	    colLabels[column]=0;
	    --newNonZeros;
	    if ( absNewCoeff < zeroTolerance_ ){
		// remove it from row
		UrowInd_[i]=UrowInd_[rowEnd-1];
		Urows_[i]=Urows_[rowEnd-1];
		--UrowLengths_[row];
		--i;
		--rowEnd;
		// remove it from column
		int indxRow=findInColumn(column, row);
		assert( indxRow >= 0 );
		const int colEnd=UcolStarts_[column]+UcolLengths_[column];
		UcolInd_[indxRow]=UcolInd_[colEnd-1];
		--UcolLengths_[column];
	    }
	    else
		{
		    if ( maxU_ < absNewCoeff )
			maxU_=absNewCoeff;
		}
	}
    }
    // now add the new nonzeros to the row
#ifdef COIN_SIMP_CAPACITY
    if ( UrowLengths_[row] + newNonZeros > UrowCapacities_[row] )
	increaseRowSize(row, UrowLengths_[row] + newNonZeros);
#endif
    const int pivotRowBeg=UrowStarts_[pivotRow];
    const int pivotRowEnd=pivotRowBeg+UrowLengths_[pivotRow];
    int numNew=0;
    int *newCols=pointers.newCols;
    for ( int i=pivotRowBeg; i<pivotRowEnd; ++i ){
	const int column=UrowInd_[i];
	if ( colLabels[column] ){
	    const double value= -multiplier*denseRow[column];
	    const double absNewCoeff=fabs(value);
	    if ( absNewCoeff >= zeroTolerance_ ){
		const int newInd=UrowStarts_[row]+UrowLengths_[row];
		Urows_[newInd]=value;
		UrowInd_[newInd]=column;
		++UrowLengths_[row];
		newCols[numNew++]=column;
		if ( maxU_ < absNewCoeff ) maxU_=absNewCoeff;
	    }
	}
	else colLabels[column]=1;
    }
    // add the new nonzeros to the columns
    for ( int i=0; i<numNew; ++i){
	const int column=newCols[i];
#ifdef COIN_SIMP_CAPACITY
	if ( UcolLengths_[column] + 1 > UcolCapacities_[column] ){
	    increaseColSize(column, UcolLengths_[column] + 1, false);
	}
#endif
	const int newInd=UcolStarts_[column]+UcolLengths_[column];
	UcolInd_[newInd]=row;
	++UcolLengths_[column];
    }
    // the row goes to a new bucket
    prevRow[row]=-1;
    nextRow[row]=firstRowKnonzeros[UrowLengths_[row]];
    if ( nextRow[row]!=-1 ) prevRow[nextRow[row]]=row;
    firstRowKnonzeros[UrowLengths_[row]]=row;
    //
    rowMax[row]=-1.0;
}
#ifdef COIN_SIMP_CAPACITY

void CoinSimpFactorization::increaseRowSize(const int row,
					   const int newSize)
{
    assert( newSize > UrowCapacities_[row] );
    const int newNumElements=newSize + minIncrease_;
    if ( UrowMaxCap_ < UrowEnd_ + newNumElements ){
	enlargeUrow( UrowEnd_ + newNumElements - UrowMaxCap_ );
    }
    int currentCapacity=UrowCapacities_[row];
    memcpy(&Urows_[UrowEnd_],&Urows_[UrowStarts_[row]],
	    UrowLengths_[row] * sizeof(double));
    memcpy(&UrowInd_[UrowEnd_],&UrowInd_[UrowStarts_[row]],
	    UrowLengths_[row] * sizeof(int));
    UrowStarts_[row]=UrowEnd_;
    UrowCapacities_[row]=newNumElements;
    UrowEnd_+=UrowCapacities_[row];
    if ( firstRowInU_==lastRowInU_ ) return; // only one element
    // remove row from list
    if( prevRowInU_[row]== -1)
	firstRowInU_=nextRowInU_[row];
    else {
	nextRowInU_[prevRowInU_[row]]=nextRowInU_[row];
	UrowCapacities_[prevRowInU_[row]]+=currentCapacity;
    }
    if ( nextRowInU_[row]==-1 )
	lastRowInU_=prevRowInU_[row];
    else
	prevRowInU_[nextRowInU_[row]]=prevRowInU_[row];
    // add row at the end of list
    nextRowInU_[lastRowInU_]=row;
    nextRowInU_[row]=-1;
    prevRowInU_[row]=lastRowInU_;
    lastRowInU_=row;
}
#endif
#ifdef COIN_SIMP_CAPACITY
void CoinSimpFactorization::increaseColSize(const int column,
					   const int newSize,
					   const bool ifElements)
{
    assert( newSize > UcolCapacities_[column] );
    const int newNumElements=newSize+minIncrease_;
    if ( UcolMaxCap_ < UcolEnd_ + newNumElements ){
	enlargeUcol(UcolEnd_ + newNumElements - UcolMaxCap_,
		    ifElements);
    }
    int currentCapacity=UcolCapacities_[column];
    memcpy(&UcolInd_[UcolEnd_], &UcolInd_[UcolStarts_[column]],
	    UcolLengths_[column] * sizeof(int));
    if ( ifElements ){
	memcpy(&Ucolumns_[UcolEnd_], &Ucolumns_[UcolStarts_[column]],
	       UcolLengths_[column] * sizeof(double) );
    }
    UcolStarts_[column]=UcolEnd_;
    UcolCapacities_[column]=newNumElements;
    UcolEnd_+=UcolCapacities_[column];
    if ( firstColInU_==lastColInU_ ) return; // only one column
    // remove from list
    if ( prevColInU_[column]==-1 )
	firstColInU_=nextColInU_[column];
    else {
	nextColInU_[prevColInU_[column]]=nextColInU_[column];
	UcolCapacities_[prevColInU_[column]]+=currentCapacity;
    }
    if ( nextColInU_[column]==-1 )
	lastColInU_=prevColInU_[column];
    else
	prevColInU_[nextColInU_[column]]=prevColInU_[column];
    // add column at the end
    nextColInU_[lastColInU_]=column;
    nextColInU_[column]=-1;
    prevColInU_[column]=lastColInU_;
    lastColInU_=column;
}
#endif
double CoinSimpFactorization::findMaxInRrow(const int row,
					    FactorPointers &pointers)
{
    double *rowMax=pointers.rowMax;
    double largest=rowMax[row];
    if ( largest >= 0.0 ) return largest;
    const int rowBeg=UrowStarts_[row];
    const int rowEnd=rowBeg+UrowLengths_[row];
    for ( int i=rowBeg; i<rowEnd; ++i ) {
	const double absValue=fabs(Urows_[i]);
	if ( absValue  > largest )
	    largest=absValue;
    }
    rowMax[row]=largest;
    return largest;
}
void CoinSimpFactorization::increaseLsize()
{
    int newcap= LcolCap_ + minIncrease_;

    double *aux=new double[newcap];
    memcpy(aux, Lcolumns_, LcolCap_ * sizeof(double));
    delete [] Lcolumns_;
    Lcolumns_=aux;

    int *iaux=new int[newcap];
    memcpy(iaux, LcolInd_, LcolCap_ * sizeof(int));
    delete [] LcolInd_;
    LcolInd_=iaux;

    LcolCap_=newcap;
}
void CoinSimpFactorization::enlargeUcol(const int numNewElements, const bool ifElements)
{
    int *iaux=new int[UcolMaxCap_+numNewElements];
    memcpy(iaux, UcolInd_, UcolMaxCap_*sizeof(int) );
    delete [] UcolInd_;
    UcolInd_=iaux;

    if ( ifElements ){
	double *aux=new double[UcolMaxCap_+numNewElements];
	memcpy(aux, Ucolumns_, UcolMaxCap_*sizeof(double) );
	delete [] Ucolumns_;
	Ucolumns_=aux;
    }

    UcolMaxCap_+=numNewElements;
}
void CoinSimpFactorization::enlargeUrow(const int numNewElements)
{
    int *iaux=new int[UrowMaxCap_+numNewElements];
    memcpy(iaux, UrowInd_, UrowMaxCap_*sizeof(int) );
    delete [] UrowInd_;
    UrowInd_=iaux;

    double *aux=new double[UrowMaxCap_+numNewElements];
    memcpy(aux, Urows_, UrowMaxCap_*sizeof(double) );
    delete [] Urows_;
    Urows_=aux;

    UrowMaxCap_+=numNewElements;
}

void CoinSimpFactorization::copyUbyColumns()
{
    memset(UcolLengths_,0,numberColumns_*sizeof(int));
    for ( int column=0; column<numberColumns_; ++column ){
	prevColInU_[column]=column-1;
	nextColInU_[column]=column+1;
    }
    nextColInU_[numberColumns_-1]=-1;
    firstColInU_=0;
    lastColInU_=numberColumns_-1;
    //int nonZeros=0;
    //for ( int row=0; row<numberRows_; ++row ){
    //const int rowBeg=UrowStarts_[row];
    //const int rowEnd=rowBeg+UrowLengths_[row];
    //for ( int j=rowBeg; j<rowEnd; ++j )
    //   ++UcolCapacities_[UrowInd_[j]];
    //	nonZeros+=UrowLengths_[row];
    //   }
    //
    //memset(UcolCapacities_, numberRows_, numberColumns_ * sizeof(int));
#ifdef COIN_SIMP_CAPACITY
    for ( int i=0; i<numberColumns_; ++i ) UcolCapacities_[i]=numberRows_;
#endif
    int k=0;
    for ( int column=0; column<numberColumns_; ++column ){
	UcolStarts_[column]=k;
	//UcolCapacities_[column]+=minIncrease_;
	//k+=UcolCapacities_[column];
	k+=numberRows_;
    }
    UcolEnd_=k;
    // go through the rows and fill the columns, assume UcolLengths_[]=0
    for ( int row=0; row<numberRows_; ++row ){
	const int rowBeg=UrowStarts_[row];
	int rowEnd=rowBeg+UrowLengths_[row];
	for ( int j=rowBeg; j<rowEnd; ++j ){
	    // remove els close to zero
	    while( fabs( Urows_[j] ) < zeroTolerance_ ){
		--UrowLengths_[row];
		--rowEnd;
		if ( j < rowEnd ){
		    Urows_[j]=Urows_[rowEnd];
		    UrowInd_[j]=UrowInd_[rowEnd];
		}
		else break;
	    }
	    if ( j==rowEnd ) continue;
	    //
	    const int column=UrowInd_[j];
	    const int indx=UcolStarts_[column]+UcolLengths_[column];
	    Ucolumns_[indx]=Urows_[j];
	    UcolInd_[indx]=row;
	    ++UcolLengths_[column];
	}
    }
}
void CoinSimpFactorization::copyLbyRows()
{
    int nonZeros=0;
    memset(LrowLengths_,0,numberRows_*sizeof(int));
    for ( int column=0; column<numberRows_; ++column ){
	const int colBeg=LcolStarts_[column];
	const int colEnd=colBeg+LcolLengths_[column];
	for ( int j=colBeg; j<colEnd; ++j )
	    ++LrowLengths_[LcolInd_[j]];
	nonZeros+=LcolLengths_[column];
    }
    //
    LrowSize_=nonZeros;
    int k=0;
    for ( int row=0; row<numberRows_; ++row ){
	LrowStarts_[row]=k;
	k+=LrowLengths_[row];
    }
#ifdef COIN_SIMP_CAPACITY
    //memset(LrowLengths_,0,numberRows_*sizeof(int));
    // fill the rows
    for ( int column=0; column<numberRows_; ++column ){
	const int colBeg=LcolStarts_[column];
	const int colEnd=colBeg+LcolLengths_[column];
	for ( int j=colBeg; j<colEnd; ++j ){
	    const int row=LcolInd_[j];
	    const int indx=LrowStarts_[row]++;
	    Lrows_[indx]=Lcolumns_[j];
	    LrowInd_[indx]=column;
	}
    }
    // Put back starts
    k=0;
    for ( int row=0; row<numberRows_; ++row ){
      int next = LrowStarts_[row];
      LrowStarts_[row]=k;
      k=next;
    }
#else
    memset(LrowLengths_,0,numberRows_*sizeof(int));
    // fill the rows
    for ( int column=0; column<numberRows_; ++column ){
	const int colBeg=LcolStarts_[column];
	const int colEnd=colBeg+LcolLengths_[column];
	for ( int j=colBeg; j<colEnd; ++j ){
	    const int row=LcolInd_[j];
	    const int indx=LrowStarts_[row] + LrowLengths_[row];
	    Lrows_[indx]=Lcolumns_[j];
	    LrowInd_[indx]=column;
	    ++LrowLengths_[row];
	}
    }
#endif
}


int CoinSimpFactorization::findInRow(const int row,
				    const int column)
{
    const int rowBeg=UrowStarts_[row];
    const int rowEnd=rowBeg+UrowLengths_[row];
    int columnIndx=-1;
    for ( int i=rowBeg; i<rowEnd; ++i ){
	if ( UrowInd_[i]==column ){
	    columnIndx=i;
	    break;
	}
    }
    return columnIndx;
}
int CoinSimpFactorization::findInColumn(const int column,
				       const int row)
{
    int indxRow=-1;
    const int colBeg=UcolStarts_[column];
    const int colEnd=colBeg+UcolLengths_[column];
    for ( int i=colBeg; i<colEnd; ++i ){
	if (UcolInd_[i]==row){
	    indxRow=i;
	    break;
	}
    }
    return indxRow;
}
void CoinSimpFactorization::removeRowFromActSet(const int row,
					       FactorPointers &pointers)
{
    int *firstRowKnonzeros=pointers.firstRowKnonzeros;
    int *prevRow=pointers.prevRow;
    int *nextRow=pointers.nextRow;
    if ( prevRow[row]==-1 )
	firstRowKnonzeros[UrowLengths_[row]]=nextRow[row];
    else nextRow[prevRow[row]]=nextRow[row];
    if ( nextRow[row] != -1)
	prevRow[nextRow[row]]=prevRow[row];
}
void CoinSimpFactorization::removeColumnFromActSet(const int column,
						  FactorPointers &pointers)
{
    int *firstColKnonzeros=pointers.firstColKnonzeros;
    int *prevColumn=pointers.prevColumn;
    int *nextColumn=pointers.nextColumn;
    if ( prevColumn[column]==-1 )
	firstColKnonzeros[UcolLengths_[column]]=nextColumn[column];
    else nextColumn[prevColumn[column]]=nextColumn[column];
    if ( nextColumn[column] != -1 )
	prevColumn[nextColumn[column]]=prevColumn[column];
}
void CoinSimpFactorization::allocateSomeArrays()
{
    if (denseVector_) delete [] denseVector_;
    denseVector_ = new double[numberRows_];
    memset(denseVector_,0,numberRows_*sizeof(double));
    if(workArea2_) delete [] workArea2_;
    workArea2_ = new double[numberRows_];
    if(workArea3_) delete [] workArea3_;
    workArea3_ = new double[numberRows_];

    if(vecLabels_) delete [] vecLabels_;
    vecLabels_ = new int[numberRows_];
    memset(vecLabels_,0, numberRows_*sizeof(int));
    if(indVector_) delete [] indVector_;
    indVector_ = new int[numberRows_];

    if (auxVector_) delete [] auxVector_;
    auxVector_ = new double[numberRows_];
    if (auxInd_) delete [] auxInd_;
    auxInd_ = new int[numberRows_];

    if (vecKeep_) delete [] vecKeep_;
    vecKeep_ = new double[numberRows_];
    if (indKeep_) delete [] indKeep_;
    indKeep_ = new int[numberRows_];

    if (LrowStarts_) delete [] LrowStarts_;
    LrowStarts_ = new int[numberRows_];
    if (LrowLengths_) delete [] LrowLengths_;
    LrowLengths_ = new int[numberRows_];

    LrowCap_=(numberRows_*(numberRows_-1))/2;
    if (Lrows_) delete [] Lrows_;
    Lrows_ = new double[LrowCap_];
    if (LrowInd_) delete [] LrowInd_;
    LrowInd_ = new int[LrowCap_];

    if (LcolStarts_) delete [] LcolStarts_;
    LcolStarts_ = new int[numberRows_];
    if (LcolLengths_) delete [] LcolLengths_;
    LcolLengths_ = new int[numberRows_];
    LcolCap_=LrowCap_;
    if (Lcolumns_) delete [] Lcolumns_;
    Lcolumns_ = new double[LcolCap_];
    if (LcolInd_) delete [] LcolInd_;
    LcolInd_ = new int[LcolCap_];

    if (UrowStarts_) delete [] UrowStarts_;
    UrowStarts_ = new int[numberRows_];
    if (UrowLengths_) delete [] UrowLengths_;
    UrowLengths_ = new int[numberRows_];
#ifdef COIN_SIMP_CAPACITY
    if (UrowCapacities_) delete [] UrowCapacities_;
    UrowCapacities_ = new int[numberRows_];
#endif

    minIncrease_=10;
    UrowMaxCap_=numberRows_*(numberRows_+minIncrease_);
    if (Urows_) delete [] Urows_;
    Urows_ = new double[UrowMaxCap_];
    if (UrowInd_) delete [] UrowInd_;
    UrowInd_ = new int[UrowMaxCap_];

    if (prevRowInU_) delete [] prevRowInU_;
    prevRowInU_ = new int[numberRows_];
    if (nextRowInU_) delete [] nextRowInU_;
    nextRowInU_ = new int[numberRows_];

    if (UcolStarts_) delete [] UcolStarts_;
    UcolStarts_ = new int[numberRows_];
    if (UcolLengths_) delete [] UcolLengths_;
    UcolLengths_ = new int[numberRows_];
#ifdef COIN_SIMP_CAPACITY
    if (UcolCapacities_) delete [] UcolCapacities_;
    UcolCapacities_ = new int[numberRows_];
#endif

    UcolMaxCap_=UrowMaxCap_;
    if (Ucolumns_) delete [] Ucolumns_;
    Ucolumns_ = new double[UcolMaxCap_];
    if (UcolInd_) delete [] UcolInd_;
    UcolInd_ = new int[UcolMaxCap_];

    if (prevColInU_) delete [] prevColInU_;
    prevColInU_ = new int[numberRows_];
    if (nextColInU_) delete [] nextColInU_;
    nextColInU_ = new int[numberRows_];
    if (colSlack_) delete [] colSlack_;
    colSlack_ = new int[numberRows_];

    if (invOfPivots_) delete [] invOfPivots_;
    invOfPivots_ = new double[numberRows_];

    if (colOfU_) delete [] colOfU_;
    colOfU_ = new int[numberRows_];
    if (colPosition_) delete [] colPosition_;
    colPosition_ = new int[numberRows_];
    if (rowOfU_) delete [] rowOfU_;
    rowOfU_ = new int[numberRows_];
    if (rowPosition_) delete [] rowPosition_;
    rowPosition_ = new int[numberRows_];
    if (secRowOfU_) delete [] secRowOfU_;
    secRowOfU_ = new int[numberRows_];
    if (secRowPosition_) delete [] secRowPosition_;
    secRowPosition_ = new int[numberRows_];

    if (EtaPosition_) delete [] EtaPosition_;
    EtaPosition_ = new int[maximumPivots_];
    if (EtaStarts_) delete [] EtaStarts_;
    EtaStarts_ = new int[maximumPivots_];
    if (EtaLengths_) delete [] EtaLengths_;
    EtaLengths_ = new int[maximumPivots_];
    maxEtaRows_=maximumPivots_;

    EtaMaxCap_=maximumPivots_*minIncrease_;
    if (EtaInd_) delete [] EtaInd_;
    EtaInd_ = new int[EtaMaxCap_];
    if (Eta_) delete [] Eta_;
    Eta_ = new double[EtaMaxCap_];

}

void CoinSimpFactorization::Lxeqb(double *b) const
{
    double *rhs=b;
    int k, colBeg, *ind, *indEnd;
    double xk, *Lcol;
    // now solve
    for ( int j=firstNumberSlacks_; j<numberRows_; ++j ){
	k=rowOfU_[j];
	xk=rhs[k];
	if ( xk!=0.0 ) {
	    //if ( fabs(xk)>zeroTolerance_ ) {
	    colBeg=LcolStarts_[k];
	    ind=LcolInd_+colBeg;
	    indEnd=ind+LcolLengths_[k];
	    Lcol=Lcolumns_+colBeg;
	    for ( ; ind!=indEnd; ++ind ){
		rhs[ *ind ]-= (*Lcol) * xk;
		++Lcol;
	    }
	}
    }
}



void CoinSimpFactorization::Lxeqb2(double *b1, double *b2) const
{
    double *rhs1=b1;
    double *rhs2=b2;
    double x1, x2, *Lcol;
    int k, colBeg, *ind, *indEnd, j;
    // now solve
    for ( j=firstNumberSlacks_; j<numberRows_; ++j ){
	k=rowOfU_[j];
	x1=rhs1[k];
	x2=rhs2[k];
	if ( x1 == 0.0 ) {
	    if (x2 == 0.0 ) {
	    } else {
		colBeg=LcolStarts_[k];
		ind=LcolInd_+colBeg;
		indEnd=ind+LcolLengths_[k];
		Lcol=Lcolumns_+colBeg;
		for ( ; ind!=indEnd; ++ind ){
#if 0
		    rhs2[ *ind ]-= (*Lcol) * x2;
#else
		    double value=rhs2[ *ind ];
		    rhs2[ *ind ]= value -(*Lcol) * x2;
#endif
		    ++Lcol;
		}
	    }
	} else {
	    if ( x2 == 0.0 ) {
		colBeg=LcolStarts_[k];
		ind=LcolInd_+colBeg;
		indEnd=ind+LcolLengths_[k];
		Lcol=Lcolumns_+colBeg;
		for ( ; ind!=indEnd; ++ind ){
#if 0
		    rhs1[ *ind ]-= (*Lcol) * x1;
#else
		    double value=rhs1[ *ind ];
		    rhs1[ *ind ]= value -(*Lcol) * x1;
#endif
		    ++Lcol;
		}
	    } else {
		colBeg=LcolStarts_[k];
		ind=LcolInd_+colBeg;
		indEnd=ind+LcolLengths_[k];
		Lcol=Lcolumns_+colBeg;
		for ( ; ind!=indEnd; ++ind ){
#if 0
		    rhs1[ *ind ]-= (*Lcol) * x1;
		    rhs2[ *ind ]-= (*Lcol) * x2;
#else
		    double value1=rhs1[ *ind ];
		    rhs1[ *ind ]= value1 - (*Lcol) * x1;
		    double value2=rhs2[ *ind ];
		    rhs2[ *ind ]= value2 - (*Lcol) * x2;
#endif
		    ++Lcol;
		}
	    }
	}
    }
}

void CoinSimpFactorization::Uxeqb(double *b, double *sol) const
{
    double *rhs=b;
    int row, column, colBeg, *ind, *indEnd, k;
    double x, *uCol;
    // now solve
    for ( k=numberRows_-1; k>=numberSlacks_; --k ){
	row=secRowOfU_[k];
	x=rhs[row];
	column=colOfU_[k];
	if ( x!=0.0 ) {
	    //if ( fabs(x) > zeroTolerance_ ) {
	    x*=invOfPivots_[row];
	    colBeg=UcolStarts_[column];
	    ind=UcolInd_+colBeg;
	    indEnd=ind+UcolLengths_[column];
	    uCol=Ucolumns_+colBeg;
	    for ( ; ind!=indEnd; ++ind ){
#if 0
		rhs[ *ind ]-= (*uCol) * x;
#else
		double value=rhs[ *ind ];
		rhs[ *ind ] = value - (*uCol) * x;
#endif
		++uCol;
	    }
	    sol[column]=x;
	}
	else sol[column]=0.0;
    }
    for ( k=numberSlacks_-1; k>=0; --k ){
	row=secRowOfU_[k];
	column=colOfU_[k];
	sol[column]=-rhs[row];
    }
}



void CoinSimpFactorization::Uxeqb2(double *b1, double *sol1, double *b2, double *sol2) const
{
    double *rhs1=b1;
    double *rhs2=b2;
    int row, column, colBeg, *ind, *indEnd;
    double x1, x2, *uCol;
    // now solve
    for ( int k=numberRows_-1; k>=numberSlacks_; --k ){
	row=secRowOfU_[k];
	x1=rhs1[row];
	x2=rhs2[row];
	column=colOfU_[k];
	if (x1 == 0.0) {
	    if (x2 == 0.0) {
		sol1[column]=0.0;
		sol2[column]=0.0;
	    } else {
		x2*=invOfPivots_[row];
		colBeg=UcolStarts_[column];
		ind=UcolInd_+colBeg;
		indEnd=ind+UcolLengths_[column];
		uCol=Ucolumns_+colBeg;
		for ( ; ind!=indEnd; ++ind ){
#if 0
		    rhs2[ *ind ]-= (*uCol) * x2;
#else
		    double value=rhs2[ *ind ];
		    rhs2[ *ind ]= value - (*uCol) * x2;
#endif
		    ++uCol;
		}
		sol1[column]=0.0;
		sol2[column]=x2;
	    }
	} else {
	    if (x2 == 0.0) {
		x1*=invOfPivots_[row];
		colBeg=UcolStarts_[column];
		ind=UcolInd_+colBeg;
		indEnd=ind+UcolLengths_[column];
		uCol=Ucolumns_+colBeg;
		for ( ; ind!=indEnd; ++ind ){
#if 0
		    rhs1[ *ind ]-= (*uCol) * x1;
#else
		    double value=rhs1[ *ind ];
		    rhs1[ *ind ] = value - (*uCol) * x1;
#endif
		    ++uCol;
		}
		sol1[column]=x1;
		sol2[column]=0.0;
	    } else {
		x1*=invOfPivots_[row];
		x2*=invOfPivots_[row];
		colBeg=UcolStarts_[column];
		ind=UcolInd_+colBeg;
		indEnd=ind+UcolLengths_[column];
		uCol=Ucolumns_+colBeg;
		for ( ; ind!=indEnd; ++ind ){
#if 0
		    rhs1[ *ind ]-= (*uCol) * x1;
		    rhs2[ *ind ]-= (*uCol) * x2;
#else
		    double value1=rhs1[ *ind ];
		    rhs1[ *ind ] = value1 - (*uCol) * x1;
		    double value2=rhs2[ *ind ];
		    rhs2[ *ind ] = value2 - (*uCol) * x2;
#endif
		    ++uCol;
		}
		sol1[column]=x1;
		sol2[column]=x2;
	    }
	}
    }
    for ( int k=numberSlacks_-1; k>=0; --k ){
	row=secRowOfU_[k];
	column=colOfU_[k];
	sol1[column]=-rhs1[row];
	sol2[column]=-rhs2[row];
    }
}


void CoinSimpFactorization::xLeqb(double *b) const
{
    double *rhs=b;
    int k, *ind, *indEnd, j;
    int colBeg;
    double x, *Lcol;
    // find last nonzero
    int last;
    for ( last=numberColumns_-1; last >= 0; --last ){
	if ( rhs[ rowOfU_[last] ] ) break;
    }
    // this seems to be faster
    if ( last >= 0 ){
	for ( j=last; j >=firstNumberSlacks_ ; --j ){
	    k=rowOfU_[j];
	    x=rhs[k];
	    colBeg=LcolStarts_[k];
	    ind=LcolInd_+colBeg;
	    indEnd=ind+LcolLengths_[k];
	    Lcol=Lcolumns_+colBeg;
	    for ( ; ind!=indEnd; ++ind ){
		x -= (*Lcol) * rhs[ *ind ];
		++Lcol;
	    }
	    rhs[k]=x;
	}
    } // if ( last >= 0 ){
}



void CoinSimpFactorization::xUeqb(double *b, double *sol) const
{
    double *rhs=b;
    int row, col, *ind, *indEnd, k;
    double xr;
    // now solve
#if 1
    int rowBeg;
    double * uRow;
    for ( k=0; k<numberSlacks_; ++k ){
	row=secRowOfU_[k];
	col=colOfU_[k];
	xr=rhs[col];
	if ( xr!=0.0 ) {
	    //if ( fabs(xr)> zeroTolerance_ ) {
	    xr=-xr;
	    rowBeg=UrowStarts_[row];
	    ind=UrowInd_+rowBeg;
	    indEnd=ind+UrowLengths_[row];
	    uRow=Urows_+rowBeg;
	    for ( ; ind!=indEnd; ++ind ){
		rhs[ *ind ]-= (*uRow) * xr;
		++uRow;
	    }
	    sol[row]=xr;
	}
	else sol[row]=0.0;
    }
    for ( k=numberSlacks_; k<numberRows_; ++k ){
	row=secRowOfU_[k];
	col=colOfU_[k];
	xr=rhs[col];
	if ( xr!=0.0 ) {
	    //if ( fabs(xr)> zeroTolerance_ ) {
	    xr*=invOfPivots_[row];
	    rowBeg=UrowStarts_[row];
	    ind=UrowInd_+rowBeg;
	    indEnd=ind+UrowLengths_[row];
	    uRow=Urows_+rowBeg;
	    for ( ; ind!=indEnd; ++ind ){
		rhs[ *ind ]-= (*uRow) * xr;
		++uRow;
	    }
	    sol[row]=xr;
	}
	else sol[row]=0.0;
    }
#else
    for ( k=0; k<numberSlacks_; ++k ){
	row=secRowOfU_[k];
	col=colOfU_[k];
	sol[row]=-rhs[col];
    }
    for ( k=numberSlacks_; k<numberRows_; ++k ){
	row=secRowOfU_[k];
	col=colOfU_[k];
	xr=rhs[col];
	int colBeg=UcolStarts_[col];
	ind=UcolInd_+colBeg;
	indEnd=ind+UcolLengths_[col];
	double * uCol=Ucolumns_+colBeg;
	for ( ; ind!=indEnd; ++ind,++uCol ){
	  int iRow = *ind;
	  double value = sol[iRow];
	  double elementValue = *uCol;
	  xr -= value*elementValue;
	}
	if ( xr!=0.0 ) {
	  xr*=invOfPivots_[row];
	  sol[row]=xr;
	}
	else sol[row]=0.0;
    }
#endif
}



int CoinSimpFactorization::LUupdate(int newBasicCol)
{
    //checkU();
    // recover vector kept in ftran
    double *newColumn=vecKeep_;
    int *indNewColumn=indKeep_;
    int sizeNewColumn=keepSize_;

    // remove elements of new column of U
    const int colBeg=UcolStarts_[newBasicCol];
    const int colEnd=colBeg+UcolLengths_[newBasicCol];
    for ( int i=colBeg; i<colEnd; ++i ){
	const int row=UcolInd_[i];
	const int colInRow=findInRow(row,newBasicCol);
	assert(colInRow >= 0);
	// remove from row
	const int rowEnd=UrowStarts_[row]+UrowLengths_[row];
	Urows_[colInRow]=Urows_[rowEnd-1];
	UrowInd_[colInRow]=UrowInd_[rowEnd-1];
	--UrowLengths_[row];
    }
    UcolLengths_[newBasicCol]=0;
    // now add new column to U
    int lastRowInU=-1;
    for ( int i=0; i < sizeNewColumn; ++i ){
	//if ( fabs(newColumn[i]) < zeroTolerance_ ) continue;
	const int row=indNewColumn[i];
	// add to row
#ifdef COIN_SIMP_CAPACITY
	if ( UrowLengths_[row] + 1 > UrowCapacities_[row] )
	    increaseRowSize(row, UrowLengths_[row] + 1);
#endif
	const int rowEnd=UrowStarts_[row]+UrowLengths_[row];
	UrowInd_[rowEnd]=newBasicCol;
	Urows_[rowEnd]=newColumn[i];
	++UrowLengths_[row];
	if ( lastRowInU < secRowPosition_[row] ) lastRowInU=secRowPosition_[row];
    }
    // add to Ucolumns
#ifdef COIN_SIMP_CAPACITY
    if ( sizeNewColumn > UcolCapacities_[newBasicCol] )
	increaseColSize(newBasicCol, sizeNewColumn , true);
#endif
    memcpy(&Ucolumns_[ UcolStarts_[newBasicCol] ], &newColumn[0],
	   sizeNewColumn * sizeof(double) );
    memcpy(&UcolInd_[ UcolStarts_[newBasicCol] ], &indNewColumn[0],
	   sizeNewColumn * sizeof(int) );
    UcolLengths_[newBasicCol]=sizeNewColumn;


    const int posNewCol=colPosition_[newBasicCol];
    if ( lastRowInU < posNewCol ){
	// matrix is singular
	return 1;
    }
    // permutations
    const int rowInU=secRowOfU_[posNewCol];
    const int colInU=colOfU_[posNewCol];
    for ( int i=posNewCol; i<lastRowInU; ++i ){
	int indx=secRowOfU_[i+1];
	secRowOfU_[i]=indx;
	secRowPosition_[indx]=i;
	int jndx=colOfU_[i+1];
	colOfU_[i]=jndx;
	colPosition_[jndx]=i;
    }
    secRowOfU_[lastRowInU]=rowInU;
    secRowPosition_[rowInU]=lastRowInU;
    colOfU_[lastRowInU]=colInU;
    colPosition_[colInU]=lastRowInU;
    if ( posNewCol < numberSlacks_ ){
	if ( lastRowInU >= numberSlacks_ )
	    --numberSlacks_;
	else
	    numberSlacks_= lastRowInU;
    }
    // rowInU will be transformed
    // denseVector_ is assumed to be initialized to zero
    const int rowBeg=UrowStarts_[rowInU];
    const int rowEnd=rowBeg+UrowLengths_[rowInU];
    for ( int i=rowBeg; i<rowEnd; ++i ){
	const int column=UrowInd_[i];
	denseVector_[column]=Urows_[i];
	// remove element
	const int indxRow=findInColumn(column,rowInU);
	assert( indxRow >= 0 );
	const int colEnd=UcolStarts_[column]+UcolLengths_[column];
	UcolInd_[indxRow]=UcolInd_[colEnd-1];
	Ucolumns_[indxRow]=Ucolumns_[colEnd-1];
	--UcolLengths_[column];
    }
    UrowLengths_[rowInU]=0;
    // rowInU is empty
    // increase Eta by (lastRowInU-posNewCol) elements
    newEta(rowInU, lastRowInU-posNewCol );
    assert(!EtaLengths_[lastEtaRow_]);
    int saveSize = EtaSize_;;
    for ( int i=posNewCol; i<lastRowInU; ++i ){
	const int row=secRowOfU_[i];
	const int column=colOfU_[i];
	if ( denseVector_[column]==0.0 ) continue;
	const double multiplier=denseVector_[column]*invOfPivots_[row];
	denseVector_[column]=0.0;
	const int rowBeg=UrowStarts_[row];
	const int rowEnd=rowBeg+UrowLengths_[row];
#if ARRAY
	for ( int j=rowBeg; j<rowEnd; ++j ){
	    denseVector_[ UrowInd_[j] ]-= multiplier*Urows_[j];
	}
#else
	int *ind=UrowInd_+rowBeg;
	int *indEnd=UrowInd_+rowEnd;
	double *uRow=Urows_+rowBeg;
	for ( ; ind!=indEnd; ++ind ){
	    denseVector_[ *ind ]-= multiplier * (*uRow);
	    ++uRow;
	}
#endif
	// store multiplier
	Eta_[EtaSize_]=multiplier;
	EtaInd_[EtaSize_++]=row;
    }
    if (EtaSize_!=saveSize)
      EtaLengths_[lastEtaRow_]=EtaSize_ - saveSize;
    else
      --lastEtaRow_;
    // inverse of diagonal
    invOfPivots_[rowInU]=1.0/denseVector_[ colOfU_[lastRowInU] ];
    denseVector_[ colOfU_[lastRowInU] ]=0.0;
    // now store row
    int newEls=0;
    for ( int i=lastRowInU+1; i<numberColumns_; ++i ){
	const int column=colOfU_[i];
	const double coeff=denseVector_[column];
	denseVector_[column]=0.0;
	if ( fabs(coeff) < zeroTolerance_ ) continue;
#ifdef COIN_SIMP_CAPACITY
	if ( UcolLengths_[column] + 1 > UcolCapacities_[column] ){
	    increaseColSize(column, UcolLengths_[column] + 1, true);
	}
#endif
	const int newInd=UcolStarts_[column]+UcolLengths_[column];
	UcolInd_[newInd]=rowInU;
	Ucolumns_[newInd]=coeff;
	++UcolLengths_[column];
	workArea2_[newEls]=coeff;
	indVector_[newEls++]=column;
    }
#ifdef COIN_SIMP_CAPACITY
    if ( UrowCapacities_[rowInU] < newEls )
	increaseRowSize(rowInU, newEls);
#endif
    const int startRow=UrowStarts_[rowInU];
    memcpy(&Urows_[startRow],&workArea2_[0], newEls*sizeof(double) );
    memcpy(&UrowInd_[startRow],&indVector_[0], newEls*sizeof(int) );
    UrowLengths_[rowInU]=newEls;
    //
    if ( fabs( invOfPivots_[rowInU] ) > updateTol_ )
	return 2;

    return 0;
}


void CoinSimpFactorization::newEta(int row, int numNewElements){
    if ( lastEtaRow_ == maxEtaRows_-1 ){
	int *iaux=new int[maxEtaRows_ + minIncrease_];
	memcpy(iaux, EtaPosition_, maxEtaRows_ * sizeof(int));
	delete [] EtaPosition_;
	EtaPosition_=iaux;

	int *jaux=new int[maxEtaRows_ + minIncrease_];
	memcpy(jaux, EtaStarts_, maxEtaRows_ * sizeof(int));
	delete [] EtaStarts_;
	EtaStarts_=jaux;

	int *kaux=new int[maxEtaRows_ + minIncrease_];
	memcpy(kaux, EtaLengths_, maxEtaRows_ * sizeof(int));
	delete [] EtaLengths_;
	EtaLengths_=kaux;

	maxEtaRows_+=minIncrease_;
    }
    if ( EtaSize_ + numNewElements > EtaMaxCap_ ){
	int number= CoinMax(EtaSize_ + numNewElements - EtaMaxCap_, minIncrease_);

	int *iaux=new int[EtaMaxCap_ + number];
	memcpy(iaux, EtaInd_, EtaSize_ * sizeof(int));
	delete [] EtaInd_;
	EtaInd_=iaux;

	double *aux=new double[EtaMaxCap_ + number];
	memcpy(aux, Eta_, EtaSize_ * sizeof(double));
	delete [] Eta_;
	Eta_=aux;

	EtaMaxCap_+=number;
    }
    EtaPosition_[++lastEtaRow_]=row;
    EtaStarts_[lastEtaRow_]=EtaSize_;
    EtaLengths_[lastEtaRow_]=0;

}
void CoinSimpFactorization::copyRowPermutations()
{
    memcpy(&secRowOfU_[0], &rowOfU_[0],
	   numberRows_ * sizeof(int) );
    memcpy(&secRowPosition_[0], &rowPosition_[0],
	   numberRows_ * sizeof(int) );
}

void CoinSimpFactorization::Hxeqb(double *b) const
{
    double *rhs=b;
    int row, rowBeg, *ind, *indEnd;
    double xr, *eta;
    // now solve
    for ( int k=0; k <= lastEtaRow_; ++k ){
	row=EtaPosition_[k];
	rowBeg=EtaStarts_[k];
	xr=0.0;
	ind=EtaInd_+rowBeg;
	indEnd=ind+EtaLengths_[k];
	eta=Eta_+rowBeg;
	for ( ; ind!=indEnd; ++ind ){
	    xr += rhs[ *ind ] * (*eta);
	    ++eta;
	}
	rhs[row]-=xr;
    }
}



void CoinSimpFactorization::Hxeqb2(double *b1, double *b2) const
{
    double *rhs1=b1;
    double *rhs2=b2;
    int row, rowBeg, *ind, *indEnd;
    double x1, x2, *eta;
    // now solve
    for ( int k=0; k <= lastEtaRow_; ++k ){
	row=EtaPosition_[k];
	rowBeg=EtaStarts_[k];
	x1=0.0;
	x2=0.0;
	ind=EtaInd_+rowBeg;
	indEnd=ind+EtaLengths_[k];
	eta=Eta_+rowBeg;
	for ( ; ind!=indEnd; ++ind ){
	    x1 += rhs1[ *ind ] * (*eta);
	    x2 += rhs2[ *ind ] * (*eta);
	    ++eta;
	}
	rhs1[row]-=x1;
	rhs2[row]-=x2;
    }
}




void CoinSimpFactorization::xHeqb(double *b) const
{
    double *rhs=b;
    int row, rowBeg, *ind, *indEnd;
    double xr, *eta;
    // now solve
    for ( int k=lastEtaRow_; k >= 0; --k ){
	row=EtaPosition_[k];
	xr=rhs[row];
	if ( xr==0.0 ) continue;
	//if ( fabs(xr) <= zeroTolerance_ ) continue;
	rowBeg=EtaStarts_[k];
	ind=EtaInd_+rowBeg;
	indEnd=ind+EtaLengths_[k];
	eta=Eta_+rowBeg;
	for ( ; ind!=indEnd; ++ind ){
	    rhs[ *ind ]-= xr * (*eta);
	    ++eta;
	}
    }
}



void CoinSimpFactorization::ftran(double *b, double *sol, bool save) const
{
    Lxeqb(b);
    Hxeqb(b);
    if ( save ){
	// keep vector
	keepSize_=0;
	for ( int i=0; i<numberRows_; ++i ){
	    if ( fabs(b[i]) < zeroTolerance_ ) continue;
	    vecKeep_[keepSize_]=b[i];
	    indKeep_[keepSize_++]=i;
	}
    }
    Uxeqb(b,sol);
}

void CoinSimpFactorization::ftran2(double *b1, double *sol1, double *b2, double *sol2) const
{
    Lxeqb2(b1,b2);
    Hxeqb2(b1,b2);
    // keep vector
    keepSize_=0;
    for ( int i=0; i<numberRows_; ++i ){
	if ( fabs(b1[i]) < zeroTolerance_ ) continue;
	vecKeep_[keepSize_]=b1[i];
	indKeep_[keepSize_++]=i;
    }
    Uxeqb2(b1,sol1,b2,sol2);
}



void CoinSimpFactorization::btran(double *b, double *sol) const
{
    xUeqb(b, sol);
    xHeqb(sol);
    xLeqb(sol);
}

