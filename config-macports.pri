boost {
  BOOST_VERSION = $$system("grep 'default boost.version' /opt/local/var/macports/sources/rsync.macports.org/macports/release/tarballs/ports/_resources/port1.0/group/boost-1.0.tcl |awk '{ print $3 }'")
  message("found boost $$BOOST_VERSION")
  INCLUDEPATH += /opt/local/libexec/boost/$$BOOST_VERSION/include
  LIBS += -L/opt/local/libexec/boost/$$BOOST_VERSION/lib -lboost_serialization-mt
}
macx:openmp {
  QMAKE_CC=/opt/local/bin/clang-mp-9.0
  QMAKE_CXX=/opt/local/bin/clang++-mp-9.0
  QMAKE_OBJECTIVE_CC=$$QMAKE_CC -stdlib=libc++
  QMAKE_LINK=$$QMAKE_CXX

  INCLUDEPATH += /opt/local/include/libomp
  LIBS += -L/opt/local/lib/libomp -liomp5
  cc_setting.name = CC
  cc_setting.value = /opt/local/bin/clang-mp-9.0
  cxx_setting.name = CXX
  cxx_setting.value = /opt/local/bin/clang++-mp-9.0
  ld_setting.name = LD
  ld_setting.value = /opt/local/bin/clang-mp-9.0
  ldplusplus_setting.name = LDPLUSPLUS
  ldplusplus_setting.value = /opt/local/bin/clang++-mp-7.0
  QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting ld_setting ldplusplus_setting
  QMAKE_FLAGS = "-B /usr/bin"

  # clang (as of 5.0) does not yet support index-while-building functionality
  # present in Xcode 9, and Xcode 9's clang does not yet support OpenMP
  compiler_index_store_enable_setting.name = COMPILER_INDEX_STORE_ENABLE
  compiler_index_store_enable_setting.value = NO
  QMAKE_MAC_XCODE_SETTINGS += compiler_index_store_enable_setting
}
