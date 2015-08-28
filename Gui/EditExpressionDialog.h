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

#ifndef _Gui_EditExpressionDialog_h_
#define _Gui_EditExpressionDialog_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <utility>

#include "Global/Macros.h"

#include <QtCore/QString>

#include "Gui/EditScriptDialog.h"

class QStringList;

class QWidget;
class KnobGui;

class EditExpressionDialog : public EditScriptDialog
{
    
    int _dimension;
    KnobGui* _knob;
    
public:
    
    EditExpressionDialog(int dimension, KnobGui* knob, QWidget* parent);
    
    virtual ~EditExpressionDialog()
    {
    }
    
    int getDimension() const;

private:
    
    virtual void getImportedModules(QStringList& modules) const OVERRIDE FINAL;
    
    virtual void getDeclaredVariables(std::list<std::pair<QString,QString> >& variables) const OVERRIDE FINAL;
    
    virtual bool hasRetVariable() const OVERRIDE FINAL;
    
    virtual void setTitle() OVERRIDE FINAL;
    
    virtual QString compileExpression(const QString& expr) OVERRIDE FINAL;

    virtual QString getCustomHelp() OVERRIDE FINAL;

};

#endif // _Gui_EditExpressionDialog_h_
