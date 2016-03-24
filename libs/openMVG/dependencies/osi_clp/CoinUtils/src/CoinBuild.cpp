/* $Id: CoinBuild.cpp 1550 2012-08-28 14:55:18Z forrest $ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cfloat>
#include <cstdio>
#include <iostream>


#include "CoinBuild.hpp"

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"

/*
  Format of each item is a bit sleazy.
  First we have pointer to next item
  Then we have two ints giving item number and number of elements
  Then we have three double for objective lower and upper
  Then we have elements
  Then indices
*/
struct buildFormat {
  buildFormat * next;
  int itemNumber;
  int numberElements;
  double objective;
  double lower;
  double upper;
  double restDouble[1]; 
  int restInt[1]; // just to make correct size 
} ;

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinBuild::CoinBuild () 
  : numberItems_(0),
    numberOther_(0),
    numberElements_(0),
    currentItem_(NULL),
    firstItem_(NULL),
    lastItem_(NULL),
    type_(-1)
{
}
//-------------------------------------------------------------------
// Constructor with type
//-------------------------------------------------------------------
CoinBuild::CoinBuild (int type) 
  : numberItems_(0),
    numberOther_(0),
    numberElements_(0),
    currentItem_(NULL),
    firstItem_(NULL),
    lastItem_(NULL),
    type_(type)
{
  if (type<0||type>1)
    type_=-1; // unset
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinBuild::CoinBuild (const CoinBuild & rhs) 
  : numberItems_(rhs.numberItems_),
    numberOther_(rhs.numberOther_),
    numberElements_(rhs.numberElements_),
    type_(rhs.type_)
{
  if (numberItems_) {
    firstItem_=NULL;
    buildFormat * lastItem = NULL;
    buildFormat * currentItem = reinterpret_cast<buildFormat *> ( rhs.firstItem_);
    for (int iItem=0;iItem<numberItems_;iItem++) {
      buildFormat * item = currentItem;
      assert (item);
      int numberElements = item->numberElements;
      int length = ( CoinSizeofAsInt(buildFormat) + (numberElements-1) *
                     (CoinSizeofAsInt(double)+CoinSizeofAsInt(int)) );
      int doubles = (length + CoinSizeofAsInt(double)-1)/CoinSizeofAsInt(double);
      double * copyOfItem = new double [doubles];
      memcpy(copyOfItem,item,length);
      if (!firstItem_) {
        firstItem_ = copyOfItem;
      } else {
        // update pointer
        lastItem->next = reinterpret_cast<buildFormat *> ( copyOfItem);
      }
      currentItem = currentItem->next; // on to next
      lastItem = reinterpret_cast<buildFormat *> ( copyOfItem);
    }
    currentItem_=firstItem_;
    lastItem_=reinterpret_cast<double *> ( lastItem);
  } else {
    currentItem_=NULL;
    firstItem_=NULL;
    lastItem_=NULL;
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinBuild::~CoinBuild ()
{
  buildFormat * item = reinterpret_cast<buildFormat *> ( firstItem_);
  for (int iItem=0;iItem<numberItems_;iItem++) {
    double * array = reinterpret_cast<double *> ( item);
    item = item->next;
    delete [] array;
  }
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinBuild &
CoinBuild::operator=(const CoinBuild& rhs)
{
  if (this != &rhs) {
    buildFormat * item = reinterpret_cast<buildFormat *> ( firstItem_);
    for (int iItem=0;iItem<numberItems_;iItem++) {
      double * array = reinterpret_cast<double *> ( item);
      item = item->next;
      delete [] array;
    }
    numberItems_=rhs.numberItems_;
    numberOther_=rhs.numberOther_;
    numberElements_=rhs.numberElements_;
    type_=rhs.type_;
    if (numberItems_) {
      firstItem_=NULL;
      buildFormat * lastItem = NULL;
      buildFormat * currentItem = reinterpret_cast<buildFormat *> ( rhs.firstItem_);
      for (int iItem=0;iItem<numberItems_;iItem++) {
        buildFormat * item = currentItem;
        assert (item);
        int numberElements = item->numberElements;
        int length = CoinSizeofAsInt(buildFormat)+(numberElements-1)*(CoinSizeofAsInt(double)+CoinSizeofAsInt(int));
        int doubles = (length + CoinSizeofAsInt(double)-1)/CoinSizeofAsInt(double);
        double * copyOfItem = new double [doubles];
        memcpy(copyOfItem,item,length);
        if (!firstItem_) {
          firstItem_ = copyOfItem;
        } else {
          // update pointer
          lastItem->next = reinterpret_cast<buildFormat *> ( copyOfItem);
        }
        currentItem = currentItem->next; // on to next
        lastItem = reinterpret_cast<buildFormat *> ( copyOfItem);
      }
      currentItem_=firstItem_;
      lastItem_=reinterpret_cast<double *> ( lastItem);
    } else {
      currentItem_=NULL;
      firstItem_=NULL;
      lastItem_=NULL;
    }
  }
  return *this;
}
// add a row
void 
CoinBuild::addRow(int numberInRow, const int * columns,
                 const double * elements, double rowLower, 
                 double rowUpper)
{
  if (type_<0) {
    type_=0;
  } else if (type_==1) {
    printf("CoinBuild:: unable to add a row in column mode\n");
    abort();
  }
  if (numberInRow<0)
    printf("bad number %d\n",numberInRow); // to stop compiler error
  addItem(numberInRow, columns, elements, 
          rowLower,rowUpper,0.0);
  if (numberInRow<0)
    printf("bad number %d\n",numberInRow); // to stop compiler error
}
/*  Returns number of elements in a row and information in row
 */
int 
CoinBuild::row(int whichRow, double & rowLower, double & rowUpper,
              const int * & indices, const double * & elements) const
{
  assert (type_==0);
  setMutableCurrent(whichRow);
  double dummyObjective;
  return currentItem(rowLower,rowUpper,dummyObjective,indices,elements);
}
/*  Returns number of elements in current row and information in row
    Used as rows may be stored in a chain
*/
int 
CoinBuild::currentRow(double & rowLower, double & rowUpper,
                     const int * & indices, const double * & elements) const
{
  assert (type_==0);
  double dummyObjective;
  return currentItem(rowLower,rowUpper,dummyObjective,indices,elements);
}
// Set current row
void 
CoinBuild::setCurrentRow(int whichRow)
{
  assert (type_==0);
  setMutableCurrent(whichRow);
}
// Returns current row number
int 
CoinBuild::currentRow() const
{
  assert (type_==0);
  return currentItem();
}
// add a column
void 
CoinBuild::addColumn(int numberInColumn, const int * rows,
                     const double * elements, 
                     double columnLower, 
                     double columnUpper, double objectiveValue)
{
  if (type_<0) {
    type_=1;
  } else if (type_==0) {
    printf("CoinBuild:: unable to add a column in row mode\n");
    abort();
  }
  addItem(numberInColumn, rows, elements,
          columnLower,columnUpper, objectiveValue);
}
/*  Returns number of elements in a column and information in column
 */
int 
CoinBuild::column(int whichColumn,
                  double & columnLower, double & columnUpper, double & objectiveValue, 
                  const int * & indices, const double * & elements) const
{
  assert (type_==1);
  setMutableCurrent(whichColumn);
  return currentItem(columnLower,columnUpper,objectiveValue,indices,elements);
}
/*  Returns number of elements in current column and information in column
    Used as columns may be stored in a chain
*/
int 
CoinBuild::currentColumn( double & columnLower, double & columnUpper, double & objectiveValue, 
                         const int * & indices, const double * & elements) const
{
  assert (type_==1);
  return currentItem(columnLower,columnUpper,objectiveValue,indices,elements);
}
// Set current column
void 
CoinBuild::setCurrentColumn(int whichColumn)
{
  assert (type_==1);
  setMutableCurrent(whichColumn);
}
// Returns current column number
int 
CoinBuild::currentColumn() const
{
  assert (type_==1);
  return currentItem();
}
// add a item
void 
CoinBuild::addItem(int numberInItem, const int * indices,
                  const double * elements, 
                  double itemLower, 
                  double itemUpper, double objectiveValue)
{
  buildFormat * lastItem = reinterpret_cast<buildFormat *> ( lastItem_);
  int length = CoinSizeofAsInt(buildFormat)+(numberInItem-1)*(CoinSizeofAsInt(double)+CoinSizeofAsInt(int));
  int doubles = (length + CoinSizeofAsInt(double)-1)/CoinSizeofAsInt(double);
  double * newItem = new double [doubles];
  if (!firstItem_) {
    firstItem_ = newItem;
  } else {
    // update pointer
    lastItem->next = reinterpret_cast<buildFormat *> ( newItem);
  }
  lastItem_=newItem;
  currentItem_=newItem;
  // now fill in
  buildFormat * item = reinterpret_cast<buildFormat *> ( newItem);
  double * els = &item->restDouble[0];
  int * cols = reinterpret_cast<int *> (els+numberInItem);
  item->next=NULL;
  item->itemNumber=numberItems_;
  numberItems_++;
  item->numberElements=numberInItem;
  numberElements_ += numberInItem;
  item->objective=objectiveValue;
  item->lower=itemLower;
  item->upper=itemUpper;
  for (int k=0;k<numberInItem;k++) {
    int iColumn = indices[k];
    assert (iColumn>=0);
    if (iColumn<0) {
      printf("bad col %d\n",iColumn); // to stop compiler error
      abort();
    }
    if (iColumn>=numberOther_)
      numberOther_ = iColumn+1;
    els[k]=elements[k];
    cols[k]=iColumn;
  }
  return;
}
/*  Returns number of elements in a item and information in item
 */
int 
CoinBuild::item(int whichItem, 
                double & itemLower, double & itemUpper, double & objectiveValue, 
                const int * & indices, const double * & elements) const
{
  setMutableCurrent(whichItem);
  return currentItem(itemLower,itemUpper,objectiveValue,indices,elements);
}
/*  Returns number of elements in current item and information in item
    Used as items may be stored in a chain
*/
int 
CoinBuild::currentItem(double & itemLower, double & itemUpper,
                       double & objectiveValue, 
                       const int * & indices, const double * & elements) const
{
  buildFormat * item = reinterpret_cast<buildFormat *> ( currentItem_);
  if (item) {
    int numberElements = item->numberElements;
    elements = &item->restDouble[0];
    indices = reinterpret_cast<const int *> (elements+numberElements);
    objectiveValue=item->objective;
    itemLower = item->lower;
    itemUpper=item->upper;
    return numberElements;
  } else {
    return -1;
  }
}
// Set current item
void 
CoinBuild::setCurrentItem(int whichItem)
{
  setMutableCurrent(whichItem);
}
// Set current item
void 
CoinBuild::setMutableCurrent(int whichItem) const
{
  if (whichItem>=0&&whichItem<numberItems_) {
    int nSkip = whichItem-1;
    buildFormat * item = reinterpret_cast<buildFormat *> ( firstItem_);
    // if further on then we can start from where we are
    buildFormat * current = reinterpret_cast<buildFormat *> ( currentItem_);
    if (current->itemNumber<=whichItem) {
      item=current;
      nSkip = whichItem-current->itemNumber;
    }
    for (int iItem=0;iItem<nSkip;iItem++) {
      item = item->next;
    }
    assert (whichItem==item->itemNumber);
    currentItem_ = reinterpret_cast<double *> ( item);
  }
}
// Returns current item number
int 
CoinBuild::currentItem() const
{
  buildFormat * item = reinterpret_cast<buildFormat *> ( currentItem_);
  if (item)
    return item->itemNumber;
  else
    return -1;
}
