//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_LINEEDIT_H_
#define NATRON_GUI_LINEEDIT_H_

#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QUrl>

class QPaintEvent;
class LineEdit : public QLineEdit {
public:
    explicit LineEdit(QWidget* parent = 0):QLineEdit(parent){ setAttribute(Qt::WA_MacShowFocusRect,0);}
    virtual ~LineEdit(){}
    
    virtual void paintEvent(QPaintEvent* e){
        QPalette p = this->palette();
        QColor c(200,200,200,255);
        p.setColor( QPalette::Highlight, c );
        p.setColor( QPalette::HighlightedText, c );
        this->setPalette( p );
        QLineEdit::paintEvent(e);
    }
    
    void dropEvent(QDropEvent* event){
        if(!event->mimeData()->hasUrls())
            return;
        
        QStringList filesList;
        QList<QUrl> urls = event->mimeData()->urls();
        QString path;
        if(urls.size() > 0){
            path = urls.at(0).path();
        }
        if(!path.isEmpty()){
            setText(path);
            
        }

    }
    
    void dragEnterEvent(QDragEnterEvent *ev){
        ev->accept();
    }
    
    void dragMoveEvent(QDragMoveEvent* e){
        e->accept();
    }
    
    void dragLeaveEvent(QDragLeaveEvent* e){
        e->accept();
    }
};


#endif
