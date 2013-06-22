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
ViewerTab::ViewerTab(Viewer* node,QWidget* parent):QWidget(parent),initialized_(false),_viewerNode(node)
{
    
    setObjectName(node->getName());
    viewer_tabLayout=new QVBoxLayout(this);
    setLayout(viewer_tabLayout);
    viewer_tabLayout->QLayout::setSpacing(0);
	/*VIEWER SETTINGS*/
	viewerSettings=new QGroupBox(this);
    QHBoxLayout* viewerSettingsLayout = new QHBoxLayout(viewerSettings);
    viewerSettingsLayout->setSpacing(0);
    viewerSettingsLayout->setContentsMargins(0, 0, 0, 0);
    layoutContainer = new QWidget(viewerSettings);
    layoutContainer_layout = new QVBoxLayout(layoutContainer);
    firstRow = new QWidget(layoutContainer);
    secondRow = new QWidget(layoutContainer);
    layoutFirst = new QHBoxLayout(firstRow);
    layoutSecond = new QHBoxLayout(secondRow);
    layoutContainer_layout->QLayout::setSpacing(0);
    layoutContainer_layout->setContentsMargins(0, 0, 0, 0);
    layoutFirst->setSpacing(10);
    layoutSecond->setSpacing(10);
    layoutFirst->setContentsMargins(0, 0, 0, 0);
    layoutSecond->setContentsMargins(0, 0, 0, 0);
    firstRow->setLayout(layoutFirst);
    secondRow->setLayout(layoutSecond);
    layoutContainer_layout->addWidget(firstRow);
    layoutContainer_layout->addWidget(secondRow);
    layoutContainer->setLayout(layoutContainer_layout);
    
    //viewerSettings->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	viewerSettings->setObjectName(QString::fromUtf8("viewerSettings"));
    //
    viewerLayers = new ComboBox(viewerSettings);
	viewerLayers->setObjectName(QString::fromUtf8("viewerLayers"));
    layoutFirst->addWidget(viewerLayers);

    //
    viewerChannels = new ComboBox(viewerSettings);
	viewerChannels->setObjectName(QString::fromUtf8("viewerChannels"));
	layoutFirst->addWidget(viewerChannels);
    //
    dimensionChoosal=new ComboBox(viewerSettings);
	dimensionChoosal->setObjectName(QString::fromUtf8("dimensionChoosal"));
	layoutFirst->addWidget(dimensionChoosal);
    //
    zoomName=new QLabel("zoom:",viewerSettings);
	layoutFirst->addWidget(zoomName);
    //
    zoomSpinbox=new FeedBackSpinBox(viewerSettings);
    zoomSpinbox->setMaximum(3000);
    zoomSpinbox->setMinimum(10);
    zoomSpinbox->setIncrement(10);
    zoomSpinbox->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
	layoutFirst->addWidget(zoomSpinbox);
    
	

    gainBox = new FeedBackSpinBox(viewerSettings,true);
    gainBox->setIncrement(0.1);
    gainBox->setValue(1.0);
    gainBox->setMinimum(-99.0);
    gainBox->setObjectName(QString::fromUtf8("gainBox"));
    
    layoutSecond->addWidget(gainBox);
    
    //
    gainSlider=new ScaleSlider(0, 64, 100,1.0,Powiter_Enums::LOG_SCALE,5,viewerSettings);
    layoutSecond->addWidget(gainSlider);
    //
	refreshButton = new Button(viewerSettings);
	refreshButton->setObjectName(QString::fromUtf8("refreshButtons"));
    layoutSecond->addWidget(refreshButton);
    //
		
    viewerColorSpace=new ComboBox(viewerSettings);
    viewerColorSpace->setObjectName("ViewerColorSpace");
   
    layoutSecond->addWidget(viewerColorSpace);
    
    viewerSettingsLayout->addWidget(layoutContainer);
    viewerSettings->setLayout(viewerSettingsLayout);
   // viewerSettings->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
	viewer_tabLayout->addWidget(viewerSettings);
    
	viewerChannels->clear();
	viewerChannels->insertItems(0, QStringList()
                                << QApplication::translate("MainWindow", "rgba" )
                                << QApplication::translate("MainWindow", "r" )
                                << QApplication::translate("MainWindow", "g" )
                                << QApplication::translate("MainWindow", "b" )
                                );
	dimensionChoosal->clear();
	dimensionChoosal->insertItems(0,QStringList()
                                  << QApplication::translate("MainWindow", "2D" )
                                  << QApplication::translate("MainWindow", "3D" )
                                  
                                  );
    viewerColorSpace->insertItems(0, QStringList()
                                  << QString("Linear(None)")
                                  << QString("sRGB")
                                  << QString("Rec.709")
                                  );
    
    
    int index= viewerColorSpace->findText(QString("sRGB"));
    viewerColorSpace->setCurrentIndex(index);
    
	/*=============================================*/
    
	/*openGL viewer*/
	viewer=new ViewerGL(this);
	viewer->setObjectName(QString::fromUtf8("viewer"));
    viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	viewer_tabLayout->addWidget(viewer);
	/*=============================================*/
    /*info bbox & color*/
    _infosWidget = new InfoViewerWidget(viewer,this);
    viewer_tabLayout->addWidget(_infosWidget);
    viewer->setInfoViewer(_infosWidget);
    /*=============================================*/
	/*timeLine buttons*/
	timeButtons=new QGroupBox(this);
	timeButtons->setObjectName(QString::fromUtf8("timeButtons"));
	timeButtonsLayout=new QHBoxLayout(timeButtons);
	timeButtonsLayout->setObjectName(QString::fromUtf8("timeButtonsLayout"));
    //
	frameNumberBox=new FeedBackSpinBox(timeButtons,true);
	frameNumberBox->setObjectName(QString::fromUtf8("frameNumberBox"));
    frameNumberBox->setValue(0);
    frameNumberBox->setMinimum(0);
    frameNumberBox->setMaximum(0);
	timeButtonsLayout->addWidget(frameNumberBox);
    //
	timeButtonsLayout->addStretch();
    
	firstFrame_Button = new Button(timeButtons);
	firstFrame_Button->setObjectName(QString::fromUtf8("firstFrame_Button"));
	timeButtonsLayout->addWidget(firstFrame_Button);
    //
    previousKeyFrame_Button=new Button(timeButtons);
    previousKeyFrame_Button->setObjectName(QString::fromUtf8("previousKeyFrame_Button"));
    timeButtonsLayout->addWidget(previousKeyFrame_Button);
    //
    play_Backward_Button=new Button(timeButtons);
    play_Backward_Button->setObjectName(QString::fromUtf8("play_Backward_Button"));
    play_Backward_Button->setCheckable(true);
    timeButtonsLayout->addWidget(play_Backward_Button);
    //
	previousFrame_Button = new Button(timeButtons);
	previousFrame_Button->setObjectName(QString::fromUtf8("previousFrame_Button"));
	timeButtonsLayout->addWidget(previousFrame_Button);
    //
    stop_Button = new Button(timeButtons);
	stop_Button->setObjectName(QString::fromUtf8("stop_Button"));
	timeButtonsLayout->addWidget(stop_Button);
    //
    nextFrame_Button = new Button(timeButtons);
	nextFrame_Button->setObjectName(QString::fromUtf8("nextFrame_Button"));
	timeButtonsLayout->addWidget(nextFrame_Button);
    //
	play_Forward_Button = new Button(timeButtons);
	play_Forward_Button->setObjectName(QString::fromUtf8("play_Forward_Button"));
    play_Forward_Button->setCheckable(true);
	timeButtonsLayout->addWidget(play_Forward_Button);
	//
    nextKeyFrame_Button = new Button(timeButtons);
	nextKeyFrame_Button->setObjectName(QString::fromUtf8("nextKeyFrame_Button"));
	timeButtonsLayout->addWidget(nextKeyFrame_Button);
    //
	lastFrame_Button = new Button(timeButtons);
	lastFrame_Button->setObjectName(QString::fromUtf8("lastFrame_Button"));
	timeButtonsLayout->addWidget(lastFrame_Button);
    //
	timeButtonsLayout->addStretch();
    //
    previousIncrement_Button = new Button(timeButtons);
	previousIncrement_Button->setObjectName(QString::fromUtf8("previousIncrement_Button"));
	timeButtonsLayout->addWidget(previousIncrement_Button);
    //
    incrementSpinBox=new FeedBackSpinBox(timeButtons);
    incrementSpinBox->setObjectName(QString::fromUtf8("incrementSpinBox"));
    incrementSpinBox->setValue(10);
    timeButtonsLayout->addWidget(incrementSpinBox);
    //
    nextIncrement_Button = new Button(timeButtons);
	nextIncrement_Button->setObjectName(QString::fromUtf8("nextIncrement_Button"));
	timeButtonsLayout->addWidget(nextIncrement_Button);
    //
    fpsName = new QLabel("fps",timeButtons);
    timeButtonsLayout->addWidget(fpsName);
    fpsBox = new FeedBackSpinBox(timeButtons,true);
    fpsBox->decimals(1);
    fpsBox->setValue(24.0);
    fpsBox->setIncrement(0.1);
    timeButtonsLayout->addWidget(fpsBox);
        
    timeButtons->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	viewer_tabLayout->addWidget(timeButtons);
    timeButtonsLayout->setSpacing(0);
    timeButtonsLayout->setContentsMargins(0, 0, 0, 0);
    timeButtons->setContentsMargins(0, 0, 0, 0);
	timeButtons->setLayout(timeButtonsLayout);
    

#ifndef QT_NO_TOOLTIP
	frameNumberBox->setToolTip(QApplication::translate("MainWindow", "<html><head/><body><p>Frame number</p></body></html>" ));
#endif // QT_NO_TOOLTIP
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
    refreshButton->setIcon(QIcon(pixRefresh));
    
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
    refreshButton->setFixedSize(iW, iH);
    
	/*=================================================*/
    
	/*frame seeker*/
	frameSeeker = new TimeLine(this);
	frameSeeker->setObjectName(QString::fromUtf8("frameSeeker"));
	viewer_tabLayout->addWidget(frameSeeker);
    viewer_tabLayout->setContentsMargins(0, 0, 0, 0);
	/*================================================*/
    
    
    /*slots & signals*/
    QObject::connect(viewerColorSpace, SIGNAL(activated(QString)), viewer,SLOT(updateColorSpace(QString)));
    QObject::connect(zoomSpinbox, SIGNAL(valueChanged(double)),viewer, SLOT(zoomSlot(double)));
    QObject::connect(viewer, SIGNAL(zoomChanged(int)), zoomSpinbox, SLOT(setValue(int)));
    QObject::connect(viewer,SIGNAL(frameChanged(int)),frameNumberBox,SLOT(setValue(int)));
    QObject::connect(viewer,SIGNAL(frameChanged(int)),frameSeeker,SLOT(seek(int)));
    QObject::connect(gainBox, SIGNAL(valueChanged(double)), viewer,SLOT(updateExposure(double)));
    QObject::connect(gainSlider, SIGNAL(positionChanged(double)), gainBox, SLOT(setValue(double)));
    QObject::connect(gainSlider, SIGNAL(positionChanged(double)), viewer, SLOT(updateExposure(double)));
    QObject::connect(gainBox, SIGNAL(valueChanged(double)), gainSlider, SLOT(seekScalePosition(double)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), frameNumberBox, SLOT(setValue(int)));
    QObject::connect(frameNumberBox, SIGNAL(valueChanged(double)), frameSeeker, SLOT(seek(double)));
    
    VideoEngine* vengine = ctrlPTR->getModel()->getVideoEngine();
    
    QObject::connect(vengine, SIGNAL(fpsChanged(double)), fpsBox, SLOT(setValue(double)));
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
    QObject::connect(frameNumberBox,SIGNAL(valueChanged(double)),this,SLOT(seekRandomFrame(double)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), this, SLOT(seekRandomFrame(int)));
    
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

 