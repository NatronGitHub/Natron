/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <map>
#include <vector>
#include <stdexcept>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QThread>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QCheckBox>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Gui/BackdropGui.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeGui.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER


void
NodeGraph::toggleAutoHideInputs(bool setSettings)
{
    if (setSettings) {
        bool autoHide = !appPTR->getCurrentSettings()->areOptionalInputsAutoHidden();
        appPTR->getCurrentSettings()->setOptionalInputsAutoHidden(autoHide);
    }
    for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->refreshEdgesVisility();
    }
    for (NodesGuiList::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
        (*it)->refreshEdgesVisility();
    }
}

void
NodeGraph::toggleHideInputs()
{
    const NodesGuiList& selectedNodes = getSelectedNodes();

    if ( selectedNodes.empty() ) {
        Dialogs::warningDialog( tr("Hide Inputs").toStdString(), tr("You must select a node first").toStdString() );

        return;
    }

    bool hidden = !selectedNodes.front()->getNode()->getHideInputsKnobValue();

    for (NodesGuiList::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        (*it)->getNode()->setHideInputsKnobValue(hidden);
        //(*it)->refreshEdgesVisility();
    }
}

NodesGuiList
NodeGraph::getNodesWithinBackdrop(const NodeGuiPtr& bd) const
{
    BackdropGui* isBd = dynamic_cast<BackdropGui*>( bd.get() );

    if (!isBd) {
        return NodesGuiList();
    }

    QRectF bbox = bd->mapToScene( bd->boundingRect() ).boundingRect();
    NodesGuiList ret;
    QMutexLocker l(&_imp->_nodesMutex);

    for (NodesGuiList::const_iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        QRectF nodeBbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( bbox.contains(nodeBbox) ) {
            ret.push_back(*it);
        }
    }

    return ret;
}

void
NodeGraph::refreshNodesKnobsAtTime(bool onlyTimeEvaluationKnobs,
                                   SequenceTime time)
{
    ///Refresh all knobs at the current time
    for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->refreshKnobsAfterTimeChange(onlyTimeEvaluationKnobs, time);
    }
}

void
NodeGraph::refreshAllKnobsGui()
{
    for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ( (*it)->isSettingsPanelVisible() ) {
            const std::list<std::pair<KnobIWPtr, KnobGuiPtr> > & knobs = (*it)->getKnobs();

            for (std::list<std::pair<KnobIWPtr, KnobGuiPtr> >::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
                KnobIPtr knob = it2->first.lock();
                if ( !knob->getIsSecret() ) {
                    for (int i = 0; i < knob->getDimension(); ++i) {
                        if ( knob->isAnimated(i) ) {
                            it2->second->onInternalValueChanged(ViewSpec::all(), i, eValueChangedReasonPluginEdited);
                            it2->second->onAnimationLevelChanged(ViewSpec::all(), i);
                        }
                    }
                }
            }
        }
    }
}

void
NodeGraph::focusInEvent(QFocusEvent* e)
{
    QGraphicsView::focusInEvent(e);

    if ( getGui() ) {
        getGui()->setLastSelectedGraph(this);
    }
}

void
NodeGraph::focusOutEvent(QFocusEvent* e)
{
    if (_imp->_bendPointsVisible) {
        _imp->setNodesBendPointsVisible(false);
    }
    QGraphicsView::focusOutEvent(e);
}

void
NodeGraph::toggleSelectedNodesEnabled()
{
    _imp->toggleSelectedNodesEnabled();
}

bool
NodeGraph::areKnobLinksVisible() const
{
    return _imp->_knobLinksVisible;
}

void
NodeGraph::popFindDialog(const QPoint& p)
{
    QPoint realPos = p;
    FindNodeDialog* dialog = new FindNodeDialog(this, this);

    if ( (realPos.x() == 0) && (realPos.y() == 0) ) {
        QPoint global = QCursor::pos();
        QSize sizeH = dialog->sizeHint();
        global.rx() -= sizeH.width() / 2;
        global.ry() -= sizeH.height() / 2;
        realPos = global;
    }

    QObject::connect( dialog, SIGNAL(rejected()), this, SLOT(onFindNodeDialogFinished()) );
    QObject::connect( dialog, SIGNAL(accepted()), this, SLOT(onFindNodeDialogFinished()) );
    dialog->move( realPos.x(), realPos.y() );
    dialog->raise();
    dialog->show();
}

void
NodeGraph::renameNode()
{
    NodeGuiPtr node;

    if (_imp->_selection.size() == 1) {
        node = _imp->_selection.front();
    } else {
        Dialogs::errorDialog( tr("Rename node").toStdString(), tr("You must select exactly 1 node to rename.").toStdString() );

        return;
    }

    assert(node);


    QPointF realPos = node->getPos_mt_safe();
    //qDebug() << "getPos" << realPos.x() << realPos.y();
    realPos = node->mapFromParent(realPos);
    //qDebug() << "fromParent" << realPos.x() << realPos.y();
    realPos = node->mapToScene(realPos);
    QPoint global = this->mapFromScene(realPos);
    global = this->mapToGlobal(global);
    //qDebug() << "toGlobal" << global.x() << global.y();
    EditNodeNameDialog* dialog = new EditNodeNameDialog(node, this);

    {
        QSize sizeH = dialog->sizeHint();
        global.rx() -= sizeH.width() / 2;
        global.ry() += sizeH.height() / 2;
        realPos = global;
        //qDebug() << realPos.x() << realPos.y();
    }
    QObject::connect( dialog, SIGNAL(rejected()), this, SLOT(onNodeNameEditDialogFinished()) );
    QObject::connect( dialog, SIGNAL(accepted()), this, SLOT(onNodeNameEditDialogFinished()) );
    dialog->move( realPos.x(), realPos.y() );
    dialog->raise();
    dialog->show();
}

void
NodeGraph::onFindNodeDialogFinished()
{
    FindNodeDialog* dialog = qobject_cast<FindNodeDialog*>( sender() );

    if (dialog) {
        dialog->deleteLater();
    }
}

struct FindNodeDialogPrivate
{
    NodeGraph* graph;
    QString currentFilter;
    NodesGuiList nodeResults;
    int currentFindIndex;
    QVBoxLayout* mainLayout;
    Label* label;
    QCheckBox* matchWhole;
    QCheckBox* caseSensitivity;
    Label* resultLabel;
    LineEdit* filter;
    DialogButtonBox* buttons;
    QPushButton* nextButton;


    FindNodeDialogPrivate(NodeGraph* graph)
        : graph(graph)
        , currentFilter()
        , nodeResults()
        , currentFindIndex(-1)
        , mainLayout(0)
        , label(0)
        , matchWhole(0)
        , caseSensitivity(0)
        , resultLabel(0)
        , filter(0)
        , buttons(0)
        , nextButton(0)
    {
    }
};

FindNodeDialog::FindNodeDialog(NodeGraph* graph,
                               QWidget* parent)
    : QDialog(parent)
    , _imp( new FindNodeDialogPrivate(graph) )
{
    setWindowFlags(Qt::Popup);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);

    _imp->label = new Label(tr("Select all nodes matching:"), this);
    //_imp->label->setFont(QFont(appFont,appFontSize));
    _imp->mainLayout->addWidget(_imp->label);

    _imp->filter = new LineEdit(this);
    _imp->filter->setToolTip( tr("Search pattern. May contain wildcards:\n"
                                 "? Matches any single character.\n"
                                 "* Matches zero or more of any characters.\n"
                                 "[...] Matches any character within the set in square brackets.") );
    QObject::connect( _imp->filter, SIGNAL(editingFinished()), this, SLOT(updateFindResultsWithCurrentFilter()) );
    QObject::connect( _imp->filter, SIGNAL(textEdited(QString)), this, SLOT(updateFindResults(QString)) );

    _imp->mainLayout->addWidget(_imp->filter);

    _imp->matchWhole = new QCheckBox(tr("Match whole pattern"), this);
    _imp->filter->setToolTip( tr("When checked, the given pattern must match the whole node name.") );
    _imp->matchWhole->setChecked(false);
    QObject::connect( _imp->matchWhole, SIGNAL(toggled(bool)), this, SLOT(forceUpdateFindResults()) );
    _imp->mainLayout->addWidget(_imp->matchWhole);

    _imp->caseSensitivity = new QCheckBox(tr("Case sensitive"), this);
    _imp->caseSensitivity->setChecked(false);
    QObject::connect( _imp->caseSensitivity, SIGNAL(toggled(bool)), this, SLOT(forceUpdateFindResults()) );
    _imp->mainLayout->addWidget(_imp->caseSensitivity);


    _imp->resultLabel = new Label(this);
    _imp->mainLayout->addWidget(_imp->resultLabel);
    //_imp->resultLabel->setFont(QFont(appFont,appFontSize));

    _imp->buttons = new DialogButtonBox(QDialogButtonBox::NoButton, Qt::Horizontal, this);
    _imp->nextButton = _imp->buttons->addButton(tr("&Next"), QDialogButtonBox::ActionRole);
    QObject::connect( _imp->buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onButtonClicked(QAbstractButton*)) );

    _imp->mainLayout->addWidget(_imp->buttons);
    _imp->filter->setFocus();

    selectNextResult();
}

FindNodeDialog::~FindNodeDialog()
{
}

void
FindNodeDialog::updateFindResults(const QString& filter)
{
    if (filter == _imp->currentFilter) {
        return;
    }

    _imp->currentFilter = filter;
    _imp->currentFindIndex = 0;
    _imp->nodeResults.clear();

    _imp->graph->deselect();

    if ( _imp->currentFilter.isEmpty() ) {
        _imp->resultLabel->setText( QString() );
        selectNextResult();

        return;
    }
    Qt::CaseSensitivity sensitivity = _imp->caseSensitivity->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const NodesGuiList& activeNodes = _imp->graph->getAllActiveNodes();
    QRegExp exp(_imp->matchWhole->isChecked() ? filter :
                ( QChar::fromLatin1('*') + filter + QChar::fromLatin1('*') ),
                sensitivity,
                QRegExp::Wildcard);

    if ( exp.isValid() ) {
        for (NodesGuiList::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
            if ( (*it)->isVisible() && exp.exactMatch( QString::fromUtf8( (*it)->getNode()->getLabel().c_str() ) ) ) {
                _imp->nodeResults.push_back(*it);
            }
        }

        if ( ( _imp->nodeResults.size() ) == 0 ) {
            _imp->resultLabel->setText( QString() );
        }
    }

    selectNextResult();
}

void
FindNodeDialog::selectNextResult()
{
    if ( _imp->currentFindIndex >= (int)( _imp->nodeResults.size() ) ) {
        _imp->currentFindIndex = 0;
    }

    _imp->buttons->setEnabled(_imp->nodeResults.size() > 1);

    if ( _imp->nodeResults.empty() ) {
        return;
    }

    NodesGuiList::iterator it = _imp->nodeResults.begin();
    std::advance(it, _imp->currentFindIndex);

    _imp->graph->selectNode(*it, false);
    _imp->graph->centerOnItem( it->get() );


    QString text = QString::fromUtf8("Selecting result %1 of %2").arg(_imp->currentFindIndex + 1).arg( _imp->nodeResults.size() );
    _imp->resultLabel->setText(text);


    ++_imp->currentFindIndex;
}

void
FindNodeDialog::updateFindResultsWithCurrentFilter()
{
    updateFindResults( _imp->filter->text() );
}

void
FindNodeDialog::forceUpdateFindResults()
{
    _imp->currentFilter.clear();
    updateFindResultsWithCurrentFilter();
}

void
FindNodeDialog::onButtonClicked(QAbstractButton* button)
{
    if (button == (QAbstractButton*)_imp->nextButton) {
        QString filterText = _imp->filter->text();

        if (_imp->currentFilter != filterText) {
            updateFindResults(filterText);
        } else {
            selectNextResult();
        }
    }
}

void
FindNodeDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        selectNextResult();
        _imp->filter->setFocus();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
FindNodeDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange) {
        if ( !isActiveWindow() ) {
            reject();

            return;
        }
    }
    QDialog::changeEvent(e);
}

struct EditNodeNameDialogPrivate
{
    LineEdit* field;
    NodeGuiPtr node;

    EditNodeNameDialogPrivate(const NodeGuiPtr& node)
        : field(0)
        , node(node)
    {
    }
};

EditNodeNameDialog::EditNodeNameDialog(const NodeGuiPtr& node,
                                       QWidget* parent)
    : QDialog(parent)
    , _imp( new EditNodeNameDialogPrivate(node) )
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    _imp->field = new LineEdit(this);
    _imp->field->setText( QString::fromUtf8( node->getNode()->getLabel().c_str() ) );
    mainLayout->addWidget(_imp->field);
}

EditNodeNameDialog::~EditNodeNameDialog()
{
}

void
EditNodeNameDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange) {
        if ( !isActiveWindow() ) {
            reject();

            return;
        }
    }
    QDialog::changeEvent(e);
}

void
EditNodeNameDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

QString
EditNodeNameDialog::getTypedName() const
{
    return _imp->field->text();
}

NodeGuiPtr
EditNodeNameDialog::getNode() const
{
    return _imp->node;
}

void
NodeGraph::onNodeNameEditDialogFinished()
{
    EditNodeNameDialog* dialog = qobject_cast<EditNodeNameDialog*>( sender() );

    if (dialog) {
        QDialog::DialogCode code =  (QDialog::DialogCode)dialog->result();
        if (code == QDialog::Accepted) {
            QString newName = dialog->getTypedName();
            QString oldName = QString::fromUtf8( dialog->getNode()->getNode()->getLabel().c_str() );
            pushUndoCommand( new RenameNodeUndoRedoCommand(dialog->getNode(), oldName, newName) );
        }
        dialog->deleteLater();
    }
}

void
NodeGraph::extractSelectedNode()
{
    if ( !_imp->_selection.empty() ) {
        pushUndoCommand( new ExtractNodeUndoRedoCommand(this, _imp->_selection) );
    }
}

void
NodeGraph::createGroupFromSelection()
{
    if ( !_imp->_selection.empty() ) {
        pushUndoCommand( new GroupFromSelectionCommand(this, _imp->_selection) );
    }
}

void
NodeGraph::expandSelectedGroups()
{
    NodesGuiList nodes;

    for (NodesGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        NodeGroup* isGroup = (*it)->getNode()->isEffectGroup();
        if ( isGroup && (isGroup->getPluginID() == PLUGINID_NATRON_GROUP) ) {
            nodes.push_back(*it);
        }
    }
    if ( !nodes.empty() ) {
        pushUndoCommand( new InlineGroupCommand(this, nodes) );
    } else {
        Dialogs::warningDialog( tr("Expand group").toStdString(), tr("You must select a group to expand first").toStdString() );
    }
}

void
NodeGraph::onGroupNameChanged(const QString& /*name*/)
{
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( getGroup().get() );

    assert(isGrp);
    if (isGrp) {
        std::string label;
        makeFullyQualifiedLabel(isGrp->getNode().get(), &label);
        setLabel(label);
        TabWidget* parent = dynamic_cast<TabWidget*>( parentWidget() );
        if (parent) {
            parent->setTabLabel( this, QString::fromUtf8( label.c_str() ) );
        }
    }
}

void
NodeGraph::onGroupScriptNameChanged(const QString& /*name*/)
{
    assert( qApp && qApp->thread() == QThread::currentThread() );

    NodeCollectionPtr group = getGroup();
    if (!group) {
        return;
    }
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( group.get() );
    if (!isGrp) {
        return;
    }
    std::string newName = isGrp->getNode()->getFullyQualifiedName();
    for (std::size_t i = 0; i < newName.size(); ++i) {
        if (newName[i] == '.') {
            newName[i] = '_';
        }
    }
    std::string oldName = getScriptName();
    for (std::size_t i = 0; i < oldName.size(); ++i) {
        if (oldName[i] == '.') {
            oldName[i] = '_';
        }
    }
    getGui()->unregisterTab(this);
    setScriptName(newName);
    getGui()->registerTab(this, this);
    TabWidget* parent = dynamic_cast<TabWidget*>( parentWidget() );
    if (parent) {
        parent->onTabScriptNameChanged(this, oldName, newName);
    }
}

void
NodeGraph::copyNodesAndCreateInGroup(const NodesGuiList& nodes,
                                     const NodeCollectionPtr& group,
                                     std::list<std::pair<std::string, NodeGuiPtr> >& createdNodes)
{
    {
        CreatingNodeTreeFlag_RAII createNodeTree( getGui()->getApp() );
        NodeClipBoard clipboard;
        _imp->copyNodesInternal(nodes, clipboard);

        std::map<std::string, std::string> oldNewScriptNamesMapping;
        std::list<NodeSerializationPtr>::const_iterator itOther = clipboard.nodes.begin();
        for (std::list<NodeGuiSerializationPtr>::const_iterator it = clipboard.nodesUI.begin();
             it != clipboard.nodesUI.end(); ++it, ++itOther) {
            NodeGuiPtr node = _imp->pasteNode( *itOther, *it, QPointF(0, 0), group, std::string(), false, &oldNewScriptNamesMapping);
            assert(node);
            if (node) {
                oldNewScriptNamesMapping[(*itOther)->getNodeScriptName()] = node->getNode()->getScriptName();
                createdNodes.push_back( std::make_pair( (*itOther)->getNodeScriptName(), node ) );
            }
        }
        assert( clipboard.nodes.size() == createdNodes.size() );
        if ( clipboard.nodes.size() != createdNodes.size() ) {
            return;
        }

        ///Now that all nodes have been duplicated, try to restore nodes connections
        _imp->restoreConnections(clipboard.nodes, createdNodes, oldNewScriptNamesMapping);

        //Restore links once all children are created for alias knobs/expressions
        NodesList allNodes;
        group->getActiveNodes(&allNodes);
        // If the group is a Group node, append to all nodes reachable through links
        NodeGroup* isGroupNode = dynamic_cast<NodeGroup*>(group.get());
        if (isGroupNode) {
            allNodes.push_back(isGroupNode->getNode());
        }

        std::list<NodeSerializationPtr>::const_iterator itSerialization = clipboard.nodes.begin();
        for (std::list<std::pair<std::string, NodeGuiPtr> > ::iterator it = createdNodes.begin(); it != createdNodes.end(); ++it, ++itSerialization) {
            it->second->getNode()->restoreKnobsLinks(**itSerialization, allNodes, oldNewScriptNamesMapping);
        }

    }

    getGui()->getApp()->getProject()->forceComputeInputDependentDataOnAllTrees();
}

QPointF
NodeGraph::getRootPos() const
{
    return _imp->_root->pos();
}

NATRON_NAMESPACE_EXIT
