//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_LinkToKnobDialog_h_
#define _Gui_LinkToKnobDialog_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <list>
#include <utility>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QDialog>

class QStringList;

class QWidget;
class KnobGui;
class KnobI;

struct LinkToKnobDialogPrivate;

class LinkToKnobDialog
    : public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    LinkToKnobDialog(KnobGui* from,
                     QWidget* parent);

    virtual ~LinkToKnobDialog();

    boost::shared_ptr<KnobI> getSelectedKnobs() const;

public Q_SLOTS:

    void onNodeComboEditingFinished();

private:

    boost::scoped_ptr<LinkToKnobDialogPrivate> _imp;
};


#endif // _Gui_LinkToKnobDialog_h_
