//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef __PowiterOsX__tabwidget__
#define __PowiterOsX__tabwidget__

#include <iostream>
#include <vector>
#include <QtWidgets/QWidget>
class QStyle;
class QTabBar;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;
class Button;

class TabWidget : public QWidget {
    
    Q_OBJECT
    
    QVBoxLayout* _mainLayout;
    
    std::vector<QWidget*> _tabs; // the actual tabs
    
    
    QWidget* _header;
    
    QHBoxLayout* _headerLayout;
    QTabBar* _tabBar; // the header containing clickable pages
    Button* _leftCornerButton;
    Button* _floatButton;
    Button* _closeButton;
    
    QWidget* _currentWidget;
    
public:
    
    TabWidget(QWidget* parent = 0);
    
    virtual ~TabWidget();
    
    void appendTab(const QString& title,QWidget* widget);
    
    void appendTab(const QString& title,const QIcon& icon,QWidget* widget);
    
    /*Inserts before the element at index.*/
    void insertTab(int index,const QIcon& icon,const QString &title,QWidget* widget);
    
    void insertTab(int index,const QString &title,QWidget* widget);
    
    /*Removes from the TabWidget, but does not delete the widget.
     Returns NULL if the index is not in a good range.*/
    QWidget* removeTab(int index);
    
    /*Removes from the TabWidget, but does not delete the widget.*/
    void removeTab(QWidget* widget);
    
    int count() const {return _tabs.size();}
    
    
    const QWidget* tabAt(int index) const {return _tabs[index];}

public slots:
    /*Makes current the tab at index "index". Passing an
     index out of range will have no effect.*/
    void makeCurrentTab(int index);
    
};

#endif /* defined(__PowiterOsX__tabwidget__) */
