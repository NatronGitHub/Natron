boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
INCLUDEPATH+=/opt/Natron-sdk/include
LIBS+="-L/opt/Natron-sdk/lib"

pyside {
PKGCONFIG -= pyside
PKGCONFIG -= pyside-py2
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtCore
INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtGui
INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
LIBS += -lpyside-python2.7
}
shiboken {
PKGCONFIG -= shiboken
PKGCONFIG -= shiboken-py2
INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken)
LIBS += -lshiboken-python2.7
}

QMAKE_LFLAGS += -Wl,-rpath,\\\$\$ORIGIN/../lib

# Mocha compat
LIBS += -lGLU

