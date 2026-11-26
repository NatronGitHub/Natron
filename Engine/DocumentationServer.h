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

#ifndef Engine_DocumentationServer_h
#define Engine_DocumentationServer_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QString>
#include <QStringList>

NATRON_NAMESPACE_ENTER

/**
 * @brief The request checks of the local documentation server (Gui/DocumentationManager.cpp).
 *
 * The server listens on 127.0.0.1 only, and its pages are only meant for the user's browser, opened by
 * Natron (Help menu, "?" buttons of the node panels and of the preferences). Pure functions, so that
 * they can be tested without a server.
 **/
namespace DocumentationServer {
/**
 * @brief Whether hostHeader, the Host header of a request, names this loopback server: "localhost",
 * "127.0.0.1" or "[::1]", optionally followed by ":serverPort".
 *
 * Any other name is refused: a web page whose host name resolves to 127.0.0.1 (DNS rebinding) would
 * otherwise read the server's answers with the page's own origin.
 **/
bool isLoopbackHost(const QString& hostHeader, int serverPort);

/**
 * @brief Whether a request with this Sec-Fetch-Site header comes from another site ("cross-site", or
 * "same-site": another port or another subdomain of localhost).
 *
 * The browser sends "none" when the user (or Natron) opens a URL, and "same-origin" from the
 * documentation pages themselves. Browsers without Fetch Metadata send no header (empty value).
 **/
bool isCrossSiteRequest(const QString& secFetchSite);

/**
 * @brief The file that serves page (a path relative to docDir), or an empty string if it is outside
 * docDir.
 *
 * The path is resolved lexically: a ".." segment that leaves docDir is refused, as are backslashes,
 * colons (drive letters, Windows alternate data streams), NUL characters and Windows device names
 * (CON, NUL, COM1, ... with any extension). Symbolic links inside
 * docDir are followed: they are part of the installed documentation.
 **/
QString resolveDocumentationFile(const QString& docDir, const QString& page);

/**
 * @brief The local file that serves "/LOCAL_FILE/<path>" URLs (the plug-in icons in the generated
 * plug-in pages, see NodeDocumentation.cpp), or an empty string if path is none of allowedFiles.
 *
 * path is percent-decoded (QHttpRequest gives it percent-encoded twice: "%2520" for a space).
 **/
QString resolveLocalFile(const QString& path, const QStringList& allowedFiles);
} // namespace DocumentationServer

NATRON_NAMESPACE_EXIT

#endif // Engine_DocumentationServer_h
