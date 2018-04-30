boost {
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib -lboost_thread-mt -lboost_system-mt
}
boost-serialization-lib: LIBS += -lboost_serialization-mt
shiboken {
    PKGCONFIG -= shiboken
    INCLUDEPATH += /opt/local/include/shiboken-2.7
    LIBS += -L/opt/local/lib -lshiboken-python2.7.1.2
}
