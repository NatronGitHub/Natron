#include "StandardPaths.h"

#include <QCoreApplication>
#include <QDir>
#if QT_VERSION < 0x050000

#else
#include <QStandardPaths>
#endif

#include "Global/Macros.h"

#ifdef __NATRON_OSX__
#include <CoreServices/CoreServices.h>
#elif defined(__NATRON_WIN32__)
#include <windows.h>
#include <IntShCut.h>
#include <ShlObj.h>
#ifndef CSIDL_MYMUSIC
#define CSIDL_MYMUSIC 13
#define CSIDL_MYVIDEO 14
#endif
#include <QFileInfo>
#elif defined(__NATRON_LINUX__)
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
CLANG_DIAG_OFF(deprecated)
#include <QTextStream>
#include <QHash>
#include <QVarLengthArray>
CLANG_DIAG_ON(deprecated)

#endif

namespace Natron {


StandardPaths::StandardPaths()
{
}

#if QT_VERSION < 0x050000
static void appendOrganizationAndApp(QString &path)
{
#ifndef QT_BOOTSTRAPPED
    const QString org = QCoreApplication::organizationName();
    if (!org.isEmpty())
        path += QLatin1Char('/') + org;
    const QString appName = QCoreApplication::applicationName();
    if (!appName.isEmpty())
        path += QLatin1Char('/') + appName;
#else
    Q_UNUSED(path);
#endif
}
#endif // QT_VERSION < 0x050000

namespace {

#ifdef __NATRON_OSX__
CLANG_DIAG_OFF(deprecated)

static
OSType translateLocation(StandardPaths::StandardLocation type)
{
    switch (type) {
    case Natron::StandardPaths::ConfigLocation:
        return kPreferencesFolderType;
    case Natron::StandardPaths::DesktopLocation:
        return kDesktopFolderType;
    case Natron::StandardPaths::DownloadLocation: // needs NSSearchPathForDirectoriesInDomains with NSDownloadsDirectory
        // which needs an objective-C *.mm file...
    case Natron::StandardPaths::DocumentsLocation:
        return kDocumentsFolderType;
    case Natron::StandardPaths::FontsLocation:
        // There are at least two different font directories on the mac: /Library/Fonts and ~/Library/Fonts.
        // To select a specific one we have to specify a different first parameter when calling FSFindFolder.
        return kFontsFolderType;
    case Natron::StandardPaths::ApplicationsLocation:
        return kApplicationsFolderType;
    case Natron::StandardPaths::MusicLocation:
        return kMusicDocumentsFolderType;
    case Natron::StandardPaths::MoviesLocation:
        return kMovieDocumentsFolderType;
    case Natron::StandardPaths::PicturesLocation:
        return kPictureDocumentsFolderType;
    case Natron::StandardPaths::TempLocation:
        return kTemporaryFolderType;
    case Natron::StandardPaths::GenericDataLocation:
    case Natron::StandardPaths::RuntimeLocation:
    case Natron::StandardPaths::DataLocation:
        return kApplicationSupportFolderType;
    case Natron::StandardPaths::GenericCacheLocation:
    case Natron::StandardPaths::CacheLocation:
        return kCachedDataFolderType;
    default:
        return kDesktopFolderType;
    }
}


/*
     Constructs a full unicode path from a FSRef.
     */
static QString getFullPath(const FSRef &ref)
{
    QByteArray ba(2048, 0);
    if (FSRefMakePath(&ref, reinterpret_cast<UInt8 *>(ba.data()), ba.size()) == noErr)
        return QString::fromUtf8(ba.constData()).normalized(QString::NormalizationForm_C);
    return QString();
}

static QString macLocation(StandardPaths::StandardLocation type, short domain)
{
    // http://developer.apple.com/documentation/Carbon/Reference/Folder_Manager/Reference/reference.html
    FSRef ref;
    OSErr err = FSFindFolder(domain, translateLocation(type), false, &ref);
    if (err)
        return QString();

    QString path = getFullPath(ref);

    if (type == Natron::StandardPaths::DataLocation || type == Natron::StandardPaths::CacheLocation)
        appendOrganizationAndApp(path);
    return path;
}

#elif defined(__NATRON_WIN32__)
static QString qSystemDirectory()
{
    QVarLengthArray<char, MAX_PATH> fullPath;

    UINT retLen = ::GetSystemDirectory(fullPath.data(), MAX_PATH);
    if (retLen > MAX_PATH) {
        fullPath.resize(retLen);
        retLen = ::GetSystemDirectory(fullPath.data(), retLen);
    }
    // in some rare cases retLen might be 0
    return QString::fromAscii(fullPath.constData(), int(retLen));
}

static HINSTANCE load(const wchar_t *libraryName, bool onlySystemDirectory = true){
    QStringList searchOrder;

#if !defined(QT_BOOTSTRAPPED)
    if (!onlySystemDirectory)
        searchOrder << QFileInfo(QCoreApplication::applicationFilePath()).path();
#endif
    searchOrder << qSystemDirectory();

    if (!onlySystemDirectory) {
        const QString PATH(QLatin1String(qgetenv("PATH").constData()));
        searchOrder << PATH.split(QLatin1Char(';'), QString::SkipEmptyParts);
    }
    QString fileName = QString::fromWCharArray(libraryName);
    fileName.append(QLatin1String(".dll"));

    // Start looking in the order specified
    for (int i = 0; i < searchOrder.count(); ++i) {
        QString fullPathAttempt = searchOrder.at(i);
        if (!fullPathAttempt.endsWith(QLatin1Char('\\'))) {
            fullPathAttempt.append(QLatin1Char('\\'));
        }
        fullPathAttempt.append(fileName);
        HINSTANCE inst = ::LoadLibrary((LPCSTR)fullPathAttempt.toStdString().c_str());
        if (inst != 0)
            return inst;
    }
    return 0;
}

typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, LPWSTR, int, BOOL);
static GetSpecialFolderPath resolveGetSpecialFolderPath()
{
    static GetSpecialFolderPath gsfp = 0;
    if (!gsfp) {
		
#ifndef Q_OS_WINCE
       QString lib("shell32");
#else
       QString lib("coredll");
#endif // Q_OS_WINCE
	   HINSTANCE libHandle = load((const wchar_t*)lib.utf16());
	   if(libHandle) {
		 gsfp = (GetSpecialFolderPath)(void*)GetProcAddress(libHandle,"SHGetSpecialFolderPathW");
	   }
    }
    return gsfp;
}

static QString convertCharArray(const wchar_t *path)
{
    return QDir::fromNativeSeparators(QString::fromWCharArray(path));
}
#elif defined(__NATRON_LINUX__)
//static
QString resolveUserName(uint userId)
{
#if !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD)
    int size_max = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (size_max == -1)
        size_max = 1024;
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
    if (pw)
        return QFile::decodeName(QByteArray(pw->pw_name));
    return QString();
}


#endif

}

QString StandardPaths::writableLocation(StandardLocation type) {
#if QT_VERSION < 0x050000
#ifdef __NATRON_OSX__
    switch (type) {
    case HomeLocation:
        return QDir::homePath();
    case TempLocation:
        return QDir::tempPath();
    case GenericDataLocation:
    case DataLocation:
    case GenericCacheLocation:
    case CacheLocation:
    case RuntimeLocation:
        return macLocation(type, kUserDomain);
    default:
        return macLocation(type, kOnAppropriateDisk);
    }
#elif defined(__NATRON_LINUX__)
    switch (type) {
    case HomeLocation:
        return QDir::homePath();
    case TempLocation:
        return QDir::tempPath();
    case CacheLocation:
    case GenericCacheLocation:
    {
        // http://standards.freedesktop.org/basedir-spec/basedir-spec-0.6.html
        QString xdgCacheHome = QFile::decodeName(qgetenv("XDG_CACHE_HOME"));
        if (xdgCacheHome.isEmpty())
            xdgCacheHome = QDir::homePath() + QLatin1String("/.cache");
        if (type == Natron::StandardPaths::CacheLocation)
            appendOrganizationAndApp(xdgCacheHome);
        return xdgCacheHome;
    }
    case DataLocation:
    case GenericDataLocation:
    {
        QString xdgDataHome = QFile::decodeName(qgetenv("XDG_DATA_HOME"));
        if (xdgDataHome.isEmpty())
            xdgDataHome = QDir::homePath() + QLatin1String("/.local/share");
        if (type == Natron::StandardPaths::DataLocation)
            appendOrganizationAndApp(xdgDataHome);
        return xdgDataHome;
    }
    case ConfigLocation:
    {
        // http://standards.freedesktop.org/basedir-spec/latest/
        QString xdgConfigHome = QFile::decodeName(qgetenv("XDG_CONFIG_HOME"));
        if (xdgConfigHome.isEmpty())
            xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
        return xdgConfigHome;
    }
    case RuntimeLocation:
    {
        const uid_t myUid = geteuid();
        // http://standards.freedesktop.org/basedir-spec/latest/
        QString xdgRuntimeDir = QFile::decodeName(qgetenv("XDG_RUNTIME_DIR"));
        if (xdgRuntimeDir.isEmpty()) {
            const QString userName = resolveUserName(myUid);
            xdgRuntimeDir = QDir::tempPath() + QLatin1String("/runtime-") + userName;
            QDir dir(xdgRuntimeDir);
            if (!dir.exists()) {
                if (!QDir().mkdir(xdgRuntimeDir)) {
                    qWarning("QStandardPaths: error creating runtime directory %s: %s", qPrintable(xdgRuntimeDir), qPrintable(qt_error_string(errno)));
                    return QString();
                }
            }
            qWarning("QStandardPaths: XDG_RUNTIME_DIR not set, defaulting to '%s'", qPrintable(xdgRuntimeDir));
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
        if (file.permissions() != wantedPerms && !file.setPermissions(wantedPerms)) {
            qWarning("QStandardPaths: wrong permissions on runtime directory %s", qPrintable(xdgRuntimeDir));
            return QString();
        }
        return xdgRuntimeDir;
    }
    default:
        break;
    }
    
#ifndef QT_BOOTSTRAPPED
    // http://www.freedesktop.org/wiki/Software/xdg-user-dirs
    QString xdgConfigHome = QFile::decodeName(qgetenv("XDG_CONFIG_HOME"));
    if (xdgConfigHome.isEmpty())
        xdgConfigHome = QDir::homePath() + QLatin1String("/.config");
    QFile file(xdgConfigHome + QLatin1String("/user-dirs.dirs"));
    if (file.open(QIODevice::ReadOnly)) {
        QHash<QString, QString> lines;
        QTextStream stream(&file);
        // Only look for lines like: XDG_DESKTOP_DIR="$HOME/Desktop"
        QRegExp exp(QLatin1String("^XDG_(.*)_DIR=(.*)$"));
        while (!stream.atEnd()) {
            const QString &line = stream.readLine();
            if (exp.indexIn(line) != -1) {
                const QStringList lst = exp.capturedTexts();
                const QString key = lst.at(1);
                QString value = lst.at(2);
                if (value.length() > 2
                        && value.startsWith(QLatin1Char('\"'))
                        && value.endsWith(QLatin1Char('\"')))
                    value = value.mid(1, value.length() - 2);
                // Store the key and value: "DESKTOP", "$HOME/Desktop"
                lines[key] = value;
            }
        }
        
        QString key;
        switch (type) {
        case DesktopLocation:
            key = QLatin1String("DESKTOP");
            break;
        case DocumentsLocation:
            key = QLatin1String("DOCUMENTS");
            break;
        case PicturesLocation:
            key = QLatin1String("PICTURES");
            break;
        case MusicLocation:
            key = QLatin1String("MUSIC");
            break;
        case MoviesLocation:
            key = QLatin1String("VIDEOS");
            break;
        case DownloadLocation:
            key = QLatin1String("DOWNLOAD");
            break;
        default:
            break;
        }
        if (!key.isEmpty()) {
            QString value = lines.value(key);
            if (!value.isEmpty()) {
                // value can start with $HOME
                if (value.startsWith(QLatin1String("$HOME")))
                    value = QDir::homePath() + value.mid(5);
                return value;
            }
        }
    }
#endif
    
    QString path;
    switch (type) {
    case DesktopLocation:
        path = QDir::homePath() + QLatin1String("/Desktop");
        break;
    case DocumentsLocation:
        path = QDir::homePath() + QLatin1String("/Documents");
        break;
    case PicturesLocation:
        path = QDir::homePath() + QLatin1String("/Pictures");
        break;

    case FontsLocation:
        path = QDir::homePath() + QLatin1String("/.fonts");
        break;

    case MusicLocation:
        path = QDir::homePath() + QLatin1String("/Music");
        break;

    case MoviesLocation:
        path = QDir::homePath() + QLatin1String("/Videos");
        break;
    case DownloadLocation:
        path = QDir::homePath() + QLatin1String("/Downloads");
        break;
    case ApplicationsLocation:
        path = writableLocation(GenericDataLocation) + QLatin1String("/applications");
        break;

    default:
        break;
    }
    
    return path;
#elif defined(__NATRON_WIN32__)
    QString result;
    
    static GetSpecialFolderPath SHGetSpecialFolderPath = resolveGetSpecialFolderPath();
    if (!SHGetSpecialFolderPath)
        return QString();
    
    wchar_t path[MAX_PATH];
    
    switch (type) {
    case ConfigLocation: // same as DataLocation, on Windows
    case DataLocation:
    case GenericDataLocation:
#if defined Q_OS_WINCE
        if (SHGetSpecialFolderPath(0, path, CSIDL_APPDATA, FALSE))
#else
        if (SHGetSpecialFolderPath(0, path, CSIDL_LOCAL_APPDATA, FALSE))
#endif
            result = convertCharArray(path);
#ifndef QT_BOOTSTRAPPED
        if (type != GenericDataLocation) {
            if (!QCoreApplication::organizationName().isEmpty())
                result += QLatin1Char('/') + QCoreApplication::organizationName();
            if (!QCoreApplication::applicationName().isEmpty())
                result += QLatin1Char('/') + QCoreApplication::applicationName();
        }
#endif
        break;

    case DesktopLocation:
        if (SHGetSpecialFolderPath(0, path, CSIDL_DESKTOPDIRECTORY, FALSE))
            result = convertCharArray(path);
        break;

    case DownloadLocation: // TODO implement with SHGetKnownFolderPath(FOLDERID_Downloads) (starting from Vista)
    case DocumentsLocation:
        if (SHGetSpecialFolderPath(0, path, CSIDL_PERSONAL, FALSE))
            result = convertCharArray(path);
        break;

    case FontsLocation:
        if (SHGetSpecialFolderPath(0, path, CSIDL_FONTS, FALSE))
            result = convertCharArray(path);
        break;

    case ApplicationsLocation:
        if (SHGetSpecialFolderPath(0, path, CSIDL_PROGRAMS, FALSE))
            result = convertCharArray(path);
        break;

    case MusicLocation:
        if (SHGetSpecialFolderPath(0, path, CSIDL_MYMUSIC, FALSE))
            result = convertCharArray(path);
        break;

    case MoviesLocation:
        if (SHGetSpecialFolderPath(0, path, CSIDL_MYVIDEO, FALSE))
            result = convertCharArray(path);
        break;

    case PicturesLocation:
        if (SHGetSpecialFolderPath(0, path, CSIDL_MYPICTURES, FALSE))
            result = convertCharArray(path);
        break;

    case CacheLocation:
        // Although Microsoft has a Cache key it is a pointer to IE's cache, not a cache
        // location for everyone.  Most applications seem to be using a
        // cache directory located in their AppData directory
        return writableLocation(DataLocation) + QLatin1String("/cache");

    case GenericCacheLocation:
        return writableLocation(GenericDataLocation) + QLatin1String("/cache");

    case RuntimeLocation:
    case HomeLocation:
        result = QDir::homePath();
        break;

    case TempLocation:
        result = QDir::tempPath();
        break;
    }
    return result;
#else
#error "Unsupported operating system"
#endif
    
#else // QT_VERSION >= 0x050000
    QStandardPaths::StandardLocation path;
    switch (type) {
    case Natron::StandardPaths::DesktopLocation :
        path = QStandardPaths::DesktopLocation;
        break;
    case Natron::StandardPaths::DocumentsLocation :
        path = QStandardPaths::DocumentsLocation;
        break;
    case Natron::StandardPaths::FontsLocation :
        path = QStandardPaths::FontsLocation;
        break;
    case Natron::StandardPaths::ApplicationsLocation :
        path = QStandardPaths::ApplicationsLocation;
        break;
    case Natron::StandardPaths::MusicLocation :
        path = QStandardPaths::MusicLocation;
        break;
    case Natron::StandardPaths::MoviesLocation :
        path = QStandardPaths::MoviesLocation;
        break;
    case Natron::StandardPaths::PicturesLocation :
        path = QStandardPaths::PicturesLocation;
        break;
    case Natron::StandardPaths::TempLocation :
        path = QStandardPaths::TempLocation;
        break;
    case Natron::StandardPaths::HomeLocation :
        path = QStandardPaths::HomeLocation;
        break;
    case Natron::StandardPaths::DataLocation :
        path = QStandardPaths::DataLocation;
        break;
    case Natron::StandardPaths::CacheLocation :
        path = QStandardPaths::CacheLocation;
        break;
    default:
        break;
    }
    return QStandardPaths::writableLocation(path);
#endif // QT_VERSION >= 0x050000
}

} //namespace Natron
