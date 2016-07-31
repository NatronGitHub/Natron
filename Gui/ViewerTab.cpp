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

    /*VIEWER SETTINGS*/

    /*1st row of buttons*/
    QFontMetrics fm(font(), 0);


    const int pixmapIconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE);
    const QSize buttonSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    const QSize buttonIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );

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


    /*Player buttons*/
    _imp->playerButtonsContainer = new QWidget(this);
    _imp->playerLayout = new QHBoxLayout(_imp->playerButtonsContainer);
    _imp->playerLayout->setSpacing(0);
    _imp->playerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->playerButtonsContainer->setLayout(_imp->playerLayout);

    _imp->currentFrameBox = new SpinBox(_imp->playerButtonsContainer, SpinBox::eSpinBoxTypeInt);
    _imp->currentFrameBox->setValue(0);
    _imp->currentFrameBox->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    _imp->currentFrameBox->setToolTip( QString::fromUtf8("<p><b>") + tr("Current frame number") + QString::fromUtf8("</b></p>") );

    _imp->firstFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->firstFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->firstFrame_Button->setFixedSize(buttonSize);
    _imp->firstFrame_Button->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, "<p>" + tr("First frame").toStdString() + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->firstFrame_Button);


    _imp->previousKeyFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousKeyFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousKeyFrame_Button->setFixedSize(buttonSize);
    _imp->previousKeyFrame_Button->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, "<p>" + tr("Previous Keyframe").toStdString() + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->previousKeyFrame_Button);


    _imp->previousFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousFrame_Button->setFixedSize(buttonSize);
    _imp->previousFrame_Button->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, "<p>" + tr("Previous frame").toStdString() + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->previousFrame_Button);


    _imp->play_Backward_Button = new Button(_imp->playerButtonsContainer);
    _imp->play_Backward_Button->setFocusPolicy(Qt::NoFocus);
    _imp->play_Backward_Button->setFixedSize(buttonSize);
    _imp->play_Backward_Button->setIconSize(buttonIconSize);
    {
        std::list<std::string> actions;
        actions.push_back(kShortcutIDActionPlayerBackward);
        actions.push_back(kShortcutIDActionPlayerStop);
        setToolTipWithShortcut2(kShortcutGroupPlayer, actions, "<p>" + tr("Play backward").toStdString() + "</p>" +
                                "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b> " +
                                tr("(%2 to stop)").toStdString() + "</p>", _imp->play_Backward_Button);
    }
    _imp->play_Backward_Button->setCheckable(true);
    _imp->play_Backward_Button->setDown(false);

    _imp->play_Forward_Button = new Button(_imp->playerButtonsContainer);
    _imp->play_Forward_Button->setFocusPolicy(Qt::NoFocus);
    _imp->play_Forward_Button->setFixedSize(buttonSize);
    _imp->play_Forward_Button->setIconSize(buttonIconSize);
    {
        std::list<std::string> actions;
        actions.push_back(kShortcutIDActionPlayerForward);
        actions.push_back(kShortcutIDActionPlayerStop);
        setToolTipWithShortcut2(kShortcutGroupPlayer, actions, "<p>" + tr("Play forward").toStdString() + "</p>" +
                                "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b> " +
                                tr("(%2 to stop)").toStdString() + "</p>", _imp->play_Forward_Button);
    }
    _imp->play_Forward_Button->setCheckable(true);
    _imp->play_Forward_Button->setDown(false);


    _imp->nextFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->nextFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->nextFrame_Button->setFixedSize(buttonSize);
    _imp->nextFrame_Button->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, "<p>" + tr("Next frame").toStdString() + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->nextFrame_Button);


    _imp->nextKeyFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->nextKeyFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->nextKeyFrame_Button->setFixedSize(buttonSize);
    _imp->nextKeyFrame_Button->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, "<p>" + tr("Next Keyframe").toStdString() + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->nextKeyFrame_Button);


    _imp->lastFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->lastFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->lastFrame_Button->setFixedSize(buttonSize);
    _imp->lastFrame_Button->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, "<p>" + tr("Last Frame").toStdString() + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->lastFrame_Button);


    _imp->previousIncrement_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousIncrement_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousIncrement_Button->setFixedSize(buttonSize);
    _imp->previousIncrement_Button->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, "<p>" + tr("Previous Increment").toStdString() + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->previousIncrement_Button);


    _imp->incrementSpinBox = new SpinBox(_imp->playerButtonsContainer);
    _imp->incrementSpinBox->setValue(10);
    _imp->incrementSpinBox->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    _imp->incrementSpinBox->setToolTip( QString::fromUtf8("<p><b>") + tr("Frame increment:") + QString::fromUtf8("</b></p>") + tr(
                                            "The previous/next increment buttons step"
                                            " with this increment.") );


    _imp->nextIncrement_Button = new Button(_imp->playerButtonsContainer);
    _imp->nextIncrement_Button->setFocusPolicy(Qt::NoFocus);
    _imp->nextIncrement_Button->setFixedSize(buttonSize);
    _imp->nextIncrement_Button->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, "<p>" + tr("Next Increment").toStdString() + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->nextIncrement_Button);

    _imp->playBackInputButton = new Button(_imp->playerButtonsContainer);
    _imp->playBackInputButton->setFocusPolicy(Qt::NoFocus);
    _imp->playBackInputButton->setFixedSize(buttonSize);
    _imp->playBackInputButton->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerPlaybackIn, "<p>" + tr("Set the playback in point at the current frame.").toStdString() + "</p>" + "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->playBackInputButton);

    _imp->playBackOutputButton = new Button(_imp->playerButtonsContainer);
    _imp->playBackOutputButton->setFocusPolicy(Qt::NoFocus);
    _imp->playBackOutputButton->setFixedSize(buttonSize);
    _imp->playBackOutputButton->setIconSize(buttonIconSize);
    setToolTipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerPlaybackOut, "<p>" + tr("Set the playback out point at the current frame.").toStdString() + "</p>" + "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->playBackOutputButton);

    _imp->playBackInputSpinbox = new SpinBox(_imp->playerButtonsContainer);
    _imp->playBackInputSpinbox->setToolTip( tr("The playback in point") );
    _imp->playBackInputSpinbox->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    _imp->playBackOutputSpinbox = new SpinBox(_imp->playerButtonsContainer);
    _imp->playBackOutputSpinbox->setToolTip( tr("The playback out point") );
    _imp->playBackOutputSpinbox->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    _imp->playbackMode_Button = new Button(_imp->playerButtonsContainer);
    _imp->playbackMode_Button->setFocusPolicy(Qt::NoFocus);
    _imp->playbackMode_Button->setFixedSize(buttonSize);
    _imp->playbackMode_Button->setIconSize(buttonIconSize);
    _imp->playbackMode_Button->setToolTip( GuiUtils::convertFromPlainText(tr("Behaviour to adopt when the playback hit the end of the range: loop,bounce or stop."), Qt::WhiteSpaceNormal) );


    TimeLinePtr timeline = getGui()->getApp()->getTimeLine();
    QPixmap tripleSyncUnlockPix, tripleSyncLockedPix;
    appPTR->getIcon(NATRON_PIXMAP_UNLOCKED, pixmapIconSize, &tripleSyncUnlockPix);
    appPTR->getIcon(NATRON_PIXMAP_LOCKED, pixmapIconSize, &tripleSyncLockedPix);

    QIcon tripleSyncIc;
    tripleSyncIc.addPixmap(tripleSyncUnlockPix, QIcon::Normal, QIcon::Off);
    tripleSyncIc.addPixmap(tripleSyncLockedPix, QIcon::Normal, QIcon::On);
    _imp->tripleSyncButton = new Button(tripleSyncIc, QString(), _imp->playerButtonsContainer);
    _imp->tripleSyncButton->setToolTip( GuiUtils::convertFromPlainText(tr("When activated, the timeline frame-range is synchronized with the Dope Sheet and the Curve Editor."), Qt::WhiteSpaceNormal) );
    _imp->tripleSyncButton->setCheckable(true);
    _imp->tripleSyncButton->setChecked(false);
    _imp->tripleSyncButton->setFixedSize(buttonSize);
    _imp->tripleSyncButton->setIconSize(buttonIconSize);
    QObject:: connect( _imp->tripleSyncButton, SIGNAL(toggled(bool)),
                       this, SLOT(toggleTripleSync(bool)) );


    _imp->canEditFpsBox = new QCheckBox(_imp->playerButtonsContainer);

    QString canEditFpsBoxTT = GuiUtils::convertFromPlainText(tr("When unchecked, the playback frame rate is automatically set from "
                                                                " the Viewer A input.  "
                                                                "When checked, the user setting is used.")
                                                             , Qt::WhiteSpaceNormal);

    _imp->canEditFpsBox->setFixedSize(buttonSize);
    //_imp->canEditFpsBox->setIconSize(buttonIconSize);
    _imp->canEditFpsBox->setToolTip(canEditFpsBoxTT);
    _imp->canEditFpsBox->setChecked(!_imp->fpsLocked);
    QObject::connect( _imp->canEditFpsBox, SIGNAL(clicked(bool)), this, SLOT(onCanSetFPSClicked(bool)) );

    _imp->canEditFpsLabel = new ClickableLabel(tr("fps:"), _imp->playerButtonsContainer);
    QObject::connect( _imp->canEditFpsLabel, SIGNAL(clicked(bool)), this, SLOT(onCanSetFPSLabelClicked(bool)) );
    _imp->canEditFpsLabel->setToolTip(canEditFpsBoxTT);
    //_imp->canEditFpsLabel->setFont(font);


    _imp->fpsBox = new SpinBox(_imp->playerButtonsContainer, SpinBox::eSpinBoxTypeDouble);
    _imp->fpsBox->setEnabled(!_imp->fpsLocked);
    _imp->fpsBox->decimals(1);
    _imp->userFps = 24.;
    _imp->fpsBox->setValue(_imp->userFps);
    _imp->fpsBox->setIncrement(0.1);
    _imp->fpsBox->setToolTip( QString::fromUtf8("<p><b>") + tr("fps:") + QString::fromUtf8("</b></p>") + tr(
                                  "Viewer playback framerate, in frames per second.") );


    QPixmap pixFreezeEnabled, pixFreezeDisabled;
    appPTR->getIcon(NATRON_PIXMAP_FREEZE_ENABLED, &pixFreezeEnabled);
    appPTR->getIcon(NATRON_PIXMAP_FREEZE_DISABLED, &pixFreezeDisabled);
    QIcon icFreeze;
    icFreeze.addPixmap(pixFreezeEnabled, QIcon::Normal, QIcon::On);
    icFreeze.addPixmap(pixFreezeDisabled, QIcon::Normal, QIcon::Off);
    _imp->turboButton = new Button(icFreeze, QString(), _imp->playerButtonsContainer);
    _imp->turboButton->setCheckable(true);
    _imp->turboButton->setChecked(false);
    _imp->turboButton->setDown(false);
    _imp->turboButton->setFixedSize(buttonSize);
    _imp->turboButton->setIconSize(buttonIconSize);
    _imp->turboButton->setToolTip( QString::fromUtf8("<p><b>") + tr("Turbo mode:") + QString::fromUtf8("</p></b><p>") +
                                   tr("When checked, only the viewer is redrawn during playback, "
                                      "for maximum efficiency.") + QString::fromUtf8("</p>") );
    _imp->turboButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _imp->turboButton, SIGNAL (clicked(bool)), getGui(), SLOT(onFreezeUIButtonClicked(bool)) );
    QPixmap pixFirst;
    QPixmap pixPrevKF;
    QPixmap pixRewindDisabled;
    QPixmap pixBack1;
    QPixmap pixStop;
    QPixmap pixForward1;
    QPixmap pixPlayDisabled;
    QPixmap pixNextKF;
    QPixmap pixLast;
    QPixmap pixPrevIncr;
    QPixmap pixNextIncr;
    QPixmap pixRefresh;
    QPixmap pixRefreshActive;
    QPixmap pixCenterViewer;
    QPixmap pixLoopMode;
    QPixmap pixClipToProjectEnabled;
    QPixmap pixClipToProjectDisabled;
    QPixmap pixViewerRoIEnabled;
    QPixmap pixViewerRoIDisabled;
    QPixmap pixViewerRs;
    QPixmap pixViewerRsChecked;
    QPixmap pixInpoint;
    QPixmap pixOutPoint;
    QPixmap pixPauseEnabled;
    QPixmap pixPauseDisabled;
    QPixmap pixFullFrameOn;
    QPixmap pixFullFrameOff;

    appPTR->getIcon(NATRON_PIXMAP_PLAYER_FIRST_FRAME, &pixFirst);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_KEY, &pixPrevKF);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_REWIND_DISABLED, &pixRewindDisabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS, &pixBack1);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_STOP_ENABLED, &pixStop);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT, &pixForward1);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_DISABLED, &pixPlayDisabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_KEY, &pixNextKF);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_LAST_FRAME, &pixLast);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_INCR, &pixPrevIncr);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_INCR, &pixNextIncr);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH, &pixRefresh);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE, &pixRefreshActive);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER, &pixCenterViewer);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE, &pixLoopMode);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED, &pixClipToProjectEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED, &pixClipToProjectDisabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_ROI_ENABLED, &pixViewerRoIEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_ROI_DISABLED, &pixViewerRoIDisabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_RENDER_SCALE, &pixViewerRs);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED, &pixViewerRsChecked);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_TIMELINE_IN, &pixInpoint);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_TIMELINE_OUT, &pixOutPoint);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PAUSE_DISABLED, &pixPauseDisabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PAUSE_ENABLED, &pixPauseEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_FULL_FRAME_OFF, &pixFullFrameOff);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_FULL_FRAME_ON, &pixFullFrameOn);

    _imp->firstFrame_Button->setIcon( QIcon(pixFirst) );
    _imp->previousKeyFrame_Button->setIcon( QIcon(pixPrevKF) );

    QIcon icRewind;
    icRewind.addPixmap(pixStop, QIcon::Normal, QIcon::On);
    icRewind.addPixmap(pixRewindDisabled, QIcon::Normal, QIcon::Off);
    _imp->play_Backward_Button->setIcon( icRewind );
    _imp->previousFrame_Button->setIcon( QIcon(pixBack1) );
    _imp->nextFrame_Button->setIcon( QIcon(pixForward1) );

    QIcon icPlay;
    icPlay.addPixmap(pixStop, QIcon::Normal, QIcon::On);
    icPlay.addPixmap(pixPlayDisabled, QIcon::Normal, QIcon::Off);
    _imp->play_Forward_Button->setIcon( icPlay );
    _imp->nextKeyFrame_Button->setIcon( QIcon(pixNextKF) );
    _imp->lastFrame_Button->setIcon( QIcon(pixLast) );
    _imp->previousIncrement_Button->setIcon( QIcon(pixPrevIncr) );
    _imp->nextIncrement_Button->setIcon( QIcon(pixNextIncr) );
    _imp->playbackMode_Button->setIcon( QIcon(pixLoopMode) );
    _imp->playBackInputButton->setIcon( QIcon(pixInpoint) );
    _imp->playBackOutputButton->setIcon( QIcon(pixOutPoint) );


    _imp->playerLayout->addWidget(_imp->playBackInputButton);
    _imp->playerLayout->addWidget(_imp->playBackInputSpinbox);

    _imp->playerLayout->addStretch();

    _imp->playerLayout->addWidget(_imp->canEditFpsBox);
    _imp->playerLayout->addSpacing( TO_DPIX(5) );
    _imp->playerLayout->addWidget(_imp->canEditFpsLabel);
    _imp->playerLayout->addWidget(_imp->fpsBox);
    _imp->playerLayout->addWidget(_imp->turboButton);
    _imp->playerLayout->addWidget(_imp->playbackMode_Button);
    _imp->playerLayout->addSpacing( TO_DPIX(10) );
    _imp->playerLayout->addWidget(_imp->tripleSyncButton);

    _imp->playerLayout->addStretch();

    _imp->playerLayout->addWidget(_imp->firstFrame_Button);
    _imp->playerLayout->addWidget(_imp->play_Backward_Button);
    _imp->playerLayout->addWidget(_imp->currentFrameBox);
    _imp->playerLayout->addWidget(_imp->play_Forward_Button);
    _imp->playerLayout->addWidget(_imp->lastFrame_Button);

    _imp->playerLayout->addSpacing( TO_DPIX(10) );

    _imp->playerLayout->addWidget(_imp->previousFrame_Button);
    _imp->playerLayout->addWidget(_imp->nextFrame_Button);

    _imp->playerLayout->addSpacing( TO_DPIX(10) );

    _imp->playerLayout->addWidget(_imp->previousKeyFrame_Button);
    _imp->playerLayout->addWidget(_imp->nextKeyFrame_Button);
    _imp->playerLayout->addSpacing( TO_DPIX(10) );

    _imp->playerLayout->addWidget(_imp->previousIncrement_Button);
    _imp->playerLayout->addWidget(_imp->incrementSpinBox);
    _imp->playerLayout->addWidget(_imp->nextIncrement_Button);
    _imp->playerLayout->addSpacing( TO_DPIX(10) );


    _imp->playerLayout->addStretch();
    _imp->playerLayout->addStretch();

    _imp->playerLayout->addWidget(_imp->playBackOutputSpinbox);
    _imp->playerLayout->addWidget(_imp->playBackOutputButton);


    /*=================================================*/

    /*frame seeker*/
    _imp->timeLineGui = new TimeLineGui(node, timeline, getGui(), this);
    QObject::connect( _imp->timeLineGui, SIGNAL(boundariesChanged(SequenceTime,SequenceTime)),
                      this, SLOT(onTimelineBoundariesChanged(SequenceTime,SequenceTime)) );
    QObject::connect( gui->getApp()->getProject().get(), SIGNAL(frameRangeChanged(int,int)), _imp->timeLineGui, SLOT(onProjectFrameRangeChanged(int,int)) );
    _imp->timeLineGui->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    //Add some spacing because the timeline might be black as the info
    _imp->mainLayout->addSpacing( TO_DPIY(5) );
    _imp->mainLayout->addWidget(_imp->timeLineGui);
    _imp->mainLayout->addWidget(_imp->playerButtonsContainer);

    double leftBound, rightBound;
    gui->getApp()->getFrameRange(&leftBound, &rightBound);
    _imp->timeLineGui->setBoundaries(leftBound, rightBound);
    onTimelineBoundariesChanged(leftBound, rightBound);
    _imp->timeLineGui->setFrameRangeEdited(false);
    /*================================================*/


    /*slots & signals*/

    manageTimelineSlot(false, timeline);
    QObject::connect( _imp->nextKeyFrame_Button, SIGNAL(clicked(bool)), getGui()->getApp().get(), SLOT(goToNextKeyframe()) );
    QObject::connect( _imp->previousKeyFrame_Button, SIGNAL(clicked(bool)), getGui()->getApp().get(), SLOT(goToPreviousKeyframe()) );

    QObject::connect( node.get(), SIGNAL(renderStatsAvailable(int,ViewIdx,double,RenderStatsMap)),
                      this, SLOT(onRenderStatsAvailable(int,ViewIdx,double,RenderStatsMap)) );
    QObject::connect( node.get(), SIGNAL(clipPreferencesChanged()), this, SLOT(onClipPreferencesChanged()) );
    QObject::connect( _imp->viewer, SIGNAL(zoomChanged(int)), this, SLOT(updateZoomComboBox(int)) );
    QObject::connect( _imp->currentFrameBox, SIGNAL(valueChanged(double)), this, SLOT(onCurrentTimeSpinBoxChanged(double)) );
    QObject::connect( _imp->play_Forward_Button, SIGNAL(clicked(bool)), this, SLOT(startPause(bool)) );
    QObject::connect( _imp->play_Backward_Button, SIGNAL(clicked(bool)), this, SLOT(startBackward(bool)) );
    QObject::connect( _imp->previousFrame_Button, SIGNAL(clicked()), this, SLOT(previousFrame()) );
    QObject::connect( _imp->nextFrame_Button, SIGNAL(clicked()), this, SLOT(nextFrame()) );
    QObject::connect( _imp->previousIncrement_Button, SIGNAL(clicked()), this, SLOT(previousIncrement()) );
    QObject::connect( _imp->nextIncrement_Button, SIGNAL(clicked()), this, SLOT(nextIncrement()) );
    QObject::connect( _imp->firstFrame_Button, SIGNAL(clicked()), this, SLOT(firstFrame()) );
    QObject::connect( _imp->lastFrame_Button, SIGNAL(clicked()), this, SLOT(lastFrame()) );
    QObject::connect( _imp->playbackMode_Button, SIGNAL(clicked(bool)), this, SLOT(togglePlaybackMode()) );
    QObject::connect( _imp->playBackInputButton, SIGNAL(clicked()), this, SLOT(onPlaybackInButtonClicked()) );
    QObject::connect( _imp->playBackOutputButton, SIGNAL(clicked()), this, SLOT(onPlaybackOutButtonClicked()) );
    QObject::connect( _imp->playBackInputSpinbox, SIGNAL(valueChanged(double)), this, SLOT(onPlaybackInSpinboxValueChanged(double)) );
    QObject::connect( _imp->playBackOutputSpinbox, SIGNAL(valueChanged(double)), this, SLOT(onPlaybackOutSpinboxValueChanged(double)) );
    QObject::connect( node.get(), SIGNAL(viewerDisconnected()), this, SLOT(disconnectViewer()) );
    QObject::connect( _imp->fpsBox, SIGNAL(valueChanged(double)), this, SLOT(onSpinboxFpsChanged(double)) );


    _imp->mustSetUpPlaybackButtonsTimer.setSingleShot(true);
    QObject::connect( &_imp->mustSetUpPlaybackButtonsTimer, SIGNAL(timeout()), this, SLOT(onSetDownPlaybackButtonsTimeout()) );

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
    ViewerInstancePtr viewerNode = getInternalNode()->getInternalViewerNode();
    RenderEnginePtr engine = viewerNode->getRenderEngine();
    QObject::connect( engine.get(), SIGNAL(renderFinished(int)), this, SLOT(onEngineStopped()) );
    QObject::connect( engine.get(), SIGNAL(renderStarted(bool)), this, SLOT(onEngineStarted(bool)) );
    manageSlotsForInfoWidget(0, true);

}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ViewerTab.cpp"
