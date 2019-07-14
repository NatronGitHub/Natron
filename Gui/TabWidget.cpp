/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TabWidget.h"

#include <stdexcept>
#include <sstream> // stringstream

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QThread>
#include <QLayout>
#include <QMenu>
#include <QApplication>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtCore/QMimeData>
#include <QtGui/QDrag>
#include <QStyle>
#include <QtCore/QDebug>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QDragEnterEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtGui/QDragLeaveEvent>
#include <QtGui/QPaintEvent>
#include <QScrollArea>
#include <QSplitter>
CLANG_DIAG_ON(deprecated)

#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ScriptObject.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/PythonPanels.h"
#include "Gui/RegisteredTabs.h"
#include "Gui/ProgressPanel.h"
#include "Gui/ScriptEditor.h"
#include "Gui/Splitter.h"
#include "Gui/ViewerTab.h"

#define LEFT_HAND_CORNER_BUTTON_TT "Manage the layouts for this pane"

#define TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING 0.13

NATRON_NAMESPACE_ENTER

class TransparentDropRect
    : public QWidget
{
    TabWidget::DropRectType _type;
    TabWidget* _widget;

public:


    TransparentDropRect(TabWidget* pane,
                        QWidget* parent = 0)
        : QWidget(parent)
        , _type(TabWidget::eDropRectNone)
        , _widget(pane)
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setStyleSheet( QString::fromUtf8("background:transparent;") );
        setEnabled(false);
        setFocusPolicy(Qt::NoFocus);
        setObjectName( QString::fromUtf8("__tab_widget_transparent_window__") );
    }

    virtual ~TransparentDropRect() {}

    TabWidget::DropRectType getDropType() const
    {
        return _type;
    }

    void setDropType(TabWidget::DropRectType type)
    {
        _type = type;
        if (type != TabWidget::eDropRectNone) {
            update();
        }
    }

    TabWidget* getPane() const
    {
        return _widget;
    }

private:

    virtual bool event(QEvent* e) OVERRIDE FINAL
    {
        QInputEvent* isInput = dynamic_cast<QInputEvent*>(e);

        if (isInput) {
            e->ignore();

            return false;
        } else {
            return QWidget::event(e);
        }
    }

    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL
    {
        QRect r;

        switch (_type) {
        case TabWidget::eDropRectAll:
            r = rect();
            break;
        case TabWidget::eDropRectLeftSplit: {
            QRect tmp = rect();
            r = tmp;
            r.setWidth(tmp.width() / 2.);
            break;
        }
        case TabWidget::eDropRectRightSplit: {
            QRect tmp = rect();
            r = QRect( tmp.left() + tmp.width() / 2., tmp.top(), tmp.width() / 2., tmp.height() );
            break;
        }
        case TabWidget::eDropRectTopSplit: {
            QRect tmp = rect();
            r = QRect(tmp.left(), tmp.top(), tmp.width(), tmp.height() / 2.);
            break;
        }
        case TabWidget::eDropRectBottomSplit: {
            QRect tmp = rect();
            r = QRect(tmp.left(), tmp.top() + tmp.height() / 2., tmp.width(), tmp.height() / 2.);
            break;
        }
        case TabWidget::eDropRectNone: {
            QWidget::paintEvent(e);

            return;
        }
        }

        QPainter p(this);
        QPen pen;
        pen.setWidth(2);
        QVector<qreal> pattern;
        pattern << 3 << 3;
        pen.setDashPattern(pattern);
        pen.setBrush( QColor(243, 149, 0, 255) );
        p.setPen(pen);
        r.adjust(1, 1, -2, -2);
        p.drawRect(r);

        QWidget::paintEvent(e);
    }
};

struct TabWidgetPrivate
{
    TabWidget* _publicInterface; // can not be a smart ptr
    Gui* gui;
    QVBoxLayout* mainLayout;
    std::vector<std::pair<PanelWidget*, ScriptObject*> > tabs; // the actual tabs
    QWidget* header;
    QHBoxLayout* headerLayout;
    bool modifyingTabBar;
    TabBar* tabBar; // the header containing clickable pages
    Button* leftCornerButton;
    Button* floatButton;
    Button* closeButton;
    PanelWidget* currentWidget;
    bool drawDropRect;
    bool fullScreen;
    bool isAnchor;
    bool tabBarVisible;

    ///Used to draw drop rects
    TransparentDropRect* transparentFloatingWidget;

    ///Protects  currentWidget, fullScreen, isViewerAnchor
    mutable QMutex tabWidgetStateMutex;

    TabWidgetPrivate(TabWidget* publicInterface,
                     Gui* gui)
        : _publicInterface(publicInterface)
        , gui(gui)
        , mainLayout(0)
        , tabs()
        , header(0)
        , headerLayout(0)
        , modifyingTabBar(false)
        , tabBar(0)
        , leftCornerButton(0)
        , floatButton(0)
        , closeButton(0)
        , currentWidget(0)
        , drawDropRect(false)
        , fullScreen(false)
        , isAnchor(false)
        , tabBarVisible(true)
        , transparentFloatingWidget(0)
        , tabWidgetStateMutex()
    {
    }

    void declareTabToPython(PanelWidget* widget, const std::string& tabName);
    void removeTabToPython(PanelWidget* widget, const std::string& tabName);
};

TabWidget::TabWidget(Gui* gui,
                     QWidget* parent)
    : QFrame(parent)
    , _imp( new TabWidgetPrivate(this, gui) )
{
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);

    _imp->transparentFloatingWidget = new TransparentDropRect(this, 0);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->header = new TabWidgetHeader(this);
    QObject::connect( _imp->header, SIGNAL(mouseLeftTabBar()), this, SLOT(onTabBarMouseLeft()) );
    _imp->headerLayout = new QHBoxLayout(_imp->header);
    _imp->headerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->headerLayout->setSpacing(0);
    _imp->header->setLayout(_imp->headerLayout);


    QPixmap pixC, pixM, pixL;
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET, &pixC);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, &pixM);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON, &pixL);

    const QSize smallButtonSize( TO_DPIX(NATRON_SMALL_BUTTON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_SIZE) );
    const QSize smallButtonIconSize( TO_DPIX(NATRON_SMALL_BUTTON_ICON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_ICON_SIZE) );

    _imp->leftCornerButton = new Button(QIcon(pixL), QString(), _imp->header);
    _imp->leftCornerButton->setFixedSize(smallButtonSize);
    _imp->leftCornerButton->setIconSize(smallButtonIconSize);
    _imp->leftCornerButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr(LEFT_HAND_CORNER_BUTTON_TT), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->leftCornerButton->setFocusPolicy(Qt::NoFocus);
    _imp->headerLayout->addWidget(_imp->leftCornerButton);
    _imp->headerLayout->addSpacing(10);

    _imp->tabBar = new TabBar(this, _imp->header);

    _imp->tabBar->setShape(QTabBar::RoundedNorth);
    _imp->tabBar->setDrawBase(false);
    QObject::connect( _imp->tabBar, SIGNAL(currentChanged(int)), this, SLOT(setCurrentIndex(int)) );
    QObject::connect( _imp->tabBar, SIGNAL(mouseLeftTabBar()), this, SLOT(onTabBarMouseLeft()) );
    _imp->headerLayout->addWidget(_imp->tabBar);
    _imp->headerLayout->addStretch();
    _imp->floatButton = new Button(QIcon(pixM), QString(), _imp->header);
    _imp->floatButton->setFixedSize(smallButtonSize);
    _imp->floatButton->setIconSize(smallButtonIconSize);
    _imp->floatButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Float pane"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->floatButton->setEnabled(true);
    _imp->floatButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _imp->floatButton, SIGNAL(clicked()), this, SLOT(floatCurrentWidget()) );
    _imp->headerLayout->addWidget(_imp->floatButton);

    _imp->closeButton = new Button(QIcon(pixC), QString(), _imp->header);
    _imp->closeButton->setFixedSize(smallButtonSize);
    _imp->closeButton->setIconSize(smallButtonIconSize);
    _imp->closeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Close pane"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->closeButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _imp->closeButton, SIGNAL(clicked()), this, SLOT(closePane()) );
    _imp->headerLayout->addWidget(_imp->closeButton);


    /*adding menu to the left corner button*/
    _imp->leftCornerButton->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect( _imp->leftCornerButton, SIGNAL(clicked()), this, SLOT(createMenu()) );


    _imp->mainLayout->addWidget(_imp->header);
    _imp->mainLayout->addStretch();
}

TabWidget::~TabWidget()
{
    delete _imp->transparentFloatingWidget;
}

Gui*
TabWidget::getGui() const
{
    return _imp->gui;
}

int
TabWidget::count() const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    return _imp->tabs.size();
}

PanelWidget*
TabWidget::tabAt(int index) const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    if ( (index < 0) || ( index >= (int)_imp->tabs.size() ) ) {
        return 0;
    }

    return _imp->tabs[index].first;
}

void
TabWidget::tabAt(int index,
                 PanelWidget** w,
                 ScriptObject** obj) const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    if ( (index < 0) || ( index >= (int)_imp->tabs.size() ) ) {
        *w = 0;
        *obj = 0;

        return;
    }
    *w = _imp->tabs[index].first;
    *obj = _imp->tabs[index].second;
}

void
TabWidget::notifyGuiAboutRemoval()
{
    _imp->gui->getApp()->unregisterTabWidget(this);
}

void
TabWidget::setClosable(bool closable)
{
    _imp->closeButton->setEnabled(closable);
    _imp->floatButton->setEnabled(closable);
}

void
TabWidget::createMenu()
{
    MenuWithToolTips menu(_imp->gui);
    //QFont f(appFont,appFontSize);
    //menu.setFont(f) ;
    QPixmap pixV, pixM, pixH, pixC, pixA;

    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixV);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixH);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixM);
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixC);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixA);
    QAction* splitVerticallyAction = new QAction(QIcon(pixV), tr("Split vertical"), &menu);
    QObject::connect( splitVerticallyAction, SIGNAL(triggered()), this, SLOT(onSplitVertically()) );
    menu.addAction(splitVerticallyAction);
    QAction* splitHorizontallyAction = new QAction(QIcon(pixH), tr("Split horizontal"), &menu);
    QObject::connect( splitHorizontallyAction, SIGNAL(triggered()), this, SLOT(onSplitHorizontally()) );
    menu.addAction(splitHorizontallyAction);
    menu.addSeparator();
    QAction* floatAction = new QAction(QIcon(pixM), tr("Float pane"), &menu);
    QObject::connect( floatAction, SIGNAL(triggered()), this, SLOT(floatCurrentWidget()) );
    menu.addAction(floatAction);


    if ( (_imp->tabBar->count() == 0) ) {
        floatAction->setEnabled(false);
    }

    QAction* closeAction = new QAction(QIcon(pixC), tr("Close pane"), &menu);
    closeAction->setEnabled( _imp->closeButton->isEnabled() );
    QObject::connect( closeAction, SIGNAL(triggered()), this, SLOT(closePane()) );
    menu.addAction(closeAction);

    QAction* hideToolbar;
    if ( _imp->gui->isLeftToolBarDisplayedOnMouseHoverOnly() ) {
        hideToolbar = new QAction(tr("Show left toolbar"), &menu);
    } else {
        hideToolbar = new QAction(tr("Hide left toolbar"), &menu);
    }
    QObject::connect( hideToolbar, SIGNAL(triggered()), this, SLOT(onHideLeftToolBarActionTriggered()) );
    menu.addAction(hideToolbar);

    /*QAction* hideTabbar;
    if (_imp->tabBarVisible) {
        hideTabbar = new QAction(tr("Hide tabs header"), &menu);
    } else {
        hideTabbar = new QAction(tr("Show tabs header"), &menu);
    }
    QObject::connect( hideTabbar, SIGNAL(triggered()), this, SLOT(onShowHideTabBarActionTriggered()) );
    menu.addAction(hideTabbar);*/
    menu.addSeparator();
    menu.addAction( tr("New viewer"), this, SLOT(addNewViewer()) );
    menu.addAction( tr("New histogram"), this, SLOT(newHistogramHere()) );
    menu.addAction( tr("Node graph here"), this, SLOT(moveNodeGraphHere()) );
    menu.addAction( tr("Dope Sheet Editor here"), this, SLOT(moveAnimationModuleEditorHere()) );
    menu.addAction( tr("Properties bin here"), this, SLOT(movePropertiesBinHere()) );
    menu.addAction( tr("Script editor here"), this, SLOT(moveScriptEditorHere()) );
    menu.addAction( tr("Progress Panel here"), this, SLOT(moveProgressPanelHere()) );


    std::list<PyPanelI*> userPanels = _imp->gui->getApp()->getPyPanelsSerialization();
    if ( !userPanels.empty() ) {
        Menu* userPanelsMenu = new Menu(tr("User panels"), &menu);
        //userPanelsMenu->setFont(f);
        menu.addAction( userPanelsMenu->menuAction() );


        for (std::list<PyPanelI*>::iterator it = userPanels.begin(); it != userPanels.end(); ++it) {
            QAction* pAction = new QAction(tr("%1 here").arg( (*it)->getPanelLabel() ), userPanelsMenu);
            QObject::connect( pAction, SIGNAL(triggered()), this, SLOT(onUserPanelActionTriggered()) );
            pAction->setData((*it)->getPanelScriptName());
            userPanelsMenu->addAction(pAction);
        }
    }
    menu.addSeparator();

    QAction* isAnchorAction = new QAction(QIcon(pixA), tr("Set this as anchor"), &menu);
    isAnchorAction->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The anchor pane is where viewers will be created by default."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    isAnchorAction->setCheckable(true);
    bool isVA = isAnchor();
    isAnchorAction->setChecked(isVA);
    isAnchorAction->setEnabled(!isVA);
    QObject::connect( isAnchorAction, SIGNAL(triggered()), this, SLOT(onSetAsAnchorActionTriggered()) );
    menu.addAction(isAnchorAction);

    menu.exec( _imp->leftCornerButton->mapToGlobal( QPoint(0, 0) ) );
} // TabWidget::createMenu

void
TabWidget::onUserPanelActionTriggered()
{
    QAction* s = qobject_cast<QAction*>( sender() );

    if (!s) {
        return;
    }

    const RegisteredTabs& tabs = _imp->gui->getRegisteredTabs();
    RegisteredTabs::const_iterator found = tabs.find( s->data().toString().toStdString() );
    if ( found != tabs.end() ) {
        moveTab(found->second.first, found->second.second);
    }
}

void
TabWidget::onHideLeftToolBarActionTriggered()
{
    _imp->gui->setLeftToolBarDisplayedOnMouseHoverOnly( !_imp->gui->isLeftToolBarDisplayedOnMouseHoverOnly() );
}

void
TabWidget::moveToNextTab()
{
    int nextTab = -1;
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);

        for (U32 i = 0; i < _imp->tabs.size(); ++i) {
            if (_imp->tabs[i].first == _imp->currentWidget) {
                if (i == _imp->tabs.size() - 1) {
                    nextTab = 0;
                } else {
                    nextTab = i + 1;
                }
            }
        }
    }

    if (nextTab != -1) {
        setCurrentIndex(nextTab);
    }
}

void
TabWidget::moveToPreviousTab()
{
    int prevTab = -1;
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);

        for (U32 i = 0; i < _imp->tabs.size(); ++i) {
            if (_imp->tabs[i].first == _imp->currentWidget) {
                if (i == 0) {
                    prevTab = _imp->tabs.size() - 1;
                } else {
                    prevTab = i - 1;
                }
            }
        }
    }

    if (prevTab != -1) {
        setCurrentIndex(prevTab);
    }
}

void
TabWidget::tryCloseFloatingPane()
{
    QWidget* parent = parentWidget();

    while (parent) {
        FloatingWidget* fw = dynamic_cast<FloatingWidget*>(parent);
        if (fw) {
            fw->close();

            return;
        }
        parent = parent->parentWidget();
    }
}

static void
getOtherSplitWidget(Splitter* parentSplitter,
                    QWidget* thisWidget,
                    QWidget* & other)
{
    for (int i = 0; i < parentSplitter->count(); ++i) {
        QWidget* w = parentSplitter->widget(i);
        if (w == thisWidget) {
            continue;
        }
        other = w;
        break;
    }
}

/**
 * @brief Deletes container and move the other split (the one that is not this) to the parent of container.
 * If the parent container is a splitter then we will insert it instead of the container.
 * Otherwise if the parent is a floating window we will insert it as embedded widget of the window.
 **/
void
TabWidget::closeSplitterAndMoveOtherSplitToParent(Splitter* container)
{
    QWidget* otherSplit = 0;

    /*identifying the other split*/
    getOtherSplitWidget(container, this, otherSplit);

    Splitter* parentSplitter = dynamic_cast<Splitter*>( container->parentWidget() );
    QWidget* parent = container->parentWidget();
    FloatingWidget* parentWindow = 0;
    while (parent) {
        parentWindow = dynamic_cast<FloatingWidget*>(parent);
        if (parentWindow) {
            break;
        }
        parent = parent->parentWidget();
    }

    assert(parentSplitter || parentWindow);

    if (parentSplitter) {
        QList<int> mainContainerSizes = parentSplitter->sizes();

        /*Removing the splitter container from its parent*/
        int containerIndexInParentSplitter = parentSplitter->indexOf(container);
        parentSplitter->removeChild_mt_safe(container);

        /*moving the other split to the mainContainer*/
        if (otherSplit) {
            parentSplitter->insertChild_mt_safe(containerIndexInParentSplitter, otherSplit);
            otherSplit->setVisible(true);
        }

        /*restore the main container sizes*/
        parentSplitter->setSizes_mt_safe(mainContainerSizes);
    } else {
        if (otherSplit) {
            assert(parentWindow);
            parentWindow->removeEmbeddedWidget();
            parentWindow->setWidget(otherSplit);
        }
    }

    /*Remove the container from everywhere*/
    _imp->gui->getApp()->unregisterSplitter(container);
    // container->setParent(NULL);
    container->deleteLater();
}

void
TabWidget::closePane()
{
    if (!_imp->gui) {
        return;
    }

    QWidget* parent = parentWidget();
    while (parent) {
        FloatingWidget* isFloating = dynamic_cast<FloatingWidget*>(parent);
        if ( isFloating && (isFloating->getEmbeddedWidget() == this) ) {
            isFloating->close();
            break;
        }
        parent = parent->parentWidget();
    }


    /*Removing it from the _panes vector*/

    _imp->gui->getApp()->unregisterTabWidget(this);


    ///This is the TabWidget to which we will move all our splits.
    TabWidget* tabToTransferTo = 0;

    ///Move living tabs to the viewer anchor TabWidget, this is better than destroying them.
    const std::list<TabWidgetI*> & panes = _imp->gui->getApp()->getTabWidgetsSerialization();
    for (std::list<TabWidgetI*>::const_iterator it = panes.begin(); it != panes.end(); ++it) {
        if ( (*it != this) && (*it)->isAnchor() ) {
            tabToTransferTo = dynamic_cast<TabWidget*>(*it);
            break;
        }
    }


    if (tabToTransferTo) {
        ///move this tab's splits
        while (count() > 0) {
            PanelWidget* w;
            ScriptObject* o;
            tabAt(0, &w, &o);
            if (w && o) {
                tabToTransferTo->moveTab(w, o);
            }
        }
    } else {
        while (count() > 0) {
            PanelWidget* w = tabAt(0);
            if (w) {
                removeTab( w, true );
            }
        }
    }

    ///Hide this
    setVisible(false);

    Splitter* container = dynamic_cast<Splitter*>( parentWidget() );
    if (container) {
        closeSplitterAndMoveOtherSplitToParent(container);
    }
} // closePane

void
TabWidget::floatPane(QPoint* position)
{
    QWidget* parent = parentWidget();

    while (parent) {
        FloatingWidget* isParentFloating = dynamic_cast<FloatingWidget*>(parent);
        if (isParentFloating) {
            return;
        }
        parent = parent->parentWidget();
    }


    FloatingWidget* floatingW = new FloatingWidget(_imp->gui, _imp->gui);
    Splitter* parentSplitter = dynamic_cast<Splitter*>( parentWidget() );
    //setParent(0);
    if (parentSplitter) {
        closeSplitterAndMoveOtherSplitToParent(parentSplitter);
    }


    floatingW->setWidget(this);

    if (position) {
        floatingW->move(*position);
    }
    _imp->gui->getApp()->registerFloatingWindow(floatingW);
    _imp->gui->checkNumberOfNonFloatingPanes();
}

void
TabWidget::addNewViewer()
{
    _imp->gui->setNextViewerAnchor(this);

    NodeGraph* lastFocusedGraph = _imp->gui->getLastSelectedGraph();
    NodeGraph* graph = lastFocusedGraph ? lastFocusedGraph : _imp->gui->getNodeGraph();
    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }
    CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_VIEWER_GROUP, graph->getGroup() ));
    args->setProperty<bool>(kCreateNodeArgsPropSubGraphOpened, false);
    _imp->gui->getApp()->createNode(args);
}

void
TabWidget::moveNodeGraphHere()
{
    NodeGraph* graph = _imp->gui->getNodeGraph();

    moveTab(graph, graph);
}


void
TabWidget::moveAnimationModuleEditorHere()
{
    AnimationModuleEditor *editor = _imp->gui->getAnimationModuleEditor();

    moveTab(editor, editor);
}

void
TabWidget::newHistogramHere()
{
    Histogram* h = _imp->gui->addNewHistogram();

    appendTab(h, h);

    _imp->gui->getApp()->triggerAutoSave();
}

/*Get the header name of the tab at index "index".*/
QString
TabWidget::getTabLabel(int index) const
{
    QMutexLocker k(&_imp->tabWidgetStateMutex);

    if ( (index < 0) || ( index >= _imp->tabBar->count() ) ) {
        return QString();
    }

    return QString::fromUtf8( _imp->tabs[index].second->getLabel().c_str() );
}

QString
TabWidget::getTabLabel(PanelWidget* tab) const
{
    QMutexLocker k(&_imp->tabWidgetStateMutex);

    for (U32 i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].first == tab) {
            return QString::fromUtf8( _imp->tabs[i].second->getLabel().c_str() );
        }
    }

    return QString();
}

void
TabWidget::setTabLabel(PanelWidget* tab,
                       const QString & name)
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    for (U32 i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].first == tab) {
            _imp->tabBar->setTabText(i, name);
        }
    }
}

void
TabWidget::floatCurrentWidget()
{
    if ( _imp->tabs.empty() ) {
        return;
    }
    if ( !_imp->closeButton->isEnabled() ) {
        ///Make a new tab widget and float it instead
        TabWidget* newPane = new TabWidget(_imp->gui, _imp->gui);
        _imp->gui->getApp()->registerTabWidget(newPane);
        newPane->setScriptName( _imp->gui->getApp()->getAvailablePaneName().toStdString() );

        PanelWidget* w;
        ScriptObject* o;
        currentWidget(&w, &o);
        if (w && o) {
            newPane->moveTab(w, o);
        }
        newPane->floatPane();
    } else {
        ///Float this tab
        floatPane();
    }
}

void
TabWidget::closeCurrentWidget()
{
    PanelWidget* w = currentWidget();

    if (!w) {
        return;
    }
    removeTab(w, true);
}

void
TabWidget::closeTab(int index)
{
    PanelWidget *w = tabAt(index);

    if (!w) {
        return;
    }
    removeTab(w, true);

    _imp->gui->getApp()->triggerAutoSave();
}

void
TabWidget::movePropertiesBinHere()
{
    moveTab(_imp->gui->getPropertiesBin(), _imp->gui->getPropertiesBin());
}

void
TabWidget::moveScriptEditorHere()
{
    moveTab(_imp->gui->getScriptEditor(), _imp->gui->getScriptEditor());
}

void
TabWidget::moveProgressPanelHere()
{
    moveTab(_imp->gui->getProgressPanel(), _imp->gui->getProgressPanel());
}

TabWidget*
TabWidget::splitInternal(bool autoSave,
                         Qt::Orientation orientation)
{
    QWidget* parent = parentWidget();
    FloatingWidget* parentIsFloating = 0;

    while (parent) {
        parentIsFloating = dynamic_cast<FloatingWidget*>(parent);
        if (parentIsFloating) {
            break;
        }
        parent = parent->parentWidget();
    }

    Splitter* parentIsSplitter = dynamic_cast<Splitter*>( parentWidget() );

    assert(parentIsSplitter || parentIsFloating);

    /*We need to know the position in the container layout of the old tab widget*/
    int oldIndexInParentSplitter = -1;
    QList<int> oldSizeInParentSplitter;
    if (parentIsSplitter) {
        oldIndexInParentSplitter = parentIsSplitter->indexOf(this);
        oldSizeInParentSplitter = parentIsSplitter->sizes();
    } else {
        assert(parentIsFloating);
        parentIsFloating->removeEmbeddedWidget();
    }

    Splitter* newSplitter = new Splitter(Qt::Horizontal, _imp->gui,0);
    newSplitter->setContentsMargins(0, 0, 0, 0);
    newSplitter->setOrientation(orientation);
    _imp->gui->getApp()->registerSplitter(newSplitter);

    /*Add this to the new splitter*/
    newSplitter->addWidget_mt_safe(this);

    /*Adding now a new TabWidget*/
    TabWidget* newTab = new TabWidget(_imp->gui, newSplitter);
    _imp->gui->getApp()->registerTabWidget(newTab);
    newTab->setScriptName( _imp->gui->getApp()->getAvailablePaneName().toStdString() );
    newSplitter->insertChild_mt_safe(-1, newTab);

    /*Resize the whole thing so each split gets the same size*/
    QSize splitterSize;
    if (parentIsSplitter) {
        splitterSize = newSplitter->sizeHint();
    } else {
        splitterSize = parentIsFloating->size();
    }

    QList<int> sizes;
    if (orientation == Qt::Vertical) {
        sizes << splitterSize.height() / 2;
        sizes << splitterSize.height() / 2;
    } else if (orientation == Qt::Horizontal) {
        sizes << splitterSize.width() / 2;
        sizes << splitterSize.width() / 2;
    } else {
        assert(false);
    }
    newSplitter->setSizes_mt_safe(sizes);

    if (parentIsSplitter) {
        /*Inserting back the new splitter at the original index*/
        parentIsSplitter->insertChild_mt_safe(oldIndexInParentSplitter, newSplitter);

        /*restore the container original sizes*/
        parentIsSplitter->setSizes_mt_safe(oldSizeInParentSplitter);
    } else {
        assert(parentIsFloating);
        parentIsFloating->setWidget(newSplitter);
        parentIsFloating->resize(splitterSize);
    }


    if (!_imp->gui->getApp()->getProject()->isLoadingProject() && autoSave) {
        _imp->gui->getApp()->triggerAutoSave();
    }

    return newTab;
} // splitInternal

TabWidget*
TabWidget::splitHorizontally(bool autoSave)
{
    return splitInternal(autoSave, Qt::Horizontal);
}

TabWidget*
TabWidget::splitVertically(bool autoSave)
{
    return splitInternal(autoSave, Qt::Vertical);
}

bool
TabWidget::appendTab(PanelWidget* widget,
                     ScriptObject* object)
{
    QString title;
    QIcon icon = widget->getIcon();
    //if (icon.isNull()) {
        title = QString::fromUtf8( object->getLabel().c_str() );
    //}
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);

        ///If we do not know the tab ignore it
        std::string name = object->getScriptName();
        if ( name.empty()) {
            return false;
        }

        /*registering this tab for drag&drop*/
        _imp->gui->registerTab(widget, object);

        _imp->tabs.push_back( std::make_pair(widget, object) );
        //widget->setParent(this);
        _imp->modifyingTabBar = true;

        _imp->tabBar->addTab( icon, title );
        _imp->tabBar->updateGeometry(); //< necessary
        _imp->modifyingTabBar = false;
        if (_imp->tabs.size() == 1) {
            for (int i = 0; i < _imp->mainLayout->count(); ++i) {
                QSpacerItem* item = dynamic_cast<QSpacerItem*>( _imp->mainLayout->itemAt(i) );
                if (item) {
                    _imp->mainLayout->removeItem(item);
                }
            }
        }
//        if (!widget->isVisible()) {
//            widget->setVisible(true);
//        }
        _imp->floatButton->setEnabled(true);
    }
    _imp->declareTabToPython( widget, object->getScriptName() );
    setCurrentIndex(_imp->tabs.size() - 1);

    return true;
}

void
TabWidget::insertTab(int index,
                     PanelWidget* widget,
                     ScriptObject* object)
{
    QString title;
    QIcon icon = widget->getIcon();
    //if (icon.isNull()) {
        title = QString::fromUtf8( object->getLabel().c_str() );
    //}
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    if ( (U32)index < _imp->tabs.size() ) {
        /*registering this tab for drag&drop*/
        _imp->gui->registerTab(widget, object);

        _imp->tabs.insert( _imp->tabs.begin() + index, std::make_pair(widget, object) );
        _imp->modifyingTabBar = true;



        _imp->tabBar->insertTab(index, icon, title);
        _imp->tabBar->updateGeometry(); //< necessary
        _imp->modifyingTabBar = false;

        QWidget* w = widget->getWidget();
        if ( !w->isVisible() ) {
            w->setVisible(true);
        }

        l.unlock();
        _imp->declareTabToPython( widget, object->getScriptName() );
    } else {
        l.unlock();
        appendTab(widget, object);
    }
    _imp->floatButton->setEnabled(true);
}



PanelWidget*
TabWidget::removeTab(int index,
                     bool userAction)
{
    PanelWidget* tab;
    ScriptObject* obj;

    tabAt(index, &tab, &obj);
    if (!tab || !obj) {
        return 0;
    }

    ViewerTab* isViewer = dynamic_cast<ViewerTab*>(tab);
    Histogram* isHisto = dynamic_cast<Histogram*>(tab);
    NodeGraph* isGraph = dynamic_cast<NodeGraph*>(tab);
    int newIndex = -1;
    if (isGraph) {
        /*
           If the tab is a group pane, try to find the parent group pane in this pane and set it active
         */
        NodeCollectionPtr collect = isGraph->getGroup();
        assert(collect);
        NodeGroupPtr isGroup = toNodeGroup( collect );
        if (isGroup) {
            collect = isGroup->getNode()->getGroup();
            if (collect) {
                NodeGraph* parentGraph = 0;
                isGroup  = toNodeGroup( collect );
                if (isGroup) {
                    parentGraph = dynamic_cast<NodeGraph*>( isGroup->getNodeGraph() );
                } else {
                    parentGraph = getGui()->getNodeGraph();
                }
                assert(parentGraph);
                for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
                    if (_imp->tabs[i].first == parentGraph) {
                        newIndex = i;
                        break;
                    }
                }
            }
        }
    }

    QWidget* w = tab->getWidget();

    if ( _imp->tabBar->hasClickFocus() && (_imp->currentWidget == tab) ) {
        tab->removeClickFocus();
    }

    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);

        _imp->tabs.erase(_imp->tabs.begin() + index);
        _imp->modifyingTabBar = true;
        _imp->tabBar->removeTab(index);
        _imp->modifyingTabBar = false;
        if (_imp->tabs.size() > 0) {
            l.unlock();
            int i;
            if (newIndex == -1) {
                i = (index - 1 >= 0) ? index - 1 : 0;
            } else {
                i = newIndex;
            }
            setCurrentIndex(i);
            l.relock();
        } else {
            _imp->currentWidget = 0;
            _imp->mainLayout->addStretch();
            if ( _imp->gui && !_imp->gui->isDraggingPanel() ) {
                if ( dynamic_cast<FloatingWidget*>( parentWidget() ) ) {
                    l.unlock();
                    tryCloseFloatingPane();
                    l.relock();
                }
            }
        }
        //tab->setParent(_imp->gui);
    }


    _imp->removeTabToPython( tab, obj->getScriptName() );

    if (userAction) {
        if (isViewer && _imp->gui) {
            /*special care is taken if this is a viewer: we also
               need to delete the viewer node.*/
            _imp->gui->removeViewerTab(isViewer, false, false);
        } else if (isHisto && _imp->gui) {
            _imp->gui->removeHistogram(isHisto);

            //Return because at this point isHisto is invalid
            return tab;
        } else {
            ///Do not delete unique widgets such as the properties bin, node graph or curve editor
            w->setVisible(false);
        }
    } else {
        w->setVisible(false);
    }
    if ( isGraph && _imp->gui && (_imp->gui->getLastSelectedGraph() == isGraph) ) {
        _imp->gui->setLastSelectedGraph(0);
    }
    w->setParent(_imp->gui);


    return tab;
} // TabWidget::removeTab

void
TabWidget::removeTab(PanelWidget* widget,
                     bool userAction)
{
    int index = -1;

    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);
        for (U32 i = 0; i < _imp->tabs.size(); ++i) {
            if (_imp->tabs[i].first == widget) {
                index = i;
                break;
            }
        }
    }

    if (index != -1) {
        PanelWidget* tab = removeTab(index, userAction);
        assert(tab == widget);
        Q_UNUSED(tab);
    }
}

void
TabWidget::setCurrentWidget(PanelWidget* w)
{
    int index = -1;
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);
        for (U32 i = 0; i < _imp->tabs.size(); ++i) {
            if (_imp->tabs[i].first == w) {
                index = i;
                break;
            }
        }
    }

    if (index != -1) {
        setCurrentIndex(index);
    }
}

void
TabWidget::setCurrentIndex(int index)
{
    if (_imp->modifyingTabBar) {
        return;
    }
    PanelWidget* tab = tabAt(index);
    if (!tab) {
        return;
    }
    QWidget* tabW = tab->getWidget();
    PanelWidget* curWidget;
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);
        curWidget = _imp->currentWidget;
    }
    bool hadFocus = false;
    if ( curWidget && _imp->tabBar->hasClickFocus() ) {
        hadFocus = true;
        curWidget->removeClickFocus();
    }
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);

        /*Remove previous widget if any*/

        if (_imp->currentWidget) {
            QWidget* w = _imp->currentWidget->getWidget();
            QObject::disconnect( w, SIGNAL(destroyed()), this, SLOT(onCurrentTabDeleted()) );
            w->setVisible(false);
            _imp->mainLayout->removeWidget(w);
        }
        _imp->currentWidget = tab;
        curWidget = _imp->currentWidget;
    }

    _imp->mainLayout->addWidget(tabW);
    QObject::connect( tabW, SIGNAL(destroyed()), this, SLOT(onCurrentTabDeleted()) );
    if ( !tabW->isVisible() ) {
        tabW->setVisible(true);
    }
    //tab->setParent(this);
    _imp->modifyingTabBar = true;
    _imp->tabBar->setCurrentIndex(index);
    _imp->modifyingTabBar = false;

    if (hadFocus) {
        curWidget->takeClickFocus();
    }
    tab->onPanelMadeCurrent();
} // TabWidget::setCurrentIndex

void
TabWidget::onCurrentTabDeleted()
{
    QObject* s = sender();
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    if ( !_imp->currentWidget || ( s != _imp->currentWidget->getWidget() ) ) {
        return;
    }
    _imp->currentWidget = 0;
    for (U32 i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].first->getWidget() == s) {
            _imp->tabs.erase(_imp->tabs.begin() + i);
            _imp->modifyingTabBar = true;
            _imp->tabBar->removeTab(i);
            _imp->modifyingTabBar = false;
            if (_imp->tabs.size() > 0) {
                l.unlock();
                setCurrentIndex(0);
                l.relock();
            } else {
                _imp->currentWidget = 0;
                _imp->mainLayout->addStretch();
                if ( !_imp->gui->isDraggingPanel() ) {
                    if ( dynamic_cast<FloatingWidget*>( parentWidget() ) ) {
                        l.unlock();
                        tryCloseFloatingPane();
                        l.relock();
                    }
                }
            }
            break;
        }
    }
}

void
TabWidget::paintEvent(QPaintEvent* e)
{
    QFrame::paintEvent(e);

#ifdef DEBUG
    // We should draw something indicating that this pane is the default pane for new viewers if it has no tabs, maybe a pixmap or something
    bool mustDraw = false;
    {
        QMutexLocker k(&_imp->tabWidgetStateMutex);
        mustDraw = _imp->tabs.empty() && _imp->isAnchor;
    }
    if (mustDraw) {
        QPainter p(this);
        QPen pen = p.pen();
        pen.setColor( QColor(200, 200, 200) );
        p.setPen(pen);
        QFont f = font();
        f.setPointSize(50);
        p.setFont(f);
        QString text = tr("(Viewer Pane)");
        QFontMetrics fm(f);
        QPointF pos(width() / 2., height() / 2.);
        pos.rx() -= (fm.width(text) / 2.);
        pos.ry() -= (fm.height() / 2.);
        p.drawText(pos, text);
    }
#endif
}

void
TabWidget::dropEvent(QDropEvent* e)
{
    e->accept();
    QString name( QString::fromUtf8( e->mimeData()->data( QString::fromUtf8("Tab") ) ) );
    PanelWidget* w;
    ScriptObject* obj;
    _imp->gui->findExistingTab(name.toStdString(), &w, &obj);
    if (w && obj) {
        TabWidget* where = 0;
        if (_imp->transparentFloatingWidget->getDropType() == eDropRectLeftSplit) {
            splitHorizontally();
            where = this;
        } else if (_imp->transparentFloatingWidget->getDropType() == eDropRectRightSplit) {
            where = splitHorizontally();
        } else {
            where = this;
        }
        assert(where);
        where->moveTab(w, obj);
    }
    _imp->drawDropRect = false;
    setFrameShape(QFrame::NoFrame);
    update();
}

TabBar::TabBar(TabWidget* tabWidget,
               QWidget* parent)
    : QTabBar(parent)
    , _dragPos()
    , _dragPix(0)
    , _tabWidget(tabWidget)
    , _processingLeaveEvent(false)
    , mouseOverFocus(false)
    , clickFocus(false)
{
    setTabsClosable(true);
    setMouseTracking(true);
    QObject::connect( this, SIGNAL(tabCloseRequested(int)), tabWidget, SLOT(closeTab(int)) );
}

QSize
TabBar::sizeHint() const
{
    return QTabBar::sizeHint();
}

QSize
TabBar::minimumSizeHint() const
{
    return QTabBar::minimumSizeHint();
}

void
TabBar::setMouseOverFocus(bool focus)
{
    if (mouseOverFocus != focus) {
        mouseOverFocus = focus;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
TabBar::setClickFocus(bool focus)
{
    if (clickFocus != focus) {
        clickFocus = focus;
        style()->unpolish(this);
        style()->polish(this);
        update();
    }
}

void
TabBar::mousePressEvent(QMouseEvent* e)
{
    if ( buttonDownIsLeft(e) ) {
        _dragPos = e->pos();
    }
    QTabBar::mousePressEvent(e);
}

void
TabBar::leaveEvent(QEvent* e)
{
    if (_processingLeaveEvent) {
        QTabBar::leaveEvent(e);
    } else {
        _processingLeaveEvent = true;
        Q_EMIT mouseLeftTabBar();
        QTabBar::leaveEvent(e);
        _processingLeaveEvent = false;
    }
}

/**
 * @brief Given the widget w, tries to find recursively if a parent is a tabwidget and returns it
 **/
static TabWidget*
findTabWidgetRecursive(QWidget* w)
{
    assert(w);
    TabWidget* isTab = dynamic_cast<TabWidget*>(w);
    TransparentDropRect* isTransparent = dynamic_cast<TransparentDropRect*>(w);
    if (isTab) {
        return isTab;
    } else if (isTransparent) {
        return isTransparent->getPane();
    } else {
        if ( w->parentWidget() ) {
            return findTabWidgetRecursive( w->parentWidget() );
        } else {
            return 0;
        }
    }
}

void
TabBar::mouseMoveEvent(QMouseEvent* e)
{
    // If the left button isn't pressed anymore then return
    if ( !buttonDownIsLeft(e) ) {
        return;
    }
    // If the distance is too small then return
    if ( (e->pos() - _dragPos).manhattanLength() < QApplication::startDragDistance() ) {
        return;
    }

    if ( _tabWidget->getGui()->isDraggingPanel() ) {
        const QPoint & globalPos = e->globalPos();
        QWidget* widgetUnderMouse = qApp->widgetAt(globalPos);
        if (widgetUnderMouse) {
            TabWidget* topLvlTabWidget = findTabWidgetRecursive(widgetUnderMouse);
            if (topLvlTabWidget) {
                QPointF localPos = topLvlTabWidget->mapFromGlobal(globalPos);
                int w = widgetUnderMouse->width();
                int h = widgetUnderMouse->height();
                TabWidget::DropRectType dropType = TabWidget::eDropRectAll;
                if ( (localPos.x() < 0) || (localPos.x() >= w) || (localPos.y() < 0) || (localPos.y() >= h) ) {
                    dropType = TabWidget::eDropRectNone;
                } else if (localPos.x() / (double)w < TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING) {
                    dropType = TabWidget::eDropRectLeftSplit;
                } else if ( localPos.x() / (double)w > (1. - TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING) ) {
                    dropType = TabWidget::eDropRectRightSplit;
                } else if (localPos.y() / (double)h < TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING) {
                    dropType = TabWidget::eDropRectTopSplit;
                } else if ( localPos.y() / (double)h > (1. - TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING) ) {
                    dropType = TabWidget::eDropRectBottomSplit;
                }


                const std::list<TabWidgetI*> panes = _tabWidget->getGui()->getApp()->getTabWidgetsSerialization();
                for (std::list<TabWidgetI*>::const_iterator it = panes.begin(); it != panes.end(); ++it) {
                    TabWidget* isTab = dynamic_cast<TabWidget*>(*it);
                    if (!isTab) {
                        continue;
                    }
                    if (*it == topLvlTabWidget) {
                        isTab->setDrawDropRect(dropType, true);
                    } else {
                        isTab->setDrawDropRect(TabWidget::eDropRectNone, false);
                    }
                }
            }
        }

        _dragPix->update(globalPos);
        QTabBar::mouseMoveEvent(e);

        return;
    }
    int selectedTabIndex = tabAt( e->pos() );
    if (selectedTabIndex != -1) {
        QPixmap pixmap = makePixmapForDrag(selectedTabIndex);

        _tabWidget->startDragTab(selectedTabIndex);

        _dragPix = new DragPixmap( pixmap, e->pos() );
        _dragPix->update( e->globalPos() );
        _dragPix->show();
        grabMouse();
    }
    QTabBar::mouseMoveEvent(e);
} // TabBar::mouseMoveEvent

QPixmap
TabBar::makePixmapForDrag(int index)
{
    std::vector<std::pair<QString, QIcon > > tabs;

    for (int i = 0; i < count(); ++i) {
        tabs.push_back( std::make_pair( tabText(i), tabIcon(i) ) );
    }

    //remove all tabs
    while (count() > 0) {
        removeTab(0);
    }

    ///insert just the tab we want to screen shot
    addTab(tabs[index].second, tabs[index].first);

    QPixmap currentTabPixmap =  Gui::screenShot( _tabWidget->tabAt(index)->getWidget() );
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QPixmap tabBarPixmap = QPixmap::grabWidget(this);
#else
    QPixmap tabBarPixmap = grab();
#endif

    ///re-insert all the tabs into the tab bar
    removeTab(0);

    for (U32 i = 0; i < tabs.size(); ++i) {
        addTab(tabs[i].second, tabs[i].first);
    }


    QImage tabBarImg = tabBarPixmap.toImage();
    QImage currentTabImg = currentTabPixmap.toImage();

    // Prevent a bug with grabWidget and retina display on Qt4
    // See https://bugreports.qt.io/browse/QTBUG-23870
    _tabWidget->getGui()->scaleImageToAdjustDPI(&tabBarImg);
    _tabWidget->getGui()->scaleImageToAdjustDPI(&currentTabImg);


    //now we just put together the 2 pixmaps and set it with mid transparency
    QImage ret(currentTabImg.width(), currentTabImg.height() + tabBarImg.height(), QImage::Format_ARGB32_Premultiplied);

    for (int y = 0; y < tabBarImg.height(); ++y) {
        QRgb* src_pixels = reinterpret_cast<QRgb*>( tabBarImg.scanLine(y) );
        for (int x = 0; x < ret.width(); ++x) {
            if ( x < tabBarImg.width() ) {
                QRgb pix = src_pixels[x];
                ret.setPixel( x, y, qRgba(qRed(pix), qGreen(pix), qBlue(pix), 255) );
            } else {
                ret.setPixel( x, y, qRgba(0, 0, 0, 128) );
            }
        }
    }

    for (int y = 0; y < currentTabImg.height(); ++y) {
        QRgb* src_pixels = reinterpret_cast<QRgb*>( currentTabImg.scanLine(y) );
        for (int x = 0; x < ret.width(); ++x) {
            QRgb pix = src_pixels[x];
            ret.setPixel( x, y + tabBarImg.height(), qRgba(qRed(pix), qGreen(pix), qBlue(pix), 255) );
        }
    }

    return QPixmap::fromImage(ret);
} // makePixmapForDrag

void
TabBar::mouseReleaseEvent(QMouseEvent* e)
{
    bool hasClosedTabWidget = false;

    if ( _tabWidget->getGui()->isDraggingPanel() ) {
        releaseMouse();
        const QPoint & p = e->globalPos();
        hasClosedTabWidget = _tabWidget->stopDragTab(p);
        _dragPix->hide();
        delete _dragPix;
        _dragPix = 0;
    }
    if (!hasClosedTabWidget) {
        QTabBar::mouseReleaseEvent(e);
    }
}

bool
TabWidget::stopDragTab(const QPoint & globalPos)
{
    if (count() == 0) {
        tryCloseFloatingPane();
    }

    QSize draggedPanelSize;
    PanelWidget* draggedPanel = _imp->gui->stopDragPanel(&draggedPanelSize);
    if (!draggedPanel) {
        return false;
    }
    ScriptObject* obj = 0;
    const RegisteredTabs& tabs = _imp->gui->getRegisteredTabs();
    for (RegisteredTabs::const_iterator it = tabs.begin(); it != tabs.end(); ++it) {
        if (it->second.first == draggedPanel) {
            obj = it->second.second;
            break;
        }
    }
    if (!obj) {
        return false;
    }


    const std::list<TabWidgetI*> panes = _imp->gui->getApp()->getTabWidgetsSerialization();
    QWidget* widgetUnderMouse = qApp->widgetAt(globalPos);
    bool foundTabWidgetUnderneath = false;
    if (widgetUnderMouse) {
        TabWidget* topLvlTabWidget = findTabWidgetRecursive(widgetUnderMouse);
        if (topLvlTabWidget) {
            QPointF localPos = topLvlTabWidget->mapFromGlobal(globalPos);
            int w = widgetUnderMouse->width();
            int h = widgetUnderMouse->height();
            TabWidget::DropRectType dropType = TabWidget::eDropRectAll;
            if ( (localPos.x() < 0) || (localPos.x() >= w) || (localPos.y() < 0) || (localPos.y() >= h) ) {
                dropType = TabWidget::eDropRectNone;
            } else if (localPos.x() / (double)w < TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING) {
                dropType = TabWidget::eDropRectLeftSplit;
            } else if ( localPos.x() / (double)w > (1. - TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING) ) {
                dropType = TabWidget::eDropRectRightSplit;
            } else if (localPos.y() / (double)h < TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING) {
                dropType = TabWidget::eDropRectTopSplit;
            } else if ( localPos.y() / (double)h > (1. - TAB_DRAG_WIDGET_PERCENT_FOR_SPLITTING) ) {
                dropType = TabWidget::eDropRectBottomSplit;
            }

            TabWidget* where = 0;
            if (dropType == eDropRectLeftSplit) {
                topLvlTabWidget->splitHorizontally();
                where = topLvlTabWidget;
            } else if (dropType == eDropRectRightSplit) {
                where = topLvlTabWidget->splitHorizontally();
            } else if (dropType == eDropRectTopSplit) {
                topLvlTabWidget->splitVertically();
                where =  topLvlTabWidget;
            } else if (dropType == eDropRectBottomSplit) {
                where = topLvlTabWidget->splitVertically();
            } else {
                where = topLvlTabWidget;
            }
            assert(where);

            where->appendTab(draggedPanel, obj);
            foundTabWidgetUnderneath = true;
        }
    }

    bool ret = false;
    if (!foundTabWidgetUnderneath) {
        ///if we reach here that means the mouse is not over any tab widget, then float the panel


        QPoint windowPos = globalPos;
        FloatingWidget* floatingW = new FloatingWidget(_imp->gui, _imp->gui);
        TabWidget* newTab = new TabWidget(_imp->gui, floatingW);
        _imp->gui->getApp()->registerTabWidget(newTab);
        newTab->setScriptName( _imp->gui->getApp()->getAvailablePaneName().toStdString() );
        newTab->appendTab(draggedPanel, obj);
        floatingW->setWidget(newTab);
        floatingW->move(windowPos);
        floatingW->resize(draggedPanelSize);
        _imp->gui->getApp()->registerFloatingWindow(floatingW);
        _imp->gui->checkNumberOfNonFloatingPanes();

        bool isClosable = _imp->closeButton->isEnabled();
        if ( isClosable && (count() == 0) ) {
            closePane();
            ret = true;
        }
    }


    for (std::list<TabWidgetI*>::const_iterator it = panes.begin(); it != panes.end(); ++it) {
        TabWidget* isTab = dynamic_cast<TabWidget*>(*it);
        if (!isTab) {
            continue;
        }
        isTab->setDrawDropRect(TabWidget::eDropRectNone, false);
    }

    return ret;
} // TabWidget::stopDragTab

void
TabWidget::startDragTab(int index)
{
    PanelWidget* selectedTab = tabAt(index);

    if (!selectedTab) {
        return;
    }
    QWidget* w = selectedTab->getWidget();
    w->setParent(this);

    _imp->gui->startDragPanel(selectedTab);

    removeTab(selectedTab, false);
    w->hide();
}

void
TabWidget::setDrawDropRect(DropRectType type,
                           bool draw)
{
    if ( (draw == _imp->drawDropRect) && (_imp->transparentFloatingWidget->getDropType() == type) ) {
        return;
    }
    _imp->transparentFloatingWidget->setDropType(type);
    _imp->drawDropRect = draw;

    if ( (type == eDropRectNone) || !draw ) {
        _imp->transparentFloatingWidget->hide();
    } else {
        _imp->transparentFloatingWidget->resize( size() );

        if ( !_imp->transparentFloatingWidget->isVisible() ) {
            _imp->transparentFloatingWidget->show();
            activateWindow();
        }
        QPoint globalPos = mapToGlobal( QPoint(0, 0) );
        _imp->transparentFloatingWidget->move(globalPos);
    }
}

bool
TabWidget::isWithinWidget(const QPoint & globalPos) const
{
    QPoint p = mapToGlobal( QPoint(0, 0) );
    QRect bbox( p.x(), p.y(), width(), height() );

    return bbox.contains(globalPos);
}

bool
TabWidget::isFullScreen() const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    return _imp->fullScreen;
}

bool
TabWidget::isAnchor() const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    return _imp->isAnchor;
}

void
TabWidget::onSetAsAnchorActionTriggered()
{
    std::list<TabWidgetI*>  allPanes = _imp->gui->getApp()->getTabWidgetsSerialization();

    for (std::list<TabWidgetI*>::const_iterator it = allPanes.begin(); it != allPanes.end(); ++it) {
        (*it)->setAsAnchor(*it == this);
    }
}
void
TabWidget::setTabHeaderVisible(bool visible)
{
    _imp->header->setVisible(visible);
}

bool
TabWidget::isTabHeaderVisible() const
{
    return _imp->header->isVisible();
}

void
TabWidget::setAsAnchor(bool anchor)
{
    bool mustUpdate = false;
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);
        _imp->isAnchor = anchor;
        if ( anchor && _imp->tabs.empty() ) {
            mustUpdate = true;
        }
    }
    QPixmap pix;

    if (anchor) {
        appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR, &pix);
    } else {
        appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON, &pix);
    }
    _imp->leftCornerButton->setIcon( QIcon(pix) );
    if (mustUpdate) {
        update();
    }
}

void
TabWidget::togglePaneFullScreen()
{
    bool oldFullScreen;
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);
        oldFullScreen = _imp->fullScreen;
        _imp->fullScreen = !_imp->fullScreen;
    }

    if (oldFullScreen) {
        _imp->gui->minimize();
    } else {
        _imp->gui->maximize(this);
    }
}

void
TabWidget::keyPressEvent (QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    bool accepted = true;

    if ( isKeybind(kShortcutGroupGlobal, kShortcutActionShowPaneFullScreen, modifiers, key) ) {
        togglePaneFullScreen();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionNextTab, modifiers, key) ) {
        moveToNextTab();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionPrevTab, modifiers, key) ) {
        moveToPreviousTab();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionCloseTab, modifiers, key) ) {
        closeCurrentWidget();
    } else if ( isFloatingWindowChild() && isKeybind(kShortcutGroupGlobal, kShortcutActionFullscreen, modifiers, key) ) {
        _imp->gui->toggleFullScreen();
    } else {
        accepted = false;
        QFrame::keyPressEvent(e);
    }
    if (accepted) {
        if (_imp->currentWidget) {
            _imp->currentWidget->takeClickFocus();
        }
    }
}

bool
TabWidget::moveTab(PanelWidget* what,
                   ScriptObject* obj)
{
    TabWidget* from = dynamic_cast<TabWidget*>( what->getWidget()->parentWidget() );

    if (from == this) {
        /*We check that even if it is the same TabWidget, it really exists.*/
        bool found = false;
        for (int i = 0; i < from->count(); ++i) {
            if ( what == from->tabAt(i) ) {
                found = true;
                break;
            }
        }
        if (found) {
            return false;
        }
        //it wasn't found somehow
    }

    bool hadClickFocus = false;
    if (from) {
        hadClickFocus = from->getGui()->getCurrentPanelFocus() == what;

        ///This line will remove click focus
        from->removeTab(what, false);
    }

    appendTab(what, obj);
    if (hadClickFocus) {
        setWidgetClickFocus(what, true);
    }

    if ( !getGui()->isAboutToClose() &&  !getGui()->getApp()->getProject()->isLoadingProject() ) {
        getGui()->getApp()->triggerAutoSave();
    }

    return true;
}

DragPixmap::DragPixmap(const QPixmap & pixmap,
                       const QPoint & offsetFromMouse)
    : QWidget(0, Qt::ToolTip | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint)
    , _pixmap(pixmap)
    , _offset(offsetFromMouse)
{
    setAttribute( Qt::WA_TransparentForMouseEvents );
    resize( _pixmap.width(), _pixmap.height() );
    setWindowOpacity(0.7);
}

void
DragPixmap::update(const QPoint & globalPos)
{
    move(globalPos - _offset);
}

void
DragPixmap::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    p.drawPixmap(0, 0, _pixmap);
}

std::list<std::string>
TabWidget::getTabScriptNames() const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);
    std::list<std::string> ret;

    for (U32 i = 0; i < _imp->tabs.size(); ++i) {
        ret.push_back(_imp->tabs[i].second->getScriptName());
    }

    return ret;
}

PanelWidget*
TabWidget::currentWidget() const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    return _imp->currentWidget;
}

void
TabWidget::currentWidget(PanelWidget** w,
                         ScriptObject** obj) const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    *w = _imp->currentWidget;
    for (U32 i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].first == _imp->currentWidget) {
            *obj = _imp->tabs[i].second;

            return;
        }
    }
    *obj = 0;
}

void
TabWidget::setWidgetMouseOverFocus(PanelWidget* w,
                                   bool hoverFocus)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (w == _imp->currentWidget) {
        _imp->tabBar->setMouseOverFocus(hoverFocus);
    }
}

void
TabWidget::setWidgetClickFocus(PanelWidget* w,
                               bool clickFocus)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (w == _imp->currentWidget) {
        _imp->tabBar->setClickFocus(clickFocus);
    }
}

int
TabWidget::activeIndex() const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    for (U32 i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].first == _imp->currentWidget) {
            return i;
        }
    }

    return -1;
}

void
TabWidget::setScriptName(const std::string & newName)
{
    std::string oldName = getScriptName();
    QString str = QString::fromUtf8(newName.c_str());
    {
        QMutexLocker l(&_imp->tabWidgetStateMutex);

        setObjectName(str);
    }
    QString tt = NATRON_NAMESPACE::convertFromPlainText(tr(LEFT_HAND_CORNER_BUTTON_TT), NATRON_NAMESPACE::WhiteSpaceNormal);
    QString toPre = QString::fromUtf8("Script name: <font size=\"4\"><b>%1</font></b><br/>").arg(str);

    tt.prepend(toPre);
    _imp->leftCornerButton->setToolTip(tt);

    std::string appID = _imp->gui->getApp()->getAppIDString();
    std::stringstream ss;
    if ( !oldName.empty() ) {
        ss << "if hasattr(" << appID << ", '"  << oldName << "'):\n";
        ss << "    del " << appID << "." << oldName << "\n";
    }
    ss << appID << "." << str.toStdString() << " = " << appID << ".getTabWidget('" << str.toStdString() << "')\n";

    std::string script = ss.str();
    std::string err;
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    if (!ok) {
        _imp->gui->getApp()->appendToScriptEditor(err);
    }
}

std::string
TabWidget::getScriptName() const
{
    QMutexLocker l(&_imp->tabWidgetStateMutex);

    return objectName().toStdString();
}

bool
TabWidget::isFloatingWindowChild() const
{
    QWidget* parent = parentWidget();

    while (parent) {
        FloatingWidget* isFloating = dynamic_cast<FloatingWidget*>(parent);
        if (isFloating) {
            return true;
        }
        parent = parent->parentWidget();
    }

    return false;
}

void
TabWidget::discardGuiPointer()
{
    _imp->gui = 0;
}

void
TabWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!_imp->tabBarVisible) {
        QSize size = _imp->header->sizeHint();
        if ( e->y() <= (size.height() * 1.2) ) {
            if ( !_imp->header->isVisible() ) {
                _imp->header->setVisible(true);
            }
        } else {
            if ( _imp->header->isVisible() ) {
                _imp->header->setVisible(false);
            }
        }
    }
    if ( _imp->gui && _imp->gui->isLeftToolBarDisplayedOnMouseHoverOnly() ) {
        _imp->gui->refreshLeftToolBarVisibility( e->globalPos() );
    }
    QFrame::mouseMoveEvent(e);
}

void
TabWidget::leaveEvent(QEvent* e)
{
    onTabBarMouseLeft();
    QFrame::leaveEvent(e);
}

void
TabWidget::enterEvent(QEvent* e)
{
    if (_imp->gui) {
        _imp->gui->setLastEnteredTabWidget(this);
    }
    QFrame::leaveEvent(e);
}

void
TabWidget::onTabBarMouseLeft()
{
    if (!_imp->tabBarVisible) {
        if ( _imp->header->isVisible() ) {
            _imp->header->setVisible(false);
        }
    }
}


void
TabWidgetPrivate::declareTabToPython(PanelWidget* widget,
                                     const std::string& tabName)
{
    NATRON_PYTHON_NAMESPACE::PyPanel* isPanel = dynamic_cast<NATRON_PYTHON_NAMESPACE::PyPanel*>(widget);

    if (!isPanel) {
        return;
    }

    std::string paneName = _publicInterface->getScriptName();
    std::string appID = gui->getApp()->getAppIDString();
    std::stringstream ss;
    ss << appID << "." << paneName << "." << tabName << " = " << appID << ".";
    ss << "getUserPanel('";
    ss << tabName << "')\n";

    std::string script = ss.str();
    std::string err;
    gui->printAutoDeclaredVariable(script);
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("TabWidget::declareTabToPython: " + err);
    }
}

void
TabWidgetPrivate::removeTabToPython(PanelWidget* widget,
                                    const std::string& tabName)
{
    if (!gui) {
        return;
    }
    NATRON_PYTHON_NAMESPACE::PyPanel* isPanel = dynamic_cast<NATRON_PYTHON_NAMESPACE::PyPanel*>(widget);

    if (!isPanel) {
        return;
    }

    std::string paneName = _publicInterface->getScriptName();
    std::string appID = gui->getApp()->getAppIDString();
    std::stringstream ss;
    ss << "del " << appID << "." << paneName << "." << tabName;

    std::string err;
    std::string script = ss.str();
    gui->printAutoDeclaredVariable(script);
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("TabWidget::removeTabToPython: " + err);
    }
}


void
TabWidget::setTabsFromScriptNames(const std::list<std::string>& tabs)
{
    const RegisteredTabs & widgets = getGui()->getRegisteredTabs();
    for (std::list<std::string>::const_iterator it = tabs.begin(); it != tabs.end(); ++it) {
        RegisteredTabs::const_iterator found = widgets.find(*it);

        ///If the tab exists in the current project, move it
        if ( found != widgets.end() ) {
            moveTab(found->second.first, found->second.second);
        }
    }
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_TabWidget.cpp"
