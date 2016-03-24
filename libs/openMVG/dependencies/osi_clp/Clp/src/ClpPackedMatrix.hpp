/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpPackedMatrix_H
#define ClpPackedMatrix_H

#include "CoinPragma.hpp"

#include "ClpMatrixBase.hpp"

// Compilers can produce better code if they know about __restrict
#ifndef COIN_RESTRICT
#ifdef COIN_USE_RESTRICT
#define COIN_RESTRICT __restrict
#else
#define COIN_RESTRICT
#endif
#endif

/** This implements CoinPackedMatrix as derived from ClpMatrixBase.

    It adds a few methods that know about model as well as matrix

    For details see CoinPackedMatrix */

class ClpPackedMatrix2;
class ClpPackedMatrix3;
class ClpPackedMatrix : public ClpMatrixBase {

public:
     /**@name Useful methods */
     //@{
     /// Return a complete CoinPackedMatrix
     virtual CoinPackedMatrix * getPackedMatrix() const {
          return matrix_;
     }
     /** Whether the packed matrix is column major ordered or not. */
     virtual bool isColOrdered() const {
          return matrix_->isColOrdered();
     }
     /** Number of entries in the packed matrix. */
     virtual  CoinBigIndex getNumElements() const {
          return matrix_->getNumElements();
     }
     /** Number of columns. */
     virtual int getNumCols() const {
          return matrix_->getNumCols();
     }
     /** Number of rows. */
     virtual int getNumRows() const {
          return matrix_->getNumRows();
     }

     /** A vector containing the elements in the packed matrix. Note that there
         might be gaps in this list, entries that do not belong to any
         major-dimension vector. To get the actual elements one should look at
         this vector together with vectorStarts and vectorLengths. */
     virtual const double * getElements() const {
          return matrix_->getElements();
     }
     /// Mutable elements
     inline double * getMutableElements() const {
          return matrix_->getMutableElements();
     }
     /** A vector containing the minor indices of the elements in the packed
          matrix. Note that there might be gaps in this list, entries that do not
          belong to any major-dimension vector. To get the actual elements one
          should look at this vector together with vectorStarts and
          vectorLengths. */
     virtual const int * getIndices() const {
          return matrix_->getIndices();
     }

     virtual const CoinBigIndex * getVectorStarts() const {
          return matrix_->getVectorStarts();
     }
     /** The lengths of the major-dimension vectors. */
     virtual const int * getVectorLengths() const {
          return matrix_->getVectorLengths();
     }
     /** The length of a single major-dimension vector. */
     virtual int getVectorLength(int index) const {
          return matrix_->getVectorSize(index);
     }

     /** Delete the columns whose indices are listed in <code>indDel</code>. */
     virtual void deleteCols(const int numDel, const int * indDel);
     /** Delete the rows whose indices are listed in <code>indDel</code>. */
     virtual void deleteRows(const int numDel, const int * indDel);
#ifndef CLP_NO_VECTOR
     /// Append Columns
     virtual void appendCols(int number, const CoinPackedVectorBase * const * columns);
     /// Append Rows
     virtual void appendRows(int number, const CoinPackedVectorBase * const * rows);
#endif
     /** Append a set of rows/columns to the end of the matrix. Returns number of errors
         i.e. if any of the new rows/columns contain an index that's larger than the
         number of columns-1/rows-1 (if numberOther>0) or duplicates
         If 0 then rows, 1 if columns */
     virtual int appendMatrix(int number, int type,
                              const CoinBigIndex * starts, const int * index,
                              const double * element, int numberOther = -1);
     /** Replace the elements of a vector.  The indices remain the same.
         This is only needed if scaling and a row copy is used.
         At most the number specified will be replaced.
         The index is between 0 and major dimension of matrix */
     virtual void replaceVector(const int index,
                                const int numReplace, const double * newElements) {
          matrix_->replaceVector(index, numReplace, newElements);
     }
     /** Modify one element of packed matrix.  An element may be added.
         This works for either ordering If the new element is zero it will be
         deleted unless keepZero true */
     virtual void modifyCoefficient(int row, int column, double newElement,
                                    bool keepZero = false) {
          matrix_->modifyCoefficient(row, column, newElement, keepZero);
     }
     /** Returns a new matrix in reverse order without gaps */
     virtual ClpMatrixBase * reverseOrderedCopy() const;
     /// Returns number of elements in column part of basis
     virtual CoinBigIndex countBasis(const int * whichColumn,
                                     int & numberColumnBasic);
     /// Fills in column part of basis
     virtual void fillBasis(ClpSimplex * model,
                            const int * whichColumn,
                            int & numberColumnBasic,
                            int * row, int * start,
                            int * rowCount, int * columnCount,
                            CoinFactorizationDouble * element);
     /** Creates scales for column copy (rowCopy in model may be modified)
         returns non-zero if no scaling done */
     virtual int scale(ClpModel * model, const ClpSimplex * baseModel = NULL) const ;
     /** Scales rowCopy if column copy scaled
         Only called if scales already exist */
     virtual void scaleRowCopy(ClpModel * model) const ;
     /// Creates scaled column copy if scales exist
     void createScaledMatrix(ClpSimplex * model) const;
     /** Realy really scales column copy
         Only called if scales already exist.
         Up to user ro delete */
     virtual ClpMatrixBase * scaledColumnCopy(ClpModel * model) const ;
     /** Checks if all elements are in valid range.  Can just
         return true if you are not paranoid.  For Clp I will
         probably expect no zeros.  Code can modify matrix to get rid of
         small elements.
         check bits (can be turned off to save time) :
         1 - check if matrix has gaps
         2 - check if zero elements
         4 - check and compress duplicates
         8 - report on large and small
     */
     virtual bool allElementsInRange(ClpModel * model,
                                     double smallest, double largest,
                                     int check = 15);
     /** Returns largest and smallest elements of both signs.
         Largest refers to largest absolute value.
     */
     virtual void rangeOfElements(double & smallestNegative, double & largestNegative,
                                  double & smallestPositive, double & largestPositive);

     /** Unpacks a column into an CoinIndexedvector
      */
     virtual void unpack(const ClpSimplex * model, CoinIndexedVector * rowArray,
                         int column) const ;
     /** Unpacks a column into an CoinIndexedvector
      ** in packed foramt
         Note that model is NOT const.  Bounds and objective could
         be modified if doing column generation (just for this variable) */
     virtual void unpackPacked(ClpSimplex * model,
                               CoinIndexedVector * rowArray,
                               int column) const;
     /** Adds multiple of a column into an CoinIndexedvector
         You can use quickAdd to add to vector */
     virtual void add(const ClpSimplex * model, CoinIndexedVector * rowArray,
                      int column, double multiplier) const ;
     /** Adds multiple of a column into an array */
     virtual void add(const ClpSimplex * model, double * array,
                      int column, double multiplier) const;
     /// Allow any parts of a created CoinPackedMatrix to be deleted
     virtual void releasePackedMatrix() const { }
     /** Given positive integer weights for each row fills in sum of weights
         for each column (and slack).
         Returns weights vector
     */
     virtual CoinBigIndex * dubiousWeights(const ClpSimplex * model, int * inputWeights) const;
     /// Says whether it can do partial pricing
     virtual bool canDoPartialPricing() const;
     /// Partial pricing
     virtual void partialPricing(ClpSimplex * model, double start, double end,
                                 int & bestSequence, int & numberWanted);
     /// makes sure active columns correct
     virtual int refresh(ClpSimplex * model);
     // Really scale matrix
     virtual void reallyScale(const double * rowScale, const double * columnScale);
     /** Set the dimensions of the matrix. In effect, append new empty
         columns/rows to the matrix. A negative number for either dimension
         means that that dimension doesn't change. Otherwise the new dimensions
         MUST be at least as large as the current ones otherwise an exception
         is thrown. */
     virtual void setDimensions(int numrows, int numcols);
     //@}

     /**@name Matrix times vector methods */
     //@{
     /** Return <code>y + A * scalar *x</code> in <code>y</code>.
         @pre <code>x</code> must be of size <code>numColumns()</code>
         @pre <code>y</code> must be of size <code>numRows()</code> */
     virtual void times(double scalar,
                        const double * x, double * y) const;
     /// And for scaling
     virtual void times(double scalar,
                        const double * x, double * y,
                        const double * rowScale,
                        const double * columnScale) const;
     /** Return <code>y + x * scalar * A</code> in <code>y</code>.
         @pre <code>x</code> must be of size <code>numRows()</code>
         @pre <code>y</code> must be of size <code>numColumns()</code> */
     virtual void transposeTimes(double scalar,
                                 const double * x, double * y) const;
     /// And for scaling
     virtual void transposeTimes(double scalar,
                                 const double * x, double * y,
                                 const double * rowScale,
                                 const double * columnScale,
                                 double * spare = NULL) const;
     /** Return <code>y - pi * A</code> in <code>y</code>.
         @pre <code>pi</code> must be of size <code>numRows()</code>
         @pre <code>y</code> must be of size <code>numColumns()</code>
     This just does subset (but puts in correct place in y) */
     void transposeTimesSubset( int number,
                                const int * which,
                                const double * pi, double * y,
                                const double * rowScale,
                                const double * columnScale,
                                double * spare = NULL) const;
     /** Return <code>x * scalar * A + y</code> in <code>z</code>.
     Can use y as temporary array (will be empty at end)
     Note - If x packed mode - then z packed mode
     Squashes small elements and knows about ClpSimplex */
     virtual void transposeTimes(const ClpSimplex * model, double scalar,
                                 const CoinIndexedVector * x,
                                 CoinIndexedVector * y,
                                 CoinIndexedVector * z) const;
     /** Return <code>x * scalar * A + y</code> in <code>z</code>.
     Note - If x packed mode - then z packed mode
     This does by column and knows no gaps
     Squashes small elements and knows about ClpSimplex */
     void transposeTimesByColumn(const ClpSimplex * model, double scalar,
                                 const CoinIndexedVector * x,
                                 CoinIndexedVector * y,
                                 CoinIndexedVector * z) const;
     /** Return <code>x * scalar * A + y</code> in <code>z</code>.
     Can use y as temporary array (will be empty at end)
     Note - If x packed mode - then z packed mode
     Squashes small elements and knows about ClpSimplex.
     This version uses row copy*/
     virtual void transposeTimesByRow(const ClpSimplex * model, double scalar,
                                      const CoinIndexedVector * x,
                                      CoinIndexedVector * y,
                                      CoinIndexedVector * z) const;
     /** Return <code>x *A</code> in <code>z</code> but
     just for indices in y.
     Note - z always packed mode */
     virtual void subsetTransposeTimes(const ClpSimplex * model,
                                       const CoinIndexedVector * x,
                                       const CoinIndexedVector * y,
                                       CoinIndexedVector * z) const;
     /** Returns true if can combine transposeTimes and subsetTransposeTimes
         and if it would be faster */
     virtual bool canCombine(const ClpSimplex * model,
                             const CoinIndexedVector * pi) const;
     /// Updates two arrays for steepest
     virtual void transposeTimes2(const ClpSimplex * model,
                                  const CoinIndexedVector * pi1, CoinIndexedVector * dj1,
                                  const CoinIndexedVector * pi2,
                                  CoinIndexedVector * spare,
                                  double referenceIn, double devex,
                                  // Array for exact devex to say what is in reference framework
                                  unsigned int * reference,
                                  double * weights, double scaleFactor);
     /// Updates second array for steepest and does devex weights
     virtual void subsetTimes2(const ClpSimplex * model,
                               CoinIndexedVector * dj1,
                               const CoinIndexedVector * pi2, CoinIndexedVector * dj2,
                               double referenceIn, double devex,
                               // Array for exact devex to say what is in reference framework
                               unsigned int * reference,
                               double * weights, double scaleFactor);
     /// Sets up an effective RHS
     void useEffectiveRhs(ClpSimplex * model);
#if COIN_LONG_WORK
     // For long double versions
     virtual void times(CoinWorkDouble scalar,
                        const CoinWorkDouble * x, CoinWorkDouble * y) const ;
     virtual void transposeTimes(CoinWorkDouble scalar,
                                 const CoinWorkDouble * x, CoinWorkDouble * y) const ;
#endif
//@}

     /**@name Other */
     //@{
     /// Returns CoinPackedMatrix (non const)
     inline CoinPackedMatrix * matrix() const {
          return matrix_;
     }
     /** Just sets matrix_ to NULL so it can be used elsewhere.
         used in GUB
     */
     inline void setMatrixNull() {
          matrix_ = NULL;
     }
     /// Say we want special column copy
     inline void makeSpecialColumnCopy() {
          flags_ |= 16;
     }
     /// Say we don't want special column copy
     void releaseSpecialColumnCopy();
     /// Are there zeros?
     inline bool zeros() const {
          return ((flags_ & 1) != 0);
     }
     /// Do we want special column copy
     inline bool wantsSpecialColumnCopy() const {
          return ((flags_ & 16) != 0);
     }
     /// Flags
     inline int flags() const {
          return flags_;
     }
     /// Sets flags_ correctly
     inline void checkGaps() {
          flags_ = (matrix_->hasGaps()) ? (flags_ | 2) : (flags_ & (~2));
     }
     /// number of active columns (normally same as number of columns)
     inline int numberActiveColumns() const
     { return numberActiveColumns_;}
     /// Set number of active columns (normally same as number of columns)
     inline void setNumberActiveColumns(int value)
     { numberActiveColumns_ = value;}
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpPackedMatrix();
     /** Destructor */
     virtual ~ClpPackedMatrix();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpPackedMatrix(const ClpPackedMatrix&);
     /** The copy constructor from an CoinPackedMatrix. */
     ClpPackedMatrix(const CoinPackedMatrix&);
     /** Subset constructor (without gaps).  Duplicates are allowed
         and order is as given */
     ClpPackedMatrix (const ClpPackedMatrix & wholeModel,
                      int numberRows, const int * whichRows,
                      int numberColumns, const int * whichColumns);
     ClpPackedMatrix (const CoinPackedMatrix & wholeModel,
                      int numberRows, const int * whichRows,
                      int numberColumns, const int * whichColumns);

     /** This takes over ownership (for space reasons) */
     ClpPackedMatrix(CoinPackedMatrix * matrix);

     ClpPackedMatrix& operator=(const ClpPackedMatrix&);
     /// Clone
     virtual ClpMatrixBase * clone() const ;
     /// Copy contents - resizing if necessary - otherwise re-use memory
     virtual void copy(const ClpPackedMatrix * from);
     /** Subset clone (without gaps).  Duplicates are allowed
         and order is as given */
     virtual ClpMatrixBase * subsetClone (
          int numberRows, const int * whichRows,
          int numberColumns, const int * whichColumns) const ;
     /// make special row copy
     void specialRowCopy(ClpSimplex * model, const ClpMatrixBase * rowCopy);
     /// make special column copy
     void specialColumnCopy(ClpSimplex * model);
     /// Correct sequence in and out to give true value
     virtual void correctSequence(const ClpSimplex * model, int & sequenceIn, int & sequenceOut) ;
     //@}
private:
     /// Meat of transposeTimes by column when not scaled
     int gutsOfTransposeTimesUnscaled(const double * COIN_RESTRICT pi,
                                      int * COIN_RESTRICT index,
                                      double * COIN_RESTRICT array,
                                      const double tolerance) const;
     /// Meat of transposeTimes by column when scaled
     int gutsOfTransposeTimesScaled(const double * COIN_RESTRICT pi,
                                    const double * COIN_RESTRICT columnScale,
                                    int * COIN_RESTRICT index,
                                    double * COIN_RESTRICT array,
                                    const double tolerance) const;
     /// Meat of transposeTimes by column when not scaled and skipping
     int gutsOfTransposeTimesUnscaled(const double * COIN_RESTRICT pi,
                                      int * COIN_RESTRICT index,
                                      double * COIN_RESTRICT array,
                                      const unsigned char * status,
                                      const double tolerance) const;
     /** Meat of transposeTimes by column when not scaled and skipping
         and doing part of dualColumn */
     int gutsOfTransposeTimesUnscaled(const double * COIN_RESTRICT pi,
                                      int * COIN_RESTRICT index,
                                      double * COIN_RESTRICT array,
                                      const unsigned char * status,
                                      int * COIN_RESTRICT spareIndex,
                                      double * COIN_RESTRICT spareArray,
                                      const double * COIN_RESTRICT reducedCost,
                                      double & upperTheta,
                                      double & bestPossible,
                                      double acceptablePivot,
                                      double dualTolerance,
                                      int & numberRemaining,
                                      const double zeroTolerance) const;
     /// Meat of transposeTimes by column when scaled and skipping
     int gutsOfTransposeTimesScaled(const double * COIN_RESTRICT pi,
                                    const double * COIN_RESTRICT columnScale,
                                    int * COIN_RESTRICT index,
                                    double * COIN_RESTRICT array,
                                    const unsigned char * status,
                                    const double tolerance) const;
     /// Meat of transposeTimes by row n > K if packed - returns number nonzero
     int gutsOfTransposeTimesByRowGEK(const CoinIndexedVector * COIN_RESTRICT piVector,
                                      int * COIN_RESTRICT index,
                                      double * COIN_RESTRICT output,
                                      int numberColumns,
                                      const double tolerance,
                                      const double scalar) const;
     /// Meat of transposeTimes by row n > 2 if packed - returns number nonzero
     int gutsOfTransposeTimesByRowGE3(const CoinIndexedVector * COIN_RESTRICT piVector,
                                      int * COIN_RESTRICT index,
                                      double * COIN_RESTRICT output,
                                      double * COIN_RESTRICT array2,
                                      const double tolerance,
                                      const double scalar) const;
     /// Meat of transposeTimes by row n > 2 if packed - returns number nonzero
     int gutsOfTransposeTimesByRowGE3a(const CoinIndexedVector * COIN_RESTRICT piVector,
                                      int * COIN_RESTRICT index,
                                      double * COIN_RESTRICT output,
                                      int * COIN_RESTRICT lookup,
                                      char * COIN_RESTRICT marked,
                                      const double tolerance,
                                      const double scalar) const;
     /// Meat of transposeTimes by row n == 2 if packed
     void gutsOfTransposeTimesByRowEQ2(const CoinIndexedVector * piVector, CoinIndexedVector * output,
                                       CoinIndexedVector * spareVector, const double tolerance, const double scalar) const;
     /// Meat of transposeTimes by row n == 1 if packed
     void gutsOfTransposeTimesByRowEQ1(const CoinIndexedVector * piVector, CoinIndexedVector * output,
                                       const double tolerance, const double scalar) const;
     /// Gets rid of special copies
     void clearCopies();


protected:
     /// Check validity
     void checkFlags(int type) const;
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Data
     CoinPackedMatrix * matrix_;
     /// number of active columns (normally same as number of columns)
     int numberActiveColumns_;
     /** Flags -
         1 - has zero elements
         2 - has gaps
         4 - has special row copy
         8 - has special column copy
         16 - wants special column copy
     */
     mutable int flags_;
     /// Special row copy
     ClpPackedMatrix2 * rowCopy_;
     /// Special column copy
     ClpPackedMatrix3 * columnCopy_;
     //@}
};
#ifdef THREAD
#include <pthread.h>
typedef struct {
     double acceptablePivot;
     const ClpSimplex * model;
     double * spare;
     int * spareIndex;
     double * arrayTemp;
     int * indexTemp;
     int * numberInPtr;
     double * bestPossiblePtr;
     double * upperThetaPtr;
     int * posFreePtr;
     double * freePivotPtr;
     int * numberOutPtr;
     const unsigned short * count;
     const double * pi;
     const CoinBigIndex * rowStart;
     const double * element;
     const unsigned short * column;
     int offset;
     int numberInRowArray;
     int numberLook;
} dualColumn0Struct;
#endif
class ClpPackedMatrix2 {

public:
     /**@name Useful methods */
     //@{
     /** Return <code>x * -1 * A in <code>z</code>.
     Note - x packed and z will be packed mode
     Squashes small elements and knows about ClpSimplex */
     void transposeTimes(const ClpSimplex * model,
                         const CoinPackedMatrix * rowCopy,
                         const CoinIndexedVector * x,
                         CoinIndexedVector * spareArray,
                         CoinIndexedVector * z) const;
     /// Returns true if copy has useful information
     inline bool usefulInfo() const {
          return rowStart_ != NULL;
     }
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpPackedMatrix2();
     /** Constructor from copy. */
     ClpPackedMatrix2(ClpSimplex * model, const CoinPackedMatrix * rowCopy);
     /** Destructor */
     virtual ~ClpPackedMatrix2();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpPackedMatrix2(const ClpPackedMatrix2&);
     ClpPackedMatrix2& operator=(const ClpPackedMatrix2&);
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Number of blocks
     int numberBlocks_;
     /// Number of rows
     int numberRows_;
     /// Column offset for each block (plus one at end)
     int * offset_;
     /// Counts of elements in each part of row
     mutable unsigned short * count_;
     /// Row starts
     mutable CoinBigIndex * rowStart_;
     /// columns within block
     unsigned short * column_;
     /// work arrays
     double * work_;
#ifdef THREAD
     pthread_t * threadId_;
     dualColumn0Struct * info_;
#endif
     //@}
};
typedef struct {
     CoinBigIndex startElements_; // point to data
     int startIndices_; // point to column_
     int numberInBlock_;
     int numberPrice_; // at beginning
     int numberElements_; // number elements per column
} blockStruct;
class ClpPackedMatrix3 {

public:
     /**@name Useful methods */
     //@{
     /** Return <code>x * -1 * A in <code>z</code>.
     Note - x packed and z will be packed mode
     Squashes small elements and knows about ClpSimplex */
     void transposeTimes(const ClpSimplex * model,
                         const double * pi,
                         CoinIndexedVector * output) const;
     /// Updates two arrays for steepest
     void transposeTimes2(const ClpSimplex * model,
                          const double * pi, CoinIndexedVector * dj1,
                          const double * piWeight,
                          double referenceIn, double devex,
                          // Array for exact devex to say what is in reference framework
                          unsigned int * reference,
                          double * weights, double scaleFactor);
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpPackedMatrix3();
     /** Constructor from copy. */
     ClpPackedMatrix3(ClpSimplex * model, const CoinPackedMatrix * columnCopy);
     /** Destructor */
     virtual ~ClpPackedMatrix3();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpPackedMatrix3(const ClpPackedMatrix3&);
     ClpPackedMatrix3& operator=(const ClpPackedMatrix3&);
     //@}
     /**@name Sort methods */
     //@{
     /** Sort blocks */
     void sortBlocks(const ClpSimplex * model);
     /// Swap one variable
     void swapOne(const ClpSimplex * model, const ClpPackedMatrix * matrix,
                  int iColumn);
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Number of blocks
     int numberBlocks_;
     /// Number of columns
     int numberColumns_;
     /// Column indices and reverse lookup (within block)
     int * column_;
     /// Starts for odd/long vectors
     CoinBigIndex * start_;
     /// Rows
     int * row_;
     /// Elements
     double * element_;
     /// Blocks (ordinary start at 0 and go to first block)
     blockStruct * block_;
     //@}
};

#endif
