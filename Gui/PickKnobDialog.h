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

#ifndef _Gui_PickKnobDialog_h_
#define _Gui_PickKnobDialog_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include <QDialog>

class DockablePanel;
class KnobGui;

struct PickKnobDialogPrivate;
class PickKnobDialog : public QDialog
{
    Q_OBJECT
    
public:
    
    PickKnobDialog(DockablePanel* panel, QWidget* parent);
    
    virtual ~PickKnobDialog();
    
    KnobGui* getSelectedKnob(bool* useExpressionLink) const;
    
public Q_SLOTS:
    
    void onNodeComboEditingFinished();
    
private:
    
    boost::scoped_ptr<PickKnobDialogPrivate> _imp;
};

#endif // _Gui_PickKnobDialog_h_
