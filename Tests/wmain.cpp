#include <stdio.h>

#include "gtest/gtest.h"

#if defined(_WIN32) && defined(UNICODE)
GTEST_API_ int wmain(int argc, wchar_t **argv) {
  printf("Running wmain() from wmain.cpp\n");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif

