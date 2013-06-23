//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <QtWidgets/QApplication>
#include <QtWidgets/QSlider>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QAbstractItemView>
#include "Gui/viewerTab.h"
#include "Gui/GLViewer.h"
#include "Gui/InfoViewerWidget.h"
#include "Superviser/controler.h"
#include "Core/VideoEngine.h"
#include "Gui/FeedbackSpinBox.h"
#include "Core/model.h"
#include "Gui/timeline.h"
#include "Core/settings.h"
#include "Gui/ScaleSlider.h"
#include "Gui/comboBox.h"
#include "Core/viewerNode.h"
#include "Gui/Button.h"
ViewerTab::ViewerTab(Viewer* node,QWidget* parent):QWidget(parent),_viewerNode(node)
{
    
    setObjectName(node->getName());
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
   // _firstSettingsRow->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _mainLayout->addWidget(_firstSettingsRow);
    
    _viewerLayers = new ComboBox(_firstSettingsRow);
    _firstRowLayout->addWidget(_viewerLayers);
    
    _viewerChannels = new ComboBox(_firstSettingsRow);
	_firstRowLayout->addWidget(_viewerChannels);
    
    _viewerChannels->addItem("Luminance",QIcon(),QKeySequence(Qt::Key_Y));
    _viewerChannels->addItem("RGB",QIcon(),QKeySequence(Qt::SHIFT+Qt::Key_R));
    _viewerChannels->addItem("R",QIcon(),QKeySequence(Qt::Key_R));
    _viewerChannels->addItem("G",QIcon(),QKeySequence(Qt::Key_G));
    _viewerChannels->addItem("B",QIcon(),QKeySequence(Qt::Key_B));
    _viewerChannels->addItem("A",QIcon(),QKeySequence(Qt::Key_A));
     
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
    
    _firstRowLayout->addStretch();
    
    /*2nd row of buttons*/
    _secondSettingsRow = new QWidget(this);
    _secondRowLayout = new QHBoxLayout(_secondSettingsRow);
    _secondSettingsRow->setLayout(_secondRowLayout);
    _secondRowLayout->setSpacing(0);
    _secondRowLayout->setContentsMargins(0, 0, 0, 0);
  //  _secondSettingsRow->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _mainLayout->addWidget(_secondSettingsRow);
    
    _gainBox = new FeedBackSpinBox(_secondSettingsRow,true);
    _gainBox->setIncrement(0.1);
    _gainBox->setValue(1.0);
    _gainBox->setMinimum(-99.0);
    _secondRowLayout->addWidget(_gainBox);
    
    
    _gainSlider=new ScaleSlider(0, 64, 100,1.0,Powiter_Enums::LOG_SCALE,5,_secondSettingsRow);
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
	viewer=new ViewerGL(this,this);
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
    
	_currentFrameBox=new FeedBackSpinBox(_playerButtonsContainer,true);
    _currentFrameBox->setValue(0);
    _currentFrameBox->setMinimum(0);
    _currentFrameBox->setMaximum(0);
    _currentFrameBox->setToolTip(QApplication::translate("MainWindow", "<html><head/><body><p>Frame number</p></body></html>" ));
	_playerLayout->addWidget(_currentFrameBox);
    
	_playerLayout->addStretch();
    
	firstFrame_Button = new Button(_playerButtonsContainer);
	_playerLayout->addWidget(firstFrame_Button);
    
    
    previousKeyFrame_Button=new Button(_playerButtonsContainer);
    _playerLayout->addWidget(previousKeyFrame_Button);
    
    
    play_Backward_Button=new Button(_playerButtonsContainer);
    play_Backward_Button->setCheckable(true);
    _playerLayout->addWidget(play_Backward_Button);
    
    
	previousFrame_Button = new Button(_playerButtonsContainer);
	_playerLayout->addWidget(previousFrame_Button);
    
    
    stop_Button = new Button(_playerButtonsContainer);
	_playerLayout->addWidget(stop_Button);
    
    
    nextFrame_Button = new Button(_playerButtonsContainer);
	_playerLayout->addWidget(nextFrame_Button);
    
    
	play_Forward_Button = new Button(_playerButtonsContainer);
    play_Forward_Button->setCheckable(true);
	_playerLayout->addWidget(play_Forward_Button);
	
    
    nextKeyFrame_Button = new Button(_playerButtonsContainer);
	_playerLayout->addWidget(nextKeyFrame_Button);
    
    
	lastFrame_Button = new Button(_playerButtonsContainer);
	_playerLayout->addWidget(lastFrame_Button);
    
    
	_playerLayout->addStretch();
    
    
    previousIncrement_Button = new Button(_playerButtonsContainer);
	_playerLayout->addWidget(previousIncrement_Button);
    
    
    incrementSpinBox=new FeedBackSpinBox(_playerButtonsContainer);
    incrementSpinBox->setValue(10);
    _playerLayout->addWidget(incrementSpinBox);
    
    
    nextIncrement_Button = new Button(_playerButtonsContainer);
	_playerLayout->addWidget(nextIncrement_Button);
    
    
    fpsName = new QLabel("fps",_playerButtonsContainer);
    _playerLayout->addWidget(fpsName);
    fpsBox = new FeedBackSpinBox(_playerButtonsContainer,true);
    fpsBox->decimals(1);
    fpsBox->setValue(24.0);
    fpsBox->setIncrement(0.1);
    _playerLayout->addWidget(fpsBox);
        
    

    QImage imgFirst(IMAGES_PATH"firstFrame.png");
    QImage imgPrevKF(IMAGES_PATH"prevKF.png");
    QImage imgRewind(IMAGES_PATH"rewind.png");
    QImage imgBack1(IMAGES_PATH"back1.png");
    QImage imgStop(IMAGES_PATH"stop.png");
    QImage imgForward1(IMAGES_PATH"forward1.png");
    QImage imgPlay(IMAGES_PATH"play.png");
    QImage imgNextKF(IMAGES_PATH"nextKF.png");
    QImage imgLast(IMAGES_PATH"lastFrame.png");
    QImage imgPrevINCR(IMAGES_PATH"previousIncr.png");
    QImage imgNextINCR(IMAGES_PATH"nextIncr.png");
    QImage imgRefresh(IMAGES_PATH"refresh.png");

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
    
    int iW=20,iH=20;
    pixFirst.scaled(iW,iH);
    pixPrevKF.scaled(iW,iH);
    pixRewind.scaled(iW,iH);
    pixBack1.scaled(iW,iH);
    pixStop.scaled(iW,iH);
    pixForward1.scaled(iW,iH);
    pixPlay.scaled(iW,iH);
    pixNextKF.scaled(iW,iH);
    pixLast.scaled(iW,iH);
    pixPrevIncr.scaled(iW,iH);
    pixNextIncr.scaled(iW,iH);
    pixRefresh.scaled(iW, iH);
    
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
    
    firstFrame_Button->setFixedSize(iW, iH);
    previousKeyFrame_Button->setFixedSize(iW, iH);
    play_Backward_Button->setFixedSize(iW, iH);
    previousFrame_Button->setFixedSize(iW, iH);
    stop_Button->setFixedSize(iW, iH);
    nextFrame_Button->setFixedSize(iW, iH);
    play_Forward_Button->setFixedSize(iW, iH);
    nextKeyFrame_Button->setFixedSize(iW, iH);
    lastFrame_Button->setFixedSize(iW, iH);
    previousIncrement_Button->setFixedSize(iW, iH);
    nextIncrement_Button->setFixedSize(iW, iH);
    _refreshButton->setFixedSize(iW, iH);
    
	/*=================================================*/
    
	/*frame seeker*/
	frameSeeker = new TimeLine(this);
  //  frameSeeker->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
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
    
    VideoEngine* vengine = ctrlPTR->getModel()->getVideoEngine();
    
    QObject::connect(vengine, SIGNAL(fpsChanged(double)), _infosWidget, SLOT(setFps(double)));
    QObject::connect(fpsBox, SIGNAL(valueChanged(double)),vengine, SLOT(setDesiredFPS(double)));
    QObject::connect(play_Forward_Button,SIGNAL(toggled(bool)),this,SLOT(startPause(bool)));
    QObject::connect(stop_Button,SIGNAL(clicked()),this,SLOT(abort()));
    QObject::connect(play_Backward_Button,SIGNAL(toggled(bool)),this,SLOT(startBackward(bool)));
    QObject::connect(previousFrame_Button,SIGNAL(clicked()),this,SLOT(previousFrame()));
    QObject::connect(nextFrame_Button,SIGNAL(clicked()),this,SLOT(nextFrame()));
    QObject::connect(previousIncrement_Button,SIGNAL(clicked()),this,SLOT(previousIncrement()));
    QObject::connect(nextIncrement_Button,SIGNAL(clicked()),this,SLOT(nextIncrement()));
    QObject::connect(firstFrame_Button,SIGNAL(clicked()),this,SLOT(firstFrame()));
    QObject::connect(lastFrame_Button,SIGNAL(clicked()),this,SLOT(lastFrame()));
    QObject::connect(_currentFrameBox,SIGNAL(valueChanged(double)),this,SLOT(seekRandomFrame(double)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), this, SLOT(seekRandomFrame(int)));
    
}

void ViewerTab::updateZoomComboBox(int value){
    QString str = QString::number(value);
    str.append(QChar('%'));
    str.prepend("  ");
    str.append("  ");
    _zoomCombobox->setCurrentText(str);
}

/*In case they're several viewer around, we need to reset the dag and tell it
 explicitly we want to use this viewer and not another one.*/
void ViewerTab::startPause(bool b){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->startPause(b);
    else
        play_Forward_Button->setChecked(false);
}
void ViewerTab::abort(){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    ctrlPTR->getModel()->getVideoEngine()->abort();
}
void ViewerTab::startBackward(bool b){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->startBackward(b);
    else
        play_Backward_Button->setChecked(false);
}
void ViewerTab::previousFrame(){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->previousFrame();
}
void ViewerTab::nextFrame(){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->nextFrame();
}
void ViewerTab::previousIncrement(){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->previousIncrement();
}
void ViewerTab::nextIncrement(){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->nextIncrement();
}
void ViewerTab::firstFrame(){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->firstFrame();
}
void ViewerTab::lastFrame(){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->lastFrame();
}
void ViewerTab::seekRandomFrame(int f){
    ctrlPTR->getModel()->getVideoEngine()->resetAndMakeNewDag(_viewerNode,true);
    if(ctrlPTR->getModel()->getVideoEngine()->dagHasInputs())
        ctrlPTR->getModel()->getVideoEngine()->seekRandomFrame(f);
}

ViewerTab::~ViewerTab()
{
}

void ViewerTab::setTextureCache(TextureCache* cache){
    viewer->setTextureCache(cache);
}
 