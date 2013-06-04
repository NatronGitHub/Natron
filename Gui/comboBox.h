//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com


/*Just a wrapper around QComboBox to make the default style the QWindowsStyle*/

#include <QtWidgets/QComboBox>
#include <QtWidgets/QStyleFactory>
#ifndef PowiterOsX_comboBox_h
#define PowiterOsX_comboBox_h

class ComboBox : public QComboBox
{
public:
    ComboBox(QWidget* parent = 0):QComboBox(parent),_style(0){
        _style = QStyleFactory::create("windows");
        setStyle(_style);
    }
    virtual ~ComboBox(){delete _style;}
    
private:
    QStyle* _style;
    
};

#endif
