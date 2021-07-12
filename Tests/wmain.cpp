#include <stdio.h>

#include "gtest/gtest.h"

#include "Engine/AppManager.h"
#include "Engine/CLArgs.h"

using namespace NATRON_NAMESPACE;

#if defined(_WIN32) && defined(UNICODE)
GTEST_API_ int wmain(int argc, wchar_t **argv)
#else
GTEST_API_ int main(int argc, char **argv)
#endif
{
    printf("Running main() from wmain.cpp\n");
    ::testing::InitGoogleTest(&argc, argv);
    AppManager manager;

    {
        int argc = 0;
        QStringList args;
        args << QString::fromUtf8("--clear-cache");
        args << QString::fromUtf8("--clear-openfx-cache");
        args << QString::fromUtf8("--no-settings");
        args << QString::fromUtf8("dummy.ntp");
        CLArgs cl(args, true);
        if (!manager.load(argc, 0, cl)) {
            printf("Failed to load AppManager\n");

            return 1;
        }
    }
    int retval = RUN_ALL_TESTS();
    return retval;
}
