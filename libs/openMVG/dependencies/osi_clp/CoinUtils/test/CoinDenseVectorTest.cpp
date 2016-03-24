// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

#include "CoinDenseVector.hpp"
#include "CoinFloatEqual.hpp"

//--------------------------------------------------------------------------
template <typename T> void
CoinDenseVectorUnitTest(T dummy)
{

  // Test default constructor
  {
    CoinDenseVector<T> r;
    assert( r.getElements() == 0 );
    assert( r.getNumElements() == 0 );
  }
    const int ne = 4;
    T el[ne] = { 10, 40, 1, 50 };

    // Create vector and set its value
    CoinDenseVector<T> r(ne,el);

    // access each element
    assert( r.getElements()[0]==10. );
    assert( r.getElements()[1]==40. );
    assert( r.getElements()[2]== 1. );
    assert( r.getElements()[3]==50. );

    // Test norms etc
    assert( r.sum() == 10.+40.+1.+50. );
    assert( r.oneNorm() == 101.0);
    // std namespace removed to compile with Microsoft Visual C++ V6
    //assert( r.twoNorm() == /*std::*/sqrt(100.0 + 1600. + 1. + 2500.));
    CoinRelFltEq eq;
    assert( eq(r.twoNorm() , /*std::*/sqrt(100.0 + 1600. + 1. + 2500.)));
    assert( r.infNorm() == 50.);
    assert(r[0]+r[1]+r[2]+r[3]==101.);

    // Test for equality
    CoinDenseVector<T> r1;
    r1=r;
    assert( r1.getElements()[0]==10. );
    assert( r1.getElements()[1]==40. );
    assert( r1.getElements()[2]== 1. );
    assert( r1.getElements()[3]==50. );

    // Add dense vectors.
    CoinDenseVector<T> add = r + r1;
    assert( add[0] == 10.+10. );
    assert( add[1] == 40.+40. );
    assert( add[2] ==  1.+ 1. );
    assert( add[3] == 50.+50. );

    // Similarly for copy constructor and subtraction and multiplication
    CoinDenseVector<T> r2(r);
    CoinDenseVector<T> diff = r - r2;
    assert(diff.sum() == 0.0);

    CoinDenseVector<T> mult = r * r2;
    assert( mult[0] == 10.*10. );
    assert( mult[1] == 40.*40. );
    assert( mult[2] ==  1.* 1. );
    assert( mult[3] == 50.*50. );

   // and division.
    CoinDenseVector<T> div = r / r1;
    assert(div.sum() == 4.0);

}

template void CoinDenseVectorUnitTest<float>(float);
template void CoinDenseVectorUnitTest<double>(double);
//template void CoinDenseVectorUnitTest<int>(int);
