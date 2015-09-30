/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_GUI_TABWIDGET_H
#define NATRON_GUI_TABWIDGET_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <vector>


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif


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
class QDropEvent;
class QDragEnterEvent;
class QDragLeaveEvent;
class TabWidget;
class QPaintEvent;
class Gui;
class ScriptObject;
class Splitter;
class PanelWidget;

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
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
    Q_PROPERTY(bool mouseOverFocus READ hasMouseOverFocus WRITE setMouseOverFocus)
    Q_PROPERTY(bool clickFocus READ hasClickFocus WRITE setClickFocus)
    
public:

    explicit TabBar(TabWidget* tabWidget,
                    QWidget* parent = 0);

    virtual ~TabBar()
    {
    }

    bool hasMouseOverFocus() const
    {
        return mouseOverFocus;
    }
    
    bool hasClickFocus() const
    {
        return clickFocus;
    }
    
    void setMouseOverFocus(bool focus);
    void setClickFocus(bool focus);
    
Q_SIGNALS:
    
    void mouseLeftTabBar();
    
private:
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    QPixmap makePixmapForDrag(int index);

    QPoint _dragPos;
    DragPixmap* _dragPix;
    TabWidget* _tabWidget; // ptr to the tabWidget
    bool _processingLeaveEvent; // to avoid recursions in leaveEvent
    
    bool mouseOverFocus;
    bool clickFocus;
};

class TabWidgetHeader : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    TabWidgetHeader(QWidget* parent)
    : QWidget(parent)
    {}
    
Q_SIGNALS:
    
    void mouseLeftTabBar();
private:
    
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL
    {
        Q_EMIT mouseLeftTabBar();
        QWidget::leaveEvent(e);
    }
    
};


struct TabWidgetPrivate;
class TabWidget
    : public QFrame
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum DropRectType {
        eDropRectNone,
        eDropRectAll,
        eDropRectLeftSplit,
        eDropRectRightSplit,
        eDropRectTopSplit,
        eDropRectBottomSplit,
    };

    explicit TabWidget(Gui* gui,
                       QWidget* parent = 0);

    virtual ~TabWidget();

    Gui* getGui() const;

    ////To be called when it is going to be deleted
    void notifyGuiAboutRemoval();

    ///When not closable a tabwidget cannot float  or be closed
    void setClosable(bool closable);

    /*Appends a new tab to the tab widget. The name of the tab will be the QWidget's object's name.
       Returns false if the object's name is empty but the TabWidget needs a decoration*/
    bool appendTab(PanelWidget* widget, ScriptObject* object);

    /*Appends a new tab to the tab widget. The name of the tab will be the QWidget's object's name.
       Returns false if the title is empty but the TabWidget needs a decoration*/
    bool appendTab(const QIcon & icon,PanelWidget* widget,  ScriptObject* object);

    /*Inserts before the element at index.*/
    void insertTab(int index,const QIcon & icon,PanelWidget* widget, ScriptObject* object);

    void insertTab(int index,PanelWidget* widget, ScriptObject* object);

    /*Removes from the TabWidget, but does not delete the widget.
       Returns NULL if the index is not in a good range.*/
    PanelWidget* removeTab(int index,bool userAction);

    /*Get the header name of the tab at index "index".*/
    QString getTabLabel(int index) const;

    /*Convenience function*/
    QString getTabLabel(PanelWidget* tab) const;

    void setTabLabel(PanelWidget* tab,const QString & name);

    /*Removes from the TabWidget, but does not delete the widget.*/
    void removeTab(PanelWidget* widget,bool userAction);

    int count() const;
    
    PanelWidget* tabAt(int index) const;
    
    void tabAt(int index, PanelWidget** w, ScriptObject** obj) const;

    QStringList getTabScriptNames() const;

    PanelWidget* currentWidget() const;
    
    void currentWidget(PanelWidget** w,ScriptObject** obj) const;
    
    /**
     * @brief Set w as the current widget of the tab
     **/
    void setCurrentWidget(PanelWidget* w);

    void dettachTabs();
    static bool moveTab(PanelWidget* what, ScriptObject* obj,TabWidget* where);

    /**
     * @brief Starts dragging the selected panel. The following actions are performed:
     * - The panel is removed from it's current TabWidget
     * - The cursor is displaying a screenshot of the panel that is going to be dragged. The Gui remembers
     * the last cursor.
     * - The Gui receives a pointer to this object making it able to other panels to know
     * the currently dragged panel.
     **/
    void startDragTab(int index);

    bool stopDragTab(const QPoint & globalPos);

    void setDrawDropRect(DropRectType type, bool draw);

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
    
    void onTabScriptNameChanged(PanelWidget* tab,const std::string& oldName,const std::string& newName);

public Q_SLOTS:
    /*Makes current the tab at index "index". Passing an
       index out of range will have no effect.*/
    void makeCurrentTab(int index);

    void createMenu();

    void addNewViewer();

    void moveNodeGraphHere();

    void moveCurveEditorHere();

    void moveDopeSheetEditorHere();

    void newHistogramHere();

    void movePropertiesBinHere();
    
    void moveScriptEditorHere();

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
    
    void onShowHideTabBarActionTriggered();

    void onTabBarMouseLeft();
    
    void onHideLeftToolBarActionTriggered();
    
    void moveToNextTab();
    
    void moveToPreviousTab();
    
    void onCurrentTabDeleted();
    
    void onUserPanelActionTriggered();
private:


    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    

private:

    TabWidget* splitInternal(bool autoSave,Qt::Orientation orientation);

    /**
     * @brief Deletes container and move the other split (the one that is not this) to the parent of container.
     * If the parent container is a splitter then we will insert it instead of the container.
     * Otherwise if the parent is a floating window we will insert it as embedded widget of the window.
     **/
    void closeSplitterAndMoveOtherSplitToParent(Splitter* container);

    
    boost::scoped_ptr<TabWidgetPrivate> _imp;
};


#endif /* defined(NATRON_GUI_TABWIDGET_H_) */
