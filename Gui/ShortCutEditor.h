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
#include "Global/Macros.h"
GCC_DIAG_OFF_48(unused-local-typedefs) // /opt/boost/include/boost/serialization/static_warning.hpp:104:18: warning: typedef 'STATIC_WARNING_LINE102' locally defined but not used [-Wunused-local-typedefs]
#include <boost/scoped_ptr.hpp>
GCC_DIAG_OFF_48(unused-local-typedefs)
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QWidget>
CLANG_DIAG_ON(deprecated-register)
#include "Gui/LineEdit.h"

struct ShortCutEditorPrivate;
class ShortCutEditor
    : public QWidget
{
    Q_OBJECT

public:

    ShortCutEditor(QWidget* parent);

    virtual ~ShortCutEditor();

public slots:

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
