boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
PKGCONFIG += expat
PKGCONFIG += fontconfig

# incluir librerias boost
INCLUDEPATH += /usr/include/boost169
# -----------------------------

cairo {
        PKGCONFIG += cairo
        LIBS -=  $$system(pkg-config --variable=libdir cairo)/libcairo.a
}

pyside {
        PYSIDE_PKG_CONFIG_PATH = /usr/lib64/pkgconfig
        PKGCONFIG += pyside

        INCLUDEPATH += /usr/include/PySide
        INCLUDEPATH += /usr/include/PySide/QtCore
        INCLUDEPATH += /usr/include/PySide/QtGui

        LIBS += -lpyside-python2.7
}

shiboken {
        PKGCONFIG -= shiboken
        INCLUDEPATH += /usr/include/shiboken
        LIBS += -lshiboken-python2.7
}

# ignora todos los mensajes "warning"
QMAKE_CXXFLAGS += -w
CONFIG += warn_off
# ----------------------------

# "silent" solo muestra el nombre del cpp y no todos los includes en la conpilacion
CONFIG += silent
# ----------------------------