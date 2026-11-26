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
 * Pure functions, so that they can be tested without a server.
 **/
namespace DocumentationServer {
/**
 * @brief The file that serves page (a path relative to docDir), or an empty string if it is outside
 * docDir.
 *
 * The path is resolved lexically: a ".." segment that leaves docDir is refused, as are backslashes,
 * colons (drive letters, Windows alternate data streams) and NUL characters. Symbolic links inside
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
