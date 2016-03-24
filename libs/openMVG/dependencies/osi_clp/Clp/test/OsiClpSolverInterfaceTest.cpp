// $Id$
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

//#include <cassert>
//#include <cstdlib>
//#include <cstdio>
//#include <iostream>

#include "OsiClpSolverInterface.hpp"
#include "OsiUnitTests.hpp"
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinMessage.hpp"
#include "ClpMessage.hpp"
#include "ClpFactorization.hpp"
#include "CoinModel.hpp"
#include "CoinIndexedVector.hpp"
#include "ClpPlusMinusOneMatrix.hpp"

//#############################################################################

class OsiClpMessageTest :
   public CoinMessageHandler {

public:
  virtual int print() ;
  OsiClpMessageTest();
};

OsiClpMessageTest::OsiClpMessageTest() : CoinMessageHandler()
{
}
int
OsiClpMessageTest::print()
{
  if (currentMessage().externalNumber()==0&&currentSource()=="Clp") 
    std::cout<<"This is not actually an advertisement by Dash Associates - just my feeble attempt to test message handling and language - JJHF"<<std::endl;
  else if (currentMessage().externalNumber()==5&&currentSource()=="Osi") 
    std::cout<<"End of search trapped"<<std::endl;
  return CoinMessageHandler::print();
}

//--------------------------------------------------------------------------
// test EKKsolution methods.
void
OsiClpSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir)
{
  // Test default constructor
  {
    OsiClpSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(m.rowsense_           == NULL, {}, "clp", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rhs_                == NULL, {}, "clp", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowrange_           == NULL, {}, "clp", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_        == NULL, {}, "clp", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.ws_                 == NULL, {}, "clp", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.itlimOrig_       == 9999999, {}, "clp", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.lastAlgorithm_      == 0,    {}, "clp", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.integerInformation_ == NULL, {}, "clp", "default constructor");
  }

  {    
    CoinRelFltEq eq;
    OsiClpSolverInterface m;
    std::string fn = mpsDir+"exmip1";
    m.readMps(fn.c_str(),"mps");
    
    {
      OsiClpSolverInterface im;    
      
      OSIUNITTEST_ASSERT_ERROR(im.getNumCols()  == 0,    {}, "clp", "default constructor");
      OSIUNITTEST_ASSERT_ERROR(im.getModelPtr() != NULL, {}, "clp", "default constructor");

      // Test reset
      im.reset();
      OSIUNITTEST_ASSERT_ERROR(im.rowsense_           == NULL, {}, "clp", "reset");
      OSIUNITTEST_ASSERT_ERROR(im.rhs_                == NULL, {}, "clp", "reset");
      OSIUNITTEST_ASSERT_ERROR(im.rowrange_           == NULL, {}, "clp", "reset");
      OSIUNITTEST_ASSERT_ERROR(im.matrixByRow_        == NULL, {}, "clp", "reset");
      OSIUNITTEST_ASSERT_ERROR(im.ws_                 == NULL, {}, "clp", "reset");
      OSIUNITTEST_ASSERT_ERROR(im.itlimOrig_       == 9999999, {}, "clp", "reset");
      OSIUNITTEST_ASSERT_ERROR(im.lastAlgorithm_      ==    0, {}, "clp", "reset");
      OSIUNITTEST_ASSERT_ERROR(im.integerInformation_ == NULL, {}, "clp", "reset");
    }
    
    // Test copy constructor and assignment operator
    {
      OsiClpSolverInterface lhs;
      {      
        OsiClpSolverInterface im(m);        
        
        OsiClpSolverInterface imC1(im);
        OSIUNITTEST_ASSERT_ERROR(imC1.getModelPtr() != im.getModelPtr(), {}, "clp", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumCols()  == im.getNumCols(),  {}, "clp", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumRows()  == im.getNumRows(),  {}, "clp", "copy constructor");
        
        OsiClpSolverInterface imC2(im);
        OSIUNITTEST_ASSERT_ERROR(imC2.getModelPtr() != im.getModelPtr(), {}, "clp", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumCols()  == im.getNumCols(),  {}, "clp", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumRows()  == im.getNumRows(),  {}, "clp", "copy constructor");

        OSIUNITTEST_ASSERT_ERROR(imC1.getModelPtr() != imC2.getModelPtr(), {}, "clp", "copy constructor");
        
        lhs = imC2;
      }

      // Test that lhs has correct values even though rhs has gone out of scope
      OSIUNITTEST_ASSERT_ERROR(lhs.getModelPtr() != m.getModelPtr(), {}, "clp", "assignment operator");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumCols()  == m.getNumCols(),  {}, "clp", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumRows()  == m.getNumRows(),  {}, "clp", "copy constructor");
    }

    // Test clone
    {
      OsiClpSolverInterface clpSi(m);
      OsiSolverInterface * siPtr = &clpSi;
      OsiSolverInterface * siClone = siPtr->clone();
      OsiClpSolverInterface * clpClone = dynamic_cast<OsiClpSolverInterface*>(siClone);
      
      OSIUNITTEST_ASSERT_ERROR(clpClone != NULL, {}, "clp", "clone");
      OSIUNITTEST_ASSERT_ERROR(clpClone->getModelPtr() != clpSi.getModelPtr(), {}, "clp", "clone");
      OSIUNITTEST_ASSERT_ERROR(clpClone->getNumRows() == clpSi.getNumRows(),   {}, "clp", "clone");
      OSIUNITTEST_ASSERT_ERROR(clpClone->getNumCols() == m.getNumCols(),       {}, "clp", "clone");

      delete siClone;
    }
  
    // Test infinity
    {
      OsiClpSolverInterface si;
      OSIUNITTEST_ASSERT_ERROR(si.getInfinity() == OsiClpInfinity, {}, "clp", "infinity");
    }
    
    // Test some catches
    if (!OsiClpHasNDEBUG())
    {
      OsiClpSolverInterface solver;
      try {
        solver.setObjCoeff(0,0.0);
        OSIUNITTEST_ADD_OUTCOME("clp", "setObjCoeff on empty model", "should throw exception", OsiUnitTest::TestOutcome::ERROR, false);
      }
      catch (CoinError e) {
        if (OsiUnitTest::verbosity >= 1)
          std::cout<<"Correct throw from setObjCoeff on empty model"<<std::endl;
      }

      std::string fn = mpsDir+"exmip1";
      solver.readMps(fn.c_str(),"mps");
      OSIUNITTEST_CATCH_ERROR(solver.setObjCoeff(0,0.0), {}, "clp", "setObjCoeff on nonempty model");

      try {
        int index[]={0,20};
        double value[]={0.0,0.0,0.0,0.0};
        solver.setColSetBounds(index,index+2,value);
        OSIUNITTEST_ADD_OUTCOME("clp", "setColSetBounds on cols not in model", "should throw exception", OsiUnitTest::TestOutcome::ERROR, false);
      }
      catch (CoinError e) {
        if (OsiUnitTest::verbosity >= 1)
          std::cout<<"Correct throw from setObjCoeff on empty model"<<std::endl;
      }
    }
    
    {
      OsiClpSolverInterface clpSi(m);
      int nc = clpSi.getNumCols();
      int nr = clpSi.getNumRows();
      const double * cl = clpSi.getColLower();
      const double * cu = clpSi.getColUpper();
      const double * rl = clpSi.getRowLower();
      const double * ru = clpSi.getRowUpper();
      OSIUNITTEST_ASSERT_ERROR(nc == 8, return, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(nr == 5, return, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[0],2.5), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[1],0.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[1],4.1), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[2],1.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[0],2.5), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[4],3.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[1],2.1), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[4],15.), {}, "clp", "read and copy exmip1");
      
      const double * cs = clpSi.getColSolution();
      OSIUNITTEST_ASSERT_ERROR(eq(cs[0],2.5), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cs[7],0.0), {}, "clp", "read and copy exmip1");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(cl[3],1.2345), {}, "clp", "set col lower");
      clpSi.setColLower( 3, 1.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cl[3],1.2345), {}, "clp", "set col lower");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(clpSi.getColUpper()[4],10.2345), {}, "clp", "set col upper");
      clpSi.setColUpper( 4, 10.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(clpSi.getColUpper()[4],10.2345), {}, "clp", "set col upper");

      double objValue = clpSi.getObjValue();
      OSIUNITTEST_ASSERT_ERROR(eq(objValue,3.5), {}, "clp", "getObjValue() before solve");

      OSIUNITTEST_ASSERT_ERROR(eq(clpSi.getObjCoefficients()[0], 1.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(clpSi.getObjCoefficients()[1], 0.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(clpSi.getObjCoefficients()[2], 0.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(clpSi.getObjCoefficients()[3], 0.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(clpSi.getObjCoefficients()[4], 2.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(clpSi.getObjCoefficients()[5], 0.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(clpSi.getObjCoefficients()[6], 0.0), {}, "clp", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(clpSi.getObjCoefficients()[7],-1.0), {}, "clp", "read and copy exmip1");
    }
    
    // Test matrixByRow method
    {
      const OsiClpSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();

      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  5, return, "clp", "getMatrixByRow: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "clp", "getMatrixByRow: num elements");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[0],   3.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[1],   1.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[2],  -2.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[3],  -1.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[4],  -1.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[5],   2.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[6],   1.1), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[7],   1.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[8],   1.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[9],   2.8), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], -1.2), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11],  5.6), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12],  1.0), {}, "clp", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13],  1.9), {}, "clp", "getMatrixByRow: elements");
      
      const int * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "clp", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "clp", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "clp", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "clp", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "clp", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "clp", "getMatrixByRow: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "clp", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "clp", "getMatrixByRow: indices");
    }

    // Test adding several cuts
    {
      OsiClpSolverInterface fim;
      std::string fn = mpsDir+"exmip1";
      fim.readMps(fn.c_str(),"mps");
      // exmip1.mps has 2 integer variables with index 2 & 3
      fim.initialSolve();
      OsiRowCut cuts[3];

      // Generate one ineffective cut plus two trivial cuts
      int c;
      int nc = fim.getNumCols();
      int *inx = new int[nc];
      for (c=0;c<nc;c++) inx[c]=c;
      double *el = new double[nc];
      for (c=0;c<nc;c++) el[c]=1.0e-50+((double)c)*((double)c);
      
      cuts[0].setRow(nc,inx,el);
      cuts[0].setLb(-100.);
      cuts[0].setUb(500.);
      cuts[0].setEffectiveness(22);
      el[4]=0.0; // to get inf later
      
      for (c=2;c<4;c++) {
        el[0]=1.0;
        inx[0]=c;
        cuts[c-1].setRow(1,inx,el);
        cuts[c-1].setLb(1.);
        cuts[c-1].setUb(100.);
        cuts[c-1].setEffectiveness(c);
      }
      fim.writeMps("x1.mps");
      fim.applyRowCuts(3,cuts);
      fim.writeMps("x2.mps");
      // resolve - should get message about zero elements
      fim.resolve();
      fim.writeMps("x3.mps");
      // check integer solution
      const double * cs = fim.getColSolution();
      CoinRelFltEq eq;
      OSIUNITTEST_ASSERT_ERROR(eq(cs[2], 1.0), {}, "clp", "add cuts");
      OSIUNITTEST_ASSERT_ERROR(eq(cs[3], 1.0), {}, "clp", "add cuts");
      // check will find invalid matrix
      el[0]=1.0/el[4];
      inx[0]=0;
      cuts[0].setRow(nc,inx,el);
      cuts[0].setLb(-100.);
      cuts[0].setUb(500.);
      cuts[0].setEffectiveness(22);
      fim.applyRowCut(cuts[0]);
      // resolve - should get message about zero elements
      fim.resolve();
      OSIUNITTEST_ASSERT_WARNING(fim.isAbandoned(), {}, "clp", "add cuts");
      delete[]el;
      delete[]inx;
    }

    // Test using bad basis
    {
      OsiClpSolverInterface fim;
      std::string fn = mpsDir+"exmip1";
      fim.readMps(fn.c_str(),"mps");
      fim.initialSolve();
      int numberRows = fim.getModelPtr()->numberRows();
      int * rowStatus = new int[numberRows];
      int numberColumns = fim.getModelPtr()->numberColumns();
      int * columnStatus = new int[numberColumns];
      fim.getBasisStatus(columnStatus,rowStatus);
      for (int i=0; i<numberRows; i++)
        rowStatus[i]=2;
      fim.setBasisStatus(columnStatus,rowStatus);
      fim.resolve();
      delete [] rowStatus;
      delete [] columnStatus;
    }

    // Test matrixByCol method
    {
      const OsiClpSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByCol();
      
      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  8, return, "clp", "getMatrixByCol: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getMinorDim()    ==  5, return, "clp", "getMatrixByCol: minor dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "clp", "getMatrixByCol: number of elements");
      OSIUNITTEST_ASSERT_ERROR(smP->getSizeVectorStarts() == 9, return, "clp", "getMatrixByCol: vector starts size");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 5.6), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2], 1.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3], 2.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4], 1.1), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 1.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6],-2.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 2.8), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8],-1.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 1.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], 1.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11],-1.2), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12],-1.0), {}, "clp", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "clp", "getMatrixByCol: elements");
      
      const CoinBigIndex * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "clp", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  2, {}, "clp", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  4, {}, "clp", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  6, {}, "clp", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] ==  8, {}, "clp", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 10, {}, "clp", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[6] == 11, {}, "clp", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[7] == 12, {}, "clp", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[8] == 14, {}, "clp", "getMatrixByCol: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 4, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 0, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 1, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 1, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 2, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 0, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 3, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 0, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 4, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 2, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 3, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 0, {}, "clp", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 4, {}, "clp", "getMatrixByCol: indices");
    }

    //--------------
    // Test rowsense, rhs, rowrange, matrixByRow
    {
      OsiClpSolverInterface lhs;
      {      
        OsiClpSolverInterface siC1(m);
        OSIUNITTEST_ASSERT_WARNING(siC1.rowrange_ == NULL, {}, "clp", "row range");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsense_ == NULL, {}, "clp", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1.rhs_ == NULL, {}, "clp", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1.matrixByRow_ == NULL, {}, "clp", "matrix by row");

        const char   * siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "clp", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "clp", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "clp", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "clp", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "clp", "row sense");

        const double * siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "clp", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "clp", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "clp", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "clp", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "clp", "right hand side");

        const double * siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "clp", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "clp", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "clp", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "clp", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "clp", "row range");

        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        OSIUNITTEST_ASSERT_ERROR(siC1mbr != NULL, {}, "clp", "matrix by row");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getMajorDim()    ==  5, return, "clp", "matrix by row: major dim");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getNumElements() == 14, return, "clp", "matrix by row: num elements");

        const double * ev = siC1mbr->getElements();
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "clp", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "clp", "matrix by row: elements");

        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "clp", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "clp", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "clp", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "clp", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "clp", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "clp", "matrix by row: vector starts");

        const int * ei = siC1mbr->getIndices();
        OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "clp", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "clp", "matrix by row: indices");

        OSIUNITTEST_ASSERT_WARNING(siC1rs  == siC1.getRowSense(), {}, "clp", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1rhs == siC1.getRightHandSide(), {}, "clp", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1rr  == siC1.getRowRange(), {}, "clp", "row range");

        // Change CLP Model by adding free row
        OsiRowCut rc;
        rc.setLb(-COIN_DBL_MAX);
        rc.setUb( COIN_DBL_MAX);
        OsiCuts cuts;
        cuts.insert(rc);
        siC1.applyCuts(cuts);

        // Since model was changed, test that cached data is now freed.
        OSIUNITTEST_ASSERT_ERROR(siC1.rowrange_ == NULL, {}, "clp", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsense_ == NULL, {}, "clp", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rhs_ == NULL, {}, "clp", "free cached data after adding row");
        // siC1.matrixByRow_ is updated, so it does not have to be NULL

        siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "clp", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "clp", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "clp", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "clp", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "clp", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[5] == 'N', {}, "clp", "row sense after adding row");

        siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "clp", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "clp", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "clp", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "clp", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "clp", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[5],0.0), {}, "clp", "right hand side after adding row");

        siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "clp", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "clp", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "clp", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "clp", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "clp", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[5],0.0), {}, "clp", "row range after adding row");

        lhs = siC1;
      }
      // Test that lhs has correct values even though siC1 has gone out of scope
      OSIUNITTEST_ASSERT_ERROR(lhs.rowrange_ == NULL, {}, "clp", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsense_ == NULL, {}, "clp", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rhs_ == NULL, {}, "clp", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByRow_ == NULL, {}, "clp", "freed origin after assignment");

      const char * lhsrs  = lhs.getRowSense();
      OSIUNITTEST_ASSERT_ERROR(lhsrs[0] == 'G', {}, "clp", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[1] == 'L', {}, "clp", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[2] == 'E', {}, "clp", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[3] == 'R', {}, "clp", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[4] == 'R', {}, "clp", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[5] == 'N', {}, "clp", "row sense after assignment");
      
      const double * lhsrhs = lhs.getRightHandSide();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[0],2.5), {}, "clp", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[1],2.1), {}, "clp", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[2],4.0), {}, "clp", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[3],5.0), {}, "clp", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[4],15.), {}, "clp", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[5],0.0), {}, "clp", "right hand side after assignment");
      
      const double *lhsrr = lhs.getRowRange();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[0],0.0), {}, "clp", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[1],0.0), {}, "clp", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[2],0.0), {}, "clp", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[3],5.0-1.8), {}, "clp", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[4],15.0-3.0), {}, "clp", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[5],0.0), {}, "clp", "row range after assignment");
      
      const CoinPackedMatrix * lhsmbr = lhs.getMatrixByRow();
      OSIUNITTEST_ASSERT_ERROR(lhsmbr != NULL, {}, "clp", "matrix by row after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getMajorDim()    ==  6, return, "clp", "matrix by row after assignment: major dim");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getNumElements() == 14, return, "clp", "matrix by row after assignment: num elements");

      const double * ev = lhsmbr->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "clp", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "clp", "matrix by row after assignment: elements");
      
      const CoinBigIndex * mi = lhsmbr->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "clp", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "clp", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "clp", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "clp", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "clp", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "clp", "matrix by row after assignment: vector starts");
      
      const int * ei = lhsmbr->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "clp", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "clp", "matrix by row after assignment: indices");
    }
  }

  // Test ClpPlusMinusOneMatrix by way of loadProblem(ClpMatrixBase, ... )
  { int pos_start[4] = {0,5,9,12};
    int neg_start[4] = {3,7,11,12};
    int col[12] = {0,1,2,3,4,5,6,7,0,1,2,3};
    double rhs[3] = {0.0,0.0,0.0};
    double cost[8];
    double var_lb[8];
    double var_ub[8];
    for (int i = 0 ; i < 8 ; i++) {
      cost[i] = 1.0;
      var_lb[i] = 0.0;
      var_ub[i] = 1.0;
    }
    ClpPlusMinusOneMatrix pmone_matrix(3,8,false,col,pos_start,neg_start);
    OsiClpSolverInterface clpSi;
    OSIUNITTEST_CATCH_ERROR(
        {clpSi.loadProblem(pmone_matrix,var_lb,var_ub,cost,rhs,rhs);
	 clpSi.initialSolve();},
	{},"clp","loadProblem(ClpMatrixBase, ...)")
  }


  // Test add/delete columns
  {    
    OsiClpSolverInterface m;
    std::string fn = mpsDir+"p0033";
    m.readMps(fn.c_str(),"mps");
    double inf = m.getInfinity();

    CoinPackedVector c0;
    c0.insert(0, 4);
    c0.insert(1, 1);
    m.addCol(c0, 0, inf, 3);
    m.initialSolve();
    double objValue = m.getObjValue();
    CoinRelFltEq eq(1.0e-2);
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,2520.57), {}, "clp", "objvalue after adding col");

    // Try deleting first column
    int * d = new int[1];
    d[0]=0;
    m.deleteCols(1,d);
    delete [] d;
    d=NULL;
    m.resolve();
    objValue = m.getObjValue();
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,2520.57), {}, "clp", "objvalue after deleting first col");

    // Try deleting column we added
    int iCol = m.getNumCols()-1;
    m.deleteCols(1,&iCol);
    m.resolve();
    objValue = m.getObjValue();
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,2520.57), {}, "clp", "objvalue after deleting added col");
  }

  // Test branch and bound
  {    
    OsiClpSolverInterface m;
    std::string fn = mpsDir+"p0033";
    m.readMps(fn.c_str(),"mps");
    // reduce printout
    m.setHintParam(OsiDoReducePrint,true,OsiHintTry);
    // test maximization
    int n  = m.getNumCols();
    int i;
    double * obj2 = new double[n];
    const double * obj = m.getObjCoefficients();
    for (i=0;i<n;i++) {
      obj2[i]=-obj[i];
    }
    m.setObjective(obj2);
    delete [] obj2;
    m.setObjSense(-1.0);
    // Save bounds
    double * saveUpper = CoinCopyOfArray(m.getColUpper(),n);
    double * saveLower = CoinCopyOfArray(m.getColLower(),n);
    m.branchAndBound();
    // reset bounds
    m.setColUpper(saveUpper);
    m.setColLower(saveLower);
    delete [] saveUpper;
    delete [] saveLower;
    // reset cutoff - otherwise we won't get solution
    m.setDblParam(OsiDualObjectiveLimit,-COIN_DBL_MAX);
    m.branchAndBound();
    double objValue = m.getObjValue();
    CoinRelFltEq eq(1.0e-2);
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,-3089), {}, "clp", "branch and bound");
    const double * cs = m.getColSolution();
    if( OsiUnitTest::verbosity >= 2 )
      for ( i=0;i<n;i++) {
        if (cs[i]>1.0e-7)
          printf("%d has value %g\n",i,cs[i]);
    }
  }

  // Test infeasible bounds
  {
    OsiClpSolverInterface solver;
    std::string fn = mpsDir+"exmip1";
    solver.readMps(fn.c_str(),"mps");
    int index[]={0};
    double value[]={1.0,0.0};
    solver.setColSetBounds(index,index+1,value);
    solver.setHintParam(OsiDoPresolveInInitial, false, OsiHintDo);
    solver.initialSolve();
    OSIUNITTEST_ASSERT_ERROR(!solver.isProvenOptimal(), {}, "clp", "infeasible bounds");
  }

  // Build a model
  {    
    OsiClpSolverInterface model;
    std::string fn = mpsDir+"p0033";
    model.readMps(fn.c_str(),"mps");
    // Point to data
    int numberRows = model.getNumRows();
    const double * rowLower = model.getRowLower();
    const double * rowUpper = model.getRowUpper();
    int numberColumns = model.getNumCols();
    const double * columnLower = model.getColLower();
    const double * columnUpper = model.getColUpper();
    const double * columnObjective = model.getObjCoefficients();
    // get row copy
    CoinPackedMatrix rowCopy = *model.getMatrixByRow();
    const int * column = rowCopy.getIndices();
    const int * rowLength = rowCopy.getVectorLengths();
    const CoinBigIndex * rowStart = rowCopy.getVectorStarts();
    const double * element = rowCopy.getElements();
    
    // solve
    model.initialSolve();
    // Now build new model
    CoinModel build;
    // Row bounds
    int iRow;
    for (iRow=0;iRow<numberRows;iRow++) {
      build.setRowBounds(iRow,rowLower[iRow],rowUpper[iRow]);
      build.setRowName(iRow,model.getRowName(iRow).c_str());
    }
    // Column bounds and objective
    int iColumn;
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      build.setColumnLower(iColumn,columnLower[iColumn]);
      build.setColumnUpper(iColumn,columnUpper[iColumn]);
      build.setObjective(iColumn,columnObjective[iColumn]);
      build.setColumnName(iColumn,model.getColName(iColumn).c_str());
    }
    // Adds elements one by one by row (backwards by row)
    for (iRow=numberRows-1;iRow>=0;iRow--) {
      int start = rowStart[iRow];
      for (int j=start;j<start+rowLength[iRow];j++) 
        build(iRow,column[j],element[j]);
    }
    // Now create Model
    OsiClpSolverInterface model2;
    model2.loadFromCoinModel(build);
    model2.initialSolve();
    // Save - should be continuous
    model2.writeMps("continuous");
    int * whichInteger = new int[numberColumns];
    for (iColumn=0;iColumn<numberColumns;iColumn++) 
      whichInteger[iColumn]=iColumn;
    // mark as integer
    model2.setInteger(whichInteger,numberColumns);
    delete [] whichInteger;
    // save - should be integer
    model2.writeMps("integer");
    // check names are there
    if (OsiUnitTest::verbosity >= 1)
      for (iColumn=0;iColumn<numberColumns;iColumn++)
        printf("%d name %s\n",iColumn,model2.getColName(iColumn).c_str());
    
    // Now do with strings attached
    // Save build to show how to go over rows
    CoinModel saveBuild = build;
    build = CoinModel();
    // Column bounds
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      build.setColumnLower(iColumn,columnLower[iColumn]);
      build.setColumnUpper(iColumn,columnUpper[iColumn]);
    }
    // Objective - half the columns as is and half with multiplier of "1.0+multiplier"
    // Pick up from saveBuild (for no reason at all)
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      double value = saveBuild.objective(iColumn);
      if (iColumn*2<numberColumns) {
        build.setObjective(iColumn,columnObjective[iColumn]);
      } else {
        // create as string
        char temp[100];
        sprintf(temp,"%g + abs(%g*multiplier)",value,value);
        build.setObjective(iColumn,temp);
      }
    }
    // It then adds rows one by one but for half the rows sets their values
    //      with multiplier of "1.0+1.5*multiplier"
    for (iRow=0;iRow<numberRows;iRow++) {
      if (iRow*2<numberRows) {
        // add row in simple way
        int start = rowStart[iRow];
        build.addRow(rowLength[iRow],column+start,element+start,
                     rowLower[iRow],rowUpper[iRow]);
      } else {
        // As we have to add one by one let's get from saveBuild
        CoinModelLink triple=saveBuild.firstInRow(iRow);
        while (triple.column()>=0) {
          int iColumn = triple.column();
          if (iColumn*2<numberColumns) {
            // just value as normal
            build(iRow,triple.column(),triple.value());
          } else {
            // create as string
            char temp[100];
            sprintf(temp,"%g + (1.5*%g*multiplier)",triple.value(), triple.value());
            build(iRow,iColumn,temp);
          }
          triple=saveBuild.next(triple);
        }
        // but remember to do rhs
        build.setRowLower(iRow,rowLower[iRow]);
        build.setRowUpper(iRow,rowUpper[iRow]);
      }
    }
    // If small switch on error printing
    if (numberColumns<50)
      build.setLogLevel(1);
    // should fail as we never set multiplier
    OSIUNITTEST_ASSERT_ERROR(model2.loadFromCoinModel(build) != 0, {}, "clp", "build model with missing multipliers");
    build.associateElement("multiplier",0.0);
    OSIUNITTEST_ASSERT_ERROR(model2.loadFromCoinModel(build) == 0, {}, "clp", "build model");
    model2.initialSolve();
    // It then loops with multiplier going from 0.0 to 2.0 in increments of 0.1
    for (double multiplier=0.0;multiplier<2.0;multiplier+= 0.1) {
      build.associateElement("multiplier",multiplier);
      OSIUNITTEST_ASSERT_ERROR(model2.loadFromCoinModel(build,true) == 0, {}, "clp", "build model with increasing multiplier");
      model2.resolve();
    }
  }

  // Solve an lp by hand
  {    
    OsiClpSolverInterface m;
    std::string fn = mpsDir+"p0033";
    m.readMps(fn.c_str(),"mps");
    m.setObjSense(-1.0);
    m.getModelPtr()->messageHandler()->setLogLevel(4);
    m.initialSolve();
    m.getModelPtr()->factorization()->maximumPivots(5);
    m.setObjSense(1.0);
    // clone to test pivot as well as primalPivot
    OsiSolverInterface * mm =m.clone();
    // enable special mode
    m.enableSimplexInterface(true);
    // need to do after clone 
    mm->enableSimplexInterface(true);
    // we happen to know that variables are 0-1 and rows are L
    int numberIterations=0;
    int numberColumns = m.getNumCols();
    int numberRows = m.getNumRows();
    double * fakeCost = new double[numberColumns];
    double * duals = new double [numberRows];
    double * djs = new double [numberColumns];
    const double * solution = m.getColSolution();
    memcpy(fakeCost,m.getObjCoefficients(),numberColumns*sizeof(double));
    while (1) {
      const double * dj;
      const double * dual;
      if ((numberIterations&1)==0) {
        // use given ones
        dj = m.getReducedCost();
        dual = m.getRowPrice();
      } else {
        // create
        dj = djs;
        dual = duals;
        m.getReducedGradient(djs,duals,fakeCost);
      }
      int i;
      int colIn=9999;
      int direction=1;
      double best=1.0e-6;
      // find most negative reduced cost
      // Should check basic - but should be okay on this problem
      for (i=0;i<numberRows;i++) {
        double value=dual[i];
        if (value>best) {
          direction=-1;
          best=value;
          colIn=-i-1;
        }
      }
      for (i=0;i<numberColumns;i++) {
        double value=dj[i];
        if (value<-best&&solution[i]<1.0e-6) {
          direction=1;
          best=-value;
          colIn=i;
        } else if (value>best&&solution[i]>1.0-1.0e-6) {
          direction=-1;
          best=value;
          colIn=i;
        }
      }
      if (colIn==9999)
        break; // should be optimal
      int colOut;
      int outStatus;
      double theta;
      OSIUNITTEST_ASSERT_ERROR(m.primalPivotResult(colIn,direction,colOut,outStatus,theta,NULL) == 0, {}, "clp", "solve model by hand");
      if (OsiUnitTest::verbosity >= 1)
        printf("out %d, direction %d theta %g\n", colOut,outStatus,theta);
      if (colIn!=colOut) {
        OSIUNITTEST_ASSERT_ERROR(mm->pivot(colIn,colOut,outStatus) >= 0, {}, "clp", "solve model by hand");
      } else {
        // bound flip (so pivot does not make sense)
        OSIUNITTEST_ASSERT_ERROR(mm->primalPivotResult(colIn,direction,colOut,outStatus,theta,NULL) == 0, {}, "clp", "solve model by hand");
      }
      numberIterations++;
    }
    delete mm;
    delete [] fakeCost;
    delete [] duals;
    delete [] djs;
    // exit special mode
    m.disableSimplexInterface();
    m.getModelPtr()->messageHandler()->setLogLevel(4);
    m.messageHandler()->setLogLevel(0);
    m.resolve();
    OSIUNITTEST_ASSERT_ERROR(m.getIterationCount() == 0, {}, "clp", "resolve after solving model by hand");
    m.setObjSense(-1.0);
    m.initialSolve();
  }

# if 0
/*
  This section stops working without setObjectiveAndRefresh. Assertion failure
  down in the guts of clp, likely due to reduced costs not properly updated.
  Leave the code in for a bit so it's easily recoverable if anyone actually
  yells about the loss. There was no response to a public announcement
  of intent to delete, but sometimes it takes a whack on the head to get
  peoples' attention. At some point, it'd be good to come back through and
  make this work again. -- lh, 100828 --
*/
  // Do parametrics on the objective by hand
  {
    std::cout << " Beginning Osi Simplex mode 2 ... " << std::endl ;
    OsiClpSolverInterface m;
    std::string fn = mpsDir+"p0033";
    m.readMps(fn.c_str(),"mps");
    ClpSimplex * simplex = m.getModelPtr();
    simplex->messageHandler()->setLogLevel(4);
    m.initialSolve();
    simplex->factorization()->maximumPivots(5);
    simplex->messageHandler()->setLogLevel(63);
    m.setObjSense(1.0);
    // enable special mode
    m.enableSimplexInterface(true);
    int numberIterations=0;
    int numberColumns = m.getNumCols();
    int numberRows = m.getNumRows();
    double * changeCost = new double[numberColumns];
    double * duals = new double [numberRows];
    double * djs = new double [numberColumns];
    // As getReducedGradient mucks about with innards of Clp get arrays to save
    double * dualsNow = new double [numberRows];
    double * djsNow = new double [numberColumns];
   
    const double * solution = m.getColSolution();
    int i;
    // Set up change cost
    for (i=0;i<numberColumns;i++)
      changeCost[i]=1.0+0.1*i;;
    // Range of investigation
    double totalChange=100.0;
    double totalDone=0.0;
    while (true) {
      std::cout << " Starting iterations ... " << std::endl ;
      // Save current
      // (would be more accurate to start from scratch)
      memcpy(djsNow, m.getReducedCost(),numberColumns*sizeof(double));
      memcpy(dualsNow,m.getRowPrice(),numberRows*sizeof(double));
      // Get reduced gradient of changeCost
      m.getReducedGradient(djs,duals,changeCost);
      int colIn=9999;
      int direction=1;
      // We are going up to totalChange but we have done some
      double best=totalChange-totalDone;
      // find best ratio
      // Should check basic - but should be okay on this problem
      // We are cheating here as we know L rows
      // Really should be using L and U status but I modified from previous example
      for (i=0;i<numberRows;i++) {
        if (simplex->getRowStatus(i)==ClpSimplex::basic) {
          assert (fabs(dualsNow[i])<1.0e-4&&fabs(duals[i])<1.0e-4);
        } else {
          assert (dualsNow[i]<1.0e-4);
          if (duals[i]>1.0e-8) {
            if (dualsNow[i]+best*duals[i]>0.0) {
              best = CoinMax(-dualsNow[i]/duals[i],0.0);
              direction=-1;
              colIn=-i-1;
            }
          }
        }
      }
      for (i=0;i<numberColumns;i++) {
        if (simplex->getColumnStatus(i)==ClpSimplex::basic) {
          assert (fabs(djsNow[i])<1.0e-4&&fabs(djs[i])<1.0e-4);
        } else {
          if (solution[i]<1.0e-6) {
            assert (djsNow[i]>-1.0e-4);
            if (djs[i]<-1.0e-8) {
              if (djsNow[i]+best*djs[i]<0.0) {
                best = CoinMax(-djsNow[i]/djs[i],0.0);
                direction=1;
                colIn=i;
              }
            }
          } else if (solution[i]>1.0-1.0e-6) {
            assert (djsNow[i]<1.0e-4);
            if (djs[i]>1.0e-8) {
              if (djsNow[i]+best*djs[i]>0.0) {
                best = CoinMax(-djsNow[i]/djs[i],0.0);
                direction=-1;
                colIn=i;
              }
            }
          }
        }
      }
      if (colIn==9999)
        break; // should be optimal
      // update objective - djs is spare array
      const double * obj = m.getObjCoefficients();
      for (i=0;i<numberColumns;i++) {
        djs[i]=obj[i]+best*changeCost[i];
      }
      totalDone += best;
      printf("Best change %g, total %g\n",best,totalDone);
      m.setObjectiveAndRefresh(djs);
      int colOut;
      int outStatus;
      double theta;
      int returnCode=m.primalPivotResult(colIn,direction,colOut,outStatus,theta,NULL);
      assert (!returnCode);
      double objValue = m.getObjValue(); // printed one may not be accurate
      printf("in %d out %d, direction %d theta %g, objvalue %g\n",
             colIn,colOut,outStatus,theta,objValue);
      numberIterations++;
    }
    // update objective to totalChange- djs is spare array
    const double * obj = m.getObjCoefficients();
    double best = totalChange-totalDone;
    for (i=0;i<numberColumns;i++) {
      djs[i]=obj[i]+best*changeCost[i];
    }
    delete [] changeCost;
    delete [] duals;
    delete [] djs;
    delete [] dualsNow;
    delete [] djsNow;
    // exit special mode
    std::cout << " Finished simplex mode 2 ; checking result." << std::endl ;
    m.disableSimplexInterface();
    simplex->messageHandler()->setLogLevel(4);
    m.resolve();
    assert (!m.getIterationCount());
  }
# endif

  // Solve an lp when interface is on
  {    
    OsiClpSolverInterface m;
    std::string fn = mpsDir+"p0033";
    m.readMps(fn.c_str(),"mps");
    // enable special mode
    m.setHintParam(OsiDoScale,false,OsiHintDo);
    m.setHintParam(OsiDoPresolveInInitial,false,OsiHintDo);
    m.setHintParam(OsiDoDualInInitial,false,OsiHintDo);
    m.setHintParam(OsiDoPresolveInResolve,false,OsiHintDo);
    m.setHintParam(OsiDoDualInResolve,false,OsiHintDo);
    m.enableSimplexInterface(true);
    m.initialSolve();
  }

  // Check tableau stuff when simplex interface is on
  {    
    OsiClpSolverInterface m;
    /* 
       Wolsey : Page 130
       max 4x1 -  x2
       7x1 - 2x2    <= 14
       x2    <= 3
       2x1 - 2x2    <= 3
       x1 in Z+, x2 >= 0
    */
    
    double inf_ = m.getInfinity();
    int n_cols = 2;
    int n_rows = 3;
    
    double obj[2] = {-4.0, 1.0};
    double collb[2] = {0.0, 0.0};
    double colub[2] = {inf_, inf_};
    double rowlb[3] = {-inf_, -inf_, -inf_};
    double rowub[3] = {14.0, 3.0, 3.0};
    
    int rowIndices[5] =  {0,     2,    0,    1,    2};
    int colIndices[5] =  {0,     0,    1,    1,    1};
    double elements[5] = {7.0, 2.0, -2.0,  1.0, -2.0};
    CoinPackedMatrix M(true, rowIndices, colIndices, elements, 5);
    
    m.loadProblem(M, collb, colub, obj, rowlb, rowub);
    m.enableSimplexInterface(true);
    
    m.initialSolve();
    
    //check that the tableau matches wolsey (B-1 A)
    // slacks in second part of binvA
    double * binvA = (double*) malloc((n_cols+n_rows) * sizeof(double));
    
    if (OsiUnitTest::verbosity >= 2)
      printf("B-1 A");
    int i;
    for( i = 0; i < n_rows; i++){
      m.getBInvARow(i, binvA,binvA+n_cols);
      if (OsiUnitTest::verbosity >= 2) {
        printf("\nrow: %d -> ",i);
        for(int j=0; j < n_cols+n_rows; j++)
          printf("%g, ", binvA[j]);
      }
    }
    if (OsiUnitTest::verbosity >= 2) {
      printf("\n");
      printf("And by column");
    }
    for( i = 0; i < n_cols+n_rows; i++){
      m.getBInvACol(i, binvA);
      if (OsiUnitTest::verbosity >= 2) {
        printf("\ncolumn: %d -> ",i);
        for(int j=0; j < n_rows; j++)
          printf("%g, ", binvA[j]);
      }
    }
    if (OsiUnitTest::verbosity >= 2)
      printf("\n");

    // and when doing as expert
    m.setSpecialOptions(m.specialOptions()|512);
    ClpSimplex * clp = m.getModelPtr();
    /* Do twice -
       first time with enableSimplexInterface still set
       then without and with scaling
    */
    for (int iPass=0;iPass<2;iPass++) {
      const double * rowScale = clp->rowScale();
      const double * columnScale = clp->columnScale();
#     if 0
      if (!iPass)
        assert (!rowScale);
      else
        assert (rowScale); // only true for this example
#     endif
      /* has to be exactly correct as in OsiClpsolverInterface.cpp
         (also redo each pass as may change
      */
      CoinIndexedVector * rowArray = clp->rowArray(1);
      CoinIndexedVector * columnArray = clp->columnArray(0);
      int n;
      int * which;
      double * array;
      if (OsiUnitTest::verbosity >= 2)
        printf("B-1 A");
      for( i = 0; i < n_rows; i++){
        m.getBInvARow(i, binvA,binvA+n_cols);
        if (OsiUnitTest::verbosity >= 2)
          printf("\nrow: %d -> ",i);
        int j;
        // First columns
        n = columnArray->getNumElements();
        which = columnArray->getIndices();
        array = columnArray->denseVector();
        for(j=0; j < n; j++){
          int k=which[j];
          if (OsiUnitTest::verbosity >= 2) {
            if (!columnScale) {
              printf("(%d %g), ", k, array[k]);
            } else {
              printf("(%d %g), ", k, array[k]/columnScale[k]);
            }
          }
          // zero out
          array[k]=0.0;
        }
        // say empty
        columnArray->setNumElements(0);
        // and check (would not be in any production code)
        columnArray->checkClear();
        // now rows
        n = rowArray->getNumElements();
        which = rowArray->getIndices();
        array = rowArray->denseVector();
        for(j=0; j < n; j++){
          int k=which[j];
          if (OsiUnitTest::verbosity >= 2) {
            if (!rowScale) {
              printf("(%d %g), ", k+n_cols, array[k]);
            } else {
              printf("(%d %g), ", k+n_cols, array[k]*rowScale[k]);
            }
          }
          // zero out
          array[k]=0.0;
        }
        // say empty
        rowArray->setNumElements(0);
        // and check (would not be in any production code)
        rowArray->checkClear();
      }
      if (OsiUnitTest::verbosity >= 2) {
        printf("\n");
        printf("And by column (trickier)");
      }
      const int * pivotVariable = clp->pivotVariable();
      for( i = 0; i < n_cols+n_rows; i++){
        m.getBInvACol(i, binvA);
        if (OsiUnitTest::verbosity >= 2)
          printf("\ncolumn: %d -> ",i);
        n = rowArray->getNumElements();
        which = rowArray->getIndices();
        array = rowArray->denseVector();
        for(int j=0; j < n; j++){
          int k=which[j];
          // need to know pivot variable for +1/-1 (slack) and row/column scaling
          int pivot = pivotVariable[k];
          if (OsiUnitTest::verbosity >= 2) {
            if (pivot<n_cols) {
              // scaled coding is in just in case
              if (!columnScale) {
                printf("(%d %g), ", k, array[k]);
              } else {
                printf("(%d %g), ", k, array[k]*columnScale[pivot]);
              }
            } else {
              if (!rowScale) {
                printf("(%d %g), ", k, -array[k]);
              } else {
                printf("(%d %g), ", k, -array[k]/rowScale[pivot-n_cols]);
              }
            }
          }
          // zero out
          array[k]=0.0;
        }
        // say empty
        rowArray->setNumElements(0);
        // and check (would not be in any production code)
        rowArray->checkClear();
      }
      if (OsiUnitTest::verbosity >= 2)
        printf("\n");
      // now deal with next pass
      if (!iPass) {
        m.disableSimplexInterface();
        // see if we can get scaling for testing
        clp->scaling(1);
        m.enableFactorization();
      } else {
        // may not be needed - but cleaner
        m.disableFactorization();
      }
    }
    m.setSpecialOptions(m.specialOptions()&~512);
    free(binvA);
    // Do using newer interface
    m.enableFactorization();
    {
      CoinIndexedVector * rowArray = new CoinIndexedVector(n_rows);
      CoinIndexedVector * columnArray = new CoinIndexedVector(n_cols);
      int n;
      int * which;
      double * array;
      if (OsiUnitTest::verbosity >= 2)
        printf("B-1 A");
      for( i = 0; i < n_rows; i++){
        m.getBInvARow(i, columnArray,rowArray);
        if (OsiUnitTest::verbosity >= 2)
          printf("\nrow: %d -> ",i);
        int j;
        // First columns
        n = columnArray->getNumElements();
        which = columnArray->getIndices();
        array = columnArray->denseVector();
        for(j=0; j < n; j++){
          int k=which[j];
          if (OsiUnitTest::verbosity >= 2)
            printf("(%d %g), ", k, array[k]);
          // zero out
          array[k]=0.0;
        }
        // say empty (if I had not zeroed array[k] I would use ->clear())
        columnArray->setNumElements(0);
        // and check (would not be in any production code)
        columnArray->checkClear();
        // now rows
        n = rowArray->getNumElements();
        which = rowArray->getIndices();
        array = rowArray->denseVector();
        for(j=0; j < n; j++){
          int k=which[j];
          if (OsiUnitTest::verbosity >= 2)
            printf("(%d %g), ", k+n_cols, array[k]);
          // zero out
          array[k]=0.0;
        }
        // say empty
        rowArray->setNumElements(0);
        // and check (would not be in any production code)
        rowArray->checkClear();
      }
      if (OsiUnitTest::verbosity >= 2) {
        printf("\n");
        printf("And by column");
      }
      for( i = 0; i < n_cols+n_rows; i++){
        m.getBInvACol(i, rowArray);
        if (OsiUnitTest::verbosity >= 2)
          printf("\ncolumn: %d -> ",i);
        n = rowArray->getNumElements();
        which = rowArray->getIndices();
        array = rowArray->denseVector();
        for(int j=0; j < n; j++){
          int k=which[j];
          if (OsiUnitTest::verbosity >= 2)
            printf("(%d %g), ", k, array[k]);
          // zero out
          array[k]=0.0;
        }
        // say empty
        rowArray->setNumElements(0);
        // and check (would not be in any production code)
        rowArray->checkClear();
      }
      if (OsiUnitTest::verbosity >= 2)
        printf("\n");
      delete rowArray;
      delete columnArray;
    }
    // may not be needed - but cleaner
    m.disableFactorization();
  }

  // Check tableau stuff when simplex interface is off
  {    
    OsiClpSolverInterface m;
    /* 
       Wolsey : Page 130
       max 4x1 -  x2
       7x1 - 2x2    <= 14
       x2    <= 3
       2x1 - 2x2    <= 3
       x1 in Z+, x2 >= 0
    */
    
    double inf_ = m.getInfinity();
    int n_cols = 2;
    int n_rows = 3;
    
    double obj[2] = {-4.0, 1.0};
    double collb[2] = {0.0, 0.0};
    double colub[2] = {inf_, inf_};
    double rowlb[3] = {-inf_, -inf_, -inf_};
    double rowub[3] = {14.0, 3.0, 3.0};
    
    int rowIndices[5] =  {0,     2,    0,    1,    2};
    int colIndices[5] =  {0,     0,    1,    1,    1};
    double elements[5] = {7.0, 2.0, -2.0,  1.0, -2.0};
    CoinPackedMatrix M(true, rowIndices, colIndices, elements, 5);
    
    m.loadProblem(M, collb, colub, obj, rowlb, rowub);
    // Test with scaling
    m.setHintParam(OsiDoScale,true);
    // Tell code to keep factorization
    m.setupForRepeatedUse(3,0);
    
    m.initialSolve();
    // Repeated stuff only in resolve
    // m.resolve(); (now in both (just for (3,0))
    
    //check that the tableau matches wolsey (B-1 A)
    // slacks in second part of binvA
    double * binvA = (double*) malloc((n_cols+n_rows) * sizeof(double));
    // and getBasics
    int * pivots = new int[n_rows];
    m.getBasics(pivots);
    
    if (OsiUnitTest::verbosity >= 2)
      printf("B-1 A");
    int i;
    for( i = 0; i < n_rows; i++){
      m.getBInvARow(i, binvA,binvA+n_cols);
      if (OsiUnitTest::verbosity >= 2) {
        printf("\nrow: %d (pivot %d) -> ",i,pivots[i]);
        for(int j=0; j < n_cols+n_rows; j++)
          printf("%g, ", binvA[j]);
      }
    }
    if (OsiUnitTest::verbosity >= 2) {
      printf("\n");
      printf("And by column");
    }
    for( i = 0; i < n_cols+n_rows; i++){
      m.getBInvACol(i, binvA);
      if (OsiUnitTest::verbosity >= 2) {
        printf("\ncolumn: %d -> ",i);
        for(int j=0; j < n_rows; j++)
          printf("%g, ", binvA[j]);
      }
    }
    if (OsiUnitTest::verbosity >= 2)
      printf("\n");
    free(binvA);
    delete [] pivots;
  }

  // Do common solverInterface testing 
  {
    OsiClpSolverInterface m;
    OsiSolverInterfaceCommonUnitTest(&m, mpsDir, netlibDir);
  }
}
