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

#include "Global/GlobalDefines.h"
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
class ScaleSliderQWidget;
class TimeLineGui;
class ViewerInstance;
class Gui;
class RectI;

class ViewerTab: public QWidget 
{ 
    Q_OBJECT
public:
    explicit ViewerTab(Gui* gui,ViewerInstance* node,QWidget* parent=0);
    
	virtual ~ViewerTab() OVERRIDE;
    
    
    ViewerInstance* getInternalNode() const {return _viewerNode;}
public:
    Gui* getGui() const {return _gui;}
    
    void setCurrentView(int view);
    
    int getCurrentView() const;
    
    void seek(SequenceTime time);
    
    /**
     *@brief Tells all the nodes in the grpah to draw their overlays
     **/
    /*All the overlay methods are forwarding calls to the default node instance*/
    void drawOverlays() const;
    
    bool notifyOverlaysPenDown(const QPointF& viewportPos,const QPointF& pos);
    
    bool notifyOverlaysPenMotion(const QPointF& viewportPos,const QPointF& pos);
    
    bool notifyOverlaysPenUp(const QPointF& viewportPos,const QPointF& pos);
    
    bool notifyOverlaysKeyDown(QKeyEvent* e);
    
    bool notifyOverlaysKeyUp(QKeyEvent* e);
    
    bool notifyOverlaysKeyRepeat(QKeyEvent* e);
    
    bool notifyOverlaysFocusGained();
    
    bool notifyOverlaysFocusLost();
    
    
    
    ////////
    /////////////The following functions are used when serializing/deserializing the project gui
    ///////////// so the viewer can restore the exact same settings to the user.
    bool isClippedToProject() const;
    
    std::string getColorSpace() const;
    
    void setUserRoIEnabled(bool b);
    
    void setUserRoI(const RectI& r);
    
    void setClipToProject(bool b);
    
    void setColorSpace(const std::string& colorSpaceName);
    
    void setExposure(double d);
    
    double getExposure() const;
    
    std::string getChannelsString() const;
    
    void setChannels(const std::string& channelsStr);

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
    void onTimeLineTimeChanged(SequenceTime time,int);
    void onCurrentTimeSpinBoxChanged(double);
    /*Updates the comboBox according to the real zoomFactor. Value is in % */
    void updateZoomComboBox(int value);
    
    /*makes the viewer black*/
    void disconnectViewer();
    
    
    void onEngineStarted(bool forward,int frameCount);
    
    void onEngineStopped();
    
    void refresh();
    
    void updateViewsMenu(int count);
    
    void showView(int view);
    
    void onEnableViewerRoIButtonToggle(bool);


private:
        
    bool eventFilter(QObject *target, QEvent *event);
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    virtual void enterEvent(QEvent*) OVERRIDE FINAL { setFocus(); }

    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;


public:
    // FIXME: public pointer members are the sign of a serious design flaw!!! at least use a getter!
	/*OpenGL viewer*/
	ViewerGL* viewer; // FIXME: used by ViewerInstance
private:
    // FIXME: PIMPL
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

    /*2nd row*/
    SpinBox* _gainBox;
    ScaleSliderQWidget* _gainSlider;
    Button* _refreshButton;
    ComboBox* _viewerColorSpace;
    ComboBox* _viewsComboBox;

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
};

#endif // NATRON_GUI_VIEWERTAB_H_
