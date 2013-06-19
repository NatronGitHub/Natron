//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef __VIEWER_TAB_H_
#define __VIEWER_TAB_H_ 

#include <QtWidgets/QWidget>

/*The ViewerTab encapsulates a viewer with all the graphical interface surrounding it. It should be instantiable as
 a tab , and several ViewerTab should run in parallel seemlessly.*/


class QPushButton;
class QVBoxLayout;
class QSlider;
class ComboBox;
class QHBoxLayout;
class QSpacerItem;
class QGridLayout;
class QLabel;
class QGroupBox;
class ViewerGL;
class InfoViewerWidget;
class Controler;
class FeedBackSpinBox;
class ScaleSlider;
class TimeSlider;
class TextureCache;
class Viewer;
class ViewerTab: public QWidget 
{
    Q_OBJECT
    
    
    Viewer* _viewerNode;// < pointer to the internal node
    bool initialized_;
    
public:
	ViewerTab(Viewer* node,QWidget* parent=0);
	virtual ~ViewerTab();
    
    void setTextureCache(TextureCache* cache);
    QVBoxLayout* viewer_tabLayout;

	/*Viewer Settings*/
	QGroupBox* viewerSettings;
    QWidget* layoutContainer;
    QWidget* firstRow,*secondRow;
    QHBoxLayout* layoutFirst,*layoutSecond;
    QVBoxLayout* layoutContainer_layout;
	QPushButton* refreshButton;
	ComboBox* viewerLayers;
	ComboBox* viewerChannels;
	ComboBox* dimensionChoosal;
    ComboBox* viewerColorSpace;
    FeedBackSpinBox* gainBox;
    FeedBackSpinBox* zoomSpinbox;
    ScaleSlider* gainSlider;
    QLabel* zoomName;

	/*openGL viewer*/
	ViewerGL* viewer;
    InfoViewerWidget* _infosWidget;
    

	/*timeLine buttons*/
	QGroupBox* timeButtons;
	QHBoxLayout* timeButtonsLayout;
	FeedBackSpinBox* frameNumberBox;
	QSpacerItem* spacerOnLeft;
	QPushButton* firstFrame_Button;
    QPushButton* previousKeyFrame_Button;
    QPushButton* play_Backward_Button;
	QPushButton* previousFrame_Button;
    QPushButton* stop_Button;
    QPushButton* nextFrame_Button;
	QPushButton* play_Forward_Button;
    QPushButton* nextKeyFrame_Button;
	QPushButton* lastFrame_Button;
    QPushButton* previousIncrement_Button;
    FeedBackSpinBox* incrementSpinBox;
    QPushButton* nextIncrement_Button;
	QSpacerItem* spacerOnRight;
    QLabel* fpsName;
    FeedBackSpinBox* fpsBox;
    
	/*frame seeker*/
	TimeSlider* frameSeeker;
    
    bool initialized(){return initialized_;}
    
    Viewer* getInternalNode(){return _viewerNode;}
    
public slots:
    
    void startPause(bool);
    void abort();
    void startBackward(bool);
    void previousFrame();
    void nextFrame();
    void previousIncrement();
    void nextIncrement();
    void firstFrame();
    void lastFrame();
    void seekRandomFrame(int);
    void seekRandomFrame(double value){seekRandomFrame((int)value);}
};

#endif // __VIEWER_TAB_H_