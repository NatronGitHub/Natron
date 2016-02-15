/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_GUI_VIEWERTAB_H
#define NATRON_GUI_VIEWERTAB_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Global/KeySymbols.h" // Key

#include "Engine/RenderStats.h"
#include "Engine/EngineFwd.h"

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

typedef std::map<NodePtr,NodeRenderStats > RenderStatsMap;

struct ViewerTabPrivate;
class ViewerTab : public QWidget, public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    explicit ViewerTab(const std::list<NodeGui*> & existingRotoNodes,
                       NodeGui* currentRoto,
                       const std::list<NodeGui*> & existingTrackerNodes,
                       NodeGui* currentTracker,
                       Gui* gui,
                       ViewerInstance* node,
                       QWidget* parent = 0);

    virtual ~ViewerTab() OVERRIDE;


    ViewerInstance* getInternalNode() const;
    void discardInternalNodePointer();
    
    ViewerGL* getViewer() const;

    void setCurrentView(ViewIdx view);

    ViewIdx getCurrentView() const;

    void seek(SequenceTime time);

    virtual void notifyGuiClosing() OVERRIDE FINAL;

    /**
     *@brief Tells all the nodes in the grpah to draw their overlays
     **/
    /*All the overlay methods are forwarding calls to the default node instance*/
    void drawOverlays(double time, const RenderScale & renderScale) const;

    bool notifyOverlaysPenDown(const RenderScale & renderScale, PenType pen, bool isTabletEvent,const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, QMouseEvent* e);

    bool notifyOverlaysPenDoubleClick(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, QMouseEvent* e);

    bool notifyOverlaysPenMotion(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, QInputEvent* e);

    bool notifyOverlaysPenUp(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, QMouseEvent* e);

    bool notifyOverlaysKeyDown(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysKeyUp(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysKeyRepeat(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysFocusGained(const RenderScale & renderScale);

    bool notifyOverlaysFocusLost(const RenderScale & renderScale);
    
private:
    
    bool notifyOverlaysPenDown_internal(const NodePtr& node, const RenderScale & renderScale, PenType pen, bool isTabletEvent, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, QMouseEvent* e);
    
    bool notifyOverlaysPenMotion_internal(const NodePtr& node, const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, QInputEvent* e);
    bool notifyOverlaysKeyDown_internal(const NodePtr& node, const RenderScale & renderScale, QKeyEvent* e, Key k,
                                        KeyboardModifiers km);
    bool notifyOverlaysKeyRepeat_internal(const NodePtr& node, const RenderScale & renderScale, QKeyEvent* e, Key k,
                                          KeyboardModifiers km);
public:
    


    ////////
    /////////////The following functions are used when serializing/deserializing the project gui
    ///////////// so the viewer can restore the exact same settings to the user.
    bool isClippedToProject() const;

    std::string getColorSpace() const;

    void setUserRoIEnabled(bool b);

    void setUserRoI(const RectD & r);

    void setClipToProject(bool b);

    void setColorSpace(const std::string & colorSpaceName);

    void setGain(double d);

    double getGain() const;
    
    void setGamma(double gamma);
    
    double getGamma() const;
    
    static std::string getChannelsString(DisplayChannelsEnum c);

    std::string getChannelsString() const;
    
    DisplayChannelsEnum getChannels() const;

    void setChannels(const std::string & channelsStr);
    
private:
    
    void setDisplayChannels(int index, bool setBothInputs);
    
public:

    bool isAutoContrastEnabled() const;

    void setAutoContrastEnabled(bool b);

    void setMipMapLevel(int level);

    int getMipMapLevel() const;

    void setRenderScaleActivated(bool act);

    bool getRenderScaleActivated() const;

    void setZoomOrPannedSinceLastFit(bool enabled);

    bool getZoomOrPannedSinceLastFit() const;

    void setInfoBarResolution(const Format & f);

    void createRotoInterface(NodeGui* n);

    /**
     * @brief Set the current roto interface
     **/
    void setRotoInterface(NodeGui* n);

    void removeRotoInterface(NodeGui* n, bool permanently, bool removeAndDontSetAnother);

    void getRotoContext(std::map<NodeGui*,RotoGui*>* rotoNodes, std::pair<NodeGui*,RotoGui*>* currentRoto) const;

    void updateRotoSelectedTool(int tool,RotoGui* sender);

    boost::shared_ptr<RotoGuiSharedData> getRotoGuiSharedData(NodeGui* node) const;

    void onRotoEvaluatedForThisViewer();


    void createTrackerInterface(NodeGui* n);

    void setTrackerInterface(NodeGui* n);

    void removeTrackerInterface(NodeGui* n, bool permanently, bool removeAndDontSetAnother);

    void getTrackerContext(std::map<NodeGui*,TrackerGui*>* trackerNodes, std::pair<NodeGui*,TrackerGui*>* currentTracker) const;


    ViewerCompositingOperatorEnum getCompositingOperator() const;

    void setCompositingOperator(ViewerCompositingOperatorEnum op);
    
    bool isFPSLocked() const;

    void connectToViewerCache();

    void disconnectFromViewerCache();

    void clearTimelineCacheLine();

    bool isInfobarVisible() const;
    bool isTopToolbarVisible() const;
    bool isPlayerVisible() const;
    bool isTimelineVisible() const;
    bool isLeftToolbarVisible() const;
    bool isRightToolbarVisible() const;
    
    ///Not MT-safe
    void setAsFileDialogViewer();
    bool isFileDialogViewer() const;
    
    void setCustomTimeline(const boost::shared_ptr<TimeLine>& timeline);
    boost::shared_ptr<TimeLine> getTimeLine() const;
    
    bool isCheckerboardEnabled() const;
    void setCheckerboardEnabled(bool enabled);
    
    double getDesiredFps() const;
    void setDesiredFps(double fps);

    ///Called by ViewerGL when the image changes to refresh the info bar
    void setImageFormat(int textureIndex,const ImageComponents& components,ImageBitDepthEnum depth);
    
	void redrawGLWidgets();

    void getTimelineBounds(int* left,int* right) const;
    
    void setTimelineBounds(int left,int right);

    void centerOn(SequenceTime left, SequenceTime right);
    
    ///Calls setTimelineBounds + set the frame range line edit
    void setFrameRange(int left,int right);
    
    void setFrameRangeEdited(bool edited);
    
    void setPlaybackMode(PlaybackModeEnum mode);
    
    PlaybackModeEnum getPlaybackMode() const;
    

    void refreshLayerAndAlphaChannelComboBox();

    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio);

    bool isViewersSynchroEnabled() const;
    
    void synchronizeOtherViewersProjection();
    
    void centerOn_tripleSync(SequenceTime left, SequenceTime right);
    
    void zoomIn();
    void zoomOut();
    
    void refresh(bool enableRenderStats);
    
    void connectToInput(int inputNb);
    
    bool isPickerEnabled() const;
    void setPickerEnabled(bool enabled);
    
    void onMousePressCalledInViewer();
    
    void updateViewsMenu(const std::vector<std::string>& viewNames);
    
    void getActiveInputs(int* a, int* b) const;

public Q_SLOTS:

    void onZoomComboboxCurrentIndexChanged(int index);
    
    void toggleStartForward();
    void toggleStartBackward();
    
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
    void togglePlaybackMode();
    void onViewerChannelsChanged(int);
    void onClipToProjectButtonToggle(bool);
    void onTimeLineTimeChanged(SequenceTime time,int);
    void onCurrentTimeSpinBoxChanged(double);
    /*Updates the comboBox according to the real zoomFactor. Value is in % */
    void updateZoomComboBox(int value);

    void onRenderScaleComboIndexChanged(int index);

    /*makes the viewer black*/
    void disconnectViewer();

    void refresh();

    void showView(int /* Qt slot, no ViewIdx */ view);

    void onEnableViewerRoIButtonToggle(bool);
    
    void onCreateNewRoIPressed();

    void onAutoContrastChanged(bool b);

    void onRenderScaleButtonClicked(bool checked);

    void onRotoRoleChanged(int previousRole,int newRole);

    void onRotoNodeGuiSettingsPanelClosed(bool closed);

    void onTrackerNodeGuiSettingsPanelClosed(bool closed);

    void onColorSpaceComboBoxChanged(int v);

    void onCompositingOperatorIndexChanged(int index);

    void onFirstInputNameChanged(const QString & text);

    void onSecondInputNameChanged(const QString & text);
    
    void switchInputAAndB();
    
    void setInputA(int index);
    
    void setInputB(int index);

    void onActiveInputsChanged();

    void onInputNameChanged(int inputNb,const QString & name);

    void onInputChanged(int inputNb);

    void onFrameRangeEditingFinished();
    
    void onCanSetFPSClicked(bool toggled);
    void onCanSetFPSLabelClicked(bool toggled);
    void setFPSLocked(bool fpsLocked);

    void onTimelineBoundariesChanged(SequenceTime,SequenceTime);
    
    void setLeftToolbarVisible(bool visible);
    void setRightToolbarVisible(bool visible);
    void setTopToolbarVisible(bool visible);
    void setPlayerVisible(bool visible);
    void setTimelineVisible(bool visible);
    void setInfobarVisible(bool visible);

    
    void toggleInfobarVisbility();
    void togglePlayerVisibility();
    void toggleTimelineVisibility();
    void toggleLeftToolbarVisiblity();
    void toggleRightToolbarVisibility();
    void toggleTopToolbarVisibility();

    void showAllToolbars();    
    void hideAllToolbars();
        
    void onCheckerboardButtonClicked();
    
    void onPickerButtonClicked(bool);
    
    void onSpinboxFpsChanged(double fps);
    
    void onEngineStopped();
    void onEngineStarted(bool forward);
    
    void onViewerRenderingStarted();
    
    void onViewerRenderingStopped();
    
    void setTurboButtonDown(bool down);
    
    void onClipPreferencesChanged();
    
    void onAvailableComponentsChanged();
    
    void onInternalNodeLabelChanged(const QString& name);
    void onInternalNodeScriptNameChanged(const QString& name);
    
    void onAlphaChannelComboChanged(int index);
    void onLayerComboChanged(int index);
    
    void onGammaToggled(bool clicked);
    
    void onGammaSliderValueChanged(double value);
    
    void onGammaSpinBoxValueChanged(double value);
    
    void onGainToggled(bool clicked);
    
    void onGainSliderChanged(double v);

    void onGainSpinBoxValueChanged(double value);
    
    void onGammaSliderEditingFinished(bool hasMovedOnce);
    void onGainSliderEditingFinished(bool hasMovedOnce);
    
    void onSyncViewersButtonPressed(bool clicked);
    
    void onRenderStatsAvailable(int time, ViewIdx view, double wallTime, const RenderStatsMap& stats);
    
    void nextLayer();
    void previousLayer();
    
    void toggleTripleSync(bool toggled);
    
private:
    
    void refreshFPSBoxFromClipPreferences();
    
    void onSpinboxFpsChangedInternal(double fps);
    
    void onPickerButtonClickedInternal(ViewerTab* caller,bool);
    
    void onCompositingOperatorChangedInternal(ViewerCompositingOperatorEnum oldOp,ViewerCompositingOperatorEnum newOp);

    
    void manageTimelineSlot(bool disconnectPrevious,const boost::shared_ptr<TimeLine>& timeline);

    void manageSlotsForInfoWidget(int textureIndex,bool connect);

    virtual bool eventFilter(QObject *target, QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    boost::scoped_ptr<ViewerTabPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_GUI_VIEWERTAB_H
