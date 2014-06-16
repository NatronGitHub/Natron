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

#ifndef NATRON_GUI_TABWIDGET_H_
#define NATRON_GUI_TABWIDGET_H_

#include <map>
#include <vector>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
#include <QTabBar>
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

class QWidget;
class QStyle;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;
class QMenu;
class Button;
class QMouseEvent;
class QLabel;
class QDropEvent;
class QDragEnterEvent;
class QDragLeaveEvent;
class TabWidget;
class QPaintEvent;
class Gui;



/*This class represents a floating pane that embeds a widget*/
class FloatingWidget : public QWidget{
    
    Q_OBJECT
    
public:
    
    explicit FloatingWidget(QWidget* parent = 0);
    
    virtual ~FloatingWidget() OVERRIDE {}
    
    /*Set the embedded widget. Only 1 widget can be embedded
     by FloatingWidget. Once set, this function does nothing
     for subsequent calls..*/
    void setWidget(const QSize& widgetSize,QWidget* w);
    
    void removeWidget();
    
signals:
    
    void closed();
    
private:
    
    virtual void closeEvent(QCloseEvent* e) OVERRIDE;
    
    QWidget* _embeddedWidget;
    QVBoxLayout* _layout;
};

class DragPixmap : public QWidget {
    
    QPixmap _pixmap;
    QPoint _offset;
    
public:
    
    DragPixmap(const QPixmap& pixmap,const QPoint& offsetFromMouse);
    
    virtual ~DragPixmap(){}
    
    void update(const QPoint& globalPos);
    
private:
    
    virtual void paintEvent(QPaintEvent*) OVERRIDE;
};

class TabBar : public QTabBar {
    Q_OBJECT
public:

    explicit TabBar(TabWidget* tabWidget,QWidget* parent = 0);
    
    virtual ~TabBar(){}

    //public slots:
    
    // void dragTargetChanged(QWidget* target);
    
private:
    virtual void mousePressEvent(QMouseEvent* event);

    virtual void mouseMoveEvent(QMouseEvent* event);
    
    virtual void mouseReleaseEvent(QMouseEvent* event);
    
    QPixmap makePixmapForDrag(int index);
    
    QPoint _dragPos;
    DragPixmap* _dragPix;
    TabWidget* _tabWidget; // ptr to the tabWidget
};

class TabWidget : public QFrame {
    
    Q_OBJECT
    
public:
    enum Decorations{
        NONE=0, // no buttons attached to the tabs
        NOT_CLOSABLE=1, // the pane cannot be removed, but each tab can be removed
        CLOSABLE=2 // the pane can be removed as well as tabs.
    };

public:
    
    static const QString splitHorizontallyTag;
    static const QString splitVerticallyTag;
        
    explicit TabWidget(Gui* gui,TabWidget::Decorations decorations,QWidget* parent = 0);
    
    virtual ~TabWidget();
    
    const Gui* getGui() const {return _gui;}
    
    /*Appends a new tab to the tab widget. The name of the tab will be the QWidget's object's name.
     Returns false if the object's name is empty but the TabWidget needs a decoration*/
    bool appendTab(QWidget* widget);
    
    /*Appends a new tab to the tab widget. The name of the tab will be the QWidget's object's name.
     Returns false if the title is empty but the TabWidget needs a decoration*/
    bool appendTab(const QIcon& icon,QWidget* widget);
    
    /*Inserts before the element at index.*/
    void insertTab(int index,const QIcon& icon,QWidget* widget);
    
    void insertTab(int index,QWidget* widget);
    
    /*Removes from the TabWidget, but does not delete the widget.
     Returns NULL if the index is not in a good range.*/
    QWidget* removeTab(int index);
    
    /*Get the header name of the tab at index "index".*/
    QString getTabName(int index) const ;
    
    /*Convenience function*/
    QString getTabName(QWidget* tab) const;
    
    void setTabName(QWidget* tab,const QString& name);
    
    /*Removes from the TabWidget, but does not delete the widget.*/
    void removeTab(QWidget* widget);
    
    int count() const {
        QMutexLocker l(&_tabWidgetStateMutex);
        return _tabs.size();
    }
    
    QWidget* tabAt(int index) const {
        QMutexLocker l(&_tabWidgetStateMutex);
        return _tabs[index];
    }
    
    QStringList getTabNames() const;
    
    bool isFloating() const {
        QMutexLocker l(&_tabWidgetStateMutex);
        return _isFloating;
    }
    
    void destroyTabs();
    
    QWidget* currentWidget() const {
        QMutexLocker l(&_tabWidgetStateMutex);
        return _currentWidget;
    }
    
    std::map<TabWidget*,bool> getUserSplits() const {
        QMutexLocker l(&_tabWidgetStateMutex);
        return _userSplits;
    }
    
    bool removeSplit(TabWidget* tab,bool* orientation = NULL);
    
    static void moveTab(QWidget* what,TabWidget* where);
    
    /**
     * @brief Starts dragging the selected panel. The following actions are performed:
     * - The panel is removed from it's current TabWidget
     * - The cursor is displaying a screenshot of the panel that is going to be dragged. The Gui remembers
     * the last cursor.
     * - The Gui receives a pointer to this object making it able to other panels to know
     * the currently dragged panel.
     **/
    void startDragTab(int index);
    
    void stopDragTab(const QPoint& globalPos);

    void setDrawDropRect(bool draw);
    
    bool isWithinWidget(const QPoint& globalPos) const;
    
    void setObjectName_mt_safe(const QString& str);
    
    QString objectName_mt_safe() const;
    
public slots:
    /*Makes current the tab at index "index". Passing an
     index out of range will have no effect.*/
    void makeCurrentTab(int index);
    
    void createMenu();
    
    void addNewViewer();
    
    void moveNodeGraphHere();
    
    void moveCurveEditorHere();
    
    void newHistogramHere();
    
    void movePropertiesBinHere();
    
    void splitHorizontally();
    
    void splitVertically();
    
    void closePane();
    
    void floatPane(QPoint* position = NULL);
    
    void closeFloatingPane();
    
    void floatCurrentWidget();
    
    void floatTab(QWidget* tab);
    
    void closeCurrentWidget();
    
    void closeTab(int index);
    
    QPoint pos_mt_safe() const;
private:
    
    virtual void dropEvent(QDropEvent* event);
    
    virtual void paintEvent(QPaintEvent* event);
    
    virtual void keyPressEvent (QKeyEvent *event);
    
    bool destroyTab(QWidget* tab) WARN_UNUSED_RETURN;

private:
    
    static void removeTagNameRecursively(TabWidget* widget, bool horizontal);
        
    // FIXME: PIMPL
    Gui* _gui;

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

    bool _fullScreen;
    std::map<TabWidget*,bool> _userSplits;//< for each split, whether the user pressed split vertically (true) or horizontally (false)
    mutable QMutex _tabWidgetStateMutex;

};



#endif /* defined(NATRON_GUI_TABWIDGET_H_) */
