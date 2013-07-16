//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com
#ifndef __VIEWER_TAB_H_
#define __VIEWER_TAB_H_ 

#include "Core/channels.h"
#include <QtWidgets/QWidget>

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
class FeedBackSpinBox;
class ScaleSlider;
class TimeLine;
class TextureCache;
class Viewer;
class ViewerInfos;
class ViewerTab: public QWidget 
{
    Q_OBJECT
    
    
    Viewer* _viewerNode;// < pointer to the internal node
    
    ChannelMask _channelsToDraw;
    
    /*True if the viewer is currently fullscreen*/
	bool _fullscreen;
    
public:
	ViewerTab(Viewer* node,QWidget* parent=0);
    
	virtual ~ViewerTab();
    
    void setTextureCache(TextureCache* cache);
    
    Viewer* getInternalNode(){return _viewerNode;}
    
    QVBoxLayout* _mainLayout;

	/*Viewer Settings*/
    QWidget* _firstSettingsRow,*_secondSettingsRow;
    QHBoxLayout* _firstRowLayout,*_secondRowLayout;
    
    /*1st row*/
	ComboBox* _viewerLayers;
	ComboBox* _viewerChannels;
    ComboBox* _zoomCombobox;

    /*2nd row*/
    FeedBackSpinBox* _gainBox;
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
	FeedBackSpinBox* _currentFrameBox;
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
    FeedBackSpinBox* incrementSpinBox;
    Button* nextIncrement_Button;
    QLabel* fpsName;
    FeedBackSpinBox* fpsBox;
    
	/*frame seeker*/
	TimeLine* frameSeeker;
    
    /*these are the channels the viewer wants to display*/
	const ChannelMask& displayChannels(){return _channelsToDraw;}
    
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
    
    /*Updates the comboBox according to the real zoomFactor. Value is in % */
    void updateZoomComboBox(int value);
    
protected:
    
    virtual void keyPressEvent(QKeyEvent* e);
};

#endif // __VIEWER_TAB_H_