//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Global/QtCompat.h"
#include <Foundation/NSString.h>
#include <Foundation/NSAutoreleasePool.h>
#include <Foundation/NSURL.h>


QString fromNSString(const NSString *string)
{
    if (!string)
        return QString();
    QString qstring;
    qstring.resize([string length]);
    [string getCharacters: reinterpret_cast<unichar*>(qstring.data()) range: NSMakeRange(0, [string length])];
    return qstring;
}

NSString *toNSString(const QString &str)
{
    return [NSString stringWithCharacters: reinterpret_cast<const UniChar*>(str.unicode()) length: str.length()];
}

QUrl fromNSURL(const NSURL *url)
{
    return QUrl(fromNSString([url absoluteString]));
}

NSURL *toNSURL(const QUrl &url)
{
    return [NSURL URLWithString:toNSString(url.toEncoded())];
}

namespace Natron {
QUrl toLocalFileUrlFixed(const QUrl& url)
{
#if QT_VERSION >= 0x050000
    return url;
#endif
    QUrl ret = url;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSURL *nsurl = toNSURL(url);
    if ([nsurl isFileReferenceURL])
        ret = fromNSURL([nsurl filePathURL]);
    [pool release];
    return ret;
}
}
