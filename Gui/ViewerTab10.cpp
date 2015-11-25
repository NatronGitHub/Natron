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
#include <stdexcept>
#include <QtCore/QDebug>

#include <QApplication>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/GuiMacros.h"
#include "Gui/Button.h"
#include "Gui/ChannelsComboBox.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/RotoGui.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/TrackerGui.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"


using namespace Natron;


void
ViewerTab::onColorSpaceComboBoxChanged(int v)
{
    Natron::ViewerColorSpaceEnum colorspace = Natron::eViewerColorSpaceSRGB;

    if (v == 0) {
        colorspace = Natron::eViewerColorSpaceLinear;
    } else if (v == 1) {
        colorspace = Natron::eViewerColorSpaceSRGB;
    } else if (v == 2) {
        colorspace = Natron::eViewerColorSpaceRec709;
    } else {
        assert(false);
        throw std::logic_error("ViewerTab::onColorSpaceComboBoxChanged(): unknown colorspace");
    }
    _imp->viewer->setLut( (int)colorspace );
    _imp->viewerNode->onColorSpaceChanged(colorspace);
}

void
ViewerTab::onEnableViewerRoIButtonToggle(bool b)
{
    _imp->enableViewerRoI->setDown(b);
    _imp->enableViewerRoI->setChecked(b);
    _imp->viewer->setUserRoIEnabled(b);
}

void
ViewerTab::onCreateNewRoIPressed()
{
    _imp->enableViewerRoI->setDown(true);
    _imp->enableViewerRoI->setChecked(true);
    _imp->viewer->setBuildNewUserRoI(true);
    _imp->viewer->setUserRoIEnabled(true);
}

void
ViewerTab::updateViewsMenu(const std::vector<std::string>& viewNames)
{
    int currentIndex = _imp->viewsComboBox->activeIndex();

    _imp->viewsComboBox->clear();
    for (std::size_t i = 0;  i < viewNames.size(); ++i) {
        if (viewNames[i] == "Left") {
            ActionWithShortcut* leftAct = new ActionWithShortcut(kShortcutGroupViewer,
                                                                 kShortcutIDShowLeftView,
                                                                 kShortcutDescShowLeftView,
                                                                 _imp->viewsComboBox);
            _imp->viewsComboBox->addAction(leftAct);

        } else if (viewNames[i] == "Right") {
            ActionWithShortcut* rightAct = new ActionWithShortcut(kShortcutGroupViewer,
                                                                  kShortcutIDShowRightView,
                                                                  kShortcutDescShowRightView,
                                                                  _imp->viewsComboBox);
            _imp->viewsComboBox->addAction(rightAct);
        } else {
            _imp->viewsComboBox->addItem(viewNames[i].c_str());
        }
    }
    if (viewNames.size() == 1) {
        _imp->viewsComboBox->hide();
    } else {
        _imp->viewsComboBox->show();
    }
    if ( ( currentIndex < _imp->viewsComboBox->count() ) && (currentIndex != -1) ) {
        _imp->viewsComboBox->setCurrentIndex(currentIndex);
    } else {
        _imp->viewsComboBox->setCurrentIndex(0);
    }
    getGui()->updateViewsActions(viewNames.size());
}

void
ViewerTab::setCurrentView(int view)
{
    _imp->viewsComboBox->setCurrentIndex(view);
}

int
ViewerTab::getCurrentView() const
{
    QMutexLocker l(&_imp->currentViewMutex);

    return _imp->currentViewIndex;
}

void
ViewerTab::setPlaybackMode(Natron::PlaybackModeEnum mode)
{
    QPixmap pix;
    switch (mode) {
        case Natron::ePlaybackModeLoop:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE, &pix);
            break;
        case Natron::ePlaybackModeBounce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_BOUNCE, &pix);
            break;
        case Natron::ePlaybackModeOnce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_ONCE, &pix);
            break;
        default:
            break;
    }
    {
        QMutexLocker k(&_imp->playbackModeMutex);
        _imp->playbackMode = mode;
    }
    _imp->playbackMode_Button->setIcon(QIcon(pix));
    _imp->viewerNode->getRenderEngine()->setPlaybackMode(mode);

}

Natron::PlaybackModeEnum
ViewerTab::getPlaybackMode() const
{
    QMutexLocker k(&_imp->playbackModeMutex);
    return _imp->playbackMode;
}

void
ViewerTab::togglePlaybackMode()
{
    Natron::PlaybackModeEnum mode = _imp->viewerNode->getRenderEngine()->getPlaybackMode();
    mode = (Natron::PlaybackModeEnum)(((int)mode + 1) % 3);
    QPixmap pix;
    switch (mode) {
        case Natron::ePlaybackModeLoop:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE, &pix);
            break;
        case Natron::ePlaybackModeBounce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_BOUNCE, &pix);
            break;
        case Natron::ePlaybackModeOnce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_ONCE, &pix);
            break;
        default:
            break;
    }
    {
        QMutexLocker k(&_imp->playbackModeMutex);
        _imp->playbackMode = mode;
    }
    _imp->playbackMode_Button->setIcon(QIcon(pix));
    _imp->viewerNode->getRenderEngine()->setPlaybackMode(mode);
}

void
ViewerTab::onClipToProjectButtonToggle(bool b)
{
    _imp->clipToProjectFormatButton->setDown(b);
    _imp->viewer->setClipToDisplayWindow(b);
}


void
ViewerTab::updateZoomComboBox(int value)
{
    assert(value > 0);
    QString str = QString::number(value);
    str.append( QChar('%') );
    str.prepend("  ");
    str.append("  ");
    _imp->zoomCombobox->setCurrentText_no_emit(str);
}

void
ViewerTab::toggleStartForward()
{
    startPause( !_imp->play_Forward_Button->isDown() );
}

void
ViewerTab::toggleStartBackward()
{
    startBackward( !_imp->play_Backward_Button->isDown() );
}

/*In case they're several viewer around, we need to reset the dag and tell it
   explicitly we want to use this viewer and not another one.*/
void
ViewerTab::startPause(bool b)
{
    abortRendering();
    if (b) {
        getGui()->getApp()->setLastViewerUsingTimeline(_imp->viewerNode->getNode());
        std::vector<int> viewsToRender;
        {
            QMutexLocker k(&_imp->currentViewMutex);
            viewsToRender.push_back(_imp->currentViewIndex);
        }
        _imp->viewerNode->getRenderEngine()->renderFromCurrentFrame(getGui()->getApp()->isRenderStatsActionChecked(), viewsToRender, OutputSchedulerThread::eRenderDirectionForward);
    }
}

void
ViewerTab::abortRendering()
{
    if (_imp->play_Forward_Button) {
        _imp->play_Forward_Button->setDown(false);
        _imp->play_Forward_Button->setChecked(false);
    }
    if (_imp->play_Backward_Button) {
        _imp->play_Backward_Button->setDown(false);
        _imp->play_Backward_Button->setChecked(false);
    }
    if (getGui() && getGui()->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        getGui()->onFreezeUIButtonClicked(false);
    }
    if (getGui()) {
        ///Abort all viewers because they are all synchronised.
        const std::list<ViewerTab*> & activeNodes = getGui()->getViewersList();

        for (std::list<ViewerTab*>::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
            ViewerInstance* viewer = (*it)->getInternalNode();
            if (viewer) {
                viewer->getRenderEngine()->abortRendering(false,true);
            }
        }
    }
}

void
ViewerTab::onEngineStarted(bool forward)
{
    if (!getGui()) {
        return;
    }
    
    if (_imp->play_Forward_Button) {
        _imp->play_Forward_Button->setDown(forward);
        _imp->play_Forward_Button->setChecked(forward);
    }

    if (_imp->play_Backward_Button) {
        _imp->play_Backward_Button->setDown(!forward);
        _imp->play_Backward_Button->setChecked(!forward);
    }

    if (getGui() && !getGui()->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        getGui()->onFreezeUIButtonClicked(true);
    }
}

void
ViewerTab::onEngineStopped()
{
    if (!getGui()) {
        return;
    }
    
    if (_imp->play_Forward_Button && _imp->play_Forward_Button->isDown()) {
        _imp->play_Forward_Button->setDown(false);
        _imp->play_Forward_Button->setChecked(false);
    }
    
    if (_imp->play_Backward_Button && _imp->play_Backward_Button->isDown()) {
        _imp->play_Backward_Button->setDown(false);
        _imp->play_Backward_Button->setChecked(false);
    }
    if (getGui() && getGui()->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        getGui()->onFreezeUIButtonClicked(false);
    }
}

void
ViewerTab::startBackward(bool b)
{
    abortRendering();
    if (b) {
        getGui()->getApp()->setLastViewerUsingTimeline(_imp->viewerNode->getNode());
        std::vector<int> viewsToRender;
        {
            QMutexLocker k(&_imp->currentViewMutex);
            viewsToRender.push_back(_imp->currentViewIndex);
        }
        _imp->viewerNode->getRenderEngine()->renderFromCurrentFrame(getGui()->getApp()->isRenderStatsActionChecked(), viewsToRender, OutputSchedulerThread::eRenderDirectionBackward);

    }
}

void
ViewerTab::seek(SequenceTime time)
{
    _imp->currentFrameBox->setValue(time);
    _imp->timeLineGui->seek(time);
    
}

void
ViewerTab::previousFrame()
{
    int prevFrame = _imp->timeLineGui->currentFrame() -1 ;
    if (prevFrame  < _imp->timeLineGui->leftBound()) {
        prevFrame = _imp->timeLineGui->rightBound();
    }
    seek(prevFrame);
}

void
ViewerTab::nextFrame()
{
    int nextFrame = _imp->timeLineGui->currentFrame() + 1;
    if (nextFrame  > _imp->timeLineGui->rightBound()) {
        nextFrame = _imp->timeLineGui->leftBound();
    }
    seek(nextFrame);
}

void
ViewerTab::previousIncrement()
{
    seek( _imp->timeLineGui->currentFrame() - _imp->incrementSpinBox->value() );
}

void
ViewerTab::nextIncrement()
{
    seek( _imp->timeLineGui->currentFrame() + _imp->incrementSpinBox->value() );
}

void
ViewerTab::firstFrame()
{
    seek( _imp->timeLineGui->leftBound() );
}

void
ViewerTab::lastFrame()
{
    seek( _imp->timeLineGui->rightBound() );
}

void
ViewerTab::onTimeLineTimeChanged(SequenceTime time,
                                 int /*reason*/)
{
    if (!getGui()) {
        return;
    }
    _imp->currentFrameBox->setValue(time);
    
    if (_imp->timeLineGui->getTimeline() != getGui()->getApp()->getTimeLine()) {
        _imp->viewerNode->renderCurrentFrame(true);
    }
}

void
ViewerTab::onCurrentTimeSpinBoxChanged(double time)
{
    _imp->timeLineGui->seek(time);
}

void
ViewerTab::centerViewer()
{
    _imp->viewer->fitImageToFormat();
    if ( _imp->viewer->displayingImage() ) {
        _imp->viewerNode->renderCurrentFrame(false);
    } else {
        _imp->viewer->update();
    }
}

void
ViewerTab::refresh(bool enableRenderStats)
{
    _imp->viewerNode->forceFullComputationOnNextFrame();
    if (!enableRenderStats) {
        _imp->viewerNode->renderCurrentFrame(false);
    } else {
        getGui()->getOrCreateRenderStatsDialog()->show();
        _imp->viewerNode->renderCurrentFrameWithRenderStats(false);
    }
}

void
ViewerTab::refresh()
{
    Qt::KeyboardModifiers m = qApp->keyboardModifiers();
    refresh(m.testFlag(Qt::ShiftModifier) && m.testFlag(Qt::ControlModifier));
}

ViewerTab::~ViewerTab()
{
    if (getGui()) {
        NodeGraph* graph = 0;
        if (_imp->viewerNode) {
            boost::shared_ptr<NodeCollection> collection = _imp->viewerNode->getNode()->getGroup();
            if (collection) {
                NodeGroup* isGrp = dynamic_cast<NodeGroup*>(collection.get());
                if (isGrp) {
                    NodeGraphI* graph_i = isGrp->getNodeGraph();
                    if (graph_i) {
                        graph = dynamic_cast<NodeGraph*>(graph_i);
                        assert(graph);
                    }
                } else {
                    graph = getGui()->getNodeGraph();
                }
            }
            _imp->viewerNode->invalidateUiContext();
        } else {
            graph = getGui()->getNodeGraph();
        }
        assert(graph);
        if ( getGui()->getApp() && !getGui()->getApp()->isClosing() && graph && (graph->getLastSelectedViewer() == this) ) {
            graph->setLastSelectedViewer(0);
        }
    }
    for (std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.begin(); it != _imp->rotoNodes.end(); ++it) {
        delete it->second;
    }
    for (std::map<NodeGui*,TrackerGui*>::iterator it = _imp->trackerNodes.begin(); it != _imp->trackerNodes.end(); ++it) {
        delete it->second;
    }
}

void
ViewerTab::nextLayer()
{
    int currentIndex = _imp->layerChoice->activeIndex();
    int nChoices = _imp->layerChoice->count();
    currentIndex = (currentIndex + 1) % nChoices;
    if (currentIndex == 0 && nChoices > 1) {
        currentIndex = 1;
    }
    _imp->layerChoice->setCurrentIndex(currentIndex);
}

void
ViewerTab::previousLayer()
{
    int currentIndex = _imp->layerChoice->activeIndex();
    int nChoices = _imp->layerChoice->count();
    if (currentIndex <= 1) {
        currentIndex = nChoices - 1;
    } else {
        --currentIndex;
    }
    if (currentIndex >= 0) {
        _imp->layerChoice->setCurrentIndex(currentIndex);
    }
}

void
ViewerTab::enterEvent(QEvent* e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void
ViewerTab::leaveEvent(QEvent* e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
ViewerTab::keyPressEvent(QKeyEvent* e)
{
    
    bool accept = true;
    
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    double scale = 1. / (1 << _imp->viewer->getCurrentRenderScale());

    if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionLuminance, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 0) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(0);
            setDisplayChannels(0, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRed, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 2) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(2);
            setDisplayChannels(2, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionGreen, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 3) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(3);
            setDisplayChannels(3, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionBlue, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 4) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(4);
            setDisplayChannels(4, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionAlpha, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 5) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(5);
            setDisplayChannels(5, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionLuminanceA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 0) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(0);
            setDisplayChannels(0, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRedA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 2) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(2);
            setDisplayChannels(2, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionGreenA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 3) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(3);
            setDisplayChannels(3, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionBlueA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 4) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(4);
            setDisplayChannels(4, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionAlphaA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 5) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(5);
            setDisplayChannels(5, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionMatteOverlay, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 6) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(6);
            setDisplayChannels(6, false);
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        previousFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward, modifiers, key) ) {
        toggleStartBackward();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerStop, modifiers, key) ) {
        abortRendering();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerForward, modifiers, key) ) {
        toggleStartForward();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        nextFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        //prev incr
        previousIncrement();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        //next incr
        nextIncrement();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        //first frame
        firstFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        //last frame
        lastFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        //prev key
        getGui()->getApp()->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        //next key
        getGui()->getApp()->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionFitViewer, modifiers, key) ) {
        centerViewer();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionClipEnabled, modifiers, key) ) {
        onClipToProjectButtonToggle( !_imp->clipToProjectFormatButton->isDown() );
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRefresh, modifiers, key) ) {
        refresh(false);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRefreshWithStats, modifiers, key) ) {
        refresh(true);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionROIEnabled, modifiers, key) ) {
        onEnableViewerRoIButtonToggle( !_imp->enableViewerRoI->isDown() );
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionNewROI, modifiers, key) ) {
        onCreateNewRoIPressed();
    }else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyEnabled, modifiers, key) ) {
        onRenderScaleButtonClicked(!_imp->renderScaleActive);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel2, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(0);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel4, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(1);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel8, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(2);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel16, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(3);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel32, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(4);
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDActionZoomLevel100, modifiers, key) ) {
        _imp->viewer->zoomSlot(100);
        _imp->zoomCombobox->setCurrentIndex_no_emit(7);
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDActionZoomIn, modifiers, key) ) {
        zoomIn();
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDActionZoomOut, modifiers, key) ) {
        zoomOut();
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDNextLayer, modifiers, key) ) {
        nextLayer();
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDPrevLayer, modifiers, key) ) {
        previousLayer();
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, modifiers, key) ) {
        connectToInput(0);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, modifiers, key) ) {
        connectToInput(1);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, modifiers, key) ) {
        connectToInput(2);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, modifiers, key) ) {
        connectToInput(3);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, modifiers, key) ) {
        connectToInput(4);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, modifiers, key) ) {
        connectToInput(5);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, modifiers, key) ) {
        connectToInput(6);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, modifiers, key) ) {
        connectToInput(7);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, modifiers, key) ) {
        connectToInput(8);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, modifiers, key) ) {
        connectToInput(9);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideOverlays, modifiers, key) ) {
        _imp->viewer->toggleOverlays();
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDToggleWipe, modifiers, key)) {
        _imp->viewer->toggleWipe();
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDCenterWipe, modifiers, key)) {
        _imp->viewer->centerWipe();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideAll, modifiers, key) ) {
        hideAllToolbars();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionShowAll, modifiers, key) ) {
        showAllToolbars();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHidePlayer, modifiers, key) ) {
        togglePlayerVisibility();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideTimeline, modifiers, key) ) {
        toggleTimelineVisibility();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideInfobar, modifiers, key) ) {
        toggleInfobarVisbility();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideLeft, modifiers, key) ) {
        toggleLeftToolbarVisiblity();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideRight, modifiers, key) ) {
        toggleRightToolbarVisibility();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionHideTop, modifiers, key) ) {
        toggleTopToolbarVisibility();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomIn, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal(QCursor::pos()), 120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomOut, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal(QCursor::pos()), -120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( e->isAutoRepeat() && notifyOverlaysKeyRepeat(scale, scale, e) ) {
        update();
    } else if ( notifyOverlaysKeyDown(scale, scale, e) ) {
        update();
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDSwitchInputAAndB, modifiers, key) ) {
        ///Put it after notifyOverlaysKeyDown() because Roto may intercept Enter
        switchInputAAndB();
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDShowLeftView, modifiers, key) ) {
        showView(0);
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDShowRightView, modifiers, key) ) {
        showView(1);
    } else {
        accept = false;
        QWidget::keyPressEvent(e);
    }
    if (accept) {
        takeClickFocus();
        e->accept();
    } else {
        handleUnCaughtKeyPressEvent(e);
    }
} // keyPressEvent



void
ViewerTab::keyReleaseEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!getGui()) {
        return QWidget::keyPressEvent(e);
    }
    double scale = 1. / (1 << _imp->viewer->getCurrentRenderScale());
    if ( notifyOverlaysKeyUp(scale, scale, e) ) {
        _imp->viewer->redraw();
    } else {
        QWidget::keyReleaseEvent(e);
    }
}

void
ViewerTab::setDisplayChannels(int i, bool setBothInputs)
{
    Natron::DisplayChannelsEnum channels;
    
    switch (i) {
        case 0:
            channels = Natron::eDisplayChannelsY;
            break;
        case 1:
            channels = Natron::eDisplayChannelsRGB;
            break;
        case 2:
            channels = Natron::eDisplayChannelsR;
            break;
        case 3:
            channels = Natron::eDisplayChannelsG;
            break;
        case 4:
            channels = Natron::eDisplayChannelsB;
            break;
        case 5:
            channels = Natron::eDisplayChannelsA;
            break;
        case 6:
            channels = Natron::eDisplayChannelsMatte;
            break;
        default:
            channels = Natron::eDisplayChannelsRGB;
            break;
    }
    _imp->viewerNode->setDisplayChannels(channels, setBothInputs);
}

void
ViewerTab::onViewerChannelsChanged(int i)
{
    setDisplayChannels(i, false);
}

bool
ViewerTab::eventFilter(QObject *target,
                       QEvent* e)
{
    if (e->type() == QEvent::MouseButtonPress) {
        if (getGui() && getGui()->getApp()) {
            boost::shared_ptr<NodeGuiI> gui_i = _imp->viewerNode->getNode()->getNodeGui();
            assert(gui_i);
            boost::shared_ptr<NodeGui> gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
            getGui()->selectNode(gui);
        }
    }

    return QWidget::eventFilter(target, e);
}

void
ViewerTab::disconnectViewer()
{
    _imp->viewer->disconnectViewer();
}

QSize
ViewerTab::minimumSizeHint() const
{
    if (!_imp->playerButtonsContainer->isVisible()) {
        return QSize(500,200);
    }
    return QWidget::minimumSizeHint();
}

QSize
ViewerTab::sizeHint() const
{
    if (!_imp->playerButtonsContainer->isVisible()) {
        return QSize(500,200);
    }

    return QWidget::sizeHint();
}

void
ViewerTab::showView(int view)
{
    {
        QMutexLocker l(&_imp->currentViewMutex);
        
        _imp->currentViewIndex = view;
    }
    abortRendering();
    _imp->viewerNode->renderCurrentFrame(true);
}
