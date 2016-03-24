/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpNetworkMatrix_H
#define ClpNetworkMatrix_H


#include "CoinPragma.hpp"

#include "ClpMatrixBase.hpp"

/** This implements a simple network matrix as derived from ClpMatrixBase.

If you want more sophisticated version then you could inherit from this.
Also you might want to allow networks with gain */

class ClpNetworkMatrix : public ClpMatrixBase {

public:
     /**@name Useful methods */
     //@{
     /// Return a complete CoinPackedMatrix
     virtual CoinPackedMatrix * getPackedMatrix() const;
     /** Whether the packed matrix is column major ordered or not. */
     virtual bool isColOrdered() const {
          return true;
     }
     /** Number of entries in the packed matrix. */
     virtual  CoinBigIndex getNumElements() const {
          return 2 * numberColumns_;
     }
     /** Number of columns. */
     virtual int getNumCols() const {
          return numberColumns_;
     }
     /** Number of rows. */
     virtual int getNumRows() const {
          return numberRows_;
     }

     /** A vector containing the elements in the packed matrix. Note that there
      might be gaps in this list, entries that do not belong to any
      major-dimension vector. To get the actual elements one should look at
      this vector together with vectorStarts and vectorLengths. */
     virtual const double * getElements() const;
     /** A vector containing the minor indices of the elements in the packed
          matrix. Note that there might be gaps in this list, entries that do not
          belong to any major-dimension vector. To get the actual elements one
          should look at this vector together with vectorStarts and
          vectorLengths. */
     virtual const int * getIndices() const {
          return indices_;
     }

     virtual const CoinBigIndex * getVectorStarts() const;
     /** The lengths of the major-dimension vectors. */
     virtual const int * getVectorLengths() const;

     /** Delete the columns whose indices are listed in <code>indDel</code>. */
     virtual void deleteCols(const int numDel, const int * indDel);
     /** Delete the rows whose indices are listed in <code>indDel</code>. */
     virtual void deleteRows(const int numDel, const int * indDel);
     /// Append Columns
     virtual void appendCols(int number, const CoinPackedVectorBase * const * columns);
     /// Append Rows
     virtual void appendRows(int number, const CoinPackedVectorBase * const * rows);
#ifndef SLIM_CLP
     /** Append a set of rows/columns to the end of the matrix. Returns number of errors
         i.e. if any of the new rows/columns contain an index that's larger than the
         number of columns-1/rows-1 (if numberOther>0) or duplicates
         If 0 then rows, 1 if columns */
     virtual int appendMatrix(int number, int type,
                              const CoinBigIndex * starts, const int * index,
                              const double * element, int numberOther = -1);
#endif
     /** Returns a new matrix in reverse order without gaps */
     virtual ClpMatrixBase * reverseOrderedCopy() const;
     /// Returns number of elements in column part of basis
     virtual CoinBigIndex countBasis(
          const int * whichColumn,
          int & numberColumnBasic);
     /// Fills in column part of basis
     virtual void fillBasis(ClpSimplex * model,
                            const int * whichColumn,
                            int & numberColumnBasic,
                            int * row, int * start,
                            int * rowCount, int * columnCount,
                            CoinFactorizationDouble * element);
     /** Given positive integer weights for each row fills in sum of weights
         for each column (and slack).
         Returns weights vector
     */
     virtual CoinBigIndex * dubiousWeights(const ClpSimplex * model, int * inputWeights) const;
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
      ** in packed format
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
     /// Allow any parts of a created CoinMatrix to be deleted
     virtual void releasePackedMatrix() const ;
     /// Says whether it can do partial pricing
     virtual bool canDoPartialPricing() const;
     /// Partial pricing
     virtual void partialPricing(ClpSimplex * model, double start, double end,
                                 int & bestSequence, int & numberWanted);
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
                                 const double * columnScale, double * spare = NULL) const;
     /** Return <code>x * scalar * A + y</code> in <code>z</code>.
     Can use y as temporary array (will be empty at end)
     Note - If x packed mode - then z packed mode
     Squashes small elements and knows about ClpSimplex */
     virtual void transposeTimes(const ClpSimplex * model, double scalar,
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
     //@}

     /**@name Other */
     //@{
     /// Return true if really network, false if has slacks
     inline bool trueNetwork() const {
          return trueNetwork_;
     }
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpNetworkMatrix();
     /** Constructor from two arrays */
     ClpNetworkMatrix(int numberColumns, const int * head,
                      const int * tail);
     /** Destructor */
     virtual ~ClpNetworkMatrix();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpNetworkMatrix(const ClpNetworkMatrix&);
     /** The copy constructor from an CoinNetworkMatrix. */
     ClpNetworkMatrix(const CoinPackedMatrix&);

     ClpNetworkMatrix& operator=(const ClpNetworkMatrix&);
     /// Clone
     virtual ClpMatrixBase * clone() const ;
     /** Subset constructor (without gaps).  Duplicates are allowed
         and order is as given */
     ClpNetworkMatrix (const ClpNetworkMatrix & wholeModel,
                       int numberRows, const int * whichRows,
                       int numberColumns, const int * whichColumns);
     /** Subset clone (without gaps).  Duplicates are allowed
         and order is as given */
     virtual ClpMatrixBase * subsetClone (
          int numberRows, const int * whichRows,
          int numberColumns, const int * whichColumns) const ;
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// For fake CoinPackedMatrix
     mutable CoinPackedMatrix * matrix_;
     mutable int * lengths_;
     /// Data -1, then +1 rows in pairs (row==-1 if one entry)
     int * indices_;
     /// Number of rows
     int numberRows_;
     /// Number of columns
     int numberColumns_;
     /// True if all entries have two elements
     bool trueNetwork_;

     //@}
};

#endif
