#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Powiter
TEMPLATE = app
CONFIG += app
CONFIG -= qt
CONFIG += moc rcc
CONFIG += openexr2 ftgl freetype freetype2 boost ffmpeg eigen opengl qt openfx

win32{
    CONFIG += glew
    CONFIG += qt
    QT += gui core opengl concurrent widgets
}

Release:DESTDIR = release
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.rcc
Release:UI_DIR = release/.ui

Debug:DESTDIR = debug
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.rcc
Debug:UI_DIR = debug/.ui

unix:macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.7
unix:macx:QMAKE_LFLAGS += -mmacosx-version-min=10.7
unix:macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
include(config.pri)


INCLUDEPATH += $$PWD/
INCLUDEPATH += $$PWD/libs/OpenFX/include


SOURCES += \
    Superviser/main.cpp \
    Gui/arrowGUI.cpp \
    Gui/DAG.cpp \
    Gui/DAGQuickNode.cpp \
    Gui/dockableSettings.cpp \
    Gui/FeedbackSpinBox.cpp \
    Gui/GLViewer.cpp \
    Gui/InfoViewerWidget.cpp \
    Gui/inputnode_ui.cpp \
    Gui/knob.cpp \
    Gui/knob_callback.cpp \
    Gui/mainGui.cpp \
    Gui/node_ui.cpp \
    Gui/operatornode_ui.cpp \
    Gui/outputnode_ui.cpp \
    Gui/ScaleSlider.cpp \
    Gui/textRenderer.cpp \
    Gui/timeline.cpp \
    Gui/viewerTab.cpp \
    Core/Box.cpp \
    Core/channels.cpp \
    Core/DataBuffer.cpp \
    Core/displayFormat.cpp \
    Core/flowOP.cpp \
    Core/hash.cpp \
    Core/imgOP.cpp \
    Core/inputnode.cpp \
    Core/lookUpTables.cpp \
    Core/mappedfile.cpp \
    Core/metadata.cpp \
    Core/model.cpp \
    Core/node.cpp \
    Core/OP.cpp \
    Core/outputnode.cpp \
    Core/row.cpp \
    Core/settings.cpp \
    Core/Timer.cpp \
    Core/VideoEngine.cpp \
    Core/viewerNode.cpp \
    Reader/Read.cpp \
    Reader/Reader.cpp \
    Reader/readExr.cpp \
    Reader/readffmpeg.cpp \
    Reader/readQt.cpp \
    Superviser/controler.cpp \
    Core/viewercache.cpp \
    Gui/texturecache.cpp \
    Gui/framefiledialog.cpp \
    Writer/Writer.cpp \
    Writer/writeQt.cpp \
    Writer/writeExr.cpp \
    Writer/Write.cpp \
    Gui/tabwidget.cpp \
    Gui/comboBox.cpp \
    Core/abstractCache.cpp \
    Core/interest.cpp \
    Core/nodecache.cpp \


HEADERS += \
    Gui/arrowGUI.h \
    Gui/comboBox.h \
    Gui/DAG.h \
    Gui/DAGQuickNode.h \
    Gui/dockableSettings.h \
    Gui/FeedbackSpinBox.h \
    Gui/GLViewer.h \
    Gui/InfoViewerWidget.h \
    Gui/inputnode_ui.h \
    Gui/knob.h \
    Gui/knob_callback.h \
    Gui/mainGui.h \
    Gui/node_ui.h \
    Gui/operatornode_ui.h \
    Gui/outputnode_ui.h \
    Gui/ScaleSlider.h \
    Gui/shaders.h \
    Gui/textRenderer.h \
    Gui/timeline.h \
    Gui/viewerTab.h \
    Core/Box.h \
    Core/channels.h \
    Core/DataBuffer.h \
    Core/displayFormat.h \
    Core/flowOP.h \
    Core/hash.h \
    Core/imgOP.h \
    Core/inputnode.h \
    Core/lookUpTables.h \
    Core/lutclasses.h \
    Core/mappedfile.h \
    Core/metadata.h \
    Core/model.h \
    Core/node.h \
    Core/OP.h \
    Core/outputnode.h \
    Core/referenceCountedObj.h \
    Core/row.h \
    Core/settings.h \
    Core/singleton.h \
    Core/sleeper.h \
    Core/Timer.h \
    Core/VideoEngine.h \
    Core/viewerNode.h \
    Reader/Read.h \
    Reader/Reader.h \
    Reader/readExr.h \
    Reader/readffmpeg.h \
    Reader/readQt.h \
    Superviser/controler.h \
    Superviser/Enums.h \
    Superviser/gl_OsDependent.h \
    Superviser/MemoryInfo.h \
    Superviser/powiterFn.h \
    Core/viewercache.h \
    Gui/texturecache.h \
    Gui/framefiledialog.h \
    Gui/tabwidget.h \
    Core/LRUcache.h \
    Core/interest.h \
    Reader/exrCommons.h \
    Writer/Writer.h \
    Writer/writeQt.h \
    Writer/writeExr.h \
    Writer/Write.h \
    Gui/lineEdit.h \
    Core/abstractCache.h \
    Core/nodecache.h \
    Gui/button.h \


INSTALLS += target

RESOURCES += \
    Resources.qrc


INSTALLS += data
OTHER_FILES += \
    config.pri

