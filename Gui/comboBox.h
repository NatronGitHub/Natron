//
//  comboBox.h
//  PowiterOsX
//
//  Created by Alexandre on 4/28/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//


/*Just a wrapper around QComboBox to make the default style the QWindowsStyle*/

#include <QtWidgets/QComboBox>
#include <QtWidgets/QStyleFactory>
#ifndef PowiterOsX_comboBox_h
#define PowiterOsX_comboBox_h

class ComboBox : public QComboBox
{
public:
    ComboBox(QWidget* parent = 0):QComboBox(parent){
        setStyle(QStyleFactory::create("windows"));
    }
    
};

#endif
