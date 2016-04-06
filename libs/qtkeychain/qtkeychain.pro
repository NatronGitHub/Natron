# created for use in Natron

QT += core
QT -= gui

CONFIG += staticlib
TEMPLATE = lib
TARGET = qtkeychain

DEFINES += QKEYCHAIN_STATICLIB
HEADERS += keychain.h keychain_p.h qkeychain_export.h
SOURCES += keychain.cpp

mac: SOURCES += keychain_mac.cpp
win32: SOURCES += keychain_win.cpp
unix:!mac {
    QT += dbus xml
    HEADERS += gnomekeyring_p.h kwallet_interface.h
    SOURCES += gnomekeyring.cpp keychain_unix.cpp kwallet_interface.cpp
    OTHER_FILES += org.kde.KWallet.xml
}
