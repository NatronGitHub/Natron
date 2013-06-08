//
//  tabwidget.cpp
//  PowiterOsX
//
//  Created by Alexandre on 6/6/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#include "tabwidget.h"
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>

TabWidget::TabWidget(QWidget* parent):QTabWidget(parent),_style(0){
    _style = QStyleFactory::create("windows");
    setStyle(_style);
}

TabWidget::~TabWidget(){}