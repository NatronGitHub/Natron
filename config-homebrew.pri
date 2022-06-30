# Edit the following if the homebrew prefix is different.
# Older homebrew installations use /usr/local instead of /opt/homebrew
HOMEBREW = /usr/local

boost: INCLUDEPATH += $$HOMEBREW/include
boost: LIBS += -L$$HOMEBREW/lib -lboost_serialization-mt -lboost_thread-mt -lboost_system-mt

openmp {
  # clang 12+ is OK to build Natron, but libomp 12+ has a bug on macOS when
  # lanching tasks from a background thread, see https://bugs.llvm.org/show_bug.cgi?id=50579
  LIBS += -L$$HOMEBREW/opt/llvm@11/lib -lomp
  QMAKE_CC = $$HOMEBREW/opt/llvm@11/bin/clang
  QMAKE_CXX = $$HOMEBREW/opt/llvm@11/bin/clang++
  # Recent clang cannot compile QtMac.mm
  QMAKE_OBJECTIVE_CC = clang
  QMAKE_OBJECTIVE_CXX = clang++
  QMAKE_LINK = $$QMAKE_CXX
  INCLUDEPATH += $HOMEBREW/include
  cc_setting.name = CC
  cc_setting.value = $$QMAKE_CC
  cxx_setting.name = CXX
  cxx_setting.value = $$QMAKE_CXX
  # These settings are useless, unless someone finds out if there is a variable
  # to override the Objective-C compiler.
  objective_cc_setting.name = OBJECTIVE_CC
  objective_cc_setting.value = $$QMAKE_OBJECTIVE_CC
  objective_cxx_setting.name = OBJECTIVE_CXX
  objective_cxx_setting.value = $$QMAKE_OBJECTIVE_CXX
  ld_setting.name = LD
  ld_setting.value = $$QMAKE_CC
  ldxx_setting.name = LDPLUSPLUS
  ldxx_setting.value = $$QMAKE_CXX
  QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting objective_cc_setting objective_cxx_setting ld_setting ldxx_setting
  QMAKE_FLAGS = "-B /usr/bin"

  # clang (as of 5.0) does not yet support index-while-building functionality
  # present in Xcode 9, and Xcode 9's clang does not yet support OpenMP
  compiler_index_store_enable_setting.name = COMPILER_INDEX_STORE_ENABLE
  compiler_index_store_enable_setting.value = NO
  QMAKE_MAC_XCODE_SETTINGS += compiler_index_store_enable_setting

  # Xcode 9 compiles for 10.13 by default
#  sdkroot_setting.name = SDKROOT
#  sdkroot_setting.value = macosx10.14
#  QMAKE_MAC_XCODE_SETTINGS += sdkroot_setting
}
