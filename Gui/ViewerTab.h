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
class AppInstance;
class SpinBox;
class ScaleSlider;
class TimeLineGui;
class ViewerNode;
class Gui;
class ViewerTab: public QWidget 
{ 
    Q_OBJECT
    
    Gui* _gui;
    
    ViewerNode* _viewerNode;// < pointer to the internal node
    
    Powiter::ChannelSet _channelsToDraw;
    
    /*True if the viewer is currently fullscreen*/
	bool _maximized;
    
public:
    explicit ViewerTab(Gui* gui,ViewerNode* node,QWidget* parent=0);
    
	virtual ~ViewerTab();
    
    
    ViewerNode* getInternalNode(){return _viewerNode;}
    
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

    /*2nd row*/
    SpinBox* _gainBox;
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
	TimeLineGui* frameSeeker;
    
    /*these are the channels the viewer wants to display*/
	const Powiter::ChannelSet& displayChannels(){return _channelsToDraw;}
    
    Gui* getGui() const {return _gui;}
    
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
public slots:
    
    void startPause(bool);
    void abortRendering();
    void startBackward(bool);
    void previousFrame();
    void nextFrame();
    void previousIncrement();
    void nextIncrement();
    void firstFrame();
    void lastFrame();
    void seek(int);
    void seek(double value){seek((int)value);}
    void centerViewer();
    void toggleLoopMode(bool);
    void onViewerChannelsChanged(int);
    void onClipToProjectButtonToggle(bool);

    /*Updates the comboBox according to the real zoomFactor. Value is in % */
    void updateZoomComboBox(int value);
    
    /*makes the viewer black*/
    void disconnectViewer();
    
    void onCachedFrameAdded(int);
    void onCachedFrameRemoved();
    void onViewerCacheCleared();
    
    void onEngineStarted(bool forward);
    
    void onEngineStopped();
    
    void refresh();
    
    
protected:
    
    bool eventFilter(QObject *target, QEvent *event);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void enterEvent(QEvent* e);
    virtual void leaveEvent(QEvent* e);
    
};

#endif // POWITER_GUI_VIEWERTAB_H_
