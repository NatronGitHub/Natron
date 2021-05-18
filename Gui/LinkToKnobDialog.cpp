/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "LinkToKnobDialog.h"

#include <cassert>
#include <climits>
#include <cfloat>
#include <stdexcept>

#include <boost/weak_ptr.hpp>


#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QtCore/QTimer>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QCompleter>

#include "Global/GlobalDefines.h"

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/Variant.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveGui.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/Menu.h"
#include "Gui/Menu.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ScriptTextEdit.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER

struct LinkToKnobDialogPrivate
{
    KnobGuiPtr fromKnob;
    QVBoxLayout* mainLayout;
    QHBoxLayout* firstLineLayout;
    QWidget* firstLine;
    Label* selectNodeLabel;
    CompleterLineEdit* nodeSelectionCombo;
    ComboBox* knobSelectionCombo;
    DialogButtonBox* buttons;
    NodesList allNodes;
    std::map<QString, KnobIPtr> allKnobs;

    LinkToKnobDialogPrivate(const KnobGuiPtr& from)
        : fromKnob(from)
        , mainLayout(0)
        , firstLineLayout(0)
        , firstLine(0)
        , selectNodeLabel(0)
        , nodeSelectionCombo(0)
        , knobSelectionCombo(0)
        , buttons(0)
        , allNodes()
        , allKnobs()
    {
    }
};

LinkToKnobDialog::LinkToKnobDialog(const KnobGuiPtr& from,
                                   QWidget* parent)
    : QDialog(parent)
    , _imp( new LinkToKnobDialogPrivate(from) )
{
    _imp->mainLayout = new QVBoxLayout(this);

    _imp->firstLine = new QWidget(this);
    _imp->firstLineLayout = new QHBoxLayout(_imp->firstLine);

    _imp->mainLayout->addWidget(_imp->firstLine);

    _imp->buttons = new DialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal, this);
    QObject::connect( _imp->buttons, SIGNAL(accepted()), this, SLOT(accept()) );
    QObject::connect( _imp->buttons, SIGNAL(rejected()), this, SLOT(reject()) );
    _imp->mainLayout->addWidget(_imp->buttons);

    _imp->selectNodeLabel = new Label(tr("Parent:"), _imp->firstLine);
    _imp->firstLineLayout->addWidget(_imp->selectNodeLabel);


    EffectInstance* isEffect = dynamic_cast<EffectInstance*>( from->getKnob()->getHolder() );
    assert(isEffect);
    if (!isEffect) {
        throw std::logic_error("");
    }
    NodeCollectionPtr group = isEffect->getNode()->getGroup();
    group->getActiveNodes(&_imp->allNodes);
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( group.get() );
    if (isGroup) {
        _imp->allNodes.push_back( isGroup->getNode() );
    }
    QStringList nodeNames;
    for (NodesList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        QString name = QString::fromUtf8( (*it)->getLabel().c_str() );
        nodeNames.push_back(name);
        //_imp->nodeSelectionCombo->addItem(name);
    }
    nodeNames.sort();
    _imp->nodeSelectionCombo = new CompleterLineEdit(nodeNames, nodeNames, false, this);
    _imp->nodeSelectionCombo->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Input the name of a node in the current project."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->firstLineLayout->addWidget(_imp->nodeSelectionCombo);


    _imp->nodeSelectionCombo->setFocus(Qt::PopupFocusReason);
    QTimer::singleShot( 25, _imp->nodeSelectionCombo, SLOT(showCompleter()) );

    _imp->knobSelectionCombo = new ComboBox(_imp->firstLine);
    _imp->firstLineLayout->addWidget(_imp->knobSelectionCombo);

    QObject::connect( _imp->nodeSelectionCombo, SIGNAL(itemCompletionChosen()), this, SLOT(onNodeComboEditingFinished()) );

    _imp->firstLineLayout->addStretch();
}

LinkToKnobDialog::~LinkToKnobDialog()
{
}

void
LinkToKnobDialog::onNodeComboEditingFinished()
{
    QString index = _imp->nodeSelectionCombo->text();

    _imp->knobSelectionCombo->clear();
    NodePtr selectedNode;
    std::string currentNodeName = index.toStdString();
    for (NodesList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        if ( (*it)->getLabel() == currentNodeName ) {
            selectedNode = *it;
            break;
        }
    }
    if (!selectedNode) {
        return;
    }

    const std::vector<KnobIPtr> & knobs = selectedNode->getKnobs();
    KnobIPtr from = _imp->fromKnob->getKnob();
    for (U32 j = 0; j < knobs.size(); ++j) {
        if ( !knobs[j]->getIsSecret() && (knobs[j] != from) ) {
            KnobButton* isButton = dynamic_cast<KnobButton*>( knobs[j].get() );
            KnobPage* isPage = dynamic_cast<KnobPage*>( knobs[j].get() );
            KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knobs[j].get() );
            if (from->isTypeCompatible(knobs[j]) && !isButton && !isPage && !isGroup) {
                QString name = QString::fromUtf8( knobs[j]->getName().c_str() );
                bool canInsertKnob = true;
                for (int k = 0; k < knobs[j]->getDimension(); ++k) {
                    if ( knobs[j]->isSlave(k) || !knobs[j]->isEnabled(k) || name.isEmpty() ) {
                        canInsertKnob = false;
                    }
                }
                if (canInsertKnob) {
                    _imp->allKnobs.insert( std::make_pair( name, knobs[j]) );
                    _imp->knobSelectionCombo->addItem(name);
                }
            }
        }
    }
}

KnobIPtr
LinkToKnobDialog::getSelectedKnobs() const
{
    QString str = _imp->knobSelectionCombo->itemText( _imp->knobSelectionCombo->activeIndex() );
    std::map<QString, KnobIPtr>::const_iterator it = _imp->allKnobs.find(str);

    if ( it != _imp->allKnobs.end() ) {
        return it->second;
    } else {
        return KnobIPtr();
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_LinkToKnobDialog.cpp"
