//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef _NATRON_MESSAGE_BOX_H_
#define _NATRON_MESSAGE_BOX_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

class QCheckBox;
class QAbstractButton;

namespace Natron {
struct MessageBoxPrivate;
class MessageBox : public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    enum MessageBoxTypeEnum
    {
        eMessageBoxTypeInformation,
        eMessageBoxTypeWarning,
        eMessageBoxTypeError,
        eMessageBoxTypeQuestion
    };
    
    MessageBox(const QString & title,
               const QString & message,
               MessageBoxTypeEnum type,
               const Natron::StandardButtons& buttons,
               Natron::StandardButtonEnum defaultButton,
               QWidget* parent);
    
    virtual ~MessageBox();
    
    Natron::StandardButtonEnum getReply() const;
    
    void setCheckBox(QCheckBox* checkbox);
    
    bool isCheckBoxChecked() const;
    
public Q_SLOTS:
    
    void onButtonClicked(QAbstractButton* button);
    
private:
    
    void updateSize();
    
    void init(const QString & title,
              const QString & message,
              const Natron::StandardButtons& buttons,
              Natron::StandardButtonEnum defaultButton);
    
    virtual bool event(QEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<MessageBoxPrivate> _imp;
};

}

#endif // _NATRON_MESSAGE_BOX_H_
