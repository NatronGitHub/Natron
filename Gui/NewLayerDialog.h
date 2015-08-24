//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_NewLayerDialog_h_
#define _Gui_NewLayerDialog_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"

#include <QDialog>
namespace Natron {
    class ImageComponents;
}

struct NewLayerDialogPrivate;
class NewLayerDialog : public QDialog
{
    Q_OBJECT
    
public:
    NewLayerDialog(QWidget* parent);
    
    virtual ~NewLayerDialog();
    
    Natron::ImageComponents getComponents() const;
    
public Q_SLOTS:
    void onNumCompsChanged(double value);
    
    void onRGBAButtonClicked();
    
private:
    boost::scoped_ptr<NewLayerDialogPrivate> _imp;
};

#endif // _Gui_NewLayerDialog_h_
