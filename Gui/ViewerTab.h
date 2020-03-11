/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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


    explicit ViewerTab(const std::string& scriptName,
                       const std::list<NodeGuiPtr> & existingNodesContext,
                       const std::list<NodeGuiPtr>& activePluginsContext,
                       Gui* gui,
                       const NodeGuiPtr& node,
                       QWidget* parent = 0);

    virtual ~ViewerTab() OVERRIDE;


    ViewerNodePtr getInternalNode() const;

    ViewerGL* getViewer() const;

    void seek(SequenceTime time);

    void previousFrame();

    void nextFrame();

    void getTimelineBounds(int* first, int* last) const;

    void getTimeLineCachedFrames(std::list<TimeValue>* cachedFrames) const;

    virtual void notifyGuiClosing() OVERRIDE FINAL;
    virtual void onPanelMadeCurrent() OVERRIDE FINAL;
    virtual QIcon getIcon() const OVERRIDE FINAL;

    /**
     *@brief Tells all the nodes in the grpah to draw their overlays
     **/
    void drawOverlays(TimeValue time, const RenderScale & renderScale) const;

    bool notifyOverlaysPenDown(const RenderScale & renderScale, PenType pen, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp);

    bool notifyOverlaysPenDoubleClick(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos);

    bool notifyOverlaysPenMotion(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp);

    bool notifyOverlaysPenUp(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp);

    bool notifyOverlaysKeyDown(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysKeyUp(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysKeyRepeat(const RenderScale & renderScale, QKeyEvent* e);

    bool notifyOverlaysFocusGained(const RenderScale & renderScale);

    bool notifyOverlaysFocusLost(const RenderScale & renderScale);

    /**
     * @brief Received when the selection rectangle has changed on the viewer.
     * @param onRelease When true, this signal is emitted on the mouse release event
     * which means this is the last selection desired by the user.
     * Receivers can either update the selection always or just on mouse release.
     **/
    void updateSelectionFromViewerSelectionRectangle(bool onRelease);

    void onViewerSelectionCleared();
    
private:

    bool notifyOverlaysPenDown_internal(const NodePtr& node, const RenderScale & renderScale, PenType pen, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp);

    bool notifyOverlaysPenMotion_internal(const NodePtr& node, const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp);
    bool notifyOverlaysKeyDown_internal(const NodePtr& node, const RenderScale & renderScale, Key k,
                                        KeyboardModifiers km, Qt::Key qKey, const Qt::KeyboardModifiers& mods);

    bool checkNodeViewerContextShortcuts(const NodePtr& node, Qt::Key qKey, const Qt::KeyboardModifiers& mods);

    bool notifyOverlaysKeyRepeat_internal(const NodePtr& node, const RenderScale & renderScale, Key k,
                                          KeyboardModifiers km, Qt::Key qKey, const Qt::KeyboardModifiers& mods);

public:

    /**
     * @brief Called even if the viewer does not have mouse hover focus nor click focus so that from anywhere the user can still trigger the timeline prev/next and playback shortcuts
     **/
    bool checkForTimelinePlayerGlobalShortcut(Qt::Key qKey,
                                              const Qt::KeyboardModifiers& mods);

    void setZoomOrPannedSinceLastFit(bool enabled);

    bool getZoomOrPannedSinceLastFit() const;

    QVBoxLayout* getMainLayout() const;

    NodeGuiPtr getCurrentNodeViewerInterface(const PluginPtr& plugin) const;


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
    void removeNodeViewerInterface(const NodeGuiPtr& n,
                                   bool permanently,
                                   bool setAnotherFromSamePlugin);

    /**
     * @brief Same as removeNodeViewerInterface but for the Viewer node UI only
     **/
    void removeViewerInterface(const NodeGuiPtr& n,
                                bool permanently);

    virtual bool saveProjection(SERIALIZATION_NAMESPACE::ViewportData* data) OVERRIDE FINAL;

    virtual bool loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& data) OVERRIDE FINAL;


private:

    void removeNodeViewerInterfaceInternal(const NodeGuiPtr& n,
                                   bool permanently,
                                   bool setAnotherFromSamePlugin);

public:

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

    TimeLineGui* getTimeLineGui() const;
    
    ///Called by ViewerGL when the image changes to refresh the info bar
    void setImageFormat(int textureIndex, const ImagePlaneDesc& components, ImageBitDepthEnum depth);

    void redrawGLWidgets();

    void redrawTimeline();

    void centerOn(SequenceTime left, SequenceTime right);

    void setFrameRangeEdited(bool edited);

    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio);

    void centerOn_tripleSync(SequenceTime left, SequenceTime right);

    void connectToAInput(int inputNb);
    void connectToBInput(int inputNb);


    void synchronizeOtherViewersProjection();
    
    void onMousePressCalledInViewer();

    void setTimelineBounds(double first, double last);

    void setTimelineFormatFrames(bool value);


    /**
     * @brief Returns in nodes all the nodes that can draw an overlay in their order of appearance in the properties bin.
     **/
    void getNodesEntitledForOverlays(TimeValue time, ViewIdx view,NodesList& nodes) const;

    void setInfoBarAndViewerResolution(const RectI& rect, const RectD& canonicalRect, double par, int texIndex);

public Q_SLOTS:

    void onTimeLineTimeChanged(SequenceTime time, int);
    /*Updates the comboBox according to the real zoomFactor. Value is in % */
    void updateZoomComboBox(int value);

    /*makes the viewer black*/
    void disconnectViewer();

    void onTimelineBoundariesChanged(SequenceTime, SequenceTime);

    void setLeftToolbarVisible(bool visible);
    void setTopToolbarVisible(bool visible);
    void setPlayerVisible(bool visible);
    void setTimelineVisible(bool visible);
    void setTabHeaderVisible(bool visible);
    void setInfobarVisible(bool visible);
    void setInfobarVisible(int index, bool visible);

    void refreshViewerRenderingState();

    void onInternalNodeLabelChanged(const QString& oldLabel, const QString& newLabel);
    
    void onInternalNodeScriptNameChanged(const QString& name);

    void onRenderStatsAvailable(int time, double wallTime, const RenderStatsMap& stats);

    void setTripleSyncEnabled(bool toggled);

    void onInternalViewerCreated();
private:

    void setInfobarVisibleInternal(bool visible);

    void abortViewersAndRefresh();

    void manageTimelineSlot(bool disconnectPrevious, const TimeLinePtr& timeline);

    void manageSlotsForInfoWidget(int textureIndex, bool connect);

    virtual bool eventFilter(QObject *target, QEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    boost::scoped_ptr<ViewerTabPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_VIEWERTAB_H
