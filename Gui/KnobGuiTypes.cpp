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

#include "Engine/KnobTypes.h"
#include "Engine/Curve.h"
#include "Engine/TimeLine.h"
#include "Engine/Lut.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/SpinBox.h"
#include "Gui/ComboBox.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/ProjectGui.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"

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
    QObject::connect(intKnob.get(), SIGNAL(displayMinMaxChanged(int, int, int)), this, SLOT(onDisplayMinMaxChanged(int, int, int)));
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
    if(hasToolTip()) {
        _descriptionLabel->setToolTip(toolTip());
    }
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    int dim = getKnob()->getDimension();
    
    QWidget *container = new QWidget(layout->parentWidget());
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    boost::shared_ptr<Int_Knob> intKnob = boost::dynamic_pointer_cast<Int_Knob>(getKnob());
    assert(intKnob);
    
    
    //  const std::vector<int> &maximums = intKnob->getMaximums();
    //    const std::vector<int> &minimums = intKnob->getMinimums();
    const std::vector<int> &increments = intKnob->getIncrements();
    const std::vector<int> &displayMins = intKnob->getDisplayMinimums();
    const std::vector<int> &displayMaxs = intKnob->getDisplayMaximums();
    for (int i = 0; i < dim; ++i) {
        QWidget *boxContainer = new QWidget(layout->parentWidget());
        QHBoxLayout *boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        boxContainer->setLayout(boxContainerLayout);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        boxContainerLayout->setSpacing(1);
        QLabel *subDesc = 0;
        if (dim != 1) {
            subDesc = new QLabel(QString(getKnob()->getDimensionName(i).c_str())+':', boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox *box = new SpinBox(layout->parentWidget(), SpinBox::INT_SPINBOX);
        QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()));
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(box,i);
        
        int min = displayMins[i];
        int max = displayMaxs[i];
        
        box->setMaximum(max);
        box->setMinimum(min);
        box->setIncrement(increments[i]);
        if(hasToolTip()) {
            box->setToolTip(toolTip());
        }
        boxContainerLayout->addWidget(box);
        if (getKnob()->getDimension() == 1 && !intKnob->isSliderDisabled()) {
            
            if ((max - min) < SLIDER_MAX_RANGE && max < INT_MAX && min > INT_MIN) {
                _slider = new ScaleSliderQWidget(min, max,
                                          getKnob()->getValue<int>(), Natron::LINEAR_SCALE, layout->parentWidget());
                if(hasToolTip()) {
                    _slider->setToolTip(toolTip());
                }
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

void Int_KnobGui::onDisplayMinMaxChanged(int mini, int maxi, int index)
{
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
    if(_slider){
        _slider->setMinimumAndMaximum(mini, maxi);
    }
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
    pushValueChangedCommand(Variant(d));
}

void Int_KnobGui::onSpinBoxValueChanged()
{
    std::vector<Variant> newValues;
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        newValues.push_back(Variant(_spinBoxes[i].first->value()));
    }
    pushValueChangedCommand(newValues);
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
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        bool b = getKnob()->isEnabled(i);
        _spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->setEnabled(b);
        }
    }
    if (_slider) {
        _slider->setReadOnly(!getKnob()->isEnabled(0));
    }
    
}

void Int_KnobGui::setReadOnly(bool readOnly,int dimension) {
    
    assert(dimension < (int)_spinBoxes.size());
    _spinBoxes[dimension].first->setReadOnly(readOnly);
    if (_slider && dimension == 0) {
        _slider->setReadOnly(readOnly);
    }
}

//==========================BOOL_KNOB_GUI======================================

void Bool_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new ClickableLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    if(hasToolTip()) {
        _descriptionLabel->setToolTip(toolTip());
    }
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    _checkBox = new AnimatedCheckBox(layout->parentWidget());
    if(hasToolTip()) {
        _checkBox->setToolTip(toolTip());
    }
    QObject::connect(_checkBox, SIGNAL(clicked(bool)), this, SLOT(onCheckBoxStateChanged(bool)));
    QObject::connect(_descriptionLabel, SIGNAL(clicked(bool)), this, SLOT(onCheckBoxStateChanged(bool)));
    
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_checkBox,0);

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
    pushValueChangedCommand(Variant(b));
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
    bool b = getKnob()->isEnabled(0);
    _descriptionLabel->setEnabled(b);
    _checkBox->setEnabled(b);
}

void Bool_KnobGui::setReadOnly(bool readOnly,int /*dimension*/) {
    _checkBox->setReadOnly(readOnly);
}



//=============================DOUBLE_KNOB_GUI===================================
Double_KnobGui::Double_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container), _slider(0)
{
    boost::shared_ptr<Double_Knob> dbl_knob = boost::dynamic_pointer_cast<Double_Knob>(getKnob());
    assert(dbl_knob);
    QObject::connect(dbl_knob.get(), SIGNAL(minMaxChanged(double, double, int)), this, SLOT(onMinMaxChanged(double, double, int)));
    QObject::connect(dbl_knob.get(), SIGNAL(displayMinMaxChanged(double, double, int)), this, SLOT(onDisplayMinMaxChanged(double, double, int)));
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
    if(hasToolTip()) {
        _descriptionLabel->setToolTip(toolTip());
    }
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    QWidget *container = new QWidget(layout->parentWidget());
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    boost::shared_ptr<Double_Knob> dbl_knob = boost::dynamic_pointer_cast<Double_Knob>(getKnob());
    assert(dbl_knob);
    int dim = getKnob()->getDimension();
    
    
    //  const std::vector<double> &maximums = dbl_knob->getMaximums();
    //    const std::vector<double> &minimums = dbl_knob->getMinimums();
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
        boxContainerLayout->setSpacing(1);
        QLabel *subDesc = 0;
        if (dim != 1) {
            subDesc = new QLabel(QString(getKnob()->getDimensionName(i).c_str())+':', boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }
        SpinBox *box = new SpinBox(layout->parentWidget(), SpinBox::DOUBLE_SPINBOX);
        QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()));
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(box,i);

        
        double min = displayMins[i];
        double max = displayMaxs[i];
        
        box->setMaximum(max);
        box->setMinimum(min);
        box->decimals(decimals[i]);
        box->setIncrement(increments[i]);
        if(hasToolTip()) {
            box->setToolTip(toolTip());
        }
        boxContainerLayout->addWidget(box);
        
        if (getKnob()->getDimension() == 1 && !dbl_knob->isSliderDisabled()) {

            if ((max - min) < SLIDER_MAX_RANGE && max < DBL_MAX && min > -DBL_MAX) {
                _slider = new ScaleSliderQWidget(min, max,
                                          getKnob()->getValue<double>(), Natron::LINEAR_SCALE, layout->parentWidget());
                if(hasToolTip()) {
                    _slider->setToolTip(toolTip());
                }
                QObject::connect(_slider, SIGNAL(positionChanged(double)), this, SLOT(onSliderValueChanged(double)));

                boxContainerLayout->addWidget(_slider);
            }
        }
        
        containerLayout->addWidget(boxContainer);
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

void Double_KnobGui::onDisplayMinMaxChanged(double mini,double maxi,int index ){
    _spinBoxes[index].first->setMinimum(mini);
    _spinBoxes[index].first->setMaximum(maxi);
    if(_slider){
        _slider->setMinimumAndMaximum(mini, maxi);
    }
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
    pushValueChangedCommand(Variant(d));
}
void Double_KnobGui::onSpinBoxValueChanged()
{
    std::vector<Variant> newValues;
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        newValues.push_back(Variant(_spinBoxes[i].first->value()));
    }
    pushValueChangedCommand(newValues);
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
    for (U32 i = 0; i < _spinBoxes.size(); ++i) {
        bool b = getKnob()->isEnabled(i);
        _spinBoxes[i].first->setEnabled(b);
        _spinBoxes[i].first->setReadOnly(!b);
        if (_spinBoxes[i].second) {
            _spinBoxes[i].second->setEnabled(b);
        }
    }
    if (_slider) {
        _slider->setReadOnly(!getKnob()->isEnabled(0));
    }
    
}

void Double_KnobGui::setReadOnly(bool readOnly,int dimension) {
    assert(dimension < (int)_spinBoxes.size());
    _spinBoxes[dimension].first->setReadOnly(readOnly);
    if (_slider && dimension==0) {
        _slider->setReadOnly(readOnly);
    }
}

//=============================BUTTON_KNOB_GUI===================================
void Button_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _button = new Button(QString(QString(getKnob()->getDescription().c_str())), layout->parentWidget());
    QObject::connect(_button, SIGNAL(clicked()), this, SLOT(emitValueChanged()));
    if(hasToolTip()) {
        _button->setToolTip(toolTip());
    }
    layout->addWidget(_button, row, 1, Qt::AlignLeft);
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
    bool b = getKnob()->isEnabled(0);
    _button->setEnabled(b);
}

void Button_KnobGui::setReadOnly(bool readOnly,int /*dimension*/) {
    _button->setEnabled(!readOnly);
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
    if (hasToolTip()) {
        _descriptionLabel->setToolTip(toolTip());
    }
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    _comboBox = new ComboBox(layout->parentWidget());
    onEntriesPopulated();
    if (hasToolTip()) {
        _comboBox->setToolTip(toolTip());
    }
    QObject::connect(_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
    
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_comboBox,0);

    layout->addWidget(_comboBox, row, 1, Qt::AlignLeft);
}

void Choice_KnobGui::onCurrentIndexChanged(int i)
{
    pushValueChangedCommand(Variant(i));
}

void Choice_KnobGui::onEntriesPopulated()
{
    int activeIndex = _comboBox->activeIndex();
    _comboBox->clear();
    _entries = boost::dynamic_pointer_cast<Choice_Knob>(getKnob())->getEntries();
    const std::vector<std::string> &help =  boost::dynamic_pointer_cast<Choice_Knob>(getKnob())->getEntriesHelp();
    for (U32 i = 0; i < _entries.size(); ++i) {
        std::string helpStr = help.empty() ? "" : help[i];
        _comboBox->addItem(_entries[i].c_str(), QIcon(), QKeySequence(), QString(helpStr.c_str()));
    }
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///we don't want that to happen because the index actually didn't change.
    _comboBox->setCurrentIndex_no_emit(activeIndex);
}

void Choice_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    int i = variant.toInt();
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///change the internal value of the knob again...
    ///The slot connected to onCurrentIndexChanged is reserved to catch user interaction with the combobox.
    ///This function is called in response to an internal change.
    _comboBox->setCurrentIndex_no_emit(i);
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
    bool b = getKnob()->isEnabled(0);
    _descriptionLabel->setEnabled(b);
    _comboBox->setEnabled(b);
}


void Choice_KnobGui::setReadOnly(bool readOnly,int /*dimension*/)
{
    _comboBox->setReadOnly(readOnly);
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
//Table_KnobGui::Table_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
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
//void Table_KnobGui::updateGUI(int dimension, const Variant &variant) {
//    if(dimension < (int)_choices.size()){
//        _choices[dimension]->setCurrentText(_choices[dimension]->itemText(variant.toInt()));
//    }
//}


//=============================SEPARATOR_KNOB_GUI===================================
void Separator_KnobGui::createWidget(QGridLayout *layout, int row)
{
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    if(hasToolTip()) {
        _descriptionLabel->setToolTip(toolTip());
    }
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignLeft);
    
    ///FIXME: this line is never visible.
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
    if(hasToolTip()) {
        _descriptionLabel->setToolTip(toolTip());
    }
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    
    mainContainer = new QWidget(layout->parentWidget());
    mainLayout = new QHBoxLayout(mainContainer);
    mainContainer->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    boxContainers = new QWidget(mainContainer);
    boxLayout = new QHBoxLayout(boxContainers);
    boxContainers->setLayout(boxLayout);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    boxLayout->setSpacing(1);
    
    
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
    
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_rBox,0);
    
    if(hasToolTip()) {
        _rBox->setToolTip(toolTip());
    }
    _rLabel = new QLabel("r:", boxContainers);
    if(hasToolTip()) {
        _rLabel->setToolTip(toolTip());
    }
    boxLayout->addWidget(_rLabel);
    boxLayout->addWidget(_rBox);
    
    if (_dimension >= 3) {
        _gBox->setMaximum(1.);
        _gBox->setMinimum(0.);
        _gBox->setIncrement(0.1);
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_gBox,1);

        if(hasToolTip()) {
            _gBox->setToolTip(toolTip());
        }
        _gLabel = new QLabel("g:", boxContainers);
        if(hasToolTip()) {
            _gLabel->setToolTip(toolTip());
        }
        boxLayout->addWidget(_gLabel);
        boxLayout->addWidget(_gBox);
        
        _bBox->setMaximum(1.);
        _bBox->setMinimum(0.);
        _bBox->setIncrement(0.1);
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_bBox,2);

        
        if(hasToolTip()) {
            _bBox->setToolTip(toolTip());
        }
        _bLabel = new QLabel("b:", boxContainers);
        if(hasToolTip()) {
            _bLabel->setToolTip(toolTip());
        }
        boxLayout->addWidget(_bLabel);
        boxLayout->addWidget(_bBox);
    }
    if (_dimension >= 4) {
        _aBox->setMaximum(1.);
        _aBox->setMinimum(0.);
        _aBox->setIncrement(0.1);
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_aBox,3);

        
        if(hasToolTip()) {
            _aBox->setToolTip(toolTip());
        }
        _aLabel = new QLabel("a:", boxContainers);
        if(hasToolTip()) {
            _aLabel->setToolTip(toolTip());
        }
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
    QObject::connect(_colorDialogButton, SIGNAL(clicked()), this, SLOT(showColorDialog()));
    colorLayout->addWidget(_colorDialogButton);
    
    mainLayout->addWidget(boxContainers);
    mainLayout->addWidget(colorContainer);
    
    layout->addWidget(mainContainer, row, 1, Qt::AlignLeft);
    
    
}
void Color_KnobGui::setEnabled()
{
    bool r = getKnob()->isEnabled(0);
    
    _rBox->setEnabled(r);
    if (_dimension >= 3) {
        bool g = getKnob()->isEnabled(1);
        bool b = getKnob()->isEnabled(2);
        _gBox->setEnabled(g);
        _bBox->setEnabled(b);
    }
    if (_dimension >= 4) {
        bool a = getKnob()->isEnabled(3);
        _aBox->setEnabled(a);

    }
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
    
    uchar r = Color::floatToInt<256>(_rBox->value());
    uchar g = r;
    uchar b = r;
    uchar a = 255;
    if (_dimension >= 3) {
        g = Color::floatToInt<256>(_gBox->value());
        b = Color::floatToInt<256>(_bBox->value());
    }
    if (_dimension >= 4) {
        a = Color::floatToInt<256>(_aBox->value());
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
        
        if (getKnob()->isEnabled(0)) {
            _rBox->setValue(realColor.redF());
        }
        
        if (_dimension >= 3) {
            
            if (getKnob()->isEnabled(1)) {
                _gBox->setValue(userColor.greenF());
            }
            if (getKnob()->isEnabled(2)) {
                _bBox->setValue(userColor.blueF());
            }
            realColor.setGreen(userColor.green());
            realColor.setBlue(userColor.blue());
        }
        if (_dimension >= 4) {
            
            if (getKnob()->isEnabled(3)) {
                _aBox->setValue(userColor.alphaF());
            }
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
    pushUndoCommand(new KnobUndoCommand(this, getKnob()->getValueForEachDimension(), newValues));
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
, _pickingEnabled(false)
{
    
    setToolTip(Qt::convertFromPlainText("To pick a color on a viewer, click this and then press control + left click on any viewer.\n"
               "Note that by default " NATRON_APPLICATION_NAME " converts to linear the color picked\n"
               "because all the processing pipeline is linear, but you can turn this off in the\n"
               "preference panel.", Qt::WhiteSpaceNormal));
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
            getGui()->registerNewColorPicker(colorKnob);
        }else{
            getGui()->removeColorPicker(colorKnob);
        }
    }
    
}

void Color_KnobGui::setReadOnly(bool readOnly,int dimension) {
    if (dimension == 0 && _rBox) {
        _rBox->setReadOnly(readOnly);
    }
    else if (dimension == 1 && _gBox) {
        _gBox->setReadOnly(readOnly);
    }
    else if (dimension == 2 && _bBox) {
        _bBox->setReadOnly(readOnly);
    }
    else if (dimension == 3 && _aBox) {
        _aBox->setReadOnly(readOnly);
    } else {
        assert(false); //< dim invalid
    }
}
//=============================STRING_KNOB_GUI===================================

void AnimatingTextEdit::setAnimation(int v) {
    animation = v;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

void AnimatingTextEdit::setReadOnlyNatron(bool ro) {
    setReadOnly(ro);
    readOnlyNatron = ro;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

void AnimatingTextEdit::focusOutEvent(QFocusEvent* e) {
    if (_hasChanged) {
        _hasChanged = false;
        emit editingFinished();
    }
    QTextEdit::focusOutEvent(e);
}

void AnimatingTextEdit::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Return) {
        if (_hasChanged) {
            _hasChanged = false;
            emit editingFinished();
        }
    }
    _hasChanged = true;
    QTextEdit::keyPressEvent(e);
}

String_KnobGui::String_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
: KnobGui(knob, container)
, _lineEdit(0)
, _textEdit(0)
, _label(0)
, _descriptionLabel(0)
{
}

void String_KnobGui::createWidget(QGridLayout *layout, int row)
{
    boost::shared_ptr<String_Knob> strKnob = boost::dynamic_pointer_cast<String_Knob>(getKnob());
    assert(strKnob);
    
    _descriptionLabel = new QLabel(QString(QString(strKnob->getDescription().c_str()) + ":"), layout->parentWidget());
    if(hasToolTip()) {
        _descriptionLabel->setToolTip(toolTip());
    }
    layout->addWidget(_descriptionLabel, row, 0, Qt::AlignRight);
    

    if (strKnob->isMultiLine()) {
        _textEdit = new AnimatingTextEdit(layout->parentWidget());
        if (hasToolTip()) {
            _textEdit->setToolTip(toolTip());
        }
        layout->addWidget(_textEdit, row, 1);
        QObject::connect(_textEdit, SIGNAL(editingFinished()), this, SLOT(onTextChanged()));

        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_textEdit,0);

    } else if (strKnob->isLabel()) {
        _label = new QLabel(layout->parentWidget());
        if (hasToolTip()) {
            _label->setToolTip(toolTip());
        }
        layout->addWidget(_label, row, 1);
    } else {
        _lineEdit = new LineEdit(layout->parentWidget());
        if (hasToolTip()) {
            _lineEdit->setToolTip(toolTip());
        }
        layout->addWidget(_lineEdit, row, 1);
        QObject::connect(_lineEdit, SIGNAL(editingFinished()), this, SLOT(onLineChanged()));
        
        if (strKnob->isCustomKnob()) {
            _lineEdit->setReadOnly(true);
        }
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_lineEdit,0);
    }
}

String_KnobGui::~String_KnobGui()
{
    delete _descriptionLabel;
    delete _lineEdit;
    delete _label;
    delete _textEdit;
}

void String_KnobGui::onLineChanged()
{
    pushValueChangedCommand(Variant(_lineEdit->text()));
}

void String_KnobGui::onTextChanged()
{
    pushValueChangedCommand(Variant(_textEdit->toPlainText()));
}


void String_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    boost::shared_ptr<String_Knob> strKnob = boost::dynamic_pointer_cast<String_Knob>(getKnob());
    assert(strKnob);

    if (strKnob->isMultiLine()) {
        assert(_textEdit);
        QObject::disconnect(_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
        QTextCursor cursor = _textEdit->textCursor();
        int pos = cursor.position();
        _textEdit->clear();
        QString txt = variant.toString();
        _textEdit->setPlainText(txt);
        cursor.setPosition(pos);
        _textEdit->setTextCursor(cursor);
        QObject::connect(_textEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
    } else if (strKnob->isLabel()) {
        assert(_label);
        QString txt = variant.toString();
        _label->setText(txt);
    } else {
        assert(_lineEdit);
        _lineEdit->setText(variant.toString());
    }
}
void String_KnobGui::_hide()
{
    boost::shared_ptr<String_Knob> strKnob = boost::dynamic_pointer_cast<String_Knob>(getKnob());
    assert(strKnob);

    _descriptionLabel->hide();
    if (strKnob->isMultiLine()) {
        assert(_textEdit);
        _textEdit->hide();
    } else if (strKnob->isLabel()) {
        assert(_label);
        _label->hide();
    } else {
        assert(_lineEdit);
        _lineEdit->hide();

    }
}

void String_KnobGui::_show()
{
    boost::shared_ptr<String_Knob> strKnob = boost::dynamic_pointer_cast<String_Knob>(getKnob());
    assert(strKnob);

    _descriptionLabel->show();
    if (strKnob->isMultiLine()) {
        assert(_textEdit);
        _textEdit->show();
    } else if (strKnob->isLabel()) {
        assert(_label);
        _label->show();
    } else {
        assert(_lineEdit);
        _lineEdit->show();
    }
}
void String_KnobGui::setEnabled()
{
    bool b = getKnob()->isEnabled(0);
    boost::shared_ptr<String_Knob> strKnob = boost::dynamic_pointer_cast<String_Knob>(getKnob());
    assert(strKnob);

    _descriptionLabel->setEnabled(b);
    if (strKnob->isMultiLine()) {
        assert(_textEdit);
        //_textEdit->setEnabled(b);
        _textEdit->setReadOnly(!b);
    } else if (strKnob->isLabel()) {
        assert(_label);
        _label->setEnabled(b);
    } else {
        assert(_lineEdit);
        //_lineEdit->setEnabled(b);
        _lineEdit->setReadOnly(!b);
    }
 }

void String_KnobGui::reflectAnimationLevel(int /*dimension*/,Natron::AnimationLevel level)
{
    boost::shared_ptr<String_Knob> strKnob = boost::dynamic_pointer_cast<String_Knob>(getKnob());
    assert(strKnob);

    switch (level) {
        case Natron::NO_ANIMATION:
            if (strKnob->isMultiLine()) {
                assert(_textEdit);
                _textEdit->setAnimation(0);
            } else if (strKnob->isLabel()) {
                assert(_label);
            } else {
                assert(_lineEdit);
                _lineEdit->setAnimation(0);
            }
            break;
        case Natron::INTERPOLATED_VALUE:
            if (strKnob->isMultiLine()) {
                assert(_textEdit);
                _textEdit->setAnimation(1);
            } else if (strKnob->isLabel()) {
                assert(_label);
            } else {
                assert(_lineEdit);
                _lineEdit->setAnimation(1);
            }
            break;
        case Natron::ON_KEYFRAME:
            if (strKnob->isMultiLine()) {
                assert(_textEdit);
                _textEdit->setAnimation(2);
            } else if (strKnob->isLabel()) {
                assert(_label);
            } else {
                assert(_lineEdit);
                _lineEdit->setAnimation(2);
            }
            break;
        default:
            break;
    }
}

void String_KnobGui::setReadOnly(bool readOnly, int /*dimension*/) {
    if (_textEdit) {
        _textEdit->setReadOnlyNatron(readOnly);
    } else {
        assert(_lineEdit);
        _lineEdit->setReadOnly(readOnly);
    }
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
    if(hasToolTip()) {
        _button->setToolTip(toolTip());
    }
    _button->setChecked(_checked);
    QWidget *header = new QWidget(layout->parentWidget());
    QHBoxLayout *headerLay = new QHBoxLayout(header);
    header->setLayout(headerLay);
    QObject::connect(_button, SIGNAL(checked(bool)), this, SLOT(setChecked(bool)));
    headerLay->addWidget(_button);
    headerLay->setSpacing(1);
    _descriptionLabel = new QLabel(QString(QString(getKnob()->getDescription().c_str()) + ":"), layout->parentWidget());
    if(hasToolTip()) {
        _descriptionLabel->setToolTip(toolTip());
    }
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

bool Group_KnobGui::eventFilter(QObject */*target*/, QEvent */*event*/) {
    //if(event->type() == QEvent::Paint){
        ///discard the paint event
        return true;
    // }
    //return QObject::eventFilter(target, event);
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
    
    if (_checked) {
        for (U32 i = 0 ; i < _children.size() ; ++i) {
            _children[i].first->show();
        }
    }
    
}

void Group_KnobGui::setEnabled() {
    
    if (getKnob()->isEnabled(0)) {
        for (U32 i = 0; i < _childrenToEnable.size(); ++i) {
            for (U32 j = 0; j < _childrenToEnable[i].second.size(); ++j) {
                _childrenToEnable[i].first->getKnob()->setEnabled(_childrenToEnable[i].second[j], true);
            }
        }
    } else {
        _childrenToEnable.clear();
        for (U32 i = 0; i < _children.size(); ++i) {
            std::vector<int> dimensions;
            for (int j = 0; j < _children[i].first->getKnob()->getDimension();++j) {
                if (_children[i].first->getKnob()->isEnabled(j)) {
                    _children[i].first->getKnob()->setEnabled(j, false);
                    dimensions.push_back(j);
                }
            }
            _childrenToEnable.push_back(std::make_pair(_children[i].first, dimensions));

        }
    }
}


//=============================Parametric_KnobGui===================================

Parametric_KnobGui::Parametric_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
: KnobGui(knob, container)
, _curveWidget(NULL)
, _tree(NULL)
, _resetButton(NULL)
, _curves()
{
    
}

Parametric_KnobGui::~Parametric_KnobGui(){
    delete _curveWidget;
    delete _tree;
}

void Parametric_KnobGui::createWidget(QGridLayout *layout, int row) {
    
    boost::shared_ptr<Parametric_Knob> parametricKnob = boost::dynamic_pointer_cast<Parametric_Knob>(getKnob());
    
    QObject::connect(parametricKnob.get(), SIGNAL(curveChanged(int)), this, SLOT(onCurveChanged(int)));
    
    QWidget* treeColumn = new QWidget(layout->parentWidget());
    QVBoxLayout* treeColumnLayout = new QVBoxLayout(treeColumn);
    treeColumnLayout->setContentsMargins(0, 0, 0, 0);
    
    _tree = new QTreeWidget(layout->parentWidget());
    _tree->setSelectionMode(QAbstractItemView::ContiguousSelection);
    _tree->setColumnCount(1);
    _tree->header()->close();
    if(hasToolTip()) {
        _tree->setToolTip(toolTip());
    }
    treeColumnLayout->addWidget(_tree);
    
    _resetButton = new Button("Reset",treeColumn);
    _resetButton->setToolTip(Qt::convertFromPlainText("Reset the selected curves in the tree to their default shape", Qt::WhiteSpaceNormal));
    QObject::connect(_resetButton, SIGNAL(clicked()), this, SLOT(resetSelectedCurves()));
    treeColumnLayout->addWidget(_resetButton);
    
    layout->addWidget(treeColumn, row, 0);
    
    _curveWidget = new CurveWidget(boost::shared_ptr<TimeLine>(),layout->parentWidget());
    if(hasToolTip()) {
        _curveWidget->setToolTip(toolTip());
    }
    layout->addWidget(_curveWidget,row,1);
    
    
    std::vector<CurveGui*> visibleCurves;
    for (int i = 0; i < getKnob()->getDimension(); ++i) {
        QString curveName = parametricKnob->getDimensionName(i).c_str();
        CurveGui* curve =  _curveWidget->createCurve(parametricKnob->getParametricCurve(i),
                                                     this,
                                                     i,
                                                     curveName);
        QColor color;
        double r,g,b;
        parametricKnob->getCurveColor(i, &r, &g, &b);
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
        _curves.insert(std::make_pair(i, desc));
        if(curve->isVisible()){
            visibleCurves.push_back(curve);
        }
    }
    
    _curveWidget->centerOn(visibleCurves);
    QObject::connect(_tree, SIGNAL(itemSelectionChanged()),this,SLOT(onItemsSelectionChanged()));

}


void Parametric_KnobGui::_hide() {
    _curveWidget->hide();
    _tree->hide();
    _resetButton->hide();
}

void Parametric_KnobGui::_show() {
    _curveWidget->show();
    _tree->show();
    _resetButton->show();
}

void Parametric_KnobGui::setEnabled() {
    bool b = getKnob()->isEnabled(0);
    _tree->setEnabled(b);
}


void Parametric_KnobGui::updateGUI(int /*dimension*/, const Variant &/*variant*/) {
    _curveWidget->update();
}

void Parametric_KnobGui::onCurveChanged(int dimension){
    CurveGuis::iterator it = _curves.find(dimension);
    assert(it != _curves.end());
    
    if(it->second.curve->getInternalCurve()->keyFramesCount() > 1 && it->second.treeItem->isSelected()){
        it->second.curve->setVisible(true);
    }else{
        it->second.curve->setVisible(false);
    }
    _curveWidget->update();
}

void Parametric_KnobGui::onItemsSelectionChanged(){
    std::vector<CurveGui*> curves;
    QList<QTreeWidgetItem*> selectedItems = _tree->selectedItems();
    for(int i = 0 ; i < selectedItems.size();++i){
        for (CurveGuis::iterator it = _curves.begin(); it!= _curves.end(); ++it) {
            if(it->second.treeItem == selectedItems.at(i)){
                if(it->second.curve->getInternalCurve()->isAnimated()){
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

void Parametric_KnobGui::resetSelectedCurves(){
    QVector<int> curveIndexes;
    QList<QTreeWidgetItem*> selected = _tree->selectedItems();
    for(int i = 0 ; i < selected.size();++i){
        //find the items in the curves
        for (CurveGuis::iterator it = _curves.begin(); it!= _curves.end(); ++it) {
            if(it->second.treeItem == selected.at(i)){
                curveIndexes.push_back(it->second.curve->getDimension());
                break;
            }
        }
    }
    boost::dynamic_pointer_cast<Parametric_Knob>(getKnob())->resetToDefault(curveIndexes);
}
