#-------------------------------------------------
#
# Project created by QtCreator 2015-12-16T11:37:59
#
#-------------------------------------------------

lessThan(QT_MAJOR_VERSION, 5): error("requires Qt 5")
QT += core gui widgets network
TARGET = breakdown
TEMPLATE = app
SOURCES += main.cpp mainwindow.cpp
HEADERS += mainwindow.h
FORMS += mainwindow.ui
