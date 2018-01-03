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


#ifndef NATRON_GUI_KNOBGUIWIDGETS_H
#define NATRON_GUI_KNOBGUIWIDGETS_H
// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui/GuiFwd.h"


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"


#define KNOB_RIGHT_CLICK_VIEW_PROPERTY "knobViewProperty"
#define KNOB_RIGHT_CLICK_DIM_PROPERTY "knobDimProperty"



NATRON_NAMESPACE_ENTER

/**
 * @class The base class for all knob widgets. It contains all widgets corresponding to one view for the 
 * associated knob.
 **/
struct KnobGuiWidgetsPrivate;
class KnobGuiWidgets : public boost::enable_shared_from_this<KnobGuiWidgets>
{
public:

    KnobGuiWidgets(const KnobGuiPtr& knobUI, ViewIdx view);

    virtual ~KnobGuiWidgets();

    KnobGuiPtr getKnobGui() const;

    ViewIdx getView() const;

    /**
     * @brief Enable right click menu for the given widget
     **/
    static void enableRightClickMenu(const KnobGuiPtr& knob, QWidget* widget, DimSpec dimension, ViewSetSpec view);

    /**
     * @brief Must fill the horizontal layout with all the widgets composing the knob.
     **/
    virtual void createWidget(QHBoxLayout* layout) = 0;

    /**
     * @brief Must be implemented to update the gui of the given dimensions according to the values 
     * of the internal knob.
     **/
    virtual void updateGUI() = 0;

    /**
     * @brief Implement to show/hide all widgets
     **/
    virtual void setWidgetsVisible(bool visible) = 0;

    /**
     * @brief Implement to refresh enabledness on the knob
     **/
    virtual void setEnabled(const std::vector<bool>& perDimEnabled) = 0;

    /**
     * @brief Implement to reflect that multiple knobs will be modified if this knob is modified.
     * This is useful for master knobs in a KnobItemsTable
     **/
    virtual void reflectMultipleSelection(bool selectedMultipleTimes) = 0;

    /**
     * @brief For a table item represented by a row in a KnobItemsTableGui, this should reflect for
     * a knob at a specific dimension the selected state of the row.
     **/
    virtual void reflectSelectionState(bool selected) = 0;

    /**
     * @brief Returns whether to create a label or not. If not, the label will never be shown
     **/
    virtual bool mustCreateLabelWidget() const
    {
        return true;
    }

    /**
     * @brief Returns whether the widget should be in the same column as the label or not
     **/
    virtual bool isLabelOnSameColumn() const
    {
        return false;
    }

    /**
     * @brief Returns whether the label should be displayed in bold or not
     **/
    virtual bool isLabelBold() const
    {
        return false;
    }

    /**
     * @brief Returns whether the layout should add some horizontal stretch after the widgets in the layout
     **/
    virtual bool shouldAddStretch() const {
        return true;
    }

    /**
     * @brief Returns the text label to use in the GUI, by default it returns the internal label
     * set on the knob
     **/
    virtual std::string getDescriptionLabel() const;


    /**
     * @brief Can be implemented to add entries to the right click menu
     **/
    virtual void addRightClickMenuEntries(QMenu* /*menu*/) {}

    /**
     * @brief Can be implemented to  reflect the given animation level with a specific color on the knob
     **/
    virtual void reflectAnimationLevel(DimIdx /*dimension*/, AnimationLevelEnum /*level*/)
    {
    }

    /**
     * @brief Can be implemented to reflect the linked state of a knob
     **/
    virtual void reflectLinkedState(DimIdx dimension, bool linked)
    {
        Q_UNUSED(dimension);
        Q_UNUSED(linked);
    }

    /**
     * @brief Can be implemented to reflect that the knob has modifications
     **/
    virtual void reflectModificationsState() {}

    /**
     * @brief Can be implemented to be notified when the label is changed
     **/
    virtual void onLabelChanged() {}

    /**
     * @brief Can be implemented to set the tooltip
     **/
    virtual void updateToolTip() {}

    /**
     * @brief If this parameter is multi-dimensional, implement it to reflect the fact that all dimensions are folded/unfolded
     **/
    virtual void setAllDimensionsVisible(bool /*visible*/) {}

    /**
     * @brief Can be implemented to set the sub dimension label
     **/
    virtual void refreshDimensionName(DimIdx /*dim*/) {}




private:

    boost::scoped_ptr<KnobGuiWidgetsPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_KNOBGUIWIDGETS_H
