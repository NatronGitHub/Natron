//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Gui/KnobGuiTypes.h"

#include <cfloat>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QTextEdit>
#include <QLabel>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>

#include "Global/AppManager.h"

#include "Engine/KnobTypes.h"
#include "Engine/Curve.h"
#include "Engine/TimeLine.h"

#include "Gui/AnimatedCheckBox.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/SpinBox.h"
#include "Gui/ComboBox.h"
#include "Gui/ScaleSlider.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/ProjectGui.h"
#include "Gui/CurveWidget.h"

#define SLIDER_MAX_RANGE 100000

using namespace Natron;
using std::make_pair;

// convenience private classes


//==========================INT_KNOB_GUI======================================
Int_KnobGui::Int_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
: KnobGui(knob, container)
, _slider(0)
{
    boost::shared_ptr<Int_Knob> intKnob = boost::dynamic_pointer_cast<Int_Knob>(getKnob());
    assert(intKnob);
    QObject::connect(intKnob.get(), SIGNAL(minMaxChanged(int, int, int)), this, SLOT(onMinMaxChanged(int, int, int)));
    QObject::connect(intKnob.get(), SIGNAL(incrementChanged(int, int)), this, SLOT(onIncrementChanged(int, int)));
}

Int_KnobGui::~Int_KnobGui()
{
    delete _descriptionLabel;
    for (U32 i  = 0 ; i < _spinBoxes.size(); ++i) {
        delete _spinBoxes[i].first;
        delete _spinBoxes[i].second;
    }
    if (_slider) {
        delete _slider;
    }
    
}

void Int_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    int dim = getKnob()->getDimension();
    
    QWidget *container = new QWidget(layout->parentWidget());
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    boost::shared_ptr<Int_Knob> intKnob = boost::dynamic_pointer_cast<Int_Knob>(getKnob());
    assert(intKnob);
    
    
    const std::vector<int> &maximums = intKnob->getMaximums();
    const std::vector<int> &minimums = intKnob->getMinimums();
    const std::vector<int> &increments = intKnob->getIncrements();
    const std::vector<int> &displayMins = intKnob->getDisplayMinimums();
    const std::vector<int> &displayMaxs = intKnob->getDisplayMaximums();
    for (int i = 0; i < dim; ++i) {
        QWidget *boxContainer = new QWidget(layout->parentWidget());
        QHBoxLayout *boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *subDesc = 0;
        if (dim != 1) {
            subDesc = new QLabel(getKnob()->getDimensionName(i).c_str(), boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox *box = new SpinBox(layout->parentWidget(), SpinBox::INT_SPINBOX);
        QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()));
        if (maximums.size() > (U32)i) {
            box->setMaximum(maximums[i]);
        }
        if (minimums.size() > (U32)i) {
            box->setMinimum(minimums[i]);
        }
        if (increments.size() > (U32)i) {
            box->setIncrement(increments[i]);
        }
        box->setToolTip(getKnob()->getHintToolTip().c_str());
        boxContainerLayout->addWidget(box);
        if (getKnob()->getDimension() == 1 && !intKnob->isSliderDisabled()) {
            int min = 0, max = 99;
            if (displayMins.size() > (U32)i && displayMins[i] != INT_MIN) {
                min = displayMins[i];
            } else if (minimums.size() > (U32)i) {
                min = minimums[i];
            }
            if (displayMaxs.size() > (U32)i && displayMaxs[i] != INT_MAX) {
                max = displayMaxs[i];
            } else if (maximums.size() > (U32)i) {
                max = maximums[i];
            }
            if ((max - min) < SLIDER_MAX_RANGE && max < INT_MAX && min > INT_MIN) {
                _slider = new ScaleSlider(min, max,
                                          getKnob()->getValue<int>(), Natron::LINEAR_SCALE, layout->parentWidget());
                _slider->setToolTip(getKnob()->getHintToolTip().c_str());
                QObject::connect(_slider, SIGNAL(positionChanged(double)), this, SLOT(onSliderValueChanged(double)));
                boxContainerLayout->addWidget(_slider);
            }
            
        }
        
        containerLayout->addWidget(boxContainer);
        _spinBoxes.push_back(make_pair(box, subDesc));
    }
    
    layout->addWidget(container, row, 1, Qt::AlignLeft);
}

void Int_KnobGui::onMinMaxChanged(int mini, int maxi, int index)
{
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
}

void Int_KnobGui::onIncrementChanged(int incr, int index)
{
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setIncrement(incr);
}

void Int_KnobGui::updateGUI(int dimension, const Variant &variant)
{
    int v = variant.toInt();
    if (_slider) {
        _slider->seekScalePosition(v);
    }
    _spinBoxes[dimension].first->setValue(v);
}

void Int_KnobGui::reflectAnimationLevel(int dimension,Natron::AnimationLevel level) {
    switch (level) {
        case Natron::NO_ANIMATION:
            _spinBoxes[dimension].first->setAnimation(0);
            break;
        case Natron::INTERPOLATED_VALUE:
            _spinBoxes[dimension].first->setAnimation(1);
            break;
        case Natron::ON_KEYFRAME:
            _spinBoxes[dimension].first->setAnimation(2);
            break;
        default:
            break;
    }
}

void Int_KnobGui::onSliderValueChanged(double d)
{
    assert(getKnob()->getDimension() == 1);
    _spinBoxes[0].first->setValue(d);
    pushUndoCommand(new KnobUndoCommand(this, 0, getKnob()->getValue(0), Variant(d)));
}

void Int_KnobGui::onSpinBoxValueChanged()
{
    std::vector<Variant> newValues;
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        newValues.push_back(Variant(_spinBoxes[i].first->value()));
    }
    
    pushUndoCommand(new KnobMultipleUndosCommand(this, getKnob()->getValueForEachDimension(), newValues));
}

void Int_KnobGui::_hide()
{
    _descriptionLabel->hide();
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

void Int_KnobGui::_show()
{
    _descriptionLabel->show();
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->show();
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->show();
        }
    }
    if (_slider) {
        _slider->show();
    }
}

void Int_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _descriptionLabel->setEnabled(b);
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->setEnabled(b);
        }
    }
    if (_slider) {
        _slider->setEnabled(b);
    }
    
}

//==========================BOOL_KNOB_GUI======================================

void Bool_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new ClickableLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    _checkBox = new AnimatedCheckBox(layout->parentWidget());
    _checkBox->setToolTip(getKnob()->getHintToolTip().c_str());
    QObject::connect(_checkBox, SIGNAL(clicked(bool)), this, SLOT(onCheckBoxStateChanged(bool)));
    QObject::connect(_descriptionLabel, SIGNAL(clicked(bool)), this, SLOT(onCheckBoxStateChanged(bool)));
    layout->addWidget(_checkBox, row, 1, Qt::AlignLeft);
}

Bool_KnobGui::~Bool_KnobGui()
{
    delete _descriptionLabel;
    delete _checkBox;
}
void Bool_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    bool b = variant.toBool();
    _checkBox->setChecked(b);
    _descriptionLabel->setClicked(b);
}

void Bool_KnobGui::reflectAnimationLevel(int /*dimension*/,Natron::AnimationLevel level) {
    switch (level) {
        case Natron::NO_ANIMATION:
            _checkBox->setAnimation(0);
            break;
        case Natron::INTERPOLATED_VALUE:
            _checkBox->setAnimation(1);
            break;
        case Natron::ON_KEYFRAME:
            _checkBox->setAnimation(2);
            break;
        default:
            break;
    }
}


void Bool_KnobGui::onCheckBoxStateChanged(bool b)
{
    std::map<int, Variant> newValues;
    pushUndoCommand(new KnobUndoCommand(this, 0, getKnob()->getValue(0), Variant(b)));
}
void Bool_KnobGui::_hide()
{
    _descriptionLabel->hide();
    _checkBox->hide();
}

void Bool_KnobGui::_show()
{
    _descriptionLabel->show();
    _checkBox->show();
}

void Bool_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _descriptionLabel->setEnabled(b);
    _checkBox->setEnabled(b);
}

void AnimatedCheckBox::setAnimation(int i)
{
    animation = i;
    style()->unpolish(this);
    style()->polish(this);
}
//=============================DOUBLE_KNOB_GUI===================================
Double_KnobGui::Double_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container), _slider(0)
{
    boost::shared_ptr<Double_Knob> dbl_knob = boost::dynamic_pointer_cast<Double_Knob>(getKnob());
    assert(dbl_knob);
    QObject::connect(dbl_knob.get(), SIGNAL(minMaxChanged(double, double, int)), this, SLOT(onMinMaxChanged(double, double, int)));
    QObject::connect(dbl_knob.get(), SIGNAL(incrementChanged(double, int)), this, SLOT(onIncrementChanged(double, int)));
    QObject::connect(dbl_knob.get(), SIGNAL(decimalsChanged(int, int)), this, SLOT(onDecimalsChanged(int, int)));
}

Double_KnobGui::~Double_KnobGui()
{
    delete _descriptionLabel;
    for (U32 i  = 0 ; i < _spinBoxes.size(); ++i) {
        delete _spinBoxes[i].first;
        delete _spinBoxes[i].second;
    }
    if (_slider) {
        delete _slider;
    }
}

void Double_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    QWidget *container = new QWidget(layout->parentWidget());
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    boost::shared_ptr<Double_Knob> dbl_knob = boost::dynamic_pointer_cast<Double_Knob>(getKnob());
    assert(dbl_knob);
    int dim = getKnob()->getDimension();
    
    
    const std::vector<double> &maximums = dbl_knob->getMaximums();
    const std::vector<double> &minimums = dbl_knob->getMinimums();
    const std::vector<double> &increments = dbl_knob->getIncrements();
    const std::vector<double> &displayMins = dbl_knob->getDisplayMinimums();
    const std::vector<double> &displayMaxs = dbl_knob->getDisplayMaximums();
    
    const std::vector<int> &decimals = dbl_knob->getDecimals();
    for (int i = 0; i < dim; ++i) {
        QWidget *boxContainer = new QWidget(layout->parentWidget());
        QHBoxLayout *boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *subDesc = 0;
        if (dim != 1) {
            subDesc = new QLabel(getKnob()->getDimensionName(i).c_str(), boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox *box = new SpinBox(layout->parentWidget(), SpinBox::DOUBLE_SPINBOX);
        QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()));
        if ((int)maximums.size() > i) {
            box->setMaximum(maximums[i]);
        }
        if ((int)minimums.size() > i) {
            box->setMinimum(minimums[i]);
        }
        if ((int)decimals.size() > i) {
            box->decimals(decimals[i]);
        }
        if ((int)increments.size() > i) {
            double incr = increments[i];
            if (incr > 0) {
                box->setIncrement(incr);
            }
        }
        box->setToolTip(getKnob()->getHintToolTip().c_str());
        boxContainerLayout->addWidget(box);
        if (getKnob()->getDimension() == 1 && !dbl_knob->isSliderDisabled()) {
            double min = 0., max = 99.;
            if (displayMins.size() > (U32)i) {
                min = displayMins[i];
            } else if (minimums.size() > (U32)i) {
                min = minimums[i];
            }
            if (displayMaxs.size() > (U32)i) {
                max = displayMaxs[i];
            } else if (maximums.size() > (U32)i) {
                max = maximums[i];
            }
            if ((max - min) < SLIDER_MAX_RANGE && max < DBL_MAX && min > -DBL_MAX) {
                _slider = new ScaleSlider(min, max,
                                          getKnob()->getValue<double>(), Natron::LINEAR_SCALE, layout->parentWidget());
                _slider->setToolTip(getKnob()->getHintToolTip().c_str());
                QObject::connect(_slider, SIGNAL(positionChanged(double)), this, SLOT(onSliderValueChanged(double)));
                boxContainerLayout->addWidget(_slider);
            }
        }
        
        containerLayout->addWidget(boxContainer);
        //        layout->addWidget(boxContainer,row,i+1+columnOffset);
        _spinBoxes.push_back(make_pair(box, subDesc));
    }
    layout->addWidget(container, row, 1, Qt::AlignLeft);
}
void Double_KnobGui::onMinMaxChanged(double mini, double maxi, int index)
{
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
}
void Double_KnobGui::onIncrementChanged(double incr, int index)
{
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->setIncrement(incr);
}
void Double_KnobGui::onDecimalsChanged(int deci, int index)
{
    assert(_spinBoxes.size() > (U32)index);
    _spinBoxes[index].first->decimals(deci);
}

void Double_KnobGui::updateGUI(int dimension, const Variant &variant)
{
    
    double v = variant.toDouble();
    if (_slider) {
        _slider->seekScalePosition(v);
    }
    _spinBoxes[dimension].first->setValue(v);
}

void Double_KnobGui::reflectAnimationLevel(int dimension,Natron::AnimationLevel level) {
    switch (level) {
        case Natron::NO_ANIMATION:
            _spinBoxes[dimension].first->setAnimation(0);
            break;
        case Natron::INTERPOLATED_VALUE:
            _spinBoxes[dimension].first->setAnimation(1);
            break;
        case Natron::ON_KEYFRAME:
            _spinBoxes[dimension].first->setAnimation(2);
            break;
        default:
            break;
    }
}

void Double_KnobGui::onSliderValueChanged(double d)
{
    assert(getKnob()->getDimension() == 1);
    _spinBoxes[0].first->setValue(d);
    pushUndoCommand(new KnobUndoCommand(this, 0, getKnob()->getValue(0), Variant(d)));
}
void Double_KnobGui::onSpinBoxValueChanged()
{
    std::vector<Variant> newValues;
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        newValues.push_back(Variant(_spinBoxes[i].first->value()));
    }
    pushUndoCommand(new KnobMultipleUndosCommand(this, getKnob()->getValueForEachDimension(), newValues));
}
void Double_KnobGui::_hide()
{
    _descriptionLabel->hide();
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

void Double_KnobGui::_show()
{
    _descriptionLabel->show();
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->show();
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->show();
        }
    }
    if (_slider) {
        _slider->show();
    }
    
}
void Double_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _descriptionLabel->setEnabled(b);
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        _spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->setEnabled(b);
        }
    }
    if (_slider) {
        _slider->setEnabled(b);
    }
    
}



//=============================BUTTON_KNOB_GUI===================================
void Button_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _button = new Button(QString(QString(getKnob()->getDescription().c_str())), layout->parentWidget());
    QObject::connect(_button, SIGNAL(pressed()), this, SLOT(emitValueChanged()));
    _button->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_button, row, 0, Qt::AlignRight);
}

Button_KnobGui::~Button_KnobGui()
{
    delete _button;
}

void Button_KnobGui::emitValueChanged()
{
    getKnob()->onValueChanged(0, Variant(), NULL);
}
void Button_KnobGui::_hide()
{
    _button->hide();
}

void Button_KnobGui::_show()
{
    _button->show();
}
void Button_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _button->setEnabled(b);
}


//=============================CHOICE_KNOB_GUI===================================
Choice_KnobGui::Choice_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container)
{
    boost::shared_ptr<Choice_Knob> cbKnob = boost::dynamic_pointer_cast<Choice_Knob>(knob);
    _entries = cbKnob->getEntries();
    QObject::connect(cbKnob.get(), SIGNAL(populated()), this, SLOT(onEntriesPopulated()));
}
Choice_KnobGui::~Choice_KnobGui()
{
    delete _comboBox;
    delete _descriptionLabel;
}
void Choice_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    _comboBox = new ComboBox(layout->parentWidget());
    
    const std::vector<std::string> &help =  boost::dynamic_pointer_cast<Choice_Knob>(getKnob())->getEntriesHelp();
    for (U32 i = 0; i < _entries.size(); ++i) {
        std::string helpStr = help.empty() ? "" : help[i];
        _comboBox->addItem(_entries[i].c_str(), QIcon(), QKeySequence(), QString(helpStr.c_str()));
    }
    if (_entries.size() > 0) {
        _comboBox->setCurrentText(_entries[0].c_str());
    }
    _comboBox->setToolTip(getKnob()->getHintToolTip().c_str());
    QObject::connect(_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
    layout->addWidget(_comboBox, row, 1, Qt::AlignLeft);
}
void Choice_KnobGui::onCurrentIndexChanged(int i)
{
    pushUndoCommand(new KnobUndoCommand(this, 0, getKnob()->getValue(0), Variant(i)));
    
}
void Choice_KnobGui::onEntriesPopulated()
{
    int i = _comboBox->activeIndex();
    _comboBox->clear();
    const std::vector<std::string> entries = boost::dynamic_pointer_cast<Choice_Knob>(getKnob())->getEntries();
    _entries = entries;
    for (U32 j = 0; j < _entries.size(); ++j) {
        _comboBox->addItem(_entries[j].c_str());
    }
    _comboBox->setCurrentText(QString(_entries[i].c_str()));
}

void Choice_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    int i = variant.toInt();
    assert(i < (int)_entries.size());
    _comboBox->setCurrentText(_entries[i].c_str());
}

void Choice_KnobGui::reflectAnimationLevel(int /*dimension*/,Natron::AnimationLevel level) {
    switch (level) {
        case Natron::NO_ANIMATION:
            _comboBox->setAnimation(0);
            break;
        case Natron::INTERPOLATED_VALUE:
            _comboBox->setAnimation(1);
            break;
        case Natron::ON_KEYFRAME:
            _comboBox->setAnimation(2);
            break;
        default:
            break;
    }
}


void Choice_KnobGui::_hide()
{
    _descriptionLabel->hide();
    _comboBox->hide();
}

void Choice_KnobGui::_show()
{
    _descriptionLabel->show();
    _comboBox->show();
}
void Choice_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _descriptionLabel->setEnabled(b);
    _comboBox->setEnabled(b);
}


//=============================SEPARATOR_KNOB_GUI===================================
void Separator_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    _line = new QFrame(layout->parentWidget());
    _line->setFrameShape(QFrame::HLine);
    _line->setFrameShadow(QFrame::Sunken);
    _line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(_line, row, 1, Qt::AlignLeft);
}

Separator_KnobGui::~Separator_KnobGui()
{
    delete _descriptionLabel;
    delete _line;
}
void Separator_KnobGui::_hide()
{
    _descriptionLabel->hide();
    _line->hide();
}

void Separator_KnobGui::_show()
{
    _descriptionLabel->show();
    _line->show();
}



//=============================RGBA_KNOB_GUI===================================

Color_KnobGui::Color_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
: KnobGui(knob, container)
, mainContainer(NULL)
, mainLayout(NULL)
, boxContainers(NULL)
, boxLayout(NULL)
, colorContainer(NULL)
, colorLayout(NULL)
, _descriptionLabel(NULL)
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
, _dimension(knob->getDimension()) {}

Color_KnobGui::~Color_KnobGui()
{
    delete mainContainer;
}

void Color_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    mainContainer = new QWidget(layout->parentWidget());
    mainLayout = new QHBoxLayout(mainContainer);
    mainContainer->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    boxContainers = new QWidget(mainContainer);
    boxLayout = new QHBoxLayout(boxContainers);
    boxContainers->setLayout(boxLayout);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setSpacing(0);
    
    
    _rBox = new SpinBox(boxContainers, SpinBox::DOUBLE_SPINBOX);
    QObject::connect(_rBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    
    if (_dimension >= 3) {
        _gBox = new SpinBox(boxContainers, SpinBox::DOUBLE_SPINBOX);
        QObject::connect(_gBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
        _bBox = new SpinBox(boxContainers, SpinBox::DOUBLE_SPINBOX);
        QObject::connect(_bBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    }
    if (_dimension >= 4) {
        _aBox = new SpinBox(boxContainers, SpinBox::DOUBLE_SPINBOX);
        QObject::connect(_aBox, SIGNAL(valueChanged(double)), this, SLOT(onColorChanged()));
    }
    
    
    _rBox->setMaximum(1.);
    _rBox->setMinimum(0.);
    _rBox->setIncrement(0.1);
    _rBox->setToolTip(getKnob()->getHintToolTip().c_str());
    
    _rLabel = new QLabel("r:", boxContainers);
    _rLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    
    boxLayout->addWidget(_rLabel);
    boxLayout->addWidget(_rBox);
    
    if (_dimension >= 3) {
        _gBox->setMaximum(1.);
        _gBox->setMinimum(0.);
        _gBox->setIncrement(0.1);
        _gBox->setToolTip(getKnob()->getHintToolTip().c_str());
        
        _gLabel = new QLabel("g:", boxContainers);
        _gLabel->setToolTip(getKnob()->getHintToolTip().c_str());
        
        boxLayout->addWidget(_gLabel);
        boxLayout->addWidget(_gBox);
        
        _bBox->setMaximum(1.);
        _bBox->setMinimum(0.);
        _bBox->setIncrement(0.1);
        _bBox->setToolTip(getKnob()->getHintToolTip().c_str());
        
        _bLabel = new QLabel("b:", boxContainers);
        _bLabel->setToolTip(getKnob()->getHintToolTip().c_str());
        
        boxLayout->addWidget(_bLabel);
        boxLayout->addWidget(_bBox);
    }
    if (_dimension >= 4) {
        _aBox->setMaximum(1.);
        _aBox->setMinimum(0.);
        _aBox->setIncrement(0.1);
        _aBox->setToolTip(getKnob()->getHintToolTip().c_str());
        
        _aLabel = new QLabel("a:", boxContainers);
        _aLabel->setToolTip(getKnob()->getHintToolTip().c_str());
        
        boxLayout->addWidget(_aLabel);
        boxLayout->addWidget(_aBox);
    }
    
    
    colorContainer = new QWidget(mainContainer);
    colorLayout = new QHBoxLayout(colorContainer);
    colorContainer->setLayout(colorLayout);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(0);
    
    _colorLabel = new ColorPickerLabel(colorContainer);
    QObject::connect(_colorLabel,SIGNAL(pickingEnabled(bool)),this,SLOT(onPickingEnabled(bool)));
    colorLayout->addWidget(_colorLabel);
    
    QPixmap buttonPix;
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, &buttonPix);
    
    _colorDialogButton = new Button(QIcon(buttonPix), "", colorContainer);
    QObject::connect(_colorDialogButton, SIGNAL(pressed()), this, SLOT(showColorDialog()));
    colorLayout->addWidget(_colorDialogButton);
    
    mainLayout->addWidget(boxContainers);
    mainLayout->addWidget(colorContainer);
    
    layout->addWidget(mainContainer, row, 1, Qt::AlignLeft);
    
    
}
void Color_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _rBox->setEnabled(b);
    if (_dimension >= 3) {
        _gBox->setEnabled(b);
        _bBox->setEnabled(b);
    }
    if (_dimension >= 4) {
        _aBox->setEnabled(b);
    }
    _colorDialogButton->setEnabled(b);
}

void Color_KnobGui::updateGUI(int dimension, const Variant &variant)
{
    assert(dimension < _dimension && dimension >= 0 && dimension <= 3);
    switch (dimension) {
        case 0:
            _rBox->setValue(variant.toDouble());
            break;
        case 1:
            _gBox->setValue(variant.toDouble());
            break;
        case 2:
            _bBox->setValue(variant.toDouble());
            break;
        case 3:
            _aBox->setValue(variant.toDouble());
            break;
        default:
            throw std::logic_error("wrong dimension");
    }
    
    uchar r = (uchar)std::min(_rBox->value() * 256., 255.);
    uchar g = r;
    uchar b = r;
    uchar a = 255;
    if (_dimension >= 3) {
        g = (uchar)std::min(_gBox->value() * 256., 255.);
        b = (uchar)std::min(_bBox->value() * 256., 255.);
    }
    if (_dimension >= 4) {
        a = (uchar)std::min(_aBox->value() * 256., 255.);
    }
    QColor color(r, g, b, a);
    updateLabel(color);
}

void Color_KnobGui::reflectAnimationLevel(int dimension,Natron::AnimationLevel level) {
    switch (level) {
        case Natron::NO_ANIMATION:
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
            break;
        case Natron::INTERPOLATED_VALUE:
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
            break;
        case Natron::ON_KEYFRAME:
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
            break;
            
        default:
            break;
    }
}

void Color_KnobGui::showColorDialog()
{
    QColorDialog dialog(_rBox->parentWidget());
    if (dialog.exec()) {
        QColor userColor = dialog.currentColor();
        QColor realColor = userColor;
        realColor.setGreen(userColor.red());
        realColor.setBlue(userColor.red());
        realColor.setAlpha(255);
        _rBox->setValue(realColor.redF());
        
        if (_dimension >= 3) {
            _gBox->setValue(userColor.greenF());
            _bBox->setValue(userColor.blueF());
            realColor.setGreen(userColor.green());
            realColor.setBlue(userColor.blue());
        }
        if (_dimension >= 4) {
            _aBox->setValue(userColor.alphaF());
            realColor.setGreen(userColor.green());
            realColor.setBlue(userColor.blue());
            realColor.setAlpha(userColor.alpha());
        }
        updateLabel(realColor);
        
        onColorChanged();
    }
}

void Color_KnobGui::onColorChanged()
{
    QColor color;
    color.setRedF(_rBox->value());
    color.setGreenF(color.redF());
    color.setBlueF(color.greenF());
    std::vector<Variant> newValues;
    newValues.push_back(Variant(color.redF()));
    if (_dimension >= 3) {
        color.setGreenF(_gBox->value());
        color.setBlueF(_bBox->value());
        newValues.push_back( Variant(color.greenF()));
        newValues.push_back(Variant(color.blueF()));
    }
    if (_dimension >= 4) {
        color.setAlphaF(_aBox->value());
        newValues.push_back(Variant(color.alphaF()));
    }
    pushUndoCommand(new KnobMultipleUndosCommand(this, getKnob()->getValueForEachDimension(), newValues));
}


void Color_KnobGui::updateLabel(const QColor &color)
{
    _colorLabel->setColor(color);
}

void Color_KnobGui::_hide()
{
    _descriptionLabel->hide();
    _rBox->hide();
    _rLabel->hide();
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
    
    _colorLabel->hide();
    _colorDialogButton->hide();
}

void Color_KnobGui::_show()
{
    _descriptionLabel->show();
    
    _rBox->show();
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
    
    _colorLabel->show();
    _colorDialogButton->show();
    
}



ColorPickerLabel::ColorPickerLabel(QWidget* parent)
: QLabel(parent)
{
    
    setToolTip("To pick a color on a viewer, click this and then press control + left click on any viewer.\n"
               "Note that by default " NATRON_APPLICATION_NAME " converts to linear the color picked\n"
               "because all the processing pipeline is linear, but you can turn this off in the\n"
               "preference panel.");
    setMouseTracking(true);
}


void ColorPickerLabel::mousePressEvent(QMouseEvent*) {
    _pickingEnabled = ! _pickingEnabled;
    emit pickingEnabled(_pickingEnabled);
    setColor(_currentColor); //< refresh the icon
}

void ColorPickerLabel::enterEvent(QEvent*){
    QToolTip::showText(QCursor::pos(), toolTip());
}

void ColorPickerLabel::leaveEvent(QEvent*){
    QToolTip::hideText();
}

void ColorPickerLabel::setColor(const QColor& color){
    
    
    _currentColor = color;
    
    if(_pickingEnabled){
        QImage img(128, 128, QImage::Format_ARGB32);
        img.fill(color.rgb());
        
        
        //draw the picker on top of the label
        QPixmap pickerIcon;
        appPTR->getIcon(Natron::NATRON_PIXMAP_COLOR_PICKER, &pickerIcon);
        QImage pickerImg = pickerIcon.toImage();
        
        
        for (int i = 0; i < pickerIcon.height(); ++i) {
            const QRgb* data = (QRgb*)pickerImg.scanLine(i);
            for (int j = 0; j < pickerImg.width(); ++j) {
                int alpha = qAlpha(data[j]);
                if(alpha > 0){
                    img.setPixel(j, i, data[j]);
                }
            }
        }
        QPixmap pix = QPixmap::fromImage(img).scaled(20, 20);
        setPixmap(pix);
        
    }else{
        QImage img(20, 20, QImage::Format_ARGB32);
        img.fill(color.rgb());
        QPixmap pix = QPixmap::fromImage(img);
        setPixmap(pix);
    }
    
}

void Color_KnobGui::onPickingEnabled(bool enabled){
    if(getKnob()->getHolder()->getApp()){
        boost::shared_ptr<Color_Knob> colorKnob = boost::dynamic_pointer_cast<Color_Knob>(getKnob());
        if (enabled) {
            getKnob()->getHolder()->getApp()->getGui()->_projectGui->registerNewColorPicker(colorKnob);
        }else{
            getKnob()->getHolder()->getApp()->getGui()->_projectGui->removeColorPicker(colorKnob);
        }
    }
    
}

//=============================STRING_KNOB_GUI===================================
void String_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    _lineEdit = new LineEdit(layout->parentWidget());
    _lineEdit->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_lineEdit, row, 1, Qt::AlignLeft);
    QObject::connect(_lineEdit, SIGNAL(textEdited(QString)), this, SLOT(onStringChanged(QString)));
    
}

String_KnobGui::~String_KnobGui()
{
    delete _descriptionLabel;
    delete _lineEdit;
}

void String_KnobGui::onStringChanged(const QString &str)
{
    pushUndoCommand(new KnobUndoCommand(this, 0, getKnob()->getValue(0), Variant(str)));
}
void String_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    _lineEdit->setText(variant.toString());
}
void String_KnobGui::_hide()
{
    _descriptionLabel->hide();
    _lineEdit->hide();
}

void String_KnobGui::_show()
{
    _descriptionLabel->show();
    _lineEdit->show();
}
void String_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _descriptionLabel->setEnabled(b);
    _lineEdit->setEnabled(b);
}

//=============================CUSTOM_KNOB_GUI===================================
void Custom_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    _lineEdit = new LineEdit(layout->parentWidget());
    _lineEdit->setToolTip(getKnob()->getHintToolTip().c_str());
    _lineEdit->setReadOnly(true);
    layout->addWidget(_lineEdit, row, 1, Qt::AlignLeft);
}

Custom_KnobGui::~Custom_KnobGui()
{
    delete _descriptionLabel;
    delete _lineEdit;
}

void Custom_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    _lineEdit->setText(variant.toString());
}

void Custom_KnobGui::_hide()
{
    _descriptionLabel->hide();
    _lineEdit->hide();
}

void Custom_KnobGui::_show()
{
    _descriptionLabel->show();
    _lineEdit->show();
}

void Custom_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _descriptionLabel->setEnabled(b);
    _lineEdit->setEnabled(b);
}

//=============================GROUP_KNOB_GUI===================================
GroupBoxLabel::GroupBoxLabel(QWidget *parent):
QLabel(parent),
_checked(false)
{
    
    QObject::connect(this, SIGNAL(checked(bool)), this, SLOT(setChecked(bool)));
}

void GroupBoxLabel::setChecked(bool b)
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

Group_KnobGui::~Group_KnobGui()
{
    delete _button;
    delete _descriptionLabel;
    //    for(U32 i  = 0 ; i < _children.size(); ++i){
    //        delete _children[i].first;
    //    }
    
}

void Group_KnobGui::addKnob(KnobGui *child, int row, int column) {
    _children.push_back(std::make_pair(child, std::make_pair(row, column)));
}

bool Group_KnobGui::isChecked() const {
    return _button->isChecked();
}

void Group_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _layout = layout;
    _button = new GroupBoxLabel(layout->parentWidget());
    _button->setToolTip(getKnob()->getHintToolTip().c_str());
    _button->setChecked(_checked);
    QWidget *header = new QWidget(layout->parentWidget());
    QHBoxLayout *headerLay = new QHBoxLayout(header);
    header->setLayout(headerLay);
    QObject::connect(_button, SIGNAL(checked(bool)), this, SLOT(setChecked(bool)));
    headerLay->addWidget(_button);
    headerLay->setSpacing(1);
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    headerLay->addWidget(_descriptionLabel);
    
    layout->addWidget(header, row, 0, 1, 2, Qt::AlignLeft);
    
    
}

void Group_KnobGui::setChecked(bool b)
{
    if (b == _checked) {
        return;
    }
    _checked = b;
    for (U32 i = 0 ; i < _children.size() ; ++i) {
        if (!b) {
            _children[i].first->hide();
        } else if (!_children[i].first->getKnob()->isSecret()) {
            _children[i].first->show();
        }
    }
}

void Group_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    bool b = variant.toBool();
    setChecked(b);
    _button->setChecked(b);
}

void Group_KnobGui::_hide()
{
    _button->hide();
    _descriptionLabel->hide();
    for (U32 i = 0 ; i < _children.size() ; ++i) {
        _children[i].first->hide();
        
    }
    
}

void Group_KnobGui::_show()
{
    if (getKnob()->isSecret()) {
        return;
    }
    _button->show();
    _descriptionLabel->show();
    for (U32 i = 0 ; i < _children.size() ; ++i) {
        _children[i].first->show();
    }
    
}

//=============================RICH_TEXT_KNOBGUI===================================

void RichText_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    _descriptionLabel->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    _textEdit = new QTextEdit(layout->parentWidget());
    _textEdit->setToolTip(getKnob()->getHintToolTip().c_str());
    layout->addWidget(_textEdit, row, 1, Qt::AlignLeft);
    QObject::connect(_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
    
}

RichText_KnobGui::~RichText_KnobGui()
{
    delete _descriptionLabel;
    delete _textEdit;
    
}

void RichText_KnobGui::_hide()
{
    _descriptionLabel->hide();
    _textEdit->hide();
}

void RichText_KnobGui::_show()
{
    _descriptionLabel->show();
    _textEdit->show();
}
void RichText_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled();
    _descriptionLabel->setEnabled(b);
    _textEdit->setEnabled(b);
}

void RichText_KnobGui::onTextChanged()
{
    pushUndoCommand(new KnobUndoCommand(this,0,getKnob()->getValue(),Variant(_textEdit->toPlainText())));
}


void RichText_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    QObject::disconnect(_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
    QTextCursor cursor = _textEdit->textCursor();
    int pos = cursor.position();
    _textEdit->clear();
    QString txt = variant.toString();
    _textEdit->setPlainText(txt);
    cursor.setPosition(pos);
    _textEdit->setTextCursor(cursor);
    QObject::connect(_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
    
}

//=============================Parametric_KnobGui===================================

Parametric_KnobGui::Parametric_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
: KnobGui(knob, container)
, _curveWidget(NULL)
, _tree(NULL)
, _curves()
{
    
}

Parametric_KnobGui::~Parametric_KnobGui(){
    delete _curveWidget;
    delete _tree;
}

void Parametric_KnobGui::createWidget(QGridLayout *layout, int row) {
    
    boost::shared_ptr<Parametric_Knob> parametricKnob = boost::dynamic_pointer_cast<Parametric_Knob>(getKnob());
    
    _tree = new QTreeWidget(layout->parentWidget());
    _tree->setSelectionMode(QAbstractItemView::NoSelection);
    _tree->setColumnCount(1);
    _tree->header()->close();

    
    layout->addWidget(_tree, row, 0);
    
    _curveWidget = new CurveWidget(boost::shared_ptr<TimeLine>(),layout->parentWidget());
    layout->addWidget(_curveWidget,row,1);
    
    
    for (int i = 0; i < getKnob()->getDimension(); ++i) {
        CurveGui* curve =  _curveWidget->createCurve(parametricKnob->getCurve(i), parametricKnob->getDimensionName(i).c_str());
        QColor color;
        double r,g,b;
        parametricKnob->getCurveColor(i, &r, &g, &b);
        color.setRedF(r);
        color.setGreenF(g);
        color.setBlueF(b);
        curve->setColor(color);
        QTreeWidgetItem* item = new QTreeWidgetItem(_tree);
        item->setSelected(true);
        _curves.insert(std::make_pair(curve, item));
    }
}

void Parametric_KnobGui::_hide() {
    _curveWidget->hide();
    _tree->hide();
}

void Parametric_KnobGui::_show() {
    _curveWidget->show();
    _tree->show();
}

void Parametric_KnobGui::setEnabled() {
    bool b = getKnob()->isEnabled();
    _tree->setEnabled(b);
}


void Parametric_KnobGui::updateGUI(int /*dimension*/, const Variant &/*variant*/) {
    _curveWidget->update();
}
