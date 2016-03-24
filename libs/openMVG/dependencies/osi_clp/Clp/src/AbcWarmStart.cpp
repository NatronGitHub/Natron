/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "CoinUtilsConfig.h"
#include <cassert>

#include "AbcWarmStart.hpp"
#include "CoinHelperFunctions.hpp"
#include <cmath>
#include <iostream>

//#############################################################################

void 
AbcWarmStart::setSize(int ns, int na) 
{
  CoinWarmStartBasis::setSize(ns,na);
  delete [] extraInformation_;
  extraInformation_=NULL;
  typeExtraInformation_=0;
}

void 
AbcWarmStart::assignBasisStatus(int ns, int na, char*& sStat, 
				char*& aStat) 
{
  CoinWarmStartBasis::assignBasisStatus(ns,na,sStat,aStat);
  delete [] extraInformation_;
  extraInformation_=NULL;
  typeExtraInformation_=0;
}
AbcWarmStart::AbcWarmStart(AbcSimplex * model,int type) :
  CoinWarmStartBasis(model->numberColumns(),model->numberRows(),
#ifdef CLP_WARMSTART
		     reinterpret_cast<const char *>(model->statusArray()),
		     reinterpret_cast<const char *>(model->statusArray()+model->numberColumns())
#else
		     reinterpret_cast<const char *>(model->statusArray()+model->maximumAbcNumberRows()),
                     reinterpret_cast<const char *>(model->statusArray())
#endif
		     ),
  typeExtraInformation_(type),
  lengthExtraInformation_(0),
  extraInformation_(NULL),
  model_(model),
  organizer_(NULL),
  previousBasis_(NULL),
  nextBasis_(NULL),
  stamp_(-1),
  numberValidRows_(0)
{
  assert (!typeExtraInformation_);
}

AbcWarmStart::AbcWarmStart(const AbcWarmStart& rhs) :
  CoinWarmStartBasis(rhs),
  typeExtraInformation_(rhs.typeExtraInformation_),
  lengthExtraInformation_(rhs.lengthExtraInformation_),
  extraInformation_(NULL),
  model_(rhs.model_),
  organizer_(rhs.organizer_),
  previousBasis_(NULL),
  nextBasis_(NULL),
  stamp_(-1),
  numberValidRows_(0)
{
  if (typeExtraInformation_)
    extraInformation_=CoinCopyOfArray(rhs.extraInformation_,lengthExtraInformation_);
}

AbcWarmStart& 
AbcWarmStart::operator=(const AbcWarmStart& rhs)
{
  if (this != &rhs) {
    CoinWarmStartBasis::operator=(rhs);
    delete [] extraInformation_;
    extraInformation_=NULL;
    typeExtraInformation_ = rhs.typeExtraInformation_;
    lengthExtraInformation_ = rhs.lengthExtraInformation_;
    model_ = rhs.model_;
    organizer_ = rhs.organizer_;
    previousBasis_ = NULL;
    nextBasis_ = NULL;
    stamp_ = -1;
    numberValidRows_ = 0;
    if (typeExtraInformation_)
      extraInformation_=CoinCopyOfArray(rhs.extraInformation_,lengthExtraInformation_);
  }
  return *this;
}

// Resizes 
void 
AbcWarmStart::resize (int newNumberRows, int newNumberColumns)
{
  if (newNumberRows==numArtificial_&&newNumberColumns==numStructural_)
    return;
  CoinWarmStartBasis::resize(newNumberRows,newNumberColumns);
  delete [] extraInformation_;
  extraInformation_=NULL;
  typeExtraInformation_=0;
}

/*
  compressRows takes an ascending list of target indices without duplicates
  and removes them, compressing the artificialStatus_ array in place. It will
  fail spectacularly if the indices are not sorted. Use deleteRows if you
  need to preprocess the target indices to satisfy the conditions.
*/
void AbcWarmStart::compressRows (int tgtCnt, const int *tgts)
{
  if (!tgtCnt)
    return;
  CoinWarmStartBasis::compressRows(tgtCnt,tgts);
  delete [] extraInformation_;
  extraInformation_=NULL;
  typeExtraInformation_=0;
} 

/*
  deleteRows takes an unordered list of target indices with duplicates and
  removes them from the basis. The strategy is to preprocesses the list into
  an ascending list without duplicates, suitable for compressRows.
*/
void 
AbcWarmStart::deleteRows (int rawTgtCnt, const int *rawTgts)
{
  if (rawTgtCnt <= 0) return ;
  CoinWarmStartBasis::deleteRows(rawTgtCnt,rawTgts);
  delete [] extraInformation_;
  extraInformation_=NULL;
  typeExtraInformation_=0;
}
// Deletes columns
void 
AbcWarmStart::deleteColumns(int number, const int * which)
{
  CoinWarmStartBasis::deleteColumns(number,which);
  delete [] extraInformation_;
  extraInformation_=NULL;
  typeExtraInformation_=0;
}
AbcWarmStart::AbcWarmStart() :
  CoinWarmStartBasis(),
  typeExtraInformation_(0),
  lengthExtraInformation_(0),
  extraInformation_(NULL),
  model_(NULL),
  organizer_(NULL),
  previousBasis_(NULL),
  nextBasis_(NULL),
  stamp_(-1),
  numberValidRows_(0)
{
}
AbcWarmStart::~AbcWarmStart()
{
  delete[] extraInformation_;
}
