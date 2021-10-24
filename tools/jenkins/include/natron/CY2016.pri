boost: LIBS += -lboost_serialization
PKGCONFIG += expat
INCLUDEPATH+=/opt/Natron-CY2016/include
LIBS+="-L/opt/Natron-CY2016/lib"

pyside {
PKGCONFIG -= pyside
PKGCONFIG -= pyside-py$$PYV
PKGCONFIG += pyside2
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside2)
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside2)/QtCore
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside2)/QtGui
INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
LIBS += -lpyside2-python$$PYVER
}
shiboken {
PKGCONFIG -= shiboken
PKGCONFIG -= shiboken-py$$PYV
INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken-py$$PYV)
LIBS += $$system(pkg-config --libs shiboken-py$$PYV)
}

QMAKE_LFLAGS += -Wl,-rpath,\\\$\$ORIGIN/../lib

#Mocha compat
LIBS += -lGLU

