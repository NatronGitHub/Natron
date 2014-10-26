//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Gui/KnobGuiTypes.h"

#include <cfloat>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QLabel>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobTypes.h"
#include "Engine/Curve.h"
#include "Engine/TimeLine.h"
#include "Engine/Lut.h"
#include "Engine/Project.h"
#include "Engine/Image.h"
#include "Engine/Settings.h"
#include "Engine/Node.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Button.h"
#include "Gui/SpinBox.h"
#include "Gui/ComboBox.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/ProjectGui.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/ClickableLabel.h"

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

// convenience private classes


//==========================INT_KNOB_GUI======================================
Int_KnobGui::Int_KnobGui(boost::shared_ptr<KnobI> knob,
                         DockablePanel *container)
    : KnobGui(knob, container)
      , _slider(0)
{
    _knob = boost::dynamic_pointer_cast<Int_Knob>(knob);
    assert(_knob);
    boost::shared_ptr<KnobSignalSlotHandler> handler = _knob->getSignalSlotHandler();
    if (handler) {
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        QObject::connect( handler.get(), SIGNAL( minMaxChanged(double, double, int) ), this, SLOT( onMinMaxChanged(double, double, int) ) );
#endif
        
        QObject::connect( handler.get(), SIGNAL( displayMinMaxChanged(double, double, int) ), this, SLOT( onDisplayMinMaxChanged(double, double, int) ) );
    }
    QObject::connect( _knob.get(), SIGNAL( incrementChanged(int, int) ), this, SLOT( onIncrementChanged(int, int) ) );
}

Int_KnobGui::~Int_KnobGui()
{
    for (U32 i  = 0; i < _spinBoxes.size(); ++i) {
        delete _spinBoxes[i].first;
        delete _spinBoxes[i].second;
    }
    if (_slider) {
        delete _slider;
    }
}

void
Int_KnobGui::createWidget(QHBoxLayout* layout)
{
    int dim = _knob->getDimension();
    QWidget *container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(container);

    container->setLayout(containerLayout);
    containerLayout->setSpacing(3);
    containerLayout->setContentsMargins(0, 0, 0, 0);


    if (getKnobsCountOnSameLine() > 1) {
        _knob->disableSlider();
    }

    //  const std::vector<int> &maximums = intKnob->getMaximums();
    //    const std::vector<int> &minimums = intKnob->getMinimums();
    const std::vector<int> &increments = _knob->getIncrements();
    const std::vector<int> &displayMins = _knob->getDisplayMinimums();
    const std::vector<int> &displayMaxs = _knob->getDisplayMaximums();
    
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    const std::vector<int > &mins = _knob->getMinimums();
    const std::vector<int > &maxs = _knob->getMaximums();
#endif
    
    for (int i = 0; i < dim; ++i) {
        QWidget *boxContainer = new QWidget( layout->parentWidget() );
        QHBoxLayout *boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        boxContainerLayout->setSpacing(3);
        QLabel *subDesc = 0;
        if (dim != 1) {
            subDesc = new QLabel(QString( _knob->getDimensionName(i).c_str() ) + ':', boxContainer);
            subDesc->setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox *box = new SpinBox(layout->parentWidget(), SpinBox::INT_SPINBOX);
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
        if ( hasToolTip() ) {
            box->setToolTip( toolTip() );
        }
        boxContainerLayout->addWidget(box);
        if ( (getKnob()->getDimension() == 1) && !_knob->isSliderDisabled() && getKnobsCountOnSameLine() == 0 ) {
            int dispmin = displayMins[i];
            int dispmax = displayMaxs[i];
            
            _slider = new ScaleSliderQWidget( dispmin, dispmax,_knob->getValue(0,false), Natron::eScaleTypeLinear, layout->parentWidget() );
            if ( hasToolTip() ) {
                _slider->setToolTip( toolTip() );
            }
            QObject::connect( _slider, SIGNAL( positionChanged(double) ), this, SLOT( onSliderValueChanged(double) ) );
            QObject::connect( _slider, SIGNAL( editingFinished() ), this, SLOT( onSliderEditingFinished() ) );
            
            boxContainerLayout->addWidget(_slider);
            onDisplayMinMaxChanged(dispmin, dispmax);
        }

        containerLayout->addWidget(boxContainer);
        _spinBoxes.push_back( make_pair(box, subDesc) );
    }

    layout->addWidget(container);
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
Int_KnobGui::onDisplayMinMaxChanged(double mini,
                                    double maxi,
                                    int index)
{
    if (_slider) {
        double sliderMin = mini;
        double sliderMax = maxi;
        if ( (sliderMax - sliderMin) >= SLIDER_MAX_RANGE ) {
            
            ///use min max for slider if dispmin/dispmax was not set
            assert(index < (int)_knob->getMinimums().size() && index < (int)_knob->getMaximums().size());
            int max = _knob->getMaximums()[index];
            int min = _knob->getMinimums()[index];
            if ( (max - min) < SLIDER_MAX_RANGE ) {
                sliderMin = min;
                sliderMax = max;
            }
        }
        if ( (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < INT_MAX) && (sliderMin > INT_MIN) ) {
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
    int v = _knob->getValue(dimension,false);

    if (_slider) {
        _slider->seekScalePosition(v);
    }
    _spinBoxes[dimension].first->setValue(v);
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
    bool penUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    if (penUpOnly) {
        return;
    }

    assert(_knob->getDimension() == 1);
    _spinBoxes[0].first->setValue(d);
    pushUndoCommand( new KnobUndoCommand<int>(this,_knob->getValue(0,false),(int)d,0, false ));
}

void
Int_KnobGui::onSliderEditingFinished()
{
    bool penUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    if (!penUpOnly) {
        return;
    }
    double d = _slider->getPosition();
    assert(_knob->getDimension() == 1);
    _spinBoxes[0].first->setValue(d);
    pushUndoCommand( new KnobUndoCommand<int>(this,_knob->getValue(0,false),(int)d) );
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
    pushUndoCommand( new KnobUndoCommand<int>(this,_knob->getValueForEachDimension_mt_safe(),newValues,false) );
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
}

void
Int_KnobGui::_show()
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->show();
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->show();
        }
    }
    if (_slider) {
        double sliderMax = _slider->maximum();
        double sliderMin = _slider->minimum();
        
        if ( (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < INT_MAX) && (sliderMin > INT_MIN) ) {
            _slider->show();
        }
    }
}

void
Int_KnobGui::setEnabled()
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        bool b = getKnob()->isEnabled(i);
        //_spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->setEnabled(b);
        }
    }
    if (_slider) {
        _slider->setReadOnly( !getKnob()->isEnabled(0) );
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
    return _knob;
}

//==========================BOOL_KNOB_GUI======================================

Bool_KnobGui::Bool_KnobGui(boost::shared_ptr<KnobI> knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
{
    _knob = boost::dynamic_pointer_cast<Bool_Knob>(knob);
}

void
Bool_KnobGui::createWidget(QHBoxLayout* layout)
{
//    _descriptionLabel = new ClickableLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
//    if(hasToolTip()) {
//        _descriptionLabel->setToolTip(toolTip());
//    }
//    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
//
    _checkBox = new AnimatedCheckBox( layout->parentWidget() );
    if ( hasToolTip() ) {
        _checkBox->setToolTip( toolTip() );
    }
    QObject::connect( _checkBox, SIGNAL( toggled(bool) ), this, SLOT( onCheckBoxStateChanged(bool) ) );
    QObject::connect( this, SIGNAL( labelClicked(bool) ), this, SLOT( onCheckBoxStateChanged(bool) ) );

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_checkBox,0);

    layout->addWidget(_checkBox);
}

Bool_KnobGui::~Bool_KnobGui()
{
    delete _checkBox;
}

void
Bool_KnobGui::updateGUI(int /*dimension*/)
{
    _checkBox->blockSignals(true);
    _checkBox->setChecked( _knob->getValue(0,false) );
    _checkBox->blockSignals(false);
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
Bool_KnobGui::onCheckBoxStateChanged(bool b)
{
    pushUndoCommand( new KnobUndoCommand<bool>(this,_knob->getValue(0,false),b) );
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
    bool b = getKnob()->isEnabled(0);

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

boost::shared_ptr<KnobI> Bool_KnobGui::getKnob() const
{
    return _knob;
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

    if (dimension == 0) {
        Double_Knob::NormalizedState state = _knob->getNormalizedState(dimension);
        if (state == Double_Knob::NORMALIZATION_X) {
            Format f;
            getKnob()->getHolder()->getApp()->getProject()->getProjectDefaultFormat(&f);
            if (normalize) {
                *value /= f.width();
            } else {
                *value *= f.width();
            }
        } else if (state == Double_Knob::NORMALIZATION_Y) {
            Format f;
            getKnob()->getHolder()->getApp()->getProject()->getProjectDefaultFormat(&f);
            if (normalize) {
                *value /= f.height();
            } else {
                *value *= f.height();
            }
        }
    }
}

Double_KnobGui::Double_KnobGui(boost::shared_ptr<KnobI> knob,
                               DockablePanel *container)
    : KnobGui(knob, container), _slider(0), _digits(0.)
{
    _knob = boost::dynamic_pointer_cast<Double_Knob>(knob);
    assert(_knob);
    boost::shared_ptr<KnobSignalSlotHandler> handler = _knob->getSignalSlotHandler();
    if (handler) {
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        QObject::connect( handler.get(), SIGNAL( minMaxChanged(double, double, int) ), this, SLOT( onMinMaxChanged(double, double, int) ) );
#endif
        QObject::connect( handler.get(), SIGNAL( displayMinMaxChanged(double, double, int) ), this, SLOT( onDisplayMinMaxChanged(double, double, int) ) );
    }
    QObject::connect( _knob.get(), SIGNAL( incrementChanged(double, int) ), this, SLOT( onIncrementChanged(double, int) ) );
    QObject::connect( _knob.get(), SIGNAL( decimalsChanged(int, int) ), this, SLOT( onDecimalsChanged(int, int) ) );
}

Double_KnobGui::~Double_KnobGui()
{
    for (U32 i  = 0; i < _spinBoxes.size(); ++i) {
        delete _spinBoxes[i].first;
        delete _spinBoxes[i].second;
    }
    if (_slider) {
        delete _slider;
    }
}

void
Double_KnobGui::createWidget(QHBoxLayout* layout)
{
    QWidget *container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(container);

    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(3);


    if (getKnobsCountOnSameLine() > 1) {
        _knob->disableSlider();
    }

    int dim = getKnob()->getDimension();


    //  const std::vector<double> &maximums = dbl_knob->getMaximums();
    //    const std::vector<double> &minimums = dbl_knob->getMinimums();
    const std::vector<double> &increments = _knob->getIncrements();
    const std::vector<double> &displayMins = _knob->getDisplayMinimums();
    const std::vector<double> &displayMaxs = _knob->getDisplayMaximums();
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    const std::vector<double > & mins = _knob->getMinimums();
    const std::vector<double > & maxs = _knob->getMaximums();
#endif
    const std::vector<int> &decimals = _knob->getDecimals();
    for (int i = 0; i < dim; ++i) {
        QWidget *boxContainer = new QWidget( layout->parentWidget() );
        QHBoxLayout *boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        boxContainerLayout->setSpacing(3);
        QLabel *subDesc = 0;
        if (dim != 1) {
            subDesc = new QLabel(QString( getKnob()->getDimensionName(i).c_str() ) + ':', boxContainer);
            subDesc->setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox *box = new SpinBox(layout->parentWidget(), SpinBox::DOUBLE_SPINBOX);
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
        if ( hasToolTip() ) {
            box->setToolTip( toolTip() );
        }
        boxContainerLayout->addWidget(box);

        if ( (_knob->getDimension() == 1) && !_knob->isSliderDisabled()  && getKnobsCountOnSameLine() == 0) {
            double dispmin = displayMins[i];
            double dispmax = displayMaxs[i];
            valueAccordingToType(false, i, &dispmin);
            valueAccordingToType(false, i, &dispmax);
            
            
            _slider = new ScaleSliderQWidget( dispmin, dispmax,_knob->getValue(0,false), Natron::eScaleTypeLinear, layout->parentWidget() );
            if ( hasToolTip() ) {
                _slider->setToolTip( toolTip() );
            }
            QObject::connect( _slider, SIGNAL( positionChanged(double) ), this, SLOT( onSliderValueChanged(double) ) );
            QObject::connect( _slider, SIGNAL( editingFinished() ), this, SLOT( onSliderEditingFinished() ) );
            boxContainerLayout->addWidget(_slider);
            
            onDisplayMinMaxChanged(dispmin, dispmax);
        }

        containerLayout->addWidget(boxContainer);
        _spinBoxes.push_back( make_pair(box, subDesc) );
    }
    layout->addWidget(container);
} // createWidget

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
        
        double sliderMin = mini,sliderMax = maxi;
        
        if ( (maxi - mini) >= SLIDER_MAX_RANGE ) {
            ///use min max for slider if dispmin/dispmax was not set
            assert(index < (int)_knob->getMinimums().size() && index < (int)_knob->getMaximums().size());
            double max = _knob->getMaximums()[index];
            double min = _knob->getMinimums()[index];
            if ( (max - min) < SLIDER_MAX_RANGE ) {
                sliderMin = min;
                sliderMax = max;
            }
        }
        
        if ( (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < DBL_MAX) && (sliderMin > -DBL_MAX) ) {
            _digits = std::max(0., std::ceil(-std::log10(sliderMax - sliderMin) + 2.));
            _slider->show();
        } else {
            _slider->hide();
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
    double v = _knob->getValue(dimension,false);

    valueAccordingToType(false, dimension, &v);
    if (_slider) {
        _slider->seekScalePosition(v);
    }
    _spinBoxes[dimension].first->setValue(v);
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
    bool penUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    if (penUpOnly) {
        return;
    }
    assert(_knob->getDimension() == 1);
    QString str;
    str.setNum(d, 'f', _digits);
    d = str.toDouble();
    _spinBoxes[0].first->setValue(d);
    valueAccordingToType(true, 0, &d);
    pushUndoCommand( new KnobUndoCommand<double>(this,_knob->getValue(0,false),d,0,false) );
}

void
Double_KnobGui::onSliderEditingFinished()
{
    bool penUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    if (!penUpOnly) {
        return;
    }
    double d = _slider->getPosition();
    assert(_knob->getDimension() == 1);
    QString str;
    str.setNum(d, 'f', _digits);
    d = str.toDouble();
    _spinBoxes[0].first->setValue(d);
    valueAccordingToType(true, 0, &d);
    pushUndoCommand( new KnobUndoCommand<double>(this,_knob->getValue(0,false),d) );
}

void
Double_KnobGui::onSpinBoxValueChanged()
{
    std::list<double> newValues;

    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        double v = _spinBoxes[i].first->value();
        valueAccordingToType(true, 0, &v);
        newValues.push_back(v);
    }
    if (_slider) {
        _slider->seekScalePosition( newValues.front() );
    }
    pushUndoCommand( new KnobUndoCommand<double>(this,_knob->getValueForEachDimension_mt_safe(),newValues,false) );
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
}

void
Double_KnobGui::_show()
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->show();
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->show();
        }
    }
    if (_slider) {
        double sliderMax = _slider->maximum();
        double sliderMin = _slider->minimum();
        if ( (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < INT_MAX) && (sliderMin > INT_MIN) ) {
            _slider->show();
        }
    }
}

void
Double_KnobGui::setEnabled()
{
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        bool b = getKnob()->isEnabled(i);
        //_spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->setEnabled(b);
        }
    }
    if (_slider) {
        _slider->setReadOnly( !getKnob()->isEnabled(0) );
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

boost::shared_ptr<KnobI> Double_KnobGui::getKnob() const
{
    return _knob;
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
    QString label( _knob->getDescription().c_str() );
    const std::string & iconFilePath = _knob->getIconFilePath();
    QPixmap pix;

    if ( pix.load( iconFilePath.c_str() ) ) {
        _button = new Button( QIcon(pix),"",layout->parentWidget() );
        _button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    } else {
        _button = new Button( label,layout->parentWidget() );
    }
    QObject::connect( _button, SIGNAL( clicked() ), this, SLOT( emitValueChanged() ) );
    if ( hasToolTip() ) {
        _button->setToolTip( toolTip() );
    }
    layout->addWidget(_button);
}

Button_KnobGui::~Button_KnobGui()
{
    delete _button;
}

void
Button_KnobGui::emitValueChanged()
{
    dynamic_cast<Button_Knob*>( getKnob().get() )->onValueChanged(true, 0, Natron::eValueChangedReasonUserEdited, NULL);
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
    bool b = getKnob()->isEnabled(0);

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
    return _knob;
}

//=============================CHOICE_KNOB_GUI===================================
Choice_KnobGui::Choice_KnobGui(boost::shared_ptr<KnobI> knob,
                               DockablePanel *container)
    : KnobGui(knob, container)
{
    _knob = boost::dynamic_pointer_cast<Choice_Knob>(knob);
    _entries = _knob->getEntries_mt_safe();
    QObject::connect( _knob.get(), SIGNAL( populated() ), this, SLOT( onEntriesPopulated() ) );
}

Choice_KnobGui::~Choice_KnobGui()
{
    delete _comboBox;
}

void
Choice_KnobGui::createWidget(QHBoxLayout* layout)
{
    _comboBox = new ComboBox( layout->parentWidget() );
    onEntriesPopulated();

    QObject::connect( _comboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( onCurrentIndexChanged(int) ) );

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_comboBox,0);

    layout->addWidget(_comboBox);
}

void
Choice_KnobGui::onCurrentIndexChanged(int i)
{
    pushUndoCommand( new KnobUndoCommand<int>(this,_knob->getValue(0,false),i) );
}

void
Choice_KnobGui::onEntriesPopulated()
{
    int activeIndex = _comboBox->activeIndex();

    _comboBox->clear();
    _entries = _knob->getEntries_mt_safe();
    const std::vector<std::string> help =  _knob->getEntriesHelp_mt_safe();
    for (U32 i = 0; i < _entries.size(); ++i) {
        std::string helpStr;
        if ( !help.empty() && !help[i].empty() ) {
            helpStr = help[i];
        }
        _comboBox->addItem( _entries[i].c_str(), QIcon(), QKeySequence(), QString( helpStr.c_str() ) );
    }
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///we don't want that to happen because the index actually didn't change.
    _comboBox->setCurrentIndex_no_emit(activeIndex);

    QString tt = getScriptNameHtml();
    QString realTt( _knob->getHintToolTipFull().c_str() );
    if ( !realTt.isEmpty() ) {
        realTt = Qt::convertFromPlainText(realTt,Qt::WhiteSpaceNormal);
        tt.append(realTt);
    }
    _comboBox->setToolTip(tt);
}

void
Choice_KnobGui::updateGUI(int /*dimension*/)
{
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///change the internal value of the knob again...
    ///The slot connected to onCurrentIndexChanged is reserved to catch user interaction with the combobox.
    ///This function is called in response to an internal change.
    _comboBox->setCurrentIndex_no_emit( _knob->getValue(0,false) );
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
    bool b = getKnob()->isEnabled(0);

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
    return _knob;
}

//=============================TABLE_KNOB_GUI===================================

//ComboBoxDelegate::ComboBoxDelegate(Table_KnobGui* tableKnob,QObject *parent)
//: QStyledItemDelegate(parent)
//, _tableKnob(tableKnob)
//{
//
//}
//
//QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
//                              const QModelIndex &index) const{
//    ComboBox* cb =  new ComboBox(parent);
//    QStringList entries = index.model()->data(index,Qt::DisplayRole).toStringList();
//    for (int i = 0; i < entries.size(); ++i) {
//        cb->addItem(entries.at(i));
//    }
//    cb->setCurrentIndex(index.model()->data(index,Qt::EditRole).toInt());
//    QObject::connect(cb, SIGNAL(currentIndexChanged(int)), _tableKnob, SLOT(onCurrentIndexChanged(int)));
//
//    ///////From Qt's doc:
//    /*The returned editor widget should have Qt::StrongFocus; otherwise,
//     QMouseEvents received by the widget will propagate to the view. The view's background will shine through unless
//     the editor paints its own background (e.g., with setAutoFillBackground()).
//     */
//    cb->setFocusPolicy(Qt::StrongFocus);
//    cb->setAutoFillBackground(true);
//    return cb;
//}
//
//void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const{
//    int value = index.model()->data(index, Qt::EditRole).toInt();
//    QStringList entries = index.model()->data(index,Qt::DisplayRole).toStringList();
//    ComboBox *cb = dynamic_cast<ComboBox*>(editor);
//    cb->clear();
//    for (int i = 0; i < entries.size(); ++i) {
//        cb->addItem(entries.at(i));
//    }
//    if(value < entries.size()){
//        cb->setCurrentText(entries.at(value));
//    }
//}
//void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
//                   const QModelIndex &index) const {
//    ComboBox *cb = dynamic_cast<ComboBox*>(editor);
//    int i = cb->activeIndex();
//
//    model->setData(index, QVariant(i),Qt::EditRole);
//}
//
//void ComboBoxDelegate::updateEditorGeometry(QWidget *editor,
//                                  const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const {
//    editor->setGeometry(option.rect);
//}
//
//void ComboBoxDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
//
//    if(index.column() == 0){
//        QStyledItemDelegate::paint(painter, option, index);
//    }else{
//        int value = index.model()->data(index, Qt::EditRole).toInt();
//        QStringList entries = index.model()->data(index,Qt::DisplayRole).toStringList();
//        if(entries.size() > 0){
//            QString str = entries.at(value);
//            QStyle *style = QApplication::style();
//            QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
//            QRect r;
//            painter->drawText(geom,Qt::TextSingleLine|Qt::AlignRight,str,&r);
//        }
//    }
//}
//
//QSize ComboBoxDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const {
//    QStringList entries = index.model()->data(index,Qt::DisplayRole).toStringList();
//    QFontMetrics metric(option.font);
//    int largestStr = 0;
//    for (int i = 0; i < entries.size(); ++i) {
//        int w = metric.width(entries.at(i));
//        if (w > largestStr) {
//            w = largestStr;
//        }
//    }
//    return QSize(largestStr,metric.height());
//}

//
//Table_KnobGui::Table_KnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container)
//: KnobGui(knob,container)
////, _tableComboBoxDelegate(0)
////, _table(0)
//, _descriptionLabel(0)
//, _container(0)
//, _layout(0)
//{
//    boost::shared_ptr<Table_Knob> tbKnob = boost::dynamic_pointer_cast<Table_Knob>(getKnob());
//    assert(tbKnob);
//
//    QObject::connect(tbKnob.get(), SIGNAL(populated()), this, SLOT(onPopulated()));
//}
//
//Table_KnobGui::~Table_KnobGui() {
//    delete _sa;
//    //   delete _tableComboBoxDelegate;
//}
//
//void Table_KnobGui::createWidget(QGridLayout *layout, int row) {
//
//    boost::shared_ptr<Table_Knob> tbKnob = boost::dynamic_pointer_cast<Table_Knob>(getKnob());
//    assert(tbKnob);
//
//    //_sa = new QScrollArea(layout->parentWidget());
//    _container = new QWidget();
//    //_sa->setWidget(_container);
//    _container->setToolTip(tbKnob->getHintToolTip().c_str());
//    _layout = new QVBoxLayout(_container);
//
//    _descriptionLabel = new QLabel(getKnob()->getDescription().c_str(),_container);
//
//    _layout->addWidget(_descriptionLabel);
//
////    _table = new TableWidget(_container);
////    _table->setRowCount(tbKnob->getDimension());
////    _table->setColumnCount(2);
////    _table->setSelectionMode(QAbstractItemView::NoSelection);
////
////    _tableComboBoxDelegate = new ComboBoxDelegate(this);
////    _table->setItemDelegateForColumn(1, _tableComboBoxDelegate);
////    _table->setEditTriggers(QAbstractItemView::AllEditTriggers);
//
//    ////what we call vertical for us is the horizontal header for qt
////    _table->verticalHeader()->hide();
//
//    std::string keyHeader,choicesHeader;
//    tbKnob->getVerticalHeaders(&keyHeader, &choicesHeader);
//
////    QStringList verticalHeaders;
////    if(!keyHeader.empty() && !choicesHeader.empty()){
////        verticalHeaders.push_back(keyHeader.c_str());
////        verticalHeaders.push_back(choicesHeader.c_str());
////    }
////
////    if(!verticalHeaders.isEmpty()){
////        _table->setHorizontalHeaderLabels(verticalHeaders);
////    }
////
//
//    QWidget* header = new QWidget(_container);
//    QHBoxLayout* headerLayout = new QHBoxLayout(header);
//    headerLayout->setContentsMargins(0, 0, 0, 0);
//    QLabel* keyHeaderLabel = new QLabel(keyHeader.c_str(),header);
//    headerLayout->addWidget(keyHeaderLabel);
//    QLabel* valueHeaderLabel = new QLabel(choicesHeader.c_str(),header);
//    headerLayout->addWidget(valueHeaderLabel);
//
//    _layout->addWidget(header);
//
//    const Table_Knob::TableEntries& entries = tbKnob->getRows();
//    if((int)entries.size() == tbKnob->getDimension()){
//        onPopulated();
//    }
//
//    // _layout->addWidget(_table);
//
//    layout->addWidget(_sa, row, 1);
//}
//
//void Table_KnobGui::onPopulated(){
//
//    boost::shared_ptr<Table_Knob> tbKnob = boost::dynamic_pointer_cast<Table_Knob>(getKnob());
//    assert(tbKnob);
//
//    const Table_Knob::TableEntries& entries = tbKnob->getRows();
//    assert((int)entries.size() == tbKnob->getDimension());
//    int i = 0;
//    for (Table_Knob::TableEntries::const_iterator it = entries.begin();it!=entries.end();++it) {
//        QWidget* row = new QWidget(_container);
//        QHBoxLayout *rowLayout = new QHBoxLayout(row);
//        rowLayout->setContentsMargins(0, 0, 0, 0);
//
//        QLabel* key = new QLabel(it->first.c_str(),row);
//        rowLayout->addWidget(key);
//
////        QTableWidgetItem* keyItem = new QTableWidgetItem(it->first.c_str());
////        keyItem->setFlags(Qt::NoItemFlags);
////        _table->setItem(i, 0, keyItem);
////
////        QTableWidgetItem* choicesItem = new QTableWidgetItem();
//        ComboBox* cb = new ComboBox(row);
//        for (U32 j = 0; j < it->second.size(); ++j) {
//            cb->addItem(it->second[j].c_str());
//        }
//        _choices.push_back(cb);
//        QObject::connect(cb, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
//        int defaultIndex = tbKnob->getValue<int>(i);
//        cb->setCurrentIndex(defaultIndex);
//
//        rowLayout->addWidget(cb);
////        _table->setItem(i, 1, choicesItem);
////        _table->setCellWidget(i, 1, cb);
////
//        _layout->addWidget(row);
//        ++i;
//    }
//    //  _table->resizeColumnsToContents();
//}
//
//void Table_KnobGui::onCurrentIndexChanged(int){
//    std::vector<Variant> newValues;
//    for (U32 i = 0; i < _choices.size(); ++i) {
//        newValues.push_back(Variant(_choices[i]->activeIndex()));
//    }
//    pushValueChangedCommand(newValues);
//}
//
//void Table_KnobGui::_hide() {
//    _container->hide();
//}
//
//void Table_KnobGui::_show() {
//    _container->show();
//}
//
//void Table_KnobGui::setEnabled() {
//    _container->setEnabled(getKnob()->isEnabled());
//}
//
//void Table_KnobGui::updateGUI(int dimension) {
//    if(dimension < (int)_choices.size()){
//        _choices[dimension]->setCurrentText(_choices[dimension]->itemText(variant.toInt()));
//    }
//}


//=============================SEPARATOR_KNOB_GUI===================================

Separator_KnobGui::Separator_KnobGui(boost::shared_ptr<KnobI> knob,
                                     DockablePanel *container)
    : KnobGui(knob, container)
{
    _knob = boost::dynamic_pointer_cast<Separator_Knob>(knob);
}

void
Separator_KnobGui::createWidget(QHBoxLayout* layout)
{
    ///FIXME: this line is never visible.
    _line = new QFrame( layout->parentWidget() );
    _line->setFrameShape(QFrame::HLine);
    _line->setFrameShadow(QFrame::Sunken);
    _line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(_line);
}

Separator_KnobGui::~Separator_KnobGui()
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
    return _knob;
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
    _knob = boost::dynamic_pointer_cast<Color_Knob>(knob);
    assert(_knob);
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    boost::shared_ptr<KnobSignalSlotHandler> handler = _knob->getSignalSlotHandler();
    if (handler) {
        QObject::connect( handler.get(), SIGNAL( minMaxChanged(double, double, int) ), this, SLOT( onMinMaxChanged(double, double, int) ) );
        QObject::connect( handler.get(), SIGNAL( displayMinMaxChanged(double, double, int) ), this, SLOT( onDisplayMinMaxChanged(double, double, int) ) );
    }
#endif
    QObject::connect( this, SIGNAL( dimensionSwitchToggled(bool) ), _knob.get(), SLOT( onDimensionSwitchToggled(bool) ) );
    QObject::connect( _knob.get(), SIGNAL( mustActivateAllDimensions() ),this, SLOT( onMustShowAllDimension() ) );
    QObject::connect( _knob.get(), SIGNAL( pickingEnabled(bool) ),this, SLOT( setPickingEnabled(bool) ) );
}

Color_KnobGui::~Color_KnobGui()
{
    delete mainContainer;
}

void
Color_KnobGui::createWidget(QHBoxLayout* layout)
{
    mainContainer = new QWidget( layout->parentWidget() );
    mainLayout = new QHBoxLayout(mainContainer);
    mainContainer->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    boxContainers = new QWidget(mainContainer);
    boxLayout = new QHBoxLayout(boxContainers);
    boxContainers->setLayout(boxLayout);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setSpacing(1);

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    const std::vector<double> & minimums = _knob->getMinimums();
    const std::vector<double> & maximums = _knob->getMaximums();
#endif
    
    _rBox = new SpinBox(boxContainers, SpinBox::DOUBLE_SPINBOX);
    QObject::connect( _rBox, SIGNAL( valueChanged(double) ), this, SLOT( onColorChanged() ) );

    if (_dimension >= 3) {
        _gBox = new SpinBox(boxContainers, SpinBox::DOUBLE_SPINBOX);
        QObject::connect( _gBox, SIGNAL( valueChanged(double) ), this, SLOT( onColorChanged() ) );
        _bBox = new SpinBox(boxContainers, SpinBox::DOUBLE_SPINBOX);
        QObject::connect( _bBox, SIGNAL( valueChanged(double) ), this, SLOT( onColorChanged() ) );
    }
    if (_dimension >= 4) {
        _aBox = new SpinBox(boxContainers, SpinBox::DOUBLE_SPINBOX);
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
    _rLabel = new QLabel(QString( _knob->getDimensionName(0).c_str() ).toLower(), boxContainers);
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
        _gLabel = new QLabel(QString( _knob->getDimensionName(1).c_str() ).toLower(), boxContainers);
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
        _bLabel = new QLabel(QString( _knob->getDimensionName(2).c_str() ).toLower(), boxContainers);
        if ( hasToolTip() ) {
            _bLabel->setToolTip( toolTip() );
        }
        boxLayout->addWidget(_bLabel);
        boxLayout->addWidget(_bBox);
    }
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
        _aLabel = new QLabel(QString( _knob->getDimensionName(3).c_str() ).toLower(), boxContainers);
        if ( hasToolTip() ) {
            _aLabel->setToolTip( toolTip() );
        }
        boxLayout->addWidget(_aLabel);
        boxLayout->addWidget(_aBox);
    }

    const std::vector<double> & displayMinimums = _knob->getDisplayMinimums();
    const std::vector<double> & displayMaximums = _knob->getDisplayMaximums();
    double slidermin = *std::min_element( displayMinimums.begin(), displayMinimums.end() );
    double slidermax = *std::max_element( displayMaximums.begin(), displayMaximums.end() );
    if ( slidermin <= -std::numeric_limits<float>::max() ) {
        slidermin = 0.;
    }
    if ( slidermax >= std::numeric_limits<float>::max() ) {
        slidermax = 1.;
    }
    _slider = new ScaleSliderQWidget(slidermin, slidermax, _knob->getValue(0,false), Natron::eScaleTypeLinear, boxContainers);
    boxLayout->addWidget(_slider);
    QObject::connect( _slider, SIGNAL( positionChanged(double) ), this, SLOT( onSliderValueChanged(double) ) );
    _slider->hide();

    colorContainer = new QWidget(mainContainer);
    colorLayout = new QHBoxLayout(colorContainer);
    colorContainer->setLayout(colorLayout);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(0);

    _colorLabel = new ColorPickerLabel(colorContainer);
    QObject::connect( _colorLabel,SIGNAL( pickingEnabled(bool) ),this,SLOT( onPickingEnabled(bool) ) );
    colorLayout->addWidget(_colorLabel);

    QPixmap buttonPix;
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, &buttonPix);

    _colorDialogButton = new Button(QIcon(buttonPix), "", colorContainer);
    _colorDialogButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _colorDialogButton->setToolTip(Qt::convertFromPlainText(tr("Open the color dialog"), Qt::WhiteSpaceNormal));
    _colorDialogButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _colorDialogButton, SIGNAL( clicked() ), this, SLOT( showColorDialog() ) );
    colorLayout->addWidget(_colorDialogButton);

    _dimensionSwitchButton = new Button(QIcon(),QString::number(_dimension),colorContainer);
    _dimensionSwitchButton->setToolTip(Qt::convertFromPlainText(tr("Switch between a single value for all dimensions and multiple values"), Qt::WhiteSpaceNormal));
    _dimensionSwitchButton->setFocusPolicy(Qt::NoFocus);
    _dimensionSwitchButton->setCheckable(true);

    bool enableAllDimensions = false;
    double firstDimensionValue = _knob->getValue(0,false);
    if (_dimension > 1) {
        if (_knob->getValue(1,false) != firstDimensionValue) {
            enableAllDimensions = true;
        }
        if (_knob->getValue(2,false) != firstDimensionValue) {
            enableAllDimensions = true;
        }
    }
    if (_dimension > 3) {
        if (_knob->getValue(3,false) != firstDimensionValue) {
            enableAllDimensions = true;
        }
    }


    colorLayout->addWidget(_dimensionSwitchButton);

    mainLayout->addWidget(boxContainers);
    mainLayout->addWidget(colorContainer);

    layout->addWidget(mainContainer);

    _dimensionSwitchButton->setChecked(enableAllDimensions);
    if (!enableAllDimensions) {
        _rBox->setValue( _knob->getValue(0,false) );
    }
    onDimensionSwitchClicked();
    QObject::connect( _dimensionSwitchButton, SIGNAL( clicked() ), this, SLOT( onDimensionSwitchClicked() ) );
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
Color_KnobGui::expandAllDimensions()
{
    ///show all the dimensions
    _dimensionSwitchButton->setChecked(true);
    _dimensionSwitchButton->setDown(true);
    _slider->hide();
    if (_dimension > 1) {
        _rLabel->show();
        _gLabel->show();
        _gBox->show();
        _bLabel->show();
        _bBox->show();
        if (_dimension > 3) {
            _aLabel->show();
            _aBox->show();
        }
    }
    emit dimensionSwitchToggled(true);
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
        _gLabel->hide();
        _gBox->hide();
        _bLabel->hide();
        _bBox->hide();
        if (_dimension > 3) {
            _aLabel->hide();
            _aBox->hide();
        }
    }
    emit dimensionSwitchToggled(false);
}

void
Color_KnobGui::onDimensionSwitchClicked()
{
    if ( _dimensionSwitchButton->isChecked() ) {
        expandAllDimensions();
    } else {
        foldAllDimensions();
        if (_dimension > 1) {
            double value( _rBox->value() );
            if (_dimension == 3) {
                _knob->setValues(value, value, value);
            } else {
                _knob->setValues(value, value, value,value);
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
    bool r = _knob->isEnabled(0);

    //_rBox->setEnabled(r);
    _rBox->setReadOnly(!r);

    if ( _slider->isVisible() ) {
        _slider->setReadOnly(!r);
    }

    if (_dimension >= 3) {
        bool g = _knob->isEnabled(1);
        bool b = _knob->isEnabled(2);
        //_gBox->setEnabled(g);
        _gBox->setReadOnly(!g);
        //_bBox->setEnabled(b);
        _bBox->setReadOnly(!b);
    }
    if (_dimension >= 4) {
        bool a = _knob->isEnabled(3);
        //_aBox->setEnabled(a);
        _aBox->setReadOnly(!a);
    }
}

void
Color_KnobGui::updateGUI(int dimension)
{
    assert(dimension < _dimension && dimension >= 0 && dimension <= 3);
    double value = _knob->getValue(dimension,false);
    switch (dimension) {
    case 0: {
        _rBox->setValue(value);
        _slider->seekScalePosition(value);
        if ( !_knob->areAllDimensionsEnabled() ) {
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

    bool colorsEqual = true;
    if (_dimension == 3) {
        colorsEqual = (r == g && r == b);
    } else {
        colorsEqual = (r == g && r == b && r == a);
    }
    if (!_knob->areAllDimensionsEnabled() && !colorsEqual) {
        expandAllDimensions();
    } else if (_knob->areAllDimensionsEnabled() && colorsEqual) {
        foldAllDimensions();
    }
} // updateGUI

void
Color_KnobGui::reflectAnimationLevel(int dimension,
                                     Natron::AnimationLevelEnum level)
{
    switch (level) {
        case Natron::eAnimationLevelNone: {
            if (_rBox->getAnimation() == 0) {
                return;
            }
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
                    if (_rBox->getAnimation() == 1) {
                        return;
                    }
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
                    if (_rBox->getAnimation() == 2) {
                        return;
                    }
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
Color_KnobGui::showColorDialog()
{
    QColorDialog dialog( _rBox->parentWidget() );

    double curR = _rBox->value();
    double curG = curR;
    double curB = curR;
    double curA = 1.;
    if (_dimension > 1) {
        curG = _gBox->value();
        curB = _bBox->value();
    }
    if (_dimension > 3) {
        curA = _aBox->value();
    }

    for (int i = 0; i < _dimension; ++i) {
        _lastColor[i] = _knob->getValue(i,false);
    }

    QColor curColor;
    curColor.setRgbF(Natron::clamp(Natron::Color::to_func_srgb(curR)),
                     Natron::clamp(Natron::Color::to_func_srgb(curG)),
                     Natron::clamp(Natron::Color::to_func_srgb(curB)),
                     Natron::clamp(Natron::Color::to_func_srgb(curA)));
    dialog.setCurrentColor(curColor);
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onDialogCurrentColorChanged(QColor) ) );
    if (!dialog.exec()) {
        
        _knob->blockEvaluation();

        for (int i = 0; i < _dimension; ++i) {
            _knob->setValue(_lastColor[i],i);
        }
        _knob->unblockEvaluation();

    } else {
        
        _knob->blockEvaluation();

        ///refresh the last value so that the undo command retrieves the value that was prior to opening the dialog
        for (int i = 0; i < _dimension; ++i) {
            _knob->setValue(_lastColor[i],i);
        }

        ///if only the first dimension is displayed, switch back to all dimensions
        if ( !_dimensionSwitchButton->isChecked() ) {
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
            if (getKnob()->isEnabled(3)) {
                _aBox->setValue(userColor.alphaF()); // no conversion, alpha is linear
            }
            realColor.setAlpha(userColor.alpha());
        }

        onColorChanged();
        
        _knob->unblockEvaluation();

    }
    _knob->evaluateValueChange(0, eValueChangedReasonNatronGuiEdited);
} // showColorDialog

void
Color_KnobGui::onDialogCurrentColorChanged(const QColor & color)
{
    _knob->blockEvaluation();
    _knob->setValue(Natron::Color::from_func_srgb(color.redF()), 0);
    if (_dimension > 1) {
        _knob->setValue(Natron::Color::from_func_srgb(color.greenF()), 1);
        _knob->setValue(Natron::Color::from_func_srgb(color.blueF()), 2);
        if (_dimension > 3) {
            _knob->setValue(color.alphaF(), 3); // no conversion, alpha is linear
        }
    }
    _knob->unblockEvaluation();
    _knob->evaluateValueChange(0, eValueChangedReasonNatronGuiEdited);
}

void
Color_KnobGui::onColorChanged()
{
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
    }
    if (_dimension >= 3) {
        newValues.push_back(g);
        newValues.push_back(b);
    }
    if (_dimension >= 4) {
        newValues.push_back(a);
    }

    updateLabel(r, g, b, a);
    pushUndoCommand( new KnobUndoCommand<double>(this, _knob->getValueForEachDimension_mt_safe(), newValues,false) );
}

void
Color_KnobGui::updateLabel(double r, double g, double b, double a)
{
    QColor color;
    color.setRgbF(Natron::clamp(Natron::Color::to_func_srgb(r)),
                  Natron::clamp(Natron::Color::to_func_srgb(g)),
                  Natron::clamp(Natron::Color::to_func_srgb(b)),
                  Natron::clamp(a));
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
    _colorLabel->show();
    _colorDialogButton->show();
}

ColorPickerLabel::ColorPickerLabel(QWidget* parent)
    : QLabel(parent)
      , _pickingEnabled(false)
{
    setToolTip( Qt::convertFromPlainText(tr("To pick a color on a viewer, click this and then press control + left click on any viewer.\n"
                                            "You can also pick the average color of a given rectangle by holding control + shift + left click\n. "
                                            "To deselect the picker left click anywhere."
                                            "Note that by default %1 converts to linear the color picked\n"
                                            "because all the processing pipeline is linear, but you can turn this off in the\n"
                                            "preference panel.").arg(NATRON_APPLICATION_NAME), Qt::WhiteSpaceNormal) );
    setMouseTracking(true);
}

void
ColorPickerLabel::mousePressEvent(QMouseEvent*)
{
    _pickingEnabled = !_pickingEnabled;
    emit pickingEnabled(_pickingEnabled);
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
    _pickingEnabled = enabled;
    setColor(_currentColor);
}

void
ColorPickerLabel::setColor(const QColor & color)
{
    _currentColor = color;

    if (_pickingEnabled) {
        //draw the picker on top of the label
        QPixmap pickerIcon;
        appPTR->getIcon(Natron::NATRON_PIXMAP_COLOR_PICKER, &pickerIcon);
        QImage pickerImg = pickerIcon.toImage();
        QImage img(pickerIcon.width(), pickerIcon.height(), QImage::Format_ARGB32);
        img.fill( color.rgb() );


        for (int i = 0; i < pickerIcon.height(); ++i) {
            const QRgb* data = (QRgb*)pickerImg.scanLine(i);
            for (int j = 0; j < pickerImg.width(); ++j) {
                int alpha = qAlpha(data[j]);
                if (alpha > 0) {
                    img.setPixel(j, i, data[j]);
                }
            }
        }
        QPixmap pix = QPixmap::fromImage(img); //.scaled(20, 20);
        setPixmap(pix);
    } else {
        QImage img(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE, QImage::Format_ARGB32);
        img.fill( color.rgb() );
        QPixmap pix = QPixmap::fromImage(img);
        setPixmap(pix);
    }
}

void
Color_KnobGui::setPickingEnabled(bool enabled)
{
    _colorLabel->setPickingEnabled(enabled);
}

void
Color_KnobGui::onPickingEnabled(bool enabled)
{
    if ( getKnob()->getHolder()->getApp() ) {
        boost::shared_ptr<Color_Knob> colorKnob = boost::dynamic_pointer_cast<Color_Knob>( getKnob() );
        if (enabled) {
            getGui()->registerNewColorPicker(colorKnob);
        } else {
            getGui()->removeColorPicker(colorKnob);
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
    return _knob;
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
        emit editingFinished();
    }
    QTextEdit::focusOutEvent(e);
}

void
AnimatingTextEdit::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return) {
        if (_hasChanged) {
            _hasChanged = false;
            emit editingFinished();
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
      , _label(0)
{
    _knob = boost::dynamic_pointer_cast<String_Knob>(knob);
}

void
String_KnobGui::createWidget(QHBoxLayout* layout)
{
    if ( _knob->isMultiLine() ) {
        _container = new QWidget( layout->parentWidget() );
        _mainLayout = new QVBoxLayout(_container);
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->setSpacing(0);

        bool useRichText = _knob->usesRichText();
        _textEdit = new AnimatingTextEdit(_container);
        if ( hasToolTip() ) {
            QString tt = toolTip();
            if (useRichText) {
                tt += tr(" This text area supports html encoding. "
                         "Please check <a href=http://qt-project.org/doc/qt-5/richtext-html-subset.html>Qt website</a> for more info. ");
            }
            _textEdit->setAcceptRichText(useRichText);
            _textEdit->setToolTip(tt);
        }

        _mainLayout->addWidget(_textEdit);

        QObject::connect( _textEdit, SIGNAL( textChanged() ), this, SLOT( onTextChanged() ) );
        layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        _textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_textEdit,0);

        if (useRichText) {
            _richTextOptions = new QWidget(_container);
            _richTextOptionsLayout = new QHBoxLayout(_richTextOptions);
            _richTextOptionsLayout->setContentsMargins(0, 0, 0, 0);
            _richTextOptionsLayout->setSpacing(8);

            _fontCombo = new QFontComboBox(_richTextOptions);
            QFont font("Verdana",NATRON_FONT_SIZE_12);
            _fontCombo->setCurrentFont(font);
            _fontCombo->setToolTip( tr("Font") );
            _richTextOptionsLayout->addWidget(_fontCombo);

            _fontSizeSpinBox = new SpinBox(_richTextOptions);
            _fontSizeSpinBox->setMinimum(1);
            _fontSizeSpinBox->setMaximum(100);
            _fontSizeSpinBox->setValue(6);
            QObject::connect( _fontSizeSpinBox,SIGNAL( valueChanged(double) ),this,SLOT( onFontSizeChanged(double) ) );
            _fontSizeSpinBox->setToolTip( tr("Font size") );
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
            _setBoldButton->setToolTip( tr("Bold") );
            _setBoldButton->setMaximumSize(18, 18);
            QObject::connect( _setBoldButton,SIGNAL( clicked(bool) ),this,SLOT( boldChanged(bool) ) );
            _richTextOptionsLayout->addWidget(_setBoldButton);

            QIcon italicIcon;
            italicIcon.addPixmap(pixItalicChecked,QIcon::Normal,QIcon::On);
            italicIcon.addPixmap(pixItalicUnchecked,QIcon::Normal,QIcon::Off);

            _setItalicButton = new Button(italicIcon,"",_richTextOptions);
            _setItalicButton->setCheckable(true);
            _setItalicButton->setToolTip( tr("Italic") );
            _setItalicButton->setMaximumSize(18,18);
            QObject::connect( _setItalicButton,SIGNAL( clicked(bool) ),this,SLOT( italicChanged(bool) ) );
            _richTextOptionsLayout->addWidget(_setItalicButton);

            QPixmap pixBlack(15,15);
            pixBlack.fill(Qt::black);
            _fontColorButton = new Button(QIcon(pixBlack),"",_richTextOptions);
            _fontColorButton->setCheckable(false);
            _fontColorButton->setToolTip( tr("Font color") );
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
    } else if ( _knob->isLabel() ) {
        _label = new QLabel( layout->parentWidget() );
        if ( hasToolTip() ) {
            _label->setToolTip( toolTip() );
        }
        _label->setFont(QFont(NATRON_FONT,NATRON_FONT_SIZE_11));
        layout->addWidget(_label);
    } else {
        _lineEdit = new LineEdit( layout->parentWidget() );
        if ( hasToolTip() ) {
            _lineEdit->setToolTip( toolTip() );
        }
        layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(_lineEdit);
        QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onLineChanged() ) );

        if ( _knob->isCustomKnob() ) {
            _lineEdit->setReadOnly(true);
        }

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_lineEdit,0);
    }
} // createWidget

String_KnobGui::~String_KnobGui()
{
    delete _lineEdit;
    delete _label;
    delete _container;
}

void
String_KnobGui::onLineChanged()
{
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(0,false),_lineEdit->text().toStdString() ) );
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
    if ( _knob->usesRichText() ) {
        txt = addHtmlTags(txt);
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(0,false),txt.toStdString() ) );
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
    QString knobOldtext( _knob->getValue(0,false).c_str() );
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
    QString text( _knob->getValue(0,false).c_str() );

    if ( text.isEmpty() ) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>( _knob->getHolder() );
        /// If the node has a sublabel, restore it in the label
        if ( effect && (_knob->getName() == kUserLabelKnobName) ) {
            boost::shared_ptr<KnobI> knob = effect->getKnobByName(kOfxParamStringSublabelName);
            if (knob) {
                String_Knob* strKnob = dynamic_cast<String_Knob*>( knob.get() );
                if (strKnob) {
                    QString sublabel = strKnob->getValue(0,false).c_str();
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


        _knob->setValue(text.toStdString(), 0);
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
            _knob->setValue(text.toStdString(), 0);
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
                          QFont & f,
                          QColor& color)
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

    f.setPointSize( sizeStr.toInt() );
    f.setFamily(faceStr);
    
    {
        toFind = QString(kBoldStartTag);
        int foundBold = label.indexOf(toFind);
        if (foundBold != -1) {
            f.setBold(true);
        }
    }
    
    {
        toFind = QString(kItalicStartTag);
        int foundItalic = label.indexOf(toFind);
        if (foundItalic != -1) {
            f.setItalic(true);
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
            color = QColor(currentColor);
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
    assert(_textEdit);
    QString text( _knob->getValue(0,false).c_str() );
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
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(0,false),text.toStdString() ) );
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
    QString text( _knob->getValue(0,false).c_str() );
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
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(0,false),text.toStdString() ) );
}

void
String_KnobGui::boldChanged(bool toggled)
{
    assert(_textEdit);
    QString text( _knob->getValue(0,false).c_str() );
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
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(0,false),text.toStdString() ) );
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

    QColorDialog dialog(_textEdit);
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( updateFontColorIcon(QColor) ) );
    dialog.setCurrentColor(_fontColor);
    if ( dialog.exec() ) {
        _fontColor = dialog.currentColor();

        QString text( _knob->getValue(0,false).c_str() );
        findReplaceColorName(text,_fontColor.name());
        pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(0,false),text.toStdString() ) );
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
    QString text( _knob->getValue(0,false).c_str() );
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
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(0,false),text.toStdString() ) );
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
String_KnobGui::removeAutoAddedHtmlTags(QString text) const
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
    return removeNatronHtmlTag(text);
} // removeAutoAddedHtmlTags

void
String_KnobGui::updateGUI(int /*dimension*/)
{
    std::string value = _knob->getValue(0,false);

    if ( _knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->blockSignals(true);
        QTextCursor cursor = _textEdit->textCursor();
        int pos = cursor.position();
        int selectionStart = cursor.selectionStart();
        int selectionEnd = cursor.selectionEnd();
        QString txt( value.c_str() );
        txt = removeAutoAddedHtmlTags(txt);

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
    } else if ( _knob->isLabel() ) {
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
    if ( _knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->hide();
    } else if ( _knob->isLabel() ) {
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
    if ( _knob->isMultiLine() ) {
        assert(_textEdit);
        _textEdit->show();
    } else if ( _knob->isLabel() ) {
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
    bool b = _knob->isEnabled(0);

    if ( _knob->isMultiLine() ) {
        assert(_textEdit);
        //_textEdit->setEnabled(b);
        //_textEdit->setReadOnly(!b);
        _textEdit->setReadOnlyNatron(!b);
    } else if ( _knob->isLabel() ) {
        assert(_label);
        _label->setEnabled(b);
    } else {
        assert(_lineEdit);
        //_lineEdit->setEnabled(b);
        if ( !_knob->isCustomKnob() ) {
            _lineEdit->setReadOnly(!b);
        }
    }
}

void
String_KnobGui::reflectAnimationLevel(int /*dimension*/,
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
    
    if ( _knob->isMultiLine() ) {
        assert(_textEdit);
        if (_textEdit->getAnimation() != value) {
            _textEdit->setAnimation(value);
        }
    } else if ( _knob->isLabel() ) {
        assert(_label);
    } else {
        assert(_lineEdit);
        if (_lineEdit->getAnimation() != value) {
            _lineEdit->setAnimation(value);
        }
    }
}

void
String_KnobGui::setReadOnly(bool readOnly,
                            int /*dimension*/)
{
    if (_textEdit) {
        _textEdit->setReadOnlyNatron(readOnly);
    } else if (_lineEdit) {
        if ( !_knob->isCustomKnob() ) {
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

boost::shared_ptr<KnobI> String_KnobGui::getKnob() const
{
    return _knob;
}

//=============================GROUP_KNOB_GUI===================================
GroupBoxLabel::GroupBoxLabel(QWidget *parent)
    : QLabel(parent),
      _checked(false)
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
{
    _knob = boost::dynamic_pointer_cast<Group_Knob>(knob);
}

Group_KnobGui::~Group_KnobGui()
{
    delete _button;
    //    for(U32 i  = 0 ; i < _children.size(); ++i){
    //        delete _children[i].first;
    //    }
}

void
Group_KnobGui::addKnob(KnobGui *child,
                       int row,
                       int column)
{
    _children.push_back( std::make_pair( child, std::make_pair(row, column) ) );
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
    _button->setChecked(_checked);
    QWidget *header = new QWidget( layout->parentWidget() );
    QHBoxLayout *headerLay = new QHBoxLayout(header);
    header->setLayout(headerLay);
    QObject::connect( _button, SIGNAL( checked(bool) ), this, SLOT( setChecked(bool) ) );
    headerLay->addWidget(_button);
    headerLay->setSpacing(1);

    layout->addWidget(header);
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
    for (U32 i = 0; i < _children.size(); ++i) {
        if (!b) {
            _children[i].first->hide();
        } else if ( !_children[i].first->getKnob()->getIsSecret() ) {
            _children[i].first->show(startChildIndex);
            if ( !_children[i].first->getKnob()->isNewLineTurnedOff() ) {
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
    bool b = _knob->getValue(0,false);

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
    for (U32 i = 0; i < _children.size(); ++i) {
        _children[i].first->hide();
    }
}

void
Group_KnobGui::_show()
{
    if ( _knob->getIsSecret() ) {
        return;
    }
    if (_button) {
        _button->show();
    }

    if (_checked) {
        for (U32 i = 0; i < _children.size(); ++i) {
            _children[i].first->show();
        }
    }
}

void
Group_KnobGui::setEnabled()
{
    bool enabled = _knob->isEnabled(0);

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
        for (U32 i = 0; i < _children.size(); ++i) {
            std::vector<int> dimensions;
            for (int j = 0; j < _children[i].first->getKnob()->getDimension(); ++j) {
                if ( _children[i].first->getKnob()->isEnabled(j) ) {
                    _children[i].first->getKnob()->setEnabled(j, false);
                    dimensions.push_back(j);
                }
            }
            _childrenToEnable.push_back( std::make_pair(_children[i].first, dimensions) );
        }
    }
}

boost::shared_ptr<KnobI> Group_KnobGui::getKnob() const
{
    return _knob;
}

//=============================Parametric_KnobGui===================================

Parametric_KnobGui::Parametric_KnobGui(boost::shared_ptr<KnobI> knob,
                                       DockablePanel *container)
    : KnobGui(knob, container)
      , _curveWidget(NULL)
      , _tree(NULL)
      , _resetButton(NULL)
      , _curves()
{
    _knob = boost::dynamic_pointer_cast<Parametric_Knob>(knob);
}

Parametric_KnobGui::~Parametric_KnobGui()
{
    delete _curveWidget;
    delete _tree;
}

void
Parametric_KnobGui::createWidget(QHBoxLayout* layout)
{
    QObject::connect( _knob.get(), SIGNAL( curveChanged(int) ), this, SLOT( onCurveChanged(int) ) );

    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QWidget* treeColumn = new QWidget( layout->parentWidget() );
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
    _resetButton->setToolTip( Qt::convertFromPlainText(tr("Reset the selected curves in the tree to their default shape"), Qt::WhiteSpaceNormal) );
    QObject::connect( _resetButton, SIGNAL( clicked() ), this, SLOT( resetSelectedCurves() ) );
    treeColumnLayout->addWidget(_resetButton);

    layout->addWidget(treeColumn);

    _curveWidget = new CurveWidget( getGui(),boost::shared_ptr<TimeLine>(),layout->parentWidget() );
    _curveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if ( hasToolTip() ) {
        _curveWidget->setToolTip( toolTip() );
    }
    layout->addWidget(_curveWidget);


    std::vector<CurveGui*> visibleCurves;
    for (int i = 0; i < _knob->getDimension(); ++i) {
        QString curveName = _knob->getDimensionName(i).c_str();
        CurveGui* curve =  _curveWidget->createCurve(_knob->getParametricCurve(i),
                                                     this,
                                                     i,
                                                     curveName);
        QColor color;
        double r,g,b;
        _knob->getCurveColor(i, &r, &g, &b);
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
    bool b = getKnob()->isEnabled(0);

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
    std::vector<CurveGui*> curves;

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
Parametric_KnobGui::resetSelectedCurves()
{
    QVector<int> curveIndexes;
    QList<QTreeWidgetItem*> selected = _tree->selectedItems();
    for (int i = 0; i < selected.size(); ++i) {
        //find the items in the curves
        for (CurveGuis::iterator it = _curves.begin(); it != _curves.end(); ++it) {
            if ( it->second.treeItem == selected.at(i) ) {
                curveIndexes.push_back( it->second.curve->getDimension() );
                break;
            }
        }
    }
    _knob->resetToDefault(curveIndexes);
}

boost::shared_ptr<KnobI> Parametric_KnobGui::getKnob() const
{
    return _knob;
}

