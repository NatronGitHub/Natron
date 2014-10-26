//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef QUESTIONDIALOG_H
#define MESSAGE_BOX_H

#include <boost/scoped_ptr.hpp>
#include <QDialog>

#include "Global/GlobalDefines.h"

class QCheckBox;
class QAbstractButton;

struct MessageBoxPrivate;
class MessageBox : public QDialog
{
    Q_OBJECT
    
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
    
public slots:
    
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

#endif // QUESTIONDIALOG_H
