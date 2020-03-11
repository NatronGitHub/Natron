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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ViewerTab.h"
#include "ViewerTabPrivate.h"

#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>

#include <QApplication>
#include <QCheckBox>
#include <QToolBar>

#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodePtr
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobTypes.h"
#include "Engine/RenderEngine.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/Button.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/Gui.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerGL.h"


NATRON_NAMESPACE_ENTER

void
ViewerTab::manageSlotsForInfoWidget(int textureIndex,
                                    bool connect)
{
    RenderEnginePtr engine = _imp->viewerNode.lock()->getNode()->getRenderEngine();

    assert(engine);
    if (connect) {
        QObject::connect( engine.get(), SIGNAL(fpsChanged(double,double)), _imp->infoWidget[textureIndex], SLOT(setFps(double,double)) );
        QObject::connect( engine.get(), SIGNAL(renderFinished(int)), _imp->infoWidget[textureIndex], SLOT(hideFps()) );
    } else {
        QObject::disconnect( engine.get(), SIGNAL(fpsChanged(double,double)), _imp->infoWidget[textureIndex],
                             SLOT(setFps(double,double)) );
        QObject::disconnect( engine.get(), SIGNAL(renderFinished(int)), _imp->infoWidget[textureIndex], SLOT(hideFps()) );
    }
}

void
ViewerTab::setImageFormat(int textureIndex,
                          const ImagePlaneDesc& components,
                          ImageBitDepthEnum depth)
{
    _imp->infoWidget[textureIndex]->setImageFormat(components, depth);
}

void
ViewerTab::setTimelineBounds(double first, double last)
{
    _imp->timeLineGui->setBoundaries(first, last);
    _imp->timeLineGui->recenterOnBounds();
}

void
ViewerTab::setTimelineFormatFrames(bool value)
{
    _imp->timeLineGui->setTimeFormatFrames(value);
}

void
ViewerTab::onTimelineBoundariesChanged(SequenceTime first,
                                       SequenceTime second)
{
    ViewerNodePtr viewer = getInternalNode();
    if (!viewer) {
        return;
    }
    KnobIntPtr inPoint = viewer->getPlaybackInPointKnob();
    KnobIntPtr outPoint = viewer->getPlaybackOutPointKnob();
    inPoint->setValue(first, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
    outPoint->setValue(second, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);

    abortViewersAndRefresh();
}



void
ViewerTab::setLeftToolbarVisible(bool visible)
{

    for (std::list<ViewerTabPrivate::PluginViewerContext>::iterator it = _imp->currentNodeContext.begin(); it != _imp->currentNodeContext.end(); ++it) {
        QToolBar* bar = it->currentContext->getToolBar();
        if (bar) {
            bar->setVisible(visible);
        }
    }
}

void
ViewerTab::setTopToolbarVisible(bool visible)
{

    for (std::list<ViewerTabPrivate::PluginViewerContext>::iterator it = _imp->currentNodeContext.begin(); it != _imp->currentNodeContext.end(); ++it) {
        it->currentContext->getContainerWidget()->setVisible(visible);
    }
}

void
ViewerTab::setPlayerVisible(bool visible)
{
    for (std::list<ViewerTabPrivate::PluginViewerContext>::iterator it = _imp->currentNodeContext.begin(); it != _imp->currentNodeContext.end(); ++it) {
        NodeGuiPtr curNode = it->currentNode.lock();
        if (!curNode) {
            continue;
        }
        ViewerNodePtr isViewer = curNode->getNode()->isEffectViewerNode();
        if (!isViewer) {
            continue;
        }
        it->currentContext->getPlayerToolbar()->setVisible(visible);
    }
}

void
ViewerTab::setTimelineVisible(bool visible)
{
    _imp->timeLineGui->setVisible(visible);
}

void
ViewerTab::setTabHeaderVisible(bool visible)
{
    TabWidget* w = getParentPane();
    if (w) {
        w->setTabHeaderVisible(visible);
    }
}

void
ViewerTab::setInfobarVisible(bool visible)
{
    getGui()->clearColorPickers();
    setInfobarVisibleInternal(visible);
}


void
ViewerTab::setInfobarVisible(int index, bool visible)
{
    manageSlotsForInfoWidget(index, visible);
    _imp->infoWidget[index]->setVisible(visible);
}

void
ViewerTab::setInfobarVisibleInternal(bool visible)
{
    ViewerNodePtr internalNode = getInternalNode();
    for (int i = 0; i < 2; ++i) {
        if (i == 1) {

            NodePtr bInput = internalNode->getCurrentBInput();
            if (!bInput) {
                continue;
            }
            if (internalNode->getCurrentOperator() == eViewerCompositingOperatorNone) {
                continue;
            }
        }

        _imp->infoWidget[i]->setVisible(visible);
    }
}

void
ViewerTab::setAsFileDialogViewer()
{
    _imp->isFileDialogViewer = true;
}

bool
ViewerTab::isFileDialogViewer() const
{
    return _imp->isFileDialogViewer;
}

void
ViewerTab::setCustomTimeline(const TimeLinePtr& timeline)
{
    _imp->timeLineGui->setTimeline(timeline);
    manageTimelineSlot(true, timeline);
}

void
ViewerTab::manageTimelineSlot(bool disconnectPrevious,
                              const TimeLinePtr& timeline)
{
    if (disconnectPrevious) {
        TimeLinePtr previous = _imp->timeLineGui->getTimeline();
        QObject::disconnect( previous.get(), SIGNAL(frameChanged(SequenceTime,int)),
                             this, SLOT(onTimeLineTimeChanged(SequenceTime,int)) );
    }

    QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)),
                      this, SLOT(onTimeLineTimeChanged(SequenceTime,int)) );
}

TimeLinePtr
ViewerTab::getTimeLine() const
{
    return _imp->timeLineGui->getTimeline();
}

TimeLineGui*
ViewerTab::getTimeLineGui() const
{
    return _imp->timeLineGui;
}

void
ViewerTab::onMousePressCalledInViewer()
{
    takeClickFocus();
    if ( getGui() ) {
        getGui()->setActiveViewer(this);
    }
}

void
ViewerTab::setProjection(double zoomLeft,
                         double zoomBottom,
                         double zoomFactor,
                         double zoomAspectRatio)
{
    _imp->viewer->setProjection(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
    updateZoomComboBox(std::floor(zoomFactor * 100 + 0.5));
}

void
ViewerTab::refreshViewerRenderingState()
{
    ViewerNodePtr internalNode = getInternalNode();
    if (!internalNode) {
        return;
    }
    RenderEnginePtr engine = internalNode->getNode()->getRenderEngine();
    bool hasActiveRender = engine ? engine->hasActiveRender() : 0;
    internalNode->setRefreshButtonDown(hasActiveRender);
}

void
ViewerTab::redrawGLWidgets()
{
    _imp->viewer->update();
    _imp->timeLineGui->update();
}

void
ViewerTab::redrawTimeline()
{
    _imp->timeLineGui->update();
}


void
ViewerTab::centerOn(SequenceTime left,
                    SequenceTime right)
{
    _imp->timeLineGui->centerOn(left, right);
}

void
ViewerTab::centerOn_tripleSync(SequenceTime left,
                               SequenceTime right)
{
    _imp->timeLineGui->centerOn_tripleSync(left, right);
}

void
ViewerTab::setFrameRangeEdited(bool edited)
{
    _imp->timeLineGui->setFrameRangeEdited(edited);
}

void
ViewerTab::onInternalNodeLabelChanged(const QString& /*oldLabel*/, const QString& newLabel)
{
    TabWidget* parent = dynamic_cast<TabWidget*>( parentWidget() );

    if (parent) {
        setLabel( newLabel.toStdString() );
        parent->setTabLabel(this, newLabel);
    }
}

void
ViewerTab::onInternalNodeScriptNameChanged(const QString& /*name*/)
{
    ViewerNodePtr viewerNode = _imp->viewerNode.lock();
    // always running in the main thread
    std::string newName = viewerNode->getNode()->getFullyQualifiedName();
    std::string oldName = getScriptName();

    for (std::size_t i = 0; i < newName.size(); ++i) {
        if (newName[i] == '.') {
            newName[i] = '_';
        }
    }


    assert( qApp && qApp->thread() == QThread::currentThread() );
    setScriptName(newName);
 
}

void
ViewerTab::onRenderStatsAvailable(int time,
                                  double wallTime,
                                  const RenderStatsMap& stats)
{
    assert( QThread::currentThread() == qApp->thread() );
    RenderStatsDialog* dialog = getGui()->getRenderStatsDialog();
    if (dialog) {
        dialog->addStats(time, wallTime, stats);
    }
}

void
ViewerTab::synchronizeOtherViewersProjection()
{
    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return;

    }
    app->setMasterSyncViewer(getInternalNode()->getNode());
    double left, bottom, factor, par;
    _imp->viewer->getProjection(&left, &bottom, &factor, &par);
    const std::list<ViewerTab*>& viewers = getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        if ( (*it) != this ) {
            (*it)->getViewer()->setProjection(left, bottom, factor, par);
            (*it)->getInternalNode()->getNode()->getRenderEngine()->renderCurrentFrame();
        }
    }
}

void
ViewerTab::setTripleSyncEnabled(bool toggled)
{
    getGui()->setTripleSyncEnabled(toggled);
    if (toggled) {
        AnimationModuleEditor* deditor = getGui()->getAnimationModuleEditor();

        SequenceTime left, right;
        _imp->timeLineGui->getVisibleRange(&left, &right);
        getGui()->centerOpenedViewersOn(left, right);
        deditor->centerOn(left, right);
    }
}

void
ViewerTab::onPanelMadeCurrent()
{
    if (!getGui()) {
        return;
    }
    GuiAppInstancePtr app = getGui()->getApp();
    if (!app) {
        return;
    }
    ViewerNodePtr viewerNode = _imp->viewerNode.lock();
    // Refresh the image since so far the viewer was probably not in sync with internal data
    if ( viewerNode && !app->getProject()->isLoadingProject() && !app->isTopLevelNodeBeingCreated(viewerNode->getNode())) {
        viewerNode->getNode()->getRenderEngine()->renderCurrentFrame();
    }
}

NATRON_NAMESPACE_EXIT
