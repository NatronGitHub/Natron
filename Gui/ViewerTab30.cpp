/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <QtCore/QDebug>

#include <QVBoxLayout>
#include <QCheckBox>
#include <QToolBar>

#include "Engine/Settings.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Button.h"
#include "Gui/ChannelsComboBox.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGui.h"
#include "Gui/RotoGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/TrackerGui.h"
#include "Gui/ViewerGL.h"


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif


using namespace Natron;

bool
ViewerTab::isClippedToProject() const
{
    return _imp->viewer->isClippingImageToProjectWindow();
}

std::string
ViewerTab::getColorSpace() const
{
    Natron::ViewerColorSpaceEnum lut = (Natron::ViewerColorSpaceEnum)_imp->viewerNode->getLutType();

    switch (lut) {
    case Natron::eViewerColorSpaceLinear:

        return "Linear(None)";
        break;
    case Natron::eViewerColorSpaceSRGB:

        return "sRGB";
        break;
    case Natron::eViewerColorSpaceRec709:

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
    _imp->viewerNode->onAutoContrastChanged(b,true);
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
ViewerTab::setColorSpace(const std::string & colorSpaceName)
{
    int index = _imp->viewerColorSpace->itemIndex( colorSpaceName.c_str() );

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

Natron::DisplayChannelsEnum
ViewerTab::getChannels() const
{
    return _imp->viewerNode->getChannels(0);
}

std::string
ViewerTab::getChannelsString(Natron::DisplayChannelsEnum c)
{
    switch (c) {
        case Natron::eDisplayChannelsRGB:
            
            return "RGB";
        case Natron::eDisplayChannelsR:
            
            return "R";
        case Natron::eDisplayChannelsG:
            
            return "G";
        case Natron::eDisplayChannelsB:
            
            return "B";
        case Natron::eDisplayChannelsA:
            
            return "A";
        case Natron::eDisplayChannelsY:
            
            return "Luminance";
            break;
        default:
            
            return "";
    }
}

std::string
ViewerTab::getChannelsString() const
{
    Natron::DisplayChannelsEnum c = _imp->viewerNode->getChannels(0);
    return getChannelsString(c);
}

void
ViewerTab::setChannels(const std::string & channelsStr)
{
    int index = _imp->viewerChannels->itemIndex( channelsStr.c_str() );

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
    _imp->viewerNode->onAutoContrastChanged(b,b);
    _imp->gammaBox->setEnabled(!b);
    _imp->gammaSlider->setEnabled(!b);
    _imp->toggleGammaButton->setEnabled(!b);
    if (!b) {
        _imp->viewerNode->onGainChanged( std::pow(2,_imp->gainBox->value()) );
        _imp->viewerNode->onGammaChanged(_imp->gammaBox->value());
    }
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

void
ViewerTab::setInfoBarResolution(const Format & f)
{
    _imp->infoWidget[0]->setResolution(f);
    _imp->infoWidget[1]->setResolution(f);
}

void
ViewerTab::createTrackerInterface(NodeGui* n)
{
    boost::shared_ptr<MultiInstancePanel> multiPanel = n->getMultiInstancePanel();
    if (!multiPanel) {
        return;
    }
    std::map<NodeGui*,TrackerGui*>::iterator found = _imp->trackerNodes.find(n);
    if (found != _imp->trackerNodes.end()) {
        return;
    }
    
    boost::shared_ptr<TrackerPanel> trackPanel = boost::dynamic_pointer_cast<TrackerPanel>(multiPanel);

    assert(trackPanel);
    TrackerGui* tracker = new TrackerGui(trackPanel,this);
    std::pair<std::map<NodeGui*,TrackerGui*>::iterator,bool> ret = _imp->trackerNodes.insert( std::make_pair(n,tracker) );
    assert(ret.second);
    if (!ret.second) {
        qDebug() << "ViewerTab::createTrackerInterface() failed";
        delete tracker;

        return;
    }
    QObject::connect( n,SIGNAL( settingsPanelClosed(bool) ),this,SLOT( onTrackerNodeGuiSettingsPanelClosed(bool) ) );
    if ( n->isSettingsPanelVisible() ) {
        setTrackerInterface(n);
    } else {
        QWidget* buttonsBar = tracker->getButtonsBar();
        assert(buttonsBar);
        if (buttonsBar) {
            buttonsBar->hide();
        }
    }
}

void
ViewerTab::setTrackerInterface(NodeGui* n)
{
    assert(n);
    std::map<NodeGui*,TrackerGui*>::iterator it = _imp->trackerNodes.find(n);
    if ( it != _imp->trackerNodes.end() ) {
        if (_imp->currentTracker.first == n) {
            return;
        }

        ///remove any existing tracker gui
        if (_imp->currentTracker.first != NULL) {
            removeTrackerInterface(_imp->currentTracker.first, false,true);
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
            _imp->mainLayout->insertWidget(index,buttonsBar);
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
ViewerTab::removeTrackerInterface(NodeGui* n,
                                  bool permanently,
                                  bool removeAndDontSetAnother)
{
    std::map<NodeGui*,TrackerGui*>::iterator it = _imp->trackerNodes.find(n);

    if ( it != _imp->trackerNodes.end() ) {
        if (!getGui()) {
            if (permanently) {
                delete it->second;
            }

            return;
        }

        if (_imp->currentTracker.first == n) {
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
                std::map<NodeGui*,TrackerGui*>::iterator newTracker = _imp->trackerNodes.end();
                for (std::map<NodeGui*,TrackerGui*>::iterator it2 = _imp->trackerNodes.begin(); it2 != _imp->trackerNodes.end(); ++it2) {
                    if ( (it2->second != it->second) && it2->first->isSettingsPanelVisible() ) {
                        newTracker = it2;
                        break;
                    }
                }

                _imp->currentTracker.first = 0;
                _imp->currentTracker.second = 0;

                if ( newTracker != _imp->trackerNodes.end() ) {
                    setTrackerInterface(newTracker->first);
                }
            }
        }

        if (permanently) {
            delete it->second;
            _imp->trackerNodes.erase(it);
        }
    }
}

void
ViewerTab::createRotoInterface(NodeGui* n)
{
    RotoGui* roto = new RotoGui( n,this,getRotoGuiSharedData(n) );
    QObject::connect( roto,SIGNAL( selectedToolChanged(int) ),getGui(),SLOT( onRotoSelectedToolChanged(int) ) );
    std::pair<std::map<NodeGui*,RotoGui*>::iterator,bool> ret = _imp->rotoNodes.insert( std::make_pair(n,roto) );

    assert(ret.second);
    if (!ret.second) {
        qDebug() << "ViewerTab::createRotoInterface() failed";
        delete roto;

        return;
    }
    QObject::connect( n,SIGNAL( settingsPanelClosed(bool) ),this,SLOT( onRotoNodeGuiSettingsPanelClosed(bool) ) );
    if ( n->isSettingsPanelVisible() ) {
        setRotoInterface(n);
    } else {
        roto->getToolBar()->hide();
        roto->getCurrentButtonsBar()->hide();
    }
}

void
ViewerTab::setRotoInterface(NodeGui* n)
{
    assert(n);
    std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.find(n);
    if ( it != _imp->rotoNodes.end() ) {
        if (_imp->currentRoto.first == n) {
            return;
        }

        ///remove any existing roto gui
        if (_imp->currentRoto.first != NULL) {
            removeRotoInterface(_imp->currentRoto.first, false,true);
        }

        ///Add the widgets
        QToolBar* toolBar = it->second->getToolBar();
        _imp->viewerLayout->insertWidget(0, toolBar);
        
        {
            QMutexLocker l(&_imp->visibleToolbarsMutex);
            if (_imp->leftToolbarVisible) {
                toolBar->show();
            }
        }

        ///If there's a tracker add it right after the tracker
        int index;
        if (_imp->currentTracker.second) {
            index = _imp->mainLayout->indexOf( _imp->currentTracker.second->getButtonsBar() );
            assert(index != -1);
            if (index >= 0) {
                ++index;
            }
        } else {
            index = _imp->mainLayout->indexOf(_imp->viewerContainer);
        }
        assert(index >= 0);
        if (index >= 0) {
            QWidget* buttonsBar = it->second->getCurrentButtonsBar();
            assert(buttonsBar);
            if (buttonsBar) {
                _imp->mainLayout->insertWidget(index,buttonsBar);
                {
                    QMutexLocker l(&_imp->visibleToolbarsMutex);
                    if (_imp->topToolbarVisible) {
                        buttonsBar->show();
                    }
                }
            }
        }
        QObject::connect( it->second,SIGNAL( roleChanged(int,int) ),this,SLOT( onRotoRoleChanged(int,int) ) );
        _imp->currentRoto.first = n;
        _imp->currentRoto.second = it->second;
        _imp->viewer->redraw();
    }
}

void
ViewerTab::removeRotoInterface(NodeGui* n,
                               bool permanently,
                               bool removeAndDontSetAnother)
{
    std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.find(n);

    if ( it != _imp->rotoNodes.end() ) {
        if (_imp->currentRoto.first == n) {
            QObject::disconnect( _imp->currentRoto.second,SIGNAL( roleChanged(int,int) ),this,SLOT( onRotoRoleChanged(int,int) ) );
            ///Remove the widgets of the current roto node
            assert(_imp->viewerLayout->count() > 1);
            QLayoutItem* currentToolBarItem = _imp->viewerLayout->itemAt(0);
            QToolBar* currentToolBar = qobject_cast<QToolBar*>( currentToolBarItem->widget() );
            currentToolBar->hide();
            assert( currentToolBar == _imp->currentRoto.second->getToolBar() );
            _imp->viewerLayout->removeItem(currentToolBarItem);
            int buttonsBarIndex = _imp->mainLayout->indexOf( _imp->currentRoto.second->getCurrentButtonsBar() );
            assert(buttonsBarIndex >= 0);
            QLayoutItem* buttonsBar = _imp->mainLayout->itemAt(buttonsBarIndex);
            assert(buttonsBar);
            _imp->mainLayout->removeItem(buttonsBar);
            buttonsBar->widget()->hide();

            if (!removeAndDontSetAnother) {
                ///If theres another roto node, set it as the current roto interface
                std::map<NodeGui*,RotoGui*>::iterator newRoto = _imp->rotoNodes.end();
                for (std::map<NodeGui*,RotoGui*>::iterator it2 = _imp->rotoNodes.begin(); it2 != _imp->rotoNodes.end(); ++it2) {
                    if ( (it2->second != it->second) && it2->first->isSettingsPanelVisible() ) {
                        newRoto = it2;
                        break;
                    }
                }

                _imp->currentRoto.first = 0;
                _imp->currentRoto.second = 0;

                if ( newRoto != _imp->rotoNodes.end() ) {
                    setRotoInterface(newRoto->first);
                }
            }
        }

        if (permanently) {
            delete it->second;
            _imp->rotoNodes.erase(it);
        }
    }
}

void
ViewerTab::getRotoContext(std::map<NodeGui*,RotoGui*>* rotoNodes,
                          std::pair<NodeGui*,RotoGui*>* currentRoto) const
{
    *rotoNodes = _imp->rotoNodes;
    *currentRoto = _imp->currentRoto;
}

void
ViewerTab::getTrackerContext(std::map<NodeGui*,TrackerGui*>* trackerNodes,
                             std::pair<NodeGui*,TrackerGui*>* currentTracker) const
{
    *trackerNodes = _imp->trackerNodes;
    *currentTracker = _imp->currentTracker;
}

void
ViewerTab::onRotoRoleChanged(int previousRole,
                             int newRole)
{
    RotoGui* roto = qobject_cast<RotoGui*>( sender() );

    if (roto) {
        assert(roto == _imp->currentRoto.second);

        ///Remove the previous buttons bar
        QWidget* previousBar = _imp->currentRoto.second->getButtonsBar( (RotoGui::RotoRoleEnum)previousRole ) ;
        assert(previousBar);
        if (previousBar) {
            int buttonsBarIndex = _imp->mainLayout->indexOf(previousBar);
            assert(buttonsBarIndex >= 0);
            if (buttonsBarIndex >= 0) {
                _imp->mainLayout->removeItem( _imp->mainLayout->itemAt(buttonsBarIndex) );
            }
            previousBar->hide();
        }

        ///Set the new buttons bar
        int viewerIndex = _imp->mainLayout->indexOf(_imp->viewerContainer);
        assert(viewerIndex >= 0);
        if (viewerIndex >= 0) {
            QWidget* currentBar = _imp->currentRoto.second->getButtonsBar( (RotoGui::RotoRoleEnum)newRole );
            assert(currentBar);
            if (currentBar) {
                _imp->mainLayout->insertWidget( viewerIndex, currentBar);
                currentBar->show();
                assert(_imp->mainLayout->itemAt(viewerIndex)->widget() == currentBar);
            }
        }
    }
}

void
ViewerTab::updateRotoSelectedTool(int tool,
                                  RotoGui* sender)
{
    if ( _imp->currentRoto.second && (_imp->currentRoto.second != sender) ) {
        _imp->currentRoto.second->setCurrentTool( (RotoGui::RotoToolEnum)tool,false );
    }
}

boost::shared_ptr<RotoGuiSharedData>
ViewerTab::getRotoGuiSharedData(NodeGui* node) const
{
    std::map<NodeGui*,RotoGui*>::const_iterator found = _imp->rotoNodes.find(node);

    if ( found == _imp->rotoNodes.end() ) {
        return boost::shared_ptr<RotoGuiSharedData>();
    } else {
        return found->second->getRotoGuiSharedData();
    }
}

void
ViewerTab::onRotoEvaluatedForThisViewer()
{
    getGui()->onViewerRotoEvaluated(this);
}

void
ViewerTab::onRotoNodeGuiSettingsPanelClosed(bool closed)
{
    NodeGui* n = qobject_cast<NodeGui*>( sender() );

    if (n) {
        if (closed) {
            removeRotoInterface(n, false,false);
        } else {
            if (n != _imp->currentRoto.first) {
                setRotoInterface(n);
            }
        }
    }
}

void
ViewerTab::onTrackerNodeGuiSettingsPanelClosed(bool closed)
{
    NodeGui* n = qobject_cast<NodeGui*>( sender() );

    if (n) {
        if (closed) {
            removeTrackerInterface(n, false,false);
        } else {
            if (n != _imp->currentTracker.first) {
                setTrackerInterface(n);
            }
        }
    }
}

void
ViewerTab::notifyGuiClosing()
{
    _imp->timeLineGui->discardGuiPointer();
    for (std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.begin() ; it!=_imp->rotoNodes.end(); ++it) {
        it->second->notifyGuiClosing();
    }
}

void
ViewerTab::onCompositingOperatorChangedInternal(Natron::ViewerCompositingOperatorEnum oldOp,Natron::ViewerCompositingOperatorEnum newOp)
{
    if ( (oldOp == eViewerCompositingOperatorNone) && (newOp != eViewerCompositingOperatorNone) ) {
        _imp->viewer->resetWipeControls();
    }
    
    _imp->secondInputImage->setEnabled_natron(newOp != eViewerCompositingOperatorNone);
    if (newOp == eViewerCompositingOperatorNone) {
        _imp->secondInputImage->setCurrentIndex_no_emit(0);
        _imp->viewerNode->setInputB(-1);
    }
    
    if (newOp == eViewerCompositingOperatorNone || !_imp->secondInputImage->getEnabled_natron()  || _imp->secondInputImage->activeIndex() == 0) {
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
    ViewerCompositingOperatorEnum newOp,oldOp;
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
            _imp->compOperator = eViewerCompositingOperatorOver;
            break;
        case 2:
            _imp->compOperator = eViewerCompositingOperatorUnder;
            break;
        case 3:
            _imp->compOperator = eViewerCompositingOperatorMinus;
            break;
        case 4:
            _imp->compOperator = eViewerCompositingOperatorWipe;
            break;
        default:
            break;
        }
        newOp = _imp->compOperator;
    }

    onCompositingOperatorChangedInternal(oldOp, newOp);
}

void
ViewerTab::setCompositingOperator(Natron::ViewerCompositingOperatorEnum op)
{
    int comboIndex;

    switch (op) {
    case Natron::eViewerCompositingOperatorNone:
        comboIndex = 0;
        break;
    case Natron::eViewerCompositingOperatorOver:
        comboIndex = 1;
        break;
    case Natron::eViewerCompositingOperatorUnder:
        comboIndex = 2;
        break;
    case Natron::eViewerCompositingOperatorMinus:
        comboIndex = 3;
        break;
    case Natron::eViewerCompositingOperatorWipe:
        comboIndex = 4;
        break;
    default:
        throw std::logic_error("ViewerTab::setCompositingOperator(): unknown operator");
        break;
    }
    Natron::ViewerCompositingOperatorEnum oldOp;
    {
        QMutexLocker l(&_imp->compOperatorMutex);
        oldOp = _imp->compOperator;
        _imp->compOperator = op;
        
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

void
ViewerTab::setInputA(int index)
{
    ViewerTabPrivate::InputNamesMap::iterator found = _imp->inputNamesMap.find(index);
    if (found == _imp->inputNamesMap.end()) {
        return;
    }
    
    int comboboxIndex = _imp->firstInputImage->itemIndex(found->second.name);
    if (comboboxIndex == -1) {
        return;
    }
    _imp->firstInputImage->setCurrentIndex(comboboxIndex);
    _imp->viewerNode->setInputA(index);
    _imp->viewerNode->renderCurrentFrame(true);
    
}

void
ViewerTab::setInputB(int index)
{
    ViewerTabPrivate::InputNamesMap::iterator found = _imp->inputNamesMap.find(index);
    if (found == _imp->inputNamesMap.end()) {
        return;
    }
    
    int comboboxIndex = _imp->secondInputImage->itemIndex(found->second.name);
    if (comboboxIndex == -1) {
        return;
    }
    _imp->secondInputImage->setCurrentIndex(comboboxIndex);
    _imp->viewerNode->setInputB(index);
    _imp->viewerNode->renderCurrentFrame(true);
}

void
ViewerTab::getActiveInputs(int* a, int* b) const
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
    
    int inputAIndex = -1,inputBIndex = -1;
    for (ViewerTabPrivate::InputNamesMap::iterator it = _imp->inputNamesMap.begin(); it != _imp->inputNamesMap.end(); ++it) {
        if (it->second.name == aIndex) {
            inputAIndex = it->first;
        } else if (it->second.name == bIndex) {
            inputBIndex = it->first;
        }
    }
    _imp->viewerNode->setInputA(inputBIndex);
    _imp->viewerNode->setInputB(inputAIndex);
    _imp->viewerNode->renderCurrentFrame(true);
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
    _imp->viewerNode->renderCurrentFrame(true);
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
        //setCompositingOperator(Natron::eViewerCompositingOperatorNone);
        _imp->infoWidget[1]->hide();
    } else {
        if ( !_imp->infoWidget[1]->isVisible() ) {
            _imp->infoWidget[1]->show();
            manageSlotsForInfoWidget(1, true);
            _imp->secondInputImage->setEnabled_natron(true);
            if (_imp->compOperator == Natron::eViewerCompositingOperatorNone) {
                _imp->viewer->resetWipeControls();
                setCompositingOperator(Natron::eViewerCompositingOperatorWipe);
            }
        }
    }
    _imp->viewerNode->renderCurrentFrame(true);
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

    Natron::ViewerCompositingOperatorEnum op = getCompositingOperator();
    _imp->secondInputImage->setEnabled_natron(op != Natron::eViewerCompositingOperatorNone);

    ViewerTabPrivate::InputNamesMap::iterator foundB = _imp->inputNamesMap.find(activeInputs[1]);
    if ( foundB != _imp->inputNamesMap.end() ) {
        int indexInB = _imp->secondInputImage->itemIndex(foundB->second.name);

        assert(indexInB != -1);
        _imp->secondInputImage->setCurrentIndex_no_emit(indexInB);
    } else {
        _imp->secondInputImage->setCurrentIndex_no_emit(0);
    }

    if (op == eViewerCompositingOperatorNone || !_imp->secondInputImage->getEnabled_natron()  || _imp->secondInputImage->activeIndex() == 0) {
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
    else*/ if ( autoWipe && (activeInputs[0] != -1) && (activeInputs[1] != -1) && (activeInputs[0] != activeInputs[1])
                && (op == eViewerCompositingOperatorNone) ) {
        _imp->viewer->resetWipeControls();
        setCompositingOperator(Natron::eViewerCompositingOperatorWipe);
    }
    
}

void
ViewerTab::connectToInput(int inputNb)
{
    InspectorNode* node = dynamic_cast<InspectorNode*>(getInternalNode()->getNode().get());
    assert(node);
    bool isAutoWipeEnabled = appPTR->getCurrentSettings()->isAutoWipeEnabled();
    if (isAutoWipeEnabled) {
        getInternalNode()->setActivateInputChangeRequestedFromViewer(true);
    }
    node->setActiveInputAndRefresh(inputNb, true);
    if (isAutoWipeEnabled) {
        getInternalNode()->setActivateInputChangeRequestedFromViewer(false);
    }
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
    EffectInstance* input0 = activeInputs[0] != - 1 ? _imp->viewerNode->getInput(activeInputs[0]) : 0;
    if (input0) {
        _imp->fpsBox->setValue(input0->getPreferredFrameRate());
    } else {
        EffectInstance* input1 = activeInputs[1] != - 1 ? _imp->viewerNode->getInput(activeInputs[1]) : 0;
        if (input1) {
            _imp->fpsBox->setValue(input1->getPreferredFrameRate());
        } else {
            _imp->fpsBox->setValue(getGui()->getApp()->getProjectFrameRate());
        }
    }
    onSpinboxFpsChangedInternal(_imp->fpsBox->value());
}

void
ViewerTab::onClipPreferencesChanged()
{
    //Try to set auto-fps if it is enabled
    if (_imp->fpsLocked) {
        refreshFPSBoxFromClipPreferences();
    }
}
