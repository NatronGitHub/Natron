/* $Id: CoinError.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinError.hpp"

bool CoinError::printErrors_ = false;

/** A function to block the popup windows that windows creates when the code
    crashes */
#ifdef HAVE_WINDOWS_H
#include <windows.h>
void WindowsErrorPopupBlocker()
{
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
}
#else
void WindowsErrorPopupBlocker() {}
#endif
