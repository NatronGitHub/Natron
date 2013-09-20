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

#include "Gui/LineEdit.h"


class QDoubleValidator;
class QIntValidator;


class SpinBox : public LineEdit
{
    Q_OBJECT
    
public:
    
    enum SPINBOX_TYPE{INT_SPINBOX = 0,DOUBLE_SPINBOX};
    
    explicit SpinBox(QWidget* parent=0,SPINBOX_TYPE type = INT_SPINBOX);
    
    virtual ~SpinBox();
    
    void decimals(int d);
    
    void setMaximum(double t);
    
    void setMinimum(double b);
    
    double value(){return text().toDouble();}
    
    void setIncrement(double d){_increment=d;}
   
    
protected:
    
    virtual void wheelEvent(QWheelEvent* e);
    
    virtual void keyPressEvent(QKeyEvent* e);
    
signals:
    
    void valueChanged(double d);

public slots:
    
    void setValue(double d);
    
    void setValue(int d){setValue((double)d);}
    
    /*Used internally when the user pressed enter*/
    void interpretReturn();
 

private:
    
    SPINBOX_TYPE _type;
    int _decimals; // for the double spinbox only
    double _increment;
    double _mini,_maxi;
    QDoubleValidator* _doubleValidator;
    QIntValidator* _intValidator;

    
};

#endif /* defined(POWITER_GUI_FEEDBACKSPINBOX_H_) */
