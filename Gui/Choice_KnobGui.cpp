//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Choice_KnobGui.h"

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


Choice_KnobGui::Choice_KnobGui(boost::shared_ptr<KnobI> knob,
                               DockablePanel *container)
    : KnobGui(knob, container)
    , _comboBox(0)
{
    boost::shared_ptr<Choice_Knob> k = boost::dynamic_pointer_cast<Choice_Knob>(knob);
    _entries = k->getEntries_mt_safe();
    QObject::connect( k.get(), SIGNAL( populated() ), this, SLOT( onEntriesPopulated() ) );
    _knob = k;
}

Choice_KnobGui::~Choice_KnobGui()
{
   
}

void Choice_KnobGui::removeSpecificGui()
{
    delete _comboBox;
}

void
Choice_KnobGui::createWidget(QHBoxLayout* layout)
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
Choice_KnobGui::onCurrentIndexChanged(int i)
{
    pushUndoCommand( new KnobUndoCommand<int>(this,_knob.lock()->getGuiValue(0),i, 0, false, 0) );
}

void
Choice_KnobGui::onEntriesPopulated()
{
    boost::shared_ptr<Choice_Knob> knob = _knob.lock();
    int activeIndex = knob->getGuiValue();

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
Choice_KnobGui::onItemNewSelected()
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
        effect->getNode()->addUserComponents(comps);
    }
}

void
Choice_KnobGui::reflectExpressionState(int /*dimension*/,
                                       bool hasExpr)
{
    _comboBox->setAnimation(3);
    bool isSlaved = _knob.lock()->isSlave(0);
    _comboBox->setReadOnly(hasExpr || isSlaved);
}

void
Choice_KnobGui::updateToolTip()
{
    QString tt = toolTip();
    _comboBox->setToolTip( tt );
}


void
Choice_KnobGui::updateGUI(int /*dimension*/)
{
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///change the internal value of the knob again...
    ///The slot connected to onCurrentIndexChanged is reserved to catch user interaction with the combobox.
    ///This function is called in response to an internal change.
    _comboBox->setCurrentIndex_no_emit( _knob.lock()->getGuiValue(0) );
}

void
Choice_KnobGui::reflectAnimationLevel(int /*dimension*/,
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
Choice_KnobGui::_hide()
{
    _comboBox->hide();
}

void
Choice_KnobGui::_show()
{
    _comboBox->show();
}

void
Choice_KnobGui::setEnabled()
{
    boost::shared_ptr<Choice_Knob> knob = _knob.lock();
    bool b = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    _comboBox->setEnabled_natron(b);
}

void
Choice_KnobGui::setReadOnly(bool readOnly,
                            int /*dimension*/)
{
    _comboBox->setReadOnly(readOnly);
}

void
Choice_KnobGui::setDirty(bool dirty)
{
    _comboBox->setDirty(dirty);
}

boost::shared_ptr<KnobI> Choice_KnobGui::getKnob() const
{
    return _knob.lock();
}

void
Choice_KnobGui::reflectModificationsState()
{
    bool hasModif = _knob.lock()->hasModifications();
    _comboBox->setAltered(!hasModif);
}

