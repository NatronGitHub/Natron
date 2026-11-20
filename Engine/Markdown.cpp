/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Markdown.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTextStream>
#include <QRegularExpression>
#include <QByteArray>
#include <QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "md4c-html.h" // libs/md4c

NATRON_NAMESPACE_ENTER

Markdown::Markdown()
{
}

namespace {

// md4c output callback: appends the UTF-8 HTML to a QByteArray
void
appendToByteArray(const MD_CHAR* text,
                  MD_SIZE size,
                  void* userdata)
{
    static_cast<QByteArray*>(userdata)->append( text, static_cast<int>(size) );
}

// md4c-html writes LaTeX math spans as <x-equation> (inline, $...$) and
// <x-equation type="display"> ($$...$$) elements. The generated documentation
// pages load MathJax, which uses the \(...\) and \[...\] delimiters (as
// hoedown wrote them), so translate the elements to these delimiters.
QByteArray
mathElementsToMathJax(const QByteArray& html)
{
    static const QByteArray inlineOpen("<x-equation>");
    static const QByteArray displayOpen("<x-equation type=\"display\">");
    static const QByteArray close("</x-equation>");

    if ( !html.contains("<x-equation") ) {
        return html;
    }
    QByteArray result;
    result.reserve( html.size() );
    qsizetype pos = 0;
    while (true) {
        qsizetype open = html.indexOf("<x-equation", pos);
        if (open < 0) {
            break;
        }
        bool display;
        qsizetype contentStart;
        if ( html.mid( open, inlineOpen.size() ) == inlineOpen ) {
            display = false;
            contentStart = open + inlineOpen.size();
        } else if ( html.mid( open, displayOpen.size() ) == displayOpen ) {
            display = true;
            contentStart = open + displayOpen.size();
        } else {
            // not written by md4c (e.g. raw HTML): leave it as is
            result.append( html.mid( pos, open + 1 - pos ) );
            pos = open + 1;
            continue;
        }
        qsizetype end = html.indexOf(close, contentStart);
        if (end < 0) {
            break;
        }
        result.append( html.mid(pos, open - pos) );
        result.append(display ? "\\[" : "\\(");
        result.append( html.mid(contentStart, end - contentStart) );
        result.append(display ? "\\]" : "\\)");
        pos = end + close.size();
    }
    result.append( html.mid(pos) );

    return result;
}

} // anonymous namespace

// Converts markdown to html, using md4c (CommonMark 0.31.2 with extensions)
QString
Markdown::convert2html(const QString& markdown)
{
    QString html;

    if ( !markdown.isEmpty() ) {
        QString markdownClean = parseCustomLinksForHTML(markdown);
        QByteArray markdownUtf8 = markdownClean.toUtf8();

        // The extensions that Natron enabled with hoedown (HOEDOWN_EXT_BLOCK,
        // HOEDOWN_EXT_SPAN and HOEDOWN_EXT_FLAGS), except:
        // - "quote" ("text" as <q>text</q>), which md4c doesn't have;
        // - underline: with MD_FLAG_UNDERLINE, md4c renders __text__ as
        //   <u><u>text</u></u>, where hoedown wrote <strong>text</strong>, and
        //   the bundled plug-ins use __text__ for bold. Without it, _text_ is
        //   emphasis, as in CommonMark.
        // Fenced code blocks and the space required after the '#' of ATX
        // headers are part of CommonMark. Indented code blocks stay enabled, as
        // they were with hoedown (HOEDOWN_EXT_DISABLE_INDENTED_CODE was not set).
        const unsigned parserFlags = (MD_FLAG_TABLES |
                                      MD_FLAG_FOOTNOTES |
                                      MD_FLAG_STRIKETHROUGH |
                                      MD_FLAG_HIGHLIGHT |
                                      MD_FLAG_SUPERSCRIPTS |
                                      MD_FLAG_LATEXMATHSPANS |
                                      MD_FLAG_PERMISSIVEAUTOLINKS);
        const unsigned rendererFlags = MD_HTML_FLAG_XHTML;
        QByteArray result;
        result.reserve(markdownUtf8.size() * 2);
        int ret = md_html(markdownUtf8.constData(), static_cast<MD_SIZE>( markdownUtf8.size() ),
                          appendToByteArray, &result, parserFlags, rendererFlags);
        if (ret != 0) {
            // md4c only fails when it runs out of memory
            qDebug() << "Markdown::convert2html: md4c failed to convert the Markdown text";
        }

        html = QString::fromUtf8( mathElementsToMathJax(result) );
    }

    return html;
}

QString
Markdown::parseCustomLinksForHTML(const QString& markdown)
{
    QString result = markdown;
    QRegularExpression rx( QString::fromUtf8("(\\|html::[^|]*\\|)\\|rst::[^|]*\\|") );
    result.replace( rx, QString::fromUtf8("\\1") );

    return result;
}

QString
Markdown::fixNodeHTML(const QString &html)
{
    QString result = html;

    // md4c writes a single newline between the header and the table (hoedown wrote two)
    result.replace( QRegularExpression( QString::fromUtf8("<h2>Inputs</h2>\\s*<table>") ), QString::fromUtf8("<h2>Inputs <span class=\"showHideTable\">(<a class=\"toggleInputTable\" href=\"#\">+/-</a>)</span></h2>\n<table class=\"inputTable\">") );
    result.replace( QRegularExpression( QString::fromUtf8("<h2>Controls</h2>\\s*<table>") ), QString::fromUtf8("<h2>Controls <span class=\"showHideTable\">(<a class=\"toggleControlTable\" href=\"#\">+/-</a>)</span></h2>\n<table class=\"controlTable\">") );

    return result;
}

QString
Markdown::fixSettingsHTML(const QString &html)
{
    QString result;

    // Replace <h2>A Title</h2> with <h2 id="a-title">A Title</h2>
    QStringList list = html.split( QString::fromUtf8("\n") );
    Q_FOREACH(const QString &line, list) {
        if ( line.startsWith(QString::fromUtf8("<h2>")) ) {
            QRegularExpression rx( QString::fromUtf8("<h2>(.*)</h2>") );
            QRegularExpressionMatch match = rx.match(line);
            QString header = match.captured(1);
            QString headerLink = header.toLower();
            headerLink.replace( QString::fromUtf8(" "), QString::fromUtf8("-") );
            result.append(QString::fromUtf8("<h2 id=\"%1\">%2</h2>").arg(headerLink).arg(header));
        } else {
            result.append(line);
        }
    }

    return result;
}

NATRON_NAMESPACE_EXIT

