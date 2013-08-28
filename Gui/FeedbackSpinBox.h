//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GUI_FEEDBACKSPINBOX_H_
#define POWITER_GUI_FEEDBACKSPINBOX_H_

#include <iostream>
#include <QtCore/QEvent>

#include "Gui/LineEdit.h"

class FeedBackSpinBox : public LineEdit
{
    Q_OBJECT
    
    bool _mode; // 0 = int, 1 = double
    int _decimals; // for the double spinbox only
    double _increment;
    double _mini,_maxi;
public:
    explicit FeedBackSpinBox(QWidget* parent=0, bool mode=false);
    void decimals(int d){
        _decimals=d;
        setMaxLength(_decimals+3);
    }
    void setMaximum(double t){
        _maxi=t;
    }
    void setMinimum(double b){
        _mini=b;
    }
    double value(){bool ok;return text().toDouble(&ok);}
    void setIncrement(double d){_increment=d;}
   
    
protected:
    virtual void wheelEvent(QWheelEvent* e);
    virtual void keyPressEvent(QKeyEvent* e);
signals:
    void valueChanged(double d);

public slots:
    void setValue(double d);
    void setValue(int d){setValue((double)d);}
    void interpretReturn();
 

    
};

#endif /* defined(POWITER_GUI_FEEDBACKSPINBOX_H_) */
