//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_EditExpressionDialog_h_
#define _Gui_EditExpressionDialog_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
