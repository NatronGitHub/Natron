//-----------------------------------------------------------------------------
// name:     OSI Interface for SOPLEX
// author:   Tobias Pfender
//           Konrad-Zuse-Zentrum Berlin (Germany)
// license:  this file may be freely distributed under the terms of EPL
// date:     01/17/2002
//-----------------------------------------------------------------------------
// Copyright (C) 2002, Tobias Pfender, International Business Machines
// Corporation and others.  All Rights Reserved.
//
// Last edit: $Id$

#include "CoinPragma.hpp"
#include "OsiConfig.h"

// Added so windows build with dsp files works,
// when not building with soplex.
#ifdef COIN_HAS_SOPLEX

#include "OsiUnitTests.hpp"
#include "OsiRowCut.hpp"
#include "OsiCuts.hpp"
#include "CoinFloatEqual.hpp"
#include "CoinPackedMatrix.hpp"

//#include <iostream>
#ifndef SOPLEX_LEGACY
#define SOPLEX_LEGACY
#endif

// to check for value of infinity
#include "soplex.h"

// it's important to include this header after soplex.h
#include "OsiSpxSolverInterface.hpp"

void OsiSpxSolverInterfaceUnitTest( const std::string & mpsDir, const std::string & netlibDir )
{
  // Test default constructor
  {
    OsiSpxSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(m.soplex_      != NULL, {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getNumCols() == 0,    {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsense_    == NULL, {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rhs_         == NULL, {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowrange_    == NULL, {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colsol_      == NULL, {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsol_      == NULL, {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_ == NULL, {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByCol_ == NULL, {}, "SoPlex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getApplicationData() == NULL, {}, "SoPlex", "default constructor");
    int i=2346;
    m.setApplicationData(&i);
    OSIUNITTEST_ASSERT_ERROR(*((int *)(m.getApplicationData())) == i, {}, "SoPlex", "set application data");
  }

  {    
    CoinRelFltEq eq;
    OsiSpxSolverInterface m;
    std::string fn = mpsDir+"exmip1";
    m.readMps(fn.c_str(),"mps");
//    int ad = 13579;
//    m.setApplicationData(&ad);
//    OSIUNITTEST_ASSERT_ERROR(*((int *)(m.getApplicationData())) == ad, {}, "SoPlex", "set application data");

    {
      const CoinPackedMatrix * colCopy = m.getMatrixByCol();
      OSIUNITTEST_ASSERT_ERROR(colCopy->getNumCols()  == 8, {}, "SoPlex", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getMajorDim() == 8, {}, "SoPlex", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getNumRows()  == 5, {}, "SoPlex", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getMinorDim() == 5, {}, "SoPlex", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getVectorLengths()[7] == 2, {}, "SoPlex", "exmip1 matrix");
      CoinPackedMatrix revColCopy;
      revColCopy.reverseOrderedCopyOf(*colCopy);
      CoinPackedMatrix rev2ColCopy;      
      rev2ColCopy.reverseOrderedCopyOf(revColCopy);
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getNumCols()  == 8, {}, "SoPlex", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getMajorDim() == 8, {}, "SoPlex", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getNumRows()  == 5, {}, "SoPlex", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getMinorDim() == 5, {}, "SoPlex", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getVectorLengths()[7] == 2, {}, "SoPlex", "twice reverse matrix copy");
    }

    // Test copy constructor and assignment operator
    {
      OsiSpxSolverInterface lhs;
      {      
        OsiSpxSolverInterface im(m);   
        OsiSpxSolverInterface imC1(im);
        OsiSpxSolverInterface imC2(im);
        
        lhs = imC2;
      }
      // Test that lhs has correct values even though rhs has gone out of scope
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumCols() == m.getNumCols(), {}, "SoPlex", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumRows() == m.getNumRows(), {}, "SoPlex", "copy constructor");
    }
    
    // Test clone
    {
      OsiSpxSolverInterface soplexSi(m);
      OsiSolverInterface * siPtr = &soplexSi;
      OsiSolverInterface * siClone = siPtr->clone();
      OsiSpxSolverInterface * soplexClone = dynamic_cast<OsiSpxSolverInterface*>(siClone);
      OSIUNITTEST_ASSERT_ERROR(soplexClone != NULL, {}, "SoPlex", "clone");
      OSIUNITTEST_ASSERT_ERROR(soplexClone->getNumRows() == soplexSi.getNumRows(), {}, "SoPlex", "clone");
      OSIUNITTEST_ASSERT_ERROR(soplexClone->getNumCols() == m.getNumCols(), {}, "SoPlex", "clone");
      
      delete siClone;
    }
   
    // test infinity
    {
      OsiSpxSolverInterface si;
      OSIUNITTEST_ASSERT_ERROR(si.getInfinity() == soplex::infinity, {}, "SoPlex", "value for infinity");
    }     

    {    
      OsiSpxSolverInterface soplexSi(m);
      int nc = soplexSi.getNumCols();
      int nr = soplexSi.getNumRows();
      const double * cl = soplexSi.getColLower();
      const double * cu = soplexSi.getColUpper();
      const double * rl = soplexSi.getRowLower();
      const double * ru = soplexSi.getRowUpper();

      OSIUNITTEST_ASSERT_ERROR(nc == 8, return, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(nr == 5, return, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[0],2.5), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[1],0.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[1],4.1), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[2],1.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[0],2.5), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[4],3.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[1],2.1), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[4],15.), {}, "SoPlex", "read and copy exmip1");
      
      double newCs[8] = {1., 2., 3., 4., 5., 6., 7., 8.};
      soplexSi.setColSolution(newCs);
      const double * cs = soplexSi.getColSolution();
      OSIUNITTEST_ASSERT_ERROR(eq(cs[0],1.0), {}, "SoPlex", "set col solution");
      OSIUNITTEST_ASSERT_ERROR(eq(cs[7],8.0), {}, "SoPlex", "set col solution");
      {
        OsiSpxSolverInterface solnSi(soplexSi);
        const double * cs = solnSi.getColSolution();
        OSIUNITTEST_ASSERT_ERROR(eq(cs[0],1.0), {}, "SoPlex", "set col solution and copy");
        OSIUNITTEST_ASSERT_ERROR(eq(cs[7],8.0), {}, "SoPlex", "set col solution and copy");
      }

      OSIUNITTEST_ASSERT_ERROR(!eq(cl[3],1.2345), {}, "SoPlex", "set col lower");
      soplexSi.setColLower( 3, 1.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cl[3],1.2345), {}, "SoPlex", "set col lower");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(cu[4],10.2345), {}, "SoPlex", "set col upper");
      soplexSi.setColUpper( 4, 10.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cu[4],10.2345), {}, "SoPlex", "set col upper");

      OSIUNITTEST_ASSERT_ERROR(eq(soplexSi.getObjCoefficients()[0], 1.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(soplexSi.getObjCoefficients()[1], 0.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(soplexSi.getObjCoefficients()[2], 0.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(soplexSi.getObjCoefficients()[3], 0.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(soplexSi.getObjCoefficients()[4], 2.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(soplexSi.getObjCoefficients()[5], 0.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(soplexSi.getObjCoefficients()[6], 0.0), {}, "SoPlex", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(soplexSi.getObjCoefficients()[7],-1.0), {}, "SoPlex", "read and copy exmip1");
    }
    
    // Test getMatrixByRow method
    { 
      const OsiSpxSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();
      
      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  5, return, "SoPlex", "getMatrixByRow: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "SoPlex", "getMatrixByRow: num elements");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[0],   3.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[1],   1.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[2],  -2.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[3],  -1.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[4],  -1.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[5],   2.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[6],   1.1), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[7],   1.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[8],   1.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[9],   2.8), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], -1.2), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11],  5.6), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12],  1.0), {}, "SoPlex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13],  1.9), {}, "SoPlex", "getMatrixByRow: elements");
      
      const int * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "SoPlex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "SoPlex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "SoPlex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "SoPlex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "SoPlex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "SoPlex", "getMatrixByRow: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "SoPlex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "SoPlex", "getMatrixByRow: indices");
    }
    //--------------
    // Test rowsense, rhs, rowrange, getMatrixByRow
    {
      OsiSpxSolverInterface lhs;
      {     
        OsiSpxSolverInterface siC1(m);
        OSIUNITTEST_ASSERT_WARNING(siC1.rowrange_ == NULL, {}, "SoPlex", "row range");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsense_ == NULL, {}, "SoPlex", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1.rhs_ == NULL, {}, "SoPlex", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1.matrixByRow_ == NULL, {}, "SoPlex", "matrix by row");
        OSIUNITTEST_ASSERT_WARNING(siC1.colsol_ == NULL, {}, "SoPlex", "col solution");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsol_ == NULL, {}, "SoPlex", "row solution");

        const char   * siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "SoPlex", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "SoPlex", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "SoPlex", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "SoPlex", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "SoPlex", "row sense");
        
        const double * siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "SoPlex", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "SoPlex", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "SoPlex", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "SoPlex", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "SoPlex", "right hand side");
        
        const double * siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "SoPlex", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "SoPlex", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "SoPlex", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "SoPlex", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "SoPlex", "row range");
        
        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        OSIUNITTEST_ASSERT_ERROR(siC1mbr != NULL, {}, "SoPlex", "matrix by row");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getMajorDim()    ==  5, return, "SoPlex", "matrix by row: major dim");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getNumElements() == 14, return, "SoPlex", "matrix by row: num elements");
        
        const double * ev = siC1mbr->getElements();
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "SoPlex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "SoPlex", "matrix by row: elements");

        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "SoPlex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "SoPlex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "SoPlex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "SoPlex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "SoPlex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "SoPlex", "matrix by row: vector starts");
        
        const int * ei = siC1mbr->getIndices();
        OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "SoPlex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "SoPlex", "matrix by row: indices");

        OSIUNITTEST_ASSERT_WARNING(siC1rs  == siC1.getRowSense(), {}, "SoPlex", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1rhs == siC1.getRightHandSide(), {}, "SoPlex", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1rr  == siC1.getRowRange(), {}, "SoPlex", "row range");

        // Change SOPLEX Model by adding free row
        OsiRowCut rc;
        rc.setLb(-COIN_DBL_MAX);
        rc.setUb( COIN_DBL_MAX);
        OsiCuts cuts;
        cuts.insert(rc);
        siC1.applyCuts(cuts);
             
        // Since model was changed, test that cached data is now freed.
        OSIUNITTEST_ASSERT_ERROR(siC1.rowrange_ == NULL, {}, "SoPlex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsense_ == NULL, {}, "SoPlex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rhs_ == NULL, {}, "SoPlex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByRow_ == NULL, {}, "SoPlex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByCol_ == NULL, {}, "SoPlex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.colsol_ == NULL, {}, "SoPlex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsol_ == NULL, {}, "SoPlex", "free cached data after adding row");

        siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "SoPlex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "SoPlex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "SoPlex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "SoPlex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "SoPlex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[5] == 'N', {}, "SoPlex", "row sense after adding row");

        siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "SoPlex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "SoPlex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "SoPlex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "SoPlex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "SoPlex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[5],0.0), {}, "SoPlex", "right hand side after adding row");

        siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "SoPlex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "SoPlex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "SoPlex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "SoPlex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "SoPlex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[5],0.0), {}, "SoPlex", "row range after adding row");
    
        lhs = siC1;
      }
      // Test that lhs has correct values even though siC1 has gone out of scope    
      OSIUNITTEST_ASSERT_ERROR(lhs.rowrange_ == NULL, {}, "SoPlex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsense_ == NULL, {}, "SoPlex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rhs_ == NULL, {}, "SoPlex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByRow_ == NULL, {}, "SoPlex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByCol_ == NULL, {}, "SoPlex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.colsol_ == NULL, {}, "SoPlex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsol_ == NULL, {}, "SoPlex", "freed origin after assignment");

      const char * lhsrs  = lhs.getRowSense();
      OSIUNITTEST_ASSERT_ERROR(lhsrs[0] == 'G', {}, "SoPlex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[1] == 'L', {}, "SoPlex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[2] == 'E', {}, "SoPlex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[3] == 'R', {}, "SoPlex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[4] == 'R', {}, "SoPlex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[5] == 'N', {}, "SoPlex", "row sense after assignment");
      
      const double * lhsrhs = lhs.getRightHandSide();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[0],2.5), {}, "SoPlex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[1],2.1), {}, "SoPlex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[2],4.0), {}, "SoPlex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[3],5.0), {}, "SoPlex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[4],15.), {}, "SoPlex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[5],0.0), {}, "SoPlex", "right hand side after assignment");
      
      const double *lhsrr = lhs.getRowRange();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[0],0.0), {}, "SoPlex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[1],0.0), {}, "SoPlex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[2],0.0), {}, "SoPlex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[3],5.0-1.8), {}, "SoPlex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[4],15.0-3.0), {}, "SoPlex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[5],0.0), {}, "SoPlex", "row range after assignment");
      
      const CoinPackedMatrix * lhsmbr = lhs.getMatrixByRow();
      OSIUNITTEST_ASSERT_ERROR(lhsmbr != NULL, {}, "SoPlex", "matrix by row after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getMajorDim()    ==  6, return, "SoPlex", "matrix by row after assignment: major dim");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getNumElements() == 14, return, "SoPlex", "matrix by row after assignment: num elements");

      const double * ev = lhsmbr->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "SoPlex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "SoPlex", "matrix by row after assignment: elements");
      
      const CoinBigIndex * mi = lhsmbr->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "SoPlex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "SoPlex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "SoPlex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "SoPlex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "SoPlex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "SoPlex", "matrix by row after assignment: vector starts");
      
      const int * ei = lhsmbr->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "SoPlex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "SoPlex", "matrix by row after assignment: indices");
    }
    
    //--------------    
  }

    
  // Do common solverInterface testing by calling the
  // base class testing method.
  {
    OsiSpxSolverInterface m;
    OsiSolverInterfaceCommonUnitTest(&m, mpsDir,netlibDir);
  }
}

#endif
