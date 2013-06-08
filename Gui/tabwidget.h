//
//  tabwidget.h
//  PowiterOsX
//
//  Created by Alexandre on 6/6/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

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
