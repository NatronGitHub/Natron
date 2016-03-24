/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cstdio>

#include "CoinPragma.hpp"

#include "ClpSimplex.hpp"
#include "ClpDummyMatrix.hpp"
#include "ClpFactorization.hpp"
#include "ClpMessage.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpDummyMatrix::ClpDummyMatrix ()
     : ClpMatrixBase()
{
     setType(14);
     numberRows_ = 0;
     numberColumns_ = 0;
     numberElements_ = 0;
}

/* Constructor from data */
ClpDummyMatrix::ClpDummyMatrix(int numberColumns, int numberRows,
                               int numberElements)
     : ClpMatrixBase()
{
     setType(14);
     numberRows_ = numberRows;
     numberColumns_ = numberColumns;
     numberElements_ = numberElements;
}
//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpDummyMatrix::ClpDummyMatrix (const ClpDummyMatrix & rhs)
     : ClpMatrixBase(rhs)
{
     numberRows_ = rhs.numberRows_;
     numberColumns_ = rhs.numberColumns_;
     numberElements_ = rhs.numberElements_;
}

ClpDummyMatrix::ClpDummyMatrix (const CoinPackedMatrix & )
     : ClpMatrixBase()
{
     std::cerr << "Constructor from CoinPackedMatrix nnot supported - ClpDummyMatrix" << std::endl;
     abort();
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpDummyMatrix::~ClpDummyMatrix ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpDummyMatrix &
ClpDummyMatrix::operator=(const ClpDummyMatrix& rhs)
{
     if (this != &rhs) {
          ClpMatrixBase::operator=(rhs);
          numberRows_ = rhs.numberRows_;
          numberColumns_ = rhs.numberColumns_;
          numberElements_ = rhs.numberElements_;
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpMatrixBase * ClpDummyMatrix::clone() const
{
     return new ClpDummyMatrix(*this);
}

/* Returns a new matrix in reverse order without gaps */
ClpMatrixBase *
ClpDummyMatrix::reverseOrderedCopy() const
{
     std::cerr << "reverseOrderedCopy not supported - ClpDummyMatrix" << std::endl;
     abort();
     return NULL;
}
//unscaled versions
void
ClpDummyMatrix::times(double ,
                      const double * , double * ) const
{
     std::cerr << "times not supported - ClpDummyMatrix" << std::endl;
     abort();
}
void
ClpDummyMatrix::transposeTimes(double ,
                               const double * , double * ) const
{
     std::cerr << "transposeTimes not supported - ClpDummyMatrix" << std::endl;
     abort();
}
void
ClpDummyMatrix::times(double ,
                      const double * , double * ,
                      const double * ,
                      const double * ) const
{
     std::cerr << "timesnot supported - ClpDummyMatrix" << std::endl;
     abort();
}
void
ClpDummyMatrix::transposeTimes( double,
                                const double * , double * ,
                                const double * ,
                                const double * ) const
{
     std::cerr << "transposeTimesnot supported - ClpDummyMatrix" << std::endl;
     abort();
}
/* Return <code>x * A + y</code> in <code>z</code>.
	Squashes small elements and knows about ClpSimplex */
void
ClpDummyMatrix::transposeTimes(const ClpSimplex * , double ,
                               const CoinIndexedVector * ,
                               CoinIndexedVector * ,
                               CoinIndexedVector * ) const
{
     std::cerr << "transposeTimes not supported - ClpDummyMatrix" << std::endl;
     abort();
}
/* Return <code>x *A in <code>z</code> but
   just for indices in y */
void
ClpDummyMatrix::subsetTransposeTimes(const ClpSimplex * ,
                                     const CoinIndexedVector * ,
                                     const CoinIndexedVector * ,
                                     CoinIndexedVector * ) const
{
     std::cerr << "subsetTransposeTimes not supported - ClpDummyMatrix" << std::endl;
     abort();
}
/// returns number of elements in column part of basis,
CoinBigIndex
ClpDummyMatrix::countBasis(const int * ,
                           int & )
{
     std::cerr << "countBasis not supported - ClpDummyMatrix" << std::endl;
     abort();
     return 0;
}
void
ClpDummyMatrix::fillBasis(ClpSimplex * ,
                          const int * ,
                          int & ,
                          int * , int * ,
                          int * , int * ,
                          CoinFactorizationDouble * )
{
     std::cerr << "fillBasis not supported - ClpDummyMatrix" << std::endl;
     abort();
}
/* Unpacks a column into an CoinIndexedvector
 */
void
ClpDummyMatrix::unpack(const ClpSimplex * , CoinIndexedVector * ,
                       int ) const
{
     std::cerr << "unpack not supported - ClpDummyMatrix" << std::endl;
     abort();
}
/* Unpacks a column into an CoinIndexedvector
** in packed foramt
Note that model is NOT const.  Bounds and objective could
be modified if doing column generation (just for this variable) */
void
ClpDummyMatrix::unpackPacked(ClpSimplex * ,
                             CoinIndexedVector * ,
                             int ) const
{
     std::cerr << "unpackPacked not supported - ClpDummyMatrix" << std::endl;
     abort();
}
/* Adds multiple of a column into an CoinIndexedvector
      You can use quickAdd to add to vector */
void
ClpDummyMatrix::add(const ClpSimplex * , CoinIndexedVector * ,
                    int , double ) const
{
     std::cerr << "add not supported - ClpDummyMatrix" << std::endl;
     abort();
}
/* Adds multiple of a column into an array */
void
ClpDummyMatrix::add(const ClpSimplex * , double * ,
                    int , double ) const
{
     std::cerr << "add not supported - ClpDummyMatrix" << std::endl;
     abort();
}

// Return a complete CoinPackedMatrix
CoinPackedMatrix *
ClpDummyMatrix::getPackedMatrix() const
{
     std::cerr << "getPackedMatrix not supported - ClpDummyMatrix" << std::endl;
     abort();
     return NULL;
}
/* A vector containing the elements in the packed matrix. Note that there
   might be gaps in this list, entries that do not belong to any
   major-dimension vector. To get the actual elements one should look at
   this vector together with vectorStarts and vectorLengths. */
const double *
ClpDummyMatrix::getElements() const
{
     std::cerr << "getElements not supported - ClpDummyMatrix" << std::endl;
     abort();
     return NULL;
}

const CoinBigIndex *
ClpDummyMatrix::getVectorStarts() const
{
     std::cerr << "getVectorStarts not supported - ClpDummyMatrix" << std::endl;
     abort();
     return NULL;
}
/* The lengths of the major-dimension vectors. */
const int *
ClpDummyMatrix::getVectorLengths() const
{
     std::cerr << "get VectorLengths not supported - ClpDummyMatrix" << std::endl;
     abort();
     return NULL;
}
/* Delete the columns whose indices are listed in <code>indDel</code>. */
void ClpDummyMatrix::deleteCols(const int , const int * )
{
     std::cerr << "deleteCols not supported - ClpDummyMatrix" << std::endl;
     abort();
}
/* Delete the rows whose indices are listed in <code>indDel</code>. */
void ClpDummyMatrix::deleteRows(const int , const int * )
{
     std::cerr << "deleteRows not supported - ClpDummyMatrix" << std::endl;
     abort();
}
const int *
ClpDummyMatrix::getIndices() const
{
     std::cerr << "getIndices not supported - ClpDummyMatrix" << std::endl;
     abort();
     return NULL;
}
