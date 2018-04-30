boost: LIBS += -lboost_serialization
PKGCONFIG += expat
INCLUDEPATH+=/opt/Natron-CY2016/include
LIBS+="-L/opt/Natron-CY2016/lib"

pyside {
PKGCONFIG -= pyside
PKGCONFIG -= pyside-py2
PKGCONFIG += pyside2
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside2)
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside2)/QtCore
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside2)/QtGui
INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
LIBS += -lpyside2-python2.7
}
shiboken {
PKGCONFIG -= shiboken
PKGCONFIG -= shiboken-py2
PKGCONFIG += shiboken2
INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken2)
LIBS += -lshiboken2-python2.7
}

QMAKE_LFLAGS += -Wl,-rpath,\\\$\$ORIGIN/../lib

#Mocha compat
LIBS += -lGLU

