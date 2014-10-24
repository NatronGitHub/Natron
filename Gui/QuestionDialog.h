//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include <boost/scoped_ptr.hpp>
#include <QDialog>

#include "Global/GlobalDefines.h"

class QCheckBox;
class QAbstractButton;

struct QuestionDialogPrivate;
class QuestionDialog : public QDialog
{
    Q_OBJECT
    
public:
    
    QuestionDialog(const QString & title,
                   const QString & message,
                   Natron::StandardButtons buttons,
                   Natron::StandardButtonEnum defaultButton,
                   QWidget* parent);
    
    virtual ~QuestionDialog();
    
    Natron::StandardButtonEnum getReply() const;
    
    void setCheckBox(QCheckBox* checkbox);
    
    bool isCheckBoxChecked() const;
    
public slots:
    
    void onButtonClicked(QAbstractButton* button);
    
private:
    
    void updateSize();
    
    virtual bool event(QEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<QuestionDialogPrivate> _imp;
};

#endif // QUESTIONDIALOG_H
