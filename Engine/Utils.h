/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Natron_Engine_Utils_h
#define Natron_Engine_Utils_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

// same as Qt::WhiteSpaceMode
enum WhiteSpaceMode {
    WhiteSpaceNormal,
    WhiteSpacePre,
    WhiteSpaceNoWrap,
    WhiteSpaceModeUndefined = -1
};

QString convertFromPlainText(const QString &plain, NATRON_NAMESPACE::WhiteSpaceMode mode);

// use genHTML=true when generating markdown for hoedown. false for pandoc
QString convertFromPlainTextToMarkdown(const QString &plain, bool genHTML, bool isTableElement);

// Exponentiation by squaring
// works with positive or negative integer exponents.
template<typename T>
double
ipow(T base,
     int exp)
{
    double result = 1.;

    if (exp >= 0) {
        while (exp) {
            if (exp & 1) {
                result *= base;
            }
            exp >>= 1;
            base *= base;
        }
    } else {
        exp = -exp;
        while (exp) {
            if (exp & 1) {
                result /= base;
            }
            exp >>= 1;
            base *= base;
        }
    }

    return result;
}

NATRON_NAMESPACE_EXIT

#endif // Natron_Gui_Utils_h
