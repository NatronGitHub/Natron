boost {
  BOOST_VERSION = $$system("ls /opt/local/libexec/boost|sort|tail -1")
  message("found boost $$BOOST_VERSION")
  INCLUDEPATH += /opt/local/libexec/boost/$$BOOST_VERSION/include
  LIBS += -L/opt/local/libexec/boost/$$BOOST_VERSION/lib -lboost_serialization-mt
}
boost-serialization-lib: LIBS += -lboost_serialization-mt
shiboken {
    PKGCONFIG -= shiboken
    INCLUDEPATH += /opt/local/include/shiboken-$$PYVER
    python3 {
        LIBS += -L/opt/local/lib -lshiboken-cpython-$$PYVERNODOT-darwin
    } else {
        LIBS += -L/opt/local/lib -lshiboken-python$$PYVER
    }
}
python {
    # required to link natron-python, which needs libintl
    LIBS += -L/opt/local/lib
}
