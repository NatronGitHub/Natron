boost {
  LIBS += -lboost_serialization-mt
}
shiboken {
    PKGCONFIG -= shiboken
    PKGCONFIG -= pyside-py$$PYV
    INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py$$PYV)
    INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py$$PYV)/QtCore
    INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py$$PYV)/QtGui
    INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
    LIBS += $$system(pkg-config --libs pyside-py$$PYV)
}
python {
    # required to link natron-python, which needs libintl
    LIBS += -L/opt/local/lib
}
