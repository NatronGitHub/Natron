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

#include "Int_KnobGui.h"

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


//==========================INT_KNOB_GUI======================================
Int_KnobGui::Int_KnobGui(boost::shared_ptr<KnobI> knob,
                         DockablePanel *container)
    : KnobGui(knob, container)
      , _container(0)
      , _slider(0)
      , _dimensionSwitchButton(0)
{
    boost::shared_ptr<Int_Knob> iKnob = boost::dynamic_pointer_cast<Int_Knob>(knob);
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

Int_KnobGui::~Int_KnobGui()
{
}

void
Int_KnobGui::removeSpecificGui()
{
    _container->setParent(NULL);
    delete _container;
    _spinBoxes.clear();
}

void
Int_KnobGui::createWidget(QHBoxLayout* layout)
{
    
    boost::shared_ptr<Int_Knob> knob = _knob.lock();
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
        
        _slider = new ScaleSliderQWidget( dispmin, dispmax,knob->getGuiValue(0),
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

    
    layout->addWidget(_container);
} // createWidget

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
void
Int_KnobGui::onMinMaxChanged(double mini,
                             double maxi,
                             int index)
{
    
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
}
#endif

void
Int_KnobGui::onDimensionSwitchClicked()
{
    if (!_dimensionSwitchButton) {
        return;
    }
    if (_dimensionSwitchButton->isChecked() ) {
        expandAllDimensions();
    } else {
        foldAllDimensions();
        boost::shared_ptr<Int_Knob> knob = _knob.lock();
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
Int_KnobGui::shouldAddStretch() const
{
    return _knob.lock()->isSliderDisabled();
}

void
Int_KnobGui::expandAllDimensions()
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
Int_KnobGui::foldAllDimensions()
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
Int_KnobGui::onDisplayMinMaxChanged(double mini,
                                    double maxi,
                                    int index)
{
    if (_slider) {
        double sliderMin = mini;
        double sliderMax = maxi;
        if ( (sliderMax - sliderMin) >= SLIDER_MAX_RANGE ) {
            
            boost::shared_ptr<Int_Knob> knob = _knob.lock();
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
Int_KnobGui::onIncrementChanged(int incr,
                                int index)
{
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setIncrement(incr);
}

void
Int_KnobGui::updateGUI(int dimension)
{
    boost::shared_ptr<Int_Knob> knob = _knob.lock();
    int v = knob->getGuiValue(dimension);

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
Int_KnobGui::reflectAnimationLevel(int dimension,
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
Int_KnobGui::onSliderValueChanged(double d)
{
    assert(_knob.lock()->isEnabled(0));
    bool penUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    if (penUpOnly) {
        return;
    }
    sliderEditingEnd(d);
}

void
Int_KnobGui::onSliderEditingFinished(bool hasMovedOnce)
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
Int_KnobGui::sliderEditingEnd(double d)
{
    boost::shared_ptr<Int_Knob> knob = _knob.lock();

    if (_dimensionSwitchButton) {
        int dims = knob->getDimension();
        for (int i = 0; i < dims; ++i) {
            _spinBoxes[i].first->setValue(d);
        }
        std::list<int> oldValues,newValues;
        for (int i = 0; i < dims; ++i) {
            oldValues.push_back(knob->getGuiValue(i));
            newValues.push_back(d);
        }
        pushUndoCommand( new KnobUndoCommand<int>(this,oldValues,newValues,false) );
    } else {
        _spinBoxes[0].first->setValue(d);
        pushUndoCommand( new KnobUndoCommand<int>(this,knob->getGuiValue(0),d,0,false) );
    }
    
}

void
Int_KnobGui::onSpinBoxValueChanged()
{
    std::list<int> newValues;

    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        newValues.push_back( _spinBoxes[i].first->value() );
    }
    if (_slider) {
        _slider->seekScalePosition( newValues.front() );
    }
    pushUndoCommand( new KnobUndoCommand<int>(this,_knob.lock()->getValueForEachDimension_mt_safe(),newValues,false) );
}

void
Int_KnobGui::_hide()
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
Int_KnobGui::_show()
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
Int_KnobGui::setEnabled()
{
    boost::shared_ptr<Int_Knob> knob = _knob.lock();

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
Int_KnobGui::setReadOnly(bool readOnly,
                         int dimension)
{
    assert( dimension < (int)_spinBoxes.size() );
    _spinBoxes[dimension].first->setReadOnly(readOnly);
    if ( _slider && (dimension == 0) ) {
        _slider->setReadOnly(readOnly);
    }
}

void
Int_KnobGui::setDirty(bool dirty)
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->setDirty(dirty);
    }
}

boost::shared_ptr<KnobI> Int_KnobGui::getKnob() const
{
    return _knob.lock();
}

void
Int_KnobGui::reflectExpressionState(int dimension,
                                    bool hasExpr)
{
    boost::shared_ptr<Int_Knob> knob = _knob.lock();

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
Int_KnobGui::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        boost::shared_ptr<Int_Knob> knob = _knob.lock();

        for (int i = 0; i < knob->getDimension(); ++i) {
            _spinBoxes[i].first->setToolTip( tt );
        }
        if (_slider) {
            _slider->setToolTip(tt);
        }
    }
}

void
Int_KnobGui::reflectModificationsState()
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
