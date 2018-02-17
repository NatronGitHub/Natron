/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Utils.h"

#include <stdexcept>

#include <QtCore/QString>
#include <QtCore/QChar>
#include <QtCore/QDebug>

NATRON_NAMESPACE_ENTER

/*!
   \fn QString NATRON_NAMESPACE::convertFromPlainText(const QString &plain, WhiteSpaceMode mode)

   Converts the plain text string \a plain to an HTML-formatted
   paragraph while preserving most of its look.

   \a mode defines how whitespace is handled.

   This function was adapted from Qt::convertFromPlainText()
   (see src/gui/text/qtextdocument.cpp in the Qt sources)
   The difference is that in NATRON_NAMESPACE::WhiteSpaceNormal mode, spaces are preserved at the beginning of the line.
 */
QString
convertFromPlainText(const QString &plain,
                     NATRON_NAMESPACE::WhiteSpaceMode mode)
{
    int col = 0;
    bool bol = true;
    QString rich;

    rich += QLatin1String("<p>");
    for (int i = 0; i < plain.length(); ++i) {
        if ( plain[i] == QLatin1Char('\n') ) {
            int c = 1;
            while ( i + 1 < plain.length() && plain[i + 1] == QLatin1Char('\n') ) {
                i++;
                c++;
            }
            if (c == 1) {
                rich += QLatin1String("<br>\n");
            } else {
                rich += QLatin1String("</p>\n");
                while (--c > 1) {
                    rich += QLatin1String("<br>\n");
                }
                rich += QLatin1String("<p>");
            }
            col = 0;
            bol = true;
        } else {
            bool bolagain = false;
            if ( (mode == NATRON_NAMESPACE::WhiteSpacePre) && ( plain[i] == QLatin1Char('\t') ) ) {
                rich += QChar(0x00a0U);
                ++col;
                while (col % 8) {
                    rich += QChar(0x00a0U);
                    ++col;
                }
                bolagain = bol;
            } else if ( ( bol || (mode == NATRON_NAMESPACE::WhiteSpacePre) ) && plain[i].isSpace() ) {
                rich += QChar(0x00a0U);
                bolagain = bol;
            } else if ( plain[i] == QLatin1Char('<') ) {
                rich += QLatin1String("&lt;");
            } else if ( plain[i] == QLatin1Char('>') ) {
                rich += QLatin1String("&gt;");
            } else if ( plain[i] == QLatin1Char('&') ) {
                rich += QLatin1String("&amp;");
            } else {
                rich += plain[i];
            }
            ++col;
            bol = bolagain;
        }
    }
    if (col != 0) {
        rich += QLatin1String("</p>");
    }

    return rich;
} // convertFromPlainText

// use genHTML=true when generating markdown for hoedown. false for pandoc
QString
convertFromPlainTextToMarkdown(const QString &plain_, bool genHTML, bool isTableElement)
{
    QString escaped;
    // we trim table elements
    QString plain = isTableElement ? plain_.trimmed() : plain_;
    // the following chars must be backslash-escaped in markdown:
    // \    backslash
    // `    backtick
    // *    asterisk
    // _    underscore
    // {}   curly braces
    // []   square brackets
    // ()   parentheses
    // #    hash mark
    // +    plus sign
    // -    minus sign (hyphen)
    // .    dot
    // !    exclamation mark

    // We must also escape angle brackets < and > , so that they don't become HTML tags

    // we do a hack for multiline elements, because the markdown->rst conversion by pandoc doesn't use the line block syntax.
    // what we do here is put a supplementary dot at the beginning of each line, which is then converted to a pipe '|' in the
    // genStaticDocs.sh script by a simple sed command after converting to RsT
    bool isMultilineElement = isTableElement && plain.contains(QLatin1Char('\n'));
    if (isMultilineElement && !genHTML) {
        escaped += QString::fromUtf8(". ");
    }
    for (int i = 0; i < plain.length(); ++i) {
        bool outputChar = true;
        if (plain[i] == QLatin1Char('\\') ||
            plain[i] == QLatin1Char('`') ||
            plain[i] == QLatin1Char('*') ||
            plain[i] == QLatin1Char('_') ||
            plain[i] == QLatin1Char('{') ||
            plain[i] == QLatin1Char('}') ||
            plain[i] == QLatin1Char('[') ||
            plain[i] == QLatin1Char(']') ||
            plain[i] == QLatin1Char('(') ||
            plain[i] == QLatin1Char(')') ||
            plain[i] == QLatin1Char('#') ||
            plain[i] == QLatin1Char('+') ||
            plain[i] == QLatin1Char('-') ||
            plain[i] == QLatin1Char('.') ||
            plain[i] == QLatin1Char('!')) {
            escaped += QLatin1Char('\\');
        } else if ( plain[i] == QLatin1Char('<') ) {
            escaped += QString::fromUtf8("&lt;");
            outputChar = false;
        } else if ( plain[i] == QLatin1Char('>') ) {
            escaped += QString::fromUtf8("&gt;");
            outputChar = false;
        } else if (isTableElement) {
            if (plain[i] == QLatin1Char('|')) {
                escaped += QString::fromUtf8("&#124;");
                outputChar = false;
            } else if (plain[i] == QLatin1Char('\n')) {
                assert(isMultilineElement);
                if (genHTML) {
                    // we are generating markdown for hoedown

                    // "<br />" should work, but actually it doesn't work well and is ignored by pandoc in many cases
                    escaped += QString::fromUtf8("<br />");
                    outputChar = false;
                } else {
                    // we are generating markdown for pandoc

                    // see http://rmarkdown.rstudio.com/authoring_pandoc_markdown.html
                    // A backslash followed by a newline is also a hard line break.
                    // Note: in multiline and grid table cells, this is the only way
                    // to create a hard line break, since trailing spaces in the cells are ignored.
                    escaped += QLatin1Char('\\');
                    // we add a dot at the beginning of the next line, which is converted to a pipe "|" by
                    // the genStaticDocs.sh script afterwards, see comment above.
                    escaped += QLatin1Char('\n');
                    escaped += QString::fromUtf8(". ");
                    outputChar = false;
                }
            }
        } else if (plain[i] == QLatin1Char('\n')) {
            // line breaks become paragraph breaks (double the line breaks)
            escaped += QLatin1Char('\n');
        }
        if (outputChar) {
            escaped += plain[i];
        }
    }
    if ( isTableElement && escaped.isEmpty() ) {
        escaped = QString::fromUtf8("&nbsp;");
    }
    if (isTableElement) {
        return escaped.trimmed();
    }
    return escaped;
} // convertFromPlainText

NATRON_NAMESPACE_EXIT
