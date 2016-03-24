//-----------------------------------------------------------------------------
// name:     OSI Interface for GUROBI
// template: OSI Cplex Interface written by T. Achterberg
// author:   Stefan Vigerske
//           Humboldt University Berlin
// license:  this file may be freely distributed under the terms of EPL
// comments: please scan this file for '???' and 'TODO' and read the comments
//-----------------------------------------------------------------------------
// Copyright (C) 2009 Humboldt University Berlin and others.
// Corporation and others.  All Rights Reserved.

#include "CoinPragma.hpp"
#include "OsiConfig.h"

//#include <cassert>
//#include <iostream>

#include "OsiUnitTests.hpp"
#include "OsiGrbSolverInterface.hpp"
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"

// Added so build windows build with dsp files works,
// when not building with gurobi.
#ifdef COIN_HAS_GRB
#include "gurobi_c.h"

//--------------------------------------------------------------------------
void OsiGrbSolverInterfaceUnitTest( const std::string & mpsDir, const std::string & netlibDir )
{
  // Test default constructor
  {
    OsiGrbSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(m.obj_         == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.collower_    == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colupper_    == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.coltype_     == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colspace_    ==    0, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsense_    == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rhs_         == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowrange_    == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowlower_    == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowupper_    == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colsol_      == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsol_      == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_ == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByCol_ == NULL, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getNumCols() ==    0, {}, "gurobi", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getApplicationData() == NULL, {}, "gurobi", "default constructor");
    int i=2346;
    m.setApplicationData(&i);
    OSIUNITTEST_ASSERT_ERROR(*((int *)(m.getApplicationData())) == i, {}, "gurobi", "default constructor");
  }

  {
    CoinRelFltEq eq;
    OsiGrbSolverInterface m;
    std::string fn = mpsDir+"exmip1";
    m.readMps(fn.c_str(),"mps");

    {
      OSIUNITTEST_ASSERT_ERROR(m.getNumCols() == 8, {}, "gurobi", "exmip1 read");
      const CoinPackedMatrix * colCopy = m.getMatrixByCol();
      OSIUNITTEST_ASSERT_ERROR(colCopy->getNumCols()  == 8, {}, "gurobi", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getMajorDim() == 8, {}, "gurobi", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getNumRows()  == 5, {}, "gurobi", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getMinorDim() == 5, {}, "gurobi", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getVectorLengths()[7] == 2, {}, "gurobi", "exmip1 matrix");
      CoinPackedMatrix revColCopy;
      revColCopy.reverseOrderedCopyOf(*colCopy);
      CoinPackedMatrix rev2ColCopy;      
      rev2ColCopy.reverseOrderedCopyOf(revColCopy);
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getNumCols()  == 8, {}, "gurobi", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getMajorDim() == 8, {}, "gurobi", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getNumRows()  == 5, {}, "gurobi", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getMinorDim() == 5, {}, "gurobi", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getVectorLengths()[7] == 2, {}, "gurobi", "twice reverse matrix copy");
    }
    
    // Test copy constructor and assignment operator
    {
      OsiGrbSolverInterface lhs;
      {      
        OsiGrbSolverInterface im(m);

        OsiGrbSolverInterface imC1(im);
        OSIUNITTEST_ASSERT_ERROR(imC1.lp_          != im.lp_,           {}, "gurobi", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumCols() == im.getNumCols(),  {}, "gurobi", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumRows() == im.getNumRows(),  {}, "gurobi", "copy constructor");
        
        OsiGrbSolverInterface imC2(im);
        OSIUNITTEST_ASSERT_ERROR(imC2.lp_          != im.lp_,           {}, "gurobi", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumCols() == im.getNumCols(),  {}, "gurobi", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumRows() == im.getNumRows(),  {}, "gurobi", "copy constructor");
        
        OSIUNITTEST_ASSERT_ERROR(imC1.lp_ != imC2.lp_, {}, "gurobi", "copy constructor");
        
        lhs = imC2;
      }

      // Test that lhs has correct values even though rhs has gone out of scope
      OSIUNITTEST_ASSERT_ERROR(lhs.lp_          != m.lp_,           {}, "gurobi", "assignment operator");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumCols() == m.getNumCols(),  {}, "gurobi", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumRows() == m.getNumRows(),  {}, "gurobi", "copy constructor");
    }
    
    // Test clone
    {
      OsiGrbSolverInterface gurobiSi(m);
      OsiSolverInterface * siPtr = &gurobiSi;
      OsiSolverInterface * siClone = siPtr->clone();
      OsiGrbSolverInterface * gurobiClone = dynamic_cast<OsiGrbSolverInterface*>(siClone);
      OSIUNITTEST_ASSERT_ERROR(gurobiClone != NULL, {}, "gurobi", "clone");
      OSIUNITTEST_ASSERT_ERROR(gurobiClone->lp_          != gurobiSi.lp_, {}, "gurobi", "clone");
      OSIUNITTEST_ASSERT_ERROR(gurobiClone->getNumRows() == gurobiSi.getNumRows(), {}, "gurobi", "clone");
      OSIUNITTEST_ASSERT_ERROR(gurobiClone->getNumCols() == m.getNumCols(), {}, "gurobi", "clone");
      
      delete siClone;
    }
   
    // test infinity
    {
      OsiGrbSolverInterface si;
      OSIUNITTEST_ASSERT_ERROR(si.getInfinity() == GRB_INFINITY, {}, "gurobi", "value for infinity");
    }     


    {    
      OsiGrbSolverInterface gurobiSi(m);
      int nc = gurobiSi.getNumCols();
      int nr = gurobiSi.getNumRows();
      const double * cl = gurobiSi.getColLower();
      const double * cu = gurobiSi.getColUpper();
      const double * rl = gurobiSi.getRowLower();
      const double * ru = gurobiSi.getRowUpper();

      OSIUNITTEST_ASSERT_ERROR(nc == 8, return, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(nr == 5, return, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[0],2.5), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[1],0.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[1],4.1), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[2],1.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[0],2.5), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[4],3.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[1],2.1), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[4],15.), {}, "gurobi", "read and copy exmip1");
      
      double newCs[8] = {1., 2., 3., 4., 5., 6., 7., 8.};
      gurobiSi.setColSolution(newCs);
      const double * cs = gurobiSi.getColSolution();
      OSIUNITTEST_ASSERT_ERROR(eq(cs[0],1.0), {}, "gurobi", "set col solution");
      OSIUNITTEST_ASSERT_ERROR(eq(cs[7],8.0), {}, "gurobi", "set col solution");
#if 0 // TODO set and copy of solutions not supported by OsiGrb currently
      {
        OsiGrbSolverInterface solnSi(gurobiSi);
        const double * cs = solnSi.getColSolution();
        OSIUNITTEST_ASSERT_ERROR(eq(cs[0],1.0), {}, "gurobi", "set col solution and copy");
        OSIUNITTEST_ASSERT_ERROR(eq(cs[7],8.0), {}, "gurobi", "set col solution and copy");
      }
#endif

      OSIUNITTEST_ASSERT_ERROR(!eq(cl[3],1.2345), {}, "gurobi", "set col lower");
      gurobiSi.setColLower( 3, 1.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cl[3],1.2345), {}, "gurobi", "set col lower");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(cu[4],10.2345), {}, "gurobi", "set col upper");
      gurobiSi.setColUpper( 4, 10.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cu[4],10.2345), {}, "gurobi", "set col upper");

      OSIUNITTEST_ASSERT_ERROR(eq(gurobiSi.getObjCoefficients()[0], 1.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(gurobiSi.getObjCoefficients()[1], 0.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(gurobiSi.getObjCoefficients()[2], 0.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(gurobiSi.getObjCoefficients()[3], 0.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(gurobiSi.getObjCoefficients()[4], 2.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(gurobiSi.getObjCoefficients()[5], 0.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(gurobiSi.getObjCoefficients()[6], 0.0), {}, "gurobi", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(gurobiSi.getObjCoefficients()[7],-1.0), {}, "gurobi", "read and copy exmip1");
    }

    // Test getMatrixByRow method
    { 
      const OsiGrbSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();
      
      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  5, return, "gurobi", "getMatrixByRow: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "gurobi", "getMatrixByRow: num elements");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[0],   3.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[1],   1.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[2],  -2.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[3],  -1.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[4],  -1.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[5],   2.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[6],   1.1), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[7],   1.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[8],   1.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[9],   2.8), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], -1.2), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11],  5.6), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12],  1.0), {}, "gurobi", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13],  1.9), {}, "gurobi", "getMatrixByRow: elements");
      
      const int * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "gurobi", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "gurobi", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "gurobi", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "gurobi", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "gurobi", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "gurobi", "getMatrixByRow: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "gurobi", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "gurobi", "getMatrixByRow: indices");
    }
    //--------------
    // Test rowsense, rhs, rowrange, getMatrixByRow
    {
      OsiGrbSolverInterface lhs;
      {     
        OsiGrbSolverInterface siC1(m);
        OSIUNITTEST_ASSERT_WARNING(siC1.obj_ == NULL, {}, "gurobi", "objective");
        OSIUNITTEST_ASSERT_WARNING(siC1.collower_ == NULL, {}, "gurobi", "col lower");
        OSIUNITTEST_ASSERT_WARNING(siC1.colupper_ == NULL, {}, "gurobi", "col upper");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowrange_ == NULL, {}, "gurobi", "row range");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsense_ == NULL, {}, "gurobi", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowlower_ == NULL, {}, "gurobi", "row lower");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowupper_ == NULL, {}, "gurobi", "row upper");
        OSIUNITTEST_ASSERT_WARNING(siC1.rhs_ == NULL, {}, "gurobi", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1.matrixByRow_ == NULL, {}, "gurobi", "matrix by row");
//TODO        OSIUNITTEST_ASSERT_WARNING(siC1.colsol_ != NULL, {}, "gurobi", "col solution");
//TODO        OSIUNITTEST_ASSERT_WARNING(siC1.rowsol_ != NULL, {}, "gurobi", "row solution");

        const char   * siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "gurobi", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "gurobi", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "gurobi", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "gurobi", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "gurobi", "row sense");
        
        const double * siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "gurobi", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "gurobi", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "gurobi", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "gurobi", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "gurobi", "right hand side");
        
        const double * siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "gurobi", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "gurobi", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "gurobi", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "gurobi", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "gurobi", "row range");
        
        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        OSIUNITTEST_ASSERT_ERROR(siC1mbr != NULL, {}, "gurobi", "matrix by row");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getMajorDim()    ==  5, return, "gurobi", "matrix by row: major dim");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getNumElements() == 14, return, "gurobi", "matrix by row: num elements");
        
        const double * ev = siC1mbr->getElements();
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "gurobi", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "gurobi", "matrix by row: elements");

        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "gurobi", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "gurobi", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "gurobi", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "gurobi", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "gurobi", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "gurobi", "matrix by row: vector starts");
        
        const int * ei = siC1mbr->getIndices();
        OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "gurobi", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "gurobi", "matrix by row: indices");

        OSIUNITTEST_ASSERT_WARNING(siC1rs  == siC1.getRowSense(), {}, "gurobi", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1rhs == siC1.getRightHandSide(), {}, "gurobi", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1rr  == siC1.getRowRange(), {}, "gurobi", "row range");

#if 0 // TODO: free rows not really supported by OsiGrb
        // Change GUROBI Model by adding free row
        OsiRowCut rc;
        rc.setLb(-COIN_DBL_MAX);
        rc.setUb( COIN_DBL_MAX);
        OsiCuts cuts;
        cuts.insert(rc);
        siC1.applyCuts(cuts);

        // Since model was changed, test that cached data is now freed.
        OSIUNITTEST_ASSERT_ERROR(siC1.obj_ == NULL, {}, "gurobi", "objective");
        OSIUNITTEST_ASSERT_ERROR(siC1.collower_ == NULL, {}, "gurobi", "col lower");
        OSIUNITTEST_ASSERT_ERROR(siC1.colupper_ == NULL, {}, "gurobi", "col upper");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowrange_ == NULL, {}, "gurobi", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsense_ == NULL, {}, "gurobi", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowlower_ == NULL, {}, "gurobi", "row lower");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowupper_ == NULL, {}, "gurobi", "row upper");
        OSIUNITTEST_ASSERT_ERROR(siC1.rhs_ == NULL, {}, "gurobi", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByRow_ == NULL, {}, "gurobi", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByCol_ == NULL, {}, "gurobi", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.colsol_ == NULL, {}, "gurobi", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsol_ == NULL, {}, "gurobi", "free cached data after adding row");

        siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "gurobi", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "gurobi", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "gurobi", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "gurobi", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "gurobi", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[5] == 'N', {}, "gurobi", "row sense after adding row");

        siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "gurobi", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "gurobi", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "gurobi", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "gurobi", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "gurobi", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[5],0.0), {}, "gurobi", "right hand side after adding row");

        siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "gurobi", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "gurobi", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "gurobi", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "gurobi", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "gurobi", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[5],0.0), {}, "gurobi", "row range after adding row");
#endif
        lhs = siC1;
      }
      // Test that lhs has correct values even though siC1 has gone out of scope    
      OSIUNITTEST_ASSERT_ERROR(lhs.obj_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.collower_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.colupper_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowrange_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsense_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowlower_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowupper_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rhs_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByRow_ == NULL, {}, "gurobi", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByCol_ == NULL, {}, "gurobi", "freed origin after assignment");
//TODO      OSIUNITTEST_ASSERT_ERROR(lhs.colsol_ != NULL, {}, "gurobi", "freed origin after assignment");
//TODO      OSIUNITTEST_ASSERT_ERROR(lhs.rowsol_ != NULL, {}, "gurobi", "freed origin after assignment");

#if 0 // TODO: free rows not really supported by OsiGrb
      const char * lhsrs  = lhs.getRowSense();
      OSIUNITTEST_ASSERT_ERROR(lhsrs[0] == 'G', {}, "gurobi", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[1] == 'L', {}, "gurobi", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[2] == 'E', {}, "gurobi", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[3] == 'R', {}, "gurobi", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[4] == 'R', {}, "gurobi", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[5] == 'N', {}, "gurobi", "row sense after assignment");
      
      const double * lhsrhs = lhs.getRightHandSide();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[0],2.5), {}, "gurobi", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[1],2.1), {}, "gurobi", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[2],4.0), {}, "gurobi", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[3],5.0), {}, "gurobi", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[4],15.), {}, "gurobi", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[5],0.0), {}, "gurobi", "right hand side after assignment");
      
      const double *lhsrr = lhs.getRowRange();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[0],0.0), {}, "gurobi", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[1],0.0), {}, "gurobi", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[2],0.0), {}, "gurobi", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[3],5.0-1.8), {}, "gurobi", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[4],15.0-3.0), {}, "gurobi", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[5],0.0), {}, "gurobi", "row range after assignment");
      
      const CoinPackedMatrix * lhsmbr = lhs.getMatrixByRow();
      OSIUNITTEST_ASSERT_ERROR(lhsmbr != NULL, {}, "gurobi", "matrix by row after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getMajorDim()    ==  6, return, "gurobi", "matrix by row after assignment: major dim");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getNumElements() == 14, return, "gurobi", "matrix by row after assignment: num elements");

      const double * ev = lhsmbr->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "gurobi", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "gurobi", "matrix by row after assignment: elements");
      
      const CoinBigIndex * mi = lhsmbr->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "gurobi", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "gurobi", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "gurobi", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "gurobi", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "gurobi", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "gurobi", "matrix by row after assignment: vector starts");
      
      const int * ei = lhsmbr->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "gurobi", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "gurobi", "matrix by row after assignment: indices");
#endif
    }
  }

  // Do common solverInterface testing by calling the
  // base class testing method.
  {
    OsiGrbSolverInterface m;
    OsiSolverInterfaceCommonUnitTest(&m, mpsDir, netlibDir);
  }
}
#endif
