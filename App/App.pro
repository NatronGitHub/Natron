TARGET = Natron
VERSION = 3.0.0

TEMPLATE = app
CONFIG += app
CONFIG += moc
CONFIG += boost boost-serialization-lib opengl qt cairo python shiboken pyside 
CONFIG += static-gui static-engine static-host-support static-breakpadclient static-libmv static-openmvg static-ceres static-qhttpserver static-libtess

QT += gui core opengl network

!noexpat: CONFIG += expat

include(../global.pri)

Resources.path = $$OUT_PWD
INSTALLS += Resources

QMAKE_SUBSTITUTES += $$PWD/../Gui/Resources/etc/fonts/fonts.conf.in

SOURCES += NatronApp_main.cpp

INSTALLS += target



