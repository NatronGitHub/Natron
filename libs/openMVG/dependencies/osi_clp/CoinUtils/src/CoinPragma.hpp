/* $Id: CoinPragma.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPragma_H
#define CoinPragma_H

//-------------------------------------------------------------------
//
// This is a file which can contain Pragma's that are
// generally applicable to any source file.
//
//-------------------------------------------------------------------

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
// Turn off compiler warning: 
// "empty controlled statement found; is this the intent?"
#  pragma warning(disable:4390)
// Turn off compiler warning about deprecated functions
#  pragma warning(disable:4996)
#endif

#endif
