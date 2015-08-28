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

#include "Color_KnobGui.h"

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

