// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <stdio.h>

#include "gtest/gtest.h"

#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QtGlobal>

#include "Engine/AppManager.h"
#include "Engine/CLArgs.h"

using namespace NATRON_NAMESPACE;

#if defined(_WIN32) && defined(UNICODE)
GTEST_API_ int wmain(int argc, wchar_t **argv); // declared, for -Wmissing-declarations
GTEST_API_ int wmain(int argc, wchar_t **argv)
#else
GTEST_API_ int main(int argc, char **argv)
#endif
{
    printf("Running main() from wmain.cpp\n");
    ::testing::InitGoogleTest(&argc, argv);
#ifdef NATRON_TESTS_OFX_PLUGIN_DIR
    // The OFX plug-in built for the tests (Tests/OfxTestPlugin.h), before the plug-in cache reads
    // OFX_PLUGIN_PATH.
    {
#ifdef _WIN32
        const char sep = ';';
#else
        const char sep = ':';
#endif
        QByteArray path(NATRON_TESTS_OFX_PLUGIN_DIR);
        const QByteArray userPath = qgetenv("OFX_PLUGIN_PATH");
        if ( !userPath.isEmpty() ) {
            path += sep;
            path += userPath;
        }
        qputenv("OFX_PLUGIN_PATH", path);
    }
#endif
    // The tests never use the user's caches (#1569): the OFX plug-in cache file
    // (OFXLoadCache/OFXCache_<version>.xml) is the same for every build of a version, so two test runs at the
    // same time would rewrite each other's. NATRON_DISK_CACHE_PATH, if set, is kept. Otherwise ctest gives
    // Tests and TestsOfxCache the same directory of the build tree (NATRON_TESTS_CACHE_DIR), since
    // TestsOfxCache reads the OFX plug-in cache that Tests wrote; a run by hand uses a temporary
    // directory, removed at exit (destroyed after the AppManager): with NATRON_TESTS_KEEP_OFX_CACHE, set
    // one of the two variables to the directory of the previous run.
    std::unique_ptr<QTemporaryDir> tmpCacheDir;
    if ( qgetenv(NATRON_DISK_CACHE_PATH_ENV_VAR).isEmpty() ) {
        QByteArray cacheDir = qgetenv("NATRON_TESTS_CACHE_DIR");
        if ( cacheDir.isEmpty() ) {
            tmpCacheDir.reset( new QTemporaryDir( QDir::tempPath() + QString::fromUtf8("/NatronTests-cache-XXXXXX") ) );
            if ( !tmpCacheDir->isValid() ) {
                printf( "Cannot create a temporary cache directory in %s\n", QDir::tempPath().toUtf8().constData() );

                return 1;
            }
            cacheDir = tmpCacheDir->path().toUtf8();
        }
        qputenv(NATRON_DISK_CACHE_PATH_ENV_VAR, cacheDir);
    }
    printf( "Disk caches in %s\n", qgetenv(NATRON_DISK_CACHE_PATH_ENV_VAR).constData() );
    // The auto-saves that the tests write (in Project::autoSavesDir()) do not go to the user's
    // auto-saves, which a Natron started meanwhile would offer to restore.
    QStandardPaths::setTestModeEnabled(true);

    AppManager manager;

    {
        int argc = 0;
        QStringList args;
#if defined(_WIN32) && defined(UNICODE)
        args << QString::fromWCharArray(argv[0]);
#else
        args << QString::fromUtf8(argv[0]);
#endif
        args << QString::fromUtf8("--clear-cache");
        // NATRON_TESTS_KEEP_OFX_CACHE: use the OFX plug-in cache written by the previous run, to test
        // the plug-ins known from the cache (loaded at their first use) instead of the scan
        if ( qgetenv("NATRON_TESTS_KEEP_OFX_CACHE").isEmpty() ) {
            args << QString::fromUtf8("--clear-openfx-cache");
        }
        args << QString::fromUtf8("--no-settings");
        args << QString::fromUtf8("--settings") << QString::fromUtf8("useStdOFXPluginsLocation=False");
        CLArgs cl(args, true);
        if (!manager.load(argc, 0, cl)) {
            printf("Failed to load AppManager\n");

            return 1;
        }
    }
    int retval = RUN_ALL_TESTS();
    return retval;
}
