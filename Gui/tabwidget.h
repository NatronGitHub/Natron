//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef __PowiterOsX__tabwidget__
#define __PowiterOsX__tabwidget__

#include <iostream>
#include <QtWidgets/QTabWidget>
class QStyle;
class TabWidget : public QTabWidget {
    
    
public:
    TabWidget(QWidget* parent = 0);
    virtual ~TabWidget();
private:
    QStyle* _style;
};

#endif /* defined(__PowiterOsX__tabwidget__) */
