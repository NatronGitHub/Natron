#-------------------------------------------------
#
# Project created by QtCreator 2013-05-05T10:22:49
#
#-------------------------------------------------


TARGET = PowiterLib
TEMPLATE = lib
CONFIG += static moc
QT -= core gui
CONFIG += create_prl
#target.path = ($POWITER_ROOT)/ProjectFiles/build

MOC_DIR = $(POWITER_ROOT)/ProjectFiles/GeneratedFiles
message(Now building PowiterLib...the working directory is:  $(POWITER_ROOT))

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


INCLUDEPATH += $(QT_INCLUDES)
INCLUDEPATH += $(BOOST_INCLUDES)
INCLUDEPATH += $(POWITER_ROOT)
INCLUDEPATH += $(OPENEXR_INCLUDES)
INCLUDEPATH += $(FREETYPE2_INCLUDES)
INCLUDEPATH += $(FREETYPE_INCLUDES)
INCLUDEPATH += $(FTGL_INCLUDES)
DEPENDPATH += $(OPENEXR_INCLUDES)
DEPENDPATH += $(QT_INCLUDES)
DEPENDPATH += $(FREETYPE2_INCLUDES)
DEPENDPATH += $(FREETYPE_INCLUDES)
DEPENDPATH += $(BOOST_INCLUDES)
DEPENDPATH += $(FTGL_INCLUDES)


unix: LIBS += -L$(OPENEXR_LIB) -lHalf -lIex -lImath -lIlmImf -lIlmThread

unix: LIBS += -L$(FFMPEG_LIB) -lavcodec -lavformat -lavutil -lswscale -lavdevice -lavfilter

unix: LIBS += -L$(FTGL_LIB) -lftgl
