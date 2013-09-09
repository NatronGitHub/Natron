//  Powiter
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
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#include <QtGui/QKeyEvent>
#include <QtGui/QKeySequence>

#include "Engine/ViewerNode.h"
#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/Model.h"

#include "Gui/ViewerGL.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/Timeline.h"
#include "Gui/ScaleSlider.h"
#include "Gui/ComboBox.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"

#include "Global/AppManager.h"

using namespace Powiter;

ViewerTab::ViewerTab(Gui* gui,ViewerNode* node,QWidget* parent):QWidget(parent),
_gui(gui),
_viewerNode(node),
_channelsToDraw(Mask_RGBA),
_maximized(false)
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
    
    _viewerLayers = new ComboBox(_firstSettingsRow);
    _firstRowLayout->addWidget(_viewerLayers);
    
    _viewerChannels = new ComboBox(_firstSettingsRow);
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
    
    _firstRowLayout->addStretch();
    
    /*2nd row of buttons*/
    _secondSettingsRow = new QWidget(this);
    _secondRowLayout = new QHBoxLayout(_secondSettingsRow);
    _secondSettingsRow->setLayout(_secondRowLayout);
    _secondRowLayout->setSpacing(0);
    _secondRowLayout->setContentsMargins(0, 0, 0, 0);
  //  _secondSettingsRow->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _mainLayout->addWidget(_secondSettingsRow);
    
    _gainBox = new FeedbackSpinBox(_secondSettingsRow,true);
    _gainBox->setIncrement(0.1);
    _gainBox->setValue(1.0);
    _gainBox->setMinimum(-99.0);
    _secondRowLayout->addWidget(_gainBox);
    
    
    _gainSlider=new ScaleSlider(0, 64, 100,1.0,Powiter::LOG_SCALE,5,_secondSettingsRow);
    _secondRowLayout->addWidget(_gainSlider);
    
    
	_refreshButton = new Button(_secondSettingsRow);
    _secondRowLayout->addWidget(_refreshButton);
    
    _viewerColorSpace=new ComboBox(_secondSettingsRow);
    _secondRowLayout->addWidget(_viewerColorSpace);
    
    _viewerColorSpace->addItem("Linear(None)");
    _viewerColorSpace->addItem("sRGB");
    _viewerColorSpace->addItem("Rec.709");
    _viewerColorSpace->setCurrentIndex(1);
  
    
    _secondRowLayout->addStretch();
    
	/*=============================================*/
    
	/*OpenGL viewer*/
	viewer=new ViewerGL(this);
    viewer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
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
    
	_currentFrameBox=new FeedbackSpinBox(_playerButtonsContainer,true);
    _currentFrameBox->setValue(0);
    _currentFrameBox->setMinimum(0);
    _currentFrameBox->setMaximum(0);
    _currentFrameBox->setToolTip("<p></br><b>Current frame number</b></p>");
	_playerLayout->addWidget(_currentFrameBox);
    
	_playerLayout->addStretch();
    
	firstFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence firstFrameKey(Qt::CTRL + Qt::Key_Left);
    firstFrame_Button->setShortcut(firstFrameKey);
    QString tooltip = "First frame";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(firstFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    firstFrame_Button->setToolTip(tooltip);
	_playerLayout->addWidget(firstFrame_Button);
    
    
    previousKeyFrame_Button=new Button(_playerButtonsContainer);
    QKeySequence previousKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Left);
    previousKeyFrame_Button->setShortcut(previousKeyFrameKey);
    tooltip = "Previous keyframe";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(previousKeyFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    previousKeyFrame_Button->setToolTip(tooltip);
    _playerLayout->addWidget(previousKeyFrame_Button);
    
    
    play_Backward_Button=new Button(_playerButtonsContainer);
    QKeySequence playbackFrameKey(Qt::Key_J);
    play_Backward_Button->setShortcut(playbackFrameKey);
    tooltip = "Play backward";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(playbackFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    play_Backward_Button->setToolTip(tooltip);
    play_Backward_Button->setCheckable(true);
    _playerLayout->addWidget(play_Backward_Button);
    
    
	previousFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence previousFrameKey(Qt::Key_Left);
    previousFrame_Button->setShortcut(previousFrameKey);
    tooltip = "Previous frame";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(previousFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    previousFrame_Button->setToolTip(tooltip);
	_playerLayout->addWidget(previousFrame_Button);
    
    
    stop_Button = new Button(_playerButtonsContainer);
    QKeySequence stopKey(Qt::Key_K);
    stop_Button->setShortcut(stopKey);
    tooltip = "Stop";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(stopKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    stop_Button->setToolTip(tooltip);
	_playerLayout->addWidget(stop_Button);
    
    
    nextFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence nextFrameKey(Qt::Key_Right);
    nextFrame_Button->setShortcut(nextFrameKey);
    tooltip = "Next frame";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(nextFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    nextFrame_Button->setToolTip(tooltip);
	_playerLayout->addWidget(nextFrame_Button);
    
    
	play_Forward_Button = new Button(_playerButtonsContainer);
    QKeySequence playKey(Qt::Key_L);
    play_Forward_Button->setShortcut(playKey);
    tooltip = "Play forward";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(playKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    play_Forward_Button->setToolTip(tooltip);
    play_Forward_Button->setCheckable(true);
	_playerLayout->addWidget(play_Forward_Button);
	
    
    nextKeyFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence nextKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Right);
    nextKeyFrame_Button->setShortcut(nextKeyFrameKey);
    tooltip = "Next keyframe";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(nextKeyFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    nextKeyFrame_Button->setToolTip(tooltip);
	_playerLayout->addWidget(nextKeyFrame_Button);
    
    
	lastFrame_Button = new Button(_playerButtonsContainer);
    QKeySequence lastFrameKey(Qt::CTRL + Qt::Key_Right);
    lastFrame_Button->setShortcut(lastFrameKey);
    tooltip = "Last frame";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(lastFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    lastFrame_Button->setToolTip(tooltip);
	_playerLayout->addWidget(lastFrame_Button);
    
    
	_playerLayout->addStretch();
    
    
    previousIncrement_Button = new Button(_playerButtonsContainer);
    QKeySequence previousIncrFrameKey(Qt::SHIFT + Qt::Key_Left);
    previousIncrement_Button->setShortcut(previousIncrFrameKey);
    tooltip = "Previous increment";
    tooltip.append("<p></br><b>Keyboard shortcut: ");
    tooltip.append(previousIncrFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    previousIncrement_Button->setToolTip(tooltip);
	_playerLayout->addWidget(previousIncrement_Button);
    
    
    incrementSpinBox=new FeedbackSpinBox(_playerButtonsContainer);
    incrementSpinBox->setValue(10);
    incrementSpinBox->setToolTip("<p></br><b>Frame increment: \n</b></p>"
                                 "The previous/next increment buttons step"
                                 " with this increment.");
    _playerLayout->addWidget(incrementSpinBox);
    
    
    nextIncrement_Button = new Button(_playerButtonsContainer);
    QKeySequence nextIncrFrameKey(Qt::SHIFT + Qt::Key_Right);
    nextIncrement_Button->setShortcut(nextIncrFrameKey);
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
    fpsBox = new FeedbackSpinBox(_playerButtonsContainer,true);
    fpsBox->decimals(1);
    fpsBox->setValue(24.0);
    fpsBox->setIncrement(0.1);
    fpsBox->setToolTip("<p></br><b>fps: \n</b></p>"
                                 "Enter here the desired playback rate.");
    _playerLayout->addWidget(fpsBox);
        
    

    QImage imgFirst(POWITER_IMAGES_PATH"firstFrame.png");
    QImage imgPrevKF(POWITER_IMAGES_PATH"prevKF.png");
    QImage imgRewind(POWITER_IMAGES_PATH"rewind.png");
    QImage imgBack1(POWITER_IMAGES_PATH"back1.png");
    QImage imgStop(POWITER_IMAGES_PATH"stop.png");
    QImage imgForward1(POWITER_IMAGES_PATH"forward1.png");
    QImage imgPlay(POWITER_IMAGES_PATH"play.png");
    QImage imgNextKF(POWITER_IMAGES_PATH"nextKF.png");
    QImage imgLast(POWITER_IMAGES_PATH"lastFrame.png");
    QImage imgPrevINCR(POWITER_IMAGES_PATH"previousIncr.png");
    QImage imgNextINCR(POWITER_IMAGES_PATH"nextIncr.png");
    QImage imgRefresh(POWITER_IMAGES_PATH"refresh.png");
    QImage imgCenterViewer(POWITER_IMAGES_PATH"centerViewer.png");
    QImage imgLoopMode(POWITER_IMAGES_PATH"loopmode.png");
    
    QPixmap pixFirst=QPixmap::fromImage(imgFirst);
    QPixmap pixPrevKF=QPixmap::fromImage(imgPrevKF);
    QPixmap pixRewind=QPixmap::fromImage(imgRewind);
    QPixmap pixBack1=QPixmap::fromImage(imgBack1);
    QPixmap pixStop=QPixmap::fromImage(imgStop);
    QPixmap pixForward1=QPixmap::fromImage(imgForward1);
    QPixmap pixPlay=QPixmap::fromImage(imgPlay);
    QPixmap pixNextKF=QPixmap::fromImage(imgNextKF);
    QPixmap pixLast=QPixmap::fromImage(imgLast);
    QPixmap pixPrevIncr=QPixmap::fromImage(imgPrevINCR);
    QPixmap pixNextIncr=QPixmap::fromImage(imgNextINCR);
    QPixmap pixRefresh = QPixmap::fromImage(imgRefresh);
    QPixmap pixCenterViewer = QPixmap::fromImage(imgCenterViewer);
    QPixmap pixLoopMode = QPixmap::fromImage(imgLoopMode);
    
    int iW=20,iH=20;
    pixFirst = pixFirst.scaled(iW,iH);
    pixPrevKF = pixPrevKF.scaled(iW,iH);
    pixRewind = pixRewind.scaled(iW,iH);
    pixBack1 = pixBack1.scaled(iW,iH);
    pixStop = pixStop.scaled(iW,iH);
    pixForward1 = pixForward1.scaled(iW,iH);
    pixPlay = pixPlay.scaled(iW,iH);
    pixNextKF = pixNextKF.scaled(iW,iH);
    pixLast = pixLast.scaled(iW,iH);
    pixPrevIncr = pixPrevIncr.scaled(iW,iH);
    pixNextIncr = pixNextIncr.scaled(iW,iH);
    pixRefresh = pixRefresh.scaled(iW, iH);
    pixCenterViewer = pixCenterViewer.scaled(50, 50);
    pixLoopMode = pixLoopMode.scaled(iW, iH);
    
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
    
    
    _centerViewerButton->setToolTip("Scale the image so it doesn't exceed the size of the viewer and center it."
                                    "<p></br><b>Keyboard shortcut: F</b></p>");
    _centerViewerButton->setShortcut(QKeySequence(Qt::Key_F));
    
	/*=================================================*/
    
	/*frame seeker*/
	frameSeeker = new TimeLineGui(_viewerNode->getTimeLine(),this);
    frameSeeker->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
	_mainLayout->addWidget(frameSeeker);
	/*================================================*/
    
    
    /*slots & signals*/
    QObject::connect(_viewerColorSpace, SIGNAL(currentIndexChanged(QString)), viewer,SLOT(updateColorSpace(QString)));
    QObject::connect(_zoomCombobox, SIGNAL(currentIndexChanged(QString)),viewer, SLOT(zoomSlot(QString)));
    QObject::connect(viewer, SIGNAL(zoomChanged(int)), this, SLOT(updateZoomComboBox(int)));
    QObject::connect(viewer,SIGNAL(frameChanged(int)),_currentFrameBox,SLOT(setValue(int)));
    QObject::connect(viewer,SIGNAL(frameChanged(int)),frameSeeker,SLOT(seek(int)));
    QObject::connect(_gainBox, SIGNAL(valueChanged(double)), viewer,SLOT(updateExposure(double)));
    QObject::connect(_gainSlider, SIGNAL(positionChanged(double)), _gainBox, SLOT(setValue(double)));
    QObject::connect(_gainSlider, SIGNAL(positionChanged(double)), viewer, SLOT(updateExposure(double)));
    QObject::connect(_gainBox, SIGNAL(valueChanged(double)), _gainSlider, SLOT(seekScalePosition(double)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), _currentFrameBox, SLOT(setValue(int)));
    QObject::connect(_currentFrameBox, SIGNAL(valueChanged(double)), frameSeeker, SLOT(seek(double)));
    
    VideoEngine* vengine = _viewerNode->getVideoEngine();
    
    QObject::connect(vengine, SIGNAL(fpsChanged(double)), _infosWidget, SLOT(setFps(double)));
    QObject::connect(fpsBox, SIGNAL(valueChanged(double)),vengine, SLOT(setDesiredFPS(double)));
    QObject::connect(play_Forward_Button,SIGNAL(clicked(bool)),this,SLOT(startPause(bool)));
    QObject::connect(stop_Button,SIGNAL(clicked()),this,SLOT(abort()));
    QObject::connect(play_Backward_Button,SIGNAL(clicked(bool)),this,SLOT(startBackward(bool)));
    QObject::connect(previousFrame_Button,SIGNAL(clicked()),this,SLOT(previousFrame()));
    QObject::connect(nextFrame_Button,SIGNAL(clicked()),this,SLOT(nextFrame()));
    QObject::connect(previousIncrement_Button,SIGNAL(clicked()),this,SLOT(previousIncrement()));
    QObject::connect(nextIncrement_Button,SIGNAL(clicked()),this,SLOT(nextIncrement()));
    QObject::connect(firstFrame_Button,SIGNAL(clicked()),this,SLOT(firstFrame()));
    QObject::connect(lastFrame_Button,SIGNAL(clicked()),this,SLOT(lastFrame()));
    QObject::connect(_currentFrameBox,SIGNAL(valueChanged(double)),this,SLOT(seekRandomFrame(double)));
    QObject::connect(loopMode_Button, SIGNAL(clicked(bool)), this, SLOT(toggleLoopMode(bool)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), this, SLOT(seekRandomFrame(int)));
    QObject::connect(viewer,SIGNAL(engineNeeded()),vengine,SLOT(repeatSameFrame()));
        
    QObject::connect(_centerViewerButton, SIGNAL(clicked()), this, SLOT(centerViewer()));
    QObject::connect(this, SIGNAL(recenteringNeeded()), vengine, SLOT(recenterViewer()));
    
}
void ViewerTab::toggleLoopMode(bool b){
    loopMode_Button->setDown(b);
    _viewerNode->getVideoEngine()->toggleLoopMode(b);
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
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->startPause(b);
    else
        play_Forward_Button->setChecked(false);
}
void ViewerTab::abort(){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    _viewerNode->getVideoEngine()->abort();
}
void ViewerTab::startBackward(bool b){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->startBackward(b);
    else
        play_Backward_Button->setChecked(false);
}
void ViewerTab::previousFrame(){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->previousFrame();
}
void ViewerTab::nextFrame(){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->nextFrame();
}
void ViewerTab::previousIncrement(){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->previousIncrement();
}
void ViewerTab::nextIncrement(){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->nextIncrement();
}
void ViewerTab::firstFrame(){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->firstFrame();
}
void ViewerTab::lastFrame(){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->lastFrame();
}
void ViewerTab::seekRandomFrame(int f){
    _gui->_appInstance->updateDAG(_viewerNode, true);
    if(_viewerNode->getVideoEngine()->dagHasInputs())
        _viewerNode->getVideoEngine()->seekRandomFrame(f);
}

void ViewerTab::centerViewer(){
    if(viewer->drawing()){
        emit recenteringNeeded();
    }else{
        viewer->fitToFormat(viewer->displayWindow());
        viewer->updateGL();
    }
}

ViewerTab::~ViewerTab()
{
    _viewerNode->setUiContext(NULL);
}


void ViewerTab::keyPressEvent ( QKeyEvent * event ){
    if(event->key() == Qt::Key_Space){
        releaseKeyboard();
        if(_maximized){
            _maximized=false;
            _gui->minimize();
        }else{
            _maximized=true;
            _gui->maximize(dynamic_cast<TabWidget*>(parentWidget()));
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
    }
    
    
}
void ViewerTab::setCurrentViewerInfos(ViewerInfos* viewerInfos,bool onInit){
    viewer->setCurrentViewerInfos(viewerInfos,onInit);
    if(!onInit){
        _currentFrameBox->setMaximum(viewerInfos->lastFrame());
        _currentFrameBox->setMinimum(viewerInfos->firstFrame());
        int curFirstFrame = frameSeeker->firstFrame();
        int curLastFrame =  frameSeeker->lastFrame();
        if ((viewerInfos->firstFrame() != curFirstFrame) || (viewerInfos->lastFrame() != curLastFrame)) {
            frameSeeker->setFrameRange(viewerInfos->firstFrame(), viewerInfos->lastFrame());
            frameSeeker->setBoundaries(viewerInfos->firstFrame(), viewerInfos->lastFrame());
        }
    }
}
void ViewerTab::onViewerChannelsChanged(int i){
    switch (i) {
        case 0:
            _channelsToDraw = Mask_RGBA;
            break;
        case 1:
            _channelsToDraw = Mask_RGBA;
            break;
        case 2:
            _channelsToDraw = Channel_red;
            break;
        case 3:
            _channelsToDraw = Channel_green;
            break;
        case 4:
            _channelsToDraw = Channel_blue;
            break;
        case 5:
            _channelsToDraw = Channel_alpha;
            break;
        default:
            break;
    }
    viewer->setDisplayChannel(_channelsToDraw, !i ? true : false);
}
bool ViewerTab::eventFilter(QObject *target, QEvent *event){
    if (event->type() == QEvent::MouseButtonPress) {
        _gui->selectNode(_gui->_appInstance->getNodeGui(_viewerNode));
        
    }
    return QWidget::eventFilter(target, event);
}
 
void ViewerTab::disconnectViewer(){
    viewer->disconnectViewer();
}