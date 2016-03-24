// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include "CoinError.hpp"

void
CoinErrorUnitTest()
{

  // Test default constructor
  {
    CoinError r;
    assert( r.message_=="" );
    assert( r.method_=="" );
    assert( r.class_=="" );
  }

  // Test alternate constructor and get method
  CoinError rhs;
  {
    CoinError a("msg","me.hpp","cl");
    assert( a.message()=="msg" );
    assert( a.methodName()=="me.hpp" );
    assert( a.className()=="cl" );
    
    // Test copy constructor
    {
      CoinError c(a);
      assert( c.message()=="msg" );
      assert( c.methodName()=="me.hpp" );
      assert( c.className()=="cl" );
    }
    
    // Test assignment
    {
      CoinError a1("msg1","meth1","cl1");
      assert( a1.message()=="msg1" );
      assert( a1.methodName()=="meth1" );
      assert( a1.className()=="cl1" );
      rhs = a1;
    }
  }
  assert( rhs.message()=="msg1" );
  assert( rhs.methodName()=="meth1" );
  assert( rhs.className()=="cl1" );


}
