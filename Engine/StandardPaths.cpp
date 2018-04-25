/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#endif

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

#elif defined(Q_OS_LINUX)
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

#elif defined(Q_OS_MAC)
#include <CoreServices/CoreServices.h>

#endif

NATRON_NAMESPACE_ENTER

StandardPaths::StandardPaths()
{
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
void
StandardPaths::appendOrganizationAndApp(QString &path)
{
#ifndef QT_BOOTSTRAPPED
    const QString org = QCoreApplication::organizationName();
    if ( !org.isEmpty() ) {
        path += QLatin1Char('/') + org;
    }
    const QString appName = QCoreApplication::applicationName();
    if ( !appName.isEmpty() ) {
        path += QLatin1Char('/') + appName;
    }
#else
    Q_UNUSED(path);
#endif
}

#endif // QT_VERSION < QT_VERSION_CHECK(5, 0, 0)


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

#elif defined(Q_OS_LINUX)
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

#if defined(Q_OS_MAC)
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

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    if ( (type == NATRON_NAMESPACE::StandardPaths::eStandardLocationData) || (type == NATRON_NAMESPACE::StandardPaths::eStandardLocationCache) ) {
        NATRON_NAMESPACE::StandardPaths::appendOrganizationAndApp(path);
    }
#endif

    return path;
}

CLANG_DIAG_ON(deprecated)
#endif // defined(Q_OS_MAC)

NATRON_NAMESPACE_ANONYMOUS_EXIT


QString
StandardPaths::writableLocation(StandardLocationEnum type)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#if defined(Q_OS_MAC)
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
#elif defined(Q_OS_LINUX)
    switch (type) {
    case eStandardLocationHome:

        return QDir::homePath();
    case eStandardLocationTemp:

        return QDir::tempPath();
    case eStandardLocationCache:
    case eStandardLocationGenericCache: {
        // http://standards.freedesktop.org/basedir-spec/basedir-spec-0.6.html
        QString xdgCacheHome = QFile::decodeName( qgetenv("XDG_CACHE_HOME") );
        if ( xdgCacheHome.isEmpty() ) {
            xdgCacheHome = QDir::homePath() + QLatin1String("/.cache");
        }
        if (type == StandardPaths::eStandardLocationCache) {
            appendOrganizationAndApp(xdgCacheHome);
        }

        return xdgCacheHome;
    }
    case eStandardLocationData:
    case eStandardLocationGenericData: {
        QString xdgDataHome = QFile::decodeName( qgetenv("XDG_DATA_HOME") );
        if ( xdgDataHome.isEmpty() ) {
            xdgDataHome = QDir::homePath() + QLatin1String("/.local/share");
        }
        if (type == StandardPaths::eStandardLocationData) {
            appendOrganizationAndApp(xdgDataHome);
        }

        return xdgDataHome;
    }
    case eStandardLocationConfig: {
        // http://standards.freedesktop.org/basedir-spec/latest/
        QString xdgConfigHome = QFile::decodeName( qgetenv("XDG_CONFIG_HOME") );
        if ( xdgConfigHome.isEmpty() ) {
            xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
        }

        return xdgConfigHome;
    }
    case eStandardLocationRuntime: {
        const uid_t myUid = geteuid();
        // http://standards.freedesktop.org/basedir-spec/latest/
        QString xdgRuntimeDir = QFile::decodeName( qgetenv("XDG_RUNTIME_DIR") );
        if ( xdgRuntimeDir.isEmpty() ) {
            const QString userName = resolveUserName(myUid);
            xdgRuntimeDir = QDir::tempPath() + QLatin1String("/runtime-") + userName;
            QDir dir(xdgRuntimeDir);
            if ( !dir.exists() ) {
                if ( !QDir().mkdir(xdgRuntimeDir) ) {
                    qWarning( "QStandardPaths: error creating runtime directory %s: %s", qPrintable(xdgRuntimeDir), qPrintable( qt_error_string(errno) ) );

                    return QString();
                }
            }
            qWarning( "QStandardPaths: XDG_RUNTIME_DIR not set, defaulting to '%s'", qPrintable(xdgRuntimeDir) );
        }
        // "The directory MUST be owned by the user"
        QFileInfo fileInfo(xdgRuntimeDir);
        if (fileInfo.ownerId() != myUid) {
            qWarning("QStandardPaths: wrong ownership on runtime directory %s, %d instead of %d", qPrintable(xdgRuntimeDir),
                     fileInfo.ownerId(), myUid);

            return QString();
        }
        // "and he MUST be the only one having read and write access to it. Its Unix access mode MUST be 0700."
        QFile file(xdgRuntimeDir);
        const QFile::Permissions wantedPerms = QFile::ReadUser | QFile::WriteUser | QFile::ExeUser;
        if ( (file.permissions() != wantedPerms) && !file.setPermissions(wantedPerms) ) {
            qWarning( "QStandardPaths: wrong permissions on runtime directory %s", qPrintable(xdgRuntimeDir) );

            return QString();
        }

        return xdgRuntimeDir;
    }
    default:
        break;
    } // switch

#ifndef QT_BOOTSTRAPPED
    // http://www.freedesktop.org/wiki/Software/xdg-user-dirs
    QString xdgConfigHome = QFile::decodeName( qgetenv("XDG_CONFIG_HOME") );
    if ( xdgConfigHome.isEmpty() ) {
        xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
    }
    QFile file( xdgConfigHome + QLatin1String("/user-dirs.dirs") );
    if ( file.open(QIODevice::ReadOnly) ) {
        QHash<QString, QString> lines;
        QTextStream stream(&file);
        // Only look for lines like: XDG_DESKTOP_DIR="$HOME/Desktop"
        QRegExp exp( QLatin1String("^XDG_(.*)_DIR=(.*)$") );
        while ( !stream.atEnd() ) {
            const QString &line = stream.readLine();
            if (exp.indexIn(line) != -1) {
                const QStringList lst = exp.capturedTexts();
                const QString key = lst.at(1);
                QString value = lst.at(2);
                if ( (value.length() > 2)
                     && value.startsWith( QLatin1Char('\"') )
                     && value.endsWith( QLatin1Char('\"') ) ) {
                    value = value.mid(1, value.length() - 2);
                }
                // Store the key and value: "DESKTOP", "$HOME/Desktop"
                lines[key] = value;
            }
        }

        QString key;
        switch (type) {
        case eStandardLocationDesktop:
            key = QLatin1String("DESKTOP");
            break;
        case eStandardLocationDocuments:
            key = QLatin1String("DOCUMENTS");
            break;
        case eStandardLocationPictures:
            key = QLatin1String("PICTURES");
            break;
        case eStandardLocationMusic:
            key = QLatin1String("MUSIC");
            break;
        case eStandardLocationMovies:
            key = QLatin1String("VIDEOS");
            break;
        case eStandardLocationDownload:
            key = QLatin1String("DOWNLOAD");
            break;
        default:
            break;
        }
        if ( !key.isEmpty() ) {
            QString value = lines.value(key);
            if ( !value.isEmpty() ) {
                // value can start with $HOME
                if ( value.startsWith( QLatin1String("$HOME") ) ) {
                    value = QDir::homePath() + value.mid(5);
                }

                return value;
            }
        }
    }
#endif // ifndef QT_BOOTSTRAPPED

    QString path;
    switch (type) {
    case eStandardLocationDesktop:
        path = QDir::homePath() + QLatin1String("/Desktop");
        break;
    case eStandardLocationDocuments:
        path = QDir::homePath() + QLatin1String("/Documents");
        break;
    case eStandardLocationPictures:
        path = QDir::homePath() + QLatin1String("/Pictures");
        break;

    case eStandardLocationFonts:
        path = QDir::homePath() + QLatin1String("/.fonts");
        break;

    case eStandardLocationMusic:
        path = QDir::homePath() + QLatin1String("/Music");
        break;

    case eStandardLocationMovies:
        path = QDir::homePath() + QLatin1String("/Videos");
        break;
    case eStandardLocationDownload:
        path = QDir::homePath() + QLatin1String("/Downloads");
        break;
    case eStandardLocationApplications:
        path = writableLocation(eStandardLocationGenericData) + QLatin1String("/applications");
        break;

    default:
        break;
    }

    return path;
#elif defined(Q_OS_WIN)
    QString result;
    static GetSpecialFolderPath SHGetSpecialFolderPath = resolveGetSpecialFolderPath();
    if (!SHGetSpecialFolderPath) {
        return QString();
    }

    wchar_t path[MAX_PATH];

    switch (type) {
    case eStandardLocationConfig: // same as eStandardLocationData, on Windows
    case eStandardLocationData:
    case eStandardLocationGenericData:
#if defined Q_OS_WINCE
        if ( SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, FALSE) )
#else
        if ( SHGetSpecialFolderPath(0, path, CSIDL_LOCAL_APPDATA, FALSE) )
#endif
        {
            result = convertCharArray(path);
        }
#ifndef QT_BOOTSTRAPPED
        if (type != eStandardLocationGenericData) {
            if ( !QCoreApplication::organizationName().isEmpty() ) {
                result += QLatin1Char('/') + QCoreApplication::organizationName();
            }
            if ( !QCoreApplication::applicationName().isEmpty() ) {
                result += QLatin1Char('/') + QCoreApplication::applicationName();
            }
        }
#endif
        break;

    case eStandardLocationDesktop:
        if ( SHGetSpecialFolderPath(0, path, CSIDL_DESKTOPDIRECTORY, FALSE) ) {
            result = convertCharArray(path);
        }
        break;

    case eStandardLocationDownload: // TODO implement with SHGetKnownFolderPath(FOLDERID_Downloads) (starting from Vista)
    case eStandardLocationDocuments:
        if ( SHGetSpecialFolderPath(0, path, CSIDL_PERSONAL, FALSE) ) {
            result = convertCharArray(path);
        }
        break;

    case eStandardLocationFonts:
        if ( SHGetSpecialFolderPath(0, path, CSIDL_FONTS, FALSE) ) {
            result = convertCharArray(path);
        }
        break;

    case eStandardLocationApplications:
        if ( SHGetSpecialFolderPath(0, path, CSIDL_PROGRAMS, FALSE) ) {
            result = convertCharArray(path);
        }
        break;

    case eStandardLocationMusic:
        if ( SHGetSpecialFolderPath(0, path, CSIDL_MYMUSIC, FALSE) ) {
            result = convertCharArray(path);
        }
        break;

    case eStandardLocationMovies:
        if ( SHGetSpecialFolderPath(0, path, CSIDL_MYVIDEO, FALSE) ) {
            result = convertCharArray(path);
        }
        break;

    case eStandardLocationPictures:
        if ( SHGetSpecialFolderPath(0, path, CSIDL_MYPICTURES, FALSE) ) {
            result = convertCharArray(path);
        }
        break;

    case eStandardLocationCache:
        // Although Microsoft has a Cache key it is a pointer to IE's cache, not a cache
        // location for everyone.  Most applications seem to be using a

        // cache directory located in their AppData directory
        return writableLocation(eStandardLocationData) + QLatin1String("/cache");

    case eStandardLocationGenericCache:

        return writableLocation(eStandardLocationGenericData) + QLatin1String("/cache");

    case eStandardLocationRuntime:
    case eStandardLocationHome:
        result = QDir::homePath();
        break;

    case eStandardLocationTemp:
        result = QDir::tempPath();
        break;
    } // switch

    return result;
#else
#error "Unsupported operating system"
#endif 

#else // QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
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
#endif // QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
} // writableLocation

NATRON_NAMESPACE_EXIT
