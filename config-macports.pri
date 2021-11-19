boost {
  LIBS += -lboost_serialization-mt
}
macx:openmp {
  QMAKE_CC = /opt/local/bin/clang-mp-12
  QMAKE_CXX = /opt/local/bin/clang++-mp-12
  QMAKE_OBJECTIVE_CC = $$QMAKE_CC -stdlib=libc++
  QMAKE_LINK = $$QMAKE_CXX

  INCLUDEPATH += /opt/local/include /opt/local/include/libomp
  LIBS += -L/opt/local/lib  -L/opt/local/lib/libomp -liomp5
  cc_setting.name = CC
  cc_setting.value = $$QMAKE_CC
  cxx_setting.name = CXX
  cxx_setting.value = $$QMAKE_CXX
  ld_setting.name = LD
  ld_setting.value = $$QMAKE_CC
  ldplusplus_setting.name = LDPLUSPLUS
  ldplusplus_setting.value = $$QMAKE_CXX
  QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting ld_setting ldplusplus_setting
  QMAKE_FLAGS = "-B /usr/bin"

  # clang (as of 12.0.1) does not yet support index-while-building functionality
  # present in Xcode 9 and later, and Xcode's clang (as of 13.0) does not yet support OpenMP
  compiler_index_store_enable_setting.name = COMPILER_INDEX_STORE_ENABLE
  compiler_index_store_enable_setting.value = NO
  QMAKE_MAC_XCODE_SETTINGS += compiler_index_store_enable_setting
}
