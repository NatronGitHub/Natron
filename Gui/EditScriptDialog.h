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

#ifndef Gui_EditScriptDialog_h
#define Gui_EditScriptDialog_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cfloat> // DBL_MAX
#include <climits> // INT_MAX

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"
#include "Global/GlobalDefines.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct EditScriptDialogPrivate;
class EditScriptDialog
    : public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    EditScriptDialog(Gui* gui,
                     const KnobGuiPtr& knobExpressionReceiver,
                     DimSpec knobDimension,
                     ViewSetSpec knobView,
                     QWidget* parent);

    virtual ~EditScriptDialog();
    
    void create(ExpressionLanguageEnum language, const QString& initialScript);

    QString getScript() const;

    ExpressionLanguageEnum getSelectedLanguage() const;

    bool isUseRetButtonChecked() const;

public Q_SLOTS:

    void onUseRetButtonClicked(bool useRet);
    void onTextEditChanged();

    void onAcceptedPressed()
    {
        Q_EMIT dialogFinished(true);
    }
    void onRejectedPressed()
    {
        Q_EMIT dialogFinished(false);
    }

    void onLanguageCurrentIndexChanged();

Q_SIGNALS:

    void dialogFinished(bool accepted);

public:

    virtual void setTitle() = 0;
    virtual bool hasRetVariable() const { return false; }

    virtual QString compileExpression(const QString& expr, ExpressionLanguageEnum language) = 0;

private:

    void compileAndSetResult(const QString& script);

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<EditScriptDialogPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_EditScriptDialog_h
