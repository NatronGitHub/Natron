//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_EditScriptDialog_h_
#define _Gui_EditScriptDialog_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <cfloat> // DBL_MAX
#include <climits> // INT_MAX

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif


class QStringList;

struct EditScriptDialogPrivate;
class EditScriptDialog : public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    EditScriptDialog(QWidget* parent);
    
    virtual ~EditScriptDialog();
    
    void create(const QString& initialScript,bool makeUseRetButton);
    
    QString getExpression(bool* hasRetVariable) const;
    
    bool isUseRetButtonChecked() const;
    
public Q_SLOTS:
    
    void onUseRetButtonClicked(bool useRet);
    void onTextEditChanged();
    void onHelpRequested();
    
    
protected:
    
    virtual void setTitle() = 0;
    
    virtual bool hasRetVariable() const { return false; }
    
    virtual void getImportedModules(QStringList& modules) const = 0;
    
    virtual void getDeclaredVariables(std::list<std::pair<QString,QString> >& variables) const = 0;
    
    virtual QString compileExpression(const QString& expr) = 0;

    static QString getHelpPart1();
    
    static QString getHelpThisNodeVariable();
    
    static QString getHelpThisGroupVariable();
    
    static QString getHelpThisParamVariable();
    
    static QString getHelpDimensionVariable();
    
    static QString getHelpPart2();
    
    virtual QString getCustomHelp() = 0;

private:
    
    void compileAndSetResult(const QString& script);
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    
    boost::scoped_ptr<EditScriptDialogPrivate> _imp;
};


#endif // _Gui_EditScriptDialog_h_
