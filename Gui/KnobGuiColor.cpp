/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "KnobGuiColor.h"

#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

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
#include "Engine/ViewIdx.h"

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


NATRON_NAMESPACE_ENTER;
using std::make_pair;

//=============================RGBA_KNOB_GUI===================================

KnobGuiColor::KnobGuiColor(KnobPtr knob,
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
    boost::shared_ptr<KnobColor> ck = boost::dynamic_pointer_cast<KnobColor>(knob);
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

KnobGuiColor::~KnobGuiColor()
{
   
}

void KnobGuiColor::removeSpecificGui()
{
   delete mainContainer;
}

void
KnobGuiColor::createWidget(QHBoxLayout* layout)
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
    
    _rBox = new KnobSpinBox(boxContainers, SpinBox::eSpinBoxTypeDouble, this, 0);
    {
        NumericKnobValidator* validator = new NumericKnobValidator(_rBox,this);
        _rBox->setValidator(validator);
    }
    
    QObject::connect( _rBox, SIGNAL( valueChanged(double) ), this, SLOT( onSpinBoxValueChanged() ) );
    
    if (_dimension >= 3) {
        _gBox = new KnobSpinBox(boxContainers, SpinBox::eSpinBoxTypeDouble, this, 1);
        QObject::connect( _gBox, SIGNAL( valueChanged(double) ), this, SLOT( onSpinBoxValueChanged() ) );
        {
            NumericKnobValidator* validator = new NumericKnobValidator(_gBox,this);
            _gBox->setValidator(validator);
        }
        
        _bBox = new KnobSpinBox(boxContainers, SpinBox::eSpinBoxTypeDouble, this, 2);
        QObject::connect( _bBox, SIGNAL( valueChanged(double) ), this, SLOT( onSpinBoxValueChanged() ) );
        
        {
            NumericKnobValidator* validator = new NumericKnobValidator(_bBox,this);
            _bBox->setValidator(validator);
        }
    }
    if (_dimension >= 4) {
        _aBox = new KnobSpinBox(boxContainers, SpinBox::eSpinBoxTypeDouble, this, 3);
        QObject::connect( _aBox, SIGNAL( valueChanged(double) ), this, SLOT( onSpinBoxValueChanged() ) );
        {
            NumericKnobValidator* validator = new NumericKnobValidator(_aBox,this);
            _aBox->setValidator(validator);
        }
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
    boost::shared_ptr<KnobColor> knob = _knob.lock();
    
    std::string dimLabel = knob->getDimensionName(0);
    if (!dimLabel.empty()) {
        dimLabel.append(":");
    }
    _rLabel = new Label(QString(dimLabel.c_str()).toLower(), boxContainers);
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
        
        _gLabel = new Label(QString(dimLabel.c_str()).toLower(), boxContainers);
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
        
        _bLabel = new Label(QString(dimLabel.c_str()).toLower(), boxContainers);
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
        
        _aLabel = new Label(QString(dimLabel.c_str()).toLower(), boxContainers);
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
    _slider = new ScaleSliderQWidget(slidermin, slidermax, knob->getValue(0),
                                     ScaleSliderQWidget::eDataTypeDouble, getGui(), eScaleTypeLinear, boxContainers);
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
        _colorLabel->setToolTip(GuiUtils::convertFromPlainText(tr("To pick a color on a viewer, click this and then press control + left click on any viewer.\n"
                                                                "You can also pick the average color of a given rectangle by holding control + shift + left click\n. "
                                                                "To deselect the picker left click anywhere."
                                                                "Note that by default %1 converts to linear the color picked\n"
                                                                "because all the processing pipeline is linear, but you can turn this off in the\n"
                                                                "preferences panel.").arg(NATRON_APPLICATION_NAME), Qt::WhiteSpaceNormal) );
    }

    QSize medSize(TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE));
    QSize medIconSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE));

    _colorLabel->setFixedSize(medSize);
    QObject::connect( _colorLabel,SIGNAL( pickingEnabled(bool) ),this,SLOT( onPickingEnabled(bool) ) );
    colorLayout->addWidget(_colorLabel);
    
    if (knob->isSimplified()) {
        colorLayout->addSpacing(5);
    }
    
    QPixmap buttonPix;


    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, NATRON_MEDIUM_BUTTON_ICON_SIZE, &buttonPix);
    _colorDialogButton = new Button(QIcon(buttonPix), "", colorContainer);
    _colorDialogButton->setFixedSize(medSize);
    _colorDialogButton->setIconSize(medIconSize);
    _colorDialogButton->setToolTip(GuiUtils::convertFromPlainText(tr("Open the color dialog."), Qt::WhiteSpaceNormal));
    _colorDialogButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _colorDialogButton, SIGNAL( clicked() ), this, SLOT( showColorDialog() ) );
    colorLayout->addWidget(_colorDialogButton);
    
    bool enableAllDimensions = false;
    
    _dimensionSwitchButton = new Button(QIcon(),QString::number(_dimension),colorContainer);
    _dimensionSwitchButton->setFixedSize(medSize);
    _dimensionSwitchButton->setIconSize(medIconSize);
    _dimensionSwitchButton->setToolTip(GuiUtils::convertFromPlainText(tr("Switch between a single value for all dimensions and multiple values."), Qt::WhiteSpaceNormal));
    _dimensionSwitchButton->setFocusPolicy(Qt::NoFocus);
    _dimensionSwitchButton->setCheckable(true);
    
    double firstDimensionValue = knob->getValue(0);
    if (_dimension > 1) {
        if (knob->getValue(1) != firstDimensionValue) {
            enableAllDimensions = true;
        }
        if (knob->getValue(2) != firstDimensionValue) {
            enableAllDimensions = true;
        }
    }
    if (_dimension > 3) {
        if (knob->getValue(3) != firstDimensionValue) {
            enableAllDimensions = true;
        }
    }
    
    
    _dimensionSwitchButton->setChecked(enableAllDimensions);
    if (!enableAllDimensions) {
        _rBox->setValue( knob->getValue(0) );
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
        enableRightClickMenu(_colorLabel,-1);
    }
    
} // createWidget

void
KnobGuiColor::onMustShowAllDimension()
{
    _dimensionSwitchButton->setChecked(true);
    onDimensionSwitchClicked();
}

void
KnobGuiColor::onSliderValueChanged(double v)
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
    onColorChangedInternal();
}

void
KnobGuiColor::onSliderEditingFinished(bool hasMovedOnce)
{
    assert(_knob.lock()->isEnabled(0));
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    bool onEditingFinishedOnly = settings->getRenderOnEditingFinishedOnly();
    bool autoProxyEnabled = settings->isAutoProxyEnabled();
    if (onEditingFinishedOnly) {
        double v = _slider->getPosition();
        onSliderValueChanged(v);
    } else if (autoProxyEnabled && hasMovedOnce) {
        getGui()->renderAllViewers(true);
    }
}

void
KnobGuiColor::expandAllDimensions()
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
KnobGuiColor::foldAllDimensions()
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
KnobGuiColor::onDimensionSwitchClicked()
{
    boost::shared_ptr<KnobColor> knob = _knob.lock();
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
                knob->setValues(value, value, value, ViewSpec::all(), eValueChangedReasonNatronGuiEdited);
            } else {
                knob->setValues(value, value, value,value, ViewSpec::all(), eValueChangedReasonNatronGuiEdited);
            }
        }
    }
}

void
KnobGuiColor::onMinMaxChanged(double mini,
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
KnobGuiColor::onDisplayMinMaxChanged(double mini,
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

bool
KnobGuiColor::getAllDimensionsVisible() const
{
    return !_dimensionSwitchButton || _dimensionSwitchButton->isChecked();
}

int
KnobGuiColor::getDimensionForSpinBox(const SpinBox* spinbox) const
{
    if (_rBox == spinbox) {
        return 0;
    }
    if (_gBox == spinbox) {
        return 1;
    }
    if (_bBox == spinbox) {
        return 2;
    }
    if (_aBox == spinbox) {
        return 3;
    }
    return -1;
}

void
KnobGuiColor::setEnabled()
{
    boost::shared_ptr<KnobColor> knob = _knob.lock();
    bool r = knob->isEnabled(0)  && !knob->isSlave(0);
    bool enabled0 = r && knob->getExpression(0).empty();

    //_rBox->setEnabled(r);
    _rBox->setReadOnly(!r);
    _rLabel->setEnabled(r);

    _slider->setReadOnly(!enabled0);
    _colorDialogButton->setEnabled(enabled0);
    
    if (_dimension >= 3) {
        
        bool g = knob->isEnabled(1) && !knob->isSlave(1);
        bool b = knob->isEnabled(2) && !knob->isSlave(2);
        //_gBox->setEnabled(g);
        _gBox->setReadOnly(!g);
        _gLabel->setEnabled(g);
        //_bBox->setEnabled(b);
        _bBox->setReadOnly(!b);
        _bLabel->setEnabled(b);
    }
    if (_dimension >= 4) {
        bool a = knob->isEnabled(3) && !knob->isSlave(3);
        //_aBox->setEnabled(a);
        _aBox->setReadOnly(!a);
        _aLabel->setEnabled(a);
    }
    _colorLabel->setEnabledMode(enabled0);
}

void
KnobGuiColor::updateGUI(int dimension)
{
    boost::shared_ptr<KnobColor> knob = _knob.lock();
    if (knob->isSimplified()) {
        double r = 0.,g = 0.,b = 0.,a = 1.;
        r = knob->getValue(0);
        if (_dimension > 1) {
            g = knob->getValue(1);
            b = knob->getValue(2);
            if (_dimension > 3) {
                a = knob->getValue(3);
            }
        }
        updateLabel(r, g, b, a);
    } else {
        const int knobDim = _dimension;
        if (knobDim < 1 || dimension >= knobDim) {
            return;
        }
        assert(1 <= knobDim && knobDim <= 4);
        assert(dimension == -1 || (0 <= dimension && dimension < knobDim));
        double values[4];
        double refValue = 0.;
        for (int i = 0; i < knobDim; ++i) {
            values[i] = knob->getValue(i);
        }
        if (dimension == -1) {
            refValue = values[0];
        } else {
            refValue = values[dimension];
        }
        bool allValuesNotEqual = false;
        for (int i = 0; i < knobDim; ++i) {
            if (values[i] != refValue) {
                allValuesNotEqual = true;
            }
        }
        if (!knob->areAllDimensionsEnabled() && allValuesNotEqual) {
            expandAllDimensions();
        } else if (knob->areAllDimensionsEnabled() && !allValuesNotEqual) {
            foldAllDimensions();
        }
        
        switch (dimension) {
            case 0: {
                assert(knobDim >= 1);
                _rBox->setValue(values[0]);
                _slider->seekScalePosition(values[0]);
                if ( !knob->areAllDimensionsEnabled() ) {
                    _gBox->setValue(values[0]);
                    _bBox->setValue(values[0]);
                    if (_dimension >= 4) {
                        _aBox->setValue(values[0]);
                    }
                }
                break;
            }
            case 1:
                assert(knobDim >= 2);
                _gBox->setValue(values[1]);
                break;
            case 2:
                assert(knobDim >= 3);
                _bBox->setValue(values[2]);
                break;
            case 3:
                assert(knobDim >= 4);
                _aBox->setValue(values[3]);
                break;
            case -1:
                if ( !knob->areAllDimensionsEnabled() ) {
                    _rBox->setValue(values[0]);
                    _slider->seekScalePosition(values[0]);
                    if ( !knob->areAllDimensionsEnabled() ) {
                        _gBox->setValue(values[0]);
                        _bBox->setValue(values[0]);
                        if (_dimension >= 4) {
                            _aBox->setValue(values[0]);
                        }
                    }
                } else {
                    _rBox->setValue(values[0]);
                    if (knobDim > 1) {
                        _gBox->setValue(values[1]);
                        if (knobDim > 2) {
                            _bBox->setValue(values[2]);
                            if (knobDim > 3) {
                                _aBox->setValue(values[3]);
                            }
                        }
                    }
                }
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
KnobGuiColor::reflectAnimationLevel(int dimension,
                                     AnimationLevelEnum level)
{
    if (_knob.lock()->isSimplified()) {
        return;
    }
    switch (level) {
        case eAnimationLevelNone: {
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
        case eAnimationLevelInterpolatedValue: {
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
        case eAnimationLevelOnKeyframe: {
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
KnobGuiColor::reflectExpressionState(int dimension,
                                      bool hasExpr)
{
    //bool isEnabled = _knob.lock()->isEnabled(dimension);
    switch (dimension) {
        case 0:
            _rBox->setAnimation(3);
            //_rBox->setReadOnly(hasExpr || !isEnabled);
            break;
        case 1:
            _gBox->setAnimation(3);
            //_gBox->setReadOnly(hasExpr || !isEnabled);
            break;
        case 2:
            _bBox->setAnimation(3);
            //_bBox->setReadOnly(hasExpr || !isEnabled);
            break;
        case 3:
            _aBox->setAnimation(3);
            //_aBox->setReadOnly(hasExpr || !isEnabled);
            break;
        default:
            break;
    }
    if (_slider) {
        _slider->setReadOnly(hasExpr);
    }
}

void
KnobGuiColor::updateToolTip()
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
KnobGuiColor::showColorDialog()
{
    QColorDialog dialog( _rBox->parentWidget() );

    boost::shared_ptr<KnobColor> knob = _knob.lock();
    double curR = knob->getValue(0);
    _lastColor[0] = curR;
    double curG = curR;
    double curB = curR;
    double curA = 1.;
    if (_dimension > 1) {
        curG = knob->getValue(1);
        _lastColor[1] =  curG;
        curB = knob->getValue(2);
        _lastColor[2] = curB;
    }
    if (_dimension > 3) {
        curA = knob->getValue(3);
        _lastColor[3] = curA;
    }
    
    bool isSimple = knob->isSimplified();

    QColor curColor;
    curColor.setRgbF(Image::clamp<qreal>(isSimple ? curR : Color::to_func_srgb(curR), 0., 1.),
                     Image::clamp<qreal>(isSimple ? curG : Color::to_func_srgb(curG), 0., 1.),
                     Image::clamp<qreal>(isSimple ? curB : Color::to_func_srgb(curB), 0., 1.),
                     Image::clamp<qreal>(curA, 0., 1.));
    dialog.setCurrentColor(curColor);
    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onDialogCurrentColorChanged(QColor) ) );
    if (!dialog.exec()) {
        
        knob->beginChanges();

        for (int i = 0; i < _dimension; ++i) {
            knob->setValue(_lastColor[i], ViewSpec::all(), i);
        }
        knob->endChanges();

    } else {
        
        knob->beginChanges();

        ///refresh the last value so that the undo command retrieves the value that was prior to opening the dialog
        for (int i = 0; i < _dimension; ++i) {
            knob->setValue(_lastColor[i], ViewSpec::all(), i);
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
            _rBox->setValue(isSimple ? realColor.redF() : Color::from_func_srgb(realColor.redF()));
        }

        if (_dimension >= 3) {
            realColor.setGreen(userColor.green());
            realColor.setBlue(userColor.blue());
            if (getKnob()->isEnabled(1)) {
                _gBox->setValue(isSimple ? realColor.greenF() : Color::from_func_srgb(userColor.greenF()));
            }
            if (getKnob()->isEnabled(2)) {
                _bBox->setValue(isSimple ? realColor.blueF() : Color::from_func_srgb(userColor.blueF()));
            }
           
        }
        if (_dimension >= 4) {
            //            realColor.setAlpha(userColor.alpha());
            ///Don't set alpha since the color dialog can only handle RGB
//            if (getKnob()->isEnabled(3)) {
//                _aBox->setValue(userColor.alphaF()); // no conversion, alpha is linear
//            }
        }

        onColorChangedInternal();
        
        knob->endChanges();

    }
    knob->evaluateValueChange(0, knob->getCurrentTime(), ViewIdx(0), eValueChangedReasonNatronGuiEdited);
} // showColorDialog

void
KnobGuiColor::onDialogCurrentColorChanged(const QColor & color)
{
    boost::shared_ptr<KnobColor> knob = _knob.lock();
    bool isSimple = knob->isSimplified();
    if (_dimension == 1) {
        knob->setValue(color.redF(), ViewSpec::all(), 0);
    } else if (_dimension == 3) {
         ///Don't set alpha since the color dialog can only handle RGB
        knob->setValues(isSimple ? color.redF() : Color::from_func_srgb(color.redF()),
                        isSimple ? color.greenF() : Color::from_func_srgb(color.greenF()),
                        isSimple ? color.blueF() : Color::from_func_srgb(color.blueF()),
                        ViewSpec::all(),
                        eValueChangedReasonNatronInternalEdited);
    } else if (_dimension == 4) {
        knob->setValues(isSimple ? color.redF() : Color::from_func_srgb(color.redF()),
                        isSimple ? color.greenF() : Color::from_func_srgb(color.greenF()),
                        isSimple ? color.blueF() : Color::from_func_srgb(color.blueF()),
                        1.,
                        ViewSpec::all(),
                        eValueChangedReasonNatronInternalEdited);
    }

}

void
KnobGuiColor::onSpinBoxValueChanged()
{
    SpinBox* isSpinbox = qobject_cast<SpinBox*>(sender());
    int spinboxdim = -1;
    if (!isSpinbox) {
        return;
    }
    if (isSpinbox == _rBox) {
        spinboxdim = 0;
    } else if (isSpinbox == _gBox) {
        spinboxdim = 1;
    } else if (isSpinbox == _bBox) {
        spinboxdim = 2;
    } else if (isSpinbox == _aBox) {
        spinboxdim = 3;
    }
    
    double newValue = 0;
    double oldValue = 0;
    if (!_dimensionSwitchButton || _dimensionSwitchButton->isChecked() ) {
        // each spinbox has a different value
        assert(spinboxdim != -1);
        newValue = isSpinbox->value();
        oldValue = _knob.lock()->getValue(spinboxdim);
        pushUndoCommand( new KnobUndoCommand<double>(this,oldValue, newValue, spinboxdim ,false) );
    } else {
        // use the value of the first dimension only, and set all spinboxes
        newValue = _rBox->value();
        std::list<double> oldValues = _knob.lock()->getValueForEachDimension_mt_safe();
        assert(!oldValues.empty());
        oldValue = oldValues.front();
        std::list<double> newValues;
        newValues.push_back(newValue);
        if (_gBox) {
            _gBox->setValue(newValue);
            newValues.push_back(newValue);
        }
        if (_bBox) {
            _bBox->setValue(newValue);
            newValues.push_back(newValue);
        }
        if (_aBox) {
            _aBox->setValue(newValue);
            newValues.push_back(newValue);
        }
        
        pushUndoCommand( new KnobUndoCommand<double>(this,oldValues, newValues ,false) );

    }
    if (_slider) {
        _slider->seekScalePosition(newValue);
    }
}

void
KnobGuiColor::onColorChangedInternal()
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
    } else {
        if (_slider) {
            _slider->seekScalePosition(r);
        }
        if (_dimension >= 3) {
            _gBox->setValue(g);
            _bBox->setValue(b);
            if (_dimension >= 4) {
                _aBox->setValue(a);
            }
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
KnobGuiColor::updateLabel(double r, double g, double b, double a)
{
    QColor color;
    boost::shared_ptr<KnobColor> knob = _knob.lock();
    bool simple = knob->isSimplified();
    color.setRgbF(Image::clamp<qreal>(simple ? r : Color::to_func_srgb(r), 0., 1.),
                  Image::clamp<qreal>(simple ? g : Color::to_func_srgb(g), 0., 1.),
                  Image::clamp<qreal>(simple ? b : Color::to_func_srgb(b), 0., 1.),
                  Image::clamp<qreal>(a, 0., 1.));
    _colorLabel->setColor(color);
}

void
KnobGuiColor::_hide()
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
KnobGuiColor::_show()
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

ColorPickerLabel::ColorPickerLabel(KnobGuiColor* knob,QWidget* parent)
    : Label(parent)
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
            _knob->getGui()->removeColorPicker(boost::dynamic_pointer_cast<KnobColor>(_knob->getKnob()));
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
        appPTR->getIcon(NATRON_PIXMAP_COLOR_PICKER, &pickerIcon);
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
KnobGuiColor::setPickingEnabled(bool enabled)
{
    if (_colorLabel->isPickingEnabled() == enabled) {
        return;
    }
    _colorLabel->setPickingEnabled(enabled);
    onPickingEnabled(enabled);
}

void
KnobGuiColor::onPickingEnabled(bool enabled)
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
KnobGuiColor::setReadOnly(bool readOnly,
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
KnobGuiColor::setDirty(bool dirty)
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

KnobPtr KnobGuiColor::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiColor::reflectModificationsState()
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

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGuiColor.cpp"
