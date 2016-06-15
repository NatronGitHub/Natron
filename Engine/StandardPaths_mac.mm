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

#include "Engine/StandardPaths.h"
#ifdef Q_OS_MAC

#include <QDir>

#include <CoreServices/CoreServices.h>


CLANG_DIAG_OFF(deprecated)

static
OSType
translateLocation(NATRON_NAMESPACE::StandardPaths::StandardLocationEnum type)
{
    switch (type) {
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationConfig:

            return kPreferencesFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationDesktop:

            return kDesktopFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationDownload: // needs NSSearchPathForDirectoriesInDomains with NSDownloadsDirectory
            // which needs an objective-C *.mm file...
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationDocuments:

            return kDocumentsFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationFonts:
            // There are at least two different font directories on the mac: /Library/Fonts and ~/Library/Fonts.

            // To select a specific one we have to specify a different first parameter when calling FSFindFolder.
            return kFontsFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationApplications:

            return kApplicationsFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationMusic:

            return kMusicDocumentsFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationMovies:

            return kMovieDocumentsFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationPictures:

            return kPictureDocumentsFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationTemp:

            return kTemporaryFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationGenericData:
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationRuntime:
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationData:

            return kApplicationSupportFolderType;
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationGenericCache:
        case NATRON_NAMESPACE::StandardPaths::eStandardLocationCache:
            
            return kCachedDataFolderType;
        default:
            
            return kDesktopFolderType;
    }
}


/*
 Constructs a full unicode path from a FSRef.
 */
static QString
getFullPath(const FSRef &ref)
{
    QByteArray ba(2048, 0);

    if (FSRefMakePath( &ref, reinterpret_cast<UInt8 *>( ba.data() ), ba.size() ) == noErr) {
        return QString::fromUtf8( ba.constData() ).normalized(QString::NormalizationForm_C);
    }

    return QString();
}


static QString
macLocation(NATRON_NAMESPACE::StandardPaths::StandardLocationEnum type, short domain)
{
    // http://developer.apple.com/documentation/Carbon/Reference/Folder_Manager/Reference/reference.html
    FSRef ref;
    OSErr err = FSFindFolder(domain, translateLocation(type), false, &ref);

    if (err) {
        return QString();
    }

    QString path = getFullPath(ref);

    if ( (type == NATRON_NAMESPACE::StandardPaths::eStandardLocationData) || (type == NATRON_NAMESPACE::StandardPaths::eStandardLocationCache) ) {
        NATRON_NAMESPACE::StandardPaths::appendOrganizationAndApp(path);
    }

    return path;
}


NATRON_NAMESPACE_ENTER;



QString
StandardPaths::writableLocation_mac_qt4(StandardLocationEnum type)
{
#if QT_VERSION < 0x050000
    switch (type) {
        case eStandardLocationHome:

            return QDir::homePath();
        case eStandardLocationTemp:

            return QDir::tempPath();
        case eStandardLocationGenericData:
        case eStandardLocationData:
        case eStandardLocationGenericCache:
        case eStandardLocationCache:
        case eStandardLocationRuntime:

            return macLocation(type, kUserDomain);
        default:
            
            return macLocation(type, kOnAppropriateDisk);
    }
#else
    Q_UNUSED(type);
#endif
}


NATRON_NAMESPACE_EXIT;

#endif
