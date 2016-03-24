// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include "OsiUnitTests.hpp"

#include "OsiCuts.hpp"

//--------------------------------------------------------------------------
void
OsiCutsUnitTest()
{
  CoinRelFltEq eq;
  // Test default constructor
  {
    OsiCuts r;
    OSIUNITTEST_ASSERT_ERROR(r.colCutPtrs_.empty(), {}, "osicuts", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.rowCutPtrs_.empty(), {}, "osicuts", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.sizeColCuts() == 0, {}, "osicuts", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.sizeRowCuts() == 0, {}, "osicuts", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.sizeCuts()    == 0, {}, "osicuts", "default constructor");
    OSIUNITTEST_ASSERT_ERROR(r.mostEffectiveCutPtr() == NULL, {}, "osicuts", "default constructor");
  }
  
  // Create some cuts
  OsiRowCut rcv[5];
  OsiColCut ccv[5];
  OsiCuts cuts;
  int i;
  for ( i=0; i<5; i++ ) {
    rcv[i].setEffectiveness(100.+i);
    ccv[i].setEffectiveness(200.+i);
    cuts.insert(rcv[i]);
    cuts.insert(ccv[i]);
  }
  
  OsiCuts rhs;
  {
    OsiCuts cs;
    
    // test inserting & accessing cut
    for ( i = 0; i < 5; i++ ) {
      OSIUNITTEST_ASSERT_ERROR(cs.sizeRowCuts() == i, {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(cs.sizeCuts() == 2*i, {}, "osicuts", "inserting and accessing cuts");
      cs.insert(rcv[i]);
      OSIUNITTEST_ASSERT_ERROR(cs.sizeRowCuts() == i+1, {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(cs.sizeCuts() == 2*i+1, {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(cs.rowCut(i) == rcv[i], {}, "osicuts", "inserting and accessing cuts");
#if 0
      const OsiCut * cp = cs.cutPtr(2*i);
      OSIUNITTEST_ASSERT_ERROR(cs.rowCut(i).effectiveness() == cp->effectiveness(), {}, "osicuts", "inserting and accessing cuts");
      const OsiRowCut * rcP = dynamic_cast<const OsiRowCut*>( cp );
      OSIUNITTEST_ASSERT_ERROR(rcP != NULL, {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(*rcP == rcv[i], {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(dynamic_cast<const OsiColCut*>(cs.cutPtr(2*i)) == NULL, {}, "osicuts", "inserting and accessing cuts");
#endif
      
      OSIUNITTEST_ASSERT_ERROR(cs.sizeColCuts() == i, {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(cs.sizeCuts() == 2*i+1, {}, "osicuts", "inserting and accessing cuts");
      cs.insert(ccv[i]);
      OSIUNITTEST_ASSERT_ERROR(cs.sizeColCuts() == i+1, {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(cs.sizeCuts() == 2*i+2, {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(cs.colCut(i) == ccv[i], {}, "osicuts", "inserting and accessing cuts");
#if 0
      OSIUNITTEST_ASSERT_ERROR(dynamic_cast<const OsiRowCut*>(cs.cutPtr(2*i+1)) == NULL, {}, "osicuts", "inserting and accessing cuts");
      ccP = dynamic_cast<const OsiColCut*>( cs.cutPtr(2*i+1) );
      OSIUNITTEST_ASSERT_ERROR(ccP != NULL, {}, "osicuts", "inserting and accessing cuts");
      OSIUNITTEST_ASSERT_ERROR(*ccP == ccv[i], {}, "osicuts", "inserting and accessing cuts");
#endif
      OSIUNITTEST_ASSERT_ERROR(eq(cs.mostEffectiveCutPtr()->effectiveness(),200.0+i), {}, "osicuts", "inserting and accessing cuts");
    }  
    
    // Test inserting collection of OsiCuts
    {
      OsiCuts cs;
      cs.insert(cuts);
      OSIUNITTEST_ASSERT_ERROR(cs.sizeColCuts() == 5, {}, "osicuts", "inserting collection of cuts");
      OSIUNITTEST_ASSERT_ERROR(cs.sizeRowCuts() == 5, {}, "osicuts", "inserting collection of cuts");
      OSIUNITTEST_ASSERT_ERROR(cs.sizeCuts() == 10, {}, "osicuts", "inserting collection of cuts");
    }
/*
  Test handling of cut pointers. Create a vector of cut pointers, then add
  them to the collection. Dump the row cuts, then add again. The cut
  objects should be deleted when the OsiCuts object is deleted at the end of
  the block.  This test should be monitored with some flavour of runtime
  access checking to be sure that cuts are not freed twice or not freed at
  all.
*/
    {
      OsiCuts cs;
      OsiRowCut *rcPVec[5] ;
      OsiColCut *ccPVec[5] ;
      for (i = 0 ; i < 5 ; i++) {
      	rcPVec[i] = rcv[i].clone() ;
      	ccPVec[i] = ccv[i].clone() ;
      }
      // test inserting cut ptr & accessing cut
      for ( i=0; i<5; i++ ) {
        OSIUNITTEST_ASSERT_ERROR(cs.sizeRowCuts() == i, {}, "osicuts", "handling of cut pointers");
        OsiRowCut * rcP = rcPVec[i] ;
        OSIUNITTEST_ASSERT_ERROR(rcP != NULL, {}, "osicuts", "handling of cut pointers");
        cs.insert(rcP);
        OSIUNITTEST_ASSERT_ERROR(rcP == NULL, {}, "osicuts", "handling of cut pointers");
        OSIUNITTEST_ASSERT_ERROR(cs.sizeRowCuts() == i+1, {}, "osicuts", "handling of cut pointers");
        OSIUNITTEST_ASSERT_ERROR(cs.rowCut(i) == rcv[i], {}, "osicuts", "handling of cut pointers");
        
        OsiColCut * ccP = ccPVec[i] ;
        OSIUNITTEST_ASSERT_ERROR(ccP != NULL, {}, "osicuts", "handling of cut pointers");
        OSIUNITTEST_ASSERT_ERROR(cs.sizeColCuts() == i, {}, "osicuts", "handling of cut pointers");
        cs.insert(ccP);
        OSIUNITTEST_ASSERT_ERROR(ccP == NULL, {}, "osicuts", "handling of cut pointers");
        OSIUNITTEST_ASSERT_ERROR(cs.sizeColCuts() == i+1, {}, "osicuts", "handling of cut pointers");
        OSIUNITTEST_ASSERT_ERROR(cs.colCut(i) == ccv[i], {}, "osicuts", "handling of cut pointers");

        OSIUNITTEST_ASSERT_ERROR(eq(cs.mostEffectiveCutPtr()->effectiveness(),200.0+i), {}, "osicuts", "handling of cut pointers");
      }
      cs.dumpCuts() ;		// row cuts only
      for ( i=0; i<5; i++ ) {
        OSIUNITTEST_ASSERT_ERROR(cs.sizeRowCuts() == i, {}, "osicuts", "handling of cut pointers");
        OsiRowCut * rcP = rcPVec[i] ;
        OSIUNITTEST_ASSERT_ERROR(rcP != NULL, {}, "osicuts", "handling of cut pointers");
        cs.insert(rcP);
        OSIUNITTEST_ASSERT_ERROR(rcP == NULL, {}, "osicuts", "handling of cut pointers");
        OSIUNITTEST_ASSERT_ERROR(cs.sizeRowCuts() == i+1, {}, "osicuts", "handling of cut pointers");
        OSIUNITTEST_ASSERT_ERROR(cs.rowCut(i) == rcv[i], {}, "osicuts", "handling of cut pointers");
        
        OSIUNITTEST_ASSERT_ERROR(cs.sizeColCuts() == 5, {}, "osicuts", "handling of cut pointers");
      }
    }
    
    // Test copy constructor
    OsiCuts csC(cs);
    OSIUNITTEST_ASSERT_ERROR(csC.sizeRowCuts() == 5, {}, "osicuts", "copy constructor");
    OSIUNITTEST_ASSERT_ERROR(csC.sizeColCuts() == 5, {}, "osicuts", "copy constructor");
    bool copy_ok = true;
    for ( i=0; i<5; i++ ) {
      copy_ok &= csC.rowCut(i) == rcv[i];
      copy_ok &= csC.colCut(i) == ccv[i];
      copy_ok &= csC.rowCut(i) == cs.rowCut(i);
      copy_ok &= csC.colCut(i) == cs.colCut(i);;
    }
    OSIUNITTEST_ASSERT_ERROR(copy_ok, {}, "osicuts", "copy constructor");
    OSIUNITTEST_ASSERT_ERROR(eq(csC.mostEffectiveCutPtr()->effectiveness(),204.0), {}, "osicuts", "copy constructor");
    
    rhs = cs;
  }
  
  // Test results of assignment operation
  bool ok = true;
  for ( i=0; i<5; i++ ) {
    ok &= rhs.rowCut(i) == rcv[i];
    ok &= rhs.colCut(i) == ccv[i];
  }
  OSIUNITTEST_ASSERT_ERROR(ok, {}, "osicuts", "assignment operator");
  OSIUNITTEST_ASSERT_ERROR(eq(rhs.mostEffectiveCutPtr()->effectiveness(),204.0), {}, "osicuts", "assignment operator");

  // Test removing cuts
  {
    OsiCuts t(rhs);  
    OSIUNITTEST_ASSERT_ERROR(t.sizeRowCuts() == 5, {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.sizeColCuts() == 5, {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(eq(rhs.mostEffectiveCutPtr()->effectiveness(),204.0), {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(eq(  t.mostEffectiveCutPtr()->effectiveness(),204.0), {}, "osicuts", "removing cuts");

    t.eraseRowCut(3);     
    OSIUNITTEST_ASSERT_ERROR(t.sizeRowCuts() == 4, {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.sizeColCuts() == 5, {}, "osicuts", "removing cuts");
    bool ok = true;
    for ( i=0; i<5; i++ )
      ok &= t.colCut(i) == ccv[i];
    OSIUNITTEST_ASSERT_ERROR(ok, {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.rowCut(0) == rcv[0], {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.rowCut(1) == rcv[1], {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.rowCut(2) == rcv[2], {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.rowCut(3) == rcv[4], {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(eq(t.mostEffectiveCutPtr()->effectiveness(),204.0), {}, "osicuts", "removing cuts");

    t.eraseColCut(4);     
    OSIUNITTEST_ASSERT_ERROR(t.sizeRowCuts() == 4, {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.sizeColCuts() == 4, {}, "osicuts", "removing cuts");
    ok = true;
    for ( i=0; i<4; i++ )
      ok &= t.colCut(i) == ccv[i];
    OSIUNITTEST_ASSERT_ERROR(ok, {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.rowCut(0) == rcv[0], {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.rowCut(1) == rcv[1], {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.rowCut(2) == rcv[2], {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(t.rowCut(3) == rcv[4], {}, "osicuts", "removing cuts");
    OSIUNITTEST_ASSERT_ERROR(eq(t.mostEffectiveCutPtr()->effectiveness(),203.0), {}, "osicuts", "removing cuts");
  }
  
  // sorting cuts
  {
    OsiCuts t(rhs);
    OSIUNITTEST_ASSERT_ERROR(t.sizeRowCuts() == 5, {}, "osicuts", "sorting cuts");
    OSIUNITTEST_ASSERT_ERROR(t.sizeColCuts() == 5, {}, "osicuts", "sorting cuts");
    t.rowCut(0).setEffectiveness(9.);
    t.rowCut(1).setEffectiveness(1.);
    t.rowCut(2).setEffectiveness(7.);
    t.rowCut(3).setEffectiveness(3.);
    t.rowCut(4).setEffectiveness(5.);
    t.colCut(0).setEffectiveness(2.);
    t.colCut(1).setEffectiveness(8.);
    t.colCut(2).setEffectiveness(4.);
    t.colCut(3).setEffectiveness(6.);
    t.colCut(4).setEffectiveness(.5);
    double totEff=1.+2.+3.+4.+5.+6.+7.+8.+9.+0.5;

    {
      // Test iterator over all cuts
      double sumEff=0.;
      for ( OsiCuts::iterator it=t.begin(); it!=t.end(); ++it ) {
        double eff=(*it)->effectiveness();
        sumEff+= eff;
      }
      OSIUNITTEST_ASSERT_ERROR(sumEff == totEff, {}, "osicuts", "sorting cuts");
    }

    t.sort();
    bool colcutsort_ok = true;
    for ( i=1; i<5; i++ ) colcutsort_ok &= t.colCut(i-1)>t.colCut(i);
    OSIUNITTEST_ASSERT_ERROR(colcutsort_ok, {}, "osicuts", "sorting cuts");
    bool rowcutsort_ok = true;
    for ( i=1; i<5; i++ ) rowcutsort_ok &= t.rowCut(i-1)>t.rowCut(i);
    OSIUNITTEST_ASSERT_ERROR(rowcutsort_ok, {}, "osicuts", "sorting cuts");

    {
      // Test iterator over all cuts
      double sumEff=0.;
      for ( OsiCuts::iterator it=t.begin(); it!=t.end(); ++it ) {
        sumEff+= (*it)->effectiveness();
      }
      OSIUNITTEST_ASSERT_ERROR(sumEff == totEff, {}, "osicuts", "sorting cuts");
    }

    {
      OsiCuts::iterator it=t.begin();
      OsiCut * cm1 = *it;
      ++it;
      bool sort_ok = true;
      for( ; it!=t.end(); it++ ) {
        OsiCut * c = *it;
        sort_ok &= (*cm1)>(*c);
        cm1 = c;
      }
      OSIUNITTEST_ASSERT_ERROR(sort_ok, {}, "osicuts", "sorting cuts");
    }
  }
}
