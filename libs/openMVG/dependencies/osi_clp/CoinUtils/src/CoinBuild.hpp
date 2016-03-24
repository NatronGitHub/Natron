/* $Id: CoinBuild.hpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinBuild_H
#define CoinBuild_H


#include "CoinPragma.hpp"
#include "CoinTypes.hpp"
#include "CoinFinite.hpp"


/** 
    In many cases it is natural to build a model by adding one row at a time.  In Coin this
    is inefficient so this class gives some help.  An instance of CoinBuild can be built up
    more efficiently and then added to the Clp/OsiModel in one go.

    It may be more efficient to have fewer arrays and re-allocate them but this should
    give a large gain over addRow.

    I have now extended it to columns.

*/

class CoinBuild {
  
public:
  /**@name Useful methods */
   //@{
   /// add a row
   void addRow(int numberInRow, const int * columns,
	       const double * elements, double rowLower=-COIN_DBL_MAX, 
              double rowUpper=COIN_DBL_MAX);
   /// add a column
   void addColumn(int numberInColumn, const int * rows,
                  const double * elements, 
                  double columnLower=0.0, 
                  double columnUpper=COIN_DBL_MAX, double objectiveValue=0.0);
   /// add a column
   inline void addCol(int numberInColumn, const int * rows,
                  const double * elements, 
                  double columnLower=0.0, 
                  double columnUpper=COIN_DBL_MAX, double objectiveValue=0.0)
  { addColumn(numberInColumn, rows, elements, columnLower, columnUpper, objectiveValue);}
   /// Return number of rows or maximum found so far
  inline int numberRows() const
  { return (type_==0) ? numberItems_ : numberOther_;}
   /// Return number of columns or maximum found so far
  inline int numberColumns() const
  { return (type_==1) ? numberItems_ : numberOther_;}
   /// Return number of elements
  inline CoinBigIndex numberElements() const
  { return numberElements_;}
  /**  Returns number of elements in a row and information in row
   */
  int row(int whichRow, double & rowLower, double & rowUpper,
          const int * & indices, const double * & elements) const;
  /**  Returns number of elements in current row and information in row
       Used as rows may be stored in a chain
   */
  int currentRow(double & rowLower, double & rowUpper,
          const int * & indices, const double * & elements) const;
  /// Set current row
  void setCurrentRow(int whichRow);
  /// Returns current row number
  int currentRow() const;
  /**  Returns number of elements in a column and information in column
   */
  int column(int whichColumn, 
             double & columnLower, double & columnUpper,double & objectiveValue,
             const int * & indices, const double * & elements) const;
  /**  Returns number of elements in current column and information in column
       Used as columns may be stored in a chain
   */
  int currentColumn( double & columnLower, double & columnUpper,double & objectiveValue,
          const int * & indices, const double * & elements) const;
  /// Set current column
  void setCurrentColumn(int whichColumn);
  /// Returns current column number
  int currentColumn() const;
  /// Returns type
  inline int type() const
  { return type_;}
   //@}


  /**@name Constructors, destructor */
   //@{
   /** Default constructor. */
   CoinBuild();
   /** Constructor with type 0==for addRow, 1== for addColumn. */
   CoinBuild(int type);
   /** Destructor */
   ~CoinBuild();
   //@}

   /**@name Copy method */
   //@{
   /** The copy constructor. */
   CoinBuild(const CoinBuild&);
  /// =
   CoinBuild& operator=(const CoinBuild&);
   //@}
private:
  /// Set current 
  void setMutableCurrent(int which) const;
   /// add a item
   void addItem(int numberInItem, const int * indices,
                  const double * elements, 
                  double itemLower, 
                  double itemUpper, double objectiveValue);
  /**  Returns number of elements in a item and information in item
   */
  int item(int whichItem, 
             double & itemLower, double & itemUpper,double & objectiveValue,
             const int * & indices, const double * & elements) const;
  /**  Returns number of elements in current item and information in item
       Used as items may be stored in a chain
   */
  int currentItem( double & itemLower, double & itemUpper,double & objectiveValue,
          const int * & indices, const double * & elements) const;
  /// Set current item
  void setCurrentItem(int whichItem);
  /// Returns current item number
  int currentItem() const;
   
private:
  /**@name Data members */
   //@{
  /// Current number of items
  int numberItems_;
  /// Current number of other dimension i.e. Columns if addRow (i.e. max)
  int numberOther_;
  /// Current number of elements
  CoinBigIndex numberElements_;
  /// Current item pointer
  mutable double * currentItem_;
  /// First item pointer
  double * firstItem_;
  /// Last item pointer
  double * lastItem_;
  /// Type of build - 0 for row, 1 for column, -1 unset
  int type_;
   //@}
};

#endif
