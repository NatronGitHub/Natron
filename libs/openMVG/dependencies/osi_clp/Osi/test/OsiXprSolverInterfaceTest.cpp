// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This file is licensed under the terms of Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "OsiConfig.h"

//#include <cassert>

#include "OsiUnitTests.hpp"
#include "OsiXprSolverInterface.hpp"
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"

// Added so build windows build with dsp files works,
// when not building with xpress.
#ifdef COIN_HAS_XPR
#define __ANSIC_
#include <xprs.h>
#undef  __ANSIC_

//-----------------------------------------------------------------------
// Test XPRESS-MP solution methods.
void
OsiXprSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir)
{
  unsigned int numInstancesStart = OsiXprSolverInterface::getNumInstances();

  // Test default constructor
  {
    OsiXprSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(m.xprProbname_ == "",   {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.collower_    == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colupper_    == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.vartype_     == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsense_    == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rhs_         == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowrange_    == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowlower_    == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowupper_    == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowprice_    == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colsol_      == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsol_      == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_ == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByCol_ == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.ivarind_     == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.ivartype_    == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getNumCols() ==    0, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getApplicationData() == NULL, {}, "xpress", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(OsiXprSolverInterface::getNumInstances() == numInstancesStart+1, {}, "xpress", "number of instances during first test");
    int i=2346;
    m.setApplicationData(&i);
    OSIUNITTEST_ASSERT_ERROR(*((int *)(m.getApplicationData())) == i, {}, "xpress", "default constructor");
  }
  OSIUNITTEST_ASSERT_ERROR(OsiXprSolverInterface::getNumInstances() == numInstancesStart, {}, "xpress", "number of instances after first test");

  {
    CoinRelFltEq eq;
    OsiXprSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(OsiXprSolverInterface::getNumInstances() == numInstancesStart+1, {}, "xpress", "number of instances");
    std::string fn = mpsDir+"exmip1";
    m.readMps(fn.c_str());
    // This assert fails on windows because fn is mixed case and xprProbname_is uppercase.
    //OSIUNITTEST_ASSERT_ERROR( m.xprProbname_ == fn, {}, "xpress", "exmip1 problem name" );

    // Test copy constructor and assignment operator
    {
      OsiXprSolverInterface lhs;
      {      
        OsiXprSolverInterface im(m);

        OsiXprSolverInterface imC1(im);
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumCols() == im.getNumCols(),  {}, "xpress", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumRows() == im.getNumRows(),  {}, "xpress", "copy constructor");
        
        OsiXprSolverInterface imC2(im);
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumCols() == im.getNumCols(),  {}, "xpress", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumRows() == im.getNumRows(),  {}, "xpress", "copy constructor");
        
        lhs = imC2;
      }

      // Test that lhs has correct values even though rhs has gone out of scope
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumCols() == m.getNumCols(),  {}, "xpress", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumRows() == m.getNumRows(),  {}, "xpress", "copy constructor");
    }

    // Test clone
    {
      OsiXprSolverInterface xpressSi(m);
      OsiSolverInterface * siPtr = &xpressSi;
      OsiSolverInterface * siClone = siPtr->clone();
      OsiXprSolverInterface * xpressClone = dynamic_cast<OsiXprSolverInterface*>(siClone);
      OSIUNITTEST_ASSERT_ERROR(xpressClone != NULL, {}, "xpress", "clone");
      OSIUNITTEST_ASSERT_ERROR(xpressClone->getNumRows() == xpressSi.getNumRows(), {}, "xpress", "clone");
      OSIUNITTEST_ASSERT_ERROR(xpressClone->getNumCols() == m.getNumCols(), {}, "xpress", "clone");
      
      delete siClone;
    }

    // Test infinity
    {
      OsiXprSolverInterface si;
      OSIUNITTEST_ASSERT_ERROR(si.getInfinity() == XPRS_PLUSINFINITY, {}, "xpress", "value for infinity");
    }

    {
      OsiXprSolverInterface xpressSi(m);
      int nc = xpressSi.getNumCols();
      int nr = xpressSi.getNumRows();
      const double * cl = xpressSi.getColLower();
      const double * cu = xpressSi.getColUpper();
      const double * rl = xpressSi.getRowLower();
      const double * ru = xpressSi.getRowUpper();

      OSIUNITTEST_ASSERT_ERROR(nc == 8, return, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(nr == 5, return, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[0],2.5), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[1],0.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[1],4.1), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[2],1.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[0],2.5), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[4],3.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[1],2.1), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[4],15.), {}, "xpress", "read and copy exmip1");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(cl[3],1.2345), {}, "xpress", "set col lower");
      xpressSi.setColLower( 3, 1.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cl[3],1.2345), {}, "xpress", "set col lower");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(cu[4],10.2345), {}, "xpress", "set col upper");
      xpressSi.setColUpper( 4, 10.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cu[4],10.2345), {}, "xpress", "set col upper");

      OSIUNITTEST_ASSERT_ERROR(eq(xpressSi.getObjCoefficients()[0], 1.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(xpressSi.getObjCoefficients()[1], 0.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(xpressSi.getObjCoefficients()[2], 0.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(xpressSi.getObjCoefficients()[3], 0.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(xpressSi.getObjCoefficients()[4], 2.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(xpressSi.getObjCoefficients()[5], 0.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(xpressSi.getObjCoefficients()[6], 0.0), {}, "xpress", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(xpressSi.getObjCoefficients()[7],-1.0), {}, "xpress", "read and copy exmip1");
    }
    
    // Test getMatrixByRow method
    { 
      const OsiXprSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();
      
      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  5, return, "xpress", "getMatrixByRow: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "xpress", "getMatrixByRow: num elements");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[0],   3.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[1],   1.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[2],  -2.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[3],  -1.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[4],  -1.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[5],   2.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[6],   1.1), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[7],   1.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[8],   1.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[9],   2.8), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], -1.2), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11],  5.6), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12],  1.0), {}, "xpress", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13],  1.9), {}, "xpress", "getMatrixByRow: elements");

      const int * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "xpress", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "xpress", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "xpress", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "xpress", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "xpress", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "xpress", "getMatrixByRow: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "xpress", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "xpress", "getMatrixByRow: indices");
    }

    //--------------
    // Test rowsense, rhs, rowrange, getMatrixByRow
    {
      OsiXprSolverInterface lhs;
      {
        OsiXprSolverInterface siC1(m);
        OSIUNITTEST_ASSERT_WARNING(siC1.rowrange_ == NULL, {}, "xpress", "row range");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsense_ == NULL, {}, "xpress", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1.rhs_ == NULL, {}, "xpress", "right hand side");
        assert( m.rowrange_==NULL );
        assert( m.rowsense_==NULL );
        assert( m.rhs_==NULL );

        const char   * siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "xpress", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "xpress", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "xpress", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "xpress", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "xpress", "row sense");

        const double * siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "xpress", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "xpress", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "xpress", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "xpress", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "xpress", "right hand side");

        const double * siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "xpress", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "xpress", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "xpress", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "xpress", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "xpress", "row range");

        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        OSIUNITTEST_ASSERT_ERROR(siC1mbr != NULL, {}, "xpress", "matrix by row");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getMajorDim()    ==  5, return, "xpress", "matrix by row: major dim");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getNumElements() == 14, return, "xpress", "matrix by row: num elements");
        
        const double * ev = siC1mbr->getElements();
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "xpress", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "xpress", "matrix by row: elements");

        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "xpress", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "xpress", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "xpress", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "xpress", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "xpress", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "xpress", "matrix by row: vector starts");

        const int * ei = siC1mbr->getIndices();
        OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "xpress", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "xpress", "matrix by row: indices");

        OSIUNITTEST_ASSERT_WARNING(siC1rs  == siC1.getRowSense(), {}, "xpress", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1rhs == siC1.getRightHandSide(), {}, "xpress", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1rr  == siC1.getRowRange(), {}, "xpress", "row range");

        // Change XPRESS Model by adding free row
        OsiRowCut rc;
        rc.setLb(-COIN_DBL_MAX);
        rc.setUb( COIN_DBL_MAX);
        OsiCuts cuts;
        cuts.insert(rc);
        siC1.applyCuts(cuts);

        // Since model was changed, test that cached data is now freed.
        OSIUNITTEST_ASSERT_ERROR(siC1.rowrange_ == NULL, {}, "xpress", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsense_ == NULL, {}, "xpress", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rhs_ == NULL, {}, "xpress", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByRow_ == NULL, {}, "xpress", "free cached data after adding row");

        siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "xpress", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "xpress", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "xpress", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "xpress", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "xpress", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[5] == 'N', {}, "xpress", "row sense after adding row");

        siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "xpress", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "xpress", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "xpress", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "xpress", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "xpress", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[5],0.0), {}, "xpress", "right hand side after adding row");

        siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "xpress", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "xpress", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "xpress", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "xpress", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "xpress", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[5],0.0), {}, "xpress", "row range after adding row");

        lhs = siC1;
      }
      // Test that lhs has correct values even though siC1 has gone out of scope    
      OSIUNITTEST_ASSERT_ERROR(lhs.rowrange_ == NULL, {}, "xpress", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsense_ == NULL, {}, "xpress", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rhs_ == NULL, {}, "xpress", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByRow_ == NULL, {}, "xpress", "freed origin after assignment");

      const char * lhsrs  = lhs.getRowSense();
      OSIUNITTEST_ASSERT_ERROR(lhsrs[0] == 'G', {}, "xpress", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[1] == 'L', {}, "xpress", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[2] == 'E', {}, "xpress", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[3] == 'R', {}, "xpress", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[4] == 'R', {}, "xpress", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[5] == 'N', {}, "xpress", "row sense after assignment");
      
      const double * lhsrhs = lhs.getRightHandSide();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[0],2.5), {}, "xpress", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[1],2.1), {}, "xpress", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[2],4.0), {}, "xpress", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[3],5.0), {}, "xpress", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[4],15.), {}, "xpress", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[5],0.0), {}, "xpress", "right hand side after assignment");
      
      const double *lhsrr = lhs.getRowRange();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[0],0.0), {}, "xpress", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[1],0.0), {}, "xpress", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[2],0.0), {}, "xpress", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[3],5.0-1.8), {}, "xpress", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[4],15.0-3.0), {}, "xpress", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[5],0.0), {}, "xpress", "row range after assignment");
      
      const CoinPackedMatrix * lhsmbr = lhs.getMatrixByRow();
      OSIUNITTEST_ASSERT_ERROR(lhsmbr != NULL, {}, "xpress", "matrix by row after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getMajorDim()    ==  6, return, "xpress", "matrix by row after assignment: major dim");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getNumElements() == 14, return, "xpress", "matrix by row after assignment: num elements");

      const double * ev = lhsmbr->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "xpress", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "xpress", "matrix by row after assignment: elements");

      const CoinBigIndex * mi = lhsmbr->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "xpress", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "xpress", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "xpress", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "xpress", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "xpress", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "xpress", "matrix by row after assignment: vector starts");

      const int * ei = lhsmbr->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "xpress", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "xpress", "matrix by row after assignment: indices");
    }
    OSIUNITTEST_ASSERT_ERROR(OsiXprSolverInterface::getNumInstances() == numInstancesStart+1, {}, "xpress", "number of instances");
  }
  OSIUNITTEST_ASSERT_ERROR(OsiXprSolverInterface::getNumInstances() == numInstancesStart, {}, "xpress", "number of instances at finish");

  // Do common solverInterface testing by calling the
  // base class testing method.
  {
    OsiXprSolverInterface m;
    OsiSolverInterfaceCommonUnitTest(&m, mpsDir, netlibDir);
  }
}
#endif
