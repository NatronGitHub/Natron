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

#include "Double_KnobGui.h"

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
#include "Gui/TabGroup.h"
#include "Gui/Utils.h"

#include "ofxNatron.h"

using namespace Natron;
using std::make_pair;


//=============================DOUBLE_KNOB_GUI===================================

/*
   FROM THE OFX SPEC:

 * kOfxParamCoordinatesNormalised is OFX > 1.2 and is used ONLY for setting defaults

   These new parameter types can set their defaults in one of two coordinate systems, the property kOfxParamPropDefaultCoordinateSystem. Specifies the coordinate system the default value is being specified in.

 * kOfxParamDoubleTypeNormalized* is OFX < 1.2 and is used ONLY for displaying the value: get/set should always return the normalized value:

   To flag to the host that a parameter as normalised, we use the kOfxParamPropDoubleType property. Parameters that are so flagged have values set and retrieved by an effect in normalized coordinates. However a host can choose to represent them to the user in whatever space it chooses.

   Both properties can be easily supported:
   - the first one is used ONLY when setting the *DEFAULT* value
   - the second is used ONLY by the GUI
 */
void
Double_KnobGui::valueAccordingToType(bool normalize,
                                     int dimension,
                                     double* value)
{
    if ( (dimension != 0) && (dimension != 1) ) {
        return;
    }
    
    Double_Knob::NormalizedStateEnum state = _knob.lock()->getNormalizedState(dimension);
    if (state == Double_Knob::eNormalizedStateX) {
        Format f;
        getKnob()->getHolder()->getApp()->getProject()->getProjectDefaultFormat(&f);
        if (normalize) {
            *value /= f.width();
        } else {
            *value *= f.width();
        }
    } else if (state == Double_Knob::eNormalizedStateY) {
        Format f;
        getKnob()->getHolder()->getApp()->getProject()->getProjectDefaultFormat(&f);
        if (normalize) {
            *value /= f.height();
        } else {
            *value *= f.height();
        }
    }
    
}

bool
Double_KnobGui::shouldAddStretch() const
{
    return _knob.lock()->isSliderDisabled();
}

Double_KnobGui::Double_KnobGui(boost::shared_ptr<KnobI> knob,
                               DockablePanel *container)
: KnobGui(knob, container)
, _container(0)
, _slider(0)
, _dimensionSwitchButton(0)
{
    boost::shared_ptr<Double_Knob> k = boost::dynamic_pointer_cast<Double_Knob>(knob);
    assert(k);
    boost::shared_ptr<KnobSignalSlotHandler> handler = k->getSignalSlotHandler();
    if (handler) {
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        QObject::connect( handler.get(), SIGNAL( minMaxChanged(double, double, int) ), this, SLOT( onMinMaxChanged(double, double, int) ) );
#endif
        QObject::connect( handler.get(), SIGNAL( displayMinMaxChanged(double, double, int) ), this, SLOT( onDisplayMinMaxChanged(double, double, int) ) );
    }
    QObject::connect( k.get(), SIGNAL( incrementChanged(double, int) ), this, SLOT( onIncrementChanged(double, int) ) );
    QObject::connect( k.get(), SIGNAL( decimalsChanged(int, int) ), this, SLOT( onDecimalsChanged(int, int) ) );
    _knob = k;
}

Double_KnobGui::~Double_KnobGui()
{

}

void Double_KnobGui::removeSpecificGui()
{
    delete _container;
    _spinBoxes.clear();
}

void
Double_KnobGui::createWidget(QHBoxLayout* layout)
{

    _container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(_container);
    layout->addWidget(_container);

    _container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(3);

    boost::shared_ptr<Double_Knob> knob = _knob.lock();

    if (getKnobsCountOnSameLine() > 1) {
        knob->disableSlider();
    }
    
    if (!knob->isSliderDisabled()) {
        layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    int dim = getKnob()->getDimension();


    //  const std::vector<double> &maximums = dbl_knob->getMaximums();
    //    const std::vector<double> &minimums = dbl_knob->getMinimums();
    const std::vector<double> &increments = knob->getIncrements();
    const std::vector<double> &displayMins = knob->getDisplayMinimums();
    const std::vector<double> &displayMaxs = knob->getDisplayMaximums();
    const std::vector<double > & mins = knob->getMinimums();
    const std::vector<double > & maxs = knob->getMaximums();
    const std::vector<int> &decimals = knob->getDecimals();
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
        SpinBox *box = new SpinBox(layout->parentWidget(), SpinBox::eSpinBoxTypeDouble);
        QObject::connect( box, SIGNAL( valueChanged(double) ), this, SLOT( onSpinBoxValueChanged() ) );
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(box,i);

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        double min = mins[i];
        double max = maxs[i];
        valueAccordingToType(false, i, &min);
        valueAccordingToType(false, i, &max);
        box->setMaximum(max);
        box->setMinimum(min);
#endif
        double incr = increments[i];
        valueAccordingToType(false, i, &incr);
        ///set the number of digits after the decimal point
        box->decimals(decimals[i]);
        
        box->setIncrement(increments[i]);
       
        boxContainerLayout->addWidget(box);

        containerLayout->addWidget(boxContainer);
        _spinBoxes.push_back( make_pair(box, subDesc) );
    }
    
    bool sliderVisible = false;
    if ( !knob->isSliderDisabled()) {
        double dispmin = displayMins[0];
        double dispmax = displayMaxs[0];
        if (dispmin == -DBL_MAX) {
            dispmin = mins[0];
        }
        if (dispmax == DBL_MAX) {
            dispmax = maxs[0];
        }
        valueAccordingToType(false, 0, &dispmin);
        valueAccordingToType(false, 0, &dispmax);
        
        bool spatial = knob->getIsSpatial();
        Format f;
        if (spatial) {
            getKnob()->getHolder()->getApp()->getProject()->getProjectDefaultFormat(&f);
        }
        if (dispmin < -SLIDER_MAX_RANGE) {
            if (spatial) {
                dispmin = -f.width();
            } else {
                dispmin = -SLIDER_MAX_RANGE;
            }
        }
        if (dispmax > SLIDER_MAX_RANGE) {
            if (spatial) {
                dispmax = f.width();
            } else {
                dispmax = SLIDER_MAX_RANGE;
            }
        }
        
        _slider = new ScaleSliderQWidget( dispmin, dispmax,knob->getGuiValue(0),
                                         ScaleSliderQWidget::eDataTypeDouble,getGui(), Natron::eScaleTypeLinear, layout->parentWidget() );
        _slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        if ( hasToolTip() ) {
            _slider->setToolTip( toolTip() );
        }
        QObject::connect( _slider, SIGNAL( positionChanged(double) ), this, SLOT( onSliderValueChanged(double) ) );
        QObject::connect( _slider, SIGNAL( editingFinished(bool) ), this, SLOT( onSliderEditingFinished(bool) ) );
        containerLayout->addWidget(_slider);
        sliderVisible = shouldSliderBeVisible(dispmin, dispmax);
        onDisplayMinMaxChanged(dispmin, dispmax);
        
    }
    
    
    if (dim > 1 && !knob->isSliderDisabled() && sliderVisible ) {
        _dimensionSwitchButton = new Button(QIcon(),QString::number(dim), _container);
        _dimensionSwitchButton->setToolTip(Natron::convertFromPlainText(tr("Switch between a single value for all dimensions and multiple values."), Qt::WhiteSpaceNormal));
        _dimensionSwitchButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _dimensionSwitchButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
        _dimensionSwitchButton->setFocusPolicy(Qt::NoFocus);
        _dimensionSwitchButton->setCheckable(true);
        containerLayout->addWidget(_dimensionSwitchButton);
        
        bool showSlider = true;
        double firstDimensionValue = knob->getGuiValue(0);
        for (int i = 0; i < dim ; ++i) {
            if (knob->getGuiValue(i) != firstDimensionValue) {
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

} // createWidget

void
Double_KnobGui::onDimensionSwitchClicked()
{
    if (!_dimensionSwitchButton) {
        return;
    }
    if (_dimensionSwitchButton->isChecked() ) {
        expandAllDimensions();
    } else {
        foldAllDimensions();
        boost::shared_ptr<Double_Knob> knob = _knob.lock();
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

void
Double_KnobGui::expandAllDimensions()
{
    if (!_dimensionSwitchButton) {
        return;
    }
    boost::shared_ptr<Double_Knob> knob = _knob.lock();
    _dimensionSwitchButton->setChecked(true);
    _dimensionSwitchButton->setDown(true);
    _slider->hide();
    for (int i = 0; i < knob->getDimension(); ++i) {
        if (i > 0) {
            _spinBoxes[i].first->show();
        }
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->show();
        }
    }
}

void
Double_KnobGui::foldAllDimensions()
{
    if (!_dimensionSwitchButton) {
        return;
    }
    boost::shared_ptr<Double_Knob> knob = _knob.lock();
    _dimensionSwitchButton->setChecked(false);
    _dimensionSwitchButton->setDown(false);
    _slider->show();
    for (int i = 0; i < knob->getDimension(); ++i) {
        if (i > 0) {
            _spinBoxes[i].first->hide();
        }
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->hide();
        }
    }
}

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
void
Double_KnobGui::onMinMaxChanged(double mini,
                                double maxi,
                                int index)
{
    assert(_spinBoxes.size() > (U32)index);
    valueAccordingToType(false, index, &mini);
    valueAccordingToType(false, index, &maxi);
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
}
#endif

void
Double_KnobGui::onDisplayMinMaxChanged(double mini,
                                       double maxi,
                                       int index )
{
    if (_slider) {
        valueAccordingToType(false, index, &mini);
        valueAccordingToType(false, index, &maxi);
        
        double sliderMin = mini;
        double sliderMax = maxi;
        boost::shared_ptr<Double_Knob> knob = _knob.lock();
        if ( (maxi - mini) >= SLIDER_MAX_RANGE ) {
            ///use min max for slider if dispmin/dispmax was not set
            assert(index < (int)knob->getMinimums().size() && index < (int)knob->getMaximums().size());
            double max = knob->getMaximums()[index];
            double min = knob->getMinimums()[index];
            if ( (max - min) < SLIDER_MAX_RANGE ) {
                sliderMin = min;
                sliderMax = max;
            }
        }
        
        if (shouldSliderBeVisible(sliderMin, sliderMax)) {
            _slider->setVisible(true);
        } else {
            _slider->setVisible(false);
        }
        
        _slider->setMinimumAndMaximum(sliderMin, sliderMax);
    }
}

void
Double_KnobGui::onIncrementChanged(double incr,
                                   int index)
{
    assert(_spinBoxes.size() > (U32)index);
    valueAccordingToType(false, index, &incr);
    _spinBoxes[index].first->setIncrement(incr);
}

void
Double_KnobGui::onDecimalsChanged(int deci,
                                  int index)
{
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->decimals(deci);
}

void
Double_KnobGui::updateGUI(int dimension)
{
    boost::shared_ptr<Double_Knob> knob = _knob.lock();
    double v = knob->getGuiValue(dimension);
    valueAccordingToType(false, dimension, &v);
    
    if (_dimensionSwitchButton && !_dimensionSwitchButton->isChecked()) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            
            if (i == dimension) {
                continue;
            }
            if (knob->getGuiValue(i) != v) {
                expandAllDimensions();
            }
        }
    }
    
    if (_slider) {
        _slider->seekScalePosition(v);
    }
    
    
    _spinBoxes[dimension].first->setValue(v);
    if (_dimensionSwitchButton && !_dimensionSwitchButton->isChecked()) {
        if (dimension == 0) {
            for (int i = 1; i < knob->getDimension(); ++i) {
                _spinBoxes[i].first->setValue(v);
            }
        }
    }
//    bool valuesEqual = true;
//    double v0 = _spinBoxes[0].first->value();
//    
//    for (int i = 1; i < _knob->getDimension(); ++i) {
//        if (_spinBoxes[i].first->value() != v0) {
//            valuesEqual = false;
//            break;
//        }
//    }
//    if (_dimensionSwitchButton && !_dimensionSwitchButton->isChecked() && !valuesEqual) {
//        expandAllDimensions();
//    } else if (_dimensionSwitchButton && _dimensionSwitchButton->isChecked() && valuesEqual) {
//        foldAllDimensions();
//    }
}

void
Double_KnobGui::reflectAnimationLevel(int dimension,
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
Double_KnobGui::onSliderValueChanged(double d)
{
    assert(_knob.lock()->isEnabled(0));
    bool penUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    if (penUpOnly) {
        return;
    }
    sliderEditingEnd(d);
}

void
Double_KnobGui::onSliderEditingFinished(bool hasMovedOnce)
{
    assert(_knob.lock()->isEnabled(0));
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    bool onEditingFinishedOnly = settings->getRenderOnEditingFinishedOnly();
    bool autoProxyEnabled = settings->isAutoProxyEnabled();
    if (onEditingFinishedOnly) {
        double v = _slider->getPosition();
        sliderEditingEnd(v);
    } else if (autoProxyEnabled && hasMovedOnce) {
        getGui()->renderAllViewers();
    }
}

void
Double_KnobGui::sliderEditingEnd(double d)
{
    boost::shared_ptr<Double_Knob> knob = _knob.lock();
    assert(knob->isEnabled(0));
    QString str;
    int digits = std::max(0,(int)-std::floor(std::log10(_slider->increment())));
    str.setNum(d, 'f', digits);
    d = str.toDouble();
    if (_dimensionSwitchButton) {
        int dims = knob->getDimension();
        for (int i = 0; i < dims; ++i) {
            _spinBoxes[i].first->setValue(d);
        }
        valueAccordingToType(true, 0, &d);
        std::list<double> oldValues,newValues;
        for (int i = 0; i < dims; ++i) {
            oldValues.push_back(knob->getGuiValue(i));
            newValues.push_back(d);
        }
        pushUndoCommand( new KnobUndoCommand<double>(this,oldValues,newValues,false) );
    } else {
        _spinBoxes[0].first->setValue(d);
        valueAccordingToType(true, 0, &d);
        pushUndoCommand( new KnobUndoCommand<double>(this,knob->getGuiValue(0),d,0,false) );
    }

}

void
Double_KnobGui::onSpinBoxValueChanged()
{
    std::list<double> newValues;

    if (!_dimensionSwitchButton || _dimensionSwitchButton->isChecked() ) {
        // each spinbox has a different value
        for (U32 i = 0; i < _spinBoxes.size(); ++i) {
            double v = _spinBoxes[i].first->value();
            valueAccordingToType(true, 0, &v);
            newValues.push_back(v);
        }
    } else {
        // use the value of the first dimension only, and set all spinboxes
        if (_spinBoxes.size() > 1) {
            double v = _spinBoxes[0].first->value();
            valueAccordingToType(true, 0, &v);
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
    pushUndoCommand( new KnobUndoCommand<double>(this,_knob.lock()->getValueForEachDimension_mt_safe(),newValues,false) );
}

void
Double_KnobGui::_hide()
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
Double_KnobGui::_show()
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
Double_KnobGui::setEnabled()
{
    boost::shared_ptr<Double_Knob> knob = _knob.lock();
    bool enabled0 = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();
    
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
Double_KnobGui::setReadOnly(bool readOnly,
                            int dimension)
{
    assert( dimension < (int)_spinBoxes.size() );
    _spinBoxes[dimension].first->setReadOnly(readOnly);
    if ( _slider && (dimension == 0) ) {
        _slider->setReadOnly(readOnly);
    }
}

void
Double_KnobGui::setDirty(bool dirty)
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->setDirty(dirty);
    }
}

boost::shared_ptr<KnobI>
Double_KnobGui::getKnob() const
{
    return _knob.lock();
}

void
Double_KnobGui::reflectExpressionState(int dimension,
                                       bool hasExpr)
{
    boost::shared_ptr<Double_Knob> knob = _knob.lock();
    bool isSlaved = knob->isSlave(dimension);
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
        _spinBoxes[dimension].first->setReadOnly(!isEnabled || isSlaved);
        if (_slider) {
            bool isEnabled0 = knob->isEnabled(0);
            _slider->setReadOnly(!isEnabled0 || isSlaved);
        }
    }
}

void
Double_KnobGui::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        for (int i = 0; i < _knob.lock()->getDimension(); ++i) {
            _spinBoxes[i].first->setToolTip( tt );
        }
        if (_slider) {
            _slider->setToolTip(tt);
        }
    }
}

void
Double_KnobGui::reflectModificationsState() {
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


