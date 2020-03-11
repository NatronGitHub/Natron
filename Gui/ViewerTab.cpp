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
#include <sstream> // stringstream

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QAction>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPalette>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Project.h"
#include "Engine/KnobTypes.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/Label.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerGL.h"

#include "Serialization/WorkspaceSerialization.h"

NATRON_NAMESPACE_ENTER


ViewerTab::ViewerTab(const std::string& scriptName,
                     const std::list<NodeGuiPtr> & existingNodesContext,
                     const std::list<NodeGuiPtr>& activePluginsContext,
                     Gui* gui,
                     const NodeGuiPtr& node_ui,
                     QWidget* parent)
    : QWidget(parent)
    , PanelWidget(scriptName, this, gui)
    , _imp( new ViewerTabPrivate(this, node_ui) )
{
    ViewerNodePtr node = node_ui->getNode()->isEffectViewerNode();
    installEventFilter(this);
    setMouseTracking(true);
    NodePtr internalNode = node->getNode();
    QObject::connect( internalNode.get(), SIGNAL(scriptNameChanged(QString)), this, SLOT(onInternalNodeScriptNameChanged(QString)) );
    QObject::connect( internalNode.get(), SIGNAL(labelChanged(QString,QString)), this, SLOT(onInternalNodeLabelChanged(QString,QString)) );
    QObject::connect( node.get(), SIGNAL(internalViewerCreated()), this, SLOT(onInternalViewerCreated()));

    _imp->mainLayout = new QVBoxLayout(this);
    setLayout(_imp->mainLayout);
    _imp->mainLayout->setSpacing(0);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);

    QFontMetrics fm(font(), 0);


    _imp->viewerContainer = new QWidget(this);
    _imp->viewerLayout = new QHBoxLayout(_imp->viewerContainer);
    _imp->viewerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->viewerLayout->setSpacing(0);

    _imp->viewerSubContainer = new QWidget(_imp->viewerContainer);
    _imp->viewerSubContainerLayout = new QVBoxLayout(_imp->viewerSubContainer);
    _imp->viewerSubContainerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->viewerSubContainerLayout->setSpacing(1);


    // Info bars
    QString inputNames[2] = {
        QString::fromUtf8("A:"), QString::fromUtf8("B:")
    };

    bool infobarvisible = node->isInfoBarVisible();
    for (int i = 0; i < 2; ++i) {
        _imp->infoWidget[i] = new InfoViewerWidget(inputNames[i], this);

    }

    // Viewer
    _imp->viewer = new ViewerGL(this);

    GuiAppInstancePtr app = gui->getApp();

    // Init viewer to project format
    {
        Format projectFormat;
        app->getProject()->getProjectDefaultFormat(&projectFormat);

        RectD canonicalFormat = projectFormat.toCanonicalFormat();
        for (int i = 0; i < 2; ++i) {
            _imp->viewer->setInfoViewer(_imp->infoWidget[i], i);
            _imp->viewer->setRegionOfDefinition(canonicalFormat, projectFormat.getPixelAspectRatio(), i);
            setInfoBarAndViewerResolution(projectFormat, canonicalFormat, projectFormat.getPixelAspectRatio(), i);
        }
        _imp->viewer->resetWipeControls();
    }

    _imp->viewerSubContainerLayout->addWidget(_imp->viewer);
    for (int i = 0; i < 2; ++i) {
        _imp->viewerSubContainerLayout->addWidget(_imp->infoWidget[i]);
        _imp->viewer->setInfoViewer(_imp->infoWidget[i], i);
        if (i == 1 || !infobarvisible) {
            _imp->infoWidget[i]->hide();
        }
    }

    manageSlotsForInfoWidget(0, true);


    _imp->viewerLayout->addWidget(_imp->viewerSubContainer);
    _imp->mainLayout->addWidget(_imp->viewerContainer);

    TimeLinePtr timeline = app->getTimeLine();
    _imp->timeLineGui = new TimeLineGui(node, timeline, getGui(), this);
    QObject::connect( _imp->timeLineGui, SIGNAL(boundariesChanged(SequenceTime,SequenceTime)),
                      this, SLOT(onTimelineBoundariesChanged(SequenceTime,SequenceTime)) );
    QObject::connect( app->getProject().get(), SIGNAL(frameRangeChanged(int,int)), _imp->timeLineGui, SLOT(onProjectFrameRangeChanged(int,int)) );
    _imp->timeLineGui->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    if (!node->isTimelineVisible()) {
        _imp->timeLineGui->hide();
    }

    //Add some spacing because the timeline might be black as the info
    _imp->mainLayout->addSpacing( TO_DPIY(5) );
    _imp->mainLayout->addWidget(_imp->timeLineGui);

    double leftBound, rightBound;
    leftBound = node->getPlaybackInPointKnob()->getValue();
    rightBound = node->getPlaybackOutPointKnob()->getValue();

    TimeValue projectLeft, projectRight;
    app->getProject()->getFrameRange(&projectLeft, &projectRight);

    _imp->timeLineGui->setBoundaries(leftBound, rightBound);
    onTimelineBoundariesChanged(leftBound, rightBound);
    _imp->timeLineGui->setFrameRangeEdited(projectLeft != leftBound || projectRight != rightBound);;


    manageTimelineSlot(false, timeline);

    QObject::connect( node.get(), SIGNAL(renderStatsAvailable(int,double,RenderStatsMap)),
                      this, SLOT(onRenderStatsAvailable(int,double,RenderStatsMap)) );
    QObject::connect( _imp->viewer, SIGNAL(zoomChanged(int)), this, SLOT(updateZoomComboBox(int)) );
    QObject::connect( node.get(), SIGNAL(viewerDisconnected()), this, SLOT(disconnectViewer()) );


    createNodeViewerInterface(node_ui);
    setPluginViewerInterface(node_ui);

    for (std::list<NodeGuiPtr>::const_iterator it = existingNodesContext.begin(); it != existingNodesContext.end(); ++it) {
        ViewerNodePtr isViewerNode = (*it)->getNode()->isEffectViewerNode();
        // For viewers, create the viewer interface separately
        if (!isViewerNode) {
            createNodeViewerInterface(*it);
        }
    }
    for (std::list<NodeGuiPtr>::const_iterator it = activePluginsContext.begin(); it != activePluginsContext.end(); ++it) {
        ViewerNodePtr isViewerNode = (*it)->getNode()->isEffectViewerNode();
        // For viewers, create the viewer interface separately
        if (!isViewerNode) {
            setPluginViewerInterface(*it);
        }
    }


    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    _imp->viewerNode.lock()->setUiContext( getViewer() );

    QTimer::singleShot( 25, _imp->timeLineGui, SLOT(recenterOnBounds()) );


    //Refresh the viewport lock state
    const std::list<ViewerTab*>& viewers = getGui()->getViewersList();
    if ( !viewers.empty() ) {
        ViewerTab* other = viewers.front();
        if ( other->getInternalNode()->isViewersSynchroEnabled() ) {
            double left, bottom, factor, par;
            other->getViewer()->getProjection(&left, &bottom, &factor, &par);
            _imp->viewer->setProjection(left, bottom, factor, par);
            node->setViewersSynchroEnabled(true);
        }
    }

    _imp->cachedFramesThread.reset(new CachedFramesThread(this));
    _imp->cachedFramesThread->start();
}


ViewerTab::~ViewerTab()
{

    _imp->cachedFramesThread->quitThread();

    Gui* gui = getGui();
    if (gui) {
        NodeGraph* graph = 0;
        ViewerNodePtr internalNode = getInternalNode();
        if (internalNode) {
            NodeCollectionPtr collection = internalNode->getNode()->getGroup();
            if (collection) {
                NodeGroupPtr isGrp = toNodeGroup(collection);
                if (isGrp) {
                    NodeGraphI* graph_i = isGrp->getNodeGraph();
                    if (graph_i) {
                        graph = dynamic_cast<NodeGraph*>(graph_i);
                        assert(graph);
                    }
                } else {
                    graph = gui->getNodeGraph();
                }
            }
            internalNode->invalidateUiContext();
        } else {
            graph = gui->getNodeGraph();
        }
        assert(graph);
        GuiAppInstancePtr app = gui->getApp();
        if ( app && !app->isClosing() && graph && (graph->getLastSelectedViewer() == this) ) {
            graph->setLastSelectedViewer(0);
        }
    }
    _imp->nodesContext.clear();
}

void
ViewerTab::getTimeLineCachedFrames(std::list<TimeValue>* cachedFrames) const
{
    _imp->cachedFramesThread->getCachedFrames(cachedFrames);
}


QVBoxLayout*
ViewerTab::getMainLayout() const
{
    return _imp->mainLayout;
}

void
ViewerTab::onInternalViewerCreated()
{
    manageSlotsForInfoWidget(0, true);

}

bool
ViewerTab::saveProjection(SERIALIZATION_NAMESPACE::ViewportData* data)
{
    _imp->viewer->getProjection(&data->left, &data->bottom, &data->zoomFactor, &data->par);
    data->zoomOrPanSinceLastFit = _imp->viewer->getZoomOrPannedSinceLastFit();
    return true;
}

bool
ViewerTab::loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& data)
{
    _imp->viewer->setProjection(data.left, data.bottom, data.zoomFactor, 1.);
    _imp->viewer->setZoomOrPannedSinceLastFit(data.zoomOrPanSinceLastFit);
    return true;
}

void
ViewerTab::mouseMoveEvent(QMouseEvent* e)
{
    // Calling e->ignore() is the same as calling the base implementation of QWidget::mouseMoveEvent(e)
    QWidget::mouseMoveEvent(e);
}

void
ViewerTab::setZoomOrPannedSinceLastFit(bool enabled)
{
    _imp->viewer->setZoomOrPannedSinceLastFit(enabled);
}

bool
ViewerTab::getZoomOrPannedSinceLastFit() const
{
    return _imp->viewer->getZoomOrPannedSinceLastFit();
}


ViewerGL*
ViewerTab::getViewer() const
{
    return _imp->viewer;
}

ViewerNodePtr
ViewerTab::getInternalNode() const
{
    return _imp->viewerNode.lock();
}



void
ViewerTab::connectToAInput(int inputNb)
{
    ViewerNodePtr internalNode = getInternalNode();
    if (internalNode) {
        internalNode->connectInputToIndex(inputNb, 0);
    }
}

void
ViewerTab::connectToBInput(int inputNb)
{
    ViewerNodePtr internalNode = getInternalNode();
    if (internalNode) {
        internalNode->connectInputToIndex(inputNb, 1);
    }
}

static std::string makeUpFormatName(const RectI& format, double par)
{
    // Format name was empty, too bad, make up one
    std::stringstream ss;
    ss << format.width();
    ss << 'x';
    ss << format.height();
    if (par != 1.) {
        ss << ':';
        ss << QString::number(par, 'f', 2).toStdString();
    }
    return ss.str();
}

void
ViewerTab::setInfoBarAndViewerResolution(const RectI& rect, const RectD& canonicalRect, double par, int texIndex)
{
    std::string formatName, infoBarName;
    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return;
    }
    if (!app->getProject()->getFormatNameFromRect(rect, par, &formatName)) {
        formatName = makeUpFormatName(rect, par);
        infoBarName = formatName;
    } else {
        // If the format has a name, for the info bar also add the resolution
        std::stringstream ss;
        ss << formatName;
        ss << ' ';
        ss << rect.width();
        ss << 'x';
        ss << rect.height();
        infoBarName = ss.str();
    }
    _imp->infoWidget[texIndex]->setResolution(QString::fromUtf8(infoBarName.c_str()));
    _imp->viewer->setFormat(formatName, canonicalRect, par, texIndex);
}

QIcon
ViewerTab::getIcon() const
{
    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap p;
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_PANEL, iconSize, &p);
    return QIcon(p);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ViewerTab.cpp"
