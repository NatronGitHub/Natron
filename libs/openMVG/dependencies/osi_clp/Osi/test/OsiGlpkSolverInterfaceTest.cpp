// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// Copyright (C) 2004  University of Pittsburgh
//   University of Pittsburgh coding done by Brady Hunsaker
// This file is licensed under the terms of Eclipse Public License (EPL).

// OsiGlpkSolverInterfaceTest.cpp adapted from OsiClpSolverInterfaceTest.cpp 
//  on 2004/10/16
// Check for ??? to see tests that aren't working correctly.
// Also note that OsiPresolve doesn't appear to work with OsiGlpk.  This
// needs to be examined.

#include "CoinPragma.hpp"
#include "OsiConfig.h"

//#include <cassert>
//#include <cstdlib>
//#include <cstdio>
//#include <iostream>

#include "OsiUnitTests.hpp"
#include "OsiGlpkSolverInterface.hpp"
#include "OsiRowCut.hpp"
#include "OsiCuts.hpp"
#include "CoinFloatEqual.hpp"

// Added so windows build with dsp files works,
// when not building with glpk.
#ifdef COIN_HAS_GLPK

void OsiGlpkSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir)
{
  // Test default constructor
  {
    OsiGlpkSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(m.obj_         == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.collower_    == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colupper_    == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.ctype_       == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsense_    == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rhs_         == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowrange_    == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowlower_    == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowupper_    == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colsol_      == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsol_      == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_ == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByCol_ == NULL, {}, "glpk", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getApplicationData() == NULL, {}, "glpk", "default constructor");
    int i=2346;
    m.setApplicationData(&i);
    OSIUNITTEST_ASSERT_ERROR(*((int *)(m.getApplicationData())) == i, {}, "glpk", "default constructor");
  }
  
  
  {
    CoinRelFltEq eq;
    OsiGlpkSolverInterface m;
    std::string fn = mpsDir+"exmip1";
    m.readMps(fn.c_str(),"mps");
    
    {
      OsiGlpkSolverInterface im;    
      
      OSIUNITTEST_ASSERT_ERROR(im.getNumCols()  == 0,    {}, "glpk", "default constructor");
      OSIUNITTEST_ASSERT_ERROR(im.getModelPtr() != NULL, {}, "glpk", "default constructor");
      
      // Test reset
      im.reset();
      OSIUNITTEST_ASSERT_ERROR(m.obj_         == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.collower_    == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.colupper_    == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.ctype_       == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.rowsense_    == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.rhs_         == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.rowrange_    == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.rowlower_    == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.rowupper_    == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.colsol_      == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.rowsol_      == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_ == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.matrixByCol_ == NULL, {}, "glpk", "reset");
      OSIUNITTEST_ASSERT_ERROR(m.getApplicationData() == NULL, {}, "glpk", "reset");
    }
    
    // Test copy constructor and assignment operator
    {
      OsiGlpkSolverInterface lhs;
      {      
        OsiGlpkSolverInterface im(m);        
	
        OsiGlpkSolverInterface imC1(im);
        OSIUNITTEST_ASSERT_ERROR(imC1.getModelPtr() != im.getModelPtr(), {}, "glpk", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumCols()  == im.getNumCols(),  {}, "glpk", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumRows()  == im.getNumRows(),  {}, "glpk", "copy constructor");
        
        OsiGlpkSolverInterface imC2(im);
        OSIUNITTEST_ASSERT_ERROR(imC2.getModelPtr() != im.getModelPtr(), {}, "glpk", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumCols()  == im.getNumCols(),  {}, "glpk", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumRows()  == im.getNumRows(),  {}, "glpk", "copy constructor");
	
        OSIUNITTEST_ASSERT_ERROR(imC1.getModelPtr() != imC2.getModelPtr(), {}, "glpk", "copy constructor");
        
        lhs = imC2;
      }

      // Test that lhs has correct values even though rhs has gone out of scope
      OSIUNITTEST_ASSERT_ERROR(lhs.getModelPtr() != m.getModelPtr(), {}, "glpk", "assignment operator");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumCols()  == m.getNumCols(),  {}, "glpk", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumRows()  == m.getNumRows(),  {}, "glpk", "copy constructor");
    }

    // Test clone
    {
      OsiGlpkSolverInterface glpkSi(m);
      OsiSolverInterface * siPtr = &glpkSi;
      OsiSolverInterface * siClone = siPtr->clone();
      OsiGlpkSolverInterface * glpkClone = dynamic_cast<OsiGlpkSolverInterface*>(siClone);
      OSIUNITTEST_ASSERT_ERROR(glpkClone != NULL, {}, "glpk", "clone");
      OSIUNITTEST_ASSERT_ERROR(glpkClone->getModelPtr() != glpkSi.getModelPtr(), {}, "glpk", "clone");
      OSIUNITTEST_ASSERT_ERROR(glpkClone->getNumRows() == glpkSi.getNumRows(), {}, "glpk", "clone");
      OSIUNITTEST_ASSERT_ERROR(glpkClone->getNumCols() == glpkSi.getNumCols(), {}, "glpk", "clone");
      
      delete siClone;
    }
  
    // test infinity
    {
      OsiGlpkSolverInterface si;
      OSIUNITTEST_ASSERT_ERROR(si.getInfinity() == COIN_DBL_MAX, {}, "glpk", "infinity");
    }
    
#if 0
    // ??? These index error 'throw's aren't in OsiGlpk 

    // Test some catches
    {
      OsiGlpkSolverInterface solver;
      try {
      	solver.setObjCoeff(0,0.0);
      }
      catch (CoinError e) {
      	std::cout<<"Correct throw"<<std::endl;
      }
      std::string fn = mpsDir+"exmip1";
      solver.readMps(fn.c_str(),"mps");
      try {
      	solver.setObjCoeff(0,0.0);
      }
      catch (CoinError e) {
      	std::cout<<"** Incorrect throw"<<std::endl;
      	abort();
      }
      try {
      	int index[]={0,20};
      	double value[]={0.0,0.0,0.0,0.0};
      	solver.setColSetBounds(index,index+2,value);
      }
      catch (CoinError e) {
      	std::cout<<"Correct throw"<<std::endl;
      }
    }
#endif
    
    {    
      OsiGlpkSolverInterface glpkSi(m);
      int nc = glpkSi.getNumCols();
      int nr = glpkSi.getNumRows();
      const double * cl = glpkSi.getColLower();
      const double * cu = glpkSi.getColUpper();
      const double * rl = glpkSi.getRowLower();
      const double * ru = glpkSi.getRowUpper();
      OSIUNITTEST_ASSERT_ERROR(nc == 8, return, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(nr == 5, return, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[0],2.5), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[1],0.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[1],4.1), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[2],1.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[0],2.5), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[4],3.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[1],2.1), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[4],15.), {}, "glpk", "read and copy exmip1");
      
      const double * cs = glpkSi.getColSolution();
      OSIUNITTEST_ASSERT_ERROR(eq(cs[0],2.5), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cs[7],0.0), {}, "glpk", "read and copy exmip1");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(cl[3],1.2345), {}, "glpk", "set col lower");
      glpkSi.setColLower( 3, 1.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cl[3],1.2345), {}, "glpk", "set col lower");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(glpkSi.getColUpper()[4],10.2345), {}, "glpk", "set col upper");
      glpkSi.setColUpper( 4, 10.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(glpkSi.getColUpper()[4],10.2345), {}, "glpk", "set col upper");

      double objValue = glpkSi.getObjValue();
      OSIUNITTEST_ASSERT_ERROR(eq(objValue,3.5), {}, "glpk", "getObjValue() before solve");

      OSIUNITTEST_ASSERT_ERROR(eq(glpkSi.getObjCoefficients()[0], 1.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(glpkSi.getObjCoefficients()[1], 0.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(glpkSi.getObjCoefficients()[2], 0.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(glpkSi.getObjCoefficients()[3], 0.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(glpkSi.getObjCoefficients()[4], 2.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(glpkSi.getObjCoefficients()[5], 0.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(glpkSi.getObjCoefficients()[6], 0.0), {}, "glpk", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(glpkSi.getObjCoefficients()[7],-1.0), {}, "glpk", "read and copy exmip1");
    }
    
    // Test matrixByRow method
    { 
      const OsiGlpkSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();

      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  5, return, "glpk", "getMatrixByRow: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "glpk", "getMatrixByRow: num elements");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();

      // GLPK returns each row in reverse order. This is consistent with 
      // the sparse matrix format but is not what most solvers do.  That's
      // why this section is different.
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4], 3.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3], 1.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1],-1.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0],-1.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 2.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 1.1), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], 2.8), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9],-1.2), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 5.6), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "glpk", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 1.9), {}, "glpk", "getMatrixByRow: elements");
      
      const CoinBigIndex * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "glpk", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "glpk", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "glpk", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "glpk", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "glpk", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "glpk", "getMatrixByRow: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 0, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 1, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 4, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 7, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 1, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 2, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 2, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 5, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 3, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 6, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 0, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "glpk", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 7, {}, "glpk", "getMatrixByRow: indices");
    }

    // Test adding several cuts
    {
      OsiGlpkSolverInterface fim;
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
      OSIUNITTEST_ASSERT_ERROR(eq(cs[2], 1.0), {}, "glpk", "add cuts");
      OSIUNITTEST_ASSERT_ERROR(eq(cs[3], 1.0), {}, "glpk", "add cuts");
#if 0  // ??? Not working for some reason.
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
      OSIUNITTEST_ASSERT_WARNING(fim.isAbandoned(), {}, "glpk", "add cuts");
#endif
      delete[]el;
      delete[]inx;
    }

    // Test matrixByCol method
    {
      const OsiGlpkSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByCol();
      
      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  8, return, "glpk", "getMatrixByCol: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getMinorDim()    ==  5, return, "glpk", "getMatrixByCol: minor dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "glpk", "getMatrixByCol: number of elements");
      OSIUNITTEST_ASSERT_ERROR(smP->getSizeVectorStarts() == 9, return, "glpk", "getMatrixByCol: vector starts size");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      // Unlike row-ordered matrices, GLPK does column-ordered the "normal" way
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 5.6), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2], 1.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3], 2.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4], 1.1), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 1.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6],-2.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 2.8), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8],-1.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 1.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], 1.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11],-1.2), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12],-1.0), {}, "glpk", "getMatrixByCol: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "glpk", "getMatrixByCol: elements");
      
      const CoinBigIndex * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "glpk", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  2, {}, "glpk", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  4, {}, "glpk", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  6, {}, "glpk", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] ==  8, {}, "glpk", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 10, {}, "glpk", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[6] == 11, {}, "glpk", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[7] == 12, {}, "glpk", "getMatrixByCol: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[8] == 14, {}, "glpk", "getMatrixByCol: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 4, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 0, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 1, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 1, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 2, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 0, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 3, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 0, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 4, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 2, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 3, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 0, {}, "glpk", "getMatrixByCol: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 4, {}, "glpk", "getMatrixByCol: indices");
    }
    //--------------
    // Test rowsense, rhs, rowrange, matrixByRow
    {
      OsiGlpkSolverInterface lhs;
      {
#if 0  // FIXME ??? these won't work because the copy constructor changes the values in m
        OSIUNITTEST_ASSERT_ERROR(m.rowrange_ == NULL, {}, "glpk", "???");
        OSIUNITTEST_ASSERT_ERROR(m.rowsense_ == NULL, {}, "glpk", "???");
        OSIUNITTEST_ASSERT_ERROR(m.rhs_ == NULL, {}, "glpk", "???");
        OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_ == NULL, {}, "glpk", "???");
#endif
        
        OsiGlpkSolverInterface siC1(m);
        OSIUNITTEST_ASSERT_WARNING(siC1.rowrange_ == NULL, {}, "glpk", "row range");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsense_ == NULL, {}, "glpk", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1.rhs_ == NULL, {}, "glpk", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1.matrixByRow_ == NULL, {}, "glpk", "matrix by row");

        const char   * siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "glpk", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "glpk", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "glpk", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "glpk", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "glpk", "row sense");
        
        const double * siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "glpk", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "glpk", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "glpk", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "glpk", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "glpk", "right hand side");
        
        const double * siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "glpk", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "glpk", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "glpk", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "glpk", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "glpk", "row range");
        
        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        OSIUNITTEST_ASSERT_ERROR(siC1mbr != NULL, {}, "glpk", "matrix by row");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getMajorDim()    ==  5, return, "glpk", "matrix by row: major dim");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getNumElements() == 14, return, "glpk", "matrix by row: num elements");
        
        const double * ev = siC1mbr->getElements();
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4], 3.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3], 1.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1],-1.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0],-1.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 2.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 1.1), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[10], 2.8), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9],-1.2), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 5.6), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "glpk", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 1.9), {}, "glpk", "matrix by row: elements");
        
        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "glpk", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "glpk", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "glpk", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "glpk", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "glpk", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "glpk", "matrix by row: vector starts");
        
        const int * ei = siC1mbr->getIndices();
        OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 0, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 1, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 4, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 7, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 1, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 2, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 2, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 5, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[10] == 3, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 6, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[13] == 0, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "glpk", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[11] == 7, {}, "glpk", "matrix by row: indices");

        OSIUNITTEST_ASSERT_WARNING(siC1rs  == siC1.getRowSense(), {}, "glpk", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1rhs == siC1.getRightHandSide(), {}, "glpk", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1rr  == siC1.getRowRange(), {}, "glpk", "row range");

        // Change glpk Model by adding free row
        OsiRowCut rc;
        rc.setLb(-COIN_DBL_MAX);
        rc.setUb( COIN_DBL_MAX);
        OsiCuts cuts;
        cuts.insert(rc);
        siC1.applyCuts(cuts);
             
        // Since model was changed, test that cached data is now freed.
        OSIUNITTEST_ASSERT_ERROR(siC1.rowrange_ == NULL, {}, "glpk", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsense_ == NULL, {}, "glpk", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rhs_ == NULL, {}, "glpk", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByRow_ == NULL, {}, "glpk", "free cached data after adding row");
        
        siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "glpk", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "glpk", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "glpk", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "glpk", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "glpk", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[5] == 'N', {}, "glpk", "row sense after adding row");

        siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "glpk", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "glpk", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "glpk", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "glpk", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "glpk", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[5],0.0), {}, "glpk", "right hand side after adding row");

        siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "glpk", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "glpk", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "glpk", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "glpk", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "glpk", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[5],0.0), {}, "glpk", "row range after adding row");
    
        lhs = siC1;
      }
      // Test that lhs has correct values even though siC1 has gone out of scope    
      OSIUNITTEST_ASSERT_ERROR(lhs.rowrange_ == NULL, {}, "glpk", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsense_ == NULL, {}, "glpk", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rhs_ == NULL, {}, "glpk", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByRow_ == NULL, {}, "glpk", "freed origin after assignment");
      
      const char * lhsrs  = lhs.getRowSense();
      OSIUNITTEST_ASSERT_ERROR(lhsrs[0] == 'G', {}, "glpk", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[1] == 'L', {}, "glpk", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[2] == 'E', {}, "glpk", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[3] == 'R', {}, "glpk", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[4] == 'R', {}, "glpk", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[5] == 'N', {}, "glpk", "row sense after assignment");
      
      const double * lhsrhs = lhs.getRightHandSide();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[0],2.5), {}, "glpk", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[1],2.1), {}, "glpk", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[2],4.0), {}, "glpk", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[3],5.0), {}, "glpk", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[4],15.), {}, "glpk", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[5],0.0), {}, "glpk", "right hand side after assignment");
      
      const double *lhsrr = lhs.getRowRange();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[0],0.0), {}, "glpk", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[1],0.0), {}, "glpk", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[2],0.0), {}, "glpk", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[3],5.0-1.8), {}, "glpk", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[4],15.0-3.0), {}, "glpk", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[5],0.0), {}, "glpk", "row range after assignment");
      
      const CoinPackedMatrix * lhsmbr = lhs.getMatrixByRow();
      OSIUNITTEST_ASSERT_ERROR(lhsmbr != NULL, return, "glpk", "matrix by row after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getMajorDim()    ==  6, return, "glpk", "matrix by row after assignment: major dim");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getNumElements() == 14, return, "glpk", "matrix by row after assignment: num elements");

      const double * ev = lhsmbr->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4], 3.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3], 1.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1],-1.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0],-1.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 2.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 1.1), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], 2.8), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9],-1.2), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 5.6), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "glpk", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 1.9), {}, "glpk", "matrix by row after assignment: elements");
      
      const CoinBigIndex * mi = lhsmbr->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "glpk", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "glpk", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "glpk", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "glpk", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "glpk", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "glpk", "matrix by row after assignment: vector starts");
      
      const int * ei = lhsmbr->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 0, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 1, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 4, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 7, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 1, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 2, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 2, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 5, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 3, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 6, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 0, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "glpk", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 7, {}, "glpk", "matrix by row after assignment: indices");
    }
    
  }

  // Test add/delete columns
  {    
    OsiGlpkSolverInterface m;
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
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,2520.57), {}, "glpk", "add/delete columns: first optimal value");
    // Try deleting first column that's nonbasic at lower bound (0).
    int * d = new int[1];
    CoinWarmStartBasis *cwsb = dynamic_cast<CoinWarmStartBasis *>(m.getWarmStart()) ;
    OSIUNITTEST_ASSERT_ERROR(cwsb != NULL, {}, "glpk", "add/delete columns: have warm start basis");
    CoinWarmStartBasis::Status stati ;
    int iCol ;
    for (iCol = 0 ;  iCol < cwsb->getNumStructural() ; iCol++)
    { stati = cwsb->getStructStatus(iCol) ;
      if (stati == CoinWarmStartBasis::atLowerBound) break ; }
    d[0]=iCol;
    m.deleteCols(1,d);
    delete [] d;
    delete cwsb;
    d=NULL;
    m.resolve();
    objValue = m.getObjValue();
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,2520.57), {}, "glpk", "add/delete columns: optimal value after deleting nonbasic column");
    // Try deleting column we added. If basic, go to initialSolve as deleting
    // basic variable trashes basis required for warm start.
    iCol = m.getNumCols()-1;
    cwsb = dynamic_cast<CoinWarmStartBasis *>(m.getWarmStart()) ;
    stati =  cwsb->getStructStatus(iCol) ;
    delete cwsb;
    m.deleteCols(1,&iCol);
    if (stati == CoinWarmStartBasis::basic)
    { m.initialSolve() ; }
    else
    { m.resolve(); }
    objValue = m.getObjValue();
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,2520.57), {}, "glpk", "add/delete columns: optimal value after deleting added column");
  }

#if 0
  // ??? Simplex routines not adapted to OsiGlpk yet

  // Solve an lp by hand
  {    
    OsiGlpkSolverInterface m;
    std::string fn = mpsDir+"p0033";
    m.readMps(fn.c_str(),"mps");
    m.setObjSense(-1.0);
    m.getModelPtr()->messageHandler()->setLogLevel(4);
    m.initialSolve();
    m.getModelPtr()->factorization()->maximumPivots(5);
    m.setObjSense(1.0);
    // enable special mode
    m.enableSimplexInterface(true);
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
      OSIUNITTEST_ASSERT_ERROR(m.primalPivotResult(colIn,direction,colOut,outStatus,theta,NULL) == 0, {}, "glpk", "simplex routines");
      printf("out %d, direction %d theta %g\n", colOut,outStatus,theta);
      numberIterations++;
    }
    delete [] fakeCost;
    delete [] duals;
    delete [] djs;
    // exit special mode
    m.disableSimplexInterface();
    m.getModelPtr()->messageHandler()->setLogLevel(4);
    m.resolve();
    OSIUNITTEST_ASSERT_ERROR(m.getIterationCount() == 0, {}, "glpk", "simplex routines");
    m.setObjSense(-1.0);
    m.initialSolve();
  }
  // Solve an lp when interface is on
  {    
    OsiGlpkSolverInterface m;
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
    OsiGlpkSolverInterface m;
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
    
    printf("B-1 A");
    for(int i = 0; i < n_rows; i++){
      m.getBInvARow(i, binvA,binvA+n_cols);
      printf("\nrow: %d -> ",i);
      for(int j=0; j < n_cols+n_rows; j++){
      	printf("%g, ", binvA[j]);
      }
    }
    printf("\n");
    m.disableSimplexInterface();
    free(binvA);
  }
#endif 

/*
  Read in exmip1 and solve it with verbose output setting.
*/
  { OsiGlpkSolverInterface osi ;
    std::cout << "Boosting verbosity.\n" ;
    osi.setHintParam(OsiDoReducePrint,false,OsiForceDo) ;

    std::string exmpsfile = mpsDir+"exmip1" ;
    std::string probname ;
    std::cout << "Reading mps file \"" << exmpsfile << "\"\n" ;
    osi.readMps(exmpsfile.c_str(), "mps") ;
    OSIUNITTEST_ASSERT_ERROR(osi.getStrParam(OsiProbName,probname), {}, "glpk", "get problem name");
    std::cout << "Solving " << probname << " ... \n" ;
    osi.initialSolve() ;
    double val = osi.getObjValue() ;
    std::cout << "And the answer is " << val << ".\n" ;
    OSIUNITTEST_ASSERT_ERROR(fabs(val - 3.23) < 0.01, {}, "glpk", "solve exmip1");
  }

  // Do common solverInterface testing
  {
    OsiGlpkSolverInterface m;
    OsiSolverInterfaceCommonUnitTest(&m, mpsDir,netlibDir);
  }

}
#endif
