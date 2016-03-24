// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#ifdef NDEBUG
#undef NDEBUG
// checkClean and checkClear not define
#define NO_CHECK_CL
#endif

#include <cassert>

#include "CoinFinite.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinShallowPackedVector.hpp"

//--------------------------------------------------------------------------
void
CoinIndexedVectorUnitTest()
{
  
  int i;
  // Test default constructor
  {
    CoinIndexedVector r;
    assert( r.indices_==NULL );
    assert( r.elements_==NULL );
    assert( r.getNumElements()==0 );
    assert( r.capacity_==0);
  }
  
  // Test set and get methods
  const int ne = 4;
  int inx[ne] = { 1, 3, 4, 7 };
  double el[ne] = { 1.2, 3.4, 5.6, 7.8 };
  {
    CoinIndexedVector r;    
    assert( r.getNumElements()==0 );
    
    // Test setting/getting elements with int* & float* vectors
    r.setVector( ne, inx, el );
    assert( r.getNumElements()==ne );
    for ( i=0; i<ne; i++ ) {
      assert( r.getIndices()[i]  == inx[i] );
      assert( r[inx[i]]  == el[i] );
    }
    
    // Test setting/getting elements with indices out of order  
    const int ne2 = 5;
    int inx2[ne2] = { 2, 4, 8, 14, 3 };
    double el2[ne2] = { 2.2, 4.4, 6.6, 8.8, 3.3 };
    
    r.setVector(ne2,inx2,el2);
    
    assert( r.getNumElements()==ne2 );    
    
    assert( r.getIndices()[0]==inx2[0] );
    
    assert( r.getIndices()[1]==inx2[1] );
    
    assert( r.getIndices()[2]==inx2[2] );
    
    assert( r.getIndices()[3]==inx2[3] );
    
    assert( r.getIndices()[4]==inx2[4] );
    

    CoinIndexedVector r1(ne2,inx2,el2);
    assert( r == r1 );   
  }    
  CoinIndexedVector r;
  
  
  {
    CoinIndexedVector r;
    const int ne = 3;
    int inx[ne] = { 1, 2, 3 };
    double el[ne] = { 2.2, 4.4, 8.8};
    r.setVector(ne,inx,el);
    int c = r.capacity();
    // Test swap function
    r.swap(0,2);
    assert( r.getIndices()[0]==3 );
    assert( r.getIndices()[1]==2 );
    assert( r.getIndices()[2]==1 );
    assert( r.capacity() == c );
    
    // Test the append function
    CoinIndexedVector s;
    const int nes = 4;
    int inxs[nes] = { 11, 12, 13, 14 };
    double els[nes] = { .122, 14.4, 18.8, 19.9};
    s.setVector(nes,inxs,els);
    r.append(s);
    assert( r.getNumElements()==7 );
    assert( r.getIndices()[0]==3 );
    assert( r.getIndices()[1]==2 );
    assert( r.getIndices()[2]==1 );
    assert( r.getIndices()[3]==11 );
    assert( r.getIndices()[4]==12 );
    assert( r.getIndices()[5]==13 );
    assert( r.getIndices()[6]==14 );
    
    // Test the resize function
    c = r.capacity();
    r.truncate(4);
    // we will lose 11
    assert( r.getNumElements()==3 );
    assert( r.getIndices()[0]==3 );
    assert( r.getIndices()[1]==2 );
    assert( r.getIndices()[2]==1 );
    assert( r.getMaxIndex() == 3 );
    assert( r.getMinIndex() == 1 );
    assert( r.capacity() == c );
  }
  
  // Test borrow and return vector
  {
    CoinIndexedVector r,r2;
    const int ne = 3;
    int inx[ne] = { 1, 2, 3 };
    double el[ne] = { 2.2, 4.4, 8.8};
    double els[4] = { 0.0,2.2, 4.4, 8.8};
    r.setVector(ne,inx,el);
    r2.borrowVector(4,ne,inx,els);
    assert (r==r2);
    r2.returnVector();
    assert (!r2.capacity());
    assert (!r2.getNumElements());
    assert (!r2.denseVector());
    assert (!r2.getIndices());
  }
  
  // Test copy constructor and assignment operator
  {
    CoinIndexedVector rhs;
    {
      CoinIndexedVector r;
      {
	CoinIndexedVector rC1(r);      
	assert( 0==r.getNumElements() );
	assert( 0==rC1.getNumElements() );
	
	
	r.setVector( ne, inx, el ); 
	
	assert( ne==r.getNumElements() );
	assert( 0==rC1.getNumElements() ); 
      }
      
      CoinIndexedVector rC2(r);   
      
      assert( ne==r.getNumElements() );
      assert( ne==rC2.getNumElements() );
      
      for ( i=0; i<ne; i++ ) {
	assert( r.getIndices()[i] == rC2.getIndices()[i] );
      }
      
      rhs=rC2;
    }
    // Test that rhs has correct values even though lhs has gone out of scope
    assert( rhs.getNumElements()==ne );
    
    for ( i=0; i<ne; i++ ) {
      assert( inx[i] == rhs.getIndices()[i] );
    } 
  }
  
  // Test operator==
  {
    CoinIndexedVector v1,v2;
    assert( v1==v2 );
    assert( v2==v1 );
    assert( v1==v1 );
    assert( !(v1!=v2) );
    
    v1.setVector( ne, inx, el );
    assert ( !(v1==v2) );
    assert ( v1!=v2 );
    
    CoinIndexedVector v3(v1);
    assert( v3==v1 );
    assert( v3!=v2 );
    
    CoinIndexedVector v4(v2);
    assert( v4!=v1 );
    assert( v4==v2 );
  }
  
  {
    // Test sorting of indexed vectors    
    const int ne = 4;
    int inx[ne] = { 1, 4, 0, 2 };
    double el[ne] = { 10., 40., 1., 20. };
    CoinIndexedVector r;
    r.setVector(ne,inx,el);
    
    // Test that indices are in increasing order
    r.sort();
    for ( i=1; i<ne; i++ ) assert( r.getIndices()[i-1] < r.getIndices()[i] );

  }    
  {
    // Test operator[] and indexExists()
    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinIndexedVector r;
    bool errorThrown = false;
    try {
      assert( r[1]==0. );
    }
    catch (CoinError& e) {
      errorThrown = true;
    }
    assert( errorThrown );
    
    r.setVector(ne,inx,el);
    
    errorThrown = false;
    try {
      assert( r[-1]==0. );
    }
    catch (CoinError& e) {
      errorThrown = true;
    }
    assert( errorThrown );
    
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);
    errorThrown = false;
    try {
      assert( r[5]==0. );
    }
    catch (CoinError& e) {
      errorThrown = true;
    }
    assert( errorThrown );
    
    assert ( r.getMaxIndex()==4 );
    assert ( r.getMinIndex()==0 );
  }
  
  // Test that attemping to get min/max index of a 0,
  // length vector 
  {
    CoinIndexedVector nullVec;
    assert( nullVec.getMaxIndex() == -COIN_INT_MAX/*0*/ );
    assert( nullVec.getMinIndex() == COIN_INT_MAX/*0*/ );
  } 
  
  // Test CoinFltEq with equivalent method
  {    
    const int ne = 4;
    int inx1[ne] = { 1, 3, 4, 7 };
    double el1[ne] = { 1.2, 3.4, 5.6, 7.8 };
    int inx2[ne] = { 7, 4, 3, 1 };
    double el2[ne] = { 7.8+.5, 5.6+.5, 3.4+.5, 1.2+.5 };
    CoinIndexedVector v1,v2;
    v1.setVector(ne,inx1,el1);
    v2.setVector(ne,inx2,el2);
  }
  
  {
    // Test reserve
    CoinIndexedVector v1,v2;
    assert( v1.capacity()==0 );
    v1.reserve(6);
    assert( v1.capacity()==6 );
    assert( v1.getNumElements()==0 );
    v2=v1;
    assert( v2.capacity() == 6 );
    assert( v2.getNumElements()==0 );
    assert( v2==v1 );
    v1.setVector(0,NULL,NULL);
    assert( v1.capacity()==6 );
    assert( v1.getNumElements()==0 );
    assert( v2==v1 );
    v2=v1;
    assert( v2.capacity() == 6 );
    assert( v2.getNumElements()==0 );
    assert( v2==v1 );
    
    const int ne = 2;
    int inx[ne] = { 1, 3 };
    double el[ne] = { 1.2, 3.4 };
    v1.setVector(ne,inx,el);
    assert( v1.capacity()==6 );
    assert( v1.getNumElements()==2 );
    v2=v1;
    assert( v2.capacity()==6 );
    assert( v2.getNumElements()==2 );
    assert( v2==v1 );
    
    const int ne1 = 5;
    int inx1[ne1] = { 1, 3, 4, 5, 6 };
    double el1[ne1] = { 1.2, 3.4, 5., 6., 7. };
    v1.setVector(ne1,inx1,el1);
    assert( v1.capacity()==7 );
    assert( v1.getNumElements()==5 );
    v2=v1;
    assert( v2.capacity()==7 );
    assert( v2.getNumElements()==5 );
    assert( v2==v1 );
    
    const int ne2 = 8;
    int inx2[ne2] = { 1, 3, 4, 5, 6, 7, 8, 9 };
    double el2[ne2] = { 1.2, 3.4, 5., 6., 7., 8., 9., 10. };
    v1.setVector(ne2,inx2,el2);
    assert( v1.capacity()==10 );
    assert( v1.getNumElements()==8 );
    v2=v1;
    assert( v2.getNumElements()==8 );    
    assert( v2==v1 );
    
    v1.setVector(ne1,inx1,el1);
    assert( v1.capacity()==10 );
    assert( v1.getNumElements()==5 );
    v2=v1;    
    assert( v2.capacity()==10 );
    assert( v2.getNumElements()==5 );
    assert( v2==v1 );
    
    v1.reserve(7);
    assert( v1.capacity()==10 );
    assert( v1.getNumElements()==5 );
    v2=v1;
    assert( v2.capacity()==10 );
    assert( v2.getNumElements()==5 );
    assert( v2==v1 );
    
  }
  
  // Test the insert method
  {
    CoinIndexedVector v1;
    assert( v1.getNumElements()==0 );
    assert( v1.capacity()==0 );
    
    v1.insert(1,1.);
    assert( v1.getNumElements()==1 );
    assert( v1.capacity()==2 );
    assert( v1.getIndices()[0] == 1 );
    
    v1.insert(10,10.);
    assert( v1.getNumElements()==2 );
    assert( v1.capacity()==11 );
    assert( v1.getIndices()[1] == 10 );
    
    v1.insert(20,20.);
    assert( v1.getNumElements()==3 );
    assert( v1.capacity()==21 );
    assert( v1.getIndices()[2] == 20 );
    
    v1.insert(30,30.);
    assert( v1.getNumElements()==4 );
    assert( v1.capacity()==31 );
    assert( v1.getIndices()[3] == 30 );

    v1.insert(40,40.);
    assert( v1.getNumElements()==5 );
    assert( v1.capacity()==41 );
    assert( v1.getIndices()[4] == 40 );
    
    v1.insert(50,50.);
    assert( v1.getNumElements()==6 );
    assert( v1.capacity()==51 );
    assert( v1.getIndices()[5] == 50 );
    
    CoinIndexedVector v2;
    const int ne1 = 3;
    int inx1[ne1] = { 1, 3, 4 };
    double el1[ne1] = { 1.2, 3.4, 5. };
    v2.setVector(ne1,inx1,el1);    
    assert( v2.getNumElements()==3 );
    assert( v2.capacity()==5 );

    // Test clean method - get rid of 1.2
    assert(v2.clean(3.0)==2);
    assert(v2.denseVector()[1]==0.0);

    // Below are purely for debug - so use assert
    // so we won't try with false
    // Test checkClean 
#ifndef NO_CHECK_CL
    v2.checkClean();
    assert( v2.getNumElements()==2 );

    // Get rid of all
    int numberRemaining = v2.clean(10.0);
    assert(numberRemaining==0);
    v2.checkClear();
#endif    
  }
  
  {
    //Test setConstant and setElement     
    CoinIndexedVector v2;
    const int ne1 = 3;
    int inx1[ne1] = { 1, 3, 4 };
    v2.setConstant(ne1,inx1,3.14);    
    assert( v2.getNumElements()==3 );
    assert( v2.capacity()==5 );
    assert( v2.getIndices()[0]==1 );
    assert( v2.getIndices()[1]==3 );
    assert( v2.getIndices()[2]==4 );
    
    assert( v2[3] == 3.14 );
    
    CoinIndexedVector v2X(ne1,inx1,3.14);
    assert( v2 == v2X );
    
  }
  
  {
    //Test setFull 
    CoinIndexedVector v2;
    const int ne2 = 3;
    double el2[ne2] = { 1., 3., 4. };
    v2.setFull(ne2,el2);    
    assert( v2.getNumElements()==3 );
    assert( v2.capacity()==3 );
    assert( v2.getIndices()[0]==0 );
    assert( v2.getIndices()[1]==1 );
    assert( v2.getIndices()[2]==2 );
    
    assert( v2[1] == 3. );
    
    CoinIndexedVector v2X(ne2,el2);
    assert( v2 == v2X ); 
    
    v2.setFull(0,el2); 
    assert( v2[2] == 0. );  
  }
  
  
#if 0
  // what happens when someone sets 
  // the number of elements to be a negative number
  {    
    const int ne = 4;
    int inx1[ne] = { 1, 3, 4, 7 };
    double el1[ne] = { 1.2, 3.4, 5.6, 7.8 };
    CoinIndexedVector v1;
    v1.set(-ne,inx1,el1);
  }
#endif
  
  
  // Test copy constructor from ShallowPackedVector
  {
    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinIndexedVector std(ne,inx,el);
    CoinShallowPackedVector * spvP = new CoinShallowPackedVector(ne,inx,el);
    CoinIndexedVector pv(*spvP);
    assert( pv == std );
    delete spvP;
    assert( pv == std );
  }
  
  // Test assignment from ShallowPackedVector
  {
    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinIndexedVector std(ne,inx,el);
    CoinShallowPackedVector * spvP = new CoinShallowPackedVector(ne,inx,el);
    CoinIndexedVector pv;
    pv = *spvP;
    assert( pv == std );
    delete spvP;
    assert( pv == std );
  }
  
  {
    // Test that sample usage works
    
    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinIndexedVector r(ne,inx,el);
    
    assert( r.getIndices()[0]== 1  );
    assert( r.getIndices()[1]== 4  );
    assert( r.getIndices()[2]== 0  );
    assert( r.getIndices()[3]== 2  );
    
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);
    
    r.sortIncrElement();
    
    assert( r.getIndices()[0]== 0  );
    assert( r.getIndices()[1]== 1  );
    assert( r.getIndices()[2]== 4  );
    assert( r.getIndices()[3]== 2  );
    
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);
    
    CoinIndexedVector r1;
    r1=r;
    assert( r==r1 );
    
    CoinIndexedVector add = r + r1;
    assert( add[0] ==  1.+ 1. );
    assert( add[1] == 10.+10. );
    assert( add[2] == 50.+50. );
    assert( add[3] ==  0.+ 0. );
    assert( add[4] == 40.+40. );
    
  }
  
}
    


