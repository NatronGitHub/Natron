//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_VIEWERTAB_H_
#define NATRON_GUI_VIEWERTAB_H_ 

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
class ViewerInstance;
class Gui;
class ViewerTab: public QWidget 
{ 
    Q_OBJECT
    
    Gui* _gui;
    
    ViewerInstance* _viewerNode;// < pointer to the internal node
    
    Natron::ChannelSet _channelsToDraw;

    
public:
    explicit ViewerTab(Gui* gui,ViewerInstance* node,QWidget* parent=0);
    
	virtual ~ViewerTab();
    
    
    ViewerInstance* getInternalNode(){return _viewerNode;}
    
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
    ComboBox* _viewsComboBox;
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
    TimeLineGui* _timeLineGui;
    
    /*these are the channels the viewer wants to display*/
	const Natron::ChannelSet& displayChannels(){return _channelsToDraw;}
    
    Gui* getGui() const {return _gui;}
    
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
    
    void setCurrentView(int view);
    
    int getCurrentView() const;
    
    void seek(SequenceTime time);

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
    void centerViewer();
    void toggleLoopMode(bool);
    void onViewerChannelsChanged(int);
    void onClipToProjectButtonToggle(bool);
    void onTimeLineTimeChanged(SequenceTime time);
    void onCurrentTimeSpinBoxChanged(double);
    /*Updates the comboBox according to the real zoomFactor. Value is in % */
    void updateZoomComboBox(int value);
    
    /*makes the viewer black*/
    void disconnectViewer();
    
    
    void onEngineStarted(bool forward);
    
    void onEngineStopped();
    
    void refresh();
    
    void updateViewsMenu(int count);
    
    void showView(int view);


protected:
    
    bool eventFilter(QObject *target, QEvent *event);
    
    virtual void keyPressEvent(QKeyEvent* e);
    
};

#endif // NATRON_GUI_VIEWERTAB_H_
