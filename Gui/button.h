//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com


#ifndef PowiterOsX_button_h
#define PowiterOsX_button_h

#include <QtWidgets/QPushButton>

class Button : public QPushButton {
    
    
public:
    Button(QWidget* parent = 0): QPushButton(parent){
        setAttribute(Qt::WA_LayoutUsesWidgetRect);
    }
    Button(const QString & text, QWidget * parent = 0) : QPushButton(text,parent){
        setAttribute(Qt::WA_LayoutUsesWidgetRect);
    }
    Button(const QIcon & icon, const QString & text, QWidget * parent = 0) : QPushButton(icon,text,parent){
        setAttribute(Qt::WA_LayoutUsesWidgetRect);
    }
};

#endif
