//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#ifndef SCRIPTTEXTEDIT_H
#define SCRIPTTEXTEDIT_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

class ScriptTextEdit : public QTextEdit
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

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
