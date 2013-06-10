//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "tabwidget.h"
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>

TabWidget::TabWidget(QWidget* parent):QTabWidget(parent),_style(0){
    _style = QStyleFactory::create("windows");
    setStyle(_style);
}

TabWidget::~TabWidget(){}