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
#include <QtWidgets/QFrame>
#include <QtCore/QPoint>
#include <QtWidgets/QTabBar>
class QStyle;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;
class QMenu;
class Button;
class QMouseEvent;
class QDropEvent;
class QDragEnterEvent;
class QDragLeaveEvent;
class TabWidget;
class QPaintEvent;

class TabBar : public QTabBar{
    QPoint _dragPos;
    TabWidget* _tabWidget; // ptr to the tabWidget
public:
    virtual ~TabBar(){}
    
    TabBar(TabWidget* tabWidget,QWidget* parent = 0);
    
    virtual void mousePressEvent(QMouseEvent* event);
    
    virtual void mouseMoveEvent(QMouseEvent* event);
    
};

class TabWidget : public QFrame {
    
    Q_OBJECT
    
public:
    enum Decorations{
        NONE=0, // no buttons attached to the tabs
        NOT_CLOSABLE=1, // the pane cannot be removed, but each tab can be removed
        CLOSABLE=2 // the pane can be removed as well as tabs.
    };
private:
    
    QVBoxLayout* _mainLayout;
    
    std::vector<QWidget*> _tabs; // the actual tabs
    
    
    QWidget* _header;
    
    QHBoxLayout* _headerLayout;
    TabBar* _tabBar; // the header containing clickable pages
    Button* _leftCornerButton;
    
    Button* _floatButton;
    Button* _closeButton;
    
    QWidget* _currentWidget;
    
    Decorations _decorations;
    bool _isFloating;
    bool _drawDropRect;
public:
    
        
    TabWidget(TabWidget::Decorations decorations,QWidget* parent = 0);
    
    virtual ~TabWidget();
    
    /*Returns false if the title is empty but the TabWidget needs a decoration*/
    bool appendTab(const QString& title,QWidget* widget);
    
    /*Returns false if the title is empty but the TabWidget needs a decoration*/
    bool appendTab(const QString& title,const QIcon& icon,QWidget* widget);
    
    /*Inserts before the element at index.*/
    void insertTab(int index,const QIcon& icon,const QString &title,QWidget* widget);
    
    void insertTab(int index,const QString &title,QWidget* widget);
    
    /*Removes from the TabWidget, but does not delete the widget.
     Returns NULL if the index is not in a good range.*/
    QWidget* removeTab(int index);
    
    /*Get the header name of the tab at index "index".*/
    QString getTabName(int index) const ;
    
    /*Convenience function*/
    QString getTabName(QWidget* tab) const;
    
    /*Removes from the TabWidget, but does not delete the widget.*/
    void removeTab(QWidget* widget);
    
    int count() const {return _tabs.size();}
    
    const QWidget* tabAt(int index) const {return _tabs[index];}
    
    bool isFloating() const {return _isFloating;}
    
    void destroyTabs();
    
    QWidget* currentWidget() const {return _currentWidget;}
    
    virtual void dragEnterEvent(QDragEnterEvent* event);
    
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    
    virtual void dropEvent(QDropEvent* event);
    
    virtual void paintEvent(QPaintEvent* event);
    
    public slots:
    /*Makes current the tab at index "index". Passing an
     index out of range will have no effect.*/
    void makeCurrentTab(int index);
    
    void createMenu();
    
    void addNewViewer();
    
    void moveNodeGraphHere();
    
    void movePropertiesBinHere();
    
    void splitHorizontally();
    
    void splitVertically();
    
    void closePane();
    
    void floatPane();
    
    void closeFloatingPane();
    
    void floatCurrentWidget();
    
    void closeCurrentWidget();
  
private:
    void destroyTab(QWidget* tab);

};

#endif /* defined(__PowiterOsX__tabwidget__) */
