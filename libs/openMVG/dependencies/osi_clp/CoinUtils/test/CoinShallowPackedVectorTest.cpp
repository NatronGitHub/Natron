// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

#include "CoinPragma.hpp"
#include "CoinFinite.hpp"
#include "CoinFloatEqual.hpp"
#include "CoinShallowPackedVector.hpp"
#include "CoinPackedVector.hpp"

void
CoinShallowPackedVectorUnitTest()
{
  CoinRelFltEq eq;
  int i;
  // Test default constructor
  {
    CoinShallowPackedVector r;
    assert( r.indices_==NULL );
    assert( r.elements_==NULL );
    assert( r.nElements_==0 );
  }

  // Test set and get methods
  const int ne = 4;
  int inx[ne] = { 1, 3, 4, 7 };
  double el[ne] = { 1.2, 3.4, 5.6, 7.8 };
  {
    CoinShallowPackedVector r;    
    assert( r.getNumElements()==0 );
    
    // Test setting/getting elements with int* & double* vectors
    r.setVector( ne, inx, el );
    assert( r.getNumElements()==ne );
    for ( i=0; i<ne; i++ ) {
      assert( r.getIndices()[i]  == inx[i] );
      assert( r.getElements()[i] == el[i]  );
    }
    assert ( r.getMaxIndex()==7 );
    assert ( r.getMinIndex()==1 );

    // try to clear it
    r.clear();
    assert( r.indices_==NULL );
    assert( r.elements_==NULL );
    assert( r.nElements_==0 );

    // Test setting/getting elements with indices out of order  
    const int ne2 = 5;
    int inx2[ne2] = { 2, 4, 8, 14, 3 };
    double el2[ne2] = { 2.2, 4.4, 6.6, 8.8, 3.3 };
 
    r.setVector(ne2,inx2,el2);
    
    assert( r.getNumElements()==ne2 );    
    for (i = 0; i < ne2; ++i) {
       assert( r.getIndices()[i]==inx2[i] );
       assert( r.getElements()[i]==el2[i] );
    }
    
    assert ( r.getMaxIndex()==14 );
    assert ( r.getMinIndex()==2 );
    // try to call it once more
    assert ( r.getMaxIndex()==14 );
    assert ( r.getMinIndex()==2 );

    CoinShallowPackedVector r1(ne2,inx2,el2);
    assert( r == r1 );

    // assignment operator
    r1.clear();
    r1 = r;
    assert( r == r1 );

    // assignment from packed vector
    CoinPackedVector pv1(ne2,inx2,el2);
    r1 = pv1;
    assert( r == r1 );

    // construction
    CoinShallowPackedVector r2(r1);
    assert( r2 == r );
    
    // construction from packed vector
    CoinShallowPackedVector r3(pv1);
    assert( r3 == r );

    // test duplicate indices
    {
      const int ne3 = 4;
      int inx3[ne3] = { 2, 4, 2, 3 };
      double el3[ne3] = { 2.2, 4.4, 8.8, 6.6 };
      r.setVector(ne3,inx3,el3, false);
      assert(r.testForDuplicateIndex() == false);
      bool errorThrown = false;
      try {
        r.setTestForDuplicateIndex(true);
      }
      catch (CoinError& e) {
        errorThrown = true;
      }
      assert( errorThrown );

      r.clear();
      errorThrown = false;
      try {
	 r.setVector(ne3,inx3,el3);
      }
      catch (CoinError& e) {
        errorThrown = true;
      }
      assert( errorThrown );
	 
      errorThrown = false;
      try {
	 CoinShallowPackedVector r1(ne3,inx3,el3);
      }
      catch (CoinError& e) {
	 errorThrown = true;
      }
      assert( errorThrown );
    } 
    
  } 

  // Test copy constructor and assignment operator
  {
    CoinShallowPackedVector rhs;
    {
      CoinShallowPackedVector r;
      {
        CoinShallowPackedVector rC1(r);      
        assert( 0==r.getNumElements() );
        assert( 0==rC1.getNumElements() );
        
        r.setVector( ne, inx, el ); 
        
        assert( ne==r.getNumElements() );
        assert( 0==rC1.getNumElements() ); 
      }
      
      CoinShallowPackedVector rC2(r);   
      
      assert( ne==r.getNumElements() );
      assert( ne==rC2.getNumElements() );
      
      for ( i=0; i<ne; i++ ) {
        assert( r.getIndices()[i] == rC2.getIndices()[i] );
        assert( r.getElements()[i] == rC2.getElements()[i] );
      }

      rhs=rC2;
    }
    // Test that rhs has correct values even though lhs has gone out of scope
    assert( rhs.getNumElements()==ne );
    
    for ( i=0; i<ne; i++ ) {
      assert( inx[i] == rhs.getIndices()[i] );
      assert(  el[i] == rhs.getElements()[i] );
    } 
  }

  // Test operator==
  {
    CoinShallowPackedVector v1,v2;
    assert( v1==v2 );
    assert( v2==v1 );
    assert( v1==v1 );
    assert( !(v1!=v2) );
    
    v1.setVector( ne, inx, el );
    assert ( !(v1==v2) );
    assert ( v1!=v2 );

    CoinShallowPackedVector v3(v1);
    assert( v3==v1 );
    assert( v3!=v2 );

    CoinShallowPackedVector v4(v2);
    assert( v4!=v1 );
    assert( v4==v2 );
  }

 

  {
    // Test operator[] and isExistingIndex()
    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinShallowPackedVector r;
    assert( r[1]==0. );

    r.setVector(ne,inx,el);

    assert( r[-1]==0. );
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);
    assert( r[ 5]==0. );
    assert(  r.isExistingIndex(2) );
    assert( !r.isExistingIndex(3) );

    assert( !r.isExistingIndex(-1) );
    assert(  r.isExistingIndex(0) );
    assert( !r.isExistingIndex(3) );
    assert(  r.isExistingIndex(4) );
    assert( !r.isExistingIndex(5) );
    
    assert ( r.getMaxIndex()==4 );
    assert ( r.getMinIndex()==0 );
  }
  
  // Test that attemping to get min/max index of a 0,
  // length vector 
  {
    CoinShallowPackedVector nullVec;
    assert( nullVec.getMaxIndex() == -COIN_INT_MAX/*0*/ );
    assert( nullVec.getMinIndex() == COIN_INT_MAX/*0*/ );
  } 

  {
     // test dense vector
     const int ne = 4;
     int inx[ne] =   {  1,   4,  0,   2 };
     double el[ne] = { 10., 40., 1., 50. };
     CoinShallowPackedVector r;
     r.setVector(ne,inx,el);
     double * dense = r.denseVector(6);
     assert(dense[0] == 1.);
     assert(dense[1] == 10.);
     assert(dense[2] == 50.);
     assert(dense[3] == 0.);
     assert(dense[4] == 40.);
     assert(dense[5] == 0.);
     delete[] dense;

     // try once more
     dense = r.denseVector(7);
     assert(dense[0] == 1.);
     assert(dense[1] == 10.);
     assert(dense[2] == 50.);
     assert(dense[3] == 0.);
     assert(dense[4] == 40.);
     assert(dense[5] == 0.);
     assert(dense[6] == 0.);
     delete[] dense;
     
  }
     
     

  
#if 0
  // what happens when someone sets 
  // the number of elements to be a negative number
  {    
    const int ne = 4;
    int inx1[ne] = { 1, 3, 4, 7 };
    double el1[ne] = { 1.2, 3.4, 5.6, 7.8 };
    CoinShallowPackedVector v1;
    v1.setVector(-ne,inx1,el1);
  }
#endif

  
  // Test adding vectors
  {    
    const int ne1 = 5;
    int inx1[ne1]   = { 1,  3,  4,  7,  5  };
    double el1[ne1] = { 1., 5., 6., 2., 9. };
    const int ne2 = 4;
    int inx2[ne2] =   { 7,  4,  2,  1  };
    double el2[ne2] = { 7., 4., 2., 1. };
    CoinShallowPackedVector v1;
    v1.setVector(ne1,inx1,el1);
    CoinShallowPackedVector v2;
    v2.setVector(ne2,inx2,el2);
    CoinPackedVector r = v1 + v2;

    const int ner = 6;
    int inxr[ner] =   {    1,     2,     3,     4,     5,     7  };
    double elr[ner] = { 1.+1., 0.+2., 5.+0., 6.+4., 9.+0., 2.+7. };
    CoinPackedVector rV;
    rV.setVector(ner,inxr,elr);
    assert( rV != r );
    assert( r.isEquivalent(rV) );
    
    CoinPackedVector p1=v1+3.1415;
    for ( i=0; i<p1.getNumElements(); i++ )
      assert( eq( p1.getElements()[i], v1.getElements()[i]+3.1415) );

    CoinPackedVector p2=(-3.1415) + p1;
    assert( p2.isEquivalent(v1) );
  } 
  
  // Test subtracting vectors
  {    
    const int ne1 = 5;
    int inx1[ne1]   = { 1,  3,  4,  7,  5  };
    double el1[ne1] = { 1., 5., 6., 2., 9. };
    const int ne2 = 4;
    int inx2[ne2] =   { 7,  4,  2,  1  };
    double el2[ne2] = { 7., 4., 2., 1. };
    CoinShallowPackedVector v1;
    v1.setVector(ne1,inx1,el1);
    CoinShallowPackedVector v2;
    v2.setVector(ne2,inx2,el2);
    CoinPackedVector r = v1 - v2;

    const int ner = 6;
    int inxr[ner] =   {    1,     2,     3,     4,     5,     7  };
    double elr[ner] = { 1.-1., 0.-2., 5.-0., 6.-4., 9.-0., 2.-7. };
    CoinPackedVector rV;
    rV.setVector(ner,inxr,elr);
    assert( r.isEquivalent(rV) );  
    
    CoinPackedVector p1=v1-3.1415;
    for ( i=0; i<p1.getNumElements(); i++ )
      assert( eq( p1.getElements()[i], v1.getElements()[i]-3.1415) );
  } 
  
  // Test multiplying vectors
  {    
    const int ne1 = 5;
    int inx1[ne1]   = { 1,  3,  4,  7,  5  };
    double el1[ne1] = { 1., 5., 6., 2., 9. };
    const int ne2 = 4;
    int inx2[ne2] =   { 7,  4,  2,  1  };
    double el2[ne2] = { 7., 4., 2., 1. };
    CoinShallowPackedVector v1;
    v1.setVector(ne1,inx1,el1);
    CoinShallowPackedVector v2;
    v2.setVector(ne2,inx2,el2);
    CoinPackedVector r = v1 * v2;

    const int ner = 6;
    int inxr[ner] =   {    1,     2,     3,     4,     5,     7  };
    double elr[ner] = { 1.*1., 0.*2., 5.*0., 6.*4., 9.*0., 2.*7. };
    CoinPackedVector rV;
    rV.setVector(ner,inxr,elr);
    assert( r.isEquivalent(rV) );

    CoinPackedVector p1=v1*3.3;
    for ( i=0; i<p1.getNumElements(); i++ )
      assert( eq( p1.getElements()[i], v1.getElements()[i]*3.3) );
    
    CoinPackedVector p2=(1./3.3) * p1;
    assert( p2.isEquivalent(v1) );
  } 
  
  // Test dividing vectors
  {    
    const int ne1 = 3;
    int inx1[ne1]   = { 1,  4,  7  };
    double el1[ne1] = { 1., 6., 2. };
    const int ne2 = 4;
    int inx2[ne2] =   { 7,  4,  2,  1  };
    double el2[ne2] = { 7., 4., 2., 1. };
    CoinShallowPackedVector v1;
    v1.setVector(ne1,inx1,el1);
    CoinShallowPackedVector v2;
    v2.setVector(ne2,inx2,el2);
    CoinPackedVector r = v1 / v2;

    const int ner = 4;
    int inxr[ner] =   {    1,     2,      4,     7  };
    double elr[ner] = { 1./1., 0./2.,  6./4., 2./7. };
    CoinPackedVector rV;
    rV.setVector(ner,inxr,elr);
    assert( r.isEquivalent(rV) );
        
    CoinPackedVector p1=v1/3.1415;
    for ( i=0; i<p1.getNumElements(); i++ )
      assert( eq( p1.getElements()[i], v1.getElements()[i]/3.1415) );
  }
   
  // Test sum
  { 
    CoinShallowPackedVector s;
    assert( s.sum() == 0 );

    int inx = 25;
    double value = 45.;
    s.setVector(1, &inx, &value);
    assert(s.sum()==45.);

    const int ne1 = 5;
    int inx1[ne1]   = { 10,  3,  4,  7,  5  };
    double el1[ne1] = { 1., 5., 6., 2., 9. };
    s.setVector(ne1,inx1,el1);

    assert(s.sum()==1.+5.+6.+2.+9.);
  }
  
  // Just another interesting test
  {    
    // Create numerator vector
    const int ne1 = 2;
    int inx1[ne1]   = { 1,  4  };
    double el1[ne1] = { 1., 6. };
    CoinShallowPackedVector v1(ne1,inx1,el1);

    // create denominator vector
    const int ne2 = 3;
    int inx2[ne2] =   { 1,  2,  4 };
    double el2[ne2] = { 1., 7., 4.};
    CoinShallowPackedVector v2(ne2,inx2,el2);

    // Compute ratio
    CoinPackedVector ratio = v1 / v2;

    // Sort ratios
    ratio.sortIncrElement();

    // Test that the sort really worked
    assert( ratio.getElements()[0] == 0.0/7.0 );
    assert( ratio.getElements()[1] == 1.0/1.0 );
    assert( ratio.getElements()[2] == 6.0/4.0 );

    // Get numerator of of sorted ratio vector
    assert( v1[ ratio.getIndices()[0] ] == 0.0 );
    assert( v1[ ratio.getIndices()[1] ] == 1.0 );
    assert( v1[ ratio.getIndices()[2] ] == 6.0 );

    // Get denominator of of sorted ratio vector
    assert( v2[ ratio.getIndices()[0] ] == 7.0 );
    assert( v2[ ratio.getIndices()[1] ] == 1.0 );
    assert( v2[ ratio.getIndices()[2] ] == 4.0 );
  }

  {
    // Test that sample usage works

    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinShallowPackedVector r(ne,inx,el);

    assert( r.getIndices()[0]== 1  );
    assert( r.getElements()[0]==10. );
    assert( r.getIndices()[1]== 4  );
    assert( r.getElements()[1]==40. );
    assert( r.getIndices()[2]== 0  );
    assert( r.getElements()[2]== 1. );
    assert( r.getIndices()[3]== 2  );
    assert( r.getElements()[3]==50. );

    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);

    CoinShallowPackedVector r1;
    r1=r;
    assert( r==r1 );

    CoinPackedVector add = r + r1;
    assert( add[0] ==  1.+ 1. );
    assert( add[1] == 10.+10. );
    assert( add[2] == 50.+50. );
    assert( add[3] ==  0.+ 0. );
    assert( add[4] == 40.+40. );

    assert( r.sum() == 10.+40.+1.+50. );
  }
  
  {
    // Test findIndex
    const int ne = 4;
    int inx[ne] =   {  1,  -4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinShallowPackedVector r(ne,inx,el);

    assert( r.findIndex(2)  == 3 );
    assert( r.findIndex(0)  == 2 );
    assert( r.findIndex(-4) == 1 );
    assert( r.findIndex(1)  == 0 );
    assert( r.findIndex(3)  == -1 );
  }  
  {
    // Test construction with testing for duplicates as false
    const int ne = 4;
    int inx[ne] =   {  1,  -4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinShallowPackedVector r(ne,inx,el,false);

    assert( r.isExistingIndex(1) );
    assert( r.isExistingIndex(-4) );
    assert( r.isExistingIndex(0) );
    assert( r.isExistingIndex(2) );
    assert( !r.isExistingIndex(3) );
    assert( !r.isExistingIndex(-3) );
  }
}
