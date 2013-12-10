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

#define SLIDER_MAX_RANGE 100000

using namespace Natron;
using std::make_pair;

// convenience private classes


//==========================INT_KNOB_GUI======================================
Int_KnobGui::Int_KnobGui(Knob *knob, DockablePanel *container)
: KnobGui(knob, container)
, _slider(0)
{
    Int_Knob *intKnob = dynamic_cast<Int_Knob *>(getKnob());
    assert(intKnob);
    QObject::connect(intKnob, SIGNAL(minMaxChanged(int, int, int)), this, SLOT(onMinMaxChanged(int, int, int)));
    QObject::connect(intKnob, SIGNAL(incrementChanged(int, int)), this, SLOT(onIncrementChanged(int, int)));
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

    Int_Knob *intKnob = dynamic_cast<Int_Knob *>(getKnob());
    assert(intKnob);

    QString subLabels_XYZW[] = {"x:", "y:", "z:", "w:"};

    const std::vector<int> &maximums = intKnob->getMaximums();
    const std::vector<int> &minimums = intKnob->getMinimums();
    const std::vector<int> &increments = intKnob->getIncrements();
    const std::vector<int> &displayMins = intKnob->getDisplayMinimums();
    const std::vector<int> &displayMaxs = intKnob->getDisplayMaximums();
    for (int i = 0; i < dim; ++i) {
        QWidget *boxContainer = new QWidget(layout->parentWidget());
        QHBoxLayout *boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *subDesc = 0;
        if (dim != 1) {
            subDesc = new QLabel(subLabels_XYZW[i], boxContainer);
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
        if (getKnob()->getDimension() == 1 && !dynamic_cast<Int_Knob *>(getKnob())->isSliderDisabled()) {
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
    boost::shared_ptr<Curve> c = getKnob()->getCurve(dimension);
    SequenceTime time = getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    if (c->keyFramesCount() >= 1) {
        const Curve::KeyFrames &keys = c->getKeyFrames();
        for (Curve::KeyFrames::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            if ((*it)->getTime() == time) {
                _spinBoxes[dimension].first->setAnimation(2);
                setIsOnKeyframe(true);
                return;
            }
        }
        _spinBoxes[dimension].first->setAnimation(1);
    } else {
        _spinBoxes[dimension].first->setAnimation(0);
    }
    setIsOnKeyframe(false);
}

void Int_KnobGui::onSliderValueChanged(double d)
{
    assert(getKnob()->getDimension() == 1);
    _spinBoxes[0].first->setValue(d);
    pushUndoCommand(new KnobUndoCommand(this, 0, getKnob()->getValue(0), Variant(d)));
}

void Int_KnobGui::onSpinBoxValueChanged()
{
    std::map<int, Variant> newValues;
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        newValues.insert(std::make_pair(i, Variant(_spinBoxes[i].first->value())));
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
void Int_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        layout->addWidget(_spinBoxes[i].first);
        if (_spinBoxes[i].second) {
            layout->addWidget(_spinBoxes[i].second);
        }
    }
    if (_slider) {
        layout->addWidget(_slider);
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
    boost::shared_ptr<Curve> c = getKnob()->getCurve(0);
    SequenceTime time = getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    if (c->keyFramesCount() >= 1) {
        const Curve::KeyFrames &keys = c->getKeyFrames();
        for (Curve::KeyFrames::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            if ((*it)->getTime() == time) {
                _checkBox->setAnimation(2);
                setIsOnKeyframe(true);
                return;
            }
        }
        _checkBox->setAnimation(1);
    } else {
        _checkBox->setAnimation(0);
    }
    setIsOnKeyframe(false);
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

void Bool_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_checkBox);
}

void AnimatedCheckBox::setAnimation(int i)
{
    animation = i;
    style()->unpolish(this);
    style()->polish(this);
}
//=============================DOUBLE_KNOB_GUI===================================
Double_KnobGui::Double_KnobGui(Knob *knob, DockablePanel *container): KnobGui(knob, container), _slider(0)
{
    Double_Knob *dbl_knob = dynamic_cast<Double_Knob *>(getKnob());
    assert(dbl_knob);
    QObject::connect(dbl_knob, SIGNAL(minMaxChanged(double, double, int)), this, SLOT(onMinMaxChanged(double, double, int)));
    QObject::connect(dbl_knob, SIGNAL(incrementChanged(double, int)), this, SLOT(onIncrementChanged(double, int)));
    QObject::connect(dbl_knob, SIGNAL(decimalsChanged(int, int)), this, SLOT(onDecimalsChanged(int, int)));
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

    Double_Knob *dbl_knob = dynamic_cast<Double_Knob *>(getKnob());
    assert(dbl_knob);
    int dim = getKnob()->getDimension();

    QString subLabels_XYZW[] = {"x:", "y:", "z:", "w:"};

    const std::vector<double> &maximums = dbl_knob->getMaximums();
    const std::vector<double> &minimums = dbl_knob->getMinimums();
    const std::vector<double> &increments = dbl_knob->getIncrements();
    const std::vector<double> &displayMins = dbl_knob->getDisplayMinimums();
    const std::vector<double> &displayMaxs = dbl_knob->getDisplayMaximums();

    const std::vector<int> &decimals = dbl_knob->getDecimals();
    for (int i = 0; i < dim; ++i) {
        QWidget *boxContainer = new QWidget(layout->parentWidget());
        QHBoxLayout *boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *subDesc = 0;
        if (dim != 1) {
            subDesc = new QLabel(subLabels_XYZW[i], boxContainer);
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
        if (getKnob()->getDimension() == 1 && !dynamic_cast<Double_Knob *>(getKnob())->isSliderDisabled()) {
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
    boost::shared_ptr<Curve> c = getKnob()->getCurve(dimension);
    SequenceTime time = getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    if (c->keyFramesCount() >= 1) {
        const Curve::KeyFrames &keys = c->getKeyFrames();
        for (Curve::KeyFrames::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            if ((*it)->getTime() == time) {
                _spinBoxes[dimension].first->setAnimation(2);
                setIsOnKeyframe(true);
                return;
            }
        }
        _spinBoxes[dimension].first->setAnimation(1);
    } else {
        _spinBoxes[dimension].first->setAnimation(0);
    }
    setIsOnKeyframe(false);
}

void Double_KnobGui::onSliderValueChanged(double d)
{
    assert(getKnob()->getDimension() == 1);
    _spinBoxes[0].first->setValue(d);
    pushUndoCommand(new KnobUndoCommand(this, 0, getKnob()->getValue(0), Variant(d)));
}
void Double_KnobGui::onSpinBoxValueChanged()
{
    std::map<int, Variant> newValues;
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        newValues.insert(std::make_pair(i, Variant(_spinBoxes[i].first->value())));
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
void Double_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        layout->addWidget(_spinBoxes[i].first);
        if (_spinBoxes[i].second) {
            layout->addWidget(_spinBoxes[i].second);
        }
    }
    if (_slider) {
        layout->addWidget(_slider);
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
    emit valueChanged(0, Variant());
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
void Button_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_button);
}


//=============================CHOICE_KNOB_GUI===================================
Choice_KnobGui::Choice_KnobGui(Knob *knob, DockablePanel *container): KnobGui(knob, container)
{
    Choice_Knob *cbKnob = dynamic_cast<Choice_Knob *>(knob);
    _entries = cbKnob->getEntries();
    QObject::connect(cbKnob, SIGNAL(populated()), this, SLOT(onEntriesPopulated()));
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

    const std::vector<std::string> &help =  dynamic_cast<Choice_Knob *>(getKnob())->getEntriesHelp();
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
    const std::vector<std::string> entries = dynamic_cast<Choice_Knob *>(getKnob())->getEntries();
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
    
    boost::shared_ptr<Curve> c = getKnob()->getCurve(0);
    SequenceTime time = getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
    if (c->keyFramesCount() >= 1) {
        const Curve::KeyFrames &keys = c->getKeyFrames();
        for (Curve::KeyFrames::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            if ((*it)->getTime() == time) {
                _comboBox->setAnimation(2);
                setIsOnKeyframe(true);
                return;
            }
        }
        _comboBox->setAnimation(1);
    } else {
        _comboBox->setAnimation(0);
    }
    setIsOnKeyframe(false);
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
void Choice_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_comboBox);

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


void Separator_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_line);

}
//=============================RGBA_KNOB_GUI===================================

Color_KnobGui::Color_KnobGui(Knob *knob, DockablePanel *container)
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

    _colorLabel = new QLabel(colorContainer);
    _colorLabel->setToolTip(getKnob()->getHintToolTip().c_str());
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

void Color_KnobGui::showColorDialog()
{
    QColorDialog dialog(_rBox->parentWidget());
    if (dialog.exec()) {
        QColor color = dialog.getColor();
        color.setGreen(color.red());
        color.setBlue(color.red());
        color.setAlpha(255);
        _rBox->setValue(color.redF());

        if (_dimension >= 3) {
            color.setAlpha(255);
            _gBox->setValue(color.greenF());
            _bBox->setValue(color.blueF());
        }
        if (_dimension >= 4) {
            _aBox->setValue(color.alphaF());
        }
        updateLabel(color);

        onColorChanged();
    }
}

void Color_KnobGui::onColorChanged()
{
    QColor color;
    color.setRedF(_rBox->value());
    color.setGreenF(color.redF());
    color.setBlueF(color.greenF());
    std::map<int, Variant> newValues;
    newValues.insert(make_pair(0, Variant(color.redF())));
    if (_dimension >= 3) {
        color.setGreenF(_gBox->value());
        color.setBlueF(_bBox->value());
        newValues.insert(make_pair(1, Variant(color.greenF())));
        newValues.insert(make_pair(2, Variant(color.blueF())));
    }
    if (_dimension >= 4) {
        color.setAlphaF(_aBox->value());
        newValues.insert(make_pair(3, Variant(color.alphaF())));
    }
    pushUndoCommand(new KnobMultipleUndosCommand(this, getKnob()->getValueForEachDimension(), newValues));
}


void Color_KnobGui::updateLabel(const QColor &color)
{
    QImage img(20, 20, QImage::Format_RGB32);
    img.fill(color.rgb());
    QPixmap pix = QPixmap::fromImage(img);
    _colorLabel->setPixmap(pix);
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
void Color_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);

    layout->addWidget(_rLabel);
    layout->addWidget(_rBox);
    if (_dimension >= 3) {
        layout->addWidget(_gLabel);
        layout->addWidget(_gBox);
        layout->addWidget(_bLabel);
        layout->addWidget(_bBox);
    }
    if (_dimension >= 4) {
        layout->addWidget(_aLabel);
        layout->addWidget(_aBox);
    }

    layout->addWidget(_colorLabel);
    layout->addWidget(_colorDialogButton);
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

void String_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_lineEdit);

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

void Custom_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_lineEdit);
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

void Group_KnobGui::addToLayout(QHBoxLayout *layout)
{
    QWidget *mainWidget = new QWidget(_layout->parentWidget());
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainWidget->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *header = new QWidget(mainWidget);
    QHBoxLayout *headerLayout = new QHBoxLayout(mainWidget);
    header->setLayout(headerLayout);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->addWidget(_button);
    headerLayout->addWidget(_descriptionLabel);

    mainLayout->addWidget(header);
    for (U32 i = 0; i < _children.size(); ++i) {
        QWidget *container = new QWidget(_layout->parentWidget());
        QHBoxLayout *containerLayout = new QHBoxLayout(container);
        container->setLayout(containerLayout);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->addStretch();
        _children[i].first->addToLayout(containerLayout);
        mainLayout->addWidget(container);
    }
    layout->addWidget(mainWidget);
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

void RichText_KnobGui::addToLayout(QHBoxLayout *layout)
{
    layout->addWidget(_descriptionLabel);
    layout->addWidget(_textEdit);
}

void RichText_KnobGui::onTextChanged()
{
    std::map<int, Variant> newV;
    newV.insert(make_pair(0, Variant(_textEdit->toPlainText())));
    pushUndoCommand(new KnobMultipleUndosCommand(this, getKnob()->getValueForEachDimension(), newV));
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

