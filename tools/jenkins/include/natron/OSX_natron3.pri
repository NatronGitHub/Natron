boost {
    LIBS += -lboost_serialization-mt
}
boost-serialization-lib: LIBS += -lboost_serialization-mt
shiboken {
    PKGCONFIG -= shiboken
    PKGCONFIG -= shiboken-py$$PYV
    INCLUDEPATH += /opt/local/include/shiboken-$$PYVER
    python3 {
        LIBS += -L/opt/local/lib -lshiboken-cpython-$$PYVERNODOT-darwin
    } else {
        LIBS += -L/opt/local/lib -lshiboken-python$$PYVER
    }
}
python {
    # required to link natron-python, which needs libintl
    LIBS += -L/opt/local/lib
}
