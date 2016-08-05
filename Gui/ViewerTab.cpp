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
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"


NATRON_NAMESPACE_ENTER;


static void
makeFullyQualifiedLabel(const NodePtr& node,
                        std::string* ret)
{
    NodeCollectionPtr parent = node->getGroup();
    NodeGroupPtr isParentGrp = toNodeGroup(parent);
    std::string toPreprend = node->getLabel();

    if (isParentGrp) {
        toPreprend.insert(0, "/");
    }
    ret->insert(0, toPreprend);
    if (isParentGrp) {
        makeFullyQualifiedLabel(isParentGrp->getNode(), ret);
    }
}


ViewerTab::ViewerTab(const std::list<NodeGuiPtr> & existingNodesContext,
                     const std::list<NodeGuiPtr>& activePluginsContext,
                     Gui* gui,
                     const NodeGuiPtr& node_ui,
                     QWidget* parent)
    : QWidget(parent)
    , PanelWidget(this, gui)
    , _imp( new ViewerTabPrivate(this, node_ui) )
{
    ViewerNodePtr node = node_ui->getNode()->isEffectViewerNode();
    installEventFilter(this);

    std::string nodeName =  node->getNode()->getFullyQualifiedName();
    for (std::size_t i = 0; i < nodeName.size(); ++i) {
        if (nodeName[i] == '.') {
            nodeName[i] = '_';
        }
    }
    setScriptName(nodeName);
    std::string label;
    makeFullyQualifiedLabel(node->getNode(), &label);
    setLabel(label);

    NodePtr internalNode = node->getNode();
    QObject::connect( internalNode.get(), SIGNAL(scriptNameChanged(QString)), this, SLOT(onInternalNodeScriptNameChanged(QString)) );
    QObject::connect( internalNode.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInternalNodeLabelChanged(QString)) );
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


    _imp->viewer = new ViewerGL(this);
    _imp->viewerSubContainerLayout->addWidget(_imp->viewer);

    QString inputNames[2] = {
        QString::fromUtf8("A:"), QString::fromUtf8("B:")
    };
    for (int i = 0; i < 2; ++i) {
        _imp->infoWidget[i] = new InfoViewerWidget(inputNames[i], this);
        _imp->viewerSubContainerLayout->addWidget(_imp->infoWidget[i]);
        _imp->viewer->setInfoViewer(_imp->infoWidget[i], i);
        if (i == 1) {
            _imp->infoWidget[i]->hide();
        }
    }

    _imp->viewerLayout->addWidget(_imp->viewerSubContainer);
    _imp->mainLayout->addWidget(_imp->viewerContainer);


    TimeLinePtr timeline = getGui()->getApp()->getTimeLine();
    _imp->timeLineGui = new TimeLineGui(node, timeline, getGui(), this);
    QObject::connect( _imp->timeLineGui, SIGNAL(boundariesChanged(SequenceTime,SequenceTime)),
                      this, SLOT(onTimelineBoundariesChanged(SequenceTime,SequenceTime)) );
    QObject::connect( gui->getApp()->getProject().get(), SIGNAL(frameRangeChanged(int,int)), _imp->timeLineGui, SLOT(onProjectFrameRangeChanged(int,int)) );
    _imp->timeLineGui->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    //Add some spacing because the timeline might be black as the info
    _imp->mainLayout->addSpacing( TO_DPIY(5) );
    _imp->mainLayout->addWidget(_imp->timeLineGui);

    double leftBound, rightBound;
    leftBound = node->getPlaybackInPointKnob()->getValue();
    rightBound = node->getPlaybackOutPointKnob()->getValue();

    double projectLeft, projectRight;
    gui->getApp()->getFrameRange(&projectLeft, &projectRight);

    _imp->timeLineGui->setBoundaries(leftBound, rightBound);
    onTimelineBoundariesChanged(leftBound, rightBound);
    _imp->timeLineGui->setFrameRangeEdited(projectLeft != leftBound || projectRight != rightBound);;


    manageTimelineSlot(false, timeline);

    QObject::connect( node.get(), SIGNAL(renderStatsAvailable(int,ViewIdx,double,RenderStatsMap)),
                      this, SLOT(onRenderStatsAvailable(int,ViewIdx,double,RenderStatsMap)) );
    QObject::connect( _imp->viewer, SIGNAL(zoomChanged(int)), this, SLOT(updateZoomComboBox(int)) );
    QObject::connect( node.get(), SIGNAL(viewerDisconnected()), this, SLOT(disconnectViewer()) );


   
    connectToViewerCache();

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

    createNodeViewerInterface(node_ui);
    setPluginViewerInterface(node_ui);

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

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ViewerTab.cpp"
