boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
INCLUDEPATH+=/opt/Natron-sdk/include
LIBS+="-L/opt/Natron-sdk/lib"

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

QMAKE_LFLAGS += -Wl,-rpath,\\\$\$ORIGIN/../lib

# Mocha compat
LIBS += -lGLU

