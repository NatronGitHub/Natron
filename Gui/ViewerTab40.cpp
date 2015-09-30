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

#include <QtCore/QDebug>

#include <QApplication>
#include <QCheckBox>
#include <QToolBar>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodePtr
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Button.h"
#include "Gui/ChannelsComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/Gui.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/LineEdit.h"
#include "Gui/NodeGraph.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/RotoGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/TrackerGui.h"
#include "Gui/ViewerGL.h"


using namespace Natron;

void
ViewerTab::onInputChanged(int inputNb)
{
    ///rebuild the name maps
    NodePtr inp;
    const std::vector<boost::shared_ptr<Natron::Node> > &inputs  = _imp->viewerNode->getNode()->getGuiInputs();
    if (inputNb >= 0 && inputNb < (int)inputs.size()) {
        if (inputs[inputNb]) {
            inp = inputs[inputNb];
        }
    }
    

    if (inp) {
        ViewerTabPrivate::InputNamesMap::iterator found = _imp->inputNamesMap.find(inputNb);
        if ( found != _imp->inputNamesMap.end() ) {
            NodePtr input = found->second.input.lock();
            if (!input) {
                return;
            }
            const std::string & curInputName = input->getLabel();
            found->second.input = inp;
            int indexInA = _imp->firstInputImage->itemIndex( curInputName.c_str() );
            int indexInB = _imp->secondInputImage->itemIndex( curInputName.c_str() );
            assert(indexInA != -1 && indexInB != -1);
            found->second.name = inp->getLabel().c_str();
            _imp->firstInputImage->setItemText(indexInA, found->second.name);
            _imp->secondInputImage->setItemText(indexInB, found->second.name);
        } else {
            ViewerTabPrivate::InputName inpName;
            inpName.input = inp;
            inpName.name = inp->getLabel().c_str();
            _imp->inputNamesMap.insert( std::make_pair(inputNb,inpName) );
            _imp->firstInputImage->addItem(inpName.name);
            _imp->secondInputImage->addItem(inpName.name);
        }
    } else {
        ViewerTabPrivate::InputNamesMap::iterator found = _imp->inputNamesMap.find(inputNb);

        ///The input has been disconnected
        if ( found != _imp->inputNamesMap.end() ) {
            
            NodePtr input = found->second.input.lock();
            if (!input) {
                return;
            }
            const std::string & curInputName = input->getLabel();
            _imp->firstInputImage->blockSignals(true);
            _imp->secondInputImage->blockSignals(true);
            _imp->firstInputImage->removeItem( curInputName.c_str() );
            _imp->secondInputImage->removeItem( curInputName.c_str() );
            _imp->firstInputImage->blockSignals(false);
            _imp->secondInputImage->blockSignals(false);
            _imp->inputNamesMap.erase(found);
        }
    }
    
    //refreshLayerAndAlphaChannelComboBox();
}

void
ViewerTab::onInputNameChanged(int inputNb,
                              const QString & name)
{
    ViewerTabPrivate::InputNamesMap::iterator found = _imp->inputNamesMap.find(inputNb);

    assert( found != _imp->inputNamesMap.end() );
    int indexInA = _imp->firstInputImage->itemIndex(found->second.name);
    int indexInB = _imp->secondInputImage->itemIndex(found->second.name);
    assert(indexInA != -1 && indexInB != -1);
    _imp->firstInputImage->setItemText(indexInA, name);
    _imp->secondInputImage->setItemText(indexInB, name);
    found->second.name = name;
}

void
ViewerTab::manageSlotsForInfoWidget(int textureIndex,
                                    bool connect)
{
    RenderEngine* engine = _imp->viewerNode->getRenderEngine();
    assert(engine);
    if (connect) {
        QObject::connect( engine, SIGNAL( fpsChanged(double,double) ), _imp->infoWidget[textureIndex], SLOT( setFps(double,double) ) );
        QObject::connect( engine,SIGNAL( renderFinished(int) ),_imp->infoWidget[textureIndex],SLOT( hideFps() ) );
    } else {
        QObject::disconnect( engine, SIGNAL( fpsChanged(double,double) ), _imp->infoWidget[textureIndex],
                            SLOT( setFps(double,double) ) );
        QObject::disconnect( engine,SIGNAL( renderFinished(int) ),_imp->infoWidget[textureIndex],SLOT( hideFps() ) );
    }
}

void
ViewerTab::setImageFormat(int textureIndex,const Natron::ImageComponents& components,Natron::ImageBitDepthEnum depth)
{
    _imp->infoWidget[textureIndex]->setImageFormat(components,depth);
}

void
ViewerTab::onFrameRangeEditingFinished()
{
    QString text = _imp->frameRangeEdit->text();
    ///try to parse the frame range, if failed set it back to what the timeline currently is
    int i = 0;
    QString firstStr;

    while ( i < text.size() && text.at(i).isDigit() ) {
        firstStr.push_back( text.at(i) );
        ++i;
    }

    ///advance the marker to the second digit if any
    while ( i < text.size() && !text.at(i).isDigit() ) {
        ++i;
    }
    
    int curLeft,curRight;
    getTimelineBounds(&curLeft, &curRight);

    bool ok;
    int first = firstStr.toInt(&ok);
    if (!ok) {
        QString text = QString("%1 - %2").arg( curLeft ).arg( curRight );
        _imp->frameRangeEdit->setText(text);
        _imp->frameRangeEdit->adjustSize();

        return;
    }

    if ( i == text.size() ) {
        ///there's no second marker, set the timeline's boundaries to be the same frame
        setTimelineBounds(first, first);
    } else {
        QString secondStr;
        while ( i < text.size() && text.at(i).isDigit() ) {
            secondStr.push_back( text.at(i) );
            ++i;
        }
        int second = secondStr.toInt(&ok);
        if (!ok) {
            ///there's no second marker, set the timeline's boundaries to be the same frame
            setTimelineBounds(first, first);
        } else {
            setTimelineBounds(first, second);
        }
    }
    _imp->frameRangeEdit->adjustSize();
}


void
ViewerTab::onCanSetFPSLabelClicked(bool toggled)
{
    _imp->canEditFpsBox->setChecked(toggled);
    onCanSetFPSClicked(toggled);
}

void
ViewerTab::onCanSetFPSClicked(bool toggled)
{
    _imp->fpsBox->setReadOnly(!toggled);
    {
        QMutexLocker l(&_imp->fpsLockedMutex);
        _imp->fpsLocked = !toggled;
    }

}

void
ViewerTab::setFPSLocked(bool fpsLocked)
{
    _imp->canEditFpsBox->setChecked(!fpsLocked);
    onCanSetFPSClicked(!fpsLocked);
}

void
ViewerTab::onTimelineBoundariesChanged(SequenceTime first,
                                       SequenceTime second)
{
    QString text = QString("%1 - %2").arg(first).arg(second);

    _imp->frameRangeEdit->setText(text);
    _imp->frameRangeEdit->adjustSize();
}


bool
ViewerTab::isFPSLocked() const
{
    QMutexLocker k(&_imp->fpsLockedMutex);
    
    return _imp->fpsLocked;
}

void
ViewerTab::connectToViewerCache()
{
    _imp->timeLineGui->connectSlotsToViewerCache();
}

void
ViewerTab::disconnectFromViewerCache()
{
    _imp->timeLineGui->disconnectSlotsFromViewerCache();
}

void
ViewerTab::clearTimelineCacheLine()
{
    if (_imp->timeLineGui) {
        _imp->timeLineGui->clearCachedFrames();
    }
}

void
ViewerTab::toggleInfobarVisbility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->infobarVisible;
    }
    setInfobarVisible(visible);
}

void
ViewerTab::togglePlayerVisibility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->playerVisible;
    }
    setPlayerVisible(visible);
}

void
ViewerTab::toggleTimelineVisibility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->timelineVisible;
    }
    setTimelineVisible(visible);
}

void
ViewerTab::toggleLeftToolbarVisiblity()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->leftToolbarVisible;
    }
    setLeftToolbarVisible(visible);
}

void
ViewerTab::toggleRightToolbarVisibility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible =  !_imp->rightToolbarVisible;
    }
    setRightToolbarVisible(visible);
}

void
ViewerTab::toggleTopToolbarVisibility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->topToolbarVisible;
    }
    setTopToolbarVisible(visible);
}

void
ViewerTab::setLeftToolbarVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->leftToolbarVisible = visible;
    if (_imp->currentRoto.second) {
        _imp->currentRoto.second->getToolBar()->setVisible(_imp->leftToolbarVisible);
    }
}

void
ViewerTab::setRightToolbarVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->rightToolbarVisible = visible;
}

void
ViewerTab::setTopToolbarVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->topToolbarVisible = visible;
    _imp->firstSettingsRow->setVisible(_imp->topToolbarVisible);
    _imp->secondSettingsRow->setVisible(_imp->topToolbarVisible);
    if (_imp->currentRoto.second) {
        _imp->currentRoto.second->getCurrentButtonsBar()->setVisible(_imp->topToolbarVisible);
    }
    if (_imp->currentTracker.second) {
        QWidget* buttonsBar = _imp->currentTracker.second->getButtonsBar();
        assert(buttonsBar);
        if (buttonsBar) {
            buttonsBar->setVisible(_imp->topToolbarVisible);
        }
    }
}

void
ViewerTab::setPlayerVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->playerVisible = visible;
    _imp->playerButtonsContainer->setVisible(_imp->playerVisible);

}

void
ViewerTab::setTimelineVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->timelineVisible = visible;
    _imp->timeLineGui->setVisible(_imp->timelineVisible);

}

void
ViewerTab::setInfobarVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->infobarVisible = visible;
    for (int i = 0; i < 2; ++i) {
        if (i == 1) {
            int inputIndex = -1;
            
            for (ViewerTabPrivate::InputNamesMap::iterator it = _imp->inputNamesMap.begin(); it != _imp->inputNamesMap.end(); ++it) {
                if (it->second.name == _imp->secondInputImage->getCurrentIndexText()) {
                    inputIndex = it->first;
                    break;
                }
            }
            if (getCompositingOperator() == eViewerCompositingOperatorNone || inputIndex == -1) {
                continue;
            }
        }
        
        _imp->infoWidget[i]->setVisible(_imp->infobarVisible);
    }

}

void
ViewerTab::showAllToolbars()
{
    if (!isTopToolbarVisible()) {
        toggleTopToolbarVisibility();
    }
    if (!isRightToolbarVisible()) {
        toggleRightToolbarVisibility();
    }
    if (!isLeftToolbarVisible()) {
        toggleLeftToolbarVisiblity();
    }
    if (!isInfobarVisible()) {
        toggleInfobarVisbility();
    }
    if (!isPlayerVisible()) {
        togglePlayerVisibility();
    }
    if (!isTimelineVisible()) {
        toggleTimelineVisibility();
    }
}

void
ViewerTab::hideAllToolbars()
{
    if (isTopToolbarVisible()) {
        toggleTopToolbarVisibility();
    }
    if (isRightToolbarVisible()) {
        toggleRightToolbarVisibility();
    }
    if (isLeftToolbarVisible()) {
        toggleLeftToolbarVisiblity();
    }
    if (isInfobarVisible()) {
        toggleInfobarVisbility();
    }
    if (isPlayerVisible()) {
        togglePlayerVisibility();
    }
    if (isTimelineVisible()) {
        toggleTimelineVisibility();
    }
}

bool
ViewerTab::isInfobarVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->infobarVisible;
}

bool
ViewerTab::isTopToolbarVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->topToolbarVisible;
}

bool
ViewerTab::isPlayerVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->playerVisible;
}

bool
ViewerTab::isTimelineVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->timelineVisible;
}

bool
ViewerTab::isLeftToolbarVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->leftToolbarVisible;
}

bool
ViewerTab::isRightToolbarVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->rightToolbarVisible;
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
ViewerTab::setCustomTimeline(const boost::shared_ptr<TimeLine>& timeline)
{
    _imp->timeLineGui->setTimeline(timeline);
    manageTimelineSlot(true,timeline);
}

void
ViewerTab::manageTimelineSlot(bool disconnectPrevious,const boost::shared_ptr<TimeLine>& timeline)
{
    if (disconnectPrevious) {
        boost::shared_ptr<TimeLine> previous = _imp->timeLineGui->getTimeline();
        QObject::disconnect( _imp->nextKeyFrame_Button,SIGNAL( clicked(bool) ),previous.get(), SLOT( goToNextKeyframe() ) );
        QObject::disconnect( _imp->previousKeyFrame_Button,SIGNAL( clicked(bool) ),previous.get(), SLOT( goToPreviousKeyframe() ) );
        QObject::disconnect( previous.get(),SIGNAL( frameChanged(SequenceTime,int) ),
                         this, SLOT( onTimeLineTimeChanged(SequenceTime,int) ) );
        

    }
    
    QObject::connect( _imp->nextKeyFrame_Button,SIGNAL( clicked(bool) ),timeline.get(), SLOT( goToNextKeyframe() ) );
    QObject::connect( _imp->previousKeyFrame_Button,SIGNAL( clicked(bool) ),timeline.get(), SLOT( goToPreviousKeyframe() ) );
    QObject::connect( timeline.get(),SIGNAL( frameChanged(SequenceTime,int) ),
                     this, SLOT( onTimeLineTimeChanged(SequenceTime,int) ) );


}

boost::shared_ptr<TimeLine>
ViewerTab::getTimeLine() const
{
    return _imp->timeLineGui->getTimeline();
}

void
ViewerTab::onVideoEngineStopped()
{
    ///Refresh knobs
    if (getGui() && getGui()->isGUIFrozen()) {
        NodeGraph* graph = getGui()->getNodeGraph();
        if (graph && _imp->timeLineGui) {
            boost::shared_ptr<TimeLine> timeline = _imp->timeLineGui->getTimeline();
            graph->refreshNodesKnobsAtTime(timeline->currentFrame());
        }
    }
}

bool
ViewerTab::isPickerEnabled() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->pickerButton->isChecked();
}

void
ViewerTab::setPickerEnabled(bool enabled)
{
    _imp->pickerButton->setChecked(enabled);
    _imp->pickerButton->setDown(enabled);
    setInfobarVisible(true);
}

void
ViewerTab::onMousePressCalledInViewer()
{
    takeClickFocus();
}

void
ViewerTab::onPickerButtonClicked(bool clicked)
{
    const std::list<ViewerTab*> &viewers = getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it!=viewers.end(); ++it) {
        if ((*it) != this) {
            (*it)->onPickerButtonClicked(clicked);
        }
    }

    
    _imp->pickerButton->setDown(clicked);

    setInfobarVisible(clicked);
    getGui()->clearColorPickers();
}

void
ViewerTab::onCheckerboardButtonClicked()
{
    {
        QMutexLocker l(&_imp->checkerboardMutex);
        _imp->checkerboardEnabled = !_imp->checkerboardEnabled;
    }
    _imp->checkerboardButton->setDown(_imp->checkerboardEnabled);
    _imp->viewer->redraw();
}

bool ViewerTab::isCheckerboardEnabled() const
{
    QMutexLocker l(&_imp->checkerboardMutex);
    return _imp->checkerboardEnabled;
}

void ViewerTab::setCheckerboardEnabled(bool enabled)
{
    {
        QMutexLocker l(&_imp->checkerboardMutex);
        _imp->checkerboardEnabled = enabled;
    }
    _imp->checkerboardButton->setDown(enabled);
    _imp->checkerboardButton->setChecked(enabled);

}

void
ViewerTab::onSpinboxFpsChanged(double fps)
{
    _imp->viewerNode->getRenderEngine()->setDesiredFPS(fps);
    QMutexLocker k(&_imp->fpsMutex);
    _imp->fps = fps;
}

double
ViewerTab::getDesiredFps() const
{
    QMutexLocker l(&_imp->fpsMutex);
    return _imp->fps;
}

void
ViewerTab::setDesiredFps(double fps)
{
    {
        QMutexLocker l(&_imp->fpsMutex);
        _imp->fps = fps;
    }
    _imp->fpsBox->setValue(fps);
    _imp->viewerNode->getRenderEngine()->setDesiredFPS(fps);
}

void
ViewerTab::setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio)
{
    
    _imp->viewer->setProjection(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
    QString str = QString::number(std::floor(zoomFactor * 100 + 0.5));
    str.append( QChar('%') );
    str.prepend("  ");
    str.append("  ");
    _imp->zoomCombobox->setCurrentText_no_emit(str);
}

void
ViewerTab::onViewerRenderingStarted()
{
    
    if (!_imp->ongoingRenderCount) {
        _imp->refreshButton->setIcon(_imp->iconRefreshOn);
    }
    ++_imp->ongoingRenderCount;
}

void
ViewerTab::onViewerRenderingStopped()
{
    --_imp->ongoingRenderCount;
    if (!_imp->ongoingRenderCount) {
        _imp->refreshButton->setIcon(_imp->iconRefreshOff);
    }
}

void
ViewerTab::setTurboButtonDown(bool down)
{
    _imp->turboButton->setDown(down);
    _imp->turboButton->setChecked(down);
}

void 
ViewerTab::redrawGLWidgets()
{
	_imp->viewer->updateGL();
	_imp->timeLineGui->updateGL();
}

void
ViewerTab::getTimelineBounds(int* left,int* right) const
{
    return _imp->timeLineGui->getBounds(left, right);
}

void
ViewerTab::setTimelineBounds(int left,int right)
{
    _imp->timeLineGui->setBoundaries(left, right);
}

void ViewerTab::centerOn(SequenceTime left, SequenceTime right)
{
    _imp->timeLineGui->centerOn(left, right);
}

void ViewerTab::centerOn_tripleSync(SequenceTime left, SequenceTime right)
{
    _imp->timeLineGui->centerOn_tripleSync(left, right);
}

void
ViewerTab::setFrameRangeEdited(bool edited)
{
    _imp->timeLineGui->setFrameRangeEdited(edited);
}


void
ViewerTab::setFrameRange(int left,int right)
{
    setTimelineBounds(left, right);
    onTimelineBoundariesChanged(left, right);
    _imp->timeLineGui->recenterOnBounds();
}

void
ViewerTab::onInternalNodeLabelChanged(const QString& name)
{
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget() );
    if (parent) {
        setLabel(name.toStdString());
        parent->setTabLabel(this, name);
    }
}

void
ViewerTab::onInternalNodeScriptNameChanged(const QString& /*name*/)
{
    // always running in the main thread
    std::string newName = _imp->viewerNode->getNode()->getFullyQualifiedName();
    std::string oldName = getScriptName();
    
    for (std::size_t i = 0; i < newName.size(); ++i) {
        if (newName[i] == '.') {
            newName[i] = '_';
        }
    }

    
    assert( qApp && qApp->thread() == QThread::currentThread() );
    getGui()->unregisterTab(this);
    setScriptName(newName);
    getGui()->registerTab(this,this);
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget() );
    if (parent) {
        parent->onTabScriptNameChanged(this, oldName, newName);
    }
}

void
ViewerTab::refreshLayerAndAlphaChannelComboBox()
{
    if (!_imp->viewerNode) {
        return;
    }
    
    QString layerCurChoice = _imp->layerChoice->getCurrentIndexText();
    QString alphaCurChoice = _imp->alphaChannelChoice->getCurrentIndexText();
    
    std::set<ImageComponents> components;
    _imp->getComponentsAvailabel(&components);
    
    _imp->layerChoice->clear();
    _imp->alphaChannelChoice->clear();
    
    _imp->layerChoice->addItem("-");
    _imp->alphaChannelChoice->addItem("-");
    
    std::set<ImageComponents>::iterator foundColorIt = components.end();
    std::set<ImageComponents>::iterator foundOtherIt = components.end();
    std::set<ImageComponents>::iterator foundCurIt = components.end();
    std::set<ImageComponents>::iterator foundCurAlphaIt = components.end();
    std::string foundAlphaChannel;
    
    for (std::set<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
        QString layerName(it->getLayerName().c_str());
        QString itemName = layerName + '.' + QString(it->getComponentsGlobalName().c_str());
        _imp->layerChoice->addItem(itemName);
        
        if (itemName == layerCurChoice) {
            foundCurIt = it;
        }
        
        if (layerName == kNatronColorPlaneName) {
            foundColorIt = it;
        } else {
            foundOtherIt = it;
        }
        
        const std::vector<std::string>& channels = it->getComponentsNames();
        for (U32 i = 0; i < channels.size(); ++i) {
            QString itemName = layerName + '.' + QString(channels[i].c_str());
            if (itemName == alphaCurChoice) {
                foundCurAlphaIt = it;
                foundAlphaChannel = channels[i];
            }
            _imp->alphaChannelChoice->addItem(itemName);
        }
        
        if (layerName == kNatronColorPlaneName) {
            //There's RGBA or alpha, set it to A
            std::string alphaChoice;
            if (channels.size() == 4) {
                alphaChoice = channels[3];
            } else if (channels.size() == 1) {
                alphaChoice = channels[0];
            }
            if (!alphaChoice.empty()) {
                _imp->alphaChannelChoice->setCurrentIndex_no_emit(_imp->alphaChannelChoice->count() - 1);
            } else {
                alphaCurChoice = _imp->alphaChannelChoice->itemText(0);
            }
            
            _imp->viewerNode->setAlphaChannel(*it, alphaChoice, false);
        }
        
        
    }
    
    int layerIdx;
    if (foundCurIt == components.end()) {
        layerCurChoice = "-";
    }

    
    
    if (layerCurChoice == "-") {
        
        ///Try to find color plane, otherwise fallback on any other layer
        if (foundColorIt != components.end()) {
            layerCurChoice = QString(foundColorIt->getLayerName().c_str())
            + '.' + QString(foundColorIt->getComponentsGlobalName().c_str());
            foundCurIt = foundColorIt;
            
        } else if (foundOtherIt != components.end()) {
            layerCurChoice = QString(foundOtherIt->getLayerName().c_str())
            + '.' + QString(foundOtherIt->getComponentsGlobalName().c_str());
            foundCurIt = foundOtherIt;
        } else {
            layerCurChoice = "-";
            foundCurIt = components.end();
        }
        
        
    }
    layerIdx = _imp->layerChoice->itemIndex(layerCurChoice);
    assert(layerIdx != -1);

    
    
    _imp->layerChoice->setCurrentIndex_no_emit(layerIdx);
    if (foundCurIt == components.end()) {
        _imp->viewerNode->setActiveLayer(ImageComponents::getNoneComponents(), false);
    } else {
        if (foundCurIt->getNumComponents() == 1) {
            _imp->viewerChannels->setCurrentIndex_no_emit(5);
            setDisplayChannels(5, true);
        }
        _imp->viewerNode->setActiveLayer(*foundCurIt, false);
    }
    
    int alphaIdx;
    if (foundCurAlphaIt == components.end() || foundAlphaChannel.empty()) {
        alphaCurChoice = "-";
    }
    
    if (alphaCurChoice == "-") {
        
        ///Try to find color plane, otherwise fallback on any other layer
        if (foundColorIt != components.end() &&
            (foundColorIt->getComponentsNames().size() == 4 || foundColorIt->getComponentsNames().size() == 1)) {
            
            std::size_t lastComp = foundColorIt->getComponentsNames().size() -1;
            
            alphaCurChoice = QString(foundColorIt->getLayerName().c_str())
            + '.' + QString(foundColorIt->getComponentsNames()[lastComp].c_str());
            foundAlphaChannel = foundColorIt->getComponentsNames()[lastComp];
            foundCurAlphaIt = foundColorIt;
            
            
        } else {
            alphaCurChoice = "-";
            foundCurAlphaIt = components.end();
        }
        
        
    }
    
    alphaIdx = _imp->alphaChannelChoice->itemIndex(alphaCurChoice);
    if (alphaIdx == -1) {
        alphaIdx = 0;
    }
    
    _imp->alphaChannelChoice->setCurrentIndex_no_emit(alphaIdx);
    if (foundCurAlphaIt != components.end()) {
        _imp->viewerNode->setAlphaChannel(*foundCurAlphaIt, foundAlphaChannel, false);
    } else {
        _imp->viewerNode->setAlphaChannel(ImageComponents::getNoneComponents(), std::string(), false);
    }
    
}

void
ViewerTab::onAlphaChannelComboChanged(int index)
{
    std::set<ImageComponents> components;
    _imp->getComponentsAvailabel(&components);
    int i = 1; // because of the "-" choice
    for (std::set<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
        
        const std::vector<std::string>& channels = it->getComponentsNames();
        if (index >= ((int)channels.size() + i)) {
            i += channels.size();
        } else {
            for (U32 j = 0; j < channels.size(); ++j, ++i) {
                if (i == index) {
                    _imp->viewerNode->setAlphaChannel(*it, channels[j], true);
                    return;
                }
            }
            
        }
    }
    _imp->viewerNode->setAlphaChannel(ImageComponents::getNoneComponents(), std::string(), true);
}

void
ViewerTab::onLayerComboChanged(int index)
{
    std::set<ImageComponents> components;
    _imp->getComponentsAvailabel(&components);
    if (index >= (int)(components.size() + 1) || index < 0) {
        qDebug() << "ViewerTab::onLayerComboChanged: invalid index";
        return;
    }
    int i = 1; // because of the "-" choice
    int chanCount = 1; // because of the "-" choice
    for (std::set<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it, ++i) {
        
        chanCount += it->getComponentsNames().size();
        if (i == index) {
            _imp->viewerNode->setActiveLayer(*it, true);
            
            ///If it has an alpha channel, set it
            if (it->getComponentsNames().size() == 4) {
                _imp->alphaChannelChoice->setCurrentIndex_no_emit(chanCount - 1);
                _imp->viewerNode->setAlphaChannel(*it, it->getComponentsNames()[3], true);
            }
            return;
        }
    }
    
    _imp->alphaChannelChoice->setCurrentIndex_no_emit(0);
    _imp->viewerNode->setAlphaChannel(ImageComponents::getNoneComponents(), std::string(), false);
    _imp->viewerNode->setActiveLayer(ImageComponents::getNoneComponents(), true);
    
}

void
ViewerTab::onGainToggled(bool clicked)
{
    double value;
    if (clicked) {
        value = _imp->lastFstopValue;
    } else {
        value = 0;
    }
    _imp->toggleGainButton->setDown(clicked);
    _imp->gainBox->setValue(value);
    _imp->gainSlider->seekScalePosition(value);
    
    double gain = std::pow(2,value);
    _imp->viewer->setGain(gain);
    _imp->viewerNode->onGainChanged(gain);
}


void
ViewerTab::onGainSliderChanged(double v)
{
    if (!_imp->toggleGainButton->isChecked()) {
        _imp->toggleGainButton->setChecked(true);
        _imp->toggleGainButton->setDown(true);
    }
    _imp->gainBox->setValue(v);
    double gain = std::pow(2,v);
    _imp->viewer->setGain(gain);
    _imp->viewerNode->onGainChanged(gain);
    _imp->lastFstopValue = v;
}

void
ViewerTab::onGainSpinBoxValueChanged(double v)
{
    if (!_imp->toggleGainButton->isChecked()) {
        _imp->toggleGainButton->setChecked(true);
        _imp->toggleGainButton->setDown(true);
    }
    _imp->gainSlider->seekScalePosition(v);
    double gain = std::pow(2,v);
    _imp->viewer->setGain(gain);
    _imp->viewerNode->onGainChanged(gain);
    _imp->lastFstopValue = v;
}

void
ViewerTab::onGammaToggled(bool clicked)
{
    double value;
    if (clicked) {
        value = _imp->lastGammaValue;
    } else {
        value = 1.;
    }
    _imp->toggleGammaButton->setDown(clicked);
    _imp->gammaBox->setValue(value);
    _imp->gammaSlider->seekScalePosition(value);
    _imp->viewer->setGamma(value);
    _imp->viewerNode->onGammaChanged(value);
}

void
ViewerTab::onGammaSliderValueChanged(double value)
{
    if (!_imp->toggleGammaButton->isChecked()) {
        _imp->toggleGammaButton->setChecked(true);
        _imp->toggleGammaButton->setDown(true);
    }
    _imp->gammaBox->setValue(value);
    _imp->viewer->setGamma(value);
    _imp->viewerNode->onGammaChanged(value);
    _imp->lastGammaValue = value;
}

void
ViewerTab::onGammaSpinBoxValueChanged(double value)
{
    if (!_imp->toggleGammaButton->isChecked()) {
        _imp->toggleGammaButton->setChecked(true);
        _imp->toggleGammaButton->setDown(true);
    }
    _imp->gammaSlider->seekScalePosition(value);
    _imp->viewer->setGamma(value);
    _imp->viewerNode->onGammaChanged(value);
    _imp->lastGammaValue = value;
}

void
ViewerTab::onGammaSliderEditingFinished(bool hasMovedOnce)
{
    bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
    if (autoProxyEnabled && hasMovedOnce) {
        getGui()->renderAllViewers();
    }
}

void
ViewerTab::onGainSliderEditingFinished(bool hasMovedOnce)
{
    bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
    if (autoProxyEnabled && hasMovedOnce) {
        getGui()->renderAllViewers();
    }
}

bool
ViewerTab::isViewersSynchroEnabled() const
{
    return _imp->syncViewerButton->isDown();
}

void
ViewerTab::synchronizeOtherViewersProjection()
{
    assert(getGui());
    getGui()->setMasterSyncViewer(this);
    double left,bottom,factor,par;
    _imp->viewer->getProjection(&left, &bottom, &factor, &par);
    const std::list<ViewerTab*>& viewers = getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        if ((*it) != this) {
            (*it)->getViewer()->setProjection(left, bottom, factor, par);
            (*it)->getInternalNode()->renderCurrentFrame(false);
            
        }
    }
    
}

void
ViewerTab::onSyncViewersButtonPressed(bool clicked)
{

    const std::list<ViewerTab*>& viewers = getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->_imp->syncViewerButton->setDown(clicked);
        (*it)->_imp->syncViewerButton->setChecked(clicked);
    }
    if (clicked) {
        synchronizeOtherViewersProjection();
    }
}

void
ViewerTab::zoomIn()
{
    double factor = _imp->viewer->getZoomFactor();
    factor *= 1.1;
    factor *= 100;
    _imp->viewer->zoomSlot(factor);
    QString text = QString::number(std::floor(factor + 0.5));
    _imp->zoomCombobox->setCurrentText_no_emit(text);
}

void
ViewerTab::zoomOut()
{
    double factor = _imp->viewer->getZoomFactor();
    factor *= 0.9;
    factor *= 100;
    _imp->viewer->zoomSlot(factor);
    QString text = QString::number(std::floor(factor + 0.5)) + "%";
    _imp->zoomCombobox->setCurrentText_no_emit(text);
}

void
ViewerTab::onZoomComboboxCurrentIndexChanged(int /*index*/)
{
    QString text = _imp->zoomCombobox->getCurrentIndexText();
    if (text == "+") {
        zoomIn();
    } else if (text == "-") {
        zoomOut();
    } else if (text == "Fit") {
        centerViewer();
    } else {
        text.remove( QChar('%') );
        int v = text.toInt();
        assert(v > 0);
        _imp->viewer->zoomSlot(v);
    }
}

void
ViewerTab::onRenderStatsAvailable(int time, int view, double wallTime, const RenderStatsMap& stats)
{
    assert(QThread::currentThread() == qApp->thread());
    RenderStatsDialog* dialog = getGui()->getRenderStatsDialog();
    if (dialog) {
        dialog->addStats(time, view, wallTime, stats);
    }
}

void
ViewerTab::toggleTripleSync(bool toggled)
{
    
    _imp->tripleSyncButton->setDown(toggled);
    getGui()->setTripleSyncEnabled(toggled);
    if (toggled) {
        DopeSheetEditor* deditor = getGui()->getDopeSheetEditor();
        CurveEditor* cEditor = getGui()->getCurveEditor();
        //Sync curve editor and dopesheet tree width
        cEditor->setTreeWidgetWidth(deditor->getTreeWidgetWidth());
        
        SequenceTime left,right;
        _imp->timeLineGui->getVisibleRange(&left, &right);
        getGui()->centerOpenedViewersOn(left, right);
        deditor->centerOn(left, right);
        cEditor->getCurveWidget()->centerOn(left, right);
    }

}
