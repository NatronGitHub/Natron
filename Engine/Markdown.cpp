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

#include "Markdown.h"

#include <sstream>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTextStream>
#include <QRegExp>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "html.h" //hoedown

NATRON_NAMESPACE_ENTER;

Markdown::Markdown()
{
}

// Converts markdown to html
QString
Markdown::convert2html(QString markdown)
{
    QString html;

    if ( !markdown.isEmpty() ) {
        markdown = parseCustomLinksForHTML(markdown);

        hoedown_html_flags flags = HOEDOWN_HTML_SKIP_HTML;
        hoedown_extensions extensions = HOEDOWN_EXT_AUTOLINK;
        size_t max_nesting = 16;
        hoedown_renderer *renderer = hoedown_html_renderer_new(flags, 0);
        hoedown_document *document = hoedown_document_new(renderer, extensions, max_nesting);
        hoedown_buffer *result = hoedown_buffer_new(max_nesting);
        hoedown_document_render( document, result, reinterpret_cast<const uint8_t*>(&markdown.toStdString()[0]), markdown.toStdString().size() );

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

// Creates a markdown grid table from plugin knobs
// for use with pandoc (which converts the markdown to rst for use in sphinx/rtd)
// Only used as an intermediate, so the table does not look good in plaintext.
QString
Markdown::genPluginKnobsTable(QVector<QStringList> items)
{
    QString ret;
    QTextStream ts(&ret);

    if (items.size() > 0) {
        int header1Length = 0;
        int header2Length = 0;
        int header3Length = 0;
        int header4Length = 0;
        int headerTotal = 0;
        int headerPadding = 4;
        int headerCap = 60;
        QString header1Text = tr("Label (UI Name)");
        QString header2Text = tr("Script-Name");
        QString header3Text = tr("Default-Value");
        QString header4Text = tr("Function");

        // get sizes
        Q_FOREACH(const QStringList &item, items) {
            int header1Count = item.at(0).count() + headerPadding + header1Text.count();

            if (header1Count > headerCap) {
                header1Count = headerCap;
            }
            if (header1Count > header1Length) {
                header1Length = header1Count;
            }
            int header2Count = item.at(1).count() + headerPadding + header2Text.count();
            if (header2Count > headerCap) {
                header2Count = headerCap;
            }
            if (header2Count > header2Length) {
                header2Length = header2Count;
            }
            int header3Count = item.at(2).count() + headerPadding + header3Text.count();
            if (header3Count > headerCap) {
                header3Count = headerCap;
            }
            if (header3Count > header3Length) {
                header3Length = header3Count;
            }
            int header4Count = item.at(3).count() + headerPadding + header4Text.count();
            if (header4Count > headerCap) {
                header4Count = headerCap;
            }
            if (header4Count > header4Length) {
                header4Length = header4Count;
            }
        }
        headerTotal = (header1Length + header2Length + header3Length + header4Length) - 2;
        int header1Split = header1Length;
        int header2Split = header1Split + header2Length;
        int header3Split = header2Split + header3Length;

        // table top
        ts << "+";
        for (int i = 0; i < headerTotal; ++i) {
            if ( (i == header1Split) || (i == header2Split) || (i == header3Split) ) {
                ts << "+";
            } else {
                ts << "-";
            }
        }
        ts << "+\n";

        // header text
        ts << "| " << header1Text;
        for (int i = 0; i < headerTotal; ++i) {
            if (i == ( header1Split - header1Text.count() ) - 1) {
                ts << "| " << header2Text;
            } else if (i == ( header2Split - header1Text.count() - header2Text.count() ) - 2) {
                ts << "| " << header3Text;
            } else if (i == ( header3Split - header1Text.count() - header2Text.count() - header3Text.count() ) - 3) {
                ts << "| " << header4Text;
            } else {
                ts << " ";
            }
        }
        ts << "|\n";

        // header bottom
        ts << "+";
        for (int i = 0; i < headerTotal; ++i) {
            if ( (i == header1Split) || (i == header2Split) || (i == header3Split) ) {
                ts << "+";
            } else {
                ts << "=";
            }
        }
        ts << "+\n";

        // table rows
        Q_FOREACH(const QStringList &item, items) {
            QString col1 = item.at(0);

            if (col1.count() < header1Length) {
                for (int i = col1.count(); i < header1Length - 1; ++i) {
                    col1.append( QString::fromUtf8(" ") );
                }
                col1.append( QString::fromUtf8("|") );
            }
            QString col2 = item.at(1);
            if (col2.count() < header2Length) {
                for (int i = col2.count(); i < header2Length - 1; ++i) {
                    col2.append( QString::fromUtf8(" ") );
                }
                col2.append( QString::fromUtf8("|") );
            }
            QString col3 = item.at(2);
            if (col3.count() < header3Length) {
                for (int i = col3.count(); i < header3Length - 1; ++i) {
                    col3.append( QString::fromUtf8(" ") );
                }
                col3.append( QString::fromUtf8("|") );
            }
            QString col4 = item.at(3);
            if (col4.count() < header4Length) {
                for (int i = col4.count(); i < header4Length - 1; ++i) {
                    col4.append( QString::fromUtf8(" ") );
                }
                col4.append( QString::fromUtf8("|\n") );
            } else {
                col4.replace( QString::fromUtf8("\n"), QString::fromUtf8("") );
                col4.append( QString::fromUtf8("|\n") );
            }
            ts << "| ";
            ts << col1 << col2 << col3 << col4;

            // table end
            ts << "+";
            for (int i = 0; i < headerTotal; ++i) {
                if ( (i == header1Split) || (i == header2Split) || (i == header3Split) ) {
                    ts << "+";
                } else {
                    ts << "-";
                }
            }
            ts << "+\n";
        }
    }

    return ret;
} // Markdown::genPluginKnobsTable

QString
Markdown::parseCustomLinksForHTML(QString markdown)
{
    QString result;

    if ( !markdown.isEmpty() ) {
        QStringList split = markdown.split( QString::fromUtf8("\n") );
        Q_FOREACH(const QString &line_const, split) {
            QString line( line_const + QChar::fromAscii('\n') );

            if ( line.contains( QString::fromUtf8("|html::") ) && line.contains( QString::fromUtf8("|rst::") ) ) {
                line.replace( QString::fromUtf8("|html::"), QString::fromUtf8("") ).replace( QRegExp( QString::fromUtf8("\\|\\|rst::.*\\|") ), QString::fromUtf8("") );
            }
        }
    }

    return result;
}

NATRON_NAMESPACE_EXIT;

