// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include "OsiUnitTests.hpp"

#include "OsiRowCutDebugger.hpp"

//--------------------------------------------------------------------------
// test cut debugger methods.
void
OsiRowCutDebuggerUnitTest(const OsiSolverInterface * baseSiP, const std::string & mpsDir)
{
  
  CoinRelFltEq eq;
  
  // Test default constructor
  {
    OsiRowCutDebugger r;
    OSIUNITTEST_ASSERT_ERROR(r.integerVariable_ == NULL, {}, "osirowcutdebugger", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.knownSolution_   == NULL, {}, "osirowcutdebugger", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.numberColumns_   == 0,    {}, "osirowcutdebugger", "default constructor");
  }
  
  {
    // Get non trivial instance
    OsiSolverInterface * imP = baseSiP->clone();
    std::string fn = mpsDir+"exmip1";
    imP->readMps(fn.c_str(),"mps");
    OSIUNITTEST_ASSERT_ERROR(imP->getNumRows() == 5, {}, "osirowcutdebugger", "read exmip1");
    
    /*
      Activate the debugger. The garbled name here is deliberate; the
      debugger should isolate the portion of the string between '/' and
      '.' (in normal use, this would be the base file name, stripped of
      the prefix and extension).
    */
    imP->activateRowCutDebugger("ab cd /x/ /exmip1.asc");
    
    int i;
    
    // return debugger
    const OsiRowCutDebugger * debugger = imP->getRowCutDebugger();
    OSIUNITTEST_ASSERT_ERROR(debugger != NULL, {}, "osirowcutdebugger", "return debugger");
    OSIUNITTEST_ASSERT_ERROR(debugger->numberColumns_ == 8, {}, "osirowcutdebugger", "return debugger");
    
    const bool type[]={0,0,1,1,0,0,0,0};
    const double values[]= {2.5, 0, 1, 1, 0.5, 3, 0, 0.26315789473684253};
    CoinPackedVector objCoefs(8,imP->getObjCoefficients());
   
    bool type_ok = true;
#if 0
    for (i=0;i<8;i++)
      type_ok &= type[i] == debugger->integerVariable_[i];
    OSIUNITTEST_ASSERT_ERROR(type_ok, {}, "osirowcutdebugger", "???");
#endif
    
    double objValue = objCoefs.dotProduct(values);
    double debuggerObjValue = objCoefs.dotProduct(debugger->knownSolution_);
    OSIUNITTEST_ASSERT_ERROR(eq(objValue, debuggerObjValue), {}, "osirowcutdebugger", "objective value");
    
    OsiRowCutDebugger rhs;
    {
      OsiRowCutDebugger rC1(*debugger);

      OSIUNITTEST_ASSERT_ERROR(rC1.numberColumns_ == 8, {}, "osirowcutdebugger", "copy constructor");
      type_ok = true;
      for (i=0;i<8;i++)
      	type_ok &= type[i] == rC1.integerVariable_[i];
      OSIUNITTEST_ASSERT_ERROR(type_ok, {}, "osirowcutdebugger", "copy constructor");
      OSIUNITTEST_ASSERT_ERROR(eq(objValue,objCoefs.dotProduct(rC1.knownSolution_)), {}, "osirowcutdebugger", "copy constructor");
      
      rhs = rC1;
      OSIUNITTEST_ASSERT_ERROR(rhs.numberColumns_ == 8, {}, "osirowcutdebugger", "assignment operator");
      type_ok = true;
      for (i=0;i<8;i++)
      	type_ok &= type[i] == rhs.integerVariable_[i];
      OSIUNITTEST_ASSERT_ERROR(type_ok, {}, "osirowcutdebugger", "assignment operator");
      OSIUNITTEST_ASSERT_ERROR(eq(objValue,objCoefs.dotProduct(rhs.knownSolution_)), {}, "osirowcutdebugger", "assignment operator");
    }
    // Test that rhs has correct values even though lhs has gone out of scope
    OSIUNITTEST_ASSERT_ERROR(rhs.numberColumns_ == 8, {}, "osirowcutdebugger", "assignment operator");
    type_ok = true;
    for (i=0;i<8;i++)
    	type_ok &= type[i] == rhs.integerVariable_[i];
    OSIUNITTEST_ASSERT_ERROR(type_ok, {}, "osirowcutdebugger", "assignment operator");
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,objCoefs.dotProduct(rhs.knownSolution_)), {}, "osirowcutdebugger", "assignment operator");

    OsiRowCut cut[2];
    
    const int ne = 3;
    int inx[ne] = { 0, 2, 3 };
    double el[ne] = { 1., 1., 1. };
    cut[0].setRow(ne,inx,el);
    cut[0].setUb(5.);
    
    el[1]=5;
    cut[1].setRow(ne,inx,el);
    cut[1].setUb(5);
    OsiCuts cs; 
    cs.insert(cut[0]);
    cs.insert(cut[1]);
    OSIUNITTEST_ASSERT_ERROR(!debugger->invalidCut(cut[0]), {}, "osirowcutdebugger", "recognize (in)valid cut");
    OSIUNITTEST_ASSERT_ERROR( debugger->invalidCut(cut[1]), {}, "osirowcutdebugger", "recognize (in)valid cut");
    OSIUNITTEST_ASSERT_ERROR(debugger->validateCuts(cs,0,2) == 1, {}, "osirowcutdebugger", "recognize (in)valid cut");
    OSIUNITTEST_ASSERT_ERROR(debugger->validateCuts(cs,0,1) == 0, {}, "osirowcutdebugger", "recognize (in)valid cut");
    delete imP;
  }
}
