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

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Global/GlobalDefines.h"

class ViewerGL;
class ViewerInstance;
class Gui;
class RectI;
class Format;
class QMouseEvent;
class RotoGui;
class NodeGui;
struct RotoGuiSharedData;

struct ViewerTabPrivate;
class ViewerTab: public QWidget 
{ 
    Q_OBJECT
    
public:
    explicit ViewerTab(const std::list<NodeGui*> existingRotoNodes,
                       NodeGui* currentRoto,
                       Gui* gui,
                       ViewerInstance* node,
                       QWidget* parent=0);
    
	virtual ~ViewerTab() OVERRIDE;
    
    
    ViewerInstance* getInternalNode() const;

    Gui* getGui() const;
    
    ViewerGL* getViewer() const;
    
    void setCurrentView(int view);
    
    int getCurrentView() const;
    
    void seek(SequenceTime time);
    
    void notifyAppClosing();
    
    /**
     *@brief Tells all the nodes in the grpah to draw their overlays
     **/
    /*All the overlay methods are forwarding calls to the default node instance*/
    void drawOverlays(double scaleX,double scaleY) const;
    
    bool notifyOverlaysPenDown(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos,QMouseEvent* e);
    
    bool notifyOverlaysPenDoubleClick(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos);
    
    bool notifyOverlaysPenMotion(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos);
    
    bool notifyOverlaysPenUp(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos);
    
    bool notifyOverlaysKeyDown(double scaleX,double scaleY,QKeyEvent* e);
    
    bool notifyOverlaysKeyUp(double scaleX,double scaleY,QKeyEvent* e);
    
    bool notifyOverlaysKeyRepeat(double scaleX,double scaleY,QKeyEvent* e);
    
    bool notifyOverlaysFocusGained(double scaleX,double scaleY);
    
    bool notifyOverlaysFocusLost(double scaleX,double scaleY);
    
    
    
    ////////
    /////////////The following functions are used when serializing/deserializing the project gui
    ///////////// so the viewer can restore the exact same settings to the user.
    bool isClippedToProject() const;
    
    std::string getColorSpace() const;
    
    void setUserRoIEnabled(bool b);
    
    void setUserRoI(const RectI& r);
    
    void setClipToProject(bool b);
    
    void setColorSpace(const std::string& colorSpaceName);
    
    void setGain(double d);
    
    double getGain() const;
    
    std::string getChannelsString() const;
    
    void setChannels(const std::string& channelsStr);
    
    bool isAutoContrastEnabled() const;
    
    void setAutoContrastEnabled(bool b);
    
    void setMipMapLevel(int level);
    
    int getMipMapLevel() const;
    
    void setRenderScaleActivated(bool act);
    
    bool getRenderScaleActivated() const;
    
    void setZoomOrPannedSinceLastFit(bool enabled);
    
    bool getZoomOrPannedSinceLastFit() const;
    
    void setInfoBarResolution(const Format& f);
    
    void createRotoInterface(NodeGui* n);
    
    /**
     * @brief Set the current roto interface
     **/
    void setRotoInterface(NodeGui* n);
    
    void removeRotoInterface(NodeGui* n,bool permanantly,bool removeAndDontSetAnother);
    
    void getRotoContext(std::map<NodeGui*,RotoGui*>* rotoNodes,std::pair<NodeGui*,RotoGui*>* currentRoto) const;
    
    void updateRotoSelectedTool(int tool,RotoGui* sender);
    
    boost::shared_ptr<RotoGuiSharedData> getRotoGuiSharedData(NodeGui* node) const;
    
    void onRotoEvaluatedForThisViewer();
    
    Natron::ViewerCompositingOperator getCompositingOperator() const;
    
    void setCompositingOperator(Natron::ViewerCompositingOperator op);
    
    bool isFrameRangeLocked() const;
    
    bool isPlayingForward() const;
    bool isPlayingBackward() const;
    
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
    
    void onRenderScaleComboIndexChanged(int index);
    
    /*makes the viewer black*/
    void disconnectViewer();
    
    
    void onEngineStarted(bool forward,int frameCount);
    
    void onEngineStopped();
    
    void refresh();
    
    void updateViewsMenu(int count);
    
    void showView(int view);
    
    void onEnableViewerRoIButtonToggle(bool);
        
    void onAutoContrastChanged(bool b);

    void onRenderScaleButtonClicked(bool checked);
    
    void onRotoRoleChanged(int previousRole,int newRole);
    
    void onRotoNodeGuiSettingsPanelClosed(bool closed);

    void onGainSliderChanged(double v);
    
    void onColorSpaceComboBoxChanged(int v);
    
    void onCompositingOperatorIndexChanged(int index);
    
    void onFirstInputNameChanged(const QString& text);
    
    void onSecondInputNameChanged(const QString& text);
    
    void onActiveInputsChanged();
    
    void onInputNameChanged(int inputNb,const QString& name);
    
    void onInputChanged(int inputNb);
    
    void onImageFormatChanged(int,int,int);
    
    void onFrameRangeEditingFinished();
    
    void onLockFrameRangeButtonClicked(bool toggled);
    void setFrameRangeLocked(bool toggled);
    
    void onTimelineBoundariesChanged(SequenceTime,SequenceTime,int);

private:
    
    void manageSlotsForInfoWidget(int textureIndex,bool connect);
    
    bool eventFilter(QObject *target, QEvent *event);
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    virtual void enterEvent(QEvent*) OVERRIDE FINAL;

    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    
    virtual QSize sizeHint() const OVERRIDE FINAL;

    
    boost::scoped_ptr<ViewerTabPrivate> _imp;
};

#endif // NATRON_GUI_VIEWERTAB_H_
