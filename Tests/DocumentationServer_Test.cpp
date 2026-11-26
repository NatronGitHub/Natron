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

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <QtCore/QChar>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "Engine/DocumentationServer.h"

// The request checks of the local documentation server (Gui/DocumentationManager.cpp).

NATRON_NAMESPACE_USING

namespace {

QString
s(const char* utf8)
{
    return QString::fromUtf8(utf8);
}

// docDir as Gui/DocumentationManager.cpp builds it: "<bin>/../Resources/docs/html/"
const char* const kDocDir = "/opt/Natron/bin/../Resources/docs/html/";

// the documentation directory, absolute ("C:/opt/..." on Windows) and without "..": "/opt/Natron/Resources/docs/html"
QString
root()
{
    return QDir::cleanPath( QDir( s(kDocDir) ).absolutePath() );
}

} // anon namespace

TEST(DocumentationServer, ResolvesFilesInDocDir)
{
    EXPECT_EQ( root() + s("/index.html"), DocumentationServer::resolveDocumentationFile( s(kDocDir), s("index.html") ) );
    EXPECT_EQ( root() + s("/_static/style.css"), DocumentationServer::resolveDocumentationFile( s(kDocDir), s("_static/style.css") ) );
    EXPECT_EQ( root() + s("/plugins/fr.inria.built-in.Read.html"),
               DocumentationServer::resolveDocumentationFile( s(kDocDir), s("plugins/fr.inria.built-in.Read.html") ) );
    // ".." that stays inside docDir
    EXPECT_EQ( root() + s("/index.html"), DocumentationServer::resolveDocumentationFile( s(kDocDir), s("plugins/../index.html") ) );
    // more slashes (QDir::cleanPath() removes them)
    EXPECT_EQ( root() + s("/etc/hosts.txt"), DocumentationServer::resolveDocumentationFile( s(kDocDir), s("//etc/hosts.txt") ) );
    EXPECT_EQ( s("/opt/Natron/Resources/docs/html"), root().mid( root().indexOf( s("/opt/") ) ) );
}

TEST(DocumentationServer, RefusesFilesOutsideDocDir)
{
    const char* const pages[] = {
        "../../../../../etc/hosts.txt",
        "plugins/../../html.txt",
        "_static/../../../bin/Natron.png",
        "..",
        "./../index.html",
        "..\\..\\secret.txt", // a separator on Windows
        "C:/Windows/win.ini.txt",
        "C:secret.txt",
        "index.html::$DATA", // Windows alternate data stream
        "",
    };

    for (const char* page : pages) {
        EXPECT_EQ( QString(), DocumentationServer::resolveDocumentationFile( s(kDocDir), s(page) ) ) << page;
    }
    // a NUL character
    EXPECT_EQ( QString(), DocumentationServer::resolveDocumentationFile( s(kDocDir), s("index.html") + QChar() + s(".png") ) );
    // a directory next to docDir whose name starts with docDir's name
    EXPECT_EQ( QString(), DocumentationServer::resolveDocumentationFile( s(kDocDir), s("../html2/index.html") ) );
}

// "/LOCAL_FILE/<path>": only the plug-in icons (any file was given)
TEST(DocumentationServer, ResolvesOnlyAllowedLocalFiles)
{
    const QStringList icons = QStringList() << s("/opt/Natron/Plugins/OFX/Natron/Misc.ofx.bundle/Contents/Resources/net.sf.openfx.Blur.png")
                                            << s("/Users/me/My PyPlugs/MyPlug.png")
                                            << QDir::toNativeSeparators( s("/opt/plugins/icon.png") );

    EXPECT_EQ( icons.at(0), DocumentationServer::resolveLocalFile(s("/opt/Natron/Plugins/OFX/Natron/Misc.ofx.bundle/Contents/Resources/net.sf.openfx.Blur.png"), icons) );
    // a space, percent-encoded twice by QHttpRequest
    EXPECT_EQ( icons.at(1), DocumentationServer::resolveLocalFile(s("/Users/me/My%2520PyPlugs/MyPlug.png"), icons) );
    EXPECT_EQ( icons.at(1), DocumentationServer::resolveLocalFile(s("/Users/me/My%20PyPlugs/MyPlug.png"), icons) );
    EXPECT_EQ( icons.at(2), DocumentationServer::resolveLocalFile(s("/opt/plugins/icon.png"), icons) );

    EXPECT_EQ( QString(), DocumentationServer::resolveLocalFile(s("/etc/hosts.txt"), icons) );
    EXPECT_EQ( QString(), DocumentationServer::resolveLocalFile(s("/Users/me/.ssh/id_rsa.txt"), icons) );
    EXPECT_EQ( QString(), DocumentationServer::resolveLocalFile(s("/Users/me/My%2520PyPlugs/Other.png"), icons) );
    EXPECT_EQ( QString(), DocumentationServer::resolveLocalFile( QString(), icons ) );
    EXPECT_EQ( QString(), DocumentationServer::resolveLocalFile( s("/opt/plugins/icon.png"), QStringList() ) );
}

// DNS rebinding: a web page whose host name resolves to 127.0.0.1 sends its own host name
TEST(DocumentationServer, AcceptsOnlyLoopbackHostHeaders)
{
    const int port = 49152;

    EXPECT_TRUE( DocumentationServer::isLoopbackHost(s("localhost:49152"), port) );
    EXPECT_TRUE( DocumentationServer::isLoopbackHost(s("localhost"), port) );
    EXPECT_TRUE( DocumentationServer::isLoopbackHost(s("LocalHost:49152"), port) );
    EXPECT_TRUE( DocumentationServer::isLoopbackHost(s("127.0.0.1:49152"), port) );
    EXPECT_TRUE( DocumentationServer::isLoopbackHost(s("[::1]:49152"), port) );
    EXPECT_TRUE( DocumentationServer::isLoopbackHost(s(" localhost:49152 "), port) );

    const char* const refused[] = {
        "", // HTTP/1.1 requires a Host header
        "evil.example.com:49152",
        "evil.example.com",
        "localhost.evil.example.com:49152",
        "127.0.0.1.nip.io:49152",
        "localhost:8080", // another port
        "localhost:",
        "localhost:+49152",
        "localhost:49152abc",
        "localhost:49152:1",
        "localhost, evil.example.com", // two Host headers (QHttpRequest joins them)
        "[::1",
        "[::1]x",
        "[::2]:49152",
        "0.0.0.0:49152",
        "192.168.1.2:49152",
        "user@localhost:49152",
    };
    for (const char* host : refused) {
        EXPECT_FALSE( DocumentationServer::isLoopbackHost(s(host), port) ) << host;
    }
}

// Fetch Metadata: requests made by other sites
TEST(DocumentationServer, DetectsCrossSiteRequests)
{
    EXPECT_TRUE( DocumentationServer::isCrossSiteRequest( s("cross-site") ) );
    EXPECT_TRUE( DocumentationServer::isCrossSiteRequest( s("same-site") ) ); // e.g. localhost:8080
    EXPECT_FALSE( DocumentationServer::isCrossSiteRequest( s("same-origin") ) ); // links of the documentation
    EXPECT_FALSE( DocumentationServer::isCrossSiteRequest( s("none") ) ); // opened by the user (or Natron)
    EXPECT_FALSE( DocumentationServer::isCrossSiteRequest( QString() ) ); // browsers without Fetch Metadata
}
