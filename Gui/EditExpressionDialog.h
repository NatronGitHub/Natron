/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Gui_EditExpressionDialog_h
#define Gui_EditExpressionDialog_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <utility>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include <QtCore/QString>

#include "Gui/EditScriptDialog.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class EditExpressionDialog
    : public EditScriptDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:
    int _dimension;
    KnobGuiPtr _knob;

public:

    EditExpressionDialog(Gui* gui,
                         int dimension,
                         const KnobGuiPtr& knob,
                         QWidget* parent);

    virtual ~EditExpressionDialog()
    {
    }

    int getDimension() const;

private:

    virtual void getImportedModules(QStringList& modules) const OVERRIDE FINAL;
    virtual void getDeclaredVariables(std::list<std::pair<QString, QString> >& variables) const OVERRIDE FINAL;
    virtual bool hasRetVariable() const OVERRIDE FINAL;
    virtual void setTitle() OVERRIDE FINAL;
    virtual QString compileExpression(const QString& expr) OVERRIDE FINAL;
    virtual QString getCustomHelp() OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_EditExpressionDialog_h
