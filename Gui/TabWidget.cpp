//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
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
#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"

using namespace Natron;

const QString TabWidget::splitHorizontallyTag = QString("_horizSplit");
const QString TabWidget::splitVerticallyTag = QString("_vertiSplit");

FloatingWidget::FloatingWidget(QWidget* parent)
: QWidget(parent)
, _embeddedWidget(0)
, _layout(0)
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose,true);
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    setLayout(_layout);
    
}
void FloatingWidget::setWidget(const QSize& widgetSize,QWidget* w)
{
    assert(w);
    if (_embeddedWidget) {
        return;
    }
    _embeddedWidget = w;
    w->setParent(this);
    assert(_layout);
    _layout->addWidget(w);
    w->setVisible(true);
    w->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    resize(widgetSize);
    show();
}

void FloatingWidget::removeWidget() {
    if (!_embeddedWidget) {
        return;
    }
    _layout->removeWidget(_embeddedWidget);
    _embeddedWidget->setParent(NULL);
    // _embeddedWidget->setVisible(false);
    hide();
}

void FloatingWidget::closeEvent(QCloseEvent* e) {
    TabWidget* embedded = dynamic_cast<TabWidget*>(_embeddedWidget);
    emit closed();
    if (embedded) {
        ///Move all tabs to another tab widget
        const std::list<TabWidget*>& panes = embedded->getGui()->getPanes();
        if (panes.empty()) {
            embedded->destroyTabs();
        } else {
            TabWidget* otherPane = *panes.begin();
            while (embedded->count() > 0) {
                if (!TabWidget::moveTab(embedded->tabAt(0), otherPane)) {
                    break;
                }
            }
        }
    }
    QWidget::closeEvent(e);
}

TabWidget::TabWidget(Gui* gui,QWidget* parent):
QFrame(parent),
_gui(gui),
_header(0),
_headerLayout(0),
_modifyingTabBar(false),
_tabBar(0),
_leftCornerButton(0),
_floatButton(0),
_closeButton(0),
_currentWidget(0),
_isFloating(false),
_drawDropRect(false),
_fullScreen(false),
_userSplits(),
_tabWidgetStateMutex(),
_mtSafeWindowPos(),
_mtSafeWindowWidth(0),
_mtSafeWindowHeight(0)
{
    
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    _mainLayout->setSpacing(0);
    setLayout(_mainLayout);
    
    _header = new QWidget(this);
    
    _headerLayout = new QHBoxLayout(_header);
    _headerLayout->setContentsMargins(0, 0, 0, 0);
    _headerLayout->setSpacing(0);
    _header->setLayout(_headerLayout);
    
    
    QPixmap pixC,pixM,pixL;
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET,&pixM);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON,&pixL);
    
    
    _leftCornerButton = new Button(QIcon(pixL),"",_header);
    _leftCornerButton->setToolTip(Qt::convertFromPlainText(tr("Manage the layouts for this pane."), Qt::WhiteSpaceNormal));
    _leftCornerButton->setFixedSize(15,15);
    _headerLayout->addWidget(_leftCornerButton);
    _headerLayout->addSpacing(10);
    
    _tabBar = new TabBar(this,_header);
    _tabBar->setShape(QTabBar::RoundedNorth);
    _tabBar->setDrawBase(false);
    QObject::connect(_tabBar, SIGNAL(currentChanged(int)), this, SLOT(makeCurrentTab(int)));
    _headerLayout->addWidget(_tabBar);
    _headerLayout->addStretch();
    _floatButton = new Button(QIcon(pixM),"",_header);
    _floatButton->setToolTip(Qt::convertFromPlainText(tr("Float pane."), Qt::WhiteSpaceNormal));
    _floatButton->setFixedSize(15,15);
    _floatButton->setEnabled(true);
    QObject::connect(_floatButton, SIGNAL(clicked()), this, SLOT(floatCurrentWidget()));
    _headerLayout->addWidget(_floatButton);
    
    _closeButton = new Button(QIcon(pixC),"",_header);
    _closeButton->setToolTip(Qt::convertFromPlainText(tr("Close pane."), Qt::WhiteSpaceNormal));
    _closeButton->setFixedSize(15,15);

    QObject::connect(_closeButton, SIGNAL(clicked()), this, SLOT(closePane()));
    _headerLayout->addWidget(_closeButton);
    
    
    /*adding menu to the left corner button*/
    _leftCornerButton->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(_leftCornerButton, SIGNAL(clicked()), this, SLOT(createMenu()));
    
    
    _mainLayout->addWidget(_header);
    _mainLayout->addStretch();
    
}

TabWidget::~TabWidget()
{

}

void TabWidget::notifyGuiAboutRemoval()
{
    _gui->removePane(this);
}

void TabWidget::setClosable(bool closable)
{
    _closeButton->setEnabled(closable);
    _floatButton->setEnabled(closable);
}

void TabWidget::destroyTabs() {
    
    for (U32 i = 0; i < _tabs.size(); ++i) {
        bool mustDelete = destroyTab(_tabs[i]);
        if (mustDelete) {
            delete _tabs[i];
            _tabs[i] = NULL;
        }
    }
}

bool TabWidget::destroyTab(QWidget* tab) {
    /*special care is taken if this is a viewer: we also
     need to delete the viewer node.*/
    ViewerTab* isViewer = dynamic_cast<ViewerTab*>(tab);
    Histogram* isHisto = dynamic_cast<Histogram*>(tab);
    if (isViewer) {
        _gui->removeViewerTab(isViewer,false,false);
    } else if(isHisto) {
        _gui->removeHistogram(isHisto);
    } else {
        ///Do not delete unique widgets such as the properties bin, node graph or curve editor
        tab->setVisible(false);
    }
    return false; // must not be deleted
}

void TabWidget::createMenu(){
    QMenu *menu = new QMenu(_gui);
    menu->setFont(QFont(NATRON_FONT,NATRON_FONT_SIZE_11));
    QPixmap pixV,pixM,pixH,pixC;
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY,&pixV);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY,&pixH);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET,&pixM);
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);
    QAction* splitVerticallyAction = new QAction(QIcon(pixV),tr("Split vertical"),this);
    QObject::connect(splitVerticallyAction, SIGNAL(triggered()), this, SLOT(onSplitVertically()));
    menu->addAction(splitVerticallyAction);
    QAction* splitHorizontallyAction = new QAction(QIcon(pixH),tr("Split horizontal"),this);
    QObject::connect(splitHorizontallyAction, SIGNAL(triggered()), this, SLOT(onSplitHorizontally()));
    menu->addAction(splitHorizontallyAction);
    if (_isFloating) {
        splitVerticallyAction->setEnabled(false);
        splitHorizontallyAction->setEnabled(false);
    }
    menu->addSeparator();
    QAction* floatAction = new QAction(QIcon(pixM),tr("Float pane"),this);
    QObject::connect(floatAction, SIGNAL(triggered()), this, SLOT(floatCurrentWidget()));
    menu->addAction(floatAction);
    if(_tabBar->count() == 0 || _isFloating){
        floatAction->setEnabled(false);
    }
    
    QAction* closeAction = new QAction(QIcon(pixC),tr("Close pane"), this);
    
    QObject::connect(closeAction, SIGNAL(triggered()), this, SLOT(closePane()));
    menu->addAction(closeAction);
    menu->addSeparator();
    menu->addAction(tr("New viewer"), this, SLOT(addNewViewer()));
    menu->addAction(tr("New histogram"), this, SLOT(newHistogramHere()));
    menu->addAction(tr("Node graph here"), this, SLOT(moveNodeGraphHere()));
    menu->addAction(tr("Curve Editor here"), this, SLOT(moveCurveEditorHere()));
    menu->addAction(tr("Properties bin here"), this, SLOT(movePropertiesBinHere()));
    menu->exec(_leftCornerButton->mapToGlobal(QPoint(0,0)));
}

void TabWidget::closeFloatingPane(){
    {
        QMutexLocker l(&_tabWidgetStateMutex);
        if (!_isFloating) {
            return;
        }
    }
    QWidget* p = parentWidget();
    p->close();
}

bool TabWidget::removeSplit(TabWidget* tab,bool* orientation){
    QMutexLocker l(&_tabWidgetStateMutex);
    for (std::list<std::pair<TabWidget*,bool> > ::iterator it =  _userSplits.begin();it!=_userSplits.end();++it) {
        if (it->first == tab) {
            if (orientation) {
                *orientation = it->second;
            }
            _userSplits.erase(it);
            return true;
            
            break;
        }
    }

    return false;
}

static void getOtherTabWidget(Splitter* parentSplitter,TabWidget* thisWidget,QWidget*& other)
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

static void getTabWidgetRecursively(const TabWidget* caller,Splitter* parentSplitter,TabWidget*& tab) {
    bool found = false;
    for (int i = 0; i < parentSplitter->count(); ++i) {
        QWidget* w = parentSplitter->widget(i);
        if (w == caller) {
            continue;
        }
        TabWidget* isTab = dynamic_cast<TabWidget*>(w);
        Splitter* isSplitter = dynamic_cast<Splitter*>(w);
        if (isTab) {
            tab = isTab;
            found = true;
            break;
        } else if (isSplitter) {
            getTabWidgetRecursively(caller,isSplitter, tab);
            if (tab) {
                return;
            }
        }
    }
    if (!found) {
        Splitter* parent = dynamic_cast<Splitter*>(parentSplitter->parentWidget());
        assert(parent);
        getTabWidgetRecursively(caller,parent, tab);
    }
}

void TabWidget::removeTagNameRecursively(TabWidget* widget, bool horizontal)
{
    QString name = widget->objectName();
    QString toRemove = horizontal ? TabWidget::splitHorizontallyTag : TabWidget::splitVerticallyTag;
    int i = name.indexOf(toRemove);
    assert(i != -1);
    name.remove(i, toRemove.size());
    
    ///also remove the unique idenfitifer
    int identifierIndex = name.size() - 1;
    while (identifierIndex >= 0 && name.at(identifierIndex).isDigit()) {
        --identifierIndex;
    }
    ++identifierIndex;
    name = name.remove(identifierIndex,name.size() - identifierIndex);
    
    widget->setObjectName_mt_safe(name);
    for (std::list<std::pair<TabWidget*,bool> >::iterator it = widget->_userSplits.begin();it != widget->_userSplits.end();++it) {
        removeTagNameRecursively(it->first,horizontal);
    }
}

void TabWidget::closePane() {
    /*If it is floating we do not need to re-arrange the splitters containing the tab*/
    if (isFloating()) {
        parentWidget()->close();
        return;
    }
    
    Splitter* container = dynamic_cast<Splitter*>(parentWidget());
    if(!container) {
        return;
    }
    
    
    /*Removing it from the _panes vector*/
    _gui->removePane(this);
    
    /*Only sub-panes are closable. That means the splitter owning them must also
     have a splitter as parent*/
    Splitter* parentContainer = dynamic_cast<Splitter*>(container->parentWidget());
    if(!parentContainer) {
        return;
    }
    
    QList<int> mainContainerSizes = parentContainer->sizes();

    
    /*identifying the other tab*/
    QWidget* other = 0;
    getOtherTabWidget(container,this,other);
    assert(other);
    
    bool vertical = false;
    bool removeOk = false;
    
    {
        Splitter*  parentSplitter = container;
        ///The only reason becasue this is not ok is because we split this TabWidget again
        while (!removeOk && parentSplitter) {
            
            for (int i = 0; i < parentSplitter->count(); ++i) {
                TabWidget* tab = dynamic_cast<TabWidget*>(parentSplitter->widget(i));
                if (tab && tab != this) {
                    removeOk = tab->removeSplit(this,&vertical);
                    if (removeOk) {
                        break;
                    }
                }
            }
            
            parentSplitter = dynamic_cast<Splitter*>(parentSplitter->parentWidget());
        }
    }
    
    ///iterate recursively over parents splitter to find a tabwidget where to move the tabs
    TabWidget* firstParentTabWidget = NULL;
    getTabWidgetRecursively(this,parentContainer,firstParentTabWidget);
    assert(firstParentTabWidget);
   
    ///move this tab's splits to the first parent tab widget
    for (std::list<std::pair<TabWidget*,bool> >::iterator it = _userSplits.begin();it != _userSplits.end();++it) {
        removeTagNameRecursively(it->first,!it->second);
    }
    
    for (std::list<std::pair<TabWidget*,bool> >::iterator it = _userSplits.begin();it != _userSplits.end();++it) {
        if (firstParentTabWidget != this && it->first != firstParentTabWidget) {
            int splitsCount = 0;
            for (std::list<std::pair<TabWidget*,bool> >::iterator it2 = firstParentTabWidget->_userSplits.begin();
                 it2!=firstParentTabWidget->_userSplits.end(); ++it2) {
                if (it->second && it2->second) {
                    ++splitsCount;
                } else if (!it->second && !it2->second) {
                    ++splitsCount;
                }
            }
            
            if (it->second) {
                it->first->setObjectName_mt_safe(it->first->objectName_mt_safe() +
                                                 TabWidget::splitVerticallyTag +
                                                 QString::number(splitsCount));
            } else {
                it->first->setObjectName_mt_safe(it->first->objectName_mt_safe() +
                                                 TabWidget::splitHorizontallyTag +
                                                 QString::number(splitsCount));
            }
            firstParentTabWidget->_userSplits.push_back(*it);
        }
    }
    
    
    
    /*Removing "what" from the container and delete it*/
    setVisible(false);
    //move all its tabs to the  firstParentTabWidget
    
    while(count() > 0) {
        moveTab(tabAt(0), firstParentTabWidget);
    }
    
    // delete what;
    
    /*Removing the container from the mainContainer*/
    int subSplitterIndex = 0;
    for (int i = 0; i < parentContainer->count(); ++i) {
        Splitter* subSplitter = dynamic_cast<Splitter*>(parentContainer->widget(i));
        if (subSplitter && subSplitter == container) {
            subSplitterIndex = i;
            container->setVisible(false);
            container->setParent(0);
            break;
        }
    }
    /*moving the other to the mainContainer*/
    parentContainer->insertWidget(subSplitterIndex, other);
    other->setVisible(true);
    other->setParent(parentContainer);
    
    ///restore the main container sizes
    parentContainer->setSizes_mt_safe(mainContainerSizes);
    
    /*deleting the subSplitter*/
    _gui->removeSplitter(container);
    container->deleteLater();
}

void TabWidget::floatPane(QPoint* position){
    {
        QMutexLocker l(&_tabWidgetStateMutex);
        _isFloating = true;
    }
    FloatingWidget* floatingW = new FloatingWidget(_gui);
    
    setVisible(false);
    setParent(0);
    floatingW->setWidget(size(),this);
    
    if (position) {
        floatingW->move(*position);
    }
    
}

void TabWidget::addNewViewer(){
    _gui->setNewViewerAnchor(this);
    _gui->getApp()->createNode(CreateNodeArgs("Viewer"));
}

void TabWidget::moveNodeGraphHere(){
    QWidget* what = dynamic_cast<QWidget*>(_gui->getNodeGraph());
    moveTab(what,this);
}

void TabWidget::moveCurveEditorHere(){
    QWidget* what = dynamic_cast<QWidget*>(_gui->getCurveEditor());
    moveTab(what,this);
}

void TabWidget::newHistogramHere() {
    Histogram* h = _gui->addNewHistogram();
    appendTab(h);
    
    _gui->getApp()->triggerAutoSave();
}

/*Get the header name of the tab at index "index".*/
QString TabWidget::getTabName(int index) const {
    if(index >= _tabBar->count()) return "";
    return _tabBar->tabText(index);
}

QString TabWidget::getTabName(QWidget* tab) const{
    return tab->objectName();
}

void TabWidget::setTabName(QWidget* tab,const QString& name) {
    QMutexLocker l(&_tabWidgetStateMutex);
    tab->setObjectName(name);
    for (U32 i = 0; i < _tabs.size(); ++i) {
        if (_tabs[i] == tab) {
            _tabBar->setTabText(i, name);
        }
    }
}

void TabWidget::floatCurrentWidget(){
    if(!_currentWidget || _isFloating)
        return;
    
    
    if (_tabs.size() > 1 || !_closeButton->isEnabled()) {
        ///Make a new tab widget and float it instead
        TabWidget* newPane = new TabWidget(_gui,_gui);
        moveTab(_currentWidget, newPane);
        newPane->floatPane();
    } else {
        ///Float this tab
        Splitter* parentSplitter = dynamic_cast<Splitter*>(parentWidget());
        floatPane();
        if(_tabBar->count() == 0){
            _floatButton->setEnabled(false);
        }
        
        if (parentSplitter) {
            QWidget* otherWidget = 0;
            for (int i = 0; i < parentSplitter->count(); ++i) {
                QWidget* w = parentSplitter->widget(i);
                if (w && w != this) {
                    otherWidget = w;
                    break;
                }
            }
            ///move otherWidget to the parent splitter of the splitter
            Splitter* parentParentSplitter = dynamic_cast<Splitter*>(parentSplitter->parentWidget());
            _gui->removeSplitter(parentSplitter);
            parentSplitter->deleteLater();
            if (parentParentSplitter) {
                parentParentSplitter->addWidget_mt_safe(otherWidget);
            }
        }
    }
}

void TabWidget::closeCurrentWidget(){
    if(!_currentWidget) {
        return;
    }
    removeTab(_currentWidget);
    bool mustDelete = destroyTab(_currentWidget);
    if (mustDelete) {
        delete _currentWidget;
        _currentWidget = NULL;
    }
}
void TabWidget::closeTab(int index){
    assert(index < (int)_tabs.size());
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

void TabWidget::movePropertiesBinHere(){
    QWidget* what = dynamic_cast<QWidget*>(_gui->getPropertiesScrollArea());
    moveTab(what, this);
}

TabWidget* TabWidget::splitHorizontally(bool autoSave)
{
    
    Splitter* container = dynamic_cast<Splitter*>(parentWidget());
    if (!container) {
        return NULL;
    }
    
    /*We need to know the position in the container layout of the old tab widget*/
    int oldIndex = container->indexOf(this);
    
    QList<int> containerSizes = container->sizes();
    
    Splitter* newSplitter = new Splitter(container);
    newSplitter->setObjectName_mt_safe(container->objectName()+TabWidget::splitHorizontallyTag);
    newSplitter->setContentsMargins(0, 0, 0, 0);
    newSplitter->setOrientation(Qt::Horizontal);
    setVisible(false);
    setParent(newSplitter);
    newSplitter->addWidget(this);
    setVisible(true);
    _gui->registerSplitter(newSplitter);
    
    /*Adding now a new tab*/
    TabWidget* newTab = new TabWidget(_gui,newSplitter);
    
    ///count the number of horizontal splits that were already made to uniquely identify this split
    int noHorizSplit = 0;
    for (std::list< std::pair<TabWidget*,bool> >::iterator it = _userSplits.begin();it!=_userSplits.end(); ++it) {
        if (!it->second) {
            ++noHorizSplit;
        }
    }
    
    newTab->setObjectName_mt_safe(objectName()+TabWidget::splitHorizontallyTag + QString::number(noHorizSplit));
    _gui->registerPane(newTab);
    newSplitter->addWidget(newTab);
    
    QSize splitterSize = newSplitter->sizeHint();
    QList<int> sizes; sizes <<   splitterSize.width()/2;
    sizes  << splitterSize.width()/2;
    newSplitter->setSizes_mt_safe(sizes);
    
    /*Inserting back the new splitter at the original index*/
    container->insertWidget(oldIndex,newSplitter);
    
    ///restore the container original sizes
    container->setSizes_mt_safe(containerSizes);
    
    _userSplits.push_back(std::make_pair(newTab,false));
    
    if (!_gui->getApp()->getProject()->isLoadingProject() && autoSave) {
        _gui->getApp()->triggerAutoSave();
    }
    return newTab;
}

TabWidget* TabWidget::splitVertically(bool autoSave) {
        
    Splitter* container = dynamic_cast<Splitter*>(parentWidget());
    if (!container)
        return NULL;
    
    QList<int> containerSizes = container->sizes();
    
    /*We need to know the position in the container layout of the old tab widget*/
    int oldIndex = container->indexOf(this);
    
    Splitter* newSplitter = new Splitter(container);
    newSplitter->setObjectName_mt_safe(container->objectName()+TabWidget::splitVerticallyTag);
    newSplitter->setContentsMargins(0, 0, 0, 0);
    newSplitter->setOrientation(Qt::Vertical);
    setVisible(false);
    setParent(newSplitter);
    newSplitter->addWidget(this);
    setVisible(true);
    _gui->registerSplitter(newSplitter);
    
    
    /*Adding now a new tab*/
    TabWidget* newTab = new TabWidget(_gui,newSplitter);
    
    ///count the number of vertical splits that were already made to uniquely identify this split
    int noVertiSplit = 0;
    for (std::list< std::pair<TabWidget*,bool> >::iterator it = _userSplits.begin();it!=_userSplits.end(); ++it) {
        if (it->second) {
            ++noVertiSplit;
        }
    }
    
    newTab->setObjectName_mt_safe(objectName()+TabWidget::splitVerticallyTag + QString::number(noVertiSplit));
    _gui->registerPane(newTab);
    newSplitter->addWidget(newTab);
    
    QSize splitterSize = newSplitter->sizeHint();
    QList<int> sizes; sizes <<   splitterSize.height()/2;
    sizes  << splitterSize.height()/2;
    newSplitter->setSizes_mt_safe(sizes);
    /*Inserting back the new splitter at the original index*/
    container->insertWidget(oldIndex,newSplitter);
    
    ///restore the container original sizes
    container->setSizes_mt_safe(containerSizes);
    
    _userSplits.push_back(std::make_pair(newTab,true));
    
    if (!_gui->getApp()->getProject()->isLoadingProject() && autoSave) {
            _gui->getApp()->triggerAutoSave();
    }
    return newTab;
}


bool TabWidget::appendTab(QWidget* widget) {
    return appendTab(QIcon(),widget);
}
bool TabWidget::appendTab(const QIcon& icon,QWidget* widget){
    {
        QMutexLocker l(&_tabWidgetStateMutex);
        
        ///If we do not know the tab ignore it
        QString title = widget->objectName();
        if (title.isEmpty())
            return false;
        
        /*registering this tab for drag&drop*/
        _gui->registerTab(widget);
        
        _tabs.push_back(widget);
        widget->setParent(this);
        _modifyingTabBar = true;
        _tabBar->addTab(icon,title);
        _tabBar->updateGeometry(); //< necessary
        _modifyingTabBar = false;
        if(_tabs.size() == 1){
            for (int i =0; i < _mainLayout->count(); ++i) {
                QSpacerItem* item = dynamic_cast<QSpacerItem*>(_mainLayout->itemAt(i));
                if(item)
                    _mainLayout->removeItem(item);
            }
        }
        _floatButton->setEnabled(true);
        
    }
    makeCurrentTab(_tabs.size()-1);
    
    return true;
}



void TabWidget::insertTab(int index,const QIcon& icon,QWidget* widget) {
    
    QMutexLocker l(&_tabWidgetStateMutex);
    QString title = widget->objectName();
    if ((U32)index < _tabs.size()) {
        /*registering this tab for drag&drop*/
        _gui->registerTab(widget);
        
        _tabs.insert(_tabs.begin()+index, widget);
        _modifyingTabBar = true;
        _tabBar->insertTab(index,icon ,title);
        _tabBar->updateGeometry(); //< necessary
        _modifyingTabBar = false;
    }else{
        appendTab(widget);
    }
    _floatButton->setEnabled(true);
    
}

void TabWidget::insertTab(int index,QWidget* widget){
    insertTab(index, QIcon(), widget);
}

QWidget*  TabWidget::removeTab(int index) {
    QMutexLocker l(&_tabWidgetStateMutex);
    if (index < 0 || index >= (int)_tabs.size()) {
        return NULL;
    }
    QWidget* tab = _tabs[index];
    _gui->unregisterTab(tab);
    _tabs.erase(_tabs.begin()+index);
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
        if (_isFloating && !_gui->isDraggingPanel()) {
            l.unlock();
            closeFloatingPane();
            l.relock();
        }
    }
    tab->setParent(NULL);
    return tab;
}

void TabWidget::removeTab(QWidget* widget){
    
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


void TabWidget::makeCurrentTab(int index){

    if (_modifyingTabBar) {
        return;
    }
    QWidget* tab;
    {
        QMutexLocker l(&_tabWidgetStateMutex);
        if(index < 0 || index >= (int)_tabs.size()) {
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



void TabWidget::paintEvent(QPaintEvent* event){
    if (_drawDropRect) {
        
        QColor c(243,149,0,255);
        QPalette* palette = new QPalette();
        palette->setColor(QPalette::Foreground,c);
        setPalette(*palette);
    }
    QFrame::paintEvent(event);
}

void TabWidget::dropEvent(QDropEvent* event){
    event->accept();
    QString name(event->mimeData()->data("Tab"));
    QWidget* w = _gui->findExistingTab(name.toStdString());
    if(w){
        moveTab(w, this);
    }
    _drawDropRect = false;
    setFrameShape(QFrame::NoFrame);
    repaint();
}


TabBar::TabBar(TabWidget* tabWidget,QWidget* parent)
: QTabBar(parent)
, _dragPos()
, _dragPix(0)
, _tabWidget(tabWidget)
{
    setTabsClosable(true);
    setMouseTracking(true);
    QObject::connect(this, SIGNAL(tabCloseRequested(int)), tabWidget, SLOT(closeTab(int)));
}

void TabBar::mousePressEvent(QMouseEvent* event){
    if (event->button() == Qt::LeftButton)
        _dragPos = event->pos();
    QTabBar::mousePressEvent(event);
}

void TabBar::mouseMoveEvent(QMouseEvent* event){
    // If the left button isn't pressed anymore then return
    if (!(event->buttons() & Qt::LeftButton))
        return;
    // If the distance is too small then return
    if ((event->pos() - _dragPos).manhattanLength() < QApplication::startDragDistance())
        return;
    
    if (_tabWidget->getGui()->isDraggingPanel()) {
        
        const QPoint& globalPos = event->globalPos();
        const std::list<TabWidget*> panes = _tabWidget->getGui()->getPanes();
        for (std::list<TabWidget*>::const_iterator it = panes.begin(); it!=panes.end(); ++it) {
            if ((*it)->isWithinWidget(globalPos)) {
                (*it)->setDrawDropRect(true);
            } else {
                (*it)->setDrawDropRect(false);
            }
        }
        _dragPix->update(globalPos);
        QTabBar::mouseMoveEvent(event);
        return;
    }
    int selectedTabIndex = tabAt(event->pos());
    if(selectedTabIndex != -1){
        
        QPixmap pixmap = makePixmapForDrag(selectedTabIndex);
        
        _tabWidget->startDragTab(selectedTabIndex);
        
        _dragPix = new DragPixmap(pixmap,event->pos());
        _dragPix->update(event->globalPos());
        _dragPix->show();
        grabMouse();
        
        
        //#if QT_VERSION < 0x050000
        //        QPixmap pix = QPixmap::grabWidget(_tabWidget);
        //#else
        //        QPixmap pix = _tabWidget->grab();
        //#endif
        //        drag->setPixmap(pix);
        //        drag->exec();
    }
    QTabBar::mouseMoveEvent(event);
    
}

QPixmap TabBar::makePixmapForDrag(int index) {
    std::vector< std::pair<QString,QIcon > > tabs;
    for (int i = 0; i < count(); ++i) {
        tabs.push_back(std::make_pair(tabText(i),tabIcon(i)));
    }
    
    //remove all tabs
    while (count() > 0) {
        removeTab(0);
    }
    
    ///insert just the tab we want to screen shot
    addTab(tabs[index].second, tabs[index].first);
    
    QPixmap currentTabPixmap =  Gui::screenShot(_tabWidget->tabAt(index));
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
        QRgb* src_pixels = reinterpret_cast<QRgb*>(tabBarImg.scanLine(y));
        for (int x = 0; x < ret.width(); ++x) {
            if (x < tabBarImg.width()) {
                QRgb pix = src_pixels[x];
                ret.setPixel(x, y, qRgba(qRed(pix), qGreen(pix), qBlue(pix), 255));
            } else {
                ret.setPixel(x, y, qRgba(0, 0, 0, 128));
            }
        }
    }
    
    for (int y = 0; y < currentTabImg.height(); ++y) {
        QRgb* src_pixels = reinterpret_cast<QRgb*>(currentTabImg.scanLine(y));
        for (int x = 0; x < ret.width(); ++x) {
            QRgb pix = src_pixels[x];
            ret.setPixel(x, y + tabBarImg.height(), qRgba(qRed(pix), qGreen(pix), qBlue(pix), 255));
        }
    }
    
    return QPixmap::fromImage(ret);
}

void TabBar::mouseReleaseEvent(QMouseEvent* event) {
    if (_tabWidget->getGui()->isDraggingPanel()) {
        releaseMouse();
        const QPoint& p = event->globalPos();
        _tabWidget->stopDragTab(p);
        _dragPix->hide();
        delete _dragPix;
        _dragPix = 0;
        
        const std::list<TabWidget*> panes = _tabWidget->getGui()->getPanes();
        for (std::list<TabWidget*>::const_iterator it = panes.begin(); it!=panes.end(); ++it) {
            (*it)->setDrawDropRect(false);
        }
        
    }
    QTabBar::mouseReleaseEvent(event);
    
}

void TabWidget::stopDragTab(const QPoint& globalPos) {
    
    if (_isFloating && count() == 0) {
        closeFloatingPane();
    }
    
    QWidget* draggedPanel = _gui->stopDragPanel();
    const std::list<TabWidget*> panes = _gui->getPanes();
    
    bool foundTabWidgetUnderneath = false;
    for (std::list<TabWidget*>::const_iterator it = panes.begin(); it!=panes.end(); ++it) {
        if ((*it)->isWithinWidget(globalPos)) {
            (*it)->appendTab(draggedPanel);
            foundTabWidgetUnderneath = true;
            break;
        }
    }
    
    if (!foundTabWidgetUnderneath) {
        ///if we reach here that means the mouse is not over any tab widget, then float the panel
        QPoint windowPos = globalPos;
        floatPane(&windowPos);
//        TabWidget* newTab = new TabWidget(_gui);
//        newTab->appendTab(draggedPanel);
//        newTab->floatPane(&windowPos);
        
    }
    
}

void TabWidget::startDragTab(int index){
    if (index >= count()) {
        return;
    }
    
    QWidget* selectedTab = tabAt(index);
    assert(selectedTab);
    selectedTab->setParent(this);
    
    _gui->startDragPanel(selectedTab);
    
    removeTab(selectedTab);
    selectedTab->hide();
    
    
}

void TabWidget::setDrawDropRect(bool draw) {
    
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

bool TabWidget::isWithinWidget(const QPoint& globalPos) const {
    QPoint p = mapToGlobal(QPoint(0,0));
    QRect bbox(p.x(),p.y(),width(),height());
    return bbox.contains(globalPos);
}



void TabWidget::keyPressEvent ( QKeyEvent * event ){
    if(event->key() == Qt::Key_Space && event->modifiers() == Qt::NoModifier){
        if(_fullScreen){
            _fullScreen = false;
            _gui->minimize();
        }else{
            _fullScreen = true;
            _gui->maximize(this);
        }
    }else{
        event->ignore();
    }
}


bool TabWidget::moveTab(QWidget* what,TabWidget *where){
    TabWidget* from = dynamic_cast<TabWidget*>(what->parentWidget());
    

    if (from == where) {
        /*We check that even if it is the same TabWidget, it really exists.*/
        bool found = false;
        for (int i =0; i < from->count(); ++i) {
            if (what == from->tabAt(i)) {
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
    if (!where->getGui()->getApp()->getProject()->isLoadingProject()) {
        where->getGui()->getApp()->triggerAutoSave();
    }
    return true;
}

void FloatingWidget::moveEvent(QMoveEvent* event)
{
    QWidget::moveEvent(event);
    TabWidget* isTab = dynamic_cast<TabWidget*>(_embeddedWidget);
    if (isTab) {
        isTab->setWindowPos(pos());
    }
}

void FloatingWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    TabWidget* isTab = dynamic_cast<TabWidget*>(_embeddedWidget);
    if (isTab) {
        isTab->setWindowSize(width(), height());
    }
}

QPoint TabWidget::getWindowPos_mt_safe() const
{
    QMutexLocker l(&_tabWidgetStateMutex);
    return _mtSafeWindowPos;
}

void TabWidget::getWindowSize_mt_safe(int& w,int& h) const
{
    QMutexLocker l(&_tabWidgetStateMutex);
    w = _mtSafeWindowWidth;
    h = _mtSafeWindowHeight;
}

void TabWidget::setWindowPos(const QPoint& globalPos)
{
    QMutexLocker l(&_tabWidgetStateMutex);
    _mtSafeWindowPos = globalPos;
}

void TabWidget::setWindowSize(int w,int h)
{
    QMutexLocker l(&_tabWidgetStateMutex);
    _mtSafeWindowWidth = w;
    _mtSafeWindowHeight = h;
}

DragPixmap::DragPixmap(const QPixmap& pixmap,const QPoint& offsetFromMouse)
: QWidget(0, Qt::ToolTip | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint)
, _pixmap(pixmap)
, _offset(offsetFromMouse)
{
    setAttribute( Qt::WA_TransparentForMouseEvents );
    resize(_pixmap.width(), _pixmap.height());
    setWindowOpacity(0.7);
}


void DragPixmap::update(const QPoint& globalPos) {
    move(globalPos - _offset);
}

void DragPixmap::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.drawPixmap(0,0,_pixmap);
}


QStringList TabWidget::getTabNames() const {
    QMutexLocker l(&_tabWidgetStateMutex);
    QStringList ret;
    for (U32 i = 0; i < _tabs.size(); ++i) {
        ret << _tabs[i]->objectName();
    }
    return ret;
}

int TabWidget::activeIndex() const
{
    QMutexLocker l(&_tabWidgetStateMutex);
    for (U32 i = 0; i < _tabs.size(); ++i) {
        if (_tabs[i] == _currentWidget) {
            return i;
        }
    }
    return -1;
}

void TabWidget::setObjectName_mt_safe(const QString& str) {
    QMutexLocker l(&_tabWidgetStateMutex);
    setObjectName(str);
}

QString TabWidget::objectName_mt_safe() const {
    QMutexLocker l(&_tabWidgetStateMutex);
    return objectName();
}
