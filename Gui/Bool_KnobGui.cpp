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

#include "Bool_KnobGui.h"

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

//==========================BOOL_KNOB_GUI======================================

void
Bool_CheckBox::getBackgroundColor(double *r,double *g,double *b) const
{
    if (useCustomColor) {
        *r = customColor.redF();
        *g = customColor.greenF();
        *b = customColor.blueF();
    } else {
        
        AnimatedCheckBox::getBackgroundColor(r, g, b);
    }
}


Bool_KnobGui::Bool_KnobGui(boost::shared_ptr<KnobI> knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
    , _checkBox(0)
{
    _knob = boost::dynamic_pointer_cast<Bool_Knob>(knob);
}

void
Bool_KnobGui::createWidget(QHBoxLayout* layout)
{
    _checkBox = new Bool_CheckBox( layout->parentWidget() );
    onLabelChanged();
    //_checkBox->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect( _checkBox, SIGNAL( clicked(bool) ), this, SLOT( onCheckBoxStateChanged(bool) ) );
    QObject::connect( this, SIGNAL( labelClicked(bool) ), this, SLOT( onLabelClicked(bool) ) );

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_checkBox,0);

    layout->addWidget(_checkBox);
}

Bool_KnobGui::~Bool_KnobGui()
{

}

void Bool_KnobGui::removeSpecificGui()
{
    _checkBox->setParent(0);
    delete _checkBox;
}

void
Bool_KnobGui::updateGUI(int /*dimension*/)
{
    _checkBox->setChecked( _knob.lock()->getGuiValue(0) );
}

void
Bool_KnobGui::reflectAnimationLevel(int /*dimension*/,
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
    if (value != _checkBox->getAnimation()) {
        _checkBox->setAnimation(value);
    }
}

void
Bool_KnobGui::onLabelChanged()
{
    const std::string& label = _knob.lock()->getDescription();
    if (label == "R" || label == "r" || label == "red") {
        QColor color;
        color.setRgbF(0.851643,0.196936,0.196936);
        _checkBox->setCustomColor(color, true);
    } else if (label == "G" || label == "g" || label == "green") {
        QColor color;
        color.setRgbF(0,0.654707,0);
        _checkBox->setCustomColor(color, true);
    } else if (label == "B" || label == "b" || label == "blue") {
        QColor color;
        color.setRgbF(0.345293,0.345293,1);
        _checkBox->setCustomColor(color, true);
    } else if (label == "A" || label == "a" || label == "alpha") {
        QColor color;
        color.setRgbF(0.398979,0.398979,0.398979);
        _checkBox->setCustomColor(color, true);
    } else {
        _checkBox->setCustomColor(Qt::black, false);
    }
}

void
Bool_KnobGui::onLabelClicked(bool b)
{
    _checkBox->setChecked(b);
    pushUndoCommand( new KnobUndoCommand<bool>(this,_knob.lock()->getGuiValue(0),b, 0, false) );
}

void
Bool_KnobGui::onCheckBoxStateChanged(bool b)
{
    pushUndoCommand( new KnobUndoCommand<bool>(this,_knob.lock()->getGuiValue(0),b, 0, false) );
}

void
Bool_KnobGui::_hide()
{
    _checkBox->hide();
}

void
Bool_KnobGui::_show()
{
    _checkBox->show();
}

void
Bool_KnobGui::setEnabled()
{
    boost::shared_ptr<Bool_Knob> knob = _knob.lock();

    bool b = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    _checkBox->setEnabled(b);
}

void
Bool_KnobGui::setReadOnly(bool readOnly,
                          int /*dimension*/)
{
    _checkBox->setReadOnly(readOnly);
}

void
Bool_KnobGui::setDirty(bool dirty)
{
    _checkBox->setDirty(dirty);
}

boost::shared_ptr<KnobI>
Bool_KnobGui::getKnob() const
{
    return _knob.lock();
}

void
Bool_KnobGui::reflectExpressionState(int /*dimension*/,
                                     bool hasExpr)
{
    bool isSlaved = _knob.lock()->isSlave(0);
    _checkBox->setAnimation(3);
    _checkBox->setReadOnly(hasExpr || isSlaved);
}

void
Bool_KnobGui::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        for (int i = 0; i < _knob.lock()->getDimension(); ++i) {
            _checkBox->setToolTip( tt );
        }
    }
}

