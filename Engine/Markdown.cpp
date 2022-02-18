/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#include <sstream> // stringstream

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QTextStream>
#include <QtCore/QRegExp>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "html.h" //hoedown

NATRON_NAMESPACE_ENTER

Markdown::Markdown()
{
}

// Converts markdown to html
QString
Markdown::convert2html(const QString& markdown)
{
    QString html;

    if ( !markdown.isEmpty() ) {
        QString markdownClean = parseCustomLinksForHTML(markdown);

        hoedown_html_flags flags = HOEDOWN_HTML_USE_XHTML;
        hoedown_extensions extensions = (hoedown_extensions)(HOEDOWN_EXT_BLOCK|HOEDOWN_EXT_SPAN|HOEDOWN_EXT_FLAGS);
        size_t max_nesting = 16;
        hoedown_renderer *renderer = hoedown_html_renderer_new(flags, 0);
        hoedown_document *document = hoedown_document_new(renderer, extensions, max_nesting);
        hoedown_buffer *result = hoedown_buffer_new(max_nesting);
        std::string markdownStr = markdownClean.toStdString();
        hoedown_document_render( document, result, reinterpret_cast<const uint8_t*>(&markdownStr[0]), markdownStr.size() );

        std::ostringstream convert;
        for (size_t x = 0; x < result->size; x++) {
            convert << result->data[x];
        }

        html = QString::fromStdString( convert.str() );

        hoedown_buffer_free(result);
        hoedown_document_free(document);
        hoedown_html_renderer_free(renderer);
    }

    return html;
}

QString
Markdown::parseCustomLinksForHTML(const QString& markdown)
{
    QString result = markdown;
    QRegExp rx( QString::fromUtf8("(\\|html::[^|]*\\|)\\|rst::[^|]*\\|") );
    result.replace( rx, QString::fromUtf8("\\1") );

    return result;
}

QString
Markdown::fixNodeHTML(const QString &html)
{
    QString result = html;

    result.replace( QString::fromUtf8("<h2>Inputs</h2>\n\n<table>"), QString::fromUtf8("<h2>Inputs <span class=\"showHideTable\">(<a class=\"toggleInputTable\" href=\"#\">+/-</a>)</span></h2>\n\n<table class=\"inputTable\">") );
    result.replace( QString::fromUtf8("<h2>Controls</h2>\n\n<table>"), QString::fromUtf8("<h2>Controls <span class=\"showHideTable\">(<a class=\"toggleControlTable\" href=\"#\">+/-</a>)</span></h2>\n\n<table class=\"controlTable\">") );

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
            QRegExp rx( QString::fromUtf8("<h2>(.*)</h2>") );
            rx.indexIn(line);
            QString header = rx.cap(1);
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

