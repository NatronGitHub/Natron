//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GUI_VIEWERTAB_H_
#define POWITER_GUI_VIEWERTAB_H_ 

#include <QWidget>

#include "Engine/ChannelSet.h"

/*The ViewerTab encapsulates a viewer with all the graphical interface surrounding it. It should be instantiable as
 a tab , and several ViewerTab should run in parallel seemlessly.*/


class Button;
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
class FeedbackSpinBox;
class ScaleSlider;
class TimeLine;
class ViewerNode;
class ViewerInfos;
class ViewerTab: public QWidget 
{
    Q_OBJECT
    
    
    ViewerNode* _viewerNode;// < pointer to the internal node
    
    ChannelSet _channelsToDraw;
    
    /*True if the viewer is currently fullscreen*/
	bool _maximized;
    
public:
    explicit ViewerTab(ViewerNode* node,QWidget* parent=0);
    
	virtual ~ViewerTab();
    
    
    ViewerNode* getInternalNode(){return _viewerNode;}
    
    QVBoxLayout* _mainLayout;

	/*Viewer Settings*/
    QWidget* _firstSettingsRow,*_secondSettingsRow;
    QHBoxLayout* _firstRowLayout,*_secondRowLayout;
    
    /*1st row*/
	ComboBox* _viewerLayers;
	ComboBox* _viewerChannels;
    ComboBox* _zoomCombobox;
    Button* _centerViewerButton;

    /*2nd row*/
    FeedbackSpinBox* _gainBox;
    ScaleSlider* _gainSlider;
    Button* _refreshButton;
    ComboBox* _viewerColorSpace;

	/*OpenGL viewer*/
	ViewerGL* viewer;
    /*Infos*/
    InfoViewerWidget* _infosWidget;
    

	/*TimeLine buttons*/
    QWidget* _playerButtonsContainer;
	QHBoxLayout* _playerLayout;
	FeedbackSpinBox* _currentFrameBox;
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
    FeedbackSpinBox* incrementSpinBox;
    Button* nextIncrement_Button;
    QLabel* fpsName;
    FeedbackSpinBox* fpsBox;
    
	/*frame seeker*/
	TimeLine* frameSeeker;
    
    /*these are the channels the viewer wants to display*/
	const ChannelSet& displayChannels(){return _channelsToDraw;}
    
    /*viewerInfo related functions)*/
    void setCurrentViewerInfos(ViewerInfos *viewerInfos,bool onInit=false);
    
    
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
    void centerViewer();
    
    void onViewerChannelsChanged(int);
    
    /*Updates the comboBox according to the real zoomFactor. Value is in % */
    void updateZoomComboBox(int value);
    
signals:
    void recenteringNeeded();
    
protected:
    
    virtual void keyPressEvent(QKeyEvent* e);
};

#endif // POWITER_GUI_VIEWERTAB_H_
