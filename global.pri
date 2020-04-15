
include(libs.pri)



CONFIG += exceptions warn_on no_keywords
DEFINES += OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_TUTTLE OFX_EXTENSIONS_VEGAS OFX_SUPPORTS_PARAMETRIC OFX_EXTENSIONS_TUTTLE OFX_EXTENSIONS_NATRON OFX_EXTENSIONS_RESOLVE OFX_SUPPORTS_OPENGLRENDER
DEFINES += OFX_SUPPORTS_MULTITHREAD
DEFINES += OFX_SUPPORTS_DIALOG


LIBS += -lGL -lX11
LIBS += -ldl
CONFIG += link_pkgconfig



DEFINES += NATRON_BUILD_NUMBER=0

openmp {
  QMAKE_CXXFLAGS += -fopenmp
  QMAKE_CFLAGS += -fopenmp
  QMAKE_LFLAGS += -fopenmp
}

python {
        LIBS += -lpython2.7
        INCLUDEPATH *= /usr/include/python2.7
}


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