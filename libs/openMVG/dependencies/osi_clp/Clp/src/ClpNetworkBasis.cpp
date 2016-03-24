/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "ClpNetworkBasis.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpSimplex.hpp"
#include "ClpMatrixBase.hpp"
#include "CoinIndexedVector.hpp"


//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpNetworkBasis::ClpNetworkBasis ()
{
#ifndef COIN_FAST_CODE
     slackValue_ = -1.0;
#endif
     numberRows_ = 0;
     numberColumns_ = 0;
     parent_ = NULL;
     descendant_ = NULL;
     pivot_ = NULL;
     rightSibling_ = NULL;
     leftSibling_ = NULL;
     sign_ = NULL;
     stack_ = NULL;
     permute_ = NULL;
     permuteBack_ = NULL;
     stack2_ = NULL;
     depth_ = NULL;
     mark_ = NULL;
     model_ = NULL;
}
// Constructor from CoinFactorization
ClpNetworkBasis::ClpNetworkBasis(const ClpSimplex * model,
                                 int numberRows, const CoinFactorizationDouble * pivotRegion,
                                 const int * permuteBack,
                                 const CoinBigIndex * startColumn,
                                 const int * numberInColumn,
                                 const int * indexRow, const CoinFactorizationDouble * /*element*/)
{
#ifndef COIN_FAST_CODE
     slackValue_ = -1.0;
#endif
     numberRows_ = numberRows;
     numberColumns_ = numberRows;
     parent_ = new int [ numberRows_+1];
     descendant_ = new int [ numberRows_+1];
     pivot_ = new int [ numberRows_+1];
     rightSibling_ = new int [ numberRows_+1];
     leftSibling_ = new int [ numberRows_+1];
     sign_ = new double [ numberRows_+1];
     stack_ = new int [ numberRows_+1];
     stack2_ = new int[numberRows_+1];
     depth_ = new int[numberRows_+1];
     mark_ = new char[numberRows_+1];
     permute_ = new int [numberRows_ + 1];
     permuteBack_ = new int [numberRows_ + 1];
     int i;
     for (i = 0; i < numberRows_ + 1; i++) {
          parent_[i] = -1;
          descendant_[i] = -1;
          pivot_[i] = -1;
          rightSibling_[i] = -1;
          leftSibling_[i] = -1;
          sign_[i] = -1.0;
          stack_[i] = -1;
          permute_[i] = i;
          permuteBack_[i] = i;
          stack2_[i] = -1;
          depth_[i] = -1;
          mark_[i] = 0;
     }
     mark_[numberRows_] = 1;
     // pivotColumnBack gives order of pivoting into basis
     // so pivotColumnback[0] is first slack in basis and
     // it pivots on row permuteBack[0]
     // a known root is given by permuteBack[numberRows_-1]
     for (i = 0; i < numberRows_; i++) {
          int iPivot = permuteBack[i];
          double sign;
          if (pivotRegion[i] > 0.0)
               sign = 1.0;
          else
               sign = -1.0;
          int other;
          if (numberInColumn[i] > 0) {
               int iRow = indexRow[startColumn[i]];
               other = permuteBack[iRow];
               //assert (parent_[other]!=-1);
          } else {
               other = numberRows_;
          }
          sign_[iPivot] = sign;
          int iParent = other;
          parent_[iPivot] = other;
          if (descendant_[iParent] >= 0) {
               // we have a sibling
               int iRight = descendant_[iParent];
               rightSibling_[iPivot] = iRight;
               leftSibling_[iRight] = iPivot;
          } else {
               rightSibling_[iPivot] = -1;
          }
          descendant_[iParent] = iPivot;
          leftSibling_[iPivot] = -1;
     }
     // do depth
     int nStack = 1;
     stack_[0] = descendant_[numberRows_];
     depth_[numberRows_] = -1; // root
     while (nStack) {
          // take off
          int iNext = stack_[--nStack];
          if (iNext >= 0) {
               depth_[iNext] = nStack;
               int iRight = rightSibling_[iNext];
               stack_[nStack++] = iRight;
               if (descendant_[iNext] >= 0)
                    stack_[nStack++] = descendant_[iNext];
          }
     }
     model_ = model;
     check();
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpNetworkBasis::ClpNetworkBasis (const ClpNetworkBasis & rhs)
{
#ifndef COIN_FAST_CODE
     slackValue_ = rhs.slackValue_;
#endif
     numberRows_ = rhs.numberRows_;
     numberColumns_ = rhs.numberColumns_;
     if (rhs.parent_) {
          parent_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.parent_, (numberRows_ + 1), parent_);
     } else {
          parent_ = NULL;
     }
     if (rhs.descendant_) {
          descendant_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.descendant_, (numberRows_ + 1), descendant_);
     } else {
          descendant_ = NULL;
     }
     if (rhs.pivot_) {
          pivot_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.pivot_, (numberRows_ + 1), pivot_);
     } else {
          pivot_ = NULL;
     }
     if (rhs.rightSibling_) {
          rightSibling_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.rightSibling_, (numberRows_ + 1), rightSibling_);
     } else {
          rightSibling_ = NULL;
     }
     if (rhs.leftSibling_) {
          leftSibling_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.leftSibling_, (numberRows_ + 1), leftSibling_);
     } else {
          leftSibling_ = NULL;
     }
     if (rhs.sign_) {
          sign_ = new double [numberRows_+1];
          CoinMemcpyN(rhs.sign_, (numberRows_ + 1), sign_);
     } else {
          sign_ = NULL;
     }
     if (rhs.stack_) {
          stack_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.stack_, (numberRows_ + 1), stack_);
     } else {
          stack_ = NULL;
     }
     if (rhs.permute_) {
          permute_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.permute_, (numberRows_ + 1), permute_);
     } else {
          permute_ = NULL;
     }
     if (rhs.permuteBack_) {
          permuteBack_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.permuteBack_, (numberRows_ + 1), permuteBack_);
     } else {
          permuteBack_ = NULL;
     }
     if (rhs.stack2_) {
          stack2_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.stack2_, (numberRows_ + 1), stack2_);
     } else {
          stack2_ = NULL;
     }
     if (rhs.depth_) {
          depth_ = new int [numberRows_+1];
          CoinMemcpyN(rhs.depth_, (numberRows_ + 1), depth_);
     } else {
          depth_ = NULL;
     }
     if (rhs.mark_) {
          mark_ = new char [numberRows_+1];
          CoinMemcpyN(rhs.mark_, (numberRows_ + 1), mark_);
     } else {
          mark_ = NULL;
     }
     model_ = rhs.model_;
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpNetworkBasis::~ClpNetworkBasis ()
{
     delete [] parent_;
     delete [] descendant_;
     delete [] pivot_;
     delete [] rightSibling_;
     delete [] leftSibling_;
     delete [] sign_;
     delete [] stack_;
     delete [] permute_;
     delete [] permuteBack_;
     delete [] stack2_;
     delete [] depth_;
     delete [] mark_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpNetworkBasis &
ClpNetworkBasis::operator=(const ClpNetworkBasis& rhs)
{
     if (this != &rhs) {
          delete [] parent_;
          delete [] descendant_;
          delete [] pivot_;
          delete [] rightSibling_;
          delete [] leftSibling_;
          delete [] sign_;
          delete [] stack_;
          delete [] permute_;
          delete [] permuteBack_;
          delete [] stack2_;
          delete [] depth_;
          delete [] mark_;
#ifndef COIN_FAST_CODE
          slackValue_ = rhs.slackValue_;
#endif
          numberRows_ = rhs.numberRows_;
          numberColumns_ = rhs.numberColumns_;
          if (rhs.parent_) {
               parent_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.parent_, (numberRows_ + 1), parent_);
          } else {
               parent_ = NULL;
          }
          if (rhs.descendant_) {
               descendant_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.descendant_, (numberRows_ + 1), descendant_);
          } else {
               descendant_ = NULL;
          }
          if (rhs.pivot_) {
               pivot_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.pivot_, (numberRows_ + 1), pivot_);
          } else {
               pivot_ = NULL;
          }
          if (rhs.rightSibling_) {
               rightSibling_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.rightSibling_, (numberRows_ + 1), rightSibling_);
          } else {
               rightSibling_ = NULL;
          }
          if (rhs.leftSibling_) {
               leftSibling_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.leftSibling_, (numberRows_ + 1), leftSibling_);
          } else {
               leftSibling_ = NULL;
          }
          if (rhs.sign_) {
               sign_ = new double [numberRows_+1];
               CoinMemcpyN(rhs.sign_, (numberRows_ + 1), sign_);
          } else {
               sign_ = NULL;
          }
          if (rhs.stack_) {
               stack_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.stack_, (numberRows_ + 1), stack_);
          } else {
               stack_ = NULL;
          }
          if (rhs.permute_) {
               permute_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.permute_, (numberRows_ + 1), permute_);
          } else {
               permute_ = NULL;
          }
          if (rhs.permuteBack_) {
               permuteBack_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.permuteBack_, (numberRows_ + 1), permuteBack_);
          } else {
               permuteBack_ = NULL;
          }
          if (rhs.stack2_) {
               stack2_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.stack2_, (numberRows_ + 1), stack2_);
          } else {
               stack2_ = NULL;
          }
          if (rhs.depth_) {
               depth_ = new int [numberRows_+1];
               CoinMemcpyN(rhs.depth_, (numberRows_ + 1), depth_);
          } else {
               depth_ = NULL;
          }
          if (rhs.mark_) {
               mark_ = new char [numberRows_+1];
               CoinMemcpyN(rhs.mark_, (numberRows_ + 1), mark_);
          } else {
               mark_ = NULL;
          }
     }
     return *this;
}
// checks looks okay
void ClpNetworkBasis::check()
{
     // check depth
     int nStack = 1;
     stack_[0] = descendant_[numberRows_];
     depth_[numberRows_] = -1; // root
     while (nStack) {
          // take off
          int iNext = stack_[--nStack];
          if (iNext >= 0) {
               //assert (depth_[iNext]==nStack);
               depth_[iNext] = nStack;
               int iRight = rightSibling_[iNext];
               stack_[nStack++] = iRight;
               if (descendant_[iNext] >= 0)
                    stack_[nStack++] = descendant_[iNext];
          }
     }
}
// prints
void ClpNetworkBasis::print()
{
     int i;
     printf("       parent descendant     left    right   sign    depth\n");
     for (i = 0; i < numberRows_ + 1; i++)
          printf("%4d  %7d   %8d  %7d  %7d  %5g  %7d\n",
                 i, parent_[i], descendant_[i], leftSibling_[i], rightSibling_[i],
                 sign_[i], depth_[i]);
}
/* Replaces one Column to basis,
   returns 0=OK
*/
int
ClpNetworkBasis::replaceColumn ( CoinIndexedVector * regionSparse,
                                 int pivotRow)
{
     // When things have settled down then redo this to make more elegant
     // I am sure lots of loops can be combined
     // regionSparse is empty
     assert (!regionSparse->getNumElements());
     model_->unpack(regionSparse, model_->sequenceIn());
     // arc given by pivotRow is leaving basis
     //int kParent = parent_[pivotRow];
     // arc coming in has these two nodes
     int * indices = regionSparse->getIndices();
     int iRow0 = indices[0];
     int iRow1;
     if (regionSparse->getNumElements() == 2)
          iRow1 = indices[1];
     else
          iRow1 = numberRows_;
     double sign = -regionSparse->denseVector()[iRow0];
     regionSparse->clear();
     // and outgoing
     model_->unpack(regionSparse, model_->pivotVariable()[pivotRow]);
     int jRow0 = indices[0];
     int jRow1;
     if (regionSparse->getNumElements() == 2)
          jRow1 = indices[1];
     else
          jRow1 = numberRows_;
     regionSparse->clear();
     // get correct pivotRow
     //#define FULL_DEBUG
#ifdef FULL_DEBUG
     printf ("irow %d %d, jrow %d %d\n",
             iRow0, iRow1, jRow0, jRow1);
#endif
     if (parent_[jRow0] == jRow1) {
          int newPivot = jRow0;
          if (newPivot != pivotRow) {
#ifdef FULL_DEBUG
               printf("pivot row of %d permuted to %d\n", pivotRow, newPivot);
#endif
               pivotRow = newPivot;
          }
     } else {
          //assert (parent_[jRow1]==jRow0);
          int newPivot = jRow1;
          if (newPivot != pivotRow) {
#ifdef FULL_DEBUG
               printf("pivot row of %d permuted to %d\n", pivotRow, newPivot);
#endif
               pivotRow = newPivot;
          }
     }
     bool extraPrint = (model_->numberIterations() > -3) &&
                       (model_->logLevel() > 10);
     if (extraPrint)
          print();
#ifdef FULL_DEBUG
     printf("In %d (region= %g, stored %g) %d (%g) pivoting on %d (%g)\n",
            iRow1, sign, sign_[iRow1], iRow0, sign_[iRow0] , pivotRow, sign_[pivotRow]);
#endif
     // see which path outgoing pivot is on
     int kRow = -1;
     int jRow = iRow1;
     while (jRow != numberRows_) {
          if (jRow == pivotRow) {
               kRow = iRow1;
               break;
          } else {
               jRow = parent_[jRow];
          }
     }
     if (kRow < 0) {
          jRow = iRow0;
          while (jRow != numberRows_) {
               if (jRow == pivotRow) {
                    kRow = iRow0;
                    break;
               } else {
                    jRow = parent_[jRow];
               }
          }
     }
     //assert (kRow>=0);
     if (iRow0 == kRow) {
          iRow0 = iRow1;
          iRow1 = kRow;
          sign = -sign;
     }
     // pivot row is on path from iRow1 back to root
     // get stack of nodes to change
     // Also get precursors for cleaning order
     int nStack = 1;
     stack_[0] = iRow0;
     while (kRow != pivotRow) {
          stack_[nStack++] = kRow;
          if (sign * sign_[kRow] < 0.0) {
               sign_[kRow] = -sign_[kRow];
          } else {
               sign = -sign;
          }
          kRow = parent_[kRow];
          //sign *= sign_[kRow];
     }
     stack_[nStack++] = pivotRow;
     if (sign * sign_[pivotRow] < 0.0) {
          sign_[pivotRow] = -sign_[pivotRow];
     } else {
          sign = -sign;
     }
     int iParent = parent_[pivotRow];
     while (nStack > 1) {
          int iLeft;
          int iRight;
          kRow = stack_[--nStack];
          int newParent = stack_[nStack-1];
#ifdef FULL_DEBUG
          printf("row %d, old parent %d, new parent %d, pivotrow %d\n", kRow,
                 iParent, newParent, pivotRow);
#endif
          int i1 = permuteBack_[pivotRow];
          int i2 = permuteBack_[kRow];
          permuteBack_[pivotRow] = i2;
          permuteBack_[kRow] = i1;
          // do Btran permutation
          permute_[i1] = kRow;
          permute_[i2] = pivotRow;
          pivotRow = kRow;
          // Take out of old parent
          iLeft = leftSibling_[kRow];
          iRight = rightSibling_[kRow];
          if (iLeft < 0) {
               // take out of tree
               if (iRight >= 0) {
                    leftSibling_[iRight] = iLeft;
                    descendant_[iParent] = iRight;
               } else {
#ifdef FULL_DEBUG
                    printf("Saying %d (old parent of %d) has no descendants\n",
                           iParent, kRow);
#endif
                    descendant_[iParent] = -1;
               }
          } else {
               // take out of tree
               rightSibling_[iLeft] = iRight;
               if (iRight >= 0)
                    leftSibling_[iRight] = iLeft;
          }
          leftSibling_[kRow] = -1;
          rightSibling_[kRow] = -1;

          // now insert new one
          // make this descendant of that
          if (descendant_[newParent] >= 0) {
               // we will have a sibling
               int jRight = descendant_[newParent];
               rightSibling_[kRow] = jRight;
               leftSibling_[jRight] = kRow;
          } else {
               rightSibling_[kRow] = -1;
          }
          descendant_[newParent] = kRow;
          leftSibling_[kRow] = -1;
          parent_[kRow] = newParent;

          iParent = kRow;
     }
     // now redo all depths from stack_[1]
     // This must be possible to combine - but later
     {
          int iPivot  = stack_[1];
          int iDepth = depth_[parent_[iPivot]]; //depth of parent
          iDepth ++; //correct for this one
          int nStack = 1;
          stack_[0] = iPivot;
          while (nStack) {
               // take off
               int iNext = stack_[--nStack];
               if (iNext >= 0) {
                    // add stack level
                    depth_[iNext] = nStack + iDepth;
                    stack_[nStack++] = rightSibling_[iNext];
                    if (descendant_[iNext] >= 0)
                         stack_[nStack++] = descendant_[iNext];
               }
          }
     }
     if (extraPrint)
          print();
     //check();
     return 0;
}

/* Updates one column (FTRAN) from region2 */
double
ClpNetworkBasis::updateColumn (  CoinIndexedVector * regionSparse,
                                 CoinIndexedVector * regionSparse2,
                                 int pivotRow)
{
     regionSparse->clear (  );
     double *region = regionSparse->denseVector (  );
     double *region2 = regionSparse2->denseVector (  );
     int *regionIndex2 = regionSparse2->getIndices (  );
     int numberNonZero = regionSparse2->getNumElements (  );
     int *regionIndex = regionSparse->getIndices (  );
     int i;
     bool doTwo = (numberNonZero == 2);
     int i0 = -1;
     int i1 = -1;
     if (doTwo) {
          i0 = regionIndex2[0];
          i1 = regionIndex2[1];
     }
     double returnValue = 0.0;
     bool packed = regionSparse2->packedMode();
     if (packed) {
          if (doTwo && region2[0]*region2[1] < 0.0) {
               region[i0] = region2[0];
               region2[0] = 0.0;
               region[i1] = region2[1];
               region2[1] = 0.0;
               int iDepth0 = depth_[i0];
               int iDepth1 = depth_[i1];
               if (iDepth1 > iDepth0) {
                    int temp = i0;
                    i0 = i1;
                    i1 = temp;
                    temp = iDepth0;
                    iDepth0 = iDepth1;
                    iDepth1 = temp;
               }
               numberNonZero = 0;
               if (pivotRow < 0) {
                    while (iDepth0 > iDepth1) {
                         double pivotValue = region[i0];
                         // put back now ?
                         int iBack = permuteBack_[i0];
                         region2[numberNonZero] = pivotValue * sign_[i0];
                         regionIndex2[numberNonZero++] = iBack;
                         int otherRow = parent_[i0];
                         region[i0] = 0.0;
                         region[otherRow] += pivotValue;
                         iDepth0--;
                         i0 = otherRow;
                    }
                    while (i0 != i1) {
                         double pivotValue = region[i0];
                         // put back now ?
                         int iBack = permuteBack_[i0];
                         region2[numberNonZero] = pivotValue * sign_[i0];
                         regionIndex2[numberNonZero++] = iBack;
                         int otherRow = parent_[i0];
                         region[i0] = 0.0;
                         region[otherRow] += pivotValue;
                         i0 = otherRow;
                         double pivotValue1 = region[i1];
                         // put back now ?
                         int iBack1 = permuteBack_[i1];
                         region2[numberNonZero] = pivotValue1 * sign_[i1];
                         regionIndex2[numberNonZero++] = iBack1;
                         int otherRow1 = parent_[i1];
                         region[i1] = 0.0;
                         region[otherRow1] += pivotValue1;
                         i1 = otherRow1;
                    }
               } else {
                    while (iDepth0 > iDepth1) {
                         double pivotValue = region[i0];
                         // put back now ?
                         int iBack = permuteBack_[i0];
                         double value = pivotValue * sign_[i0];
                         region2[numberNonZero] = value;
                         regionIndex2[numberNonZero++] = iBack;
                         if (iBack == pivotRow)
                              returnValue = value;
                         int otherRow = parent_[i0];
                         region[i0] = 0.0;
                         region[otherRow] += pivotValue;
                         iDepth0--;
                         i0 = otherRow;
                    }
                    while (i0 != i1) {
                         double pivotValue = region[i0];
                         // put back now ?
                         int iBack = permuteBack_[i0];
                         double value = pivotValue * sign_[i0];
                         region2[numberNonZero] = value;
                         regionIndex2[numberNonZero++] = iBack;
                         if (iBack == pivotRow)
                              returnValue = value;
                         int otherRow = parent_[i0];
                         region[i0] = 0.0;
                         region[otherRow] += pivotValue;
                         i0 = otherRow;
                         double pivotValue1 = region[i1];
                         // put back now ?
                         int iBack1 = permuteBack_[i1];
                         value = pivotValue1 * sign_[i1];
                         region2[numberNonZero] = value;
                         regionIndex2[numberNonZero++] = iBack1;
                         if (iBack1 == pivotRow)
                              returnValue = value;
                         int otherRow1 = parent_[i1];
                         region[i1] = 0.0;
                         region[otherRow1] += pivotValue1;
                         i1 = otherRow1;
                    }
               }
          } else {
               // set up linked lists at each depth
               // stack2 is start, stack is next
               int greatestDepth = -1;
               //mark_[numberRows_]=1;
               for (i = 0; i < numberNonZero; i++) {
                    int j = regionIndex2[i];
                    double value = region2[i];
                    region2[i] = 0.0;
                    region[j] = value;
                    regionIndex[i] = j;
                    int iDepth = depth_[j];
                    if (iDepth > greatestDepth)
                         greatestDepth = iDepth;
                    // and back until marked
                    while (!mark_[j]) {
                         int iNext = stack2_[iDepth];
                         stack2_[iDepth] = j;
                         stack_[j] = iNext;
                         mark_[j] = 1;
                         iDepth--;
                         j = parent_[j];
                    }
               }
               numberNonZero = 0;
               if (pivotRow < 0) {
                    for (; greatestDepth >= 0; greatestDepth--) {
                         int iPivot = stack2_[greatestDepth];
                         stack2_[greatestDepth] = -1;
                         while (iPivot >= 0) {
                              mark_[iPivot] = 0;
                              double pivotValue = region[iPivot];
                              if (pivotValue) {
                                   // put back now ?
                                   int iBack = permuteBack_[iPivot];
                                   region2[numberNonZero] = pivotValue * sign_[iPivot];
                                   regionIndex2[numberNonZero++] = iBack;
                                   int otherRow = parent_[iPivot];
                                   region[iPivot] = 0.0;
                                   region[otherRow] += pivotValue;
                              }
                              iPivot = stack_[iPivot];
                         }
                    }
               } else {
                    for (; greatestDepth >= 0; greatestDepth--) {
                         int iPivot = stack2_[greatestDepth];
                         stack2_[greatestDepth] = -1;
                         while (iPivot >= 0) {
                              mark_[iPivot] = 0;
                              double pivotValue = region[iPivot];
                              if (pivotValue) {
                                   // put back now ?
                                   int iBack = permuteBack_[iPivot];
                                   double value = pivotValue * sign_[iPivot];
                                   region2[numberNonZero] = value;
                                   regionIndex2[numberNonZero++] = iBack;
                                   if (iBack == pivotRow)
                                        returnValue = value;
                                   int otherRow = parent_[iPivot];
                                   region[iPivot] = 0.0;
                                   region[otherRow] += pivotValue;
                              }
                              iPivot = stack_[iPivot];
                         }
                    }
               }
          }
     } else {
          if (doTwo && region2[i0]*region2[i1] < 0.0) {
               // If just +- 1 then could go backwards on depth until join
               region[i0] = region2[i0];
               region2[i0] = 0.0;
               region[i1] = region2[i1];
               region2[i1] = 0.0;
               int iDepth0 = depth_[i0];
               int iDepth1 = depth_[i1];
               if (iDepth1 > iDepth0) {
                    int temp = i0;
                    i0 = i1;
                    i1 = temp;
                    temp = iDepth0;
                    iDepth0 = iDepth1;
                    iDepth1 = temp;
               }
               numberNonZero = 0;
               while (iDepth0 > iDepth1) {
                    double pivotValue = region[i0];
                    // put back now ?
                    int iBack = permuteBack_[i0];
                    regionIndex2[numberNonZero++] = iBack;
                    int otherRow = parent_[i0];
                    region2[iBack] = pivotValue * sign_[i0];
                    region[i0] = 0.0;
                    region[otherRow] += pivotValue;
                    iDepth0--;
                    i0 = otherRow;
               }
               while (i0 != i1) {
                    double pivotValue = region[i0];
                    // put back now ?
                    int iBack = permuteBack_[i0];
                    regionIndex2[numberNonZero++] = iBack;
                    int otherRow = parent_[i0];
                    region2[iBack] = pivotValue * sign_[i0];
                    region[i0] = 0.0;
                    region[otherRow] += pivotValue;
                    i0 = otherRow;
                    double pivotValue1 = region[i1];
                    // put back now ?
                    int iBack1 = permuteBack_[i1];
                    regionIndex2[numberNonZero++] = iBack1;
                    int otherRow1 = parent_[i1];
                    region2[iBack1] = pivotValue1 * sign_[i1];
                    region[i1] = 0.0;
                    region[otherRow1] += pivotValue1;
                    i1 = otherRow1;
               }
          } else {
               // set up linked lists at each depth
               // stack2 is start, stack is next
               int greatestDepth = -1;
               //mark_[numberRows_]=1;
               for (i = 0; i < numberNonZero; i++) {
                    int j = regionIndex2[i];
                    double value = region2[j];
                    region2[j] = 0.0;
                    region[j] = value;
                    regionIndex[i] = j;
                    int iDepth = depth_[j];
                    if (iDepth > greatestDepth)
                         greatestDepth = iDepth;
                    // and back until marked
                    while (!mark_[j]) {
                         int iNext = stack2_[iDepth];
                         stack2_[iDepth] = j;
                         stack_[j] = iNext;
                         mark_[j] = 1;
                         iDepth--;
                         j = parent_[j];
                    }
               }
               numberNonZero = 0;
               for (; greatestDepth >= 0; greatestDepth--) {
                    int iPivot = stack2_[greatestDepth];
                    stack2_[greatestDepth] = -1;
                    while (iPivot >= 0) {
                         mark_[iPivot] = 0;
                         double pivotValue = region[iPivot];
                         if (pivotValue) {
                              // put back now ?
                              int iBack = permuteBack_[iPivot];
                              regionIndex2[numberNonZero++] = iBack;
                              int otherRow = parent_[iPivot];
                              region2[iBack] = pivotValue * sign_[iPivot];
                              region[iPivot] = 0.0;
                              region[otherRow] += pivotValue;
                         }
                         iPivot = stack_[iPivot];
                    }
               }
          }
          if (pivotRow >= 0)
               returnValue = region2[pivotRow];
     }
     region[numberRows_] = 0.0;
     regionSparse2->setNumElements(numberNonZero);
#ifdef FULL_DEBUG
     {
          int i;
          for (i = 0; i < numberRows_; i++) {
               assert(!mark_[i]);
               assert (stack2_[i] == -1);
          }
     }
#endif
     return returnValue;
}
/* Updates one column (FTRAN) to/from array
    ** For large problems you should ALWAYS know where the nonzeros
   are, so please try and migrate to previous method after you
   have got code working using this simple method - thank you!
   (the only exception is if you know input is dense e.g. rhs) */
int
ClpNetworkBasis::updateColumn (  CoinIndexedVector * regionSparse,
                                 double region2[] ) const
{
     regionSparse->clear (  );
     double *region = regionSparse->denseVector (  );
     int numberNonZero = 0;
     int *regionIndex = regionSparse->getIndices (  );
     int i;
     // set up linked lists at each depth
     // stack2 is start, stack is next
     int greatestDepth = -1;
     for (i = 0; i < numberRows_; i++) {
          double value = region2[i];
          if (value) {
               region2[i] = 0.0;
               region[i] = value;
               regionIndex[numberNonZero++] = i;
               int j = i;
               int iDepth = depth_[j];
               if (iDepth > greatestDepth)
                    greatestDepth = iDepth;
               // and back until marked
               while (!mark_[j]) {
                    int iNext = stack2_[iDepth];
                    stack2_[iDepth] = j;
                    stack_[j] = iNext;
                    mark_[j] = 1;
                    iDepth--;
                    j = parent_[j];
               }
          }
     }
     numberNonZero = 0;
     for (; greatestDepth >= 0; greatestDepth--) {
          int iPivot = stack2_[greatestDepth];
          stack2_[greatestDepth] = -1;
          while (iPivot >= 0) {
               mark_[iPivot] = 0;
               double pivotValue = region[iPivot];
               if (pivotValue) {
                    // put back now ?
                    int iBack = permuteBack_[iPivot];
                    numberNonZero++;
                    int otherRow = parent_[iPivot];
                    region2[iBack] = pivotValue * sign_[iPivot];
                    region[iPivot] = 0.0;
                    region[otherRow] += pivotValue;
               }
               iPivot = stack_[iPivot];
          }
     }
     region[numberRows_] = 0.0;
     return numberNonZero;
}
/* Updates one column transpose (BTRAN)
   For large problems you should ALWAYS know where the nonzeros
   are, so please try and migrate to previous method after you
   have got code working using this simple method - thank you!
   (the only exception is if you know input is dense e.g. dense objective)
   returns number of nonzeros */
int
ClpNetworkBasis::updateColumnTranspose (  CoinIndexedVector * regionSparse,
          double region2[] ) const
{
     // permute in after copying
     // so will end up in right place
     double *region = regionSparse->denseVector (  );
     int *regionIndex = regionSparse->getIndices (  );
     int i;
     int numberNonZero = 0;
     CoinMemcpyN(region2, numberRows_, region);
     for (i = 0; i < numberRows_; i++) {
          double value = region[i];
          if (value) {
               int k = permute_[i];
               region[i] = 0.0;
               region2[k] = value;
               regionIndex[numberNonZero++] = k;
               mark_[k] = 1;
          }
     }
     // copy back
     // set up linked lists at each depth
     // stack2 is start, stack is next
     int greatestDepth = -1;
     int smallestDepth = numberRows_;
     for (i = 0; i < numberNonZero; i++) {
          int j = regionIndex[i];
          // add in
          int iDepth = depth_[j];
          smallestDepth = CoinMin(iDepth, smallestDepth) ;
          greatestDepth = CoinMax(iDepth, greatestDepth) ;
          int jNext = stack2_[iDepth];
          stack2_[iDepth] = j;
          stack_[j] = jNext;
          // and put all descendants on list
          int iChild = descendant_[j];
          while (iChild >= 0) {
               if (!mark_[iChild]) {
                    regionIndex[numberNonZero++] = iChild;
                    mark_[iChild] = 1;
               }
               iChild = rightSibling_[iChild];
          }
     }
     numberNonZero = 0;
     region2[numberRows_] = 0.0;
     int iDepth;
     for (iDepth = smallestDepth; iDepth <= greatestDepth; iDepth++) {
          int iPivot = stack2_[iDepth];
          stack2_[iDepth] = -1;
          while (iPivot >= 0) {
               mark_[iPivot] = 0;
               double pivotValue = region2[iPivot];
               int otherRow = parent_[iPivot];
               double otherValue = region2[otherRow];
               pivotValue = sign_[iPivot] * pivotValue + otherValue;
               region2[iPivot] = pivotValue;
               if (pivotValue)
                    numberNonZero++;
               iPivot = stack_[iPivot];
          }
     }
     return numberNonZero;
}
/* Updates one column (BTRAN) from region2 */
int
ClpNetworkBasis::updateColumnTranspose (  CoinIndexedVector * regionSparse,
          CoinIndexedVector * regionSparse2) const
{
     // permute in - presume small number so copy back
     // so will end up in right place
     regionSparse->clear (  );
     double *region = regionSparse->denseVector (  );
     double *region2 = regionSparse2->denseVector (  );
     int *regionIndex2 = regionSparse2->getIndices (  );
     int numberNonZero2 = regionSparse2->getNumElements (  );
     int *regionIndex = regionSparse->getIndices (  );
     int i;
     int numberNonZero = 0;
     bool packed = regionSparse2->packedMode();
     if (packed) {
          for (i = 0; i < numberNonZero2; i++) {
               int k = regionIndex2[i];
               int j = permute_[k];
               double value = region2[i];
               region2[i] = 0.0;
               region[j] = value;
               mark_[j] = 1;
               regionIndex[numberNonZero++] = j;
          }
          // set up linked lists at each depth
          // stack2 is start, stack is next
          int greatestDepth = -1;
          int smallestDepth = numberRows_;
          //mark_[numberRows_]=1;
          for (i = 0; i < numberNonZero2; i++) {
               int j = regionIndex[i];
               regionIndex2[i] = j;
               // add in
               int iDepth = depth_[j];
               smallestDepth = CoinMin(iDepth, smallestDepth) ;
               greatestDepth = CoinMax(iDepth, greatestDepth) ;
               int jNext = stack2_[iDepth];
               stack2_[iDepth] = j;
               stack_[j] = jNext;
               // and put all descendants on list
               int iChild = descendant_[j];
               while (iChild >= 0) {
                    if (!mark_[iChild]) {
                         regionIndex2[numberNonZero++] = iChild;
                         mark_[iChild] = 1;
                    }
                    iChild = rightSibling_[iChild];
               }
          }
          for (; i < numberNonZero; i++) {
               int j = regionIndex2[i];
               // add in
               int iDepth = depth_[j];
               smallestDepth = CoinMin(iDepth, smallestDepth) ;
               greatestDepth = CoinMax(iDepth, greatestDepth) ;
               int jNext = stack2_[iDepth];
               stack2_[iDepth] = j;
               stack_[j] = jNext;
               // and put all descendants on list
               int iChild = descendant_[j];
               while (iChild >= 0) {
                    if (!mark_[iChild]) {
                         regionIndex2[numberNonZero++] = iChild;
                         mark_[iChild] = 1;
                    }
                    iChild = rightSibling_[iChild];
               }
          }
          numberNonZero2 = 0;
          region[numberRows_] = 0.0;
          int iDepth;
          for (iDepth = smallestDepth; iDepth <= greatestDepth; iDepth++) {
               int iPivot = stack2_[iDepth];
               stack2_[iDepth] = -1;
               while (iPivot >= 0) {
                    mark_[iPivot] = 0;
                    double pivotValue = region[iPivot];
                    int otherRow = parent_[iPivot];
                    double otherValue = region[otherRow];
                    pivotValue = sign_[iPivot] * pivotValue + otherValue;
                    region[iPivot] = pivotValue;
                    if (pivotValue) {
                         region2[numberNonZero2] = pivotValue;
                         regionIndex2[numberNonZero2++] = iPivot;
                    }
                    iPivot = stack_[iPivot];
               }
          }
          // zero out
          for (i = 0; i < numberNonZero2; i++) {
               int k = regionIndex2[i];
               region[k] = 0.0;
          }
     } else {
          for (i = 0; i < numberNonZero2; i++) {
               int k = regionIndex2[i];
               int j = permute_[k];
               double value = region2[k];
               region2[k] = 0.0;
               region[j] = value;
               mark_[j] = 1;
               regionIndex[numberNonZero++] = j;
          }
          // copy back
          // set up linked lists at each depth
          // stack2 is start, stack is next
          int greatestDepth = -1;
          int smallestDepth = numberRows_;
          //mark_[numberRows_]=1;
          for (i = 0; i < numberNonZero2; i++) {
               int j = regionIndex[i];
               double value = region[j];
               region[j] = 0.0;
               region2[j] = value;
               regionIndex2[i] = j;
               // add in
               int iDepth = depth_[j];
               smallestDepth = CoinMin(iDepth, smallestDepth) ;
               greatestDepth = CoinMax(iDepth, greatestDepth) ;
               int jNext = stack2_[iDepth];
               stack2_[iDepth] = j;
               stack_[j] = jNext;
               // and put all descendants on list
               int iChild = descendant_[j];
               while (iChild >= 0) {
                    if (!mark_[iChild]) {
                         regionIndex2[numberNonZero++] = iChild;
                         mark_[iChild] = 1;
                    }
                    iChild = rightSibling_[iChild];
               }
          }
          for (; i < numberNonZero; i++) {
               int j = regionIndex2[i];
               // add in
               int iDepth = depth_[j];
               smallestDepth = CoinMin(iDepth, smallestDepth) ;
               greatestDepth = CoinMax(iDepth, greatestDepth) ;
               int jNext = stack2_[iDepth];
               stack2_[iDepth] = j;
               stack_[j] = jNext;
               // and put all descendants on list
               int iChild = descendant_[j];
               while (iChild >= 0) {
                    if (!mark_[iChild]) {
                         regionIndex2[numberNonZero++] = iChild;
                         mark_[iChild] = 1;
                    }
                    iChild = rightSibling_[iChild];
               }
          }
          numberNonZero2 = 0;
          region2[numberRows_] = 0.0;
          int iDepth;
          for (iDepth = smallestDepth; iDepth <= greatestDepth; iDepth++) {
               int iPivot = stack2_[iDepth];
               stack2_[iDepth] = -1;
               while (iPivot >= 0) {
                    mark_[iPivot] = 0;
                    double pivotValue = region2[iPivot];
                    int otherRow = parent_[iPivot];
                    double otherValue = region2[otherRow];
                    pivotValue = sign_[iPivot] * pivotValue + otherValue;
                    region2[iPivot] = pivotValue;
                    if (pivotValue)
                         regionIndex2[numberNonZero2++] = iPivot;
                    iPivot = stack_[iPivot];
               }
          }
     }
     regionSparse2->setNumElements(numberNonZero2);
#ifdef FULL_DEBUG
     {
          int i;
          for (i = 0; i < numberRows_; i++) {
               assert(!mark_[i]);
               assert (stack2_[i] == -1);
          }
     }
#endif
     return numberNonZero2;
}
