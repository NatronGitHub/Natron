/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpDummyMatrix_H
#define ClpDummyMatrix_H


#include "CoinPragma.hpp"

#include "ClpMatrixBase.hpp"

/** This implements a dummy matrix as derived from ClpMatrixBase.
    This is so you can do ClpPdco but may come in useful elsewhere.
    It just has dimensions but no data
*/


class ClpDummyMatrix : public ClpMatrixBase {

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
          return numberElements_;
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
     virtual const int * getIndices() const;

     virtual const CoinBigIndex * getVectorStarts() const;
     /** The lengths of the major-dimension vectors. */
     virtual const int * getVectorLengths() const;

     /** Delete the columns whose indices are listed in <code>indDel</code>. */
     virtual void deleteCols(const int numDel, const int * indDel);
     /** Delete the rows whose indices are listed in <code>indDel</code>. */
     virtual void deleteRows(const int numDel, const int * indDel);
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
     /// Allow any parts of a created CoinMatrix to be deleted
     /// Allow any parts of a created CoinPackedMatrix to be deleted
     virtual void releasePackedMatrix() const {}
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
                                 const double * columnScale) const;

     using ClpMatrixBase::transposeTimes ;
     /** Return <code>x * scalar * A + y</code> in <code>z</code>.
     Can use y as temporary array (will be empty at end)
     Note - If x packed mode - then z packed mode */
     virtual void transposeTimes(const ClpSimplex * model, double scalar,
                                 const CoinIndexedVector * x,
                                 CoinIndexedVector * y,
                                 CoinIndexedVector * z) const;
     /** Return <code>x *A</code> in <code>z</code> but
     just for indices in y.
     Note - If x packed mode - then z packed mode
     Squashes small elements and knows about ClpSimplex */
     virtual void subsetTransposeTimes(const ClpSimplex * model,
                                       const CoinIndexedVector * x,
                                       const CoinIndexedVector * y,
                                       CoinIndexedVector * z) const;
     //@}

     /**@name Other */
     //@{
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpDummyMatrix();
     /// Constructor with data
     ClpDummyMatrix(int numberColumns, int numberRows,
                    int numberElements);
     /** Destructor */
     virtual ~ClpDummyMatrix();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpDummyMatrix(const ClpDummyMatrix&);
     /** The copy constructor from an CoinDummyMatrix. */
     ClpDummyMatrix(const CoinPackedMatrix&);

     ClpDummyMatrix& operator=(const ClpDummyMatrix&);
     /// Clone
     virtual ClpMatrixBase * clone() const ;
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Number of rows
     int numberRows_;
     /// Number of columns
     int numberColumns_;
     /// Number of elements
     int numberElements_;

     //@}
};

#endif
