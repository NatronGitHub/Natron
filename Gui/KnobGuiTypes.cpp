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

#include "KnobGuiTypes.h"

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
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/Utils.h"

#include "ofxNatron.h"

#define SLIDER_MAX_RANGE 100000

#define kFontSizeTag "<font size=\""
#define kFontColorTag "color=\""
#define kFontFaceTag "face=\""
#define kFontEndTag "</font>"
#define kBoldStartTag "<b>"
#define kBoldEndTag "</b>"
#define kItalicStartTag "<i>"
#define kItalicEndTag "</i>"

using namespace Natron;
using std::make_pair;


static bool shouldSliderBeVisible(int sliderMin,int sliderMax)
{
    return (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < INT_MAX) && (sliderMin > INT_MIN);
}

static bool shouldSliderBeVisible(double sliderMin,double sliderMax)
{
    return (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < DBL_MAX) && (sliderMin > -DBL_MAX);
}

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

    double v0 = _spinBoxes[0].first->value();
    
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        double v;
        if (_dimensionSwitchButton && !_dimensionSwitchButton->isChecked()) {
            v = v0;
            _spinBoxes[i].first->setValue(v);
        } else {
            v = _spinBoxes[i].first->value();
        }
        valueAccordingToType(true, 0, &v);
        newValues.push_back(v);
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


//=============================BUTTON_KNOB_GUI===================================

Button_KnobGui::Button_KnobGui(boost::shared_ptr<KnobI> knob,
                               DockablePanel *container)
    : KnobGui(knob, container)
      , _button(0)
{
    _knob = boost::dynamic_pointer_cast<Button_Knob>(knob);
}

void
Button_KnobGui::createWidget(QHBoxLayout* layout)
{
    boost::shared_ptr<Button_Knob> knob = _knob.lock();
    QString label( knob->getDescription().c_str() );
    const std::string & iconFilePath = knob->getIconFilePath();
    
    QString filePath(iconFilePath.c_str());
    if (!iconFilePath.empty() && !QFile::exists(filePath)) {
        ///Search all natron paths for a file
        
        QStringList paths = appPTR->getAllNonOFXPluginsPaths();
        for (int i = 0; i < paths.size(); ++i) {
            QString tmp = paths[i] + QChar('/') + filePath;
            if (QFile::exists(tmp)) {
                filePath = tmp;
                break;
            }
        }
    }
    
    QPixmap pix;

    if (pix.load(filePath)) {
        _button = new Button( QIcon(pix),"",layout->parentWidget() );
        _button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    } else {
        _button = new Button( label,layout->parentWidget() );
    }
    QObject::connect( _button, SIGNAL(clicked()), this, SLOT(emitValueChanged()));
    if ( hasToolTip() ) {
        _button->setToolTip( toolTip() );
    }
    layout->addWidget(_button);
}

Button_KnobGui::~Button_KnobGui()
{
}

void Button_KnobGui::removeSpecificGui()
{
    delete _button;
}

void
Button_KnobGui::emitValueChanged()
{
   _knob.lock()->evaluateValueChange(0, Natron::eValueChangedReasonUserEdited);
}

void
Button_KnobGui::_hide()
{
    _button->hide();
}

void
Button_KnobGui::_show()
{
    _button->show();
}

void
Button_KnobGui::setEnabled()
{
    boost::shared_ptr<Button_Knob> knob = _knob.lock();
    bool b = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    _button->setEnabled(b);
}

void
Button_KnobGui::setReadOnly(bool readOnly,
                            int /*dimension*/)
{
    _button->setEnabled(!readOnly);
}

boost::shared_ptr<KnobI> Button_KnobGui::getKnob() const
{
    return _knob.lock();
}

//=============================CHOICE_KNOB_GUI===================================

struct NewLayerDialogPrivate
{
    QGridLayout* mainLayout;
    Natron::Label* layerLabel;
    LineEdit* layerEdit;
    Natron::Label* numCompsLabel;
    SpinBox* numCompsBox;
    Natron::Label* rLabel;
    LineEdit* rEdit;
    Natron::Label* gLabel;
    LineEdit* gEdit;
    Natron::Label* bLabel;
    LineEdit* bEdit;
    Natron::Label* aLabel;
    LineEdit* aEdit;
    
    Button* setRgbaButton;
    QDialogButtonBox* buttons;
    
    NewLayerDialogPrivate()
    : mainLayout(0)
    , layerLabel(0)
    , layerEdit(0)
    , numCompsLabel(0)
    , numCompsBox(0)
    , rLabel(0)
    , rEdit(0)
    , gLabel(0)
    , gEdit(0)
    , bLabel(0)
    , bEdit(0)
    , aLabel(0)
    , aEdit(0)
    , setRgbaButton(0)
    , buttons(0)
    {
        
    }
};

NewLayerDialog::NewLayerDialog(QWidget* parent)
: QDialog(parent)
, _imp(new NewLayerDialogPrivate())
{
    _imp->mainLayout = new QGridLayout(this);
    _imp->layerLabel = new Natron::Label(tr("Layer Name"),this);
    _imp->layerEdit = new LineEdit(this);
    
    _imp->numCompsLabel = new Natron::Label(tr("No. Channels"),this);
    _imp->numCompsBox = new SpinBox(this,SpinBox::eSpinBoxTypeInt);
    QObject::connect(_imp->numCompsBox, SIGNAL(valueChanged(double)), this, SLOT(onNumCompsChanged(double)));
    _imp->numCompsBox->setMinimum(1);
    _imp->numCompsBox->setMaximum(4);
    _imp->numCompsBox->setValue(4);
    
    _imp->rLabel = new Natron::Label(tr("1st Channel"),this);
    _imp->rEdit = new LineEdit(this);
    _imp->gLabel = new Natron::Label(tr("2nd Channel"),this);
    _imp->gEdit = new LineEdit(this);
    _imp->bLabel = new Natron::Label(tr("3rd Channel"),this);
    _imp->bEdit = new LineEdit(this);
    _imp->aLabel = new Natron::Label(tr("4th Channel"),this);
    _imp->aEdit = new LineEdit(this);
    
    _imp->setRgbaButton = new Button(this);
    _imp->setRgbaButton->setText(tr("Set RGBA"));
    QObject::connect(_imp->setRgbaButton, SIGNAL(clicked(bool)), this, SLOT(onRGBAButtonClicked()));
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal,this);
    QObject::connect(_imp->buttons, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(_imp->buttons, SIGNAL(rejected()), this, SLOT(reject()));
    
    _imp->mainLayout->addWidget(_imp->layerLabel, 0, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->layerEdit, 0, 1, 1, 1);
    
    _imp->mainLayout->addWidget(_imp->numCompsLabel, 1, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->numCompsBox, 1, 1, 1, 1);
    
    _imp->mainLayout->addWidget(_imp->rLabel, 2, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->rEdit, 2, 1, 1, 1);
    
    _imp->mainLayout->addWidget(_imp->gLabel, 3, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->gEdit, 3, 1, 1, 1);

    
    _imp->mainLayout->addWidget(_imp->bLabel, 4, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->bEdit, 4, 1, 1, 1);

    
    _imp->mainLayout->addWidget(_imp->aLabel, 5, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->aEdit, 5, 1, 1, 1);
    
    _imp->mainLayout->addWidget(_imp->setRgbaButton, 6, 0, 1, 2);
    
    _imp->mainLayout->addWidget(_imp->buttons, 7, 0, 1, 2);


    
}

NewLayerDialog::~NewLayerDialog()
{
    
}

void
NewLayerDialog::onNumCompsChanged(double value)
{
    if (value == 1) {
        _imp->rLabel->setVisible(false);
        _imp->rEdit->setVisible(false);
        _imp->gLabel->setVisible(false);
        _imp->gEdit->setVisible(false);
        _imp->bLabel->setVisible(false);
        _imp->bEdit->setVisible(false);
        _imp->aLabel->setVisible(true);
        _imp->aEdit->setVisible(true);
    } else if (value == 2) {
        _imp->rLabel->setVisible(true);
        _imp->rEdit->setVisible(true);
        _imp->gLabel->setVisible(true);
        _imp->gEdit->setVisible(true);
        _imp->bLabel->setVisible(false);
        _imp->bEdit->setVisible(false);
        _imp->aLabel->setVisible(false);
        _imp->aEdit->setVisible(false);
    } else if (value == 3) {
        _imp->rLabel->setVisible(true);
        _imp->rEdit->setVisible(true);
        _imp->gLabel->setVisible(true);
        _imp->gEdit->setVisible(true);
        _imp->bLabel->setVisible(true);
        _imp->bEdit->setVisible(true);
        _imp->aLabel->setVisible(false);
        _imp->aEdit->setVisible(false);
    } else if (value == 3) {
        _imp->rLabel->setVisible(true);
        _imp->rEdit->setVisible(true);
        _imp->gLabel->setVisible(true);
        _imp->gEdit->setVisible(true);
        _imp->bLabel->setVisible(true);
        _imp->bEdit->setVisible(true);
        _imp->aLabel->setVisible(true);
        _imp->aEdit->setVisible(true);
    }
}

Natron::ImageComponents
NewLayerDialog::getComponents() const
{
    QString layer = _imp->layerEdit->text();
    int nComps = (int)_imp->numCompsBox->value();
    QString r = _imp->rEdit->text();
    QString g = _imp->gEdit->text();
    QString b = _imp->bEdit->text();
    QString a = _imp->aEdit->text();
    std::string layerFixed = Natron::makeNameScriptFriendly(layer.toStdString());
    if (layerFixed.empty()) {
        return Natron::ImageComponents::getNoneComponents();
    }
    
    if (nComps == 1) {
        if (a.isEmpty()) {
            return Natron::ImageComponents::getNoneComponents();
        }
        std::vector<std::string> comps;
        std::string compsGlobal;
        comps.push_back(a.toStdString());
        compsGlobal.append(a.toStdString());
        return Natron::ImageComponents(layerFixed,compsGlobal,comps);
    } else if (nComps == 2) {
        if (r.isEmpty() || g.isEmpty()) {
            return Natron::ImageComponents::getNoneComponents();
        }
        std::vector<std::string> comps;
        std::string compsGlobal;
        comps.push_back(r.toStdString());
        compsGlobal.append(r.toStdString());
        comps.push_back(g.toStdString());
        compsGlobal.append(g.toStdString());
        return Natron::ImageComponents(layerFixed,compsGlobal,comps);
    } else if (nComps == 3) {
        if (r.isEmpty() || g.isEmpty() || b.isEmpty()) {
            return Natron::ImageComponents::getNoneComponents();
        }
        std::vector<std::string> comps;
        std::string compsGlobal;
        comps.push_back(r.toStdString());
        compsGlobal.append(r.toStdString());
        comps.push_back(g.toStdString());
        compsGlobal.append(g.toStdString());
        comps.push_back(b.toStdString());
        compsGlobal.append(b.toStdString());
        return Natron::ImageComponents(layerFixed,compsGlobal,comps);
    } else if (nComps == 4) {
        if (r.isEmpty() || g.isEmpty() || b.isEmpty() | a.isEmpty())  {
            return Natron::ImageComponents::getNoneComponents();
        }
        std::vector<std::string> comps;
        std::string compsGlobal;
        comps.push_back(r.toStdString());
        compsGlobal.append(r.toStdString());
        comps.push_back(g.toStdString());
        compsGlobal.append(g.toStdString());
        comps.push_back(b.toStdString());
        compsGlobal.append(b.toStdString());
        comps.push_back(a.toStdString());
        compsGlobal.append(a.toStdString());
        return Natron::ImageComponents(layerFixed,compsGlobal,comps);
    }
    return Natron::ImageComponents::getNoneComponents();
}

void
NewLayerDialog::onRGBAButtonClicked()
{
    _imp->rEdit->setText("R");
    _imp->gEdit->setText("G");
    _imp->bEdit->setText("B");
    _imp->aEdit->setText("A");
    
    _imp->rLabel->setVisible(true);
    _imp->rEdit->setVisible(true);
    _imp->gLabel->setVisible(true);
    _imp->gEdit->setVisible(true);
    _imp->bLabel->setVisible(true);
    _imp->bEdit->setVisible(true);
    _imp->aLabel->setVisible(true);
    _imp->aEdit->setVisible(true);

}

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

//=============================SEPARATOR_KNOB_GUI===================================

Separator_KnobGui::Separator_KnobGui(boost::shared_ptr<KnobI> knob,
                                     DockablePanel *container)
    : KnobGui(knob, container)
    , _line(0)
{
    _knob = boost::dynamic_pointer_cast<Separator_Knob>(knob);
}

void
Separator_KnobGui::createWidget(QHBoxLayout* layout)
{
    ///FIXME: this line is never visible.
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
    _line = new QFrame( layout->parentWidget() );
    _line->setFixedHeight(2);
    _line->setGeometry(0, 0, 300, 2);
    _line->setFrameShape(QFrame::HLine);
    _line->setFrameShadow(QFrame::Sunken);
    _line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(_line);
}

Separator_KnobGui::~Separator_KnobGui()
{
    
}

void Separator_KnobGui::removeSpecificGui()
{
    delete _line;
}

void
Separator_KnobGui::_hide()
{
    _line->hide();
}

void
Separator_KnobGui::_show()
{
    _line->show();
}

boost::shared_ptr<KnobI> Separator_KnobGui::getKnob() const
{
    return _knob.lock();
}

//=============================RGBA_KNOB_GUI===================================

Color_KnobGui::Color_KnobGui(boost::shared_ptr<KnobI> knob,
                             DockablePanel *container)
    : KnobGui(knob, container)
      , mainContainer(NULL)
      , mainLayout(NULL)
      , boxContainers(NULL)
      , boxLayout(NULL)
      , colorContainer(NULL)
      , colorLayout(NULL)
      , _rLabel(NULL)
      , _gLabel(NULL)
      , _bLabel(NULL)
      , _aLabel(NULL)
      , _rBox(NULL)
      , _gBox(NULL)
      , _bBox(NULL)
      , _aBox(NULL)
      , _colorLabel(NULL)
      , _colorDialogButton(NULL)
      , _dimensionSwitchButton(NULL)
      , _slider(NULL)
      , _dimension( knob->getDimension() )
      , _lastColor(_dimension)
{
    boost::shared_ptr<Color_Knob> ck = boost::dynamic_pointer_cast<Color_Knob>(knob);
    assert(ck);
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    boost::shared_ptr<KnobSignalSlotHandler> handler = _knob->getSignalSlotHandler();
    if (handler) {
        QObject::connect( handler.get(), SIGNAL( minMaxChanged(double, double, int) ), this, SLOT( onMinMaxChanged(double, double, int) ) );
        QObject::connect( handler.get(), SIGNAL( displayMinMaxChanged(double, double, int) ), this, SLOT( onDisplayMinMaxChanged(double, double, int) ) );
    }
#endif
    QObject::connect( this, SIGNAL( dimensionSwitchToggled(bool) ), ck.get(), SLOT( onDimensionSwitchToggled(bool) ) );
    QObject::connect( ck.get(), SIGNAL( mustActivateAllDimensions() ),this, SLOT( onMustShowAllDimension() ) );
    QObject::connect( ck.get(), SIGNAL( pickingEnabled(bool) ),this, SLOT( setPickingEnabled(bool) ) );
    _knob = ck;
}

Color_KnobGui::~Color_KnobGui()
{
   
}

void Color_KnobGui::removeSpecificGui()
{
   delete mainContainer;
}

void
Color_KnobGui::createWidget(QHBoxLayout* layout)
{
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mainContainer = new QWidget( layout->parentWidget() );
    mainLayout = new QHBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    boxContainers = new QWidget(mainContainer);
    boxContainers->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    boxLayout = new QHBoxLayout(boxContainers);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setSpacing(1);
    
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    const std::vector<double> & minimums = _knob->getMinimums();
    const std::vector<double> & maximums = _knob->getMaximums();
#endif
    
    _rBox = new SpinBox(boxContainers, SpinBox::eSpinBoxTypeDouble);
    QObject::connect( _rBox, SIGNAL( valueChanged(double) ), this, SLOT( onColorChanged() ) );
    
    if (_dimension >= 3) {
        _gBox = new SpinBox(boxContainers, SpinBox::eSpinBoxTypeDouble);
        QObject::connect( _gBox, SIGNAL( valueChanged(double) ), this, SLOT( onColorChanged() ) );
        _bBox = new SpinBox(boxContainers, SpinBox::eSpinBoxTypeDouble);
        QObject::connect( _bBox, SIGNAL( valueChanged(double) ), this, SLOT( onColorChanged() ) );
    }
    if (_dimension >= 4) {
        _aBox = new SpinBox(boxContainers, SpinBox::eSpinBoxTypeDouble);
        QObject::connect( _aBox, SIGNAL( valueChanged(double) ), this, SLOT( onColorChanged() ) );
    }
    
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    _rBox->setMaximum(maximums[0]);
    _rBox->setMinimum(minimums[0]);
#endif
    
    _rBox->decimals(6);
    _rBox->setIncrement(0.001);
    
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_rBox,0);

    if ( hasToolTip() ) {
        _rBox->setToolTip( toolTip() );
    }
    
    //QFont font(appFont,appFontSize);
    boost::shared_ptr<Color_Knob> knob = _knob.lock();
    
    std::string dimLabel = knob->getDimensionName(0);
    if (!dimLabel.empty()) {
        dimLabel.append(":");
    }
    _rLabel = new Natron::Label(QString(dimLabel.c_str()).toLower(), boxContainers);
    //_rLabel->setFont(font);
    if ( hasToolTip() ) {
        _rLabel->setToolTip( toolTip() );
    }
    boxLayout->addWidget(_rLabel);
    boxLayout->addWidget(_rBox);
    
    if (_dimension >= 3) {
        
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        _gBox->setMaximum(maximums[1]);
        _gBox->setMinimum(minimums[1]);
#endif
        
        _gBox->decimals(6);
        _gBox->setIncrement(0.001);
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_gBox,1);
        
        if ( hasToolTip() ) {
            _gBox->setToolTip( toolTip() );
        }
        
        dimLabel = knob->getDimensionName(1);
        if (!dimLabel.empty()) {
            dimLabel.append(":");
        }
        
        _gLabel = new Natron::Label(QString(dimLabel.c_str()).toLower(), boxContainers);
        //_gLabel->setFont(font);
        if ( hasToolTip() ) {
            _gLabel->setToolTip( toolTip() );
        }
        boxLayout->addWidget(_gLabel);
        boxLayout->addWidget(_gBox);
        
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        _bBox->setMaximum(maximums[2]);
        _bBox->setMinimum(minimums[2]);
#endif
        
        _bBox->decimals(6);
        _bBox->setIncrement(0.001);
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_bBox,2);
        
        
        if ( hasToolTip() ) {
            _bBox->setToolTip( toolTip() );
        }
        
        dimLabel = knob->getDimensionName(2);
        if (!dimLabel.empty()) {
            dimLabel.append(":");
        }
        
        _bLabel = new Natron::Label(QString(dimLabel.c_str()).toLower(), boxContainers);
        //_bLabel->setFont(font);
        if ( hasToolTip() ) {
            _bLabel->setToolTip( toolTip() );
        }
        boxLayout->addWidget(_bLabel);
        boxLayout->addWidget(_bBox);
    } //       if (_dimension >= 3) {
    
    if (_dimension >= 4) {
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        _aBox->setMaximum(maximums[3]);
        _aBox->setMinimum(minimums[3]);
#endif
        
        _aBox->decimals(6);
        _aBox->setIncrement(0.001);
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_aBox,3);
        
        
        if ( hasToolTip() ) {
            _aBox->setToolTip( toolTip() );
        }
        
        dimLabel = knob->getDimensionName(3);
        if (!dimLabel.empty()) {
            dimLabel.append(":");
        }
        
        _aLabel = new Natron::Label(QString(dimLabel.c_str()).toLower(), boxContainers);
        //_aLabel->setFont(font);
        if ( hasToolTip() ) {
            _aLabel->setToolTip( toolTip() );
        }
        boxLayout->addWidget(_aLabel);
        boxLayout->addWidget(_aBox);
    } // if (_dimension >= 4) {
    
    const std::vector<double> & displayMinimums = knob->getDisplayMinimums();
    const std::vector<double> & displayMaximums = knob->getDisplayMaximums();
    double slidermin = *std::min_element( displayMinimums.begin(), displayMinimums.end() );
    double slidermax = *std::max_element( displayMaximums.begin(), displayMaximums.end() );
    if ( slidermin <= -std::numeric_limits<float>::max() ) {
        slidermin = 0.;
    }
    if ( slidermax >= std::numeric_limits<float>::max() ) {
        slidermax = 1.;
    }
    _slider = new ScaleSliderQWidget(slidermin, slidermax, knob->getGuiValue(0),
                                     ScaleSliderQWidget::eDataTypeDouble,getGui(),Natron::eScaleTypeLinear, boxContainers);
    _slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QObject::connect( _slider, SIGNAL( positionChanged(double) ), this, SLOT( onSliderValueChanged(double) ) );
    QObject::connect( _slider, SIGNAL( editingFinished(bool) ), this, SLOT( onSliderEditingFinished(bool) ) );
    _slider->hide();
    
    colorContainer = new QWidget(mainContainer);
    colorLayout = new QHBoxLayout(colorContainer);
    colorContainer->setLayout(colorLayout);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(0);
    
    _colorLabel = new ColorPickerLabel(knob->isSimplified() ? NULL : this,colorContainer);
    if (!knob->isSimplified()) {
        _colorLabel->setToolTip(Natron::convertFromPlainText(tr("To pick a color on a viewer, click this and then press control + left click on any viewer.\n"
                                                                "You can also pick the average color of a given rectangle by holding control + shift + left click\n. "
                                                                "To deselect the picker left click anywhere."
                                                                "Note that by default %1 converts to linear the color picked\n"
                                                                "because all the processing pipeline is linear, but you can turn this off in the\n"
                                                                "preferences panel.").arg(NATRON_APPLICATION_NAME), Qt::WhiteSpaceNormal) );
    }
    _colorLabel->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect( _colorLabel,SIGNAL( pickingEnabled(bool) ),this,SLOT( onPickingEnabled(bool) ) );
    colorLayout->addWidget(_colorLabel);
    
    if (knob->isSimplified()) {
        colorLayout->addSpacing(5);
    }
    
    QPixmap buttonPix;
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, &buttonPix);
    
    _colorDialogButton = new Button(QIcon(buttonPix), "", colorContainer);
    _colorDialogButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _colorDialogButton->setToolTip(Natron::convertFromPlainText(tr("Open the color dialog."), Qt::WhiteSpaceNormal));
    _colorDialogButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _colorDialogButton, SIGNAL( clicked() ), this, SLOT( showColorDialog() ) );
    colorLayout->addWidget(_colorDialogButton);
    
    bool enableAllDimensions = false;
    
    _dimensionSwitchButton = new Button(QIcon(),QString::number(_dimension),colorContainer);
    _dimensionSwitchButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _dimensionSwitchButton->setToolTip(Natron::convertFromPlainText(tr("Switch between a single value for all dimensions and multiple values."), Qt::WhiteSpaceNormal));
    _dimensionSwitchButton->setFocusPolicy(Qt::NoFocus);
    _dimensionSwitchButton->setCheckable(true);
    
    double firstDimensionValue = knob->getGuiValue(0);
    if (_dimension > 1) {
        if (knob->getGuiValue(1) != firstDimensionValue) {
            enableAllDimensions = true;
        }
        if (knob->getGuiValue(2) != firstDimensionValue) {
            enableAllDimensions = true;
        }
    }
    if (_dimension > 3) {
        if (knob->getGuiValue(3) != firstDimensionValue) {
            enableAllDimensions = true;
        }
    }
    
    
    _dimensionSwitchButton->setChecked(enableAllDimensions);
    if (!enableAllDimensions) {
        _rBox->setValue( knob->getGuiValue(0) );
    }
    onDimensionSwitchClicked();
    QObject::connect( _dimensionSwitchButton, SIGNAL( clicked() ), this, SLOT( onDimensionSwitchClicked() ) );
    
    colorLayout->addWidget(_dimensionSwitchButton);
    
    mainLayout->addWidget(boxContainers);
    mainLayout->addWidget(_slider);
    
    mainLayout->addWidget(colorContainer);
    
    layout->addWidget(mainContainer);
    
    if (knob->isSimplified()) {
        boxContainers->hide();
        _slider->hide();
        _dimensionSwitchButton->hide();
    }
    
} // createWidget

void
Color_KnobGui::onMustShowAllDimension()
{
    _dimensionSwitchButton->setChecked(true);
    onDimensionSwitchClicked();
}

void
Color_KnobGui::onSliderValueChanged(double v)
{
    assert(_knob.lock()->isEnabled(0));
    _rBox->setValue(v);
    if (_dimension > 1) {
        _gBox->setValue(v);
        _bBox->setValue(v);
        if (_dimension > 3) {
            _aBox->setValue(v);
        }
    }
    onColorChanged();
}

void
Color_KnobGui::onSliderEditingFinished(bool hasMovedOnce)
{
    assert(_knob.lock()->isEnabled(0));
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    bool onEditingFinishedOnly = settings->getRenderOnEditingFinishedOnly();
    bool autoProxyEnabled = settings->isAutoProxyEnabled();
    if (onEditingFinishedOnly) {
        double v = _slider->getPosition();
        onSliderValueChanged(v);
    } else if (autoProxyEnabled && hasMovedOnce) {
        getGui()->renderAllViewers();
    }
}

void
Color_KnobGui::expandAllDimensions()
{
    ///show all the dimensions
    _dimensionSwitchButton->setChecked(true);
    _dimensionSwitchButton->setDown(true);
    _slider->hide();
    if (_dimension > 1) {
        _rLabel->show();
        QColor rColor,gColor,bColor,aColor;
        rColor.setRgbF(0.851643,0.196936,0.196936);
        gColor.setRgbF(0,0.654707,0);
        bColor.setRgbF(0.345293,0.345293,1);
        aColor.setRgbF(0.398979,0.398979,0.398979);
        _rBox->setUseLineColor(true, rColor);
        _gLabel->show();
        _gBox->show();
        _gBox->setUseLineColor(true, gColor);
        _bLabel->show();
        _bBox->show();
        _bBox->setUseLineColor(true, bColor);
        if (_dimension > 3) {
            _aLabel->show();
            _aBox->show();
            _aBox->setUseLineColor(true, aColor);
        }
    }
    Q_EMIT dimensionSwitchToggled(true);
}

void
Color_KnobGui::foldAllDimensions()
{
    ///hide all the dimensions except red
    _dimensionSwitchButton->setChecked(false);
    _dimensionSwitchButton->setDown(false);
    _slider->show();
    if (_dimension > 1) {
        _rLabel->hide();
        _rBox->setUseLineColor(false, Qt::red);
        _gLabel->hide();
        _gBox->hide();
        _bLabel->hide();
        _bBox->hide();
        if (_dimension > 3) {
            _aLabel->hide();
            _aBox->hide();
        }
    }
    Q_EMIT dimensionSwitchToggled(false);
}

void
Color_KnobGui::onDimensionSwitchClicked()
{
    boost::shared_ptr<Color_Knob> knob = _knob.lock();
    if (knob->isSimplified()) {
        return;
    }
    if ( _dimensionSwitchButton->isChecked() ) {
        expandAllDimensions();
    } else {
        foldAllDimensions();
        if (_dimension > 1) {
            double value( _rBox->value() );
            if (_dimension == 3) {
                knob->setValues(value, value, value, Natron::eValueChangedReasonNatronGuiEdited);
            } else {
                knob->setValues(value, value, value,value, Natron::eValueChangedReasonNatronGuiEdited);
            }
        }
    }
}

void
Color_KnobGui::onMinMaxChanged(double mini,
                               double maxi,
                               int index)
{
    assert(index < _dimension);
    switch (index) {
    case 0:
        _rBox->setMinimum(mini);
        _rBox->setMaximum(maxi);
        break;
    case 1:
        _gBox->setMinimum(mini);
        _gBox->setMaximum(maxi);
        break;
    case 2:
        _bBox->setMinimum(mini);
        _bBox->setMaximum(maxi);
        break;
    case 3:
        _aBox->setMinimum(mini);
        _aBox->setMaximum(maxi);
        break;
    }
}

void
Color_KnobGui::onDisplayMinMaxChanged(double mini,
                                      double maxi,
                                      int index )
{
    assert(index < _dimension);
    switch (index) {
    case 0:
        _rBox->setMinimum(mini);
        _rBox->setMaximum(maxi);
        break;
    case 1:
        _gBox->setMinimum(mini);
        _gBox->setMaximum(maxi);
        break;
    case 2:
        _bBox->setMinimum(mini);
        _bBox->setMaximum(maxi);
        break;
    case 3:
        _aBox->setMinimum(mini);
        _aBox->setMaximum(maxi);
        break;
    }
}

void
Color_KnobGui::setEnabled()
{
    boost::shared_ptr<Color_Knob> knob = _knob.lock();
    bool r = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    //_rBox->setEnabled(r);
    _rBox->setReadOnly(!r);
    _rLabel->setEnabled(r);

    _slider->setReadOnly(!r);
    _colorDialogButton->setEnabled(r);
    
    if (_dimension >= 3) {
        
        bool g = knob->isEnabled(1) && !knob->isSlave(1) && knob->getExpression(1).empty();
        bool b = knob->isEnabled(2) && !knob->isSlave(2) && knob->getExpression(2).empty();
        //_gBox->setEnabled(g);
        _gBox->setReadOnly(!g);
        _gLabel->setEnabled(g);
        //_bBox->setEnabled(b);
        _bBox->setReadOnly(!b);
        _bLabel->setEnabled(b);
    }
    if (_dimension >= 4) {
        bool a = knob->isEnabled(3) && !knob->isSlave(3) && knob->getExpression(3).empty();
        //_aBox->setEnabled(a);
        _aBox->setReadOnly(!a);
        _aLabel->setEnabled(a);
    }
    _dimensionSwitchButton->setEnabled(r);
    _colorLabel->setEnabledMode(r);
}

void
Color_KnobGui::updateGUI(int dimension)
{
    boost::shared_ptr<Color_Knob> knob = _knob.lock();
    if (knob->isSimplified()) {
        double r = 0.,g = 0.,b = 0.,a = 1.;
        r = knob->getGuiValue(0);
        if (_dimension > 1) {
            g = knob->getGuiValue(1);
            b = knob->getGuiValue(2);
            if (_dimension > 3) {
                a = knob->getGuiValue(3);
            }
        }
        updateLabel(r, g, b, a);
    } else {
        assert(dimension < _dimension && dimension >= 0 && dimension <= 3);
        double value = knob->getGuiValue(dimension);
        
        if (!knob->areAllDimensionsEnabled()) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                
                if (i == dimension) {
                    continue;
                }
                if (knob->getGuiValue(i) != value) {
                    expandAllDimensions();
                }
            }
        }
        
        switch (dimension) {
            case 0: {
                _rBox->setValue(value);
                _slider->seekScalePosition(value);
                if ( !knob->areAllDimensionsEnabled() ) {
                    _gBox->setValue(value);
                    _bBox->setValue(value);
                    if (_dimension >= 4) {
                        _aBox->setValue(value);
                    }
                }
                break;
            }
            case 1:
                _gBox->setValue(value);
                break;
            case 2:
                _bBox->setValue(value);
                break;
            case 3:
                _aBox->setValue(value);
                break;
            default:
                throw std::logic_error("wrong dimension");
        }
        
        
        double r = _rBox->value();
        double g = r;
        double b = r;
        double a = 1.;
        
        if (_dimension >= 3) {
            g = _gBox->value();
            b = _bBox->value();
        }
        if (_dimension >= 4) {
            a = _aBox->value();
        }
        updateLabel(r, g, b, a);
    }


} // updateGUI

void
Color_KnobGui::reflectAnimationLevel(int dimension,
                                     Natron::AnimationLevelEnum level)
{
    if (_knob.lock()->isSimplified()) {
        return;
    }
    switch (level) {
        case Natron::eAnimationLevelNone: {
            switch (dimension) {
                case 0:
                    _rBox->setAnimation(0);
                    break;
                case 1:
                    _gBox->setAnimation(0);
                    break;
                case 2:
                    _bBox->setAnimation(0);
                    break;
                case 3:
                    _aBox->setAnimation(0);
                    break;
                default:
                    assert(false && "Dimension out of range");
                    break;
            }
        }  break;
        case Natron::eAnimationLevelInterpolatedValue: {
            switch (dimension) {
                case 0:
                    _rBox->setAnimation(1);
                    break;
                case 1:
                    _gBox->setAnimation(1);
                    break;
                case 2:
                    _bBox->setAnimation(1);
                    break;
                case 3:
                    _aBox->setAnimation(1);
                    break;
                default:
                    assert(false && "Dimension out of range");
                    break;
            }
        }    break;
        case Natron::eAnimationLevelOnKeyframe: {
            switch (dimension) {
                case 0:
                    _rBox->setAnimation(2);
                    break;
                case 1:
                    _gBox->setAnimation(2);
                    break;
                case 2:
                    _bBox->setAnimation(2);
                    break;
                case 3:
                    _aBox->setAnimation(2);
                    break;
                default:
                    assert(false && "Dimension out of range");
                    break;
            }
        } break;
            
        default:
            break;
    } // switch
} // reflectAnimationLevel

void
Color_KnobGui::reflectExpressionState(int dimension,
                                      bool hasExpr)
{
    bool isSlaved = _knob.lock()->isSlave(dimension);
    switch (dimension) {
        case 0:
            _rBox->setAnimation(3);
            _rBox->setReadOnly(hasExpr || isSlaved);
            break;
        case 1:
            _gBox->setAnimation(3);
            _gBox->setReadOnly(hasExpr || isSlaved);
            break;
        case 2:
            _bBox->setAnimation(3);
            _bBox->setReadOnly(hasExpr || isSlaved);
            break;
        case 3:
            _aBox->setAnimation(3);
            _aBox->setReadOnly(hasExpr || isSlaved);
            break;
        default:
            break;
    }
    
}

void
Color_KnobGui::updateToolTip()
{
    if (hasToolTip()) {
        QString tt = toolTip();
        _rBox->setToolTip(tt);
        if (_dimension >= 3) {
            _gBox->setToolTip(tt);
            _bBox->setToolTip(tt);
            if (_dimension >= 4) {
                _aBox->setToolTip(tt);
            }
        }
        _slider->setToolTip(tt);
    }
}

void
Color_KnobGui::showColorDialog()
{
    QColorDialog dialog( _rBox->parentWidget() );

    boost::shared_ptr<Color_Knob> knob = _knob.lock();
    double curR = knob->getGuiValue(0);
    _lastColor[0] = curR;
    double curG = curR;
    double curB = curR;
    double curA = 1.;
    if (_dimension > 1) {
        curG = knob->getGuiValue(1);
        _lastColor[1] =  curG;
        curB = knob->getGuiValue(2);
        _lastColor[2] = curB;
    }
    if (_dimension > 3) {
        curA = knob->getGuiValue(3);
        _lastColor[3] = curA;
    }

    QColor curColor;
    curColor.setRgbF(Natron::clamp<qreal>(Natron::Color::to_func_srgb(curR), 0., 1.),
                     Natron::clamp<qreal>(Natron::Color::to_func_srgb(curG), 0., 1.),
                     Natron::clamp<qreal>(Natron::Color::to_func_srgb(curB), 0., 1.),
                     Natron::clamp<qreal>(Natron::Color::to_func_srgb(curA), 0., 1.));
    dialog.setCurrentColor(curColor);
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onDialogCurrentColorChanged(QColor) ) );
    if (!dialog.exec()) {
        
        knob->beginChanges();

        for (int i = 0; i < _dimension; ++i) {
            knob->setValue(_lastColor[i],i);
        }
        knob->endChanges();

    } else {
        
        knob->beginChanges();

        ///refresh the last value so that the undo command retrieves the value that was prior to opening the dialog
        for (int i = 0; i < _dimension; ++i) {
            knob->setValue(_lastColor[i],i);
        }

        ///if only the first dimension is displayed, switch back to all dimensions
        if (_dimensionSwitchButton &&  !_dimensionSwitchButton->isChecked() ) {
            _dimensionSwitchButton->setChecked(true);
            _dimensionSwitchButton->setDown(true);
            onDimensionSwitchClicked();
        }

        QColor userColor = dialog.currentColor();
        QColor realColor = userColor;
        realColor.setGreen(userColor.red());
        realColor.setBlue(userColor.red());
        realColor.setAlpha(255);

        if (getKnob()->isEnabled(0)) {
            _rBox->setValue(Natron::Color::from_func_srgb(realColor.redF()));
        }

        if (_dimension >= 3) {
            if (getKnob()->isEnabled(1)) {
                _gBox->setValue(Natron::Color::from_func_srgb(userColor.greenF()));
            }
            if (getKnob()->isEnabled(2)) {
                _bBox->setValue(Natron::Color::from_func_srgb(userColor.blueF()));
            }
            realColor.setGreen(userColor.green());
            realColor.setBlue(userColor.blue());
        }
        if (_dimension >= 4) {
            ///Don't set alpha since the color dialog can only handle RGB
//            if (getKnob()->isEnabled(3)) {
//                _aBox->setValue(userColor.alphaF()); // no conversion, alpha is linear
//            }
//            realColor.setAlpha(userColor.alpha());
        }

        onColorChanged();
        
        knob->endChanges();

    }
    knob->evaluateValueChange(0, eValueChangedReasonNatronGuiEdited);
} // showColorDialog

void
Color_KnobGui::onDialogCurrentColorChanged(const QColor & color)
{
    boost::shared_ptr<Color_Knob> knob = _knob.lock();
    knob->beginChanges();
    knob->setValue(Natron::Color::from_func_srgb(color.redF()), 0);
    if (_dimension > 1) {
        knob->setValue(Natron::Color::from_func_srgb(color.greenF()), 1);
        knob->setValue(Natron::Color::from_func_srgb(color.blueF()), 2);
        ///Don't set alpha since the color dialog can only handle RGB
//        if (_dimension > 3) {
//            _knob->setValue(color.alphaF(), 3); // no conversion, alpha is linear
//        }
    }
    knob->endChanges();
}

void
Color_KnobGui::onColorChanged()
{
    SpinBox* isSpinbox = qobject_cast<SpinBox*>(sender());
    
    
    std::list<double> newValues;
    double r = _rBox->value();
    double g = r;
    double b = r;
    double a = r;
    newValues.push_back(r);
    if ( _dimensionSwitchButton->isChecked() ) {
        if (_dimension >= 3) {
            g = _gBox->value();
            b = _bBox->value();
        }
        if (_dimension >= 4) {
            a = _aBox->value();
        }
    } else {
        if (isSpinbox) {
            _slider->seekScalePosition(r);
        }
    }
    if (_dimension >= 3) {
        newValues.push_back(g);
        newValues.push_back(b);
    }
    if (_dimension >= 4) {
        newValues.push_back(a);
    }
    std::list<double> oldValues = _knob.lock()->getValueForEachDimension_mt_safe();
    assert(oldValues.size() == newValues.size());
    std::list<double>::iterator itNew = newValues.begin();
    for (std::list<double>::iterator it = oldValues.begin() ; it != oldValues.end() ; ++it, ++itNew) {
        if (*it != *itNew) {
            updateLabel(r, g, b, a);
            pushUndoCommand( new KnobUndoCommand<double>(this, oldValues, newValues,false) );
            break;
        }
    }
    
    
}

void
Color_KnobGui::updateLabel(double r, double g, double b, double a)
{
    QColor color;
    boost::shared_ptr<Color_Knob> knob = _knob.lock();
    bool simple = knob->isSimplified();
    color.setRgbF(Natron::clamp<qreal>(simple ? r : Natron::Color::to_func_srgb(r), 0., 1.),
                  Natron::clamp<qreal>(simple ? g : Natron::Color::to_func_srgb(g), 0., 1.),
                  Natron::clamp<qreal>(simple ? b : Natron::Color::to_func_srgb(b), 0., 1.),
                  Natron::clamp<qreal>(a, 0., 1.));
    _colorLabel->setColor(color);
}

void
Color_KnobGui::_hide()
{
    _rBox->hide();
    _rLabel->hide();
    _slider->hide();
    if (_dimension >= 3) {
        _gBox->hide();
        _gLabel->hide();
        _bBox->hide();
        _bLabel->hide();
    }
    if (_dimension >= 4) {
        _aBox->hide();
        _aLabel->hide();
    }

    _dimensionSwitchButton->hide();
    _colorLabel->hide();
    _colorDialogButton->hide();
}

void
Color_KnobGui::_show()
{
    if (!_knob.lock()->isSimplified()) {
        bool areAllDimensionShown = _dimensionSwitchButton->isChecked();
        
        _rBox->show();
        if (areAllDimensionShown) {
            _rLabel->show();
            if (_dimension >= 3) {
                _gBox->show();
                _gLabel->show();
                _bBox->show();
                _bLabel->show();
            }
            if (_dimension >= 4) {
                _aBox->show();
                _aLabel->show();
            }
        } else {
            _slider->show();
        }
        
        _dimensionSwitchButton->show();
    }
    _colorLabel->show();
    _colorDialogButton->show();
}

ColorPickerLabel::ColorPickerLabel(Color_KnobGui* knob,QWidget* parent)
    : Natron::Label(parent)
      , _pickingEnabled(false)
    , _knob(knob)
{
    setMouseTracking(true);
}

void
ColorPickerLabel::mousePressEvent(QMouseEvent*)
{
    if (!_knob) {
        return;
    }
    _pickingEnabled = !_pickingEnabled;
    Q_EMIT pickingEnabled(_pickingEnabled);
    setColor(_currentColor); //< refresh the icon
}

void
ColorPickerLabel::enterEvent(QEvent*)
{
    QToolTip::showText( QCursor::pos(), toolTip() );
}

void
ColorPickerLabel::leaveEvent(QEvent*)
{
    QToolTip::hideText();
}

void
ColorPickerLabel::setPickingEnabled(bool enabled)
{
    if (!isEnabled()) {
        return;
    }
    _pickingEnabled = enabled;
    setColor(_currentColor);
}

void
ColorPickerLabel::setEnabledMode(bool enabled)
{
    setEnabled(enabled);
    if (!enabled && _pickingEnabled) {
        _pickingEnabled = false;
        setColor(_currentColor);
        if (_knob) {
            _knob->getGui()->removeColorPicker(boost::dynamic_pointer_cast<Color_Knob>(_knob->getKnob()));
        }
    }
}

void
ColorPickerLabel::setColor(const QColor & color)
{
    _currentColor = color;

    if (_pickingEnabled && _knob) {
        //draw the picker on top of the label
        QPixmap pickerIcon;
        appPTR->getIcon(Natron::NATRON_PIXMAP_COLOR_PICKER, &pickerIcon);
        QImage pickerImg = pickerIcon.toImage();
        QImage img(pickerIcon.width(), pickerIcon.height(), QImage::Format_ARGB32);
        img.fill( color.rgb() );
        
        
        int h = pickerIcon.height();
        int w = pickerIcon.width();
        for (int i = 0; i < h; ++i) {
            const QRgb* data = (QRgb*)pickerImg.scanLine(i);
            if ((i == 0) || (i == h -1)) {
                for (int j = 0; j < w; ++j) {
                    img.setPixel(j, i, qRgb(0, 0, 0));
                }
            } else {
                for (int j = 0; j < w; ++j) {
                    if ((j == 0) || (j == w -1)) {
                        img.setPixel(j, i, qRgb(0,0,0));
                    } else {
                        int alpha = qAlpha(data[j]);
                        if (alpha > 0) {
                            img.setPixel(j, i, data[j]);
                        }
                    }
                }
            }
        }
        QPixmap pix = QPixmap::fromImage(img); //.scaled(20, 20);
        setPixmap(pix);
    } else {
        QImage img(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE, QImage::Format_ARGB32);
        int h = img.height();
        int w = img.width();
        QRgb c = color.rgb();
        for (int i = 0; i < h; ++i) {
            if ((i == 0) || (i == h -1)) {
                for (int j = 0; j < w; ++j) {
                    img.setPixel(j, i, qRgb(0, 0, 0));
                }
            } else {
                for (int j = 0; j < w; ++j) {
                    if ((j == 0) || (j == w -1)) {
                        img.setPixel(j, i, qRgb(0,0,0));
                    } else {
                        img.setPixel(j, i, c);
                    }
                }
            }
        }
        QPixmap pix = QPixmap::fromImage(img);
        setPixmap(pix);
    }
}

void
Color_KnobGui::setPickingEnabled(bool enabled)
{
    if (_colorLabel->isPickingEnabled() == enabled) {
        return;
    }
    _colorLabel->setPickingEnabled(enabled);
    onPickingEnabled(enabled);
}

void
Color_KnobGui::onPickingEnabled(bool enabled)
{
    if ( getKnob()->getHolder()->getApp() ) {
        if (enabled) {
            getGui()->registerNewColorPicker(_knob.lock());
        } else {
            getGui()->removeColorPicker(_knob.lock());
        }
    }
}

void
Color_KnobGui::setReadOnly(bool readOnly,
                           int dimension)
{
    if ( (dimension == 0) && _rBox ) {
        _rBox->setReadOnly(readOnly);
    } else if ( (dimension == 1) && _gBox ) {
        _gBox->setReadOnly(readOnly);
    } else if ( (dimension == 2) && _bBox ) {
        _bBox->setReadOnly(readOnly);
    } else if ( (dimension == 3) && _aBox ) {
        _aBox->setReadOnly(readOnly);
    } else {
        assert(false); //< dim invalid
    }
}

void
Color_KnobGui::setDirty(bool dirty)
{
    _rBox->setDirty(dirty);
    if (_dimension > 1) {
        _gBox->setDirty(dirty);
        _bBox->setDirty(dirty);
    }
    if (_dimension > 3) {
        _aBox->setDirty(dirty);
    }
}

boost::shared_ptr<KnobI> Color_KnobGui::getKnob() const
{
    return _knob.lock();
}

void
Color_KnobGui::reflectModificationsState()
{
    bool hasModif = _knob.lock()->hasModifications();
    
    if (_rLabel) {
        _rLabel->setAltered(!hasModif);
    }
    _rBox->setAltered(!hasModif);
    if (_dimension > 1) {
        if (_gLabel) {
            _gLabel->setAltered(!hasModif);
        }
        _gBox->setAltered(!hasModif);
        if (_bLabel) {
            _bLabel->setAltered(!hasModif);
        }
        _bBox->setAltered(!hasModif);
    }
    if (_dimension > 3) {
        if (_aLabel) {
            _aLabel->setAltered(!hasModif);
        }
        _aBox->setAltered(!hasModif);
    }
    if (_slider) {
        _slider->setAltered(!hasModif);
    }
}

//=============================STRING_KNOB_GUI===================================

void
AnimatingTextEdit::setAnimation(int v)
{
    animation = v;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

void
AnimatingTextEdit::setReadOnlyNatron(bool ro)
{
    setReadOnly(ro);
    readOnlyNatron = ro;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

void
AnimatingTextEdit::setDirty(bool b)
{
    dirty = b;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

void
AnimatingTextEdit::focusOutEvent(QFocusEvent* e)
{
    if (_hasChanged) {
        _hasChanged = false;
        Q_EMIT editingFinished();
    }
    QTextEdit::focusOutEvent(e);
}

void
AnimatingTextEdit::keyPressEvent(QKeyEvent* e)
{
    if (modCASIsControl(e) && e->key() == Qt::Key_Return) {
        if (_hasChanged) {
            _hasChanged = false;
            Q_EMIT editingFinished();
        }
    }
    _hasChanged = true;
    QTextEdit::keyPressEvent(e);
}

void
AnimatingTextEdit::paintEvent(QPaintEvent* e)
{
    QPalette p = this->palette();
    QColor c(200,200,200,255);

    p.setColor( QPalette::Highlight, c );
    //p.setColor( QPalette::HighlightedText, c );
    this->setPalette( p );
    QTextEdit::paintEvent(e);
}

String_KnobGui::String_KnobGui(boost::shared_ptr<KnobI> knob,
                               DockablePanel *container)
    : KnobGui(knob, container)
      , _lineEdit(0)
      , _container(0)
      , _mainLayout(0)
      , _textEdit(0)
      , _richTextOptions(0)
      , _richTextOptionsLayout(0)
      , _fontCombo(0)
      , _setBoldButton(0)
      , _setItalicButton(0)
      , _fontSizeSpinBox(0)
      , _fontColorButton(0)
      , _fontSize(0)
      , _boldActivated(false)
      , _italicActivated(false)
      , _label(0)
{
    _knob = boost::dynamic_pointer_cast<String_Knob>(knob);
}

void
String_KnobGui::createWidget(QHBoxLayout* layout)
{
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    if ( knob->isMultiLine() ) {
        _container = new QWidget( layout->parentWidget() );
        _mainLayout = new QVBoxLayout(_container);
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->setSpacing(0);

        bool useRichText = knob->usesRichText();
        _textEdit = new AnimatingTextEdit(_container);
        _textEdit->setAcceptRichText(useRichText);


        _mainLayout->addWidget(_textEdit);

        QObject::connect( _textEdit, SIGNAL( editingFinished() ), this, SLOT( onTextChanged() ) );
       // layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        _textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_textEdit,0);

        if (useRichText) {
            _richTextOptions = new QWidget(_container);
            _richTextOptionsLayout = new QHBoxLayout(_richTextOptions);
            _richTextOptionsLayout->setContentsMargins(0, 0, 0, 0);
            _richTextOptionsLayout->setSpacing(8);

            _fontCombo = new QFontComboBox(_richTextOptions);
            _fontCombo->setCurrentFont(QApplication::font());
            _fontCombo->setToolTip(Natron::convertFromPlainText(tr("Font."), Qt::WhiteSpaceNormal));
            _richTextOptionsLayout->addWidget(_fontCombo);

            _fontSizeSpinBox = new SpinBox(_richTextOptions);
            _fontSizeSpinBox->setMinimum(1);
            _fontSizeSpinBox->setMaximum(100);
            _fontSizeSpinBox->setValue(6);
            QObject::connect( _fontSizeSpinBox,SIGNAL( valueChanged(double) ),this,SLOT( onFontSizeChanged(double) ) );
            _fontSizeSpinBox->setToolTip(Natron::convertFromPlainText(tr("Font size."), Qt::WhiteSpaceNormal));
            _richTextOptionsLayout->addWidget(_fontSizeSpinBox);

            QPixmap pixBoldChecked,pixBoldUnchecked,pixItalicChecked,pixItalicUnchecked;
            appPTR->getIcon(NATRON_PIXMAP_BOLD_CHECKED,&pixBoldChecked);
            appPTR->getIcon(NATRON_PIXMAP_BOLD_UNCHECKED,&pixBoldUnchecked);
            appPTR->getIcon(NATRON_PIXMAP_ITALIC_CHECKED,&pixItalicChecked);
            appPTR->getIcon(NATRON_PIXMAP_ITALIC_UNCHECKED,&pixItalicUnchecked);
            QIcon boldIcon;
            boldIcon.addPixmap(pixBoldChecked,QIcon::Normal,QIcon::On);
            boldIcon.addPixmap(pixBoldUnchecked,QIcon::Normal,QIcon::Off);
            _setBoldButton = new Button(boldIcon,"",_richTextOptions);
            _setBoldButton->setCheckable(true);
            _setBoldButton->setToolTip(Natron::convertFromPlainText(tr("Bold."), Qt::WhiteSpaceNormal));
            _setBoldButton->setMaximumSize(18, 18);
            QObject::connect( _setBoldButton,SIGNAL( clicked(bool) ),this,SLOT( boldChanged(bool) ) );
            _richTextOptionsLayout->addWidget(_setBoldButton);

            QIcon italicIcon;
            italicIcon.addPixmap(pixItalicChecked,QIcon::Normal,QIcon::On);
            italicIcon.addPixmap(pixItalicUnchecked,QIcon::Normal,QIcon::Off);

            _setItalicButton = new Button(italicIcon,"",_richTextOptions);
            _setItalicButton->setCheckable(true);
            _setItalicButton->setToolTip(Natron::convertFromPlainText(tr("Italic."), Qt::WhiteSpaceNormal));
            _setItalicButton->setMaximumSize(18,18);
            QObject::connect( _setItalicButton,SIGNAL( clicked(bool) ),this,SLOT( italicChanged(bool) ) );
            _richTextOptionsLayout->addWidget(_setItalicButton);

            QPixmap pixBlack(15,15);
            pixBlack.fill(Qt::black);
            _fontColorButton = new Button(QIcon(pixBlack),"",_richTextOptions);
            _fontColorButton->setCheckable(false);
            _fontColorButton->setToolTip(Natron::convertFromPlainText(tr("Font color."), Qt::WhiteSpaceNormal));
            _fontColorButton->setMaximumSize(18, 18);
            QObject::connect( _fontColorButton, SIGNAL( clicked(bool) ), this, SLOT( colorFontButtonClicked() ) );
            _richTextOptionsLayout->addWidget(_fontColorButton);

            _richTextOptionsLayout->addStretch();

            _mainLayout->addWidget(_richTextOptions);

            restoreTextInfoFromString();

            ///Connect the slot after restoring
            QObject::connect( _fontCombo,SIGNAL( currentFontChanged(QFont) ),this,SLOT( onCurrentFontChanged(QFont) ) );
        }

        layout->addWidget(_container);
    } else if ( knob->isLabel() ) {
        _label = new Natron::Label( layout->parentWidget() );

        if ( hasToolTip() ) {
            _label->setToolTip( toolTip() );
        }
        //_label->setFont(QFont(appFont,appFontSize));
        layout->addWidget(_label);
    } else {
        _lineEdit = new LineEdit( layout->parentWidget() );

        if ( hasToolTip() ) {
            _lineEdit->setToolTip( toolTip() );
        }
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(_lineEdit);
        QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onLineChanged() ) );

        if ( knob->isCustomKnob() ) {
            _lineEdit->setReadOnly(true);
        }

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_lineEdit,0);
    }
} // createWidget

String_KnobGui::~String_KnobGui()
{
}

void String_KnobGui::removeSpecificGui()
{
    delete _lineEdit;
    delete _label;
    delete _container;

}

void
String_KnobGui::onLineChanged()
{
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob.lock()->getGuiValue(0),_lineEdit->text().toStdString() ) );
}

QString
String_KnobGui::stripWhitespaces(const QString & str)
{
    ///QString::trimmed() doesn't do the job because it doesn't leave the last character
    ///The code is taken from QString::trimmed
    const QChar* s = str.data();

    if ( !s->isSpace() && !s[str.size() - 1].isSpace() ) {
        return str;
    }

    int start = 0;
    int end = str.size() - 2; ///< end before the last character so we don't remove it

    while ( start <= end && s[start].isSpace() ) { // skip white space from start
        ++start;
    }

    if (start <= end) {                          // only white space
        while ( end && s[end].isSpace() ) {           // skip white space from end
            --end;
        }
    }
    int l = end - start + 2;
    if (l <= 0) {
        return QString();
    }

    return QString(s + start, l);
}

void
String_KnobGui::onTextChanged()
{
    QString txt = _textEdit->toPlainText();

    txt = stripWhitespaces(txt);
    if ( _knob.lock()->usesRichText() ) {
        txt = addHtmlTags(txt);
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob.lock()->getGuiValue(0),txt.toStdString() ) );
}

QString
String_KnobGui::addHtmlTags(QString text) const
{
    QString fontTag = makeFontTag(_fontFamily, _fontSize, _fontColor);

    text.prepend(fontTag);
    text.append(kFontEndTag);

    if (_boldActivated) {
        text.prepend(kBoldStartTag);
        text.append(kBoldEndTag);
    }
    if (_italicActivated) {
        text.prepend(kItalicStartTag);
        text.append(kItalicEndTag);
    }

    ///if the knob had custom data, set them
    QString knobOldtext( _knob.lock()->getGuiValue(0).c_str() );
    QString startCustomTag(NATRON_CUSTOM_HTML_TAG_START);
    int startCustomData = knobOldtext.indexOf(startCustomTag);
    if (startCustomData != -1) {
        QString customEndTag(NATRON_CUSTOM_HTML_TAG_END);
        int endCustomData = knobOldtext.indexOf(customEndTag,startCustomData);
        assert(endCustomData != -1);
        startCustomData += startCustomTag.size();

        int fontStart = text.indexOf(kFontSizeTag);
        assert(fontStart != -1);

        QString endFontTag("\">");
        int fontTagEnd = text.indexOf(endFontTag,fontStart);
        assert(fontTagEnd != -1);
        fontTagEnd += endFontTag.size();

        QString customData = knobOldtext.mid(startCustomData,endCustomData - startCustomData);

        text.insert(fontTagEnd, startCustomTag);
        text.insert(fontTagEnd + startCustomTag.size(), customData);
        text.insert(fontTagEnd + startCustomTag.size() + customData.size(), customEndTag);
    }

    return text;
}

void
String_KnobGui::restoreTextInfoFromString()
{
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    QString text( knob->getGuiValue(0).c_str() );

    if ( text.isEmpty() ) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>( knob->getHolder() );
        /// If the node has a sublabel, restore it in the label
        if ( effect && (knob->getName() == kUserLabelKnobName) ) {
            boost::shared_ptr<KnobI> knob = effect->getKnobByName(kNatronOfxParamStringSublabelName);
            if (knob) {
                String_Knob* strKnob = dynamic_cast<String_Knob*>( knob.get() );
                if (strKnob) {
                    QString sublabel = strKnob->getGuiValue(0).c_str();
                    text.append(NATRON_CUSTOM_HTML_TAG_START);
                    text.append('(' + sublabel + ')');
                    text.append(NATRON_CUSTOM_HTML_TAG_END);
                }
            }
        }


        _fontSize = _fontSizeSpinBox->value();
        _fontColor = Qt::black;
        _fontFamily = _fontCombo->currentFont().family();
        _boldActivated = false;
        _italicActivated = false;
        QString fontTag = makeFontTag(_fontFamily, _fontSize, _fontColor);
        text.prepend(fontTag);
        text.append(kFontEndTag);


        knob->setValue(text.toStdString(), 0);
    } else {
        QString toFind = QString(kItalicStartTag);
        int i = text.indexOf(toFind);
        if (i != -1) {
            _italicActivated = true;
        } else {
            _italicActivated = false;
        }

        _setItalicButton->setChecked(_italicActivated);
        _setItalicButton->setDown(_italicActivated);

        toFind = QString(kBoldStartTag);
        i = text.indexOf(toFind);
        if (i != -1) {
            _boldActivated = true;
        } else {
            _boldActivated = false;
        }

        _setBoldButton->setChecked(_boldActivated);
        _setBoldButton->setDown(_boldActivated);

        QString fontSizeString;
        QString fontColorString;
        toFind = QString(kFontSizeTag);
        i = text.indexOf(toFind);
        bool foundFontTag = false;
        if (i != -1) {
            foundFontTag = true;
            i += toFind.size();
            while ( i < text.size() && text.at(i).isDigit() ) {
                fontSizeString.append( text.at(i) );
                ++i;
            }
        }
        toFind = QString(kFontColorTag);
        i = text.indexOf(toFind,i);
        assert( (!foundFontTag && i == -1) || (foundFontTag && i != -1) );
        if (i != -1) {
            i += toFind.size();
            while ( i < text.size() && text.at(i) != QChar('"') ) {
                fontColorString.append( text.at(i) );
                ++i;
            }
        }
        toFind = QString(kFontFaceTag);
        i = text.indexOf(toFind,i);
        assert( (!foundFontTag && i == -1) || (foundFontTag && i != -1) );
        if (i != -1) {
            i += toFind.size();
            while ( i < text.size() && text.at(i) != QChar('"') ) {
                _fontFamily.append( text.at(i) );
                ++i;
            }
        }

        if (!foundFontTag) {
            _fontSize = _fontSizeSpinBox->value();
            _fontColor = Qt::black;
            _fontFamily = _fontCombo->currentFont().family();
            _boldActivated = false;
            _italicActivated = false;
            QString fontTag = makeFontTag(_fontFamily, _fontSize, _fontColor);
            text.prepend(fontTag);
            text.append(kFontEndTag);
            knob->setValue(text.toStdString(), 0);
        } else {
            _fontCombo->setCurrentFont( QFont(_fontFamily) );

            _fontSize = fontSizeString.toInt();

            _fontSizeSpinBox->setValue(_fontSize);

            _fontColor = QColor(fontColorString);
        }


        updateFontColorIcon(_fontColor);
    }
} // restoreTextInfoFromString

void
String_KnobGui::parseFont(const QString & label,
                          QFont *f,
                          QColor *color)
{
    QString toFind = QString(kFontSizeTag);
    int startFontTag = label.indexOf(toFind);

    assert(startFontTag != -1);
    startFontTag += toFind.size();
    int j = startFontTag;
    QString sizeStr;
    while ( j < label.size() && label.at(j).isDigit() ) {
        sizeStr.push_back( label.at(j) );
        ++j;
    }

    toFind = QString(kFontFaceTag);
    startFontTag = label.indexOf(toFind,startFontTag);
    assert(startFontTag != -1);
    startFontTag += toFind.size();
    j = startFontTag;
    QString faceStr;
    while ( j < label.size() && label.at(j) != QChar('"') ) {
        faceStr.push_back( label.at(j) );
        ++j;
    }

    f->setPointSize( sizeStr.toInt() );
    f->setFamily(faceStr);
    
    {
        toFind = QString(kBoldStartTag);
        int foundBold = label.indexOf(toFind);
        if (foundBold != -1) {
            f->setBold(true);
        }
    }
    
    {
        toFind = QString(kItalicStartTag);
        int foundItalic = label.indexOf(toFind);
        if (foundItalic != -1) {
            f->setItalic(true);
        }
    }
    {
        toFind = QString(kFontColorTag);
        int foundColor = label.indexOf(toFind);
        if (foundColor != -1) {
            foundColor += toFind.size();
            QString currentColor;
            int j = foundColor;
            while ( j < label.size() && label.at(j) != QChar('"') ) {
                currentColor.push_back( label.at(j) );
                ++j;
            }
            *color = QColor(currentColor);
        }
    }
}

void
String_KnobGui::updateFontColorIcon(const QColor & color)
{
    QPixmap p(18,18);

    p.fill(color);
    _fontColorButton->setIcon( QIcon(p) );
}

void
String_KnobGui::onCurrentFontChanged(const QFont & font)
{
    
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    assert(_textEdit);
    QString text( knob->getGuiValue(0).c_str() );
    //find the first font tag
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);
    _fontFamily = font.family();
    if (i != -1) {
        toFind = QString(kFontFaceTag);
        i = text.indexOf(toFind,i);
        assert(i != -1);
        i += toFind.size();
        ///erase the current font face (family)
        QString currentFontFace;
        int j = i;
        while ( j < text.size() && text.at(j) != QChar('"') ) {
            currentFontFace.push_back( text.at(j) );
            ++j;
        }
        text.remove( i, currentFontFace.size() );
        text.insert( i != -1 ? i : 0, font.family() );
    } else {
        QString fontTag = makeFontTag(_fontFamily,_fontSize,_fontColor);
        text.prepend(fontTag);
        text.append(kFontEndTag);
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getGuiValue(0),text.toStdString() ) );
}

QString
String_KnobGui::makeFontTag(const QString& family,int fontSize,const QColor& color)
{
    return QString(kFontSizeTag "%1\" " kFontColorTag "%2\" " kFontFaceTag "%3\">")
    .arg(fontSize)
    .arg( color.name() )
    .arg(family);
}

QString
String_KnobGui::decorateTextWithFontTag(const QString& family,int fontSize,const QColor& color,const QString& text)
{
    return makeFontTag(family, fontSize, color) + text + kFontEndTag;
}

void
String_KnobGui::onFontSizeChanged(double size)
{
    assert(_textEdit);
    boost::shared_ptr<String_Knob> knob = _knob.lock();;
    QString text( knob->getGuiValue(0).c_str() );
    //find the first font tag
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);
    assert(i != -1);
    i += toFind.size();
    ///erase the current font face (family)
    QString currentSize;
    int j = i;
    while ( j < text.size() && text.at(j).isDigit() ) {
        currentSize.push_back( text.at(j) );
        ++j;
    }
    text.remove( i, currentSize.size() );
    text.insert( i, QString::number(size) );
    _fontSize = size;
    pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getGuiValue(0),text.toStdString() ) );
}

void
String_KnobGui::boldChanged(bool toggled)
{
    assert(_textEdit);
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    QString text( knob->getGuiValue(0).c_str() );
    QString toFind = QString(kBoldStartTag);
    int i = text.indexOf(toFind);

    assert( (toggled && i == -1) || (!toggled && i != -1) );

    if (!toggled) {
        text.remove( i, toFind.size() );
        toFind = QString(kBoldEndTag);
        i = text.lastIndexOf(toFind);
        assert (i != -1);
        text.remove( i,toFind.size() );
    } else {
        ///insert right prior to the font size
        toFind = QString(kFontSizeTag);
        i = text.indexOf(toFind);
        assert(i != -1);
        text.insert(i, kBoldStartTag);
        toFind = QString(kFontEndTag);
        i = text.lastIndexOf(toFind);
        assert(i != -1);
        i += toFind.size();
        text.insert(i, kBoldEndTag);
    }

    _boldActivated = toggled;
    pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getGuiValue(0),text.toStdString() ) );
}

void
String_KnobGui::mergeFormat(const QTextCharFormat & fmt)
{
    QTextCursor cursor = _textEdit->textCursor();

    if ( cursor.hasSelection() ) {
        cursor.mergeCharFormat(fmt);
        _textEdit->mergeCurrentCharFormat(fmt);
    }
}

void
String_KnobGui::colorFontButtonClicked()
{
    assert(_textEdit);
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    QColorDialog dialog(_textEdit);
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( updateFontColorIcon(QColor) ) );
    dialog.setCurrentColor(_fontColor);
    if ( dialog.exec() ) {
        _fontColor = dialog.currentColor();

        QString text( knob->getGuiValue(0).c_str() );
        findReplaceColorName(text,_fontColor.name());
        pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getGuiValue(0),text.toStdString() ) );
    }
    updateFontColorIcon(_fontColor);
}

void
String_KnobGui::findReplaceColorName(QString& text,const QColor& color)
{
    //find the first font tag
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);
    if (i != -1) {
        toFind = QString(kFontColorTag);
        int foundColorTag = text.indexOf(toFind,i);
        if (foundColorTag != -1) {
            foundColorTag += toFind.size();
            QString currentColor;
            int j = foundColorTag;
            while ( j < text.size() && text.at(j) != QChar('"') ) {
                currentColor.push_back( text.at(j) );
                ++j;
            }
            text.remove( foundColorTag,currentColor.size() );
            text.insert( foundColorTag, color.name() );
        } else {
            text.insert(i, kFontColorTag);
            text.insert(i + toFind.size(), color.name() + "\"");
        }
    }
    
    
}

void
String_KnobGui::italicChanged(bool toggled)
{
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    QString text( knob->getGuiValue(0).c_str() );
    //find the first font tag
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);

    assert(i != -1);

    ///search before i
    toFind = QString(kItalicStartTag);
    int foundItalic = text.lastIndexOf(toFind,i);
    assert( (toggled && foundItalic == -1) || (!toggled && foundItalic != -1) );
    if (!toggled) {
        text.remove( foundItalic, toFind.size() );
        toFind = QString(kItalicEndTag);
        foundItalic = text.lastIndexOf(toFind);
        assert(foundItalic != -1);
        text.remove( foundItalic, toFind.size() );
    } else {
        int foundBold = text.lastIndexOf(kBoldStartTag,i);
        assert( (foundBold == -1 && !_boldActivated) || (foundBold != -1 && _boldActivated) );

        ///if bold is activated, insert prior to bold
        if (foundBold != -1) {
            foundBold = foundBold == 0 ? 0 : foundBold - 1;
            text.insert(foundBold, kItalicStartTag);
        } else {
            //there's no bold
            i = i == 0 ? 0 : i - 1;
            text.insert(i, kItalicStartTag);
        }
        text.append(kItalicEndTag); //< this is always the last tag
    }
    _italicActivated = toggled;
    pushUndoCommand( new KnobUndoCommand<std::string>( this,knob->getGuiValue(0),text.toStdString() ) );
}

QString
String_KnobGui::removeNatronHtmlTag(QString text)
{
    ///we also remove any custom data added by natron so the user doesn't see it
    int startCustomData = text.indexOf(NATRON_CUSTOM_HTML_TAG_START);

    if (startCustomData != -1) {
        QString endTag(NATRON_CUSTOM_HTML_TAG_END);
        int endCustomData = text.indexOf(endTag,startCustomData);
        assert(endCustomData != -1);
        endCustomData += endTag.size();
        text.remove(startCustomData, endCustomData - startCustomData);
    }

    return text;
}

QString
String_KnobGui::getNatronHtmlTagContent(QString text)
{
    QString label = removeAutoAddedHtmlTags(text,false);
    QString startTag(NATRON_CUSTOM_HTML_TAG_START);
    int startCustomData = label.indexOf(startTag);
    if (startCustomData != -1) {
        QString endTag(NATRON_CUSTOM_HTML_TAG_END);
        int endCustomData = label.indexOf(endTag,startCustomData);
        assert(endCustomData != -1);
        label = label.remove(endCustomData, endTag.size());
        label = label.remove(startCustomData, startTag.size());
    }
    return label;
}


QString
String_KnobGui::removeAutoAddedHtmlTags(QString text,bool removeNatronTag)
{
    QString toFind = QString(kFontSizeTag);
    int i = text.indexOf(toFind);
    bool foundFontStart = i != -1;
    QString boldStr(kBoldStartTag);
    int foundBold = text.lastIndexOf(boldStr,i);

    ///Assert removed: the knob might be linked from elsewhere and the button might not have been pressed.
    //assert((foundBold == -1 && !_boldActivated) || (foundBold != -1 && _boldActivated));

    if (foundBold != -1) {
        text.remove( foundBold, boldStr.size() );
        boldStr = QString(kBoldEndTag);
        foundBold = text.lastIndexOf(boldStr);
        assert(foundBold != -1);
        text.remove( foundBold,boldStr.size() );
    }

    ///refresh the index
    i = text.indexOf(toFind);

    QString italStr(kItalicStartTag);
    int foundItal = text.lastIndexOf(italStr,i);

    //Assert removed: the knob might be linked from elsewhere and the button might not have been pressed.
    // assert((_italicActivated && foundItal != -1) || (!_italicActivated && foundItal == -1));

    if (foundItal != -1) {
        text.remove( foundItal, italStr.size() );
        italStr = QString(kItalicEndTag);
        foundItal = text.lastIndexOf(italStr);
        assert(foundItal != -1);
        text.remove( foundItal,italStr.size() );
    }

    ///refresh the index
    i = text.indexOf(toFind);

    QString endTag("\">");
    int foundEndTag = text.indexOf(endTag,i);
    foundEndTag += endTag.size();
    if (foundFontStart) {
        ///remove the whole font tag
        text.remove(i,foundEndTag - i);
    }

    endTag = QString(kFontEndTag);
    foundEndTag = text.lastIndexOf(endTag);
    assert( (foundEndTag != -1 && foundFontStart) || !foundFontStart );
    if (foundEndTag != -1) {
        text.remove( foundEndTag, endTag.size() );
    }

    ///we also remove any custom data added by natron so the user doesn't see it
    if (removeNatronTag) {
        return removeNatronHtmlTag(text);
    } else {
        return text;
    }
} // removeAutoAddedHtmlTags

void
String_KnobGui::updateGUI(int /*dimension*/)
{
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    std::string value = knob->getGuiValue(0);

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->blockSignals(true);
        QTextCursor cursor = _textEdit->textCursor();
        int pos = cursor.position();
        int selectionStart = cursor.selectionStart();
        int selectionEnd = cursor.selectionEnd();
        QString txt( value.c_str() );
        if (_knob.lock()->usesRichText()) {
            txt = removeAutoAddedHtmlTags(txt);
        }

        _textEdit->setPlainText(txt);

        if ( pos < txt.size() ) {
            cursor.setPosition(pos);
        } else {
            cursor.movePosition(QTextCursor::End);
        }

        ///restore selection
        cursor.setPosition(selectionStart);
        cursor.setPosition(selectionEnd,QTextCursor::KeepAnchor);

        _textEdit->setTextCursor(cursor);
        _textEdit->blockSignals(false);
    } else if ( knob->isLabel() ) {
        assert(_label);
        QString txt = value.c_str();
        txt.replace("\n", "<br>");
        _label->setText(txt);
    } else {
        assert(_lineEdit);
        _lineEdit->setText( value.c_str() );
    }
}

void
String_KnobGui::_hide()
{
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->hide();
    } else if ( knob->isLabel() ) {
        assert(_label);
        _label->hide();
    } else {
        assert(_lineEdit);
        _lineEdit->hide();
    }
}

void
String_KnobGui::_show()
{
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->show();
    } else if ( knob->isLabel() ) {
        assert(_label);
        _label->show();
    } else {
        assert(_lineEdit);
        _lineEdit->show();
    }
}

void
String_KnobGui::setEnabled()
{
    boost::shared_ptr<String_Knob> knob = _knob.lock();
    bool b = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        //_textEdit->setEnabled(b);
        //_textEdit->setReadOnly(!b);
        _textEdit->setReadOnlyNatron(!b);
    } else if ( knob->isLabel() ) {
        assert(_label);
        _label->setEnabled(b);
    } else {
        assert(_lineEdit);
        //_lineEdit->setEnabled(b);
        if ( !knob->isCustomKnob() ) {
            _lineEdit->setReadOnly(!b);
        }
    }
}

void
String_KnobGui::reflectAnimationLevel(int /*dimension*/,
                                      Natron::AnimationLevelEnum level)
{
    boost::shared_ptr<String_Knob> knob = _knob.lock();
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
    
    if ( knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->setAnimation(value);
    } else if ( knob->isLabel() ) {
        assert(_label);
    } else {
        assert(_lineEdit);
        _lineEdit->setAnimation(value);
    }
}

void
String_KnobGui::setReadOnly(bool readOnly,
                            int /*dimension*/)
{
    if (_textEdit) {
        _textEdit->setReadOnlyNatron(readOnly);
    } else if (_lineEdit) {
        if ( !_knob.lock()->isCustomKnob() ) {
            _lineEdit->setReadOnly(readOnly);
        }
    }
}



void
String_KnobGui::setDirty(bool dirty)
{
    if (_textEdit) {
        _textEdit->setDirty(dirty);
    } else if (_lineEdit) {
        _lineEdit->setDirty(dirty);
    }
}

boost::shared_ptr<KnobI>
String_KnobGui::getKnob() const
{
    return _knob.lock();
}

void
String_KnobGui::reflectExpressionState(int /*dimension*/,
                                       bool hasExpr)
{
    bool isSlaved = _knob.lock()->isSlave(0);
    if (_textEdit) {
        _textEdit->setAnimation(3);
        _textEdit->setReadOnly(hasExpr || isSlaved);
    } else if (_lineEdit) {
        _lineEdit->setAnimation(3);
        _lineEdit->setReadOnly(hasExpr || isSlaved);
    }
}

void
String_KnobGui::updateToolTip()
{
    if (hasToolTip()) {
        QString tt = toolTip();
        if (_textEdit) {
            bool useRichText = _knob.lock()->usesRichText();
            if (useRichText) {
                tt += tr(" This text area supports html encoding. "
                         "Please check <a href=http://qt-project.org/doc/qt-5/richtext-html-subset.html>Qt website</a> for more info. ");
            }
            QKeySequence seq(Qt::CTRL + Qt::Key_Return);
            tt += tr("Use ") + seq.toString(QKeySequence::NativeText) + tr(" to validate changes made to the text. ");
            _textEdit->setToolTip(tt);
        } else if (_lineEdit) {
            _lineEdit->setToolTip(tt);
        } else if (_label) {
            _label->setToolTip(tt);
        }
    }
}

void
String_KnobGui::reflectModificationsState()
{
    bool hasModif = _knob.lock()->hasModifications();
    if (_lineEdit) {
        _lineEdit->setAltered(!hasModif);
    }
}

//=============================GROUP_KNOB_GUI===================================
GroupBoxLabel::GroupBoxLabel(QWidget *parent)
: Natron::Label(parent)
, _checked(false)

{
    QObject::connect( this, SIGNAL( checked(bool) ), this, SLOT( setChecked(bool) ) );
}

void
GroupBoxLabel::setChecked(bool b)
{
    _checked = b;
    QPixmap pix;
    if (b) {
        appPTR->getIcon(NATRON_PIXMAP_GROUPBOX_UNFOLDED, &pix);
    } else {
        appPTR->getIcon(NATRON_PIXMAP_GROUPBOX_FOLDED, &pix);
    }
    setPixmap(pix);
}

Group_KnobGui::Group_KnobGui(boost::shared_ptr<KnobI> knob,
                             DockablePanel *container)
: KnobGui(knob, container)
, _checked(false)
, _button(0)
, _children()
, _childrenToEnable()
, _tabGroup(0)
, _knob( boost::dynamic_pointer_cast<Group_Knob>(knob))
{

}

Group_KnobGui::~Group_KnobGui()
{
    
}

TabGroup*
Group_KnobGui::getOrCreateTabWidget()
{
    if (_tabGroup) {
        return _tabGroup;
    }
    
    _tabGroup = new TabGroup(getContainer());
    return _tabGroup;
}

void
Group_KnobGui::removeTabWidget()
{
    delete _tabGroup;
    _tabGroup = 0;
}

void Group_KnobGui::removeSpecificGui()
{
    delete _button;
//    for (std::list<KnobGui*>::iterator it = _children.begin() ; it != _children.end(); ++it) {
//        (*it)->removeSpecificGui();
//        delete *it;
//    }
}

void
Group_KnobGui::addKnob(KnobGui *child)
{
    _children.push_back(child);
}

bool
Group_KnobGui::isChecked() const
{
    return hasWidgetBeenCreated() ? _button->isChecked() : true;
}

void
Group_KnobGui::createWidget(QHBoxLayout* layout)
{
    _button = new GroupBoxLabel( layout->parentWidget() );
    if ( hasToolTip() ) {
        _button->setToolTip( toolTip() );
    }
    _button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _button->setChecked(_checked);
    QObject::connect( _button, SIGNAL( checked(bool) ), this, SLOT( setChecked(bool) ) );
    layout->addWidget(_button);
}

void
Group_KnobGui::setChecked(bool b)
{
    if (b == _checked) {
        return;
    }
    _checked = b;

    ///get the current index of the group knob in the layout, and reinsert
    ///the children back with an offset relative to the group.
    int realIndexInLayout = getActualIndexInLayout();
    int startChildIndex = realIndexInLayout + 1;
    
    for (std::list<KnobGui*>::iterator it = _children.begin(); it != _children.end(); ++it) {
        if (!b) {
            (*it)->hide();
        } else if ( !(*it)->getKnob()->getIsSecret() ) {
            (*it)->show(startChildIndex);
            if ( (*it)->getKnob()->isNewLineActivated() ) {
                ++startChildIndex;
            }
        }
    }
}

bool
Group_KnobGui::eventFilter(QObject */*target*/,
                           QEvent* /*event*/)
{
    //if(e->type() == QEvent::Paint){

    ///discard the paint event
    return true;
    // }
    //return QObject::eventFilter(target, event);
}

void
Group_KnobGui::updateGUI(int /*dimension*/)
{
    bool b = _knob.lock()->getGuiValue(0);

    setChecked(b);
    if (_button) {
        _button->setChecked(b);
    }
}

void
Group_KnobGui::_hide()
{
    if (_button) {
        _button->hide();
    }
    for (std::list<KnobGui*>::iterator it = _children.begin(); it != _children.end(); ++it) {
        (*it)->hide();
    }
}

void
Group_KnobGui::_show()
{
//    if ( _knob->getIsSecret() ) {
//        return;
//    }
    if (_button) {
        _button->show();
    }

    if (_checked) {
        for (std::list<KnobGui*>::iterator it = _children.begin(); it != _children.end(); ++it) {
            (*it)->show();
        }
    }
}

void
Group_KnobGui::setEnabled()
{
    boost::shared_ptr<Group_Knob> knob = _knob.lock();
    bool enabled = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    if (_button) {
        _button->setEnabled(enabled);
    }
    if (enabled) {
        for (U32 i = 0; i < _childrenToEnable.size(); ++i) {
            for (U32 j = 0; j < _childrenToEnable[i].second.size(); ++j) {
                _childrenToEnable[i].first->getKnob()->setEnabled(_childrenToEnable[i].second[j], true);
            }
        }
    } else {
        _childrenToEnable.clear();
        for (std::list<KnobGui*>::iterator it = _children.begin(); it != _children.end(); ++it) {
            std::vector<int> dimensions;
            for (int j = 0; j < (*it)->getKnob()->getDimension(); ++j) {
                if ( (*it)->getKnob()->isEnabled(j) ) {
                    (*it)->getKnob()->setEnabled(j, false);
                    dimensions.push_back(j);
                }
            }
            _childrenToEnable.push_back( std::make_pair(*it, dimensions) );
        }
    }
}

boost::shared_ptr<KnobI> Group_KnobGui::getKnob() const
{
    return _knob.lock();
}

//=============================Parametric_KnobGui===================================

Parametric_KnobGui::Parametric_KnobGui(boost::shared_ptr<KnobI> knob,
                                       DockablePanel *container)
: KnobGui(knob, container)
, treeColumn(NULL)
, _curveWidget(NULL)
, _tree(NULL)
, _resetButton(NULL)
, _curves()
{
    _knob = boost::dynamic_pointer_cast<Parametric_Knob>(knob);
    QObject::connect(_knob.lock().get(), SIGNAL(curveColorChanged(int)), this, SLOT(onColorChanged(int)));
}

Parametric_KnobGui::~Parametric_KnobGui()
{
    
}

void Parametric_KnobGui::removeSpecificGui()
{
    delete _curveWidget;
    delete treeColumn;
}

void
Parametric_KnobGui::createWidget(QHBoxLayout* layout)
{
    boost::shared_ptr<Parametric_Knob> knob = _knob.lock();
    QObject::connect( knob.get(), SIGNAL( curveChanged(int) ), this, SLOT( onCurveChanged(int) ) );

    //layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    treeColumn = new QWidget( layout->parentWidget() );
    QVBoxLayout* treeColumnLayout = new QVBoxLayout(treeColumn);
    treeColumnLayout->setContentsMargins(0, 0, 0, 0);

    _tree = new QTreeWidget( layout->parentWidget() );
    _tree->setSelectionMode(QAbstractItemView::ContiguousSelection);
    _tree->setColumnCount(1);
    _tree->header()->close();
    if ( hasToolTip() ) {
        _tree->setToolTip( toolTip() );
    }
    treeColumnLayout->addWidget(_tree);

    _resetButton = new Button("Reset",treeColumn);
    _resetButton->setToolTip( Natron::convertFromPlainText(tr("Reset the selected curves in the tree to their default shape."), Qt::WhiteSpaceNormal) );
    QObject::connect( _resetButton, SIGNAL( clicked() ), this, SLOT( resetSelectedCurves() ) );
    treeColumnLayout->addWidget(_resetButton);

    layout->addWidget(treeColumn);

    _curveWidget = new CurveWidget( getGui(),this, boost::shared_ptr<TimeLine>(),layout->parentWidget() );
    _curveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if ( hasToolTip() ) {
        _curveWidget->setToolTip( toolTip() );
    }
    layout->addWidget(_curveWidget);


    std::vector<boost::shared_ptr<CurveGui> > visibleCurves;
    for (int i = 0; i < knob->getDimension(); ++i) {
        QString curveName = knob->getDimensionName(i).c_str();
        boost::shared_ptr<KnobCurveGui> curve(new KnobCurveGui(_curveWidget,knob->getParametricCurve(i),this,i,curveName,QColor(255,255,255),1.));
        _curveWidget->addCurveAndSetColor(curve);
        QColor color;
        double r,g,b;
        knob->getCurveColor(i, &r, &g, &b);
        color.setRedF(r);
        color.setGreenF(g);
        color.setBlueF(b);
        curve->setColor(color);
        QTreeWidgetItem* item = new QTreeWidgetItem(_tree);
        item->setText(0, curveName);
        item->setSelected(true);
        CurveDescriptor desc;
        desc.curve = curve;
        desc.treeItem = item;
        _curves.insert( std::make_pair(i, desc) );
        if ( curve->isVisible() ) {
            visibleCurves.push_back(curve);
        }
    }

    _curveWidget->centerOn(visibleCurves);
    QObject::connect( _tree, SIGNAL( itemSelectionChanged() ),this,SLOT( onItemsSelectionChanged() ) );
} // createWidget

void
Parametric_KnobGui::onColorChanged(int dimension)
{
    double r, g, b;
    _knob.lock()->getCurveColor(dimension, &r, &g, &b);
    CurveGuis::iterator found = _curves.find(dimension);
    if (found != _curves.end()) {
        QColor c;
        c.setRgbF(r, g, b);
        found->second.curve->setColor(c);
    }
    _curveWidget->update();
}

void
Parametric_KnobGui::_hide()
{
    _curveWidget->hide();
    _tree->hide();
    _resetButton->hide();
}

void
Parametric_KnobGui::_show()
{
    _curveWidget->show();
    _tree->show();
    _resetButton->show();
}

void
Parametric_KnobGui::setEnabled()
{
    boost::shared_ptr<Parametric_Knob> knob = _knob.lock();
    bool b = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    _tree->setEnabled(b);
}

void
Parametric_KnobGui::updateGUI(int /*dimension*/)
{
    _curveWidget->update();
}

void
Parametric_KnobGui::onCurveChanged(int dimension)
{
    CurveGuis::iterator it = _curves.find(dimension);

    assert( it != _curves.end() );

    // even when there is only one keyframe, there may be tangents!
    if ( (it->second.curve->getInternalCurve()->getKeyFramesCount() > 0) && it->second.treeItem->isSelected() ) {
        it->second.curve->setVisible(true);
    } else {
        it->second.curve->setVisible(false);
    }
    _curveWidget->update();
}

void
Parametric_KnobGui::onItemsSelectionChanged()
{
    std::vector<boost::shared_ptr<CurveGui> > curves;

    QList<QTreeWidgetItem*> selectedItems = _tree->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->second.treeItem == selectedItems.at(i) ) {
                if ( it->second.curve->getInternalCurve()->isAnimated() ) {
                    curves.push_back(it->second.curve);
                }
                break;
            }
        }
    }

    ///find in the _curves map if an item's map the current

    _curveWidget->showCurvesAndHideOthers(curves);
    _curveWidget->centerOn(curves); //remove this if you don't want the editor to switch to a curve on a selection change
}

void
Parametric_KnobGui::getSelectedCurves(std::vector<boost::shared_ptr<CurveGui> >* selection)
{
    QList<QTreeWidgetItem*> selected = _tree->selectedItems();
    for (int i = 0; i < selected.size(); ++i) {
        //find the items in the curves
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->second.treeItem == selected.at(i) ) {
                selection->push_back(it->second.curve);
            }
        }
    }
}

void
Parametric_KnobGui::resetSelectedCurves()
{
    QList<QTreeWidgetItem*> selected = _tree->selectedItems();
    for (int i = 0; i < selected.size(); ++i) {
        //find the items in the curves
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->second.treeItem == selected.at(i) ) {
                _knob.lock()->resetToDefaultValue(it->second.curve->getDimension());
                break;
            }
        }
    }
}

boost::shared_ptr<KnobI> Parametric_KnobGui::getKnob() const
{
    return _knob.lock();
}

