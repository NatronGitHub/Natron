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

#ifndef POWITER_GUI_BUTTON_H_
#define POWITER_GUI_BUTTON_H_

#include <QPushButton>

class Button : public QPushButton {
public:
    explicit Button(QWidget* parent = 0): QPushButton(parent){
        setAttribute(Qt::WA_LayoutUsesWidgetRect);
    }
    explicit Button(const QString & text, QWidget * parent = 0) : QPushButton(text,parent){
        setAttribute(Qt::WA_LayoutUsesWidgetRect);
    }
    Button(const QIcon & icon, const QString & text, QWidget * parent = 0) : QPushButton(icon,text,parent){
        setAttribute(Qt::WA_LayoutUsesWidgetRect);
    }
   
};

#endif
