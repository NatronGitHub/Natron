/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef Gui_KnobGuiGroup_h
#define Gui_KnobGuiGroup_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector> // KnobGuiInt
#include <list>

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

#include "Gui/KnobGuiWidgets.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class KnobGuiGroup
: QObject
, public KnobGuiWidgets
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiGroup(knob, view);
    }

    KnobGuiGroup(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiGroup() OVERRIDE;

    void addKnob(const KnobGuiPtr& child);

    const std::list<KnobGuiWPtr>& getChildren() const { return _children; }

    bool isChecked() const;


public Q_SLOTS:
    void onCheckboxChecked(bool b);

private:

    void setCheckedInternal(bool checked, bool userRequested);

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void setWidgetsVisible(bool visible) OVERRIDE FINAL;
    virtual void setEnabled(const std::vector<bool>& perDimEnabled)  OVERRIDE FINAL;
    virtual void updateGUI() OVERRIDE FINAL;
    virtual void reflectMultipleSelection(bool /*dirty*/) OVERRIDE FINAL
    {
    }
    virtual void reflectSelectionState(bool /*selected*/) OVERRIDE FINAL
    {

    }
    virtual bool eventFilter(QObject *target, QEvent* e) OVERRIDE FINAL;


private:
    bool _checked;
    GroupBoxLabel *_button;
    std::list<KnobGuiWPtr> _children;
    //enabled too
    KnobGroupWPtr _knob;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_KnobGuiGroup_h
