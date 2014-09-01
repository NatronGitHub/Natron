//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
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
class Splitter;


class DragPixmap
    : public QWidget
{
    QPixmap _pixmap;
    QPoint _offset;

public:

    DragPixmap(const QPixmap & pixmap,
               const QPoint & offsetFromMouse);

    virtual ~DragPixmap()
    {
    }

    void update(const QPoint & globalPos);

private:

    virtual void paintEvent(QPaintEvent*) OVERRIDE;
};

class TabBar
    : public QTabBar
{
    Q_OBJECT

public:

    explicit TabBar(TabWidget* tabWidget,
                    QWidget* parent = 0);

    virtual ~TabBar()
    {
    }

private:
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;

    QPixmap makePixmapForDrag(int index);

    QPoint _dragPos;
    DragPixmap* _dragPix;
    TabWidget* _tabWidget; // ptr to the tabWidget
};

class TabWidget
    : public QFrame
{
    Q_OBJECT

public:


    explicit TabWidget(Gui* gui,
                       QWidget* parent = 0);

    virtual ~TabWidget();

    Gui* getGui() const
    {
        return _gui;
    }

    ////To be called when it is going to be deleted
    void notifyGuiAboutRemoval();

    ///When not closable a tabwidget cannot float  or be closed
    void setClosable(bool closable);

    /*Appends a new tab to the tab widget. The name of the tab will be the QWidget's object's name.
       Returns false if the object's name is empty but the TabWidget needs a decoration*/
    bool appendTab(QWidget* widget);

    /*Appends a new tab to the tab widget. The name of the tab will be the QWidget's object's name.
       Returns false if the title is empty but the TabWidget needs a decoration*/
    bool appendTab(const QIcon & icon,QWidget* widget);

    /*Inserts before the element at index.*/
    void insertTab(int index,const QIcon & icon,QWidget* widget);

    void insertTab(int index,QWidget* widget);

    /*Removes from the TabWidget, but does not delete the widget.
       Returns NULL if the index is not in a good range.*/
    QWidget* removeTab(int index);

    /*Get the header name of the tab at index "index".*/
    QString getTabName(int index) const;

    /*Convenience function*/
    QString getTabName(QWidget* tab) const;

    void setTabName(QWidget* tab,const QString & name);

    /*Removes from the TabWidget, but does not delete the widget.*/
    void removeTab(QWidget* widget);

    int count() const
    {
        QMutexLocker l(&_tabWidgetStateMutex);

        return _tabs.size();
    }

    QWidget* tabAt(int index) const
    {
        QMutexLocker l(&_tabWidgetStateMutex);

        return _tabs[index];
    }

    QStringList getTabNames() const;

    void destroyTabs();

    QWidget* currentWidget() const
    {
        QMutexLocker l(&_tabWidgetStateMutex);

        return _currentWidget;
    }

    void dettachTabs();
    static bool moveTab(QWidget* what,TabWidget* where);

    /**
     * @brief Starts dragging the selected panel. The following actions are performed:
     * - The panel is removed from it's current TabWidget
     * - The cursor is displaying a screenshot of the panel that is going to be dragged. The Gui remembers
     * the last cursor.
     * - The Gui receives a pointer to this object making it able to other panels to know
     * the currently dragged panel.
     **/
    void startDragTab(int index);

    void stopDragTab(const QPoint & globalPos);

    void setDrawDropRect(bool draw);

    bool isWithinWidget(const QPoint & globalPos) const;

    void setObjectName_mt_safe(const QString & str);

    QString objectName_mt_safe() const;

    TabWidget* splitHorizontally(bool autoSave = true);
    TabWidget* splitVertically(bool autoSave = true);

    int activeIndex() const;


    ///MT-Safe
    bool isFullScreen() const;

    ///MT-Safe
    bool isAnchor() const;

    void setAsAnchor(bool anchor);

    /**
     * @brief Returns true if this tabwidget is part of a floating window
     **/
    bool isFloatingWindowChild() const;

    void discardGuiPointer();

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

    void onSplitHorizontally()
    {
        splitHorizontally();
    }

    void onSplitVertically()
    {
        splitVertically();
    }

    void closePane();

    /**
     * @brief Make a new floating window and embbed this widget in it.
     **/
    void floatPane(QPoint* position = NULL);

    /**
     * @brief If the direct parent of this widget is a floating window then close it.
     **/
    void tryCloseFloatingPane();

    ///If there's 1 tab it floats this pane
    ///Otherwise it creates a new pane, move the current tab to it and floats it
    void floatCurrentWidget();

    void closeCurrentWidget();

    void closeTab(int index);

    void onSetAsAnchorActionTriggered();

private:


    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent (QKeyEvent* e) OVERRIDE FINAL;

    bool destroyTab(QWidget* tab) WARN_UNUSED_RETURN;

private:

    TabWidget* splitInternal(bool autoSave,Qt::Orientation orientation);

    /**
     * @brief Deletes container and move the other split (the one that is not this) to the parent of container.
     * If the parent container is a splitter then we will insert it instead of the container.
     * Otherwise if the parent is a floating window we will insert it as embedded widget of the window.
     **/
    void closeSplitterAndMoveOtherSplitToParent(Splitter* container);

    // FIXME: PIMPL
    Gui* _gui;
    QVBoxLayout* _mainLayout;
    std::vector<QWidget*> _tabs; // the actual tabs
    QWidget* _header;
    QHBoxLayout* _headerLayout;
    bool _modifyingTabBar;
    TabBar* _tabBar; // the header containing clickable pages
    Button* _leftCornerButton;
    Button* _floatButton;
    Button* _closeButton;
    QWidget* _currentWidget;
    bool _drawDropRect;
    bool _fullScreen;
    bool _isAnchor;

    ///Protects  _currentWidget, _fullScreen, _isViewerAnchor
    mutable QMutex _tabWidgetStateMutex;
};


#endif /* defined(NATRON_GUI_TABWIDGET_H_) */
