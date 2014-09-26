//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "TabWidget.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QLayout>
#include <QMenu>
#include <QApplication>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtCore/QMimeData>
#include <QtGui/QDrag>
#include <QDebug>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QDragEnterEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QtGui/QDragLeaveEvent>
#include <QtGui/QPaintEvent>
#include <QScrollArea>
#include <QSplitter>
#include <QTextDocument> // for Qt::convertFromPlainText
CLANG_DIAG_ON(deprecated)

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"
#include "Gui/NodeGraph.h"
#include "Gui/CurveEditor.h"
#include "Gui/ViewerTab.h"
#include "Gui/Histogram.h"
#include "Gui/Splitter.h"
#include "Gui/GuiMacros.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/MenuWithToolTips.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"

using namespace Natron;

TabWidget::TabWidget(Gui* gui,
                     QWidget* parent)
    : QFrame(parent),
      _gui(gui),
      _header(0),
      _headerLayout(0),
      _modifyingTabBar(false),
      _tabBar(0),
      _leftCornerButton(0),
      _floatButton(0),
      _closeButton(0),
      _currentWidget(0),
      _drawDropRect(false),
      _fullScreen(false),
      _isAnchor(false),
      _tabBarVisible(true),
      _tabWidgetStateMutex()
{
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0, 5, 0, 0);
    _mainLayout->setSpacing(0);
    setLayout(_mainLayout);

    _header = new TabWidgetHeader(this);
    QObject::connect( _header, SIGNAL(mouseLeftTabBar()), this, SLOT(onTabBarMouseLeft()));
    _headerLayout = new QHBoxLayout(_header);
    _headerLayout->setContentsMargins(0, 0, 0, 0);
    _headerLayout->setSpacing(0);
    _header->setLayout(_headerLayout);


    QPixmap pixC,pixM,pixL;
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET,&pixM);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON,&pixL);


    _leftCornerButton = new Button(QIcon(pixL),"",_header);
    _leftCornerButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _leftCornerButton->setToolTip( Qt::convertFromPlainText(tr("Manage the layouts for this pane."), Qt::WhiteSpaceNormal) );
    _headerLayout->addWidget(_leftCornerButton);
    _headerLayout->addSpacing(10);

    _tabBar = new TabBar(this,_header);
    _tabBar->setShape(QTabBar::RoundedNorth);
    _tabBar->setDrawBase(false);
    QObject::connect( _tabBar, SIGNAL( currentChanged(int) ), this, SLOT( makeCurrentTab(int) ) );
    QObject::connect( _tabBar, SIGNAL(mouseLeftTabBar()), this, SLOT(onTabBarMouseLeft()));
    _headerLayout->addWidget(_tabBar);
    _headerLayout->addStretch();
    _floatButton = new Button(QIcon(pixM),"",_header);
    _floatButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _floatButton->setToolTip( Qt::convertFromPlainText(tr("Float pane."), Qt::WhiteSpaceNormal) );
    _floatButton->setEnabled(true);
    QObject::connect( _floatButton, SIGNAL( clicked() ), this, SLOT( floatCurrentWidget() ) );
    _headerLayout->addWidget(_floatButton);

    _closeButton = new Button(QIcon(pixC),"",_header);
    _closeButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _closeButton->setToolTip( Qt::convertFromPlainText(tr("Close pane."), Qt::WhiteSpaceNormal) );

    QObject::connect( _closeButton, SIGNAL( clicked() ), this, SLOT( closePane() ) );
    _headerLayout->addWidget(_closeButton);


    /*adding menu to the left corner button*/
    _leftCornerButton->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect( _leftCornerButton, SIGNAL( clicked() ), this, SLOT( createMenu() ) );


    _mainLayout->addWidget(_header);
    _mainLayout->addStretch();
}

TabWidget::~TabWidget()
{
}

void
TabWidget::notifyGuiAboutRemoval()
{
    _gui->unregisterPane(this);
}

void
TabWidget::setClosable(bool closable)
{
    _closeButton->setEnabled(closable);
    _floatButton->setEnabled(closable);
}

void
TabWidget::destroyTabs()
{
    for (U32 i = 0; i < _tabs.size(); ++i) {
        bool mustDelete = destroyTab(_tabs[i]);
        if (mustDelete) {
            delete _tabs[i];
            _tabs[i] = NULL;
        }
    }
}

bool
TabWidget::destroyTab(QWidget* tab)
{
    /*special care is taken if this is a viewer: we also
       need to delete the viewer node.*/
    ViewerTab* isViewer = dynamic_cast<ViewerTab*>(tab);
    Histogram* isHisto = dynamic_cast<Histogram*>(tab);

    if (isViewer) {
        _gui->removeViewerTab(isViewer,false,false);
    } else if (isHisto) {
        _gui->removeHistogram(isHisto);
    } else {
        ///Do not delete unique widgets such as the properties bin, node graph or curve editor
        tab->setVisible(false);
    }

    return false; // must not be deleted
}

void
TabWidget::createMenu()
{
    MenuWithToolTips menu(_gui);

    menu.setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );
    QPixmap pixV,pixM,pixH,pixC,pixA;
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY,&pixV);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY,&pixH);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET,&pixM);
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR,&pixA);
    QAction* splitVerticallyAction = new QAction(QIcon(pixV),tr("Split vertical"),&menu);
    QObject::connect( splitVerticallyAction, SIGNAL( triggered() ), this, SLOT( onSplitVertically() ) );
    menu.addAction(splitVerticallyAction);
    QAction* splitHorizontallyAction = new QAction(QIcon(pixH),tr("Split horizontal"),&menu);
    QObject::connect( splitHorizontallyAction, SIGNAL( triggered() ), this, SLOT( onSplitHorizontally() ) );
    menu.addAction(splitHorizontallyAction);
    menu.addSeparator();
    QAction* floatAction = new QAction(QIcon(pixM),tr("Float pane"),&menu);
    QObject::connect( floatAction, SIGNAL( triggered() ), this, SLOT( floatCurrentWidget() ) );
    menu.addAction(floatAction);


    if ( (_tabBar->count() == 0) ) {
        floatAction->setEnabled(false);
    }

    QAction* closeAction = new QAction(QIcon(pixC),tr("Close pane"), &menu);
    closeAction->setEnabled( _closeButton->isEnabled() );
    QObject::connect( closeAction, SIGNAL( triggered() ), this, SLOT( closePane() ) );
    menu.addAction(closeAction);
    
    QAction* hideTabbar;
    if (_tabBarVisible) {
        hideTabbar = new QAction(tr("Hide tabs header"),&menu);
    } else {
        hideTabbar = new QAction(tr("Show tabs header"),&menu);
    }
    QObject::connect(hideTabbar, SIGNAL(triggered()), this, SLOT(onShowHideTabBarActionTriggered()));
    menu.addAction(hideTabbar);
    menu.addSeparator();
    menu.addAction( tr("New viewer"), this, SLOT( addNewViewer() ) );
    menu.addAction( tr("New histogram"), this, SLOT( newHistogramHere() ) );
    menu.addAction( tr("Node graph here"), this, SLOT( moveNodeGraphHere() ) );
    menu.addAction( tr("Curve Editor here"), this, SLOT( moveCurveEditorHere() ) );
    menu.addAction( tr("Properties bin here"), this, SLOT( movePropertiesBinHere() ) );
    menu.addSeparator();
    
    QAction* isAnchorAction = new QAction(QIcon(pixA),tr("Set this as anchor"),&menu);
    isAnchorAction->setToolTip(tr("The anchor pane is where viewers will be created by default."));
    isAnchorAction->setCheckable(true);
    bool isVA = isAnchor();
    isAnchorAction->setChecked(isVA);
    isAnchorAction->setEnabled(!isVA);
    QObject::connect( isAnchorAction, SIGNAL( triggered() ), this, SLOT( onSetAsAnchorActionTriggered() ) );
    menu.addAction(isAnchorAction);

    menu.exec( _leftCornerButton->mapToGlobal( QPoint(0,0) ) );
}

void
TabWidget::tryCloseFloatingPane()
{
    FloatingWidget* parent = dynamic_cast<FloatingWidget*>( parentWidget() );

    if (parent) {
        parent->close();
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
    getOtherSplitWidget(container,this,otherSplit);
    assert(otherSplit);

    Splitter* parentSplitter = dynamic_cast<Splitter*>( container->parentWidget() );
    FloatingWidget* parentWindow = dynamic_cast<FloatingWidget*>( container->parentWidget() );

    assert(parentSplitter || parentWindow);

    if (parentSplitter) {
        QList<int> mainContainerSizes = parentSplitter->sizes();

        /*Removing the splitter container from its parent*/
        int containerIndexInParentSplitter = parentSplitter->indexOf(container);
        parentSplitter->removeChild_mt_safe(container);

        /*moving the other split to the mainContainer*/
        parentSplitter->insertChild_mt_safe(containerIndexInParentSplitter, otherSplit);
        otherSplit->setVisible(true);

        /*restore the main container sizes*/
        parentSplitter->setSizes_mt_safe(mainContainerSizes);
    } else {
        assert(parentWindow);
        parentWindow->removeEmbeddedWidget();
        parentWindow->setWidget(otherSplit);
    }

    /*Remove the container from everywhere*/
    _gui->unregisterSplitter(container);
    container->setParent(NULL);
    container->deleteLater();
}

void
TabWidget::closePane()
{
    if (!_gui) {
        return;
    }

    FloatingWidget* isFloating = dynamic_cast<FloatingWidget*>( parentWidget() );
    if ( isFloating && (isFloating->getEmbeddedWidget() == this) ) {
        isFloating->close();
    }


    /*Removing it from the _panes vector*/

    _gui->unregisterPane(this);


    ///This is the TabWidget to which we will move all our splits.
    TabWidget* tabToTransferTo = 0;

    ///Move living tabs to the viewer anchor TabWidget, this is better than destroying them.
    const std::list<TabWidget*> & panes = _gui->getPanes();
    for (std::list<TabWidget*>::const_iterator it = panes.begin(); it != panes.end(); ++it) {
        if ( (*it != this) && (*it)->isAnchor() ) {
            tabToTransferTo = *it;
            break;
        }
    }


    if (tabToTransferTo) {
        ///move this tab's splits
        while (count() > 0) {
            moveTab(tabAt(0), tabToTransferTo);
        }
    } else {
        while (count() > 0) {
            removeTab( tabAt(0) );
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
    FloatingWidget* isParentFloating = dynamic_cast<FloatingWidget*>( parentWidget() );

    if (isParentFloating) {
        return;
    }

    FloatingWidget* floatingW = new FloatingWidget(_gui,_gui);
    Splitter* parentSplitter = dynamic_cast<Splitter*>( parentWidget() );
    setParent(0);
    if (parentSplitter) {
        closeSplitterAndMoveOtherSplitToParent(parentSplitter);
    }


    floatingW->setWidget(this);

    if (position) {
        floatingW->move(*position);
    }
    _gui->registerFloatingWindow(floatingW);
    _gui->checkNumberOfNonFloatingPanes();
}

void
TabWidget::addNewViewer()
{
    _gui->setNextViewerAnchor(this);
    _gui->getApp()->createNode(  CreateNodeArgs("Viewer",
                                                "",
                                                -1,-1,
                                                -1,
                                                true,
                                                INT_MIN,INT_MIN,
                                                true,
                                                true,
                                                QString(),
                                                CreateNodeArgs::DefaultValuesList()) );
}

void
TabWidget::moveNodeGraphHere()
{
    QWidget* what = dynamic_cast<QWidget*>( _gui->getNodeGraph() );

    moveTab(what,this);
}

void
TabWidget::moveCurveEditorHere()
{
    QWidget* what = dynamic_cast<QWidget*>( _gui->getCurveEditor() );

    moveTab(what,this);
}

void
TabWidget::newHistogramHere()
{
    Histogram* h = _gui->addNewHistogram();

    appendTab(h);

    _gui->getApp()->triggerAutoSave();
}

/*Get the header name of the tab at index "index".*/
QString
TabWidget::getTabName(int index) const
{
    if ( index >= _tabBar->count() ) {
        return "";
    }

    return _tabBar->tabText(index);
}

QString
TabWidget::getTabName(QWidget* tab) const
{
    return tab->objectName();
}

void
TabWidget::setTabName(QWidget* tab,
                      const QString & name)
{
    QMutexLocker l(&_tabWidgetStateMutex);

    tab->setObjectName(name);
    for (U32 i = 0; i < _tabs.size(); ++i) {
        if (_tabs[i] == tab) {
            _tabBar->setTabText(i, name);
        }
    }
}

void
TabWidget::floatCurrentWidget()
{
    if ( (_tabs.size() > 1) || !_closeButton->isEnabled() ) {
        ///Make a new tab widget and float it instead
        TabWidget* newPane = new TabWidget(_gui,_gui);
        newPane->setObjectName_mt_safe( _gui->getAvailablePaneName() );
        _gui->registerPane(newPane);
        moveTab(_currentWidget, newPane);
        newPane->floatPane();
    } else {
        ///Float this tab
        floatPane();
    }
}

void
TabWidget::closeCurrentWidget()
{
    if (!_currentWidget) {
        return;
    }
    removeTab(_currentWidget);
    bool mustDelete = destroyTab(_currentWidget);
    if (mustDelete) {
        delete _currentWidget;
        _currentWidget = NULL;
    }
}

void
TabWidget::closeTab(int index)
{
    assert( index < (int)_tabs.size() );
    QWidget *tab = _tabs[index];
    assert(_tabs[index]);
    removeTab(tab);
    bool mustDelete = destroyTab(tab);
    if (mustDelete) {
        delete tab;
        // tab was already removed from the _tabs array by removeTab()
    }
    _gui->getApp()->triggerAutoSave();
}

void
TabWidget::movePropertiesBinHere()
{
    QWidget* what = dynamic_cast<QWidget*>( _gui->getPropertiesScrollArea() );

    moveTab(what, this);
}

TabWidget*
TabWidget::splitInternal(bool autoSave,
                         Qt::Orientation orientation)
{
    FloatingWidget* parentIsFloating = dynamic_cast<FloatingWidget*>( parentWidget() );
    Splitter* parentIsSplitter = dynamic_cast<Splitter*>( parentWidget() );

    assert(parentIsSplitter || parentIsFloating);

    /*We need to know the position in the container layout of the old tab widget*/
    int oldIndexInParentSplitter;
    QList<int> oldSizeInParentSplitter;
    if (parentIsSplitter) {
        oldIndexInParentSplitter = parentIsSplitter->indexOf(this);
        oldSizeInParentSplitter = parentIsSplitter->sizes();
    } else {
        assert(parentIsFloating);
        parentIsFloating->removeEmbeddedWidget();
    }

    Splitter* newSplitter = new Splitter;
    newSplitter->setContentsMargins(0, 0, 0, 0);
    newSplitter->setOrientation(orientation);
    _gui->registerSplitter(newSplitter);
 
    /*Add this to the new splitter*/
    newSplitter->addWidget_mt_safe(this);

    /*Adding now a new TabWidget*/
    TabWidget* newTab = new TabWidget(_gui,newSplitter);
    newTab->setObjectName_mt_safe( _gui->getAvailablePaneName() );
    _gui->registerPane(newTab);
    newSplitter->insertChild_mt_safe(-1,newTab);

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

    if (parentIsFloating) {
        parentIsFloating->setWidget(newSplitter);
        parentIsFloating->resize(splitterSize);
    } else {
        /*The parent MUST be a splitter otherwise there's a serious bug!*/
        assert(parentIsSplitter);

        /*Inserting back the new splitter at the original index*/
        parentIsSplitter->insertChild_mt_safe(oldIndexInParentSplitter,newSplitter);

        /*restore the container original sizes*/
        parentIsSplitter->setSizes_mt_safe(oldSizeInParentSplitter);
    }


    if (!_gui->getApp()->getProject()->isLoadingProject() && autoSave) {
        _gui->getApp()->triggerAutoSave();
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
TabWidget::appendTab(QWidget* widget)
{
    return appendTab(QIcon(),widget);
}

bool
TabWidget::appendTab(const QIcon & icon,
                     QWidget* widget)
{
    {
        QMutexLocker l(&_tabWidgetStateMutex);

        ///If we do not know the tab ignore it
        QString title = widget->objectName();
        if ( title.isEmpty() ) {
            return false;
        }

        /*registering this tab for drag&drop*/
        _gui->registerTab(widget);

        _tabs.push_back(widget);
        widget->setParent(this);
        _modifyingTabBar = true;
        _tabBar->addTab(icon,title);
        _tabBar->updateGeometry(); //< necessary
        _modifyingTabBar = false;
        if (_tabs.size() == 1) {
            for (int i = 0; i < _mainLayout->count(); ++i) {
                QSpacerItem* item = dynamic_cast<QSpacerItem*>( _mainLayout->itemAt(i) );
                if (item) {
                    _mainLayout->removeItem(item);
                }
            }
        }
        _floatButton->setEnabled(true);
    }
    makeCurrentTab(_tabs.size() - 1);

    return true;
}

void
TabWidget::insertTab(int index,
                     const QIcon & icon,
                     QWidget* widget)
{
    QMutexLocker l(&_tabWidgetStateMutex);
    QString title = widget->objectName();

    if ( (U32)index < _tabs.size() ) {
        /*registering this tab for drag&drop*/
        _gui->registerTab(widget);

        _tabs.insert(_tabs.begin() + index, widget);
        _modifyingTabBar = true;
        _tabBar->insertTab(index,icon,title);
        _tabBar->updateGeometry(); //< necessary
        _modifyingTabBar = false;
    } else {
        appendTab(widget);
    }
    _floatButton->setEnabled(true);
}

void
TabWidget::insertTab(int index,
                     QWidget* widget)
{
    insertTab(index, QIcon(), widget);
}

QWidget*
TabWidget::removeTab(int index)
{
    QMutexLocker l(&_tabWidgetStateMutex);

    if ( (index < 0) || ( index >= (int)_tabs.size() ) ) {
        return NULL;
    }
    QWidget* tab = _tabs[index];
    _tabs.erase(_tabs.begin() + index);
    _modifyingTabBar = true;
    _tabBar->removeTab(index);
    _modifyingTabBar = false;
    if (_tabs.size() > 0) {
        l.unlock();
        makeCurrentTab(0);
        l.relock();
    } else {
        _currentWidget = 0;
        _mainLayout->addStretch();
        if ( !_gui->isDraggingPanel() ) {
            l.unlock();
            tryCloseFloatingPane();
            l.relock();
        }
    }
    tab->setParent(NULL);

    return tab;
}

void
TabWidget::removeTab(QWidget* widget)
{
    int index = -1;

    {
        QMutexLocker l(&_tabWidgetStateMutex);
        for (U32 i = 0; i < _tabs.size(); ++i) {
            if (_tabs[i] == widget) {
                index = i;
                break;
            }
        }
    }

    if (index != -1) {
        QWidget* tab = removeTab(index);
        assert(tab == widget);
        (void)tab;
    }
}

void
TabWidget::makeCurrentTab(int index)
{
    if (_modifyingTabBar) {
        return;
    }
    QWidget* tab;
    {
        QMutexLocker l(&_tabWidgetStateMutex);
        if ( (index < 0) || ( index >= (int)_tabs.size() ) ) {
            return;
        }
        /*Removing previous widget if any*/
        if (_currentWidget) {
            _currentWidget->setVisible(false);
            _mainLayout->removeWidget(_currentWidget);
            // _currentWidget->setParent(0);
        }
        tab = _tabs[index];
        _mainLayout->addWidget(tab);
        _currentWidget = tab;
    }
    tab->setVisible(true);
    tab->setParent(this);
    _modifyingTabBar = true;
    _tabBar->setCurrentIndex(index);
    _modifyingTabBar = false;
}

void
TabWidget::paintEvent(QPaintEvent* e)
{
    QFrame::paintEvent(e);

    if (_drawDropRect) {
        QRect r = rect();
        QPainter p(this);
        QPen pen;
        pen.setBrush( QColor(243,149,0,255) );
        p.setPen(pen);
        //        QPalette* palette = new QPalette();
        //        palette->setColor(QPalette::Foreground,c);
        //        setPalette(*palette);
        r.adjust(0, 0, -1, -1);
        p.drawRect(r);
    }
}

void
TabWidget::dropEvent(QDropEvent* e)
{
    e->accept();
    QString name( e->mimeData()->data("Tab") );
    QWidget* w = _gui->findExistingTab( name.toStdString() );
    if (w) {
        moveTab(w, this);
    }
    _drawDropRect = false;
    setFrameShape(QFrame::NoFrame);
    repaint();
}

TabBar::TabBar(TabWidget* tabWidget,
               QWidget* parent)
    : QTabBar(parent)
      , _dragPos()
      , _dragPix(0)
      , _tabWidget(tabWidget)
      , _processingLeaveEvent(false)
{
    setTabsClosable(true);
    setMouseTracking(true);
    QObject::connect( this, SIGNAL( tabCloseRequested(int) ), tabWidget, SLOT( closeTab(int) ) );
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
        emit mouseLeftTabBar();
        QTabBar::leaveEvent(e);
        _processingLeaveEvent = false;
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
        const std::list<TabWidget*> panes = _tabWidget->getGui()->getPanes();
        for (std::list<TabWidget*>::const_iterator it = panes.begin(); it != panes.end(); ++it) {
            if ( (*it)->isWithinWidget(globalPos) ) {
                (*it)->setDrawDropRect(true);
            } else {
                (*it)->setDrawDropRect(false);
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

        _dragPix = new DragPixmap( pixmap,e->pos() );
        _dragPix->update( e->globalPos() );
        _dragPix->show();
        grabMouse();
    }
    QTabBar::mouseMoveEvent(e);
}

QPixmap
TabBar::makePixmapForDrag(int index)
{
    std::vector< std::pair<QString,QIcon > > tabs;

    for (int i = 0; i < count(); ++i) {
        tabs.push_back( std::make_pair( tabText(i),tabIcon(i) ) );
    }

    //remove all tabs
    while (count() > 0) {
        removeTab(0);
    }

    ///insert just the tab we want to screen shot
    addTab(tabs[index].second, tabs[index].first);

    QPixmap currentTabPixmap =  Gui::screenShot( _tabWidget->tabAt(index) );
#if QT_VERSION < 0x050000
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

    //now we just put together the 2 pixmaps and set it with mid transparancy
    QImage ret(currentTabImg.width(),currentTabImg.height() + tabBarImg.height(),QImage::Format_ARGB32_Premultiplied);

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

    QWidget* draggedPanel = _gui->stopDragPanel();
    const std::list<TabWidget*> panes = _gui->getPanes();
    bool foundTabWidgetUnderneath = false;
    for (std::list<TabWidget*>::const_iterator it = panes.begin(); it != panes.end(); ++it) {
        if ( (*it)->isWithinWidget(globalPos) ) {
            (*it)->appendTab(draggedPanel);
            foundTabWidgetUnderneath = true;
            break;
        }
    }

    bool ret = false;
    if (!foundTabWidgetUnderneath) {
        ///if we reach here that means the mouse is not over any tab widget, then float the panel
        
        
        
        QPoint windowPos = globalPos;
        FloatingWidget* floatingW = new FloatingWidget(_gui,_gui);
        TabWidget* newTab = new TabWidget(_gui,floatingW);
        newTab->setObjectName_mt_safe( _gui->getAvailablePaneName() );
        _gui->registerPane(newTab);
        newTab->appendTab(draggedPanel);
        floatingW->setWidget(newTab);
        floatingW->move(windowPos);
        _gui->registerFloatingWindow(floatingW);
        
        _gui->checkNumberOfNonFloatingPanes();
        
        bool isClosable = _closeButton->isEnabled();
        if (isClosable && count() == 0) {
            closePane();
            ret = true;
        }
    }
    
    
    for (std::list<TabWidget*>::const_iterator it = panes.begin(); it != panes.end(); ++it) {
        (*it)->setDrawDropRect(false);
    }
    return ret;
}

void
TabWidget::startDragTab(int index)
{
    if ( index >= count() ) {
        return;
    }

    QWidget* selectedTab = tabAt(index);
    assert(selectedTab);
    selectedTab->setParent(this);

    _gui->startDragPanel(selectedTab);

    removeTab(selectedTab);
    selectedTab->hide();
}

void
TabWidget::setDrawDropRect(bool draw)
{
    if (draw == _drawDropRect) {
        return;
    }

    _drawDropRect = draw;
    if (draw) {
        setFrameShape(QFrame::Box);
    } else {
        setFrameShape(QFrame::NoFrame);
    }
    repaint();
}

bool
TabWidget::isWithinWidget(const QPoint & globalPos) const
{
    QPoint p = mapToGlobal( QPoint(0,0) );
    QRect bbox( p.x(),p.y(),width(),height() );

    return bbox.contains(globalPos);
}

bool
TabWidget::isFullScreen() const
{
    QMutexLocker l(&_tabWidgetStateMutex);

    return _fullScreen;
}

bool
TabWidget::isAnchor() const
{
    QMutexLocker l(&_tabWidgetStateMutex);

    return _isAnchor;
}

void
TabWidget::onSetAsAnchorActionTriggered()
{
    const std::list<TabWidget*> & allPanes = _gui->getPanes();

    for (std::list<TabWidget*>::const_iterator it = allPanes.begin(); it != allPanes.end(); ++it) {
        (*it)->setAsAnchor(*it == this);
    }
}

void
TabWidget::onShowHideTabBarActionTriggered()
{
    _tabBarVisible = !_tabBarVisible;
    _header->setVisible(_tabBarVisible);
}

void
TabWidget::setAsAnchor(bool anchor)
{
    {
        QMutexLocker l(&_tabWidgetStateMutex);
        _isAnchor = anchor;
    }
    QPixmap pix;

    if (anchor) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR, &pix);
    } else {
        appPTR->getIcon(Natron::NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON, &pix);
    }
    _leftCornerButton->setIcon( QIcon(pix) );
}

void
TabWidget::keyPressEvent (QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Space) && modCASIsNone(e) ) {
        bool fullScreen;
        {
            QMutexLocker l(&_tabWidgetStateMutex);
            fullScreen = _fullScreen;
        }
        {
            QMutexLocker l(&_tabWidgetStateMutex);
            _fullScreen = !_fullScreen;
        }
        if (fullScreen) {
            _gui->minimize();
        } else {
            _gui->maximize(this);
        }
        e->accept();
    } else if (isFloatingWindowChild() && isKeybind(kShortcutGroupGlobal, kShortcutIDActionFullscreen, e->modifiers(), e->key())) {
        _gui->toggleFullScreen();
        e->accept();
    } else {
        e->ignore();
    }
}

bool
TabWidget::moveTab(QWidget* what,
                   TabWidget *where)
{
    TabWidget* from = dynamic_cast<TabWidget*>( what->parentWidget() );

    if (from == where) {
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

    if (from) {
        from->removeTab(what);
    }
    assert(where);
    where->appendTab(what);
    what->setParent(where);
    if ( !where->getGui()->getApp()->getProject()->isLoadingProject() ) {
        where->getGui()->getApp()->triggerAutoSave();
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

    p.drawPixmap(0,0,_pixmap);
}

QStringList
TabWidget::getTabNames() const
{
    QMutexLocker l(&_tabWidgetStateMutex);
    QStringList ret;

    for (U32 i = 0; i < _tabs.size(); ++i) {
        ret << _tabs[i]->objectName();
    }

    return ret;
}

int
TabWidget::activeIndex() const
{
    QMutexLocker l(&_tabWidgetStateMutex);

    for (U32 i = 0; i < _tabs.size(); ++i) {
        if (_tabs[i] == _currentWidget) {
            return i;
        }
    }

    return -1;
}

void
TabWidget::setObjectName_mt_safe(const QString & str)
{
    QMutexLocker l(&_tabWidgetStateMutex);

    setObjectName(str);
}

QString
TabWidget::objectName_mt_safe() const
{
    QMutexLocker l(&_tabWidgetStateMutex);

    return objectName();
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
    _gui = 0;
}

void
TabWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!_tabBarVisible) {
        QSize size = _header->sizeHint();
        if (e->y() <= (size.height() * 1.2)) {
            if (!_header->isVisible()) {
                _header->setVisible(true);
            }
        } else {
            if (_header->isVisible()) {
                _header->setVisible(false);
            }
        }
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
TabWidget::onTabBarMouseLeft()
{
    if (!_tabBarVisible) {
        if (_header->isVisible()) {
            _header->setVisible(false);
        }
    }
}