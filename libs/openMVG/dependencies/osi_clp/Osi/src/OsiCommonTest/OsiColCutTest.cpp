// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

//#include <cstdlib>
//#include <cassert>
//#include <cmath>

#include "CoinPragma.hpp"

#include "OsiUnitTests.hpp"

#include "OsiColCut.hpp"

//--------------------------------------------------------------------------
void
OsiColCutUnitTest(const OsiSolverInterface * baseSiP, const std::string & mpsDir)
{

  // Test default constructor
  {
    OsiColCut r;
    OSIUNITTEST_ASSERT_ERROR(r.lbs_.getIndices()     == NULL, {}, "osicolcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.lbs_.getElements()    == NULL, {}, "osicolcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.lbs_.getNumElements() == 0,    {}, "osicolcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.lbs().getNumElements() == 0,   {}, "osicolcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.ubs_.getIndices()     == NULL, {}, "osicolcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.ubs_.getElements()    == NULL, {}, "osicolcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.ubs_.getNumElements() == 0,    {}, "osicolcut", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.ubs().getNumElements() == 0,   {}, "osicolcut", "default constructor");
  }

  // Test set and get methods
  const int ne = 4;
  int inx[ne] = { 1, 3, 4, 7 };
  double el[ne] = { 1.2, 3.4, 5.6, 7.8 };
  const int ne3 = 0;
  int * inx3=NULL;
  double * el3= NULL;
  {
    OsiColCut r;    
        
    // Test setting/getting bounds
    r.setLbs( ne, inx, el );
    r.setEffectiveness(222.);
    OSIUNITTEST_ASSERT_ERROR(r.lbs().getNumElements() == ne, return, "osicolcut", "setting bounds");
    bool bounds_ok = true;
    for ( int i=0; i<ne; i++ ) {
    	bounds_ok &= r.lbs().getIndices()[i] == inx[i];
    	bounds_ok &= r.lbs().getElements()[i] == el[i];
    }
    OSIUNITTEST_ASSERT_ERROR(bounds_ok, {}, "osicolcut", "setting bounds");
    OSIUNITTEST_ASSERT_ERROR(r.effectiveness() == 222.0, {}, "osicolcut", "setting bounds");
    
    r.setUbs( ne3, inx3, el3 );
    OSIUNITTEST_ASSERT_ERROR(r.ubs().getNumElements() == 0, {}, "osicolcut", "setting bounds");
    OSIUNITTEST_ASSERT_ERROR(r.ubs().getIndices()  == NULL, {}, "osicolcut", "setting bounds");
    OSIUNITTEST_ASSERT_ERROR(r.ubs().getElements() == NULL, {}, "osicolcut", "setting bounds");
  }
  
  // Test copy constructor and assignment operator
  {
    OsiColCut rhs;
    {
      OsiColCut r;
      OsiColCut rC1(r);
      OSIUNITTEST_ASSERT_ERROR(rC1.lbs().getNumElements() == r.lbs().getNumElements(), {}, "osicolcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC1.ubs().getNumElements() == r.ubs().getNumElements(), {}, "osicolcut", "copy constructor");
  
      r.setLbs( ne, inx, el );
      r.setUbs( ne, inx, el );
      r.setEffectiveness(121.);

      OSIUNITTEST_ASSERT_ERROR(rC1.lbs().getNumElements() != r.lbs().getNumElements(), {}, "osicolcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC1.ubs().getNumElements() != r.lbs().getNumElements(), {}, "osicolcut", "copy constructor");
                
      OsiColCut rC2(r);
      OSIUNITTEST_ASSERT_ERROR(rC2.lbs().getNumElements() == r.lbs().getNumElements(), {}, "osicolcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC2.ubs().getNumElements() == r.ubs().getNumElements(), {}, "osicolcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC2.lbs().getNumElements() == ne, return, "osicolcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC2.ubs().getNumElements() == ne, return, "osicolcut", "copy constructor");
      bool bounds_ok = true;
      for ( int i=0; i<ne; i++ ) {
        bounds_ok &= rC2.lbs().getIndices()[i] == inx[i];
        bounds_ok &= rC2.lbs().getElements()[i] == el[i];
        bounds_ok &= rC2.ubs().getIndices()[i] == inx[i];
        bounds_ok &= rC2.ubs().getElements()[i] == el[i];
      }
      OSIUNITTEST_ASSERT_ERROR(bounds_ok, {}, "osicolcut", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(rC2.effectiveness() == 121.0, {}, "osicolcut", "copy constructor");
      
      rhs = rC2;
    }
    // Test that rhs has correct values even though lhs has gone out of scope
    OSIUNITTEST_ASSERT_ERROR(rhs.lbs().getNumElements() == ne, return, "osicolcut", "assignment operator");
    OSIUNITTEST_ASSERT_ERROR(rhs.ubs().getNumElements() == ne, return, "osicolcut", "assignment operator");
    bool bounds_ok = true;
    for ( int i=0; i<ne; i++ ) {
    	bounds_ok &= rhs.lbs().getIndices()[i] == inx[i];
    	bounds_ok &= rhs.lbs().getElements()[i] == el[i];
    	bounds_ok &= rhs.ubs().getIndices()[i] == inx[i];
    	bounds_ok &= rhs.ubs().getElements()[i] == el[i];
    }  
    OSIUNITTEST_ASSERT_ERROR(bounds_ok, {}, "osicolcut", "assignment operator");
    OSIUNITTEST_ASSERT_ERROR(rhs.effectiveness() == 121.0, {}, "osicolcut", "assignment operator");
  }

  // Test setting bounds with packed vector and operator==
  {    
    const int ne1 = 4;
    int inx1[ne] = { 1, 3, 4, 7 };
    double el1[ne] = { 1.2, 3.4, 5.6, 7.8 };
    const int ne2 = 2;
    int inx2[ne2]={ 1, 3 };
    double el2[ne2]= { 1.2, 3.4 };
    CoinPackedVector v1,v2;
    v1.setVector(ne1,inx1,el1);
    v2.setVector(ne2,inx2,el2);
    
    OsiColCut c1,c2;
    OSIUNITTEST_ASSERT_ERROR(  c1 == c2 , {}, "osicolcut", "setting bounds with packed vector and operator ==");
    OSIUNITTEST_ASSERT_ERROR(!(c1 != c2), {}, "osicolcut", "setting bounds with packed vector and operator !=");
    
    c1.setLbs(v1);
    OSIUNITTEST_ASSERT_ERROR(  c1 != c2 , {}, "osicolcut", "setting bounds with packed vector and operator !=");
    OSIUNITTEST_ASSERT_ERROR(!(c1 == c2), {}, "osicolcut", "setting bounds with packed vector and operator ==");
    OSIUNITTEST_ASSERT_ERROR(c1.lbs() == v1, {}, "osicolcut", "setting bounds with packed vector and operator !=");
    
    c1.setUbs(v2);
    OSIUNITTEST_ASSERT_ERROR(c1.ubs() == v2, {}, "osicolcut", "setting bounds with packed vector and operator !=");
    c1.setEffectiveness(3.);
    OSIUNITTEST_ASSERT_ERROR(c1.effectiveness() == 3.0, {}, "osicolcut", "setting bounds with packed vector and operator !=");

    {
      OsiColCut c3(c1);
      OSIUNITTEST_ASSERT_ERROR(  c3 == c1 , {}, "osicolcut", "operator ==");
      OSIUNITTEST_ASSERT_ERROR(!(c3 != c1), {}, "osicolcut", "operator !=");
    }
    {
      OsiColCut c3(c1);
      c3.setLbs(v2);
      OSIUNITTEST_ASSERT_ERROR(  c3 != c1 , {}, "osicolcut", "operator !=");
      OSIUNITTEST_ASSERT_ERROR(!(c3 == c1), {}, "osicolcut", "operator ==");
    }
    {
      OsiColCut c3(c1);
      c3.setUbs(v1);
      OSIUNITTEST_ASSERT_ERROR(  c3 != c1 , {}, "osicolcut", "operator !=");
      OSIUNITTEST_ASSERT_ERROR(!(c3 == c1), {}, "osicolcut", "operator ==");
    }
    {
      OsiColCut c3(c1);
      c3.setEffectiveness(5.);
      OSIUNITTEST_ASSERT_ERROR(  c3 != c1 , {}, "osicolcut", "operator !=");
      OSIUNITTEST_ASSERT_ERROR(!(c3 == c1), {}, "osicolcut", "operator ==");
    }
  }

  // internal consistency 
  {
    const int ne = 1;
    int inx[ne] = { -3 };
    double el[ne] = { 1.2 };
    OsiColCut r; 
    r.setLbs( ne, inx, el );
    OSIUNITTEST_ASSERT_ERROR(!r.consistent(), {}, "osicolcut", "consistent");
  }
  {
    const int ne = 1;
    int inx[ne] = { -3 };
    double el[ne] = { 1.2 };
    OsiColCut r; 
    r.setUbs( ne, inx, el );
    OSIUNITTEST_ASSERT_ERROR(!r.consistent(), {}, "osicolcut", "consistent");
  }
  {
    const int ne = 1;
    int inx[ne] = { 100 };
    double el[ne] = { 1.2 };
    const int ne1 = 2;
    int inx1[ne1] = { 50, 100 };
    double el1[ne1] = { 100., 100. };
    OsiColCut r; 
    r.setUbs( ne, inx, el );
    r.setLbs( ne1, inx1, el1 );
    OSIUNITTEST_ASSERT_ERROR(r.consistent(), {}, "osicolcut", "consistent");

    OsiSolverInterface * imP = baseSiP->clone();
    std::string fn = mpsDir+"exmip1";
    imP->readMps(fn.c_str(),"mps");
    OSIUNITTEST_ASSERT_ERROR(!r.consistent(*imP), {}, "osicolcut", "consistent");
    delete imP;
  }
  {
    const int ne = 1;
    int inx[ne] = { 100 };
    double el[ne] = { 1.2 };
    const int ne1 = 2;
    int inx1[ne1] = { 50, 100 };
    double el1[ne1] = { 100., 1. };
    OsiColCut r; 
    r.setUbs( ne, inx, el );
    r.setLbs( ne1, inx1, el1 );
    OSIUNITTEST_ASSERT_ERROR(r.consistent(), {}, "osicolcut", "consistent");
  }
  {
    // Test consistent(IntegerModel) method.
    OsiSolverInterface * imP = baseSiP->clone();
    std::string fn = mpsDir+"exmip1";
    imP->readMps(fn.c_str(),"mps");
    
    OsiColCut cut;
    const int ne=1;
    int inx[ne]={20};
    double el[ne]={0.25};
    cut.setLbs(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR(!cut.consistent(*imP), {}, "osicolcut", "consistent(IntegerModel)");
    
    cut.setLbs(0,NULL,NULL);
    cut.setUbs(ne,inx,el); 
    OSIUNITTEST_ASSERT_ERROR(!cut.consistent(*imP), {}, "osicolcut", "consistent(IntegerModel)");
    
    inx[0]=4;
    cut.setLbs(ne,inx,el);
    cut.setUbs(0,NULL,NULL); 
    OSIUNITTEST_ASSERT_ERROR( cut.consistent(*imP), {}, "osicolcut", "consistent(IntegerModel)");
    
    el[0]=4.5;
    cut.setLbs(0,NULL,NULL);
    cut.setUbs(ne,inx,el); 
    OSIUNITTEST_ASSERT_ERROR( cut.consistent(*imP), {}, "osicolcut", "consistent(IntegerModel)");

    cut.setLbs(ne,inx,el);
    cut.setUbs(0,NULL,NULL); 
    OSIUNITTEST_ASSERT_ERROR( cut.consistent(*imP), {}, "osicolcut", "consistent(IntegerModel)");
    OSIUNITTEST_ASSERT_ERROR( cut.infeasible(*imP), {}, "osicolcut", "infeasible(IntegerModel)");
    
    el[0]=3.0;
    cut.setLbs(ne,inx,el);
    cut.setUbs(ne,inx,el); 
    OSIUNITTEST_ASSERT_ERROR( cut.consistent(*imP), {}, "osicolcut", "consistent(IntegerModel)");
    delete imP;
  }
  {
    //Test infeasible(im) method
    // Test consistent(IntegerModel) method.
    OsiSolverInterface * imP = baseSiP->clone();
    std::string fn = mpsDir+"exmip1";
    imP->readMps(fn.c_str(),"mps");
    
    OsiColCut cut;
    const int ne=1;
    int inx[ne]={4};
    double el[ne]={4.5};
    cut.setLbs(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR( cut.infeasible(*imP), {}, "osicolcut", "infeasible(IntegerModel)");

    el[0]=0.25;
    cut.setLbs(0,NULL,NULL);
    cut.setUbs(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR( cut.infeasible(*imP), {}, "osicolcut", "infeasible(IntegerModel)");
    
    el[0]=3.0;
    cut.setLbs(ne,inx,el);
    cut.setUbs(ne,inx,el); 
    OSIUNITTEST_ASSERT_ERROR(!cut.infeasible(*imP), {}, "osicolcut", "infeasible(IntegerModel)");

    delete imP;
  }
  {
    //Test violation

    double solution[]={1.0};
    OsiColCut cut;
    const int ne=1;
    int inx[ne]={0};
    double el[ne]={4.5};
    cut.setLbs(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR( cut.violated(solution), {}, "osicolcut", "violated");

    el[0]=0.25;
    cut.setLbs(0,NULL,NULL);
    cut.setUbs(ne,inx,el);
    OSIUNITTEST_ASSERT_ERROR( cut.violated(solution), {}, "osicolcut", "violated");
    
    el[0]=1.0;
    cut.setLbs(ne,inx,el);
    cut.setUbs(ne,inx,el); 
    OSIUNITTEST_ASSERT_ERROR(!cut.violated(solution), {}, "osicolcut", "violated");
  }
}
