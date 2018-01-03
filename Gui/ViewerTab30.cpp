/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <cmath>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDebug>

#include <QVBoxLayout>
#include <QCheckBox>
#include <QToolBar>

#include "Engine/Settings.h"
#include "Engine/Node.h"
#include "Engine/Format.h"
#include "Engine/Project.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Button.h"
#include "Gui/ChannelsComboBox.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeViewerContext.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerGL.h"


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif


NATRON_NAMESPACE_ENTER

bool
ViewerTab::isClippedToProject() const
{
    return _imp->viewer->isClippingImageToFormat();
}

std::string
ViewerTab::getColorSpace() const
{
    ViewerColorSpaceEnum lut = (ViewerColorSpaceEnum)_imp->viewerNode->getLutType();

    switch (lut) {
    case eViewerColorSpaceLinear:

        return "Linear(None)";
        break;
    case eViewerColorSpaceSRGB:

        return "sRGB";
        break;
    case eViewerColorSpaceRec709:

        return "Rec.709";
        break;
    default:

        return "";
        break;
    }
}

void
ViewerTab::setUserRoIEnabled(bool b)
{
    onEnableViewerRoIButtonToggle(b);
}

bool
ViewerTab::isAutoContrastEnabled() const
{
    return _imp->viewerNode->isAutoContrastEnabled();
}

void
ViewerTab::setAutoContrastEnabled(bool b)
{
    _imp->autoContrast->setChecked(b);
    _imp->gainSlider->setEnabled(!b);
    _imp->gainBox->setEnabled(!b);
    _imp->toggleGainButton->setEnabled(!b);
    _imp->gammaSlider->setEnabled(!b);
    _imp->gammaBox->setEnabled(!b);
    _imp->toggleGammaButton->setEnabled(!b);
    _imp->viewerNode->onAutoContrastChanged(b, true);
}

void
ViewerTab::setUserRoI(const RectD & r)
{
    _imp->viewer->setUserRoI(r);
}

void
ViewerTab::setClipToProject(bool b)
{
    onClipToProjectButtonToggle(b);
}

void
ViewerTab::setFullFrameProcessing(bool fullFrame)
{
    onFullFrameButtonToggle(fullFrame);
}

bool
ViewerTab::isFullFrameProcessingEnabled() const
{
    return _imp->viewerNode->isFullFrameProcessingEnabled();
}

void
ViewerTab::setColorSpace(const std::string & colorSpaceName)
{
    int index = _imp->viewerColorSpace->itemIndex( QString::fromUtf8( colorSpaceName.c_str() ) );

    if (index != -1) {
        _imp->viewerColorSpace->setCurrentIndex(index);
    }
}

void
ViewerTab::setGain(double d)
{
    double fstop = std::log(d) / M_LN2;

    _imp->gainBox->setValue(fstop);
    _imp->gainSlider->seekScalePosition(fstop);
    _imp->viewer->setGain(d);
    _imp->viewerNode->onGainChanged(d);
    _imp->toggleGainButton->setDown(d != 1.);
    _imp->toggleGainButton->setChecked(d != 1.);
    _imp->lastFstopValue = fstop;
}

double
ViewerTab::getGain() const
{
    return _imp->viewerNode->getGain();
}

void
ViewerTab::setGamma(double gamma)
{
    _imp->gammaBox->setValue(gamma);
    _imp->gammaSlider->seekScalePosition(gamma);
    _imp->viewerNode->onGammaChanged(gamma);
    _imp->viewer->setGamma(gamma);
    _imp->toggleGammaButton->setDown(gamma != 1.);
    _imp->toggleGammaButton->setChecked(gamma != 1.);
    _imp->lastGammaValue = gamma;
}

double
ViewerTab::getGamma() const
{
    return _imp->viewerNode->getGamma();
}

void
ViewerTab::setMipMapLevel(int level)
{
    if (level > 0) {
        _imp->renderScaleCombo->setCurrentIndex(level - 1);
    }

    _imp->viewerNode->onMipMapLevelChanged(level);
    _imp->viewer->checkIfViewPortRoIValidOrRender();
}

int
ViewerTab::getMipMapLevel() const
{
    return _imp->viewerNode->getMipMapLevel();
}

void
ViewerTab::setRenderScaleActivated(bool act)
{
    onRenderScaleButtonClicked(act);
}

bool
ViewerTab::getRenderScaleActivated() const
{
    return _imp->viewerNode->getMipMapLevel() != 0;
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

DisplayChannelsEnum
ViewerTab::getChannels() const
{
    return _imp->viewerNode->getChannels(0);
}

std::string
ViewerTab::getChannelsString(DisplayChannelsEnum c)
{
    switch (c) {
    case eDisplayChannelsRGB:

        return "RGB";
    case eDisplayChannelsR:

        return "R";
    case eDisplayChannelsG:

        return "G";
    case eDisplayChannelsB:

        return "B";
    case eDisplayChannelsA:

        return "A";
    case eDisplayChannelsY:

        return "Luminance";
        break;
    default:

        return "";
    }
}

std::string
ViewerTab::getChannelsString() const
{
    DisplayChannelsEnum c = _imp->viewerNode->getChannels(0);

    return getChannelsString(c);
}

void
ViewerTab::setChannels(const std::string & channelsStr)
{
    int index = _imp->viewerChannels->itemIndex( QString::fromUtf8( channelsStr.c_str() ) );

    if (index != -1) {
        _imp->viewerChannels->setCurrentIndex_no_emit(index);
        setDisplayChannels(index, true);
    }
}

ViewerGL*
ViewerTab::getViewer() const
{
    return _imp->viewer;
}

ViewerInstance*
ViewerTab::getInternalNode() const
{
    return _imp->viewerNode;
}

void
ViewerTab::discardInternalNodePointer()
{
    _imp->viewerNode = 0;
}

void
ViewerTab::onAutoContrastChanged(bool b)
{
    _imp->autoContrast->setDown(b);
    _imp->autoContrast->setChecked(b);
    _imp->gainSlider->setEnabled(!b);
    _imp->gainBox->setEnabled(!b);
    _imp->toggleGainButton->setEnabled(!b);
    _imp->gammaBox->setEnabled(!b);
    _imp->gammaSlider->setEnabled(!b);
    _imp->toggleGammaButton->setEnabled(!b);
    if (!b) {
        _imp->viewerNode->onGainChanged( std::pow( 2, _imp->gainBox->value() ) );
        _imp->viewerNode->onGammaChanged( _imp->gammaBox->value() );
    }
    _imp->viewerNode->onAutoContrastChanged(b, true);
}

void
ViewerTab::onRenderScaleComboIndexChanged(int index)
{
    int level;

    if (_imp->renderScaleActive) {
        level = index + 1;
    } else {
        level = 0;
    }
    _imp->viewerNode->onMipMapLevelChanged(level);
    _imp->viewer->checkIfViewPortRoIValidOrRender();
}

void
ViewerTab::onRenderScaleButtonClicked(bool checked)
{
    _imp->activateRenderScale->blockSignals(true);
    _imp->renderScaleActive = checked;
    _imp->activateRenderScale->setDown(checked);
    _imp->activateRenderScale->setChecked(checked);
    _imp->activateRenderScale->blockSignals(false);
    onRenderScaleComboIndexChanged( _imp->renderScaleCombo->activeIndex() );
}

#if 0
void
ViewerTab::createTrackerInterface(const NodeGuiPtr& n)
{
    std::map<NodeGuiWPtr, TrackerGui*>::iterator found = _imp->trackerNodes.find(n);

    if ( found != _imp->trackerNodes.end() ) {
        return;
    }

    TrackerGui* tracker = 0;
    if ( n->getNode()->getEffectInstance()->isBuiltinTrackerNode() ) {
        TrackerPanel* panel = n->getTrackerPanel();
        assert(panel);
        tracker = new TrackerGui(panel, this);
    } else {
        assert( n->getNode()->isMultiInstance() );
        n->ensurePanelCreated();
        boost::shared_ptr<MultiInstancePanel> multiPanel = n->getMultiInstancePanel();
        if (multiPanel) {
            boost::shared_ptr<TrackerPanelV1> trackPanel = boost::dynamic_pointer_cast<TrackerPanelV1>(multiPanel);
            assert(trackPanel);
            tracker = new TrackerGui(trackPanel, this);
        }
    }


    std::pair<std::map<NodeGui*, TrackerGui*>::iterator, bool> ret = _imp->trackerNodes.insert( std::make_pair(n, tracker) );
    assert(ret.second);
    if (!ret.second) {
        qDebug() << "ViewerTab::createTrackerInterface() failed";
        delete tracker;

        return;
    }
    QObject::connect( n.get(), SIGNAL(settingsPanelClosed(bool)), this, SLOT(onTrackerNodeGuiSettingsPanelClosed(bool)) );
    if ( n->isSettingsPanelVisible() ) {
        setTrackerInterface(n);
    } else if (tracker) {
        QWidget* buttonsBar = tracker->getButtonsBar();
        assert(buttonsBar);
        if (buttonsBar) {
            buttonsBar->hide();
        }
    }
}

void
ViewerTab::setTrackerInterface(const NodeGuiPtr& n)
{
    assert(n);
    std::map<NodeGuiWPtr, TrackerGui*>::iterator it = _imp->trackerNodes.find(n);
    if ( it != _imp->trackerNodes.end() ) {
        if (_imp->currentTracker.first.lock() == n) {
            return;
        }

        ///remove any existing tracker gui
        if ( _imp->currentTracker.first.lock() ) {
            removeTrackerInterface(_imp->currentTracker.first.lock(), false, true);
        }

        ///Add the widgets

        ///if there's a current roto add it before it
        int index;
        if (_imp->currentRoto.second) {
            index = _imp->mainLayout->indexOf( _imp->currentRoto.second->getCurrentButtonsBar() );
            assert(index != -1);
        } else {
            index = _imp->mainLayout->indexOf(_imp->viewerContainer);
        }

        assert(index >= 0);
        QWidget* buttonsBar = it->second->getButtonsBar();
        assert(buttonsBar);
        if (buttonsBar) {
            _imp->mainLayout->insertWidget(index, buttonsBar);
            {
                QMutexLocker l(&_imp->visibleToolbarsMutex);
                if (_imp->topToolbarVisible) {
                    buttonsBar->show();
                }
            }
        }

        _imp->currentTracker.first = n;
        _imp->currentTracker.second = it->second;
        _imp->viewer->redraw();
    }
}

void
ViewerTab::removeTrackerInterface(const NodeGuiPtr& n,
                                  bool permanently,
                                  bool removeAndDontSetAnother)
{
    std::map<NodeGuiWPtr, TrackerGui*>::iterator it = _imp->trackerNodes.find(n);

    if ( it != _imp->trackerNodes.end() ) {
        if ( !getGui() ) {
            if (permanently) {
                delete it->second;
            }

            return;
        }

        if (_imp->currentTracker.first.lock() == n) {
            ///Remove the widgets of the current tracker node

            int buttonsBarIndex = _imp->mainLayout->indexOf( _imp->currentTracker.second->getButtonsBar() );
            assert(buttonsBarIndex >= 0);
            if (buttonsBarIndex >= 0) {
                QLayoutItem* buttonsBar = _imp->mainLayout->itemAt(buttonsBarIndex);
                assert(buttonsBar);
                if (buttonsBar) {
                    _imp->mainLayout->removeItem(buttonsBar);
                    buttonsBar->widget()->hide();
                }
            }
            if (!removeAndDontSetAnother) {
                ///If theres another tracker node, set it as the current tracker interface
                std::map<NodeGuiWPtr, TrackerGui*>::iterator newTracker = _imp->trackerNodes.end();
                for (std::map<NodeGuiWPtr, TrackerGui*>::iterator it2 = _imp->trackerNodes.begin(); it2 != _imp->trackerNodes.end(); ++it2) {
                    if ( (it2->second != it->second) && it2->first.lock()->isSettingsPanelVisible() ) {
                        newTracker = it2;
                        break;
                    }
                }

                _imp->currentTracker.first.reset();
                _imp->currentTracker.second = 0;

                if ( newTracker != _imp->trackerNodes.end() ) {
                    setTrackerInterface( newTracker->first.lock() );
                }
            }
        }

        if (permanently) {
            delete it->second;
            _imp->trackerNodes.erase(it);
        }
    }
} // ViewerTab::removeTrackerInterface

#endif // if 0

/**
 * @brief Creates a new viewer interface context for this node. This is not shared among viewers.
 **/
void
ViewerTab::createNodeViewerInterface(const NodeGuiPtr& n)
{
    if (!n) {
        return;
    }
    std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator found = _imp->nodesContext.find(n);
    if ( found != _imp->nodesContext.end() ) {
        // Already exists
        return;
    }

    boost::shared_ptr<NodeViewerContext> nodeContext = NodeViewerContext::create(n, this);
    nodeContext->createGui();
    _imp->nodesContext.insert( std::make_pair(n, nodeContext) );

    if ( n->isSettingsPanelVisible() ) {
        setPluginViewerInterface(n);
    } else {
        QToolBar *toolbar = nodeContext->getToolBar();
        if (toolbar) {
            toolbar->hide();
        }
        QWidget* w = nodeContext->getContainerWidget();
        if (w) {
            w->hide();
        }
    }
}

/**
 * @brief Set the current viewer interface for a given plug-in to be the one of the given node
 **/
void
ViewerTab::setPluginViewerInterface(const NodeGuiPtr& n)
{
    if (!n) {
        return;
    }
    std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator it = _imp->nodesContext.find(n);
    if ( it == _imp->nodesContext.end() ) {
        return;
    }

    std::string pluginID = n->getNode()->getPluginID();
    std::list<ViewerTabPrivate::PluginViewerContext>::iterator foundActive = _imp->findActiveNodeContextForPlugin(pluginID);
    NodeGuiPtr activeNodeForPlugin;
    if ( foundActive != _imp->currentNodeContext.end() ) {
        activeNodeForPlugin = foundActive->currentNode.lock();
    }

    // If already active, return
    if (activeNodeForPlugin == n) {
        return;
    }

    QToolBar* newToolbar = it->second->getToolBar();
    QWidget* newContainer = it->second->getContainerWidget();

    // Plugin has no viewer interface
    if (!newToolbar && !newContainer) {
        return;
    }

    ///remove any existing roto gui
    if (activeNodeForPlugin) {
        removeNodeViewerInterface(activeNodeForPlugin, false /*permanantly*/, /*setAnother*/ false);
    }

    // Add the widgets

    if (newToolbar) {
        _imp->viewerLayout->insertWidget(0, newToolbar);

        {
            QMutexLocker l(&_imp->visibleToolbarsMutex);
            if (_imp->leftToolbarVisible) {
                newToolbar->show();
            }
        }
    }

    // If there are other interface active, add it after them
    int index;
    if ( _imp->currentNodeContext.empty() ) {
        // insert before the viewer
        index = _imp->mainLayout->indexOf(_imp->viewerContainer);
    } else {
        // Remove the oldest opened interface if we reached the maximum
        int maxNodeContextOpened = appPTR->getCurrentSettings()->getMaxOpenedNodesViewerContext();
        if ( (int)_imp->currentNodeContext.size() == maxNodeContextOpened ) {
            const ViewerTabPrivate::PluginViewerContext& oldestNodeViewerInterface = _imp->currentNodeContext.front();
            removeNodeViewerInterface(oldestNodeViewerInterface.currentNode.lock(), false /*permanantly*/, false /*setAnother*/);
        }

        QWidget* container = _imp->currentNodeContext.back().currentContext->getContainerWidget();
        index = _imp->mainLayout->indexOf(container);
        assert(index != -1);
        if (index >= 0) {
            ++index;
        }
    }
    assert(index >= 0);
    if (index >= 0) {
        assert(newContainer);
        if (newContainer) {
            _imp->mainLayout->insertWidget(index, newContainer);
            {
                QMutexLocker l(&_imp->visibleToolbarsMutex);
                if (_imp->topToolbarVisible) {
                    newContainer->show();
                }
            }
        }
    }
    ViewerTabPrivate::PluginViewerContext p;
    p.pluginID = pluginID;
    p.currentNode = n;
    p.currentContext = it->second;
    _imp->currentNodeContext.push_back(p);

    _imp->viewer->redraw();
} // ViewerTab::setPluginViewerInterface

/**
 * @brief Removes the interface associated to the given node.
 * @param permanently The interface is destroyed instead of being hidden
 * @param setAnotherFromSamePlugin If true, if another node of the same plug-in is a candidate for a viewer interface, it will replace the existing
 * viewer interface for this plug-in
 **/
void
ViewerTab::removeNodeViewerInterface(const NodeGuiPtr& n,
                                     bool permanently,
                                     bool setAnotherFromSamePlugin)
{
    std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator found = _imp->nodesContext.find(n);

    if ( found == _imp->nodesContext.end() ) {
        return;
    }

    std::string pluginID = n->getNode()->getPluginID();
    NodeGuiPtr activeNodeForPlugin;
    QToolBar* activeItemToolBar = 0;
    QWidget* activeItemContainer = 0;

    {
        // Keep the iterator under this scope since we erase it
        std::list<ViewerTabPrivate::PluginViewerContext>::iterator foundActive = _imp->findActiveNodeContextForPlugin(pluginID);
        if ( foundActive != _imp->currentNodeContext.end() ) {
            activeNodeForPlugin = foundActive->currentNode.lock();

            if (activeNodeForPlugin == n) {
                activeItemToolBar = foundActive->currentContext->getToolBar();
                activeItemContainer = foundActive->currentContext->getContainerWidget();
                _imp->currentNodeContext.erase(foundActive);
            }
        }
    }

    // Remove the widgets of the current node
    if (activeItemToolBar) {
        int nLayoutItems = _imp->viewerLayout->count();
        for (int i = 0; i < nLayoutItems; ++i) {
            QLayoutItem* item = _imp->viewerLayout->itemAt(i);
            if (item->widget() == activeItemToolBar) {
                activeItemToolBar->hide();
                _imp->viewerLayout->removeItem(item);
                break;
            }
        }
    }
    if (activeItemContainer) {
        int buttonsBarIndex = _imp->mainLayout->indexOf(activeItemContainer);
        assert(buttonsBarIndex >= 0);
        if (buttonsBarIndex >= 0) {
            _imp->mainLayout->removeWidget(activeItemContainer);
        }
        activeItemContainer->hide();
    }

    if (setAnotherFromSamePlugin && activeNodeForPlugin == n) {
        ///If theres another roto node, set it as the current roto interface
        std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator newInterface = _imp->nodesContext.end();
        for (std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator it2 = _imp->nodesContext.begin(); it2 != _imp->nodesContext.end(); ++it2) {
            NodeGuiPtr otherNode = it2->first.lock();
            if (!otherNode) {
                continue;
            }
            if (otherNode == n) {
                continue;
            }
            if (otherNode->getNode()->getPluginID() != pluginID) {
                continue;
            }
            if ( (it2->second != found->second) && it2->first.lock()->isSettingsPanelVisible() ) {
                newInterface = it2;
                break;
            }
        }

        if ( newInterface != _imp->nodesContext.end() ) {
            setPluginViewerInterface( newInterface->first.lock() );
        }
    }


    if (permanently) {
        found->second.reset();
        _imp->nodesContext.erase(found);
    }
} // ViewerTab::removeNodeViewerInterface

/**
 * @brief Get the list of all nodes that have a user interface created on this viewer (but not necessarily displayed)
 * and a list for each plug-in of the active node.
 **/
void
ViewerTab::getNodesViewerInterface(std::list<NodeGuiPtr>* nodesWithUI,
                                   std::list<NodeGuiPtr>* perPluginActiveUI) const
{
    for (std::map<NodeGuiWPtr, NodeViewerContextPtr>::const_iterator it = _imp->nodesContext.begin(); it != _imp->nodesContext.end(); ++it) {
        NodeGuiPtr n = it->first.lock();
        if (n) {
            nodesWithUI->push_back(n);
        }
    }
    for (std::list<ViewerTabPrivate::PluginViewerContext>::const_iterator it = _imp->currentNodeContext.begin(); it != _imp->currentNodeContext.end(); ++it) {
        NodeGuiPtr n = it->currentNode.lock();
        if (n) {
            perPluginActiveUI->push_back(n);
        }
    }
}

void
ViewerTab::updateSelectedToolForNode(const QString& toolID,
                                     const NodeGuiPtr& node)
{
    std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator found = _imp->nodesContext.find(node);

    if ( found == _imp->nodesContext.end() ) {
        // Already exists
        return;
    }
    found->second->setCurrentTool(toolID, false /*notifyNode*/);
}

void
ViewerTab::notifyGuiClosing()
{
    _imp->timeLineGui->discardGuiPointer();
    for (std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator it = _imp->nodesContext.begin(); it != _imp->nodesContext.end(); ++it) {
        it->second->notifyGuiClosing();
    }
}

void
ViewerTab::onCompositingOperatorChangedInternal(ViewerCompositingOperatorEnum oldOp,
                                                ViewerCompositingOperatorEnum newOp)
{
    // Do not reset the wipe controls, this makes "toggle wipe" impractical!
    // instead, teh user should use the "center wipe" shortcut (Shift-F)
    //if ( (oldOp == eViewerCompositingOperatorNone) && (newOp != eViewerCompositingOperatorNone) ) {
    //    _imp->viewer->resetWipeControls();
    //}
    Q_UNUSED(oldOp);

    _imp->secondInputImage->setEnabled_natron(newOp != eViewerCompositingOperatorNone);

    // Do not reset input B, it should only be disabled! Else "toggle wipe" becomes impractical
    //if (newOp == eViewerCompositingOperatorNone) {
    //    _imp->secondInputImage->setCurrentIndex_no_emit(0);
    //    _imp->viewerNode->setInputB(-1);
    //}

    if ( (newOp == eViewerCompositingOperatorNone) || !_imp->secondInputImage->getEnabled_natron()  || (_imp->secondInputImage->activeIndex() == 0) ) {
        manageSlotsForInfoWidget(1, false);
        _imp->infoWidget[1]->hide();
    } else if (newOp != eViewerCompositingOperatorNone) {
        manageSlotsForInfoWidget(1, true);
        _imp->infoWidget[1]->show();
    }

    _imp->viewer->update();
}

void
ViewerTab::onCompositingOperatorIndexChanged(int index)
{
    ViewerCompositingOperatorEnum newOp, oldOp;
    {
        QMutexLocker l(&_imp->compOperatorMutex);
        oldOp = _imp->compOperator;
        switch (index) {
        case 0:
            _imp->compOperator = eViewerCompositingOperatorNone;
            _imp->secondInputImage->setEnabled_natron(false);
            manageSlotsForInfoWidget(1, false);
            _imp->infoWidget[1]->hide();
            break;
        case 1:
            _imp->compOperator = eViewerCompositingOperatorWipeUnder;
            break;
        case 2:
            _imp->compOperator = eViewerCompositingOperatorWipeOver;
            break;
        case 3:
            _imp->compOperator = eViewerCompositingOperatorWipeMinus;
            break;
        case 4:
            _imp->compOperator = eViewerCompositingOperatorWipeOnionSkin;
            break;
        case 5:
            _imp->compOperator = eViewerCompositingOperatorStackUnder;
            break;
        case 6:
            _imp->compOperator = eViewerCompositingOperatorStackOver;
            break;
        case 7:
            _imp->compOperator = eViewerCompositingOperatorStackMinus;
            break;
        case 8:
            _imp->compOperator = eViewerCompositingOperatorStackOnionSkin;
            break;

        default:
            break;
        }
        assert(index == (int)_imp->compOperator);
        newOp = _imp->compOperator;
    }

    onCompositingOperatorChangedInternal(oldOp, newOp);
}

void
ViewerTab::setCompositingOperator(ViewerCompositingOperatorEnum op)
{
    int comboIndex;

    switch (op) {
    case eViewerCompositingOperatorNone:
        comboIndex = 0;
        break;
    case eViewerCompositingOperatorWipeUnder:
        comboIndex = 1;
        break;
    case eViewerCompositingOperatorWipeOver:
        comboIndex = 2;
        break;
    case eViewerCompositingOperatorWipeMinus:
        comboIndex = 3;
        break;
    case eViewerCompositingOperatorWipeOnionSkin:
        comboIndex = 4;
        break;
    case eViewerCompositingOperatorStackUnder:
        comboIndex = 5;
        break;
    case eViewerCompositingOperatorStackOver:
        comboIndex = 6;
        break;
    case eViewerCompositingOperatorStackMinus:
        comboIndex = 7;
        break;
    case eViewerCompositingOperatorStackOnionSkin:
        comboIndex = 8;
        break;

    default:
        throw std::logic_error("ViewerTab::setCompositingOperator(): unknown operator");
        break;
    }
    assert(comboIndex == (int)op);

    ViewerCompositingOperatorEnum oldOp;
    {
        QMutexLocker l(&_imp->compOperatorMutex);
        oldOp = _imp->compOperator;
        _imp->compOperator = op;
        _imp->compOperatorPrevious = oldOp;
    }
    _imp->compositingOperator->setCurrentIndex_no_emit(comboIndex);
    onCompositingOperatorChangedInternal(oldOp, op);
}

ViewerCompositingOperatorEnum
ViewerTab::getCompositingOperator() const
{
    QMutexLocker l(&_imp->compOperatorMutex);

    return _imp->compOperator;
}

ViewerCompositingOperatorEnum
ViewerTab::getCompositingOperatorPrevious() const
{
    QMutexLocker l(&_imp->compOperatorMutex);

    return _imp->compOperatorPrevious;
}

void
ViewerTab::setInputA(int index)
{
    ViewerTabPrivate::InputNamesMap::iterator found = _imp->inputNamesMap.find(index);

    if ( found == _imp->inputNamesMap.end() ) {
        return;
    }

    int comboboxIndex = _imp->firstInputImage->itemIndex(found->second.name);
    if (comboboxIndex == -1) {
        return;
    }
    _imp->firstInputImage->setCurrentIndex(comboboxIndex);
    _imp->viewerNode->setInputA(index);

    abortViewersAndRefresh();
}

void
ViewerTab::setInputB(int index)
{
    ViewerTabPrivate::InputNamesMap::iterator found = _imp->inputNamesMap.find(index);

    if ( found == _imp->inputNamesMap.end() ) {
        return;
    }

    int comboboxIndex = _imp->secondInputImage->itemIndex(found->second.name);
    if (comboboxIndex == -1) {
        return;
    }
    _imp->secondInputImage->setCurrentIndex(comboboxIndex);
    _imp->viewerNode->setInputB(index);
    abortViewersAndRefresh();
}

void
ViewerTab::getActiveInputs(int* a,
                           int* b) const
{
    _imp->viewerNode->getActiveInputs(*a, *b);
}

void
ViewerTab::switchInputAAndB()
{
    QString aIndex = _imp->firstInputImage->getCurrentIndexText();
    QString bIndex = _imp->secondInputImage->getCurrentIndexText();

    _imp->firstInputImage->setCurrentText_no_emit(bIndex);
    _imp->secondInputImage->setCurrentText_no_emit(aIndex);

    int inputAIndex = -1, inputBIndex = -1;
    for (ViewerTabPrivate::InputNamesMap::iterator it = _imp->inputNamesMap.begin(); it != _imp->inputNamesMap.end(); ++it) {
        if (it->second.name == aIndex) {
            inputAIndex = it->first;
        } else if (it->second.name == bIndex) {
            inputBIndex = it->first;
        }
    }
    if (inputBIndex == inputAIndex) {
        return;
    }
    _imp->viewerNode->setInputA(inputBIndex);
    _imp->viewerNode->setInputB(inputAIndex);

    abortViewersAndRefresh();
}

///Called when the user change the combobox choice
void
ViewerTab::onFirstInputNameChanged(const QString & text)
{
    int inputIndex = -1;

    for (ViewerTabPrivate::InputNamesMap::iterator it = _imp->inputNamesMap.begin(); it != _imp->inputNamesMap.end(); ++it) {
        if (it->second.name == text) {
            inputIndex = it->first;
            break;
        }
    }
    _imp->viewerNode->setInputA(inputIndex);

    abortViewersAndRefresh();
}

///Called when the user change the combobox choice
void
ViewerTab::onSecondInputNameChanged(const QString & text)
{
    int inputIndex = -1;

    for (ViewerTabPrivate::InputNamesMap::iterator it = _imp->inputNamesMap.begin(); it != _imp->inputNamesMap.end(); ++it) {
        if (it->second.name == text) {
            inputIndex = it->first;
            break;
        }
    }
    _imp->viewerNode->setInputB(inputIndex);
    if (inputIndex == -1) {
        manageSlotsForInfoWidget(1, false);
        //setCompositingOperator(eViewerCompositingOperatorNone);
        _imp->infoWidget[1]->hide();
    } else {
        if ( !_imp->infoWidget[1]->isVisible() ) {
            _imp->infoWidget[1]->show();
            manageSlotsForInfoWidget(1, true);
            _imp->secondInputImage->setEnabled_natron(true);
            if (_imp->compOperator == eViewerCompositingOperatorNone) {
                //_imp->viewer->resetWipeControls();
                setCompositingOperator(eViewerCompositingOperatorWipeUnder);
            }
        }
    }
    abortViewersAndRefresh();
}

///This function is called only when the user changed inputs on the node graph
void
ViewerTab::onActiveInputsChanged()
{
    int activeInputs[2];

    _imp->viewerNode->getActiveInputs(activeInputs[0], activeInputs[1]);
    ViewerTabPrivate::InputNamesMap::iterator foundA = _imp->inputNamesMap.find(activeInputs[0]);
    if ( foundA != _imp->inputNamesMap.end() ) {
        int indexInA = _imp->firstInputImage->itemIndex(foundA->second.name);
        assert(indexInA != -1);
        if (indexInA != -1) {
            _imp->firstInputImage->setCurrentIndex_no_emit(indexInA);
        }
    } else {
        _imp->firstInputImage->setCurrentIndex_no_emit(0);
    }

    ViewerCompositingOperatorEnum op = getCompositingOperator();
    _imp->secondInputImage->setEnabled_natron(op != eViewerCompositingOperatorNone);

    ViewerTabPrivate::InputNamesMap::iterator foundB = _imp->inputNamesMap.find(activeInputs[1]);
    if ( foundB != _imp->inputNamesMap.end() ) {
        int indexInB = _imp->secondInputImage->itemIndex(foundB->second.name);

        assert(indexInB != -1);
        _imp->secondInputImage->setCurrentIndex_no_emit(indexInB);
    } else {
        _imp->secondInputImage->setCurrentIndex_no_emit(0);
    }

    if ( (op == eViewerCompositingOperatorNone) || !_imp->secondInputImage->getEnabled_natron()  || (_imp->secondInputImage->activeIndex() == 0) ) {
        manageSlotsForInfoWidget(1, false);
        _imp->infoWidget[1]->hide();
    } else if (op != eViewerCompositingOperatorNone) {
        manageSlotsForInfoWidget(1, true);
        _imp->infoWidget[1]->show();
    }

    bool autoWipe = getInternalNode()->isInputChangeRequestedFromViewer();

    /*if ( ( (activeInputs[0] == -1) || (activeInputs[1] == -1) ) //only 1 input is valid
         && ( op != eViewerCompositingOperatorNone) ) {
        //setCompositingOperator(eViewerCompositingOperatorNone);
        _imp->infoWidget[1]->hide();
        manageSlotsForInfoWidget(1, false);
        // _imp->secondInputImage->setEnabled_natron(false);
       }
       else*/if ( autoWipe && (activeInputs[0] != -1) && (activeInputs[1] != -1) && (activeInputs[0] != activeInputs[1])
         && (op == eViewerCompositingOperatorNone) ) {
        //_imp->viewer->resetWipeControls();
        setCompositingOperator(eViewerCompositingOperatorWipeUnder);
    }
}

void
ViewerTab::connectToInput(int inputNb,
                          bool isASide)
{
    InspectorNode* node = dynamic_cast<InspectorNode*>( getInternalNode()->getNode().get() );

    assert(node);
    if (!node) {
        return;
    }
    bool isAutoWipeEnabled = appPTR->getCurrentSettings()->isAutoWipeEnabled();
    if (isAutoWipeEnabled) {
        getInternalNode()->setActivateInputChangeRequestedFromViewer(true);
    }
    node->setActiveInputAndRefresh(inputNb, isASide);
    if (isAutoWipeEnabled) {
        getInternalNode()->setActivateInputChangeRequestedFromViewer(false);
    }
}

void
ViewerTab::connectToAInput(int inputNb)
{
    connectToInput(inputNb, true);
}

void
ViewerTab::connectToBInput(int inputNb)
{
    connectToInput(inputNb, false);
}

void
ViewerTab::onAvailableComponentsChanged()
{
    refreshLayerAndAlphaChannelComboBox();
}

void
ViewerTab::refreshFPSBoxFromClipPreferences()
{
    int activeInputs[2];

    _imp->viewerNode->getActiveInputs(activeInputs[0], activeInputs[1]);
    EffectInstPtr input0 = activeInputs[0] != -1 ? _imp->viewerNode->getInput(activeInputs[0]) : EffectInstPtr();
    if (input0) {
        _imp->fpsBox->setValue( input0->getFrameRate() );
    } else {
        EffectInstPtr input1 = activeInputs[1] != -1 ? _imp->viewerNode->getInput(activeInputs[1]) : EffectInstPtr();
        if (input1) {
            _imp->fpsBox->setValue( input1->getFrameRate() );
        } else {
            _imp->fpsBox->setValue( getGui()->getApp()->getProjectFrameRate() );
        }
    }
    onSpinboxFpsChangedInternal( _imp->fpsBox->value() );
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
    if (!getGui()->getApp()->getProject()->getFormatNameFromRect(rect, par, &formatName)) {
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

void
ViewerTab::onClipPreferencesChanged()
{
    //Try to set auto-fps if it is enabled
    if (_imp->fpsLocked) {
        refreshFPSBoxFromClipPreferences();
    }

    Format format;
    getGui()->getApp()->getProject()->getProjectDefaultFormat(&format);
    RectD canonicalFormat = format.toCanonicalFormat();

    int activeInputs[2];
    _imp->viewerNode->getActiveInputs(activeInputs[0], activeInputs[1]);
    for (int i = 0; i < 2; ++i) {
        EffectInstPtr input;
        if (activeInputs[i] != -1) {
            input = _imp->viewerNode->getInput(activeInputs[i]);
        }
        if (!input) {
            setInfoBarAndViewerResolution(format, canonicalFormat, format.getPixelAspectRatio(), i);
        } else {
            RectI inputFormat = input->getOutputFormat();
            RectD inputCanonicalFormat;
            double inputPar = input->getAspectRatio(-1);
            inputFormat.toCanonical_noClipping(0, inputPar, &inputCanonicalFormat);

            setInfoBarAndViewerResolution(inputFormat, inputCanonicalFormat, inputPar, i);

        }

    }

}

NATRON_NAMESPACE_EXIT
