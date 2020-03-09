/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef Engine_MemoryInfo_h
#define Engine_MemoryInfo_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstddef> // std::size_t

#include <QtCore/QString>

#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER

// Memory utility functions ( info about RAM etc...)

U64 getSystemTotalRAM();

inline bool isApplication32Bits()
{
    return sizeof(void*) == 4;
}

U64 getSystemTotalRAM_conditionnally();

// prints RAM value as KB, MB or GB
QString printAsRAM(U64 bytes);

#if 0 // not used for now
/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
std::size_t getPeakRSS( );

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
std::size_t getCurrentRSS( );
#endif // 0

std::size_t getAmountFreePhysicalRAM();

NATRON_NAMESPACE_EXIT

#endif // ifndef Engine_MemoryInfo_h
