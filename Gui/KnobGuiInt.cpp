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

#include "KnobGuiInt.h"

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
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/SpinBoxValidator.h"
#include "Gui/TabGroup.h"
#include "Gui/Utils.h"

#include "ofxNatron.h"


using namespace Natron;
using std::make_pair;


//==========================INT_KNOB_GUI======================================
KnobGuiInt::KnobGuiInt(boost::shared_ptr<KnobI> knob,
                         DockablePanel *container)
    : KnobGui(knob, container)
      , _container(0)
      , _slider(0)
      , _dimensionSwitchButton(0)
{
    boost::shared_ptr<KnobInt> iKnob = boost::dynamic_pointer_cast<KnobInt>(knob);
    assert(iKnob);
    boost::shared_ptr<KnobSignalSlotHandler> handler = iKnob->getSignalSlotHandler();
    if (handler) {
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        QObject::connect( handler.get(), SIGNAL( minMaxChanged(double, double, int) ), this, SLOT( onMinMaxChanged(double, double, int) ) );
#endif
        
        QObject::connect( handler.get(), SIGNAL( displayMinMaxChanged(double, double, int) ), this, SLOT( onDisplayMinMaxChanged(double, double, int) ) );
    }
    QObject::connect( iKnob.get(), SIGNAL( incrementChanged(int, int) ), this, SLOT( onIncrementChanged(int, int) ) );
    _knob = iKnob;
}

KnobGuiInt::~KnobGuiInt()
{
}

void
KnobGuiInt::removeSpecificGui()
{
    _container->setParent(NULL);
    delete _container;
    _spinBoxes.clear();
}

void
KnobGuiInt::createWidget(QHBoxLayout* layout)
{
    
    boost::shared_ptr<KnobInt> knob = _knob.lock();
    int dim = knob->getDimension();
    _container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(_container);

    _container->setLayout(containerLayout);
    containerLayout->setSpacing(3);
    containerLayout->setContentsMargins(0, 0, 0, 0);


    if (getKnobsCountOnSameLine() > 1) {
        knob->disableSlider();
    }

    if (!knob->isSliderDisabled()) {
        layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    
    //  const std::vector<int> &maximums = intKnob->getMaximums();
    //    const std::vector<int> &minimums = intKnob->getMinimums();
    const std::vector<int> &increments = knob->getIncrements();
    const std::vector<int> &displayMins = knob->getDisplayMinimums();
    const std::vector<int> &displayMaxs = knob->getDisplayMaximums();
    
    const std::vector<int > &mins = knob->getMinimums();
    const std::vector<int > &maxs = knob->getMaximums();
    
    for (int i = 0; i < dim; ++i) {

        QWidget *boxContainer = new QWidget( _container );
        boxContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QHBoxLayout *boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        boxContainerLayout->setSpacing(3);
        Natron::Label *subDesc = 0;
        if (dim != 1) {
            std::string dimLabel = getKnob()->getDimensionName(i);
            if (!dimLabel.empty()) {
                dimLabel.append(":");
            }
            subDesc = new Natron::Label(QString(dimLabel.c_str()), boxContainer);
            //subDesc->setFont( QFont(appFont,appFontSize) );
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox *box = new SpinBox(layout->parentWidget(), SpinBox::eSpinBoxTypeInt);
        NumericKnobValidator* validator = new NumericKnobValidator(box,this);
        box->setValidator(validator);
        QObject::connect( box, SIGNAL( valueChanged(double) ), this, SLOT( onSpinBoxValueChanged() ) );

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(box,i);

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        int min = mins[i];
        int max = maxs[i];
        box->setMaximum(max);
        box->setMinimum(min);
#endif
        
        box->setIncrement(increments[i]);

        boxContainerLayout->addWidget(box);
        containerLayout->addWidget(boxContainer);
        _spinBoxes.push_back( make_pair(box, subDesc) );
    }
    
    bool sliderVisible = false;
    if (!knob->isSliderDisabled()) {
        int dispmin = displayMins[0];
        int dispmax = displayMaxs[0];
        if (dispmin == -DBL_MAX) {
            dispmin = mins[0];
        }
        if (dispmax == DBL_MAX) {
            dispmax = maxs[0];
        }
        
        _slider = new ScaleSliderQWidget( dispmin, dispmax,knob->getValue(0),
                                         ScaleSliderQWidget::eDataTypeInt,getGui(),Natron::eScaleTypeLinear, layout->parentWidget() );
        _slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        if ( hasToolTip() ) {
            _slider->setToolTip( toolTip() );
        }
        QObject::connect( _slider, SIGNAL( positionChanged(double) ), this, SLOT( onSliderValueChanged(double) ) );
        QObject::connect( _slider, SIGNAL( editingFinished(bool) ), this, SLOT( onSliderEditingFinished(bool) ) );
        
        containerLayout->addWidget(_slider);
        onDisplayMinMaxChanged(dispmin, dispmax);
        sliderVisible = shouldSliderBeVisible(dispmin, dispmax);
    }

    if (dim > 1 && !knob->isSliderDisabled() && sliderVisible) {
        _dimensionSwitchButton = new Button(QIcon(),QString::number(dim),_container);
        _dimensionSwitchButton->setToolTip(Natron::convertFromPlainText(tr("Switch between a single value for all dimensions and multiple values."), Qt::WhiteSpaceNormal));
        _dimensionSwitchButton->setFocusPolicy(Qt::NoFocus);
        _dimensionSwitchButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _dimensionSwitchButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
        _dimensionSwitchButton->setCheckable(true);
        containerLayout->addWidget(_dimensionSwitchButton);
        
        bool showSlider = true;
        double firstDimensionValue = knob->getValue(0);
        for (int i = 0; i < dim ; ++i) {
            if (knob->getValue(i) != firstDimensionValue) {
                showSlider = false;
                break;
            }
        }
        if (!showSlider) {
            _slider->hide();
            _dimensionSwitchButton->setChecked(true);
        } else {
            foldAllDimensions();
        }
        
        QObject::connect( _dimensionSwitchButton, SIGNAL( clicked(bool) ), this, SLOT( onDimensionSwitchClicked() ) );
        
    }

    
    layout->addWidget(_container);
} // createWidget

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
void
KnobGuiInt::onMinMaxChanged(double mini,
                             double maxi,
                             int index)
{
    
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
}
#endif

void
KnobGuiInt::onDimensionSwitchClicked()
{
    if (!_dimensionSwitchButton) {
        return;
    }
    if (_dimensionSwitchButton->isChecked() ) {
        expandAllDimensions();
    } else {
        foldAllDimensions();
        boost::shared_ptr<KnobInt> knob = _knob.lock();
        int dim = knob->getDimension();
        if (dim > 1) {
            double value(_spinBoxes[0].first->value());
            knob->beginChanges();
            for (int i = 1; i < dim; ++i) {
                knob->setValue(value,i);
            }
            knob->endChanges();
        }
    }
    
}

bool
KnobGuiInt::shouldAddStretch() const
{
    return _knob.lock()->isSliderDisabled();
}

bool
KnobGuiInt::getAllDimensionsVisible() const
{
    return !_dimensionSwitchButton || _dimensionSwitchButton->isChecked();
}

int
KnobGuiInt::getDimensionForSpinBox(const SpinBox* spinbox) const
{
    for (std::size_t i = 0; i < _spinBoxes.size(); ++i) {
        if (_spinBoxes[i].first == spinbox) {
            return (int)i;
        }
    }
    return -1;
}

void
KnobGuiInt::expandAllDimensions()
{
    if (!_dimensionSwitchButton) {
        return;
    }
    _dimensionSwitchButton->setChecked(true);
    _dimensionSwitchButton->setDown(true);
    _slider->hide();
    int dims = _knob.lock()->getDimension();
    for (int i = 0; i < dims; ++i) {
        if (i > 0) {
            _spinBoxes[i].first->show();
        }
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->show();
        }
    }
}

void
KnobGuiInt::foldAllDimensions()
{
    if (!_dimensionSwitchButton) {
        return;
    }
    _dimensionSwitchButton->setChecked(false);
    _dimensionSwitchButton->setDown(false);
    if (_spinBoxes[0].second) {
        _spinBoxes[0].second->hide();
    }
    _slider->show();
    int dims = _knob.lock()->getDimension();
    for (int i = 0; i < dims; ++i) {
        if (i > 0) {
            _spinBoxes[i].first->hide();
        }
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->hide();
        }
    }
}


void
KnobGuiInt::onDisplayMinMaxChanged(double mini,
                                    double maxi,
                                    int index)
{
    if (_slider) {
        double sliderMin = mini;
        double sliderMax = maxi;
        if ( (sliderMax - sliderMin) >= SLIDER_MAX_RANGE ) {
            
            boost::shared_ptr<KnobInt> knob = _knob.lock();
            ///use min max for slider if dispmin/dispmax was not set
            assert(index < (int)knob->getMinimums().size() && index < (int)knob->getMaximums().size());
            int max = knob->getMaximums()[index];
            int min = knob->getMinimums()[index];
            if ( (max - min) < SLIDER_MAX_RANGE ) {
                sliderMin = min;
                sliderMax = max;
            }
        }
        if (shouldSliderBeVisible(sliderMin,sliderMax)) {
            _slider->show();
        } else {
            _slider->hide();
        }
        
        _slider->setMinimumAndMaximum(sliderMin ,sliderMax);
    }
}

void
KnobGuiInt::onIncrementChanged(int incr,
                                int index)
{
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setIncrement(incr);
}

void
KnobGuiInt::updateGUI(int dimension)
{
    boost::shared_ptr<KnobInt> knob = _knob.lock();
    
    assert(dimension == -1 || (dimension >= 0 && dimension < knob->getDimension()));
    
    int values[3];
    int refValue;
    if (dimension == -1) {
        values[0] = knob->getValue(0);
        refValue = values[0];
    } else {
        values[dimension] = knob->getValue(dimension);
        refValue = values[dimension];
    }
    bool allValuesDifferent = false;
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if ((dimension != -1 && i == dimension) || (dimension == -1 && i == 0)) {
            ///Already processed
            continue;
        }
        values[i] = knob->getValue(i);
        if (values[i] != refValue) {
            allValuesDifferent = true;
        }
    }
    if (_dimensionSwitchButton && !_dimensionSwitchButton->isChecked() && allValuesDifferent) {
        expandAllDimensions();
    } else if (_dimensionSwitchButton && _dimensionSwitchButton->isChecked() && !allValuesDifferent) {
        foldAllDimensions();
    }
    
    if (_slider) {
        _slider->seekScalePosition(values[0]);
    }
    
    
    if (dimension != -1) {
        switch (dimension) {
            case 0:
                _spinBoxes[dimension].first->setValue(values[dimension]);
                if (_dimensionSwitchButton && !_dimensionSwitchButton->isChecked()) {
                    for (int i = 1; i < knob->getDimension(); ++i) {
                        _spinBoxes[i].first->setValue(values[dimension]);
                    }
                }
                break;
            case 1:
            case 2:
                _spinBoxes[dimension].first->setValue(values[dimension]);
                break;
            default:
                break;
        }
        
    } else {
        _spinBoxes[0].first->setValue(values[0]);
        if (_dimensionSwitchButton && !_dimensionSwitchButton->isChecked()) {
            for (int i = 1; i < knob->getDimension(); ++i) {
                _spinBoxes[i].first->setValue(values[0]);
            }
        } else {
            for (int i = 1; i < knob->getDimension(); ++i) {
                _spinBoxes[i].first->setValue(values[i]);
            }
        }
    }
}

void
KnobGuiInt::reflectAnimationLevel(int dimension,
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
    if (value != _spinBoxes[dimension].first->getAnimation()) {
        _spinBoxes[dimension].first->setAnimation(value);
    }
}

void
KnobGuiInt::onSliderValueChanged(double d)
{
    assert(_knob.lock()->isEnabled(0));
    bool penUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    if (penUpOnly) {
        return;
    }
    sliderEditingEnd(d);
}

void
KnobGuiInt::onSliderEditingFinished(bool hasMovedOnce)
{
    assert(_knob.lock()->isEnabled(0));
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    bool onEditingFinishedOnly = settings->getRenderOnEditingFinishedOnly();
    bool autoProxyEnabled = settings->isAutoProxyEnabled();
    if (onEditingFinishedOnly) {
        double v = _slider->getPosition();
        sliderEditingEnd(v);
    } else if (autoProxyEnabled && hasMovedOnce) {
        getGui()->renderAllViewers(true);
    }
}

void
KnobGuiInt::sliderEditingEnd(double d)
{
    boost::shared_ptr<KnobInt> knob = _knob.lock();

    if (_dimensionSwitchButton) {
        int dims = knob->getDimension();
        for (int i = 0; i < dims; ++i) {
            _spinBoxes[i].first->setValue(d);
        }
        std::list<int> oldValues,newValues;
        for (int i = 0; i < dims; ++i) {
            oldValues.push_back(knob->getValue(i));
            newValues.push_back(d);
        }
        pushUndoCommand( new KnobUndoCommand<int>(this,oldValues,newValues,false) );
    } else {
        _spinBoxes[0].first->setValue(d);
        pushUndoCommand( new KnobUndoCommand<int>(this,knob->getValue(0),d,0,false) );
    }
    
}

void
KnobGuiInt::onSpinBoxValueChanged()
{
    std::list<int> newValues;

    if (!_dimensionSwitchButton || _dimensionSwitchButton->isChecked() ) {
        // each spinbox has a different value
        for (U32 i = 0; i < _spinBoxes.size(); ++i) {
            newValues.push_back( _spinBoxes[i].first->value() );
        }
    } else {
        // use the value of the first dimension only, and set all spinboxes
        if (_spinBoxes.size() > 1) {
            int v = _spinBoxes[0].first->value();
            newValues.push_back(v);
            for (U32 i = 1; i < _spinBoxes.size(); ++i) {
                newValues.push_back(v);
                _spinBoxes[i].first->setValue(v);
            }
        }
    }

    if (_slider) {
        _slider->seekScalePosition( newValues.front() );
    }
    pushUndoCommand( new KnobUndoCommand<int>(this,_knob.lock()->getValueForEachDimension_mt_safe(),newValues,false) );
}

void
KnobGuiInt::_hide()
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->hide();
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->hide();
        }
    }
    if (_slider) {
        _slider->hide();
    }
    if (_dimensionSwitchButton) {
        _dimensionSwitchButton->hide();
    }
}

void
KnobGuiInt::_show()
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        
        if (!_dimensionSwitchButton || (i > 0 && _dimensionSwitchButton->isChecked()) || (i == 0)) {
            _spinBoxes[i].first->show();
        }
        if (!_dimensionSwitchButton || _dimensionSwitchButton->isChecked()) {
            if (_spinBoxes[i].second) {
                _spinBoxes[i].second->show();
            }
        }
    }
    if (_slider && (!_dimensionSwitchButton || (_dimensionSwitchButton && !_dimensionSwitchButton->isChecked()))) {
        double sliderMax = _slider->maximum();
        double sliderMin = _slider->minimum();
        
        if ( (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < INT_MAX) && (sliderMin > INT_MIN) ) {
            _slider->show();
        }
    }
    if (_dimensionSwitchButton) {
        _dimensionSwitchButton->show();
    }

}

void
KnobGuiInt::setEnabled()
{
    boost::shared_ptr<KnobInt> knob = _knob.lock();

    bool enabled0 = knob->isEnabled(0) && !knob->isSlave(0) && knob->getExpression(0).empty();
    
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        bool b = knob->isEnabled(i) && !knob->isSlave(i) && knob->getExpression(i).empty();
        //_spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->setEnabled(b);
        }
    }
    if (_slider) {
        _slider->setReadOnly( !enabled0 );
    }
    
    if (_dimensionSwitchButton) {
        _dimensionSwitchButton->setEnabled(enabled0);
    }
}

void
KnobGuiInt::setReadOnly(bool readOnly,
                         int dimension)
{
    assert( dimension < (int)_spinBoxes.size() );
    _spinBoxes[dimension].first->setReadOnly(readOnly);
    if ( _slider && (dimension == 0) ) {
        _slider->setReadOnly(readOnly);
    }
}

void
KnobGuiInt::setDirty(bool dirty)
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->setDirty(dirty);
    }
}

boost::shared_ptr<KnobI> KnobGuiInt::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiInt::reflectExpressionState(int dimension,
                                    bool hasExpr)
{
    boost::shared_ptr<KnobInt> knob = _knob.lock();

    if (hasExpr) {
        _spinBoxes[dimension].first->setAnimation(3);
        _spinBoxes[dimension].first->setReadOnly(true);
        if (_slider) {
            _slider->setReadOnly(true);
        }
    } else {
        Natron::AnimationLevelEnum lvl = knob->getAnimationLevel(dimension);
        _spinBoxes[dimension].first->setAnimation((int)lvl);
        bool isEnabled = knob->isEnabled(dimension);
        _spinBoxes[dimension].first->setReadOnly(!isEnabled);
        if (_slider) {
            bool isEnabled0 = knob->isEnabled(0);
            _slider->setReadOnly(!isEnabled0);
        }
    }
}

void
KnobGuiInt::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        boost::shared_ptr<KnobInt> knob = _knob.lock();

        for (int i = 0; i < knob->getDimension(); ++i) {
            _spinBoxes[i].first->setToolTip( tt );
        }
        if (_slider) {
            _slider->setToolTip(tt);
        }
    }
}

void
KnobGuiInt::reflectModificationsState()
{
    bool hasModif = _knob.lock()->hasModifications();
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->setAltered(!hasModif);
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->setAltered(!hasModif);
        }
    }
    if (_slider) {
        _slider->setAltered(!hasModif);
    }
}
