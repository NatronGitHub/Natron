//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef PowiterOsX_lineEdit_h
#define PowiterOsX_lineEdit_h
#include <QtWidgets/QLineEdit>

class QPaintEvent;
class LineEdit : public QLineEdit {
public:
    LineEdit(QWidget* parent = 0):QLineEdit(parent){ setAttribute(Qt::WA_MacShowFocusRect,0);}
    virtual ~LineEdit(){}
    
    virtual void paintEvent(QPaintEvent* e){
        QPalette p = this->palette();
        p.setColor( QPalette::Highlight, Qt::red );
        p.setColor( QPalette::HighlightedText, Qt::red );
        this->setPalette( p );
        QLineEdit::paintEvent(e);
    }
};


#endif
