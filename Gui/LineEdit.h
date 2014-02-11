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

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QLineEdit>
CLANG_DIAG_ON(deprecated)

#include "Global/Macros.h"

class QPaintEvent;
class QDropEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;

class LineEdit : public QLineEdit {
    
    Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)
    
public:
    explicit LineEdit(QWidget* parent = 0);
    virtual ~LineEdit() OVERRIDE;
    
    int getAnimation() const { return animation; }
    
    void setAnimation(int v) ;
    
private:
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;

    void dropEvent(QDropEvent* event);

    void dragEnterEvent(QDragEnterEvent *ev);

    void dragMoveEvent(QDragMoveEvent* e);

    void dragLeaveEvent(QDragLeaveEvent* e);
    
    int animation;
};


#endif
