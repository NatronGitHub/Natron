/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Utils.h"

NATRON_NAMESPACE_ENTER;

/*!
 \fn QString Natron::convertFromPlainText(const QString &plain, WhiteSpaceMode mode)

 Converts the plain text string \a plain to an HTML-formatted
 paragraph while preserving most of its look.

 \a mode defines how whitespace is handled.

 This function was adapted from Natron::convertFromPlainText()
 (see src/gui/text/qtextdocument.cpp in the Qt sources)
 The difference is that in Qt::WhiteSpaceNormal mode, spaces are preserved at the beginning of the line.
 */
QString convertFromPlainText(const QString &plain, Qt::WhiteSpaceMode mode)
{
    int col = 0;
    bool bol = true;
    QString rich;
    rich += QLatin1String("<p>");
    for (int i = 0; i < plain.length(); ++i) {
        if (plain[i] == QLatin1Char('\n')){
            int c = 1;
            while (i+1 < plain.length() && plain[i+1] == QLatin1Char('\n')) {
                i++;
                c++;
            }
            if (c == 1)
                rich += QLatin1String("<br>\n");
            else {
                rich += QLatin1String("</p>\n");
                while (--c > 1)
                    rich += QLatin1String("<br>\n");
                rich += QLatin1String("<p>");
            }
            col = 0;
            bol = true;
        } else {
            bool bolagain = false;
            if (mode == Qt::WhiteSpacePre && plain[i] == QLatin1Char('\t')){
                rich += QChar(0x00a0U);
                ++col;
                while (col % 8) {
                    rich += QChar(0x00a0U);
                    ++col;
                }
                bolagain = bol;
            }
            else if ((bol || mode == Qt::WhiteSpacePre) && plain[i].isSpace()){
                rich += QChar(0x00a0U);
                bolagain = bol;
            }
            else if (plain[i] == QLatin1Char('<'))
                rich += QLatin1String("&lt;");
            else if (plain[i] == QLatin1Char('>'))
                rich += QLatin1String("&gt;");
            else if (plain[i] == QLatin1Char('&'))
                rich += QLatin1String("&amp;");
            else
                rich += plain[i];
            ++col;
            bol = bolagain;
        }
    }
    if (col != 0)
        rich += QLatin1String("</p>");
    return rich;
}

NATRON_NAMESPACE_EXIT;
