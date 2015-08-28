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
#ifndef SCRIPTTEXTEDIT_H
#define SCRIPTTEXTEDIT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

class ScriptTextEdit : public QTextEdit
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    Q_PROPERTY(bool isOutput READ getOutput WRITE setOutput)

public:
    
    ScriptTextEdit(QWidget* parent);
    
    virtual ~ScriptTextEdit();
    
    void setOutput(bool o);
    
    bool getOutput() const;
private:
    
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    
    bool isOutput;
};

#endif // SCRIPTTEXTEDIT_H
