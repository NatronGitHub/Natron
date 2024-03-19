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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QStandardPaths>

#include "Global/GlobalDefines.h"

#include <QtCore/QtGlobal> // for Q_OS_*
#if defined(Q_OS_WIN)
#include <windows.h>
#include <IntShCut.h>
#include <ShlObj.h>
#ifndef CSIDL_MYMUSIC
#define CSIDL_MYMUSIC 13
#define CSIDL_MYVIDEO 14
#endif
#include <QtCore/QFileInfo>

#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QTextStream>
#include <QHash>
#include <QVarLengthArray>
CLANG_DIAG_ON(deprecated)

#elif defined(Q_OS_DARWIN)
#include <CoreServices/CoreServices.h>

#endif

NATRON_NAMESPACE_ENTER

StandardPaths::StandardPaths()
{
}


NATRON_NAMESPACE_ANONYMOUS_ENTER

#if defined(Q_OS_WIN)
static QString
qSystemDirectory()
{
    QVarLengthArray<wchar_t, MAX_PATH> fullPath;
    UINT retLen = ::GetSystemDirectoryW(fullPath.data(), MAX_PATH);
    if (retLen > MAX_PATH) {
        fullPath.resize(retLen);
        retLen = ::GetSystemDirectoryW(fullPath.data(), retLen);
    }
    if (!fullPath.constData()) {
        return QString();
    }
    std::wstring ws(fullPath.constData(), retLen);
    return QString::fromStdWString(ws);
}

static HINSTANCE
load(const wchar_t *libraryName,
     bool onlySystemDirectory = true)
{
    QStringList searchOrder;

#if !defined(QT_BOOTSTRAPPED)
    if (!onlySystemDirectory) {
        searchOrder << QFileInfo( QCoreApplication::applicationFilePath() ).path();
    }
#endif
    searchOrder << qSystemDirectory();

    if (!onlySystemDirectory) {
        const QString PATH( QLatin1String( qgetenv("PATH").constData() ) );
#     ifdef __NATRON_WIN32__
        const QChar pathSep = QChar::fromLatin1(';');
#     else
        // This code is windows-only anyway, but this is here for consistency with other parts of the source
        const QChar pathSep = QChar::fromLatin1(':');
#     endif
        searchOrder << PATH.split(pathSep, QString::SkipEmptyParts);
    }
    QString fileName = QString::fromWCharArray(libraryName);
    fileName.append( QLatin1String(".dll") );

    // Start looking in the order specified
    Q_FOREACH(const QString &path, searchOrder) {
        QString fullPathAttempt = path;

        if ( !fullPathAttempt.endsWith( QLatin1Char('\\') ) ) {
            fullPathAttempt.append( QLatin1Char('\\') );
        }
        fullPathAttempt.append(fileName);

        std::wstring ws = fullPathAttempt.toStdWString();
        HINSTANCE inst = ::LoadLibraryW( ws.c_str() );

        if (inst != 0) {
            return inst;
        }
    }

    return 0;
}

typedef BOOL (WINAPI * GetSpecialFolderPath)(HWND, LPWSTR, int, BOOL);
static GetSpecialFolderPath
resolveGetSpecialFolderPath()
{
    static GetSpecialFolderPath gsfp = 0;

    if (!gsfp) {
#ifndef Q_OS_WINCE
        QString lib = QString::fromUtf8("shell32");
#else
        QString lib = QString::fromUtf8("coredll");
#endif // Q_OS_WINCE
        HINSTANCE libHandle = load( (const wchar_t*)lib.utf16() );
        if (libHandle) {
            gsfp = (GetSpecialFolderPath)(void*)GetProcAddress(libHandle, "SHGetSpecialFolderPathW");
        }
    }

    return gsfp;
}

static QString
convertCharArray(const wchar_t *path)
{
    return QDir::fromNativeSeparators( QString::fromWCharArray(path) );
}

#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
//static
QString
resolveUserName(uint userId)
{
#if !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD)
    int size_max = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (size_max == -1) {
        size_max = 1024;
    }
    QVarLengthArray<char, 1024> buf(size_max);
#endif

    struct passwd *pw = 0;
#if !defined(Q_OS_INTEGRITY)
#if !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD) && !defined(Q_OS_VXWORKS)
    struct passwd entry;
    getpwuid_r(userId, &entry, buf.data(), buf.size(), &pw);
#else
    pw = getpwuid(userId);
#endif
#endif
    if (pw) {
        return QFile::decodeName( QByteArray(pw->pw_name) );
    }

    return QString();
}

#endif // defined(Q_OS_LINUX)

#if defined(Q_OS_DARWIN)
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

    return getFullPath(ref);
}

CLANG_DIAG_ON(deprecated)
#endif // defined(Q_OS_DARWIN)

NATRON_NAMESPACE_ANONYMOUS_EXIT


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
        path = QStandardPaths::DataLocation;
        break;
    case StandardPaths::eStandardLocationCache:
        path = QStandardPaths::CacheLocation;
        break;
    default:
        break;
    }

    return QStandardPaths::writableLocation(path);
} // writableLocation

NATRON_NAMESPACE_EXIT
