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

#ifndef _Gui_AddKnobDialog_h_
#define _Gui_AddKnobDialog_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include <QDialog>

class KnobI;
class DockablePanel;

struct AddKnobDialogPrivate;
class AddKnobDialog : public QDialog
{
    Q_OBJECT
public:
    
    AddKnobDialog(DockablePanel* panel,const boost::shared_ptr<KnobI>& knob, QWidget* parent);
    
    virtual ~AddKnobDialog();
    
    boost::shared_ptr<KnobI> getKnob() const;

public Q_SLOTS:
    
    void onPageCurrentIndexChanged(int index);
    
    void onTypeCurrentIndexChanged(int index);
    
    void onOkClicked();
private:
    
    boost::scoped_ptr<AddKnobDialogPrivate> _imp;
};
#endif // _Gui_AddKnobDialog_h_
