#-------------------------------------------------
#
# Project created by QtCreator 2013-05-05T10:22:49
#
#-------------------------------------------------

QT -= core gui

TARGET = PowiterLib
TEMPLATE = lib
CONFIG += static create_prl

MOC_DIR = $$PWD/../../../GeneratedFiles

SOURCES += \
    ../../../../Gui/arrowGUI.cpp \
    ../../../../Gui/DAG.cpp \
    ../../../../Gui/DAGQuickNode.cpp \
    ../../../../Gui/dockableSettings.cpp \
    ../../../../Gui/FeedbackSpinBox.cpp \
    ../../../../Gui/GLViewer.cpp \
    ../../../../Gui/InfoViewerWidget.cpp \
    ../../../../Gui/inputnode_ui.cpp \
    ../../../../Gui/knob.cpp \
    ../../../../Gui/knob_callback.cpp \
    ../../../../Gui/mainGui.cpp \
    ../../../../Gui/node_ui.cpp \
    ../../../../Gui/operatornode_ui.cpp \
    ../../../../Gui/outputnode_ui.cpp \
    ../../../../Gui/ScaleSlider.cpp \
    ../../../../Gui/textRenderer.cpp \
    ../../../../Gui/timeline.cpp \
    ../../../../Gui/viewerTab.cpp \
    ../../../../Core/Box.cpp \
    ../../../../Core/channels.cpp \
    ../../../../Core/DataBuffer.cpp \
    ../../../../Core/diskcache.cpp \
    ../../../../Core/displayFormat.cpp \
    ../../../../Core/flowOP.cpp \
    ../../../../Core/hash.cpp \
    ../../../../Core/imgOP.cpp \
    ../../../../Core/inputnode.cpp \
    ../../../../Core/lookUpTables.cpp \
    ../../../../Core/mappedfile.cpp \
    ../../../../Core/metadata.cpp \
    ../../../../Core/model.cpp \
    ../../../../Core/node.cpp \
    ../../../../Core/OP.cpp \
    ../../../../Core/outputnode.cpp \
    ../../../../Core/row.cpp \
    ../../../../Core/settings.cpp \
    ../../../../Core/Timer.cpp \
    ../../../../Core/VideoEngine.cpp \
    ../../../../Core/viewerNode.cpp \
    ../../../../Core/workerthread.cpp \
    ../../../../Reader/Read.cpp \
    ../../../../Reader/Reader.cpp \
    ../../../../Reader/readExr.cpp \
    ../../../../Reader/readffmpeg.cpp \
    ../../../../Reader/readQt.cpp \
    ../../../../Superviser/controler.cpp

HEADERS += \
    ../../../../Gui/arrowGUI.h \
    ../../../../Gui/comboBox.h \
    ../../../../Gui/DAG.h \
    ../../../../Gui/DAGQuickNode.h \
    ../../../../Gui/dockableSettings.h \
    ../../../../Gui/FeedbackSpinBox.h \
    ../../../../Gui/GLViewer.h \
    ../../../../Gui/InfoViewerWidget.h \
    ../../../../Gui/inputnode_ui.h \
    ../../../../Gui/knob.h \
    ../../../../Gui/knob_callback.h \
    ../../../../Gui/mainGui.h \
    ../../../../Gui/node_ui.h \
    ../../../../Gui/operatornode_ui.h \
    ../../../../Gui/outputnode_ui.h \
    ../../../../Gui/ScaleSlider.h \
    ../../../../Gui/shaders.h \
    ../../../../Gui/textRenderer.h \
    ../../../../Gui/timeline.h \
    ../../../../Gui/viewerTab.h \
    ../../../../Core/Box.h \
    ../../../../Core/channels.h \
    ../../../../Core/DataBuffer.h \
    ../../../../Core/diskcache.h \
    ../../../../Core/displayFormat.h \
    ../../../../Core/flowOP.h \
    ../../../../Core/hash.h \
    ../../../../Core/imgOP.h \
    ../../../../Core/inputnode.h \
    ../../../../Core/lookUpTables.h \
    ../../../../Core/lutclasses.h \
    ../../../../Core/mappedfile.h \
    ../../../../Core/metadata.h \
    ../../../../Core/model.h \
    ../../../../Core/node.h \
    ../../../../Core/OP.h \
    ../../../../Core/outputnode.h \
    ../../../../Core/referenceCountedObj.h \
    ../../../../Core/row.h \
    ../../../../Core/settings.h \
    ../../../../Core/singleton.h \
    ../../../../Core/sleeper.h \
    ../../../../Core/Timer.h \
    ../../../../Core/VideoEngine.h \
    ../../../../Core/viewerNode.h \
    ../../../../Core/workerthread.h \
    ../../../../Reader/Read.h \
    ../../../../Reader/Reader.h \
    ../../../../Reader/readerInfo.h \
    ../../../../Reader/readExr.h \
    ../../../../Reader/readffmpeg.h \
    ../../../../Reader/readQt.h \
    ../../../../Superviser/controler.h \
    ../../../../Superviser/Enums.h \
    ../../../../Superviser/gl_OsDependent.h \
    ../../../../Superviser/MemoryInfo.h \
    ../../../../Superviser/powiterFn.h \
    ../../../../Superviser/PwStrings.h

INSTALLS += target


INCLUDEPATH += /usr/local/Trolltech/Qt-4.8.4/include
INCLUDEPATH += $$PWD/../../../../libs/boost
INCLUDEPATH += $$PWD/../../../../
INCLUDEPATH += $$PWD/../../../../libs/OpenExr_linux/include
INCLUDEPATH += /usr/include/freetype2
DEPENDPATH += $$PWD/../../../../libs/OpenExr_linux/include
INCLUDEPATH += /usr/include
DEPENDPATH += /usr/include


unix:!macx: PRE_TARGETDEPS +=$$PWD/../../../../libs/OpenExr_linux/lib/libHalf.a
unix:!macx: PRE_TARGETDEPS +=$$PWD/../../../../libs/OpenExr_linux/lib/libIex.a
unix:!macx: PRE_TARGETDEPS +=$$PWD/../../../../libs/OpenExr_linux/lib/libImath.a
unix:!macx: PRE_TARGETDEPS +=$$PWD/../../../../libs/OpenExr_linux/lib/libIlmThread.a
unix:!macx: PRE_TARGETDEPS +=$$PWD/../../../../libs/OpenExr_linux/lib/libIlmImf.a

unix:!macx: LIBS += -L$$PWD/../../../../libs/OpenExr_linux/lib/ -lHalf -lIex -lImath -lIlmImf -lIlmThread


unix: PRE_TARGETDEPS +=/usr/lib/i386-linux-gnu/libavcodec.a
unix: PRE_TARGETDEPS +=/usr/lib/i386-linux-gnu/libavformat.a
unix: PRE_TARGETDEPS +=/usr/lib/i386-linux-gnu/libavdevice.a
unix: PRE_TARGETDEPS +=/usr/lib/i386-linux-gnu/libavutil.a
unix: PRE_TARGETDEPS +=/usr/lib/i386-linux-gnu/libavfilter.a
unix: PRE_TARGETDEPS +=/usr/lib/i386-linux-gnu/libswscale.a
unix: LIBS += -L/usr/lib/i386-linux-gnu/ -lavcodec -lavformat -lavutil -lswscale -lavdevice -lavfilter


unix: PRE_TARGETDEPS += /usr/lib/libftgl.a
unix: LIBS += -L/usr/lib/ -lftgl
