//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef __PowiterOsX__FeedbackSpinBox__
#define __PowiterOsX__FeedbackSpinBox__

#include <iostream>
#include <QtWidgets/qlineedit.h>
#include <QtGui/qevent.h>

class FeedBackSpinBox : public QLineEdit
{
    Q_OBJECT
    
    bool _mode; // 0 = int, 1 = double
    int _decimals; // for the double spinbox only
    double _increment;
    double _mini,_maxi;
public:
    FeedBackSpinBox(QWidget* parent=0,bool mode=0);
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

#endif /* defined(__PowiterOsX__FeedbackSpinBox__) */
