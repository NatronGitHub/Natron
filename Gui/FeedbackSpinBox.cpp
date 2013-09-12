//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "FeedbackSpinBox.h"

#include "Global/Macros.h"
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QtGui/QWheelEvent>

FeedbackSpinBox::FeedbackSpinBox(QWidget* parent,bool mode):LineEdit(parent),_mode(mode),_decimals(1),_increment(1.0),_mini(0),_maxi(99)
{

    QObject::connect(this, SIGNAL(returnPressed()), this, SLOT(interpretReturn()));
    setValue(0);
    setMaximumWidth(50);
    setMinimumWidth(35);
    
    decimals(_decimals);
}

void FeedbackSpinBox::setValue(double d){
    clear();
    QString str;
    if(_mode)
        str.setNum(d);
    else
        str.setNum((int)d);
    insert(str);
}
void FeedbackSpinBox::interpretReturn(){
    bool ok;
    emit valueChanged(text().toDouble(&ok));
}

void FeedbackSpinBox::wheelEvent(QWheelEvent *e){
    bool ok;
    double cur= text().toDouble(&ok);
    clear();
    if(e->delta()>0){
        if(cur+_increment <= _maxi)
            cur+=_increment;
    }else{
        if(cur-_increment >= _mini)
            cur-=_increment;
    }
    QString str;
    if(_mode)
        str.setNum(cur);
    else
        str.setNum((int)cur);
    
    insert(str);
    emit valueChanged(cur);
}

void FeedbackSpinBox::keyPressEvent(QKeyEvent *e){
    bool ok;
    double cur= text().toDouble(&ok);
    if(e->key() == Qt::Key_Up){
        clear();
        if(cur+_increment <= _maxi)
            cur+=_increment;
        QString str;
        if(_mode)
            str.setNum(cur);
        else
            str.setNum((int)cur);
        insert(str);
        emit valueChanged(cur);
    }else if(e->key() == Qt::Key_Down){
        clear();
        if(cur-_increment >= _mini)
            cur-=_increment;
        QString str;
        if(_mode)
            str.setNum(cur);
        else
            str.setNum((int)cur);
        insert(str);
        emit valueChanged(cur);
    }
    QLineEdit::keyPressEvent(e);
}
