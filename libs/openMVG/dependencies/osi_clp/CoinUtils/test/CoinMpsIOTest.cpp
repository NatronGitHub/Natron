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

#include "CoinMpsIO.hpp"
#include "CoinFloatEqual.hpp"

//#############################################################################

//--------------------------------------------------------------------------
// test import methods
void
CoinMpsIOUnitTest(const std::string & mpsDir)
{
  
  // Test default constructor
  {
    CoinMpsIO m;
    assert( m.rowsense_==NULL );
    assert( m.rhs_==NULL );
    assert( m.rowrange_==NULL );
    assert( m.matrixByRow_==NULL );
    assert( m.matrixByColumn_==NULL );
    assert( m.integerType_==NULL);
    assert( !strcmp( m.getFileName() , "????"));
    assert( !strcmp( m.getProblemName() , ""));
    assert( !strcmp( m.objectiveName_ , ""));
    assert( !strcmp( m.rhsName_ , ""));
    assert( !strcmp( m.rangeName_ , ""));
    assert( !strcmp( m.boundName_ , ""));
  }
  
  
  {    
    CoinRelFltEq eq;
    CoinMpsIO m;
    std::string fn = mpsDir+"exmip1";
    int numErr = m.readMps(fn.c_str(),"mps");
    assert( numErr== 0 );

    assert( !strcmp( m.problemName_ , "EXAMPLE"));
    assert( !strcmp( m.objectiveName_ , "OBJ"));
    assert( !strcmp( m.rhsName_ , "RHS1"));
    assert( !strcmp( m.rangeName_ , "RNG1"));
    assert( !strcmp( m.boundName_ , "BND1"));
    
     // Test language and re-use
    m.newLanguage(CoinMessages::it);
    m.messageHandler()->setPrefix(false);

    // This call should return an error indicating that the 
    // end-of-file was reached.
    // This is because the file remains open to possibly read
    // a quad. section.
    numErr = m.readMps(fn.c_str(),"mps");
    assert( numErr < 0 );

    // Test copy constructor and assignment operator
    {
      CoinMpsIO lhs;
      {      
        CoinMpsIO im(m);        
	
        CoinMpsIO imC1(im);
        assert( imC1.getNumCols() == im.getNumCols() );
        assert( imC1.getNumRows() == im.getNumRows() );   
        
        CoinMpsIO imC2(im);
        assert( imC2.getNumCols() == im.getNumCols() );
        assert( imC2.getNumRows() == im.getNumRows() );  
        
        lhs=imC2;
      }
      // Test that lhs has correct values even though rhs has gone out of scope
      
      assert( lhs.getNumCols() == m.getNumCols() );
      assert( lhs.getNumRows() == m.getNumRows() );      
    }
    
    
    {    
      CoinMpsIO dumSi(m);
      int nc = dumSi.getNumCols();
      int nr = dumSi.getNumRows();
      const double * cl = dumSi.getColLower();
      const double * cu = dumSi.getColUpper();
      const double * rl = dumSi.getRowLower();
      const double * ru = dumSi.getRowUpper();
      assert( nc == 8 );
      assert( nr == 5 );
      assert( eq(cl[0],2.5) );
      assert( eq(cl[1],0.0) );
      assert( eq(cu[1],4.1) );
      assert( eq(cu[2],1.0) );
      assert( eq(rl[0],2.5) );
      assert( eq(rl[4],3.0) );
      assert( eq(ru[1],2.1) );
      assert( eq(ru[4],15.0) );
      
      assert( !eq(cl[3],1.2345) );
      
      assert( !eq(cu[4],10.2345) );
      
      assert( eq( dumSi.getObjCoefficients()[0],  1.0) );
      assert( eq( dumSi.getObjCoefficients()[1],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[2],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[3],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[4],  2.0) );
      assert( eq( dumSi.getObjCoefficients()[5],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[6],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[7], -1.0) );

      dumSi.writeMps("CoinMpsIoTest.mps");//,0,0,1);
    }

    // Read just written file
    {    
      CoinMpsIO dumSi;
      dumSi.readMps("CoinMpsIoTest");
      int nc = dumSi.getNumCols();
      int nr = dumSi.getNumRows();
      const double * cl = dumSi.getColLower();
      const double * cu = dumSi.getColUpper();
      const double * rl = dumSi.getRowLower();
      const double * ru = dumSi.getRowUpper();
      assert( nc == 8 );
      assert( nr == 5 );
      assert( eq(cl[0],2.5) );
      assert( eq(cl[1],0.0) );
      assert( eq(cu[1],4.1) );
      assert( eq(cu[2],1.0) );
      assert( eq(rl[0],2.5) );
      assert( eq(rl[4],3.0) );
      assert( eq(ru[1],2.1) );
      assert( eq(ru[4],15.0) );
      
      assert( !eq(cl[3],1.2345) );
      
      assert( !eq(cu[4],10.2345) );
      
      assert( eq( dumSi.getObjCoefficients()[0],  1.0) );
      assert( eq( dumSi.getObjCoefficients()[1],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[2],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[3],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[4],  2.0) );
      assert( eq( dumSi.getObjCoefficients()[5],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[6],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[7], -1.0) );
    }
    
    // Test matrixByRow method
    { 
      const CoinMpsIO si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();
      // LL:      const CoinDumPackedMatrix * osmP = dynamic_cast<const CoinDumPackedMatrix*>(smP);
      // LL: assert( osmP!=NULL );
      
      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      assert( eq(ev[0],   3.0) );
      assert( eq(ev[1],   1.0) );
      assert( eq(ev[2],  -2.0) );
      assert( eq(ev[3],  -1.0) );
      assert( eq(ev[4],  -1.0) );
      assert( eq(ev[5],   2.0) );
      assert( eq(ev[6],   1.1) );
      assert( eq(ev[7],   1.0) );
      assert( eq(ev[8],   1.0) );
      assert( eq(ev[9],   2.8) );
      assert( eq(ev[10], -1.2) );
      assert( eq(ev[11],  5.6) );
      assert( eq(ev[12],  1.0) );
      assert( eq(ev[13],  1.9) );
      
      const CoinBigIndex * mi = smP->getVectorStarts();
      assert( mi[0]==0 );
      assert( mi[1]==5 );
      assert( mi[2]==7 );
      assert( mi[3]==9 );
      assert( mi[4]==11 );
      assert( mi[5]==14 );
      
      const int * ei = smP->getIndices();
      assert( ei[0]  ==  0 );
      assert( ei[1]  ==  1 );
      assert( ei[2]  ==  3 );
      assert( ei[3]  ==  4 );
      assert( ei[4]  ==  7 );
      assert( ei[5]  ==  1 );
      assert( ei[6]  ==  2 );
      assert( ei[7]  ==  2 );
      assert( ei[8]  ==  5 );
      assert( ei[9]  ==  3 );
      assert( ei[10] ==  6 );
      assert( ei[11] ==  0 );
      assert( ei[12] ==  4 );
      assert( ei[13] ==  7 );    
      
      assert( smP->getMajorDim() == 5 ); 
      assert( smP->getNumElements() == 14 );
      
    }
        // Test matrixByCol method
    {
      
      const CoinMpsIO si(m);
      const CoinPackedMatrix * smP = si.getMatrixByCol();
      // LL:      const CoinDumPackedMatrix * osmP = dynamic_cast<const CoinDumPackedMatrix*>(smP);
      // LL: assert( osmP!=NULL );
      
      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      assert( eq(ev[0],   3.0) );
      assert( eq(ev[1],   5.6) );
      assert( eq(ev[2],   1.0) );
      assert( eq(ev[3],   2.0) );
      assert( eq(ev[4],   1.1) );
      assert( eq(ev[5],   1.0) );
      assert( eq(ev[6],  -2.0) );
      assert( eq(ev[7],   2.8) );
      assert( eq(ev[8],  -1.0) );
      assert( eq(ev[9],   1.0) );
      assert( eq(ev[10],  1.0) );
      assert( eq(ev[11], -1.2) );
      assert( eq(ev[12], -1.0) );
      assert( eq(ev[13],  1.9) );
      
      const CoinBigIndex * mi = smP->getVectorStarts();
      assert( mi[0]==0 );
      assert( mi[1]==2 );
      assert( mi[2]==4 );
      assert( mi[3]==6 );
      assert( mi[4]==8 );
      assert( mi[5]==10 );
      assert( mi[6]==11 );
      assert( mi[7]==12 );
      assert( mi[8]==14 );
      
      const int * ei = smP->getIndices();
      assert( ei[0]  ==  0 );
      assert( ei[1]  ==  4 );
      assert( ei[2]  ==  0 );
      assert( ei[3]  ==  1 );
      assert( ei[4]  ==  1 );
      assert( ei[5]  ==  2 );
      assert( ei[6]  ==  0 );
      assert( ei[7]  ==  3 );
      assert( ei[8]  ==  0 );
      assert( ei[9]  ==  4 );
      assert( ei[10] ==  2 );
      assert( ei[11] ==  3 );
      assert( ei[12] ==  0 );
      assert( ei[13] ==  4 );    
      
      assert( smP->getMajorDim() == 8 ); 
      assert( smP->getNumElements() == 14 );

      assert( smP->getSizeVectorStarts()==9 );
      assert( smP->getMinorDim() == 5 );
      
    }
    //--------------
    // Test rowsense, rhs, rowrange, matrixByRow
    {
      CoinMpsIO lhs;
      {      
        assert( m.rowrange_==NULL );
        assert( m.rowsense_==NULL );
        assert( m.rhs_==NULL );
        assert( m.matrixByRow_==NULL );
        
        CoinMpsIO siC1(m);     
        assert( siC1.rowrange_==NULL );
        assert( siC1.rowsense_==NULL );
        assert( siC1.rhs_==NULL );
        assert( siC1.matrixByRow_==NULL );

        const char   * siC1rs  = siC1.getRowSense();
        assert( siC1rs[0]=='G' );
        assert( siC1rs[1]=='L' );
        assert( siC1rs[2]=='E' );
        assert( siC1rs[3]=='R' );
        assert( siC1rs[4]=='R' );
        
        const double * siC1rhs = siC1.getRightHandSide();
        assert( eq(siC1rhs[0],2.5) );
        assert( eq(siC1rhs[1],2.1) );
        assert( eq(siC1rhs[2],4.0) );
        assert( eq(siC1rhs[3],5.0) );
        assert( eq(siC1rhs[4],15.) ); 
        
        const double * siC1rr  = siC1.getRowRange();
        assert( eq(siC1rr[0],0.0) );
        assert( eq(siC1rr[1],0.0) );
        assert( eq(siC1rr[2],0.0) );
        assert( eq(siC1rr[3],5.0-1.8) );
        assert( eq(siC1rr[4],15.0-3.0) );
        
        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        assert( siC1mbr != NULL );
        
        const double * ev = siC1mbr->getElements();
        assert( eq(ev[0],   3.0) );
        assert( eq(ev[1],   1.0) );
        assert( eq(ev[2],  -2.0) );
        assert( eq(ev[3],  -1.0) );
        assert( eq(ev[4],  -1.0) );
        assert( eq(ev[5],   2.0) );
        assert( eq(ev[6],   1.1) );
        assert( eq(ev[7],   1.0) );
        assert( eq(ev[8],   1.0) );
        assert( eq(ev[9],   2.8) );
        assert( eq(ev[10], -1.2) );
        assert( eq(ev[11],  5.6) );
        assert( eq(ev[12],  1.0) );
        assert( eq(ev[13],  1.9) );
        
        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        assert( mi[0]==0 );
        assert( mi[1]==5 );
        assert( mi[2]==7 );
        assert( mi[3]==9 );
        assert( mi[4]==11 );
        assert( mi[5]==14 );
        
        const int * ei = siC1mbr->getIndices();
        assert( ei[0]  ==  0 );
        assert( ei[1]  ==  1 );
        assert( ei[2]  ==  3 );
        assert( ei[3]  ==  4 );
        assert( ei[4]  ==  7 );
        assert( ei[5]  ==  1 );
        assert( ei[6]  ==  2 );
        assert( ei[7]  ==  2 );
        assert( ei[8]  ==  5 );
        assert( ei[9]  ==  3 );
        assert( ei[10] ==  6 );
        assert( ei[11] ==  0 );
        assert( ei[12] ==  4 );
        assert( ei[13] ==  7 );    
        
        assert( siC1mbr->getMajorDim() == 5 ); 
        assert( siC1mbr->getNumElements() == 14 );
        

        assert( siC1rs  == siC1.getRowSense() );
        assert( siC1rhs == siC1.getRightHandSide() );
        assert( siC1rr  == siC1.getRowRange() );
      }
    }

#ifdef COIN_HAS_GLPK
    // test GMPL reader
    {
      CoinMessageHandler msghandler;
      CoinMpsIO mpsio;
      mpsio.passInMessageHandler(&msghandler);
      int ret = mpsio.readGMPL("plan.mod", NULL, true);
      printf("read plan.mod with return == %d\n", ret);
      assert(ret == 0);

      int nc = mpsio.getNumCols();
      int nr = mpsio.getNumRows();
      const double * cl = mpsio.getColLower();
      const double * cu = mpsio.getColUpper();
      const double * rl = mpsio.getRowLower();
      const double * ru = mpsio.getRowUpper();
      const double * obj = mpsio.getObjCoefficients();
      assert( nc == 7 );
      assert( nr == 8 );

      assert( eq(cl[0],0) );
      assert( eq(cl[1],0) );
      assert( eq(cl[2],400) );
      assert( eq(cl[3],100) );
      assert( eq(cl[4],0) );
      assert( eq(cl[5],0) );
      assert( eq(cl[6],0) );

      assert( eq(cu[0],200) );
      assert( eq(cu[1],2500) );
      assert( eq(cu[2],800) );
      assert( eq(cu[3],700) );
      assert( eq(cu[4],1500) );
      assert( eq(cu[5],mpsio.getInfinity()) );
      assert( eq(cu[6],mpsio.getInfinity()) );

      assert( eq(rl[0],-mpsio.getInfinity()) );
      assert( eq(rl[1],2000) );
      assert( eq(rl[2],-mpsio.getInfinity()) );
      assert( eq(rl[3],-mpsio.getInfinity()) );
      assert( eq(rl[4],-mpsio.getInfinity()) );
      assert( eq(rl[5],-mpsio.getInfinity()) );
      assert( eq(rl[6],1500) );
      assert( eq(rl[7],250) );

      assert( eq(ru[0],mpsio.getInfinity()) );
      assert( eq(ru[1],2000) );
      assert( eq(ru[2],60) );
      assert( eq(ru[3],100) );
      assert( eq(ru[4],40) );
      assert( eq(ru[5],30) );
      assert( eq(ru[6],mpsio.getInfinity()) );
      assert( eq(ru[7],300) );

      assert( eq(obj[0],0.03) );
      assert( eq(obj[1],0.08) );
      assert( eq(obj[2],0.17) );
      assert( eq(obj[3],0.12) );
      assert( eq(obj[4],0.15) );
      assert( eq(obj[5],0.21) );
      assert( eq(obj[6],0.38) );

      const CoinPackedMatrix * matrix = mpsio.getMatrixByRow();
      assert( matrix != NULL );
      assert( matrix->getMajorDim() == nr ); 

      int nel = matrix->getNumElements();
      assert( nel == 48 );

      const double * ev = matrix->getElements();
      assert( ev != NULL );
      const int * ei = matrix->getIndices();
      assert( ei != NULL );
      const CoinBigIndex * mi = matrix->getVectorStarts();
      assert( mi != NULL );

      assert( eq(ev[0],0.03) );
      assert( eq(ev[1],0.08) );
      assert( eq(ev[2],0.17) );
      assert( eq(ev[3],0.12) );
      assert( eq(ev[4],0.15) );
      assert( eq(ev[5],0.21) );
      assert( eq(ev[6],0.38) );
      assert( eq(ev[7],1) );
      assert( eq(ev[8],1) );
      assert( eq(ev[9],1) );
      assert( eq(ev[10],1) );
      assert( eq(ev[11],1) );
      assert( eq(ev[12],1) );
      assert( eq(ev[13],1) );
      assert( eq(ev[14],0.15) );
      assert( eq(ev[15],0.04) );
      assert( eq(ev[16],0.02) );
      assert( eq(ev[17],0.04) );
      assert( eq(ev[18],0.02) );
      assert( eq(ev[19],0.01) );
      assert( eq(ev[20],0.03) );
      assert( eq(ev[21],0.03) );
      assert( eq(ev[22],0.05) );
      assert( eq(ev[23],0.08) );
      assert( eq(ev[24],0.02) );
      assert( eq(ev[25],0.06) );
      assert( eq(ev[26],0.01) );
      assert( eq(ev[27],0.02) );
      assert( eq(ev[28],0.04) );
      assert( eq(ev[29],0.01) );
      assert( eq(ev[30],0.02) );
      assert( eq(ev[31],0.02) );
      assert( eq(ev[32],0.02) );
      assert( eq(ev[33],0.03) );
      assert( eq(ev[34],0.01) );
      assert( eq(ev[35],0.7) );
      assert( eq(ev[36],0.75) );
      assert( eq(ev[37],0.8) );
      assert( eq(ev[38],0.75) );
      assert( eq(ev[39],0.8) );
      assert( eq(ev[40],0.97) );
      assert( eq(ev[41],0.02) );
      assert( eq(ev[42],0.06) );
      assert( eq(ev[43],0.08) );
      assert( eq(ev[44],0.12) );
      assert( eq(ev[45],0.02) );
      assert( eq(ev[46],0.01) );
      assert( eq(ev[47],0.97) );

      assert( ei[0] == 0 );
      assert( ei[1] == 1 );
      assert( ei[2] == 2 );
      assert( ei[3] == 3 );
      assert( ei[4] == 4 );
      assert( ei[5] == 5 );
      assert( ei[6] == 6 );
      assert( ei[7] == 0 );
      assert( ei[8] == 1 );
      assert( ei[9] == 2 );
      assert( ei[10] == 3 );
      assert( ei[11] == 4 );
      assert( ei[12] == 5 );
      assert( ei[13] == 6 );
      assert( ei[14] == 0 );
      assert( ei[15] == 1 );
      assert( ei[16] == 2 );
      assert( ei[17] == 3 );
      assert( ei[18] == 4 );
      assert( ei[19] == 5 );
      assert( ei[20] == 6 );
      assert( ei[21] == 0 );
      assert( ei[22] == 1 );
      assert( ei[23] == 2 );
      assert( ei[24] == 3 );
      assert( ei[25] == 4 );
      assert( ei[26] == 5 );
      assert( ei[27] == 0 );
      assert( ei[28] == 1 );
      assert( ei[29] == 2 );
      assert( ei[30] == 3 );
      assert( ei[31] == 4 );
      assert( ei[32] == 0 );
      assert( ei[33] == 1 );
      assert( ei[34] == 4 );
      assert( ei[35] == 0 );
      assert( ei[36] == 1 );
      assert( ei[37] == 2 );
      assert( ei[38] == 3 );
      assert( ei[39] == 4 );
      assert( ei[40] == 5 );
      assert( ei[41] == 0 );
      assert( ei[42] == 1 );
      assert( ei[43] == 2 );
      assert( ei[44] == 3 );
      assert( ei[45] == 4 );
      assert( ei[46] == 5 );
      assert( ei[47] == 6 );

      assert( mi[0] == 0 );
      assert( mi[1] == 7 );
      assert( mi[2] == 14 );
      assert( mi[3] == 21 );
      assert( mi[4] == 27 );
      assert( mi[5] == 32 );
      assert( mi[6] == 35 );
      assert( mi[7] == 41 );
      assert( mi[8] == 48 );
    }
#endif
  }

}
