boost: INCLUDEPATH += /usr/local/include
boost: LIBS += -L/usr/local/lib -lboost_serialization-mt -lboost_thread-mt -lboost_system-mt
expat: PKGCONFIG -= expat
expat: INCLUDEPATH += /usr/local/opt/expat/include
expat: LIBS += -L/usr/local/opt/expat/lib -lexpat
openmp {
  LIBS += -L/usr/local/opt/llvm@11/lib -lomp

  QMAKE_CC=/usr/local/opt/llvm@11/bin/clang
  QMAKE_CXX='/usr/local/opt/llvm@11/bin/clang++ -stdlib=libc++'
  QMAKE_OBJECTIVE_CXX='/usr/local/opt/llvm@11/bin/clang++ -stdlib=libc++'
  QMAKE_OBJECTIVE_CC='/usr/local/opt/llvm@11/bin/clang -stdlib=libc++'
  QMAKE_LD='/usr/local/opt/llvm@11/bin/clang++ -stdlib=libc++'
  cc_setting.name = CC
  cc_setting.value = /usr/local/opt/llvm@11/bin/clang
  cxx_setting.name = CXX
  cxx_setting.value = /usr/local/opt/llvm@11/bin/clang++
  ld_setting.name = LD
  ld_setting.value = /usr/local/opt/llvm@11/bin/clang
  ldxx_setting.name = LDPLUSPLUS
  ldxx_setting.value = /usr/local/opt/llvm@11/bin/clang++
  QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting ld_setting ldxx_setting
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
#expat: INCLUDEPATH += /Users/devernay/Development/openfx/HostSupport/expat-2.1.0/lib
#expat: LIBS += /Users/devernay/Development/openfx/HostSupport/Darwin-debug
