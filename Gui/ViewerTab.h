/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/GlobalDefines.h"
#include "Global/KeySymbols.h" // Key

#include "Engine/RenderStats.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

#include <QtCore/QSize>

NATRON_NAMESPACE_ENTER

typedef std::map<NodePtr, NodeRenderStats > RenderStatsMap;

struct ViewerTabPrivate;
class ViewerTab
    : public QWidget, public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    explicit ViewerTab(const std::list<NodeGuiPtr> & existingNodesContext,
                       const std::list<NodeGuiPtr>& activePluginsContext,
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
    virtual void onPanelMadeCurrent() OVERRIDE FINAL;

    /**
     *@brief Tells all the nodes in the grpah to draw their overlays
     **/
    /*All the overlay methods are forwarding calls to the default node instance*/
    void drawOverlays(double time, const RenderScale & renderScale) const;

    bool notifyOverlaysPenDown(const RenderScale & renderScale, PenType pen, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp);

    bool notifyOverlaysPenDoubleClick(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos);

    bool notifyOverlaysPenMotion(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp);

    bool notifyOverlaysPenUp(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp);

    bool notifyOverlaysKeyDown(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysKeyUp(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysKeyRepeat(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysFocusGained(const RenderScale & renderScale);

    bool notifyOverlaysFocusLost(const RenderScale & renderScale);

private:

    bool notifyOverlaysPenDown_internal(const NodePtr& node, const RenderScale & renderScale, PenType pen, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp);

    bool notifyOverlaysPenMotion_internal(const NodePtr& node, const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp);
    bool notifyOverlaysKeyDown_internal(const NodePtr& node, const RenderScale & renderScale, Key k,
                                        KeyboardModifiers km, Qt::Key qKey, const Qt::KeyboardModifiers& mods);

    bool checkNodeViewerContextShortcuts(const NodePtr& node, Qt::Key qKey, const Qt::KeyboardModifiers& mods);

    bool notifyOverlaysKeyRepeat_internal(const NodePtr& node, const RenderScale & renderScale, Key k,
                                          KeyboardModifiers km, Qt::Key qKey, const Qt::KeyboardModifiers& mods);

public:


    ////////
    /////////////The following functions are used when serializing/deserializing the project gui
    ///////////// so the viewer can restore the exact same settings to the user.
    bool isClippedToProject() const;

    std::string getColorSpace() const;

    void setUserRoIEnabled(bool b);

    void setUserRoI(const RectD & r);

    void setClipToProject(bool b);

    void setFullFrameProcessing(bool fullFrame);

    bool isFullFrameProcessingEnabled() const;

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

    void setMipmapLevel(unsigned int level);

    unsigned int getMipmapLevel() const;

    void setRenderScaleActivated(bool act);

    bool getRenderScaleActivated() const;

    void setZoomOrPannedSinceLastFit(bool enabled);

    bool getZoomOrPannedSinceLastFit() const;

    /**
     * @brief Creates a new viewer interface context for this node. This is not shared among viewers.
     **/
    void createNodeViewerInterface(const NodeGuiPtr& n);

    /**
     * @brief Set the current viewer interface for a given plug-in to be the one of the given node
     **/
    void setPluginViewerInterface(const NodeGuiPtr& n);

    /**
     * @brief Removes the interface associated to the given node.
     * @param permanently The interface is destroyed instead of being hidden
     * @param setAnotherFromSamePlugin If true, if another node of the same plug-in is a candidate for a viewer interface, it will replace the existing
     * viewer interface for this plug-in
     **/
    void removeNodeViewerInterface(const NodeGuiPtr& n, bool permanently, bool setAnotherFromSamePlugin);

    /**
     * @brief Get the list of all nodes that have a user interface created on this viewer (but not necessarily displayed)
     * and a list for each plug-in of the active node.
     **/
    void getNodesViewerInterface(std::list<NodeGuiPtr>* nodesWithUI,
                                 std::list<NodeGuiPtr>* perPluginActiveUI) const;

    /**
     * @brief Called to refresh the current selected tool on the toolbar
     **/
    void updateSelectedToolForNode(const QString& toolID, const NodeGuiPtr& node);

    ViewerCompositingOperatorEnum getCompositingOperator() const;

    ViewerCompositingOperatorEnum getCompositingOperatorPrevious() const;

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

    void setCustomTimeline(const TimeLinePtr& timeline);
    TimeLinePtr getTimeLine() const;

    bool isCheckerboardEnabled() const;
    void setCheckerboardEnabled(bool enabled);

    double getDesiredFps() const;
    void setDesiredFps(double fps);

    ///Called by ViewerGL when the image changes to refresh the info bar
    void setImageFormat(int textureIndex, const ImagePlaneDesc& components, ImageBitDepthEnum depth);

    void redrawGLWidgets();

    void getTimelineBounds(int* left, int* right) const;

    void setTimelineBounds(int left, int right);

    void centerOn(SequenceTime left, SequenceTime right);

    ///Calls setTimelineBounds + set the frame range line edit
    void setFrameRange(int left, int right);

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

    void connectToInput(int inputNb, bool isASide);
    void connectToAInput(int inputNb);
    void connectToBInput(int inputNb);

    bool isPickerEnabled() const;
    void setPickerEnabled(bool enabled);

    void onMousePressCalledInViewer();

    void updateViewsMenu(const std::vector<std::string>& viewNames);

    void getActiveInputs(int* a, int* b) const;

    void setViewerPaused(bool paused, bool allInputs);

    void toggleViewerPauseMode(bool allInputs);

    bool isViewerPaused(int texIndex) const;

    QString getCurrentLayerName() const;

    QString getCurrentAlphaLayerName() const;

    void setCurrentLayers(const QString& layer, const QString& alphaLayer);

    void setInfoBarAndViewerResolution(const RectI& rect, const RectD& canonicalRect, double par, int texIndex);

public Q_SLOTS:

    void onPauseViewerButtonClicked(bool clicked);

    void onPlaybackInButtonClicked();
    void onPlaybackOutButtonClicked();
    void onPlaybackInSpinboxValueChanged(double value);
    void onPlaybackOutSpinboxValueChanged(double value);

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
    void onFullFrameButtonToggle(bool);
    void onClipToProjectButtonToggle(bool);
    void onTimeLineTimeChanged(SequenceTime time, int);
    void onCurrentTimeSpinBoxChanged(double);
    /*Updates the comboBox according to the real zoomFactor. Value is in % */
    void updateZoomComboBox(int value);

    void onRenderScaleComboIndexChanged(int index);

    /*makes the viewer black*/
    void disconnectViewer();

    void refresh();

    void onViewsComboboxChanged(int index);

    void onEnableViewerRoIButtonToggle(bool);

    void onCreateNewRoIPressed();

    void onAutoContrastChanged(bool b);

    void onRenderScaleButtonClicked(bool checked);

    void onColorSpaceComboBoxChanged(int v);

    void onCompositingOperatorIndexChanged(int index);

    void onFirstInputNameChanged(const QString & text);

    void onSecondInputNameChanged(const QString & text);

    void switchInputAAndB();

    void setInputA(int index);

    void setInputB(int index);

    void onActiveInputsChanged();

    void onInputNameChanged(int inputNb, const QString & name);

    void onInputChanged(int inputNb);

    void onCanSetFPSClicked(bool toggled);
    void onCanSetFPSLabelClicked(bool toggled);
    void setFPSLocked(bool fpsLocked);

    void onTimelineBoundariesChanged(SequenceTime, SequenceTime);

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

    void onSetDownPlaybackButtonsTimeout();

    void refreshViewerRenderingState();

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

    void previousView();
    void nextView();

    void toggleTripleSync(bool toggled);

private:

    void abortViewersAndRefresh();

    void refreshFPSBoxFromClipPreferences();

    void onSpinboxFpsChangedInternal(double fps);

    void onPickerButtonClickedInternal(ViewerTab* caller, bool);

    void onCompositingOperatorChangedInternal(ViewerCompositingOperatorEnum oldOp, ViewerCompositingOperatorEnum newOp);


    void manageTimelineSlot(bool disconnectPrevious, const TimeLinePtr& timeline);

    void manageSlotsForInfoWidget(int textureIndex, bool connect);

    virtual bool eventFilter(QObject *target, QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    std::unique_ptr<ViewerTabPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_VIEWERTAB_H
