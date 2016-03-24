/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Forrest

 */
#ifndef ClpNetworkBasis_H
#define ClpNetworkBasis_H

class ClpMatrixBase;
class CoinIndexedVector;
class ClpSimplex;
#include "CoinTypes.hpp"
#ifndef COIN_FAST_CODE
#define COIN_FAST_CODE
#endif

/** This deals with Factorization and Updates for network structures
 */


class ClpNetworkBasis {

public:

     /**@name Constructors and destructor and copy */
     //@{
     /// Default constructor
     ClpNetworkBasis (  );
     /// Constructor from CoinFactorization
     ClpNetworkBasis(const ClpSimplex * model,
                     int numberRows, const CoinFactorizationDouble * pivotRegion,
                     const int * permuteBack, const CoinBigIndex * startColumn,
                     const int * numberInColumn,
                     const int * indexRow, const CoinFactorizationDouble * element);
     /// Copy constructor
     ClpNetworkBasis ( const ClpNetworkBasis &other);

     /// Destructor
     ~ClpNetworkBasis (  );
     /// = copy
     ClpNetworkBasis & operator = ( const ClpNetworkBasis & other );
     //@}

     /**@name Do factorization */
     //@{
     /** When part of LP - given by basic variables.
     Actually does factorization.
     Arrays passed in have non negative value to say basic.
     If status is okay, basic variables have pivot row - this is only needed
     if increasingRows_ >1.
     If status is singular, then basic variables have pivot row
     and ones thrown out have -1
     returns 0 -okay, -1 singular, -2 too many in basis */
     int factorize ( const ClpMatrixBase * matrix,
                     int rowIsBasic[], int columnIsBasic[]);
     //@}

     /**@name rank one updates which do exist */
     //@{

     /** Replaces one Column to basis,
      returns 0=OK, 1=Probably OK, 2=singular!!
     */
     int replaceColumn ( CoinIndexedVector * column,
                         int pivotRow);
     //@}

     /**@name various uses of factorization (return code number elements)
      which user may want to know about */
     //@{
     /** Updates one column (FTRAN) from region,
         Returns pivot value if "pivotRow" >=0
     */
     double updateColumn ( CoinIndexedVector * regionSparse,
                           CoinIndexedVector * regionSparse2,
                           int pivotRow);
     /** Updates one column (FTRAN) to/from array
         ** For large problems you should ALWAYS know where the nonzeros
         are, so please try and migrate to previous method after you
         have got code working using this simple method - thank you!
         (the only exception is if you know input is dense e.g. rhs) */
     int updateColumn (  CoinIndexedVector * regionSparse,
                         double array[] ) const;
     /** Updates one column transpose (BTRAN)
         ** For large problems you should ALWAYS know where the nonzeros
         are, so please try and migrate to previous method after you
         have got code working using this simple method - thank you!
         (the only exception is if you know input is dense e.g. dense objective)
         returns number of nonzeros */
     int updateColumnTranspose (  CoinIndexedVector * regionSparse,
                                  double array[] ) const;
     /** Updates one column (BTRAN) from region2 */
     int updateColumnTranspose (  CoinIndexedVector * regionSparse,
                                  CoinIndexedVector * regionSparse2) const;
     //@}
////////////////// data //////////////////
private:

     // checks looks okay
     void check();
     // prints data
     void print();
     /**@name data */
     //@{
#ifndef COIN_FAST_CODE
     /// Whether slack value is  +1 or -1
     double slackValue_;
#endif
     /// Number of Rows in factorization
     int numberRows_;
     /// Number of Columns in factorization
     int numberColumns_;
     /// model
     const ClpSimplex * model_;
     /// Parent for each column
     int * parent_;
     /// Descendant
     int * descendant_;
     /// Pivot row
     int * pivot_;
     /// Right sibling
     int * rightSibling_;
     /// Left sibling
     int * leftSibling_;
     /// Sign of pivot
     double * sign_;
     /// Stack
     int * stack_;
     /// Permute into array
     int * permute_;
     /// Permute back array
     int * permuteBack_;
     /// Second stack
     int * stack2_;
     /// Depth
     int * depth_;
     /// To mark rows
     char * mark_;
     //@}
};
#endif
