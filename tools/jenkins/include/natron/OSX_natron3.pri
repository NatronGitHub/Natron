boost {
  BOOST_VERSION = $$system("ls /opt/local/libexec/boost|sort|tail -1")
  message("found boost $$BOOST_VERSION")
  INCLUDEPATH += /opt/local/libexec/boost/$$BOOST_VERSION/include
  LIBS += -L/opt/local/libexec/boost/$$BOOST_VERSION/lib -lboost_serialization-mt
}
boost-serialization-lib: LIBS += -lboost_serialization-mt
shiboken {
    PKGCONFIG -= shiboken
    INCLUDEPATH += /opt/local/include/shiboken-2.7
    LIBS += -L/opt/local/lib -lshiboken-python2.7.1.2
}
python {
    # required to link natron-python, which needs libintl
    LIBS += -L/opt/local/lib
}
