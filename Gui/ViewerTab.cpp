//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ViewerTab.h"

#include <cassert>
#include <QApplication>
#include <QSlider>
#include <QComboBox>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QCoreApplication>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QtGui/QKeySequence>

#include "Engine/ViewerInstance.h"
#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"

#include "Gui/ViewerGL.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/ComboBox.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/FromQtEnums.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ClickableLabel.h"

using namespace Natron;

struct ViewerTabPrivate {
    
    /*OpenGL viewer*/
	ViewerGL* viewer;
    
    QVBoxLayout* _mainLayout;
    
	/*Viewer Settings*/
    QWidget* _firstSettingsRow,*_secondSettingsRow;
    QHBoxLayout* _firstRowLayout,*_secondRowLayout;
    
    /*1st row*/
	//ComboBox* _viewerLayers;
	ComboBox* _viewerChannels;
    ComboBox* _zoomCombobox;
    Button* _centerViewerButton;
    Button* _clipToProjectFormatButton;
    Button* _enableViewerRoI;
    Button* _refreshButton;
    
    /*2nd row*/
    SpinBox* _gainBox;
    ScaleSliderQWidget* _gainSlider;
    ClickableLabel* _autoConstrastLabel;
    QCheckBox* _autoContrast;
    ComboBox* _viewerColorSpace;
    ComboBox* _viewsComboBox;
    int _currentViewIndex;
    QMutex _currentViewMutex;
    /*Infos*/
    InfoViewerWidget* _infosWidget;
    
    
	/*TimeLine buttons*/
    QWidget* _playerButtonsContainer;
	QHBoxLayout* _playerLayout;
	SpinBox* _currentFrameBox;
	Button* firstFrame_Button;
    Button* previousKeyFrame_Button;
    Button* play_Backward_Button;
	Button* previousFrame_Button;
    Button* stop_Button;
    Button* nextFrame_Button;
	Button* play_Forward_Button;
    Button* nextKeyFrame_Button;
	Button* lastFrame_Button;
    Button* previousIncrement_Button;
    SpinBox* incrementSpinBox;
    Button* nextIncrement_Button;
    Button* loopMode_Button;
    
    QLabel* fpsName;
    SpinBox* fpsBox;
    
	/*frame seeker*/
    TimeLineGui* _timeLineGui;
    
    
    Gui* _gui;
    
    ViewerInstance* _viewerNode;// < pointer to the internal node

    ViewerTabPrivate(Gui* gui,ViewerInstance* node)
    : _currentViewIndex(0)
    , _gui(gui)
    , _viewerNode(node)
    {
        
    }
};

ViewerTab::ViewerTab(Gui* gui,ViewerInstance* node,QWidget* parent)
: QWidget(parent)
, _imp(new ViewerTabPrivate(gui,node))
{
    
    installEventFilter(this);
    setObjectName(node->getName().c_str());
    _imp->_mainLayout=new QVBoxLayout(this);
    setLayout(_imp->_mainLayout);
    _imp->_mainLayout->setSpacing(0);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    /*VIEWER SETTINGS*/
    
    /*1st row of buttons*/
    _imp->_firstSettingsRow = new QWidget(this);
    _imp->_firstRowLayout = new QHBoxLayout(_imp->_firstSettingsRow);
    _imp->_firstSettingsRow->setLayout(_imp->_firstRowLayout);
    _imp->_firstRowLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_firstRowLayout->setSpacing(0);
    _imp->_mainLayout->addWidget(_imp->_firstSettingsRow);
    
    // _viewerLayers = new ComboBox(_firstSettingsRow);
    //_firstRowLayout->addWidget(_viewerLayers);
    
    _imp->_viewerChannels = new ComboBox(_imp->_firstSettingsRow);
    _imp->_viewerChannels->setToolTip("<p><b>Channels: \n</b></p>"
                                "The channels to display on the viewer.");
    _imp->_firstRowLayout->addWidget(_imp->_viewerChannels);
    
    _imp->_viewerChannels->addItem("Luminance",QIcon(),QKeySequence(Qt::Key_Y));
    _imp->_viewerChannels->addItem("RGB",QIcon(),QKeySequence(Qt::SHIFT+Qt::Key_R));
    _imp->_viewerChannels->addItem("R",QIcon(),QKeySequence(Qt::Key_R));
    _imp->_viewerChannels->addItem("G",QIcon(),QKeySequence(Qt::Key_G));
    _imp->_viewerChannels->addItem("B",QIcon(),QKeySequence(Qt::Key_B));
    _imp->_viewerChannels->addItem("A",QIcon(),QKeySequence(Qt::Key_A));
    _imp->_viewerChannels->setCurrentIndex(1);
    QObject::connect(_imp->_viewerChannels, SIGNAL(currentIndexChanged(int)), this, SLOT(onViewerChannelsChanged(int)));
    
    _imp->_zoomCombobox = new ComboBox(_imp->_firstSettingsRow);
    _imp->_zoomCombobox->setToolTip("<p><b>Zoom: \n</b></p>"
                              "The zoom applied to the image on the viewer.");
    _imp->_zoomCombobox->addItem("10%");
    _imp->_zoomCombobox->addItem("25%");
    _imp->_zoomCombobox->addItem("50%");
    _imp->_zoomCombobox->addItem("75%");
    _imp->_zoomCombobox->addItem("100%");
    _imp->_zoomCombobox->addItem("125%");
    _imp->_zoomCombobox->addItem("150%");
    _imp->_zoomCombobox->addItem("200%");
    _imp->_zoomCombobox->addItem("400%");
    _imp->_zoomCombobox->addItem("800%");
    _imp->_zoomCombobox->addItem("1600%");
    _imp->_zoomCombobox->addItem("2400%");
    _imp->_zoomCombobox->addItem("3200%");
    _imp->_zoomCombobox->addItem("6400%");
    _imp->_zoomCombobox->setMaximumWidthFromText("100000%");

    _imp->_firstRowLayout->addWidget(_imp->_zoomCombobox);
    
    _imp->_centerViewerButton = new Button(_imp->_firstSettingsRow);
    _imp->_firstRowLayout->addWidget(_imp->_centerViewerButton);
    
    
    _imp->_clipToProjectFormatButton = new Button(_imp->_firstSettingsRow);
    _imp->_clipToProjectFormatButton->setCheckable(true);
    _imp->_clipToProjectFormatButton->setChecked(true);
    _imp->_clipToProjectFormatButton->setDown(true);
    _imp->_firstRowLayout->addWidget(_imp->_clipToProjectFormatButton);
    
    _imp->_enableViewerRoI = new Button(_imp->_firstSettingsRow);
    _imp->_enableViewerRoI->setCheckable(true);
    _imp->_enableViewerRoI->setChecked(false);
    _imp->_enableViewerRoI->setDown(false);
    _imp->_firstRowLayout->addWidget(_imp->_enableViewerRoI);
    
    _imp->_refreshButton = new Button(_imp->_firstSettingsRow);
    _imp->_refreshButton->setToolTip("Force a new render of the current frame."
                                     "<p><b>Keyboard shortcut: U</b></p>");
    _imp->_firstRowLayout->addWidget(_imp->_refreshButton);
    
    _imp->_firstRowLayout->addStretch();
    
    /*2nd row of buttons*/
    _imp->_secondSettingsRow = new QWidget(this);
    _imp->_secondRowLayout = new QHBoxLayout(_imp->_secondSettingsRow);
    _imp->_secondSettingsRow->setLayout(_imp->_secondRowLayout);
    _imp->_secondRowLayout->setSpacing(0);
    _imp->_secondRowLayout->setContentsMargins(0, 0, 0, 0);
    //  _secondSettingsRow->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _imp->_mainLayout->addWidget(_imp->_secondSettingsRow);
    
    _imp->_gainBox = new SpinBox(_imp->_secondSettingsRow,SpinBox::DOUBLE_SPINBOX);
    _imp->_gainBox->setToolTip("<p><b>Gain: \n</b></p>"
                         "Multiplies the image by \nthis amount before display.");
    _imp->_gainBox->setIncrement(0.1);
    _imp->_gainBox->setValue(1.0);
    _imp->_gainBox->setMinimum(0.0);
    _imp->_secondRowLayout->addWidget(_imp->_gainBox);
    
    
    _imp->_gainSlider=new ScaleSliderQWidget(0, 64,1.0,Natron::LINEAR_SCALE,_imp->_secondSettingsRow);
    _imp->_gainSlider->setToolTip("<p><b>Gain: \n</b></p>"
                            "Multiplies the image by \nthis amount before display.");
    _imp->_secondRowLayout->addWidget(_imp->_gainSlider);
    
    QString autoContrastToolTip("<p><b>Auto-contrast: \n</b></p>"
                                "Automatically adjusts the gain and the offset applied \n"
                                "to the colors of the visible image portion on the viewer.");
    _imp->_autoConstrastLabel = new ClickableLabel("Auto-contrast:",_imp->_secondSettingsRow);
    _imp->_autoConstrastLabel->setToolTip(autoContrastToolTip);
    _imp->_secondRowLayout->addWidget(_imp->_autoConstrastLabel);
    
    _imp->_autoContrast = new QCheckBox(_imp->_secondSettingsRow);
    _imp->_autoContrast->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    _imp->_autoContrast->setToolTip(autoContrastToolTip);
    _imp->_secondRowLayout->addWidget(_imp->_autoContrast);
    
    _imp->_viewerColorSpace=new ComboBox(_imp->_secondSettingsRow);
    _imp->_viewerColorSpace->setToolTip("<p><b>Viewer color process: \n</b></p>"
                                  "The operation applied to the image before it is displayed\n"
                                  "on screen. All the color pipeline \n"
                                  "is linear,thus the process converts from linear\n"
                                  "to your monitor's colorspace.");
    _imp->_secondRowLayout->addWidget(_imp->_viewerColorSpace);
    
    _imp->_viewerColorSpace->addItem("Linear(None)");
    _imp->_viewerColorSpace->addItem("sRGB");
    _imp->_viewerColorSpace->addItem("Rec.709");
    _imp->_viewerColorSpace->setCurrentIndex(1);
    
    _imp->_viewsComboBox = new ComboBox(_imp->_secondSettingsRow);
    _imp->_viewsComboBox->setToolTip("<p><b>Active view: \n</b></p>"
                               "Tells the viewer what view should be displayed.");
    _imp->_secondRowLayout->addWidget(_imp->_viewsComboBox);
    _imp->_viewsComboBox->hide();
    int viewsCount = _imp->_gui->getApp()->getProject()->getProjectViewsCount(); //getProjectViewsCount
    updateViewsMenu(viewsCount);
    
    _imp->_secondRowLayout->addStretch();
    
    /*=============================================*/
    
    /*OpenGL viewer*/
    _imp->viewer = new ViewerGL(this);
    _imp->_mainLayout->addWidget(_imp->viewer);
    /*=============================================*/
    /*info bbox & color*/
    _imp->_infosWidget = new InfoViewerWidget(_imp->viewer,this);
    //  _infosWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _imp->_mainLayout->addWidget(_imp->_infosWidget);
    _imp->viewer->setInfoViewer(_imp->_infosWidget);
    /*=============================================*/
    
    /*Player buttons*/
    _imp->_playerButtonsContainer = new QWidget(this);
    _imp->_playerLayout=new QHBoxLayout(_imp->_playerButtonsContainer);
    _imp->_playerLayout->setSpacing(0);
    _imp->_playerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_playerButtonsContainer->setLayout(_imp->_playerLayout);
    //   _playerButtonsContainer->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _imp->_mainLayout->addWidget(_imp->_playerButtonsContainer);
    
    _imp->_currentFrameBox=new SpinBox(_imp->_playerButtonsContainer,SpinBox::INT_SPINBOX);
    _imp->_currentFrameBox->setValue(0);
    _imp->_currentFrameBox->setToolTip("<p><b>Current frame number</b></p>");
    _imp->_playerLayout->addWidget(_imp->_currentFrameBox);
    
    _imp->_playerLayout->addStretch();
    
    _imp->firstFrame_Button = new Button(_imp->_playerButtonsContainer);
    QKeySequence firstFrameKey(Qt::CTRL + Qt::Key_Left);
    QString tooltip = "First frame";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(firstFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->firstFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->firstFrame_Button);
    
    
    _imp->previousKeyFrame_Button=new Button(_imp->_playerButtonsContainer);
    _imp->previousKeyFrame_Button->hide();
    QKeySequence previousKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Left);
    tooltip = "Previous keyframe";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(previousKeyFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->previousKeyFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->previousKeyFrame_Button);
    
    
    _imp->play_Backward_Button=new Button(_imp->_playerButtonsContainer);
    QKeySequence playbackFrameKey(Qt::Key_J);
    tooltip = "Play backward";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(playbackFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->play_Backward_Button->setToolTip(tooltip);
    _imp->play_Backward_Button->setCheckable(true);
    _imp->_playerLayout->addWidget(_imp->play_Backward_Button);
    
    
    _imp->previousFrame_Button = new Button(_imp->_playerButtonsContainer);
    QKeySequence previousFrameKey(Qt::Key_Left);
    tooltip = "Previous frame";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(previousFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->previousFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->previousFrame_Button);
    
    
    _imp->stop_Button = new Button(_imp->_playerButtonsContainer);
    QKeySequence stopKey(Qt::Key_K);
    tooltip = "Stop";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(stopKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->stop_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->stop_Button);
    
    
    _imp->nextFrame_Button = new Button(_imp->_playerButtonsContainer);
    QKeySequence nextFrameKey(Qt::Key_Right);
    tooltip = "Next frame";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(nextFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->nextFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->nextFrame_Button);
    
    
    _imp->play_Forward_Button = new Button(_imp->_playerButtonsContainer);
    QKeySequence playKey(Qt::Key_L);
    tooltip = "Play forward";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(playKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->play_Forward_Button->setToolTip(tooltip);
    _imp->play_Forward_Button->setCheckable(true);
    _imp->_playerLayout->addWidget(_imp->play_Forward_Button);
    
    
    _imp->nextKeyFrame_Button = new Button(_imp->_playerButtonsContainer);
    _imp->nextKeyFrame_Button->hide();
    QKeySequence nextKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Right);
    tooltip = "Next keyframe";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(nextKeyFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->nextKeyFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->nextKeyFrame_Button);
    
    
    _imp->lastFrame_Button = new Button(_imp->_playerButtonsContainer);
    QKeySequence lastFrameKey(Qt::CTRL + Qt::Key_Right);
    tooltip = "Last frame";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(lastFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->lastFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->lastFrame_Button);
    
    
    _imp->_playerLayout->addStretch();
    
    
    _imp->previousIncrement_Button = new Button(_imp->_playerButtonsContainer);
    QKeySequence previousIncrFrameKey(Qt::SHIFT + Qt::Key_Left);
    tooltip = "Previous increment";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(previousIncrFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->previousIncrement_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->previousIncrement_Button);
    
    
    _imp->incrementSpinBox=new SpinBox(_imp->_playerButtonsContainer);
    _imp->incrementSpinBox->setValue(10);
    _imp->incrementSpinBox->setToolTip("<p><b>Frame increment: \n</b></p>"
                                 "The previous/next increment buttons step"
                                 " with this increment.");
    _imp->_playerLayout->addWidget(_imp->incrementSpinBox);
    
    
    _imp->nextIncrement_Button = new Button(_imp->_playerButtonsContainer);
    QKeySequence nextIncrFrameKey(Qt::SHIFT + Qt::Key_Right);
    tooltip = "Next increment";
    tooltip.append("<p><b>Keyboard shortcut: ");
    tooltip.append(nextIncrFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->nextIncrement_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->nextIncrement_Button);
    
    _imp->loopMode_Button = new Button(_imp->_playerButtonsContainer);
    _imp->loopMode_Button->setCheckable(true);
    _imp->loopMode_Button->setChecked(true);
    _imp->loopMode_Button->setDown(true);
    _imp->loopMode_Button->setToolTip("Behaviour to adopt when the playback\n hit the end of the range: loop or stop.");
    _imp->_playerLayout->addWidget(_imp->loopMode_Button);
    
    
    _imp->_playerLayout->addStretch();
    
    _imp->fpsName = new QLabel("fps",_imp->_playerButtonsContainer);
    _imp->_playerLayout->addWidget(_imp->fpsName);
    _imp->fpsBox = new SpinBox(_imp->_playerButtonsContainer,SpinBox::DOUBLE_SPINBOX);
    _imp->fpsBox->decimals(1);
    _imp->fpsBox->setValue(24.0);
    _imp->fpsBox->setIncrement(0.1);
    _imp->fpsBox->setToolTip("<p><b>fps: \n</b></p>"
                       "Enter here the desired playback rate.");
    _imp->_playerLayout->addWidget(_imp->fpsBox);
    
    
    QPixmap pixFirst;
    QPixmap pixPrevKF;
    QPixmap pixRewind;
    QPixmap pixBack1;
    QPixmap pixStop;
    QPixmap pixForward1;
    QPixmap pixPlay;
    QPixmap pixNextKF;
    QPixmap pixLast;
    QPixmap pixPrevIncr;
    QPixmap pixNextIncr;
    QPixmap pixRefresh ;
    QPixmap pixCenterViewer ;
    QPixmap pixLoopMode;
    QPixmap pixClipToProject ;
    QPixmap pixViewerRoI;
    
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_FIRST_FRAME,&pixFirst);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_KEY,&pixPrevKF);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_REWIND,&pixRewind);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS,&pixBack1);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_STOP,&pixStop);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT,&pixForward1);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY,&pixPlay);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_KEY,&pixNextKF);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_LAST_FRAME,&pixLast);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_INCR,&pixPrevIncr);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_INCR,&pixNextIncr);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH,&pixRefresh);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER,&pixCenterViewer);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE,&pixLoopMode);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT,&pixClipToProject);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_ROI,&pixViewerRoI);
    
    _imp->firstFrame_Button->setIcon(QIcon(pixFirst));
    _imp->previousKeyFrame_Button->setIcon(QIcon(pixPrevKF));
    _imp->play_Backward_Button->setIcon(QIcon(pixRewind));
    _imp->previousFrame_Button->setIcon(QIcon(pixBack1));
    _imp->stop_Button->setIcon(QIcon(pixStop));
    _imp->nextFrame_Button->setIcon(QIcon(pixForward1));
    _imp->play_Forward_Button->setIcon(QIcon(pixPlay));
    _imp->nextKeyFrame_Button->setIcon(QIcon(pixNextKF));
    _imp->lastFrame_Button->setIcon(QIcon(pixLast));
    _imp->previousIncrement_Button->setIcon(QIcon(pixPrevIncr));
    _imp->nextIncrement_Button->setIcon(QIcon(pixNextIncr));
    _imp->_refreshButton->setIcon(QIcon(pixRefresh));
    _imp->_centerViewerButton->setIcon(QIcon(pixCenterViewer));
    _imp->loopMode_Button->setIcon(QIcon(pixLoopMode));
    _imp->_clipToProjectFormatButton->setIcon(QIcon(pixClipToProject));
    _imp->_enableViewerRoI->setIcon(QIcon(pixViewerRoI));
    
    
    _imp->_centerViewerButton->setToolTip("Scales the image so it doesn't exceed the size of the viewer and centers it."
                                    "<p><b>Keyboard shortcut: F</b></p>");
    
    _imp->_clipToProjectFormatButton->setToolTip("<p>Clips the portion of the image displayed "
                                           "on the viewer to the project format. "
                                           "When off, everything in the union of all nodes "
                                           "region of definition will be displayed.</p>"
                                           "<p><b>Keyboard shortcut: C</b></p>");
    
    QKeySequence enableViewerKey(Qt::SHIFT + Qt::Key_W);
    _imp->_enableViewerRoI->setToolTip("<p>When active, enables the region of interest that will limit"
                                 " the portion of the viewer that is kept updated.</p>"
                                 "<p><b>Keyboard shortcut:"+ enableViewerKey.toString() + "</b></p>");
    /*=================================================*/
    
    /*frame seeker*/
    _imp->_timeLineGui = new TimeLineGui(_imp->_gui->getApp()->getTimeLine(),_imp->_gui,this);
    _imp->_timeLineGui->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _imp->_mainLayout->addWidget(_imp->_timeLineGui);
    /*================================================*/
    
    
    /*slots & signals*/
    QObject::connect(_imp->_viewerColorSpace, SIGNAL(currentIndexChanged(QString)), _imp->_viewerNode,SLOT(onColorSpaceChanged(QString)));
    QObject::connect(_imp->_zoomCombobox, SIGNAL(currentIndexChanged(QString)),_imp->viewer, SLOT(zoomSlot(QString)));
    QObject::connect(_imp->viewer, SIGNAL(zoomChanged(int)), this, SLOT(updateZoomComboBox(int)));
    QObject::connect(_imp->_gainBox, SIGNAL(valueChanged(double)), _imp->_viewerNode,SLOT(onExposureChanged(double)));
    QObject::connect(_imp->_gainSlider, SIGNAL(positionChanged(double)), _imp->_gainBox, SLOT(setValue(double)));
    QObject::connect(_imp->_gainSlider, SIGNAL(positionChanged(double)), _imp->_viewerNode, SLOT(onExposureChanged(double)));
    QObject::connect(_imp->_gainBox, SIGNAL(valueChanged(double)), _imp->_gainSlider, SLOT(seekScalePosition(double)));
    QObject::connect(_imp->_viewerNode,SIGNAL(exposureChanged(double)),this,SLOT(onInternalExposureChanged(double)));
    QObject::connect(_imp->_currentFrameBox, SIGNAL(valueChanged(double)), this, SLOT(onCurrentTimeSpinBoxChanged(double)));
    
    VideoEngine* vengine = _imp->_viewerNode->getVideoEngine().get();
    
    QObject::connect(_imp->play_Forward_Button,SIGNAL(clicked(bool)),this,SLOT(startPause(bool)));
    QObject::connect(_imp->stop_Button,SIGNAL(clicked()),this,SLOT(abortRendering()));
    QObject::connect(_imp->play_Backward_Button,SIGNAL(clicked(bool)),this,SLOT(startBackward(bool)));
    QObject::connect(_imp->previousFrame_Button,SIGNAL(clicked()),this,SLOT(previousFrame()));
    QObject::connect(_imp->nextFrame_Button,SIGNAL(clicked()),this,SLOT(nextFrame()));
    QObject::connect(_imp->previousIncrement_Button,SIGNAL(clicked()),this,SLOT(previousIncrement()));
    QObject::connect(_imp->nextIncrement_Button,SIGNAL(clicked()),this,SLOT(nextIncrement()));
    QObject::connect(_imp->firstFrame_Button,SIGNAL(clicked()),this,SLOT(firstFrame()));
    QObject::connect(_imp->lastFrame_Button,SIGNAL(clicked()),this,SLOT(lastFrame()));
    QObject::connect(_imp->loopMode_Button, SIGNAL(clicked(bool)), this, SLOT(toggleLoopMode(bool)));
    QObject::connect(_imp->_gui->getApp()->getTimeLine().get(),SIGNAL(frameChanged(SequenceTime,int)),
                     this, SLOT(onTimeLineTimeChanged(SequenceTime,int)));
    QObject::connect(_imp->_viewerNode,SIGNAL(addedCachedFrame(SequenceTime)),_imp->_timeLineGui,SLOT(onCachedFrameAdded(SequenceTime)));
    QObject::connect(_imp->_viewerNode,SIGNAL(removedLRUCachedFrame()),_imp->_timeLineGui,SLOT(onLRUCachedFrameRemoved()));
    QObject::connect(appPTR,SIGNAL(imageRemovedFromViewerCache(SequenceTime)),_imp->_timeLineGui,SLOT(onCachedFrameRemoved(SequenceTime)));
    QObject::connect(_imp->_viewerNode,SIGNAL(clearedViewerCache()),_imp->_timeLineGui,SLOT(onCachedFramesCleared()));
    QObject::connect(_imp->_refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    
    QObject::connect(_imp->_centerViewerButton, SIGNAL(clicked()), this, SLOT(centerViewer()));
    QObject::connect(_imp->_viewerNode,SIGNAL(viewerDisconnected()),this,SLOT(disconnectViewer()));
    QObject::connect(_imp->fpsBox, SIGNAL(valueChanged(double)), vengine, SLOT(setDesiredFPS(double)));
    QObject::connect(vengine, SIGNAL(fpsChanged(double,double)), _imp->_infosWidget, SLOT(setFps(double,double)));
    QObject::connect(vengine,SIGNAL(engineStopped()),_imp->_infosWidget,SLOT(hideFps()));
    QObject::connect(vengine, SIGNAL(engineStarted(bool,int)), this, SLOT(onEngineStarted(bool,int)));
    QObject::connect(vengine, SIGNAL(engineStopped()), this, SLOT(onEngineStopped()));
    
    
    QObject::connect(_imp->_viewerNode, SIGNAL(mustRedraw()), _imp->viewer, SLOT(update()));
    
    QObject::connect(_imp->_clipToProjectFormatButton,SIGNAL(clicked(bool)),this,SLOT(onClipToProjectButtonToggle(bool)));
    
    QObject::connect(_imp->_viewsComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(showView(int)));
    
    QObject::connect(_imp->_enableViewerRoI, SIGNAL(clicked(bool)), this, SLOT(onEnableViewerRoIButtonToggle(bool)));
    QObject::connect(_imp->_autoContrast,SIGNAL(clicked(bool)),this,SLOT(onAutoContrastChanged(bool)));
    QObject::connect(_imp->_autoConstrastLabel,SIGNAL(clicked(bool)),this,SLOT(onAutoContrastChanged(bool)));
    QObject::connect(_imp->_autoConstrastLabel,SIGNAL(clicked(bool)),_imp->_autoContrast,SLOT(setChecked(bool)));
}

void ViewerTab::onEnableViewerRoIButtonToggle(bool b) {
    _imp->_enableViewerRoI->setDown(b);
    _imp->viewer->setUserRoIEnabled(b);
}

void ViewerTab::updateViewsMenu(int count){
    int currentIndex = _imp->_viewsComboBox->activeIndex();
    _imp->_viewsComboBox->clear();
    if(count == 1){
        _imp->_viewsComboBox->hide();
        _imp->_viewsComboBox->addItem("Main");
        
    }else if(count == 2){
        _imp->_viewsComboBox->show();
        _imp->_viewsComboBox->addItem("Left",QIcon(),QKeySequence(Qt::CTRL + Qt::Key_1));
        _imp->_viewsComboBox->addItem("Right",QIcon(),QKeySequence(Qt::CTRL + Qt::Key_2));
    }else{
        _imp->_viewsComboBox->show();
        for(int i = 0 ; i < count;++i){
            _imp->_viewsComboBox->addItem(QString("View ")+QString::number(i+1),QIcon(),Gui::keySequenceForView(i));
        }
    }
    if(currentIndex < _imp->_viewsComboBox->count() && currentIndex != -1){
        _imp->_viewsComboBox->setCurrentIndex(currentIndex);
    }else{
        _imp->_viewsComboBox->setCurrentIndex(0);
    }
    _imp->_gui->updateViewsActions(count);
}

void ViewerTab::setCurrentView(int view){
    _imp->_viewsComboBox->setCurrentIndex(view);
}
int ViewerTab::getCurrentView() const{
    QMutexLocker l(&_imp->_currentViewMutex);
    return _imp->_currentViewIndex;
}

void ViewerTab::toggleLoopMode(bool b){
    _imp->loopMode_Button->setDown(b);
    _imp->_viewerNode->getVideoEngine()->toggleLoopMode(b);
}

void ViewerTab::onClipToProjectButtonToggle(bool b){
    _imp->_clipToProjectFormatButton->setDown(b);
    _imp->viewer->setClipToDisplayWindow(b);
}

void ViewerTab::onEngineStarted(bool forward,int frameCount){
    
    if (frameCount > 1) {
        _imp->play_Forward_Button->setChecked(forward);
        _imp->play_Forward_Button->setDown(forward);
        _imp->play_Backward_Button->setChecked(!forward);
        _imp->play_Backward_Button->setDown(!forward);
    }
}

void ViewerTab::onEngineStopped(){
    _imp->play_Forward_Button->setChecked(false);
    _imp->play_Forward_Button->setDown(false);
    _imp->play_Backward_Button->setChecked(false);
    _imp->play_Backward_Button->setDown(false);
}

void ViewerTab::updateZoomComboBox(int value){
    assert(value > 0);
    QString str = QString::number(value);
    str.append(QChar('%'));
    str.prepend("  ");
    str.append("  ");
    _imp->_zoomCombobox->setCurrentText_no_emit(str);
}

/*In case they're several viewer around, we need to reset the dag and tell it
 explicitly we want to use this viewer and not another one.*/
void ViewerTab::startPause(bool b){
    abortRendering();
    if(b){
        _imp->_viewerNode->getVideoEngine()->render(-1, /*frame count*/
                                              true,/*seek timeline ?*/
                                              true,/*rebuild tree?*/
                                              true, /*forward ?*/
                                              false); /*same frame ?*/
    }
}
void ViewerTab::abortRendering(){
    _imp->_viewerNode->getVideoEngine()->abortRendering(false);
}
void ViewerTab::startBackward(bool b){
    abortRendering();
    if(b){
        _imp->_viewerNode->getVideoEngine()->render(-1, /*frame count*/
                                              true,/*seek timeline ?*/
                                              true,/*rebuild tree?*/
                                              false,/*forward?*/
                                              false);/*same frame ?*/
    }
}
void ViewerTab::seek(SequenceTime time){
    _imp->_currentFrameBox->setValue(time);
    _imp->_timeLineGui->seek(time);
}

void ViewerTab::previousFrame(){
    seek(_imp->_timeLineGui->currentFrame()-1);
}
void ViewerTab::nextFrame(){
    seek(_imp->_timeLineGui->currentFrame()+1);
}
void ViewerTab::previousIncrement(){
    seek(_imp->_timeLineGui->currentFrame()-_imp->incrementSpinBox->value());
}
void ViewerTab::nextIncrement(){
    seek(_imp->_timeLineGui->currentFrame()+_imp->incrementSpinBox->value());
}
void ViewerTab::firstFrame(){
    seek(_imp->_timeLineGui->leftBound());
}
void ViewerTab::lastFrame(){
    seek(_imp->_timeLineGui->rightBound());
}

void ViewerTab::onTimeLineTimeChanged(SequenceTime time,int /*reason*/){
    _imp->_currentFrameBox->setValue(time);
}

void ViewerTab::onCurrentTimeSpinBoxChanged(double time){
    _imp->_timeLineGui->seek(time);
}


void ViewerTab::centerViewer(){
    _imp->viewer->fitImageToFormat();
    if(_imp->viewer->displayingImage()){
        _imp->_viewerNode->refreshAndContinueRender(false);
        
    }else{
        _imp->viewer->updateGL();
    }
}

void ViewerTab::refresh(){
    _imp->_viewerNode->forceFullComputationOnNextFrame();
    _imp->_viewerNode->updateTreeAndRender();
}

ViewerTab::~ViewerTab()
{
    _imp->_viewerNode->setUiContext(NULL);
}


void ViewerTab::keyPressEvent ( QKeyEvent * event ){

    if (event->key() == Qt::Key_Space) {
        if (parentWidget()) {
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);
            QCoreApplication::postEvent(parentWidget(),ev);
        }
    }else if (event->key() == Qt::Key_Y) {
        _imp->_viewerChannels->setCurrentIndex(0);
    }else if (event->key() == Qt::Key_R && event->modifiers() == Qt::ShiftModifier ) {
        _imp->_viewerChannels->setCurrentIndex(1);
    }else if (event->key() == Qt::Key_R && event->modifiers() == Qt::NoModifier) {
        _imp->_viewerChannels->setCurrentIndex(2);
    }else if (event->key() == Qt::Key_G) {
        _imp->_viewerChannels->setCurrentIndex(3);
    }else if (event->key() == Qt::Key_B) {
        _imp->_viewerChannels->setCurrentIndex(4);
    }else if (event->key() == Qt::Key_A) {
        _imp->_viewerChannels->setCurrentIndex(5);
    }else if (event->key() == Qt::Key_J) {
        startBackward(!_imp->play_Backward_Button->isDown());
    }
    else if (event->key() == Qt::Key_Left && !event->modifiers().testFlag(Qt::ShiftModifier)
                                         && !event->modifiers().testFlag(Qt::ControlModifier)) {
        previousFrame();
    }
    else if (event->key() == Qt::Key_K) {
        abortRendering();
    }
    else if (event->key() == Qt::Key_Right  && !event->modifiers().testFlag(Qt::ShiftModifier)
                                            && !event->modifiers().testFlag(Qt::ControlModifier)) {
        nextFrame();
    }
    else if (event->key() == Qt::Key_L) {
        startPause(!_imp->play_Forward_Button->isDown());
        
    }else if (event->key() == Qt::Key_Left && event->modifiers().testFlag(Qt::ShiftModifier)) {
        //prev incr
        previousIncrement();
    }
    else if (event->key() == Qt::Key_Right && event->modifiers().testFlag(Qt::ShiftModifier)) {
        //next incr
        nextIncrement();
    }else if (event->key() == Qt::Key_Left && event->modifiers().testFlag(Qt::ControlModifier)) {
        //first frame
        firstFrame();
    }
    else if (event->key() == Qt::Key_Right && event->modifiers().testFlag(Qt::ControlModifier)) {
        //last frame
        lastFrame();
    }
    else if (event->key() == Qt::Key_Left && event->modifiers().testFlag(Qt::ControlModifier)
            && event->modifiers().testFlag(Qt::ShiftModifier)) {
        //prev key
    }
    else if (event->key() == Qt::Key_Right && event->modifiers().testFlag(Qt::ControlModifier)
            &&  event->modifiers().testFlag(Qt::ShiftModifier)) {
        //next key
    } else if(event->key() == Qt::Key_F) {
        centerViewer();
        
    } else if(event->key() == Qt::Key_C) {
        onClipToProjectButtonToggle(!_imp->_clipToProjectFormatButton->isDown());
    } else if(event->key() == Qt::Key_U) {
        refresh();
    } else if(event->key() == Qt::Key_W && event->modifiers().testFlag(Qt::ShiftModifier)) {
        onEnableViewerRoIButtonToggle(!_imp->_enableViewerRoI->isDown());
    }
    
}

void ViewerTab::onViewerChannelsChanged(int i){
    ViewerInstance::DisplayChannels channels;
    switch (i) {
        case 0:
            channels = ViewerInstance::LUMINANCE;
            break;
        case 1:
            channels = ViewerInstance::RGB;
            break;
        case 2:
            channels = ViewerInstance::R;
            break;
        case 3:
            channels = ViewerInstance::G;
            break;
        case 4:
            channels = ViewerInstance::B;
            break;
        case 5:
            channels = ViewerInstance::A;
            break;
        default:
            channels = ViewerInstance::RGB;
            break;
    }
    _imp->_viewerNode->setDisplayChannels(channels);
}
bool ViewerTab::eventFilter(QObject *target, QEvent *event){
    if (event->type() == QEvent::MouseButtonPress) {
        _imp->_gui->selectNode(_imp->_gui->getApp()->getNodeGui(_imp->_viewerNode->getNode()));
        
    }
    return QWidget::eventFilter(target, event);
}

void ViewerTab::disconnectViewer(){
    if (_imp->viewer->displayingImage()) {
        _imp->viewer->disconnectViewer();
    }
}



QSize ViewerTab::minimumSizeHint() const{
    return QWidget::minimumSizeHint();
}
QSize ViewerTab::sizeHint() const{
    return QWidget::sizeHint();
}


void ViewerTab::showView(int view){
    QMutexLocker l(&_imp->_currentViewMutex);
    _imp->_currentViewIndex = view;
    abortRendering();
    bool isAutoPreview = _imp->_gui->getApp()->getProject()->isAutoPreviewEnabled();
    _imp->_viewerNode->refreshAndContinueRender(isAutoPreview);
}


void ViewerTab::drawOverlays() const{
    if (_imp->_gui->isClosing()) {
        return;
    }

    
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays(_imp->viewer);
        effect->drawOverlay();
    }
}

bool ViewerTab::notifyOverlaysPenDown(const QPointF& viewportPos,const QPointF& pos){
    
    if (_imp->_gui->isClosing()) {
        return false;
    }
    
    bool ret = false;
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        
        effect->setCurrentViewportForOverlays(_imp->viewer);
        ret = effect->onOverlayPenDown(viewportPos, pos);
    }
    return ret;
}

bool ViewerTab::notifyOverlaysPenMotion(const QPointF& viewportPos,const QPointF& pos){
    
    if (_imp->_gui->isClosing()) {
        return false;
    }
    bool ret = false;
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays(_imp->viewer);
        ret = effect->onOverlayPenMotion(viewportPos, pos);
    }
    return ret;
}

bool ViewerTab::notifyOverlaysPenUp(const QPointF& viewportPos,const QPointF& pos){
    
    if (_imp->_gui->isClosing()) {
        return false;
    }
    
    bool ret = false;
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays(_imp->viewer);
        ret = effect->onOverlayPenUp(viewportPos, pos);
    }
    return ret;
}

bool ViewerTab::notifyOverlaysKeyDown(QKeyEvent* e){
    
    if (_imp->_gui->isClosing()) {
        return false;
    }
    
    bool ret = false;
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays(_imp->viewer);
        ret = effect->onOverlayKeyDown(QtEnumConvert::fromQtKey((Qt::Key)e->key()),QtEnumConvert::fromQtModifiers(e->modifiers()));
    }
    return ret;
}

bool ViewerTab::notifyOverlaysKeyUp(QKeyEvent* e){
    
    if (_imp->_gui->isClosing()) {
        return false;
    }
    
    bool ret = false;
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        
        effect->setCurrentViewportForOverlays(_imp->viewer);
        ret = effect->onOverlayKeyUp(QtEnumConvert::fromQtKey((Qt::Key)e->key()),QtEnumConvert::fromQtModifiers(e->modifiers()));
    }
    return ret;
}

bool ViewerTab::notifyOverlaysKeyRepeat(QKeyEvent* e){
    
    if (_imp->_gui->isClosing()) {
        return false;
    }
    bool ret = false;
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        
        effect->setCurrentViewportForOverlays(_imp->viewer);
        ret = effect->onOverlayKeyRepeat(QtEnumConvert::fromQtKey((Qt::Key)e->key()),QtEnumConvert::fromQtModifiers(e->modifiers()));
    }
    return ret;
}

bool ViewerTab::notifyOverlaysFocusGained(){
    
    if (_imp->_gui->isClosing()) {
        return false;
    }
    bool ret = false;
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        
        effect->setCurrentViewportForOverlays(_imp->viewer);
        ret = effect->onOverlayFocusGained();
        
    }
    return ret;
}

bool ViewerTab::notifyOverlaysFocusLost(){
    
    if (_imp->_gui->isClosing()) {
        return false;
    }
    bool ret = false;
    std::vector<Natron::Node*> nodes;
    _imp->_gui->getApp()->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        Natron::EffectInstance* effect = nodes[i]->getLiveInstance();
        assert(effect);
        
        effect->setCurrentViewportForOverlays(_imp->viewer);
        ret = effect->onOverlayFocusLost();
    }
    return ret;
}

bool ViewerTab::isClippedToProject() const {
    return _imp->_clipToProjectFormatButton->isDown();
}

std::string ViewerTab::getColorSpace() const {
    return _imp->_viewerColorSpace->getCurrentIndexText().toStdString();
}

void ViewerTab::setUserRoIEnabled(bool b) {
    onEnableViewerRoIButtonToggle(b);
}

bool ViewerTab::isAutoContrastEnabled() const {
    return _imp->_autoContrast->isChecked();
}

void ViewerTab::setAutoContrastEnabled(bool b) {
    _imp->_autoContrast->setChecked(b);
    _imp->_gainSlider->setEnabled(!b);
    _imp->_gainBox->setEnabled(!b);
    _imp->_viewerNode->onAutoContrastChanged(b);
}

void ViewerTab::setUserRoI(const RectI& r) {
    _imp->viewer->setUserRoI(r);
}

void ViewerTab::setClipToProject(bool b) {
    onClipToProjectButtonToggle(b);
}

void ViewerTab::setColorSpace(const std::string& colorSpaceName) {
    int index = _imp->_viewerColorSpace->itemIndex(colorSpaceName.c_str());
    if (index != -1) {
        _imp->_viewerColorSpace->setCurrentIndex(index);
    }
}

void ViewerTab::setExposure(double d) {
    _imp->_gainBox->setValue(d);
    _imp->_gainSlider->seekScalePosition(d);
    _imp->_viewerNode->onExposureChanged(d);
}

double ViewerTab::getExposure() const {
    return _imp->_gainBox->value();
}

std::string ViewerTab::getChannelsString() const {
    return _imp->_viewerChannels->getCurrentIndexText().toStdString();
}

void ViewerTab::setChannels(const std::string& channelsStr) {
    int index = _imp->_viewerChannels->itemIndex(channelsStr.c_str());
    if (index != -1) {
        _imp->_viewerChannels->setCurrentIndex(index);
    }
}

ViewerGL* ViewerTab::getViewer() const {
    return _imp->viewer;
}

ViewerInstance* ViewerTab::getInternalNode() const { return _imp->_viewerNode; }

Gui* ViewerTab::getGui() const { return _imp->_gui; }

void ViewerTab::enterEvent(QEvent*)  { setFocus(); }

void ViewerTab::onInternalExposureChanged(double d) {
    _imp->_gainSlider->seekScalePosition(d);
    _imp->_gainBox->setValue(d);
}

void ViewerTab::onAutoContrastChanged(bool b) {
    _imp->_gainSlider->setEnabled(!b);
    _imp->_gainBox->setEnabled(!b);
    _imp->_viewerNode->onAutoContrastChanged(b);
}
