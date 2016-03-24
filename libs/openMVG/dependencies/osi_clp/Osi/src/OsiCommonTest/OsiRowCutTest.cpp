// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cfloat>

#include "CoinPragma.hpp"

#include "OsiUnitTests.hpp"

#include "OsiRowCut.hpp"
#include "CoinFloatEqual.hpp"

//--------------------------------------------------------------------------
void
OsiRowCutUnitTest(const OsiSolverInterface * baseSiP,
		  const std::string & mpsDir)
{

  CoinRelFltEq eq;

  // Test default constructor
  {
    OsiRowCut r;
    OSIUNITTEST_ASSERT_ERROR(r.row_.getIndices()  == NULL, {}, "osirowcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.row_.getElements() == NULL, {}, "osirowcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.row_.getNumElements() == 0, {}, "osirowcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.lb_ == -COIN_DBL_MAX, {}, "osirowcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.ub_ ==  COIN_DBL_MAX, {}, "osirowcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.effectiveness() == 0.0, {}, "osirowcut", "default constructor");
  }

  // Test set and get methods
  const int ne = 4;
  int inx[ne] = { 1, 3, 4, 7 };
  double el[ne] = { 1.2, 3.4, 5.6, 7.8 };
  {
    OsiRowCut r;

    // Test setting getting bounds
    r.setLb(65.432);
    r.setUb(123.45);
    OSIUNITTEST_ASSERT_ERROR(r.lb() == 65.432, {}, "osirowcut", "set bounds");
    OSIUNITTEST_ASSERT_ERROR(r.ub() == 123.45, {}, "osirowcut", "set bounds");

    // Test setting/getting of effectiveness,timesUsed,timesTested
    r.setEffectiveness(45.);
    OSIUNITTEST_ASSERT_ERROR(r.effectiveness() == 45.0, {}, "osirowcut", "set effectivenesss");
    
    // Test setting/getting elements with int* & float* vectors
    r.setRow( ne, inx, el );
    OSIUNITTEST_ASSERT_ERROR(r.row().getNumElements() == ne, return, "osirowcut", "set row");
    bool row_ok = true;
    for ( int i=0; i<ne; i++ ) {
      row_ok &= r.row().getIndices()[i] == inx[i];
      row_ok &= r.row().getElements()[i] == el[i];
    }
    OSIUNITTEST_ASSERT_ERROR(row_ok, {}, "osirowcut", "set row");
  }

  // Repeat test with ownership constructor
  {
    int *inxo = new int[ne];
    double *elo = new double[ne];
    double lb = 65.432;
    double ub = 123.45;

    int i;
    for ( i = 0 ; i < ne ; i++) inxo[i] = inx[i] ;
    for ( i = 0 ; i < ne ; i++) elo[i] = el[i] ;
    OsiRowCut r(lb,ub,ne,ne,inxo,elo);

    OSIUNITTEST_ASSERT_ERROR(r.row().getNumElements() == ne, {}, "osirowcut", "ownership constructor");
    OSIUNITTEST_ASSERT_ERROR(r.effectiveness() == 0.0, {}, "osirowcut", "ownership constructor");

    // Test getting bounds
    OSIUNITTEST_ASSERT_ERROR(r.lb() == lb, {}, "osirowcut", "ownership constructor");
    OSIUNITTEST_ASSERT_ERROR(r.ub() == ub, {}, "osirowcut", "ownership constructor");

    // Test setting/getting of effectiveness
    r.setEffectiveness(45.);
    OSIUNITTEST_ASSERT_ERROR(r.effectiveness() == 45.0, {}, "osirowcut", "ownership constructor");
    
    // Test getting elements with int* & float* vectors
    OSIUNITTEST_ASSERT_ERROR(r.row().getNumElements() == ne, return, "osirowcut", "ownership constructor");
    bool row_ok = true;
    for ( int i=0; i<ne; i++ ) {
      row_ok &= r.row().getIndices()[i] == inx[i];
      row_ok &= r.row().getElements()[i] == el[i];
    }
    OSIUNITTEST_ASSERT_ERROR(row_ok, {}, "osirowcut", "ownership constructor");
  }

  // Test sense, rhs, range
  {
    {
      OsiRowCut r;
      OSIUNITTEST_ASSERT_ERROR(r.sense() == 'N', {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.rhs()   == 0.0, {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.range() == 0.0, {}, "osirowcut", "sense, rhs, range");
    }
    {
      OsiRowCut r;
      r.setLb(65.432);
      OSIUNITTEST_ASSERT_ERROR(r.sense() == 'G', {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.rhs()   == 65.432, {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.range() == 0.0, {}, "osirowcut", "sense, rhs, range");
    }
    {
      OsiRowCut r;
      r.setLb(65.432);
      r.setUb(65.432);
      OSIUNITTEST_ASSERT_ERROR(r.sense() == 'E', {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.rhs()   == 65.432, {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.range() == 0.0, {}, "osirowcut", "sense, rhs, range");
    }
    {
      OsiRowCut r;
      r.setUb(123.45);
      OSIUNITTEST_ASSERT_ERROR(r.sense() == 'L', {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.rhs()   == 123.45, {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.range() == 0.0, {}, "osirowcut", "sense, rhs, range");
    }
    {
      OsiRowCut r;
      r.setLb(65.432);
      r.setUb(123.45);
      OSIUNITTEST_ASSERT_ERROR(r.sense() == 'R', {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(r.rhs()   == 123.45, {}, "osirowcut", "sense, rhs, range");
      OSIUNITTEST_ASSERT_ERROR(eq(r.range(),123.45 - 65.432), {}, "osirowcut", "sense, rhs, range");
    }
  }
  
  // Test copy constructor and assignment operator
  {
    OsiRowCut rhs;
    {
      OsiRowCut r;
      OsiRowCut rC1(r);
      OSIUNITTEST_ASSERT_ERROR(rC1.row().getNumElements() == r.row().getNumElements(), {}, "osirowcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC1.row().getIndices()     == r.row().getIndices(), {}, "osirowcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC1.row().getElements()    == r.row().getElements(), {}, "osirowcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC1.lb()                   == r.lb(), {}, "osirowcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC1.ub()                   == r.ub(), {}, "osirowcut", "copy constructor");
      
      r.setLb(65.432);
      r.setUb(123.45);
      r.setRow( ne, inx, el );
      r.setEffectiveness(123.);

      OSIUNITTEST_ASSERT_ERROR(rC1.row().getNumElements() != r.row().getNumElements(), {}, "osirowcut", "modify copy");
      OSIUNITTEST_ASSERT_ERROR(rC1.row().getIndices()     != r.row().getIndices(), {}, "osirowcut", "modify copy");
      OSIUNITTEST_ASSERT_ERROR(rC1.row().getElements()    != r.row().getElements(), {}, "osirowcut", "modify copy");
      OSIUNITTEST_ASSERT_ERROR(rC1.lb()                   != r.lb(), {}, "osirowcut", "modify copy");
      OSIUNITTEST_ASSERT_ERROR(rC1.ub()                   != r.ub(), {}, "osirowcut", "modify copy");
      OSIUNITTEST_ASSERT_ERROR(rC1.effectiveness()        != r.effectiveness(), {}, "osirowcut", "modify copy");
      
      OsiRowCut rC2(r);
      OSIUNITTEST_ASSERT_ERROR(rC2.row().getNumElements() == r.row().getNumElements(), return, "osirowcut", "copy constructor");
      bool row_ok = true;
      for( int i=0; i<r.row().getNumElements(); i++ ){
        row_ok &= rC2.row().getIndices()[i]  == r.row().getIndices()[i];
        row_ok &= rC2.row().getElements()[i] == r.row().getElements()[i];
      }
      OSIUNITTEST_ASSERT_ERROR(row_ok, {}, "osirowcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC2.lb() == r.lb(), {}, "osirowcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC2.ub() == r.ub(), {}, "osirowcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC2.effectiveness() == r.effectiveness(), {}, "osirowcut", "copy constructor");
      
      rhs = rC2;
    }
    // Test that rhs has correct values even though lhs has gone out of scope
    OSIUNITTEST_ASSERT_ERROR(rhs.row().getNumElements() == ne, return, "osirowcut", "assignment operator");
    bool row_ok = true;
    for ( int i=0; i<ne; i++ ) {
      row_ok &= rhs.row().getIndices()[i] == inx[i];
      row_ok &= rhs.row().getElements()[i] == el[i];
    }
    OSIUNITTEST_ASSERT_ERROR(row_ok, {}, "osirowcut", "assignment operator");
    OSIUNITTEST_ASSERT_ERROR(rhs.effectiveness() == 123.0, {}, "osirowcut", "assignment operator");
    OSIUNITTEST_ASSERT_ERROR(rhs.lb() == 65.432, {}, "osirowcut", "assignment operator");
    OSIUNITTEST_ASSERT_ERROR(rhs.ub() == 123.45, {}, "osirowcut", "assignment operator");
  }

  // Test setting row with packed vector
  {
    CoinPackedVector r;
    r.setVector(ne,inx,el);

    OsiRowCut rc;
    OSIUNITTEST_ASSERT_ERROR(rc.row() != r, {}, "osirowcut", "setting row with packed vector");
    rc.setRow(r);
    OSIUNITTEST_ASSERT_ERROR(rc.row() == r, {}, "osirowcut", "setting row with packed vector");
  }

  // Test operator==
  {
    CoinPackedVector r;
    r.setVector(ne,inx,el);

    OsiRowCut rc;
    rc.setRow(r);
    rc.setEffectiveness(2.);
    rc.setLb(3.0);
    rc.setUb(4.0);

    {
      OsiRowCut c(rc);
      OSIUNITTEST_ASSERT_ERROR(  c == rc,  {}, "osirowcut", "operator ==");
      OSIUNITTEST_ASSERT_ERROR(!(c != rc), {}, "osirowcut", "operator !=");
      OSIUNITTEST_ASSERT_ERROR(!(c <  rc), {}, "osirowcut", "operator <");
      OSIUNITTEST_ASSERT_ERROR(!(rc <  c), {}, "osirowcut", "operator <");
      OSIUNITTEST_ASSERT_ERROR(!(c >  rc), {}, "osirowcut", "operator >");
      OSIUNITTEST_ASSERT_ERROR(!(rc >  c), {}, "osirowcut", "operator >");
    }
    {
      OsiRowCut c(rc);      
      const int ne1 = 4;
      int inx1[ne] = { 1, 3, 4, 7 };
      double el1[ne] = { 1.2, 3.4, 5.6, 7.9 };
      c.setRow(ne1,inx1,el1);
      OSIUNITTEST_ASSERT_ERROR(!(c == rc), {}, "osirowcut", "operator ==");
      OSIUNITTEST_ASSERT_ERROR(  c != rc , {}, "osirowcut", "operator !=");
      OSIUNITTEST_ASSERT_ERROR(!(rc <  c), {}, "osirowcut", "operator <");
    }
    {
      OsiRowCut c(rc); 
      c.setEffectiveness(3.);
      OSIUNITTEST_ASSERT_ERROR(!(c == rc), {}, "osirowcut", "operator ==");
      OSIUNITTEST_ASSERT_ERROR(  c != rc , {}, "osirowcut", "operator !=");
      OSIUNITTEST_ASSERT_ERROR(!(c <  rc), {}, "osirowcut", "operator <");
      OSIUNITTEST_ASSERT_ERROR( (rc <  c), {}, "osirowcut", "operator <");
      OSIUNITTEST_ASSERT_ERROR( (c >  rc), {}, "osirowcut", "operator >");
      OSIUNITTEST_ASSERT_ERROR(!(rc >  c), {}, "osirowcut", "operator >");
    }
    {
      OsiRowCut c(rc); 
      c.setLb(4.0);
      OSIUNITTEST_ASSERT_ERROR(!(c == rc), {}, "osirowcut", "operator ==");
      OSIUNITTEST_ASSERT_ERROR(  c != rc , {}, "osirowcut", "operator !=");
    }
    {
      OsiRowCut c(rc); 
      c.setUb(5.0);
      OSIUNITTEST_ASSERT_ERROR(!(c == rc), {}, "osirowcut", "operator ==");
      OSIUNITTEST_ASSERT_ERROR(  c != rc , {}, "osirowcut", "operator !=");
    }
  }
#ifndef COIN_NOTEST_DUPLICATE
  {
    // Test consistent
    OsiSolverInterface * imP = baseSiP->clone();
    std::string fn = mpsDir+"exmip1";
    OSIUNITTEST_ASSERT_ERROR(imP->readMps(fn.c_str(),"mps") == 0, {}, "osirowcut", "read MPS");

    OsiRowCut c;      
    const int ne = 3;
    int inx[ne] = { -1, 5, 4 };
    double el[ne] = { 1., 1., 1. };
    c.setRow(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR(!c.consistent(), {}, "osirowcut", "consistent");
    
    inx[0]=5;
#if 0
    c.setRow(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR(!c.consistent(), {}, "osirowcut", "consistent");
#else
    bool errorThrown = false;
    try {
       c.setRow(ne,inx,el);
    }
    catch (CoinError e) {
       errorThrown = true;
    }
    OSIUNITTEST_ASSERT_ERROR(errorThrown == true, {}, "osirowcut", "duplicate entries");
#endif
    
    inx[0]=3;
    c.setRow(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR(c.consistent(), {}, "osirowcut", "consistent");
    
    c.setLb(5.);
    c.setUb(5.);
    OSIUNITTEST_ASSERT_ERROR(c.consistent(), {}, "osirowcut", "consistent");

    c.setLb(5.5);
    OSIUNITTEST_ASSERT_ERROR(c.consistent(), {}, "osirowcut", "consistent");
    OSIUNITTEST_ASSERT_ERROR(c.infeasible(*imP), {}, "osirowcut", "infeasible");
    delete imP;
  }
#endif
  {
    // Test consistent(IntegerModel) method.
    OsiSolverInterface * imP = baseSiP->clone();
    std::string fn = mpsDir+"exmip1";
    OSIUNITTEST_ASSERT_ERROR(imP->readMps(fn.c_str(),"mps") == 0, {}, "osirowcut", "read MPS");
    
    OsiRowCut cut;    
    const int ne = 3;
    int inx[ne] = { 3, 5, 4 };
    double el[ne] = { 1., 1., 1. };
    cut.setRow(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR(cut.consistent(), {}, "osirowcut", "consistent");
    
    inx[0]=7;
    cut.setRow(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR(cut.consistent(*imP), {}, "osirowcut", "consistent(IntegerModel)");
    
    inx[0]=8;
    cut.setRow(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR( cut.consistent(), {}, "osirowcut", "consistent(IntegerModel)");
    OSIUNITTEST_ASSERT_ERROR(!cut.consistent(*imP), {}, "osirowcut", "consistent(IntegerModel)");
    delete imP;
  }
  {
    //Test infeasible(im) method
    OsiSolverInterface * imP = baseSiP->clone();
    std::string fn = mpsDir+"exmip1";
    OSIUNITTEST_ASSERT_ERROR(imP->readMps(fn.c_str(),"mps") == 0, {}, "osirowcut", "read MPS");
    
    OsiRowCut cut;   
    const int ne = 3;
    int inx[ne] = { 3, 5, 4 };
    double el[ne] = { 1., 1., 1. };
    cut.setRow(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR(!cut.infeasible(*imP), {}, "osirowcut", "infeasible(IntegerModel)");
    
    OsiRowCut c1;
    OSIUNITTEST_ASSERT_ERROR(!c1.infeasible(*imP), {}, "osirowcut", "infeasible(IntegerModel)");
    OSIUNITTEST_ASSERT_ERROR( c1.consistent(), {}, "osirowcut", "consistent");
    OSIUNITTEST_ASSERT_ERROR( c1.consistent(*imP), {}, "osirowcut", "consistent(IntegerModel)");
    delete imP;
  }
  {
    //Test violation

    double solution[]={1.0,1.0,1.0,2.0,2.0,2.0};
    OsiRowCut cut;   
    const int ne = 3;
    int inx[ne] = { 3, 5, 4 };
    double el[ne] = { 1., 1., 1. };
    cut.setRow(ne,inx,el);
    cut.setUb(5.);
    OSIUNITTEST_ASSERT_ERROR(cut.violated(solution), {}, "osirowcut", "violation");

    cut.setLb(5.);
    cut.setUb(10.);
    OSIUNITTEST_ASSERT_ERROR(!cut.violated(solution), {}, "osirowcut", "violation");

    cut.setLb(6.1);
    OSIUNITTEST_ASSERT_ERROR( cut.violated(solution), {}, "osirowcut", "violation");
  }
}
