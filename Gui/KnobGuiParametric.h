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

#ifndef Gui_KnobGuiParametric_h
#define Gui_KnobGuiParametric_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector> // KnobGuiInt
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif


CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/EngineFwd.h"

#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/KnobGuiWidgets.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct KnobGuiParametricPrivate;
class KnobGuiParametric
    : public QObject, public KnobGuiWidgets
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiParametric(knob, view);
    }

    KnobGuiParametric(const KnobGuiPtr& knob, ViewIdx view);
    virtual bool mustCreateLabelWidget() const OVERRIDE
    {
        return false;
    }

    virtual ~KnobGuiParametric() OVERRIDE;

Q_SIGNALS:

    void mustUpdateCurveWidgetLater();

public Q_SLOTS:

    void onUpdateCurveWidgetLaterReceived();

    void onCurveChanged(DimSpec dimension);

    void onItemsSelectionChanged();

    void resetSelectedCurves();

    void onColorChanged(DimSpec dimension);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void setWidgetsVisible(bool visible) OVERRIDE FINAL;
    virtual void setEnabled(const std::vector<bool>& perDimEnabled) OVERRIDE FINAL;
    virtual void updateGUI() OVERRIDE FINAL;
    virtual void reflectMultipleSelection(bool /*dirty*/) OVERRIDE FINAL
    {
    }
    virtual void reflectSelectionState(bool /*selected*/) OVERRIDE FINAL
    {

    }


    virtual void refreshDimensionName(DimIdx dim) OVERRIDE FINAL;

private:

    boost::scoped_ptr<KnobGuiParametricPrivate> _imp;

};

NATRON_NAMESPACE_EXIT

#endif // Gui_KnobGuiParametric_h
