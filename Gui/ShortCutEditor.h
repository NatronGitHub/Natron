//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef SHORTCUTEDITOR_H
#define SHORTCUTEDITOR_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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

class BoundAction;
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
