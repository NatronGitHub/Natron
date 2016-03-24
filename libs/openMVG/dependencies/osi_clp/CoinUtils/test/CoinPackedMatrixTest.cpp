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

#include "CoinFloatEqual.hpp"
#include "CoinPackedVector.hpp"
#include "CoinPackedMatrix.hpp"

//#############################################################################

void
CoinPackedMatrixUnitTest()
{
  
  CoinRelFltEq eq;
  
  {
    // Test construction on empty matrices
    CoinPackedMatrix m;      
    CoinPackedMatrix lhs = m;    
    CoinPackedMatrix mCopy(m);
    
    assert( eq(m.getExtraGap(),0.0) );
    assert( eq(lhs.getExtraGap(),0.0) );
    assert( eq(mCopy.getExtraGap(),0.0) );
    
    assert( eq(m.getExtraMajor(),0.0) );
    assert( eq(lhs.getExtraMajor(),0.0) );
    assert( eq(mCopy.getExtraMajor(),0.0) );
    
    assert(       m.isColOrdered() );
    assert(     lhs.isColOrdered() );
    assert(   mCopy.isColOrdered() );
    
    assert(       m.getNumElements() == 0 );
    assert(     lhs.getNumElements() == 0 );
    assert(   mCopy.getNumElements() == 0 );
    
    assert(       m.getNumCols() == 0 );
    assert(     lhs.getNumCols() == 0 );
    assert(   mCopy.getNumCols() == 0 );
    
    assert(       m.getNumRows() == 0 );
    assert(     lhs.getNumRows() == 0 );
    assert(   mCopy.getNumRows() == 0 );
    
    assert(       m.getElements() == 0 );
    assert(     lhs.getElements() == 0 );
    assert(   mCopy.getElements() == 0 );
    
    assert(       m.getIndices() == 0 );
    assert(     lhs.getIndices() == 0 );
    assert(   mCopy.getIndices() == 0 );
    
    assert( m.getSizeVectorStarts()==0 );
    assert( lhs.getSizeVectorStarts()==0 );
    assert( mCopy.getSizeVectorStarts()==0 );
    
    assert( m.getSizeVectorLengths()==0 );
    assert( lhs.getSizeVectorLengths()==0 );
    assert( mCopy.getSizeVectorLengths()==0 );
    
    // out as empty matrix still has one start
    //assert( m.getVectorStarts()==NULL );
    //assert( lhs.getVectorStarts()==NULL );
    //assert( mCopy.getVectorStarts()==NULL );
    
    assert( m.getVectorLengths()==NULL );
    assert( lhs.getVectorLengths()==NULL );
    assert( mCopy.getVectorLengths()==NULL );
    
    assert(       m.getMajorDim() == 0 );
    assert(     lhs.getMajorDim() == 0 );
    assert(   mCopy.getMajorDim() == 0 );
    
    assert(       m.getMinorDim() == 0 );
    assert(     lhs.getMinorDim() == 0 );
    assert(   mCopy.getMinorDim() == 0 );
    
  }
  
  
  {
    CoinPackedMatrix * globalP;  
    
    { 
    /*************************************************************************
    *   Setup data to represent this matrix by rows
    *
    *    3x1 +  x2         -  2x4 - x5               -    x8
    *          2x2 + 1.1x3
    *                   x3              +  x6               
    *                       2.8x4             -1.2x7
    *  5.6x1                      + x5               + 1.9x8
    *
    *************************************************************************/
#if 0
      // By columns
      const int minor=5;
      const int major=8;
      const int numels=14;
      const double elemBase[numels]={3., 5.6, 1., 2., 1.1, 1., -2., 2.8, -1., 1., 1., -1.2, -1., 1.9};
      const int indBase[numels]={0,4,0,1,1,2,0,3,0,4,2,3,0,4};
      const CoinBigIndex startsBase[major+1]={0,2,4,6,8,10,11,12,14};
      const int lenBase[major]={2,2,2,2,2,1,1,2};
#else
      // By rows
      const int minor=8;
      const int major=5;
      const int numels=14;
      const double elemBase[numels]={3., 1., -2., -1., -1., 2., 1.1, 1., 1., 2.8, -1.2, 5.6, 1., 1.9 };
      const int indBase[numels]={0,1,3,4,7,1,2,2,5,3,6,0,4,7};
      const CoinBigIndex startsBase[major+1]={0,5,7,9,11,14};
      const int lenBase[major]={5,2,2,2,3};
#endif
      double * elem = new double[numels];
      int * ind = new int[numels];
      CoinBigIndex * starts = new CoinBigIndex[major+1];
      int * lens = new int[major];
      std::copy(elemBase,elemBase+numels,elem);
      std::copy(indBase,indBase+numels,ind);
      std::copy(startsBase,startsBase+major+1,starts);
      std::copy(lenBase,lenBase+major,lens);
 
      /*
	Explicitly request gap on this matrix, so we can see it propagate
	in subsequent copy & assignment.
      */
      CoinPackedMatrix pm(false,minor,major,numels,elem,ind,starts,lens,
			 .25,.25);
      
      assert( elem!=NULL );
      assert( ind!=NULL );
      assert( starts!=NULL );
      assert( lens!=NULL );
      
      delete[] elem;
      delete[] ind;
      delete[] starts;
      delete[] lens;
      
      assert( eq(pm.getExtraGap(),.25) );
      assert( eq(pm.getExtraMajor(),.25) );
      assert( !pm.isColOrdered() );
      assert( pm.getNumElements()==numels );
      assert( pm.getNumCols()==minor );
      assert( pm.getNumRows()==major);
      assert( pm.getSizeVectorStarts()==major+1 );
      assert( pm.getSizeVectorLengths()==major );
      
      const double * ev = pm.getElements();
      assert( eq(ev[0],   3.0) );
      assert( eq(ev[1],   1.0) );
      assert( eq(ev[2],  -2.0) );
      assert( eq(ev[3],  -1.0) );
      assert( eq(ev[4],  -1.0) );
      assert( eq(ev[7],   2.0) );
      assert( eq(ev[8],   1.1) );
      assert( eq(ev[10],   1.0) );
      assert( eq(ev[11],   1.0) );
      assert( eq(ev[13],   2.8) );
      assert( eq(ev[14], -1.2) );
      assert( eq(ev[16],  5.6) );
      assert( eq(ev[17],  1.0) );
      assert( eq(ev[18],  1.9) );
      
      const CoinBigIndex * mi = pm.getVectorStarts();
      assert( mi[0]==0 );
      assert( mi[1]==7 );
      assert( mi[2]==10 );
      assert( mi[3]==13 );
      assert( mi[4]==16 );
      assert( mi[5]==20 ); 
      
      const int * vl = pm.getVectorLengths();
      assert( vl[0]==5 );
      assert( vl[1]==2 );
      assert( vl[2]==2 );
      assert( vl[3]==2 );
      assert( vl[4]==3 );
      
      const int * ei = pm.getIndices();
      assert( ei[0]  ==  0 );
      assert( ei[1]  ==  1 );
      assert( ei[2]  ==  3 );
      assert( ei[3]  ==  4 );
      assert( ei[4]  ==  7 );
      assert( ei[7]  ==  1 );
      assert( ei[8]  ==  2 );
      assert( ei[10]  ==  2 );
      assert( ei[11]  ==  5 );
      assert( ei[13]  ==  3 );
      assert( ei[14] ==  6 );
      assert( ei[16] ==  0 );
      assert( ei[17] ==  4 );
      assert( ei[18] ==  7 );  
      
      assert( pm.getMajorDim() == 5 ); 
      assert( pm.getMinorDim() == 8 ); 
      assert( pm.getNumElements() == 14 ); 
      assert( pm.getSizeVectorStarts()==6 );
      
      {
        // Test copy constructor
        CoinPackedMatrix pmC(pm);
        
        assert( eq(pmC.getExtraGap(),.25) );
        assert( eq(pmC.getExtraMajor(),.25) );
        assert( !pmC.isColOrdered() );
        assert( pmC.getNumElements()==numels );
        assert( pmC.getNumCols()==minor );
        assert( pmC.getNumRows()==major);
        assert( pmC.getSizeVectorStarts()==major+1 );
        assert( pmC.getSizeVectorLengths()==major );
        
        // Test that osm has the correct values
        assert( pm.getElements() != pmC.getElements() );
        const double * ev = pmC.getElements();
        assert( eq(ev[0],   3.0) );
        assert( eq(ev[1],   1.0) );
        assert( eq(ev[2],  -2.0) );
        assert( eq(ev[3],  -1.0) );
        assert( eq(ev[4],  -1.0) );
        assert( eq(ev[7],   2.0) );
        assert( eq(ev[8],   1.1) );
        assert( eq(ev[10],   1.0) );
        assert( eq(ev[11],   1.0) );
        assert( eq(ev[13],   2.8) );
        assert( eq(ev[14], -1.2) );
        assert( eq(ev[16],  5.6) );
        assert( eq(ev[17],  1.0) );
        assert( eq(ev[18],  1.9) );
        
        assert( pm.getVectorStarts() != pmC.getVectorStarts() );
        const CoinBigIndex * mi = pmC.getVectorStarts();
        assert( mi[0]==0 );
        assert( mi[1]==7 );
        assert( mi[2]==10 );
        assert( mi[3]==13 );
        assert( mi[4]==16 );
        assert( mi[5]==20 ); 
        
        assert( pm.getVectorLengths() != pmC.getVectorLengths() );     
        const int * vl = pmC.getVectorLengths();
        assert( vl[0]==5 );
        assert( vl[1]==2 );
        assert( vl[2]==2 );
        assert( vl[3]==2 );
        assert( vl[4]==3 );
        
        assert( pm.getIndices() != pmC.getIndices() );
        const int * ei = pmC.getIndices();
        assert( ei[0]  ==  0 );
        assert( ei[1]  ==  1 );
        assert( ei[2]  ==  3 );
        assert( ei[3]  ==  4 );
        assert( ei[4]  ==  7 );
        assert( ei[7]  ==  1 );
        assert( ei[8]  ==  2 );
        assert( ei[10]  ==  2 );
        assert( ei[11]  ==  5 );
        assert( ei[13]  ==  3 );
        assert( ei[14] ==  6 );
        assert( ei[16] ==  0 );
        assert( ei[17] ==  4 );
        assert( ei[18] ==  7 );  
        
        assert( pmC.isEquivalent(pm) );      
        
        // Test assignment
        {
          CoinPackedMatrix pmA;
          // Gap should be 0.0
          assert( eq(pmA.getExtraGap(),0.0) );
          assert( eq(pmA.getExtraMajor(),0.0) );
          
          pmA = pm;
          
          assert( eq(pmA.getExtraGap(),0.25) );
          assert( eq(pmA.getExtraMajor(),0.25) );
          assert( !pmA.isColOrdered() );
          assert( pmA.getNumElements()==numels );
          assert( pmA.getNumCols()==minor );
          assert( pmA.getNumRows()==major);
          assert( pmA.getSizeVectorStarts()==major+1 );
          assert( pmA.getSizeVectorLengths()==major );
          
          // Test that osm1 has the correct values
          assert( pm.getElements() != pmA.getElements() );
          const double * ev = pmA.getElements();
          assert( eq(ev[0],   3.0) );
          assert( eq(ev[1],   1.0) );
          assert( eq(ev[2],  -2.0) );
          assert( eq(ev[3],  -1.0) );
          assert( eq(ev[4],  -1.0) );
          assert( eq(ev[7],   2.0) );
          assert( eq(ev[8],   1.1) );
          assert( eq(ev[10],   1.0) );
          assert( eq(ev[11],   1.0) );
          assert( eq(ev[13],   2.8) );
          assert( eq(ev[14], -1.2) );
          assert( eq(ev[16],  5.6) );
          assert( eq(ev[17],  1.0) );
          assert( eq(ev[18],  1.9) );
	  
	  // Test modification of a single element
	  pmA.modifyCoefficient(2,5,-7.0);
          assert( eq(ev[11],   -7.0) );
	  // and back
	  pmA.modifyCoefficient(2,5,1.0);
          assert( eq(ev[11],   1.0) );
          
          assert( pm.getVectorStarts() != pmA.getVectorStarts() );
          const CoinBigIndex * mi = pmA.getVectorStarts();
          assert( mi[0]==0 );
          assert( mi[1]==7 );
          assert( mi[2]==10 );
          assert( mi[3]==13 );
          assert( mi[4]==16 );
          assert( mi[5]==20 ); 
          
          assert( pm.getVectorLengths() != pmA.getVectorLengths() );     
          const int * vl = pmC.getVectorLengths();
          assert( vl[0]==5 );
          assert( vl[1]==2 );
          assert( vl[2]==2 );
          assert( vl[3]==2 );
          assert( vl[4]==3 );
          
          assert( pm.getIndices() != pmA.getIndices() );
          const int * ei = pmA.getIndices();
          assert( ei[0]  ==  0 );
          assert( ei[1]  ==  1 );
          assert( ei[2]  ==  3 );
          assert( ei[3]  ==  4 );
          assert( ei[4]  ==  7 );
          assert( ei[7]  ==  1 );
          assert( ei[8]  ==  2 );
          assert( ei[10]  ==  2 );
          assert( ei[11]  ==  5 );
          assert( ei[13]  ==  3 );
          assert( ei[14] ==  6 );
          assert( ei[16] ==  0 );
          assert( ei[17] ==  4 );
          assert( ei[18] ==  7 );  
          
          assert( pmA.isEquivalent(pm) );
          assert( pmA.isEquivalent(pmC) );
          
          // Test new to global
          globalP = new CoinPackedMatrix(pmA);
          assert( eq(globalP->getElements()[0],   3.0) );
          assert( globalP->isEquivalent(pmA) );
        }
        assert( eq(globalP->getElements()[0],   3.0) );
      }
      assert( eq(globalP->getElements()[0],   3.0) );
    }
  
    // Test that cloned matrix contains correct values
    const double * ev = globalP->getElements();
    assert( eq(ev[0],   3.0) );
    assert( eq(ev[1],   1.0) );
    assert( eq(ev[2],  -2.0) );
    assert( eq(ev[3],  -1.0) );
    assert( eq(ev[4],  -1.0) );
    assert( eq(ev[7],   2.0) );
    assert( eq(ev[8],   1.1) );
    assert( eq(ev[10],   1.0) );
    assert( eq(ev[11],   1.0) );
    assert( eq(ev[13],   2.8) );
    assert( eq(ev[14], -1.2) );
    assert( eq(ev[16],  5.6) );
    assert( eq(ev[17],  1.0) );
    assert( eq(ev[18],  1.9) );

    // Check printMatrixElement
    std::cout << "Testing printMatrixElement:" << std::endl ;
    std::cout << "Expecting 1.1, 0.0, -1.2." << std::endl ;
    std::cout << "a(1,2) = " ;
    globalP->printMatrixElement(1,2) ;
    std::cout << "; a(3,5) = " ;
    globalP->printMatrixElement(3,5) ;
    std::cout << "; a(3,6) = " ;
    globalP->printMatrixElement(3,6) ;
    std::cout << std::endl ;
    std::cout << "Expecting bounds error messages." << std::endl ;
    std::cout << "a(-1,-1) = " ;
    globalP->printMatrixElement(-1,-1) ;
    std::cout << "a(0,-1) = " ;
    globalP->printMatrixElement(0,-1) ;
    std::cout << "a(4,8) = " ;
    globalP->printMatrixElement(4,8) ;
    std::cout << "a(5,8) = " ;
    globalP->printMatrixElement(5,8) ;
#   if 0
    /* If you want an exhaustive test, enable these loops. */
    for (CoinBigIndex rowndx = -1 ;
         rowndx <= globalP->getMajorDim() ; rowndx++) {
      for (CoinBigIndex colndx = -1 ;
           colndx < globalP->getMinorDim() ; colndx++) {
        std::cout << "a(" << rowndx << "," << colndx << ") = " ;
	globalP->printMatrixElement(rowndx,colndx) ;
	std::cout << std::endl ;
      }
    }
#   endif
    
    const CoinBigIndex * mi = globalP->getVectorStarts();
    assert( mi[0]==0 );
    assert( mi[1]==7 );
    assert( mi[2]==10 );
    assert( mi[3]==13 );
    assert( mi[4]==16 );
    assert( mi[5]==20 ); 
    
    const int * ei = globalP->getIndices();
    assert( ei[0]  ==  0 );
    assert( ei[1]  ==  1 );
    assert( ei[2]  ==  3 );
    assert( ei[3]  ==  4 );
    assert( ei[4]  ==  7 );
    assert( ei[7]  ==  1 );
    assert( ei[8]  ==  2 );
    assert( ei[10] ==  2 );
    assert( ei[11] ==  5 );
    assert( ei[13] ==  3 );
    assert( ei[14] ==  6 );
    assert( ei[16] ==  0 );
    assert( ei[17] ==  4 );
    assert( ei[18] ==  7 );  
    
    assert( globalP->getMajorDim() == 5 ); 
    assert( globalP->getMinorDim() == 8 ); 
    assert( globalP->getNumElements() == 14 ); 
    assert( globalP->getSizeVectorStarts()==6 );
    
    // Test method which returns length of vectors
    assert( globalP->getVectorSize(0)==5 );
    assert( globalP->getVectorSize(1)==2 );
    assert( globalP->getVectorSize(2)==2 );
    assert( globalP->getVectorSize(3)==2 );
    assert( globalP->getVectorSize(4)==3 );
    
    // Test getVectorSize exceptions
    {
      bool errorThrown = false;
      try {
        globalP->getVectorSize(-1);
      }
      catch (CoinError& e) {
	errorThrown = true;
      }
      assert( errorThrown );
    }
    {
      bool errorThrown = false;
      try {
        globalP->getVectorSize(5);
      }
      catch (CoinError& e) {
        errorThrown = true;
      }
      assert( errorThrown );
    }
    
    // Test vector method
    {
      // 3x1 +  x2         -  2x4 - x5               -    x8       
      CoinShallowPackedVector pv = globalP->getVector(0);
      assert( pv.getNumElements() == 5 );
      assert( eq(pv[0], 3.0) );
      assert( eq(pv[1], 1.0) );
      assert( eq(pv[3],-2.0) );
      assert( eq(pv[4],-1.0) );
      assert( eq(pv[7],-1.0) );
      
      //          2x2 + 1.1x3               
      pv = globalP->getVector(1);
      assert( pv.getNumElements() == 2 );
      assert( eq(pv[1], 2.0) );
      assert( eq(pv[2], 1.1) );
      
      //                   x3              +  x6             
      pv = globalP->getVector(2);
      assert( pv.getNumElements() == 2 );
      assert( eq(pv[2], 1.0) );
      assert( eq(pv[5], 1.0) ); 
      
      //                      2.8x4             -1.2x7             
      pv = globalP->getVector(3);
      assert( pv.getNumElements() == 2 );
      assert( eq(pv[3], 2.8) );
      assert( eq(pv[6],-1.2) );
      
      //  5.6x1                      + x5               + 1.9x8              
      pv = globalP->getVector(4);
      assert( pv.getNumElements() == 3 );
      assert( eq(pv[0], 5.6) );
      assert( eq(pv[4], 1.0) ); 
      assert( eq(pv[7], 1.9) ); 
    }
    
    // Test vector method exceptions
    {
      bool errorThrown = false;
      try {
        CoinShallowPackedVector v = globalP->getVector(-1);
      }
      catch (CoinError& e) {
        errorThrown = true;
      }
      assert( errorThrown );
    }
    {
      bool errorThrown = false;
      try {
        CoinShallowPackedVector vs = globalP->getVector(5);
      }
      catch (CoinError& e) {
        errorThrown = true;
      }
      assert( errorThrown );
    }

    {
      CoinPackedMatrix pm(*globalP);
      
      assert( pm.getExtraGap() != 0.0 );
      assert( pm.getExtraMajor() != 0.0 );
      
      pm.setExtraGap(0.0);
      pm.setExtraMajor(0.0);
      
      assert( pm.getExtraGap() == 0.0 );
      assert( pm.getExtraMajor() == 0.0 );
      
      pm.reverseOrdering();
      
      assert( pm.getExtraGap() == 0.0 );
      assert( pm.getExtraMajor() == 0.0 );
    }


    // Test ordered triples constructor
    {
    /*************************************************************************
    *   Setup data to represent this matrix by rows
    *
    *    3x1 +  x2         -  2x4 - x5               -    x8 
    *          2x2y+ 1.1x3
    *                   x3              +  x6y              
    *                       2.8x4             -1.2x7 
    *  5.6x1                      + x5               + 1.9x8 
    *
    *************************************************************************/
      const int ne=17;
      int ri[ne];
      int ci[ne];
      double el[ne];
      ri[ 0]=1; ci[ 0]=2; el[ 0]=1.1;
      ri[ 1]=0; ci[ 1]=3; el[ 1]=-2.0;
      ri[ 2]=4; ci[ 2]=7; el[ 2]=1.9;
      ri[ 3]=3; ci[ 3]=6; el[ 3]=-1.2;
      ri[ 4]=2; ci[ 4]=5; el[ 4]=1.0;
      ri[ 5]=4; ci[ 5]=0; el[ 5]=5.6;
      ri[ 6]=0; ci[ 6]=7; el[ 6]=-1.0;
      ri[ 7]=0; ci[ 7]=0; el[ 7]=3.0;
      ri[ 8]=0; ci[ 8]=4; el[ 8]=-1.0;
      ri[ 9]=4; ci[ 9]=4; el[ 9]=1.0;
      ri[10]=3; ci[10]=3; el[10]=2.0; // (3,3) is a duplicate element
      ri[11]=1; ci[11]=1; el[11]=2.0;
      ri[12]=0; ci[12]=1; el[12]=1.0;
      ri[13]=2; ci[13]=2; el[13]=1.0;
      ri[14]=3; ci[14]=3; el[14]=0.8; // (3,3) is a duplicate element
      ri[15]=2; ci[15]=0; el[15]=-3.1415;
      ri[16]=2; ci[16]=0; el[16]= 3.1415;
      assert(!globalP->isColOrdered());

      // create col ordered matrix from triples
      CoinPackedMatrix pmtco(true,ri,ci,el,ne);

      // Test that duplicates got the correct value
      assert( eq( pmtco.getVector(3)[3] , 2.8 ) );
      assert( eq( pmtco.getVector(0)[2] , 0.0 ) );

      // Test to make sure there are no gaps in the created matrix
      assert( pmtco.getVectorStarts()[0]==0 );
      assert( pmtco.getVectorStarts()[1]==2 );
      assert( pmtco.getVectorStarts()[2]==4 );
      assert( pmtco.getVectorStarts()[3]==6 );
      assert( pmtco.getVectorStarts()[4]==8 );
      assert( pmtco.getVectorStarts()[5]==10 );
      assert( pmtco.getVectorStarts()[6]==11 );
      assert( pmtco.getVectorStarts()[7]==12 );
      assert( pmtco.getVectorStarts()[8]==14 );

      // Test the whole matrix
      CoinPackedMatrix globalco;
      globalco.reverseOrderedCopyOf(*globalP);
      assert(pmtco.isEquivalent(globalco));

      // create row ordered matrix from triples
      CoinPackedMatrix pmtro(false,ri,ci,el,ne);
      assert(!pmtro.isColOrdered());
      assert( eq( pmtro.getVector(3)[3] , 2.8 ) );
      assert( eq( pmtro.getVector(2)[0] , 0.0 ) );
      assert( pmtro.getVectorStarts()[0]==0 );
      assert( pmtro.getVectorStarts()[1]==5 );
      assert( pmtro.getVectorStarts()[2]==7 );
      assert( pmtro.getVectorStarts()[3]==9 );
      assert( pmtro.getVectorStarts()[4]==11 );
      assert( pmtro.getVectorStarts()[5]==14 );
      assert(globalP->isEquivalent(pmtro));

    }
    
    delete globalP;
  }
  
#if 0
  {
    // test append
    CoinPackedMatrix pm;
    
    const int ne = 4;
    int inx[ne] =   {  1,  -4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    CoinPackedVector r(ne,inx,el);

    pm.appendRow(r);  // This line fails

  }
#endif 
  
}
