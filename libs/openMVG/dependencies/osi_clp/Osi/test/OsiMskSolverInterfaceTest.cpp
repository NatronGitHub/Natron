/*
  Unit test for OsiMsk.

  This file is licensed under the terms of Eclipse Public License (EPL).
*/

#include "CoinPragma.hpp"
#include "OsiConfig.h"

//#include <cassert>
//#include <iostream>

#include "OsiUnitTests.hpp"
#include "OsiMskSolverInterface.hpp"
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"

// Added so build windows build with dsp files works,
// when not building with gurobi.
#ifdef COIN_HAS_MSK
#include "mosek.h"

//--------------------------------------------------------------------------
void OsiMskSolverInterfaceUnitTest( const std::string & mpsDir, const std::string & netlibDir )
{
  unsigned int numInstancesStart = OsiMskSolverInterface::getNumInstances();
  // Test default constructor
  {
    OsiMskSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(m.obj_         == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.collower_    == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colupper_    == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.coltype_     == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.coltypesize_ ==    0, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsense_    == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rhs_         == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowrange_    == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowlower_    == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowupper_    == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.colsol_      == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.rowsol_      == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByRow_ == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.matrixByCol_ == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getNumCols() ==    0, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(m.getApplicationData() == NULL, {}, "mosek", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(OsiMskSolverInterface::getNumInstances() == numInstancesStart+1, {}, "mosek", "number of instances during first test");
    int i=2346;
    m.setApplicationData(&i);
    OSIUNITTEST_ASSERT_ERROR(*((int *)(m.getApplicationData())) == i, {}, "mosek", "default constructor");
  }
  OSIUNITTEST_ASSERT_ERROR(OsiMskSolverInterface::getNumInstances() == numInstancesStart, {}, "mosek", "number of instances after first test");

  {    
    CoinRelFltEq eq;
    OsiMskSolverInterface m;
    OSIUNITTEST_ASSERT_ERROR(OsiMskSolverInterface::getNumInstances() == numInstancesStart+1, {}, "mosek", "number of instances");
    std::string fn = mpsDir+"exmip1";
    m.readMps(fn.c_str(),"mps");

    {
      OSIUNITTEST_ASSERT_ERROR(m.getNumCols() == 8, {}, "mosek", "exmip1 read");
      const CoinPackedMatrix * colCopy = m.getMatrixByCol();
      OSIUNITTEST_ASSERT_ERROR(colCopy->getNumCols()  == 8, {}, "mosek", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getMajorDim() == 8, {}, "mosek", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getNumRows()  == 5, {}, "mosek", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getMinorDim() == 5, {}, "mosek", "exmip1 matrix");
      OSIUNITTEST_ASSERT_ERROR(colCopy->getVectorLengths()[7] == 2, {}, "mosek", "exmip1 matrix");
      CoinPackedMatrix revColCopy;
      revColCopy.reverseOrderedCopyOf(*colCopy);
      CoinPackedMatrix rev2ColCopy;      
      rev2ColCopy.reverseOrderedCopyOf(revColCopy);
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getNumCols()  == 8, {}, "mosek", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getMajorDim() == 8, {}, "mosek", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getNumRows()  == 5, {}, "mosek", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getMinorDim() == 5, {}, "mosek", "twice reverse matrix copy");
      OSIUNITTEST_ASSERT_ERROR(rev2ColCopy.getVectorLengths()[7] == 2, {}, "mosek", "twice reverse matrix copy");
    }

    // Test copy constructor and assignment operator
    {
      OsiMskSolverInterface lhs;
      {      
        OsiMskSolverInterface im(m);

        OsiMskSolverInterface imC1(im);
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumCols() == im.getNumCols(),  {}, "mosek", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC1.getNumRows() == im.getNumRows(),  {}, "mosek", "copy constructor");
        
        OsiMskSolverInterface imC2(im);
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumCols() == im.getNumCols(),  {}, "mosek", "copy constructor");
        OSIUNITTEST_ASSERT_ERROR(imC2.getNumRows() == im.getNumRows(),  {}, "mosek", "copy constructor");
        
        lhs = imC2;
      }

      // Test that lhs has correct values even though rhs has gone out of scope
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumCols() == m.getNumCols(),  {}, "mosek", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(lhs.getNumRows() == m.getNumRows(),  {}, "mosek", "copy constructor");
    }
    
    // Test clone
    {
      OsiMskSolverInterface mosekSi(m);
      OsiSolverInterface * siPtr = &mosekSi;
      OsiSolverInterface * siClone = siPtr->clone();
      OsiMskSolverInterface * mosekClone = dynamic_cast<OsiMskSolverInterface*>(siClone);
      OSIUNITTEST_ASSERT_ERROR(mosekClone != NULL, {}, "mosek", "clone");
      OSIUNITTEST_ASSERT_ERROR(mosekClone->getNumRows() == mosekSi.getNumRows(), {}, "mosek", "clone");
      OSIUNITTEST_ASSERT_ERROR(mosekClone->getNumCols() == m.getNumCols(), {}, "mosek", "clone");
      
      delete siClone;
    }

    // test infinity
    {
      OsiMskSolverInterface si;
      OSIUNITTEST_ASSERT_ERROR(si.getInfinity() == MSK_INFINITY, {}, "mosek", "value for infinity");
    }     

    {    
      OsiMskSolverInterface mosekSi(m);
      int nc = mosekSi.getNumCols();
      int nr = mosekSi.getNumRows();
      const double * cl = mosekSi.getColLower();
      const double * cu = mosekSi.getColUpper();
      const double * rl = mosekSi.getRowLower();
      const double * ru = mosekSi.getRowUpper();

      OSIUNITTEST_ASSERT_ERROR(nc == 8, return, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(nr == 5, return, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[0],2.5), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cl[1],0.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[1],4.1), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(cu[2],1.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[0],2.5), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(rl[4],3.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[1],2.1), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(ru[4],15.), {}, "mosek", "read and copy exmip1");
      
      double newCs[8] = {1., 2., 3., 4., 5., 6., 7., 8.};
      mosekSi.setColSolution(newCs);
      const double * cs = mosekSi.getColSolution();
      OSIUNITTEST_ASSERT_ERROR(eq(cs[0],1.0), {}, "mosek", "set col solution");
      OSIUNITTEST_ASSERT_ERROR(eq(cs[7],8.0), {}, "mosek", "set col solution");
      {
        OsiMskSolverInterface solnSi(mosekSi);
        const double * cs = solnSi.getColSolution();
        OSIUNITTEST_ASSERT_ERROR(eq(cs[0],1.0), {}, "mosek", "set col solution and copy");
        OSIUNITTEST_ASSERT_ERROR(eq(cs[7],8.0), {}, "mosek", "set col solution and copy");
      }

      OSIUNITTEST_ASSERT_ERROR(!eq(cl[3],1.2345), {}, "mosek", "set col lower");
      mosekSi.setColLower( 3, 1.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cl[3],1.2345), {}, "mosek", "set col lower");
      
      OSIUNITTEST_ASSERT_ERROR(!eq(cu[4],10.2345), {}, "mosek", "set col upper");
      mosekSi.setColUpper( 4, 10.2345 );
      OSIUNITTEST_ASSERT_ERROR( eq(cu[4],10.2345), {}, "mosek", "set col upper");

      OSIUNITTEST_ASSERT_ERROR(eq(mosekSi.getObjCoefficients()[0], 1.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(mosekSi.getObjCoefficients()[1], 0.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(mosekSi.getObjCoefficients()[2], 0.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(mosekSi.getObjCoefficients()[3], 0.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(mosekSi.getObjCoefficients()[4], 2.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(mosekSi.getObjCoefficients()[5], 0.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(mosekSi.getObjCoefficients()[6], 0.0), {}, "mosek", "read and copy exmip1");
      OSIUNITTEST_ASSERT_ERROR(eq(mosekSi.getObjCoefficients()[7],-1.0), {}, "mosek", "read and copy exmip1");
    }

    // Test getMatrixByRow method
    { 
      const OsiMskSolverInterface si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();
      
      OSIUNITTEST_ASSERT_ERROR(smP->getMajorDim()    ==  5, return, "mosek", "getMatrixByRow: major dim");
      OSIUNITTEST_ASSERT_ERROR(smP->getNumElements() == 14, return, "mosek", "getMatrixByRow: num elements");

      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[0],   3.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[1],   1.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[2],  -2.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[3],  -1.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[4],  -1.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[5],   2.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[6],   1.1), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[7],   1.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[8],   1.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[9],   2.8), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10], -1.2), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11],  5.6), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12],  1.0), {}, "mosek", "getMatrixByRow: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13],  1.9), {}, "mosek", "getMatrixByRow: elements");
      
      const int * mi = smP->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "mosek", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "mosek", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "mosek", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "mosek", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "mosek", "getMatrixByRow: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "mosek", "getMatrixByRow: vector starts");
      
      const int * ei = smP->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "mosek", "getMatrixByRow: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "mosek", "getMatrixByRow: indices");
    }
    //--------------
    // Test rowsense, rhs, rowrange, getMatrixByRow
    {
      OsiMskSolverInterface lhs;
      {     
        OsiMskSolverInterface siC1(m);
        OSIUNITTEST_ASSERT_WARNING(siC1.obj_ == NULL, {}, "mosek", "objective");
        OSIUNITTEST_ASSERT_WARNING(siC1.collower_ == NULL, {}, "mosek", "col lower");
        OSIUNITTEST_ASSERT_WARNING(siC1.colupper_ == NULL, {}, "mosek", "col upper");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowrange_ == NULL, {}, "mosek", "row range");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsense_ == NULL, {}, "mosek", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowlower_ == NULL, {}, "mosek", "row lower");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowupper_ == NULL, {}, "mosek", "row upper");
        OSIUNITTEST_ASSERT_WARNING(siC1.rhs_ == NULL, {}, "mosek", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1.matrixByRow_ == NULL, {}, "mosek", "matrix by row");
        OSIUNITTEST_ASSERT_WARNING(siC1.colsol_ == NULL, {}, "mosek", "col solution");
        OSIUNITTEST_ASSERT_WARNING(siC1.rowsol_ == NULL, {}, "mosek", "row solution");

        const char   * siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "mosek", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "mosek", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "mosek", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "mosek", "row sense");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "mosek", "row sense");
        
        const double * siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "mosek", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "mosek", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "mosek", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "mosek", "right hand side");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "mosek", "right hand side");
        
        const double * siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "mosek", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "mosek", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "mosek", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "mosek", "row range");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "mosek", "row range");
        
        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        OSIUNITTEST_ASSERT_ERROR(siC1mbr != NULL, {}, "mosek", "matrix by row");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getMajorDim()    ==  5, return, "mosek", "matrix by row: major dim");
        OSIUNITTEST_ASSERT_ERROR(siC1mbr->getNumElements() == 14, return, "mosek", "matrix by row: num elements");
        
        const double * ev = siC1mbr->getElements();
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "mosek", "matrix by row: elements");
        OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "mosek", "matrix by row: elements");

        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "mosek", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "mosek", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "mosek", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "mosek", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "mosek", "matrix by row: vector starts");
        OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "mosek", "matrix by row: vector starts");
        
        const int * ei = siC1mbr->getIndices();
        OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "mosek", "matrix by row: indices");
        OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "mosek", "matrix by row: indices");

        OSIUNITTEST_ASSERT_WARNING(siC1rs  == siC1.getRowSense(), {}, "mosek", "row sense");
        OSIUNITTEST_ASSERT_WARNING(siC1rhs == siC1.getRightHandSide(), {}, "mosek", "right hand side");
        OSIUNITTEST_ASSERT_WARNING(siC1rr  == siC1.getRowRange(), {}, "mosek", "row range");

        // Change MOSEK Model by adding free row
        OsiRowCut rc;
        rc.setLb(-COIN_DBL_MAX);
        rc.setUb( COIN_DBL_MAX);
        OsiCuts cuts;
        cuts.insert(rc);
        siC1.applyCuts(cuts);

        // Since model was changed, test that cached data is now freed.
        OSIUNITTEST_ASSERT_ERROR(siC1.obj_ == NULL, {}, "mosek", "objective");
        OSIUNITTEST_ASSERT_ERROR(siC1.collower_ == NULL, {}, "mosek", "col lower");
        OSIUNITTEST_ASSERT_ERROR(siC1.colupper_ == NULL, {}, "mosek", "col upper");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowrange_ == NULL, {}, "mosek", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsense_ == NULL, {}, "mosek", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowlower_ == NULL, {}, "mosek", "row lower");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowupper_ == NULL, {}, "mosek", "row upper");
        OSIUNITTEST_ASSERT_ERROR(siC1.rhs_ == NULL, {}, "mosek", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByRow_ == NULL, {}, "mosek", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.matrixByCol_ == NULL, {}, "mosek", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.colsol_ == NULL, {}, "mosek", "free cached data after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1.rowsol_ == NULL, {}, "mosek", "free cached data after adding row");

        siC1rs  = siC1.getRowSense();
        OSIUNITTEST_ASSERT_ERROR(siC1rs[0] == 'G', {}, "mosek", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[1] == 'L', {}, "mosek", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[2] == 'E', {}, "mosek", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[3] == 'R', {}, "mosek", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[4] == 'R', {}, "mosek", "row sense after adding row");
        OSIUNITTEST_ASSERT_ERROR(siC1rs[5] == 'N', {}, "mosek", "row sense after adding row");

        siC1rhs = siC1.getRightHandSide();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[0],2.5), {}, "mosek", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[1],2.1), {}, "mosek", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[2],4.0), {}, "mosek", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[3],5.0), {}, "mosek", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[4],15.), {}, "mosek", "right hand side after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rhs[5],0.0), {}, "mosek", "right hand side after adding row");

        siC1rr  = siC1.getRowRange();
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[0],0.0), {}, "mosek", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[1],0.0), {}, "mosek", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[2],0.0), {}, "mosek", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[3],5.0-1.8), {}, "mosek", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[4],15.0-3.0), {}, "mosek", "row range after adding row");
        OSIUNITTEST_ASSERT_ERROR(eq(siC1rr[5],0.0), {}, "mosek", "row range after adding row");
    
        lhs = siC1;
      }
      // Test that lhs has correct values even though siC1 has gone out of scope    
      OSIUNITTEST_ASSERT_ERROR(lhs.obj_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.collower_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.colupper_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowrange_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsense_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowlower_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowupper_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rhs_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByRow_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.matrixByCol_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.colsol_ == NULL, {}, "mosek", "freed origin after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhs.rowsol_ == NULL, {}, "mosek", "freed origin after assignment");

      const char * lhsrs  = lhs.getRowSense();
      OSIUNITTEST_ASSERT_ERROR(lhsrs[0] == 'G', {}, "mosek", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[1] == 'L', {}, "mosek", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[2] == 'E', {}, "mosek", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[3] == 'R', {}, "mosek", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[4] == 'R', {}, "mosek", "row sense after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsrs[5] == 'N', {}, "mosek", "row sense after assignment");
      
      const double * lhsrhs = lhs.getRightHandSide();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[0],2.5), {}, "mosek", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[1],2.1), {}, "mosek", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[2],4.0), {}, "mosek", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[3],5.0), {}, "mosek", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[4],15.), {}, "mosek", "right hand side after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrhs[5],0.0), {}, "mosek", "right hand side after assignment");
      
      const double *lhsrr = lhs.getRowRange();
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[0],0.0), {}, "mosek", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[1],0.0), {}, "mosek", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[2],0.0), {}, "mosek", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[3],5.0-1.8), {}, "mosek", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[4],15.0-3.0), {}, "mosek", "row range after assignment");
      OSIUNITTEST_ASSERT_ERROR(eq(lhsrr[5],0.0), {}, "mosek", "row range after assignment");
      
      const CoinPackedMatrix * lhsmbr = lhs.getMatrixByRow();
      OSIUNITTEST_ASSERT_ERROR(lhsmbr != NULL, {}, "mosek", "matrix by row after assignment");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getMajorDim()    ==  6, return, "mosek", "matrix by row after assignment: major dim");
      OSIUNITTEST_ASSERT_ERROR(lhsmbr->getNumElements() == 14, return, "mosek", "matrix by row after assignment: num elements");

      const double * ev = lhsmbr->getElements();
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 0], 3.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 1], 1.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 2],-2.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 3],-1.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 4],-1.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 5], 2.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 6], 1.1), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 7], 1.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 8], 1.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[ 9], 2.8), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[10],-1.2), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[11], 5.6), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[12], 1.0), {}, "mosek", "matrix by row after assignment: elements");
      OSIUNITTEST_ASSERT_ERROR(eq(ev[13], 1.9), {}, "mosek", "matrix by row after assignment: elements");
      
      const CoinBigIndex * mi = lhsmbr->getVectorStarts();
      OSIUNITTEST_ASSERT_ERROR(mi[0] ==  0, {}, "mosek", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[1] ==  5, {}, "mosek", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[2] ==  7, {}, "mosek", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[3] ==  9, {}, "mosek", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[4] == 11, {}, "mosek", "matrix by row after assignment: vector starts");
      OSIUNITTEST_ASSERT_ERROR(mi[5] == 14, {}, "mosek", "matrix by row after assignment: vector starts");
      
      const int * ei = lhsmbr->getIndices();
      OSIUNITTEST_ASSERT_ERROR(ei[ 0] == 0, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 1] == 1, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 2] == 3, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 3] == 4, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 4] == 7, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 5] == 1, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 6] == 2, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 7] == 2, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 8] == 5, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[ 9] == 3, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[10] == 6, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[11] == 0, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[12] == 4, {}, "mosek", "matrix by row after assignment: indices");
      OSIUNITTEST_ASSERT_ERROR(ei[13] == 7, {}, "mosek", "matrix by row after assignment: indices");
    }
    OSIUNITTEST_ASSERT_ERROR(OsiMskSolverInterface::getNumInstances() == numInstancesStart+1, {}, "mosek", "number of instances");
  }
  OSIUNITTEST_ASSERT_ERROR(OsiMskSolverInterface::getNumInstances() == numInstancesStart, {}, "mosek", "number of instances at finish");

  // Do common solverInterface testing by calling the
  // base class testing method.
  {
    OsiMskSolverInterface m;
    OsiSolverInterfaceCommonUnitTest(&m, mpsDir, netlibDir);
  }
}
#endif
