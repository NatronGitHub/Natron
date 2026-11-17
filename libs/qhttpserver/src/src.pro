QHTTPSERVER_BASE = ..
TEMPLATE = lib
CONFIG += staticlib

TARGET = qhttpserver

!win32:VERSION = 0.1.0

QT += network
QT -= gui

include(../../../global.pri)
include(../../../libs.pri)

DEFINES += QHTTP_SERVER_STATIC
DEFINES += QHTTPSERVER_EXPORT
INCLUDEPATH += $$QHTTPSERVER_BASE/http-parser
PRIVATE_HEADERS += $$QHTTPSERVER_BASE/http-parser/http_parser.h qhttpconnection.h
PUBLIC_HEADERS += qhttpserver.h qhttprequest.h qhttpresponse.h qhttpserverapi.h qhttpserverfwd.h

HEADERS = $$PRIVATE_HEADERS $$PUBLIC_HEADERS
SOURCES = *.cpp $$QHTTPSERVER_BASE/http-parser/http_parser.c

OBJECTS_DIR = $$QHTTPSERVER_BASE/build
MOC_DIR = $$QHTTPSERVER_BASE/build
DESTDIR = $$QHTTPSERVER_BASE/build

