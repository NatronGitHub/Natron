boost {
  LIBS += -lboost_serialization-mt
}
boost-serialization-lib: LIBS += -lboost_serialization-mt
pyside {
    PKGCONFIG -= pyside
    PKGCONFIG -= pyside-py$$PYV
    INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py$$PYV)
    INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py$$PYV)/QtCore
    INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py$$PYV)/QtGui
    INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
    LIBS += $$system(pkg-config --libs pyside-py$$PYV)
}
shiboken {
    PKGCONFIG -= shiboken
    PKGCONFIG -= shiboken-py$$PYV
    INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken-py$$PYV)
    LIBS += $$system(pkg-config --libs shiboken-py$$PYV)
}
python {
    # required to link natron-python, which needs libintl
    LIBS += -L/opt/local/lib
}
