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

#include "StandardPaths.h"

#include <cassert>
#include <stdexcept>

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QStandardPaths>

#include "Global/GlobalDefines.h"


NATRON_NAMESPACE_ENTER

StandardPaths::StandardPaths()
{
}

QString
StandardPaths::writableLocation(StandardLocationEnum type)
{
    QStandardPaths::StandardLocation path;
    switch (type) {
    case StandardPaths::eStandardLocationDesktop:
        path = QStandardPaths::DesktopLocation;
        break;
    case StandardPaths::eStandardLocationDocuments:
        path = QStandardPaths::DocumentsLocation;
        break;
    case StandardPaths::eStandardLocationFonts:
        path = QStandardPaths::FontsLocation;
        break;
    case StandardPaths::eStandardLocationApplications:
        path = QStandardPaths::ApplicationsLocation;
        break;
    case StandardPaths::eStandardLocationMusic:
        path = QStandardPaths::MusicLocation;
        break;
    case StandardPaths::eStandardLocationMovies:
        path = QStandardPaths::MoviesLocation;
        break;
    case StandardPaths::eStandardLocationPictures:
        path = QStandardPaths::PicturesLocation;
        break;
    case StandardPaths::eStandardLocationTemp:
        path = QStandardPaths::TempLocation;
        break;
    case StandardPaths::eStandardLocationHome:
        path = QStandardPaths::HomeLocation;
        break;
    case StandardPaths::eStandardLocationData:
        path = QStandardPaths::AppLocalDataLocation;
        break;
    case StandardPaths::eStandardLocationCache:
        path = QStandardPaths::CacheLocation;
        break;
    case StandardPaths::eStandardLocationGenericData:
        path = QStandardPaths::GenericDataLocation;
        break;
    case StandardPaths::eStandardLocationRuntime:
        path = QStandardPaths::RuntimeLocation;
        break;
    case StandardPaths::eStandardLocationConfig:
        path = QStandardPaths::ConfigLocation;
        break;
    case StandardPaths::eStandardLocationDownload:
        path = QStandardPaths::DownloadLocation;
        break;
    case StandardPaths::eStandardLocationGenericCache:
        path = QStandardPaths::GenericCacheLocation;
        break;
    default:
        return QString();
    }

    return QStandardPaths::writableLocation(path);
} // writableLocation

NATRON_NAMESPACE_EXIT
