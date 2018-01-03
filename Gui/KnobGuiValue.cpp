/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "KnobGuiValue.h"

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
#include <QtCore/QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/KnobUndoCommand.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

#include "Gui/ActionShortcuts.h"
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
#include "Gui/Label.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/SpinBoxValidator.h"
#include "Gui/TabGroup.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/PreferencesPanel.h"


#include <ofxNatron.h>

NATRON_NAMESPACE_ENTER

static bool shouldSliderBeVisible(double sliderMin,
                                  double sliderMax)
{
    return (sliderMax > sliderMin) && ( (sliderMax - sliderMin) < SLIDER_MAX_RANGE ) && (sliderMax < DBL_MAX) && (sliderMin > -DBL_MAX);
}


struct KnobGuiValuePrivate
{
    KnobGuiValue *publicInterface;
    KnobIWPtr knob;
    boost::weak_ptr<KnobIntBase > intKnob;
    boost::weak_ptr<KnobDoubleBase > doubleKnob;


    struct DimensionWidgets
    {
        SpinBox* box;
        ClickableLabel* label;
        QWidget* container;
        QHBoxLayout* layout;

        DimensionWidgets()
        : box(0)
        , label(0)
        , container(0)
        , layout(0)
        {

        }
    };
    std::vector<DimensionWidgets> spinBoxes;
    QWidget *container;
    ScaleSliderQWidget *slider;
    Button* rectangleFormatButton;
    Button *dimensionSwitchButton;
    bool rectangleFormatIsWidthHeight;


    KnobGuiValuePrivate(KnobGuiValue *publicInterface, const KnobIPtr& knob)
    : publicInterface(publicInterface)
    , knob(knob)
    , intKnob( boost::dynamic_pointer_cast<KnobIntBase >(knob) )
    , doubleKnob( toKnobDoubleBase(knob) )
    , spinBoxes()
    , container(0)
    , slider(0)
    , rectangleFormatButton(0)
    , dimensionSwitchButton(0)
    , rectangleFormatIsWidthHeight(true)
    {

    }

    KnobIntBasePtr getKnobAsInt() const
    {
        return intKnob.lock();
    }

    KnobDoubleBasePtr getKnobAsDouble() const
    {
        return doubleKnob.lock();
    }

    KnobIPtr getKnob() const
    {
        return knob.lock();
    }

    double getKnobValue(DimIdx dimension) const
    {
        double value;
        KnobDoubleBasePtr k = getKnobAsDouble();
        KnobIntBasePtr i = getKnobAsInt();

        const bool clampToMinMax = false;
        if (k) {
            value = k->getValue(dimension, publicInterface->getView(), clampToMinMax);
        } else if (i) {
            value = (double)i->getValue(dimension, publicInterface->getView(), clampToMinMax);
        } else {
            value = 0;
        }

        return value;
    }
};


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
double
KnobGuiValue::valueAccordingToType(const bool doNormalize,
                                   const DimIdx dimension,
                                   const double value)
{
    if ( (dimension != 0) && (dimension != 1) ) {
        return value;
    }

    ValueIsNormalizedEnum state = getNormalizationPolicy(dimension);

    if (state != eValueIsNormalizedNone) {
        KnobIPtr knob = _imp->getKnob();
        if (knob) {
            TimeValue time = knob->getCurrentRenderTime();
            if (doNormalize) {
                return normalize(dimension, time, value);
            } else {
                return denormalize(dimension, time, value);
            }
        }
    }

    return value;
}

bool
KnobGuiValue::shouldAddStretch() const
{
    return true; //isSliderDisabled();
}

KnobGuiValue::KnobGuiValue(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _imp( new KnobGuiValuePrivate(this, knob->getKnob()) )
{
    KnobIPtr internalKnob = _imp->knob.lock();
    boost::shared_ptr<KnobSignalSlotHandler> handler = internalKnob->getSignalSlotHandler();

    if (handler) {
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        QObject::connect( handler.get(), SIGNAL(minMaxChanged(DimSpec)), this, SLOT(onMinMaxChanged(DimSpec)) );
#endif
        QObject::connect( handler.get(), SIGNAL(displayMinMaxChanged(DimSpec)), this, SLOT(onDisplayMinMaxChanged(DimSpec)) );
    }
}

KnobGuiValue::~KnobGuiValue()
{
}

void
KnobGuiValue::createWidget(QHBoxLayout* layout)
{

    connectKnobSignalSlots();

    _imp->container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(_imp->container);
    layout->addWidget(_imp->container);

    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(TO_DPIX(3));

    KnobGuiPtr knobUI = getKnobGui();

    DimIdx singleDimension;
    bool singleDimensionEnabled = knobUI->isSingleDimensionalEnabled(&singleDimension);

    bool tooManyKnobOnLine = knobUI->getLayoutType() != KnobGui::eKnobLayoutTypeViewerUI && knobUI->getKnobsCountOnSameLine() > 1;
    if (tooManyKnobOnLine || singleDimensionEnabled) {
        disableSlider();
    }

    if ( !isSliderDisabled() ) {
        layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    KnobIPtr knob = _imp->getKnob();
    const int nDims = knob->getNDimensions();
    std::vector<double> increments, displayMins, displayMaxs, mins, maxs;
    std::vector<int> decimals;
    KnobDoubleBasePtr doubleKnob = _imp->getKnobAsDouble();
    KnobIntBasePtr intKnob = _imp->getKnobAsInt();
    if (doubleKnob) {
        displayMins = doubleKnob->getDisplayMinimums();
        displayMaxs = doubleKnob->getDisplayMaximums();
        mins = doubleKnob->getMinimums();
        maxs = doubleKnob->getMaximums();
    } else {
        const std::vector<int>& intDmins = intKnob->getDisplayMinimums();
        const std::vector<int>& intDMaxs = intKnob->getDisplayMaximums();
        const std::vector<int>& intMins = intKnob->getMinimums();
        const std::vector<int>& intMaxs = intKnob->getMaximums();
        assert( intDMaxs.size() == intDmins.size() && intDmins.size() == intMins.size() && intMins.size() == intMaxs.size() );
        displayMins.resize( intDmins.size() );
        displayMaxs.resize( intDmins.size() );
        mins.resize( intDmins.size() );
        maxs.resize( intDmins.size() );

        for (std::size_t i = 0; i < intMins.size(); ++i) {
            displayMins[i] = intDmins[i];
            displayMaxs[i] = intDMaxs[i];
            mins[i] = intMins[i];
            maxs[i] = intMaxs[i];
        }
    }

    getIncrements(&increments);
    getDecimals(&decimals);

    std::vector<std::string> dimensionLabels(nDims);
    bool isRectangleParam = isRectangleType();
    // This is a rectangle parameter
    if (isRectangleParam) {
        dimensionLabels[0] = "x";
        dimensionLabels[1] = "y";
        dimensionLabels[2] = "w";
        dimensionLabels[3] = "h";
    } else {
        for (int i = 0; i < nDims; ++i) {
            dimensionLabels[i] = knob->getDimensionName(DimIdx(i));
        }
    }

    SpinBox::SpinBoxTypeEnum type;
    if (doubleKnob) {
        type = SpinBox::eSpinBoxTypeDouble;
    } else {
        type = SpinBox::eSpinBoxTypeInt;
    }


    int nItemsPerRow = singleDimensionEnabled ? 1 : nDims;
    if (std::floor(nDims / 3. + 0.5) == nDims / 3.) {
        nItemsPerRow = 3;
    }
    if (std::floor(nDims / 4. + 0.5) == nDims / 4.) {
        nItemsPerRow = 4;
    }

    int nbRows = nDims / nItemsPerRow;
    assert(nbRows >= 1);
    QWidget* allSpinBoxesContainer = 0;
    QGridLayout* spinBoxesGrid = 0;
    if (nbRows == 1) {
        allSpinBoxesContainer = _imp->container;
    } else {
        allSpinBoxesContainer = new QWidget(_imp->container);
        spinBoxesGrid = new QGridLayout(allSpinBoxesContainer);
        spinBoxesGrid->setContentsMargins(0, 0, 0, 0);
        spinBoxesGrid->setVerticalSpacing( TO_DPIY(1) );
        spinBoxesGrid->setHorizontalSpacing( TO_DPIX(1) );
    }

    _imp->spinBoxes.resize(nDims);

    KnobGuiWidgetsPtr thisShared = shared_from_this();
    int rowIndex = 0;
    int columnIndex = 0;
    for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {

        // When the knob is a table item, ensure only 1 spinbox is displayed, or a custom widget (for KnobColor)
        if ((singleDimensionEnabled && singleDimension != (int)i) || (knobUI->getLayoutType() == KnobGui::eKnobLayoutTypeTableItemWidget && !singleDimensionEnabled)) {
            _imp->spinBoxes[i].box = 0;
            _imp->spinBoxes[i].label = 0;
            continue;
        }

        QWidget *boxContainer = new QWidget(allSpinBoxesContainer);
        boxContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        QHBoxLayout *boxContainerLayout = 0;
        boxContainerLayout = new QHBoxLayout(boxContainer);
        boxContainerLayout->setContentsMargins(0, 0, 0, 0);
        boxContainerLayout->setSpacing(TO_DPIX(3));


        ClickableLabel *subDesc = 0;
        if ( (nDims != 1) && (nbRows == 1) ) {
            subDesc = new ClickableLabel(QString::fromUtf8( dimensionLabels[i].c_str() ), boxContainer);
            boxContainerLayout->addWidget(subDesc);
        }

        SpinBox *box = new KnobSpinBox(layout->parentWidget(), type, knobUI, DimIdx(i), getView());
        box->setProperty(kKnobGuiValueSpinBoxDimensionProperty, (int)i);
        box->setAlignment(getSpinboxAlignment());
        NumericKnobValidator* validator = new NumericKnobValidator(box, thisShared, getView());
        box->setValidator(validator);
        QObject::connect( box, SIGNAL(valueChanged(double)), this, SLOT(onSpinBoxValueChanged()) );

        // Set the copy/link actions in the right click menu of the SpinBox
        KnobGuiWidgets::enableRightClickMenu(knobUI, box, DimIdx(i), getView());

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
        double min = valueAccordingToType(false, i, mins[i]);
        double max = valueAccordingToType(false, i, maxs[i]);
        box->setMaximum(max);
        box->setMinimum(min);
#endif


        if (type == SpinBox::eSpinBoxTypeDouble) {
            // Set the number of digits after the decimal point
            if ( i < decimals.size() ) {
                box->decimals(decimals[i]);
            }
        }

        // Set increment
        if ( i < increments.size() ) {
            double incr = 1;
            incr = valueAccordingToType(false, DimIdx(i), increments[i]);
            box->setIncrement(incr);
        }
        boxContainerLayout->addWidget(box);

        if (!spinBoxesGrid) {
            // If we have only a single row of widget, add it to the main container layout
            containerLayout->addWidget(boxContainer);
        } else {
            spinBoxesGrid->addWidget(boxContainer, rowIndex, columnIndex);
        }
        _imp->spinBoxes[i].box = box;
        _imp->spinBoxes[i].label = subDesc;
        _imp->spinBoxes[i].container = boxContainer;
        _imp->spinBoxes[i].layout = boxContainerLayout;

        ++columnIndex;
        if (columnIndex >= nItemsPerRow) {
            columnIndex = 0;
            ++rowIndex;
        }
    }

    if (spinBoxesGrid) {
        containerLayout->addWidget(allSpinBoxesContainer);
    }

    KnobGui::KnobLayoutTypeEnum layoutType = knobUI->getLayoutType();

    if (!isSliderDisabled() && !isRectangleParam && layoutType != KnobGui::eKnobLayoutTypeTableItemWidget) {
        double dispmin = displayMins[0];
        double dispmax = displayMaxs[0];
        if (dispmin == -DBL_MAX) {
            dispmin = mins[0];
        }
        if (dispmax == DBL_MAX) {
            dispmax = maxs[0];
        }

        // denormalize if necessary
        double dispminGui = valueAccordingToType(false, DimIdx(0), dispmin);
        double dispmaxGui = valueAccordingToType(false, DimIdx(0), dispmax);
        bool spatial = isSpatialType();
        Format f;
        if (spatial) {
            _imp->getKnob()->getHolder()->getApp()->getProject()->getProjectDefaultFormat(&f);
        }
        if (dispminGui < -SLIDER_MAX_RANGE) {
            if (spatial) {
                dispminGui = -f.width();
            } else {
                dispminGui = -SLIDER_MAX_RANGE;
            }
        }
        if (dispmaxGui > SLIDER_MAX_RANGE) {
            if (spatial) {
                dispmaxGui = f.width();
            } else {
                dispmaxGui = SLIDER_MAX_RANGE;
            }
        }

        double value0 = _imp->getKnobValue(DimIdx(0));
        ScaleSliderQWidget::DataTypeEnum sliderType;
        if (doubleKnob) {
            sliderType = ScaleSliderQWidget::eDataTypeDouble;
        } else {
            sliderType = ScaleSliderQWidget::eDataTypeInt;
        }


        _imp->slider = new ScaleSliderQWidget( dispminGui, dispmaxGui, value0, knob->getEvaluateOnChange(),
                                               sliderType, knobUI->getGui(), eScaleTypeLinear, layout->parentWidget() );
        if (knobUI->getLayoutType() == KnobGui::eKnobLayoutTypeViewerUI) {
            // When in horizontal layout, we don't want the slider to grow
             _imp->slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        } else {
            _imp->slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
        if ( knobUI->hasToolTip() ) {
            knobUI->toolTip(_imp->slider, getView());
        }
        QObject::connect( _imp->slider, SIGNAL(resetToDefaultRequested()), this, SLOT(onSliderResetToDefaultRequested()) );
        QObject::connect( _imp->slider, SIGNAL(positionChanged(double)), this, SLOT(onSliderValueChanged(double)) );
        QObject::connect( _imp->slider, SIGNAL(editingFinished(bool)), this, SLOT(onSliderEditingFinished(bool)) );
        if (spinBoxesGrid) {
            spinBoxesGrid->addWidget(_imp->slider, rowIndex, columnIndex);
        } else {
            containerLayout->addWidget(_imp->slider);
        }

        // onDisplayMinMaxChanged takes original (maybe normalized) values
        onDisplayMinMaxChanged(DimSpec::all());
    }

    QSize medSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    QSize medIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );

    if (isRectangleParam && layoutType != KnobGui::eKnobLayoutTypeTableItemWidget) {
        _imp->rectangleFormatButton = new Button(QIcon(), QString::fromUtf8("wh"), _imp->container);
        _imp->rectangleFormatButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Switch between width/height and right/top notation"), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->rectangleFormatButton->setFixedSize(medSize);
        _imp->rectangleFormatButton->setIconSize(medIconSize);
        _imp->rectangleFormatButton->setFocusPolicy(Qt::NoFocus);
        _imp->rectangleFormatButton->setCheckable(true);
        _imp->rectangleFormatButton->setChecked(_imp->rectangleFormatIsWidthHeight);
        _imp->rectangleFormatButton->setDown(_imp->rectangleFormatIsWidthHeight);
        QObject::connect( _imp->rectangleFormatButton, SIGNAL(clicked(bool)), this, SLOT(onRectangleFormatButtonClicked()) );
        containerLayout->addWidget(_imp->rectangleFormatButton);
    }

    if (nDims > 1 && !singleDimensionEnabled && layoutType != KnobGui::eKnobLayoutTypeTableItemWidget) {
        _imp->dimensionSwitchButton = new Button(QIcon(), QString::number(nDims), _imp->container);
        _imp->dimensionSwitchButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Switch between a single value for all dimensions and multiple values."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->dimensionSwitchButton->setFixedSize(medSize);
        _imp->dimensionSwitchButton->setIconSize(medIconSize);
        _imp->dimensionSwitchButton->setFocusPolicy(Qt::NoFocus);
        _imp->dimensionSwitchButton->setCheckable(true);
        QObject::connect( _imp->dimensionSwitchButton, SIGNAL(clicked(bool)), this, SLOT(onDimensionSwitchClicked(bool)) );
        containerLayout->addWidget(_imp->dimensionSwitchButton);
    }

    addExtraWidgets(containerLayout);

    updateGUI();

    // Refresh widgets state if auto-expand is not enabled
    setAllDimensionsVisible(knob->getAllDimensionsVisible(getView()));

} // createWidget

void
KnobGuiValue::onSliderResetToDefaultRequested()
{

    KnobIPtr knob = _imp->knob.lock();
    std::list<KnobIPtr> knobsList;
    knobsList.push_back(knob);

    getKnobGui()->pushUndoCommand( new RestoreDefaultsCommand(knobsList, DimSpec::all(), getView()) );
}

bool
KnobGuiValue::getSpinBox(DimIdx dim,
                         SpinBox** spinbox,
                         Label** label) const
{
    if ( dim < 0 && dim >= (int)_imp->spinBoxes.size() ) {
        return false;
    }
    DimIdx singleDim;
    bool singleEnabled = getKnobGui()->isSingleDimensionalEnabled(&singleDim);
    if (singleEnabled && dim != singleDim) {
        return false;
    }
    if (spinbox) {
        *spinbox = _imp->spinBoxes[dim].box;
    }
    if (label) {
        *label = _imp->spinBoxes[dim].label;
    }
    return true;
}

bool
KnobGuiValue::getDimForSpinBox(const SpinBox* spinbox, DimIdx* dimension) const
{
    for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
        if (_imp->spinBoxes[i].box == spinbox) {
            *dimension = DimIdx(i);
            return true;
        }
    }

    return false;
}

void
KnobGuiValue::onRectangleFormatButtonClicked()
{
    if (!_imp->rectangleFormatButton) {
        return;
    }
    _imp->rectangleFormatIsWidthHeight = !_imp->rectangleFormatIsWidthHeight;
    _imp->rectangleFormatButton->setDown(_imp->rectangleFormatIsWidthHeight);
    if (_imp->rectangleFormatIsWidthHeight) {
        if (_imp->spinBoxes[2].label) {
            _imp->spinBoxes[2].label->setText( QString::fromUtf8("w") );
        }
        if (_imp->spinBoxes[3].label) {
            _imp->spinBoxes[3].label->setText( QString::fromUtf8("h") );
        }
    } else {
        if (_imp->spinBoxes[2].label) {
            _imp->spinBoxes[2].label->setText( QString::fromUtf8("r") );
        }
        if (_imp->spinBoxes[3].label) {
            _imp->spinBoxes[3].label->setText( QString::fromUtf8("t") );
        }
    }
    updateGUI();
}

void
KnobGuiValue::setVisibleSlider(bool visible)
{
    if (_imp->slider) {
        _imp->slider->setVisible(visible);
        if (!visible) {
            getKnobGui()->addSpacerItemAtEndOfLine();
        } else {
            getKnobGui()->removeSpacerItemAtEndOfLine();
        }
    }
}

void
KnobGuiValue::setAllDimensionsVisible(bool visible)
{
    if (!_imp->dimensionSwitchButton) {
        if (_imp->spinBoxes.size() > 1) {
            setVisibleSlider(false);
        }

        return;
    }

    _imp->dimensionSwitchButton->setDown(visible);
    _imp->dimensionSwitchButton->setChecked(visible);


    onDimensionsMadeVisible(visible);

    if (visible) {
        setVisibleSlider(false);
        for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
            if (i > 0) {
                if (_imp->spinBoxes[i].box) {
                    _imp->spinBoxes[i].box->show();
                }
            }
            if (_imp->spinBoxes[i].label) {
                _imp->spinBoxes[i].label->show();
            }
        }
    } else {
        setVisibleSlider(true);

        for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
            if (i > 0) {
                if ( _imp->spinBoxes[i].box) {
                    _imp->spinBoxes[i].box->hide();
                }
            }
            if (_imp->spinBoxes[i].label) {
                _imp->spinBoxes[i].label->hide();
            }
        }

    }
}

void
KnobGuiValue::disableSpinBoxesBorder()
{
    for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
        if ( _imp->spinBoxes[i].box) {
            _imp->spinBoxes[i].box->setBorderDisabled(true);
        }
    }

}

void
KnobGuiValue::onDimensionSwitchClicked(bool clicked)
{
    // User clicked once on the dimension switch, disable auto switch
    KnobIPtr knob = getKnobGui()->getKnob();
    knob->setAllDimensionsVisible(getView(), clicked);

    // If the user clicked on the button, prevent any programatic setValue* call to undo the user operation,
    // unless the user clicked fold.
    knob->setCanAutoFoldDimensions(!clicked);
}




void
KnobGuiValue::onMinMaxChanged(DimSpec dimension)
{
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    assert( _imp->spinBoxes.size() > std::size_t(index) );
    if (_imp->spinBoxes[index].first) {
        _imp->spinBoxes[index].first->setMinimum( valueAccordingToType(false, index, mini) );
        _imp->spinBoxes[index].first->setMaximum( valueAccordingToType(false, index, maxi) );
    }
#else
    (void)dimension;
#endif
}

void
KnobGuiValue::onDisplayMinMaxChanged(DimSpec /*dimension*/)
{
    if (!getKnobGui()) {
        return;
    }
    if (!_imp->slider) {
        return;
    }
    KnobDoubleBasePtr isDouble = _imp->getKnobAsDouble();
    KnobIntBasePtr isInt = _imp->getKnobAsInt();

    double displayMin, displayMax, min, max;
    if (isDouble) {
        displayMin = isDouble->getDisplayMinimum();
        displayMax = isDouble->getDisplayMaximum();
        min = isDouble->getMinimum();
        max = isDouble->getMaximum();
    } else {
        displayMin = isInt->getDisplayMinimum();
        displayMax = isInt->getDisplayMaximum();
        min = isInt->getMinimum();
        max = isInt->getMaximum();
    }
    displayMin = valueAccordingToType(false, DimIdx(0), displayMin);
    displayMax = valueAccordingToType(false, DimIdx(0), displayMax);
    min = valueAccordingToType(false, DimIdx(0), min);
    max = valueAccordingToType(false, DimIdx(0), max);


    if ( (displayMax - displayMin) >= SLIDER_MAX_RANGE ) {
        if ( (max - min) < SLIDER_MAX_RANGE ) {
            displayMin = min;
            displayMax = max;
        }
    }

    if (displayMin < -SLIDER_MAX_RANGE) {
        if ( isSpatialType() ) {
            Format f;
            _imp->getKnob()->getHolder()->getApp()->getProject()->getProjectDefaultFormat(&f);
            displayMin = -f.width();
        } else {
            displayMin = -SLIDER_MAX_RANGE;
        }
    }
    if (displayMax > SLIDER_MAX_RANGE) {
        if ( isSpatialType() ) {
            Format f;
            _imp->getKnob()->getHolder()->getApp()->getProject()->getProjectDefaultFormat(&f);
            displayMax = f.width();
        } else {
            displayMax = SLIDER_MAX_RANGE;
        }
    }

    bool sliderVisible = shouldSliderBeVisible(displayMin, displayMax);
    setVisibleSlider(sliderVisible);

    if (displayMax > displayMin) {
        _imp->slider->setMinimumAndMaximum(displayMin, displayMax);
    }


} // KnobGuiValue::onDisplayMinMaxChanged

void
KnobGuiValue::onIncrementChanged(const double incr,
                                 DimIdx dimension)
{
    if (_imp->spinBoxes.size() > (std::size_t)dimension) {
        if (_imp->spinBoxes[dimension].box) {
            _imp->spinBoxes[dimension].box->setIncrement( valueAccordingToType(false, dimension, incr) );
        }
    }
}

void
KnobGuiValue::onDecimalsChanged(const int deci,
                                DimIdx dimension)
{
    if (_imp->spinBoxes.size() > (std::size_t)dimension) {
        if (_imp->spinBoxes[dimension].box) {
            _imp->spinBoxes[dimension].box->decimals(deci);
        }
    }
}

void
KnobGuiValue::updateGUI()
{
    KnobIPtr knob = _imp->getKnob();
    const int knobDim = (int)_imp->spinBoxes.size();

    std::vector<double> values(knobDim);
    std::vector<std::string> expressions(knobDim);
    std::string refExpresion;
    TimeValue time(0);
    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        time = knob->getHolder()->getTimelineCurrentTime();
    }

    for (int i = 0; i < knobDim; ++i) {
        double v = _imp->getKnobValue(DimIdx(i));
        if (getNormalizationPolicy(DimIdx(i)) != eValueIsNormalizedNone) {
            v = denormalize(DimIdx(i), time, v);
        }

        // For rectangle in top/right mode, convert values
        if ( (knobDim == 4) && !_imp->rectangleFormatIsWidthHeight ) {
            if (i == 2) {
                v = values[0] + v;
            } else if (i == 3) {
                v = values[1] + v;
            }
        }
        values[i] = v;

        expressions[i] = knob->getExpression(DimIdx(i), getView());
    }


    refExpresion = expressions[0];


    if (_imp->slider) {
        _imp->slider->seekScalePosition(values[0]);
    }
    if ( _imp->dimensionSwitchButton && !_imp->dimensionSwitchButton->isChecked() ) {
        for (int i = 0; i < knobDim; ++i) {
            if (_imp->spinBoxes[i].box) {
                _imp->spinBoxes[i].box->setValue(values[0]);
            }
        }
    } else {
        for (int i = 0; i < knobDim; ++i) {
            if (_imp->spinBoxes[i].box) {
                _imp->spinBoxes[i].box->setValue(values[i]);
            }
        }
    }


    updateExtraGui(values);
} // KnobGuiValue::updateGUI

void
KnobGuiValue::reflectAnimationLevel(DimIdx dimension,
                                    AnimationLevelEnum level)
{
    if (!_imp->spinBoxes[dimension].box) {
        return;
    }
    KnobIPtr knob = _imp->knob.lock();

    bool isEnabled = knob->isEnabled();
    if (_imp->slider) {
        _imp->slider->setReadOnly(!isEnabled || level == eAnimationLevelExpression);
    }

    _imp->spinBoxes[dimension].box->setReadOnly_NoFocusRect(!isEnabled || level == eAnimationLevelExpression);
    if ( level != (AnimationLevelEnum)_imp->spinBoxes[dimension].box->getAnimation() ) {
        _imp->spinBoxes[dimension].box->setAnimation((int)level);
    }

}

void
KnobGuiValue::onSliderValueChanged(const double d)
{
    assert( _imp->knob.lock()->isEnabled() );
    bool penUpOnly = appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();

    if (penUpOnly) {
        return;
    }
    sliderEditingEnd(d);
}

void
KnobGuiValue::onSliderEditingFinished(bool hasMovedOnce)
{
    assert( _imp->knob.lock()->isEnabled() );
    SettingsPtr settings = appPTR->getCurrentSettings();
    bool onEditingFinishedOnly = settings->getRenderOnEditingFinishedOnly();
    bool autoProxyEnabled = settings->isAutoProxyEnabled();
    if (onEditingFinishedOnly) {
        double v = _imp->slider->getPosition();
        sliderEditingEnd(v);
    } else if (autoProxyEnabled && hasMovedOnce) {
        getKnobGui()->getGui()->renderAllViewers();
    }
}

void
KnobGuiValue::sliderEditingEnd(double d)
{

    KnobIPtr internalKnob = _imp->getKnob();
    KnobDoubleBasePtr doubleKnob = _imp->getKnobAsDouble();
    KnobIntBasePtr intKnob = _imp->getKnobAsInt();

    if (doubleKnob) {
        int digits = std::max( 0, (int)-std::floor( std::log10( _imp->slider->increment() ) ) );
        QString str;
        str.setNum(d, 'f', digits);
        d = str.toDouble();
    }
    ViewIdx view = getView();
    std::vector<double> newValuesVec;
    if (_imp->dimensionSwitchButton) {
        for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
            if (_imp->spinBoxes[i].box) {
                _imp->spinBoxes[i].box->setValue(d);
            }
        }
        d = valueAccordingToType(true, DimIdx(0), d);

        if (doubleKnob) {
            std::vector<double> oldValues, newValues;
            for (int i = 0; i < (int)_imp->spinBoxes.size(); ++i) {
                oldValues.push_back( doubleKnob->getValue(DimIdx(i),view, false /*clampToMinmax*/) );
                newValues.push_back(d);
                newValuesVec.push_back(d);
            }
            getKnobGui()->pushUndoCommand( new KnobUndoCommand<double>(internalKnob, oldValues, newValues, view) );
        } else {
            assert(intKnob);
            std::vector<int> oldValues, newValues;
            for (int i = 0; i < (int)_imp->spinBoxes.size(); ++i) {
                oldValues.push_back( intKnob->getValue(DimIdx(i), view, false /*clampToMinmax*/) );
                newValues.push_back( (int)d );
                newValuesVec.push_back(d);
            }
            getKnobGui()->pushUndoCommand( new KnobUndoCommand<int>(internalKnob, oldValues, newValues, view) );
        }
    } else {
        if (_imp->spinBoxes[0].box) {
            _imp->spinBoxes[0].box->setValue(d);
        }
        d = valueAccordingToType(true, DimIdx(0), d);
        if (doubleKnob) {
            getKnobGui()->pushUndoCommand( new KnobUndoCommand<double>(internalKnob, doubleKnob->getValue(DimIdx(0), view, false /*clampToMinmax*/), d, DimIdx(0) ,view) );
        } else {
            assert(intKnob);
            getKnobGui()->pushUndoCommand( new KnobUndoCommand<int>(internalKnob, intKnob->getValue(DimIdx(0), view, false /*clampToMinmax*/), (int)d, DimIdx(0) ,view) );
        }
        newValuesVec.push_back(d);
    }
    //updateExtraGui(newValuesVec);
}

void
KnobGuiValue::onSpinBoxValueChanged()
{
    SpinBox* changedBox = qobject_cast<SpinBox*>( sender() );
    KnobDoubleBasePtr doubleKnob = _imp->getKnobAsDouble();
    KnobIntBasePtr intKnob = _imp->getKnobAsInt();

    // The dimension corresponding to the changed box
    DimSpec spinBoxDim = DimSpec::all();
    std::vector<double> oldValue( _imp->spinBoxes.size() );
    std::vector<double> newValue( _imp->spinBoxes.size() );

    for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
        oldValue[i] = _imp->getKnobValue(DimIdx(i));
        newValue[i] = _imp->spinBoxes[i].box ? valueAccordingToType( true, DimIdx(i), _imp->spinBoxes[i].box->value() ) : oldValue[i];
        if ( (_imp->spinBoxes.size() == 4) && !_imp->rectangleFormatIsWidthHeight ) {
            if (i == 2) {
                newValue[i] = _imp->spinBoxes[0].box ? _imp->spinBoxes[0].box->value() : oldValue[0];
            } else if (i == 3) {
                newValue[i] = _imp->spinBoxes[1].box ? _imp->spinBoxes[1].box->value() : oldValue[1];
            }
        }
        if ( (_imp->spinBoxes[i].box == changedBox) && changedBox ) {
            spinBoxDim = DimSpec(i);
        }
    }

    if ( _imp->dimensionSwitchButton && !_imp->dimensionSwitchButton->isChecked() ) {
        // use the value of the first dimension only, and set all spinboxes
        for (std::size_t i = 1; i < _imp->spinBoxes.size(); ++i) {
            if (_imp->spinBoxes[i].box && _imp->spinBoxes[i].box != changedBox) {
                _imp->spinBoxes[i].box->setValue(newValue[0]);
            }
        }

        for (std::size_t i = 1; i < _imp->spinBoxes.size(); ++i) {
            newValue[i] = newValue[0];
        }
        spinBoxDim = DimSpec::all();
    }
    KnobIPtr internalKnob = _imp->getKnob();
    KnobGuiPtr knobUI = getKnobGui();
    if (!spinBoxDim.isAll()) {
        if (doubleKnob) {
            knobUI->pushUndoCommand( new KnobUndoCommand<double>(internalKnob, oldValue[spinBoxDim], newValue[spinBoxDim], DimIdx(spinBoxDim), ViewSetSpec(getView())) );
        } else {
            knobUI->pushUndoCommand( new KnobUndoCommand<int>(internalKnob, (int)oldValue[spinBoxDim], (int)newValue[spinBoxDim], DimIdx(spinBoxDim), ViewSetSpec(getView())) );
        }
    } else {
        if (doubleKnob) {
            std::vector<double> oldValues, newValues;
            for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
                newValues.push_back(newValue[i]);
                oldValues.push_back(oldValue[i]);
            }
            knobUI->pushUndoCommand( new KnobUndoCommand<double>(internalKnob, oldValues, newValues, getView()) );
        } else {
            std::vector<int> oldValues, newValues;
            for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
                newValues.push_back( (int)newValue[i] );
                oldValues.push_back( (int)oldValue[i] );
            }
            knobUI->pushUndoCommand( new KnobUndoCommand<int>(internalKnob, oldValues, newValues, getView()) );
        }
    }

} // KnobGuiValue::onSpinBoxValueChanged

void
KnobGuiValue::setWidgetsVisible(bool visible)
{
    if (_imp->container) {
        _imp->container->setVisible(visible);
    }
}

void
KnobGuiValue::setWidgetsVisibleInternal(bool visible)
{
    for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
        if (_imp->spinBoxes[i].container) {
            _imp->spinBoxes[i].container->setVisible(visible);
        }
    }
    setVisibleSlider(visible);
    if (_imp->dimensionSwitchButton) {
        _imp->dimensionSwitchButton->setVisible(visible);
    }
    if (_imp->rectangleFormatButton) {
        _imp->rectangleFormatButton->setVisible(visible);
    }
}

void
KnobGuiValue::setEnabled(const std::vector<bool>& perDimEnabled)
{
    assert(perDimEnabled.size() == _imp->spinBoxes.size());

    for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
        if (_imp->spinBoxes[i].box) {
            _imp->spinBoxes[i].box->setReadOnly_NoFocusRect(!perDimEnabled[i]);
        }
        if (_imp->spinBoxes[i].label) {
            _imp->spinBoxes[i].label->setEnabled(perDimEnabled[i]);
        }
    }
    if (_imp->slider) {
        _imp->slider->setReadOnly( !perDimEnabled[0] );
    }

    setEnabledExtraGui(perDimEnabled[0]);
}

void
KnobGuiValue::reflectMultipleSelection(bool dirty)
{
    for (U32 i = 0; i < _imp->spinBoxes.size(); ++i) {
        if (_imp->spinBoxes[i].box) {
            _imp->spinBoxes[i].box->setIsSelectedMultipleTimes(dirty);
        }
    }
}


void
KnobGuiValue::reflectSelectionState(bool selected)
{
    for (U32 i = 0; i < _imp->spinBoxes.size(); ++i) {
        if (_imp->spinBoxes[i].box) {
            _imp->spinBoxes[i].box->setIsSelected(selected);
        }
    }

}

void
KnobGuiValue::updateToolTip()
{
    KnobGuiPtr knob = getKnobGui();
    ViewIdx view = getView();
    if ( knob->hasToolTip() ) {
        for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
            if (_imp->spinBoxes[i].box) {
                knob->toolTip(_imp->spinBoxes[i].box, view);
            }
        }
        if (_imp->slider) {
            knob->toolTip(_imp->slider, view);
        }
    }
}

void
KnobGuiValue::reflectModificationsState()
{
    bool hasModif = _imp->getKnob()->hasModifications();

    for (std::size_t i = 0; i < _imp->spinBoxes.size(); ++i) {
        if (_imp->spinBoxes[i].box) {
            _imp->spinBoxes[i].box->setIsModified(hasModif);
        }
        if (_imp->spinBoxes[i].label) {
            _imp->spinBoxes[i].label->setIsModified(hasModif);
        }
    }
    if (_imp->slider) {
        _imp->slider->setIsModified(hasModif);
    }
}

void
KnobGuiValue::reflectLinkedState(DimIdx dimension, bool linked)
{
    if (dimension < 0 || dimension >= (int)_imp->spinBoxes.size()) {
        return;
    }
    if (_imp->spinBoxes[dimension].box) {
        QColor c;
        if (linked) {
            double r,g,b;
            appPTR->getCurrentSettings()->getExprColor(&r, &g, &b);
            c.setRgbF(Image::clamp(r, 0., 1.),
                      Image::clamp(g, 0., 1.),
                      Image::clamp(b, 0., 1.));
            _imp->spinBoxes[dimension].box->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, true, c);
        } else {
            _imp->spinBoxes[dimension].box->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, false);
        }
    }

}

void
KnobGuiValue::refreshDimensionName(DimIdx dim)
{
    if ( (dim < 0) || ( dim >= (int)_imp->spinBoxes.size() ) ) {
        return;
    }
    KnobIPtr knob = _imp->getKnob();
    if (_imp->spinBoxes[dim].label) {
        _imp->spinBoxes[dim].label->setText( QString::fromUtf8( knob->getDimensionName(dim).c_str() ) );
    }
}

KnobGuiDouble::KnobGuiDouble(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiValue(knob, view)
    , _knob( boost::dynamic_pointer_cast<KnobDouble>(knob->getKnob()) )
{
}

bool
KnobGuiDouble::isSliderDisabled() const
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return false;
    }
    return knob->isSliderDisabled();
}

bool
KnobGuiDouble::isRectangleType() const
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return false;
    }
    return knob->isRectangle();
}

bool
KnobGuiDouble::isSpatialType() const
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return false;
    }
    return knob->getIsSpatial();
}

ValueIsNormalizedEnum
KnobGuiDouble::getNormalizationPolicy(DimIdx dimension) const
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return eValueIsNormalizedNone;
    }
    return knob->getValueIsNormalized(dimension);
}

double
KnobGuiDouble::denormalize(DimIdx dimension,
                           TimeValue time,
                           double value) const
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return value;
    }
    return knob->denormalize(dimension, time, value);
}

double
KnobGuiDouble::normalize(DimIdx dimension,
                         TimeValue time,
                         double value) const
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return value;
    }
    return knob->normalize(dimension, time, value);
}

void
KnobGuiDouble::connectKnobSignalSlots()
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    QObject::connect( knob.get(), SIGNAL(incrementChanged(double,DimIdx)), this, SLOT(onIncrementChanged(double,DimIdx)) );
    QObject::connect( knob.get(), SIGNAL(decimalsChanged(int,DimIdx)), this, SLOT(onDecimalsChanged(int,DimIdx)) );
}

void
KnobGuiDouble::disableSlider()
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    return knob->disableSlider();
}

void
KnobGuiDouble::getIncrements(std::vector<double>* increments) const
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    *increments = knob->getIncrements();
}

void
KnobGuiDouble::getDecimals(std::vector<int>* decimals) const
{
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    *decimals = knob->getDecimals();
}

KnobGuiInt::KnobGuiInt(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiValue(knob, view)
    , _knob( toKnobInt(knob->getKnob()) )
    , _shortcutRecorder(0)
{
}

bool
KnobGuiInt::isSliderDisabled() const
{
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return false;
    }
    return knob->isSliderDisabled();
}

bool
KnobGuiInt::isRectangleType() const
{
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return false;
    }
    return knob->isRectangle();
}

void
KnobGuiInt::connectKnobSignalSlots()
{
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    QObject::connect( knob.get(), SIGNAL(incrementChanged(double,DimIdx)), this, SLOT(onIncrementChanged(double,DimIdx)) );
}

void
KnobGuiInt::disableSlider()
{
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    return knob->disableSlider();
}

void
KnobGuiInt::getIncrements(std::vector<double>* increments) const
{
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    const std::vector<int>& incr = knob->getIncrements();

    increments->resize( incr.size() );
    for (std::size_t i = 0; i < incr.size(); ++i) {
        (*increments)[i] = (double)incr[i];
    }
}


Qt::Alignment
KnobGuiInt::getSpinboxAlignment() const 
{
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return 0;
    }
    if (knob->isValueCenteredInSpinBox()) {
        return Qt::AlignVCenter | Qt::AlignHCenter;
    } else {
        return KnobGuiValue::getSpinboxAlignment();
    }
}

void
KnobGuiInt::createWidget(QHBoxLayout* layout)
{
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if (!knob->isShortcutKnob()) {
        KnobGuiValue::createWidget(layout);
    } else {
        KnobGuiPtr knobUI = getKnobGui();
        _shortcutRecorder = new KeybindRecorder(layout->parentWidget());
        if ( knobUI->hasToolTip() ) {
            knobUI->toolTip(_shortcutRecorder, getView());
        }
        QObject::connect(_shortcutRecorder, SIGNAL(editingFinished()), this, SLOT(onKeybindRecorderEditingFinished()));
        layout->addWidget(_shortcutRecorder);
        KnobGuiWidgets::enableRightClickMenu(knobUI, _shortcutRecorder, DimSpec::all(), getView());
    }
}

void
KnobGuiInt::onKeybindRecorderEditingFinished()
{
    Qt::KeyboardModifiers qtMods;
    Qt::Key qtKey;

    {
        QString presetShortcut = _shortcutRecorder->text();
        QKeySequence keySeq(presetShortcut, QKeySequence::NativeText);
        extractKeySequence(keySeq, qtMods, qtKey);
    }
    int symbol = (int)QtEnumConvert::fromQtKey(qtKey);
    int mods = (int)QtEnumConvert::fromQtModifiers(qtMods);

    std::vector<int> newValues;
    newValues.push_back(symbol);
    newValues.push_back(mods);

    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    std::vector<int> oldValuesVec(knob->getNDimensions());
    for (int i = 0; i < knob->getNDimensions(); ++i) {
        oldValuesVec[i] = knob->getValue(DimIdx(i), getView());
    }
    getKnobGui()->pushUndoCommand(new KnobUndoCommand<int>(knob, oldValuesVec, newValues, getView()));
}

void
KnobGuiInt::updateGUI()
{
    if (!_shortcutRecorder) {
        KnobGuiValue::updateGUI();
    } else {
        KnobIntPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        assert(knob->getNDimensions() == 2);
        Key symbol = (Key)knob->getValue(DimIdx(0));
        KeyboardModifiers modifiers = (KeyboardModifiers)knob->getValue(DimIdx(1));
        QKeySequence sequence = makeKeySequence(QtEnumConvert::toQtModifiers(modifiers), QtEnumConvert::toQtKey(symbol));
        _shortcutRecorder->setText(sequence.toString(QKeySequence::NativeText));
    }
}

void
KnobGuiInt::setEnabled(const std::vector<bool>& perDimEnabled)
{
    if (!_shortcutRecorder) {
        KnobGuiValue::setEnabled(perDimEnabled);
    } else {
        _shortcutRecorder->setReadOnly_NoFocusRect(!perDimEnabled[0]);
    }
}

void
KnobGuiInt::reflectMultipleSelection(bool dirty)
{
    if (!_shortcutRecorder) {
        KnobGuiValue::reflectMultipleSelection(dirty);
    } else {
        _shortcutRecorder->setIsSelectedMultipleTimes(dirty);
    }
}


void
KnobGuiInt::reflectSelectionState(bool selected)
{
    if (!_shortcutRecorder) {
        KnobGuiValue::reflectSelectionState(selected);
    } else {
        _shortcutRecorder->setIsSelected(selected);
    }

}

void
KnobGuiInt::reflectAnimationLevel(DimIdx dimension, AnimationLevelEnum level)
{
    if (!_shortcutRecorder) {
        KnobGuiValue::reflectAnimationLevel(dimension, level);
    } else {
        KnobIntPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        bool isEnabled = knob->isEnabled();
        _shortcutRecorder->setReadOnly_NoFocusRect(level == eAnimationLevelExpression || !isEnabled);
        _shortcutRecorder->setAnimation((int)level);
    }
}

void
KnobGuiInt::updateToolTip()
{
    if (!_shortcutRecorder) {
        KnobGuiValue::updateToolTip();
    } else {
        getKnobGui()->toolTip(_shortcutRecorder, getView());
    }
}

void
KnobGuiInt::reflectModificationsState()
{
    if (!_shortcutRecorder) {
        KnobGuiValue::reflectModificationsState();
    } else {
        KnobIntPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        bool hasModif = knob->hasModifications();

        if (_shortcutRecorder) {
            _shortcutRecorder->setIsModified(hasModif);
        }
    }
}

void
KnobGuiInt::refreshDimensionName(DimIdx dim)
{
    if (!_shortcutRecorder) {
        KnobGuiValue::refreshDimensionName(dim);
    } 
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_KnobGuiValue.cpp"
