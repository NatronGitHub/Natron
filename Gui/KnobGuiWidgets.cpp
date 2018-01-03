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

#include "KnobGuiWidgets.h"

#include "Engine/KnobTypes.h"
#include "Gui/KnobGui.h"


NATRON_NAMESPACE_ENTER

struct KnobGuiWidgetsPrivate
{
    KnobGuiWidgets* publicInterface;

    KnobGuiWPtr knobUI;

    ViewIdx view;

    KnobGuiWidgetsPrivate(KnobGuiWidgets* publicInterface, const KnobGuiPtr& knobUI, ViewIdx view)
    : publicInterface(publicInterface)
    , knobUI(knobUI)
    , view(view)
    {

    }
};

KnobGuiWidgets::KnobGuiWidgets(const KnobGuiPtr& knobUI, ViewIdx view)
: _imp(new KnobGuiWidgetsPrivate(this, knobUI, view))
{
}

KnobGuiWidgets::~KnobGuiWidgets()
{

}

KnobGuiPtr
KnobGuiWidgets::getKnobGui() const
{
    return _imp->knobUI.lock();
}

ViewIdx
KnobGuiWidgets::getView() const
{
    return _imp->view;
}

std::string
KnobGuiWidgets::getDescriptionLabel() const
{
    std::string ret;
    KnobGuiPtr knob = getKnobGui();
    if (!knob) {
        return ret;
    }
    KnobIPtr internalKnob = knob->getKnob();
    if (!internalKnob) {
        return ret;
    }
    ret = internalKnob->getLabel();
    return ret;
}


void
KnobGuiWidgets::enableRightClickMenu(const KnobGuiPtr& knob,
                                     QWidget* widget,
                                     DimSpec dimension,
                                     ViewSetSpec view)
{

    KnobIPtr internalKnob = knob->getKnob();
    if (!internalKnob) {
        return;
    }
    KnobSeparatorPtr sep = toKnobSeparator(internalKnob);
    KnobGroupPtr grp = toKnobGroup(internalKnob);
    if (sep || grp) {
        return;
    }

    widget->setProperty(KNOB_RIGHT_CLICK_DIM_PROPERTY, QVariant(dimension));
    widget->setProperty(KNOB_RIGHT_CLICK_VIEW_PROPERTY, QVariant(view));
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect( widget, SIGNAL(customContextMenuRequested(QPoint)), knob.get(), SLOT(onRightClickClicked(QPoint)) );
}


NATRON_NAMESPACE_EXIT
