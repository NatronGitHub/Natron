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

#include "NodeViewerContext.h"

#include <QMouseEvent>
#include <QStyle>
#include <QToolBar>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPalette>
#include <QFrame>
#include <QUndoStack>
#include <QUndoCommand>
#include <ofxKeySyms.h>

#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/ClickableLabel.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerToolButton.h"

NATRON_NAMESPACE_ENTER;

struct NodeViewerContextPrivate
{
    NodeViewerContext* publicInterface;
    NodeGuiWPtr node;
    ViewerGL* viewer;
    ViewerTab* viewerTab;
    std::map<KnobWPtr, KnobGuiPtr> knobsMapping;
    QString currentRole, currentTool;
    QToolBar* toolbar;
    std::map<QString, ViewerToolButton*> toolButtons;
    QWidget* mainContainer;
    QVBoxLayout* mainContainerLayout;

    NodeViewerContextPrivate(NodeViewerContext* pi,
                             const NodeGuiPtr& node,
                             ViewerTab* viewer)
        : publicInterface(pi)
        , node(node)
        , viewer( viewer->getViewer() )
        , viewerTab(viewer)
        , knobsMapping()
        , currentRole()
        , currentTool()
        , toolbar(0)
        , toolButtons()
        , mainContainer(0)
        , mainContainerLayout(0)
    {
    }

    void createKnobs();

    NodeGuiPtr getNode() const
    {
        return node.lock();
    }

    /**
     * @brief Add a new tool to the toolbutton corresponding to the given roleID.
     * If a ViewerToolButton for this roleID does not exist yet, it is created and the
     * given roleShortcutID is associated to it.
     * The shortcut for this button will be added to the Shorcut Editor with by default
     * the given modifiers and symbols (as defined in <ofxKeySyms.h>).
     * The tool will have the given label, and when hovering the button with the mouse, the user
     * will receive the hintToolTip help.
     * Optionnally, a path to an icon can be specified for this button.
     **/
    void addToolBarTool(const std::string& toolID,
                        const std::string& roleID,
                        const std::string& roleShortcutID,
                        const std::string& label,
                        const std::string& hintToolTip,
                        const std::string& iconPath);

    void onToolActionTriggeredInternal(QAction* action, bool notifyNode);

    void toggleToolsSelection(ViewerToolButton* selected)
    {
        for (std::map<QString, ViewerToolButton*>::iterator it = toolButtons.begin(); it != toolButtons.end(); ++it) {
            if (it->second == selected) {
                it->second->setIsSelected(true);
            } else {
                it->second->setIsSelected(false);
            }
        }
    }
};

NodeViewerContext::NodeViewerContext(const NodeGuiPtr& node,
                                     ViewerTab* viewer)
    : QObject()
    , KnobGuiContainerI()
    , _imp( new NodeViewerContextPrivate(this, node, viewer) )
{
    _imp->mainContainer = new QWidget(viewer);
    _imp->mainContainerLayout = new QVBoxLayout(_imp->mainContainer);
    _imp->mainContainerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainContainerLayout->setSpacing(0);

    setContainerWidget(_imp->mainContainer);
}

NodeViewerContext::~NodeViewerContext()
{
}

static void
addSpacer(QBoxLayout* layout)
{
    layout->addSpacing( TO_DPIX(5) );
    QFrame* line = new QFrame( layout->parentWidget() );
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Raised);
    QPalette palette;
    palette.setColor(QPalette::Foreground, Qt::black);
    line->setPalette(palette);
    layout->addWidget(line);
    layout->addSpacing( TO_DPIX(5) );
}

void
NodeViewerContextPrivate::createKnobs()
{
    NodeGuiPtr thisNode = getNode();
    const KnobsVec& knobs = thisNode->getNode()->getKnobs();
    std::map<int, KnobPtr> knobsOrdered;

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        int index = (*it)->getInViewerContextIndex();
        if (index != -1) {
            knobsOrdered[index] = *it;
        }
    }

    // hasKnobWithViewerInContextUI() should have been checked before
    assert( !knobsOrdered.empty() );

    if ( knobsOrdered.empty() ) {
        return;
    }

    knobsMapping.clear();


    QWidget* lastRowContainer = new QWidget(mainContainer);
    QHBoxLayout* lastRowLayout = new QHBoxLayout(lastRowContainer);
    lastRowLayout->setContentsMargins(TO_DPIX(3), TO_DPIY(2), 0, 0);
    lastRowLayout->setSpacing(0);
    mainContainerLayout->addWidget(lastRowContainer);

    KnobsVec knobsOnSameLine;
    std::map<int, KnobPtr>::iterator next = knobsOrdered.begin();
    ++next;
    for (std::map<int, KnobPtr>::iterator it = knobsOrdered.begin(); it != knobsOrdered.end(); ++it) {
        KnobGuiPtr ret( appPTR->createGuiForKnob(it->second, publicInterface) );
        if (!ret) {
            assert(false);
            continue;
        }
        ret->initialize();

        knobsMapping.insert( std::make_pair(it->second, ret) );

        bool makeNewLine = it->second->getInViewerContextNewLineActivated();
        QWidget* labelContainer = 0;
        KnobClickableLabel* label = 0;

        ret->createGUI(lastRowContainer, labelContainer, label, 0 /*warningIndicator*/, lastRowLayout, makeNewLine, knobsOnSameLine);

        if (makeNewLine) {
            knobsOnSameLine.clear();
            lastRowContainer = new QWidget(mainContainer);
            lastRowLayout = new QHBoxLayout(lastRowContainer);
            lastRowLayout->setContentsMargins(TO_DPIX(3), TO_DPIY(2), 0, 0);
            lastRowLayout->setSpacing(0);
            mainContainerLayout->addWidget(lastRowContainer);
        } else {
            knobsOnSameLine.push_back(it->second);
            if ( next != knobsOrdered.end() ) {
                if ( it->second->getInViewerContextAddSeparator() ) {
                    addSpacer(lastRowLayout);
                } else {
                    int spacing = it->second->getInViewerContextItemSpacing();
                    lastRowLayout->addSpacing( TO_DPIX(spacing) );
                }
            }
        } // makeNewLine

        if ( next == knobsOrdered.end() ) {
            ++next;
        }
    }
} // NodeViewerContextPrivate::createKnobs

void
NodeViewerContextPrivate::addToolBarTool(const std::string& toolID,
                                         const std::string& roleID,
                                         const std::string& roleShortcutID,
                                         const std::string& label,
                                         const std::string& hintToolTip,
                                         const std::string& iconPath)
{
    QString qRoleId = QString::fromUtf8( roleID.c_str() );
    std::map<QString, ViewerToolButton*>::iterator foundToolButton = toolButtons.find(qRoleId);
    ViewerToolButton* toolButton = 0;

    if ( foundToolButton != toolButtons.end() ) {
        toolButton = foundToolButton->second;
    } else {
        toolButton = new ViewerToolButton(toolbar);
        toolButtons.insert( std::make_pair(qRoleId, toolButton) );
        QSize rotoToolSize( TO_DPIX(NATRON_LARGE_BUTTON_SIZE), TO_DPIY(NATRON_LARGE_BUTTON_SIZE) );
        toolButton->setFixedSize(rotoToolSize);
        toolButton->setIconSize(rotoToolSize);
        toolButton->setPopupMode(QToolButton::InstantPopup);
        QObject::connect( toolButton, SIGNAL(triggered(QAction*)), publicInterface, SLOT(onToolActionTriggered(QAction*)) );
    }


    QString shortcutGroup = getNode()->getNode()->getPlugin()->getPluginShortcutGroup();
    QIcon icon;
    if ( !iconPath.empty() ) {
        QString iconPathStr = QString::fromUtf8( iconPath.c_str() );
        if ( QFile::exists(iconPathStr) ) {
            QPixmap pix;
            pix.load(iconPathStr);
            if ( !pix.isNull() ) {
                icon.addPixmap(pix);
            }
        }
    }

    QAction* action;
    if ( !hintToolTip.empty() ) {
        action = new TooltipActionShortcut(shortcutGroup.toStdString(), toolID, label, toolButton);
        action->setText( QString::fromUtf8( label.c_str() ) );
        action->setIcon(icon);
    } else {
        action = new QAction(icon, QString::fromUtf8( label.c_str() ), toolButton);
    }

    QStringList data;
    data.push_back(qRoleId);
    data.push_back( QString::fromUtf8( toolID.c_str() ) );
    action->setData(data);
    QString toolTip;
    if ( !hintToolTip.empty() ) {
        toolTip.append( QString::fromUtf8("<p>") );
        toolTip += QString::fromUtf8( hintToolTip.c_str() );
        toolTip.append( QString::fromUtf8("</p>") );
        if ( !roleShortcutID.empty() ) {
            std::list<QKeySequence> keybinds = getKeybind( shortcutGroup, QString::fromUtf8( toolID.c_str() ) );
            if (keybinds.size() >= 1) {
                toolTip += QString::fromUtf8("<p><b>");
                toolTip += QObject::tr("Keyboard shortcut: ");
                toolTip += keybinds.front().toString(QKeySequence::NativeText);
                toolTip += QString::fromUtf8("</b></p>");
            }
        }
        action->setToolTip(toolTip);
    }
    QObject::connect( action, SIGNAL(triggered()), publicInterface, SLOT(onToolActionTriggered()) );
    toolButton->addAction(action);
} // NodeViewerContextPrivate::addToolBarTool

QWidget*
NodeViewerContext::getButtonsContainer() const
{
    return _imp->mainContainer;
}

QToolBar*
NodeViewerContext::getToolBar() const
{
    return _imp->toolbar;
}

const QString&
NodeViewerContext::getCurrentRole() const
{
    return _imp->currentRole;
}

const QString&
NodeViewerContext::getCurrentTool() const
{
    return _imp->currentTool;
}

Gui*
NodeViewerContext::getGui() const
{
    return _imp->viewerTab->getGui();
}

const QUndoCommand*
NodeViewerContext::getLastUndoCommand() const
{
    NodeSettingsPanel* panel = _imp->getNode()->getSettingPanel();

    if (panel) {
        return panel->getLastUndoCommand();
    }

    return 0;
}

void
NodeViewerContext::pushUndoCommand(QUndoCommand* cmd)
{
    NodeSettingsPanel* panel = _imp->getNode()->getSettingPanel();

    if (panel) {
        panel->pushUndoCommand(cmd);
    }
}

KnobGuiPtr
NodeViewerContext::getKnobGui(const KnobPtr& knob) const
{
    std::map<KnobWPtr, KnobGuiPtr>::const_iterator found =  _imp->knobsMapping.find(knob);

    if ( found == _imp->knobsMapping.end() ) {
        return KnobGuiPtr();
    }

    return found->second;
}

void
NodeViewerContext::onToolActionTriggered()
{
    QAction* act = qobject_cast<QAction*>( sender() );

    if (act) {
        onToolActionTriggered(act);
    }
}

void
NodeViewerContext::onToolActionTriggered(QAction* act)
{
    _imp->onToolActionTriggeredInternal(act, true);
}

void
NodeViewerContext::setCurrentTool(const QString& toolID,
                                  bool notifyNode)
{
    QList<QAction*> actions;
    for (std::map<QString, ViewerToolButton*>::iterator it = _imp->toolButtons.begin(); it != _imp->toolButtons.end(); ++it) {
        QList<QAction*> roleActions = it->second->actions();
        actions.append(roleActions);
    }
    for (int i = 0; i < actions.size(); ++i) {
        QStringList actionData = actions[i]->data().toStringList();
        if (actionData.size() != 2) {
            continue;
        }
        if (actionData[1] == toolID) {
            _imp->onToolActionTriggeredInternal(actions[i], notifyNode);

            return;
        }
    }
}

void
NodeViewerContextPrivate::onToolActionTriggeredInternal(QAction* action,
                                                        bool notifyNode)
{
    QStringList actionData = action->data().toStringList();

    if (actionData.size() != 2) {
        return;
    }


    const QString& newRoleID = actionData[0];
    const QString& newToolID = actionData[1];

    if (currentTool == newToolID) {
        return;
    }

    std::map<QString, ViewerToolButton*>::iterator foundOldTool = toolButtons.find(newRoleID);
    assert( foundOldTool != toolButtons.end() );
    if ( foundOldTool == toolButtons.end() ) {
        return;
    }

    ViewerToolButton* newToolButton = foundOldTool->second;
    assert(newToolButton);
    toggleToolsSelection(newToolButton);
    newToolButton->setDown(true);
    newToolButton->setDefaultAction(action);

    QString oldRole = currentRole;
    QString oldTool = currentTool;

    currentRole = newRoleID;
    currentTool = newToolID;

    if (notifyNode) {
#pragma message WARN("To implement when a toolbutton changes")
    }
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_NodeViewerContext.cpp"
