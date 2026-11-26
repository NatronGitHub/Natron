/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

#include "DocumentationServer.h"

#include <QChar>
#include <QDir>
#include <QUrl>

NATRON_NAMESPACE_ENTER

namespace DocumentationServer {
QString
resolveDocumentationFile(const QString& docDir,
                         const QString& page)
{
    if ( page.isEmpty() || page.contains( QLatin1Char('\\') ) || page.contains( QLatin1Char(':') ) || page.contains( QChar() ) ) {
        return QString();
    }
    const QString root = QDir::cleanPath( QDir(docDir).absolutePath() );
    const QString file = QDir::cleanPath( root + QLatin1Char('/') + page );
    if ( !file.startsWith( root + QLatin1Char('/') ) ) {
        return QString();
    }

    return file;
}

QString
resolveLocalFile(const QString& path,
                 const QStringList& allowedFiles)
{
    QString decoded = path;
    decoded.replace( QLatin1String("%25"), QLatin1String("%") );
    decoded = QDir::cleanPath( QUrl::fromPercentEncoding( decoded.toUtf8() ) );
    if ( decoded.isEmpty() || decoded.contains( QChar() ) ) {
        return QString();
    }
    for (const QString& allowed : allowedFiles) {
        // the URL was made from the same path, with '/' as the separator (NodeDocumentation.cpp)
        if ( !allowed.isEmpty() && ( QDir::cleanPath( QDir::fromNativeSeparators(allowed) ) == decoded ) ) {
            return allowed;
        }
    }

    return QString();
}
} // namespace DocumentationServer

NATRON_NAMESPACE_EXIT
