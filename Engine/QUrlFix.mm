/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "Global/QtCompat.h"

#include <QtCore/QtGlobal> // for Q_OS_*
#if defined(Q_OS_MAC) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)

// in Qt 4.8 QUrl is broken on mac, it returns /.file/id= for local files
// See https://bugreports.qt.io/browse/QTBUG-40449

#include <Foundation/NSString.h>
#include <Foundation/NSAutoreleasePool.h>
#include <Foundation/NSURL.h>


static
QString fromNSString(const NSString *string)
{
    if (!string)
        return QString();
    QString qstring;
    qstring.resize([string length]);
    [string getCharacters: reinterpret_cast<unichar*>(qstring.data()) range: NSMakeRange(0, [string length])];
    return qstring;
}

static
NSString *toNSString(const QString &str)
{
    return [NSString stringWithCharacters: reinterpret_cast<const UniChar*>(str.unicode()) length: str.length()];
}

static
QUrl fromNSURL(const NSURL *url)
{
    return QUrl(fromNSString([url absoluteString]));
}

static
NSURL *toNSURL(const QUrl &url)
{
    return [NSURL URLWithString:toNSString(QString::fromUtf8(url.toEncoded()))];
}

QUrl NATRON_NAMESPACE::QtCompat::toLocalFileUrlFixed(const QUrl& url)
{
    QUrl ret = url;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSURL *nsurl = toNSURL(url);
    if ([nsurl isFileReferenceURL])
        ret = fromNSURL([nsurl filePathURL]);
    [pool release];
    return ret;
}

#endif // #if defined(Q_OS_MAC) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
