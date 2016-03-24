//-----------------------------------------------------------------------------
// name:     OSI Interface for CPLEX
// author:   Tobias Pfender
//           Konrad-Zuse-Zentrum Berlin (Germany)
// date:     09/25/2000
// license:  this file may be freely distributed under the terms of EPL
// comments: please scan this file for '???' and read the comments
//-----------------------------------------------------------------------------
// Copyright (C) 2000, Tobias Pfender, International Business Machines
// Corporation and others.  All Rights Reserved.

#include "CoinPragma.hpp"
#include "OsiConfig.h"

//#include <cassert>
//#include <iostream>

#include "OsiUnitTests.hpp"
#include "OsiCpxSolverInterface.hpp"
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"

// Added so build windows build with dsp files works,
// when not building with cplex.
#ifdef COIN_HAS_CPX
#include "cplex.h"

void OsiCpxSolverInterfaceUnitTest( const std::string & mpsDir, const std::string & netlibDir )
{
  // Test default constructor
  {
    OsiCpxSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(m.obj_         == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.collower_    == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colupper_    == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.coltype_     == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.coltypesize_ ==    0, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsense_    == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rhs_         == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowrange_    == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowlower_    == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowupper_    == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colsol_      == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsol_      == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_ == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByCol_ == NULL, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getNumCols() ==    0, {}, "cplex", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getApplicationData() == NULL, {}, "cplex", "default constructor");
    int i=2346;
    m.setApplicationData(&i);
    OSIUNITTEST_ASSERT_ERROR(*((int *)(m.getApplicationData())) == i, {}, "cplex", "default constructor");
  }

  {
    CoinRelFltEq eq;
    OsiCpxSolverInterface m;
    std::string fn = mpsDir+"exmip1";
    m.readMps(fn.c_str(),"mps");

    {
      OSIUNITTEST_ASSERT_ERROR(m.getNumCols() == 8, {}, "cplex", "exmip1 read");
      const CoinPackedMatrix * colCopy = m.getMatrixByCol();
      OSIUNITTEST_ASSERT_ERROR(colCopy->getNumCols()  == 8, {}, "cplex", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getMajorDim() == 8, {}, "cplex", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getNumRows()  == 5, {}, "cplex", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getMinorDim() == 5, {}, "cplex", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getVectorLengths()[7] == 2, {}, "cplex", "exmip1 matrix");
      CoinPackedMatrix revColCopy;
      revColCopy.reverseOrderedCopyOf(*colCopy);
      CoinPackedMatrix rev2ColCopy;      
      rev2ColCopy.reverseOrderedCopyOf(revColCopy);
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getNumCols()  == 8, {}, "cplex", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getMajorDim() == 8, {}, "cplex", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getNumRows()  == 5, {}, "cplex", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getMinorDim() == 5, {}, "cplex", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getVectorLengths()[7] == 2, {}, "cplex", "twice reverse matrix copy");
    }
    
    // Test copy constructor and assignment operator
    {
      OsiCpxSolverInterface lhs;
      {      
        OsiCpxSolverInterface im(m);

        OsiCpxSolverInterface imC1(im);
        OSIUNITTEST_ASSERT_ERROR(imC1.lp_          != im.lp_,           {}, "cplex", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumCols() == im.getNumCols(),  {}, "cplex", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumRows() == im.getNumRows(),  {}, "cplex", "copy constructor");
        
        OsiCpxSolverInterface imC2(im);
        OSIUNITTEST_ASSERT_ERROR(imC2.lp_          != im.lp_,           {}, "cplex", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumCols() == im.getNumCols(),  {}, "cplex", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumRows() == im.getNumRows(),  {}, "cplex", "copy constructor");
        
        OSIUNITTEST_ASSERT_ERROR(imC1.lp_ != imC2.lp_, {}, "cplex", "copy constructor");
        
        lhs = imC2;
      }

      // Test that lhs has correct values even though rhs has gone out of scope
      OSIUNITTEST_ASSERT_ERROR(lhs.lp_          != m.lp_,           {}, "cplex", "assignment operator");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumCols() == m.getNumCols(),  {}, "cplex", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumRows() == m.getNumRows(),  {}, "cplex", "copy constructor");
    }
    
    // Test clone
    {
      OsiCpxSolverInterface cplexSi(m);
      OsiSolverInterface * siPtr = &cplexSi;
      OsiSolverInterface * siClone = siPtr->clone();
      OsiCpxSolverInterface * cplexClone = dynamic_cast<OsiCpxSolverInterface*>(siClone);
      OSIUNITTEST_ASSERT_ERROR(cplexClone != NULL, {}, "cplex", "clone");
      OSIUNITTEST_ASSERT_ERROR(cplexClone->lp_          != cplexSi.lp_, {}, "cplex", "clone");
      OSIUNITTEST_ASSERT_ERROR(cplexClone->getNumRows() == cplexSi.getNumRows(), {}, "cplex", "clone");
      OSIUNITTEST_ASSERT_ERROR(cplexClone->getNumCols() == m.getNumCols(), {}, "cplex", "clone");
      
      delete siClone;
    }
   
    // test infinity
    {
      OsiCpxSolverInterface si;
      OSIUNITTEST_ASSERT_ERROR(si.getInfinity() == CPX_INFBOUND, {}, "cplex", "value for infinity");
    }     

    // Test getMatrixByRow method
    { 
      const OsiCpxSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();
      
      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  5, return, "cplex", "getMatrixByRow: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "cplex", "getMatrixByRow: num elements");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[0],   3.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[1],   1.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[2],  -2.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[3],  -1.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[4],  -1.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[5],   2.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[6],   1.1), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[7],   1.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[8],   1.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[9],   2.8), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], -1.2), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11],  5.6), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12],  1.0), {}, "cplex", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13],  1.9), {}, "cplex", "getMatrixByRow: elements");
      
      const int * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "cplex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "cplex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "cplex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "cplex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "cplex", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "cplex", "getMatrixByRow: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "cplex", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "cplex", "getMatrixByRow: indices");
    }
    //--------------
    // Test rowsense, rhs, rowrange, getMatrixByRow
    {
      OsiCpxSolverInterface lhs;
      {     
        OsiCpxSolverInterface siC1(m);
        OSIUNITTEST_ASSERT_WARNING(siC1.obj_ == NULL, {}, "cplex", "objective");
        OSIUNITTEST_ASSERT_WARNING(siC1.collower_ == NULL, {}, "cplex", "col lower");
        OSIUNITTEST_ASSERT_WARNING(siC1.colupper_ == NULL, {}, "cplex", "col upper");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowrange_ == NULL, {}, "cplex", "row range");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsense_ == NULL, {}, "cplex", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowlower_ == NULL, {}, "cplex", "row lower");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowupper_ == NULL, {}, "cplex", "row upper");
        OSIUNITTEST_ASSERT_WARNING(siC1.rhs_ == NULL, {}, "cplex", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1.matrixByRow_ == NULL, {}, "cplex", "matrix by row");
        OSIUNITTEST_ASSERT_WARNING(siC1.colsol_ != NULL, {}, "cplex", "col solution");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsol_ != NULL, {}, "cplex", "row solution");

        const char   * siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "cplex", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "cplex", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "cplex", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "cplex", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "cplex", "row sense");
        
        const double * siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "cplex", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "cplex", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "cplex", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "cplex", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "cplex", "right hand side");
        
        const double * siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "cplex", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "cplex", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "cplex", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "cplex", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "cplex", "row range");
        
        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        OSIUNITTEST_ASSERT_ERROR(siC1mbr != NULL, {}, "cplex", "matrix by row");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getMajorDim()    ==  5, return, "cplex", "matrix by row: major dim");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getNumElements() == 14, return, "cplex", "matrix by row: num elements");
        
        const double * ev = siC1mbr->getElements();
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "cplex", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "cplex", "matrix by row: elements");

        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "cplex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "cplex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "cplex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "cplex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "cplex", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "cplex", "matrix by row: vector starts");
        
        const int * ei = siC1mbr->getIndices();
        OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "cplex", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "cplex", "matrix by row: indices");

        OSIUNITTEST_ASSERT_WARNING(siC1rs  == siC1.getRowSense(), {}, "cplex", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1rhs == siC1.getRightHandSide(), {}, "cplex", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1rr  == siC1.getRowRange(), {}, "cplex", "row range");

        // Change CPLEX Model by adding free row
        OsiRowCut rc;
        rc.setLb(-COIN_DBL_MAX);
        rc.setUb( COIN_DBL_MAX);
        OsiCuts cuts;
        cuts.insert(rc);
        siC1.applyCuts(cuts);

        // Since model was changed, test that cached data is now freed.
        OSIUNITTEST_ASSERT_ERROR(siC1.obj_ == NULL, {}, "cplex", "objective");
        OSIUNITTEST_ASSERT_ERROR(siC1.collower_ == NULL, {}, "cplex", "col lower");
        OSIUNITTEST_ASSERT_ERROR(siC1.colupper_ == NULL, {}, "cplex", "col upper");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowrange_ == NULL, {}, "cplex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsense_ == NULL, {}, "cplex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowlower_ == NULL, {}, "cplex", "row lower");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowupper_ == NULL, {}, "cplex", "row upper");
        OSIUNITTEST_ASSERT_ERROR(siC1.rhs_ == NULL, {}, "cplex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByRow_ == NULL, {}, "cplex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByCol_ == NULL, {}, "cplex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.colsol_ == NULL, {}, "cplex", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsol_ == NULL, {}, "cplex", "free cached data after adding row");

        siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "cplex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "cplex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "cplex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "cplex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "cplex", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[5] == 'N', {}, "cplex", "row sense after adding row");

        siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "cplex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "cplex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "cplex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "cplex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "cplex", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[5],0.0), {}, "cplex", "right hand side after adding row");

        siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "cplex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "cplex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "cplex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "cplex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "cplex", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[5],0.0), {}, "cplex", "row range after adding row");
    
        lhs = siC1;
      }
      // Test that lhs has correct values even though siC1 has gone out of scope    
      OSIUNITTEST_ASSERT_ERROR(lhs.obj_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.collower_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.colupper_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowrange_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsense_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowlower_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowupper_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rhs_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByRow_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByCol_ == NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.colsol_ != NULL, {}, "cplex", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsol_ != NULL, {}, "cplex", "freed origin after assignment");

      const char * lhsrs  = lhs.getRowSense();
      OSIUNITTEST_ASSERT_ERROR(lhsrs[0] == 'G', {}, "cplex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[1] == 'L', {}, "cplex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[2] == 'E', {}, "cplex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[3] == 'R', {}, "cplex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[4] == 'R', {}, "cplex", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[5] == 'N', {}, "cplex", "row sense after assignment");
      
      const double * lhsrhs = lhs.getRightHandSide();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[0],2.5), {}, "cplex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[1],2.1), {}, "cplex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[2],4.0), {}, "cplex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[3],5.0), {}, "cplex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[4],15.), {}, "cplex", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[5],0.0), {}, "cplex", "right hand side after assignment");
      
      const double *lhsrr = lhs.getRowRange();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[0],0.0), {}, "cplex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[1],0.0), {}, "cplex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[2],0.0), {}, "cplex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[3],5.0-1.8), {}, "cplex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[4],15.0-3.0), {}, "cplex", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[5],0.0), {}, "cplex", "row range after assignment");
      
      const CoinPackedMatrix * lhsmbr = lhs.getMatrixByRow();
      OSIUNITTEST_ASSERT_ERROR(lhsmbr != NULL, {}, "cplex", "matrix by row after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getMajorDim()    ==  6, return, "cplex", "matrix by row after assignment: major dim");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getNumElements() == 14, return, "cplex", "matrix by row after assignment: num elements");

      const double * ev = lhsmbr->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "cplex", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "cplex", "matrix by row after assignment: elements");
      
      const CoinBigIndex * mi = lhsmbr->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "cplex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "cplex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "cplex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "cplex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "cplex", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "cplex", "matrix by row after assignment: vector starts");
      
      const int * ei = lhsmbr->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "cplex", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "cplex", "matrix by row after assignment: indices");
    }
  }

  // Do common solverInterface testing by calling the
  // base class testing method.
  {
    OsiCpxSolverInterface m;
    OsiSolverInterfaceCommonUnitTest(&m, mpsDir, netlibDir);
  }
}
#endif
