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
#include <QCoreApplication>
#include <QtGui/QKeyEvent>
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
#include "Gui/ScaleSlider.h"
#include "Gui/ComboBox.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"

#include "Global/AppManager.h"

using namespace Natron;

ViewerTab::ViewerTab(Gui* gui,ViewerInstance* node,QWidget* parent):QWidget(parent),
_gui(gui),
_viewerNode(node)
{
    
    installEventFilter(this);
    setObjectName(node->getName().c_str());
    _mainLayout=new QVBoxLayout(this);
    setLayout(_mainLayout);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    
    /*VIEWER SETTINGS*/
    
    /*1st row of buttons*/
    _firstSettingsRow = new QWidget(this);
    _firstRowLayout = new QHBoxLayout(_firstSettingsRow);
    _firstSettingsRow->setLayout(_firstRowLayout);
    _firstRowLayout->setContentsMargins(0, 0, 0, 0);
    _firstRowLayout->setSpacing(0);
    _mainLayout->addWidget(_firstSettingsRow);
    
    // _viewerLayers = new ComboBox(_firstSettingsRow);
    //_firstRowLayout->addWidget(_viewerLayers);
    
    _viewerChannels = new ComboBox(_firstSettingsRow);
    _viewerChannels->setToolTip("<p></br><b>Channels: \n</b></p>"
                                "The channels to display on the viewer.");
    _firstRowLayout->addWidget(_viewerChannels);
    
    _viewerChannels->addItem("Luminance",QIcon(),QKeySequence(Qt::Key_Y));
    _viewerChannels->addItem("RGBA",QIcon(),QKeySequence(Qt::SHIFT+Qt::Key_R));
    _viewerChannels->addItem("R",QIcon(),QKeySequence(Qt::Key_R));
    _viewerChannels->addItem("G",QIcon(),QKeySequence(Qt::Key_G));
    _viewerChannels->addItem("B",QIcon(),QKeySequence(Qt::Key_B));
    _viewerChannels->addItem("A",QIcon(),QKeySequence(Qt::Key_A));
    _viewerChannels->setCurrentIndex(1);
    QObject::connect(_viewerChannels, SIGNAL(currentIndexChanged(int)), this, SLOT(onViewerChannelsChanged(int)));
    
    _zoomCombobox = new ComboBox(_firstSettingsRow);
    _zoomCombobox->setToolTip("<p></br><b>Zoom: \n</b></p>"
                              "The zoom applied to the image on the viewer.");
    _zoomCombobox->addItem("10%");
    _zoomCombobox->addItem("25%");
    _zoomCombobox->addItem("50%");
    _zoomCombobox->addItem("75%");
    _zoomCombobox->addItem("100%");
    _zoomCombobox->addItem("125%");
    _zoomCombobox->addItem("150%");
    _zoomCombobox->addItem("200%");
    _zoomCombobox->addItem("400%");
    _zoomCombobox->addItem("800%");
    _zoomCombobox->addItem("1600%");
    _zoomCombobox->addItem("2400%");
    _zoomCombobox->addItem("3200%");
    _zoomCombobox->addItem("6400%");
    
    _firstRowLayout->addWidget(_zoomCombobox);
    
    _centerViewerButton = new Button(_firstSettingsRow);
    _firstRowLayout->addWidget(_centerViewerButton);
    
    
    _clipToProjectFormatButton = new Button(_firstSettingsRow);
    _clipToProjectFormatButton->setCheckable(true);
    _clipToProjectFormatButton->setChecked(true);
    _clipToProjectFormatButton->setDown(true);
    _firstRowLayout->addWidget(_clipToProjectFormatButton);
    
    _firstRowLayout->addStretch();
    
    /*2nd row of buttons*/
    _secondSettingsRow = new QWidget(this);
    _secondRowLayout = new QHBoxLayout(_secondSettingsRow);
    _secondSettingsRow->setLayout(_secondRowLayout);
    _secondRowLayout->setSpacing(0);
    _secondRowLayout->setContentsMargins(0, 0, 0, 0);
    //  _secondSettingsRow->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _mainLayout->addWidget(_secondSettingsRow);
    
    _gainBox = new SpinBox(_secondSettingsRow,SpinBox::DOUBLE_SPINBOX);
    _gainBox->setToolTip("<p></br><b>Gain: \n</b></p>"
                         "Multiplies the image by \nthis amount before display.");
    _gainBox->setIncrement(0.1);
    _gainBox->setValue(1.0);
    _gainBox->setMinimum(0.0);
    _secondRowLayout->addWidget(_gainBox);
    
    
    _gainSlider=new ScaleSlider(0, 64,1.0,Natron::LINEAR_SCALE,_secondSettingsRow);
    _gainSlider->setToolTip("<p></br><b>Gain: \n</b></p>"
                            "Multiplies the image by \nthis amount before display.");
    _secondRowLayout->addWidget(_gainSlider);
    
    
    _refreshButton = new Button(_secondSettingsRow);
    _refreshButton->setToolTip("Force a new render of the current frame."
                               "<p></br><b>Keyboard shortcut: U</b></p>");
    _secondRowLayout->addWidget(_refreshButton);
    
    _viewerColorSpace=new ComboBox(_secondSettingsRow);
    _viewerColorSpace->setToolTip("<p></br><b>Viewer color process: \n</b></p>"
                                  "The operation applied to the image before it is displayed\n"
                                  "on screen. All the color pipeline \n"
                                  "is linear,thus the process converts from linear\n"
                                  "to your monitor's colorspace.");
    _secondRowLayout->addWidget(_viewerColorSpace);
    
    _viewerColorSpace->addItem("Linear(None)");
    _viewerColorSpace->addItem("sRGB");
    _viewerColorSpace->addItem("Rec.709");
    _viewerColorSpace->setCurrentIndex(1);
    
    _viewsComboBox = new ComboBox(_secondSettingsRow);
    _viewsComboBox->setToolTip("<p></br><b>Active view: \n</b></p>"
                               "Tells the viewer what view should be displayed.");
    _secondRowLayout->addWidget(_viewsComboBox);
    _viewsComboBox->hide();
    int viewsCount = _gui->getApp()->getProjectViewsCount(); //getProjectViewsCount
    updateViewsMenu(viewsCount);
    
    _secondRowLayout->addStretch();
    
    /*=============================================*/
    
    /*OpenGL viewer*/
    viewer = new ViewerGL(this);
    _mainLayout->addWidget(viewer);
    /*=============================================*/
    /*info bbox & color*/
    _infosWidget = new InfoViewerWidget(viewer,this);
    //  _infosWidget->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _mainLayout->addWidget(_infosWidget);
    viewer->setInfoViewer(_infosWidget);
    /*=============================================*/
    
    /*Player buttons*/
    _playerButtonsContainer = new QWidget(this);
    _playerLayout=new QHBoxLayout(_playerButtonsContainer);
    _playerLayout->setSpacing(0);
    _playerLayout->setContentsMargins(0, 0, 0, 0);
    _playerButtonsContainer->setLayout(_playerLayout);
    //   _playerButtonsContainer->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _mainLayout->addWidget(_playerButtonsContainer);
    
    _currentFrameBox=new SpinBox(_playerButtonsContainer,SpinBox::INT_SPINBOX);
    _currentFrameBox->setValue(0);
    _currentFrameBox->setToolTip("<p></br><b>Current frame number</b></p>");
    _playerLayout->addWidget(_currentFrameBox);
    
    _playerLayout->addStretch();
    
    firstFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence firstFrameKey(Qt::CTRL + Qt::Key_Left);
    QString tooltip = "First frame";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(firstFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    firstFrame_Button->setToolTip(tooltip);
    _playerLayout->addWidget(firstFrame_Button);
    
    
    previousKeyFrame_Button=new Button(_playerButtonsContainer);
    previousKeyFrame_Button->hide();
    QKeySequence previousKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Left);
    tooltip = "Previous keyframe";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(previousKeyFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    previousKeyFrame_Button->setToolTip(tooltip);
    _playerLayout->addWidget(previousKeyFrame_Button);
    
    
    play_Backward_Button=new Button(_playerButtonsContainer);
    QKeySequence playbackFrameKey(Qt::Key_J);
    tooltip = "Play backward";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(playbackFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    play_Backward_Button->setToolTip(tooltip);
    play_Backward_Button->setCheckable(true);
    _playerLayout->addWidget(play_Backward_Button);
    
    
    previousFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence previousFrameKey(Qt::Key_Left);
    tooltip = "Previous frame";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(previousFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    previousFrame_Button->setToolTip(tooltip);
    _playerLayout->addWidget(previousFrame_Button);
    
    
    stop_Button = new Button(_playerButtonsContainer);
    QKeySequence stopKey(Qt::Key_K);
    tooltip = "Stop";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(stopKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    stop_Button->setToolTip(tooltip);
    _playerLayout->addWidget(stop_Button);
    
    
    nextFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence nextFrameKey(Qt::Key_Right);
    tooltip = "Next frame";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(nextFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    nextFrame_Button->setToolTip(tooltip);
    _playerLayout->addWidget(nextFrame_Button);
    
    
    play_Forward_Button = new Button(_playerButtonsContainer);
    QKeySequence playKey(Qt::Key_L);
    tooltip = "Play forward";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(playKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    play_Forward_Button->setToolTip(tooltip);
    play_Forward_Button->setCheckable(true);
    _playerLayout->addWidget(play_Forward_Button);
    
    
    nextKeyFrame_Button = new Button(_playerButtonsContainer);
    nextKeyFrame_Button->hide();
    QKeySequence nextKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Right);
    tooltip = "Next keyframe";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(nextKeyFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    nextKeyFrame_Button->setToolTip(tooltip);
    _playerLayout->addWidget(nextKeyFrame_Button);
    
    
    lastFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence lastFrameKey(Qt::CTRL + Qt::Key_Right);
    tooltip = "Last frame";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(lastFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    lastFrame_Button->setToolTip(tooltip);
    _playerLayout->addWidget(lastFrame_Button);
    
    
    _playerLayout->addStretch();
    
    
    previousIncrement_Button = new Button(_playerButtonsContainer);
    QKeySequence previousIncrFrameKey(Qt::SHIFT + Qt::Key_Left);
    tooltip = "Previous increment";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(previousIncrFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    previousIncrement_Button->setToolTip(tooltip);
    _playerLayout->addWidget(previousIncrement_Button);
    
    
    incrementSpinBox=new SpinBox(_playerButtonsContainer);
    incrementSpinBox->setValue(10);
    incrementSpinBox->setToolTip("<p></br><b>Frame increment: \n</b></p>"
                                 "The previous/next increment buttons step"
                                 " with this increment.");
    _playerLayout->addWidget(incrementSpinBox);
    
    
    nextIncrement_Button = new Button(_playerButtonsContainer);
    QKeySequence nextIncrFrameKey(Qt::SHIFT + Qt::Key_Right);
    tooltip = "Next increment";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(nextIncrFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    nextIncrement_Button->setToolTip(tooltip);
    _playerLayout->addWidget(nextIncrement_Button);
    
    loopMode_Button = new Button(_playerButtonsContainer);
    loopMode_Button->setCheckable(true);
    loopMode_Button->setChecked(true);
    loopMode_Button->setDown(true);
    loopMode_Button->setToolTip("Behaviour to adopt when the playback\n hit the end of the range: loop or stop.");
    _playerLayout->addWidget(loopMode_Button);
    
    
    _playerLayout->addStretch();
    
    fpsName = new QLabel("fps",_playerButtonsContainer);
    _playerLayout->addWidget(fpsName);
    fpsBox = new SpinBox(_playerButtonsContainer,SpinBox::DOUBLE_SPINBOX);
    fpsBox->decimals(1);
    fpsBox->setValue(24.0);
    fpsBox->setIncrement(0.1);
    fpsBox->setToolTip("<p></br><b>fps: \n</b></p>"
                       "Enter here the desired playback rate.");
    _playerLayout->addWidget(fpsBox);
    
    
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
    
    firstFrame_Button->setIcon(QIcon(pixFirst));
    previousKeyFrame_Button->setIcon(QIcon(pixPrevKF));
    play_Backward_Button->setIcon(QIcon(pixRewind));
    previousFrame_Button->setIcon(QIcon(pixBack1));
    stop_Button->setIcon(QIcon(pixStop));
    nextFrame_Button->setIcon(QIcon(pixForward1));
    play_Forward_Button->setIcon(QIcon(pixPlay));
    nextKeyFrame_Button->setIcon(QIcon(pixNextKF));
    lastFrame_Button->setIcon(QIcon(pixLast));
    previousIncrement_Button->setIcon(QIcon(pixPrevIncr));
    nextIncrement_Button->setIcon(QIcon(pixNextIncr));
    _refreshButton->setIcon(QIcon(pixRefresh));
    _centerViewerButton->setIcon(QIcon(pixCenterViewer));
    loopMode_Button->setIcon(QIcon(pixLoopMode));
    _clipToProjectFormatButton->setIcon(QIcon(pixClipToProject));
    
    
    _centerViewerButton->setToolTip("Scales the image so it doesn't exceed the size of the viewer and centers it."
                                    "<p></br><b>Keyboard shortcut: F</b></p>");
    
    _clipToProjectFormatButton->setToolTip("<p> Clips the portion of the image displayed <br/>"
                                           "on the viewer to the project format. <br/>"
                                           "When off, everything in the union of all nodes <br/>"
                                           "region of definition will be displayed. <br/> <br/>"
                                           "<b>Keyboard shortcut: C</b></p>");
    /*=================================================*/
    
    /*frame seeker*/
    _timeLineGui = new TimeLineGui(_gui->getApp()->getTimeLine(),this);
    _timeLineGui->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _mainLayout->addWidget(_timeLineGui);
    /*================================================*/
    
    
    /*slots & signals*/
    QObject::connect(_viewerColorSpace, SIGNAL(currentIndexChanged(QString)), _viewerNode,SLOT(onColorSpaceChanged(QString)));
    QObject::connect(_zoomCombobox, SIGNAL(currentIndexChanged(QString)),viewer, SLOT(zoomSlot(QString)));
    QObject::connect(viewer, SIGNAL(zoomChanged(int)), this, SLOT(updateZoomComboBox(int)));
    QObject::connect(_gainBox, SIGNAL(valueChanged(double)), _viewerNode,SLOT(onExposureChanged(double)));
    QObject::connect(_gainSlider, SIGNAL(positionChanged(double)), _gainBox, SLOT(setValue(double)));
    QObject::connect(_gainSlider, SIGNAL(positionChanged(double)), _viewerNode, SLOT(onExposureChanged(double)));
    QObject::connect(_gainBox, SIGNAL(valueChanged(double)), _gainSlider, SLOT(seekScalePosition(double)));
    QObject::connect(_currentFrameBox, SIGNAL(valueChanged(double)), this, SLOT(onCurrentTimeSpinBoxChanged(double)));
    
    VideoEngine* vengine = _viewerNode->getVideoEngine().get();
    
    QObject::connect(play_Forward_Button,SIGNAL(clicked(bool)),this,SLOT(startPause(bool)));
    QObject::connect(stop_Button,SIGNAL(clicked()),this,SLOT(abortRendering()));
    QObject::connect(play_Backward_Button,SIGNAL(clicked(bool)),this,SLOT(startBackward(bool)));
    QObject::connect(previousFrame_Button,SIGNAL(clicked()),this,SLOT(previousFrame()));
    QObject::connect(nextFrame_Button,SIGNAL(clicked()),this,SLOT(nextFrame()));
    QObject::connect(previousIncrement_Button,SIGNAL(clicked()),this,SLOT(previousIncrement()));
    QObject::connect(nextIncrement_Button,SIGNAL(clicked()),this,SLOT(nextIncrement()));
    QObject::connect(firstFrame_Button,SIGNAL(clicked()),this,SLOT(firstFrame()));
    QObject::connect(lastFrame_Button,SIGNAL(clicked()),this,SLOT(lastFrame()));
    QObject::connect(loopMode_Button, SIGNAL(clicked(bool)), this, SLOT(toggleLoopMode(bool)));
    QObject::connect(_gui->getApp()->getTimeLine().get(),SIGNAL(frameChanged(SequenceTime,int)),
                     this, SLOT(onTimeLineTimeChanged(SequenceTime,int)));
    QObject::connect(_viewerNode,SIGNAL(addedCachedFrame(SequenceTime)),_timeLineGui,SLOT(onCachedFrameAdded(SequenceTime)));
    QObject::connect(_viewerNode,SIGNAL(removedLRUCachedFrame()),_timeLineGui,SLOT(onLRUCachedFrameRemoved()));
    QObject::connect(appPTR,SIGNAL(imageRemovedFromViewerCache(SequenceTime)),_timeLineGui,SLOT(onCachedFrameRemoved(SequenceTime)));
    QObject::connect(_viewerNode,SIGNAL(clearedViewerCache()),_timeLineGui,SLOT(onCachedFramesCleared()));
    QObject::connect(_refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    
    QObject::connect(_centerViewerButton, SIGNAL(clicked()), this, SLOT(centerViewer()));
    QObject::connect(_viewerNode,SIGNAL(viewerDisconnected()),this,SLOT(disconnectViewer()));
    QObject::connect(fpsBox, SIGNAL(valueChanged(double)), vengine, SLOT(setDesiredFPS(double)));
    QObject::connect(vengine, SIGNAL(fpsChanged(double,double)), _infosWidget, SLOT(setFps(double,double)));
    QObject::connect(vengine,SIGNAL(engineStopped()),_infosWidget,SLOT(hideFps()));
    QObject::connect(vengine, SIGNAL(engineStarted(bool)), this, SLOT(onEngineStarted(bool)));
    QObject::connect(vengine, SIGNAL(engineStopped()), this, SLOT(onEngineStopped()));
    
    
    QObject::connect(_viewerNode, SIGNAL(mustRedraw()), viewer, SLOT(updateGL()));
    QObject::connect(_viewerNode, SIGNAL(mustSwapBuffers()), viewer, SLOT(doSwapBuffers()));
    
    QObject::connect(_clipToProjectFormatButton,SIGNAL(clicked(bool)),this,SLOT(onClipToProjectButtonToggle(bool)));
    
    QObject::connect(_viewsComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(showView(int)));
    
}

void ViewerTab::updateViewsMenu(int count){
    int currentIndex = _viewsComboBox->activeIndex();
    _viewsComboBox->clear();
    if(count == 1){
        _viewsComboBox->hide();
        _viewsComboBox->addItem("Main");
        
    }else if(count == 2){
        _viewsComboBox->show();
        _viewsComboBox->addItem("Left",QIcon(),QKeySequence(Qt::CTRL + Qt::Key_1));
        _viewsComboBox->addItem("Right",QIcon(),QKeySequence(Qt::CTRL + Qt::Key_2));
    }else{
        _viewsComboBox->show();
        for(int i = 0 ; i < count;++i){
            _viewsComboBox->addItem(QString("View ")+QString::number(i+1),QIcon(),Gui::keySequenceForView(i));
        }
    }
    if(currentIndex < _viewsComboBox->count() && currentIndex != -1){
        _viewsComboBox->setCurrentIndex(currentIndex);
    }else{
        _viewsComboBox->setCurrentIndex(0);
    }
    _gui->updateViewsActions(count);
}

void ViewerTab::setCurrentView(int view){
    _viewsComboBox->setCurrentIndex(view);
}
int ViewerTab::getCurrentView() const{
    return _viewsComboBox->activeIndex();
}

void ViewerTab::toggleLoopMode(bool b){
    loopMode_Button->setDown(b);
    _viewerNode->getVideoEngine()->toggleLoopMode(b);
}

void ViewerTab::onClipToProjectButtonToggle(bool b){
    _clipToProjectFormatButton->setDown(b);
    viewer->setClipToDisplayWindow(b);
}

void ViewerTab::onEngineStarted(bool forward){
    play_Forward_Button->setChecked(forward);
    play_Forward_Button->setDown(forward);
    play_Backward_Button->setChecked(!forward);
    play_Backward_Button->setDown(!forward);
}

void ViewerTab::onEngineStopped(){
    play_Forward_Button->setChecked(false);
    play_Forward_Button->setDown(false);
    play_Backward_Button->setChecked(false);
    play_Backward_Button->setDown(false);
}

void ViewerTab::updateZoomComboBox(int value){
    assert(value > 0);
    QString str = QString::number(value);
    str.append(QChar('%'));
    str.prepend("  ");
    str.append("  ");
    _zoomCombobox->setCurrentText(str);
}

/*In case they're several viewer around, we need to reset the dag and tell it
 explicitly we want to use this viewer and not another one.*/
void ViewerTab::startPause(bool b){
    abortRendering();
    if(b){
        _viewerNode->getVideoEngine()->render(-1, /*frame count*/
                                              true,/*rebuild tree?*/
                                              false, /*fit to viewer ?*/
                                              true, /*forward ?*/
                                              false); /*same frame ?*/
    }
}
void ViewerTab::abortRendering(){
    _viewerNode->getVideoEngine()->abortRendering();
}
void ViewerTab::startBackward(bool b){
    abortRendering();
    if(b){
        _viewerNode->getVideoEngine()->render(-1, /*frame count*/
                                              true,/*rebuild tree?*/
                                              false,/*fit to viewer?*/
                                              false,/*forward?*/
                                              false);/*same frame ?*/
    }
}
void ViewerTab::seek(SequenceTime time){
    _currentFrameBox->setValue(time);
    _timeLineGui->seek(time);
    _viewerNode->refreshAndContinueRender();
}

void ViewerTab::previousFrame(){
    seek(_timeLineGui->currentFrame()-1);
}
void ViewerTab::nextFrame(){
    seek(_timeLineGui->currentFrame()+1);
}
void ViewerTab::previousIncrement(){
    seek(_timeLineGui->currentFrame()-incrementSpinBox->value());
}
void ViewerTab::nextIncrement(){
    seek(_timeLineGui->currentFrame()+incrementSpinBox->value());
}
void ViewerTab::firstFrame(){
    seek(_timeLineGui->leftBound());
}
void ViewerTab::lastFrame(){
    seek(_timeLineGui->rightBound());
}

void ViewerTab::onTimeLineTimeChanged(SequenceTime time,int /*reason*/){
    _currentFrameBox->setValue(time);
    //_viewerNode->refreshAndContinueRender();
}

void ViewerTab::onCurrentTimeSpinBoxChanged(double time){
    _timeLineGui->seek(time);
}


void ViewerTab::centerViewer(){
    if(viewer->displayingImage()){
        _viewerNode->refreshAndContinueRender(true);
        
    }else{
        viewer->fitToFormat(viewer->getDisplayWindow());
        viewer->updateGL();
    }
}

void ViewerTab::refresh(){
    _viewerNode->forceFullComputationOnNextFrame();
    _viewerNode->updateTreeAndRender();
}

ViewerTab::~ViewerTab()
{
    _viewerNode->setUiContext(NULL);
}


void ViewerTab::keyPressEvent ( QKeyEvent * event ){

    if(event->key() == Qt::Key_Space){
        if(parentWidget()){
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);
            QCoreApplication::postEvent(parentWidget(),ev);
        }
    }else if(event->key() == Qt::Key_Y){
        _viewerChannels->setCurrentIndex(0);
    }else if(event->key() == Qt::Key_R && event->modifiers() == Qt::ShiftModifier ){
        _viewerChannels->setCurrentIndex(1);
    }else if(event->key() == Qt::Key_R && event->modifiers() == Qt::NoModifier){
        _viewerChannels->setCurrentIndex(2);
    }else if(event->key() == Qt::Key_G){
        _viewerChannels->setCurrentIndex(3);
    }else if(event->key() == Qt::Key_B){
        _viewerChannels->setCurrentIndex(4);
    }else if(event->key() == Qt::Key_A){
        _viewerChannels->setCurrentIndex(5);
    }else if(event->key() == Qt::Key_J){
        startBackward(!play_Backward_Button->isDown());
    }
    else if(event->key() == Qt::Key_Left && !event->modifiers().testFlag(Qt::ShiftModifier)
                                         && !event->modifiers().testFlag(Qt::ControlModifier)){
        previousFrame();
    }
    else if(event->key() == Qt::Key_K){
        abortRendering();
    }
    else if(event->key() == Qt::Key_Right  && !event->modifiers().testFlag(Qt::ShiftModifier)
                                            && !event->modifiers().testFlag(Qt::ControlModifier)){
        nextFrame();
    }
    else if(event->key() == Qt::Key_L){
        startPause(!play_Forward_Button->isDown());
        
    }else if(event->key() == Qt::Key_Left && event->modifiers().testFlag(Qt::ShiftModifier)){
        //prev incr
        previousIncrement();
    }
    else if(event->key() == Qt::Key_Right && event->modifiers().testFlag(Qt::ShiftModifier)){
        //next incr
        nextIncrement();
    }else if(event->key() == Qt::Key_Left && event->modifiers().testFlag(Qt::ControlModifier)){
        //first frame
        firstFrame();
    }
    else if(event->key() == Qt::Key_Right && event->modifiers().testFlag(Qt::ControlModifier)){
        //last frame
        lastFrame();
    }
    else if(event->key() == Qt::Key_Left && event->modifiers().testFlag(Qt::ControlModifier)
            && event->modifiers().testFlag(Qt::ShiftModifier)){
        //prev key
    }
    else if(event->key() == Qt::Key_Right && event->modifiers().testFlag(Qt::ControlModifier)
            &&  event->modifiers().testFlag(Qt::ShiftModifier)){
        //next key
    } else if(event->key() == Qt::Key_F){
        centerViewer();
        
    }else if(event->key() == Qt::Key_C){
        onClipToProjectButtonToggle(!_clipToProjectFormatButton->isDown());
    }
    else if(event->key() == Qt::Key_U){
        refresh();
    }
    
}

void ViewerTab::onViewerChannelsChanged(int i){
    ViewerInstance::DisplayChannels channels;
    switch (i) {
        case 0:
            channels = ViewerInstance::LUMINANCE;
            break;
        case 1:
            channels = ViewerInstance::RGBA;
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
            channels = ViewerInstance::RGBA;
            break;
    }
    _viewerNode->setDisplayChannels(channels);
}
bool ViewerTab::eventFilter(QObject *target, QEvent *event){
    if (event->type() == QEvent::MouseButtonPress) {
        _gui->selectNode(_gui->getApp()->getNodeGui(_viewerNode->getNode()));
        
    }
    return QWidget::eventFilter(target, event);
}

void ViewerTab::disconnectViewer(){
    viewer->disconnectViewer();
}



QSize ViewerTab::minimumSizeHint() const{
    return QWidget::minimumSizeHint();
}
QSize ViewerTab::sizeHint() const{
    return QWidget::sizeHint();
}


void ViewerTab::showView(int /*view*/){
    abortRendering();
    _viewerNode->refreshAndContinueRender();
}
