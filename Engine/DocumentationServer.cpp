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
bool
isLoopbackHost(const QString& hostHeader,
               int serverPort)
{
    const QString host = hostHeader.trimmed().toLower();
    QString name;
    QString port;

    if ( host.startsWith( QLatin1Char('[') ) ) {
        // IPv6 literal: "[::1]" or "[::1]:port"
        const int end = host.indexOf( QLatin1Char(']') );
        if (end < 0) {
            return false;
        }
        name = host.left(end + 1);
        port = host.mid(end + 1);
    } else {
        const int colon = host.indexOf( QLatin1Char(':') );
        name = (colon < 0) ? host : host.left(colon);
        port = (colon < 0) ? QString() : host.mid(colon);
    }
    if ( !port.isEmpty() ) {
        // ":" followed by the server's port, in decimal digits only (QString::toInt() would also take a sign)
        if ( ( port.size() < 2 ) || ( port.at(0) != QLatin1Char(':') ) ) {
            return false;
        }
        for (int i = 1; i < port.size(); ++i) {
            if ( ( port.at(i) < QLatin1Char('0') ) || ( port.at(i) > QLatin1Char('9') ) ) {
                return false;
            }
        }
        bool ok = false;
        const int value = port.mid(1).toInt(&ok);
        if ( !ok || (value != serverPort) ) {
            return false;
        }
    }

    return name == QLatin1String("localhost") || name == QLatin1String("127.0.0.1") || name == QLatin1String("[::1]");
}

bool
isCrossSiteRequest(const QString& secFetchSite)
{
    const QString site = secFetchSite.trimmed().toLower();

    return site == QLatin1String("cross-site") || site == QLatin1String("same-site");
}

QString
resolveDocumentationFile(const QString& docDir,
                         const QString& page)
{
    if ( page.isEmpty() || page.contains( QLatin1Char('\\') ) || page.contains( QLatin1Char(':') ) || page.contains( QChar() ) ) {
        return QString();
    }
    // Windows device names (CON, NUL, COM1...), with any extension, in any directory: opening one may
    // block (e.g. "_static/CON.txt" reads the console)
    const QStringList segments = page.split( QLatin1Char('/') );
    for (const QString& segment : segments) {
        const QString base = segment.section( QLatin1Char('.'), 0, 0 ).trimmed().toUpper();
        if ( ( base == QLatin1String("CON") ) || ( base == QLatin1String("PRN") ) || ( base == QLatin1String("AUX") ) ||
             ( base == QLatin1String("NUL") ) || ( base == QLatin1String("CONIN$") ) || ( base == QLatin1String("CONOUT$") ) ||
             ( ( base.size() == 4 ) && ( base.startsWith( QLatin1String("COM") ) || base.startsWith( QLatin1String("LPT") ) ) &&
               ( base.at(3) >= QLatin1Char('0') ) && ( base.at(3) <= QLatin1Char('9') ) ) ) {
            return QString();
        }
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
