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

 

 




#ifndef PowiterOsX_lineEdit_h
#define PowiterOsX_lineEdit_h
#include <QLineEdit>

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
