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

#ifndef SHORTCUTEDITOR_H
#define SHORTCUTEDITOR_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /opt/boost/include/boost/serialization/static_warning.hpp:104:18: warning: typedef 'STATIC_WARNING_LINE102' locally defined but not used [-Wunused-local-typedefs]
#include <boost/scoped_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
CLANG_DIAG_ON(deprecated-register)

#include "Gui/LineEdit.h"
#include "Gui/GuiFwd.h"


struct ShortCutEditorPrivate;

class ShortCutEditor
    : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    ShortCutEditor(QWidget* parent);

    virtual ~ShortCutEditor();

    void addShortcut(BoundAction* action);
    
public Q_SLOTS:

    void onSelectionChanged();

    void onValidateButtonClicked();

    void onClearButtonClicked();

    void onResetButtonClicked();

    void onRestoreDefaultsButtonClicked();

    void onApplyButtonClicked();

    void onCancelButtonClicked();

    void onOkButtonClicked();

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<ShortCutEditorPrivate> _imp;
};

class KeybindRecorder
    : public LineEdit
{
public:

    KeybindRecorder(QWidget* parent);

    virtual ~KeybindRecorder();

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
};

#endif // SHORTCUTEDITOR_H
