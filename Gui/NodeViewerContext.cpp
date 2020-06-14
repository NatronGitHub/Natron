/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/ViewerNode.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/ColoredFrame.h"
#include "Gui/ClickableLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerToolButton.h"

NATRON_NAMESPACE_ENTER

struct NodeViewerContextPrivate
{
    Q_DECLARE_TR_FUNCTIONS(NodeViewerContext)

public:
    NodeViewerContext* publicInterface; // can not be a smart ptr
    NodeGuiWPtr node;
    ViewerGL* viewer;
    ViewerTab* viewerTab;
    std::map<KnobIWPtr, KnobGuiPtr> knobsMapping;
    QString currentRole, currentTool;
    QToolBar* toolbar;
    std::map<QString, ViewerToolButton*> toolButtons;
    QWidget* mainContainer;
    QHBoxLayout* mainContainerLayout;
    Label* nodeLabel;
    QWidget* widgetsContainer;
    QVBoxLayout* widgetsContainerLayout;

    // This is specific to the viewer node only
    QWidget* playerContainer;
    QHBoxLayout* playerLayout;

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
        , nodeLabel(0)
        , widgetsContainer(0)
        , widgetsContainerLayout(0)
        , playerContainer(0)
        , playerLayout(0)
    {
    }

    KnobGuiPtr createKnobInternal(const KnobIPtr& knob);

    void createKnobs(const KnobsVec& knobsUi);

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
    QAction* addToolBarTool(const std::string& toolID,
                            const std::string& roleID,
                            const std::string& roleShortcutID,
                            const std::string& label,
                            const std::string& hintToolTip,
                            const std::string& iconPath,
                            ViewerToolButton** toolButton);

    void onToolActionTriggeredInternal(QAction* action, bool notifyNode);

    void toggleToolsSelection(ViewerToolButton* selected)
    {
        for (std::map<QString, ViewerToolButton*>::iterator it = toolButtons.begin(); it != toolButtons.end(); ++it) {
            if (it->second == selected) {
                it->second->setIsSelected(true);
            } else {
                it->second->setIsSelected(false);
                if ( it->second->isDown() ) {
                    it->second->setDown(false);
                }
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
}

NodeViewerContext::~NodeViewerContext()
{
}

void
NodeViewerContext::createGui()
{
  
    NodeGuiPtr node = _imp->getNode();
    QObject::connect( node.get(), SIGNAL(settingsPanelClosed(bool)), this, SLOT(onNodeSettingsPanelClosed(bool)), Qt::UniqueConnection );
    KnobsVec knobsOrdered = node->getNode()->getEffectInstance()->getViewerUIKnobs();

    ViewerNodePtr isViewerNode = node->getNode()->isEffectViewerNode();

    if ( !knobsOrdered.empty() ) {
        if (!isViewerNode) {
            _imp->mainContainer = new ColoredFrame(_imp->viewer);
        } else {
             _imp->mainContainer = new QWidget(_imp->viewer);
        }
        _imp->mainContainerLayout = new QHBoxLayout(_imp->mainContainer);
        _imp->mainContainerLayout->setContentsMargins(0, 0, 0, 0);
        _imp->mainContainerLayout->setSpacing(0);
        if (!isViewerNode) {
            _imp->nodeLabel = new Label(QString::fromUtf8( node->getNode()->getLabel().c_str() ), _imp->mainContainer);
            QObject::connect( node->getNode().get(), SIGNAL(labelChanged(QString,QString)), this, SLOT(onNodeLabelChanged(QString,QString)) );
        }
        _imp->widgetsContainer = new QWidget(_imp->mainContainer);
        _imp->widgetsContainerLayout = new QVBoxLayout(_imp->widgetsContainer);
        _imp->widgetsContainerLayout->setContentsMargins(0, 0, 0, 0);
        _imp->widgetsContainerLayout->setSpacing(0);
        _imp->mainContainerLayout->addWidget(_imp->widgetsContainer);
        if (_imp->nodeLabel) {
            _imp->mainContainerLayout->addWidget(_imp->nodeLabel);
        }
        onNodeColorChanged( node->getCurrentColor() );
        QObject::connect( node.get(), SIGNAL(colorChanged(QColor)), this, SLOT(onNodeColorChanged(QColor)) );
        setContainerWidget(_imp->mainContainer);
        _imp->createKnobs(knobsOrdered);
    }

    const KnobsVec& allKnobs = node->getNode()->getKnobs();
    KnobPagePtr toolbarPage;
    for (KnobsVec::const_iterator it = allKnobs.begin(); it != allKnobs.end(); ++it) {
        KnobPagePtr isPage = toKnobPage(*it);
        if ( isPage && isPage->getIsToolBar() ) {
            toolbarPage = isPage;
            break;
        }
    }
    if (toolbarPage) {
        KnobsVec pageChildren = toolbarPage->getChildren();
        if ( !pageChildren.empty() ) {
            _imp->toolbar = new QToolBar(_imp->viewer);
            _imp->toolbar->setOrientation(Qt::Vertical);

            for (std::size_t i = 0; i < pageChildren.size(); ++i) {
                KnobGroupPtr isGroup = toKnobGroup( pageChildren[i] );
                if (isGroup) {
                    QObject::connect( isGroup->getSignalSlotHandler().get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onToolGroupValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum)) );
                    KnobsVec toolButtonChildren = isGroup->getChildren();
                    ViewerToolButton* createdToolButton = 0;
                    QString currentActionForGroup;
                    for (std::size_t j = 0; j < toolButtonChildren.size(); ++j) {
                        KnobButtonPtr isButton = toKnobButton( toolButtonChildren[j] );
                        if (isButton) {
                            QObject::connect( isButton->getSignalSlotHandler().get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onToolActionValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum) ));
                            const std::string& roleShortcutID = isGroup->getName();
                            QAction* act = _imp->addToolBarTool(isButton->getName(), isGroup->getName(), roleShortcutID, isButton->getLabel(), isButton->getHintToolTip(), isButton->getIconLabel(), &createdToolButton);
                            if ( act && createdToolButton && isButton->getValue() ) {
                                createdToolButton->setDefaultAction(act);
                                currentActionForGroup = QString::fromUtf8( isButton->getName().c_str() );
                            }
                        }
                    }
                    if ( isGroup->getValue() ) {
                        _imp->currentTool = currentActionForGroup;
                        _imp->currentRole = QString::fromUtf8( isGroup->getName().c_str() );
                        if (createdToolButton) {
                            createdToolButton->setDown(true);
                            createdToolButton->setIsSelected(true);
                        }
                    }
                }
            }
        }
    }
} // NodeViewerContext::createGui


void
NodeViewerContext::onNodeColorChanged(const QColor& color)
{
    QString labelStyle = QString::fromUtf8("Label { color: rgb(%1, %2, %3); }").arg( color.red() ).arg( color.green() ).arg( color.blue() );
    if (_imp->nodeLabel) {
        _imp->nodeLabel->setStyleSheet(labelStyle);
    }
    ColoredFrame* w = dynamic_cast<ColoredFrame*>(_imp->mainContainer);
    if (w) {
        w->setFrameColor(color);
    }
}

void
NodeViewerContext::onNodeSettingsPanelClosed(bool closed)
{
    if (!_imp->viewerTab) {
        return;
    }
    NodeGuiPtr node = _imp->node.lock();
    if (closed) {
        _imp->viewerTab->removeNodeViewerInterface(node, false /*permanently*/, true /*setAnother*/);
    } else {
        // Set the viewer interface for this plug-in to be the one of this node
        _imp->viewerTab->setPluginViewerInterface(node);
    }
}

void
NodeViewerContext::onNodeLabelChanged(const QString& /*oldLabel*/, const QString& newLabel)
{
    _imp->nodeLabel->setText(newLabel);
}

int
NodeViewerContext::getItemsSpacingOnSameLine() const
{
    return 0;
}

KnobGuiPtr
NodeViewerContextPrivate::createKnobInternal(const KnobIPtr& knob)
{
    KnobGuiPtr ret = KnobGui::create(knob, KnobGui::eKnobLayoutTypeViewerUI, publicInterface);

    knobsMapping.insert( std::make_pair(knob, ret) );

    ret->createGUI(widgetsContainer);

    return ret;

}

QWidget*
NodeViewerContext::createKnobHorizontalFieldContainer(QWidget* parent) const
{
    return new QWidget(parent);
}

void
NodeViewerContextPrivate::createKnobs(const KnobsVec& knobsOrdered)
{
    NodeGuiPtr thisNode = getNode();

    assert( !knobsOrdered.empty() );


    knobsMapping.clear();

    {
        for (KnobsVec::const_iterator it = knobsOrdered.begin(); it != knobsOrdered.end(); ++it) {
            KnobGuiPtr ret = createKnobInternal(*it);

            if (ret->isOnNewLine()) {
                QWidget* mainContainer = ret->getFieldContainer();
                widgetsContainerLayout->addWidget(mainContainer);
            }
        }
    }

    ViewerNodePtr isViewer = thisNode->getNode()->isEffectViewerNode();
    if (isViewer) {
        KnobIPtr playerKnob = thisNode->getNode()->getKnobByName(kViewerNodeParamPlayerToolBarPage);
        KnobPagePtr playerPage = toKnobPage(playerKnob);
        assert(playerPage);

        KnobsVec playerKnobs = playerPage->getChildren();
        assert(!playerKnobs.empty());
        for (KnobsVec::const_iterator it = playerKnobs.begin(); it != playerKnobs.end(); ++it) {
            KnobGuiPtr ret = createKnobInternal(*it);
            if (it == playerKnobs.begin()) {
                playerContainer = ret->getFieldContainer();
                playerLayout = dynamic_cast<QHBoxLayout*>(playerContainer->layout());
            }
        }

    }

} // NodeViewerContextPrivate::createKnobs

QAction*
NodeViewerContextPrivate::addToolBarTool(const std::string& toolID,
                                         const std::string& roleID,
                                         const std::string& roleShortcutID,
                                         const std::string& label,
                                         const std::string& hintToolTip,
                                         const std::string& iconPath,
                                         ViewerToolButton** createdToolButton)
{
    QString qRoleId = QString::fromUtf8( roleID.c_str() );
    std::map<QString, ViewerToolButton*>::iterator foundToolButton = toolButtons.find(qRoleId);
    ViewerToolButton* toolButton = 0;

    if ( foundToolButton != toolButtons.end() ) {
        toolButton = foundToolButton->second;
    } else {
        toolButton = new ViewerToolButton(toolbar);
        toolbar->addWidget(toolButton);
        toolButtons.insert( std::make_pair(qRoleId, toolButton) );
        QSize rotoToolSize( TO_DPIX(NATRON_LARGE_BUTTON_SIZE), TO_DPIY(NATRON_LARGE_BUTTON_SIZE) );
        toolButton->setFixedSize(rotoToolSize);
        toolButton->setIconSize(rotoToolSize);
        toolButton->setPopupMode(QToolButton::InstantPopup);
        QObject::connect( toolButton, SIGNAL(triggered(QAction*)), publicInterface, SLOT(onToolActionTriggered(QAction*)) );
    }

    *createdToolButton = toolButton;


    QString shortcutGroup = QString::fromUtf8(getNode()->getNode()->getOriginalPlugin()->getPluginShortcutGroup().c_str());
    QIcon icon;
    if ( !iconPath.empty() ) {
        QString iconPathStr = QString::fromUtf8( iconPath.c_str() );

        if ( !QFile::exists(iconPathStr) ) {
            QString resourcesPath = QString::fromUtf8(node.lock()->getNode()->getPluginResourcesPath().c_str());

            if ( !resourcesPath.endsWith( QLatin1Char('/') ) ) {
                resourcesPath += QLatin1Char('/');
            }
            iconPathStr.prepend(resourcesPath);
        }


        if ( QFile::exists(iconPathStr) ) {
            QPixmap pix;
            pix.load(iconPathStr);
            if ( !pix.isNull() ) {
                icon.addPixmap(pix);
            }
        }
    }

    //QString labelTouse = icon.isNull() ? QString::fromUtf8(label.c_str()) : QString();
    QAction* action = new QAction(icon, QString::fromUtf8( label.c_str() ), toolButton);
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
            QKeySequence keybinds = getKeybind( shortcutGroup, QString::fromUtf8( toolID.c_str() ) );
            if (!keybinds.isEmpty()) {
                toolTip += QString::fromUtf8("<p><b>");
                toolTip += tr("Keyboard shortcut: %1").arg( keybinds.toString(QKeySequence::NativeText) );
                toolTip += QString::fromUtf8("</b></p>");
            }
        }
        action->setToolTip(toolTip);
    }
    QObject::connect( action, SIGNAL(triggered()), publicInterface, SLOT(onToolActionTriggered()) );
    toolButton->addAction(action);

    return action;
} // NodeViewerContextPrivate::addToolBarTool

QWidget*
NodeViewerContext::getPlayerToolbar() const
{
    return _imp->playerContainer;
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
    return _imp->viewerTab ? _imp->viewerTab->getGui() : 0;
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
    if (!panel) {
        _imp->getNode()->ensurePanelCreated();
        panel = _imp->getNode()->getSettingPanel();
        if (panel) {
            panel->setClosed(true);
        }
    }
    if (panel) {
        panel->pushUndoCommand(cmd);
    }
}

KnobGuiPtr
NodeViewerContext::getKnobGui(const KnobIPtr& knob) const
{
    std::map<KnobIWPtr, KnobGuiPtr>::const_iterator found =  _imp->knobsMapping.find(knob);

    if ( found == _imp->knobsMapping.end() ) {
        return KnobGuiPtr();
    }

    return found->second;
}

void
NodeViewerContext::onToolButtonShortcutPressed(const QString& roleID)
{
    std::map<QString, ViewerToolButton*>::iterator found = _imp->toolButtons.find(roleID);

    if ( found == _imp->toolButtons.end() ) {
        return;
    }
    found->second->handleSelection();
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
NodeViewerContext::onToolGroupValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum reason)
{
    KnobSignalSlotHandler* caller = dynamic_cast<KnobSignalSlotHandler*>( sender() );

    if (!caller) {
        return;
    }
    KnobIPtr knob = caller->getKnob();
    if (!knob) {
        return;
    }

    if ( (reason == eValueChangedReasonUserEdited) ||
         ( reason == eValueChangedReasonUserEdited) ) {
        return;
    }

    QString newRoleID = QString::fromUtf8( knob->getName().c_str() );
    std::map<QString, ViewerToolButton*>::iterator foundOldTool = _imp->toolButtons.find(newRoleID);
    assert( foundOldTool != _imp->toolButtons.end() );
    if ( foundOldTool == _imp->toolButtons.end() ) {
        return;
    }

    ViewerToolButton* newToolButton = foundOldTool->second;
    assert(newToolButton);
    _imp->toggleToolsSelection(newToolButton);
    newToolButton->setDown(true);

    _imp->currentRole = newRoleID;
}

void
NodeViewerContext::onToolActionValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum reason)
{
    KnobSignalSlotHandler* caller = dynamic_cast<KnobSignalSlotHandler*>( sender() );

    if (!caller) {
        return;
    }
    KnobIPtr knob = caller->getKnob();
    if (!knob) {
        return;
    }

    if ( (reason == eValueChangedReasonUserEdited) ||
         ( reason == eValueChangedReasonUserEdited) ) {
        return;
    }

    QString newToolID = QString::fromUtf8( knob->getName().c_str() );
    std::map<QString, ViewerToolButton*>::iterator foundOldTool = _imp->toolButtons.find(_imp->currentRole);
    assert( foundOldTool != _imp->toolButtons.end() );
    if ( foundOldTool == _imp->toolButtons.end() ) {
        return;
    }

    ViewerToolButton* newToolButton = foundOldTool->second;
    assert(newToolButton);
    QList<QAction*> actions = newToolButton->actions();
    for (QList<QAction*>::iterator it = actions.begin(); it != actions.end(); ++it) {
        QStringList actionData = (*it)->data().toStringList();

        if (actionData.size() != 2) {
            continue;
        }
        const QString& actionRoleID = actionData[0];
        const QString& actionTool = actionData[1];
        assert(actionRoleID == _imp->currentRole);
        if ( (actionRoleID == _imp->currentRole) && (actionTool == newToolID) ) {
            newToolButton->setDefaultAction(*it);
            _imp->currentTool = newToolID;

            return;
        }
    }
}

void
NodeViewerContextPrivate::onToolActionTriggeredInternal(QAction* action,
                                                        bool notifyNode)
{
    if (!action) {
        return;
    }
    QStringList actionData = action->data().toStringList();

    if (actionData.size() != 2) {
        return;
    }


    const QString& newRoleID = actionData[0];
    const QString& newToolID = actionData[1];

    if (currentTool == newToolID && currentRole == newRoleID) {
        return;
    }

    std::map<QString, ViewerToolButton*>::iterator foundNextTool = toolButtons.find(newRoleID);
    assert( foundNextTool != toolButtons.end() );
    if ( foundNextTool == toolButtons.end() ) {
        return;
    }

    ViewerToolButton* newToolButton = foundNextTool->second;
    assert(newToolButton);
    toggleToolsSelection(newToolButton);
    newToolButton->setDown(true);
    newToolButton->setDefaultAction(action);

    QString oldRole = currentRole;
    QString oldTool = currentTool;

    currentRole = newRoleID;
    currentTool = newToolID;

    if (notifyNode) {
        // Refresh other viewers toolbars
        NodeGuiPtr n = node.lock();
        const std::list<ViewerTab*> viewers = publicInterface->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            if (*it != viewerTab) {
                (*it)->updateSelectedToolForNode(newToolID, n);
            }
        }

        KnobIPtr oldGroupKnob = n->getNode()->getKnobByName( oldRole.toStdString() );
        KnobIPtr newGroupKnob = n->getNode()->getKnobByName( newRoleID.toStdString() );
        KnobIPtr oldToolKnob = n->getNode()->getKnobByName( oldTool.toStdString() );
        KnobIPtr newToolKnob = n->getNode()->getKnobByName( newToolID.toStdString() );
        assert(newToolKnob && newGroupKnob);
        if (newToolKnob && newGroupKnob) {

            KnobButtonPtr oldIsButton = oldToolKnob ? toKnobButton( oldToolKnob ) : KnobButtonPtr();
            KnobButtonPtr newIsButton = toKnobButton( newToolKnob );
            assert(newIsButton);

            KnobGroupPtr oldIsGroup = oldGroupKnob ? toKnobGroup( oldGroupKnob ) : KnobGroupPtr();
            KnobGroupPtr newIsGroup = toKnobGroup( newGroupKnob );
            assert(newIsGroup);
            if (newIsButton && newIsGroup) {
                EffectInstancePtr effect = n->getNode()->getEffectInstance();

                if (oldIsGroup) {
                    if (oldIsGroup->getValue() != false) {
                        oldIsGroup->setValue(false, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited);
                    } else {
                        // We must issue at least a knobChanged call
                        effect->onKnobValueChanged_public(oldIsGroup, eValueChangedReasonUserEdited, effect->getTimelineCurrentTime(), ViewSetSpec(0));
                    }
                }
                
                if (newIsGroup->getValue() != true) {
                    newIsGroup->setValue(true, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited);
                } else {
                    // We must issue at least a knobChanged call
                    effect->onKnobValueChanged_public(newIsGroup, eValueChangedReasonUserEdited, effect->getTimelineCurrentTime(), ViewSetSpec(0));
                }


                // Only change the value of the button if we are in the same group
                if (oldIsButton && oldIsGroup == newIsGroup) {
                    if (oldIsButton->getValue() != false) {
                        oldIsButton->setValue(false, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited);
                    } else {
                        // We must issue at least a knobChanged call
                        effect->onKnobValueChanged_public(oldIsButton, eValueChangedReasonUserEdited, effect->getTimelineCurrentTime(), ViewSetSpec(0));
                    }
                }
                if (newIsButton->getValue() != true) {
                    newIsButton->setValue(true, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited);
                } else {
                    // We must issue at least a knobChanged call
                    effect->onKnobValueChanged_public(newIsButton, eValueChangedReasonUserEdited, effect->getTimelineCurrentTime(), ViewSetSpec(0));
                }
            }
        }
    }
} // NodeViewerContextPrivate::onToolActionTriggeredInternal


void
NodeViewerContext::notifyGuiClosing()
{
    _imp->viewer = 0;
    _imp->viewerTab = 0;
}


NodeGuiPtr
NodeViewerContext::getNodeGui() const
{
    return _imp->node.lock();
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_NodeViewerContext.cpp"
