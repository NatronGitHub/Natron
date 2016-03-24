/* $Id$ */
/*
  Copyright (C) 2003, International Business Machines Corporation
  and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/
#ifndef ClpCholeskyDense_H
#define ClpCholeskyDense_H

#include "ClpCholeskyBase.hpp"
class ClpMatrixBase;

class ClpCholeskyDense : public ClpCholeskyBase {

public:
     /**@name Virtual methods that the derived classes provides  */
     /**@{*/
     /** Orders rows and saves pointer to matrix.and model.
      Returns non-zero if not enough memory */
     virtual int order(ClpInterior * model) ;
     /** Does Symbolic factorization given permutation.
         This is called immediately after order.  If user provides this then
         user must provide factorize and solve.  Otherwise the default factorization is used
         returns non-zero if not enough memory */
     virtual int symbolic();
     /** Factorize - filling in rowsDropped and returning number dropped.
         If return code negative then out of memory */
     virtual int factorize(const CoinWorkDouble * diagonal, int * rowsDropped) ;
     /** Uses factorization to solve. */
     virtual void solve (CoinWorkDouble * region) ;
     /**@}*/

     /**@name Non virtual methods for ClpCholeskyDense  */
     /**@{*/
     /** Reserves space.
         If factor not NULL then just uses passed space
      Returns non-zero if not enough memory */
     int reserveSpace(const ClpCholeskyBase * factor, int numberRows) ;
     /** Returns space needed */
     CoinBigIndex space( int numberRows) const;
     /** part 2 of Factorize - filling in rowsDropped */
     void factorizePart2(int * rowsDropped) ;
     /** part 2 of Factorize - filling in rowsDropped - blocked */
     void factorizePart3(int * rowsDropped) ;
     /** Forward part of solve */
     void solveF1(longDouble * a, int n, CoinWorkDouble * region);
     void solveF2(longDouble * a, int n, CoinWorkDouble * region, CoinWorkDouble * region2);
     /** Backward part of solve */
     void solveB1(longDouble * a, int n, CoinWorkDouble * region);
     void solveB2(longDouble * a, int n, CoinWorkDouble * region, CoinWorkDouble * region2);
     int bNumber(const longDouble * array, int &, int&);
     /** A */
     inline longDouble * aMatrix() const {
          return sparseFactor_;
     }
     /** Diagonal */
     inline longDouble * diagonal() const {
          return diagonal_;
     }
     /**@}*/


     /**@name Constructors, destructor */
     /**@{*/
     /** Default constructor. */
     ClpCholeskyDense();
     /** Destructor  */
     virtual ~ClpCholeskyDense();
     /** Copy */
     ClpCholeskyDense(const ClpCholeskyDense&);
     /** Assignment */
     ClpCholeskyDense& operator=(const ClpCholeskyDense&);
     /** Clone */
     virtual ClpCholeskyBase * clone() const ;
     /**@}*/


private:
     /**@name Data members */
     /**@{*/
     /** Just borrowing space */
     bool borrowSpace_;
     /**@}*/
};

/* structure for C */
typedef struct {
     longDouble * diagonal_;
     longDouble * a;
     longDouble * work;
     int * rowsDropped;
     double doubleParameters_[1]; /* corresponds to 10 */
     int integerParameters_[2]; /* corresponds to 34, nThreads */
     int n;
     int numberBlocks;
} ClpCholeskyDenseC;

extern "C" {
     void ClpCholeskySpawn(void *);
}
/**Non leaf recursive factor */
void
ClpCholeskyCfactor(ClpCholeskyDenseC * thisStruct,
                   longDouble * a, int n, int numberBlocks,
                   longDouble * diagonal, longDouble * work, int * rowsDropped);

/**Non leaf recursive triangle rectangle update */
void
ClpCholeskyCtriRec(ClpCholeskyDenseC * thisStruct,
                   longDouble * aTri, int nThis,
                   longDouble * aUnder, longDouble * diagonal,
                   longDouble * work,
                   int nLeft, int iBlock, int jBlock,
                   int numberBlocks);
/**Non leaf recursive rectangle triangle update */
void
ClpCholeskyCrecTri(ClpCholeskyDenseC * thisStruct,
                   longDouble * aUnder, int nTri, int nDo,
                   int iBlock, int jBlock, longDouble * aTri,
                   longDouble * diagonal, longDouble * work,
                   int numberBlocks);
/** Non leaf recursive rectangle rectangle update,
    nUnder is number of rows in iBlock,
    nUnderK is number of rows in kBlock
*/
void
ClpCholeskyCrecRec(ClpCholeskyDenseC * thisStruct,
                   longDouble * above, int nUnder, int nUnderK,
                   int nDo, longDouble * aUnder, longDouble *aOther,
                   longDouble * work,
                   int iBlock, int jBlock,
                   int numberBlocks);
/**Leaf recursive factor */
void
ClpCholeskyCfactorLeaf(ClpCholeskyDenseC * thisStruct,
                       longDouble * a, int n,
                       longDouble * diagonal, longDouble * work,
                       int * rowsDropped);
/**Leaf recursive triangle rectangle update */
void
ClpCholeskyCtriRecLeaf(/*ClpCholeskyDenseC * thisStruct,*/
     longDouble * aTri, longDouble * aUnder,
     longDouble * diagonal, longDouble * work,
     int nUnder);
/**Leaf recursive rectangle triangle update */
void
ClpCholeskyCrecTriLeaf(/*ClpCholeskyDenseC * thisStruct, */
     longDouble * aUnder, longDouble * aTri,
     /*longDouble * diagonal,*/ longDouble * work, int nUnder);
/** Leaf recursive rectangle rectangle update,
    nUnder is number of rows in iBlock,
    nUnderK is number of rows in kBlock
*/
void
ClpCholeskyCrecRecLeaf(/*ClpCholeskyDenseC * thisStruct, */
     const longDouble * COIN_RESTRICT above,
     const longDouble * COIN_RESTRICT aUnder,
     longDouble * COIN_RESTRICT aOther,
     const longDouble * COIN_RESTRICT work,
     int nUnder);
#endif
