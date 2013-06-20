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
        QColor c(200,200,200,255);
        p.setColor( QPalette::Highlight, c );
        p.setColor( QPalette::HighlightedText, c );
        this->setPalette( p );
        QLineEdit::paintEvent(e);
    }
};


#endif
