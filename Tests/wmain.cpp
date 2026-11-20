// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <stdio.h>

#include "gtest/gtest.h"

#include <QtCore/QByteArray>
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
