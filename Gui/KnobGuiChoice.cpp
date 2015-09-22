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

#include "KnobGuiChoice.h"

#include <cfloat>
#include <algorithm> // min, max

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QDebug>
#include <QFontComboBox>
#include <QDialogButtonBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/Utils.h"

#include "ofxNatron.h"


using namespace Natron;
using std::make_pair;


//=============================CHOICE_KNOB_GUI===================================


KnobGuiChoice::KnobGuiChoice(boost::shared_ptr<KnobI> knob,
                               DockablePanel *container)
    : KnobGui(knob, container)
    , _comboBox(0)
{
    boost::shared_ptr<KnobChoice> k = boost::dynamic_pointer_cast<KnobChoice>(knob);
    _entries = k->getEntries_mt_safe();
    QObject::connect( k.get(), SIGNAL( populated() ), this, SLOT( onEntriesPopulated() ) );
    _knob = k;
}

KnobGuiChoice::~KnobGuiChoice()
{
   
}

void KnobGuiChoice::removeSpecificGui()
{
    delete _comboBox;
}

void
KnobGuiChoice::createWidget(QHBoxLayout* layout)
{
    _comboBox = new ComboBox( layout->parentWidget() );
    _comboBox->setCascading(_knob.lock()->isCascading());
    onEntriesPopulated();

    QObject::connect( _comboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( onCurrentIndexChanged(int) ) );
    QObject::connect( _comboBox, SIGNAL(itemNewSelected()), this, SLOT(onItemNewSelected()));
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_comboBox,0);

    layout->addWidget(_comboBox);
}

void
KnobGuiChoice::onCurrentIndexChanged(int i)
{
    pushUndoCommand( new KnobUndoCommand<int>(this,_knob.lock()->getValue(0),i, 0, false, 0) );
}

void
KnobGuiChoice::onEntriesPopulated()
{
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    int activeIndex = knob->getValue();

    _comboBox->clear();
    _entries = knob->getEntries_mt_safe();
    const std::vector<std::string> help =  knob->getEntriesHelp_mt_safe();
    for (U32 i = 0; i < _entries.size(); ++i) {
        std::string helpStr;
        if ( !help.empty() && !help[i].empty() ) {
            helpStr = help[i];
        }
        _comboBox->addItem( _entries[i].c_str(), QIcon(), QKeySequence(), QString( helpStr.c_str() ) );
    }
    // the "New" menu is only added to known parameters (e.g. the choice of output channels)
    if (knob->getHostCanAddOptions() &&
        (knob->getName() == kNatronOfxParamOutputChannels || knob->getName() == kOutputChannelsKnobName)) {
        _comboBox->addItemNew();
    }
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///we don't want that to happen because the index actually didn't change.
    _comboBox->setCurrentIndex_no_emit(activeIndex);

    updateToolTip();
}

void
KnobGuiChoice::onItemNewSelected()
{
    NewLayerDialog dialog(getGui());
    if (dialog.exec()) {
        Natron::ImageComponents comps = dialog.getComponents();
        if (comps == Natron::ImageComponents::getNoneComponents()) {
            Natron::errorDialog(tr("Layer").toStdString(), tr("Invalid layer").toStdString());
            return;
        }
        KnobHolder* holder = _knob.lock()->getHolder();
        assert(holder);
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        assert(effect);
        assert(effect->getNode());
        if (!effect->getNode()->addUserComponents(comps)) {
            Natron::errorDialog(tr("Layer").toStdString(), tr("A Layer with the same name already exists").toStdString());
        }
    }
}

void
KnobGuiChoice::reflectExpressionState(int /*dimension*/,
                                       bool hasExpr)
{
    _comboBox->setAnimation(3);
    bool isEnabled = _knob.lock()->isEnabled(0);
    _comboBox->setEnabled_natron(!hasExpr && isEnabled);
}

void
KnobGuiChoice::updateToolTip()
{
    QString tt = toolTip();
    _comboBox->setToolTip( tt );
}


void
KnobGuiChoice::updateGUI(int /*dimension*/)
{
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///change the internal value of the knob again...
    ///The slot connected to onCurrentIndexChanged is reserved to catch user interaction with the combobox.
    ///This function is called in response to an internal change.
    _comboBox->setCurrentIndex_no_emit( _knob.lock()->getValue(0) );
}

void
KnobGuiChoice::reflectAnimationLevel(int /*dimension*/,
                                      Natron::AnimationLevelEnum level)
{
    int value;
    switch (level) {
    case Natron::eAnimationLevelNone:
            value = 0;
        break;
    case Natron::eAnimationLevelInterpolatedValue:
            value = 1;
        break;
    case Natron::eAnimationLevelOnKeyframe:
            value = 2;
        break;
    default:
            value = 0;
        break;
    }
    if (value != _comboBox->getAnimation()) {
        _comboBox->setAnimation(value);
    }
}

void
KnobGuiChoice::_hide()
{
    _comboBox->hide();
}

void
KnobGuiChoice::_show()
{
    _comboBox->show();
}

void
KnobGuiChoice::setEnabled()
{
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    bool b = knob->isEnabled(0) && knob->getExpression(0).empty();

    _comboBox->setEnabled_natron(b);
}

void
KnobGuiChoice::setReadOnly(bool readOnly,
                            int /*dimension*/)
{
    _comboBox->setEnabled_natron(!readOnly);
}

void
KnobGuiChoice::setDirty(bool dirty)
{
    _comboBox->setDirty(dirty);
}

boost::shared_ptr<KnobI> KnobGuiChoice::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiChoice::reflectModificationsState()
{
    bool hasModif = _knob.lock()->hasModifications();
    _comboBox->setAltered(!hasModif);
}

