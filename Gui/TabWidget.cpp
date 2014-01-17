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

#include <QLayout>
#include <QMenu>
#include <QApplication>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtCore/QMimeData>
#include <QtGui/QDrag>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDragLeaveEvent>
#include <QtGui/QPaintEvent>
#include <QScrollArea>
#include <QSplitter>

#include "Gui/Button.h"
#include "Global/AppManager.h"
#include "Gui/Gui.h"
#include "Gui/NodeGraph.h"
#include "Gui/CurveEditor.h"
#include "Gui/ViewerTab.h"
#include "Engine/ViewerInstance.h"

using namespace Natron;

const QString TabWidget::splitHorizontallyTag = QString("_horizSplit");
const QString TabWidget::splitVerticallyTag = QString("_vertiSplit");

FloatingWidget::FloatingWidget(QWidget* parent)
: QWidget(parent)
, _embeddedWidget(0)
, _layout(0)
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
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
    w->setParent(this);
    assert(_layout);
    _layout->addWidget(w);
    w->setVisible(true);
    resize(widgetSize);
    show();
}



TabWidget::TabWidget(Gui* gui,TabWidget::Decorations decorations,QWidget* parent):
    QFrame(parent),
    _gui(gui),
    _header(0),
    _headerLayout(0),
    _tabBar(0),
    _leftCornerButton(0),
    _floatButton(0),
    _closeButton(0),
    _currentWidget(0),
    _decorations(decorations),
    _isFloating(false),
    _drawDropRect(false),
    _fullScreen(false),
    _userSplits()
{
    
    if(decorations!=NONE){
        setAcceptDrops(true);
    }
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

    if(decorations != NONE){
        
        _leftCornerButton = new Button(QIcon(pixL),"",_header);
        _leftCornerButton->setToolTip(tr("Manage the layouts for this pane."));
        _leftCornerButton->setFixedSize(15,15);
        _headerLayout->addWidget(_leftCornerButton);
        _headerLayout->addSpacing(10);
    }
    _tabBar = new TabBar(this,_header);
    _tabBar->setShape(QTabBar::RoundedNorth);
    _tabBar->setDrawBase(false);
    QObject::connect(_tabBar, SIGNAL(currentChanged(int)), this, SLOT(makeCurrentTab(int)));
    _headerLayout->addWidget(_tabBar);
    _headerLayout->addStretch();
    if(decorations != NONE){
        _floatButton = new Button(QIcon(pixM),"",_header);
        _floatButton->setToolTip(tr("Float pane."));
        _floatButton->setFixedSize(15,15);
        _floatButton->setEnabled(true);
        QObject::connect(_floatButton, SIGNAL(clicked()), this, SLOT(floatCurrentWidget()));
        _headerLayout->addWidget(_floatButton);
        
        _closeButton = new Button(QIcon(pixC),"",_header);
        _closeButton->setToolTip(tr("Close pane."));
        _closeButton->setFixedSize(15,15);
        if(decorations == NOT_CLOSABLE){
            _closeButton->setVisible(false);
        }
        QObject::connect(_closeButton, SIGNAL(clicked()), this, SLOT(closePane()));
        _headerLayout->addWidget(_closeButton);
        
        
        /*adding menu to the left corner button*/
        _leftCornerButton->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(_leftCornerButton, SIGNAL(clicked()), this, SLOT(createMenu()));
    }
    
    _mainLayout->addWidget(_header);
    _mainLayout->addStretch();
}

TabWidget::~TabWidget(){
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
    if (isViewer) {
        _gui->removeViewerTab(isViewer,false);
    } else {
        tab->setVisible(false);
    }
    return false; // must not be deleted
}

void TabWidget::createMenu(){
    QMenu *menu = new QMenu(_gui);
    QPixmap pixV,pixM,pixH,pixC;
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY,&pixV);
    appPTR->getIcon(NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY,&pixH);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET,&pixM);
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_WIDGET,&pixC);
    QAction* splitVerticallyAction = new QAction(QIcon(pixV),tr("Split vertical"),this);
    QObject::connect(splitVerticallyAction, SIGNAL(triggered()), this, SLOT(splitVertically()));
    menu->addAction(splitVerticallyAction);
    QAction* splitHorizontallyAction = new QAction(QIcon(pixH),tr("Split horizontal"),this);
    QObject::connect(splitHorizontallyAction, SIGNAL(triggered()), this, SLOT(splitHorizontally()));
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
    if (_decorations == NOT_CLOSABLE) {
        closeAction->setEnabled(false);
    }
    QObject::connect(closeAction, SIGNAL(triggered()), this, SLOT(closePane()));
    menu->addAction(closeAction);
    menu->addSeparator();
    menu->addAction(tr("New viewer"), this, SLOT(addNewViewer()));
    menu->addAction(tr("Node graph here"), this, SLOT(moveNodeGraphHere()));
    menu->addAction(tr("Curve Editor here"), this, SLOT(moveCurveEditorHere()));
    menu->addAction(tr("Properties bin here"), this, SLOT(movePropertiesBinHere()));
    menu->exec(_leftCornerButton->mapToGlobal(QPoint(0,0)));
}

void TabWidget::closeFloatingPane(){
    if (!_isFloating) {
        return;
    }
    QWidget* p = parentWidget();
    p->close();
}

void TabWidget::removeSplit(TabWidget* tab){
    std::map<TabWidget*,bool> ::iterator it = _userSplits.find(tab);
    assert(it!= _userSplits.end());
    _userSplits.erase(it);
}

void TabWidget::closePane(){
    /*If it is floating we do not need to re-arrange the splitters containing the tab*/
    if (isFloating()) {
        parentWidget()->close();
        destroyTabs();
        return;
    }
    
    QSplitter* container = dynamic_cast<QSplitter*>(parentWidget());
    if(!container) {
        return;
    }
    
    /*Removing it from the _panes vector*/
    _gui->removePane(this);
    
    /*Only sub-panes are closable. That means the splitter owning them must also
     have a splitter as parent*/
    QSplitter* mainContainer = dynamic_cast<QSplitter*>(container->parentWidget());
    if(!mainContainer) {
        return;
    }
    
    /*identifying the other tab*/
    TabWidget* other = 0;
    for (int i = 0; i < container->count(); ++i) {
        TabWidget* tab = dynamic_cast<TabWidget*>(container->widget(i));
        if (tab && tab != this) {
            other = tab;
            break;
        }
    }
    assert(other);
    other->removeSplit(this);

    /*Removing "what" from the container and delete it*/
    setVisible(false);
    //move all its tabs to the other TabWidget
    while(count() > 0) {
        moveTab(tabAt(0), other);
    }
    // delete what;
    
    /*Removing the container from the mainContainer*/
    int subSplitterIndex = 0;
    for (int i = 0; i < mainContainer->count(); ++i) {
        QSplitter* subSplitter = dynamic_cast<QSplitter*>(mainContainer->widget(i));
        if (subSplitter && subSplitter == container) {
            subSplitterIndex = i;
            container->setVisible(false);
            container->setParent(0);
            break;
        }
    }
    /*moving the other to the mainContainer*/
    if(other){
        other->setVisible(true);
        other->setParent(mainContainer);
    }
    mainContainer->insertWidget(subSplitterIndex, other);
    
    /*deleting the subSplitter*/
    _gui->removeSplitter(container);
    delete container;
}

void TabWidget::floatPane(){
    _isFloating = true;
    
    FloatingWidget* floatingW = new FloatingWidget(_gui);
    setVisible(false);
    setParent(0);
    floatingW->setWidget(size(),this);

}

void TabWidget::addNewViewer(){
    _gui->setNewViewerAnchor(this);
    _gui->getApp()->createNode("Viewer");
}

void TabWidget::moveNodeGraphHere(){
    QWidget* what = dynamic_cast<QWidget*>(_gui->_nodeGraphArea);
    what->setParent(this);
    moveTab(what,this);
}

void TabWidget::moveCurveEditorHere(){
    QWidget* what = dynamic_cast<QWidget*>(_gui->_curveEditor);
    what->setParent(this);
    moveTab(what,this);
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
    tab->setObjectName(name);
    for (U32 i = 0; i < _tabs.size(); ++i) {
        if (_tabs[i] == tab) {
            _tabBar->setTabText(i, name);
        }
    }
}

void TabWidget::floatCurrentWidget(){
    if(!_currentWidget)
        return;
    TabWidget* newTab = new TabWidget(_gui,TabWidget::CLOSABLE);
    newTab->appendTab(_currentWidget);
    newTab->floatPane();
    removeTab(_currentWidget);
    if(_tabBar->count() == 0){
        _floatButton->setEnabled(false);
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
    QWidget* what = dynamic_cast<QWidget*>(_gui->_propertiesScrollArea);
    what->setParent(this);
    moveTab(what, this);
}

void TabWidget::splitHorizontally(){

    QSplitter* container = dynamic_cast<QSplitter*>(parentWidget());
    if(!container){
        return;
    }
    
    /*We need to know the position in the container layout of the old tab widget*/
    int oldIndex = container->indexOf(this);
    
    QSplitter* newSplitter = new QSplitter(container);
    newSplitter->setObjectName(container->objectName()+TabWidget::splitHorizontallyTag);
    newSplitter->setContentsMargins(0, 0, 0, 0);
    newSplitter->setOrientation(Qt::Horizontal);
    setVisible(false);
    setParent(newSplitter);
    newSplitter->addWidget(this);
    setVisible(true);
    
    
    /*Adding now a new tab*/
    TabWidget* newTab = new TabWidget(_gui,TabWidget::CLOSABLE,newSplitter);
    newTab->setObjectName(objectName()+TabWidget::splitHorizontallyTag);
    _gui->registerPane(newTab);
    newSplitter->addWidget(newTab);
    
    QSize splitterSize = newSplitter->sizeHint();
    QList<int> sizes; sizes <<   splitterSize.width()/2;
    sizes  << splitterSize.width()/2;
    newSplitter->setSizes(sizes);
    
    /*Inserting back the new splitter at the original index*/
    container->insertWidget(oldIndex,newSplitter);
    _userSplits.insert(std::make_pair(newTab,false));
    
    _gui->getApp()->triggerAutoSave();
}

void TabWidget::splitVertically(){
    // _gui->splitPaneVertically(this);
    QSplitter* container = dynamic_cast<QSplitter*>(parentWidget());
    if(!container) return;
    
    /*We need to know the position in the container layout of the old tab widget*/
    int oldIndex = container->indexOf(this);
    
    QSplitter* newSplitter = new QSplitter(container);
    newSplitter->setObjectName(container->objectName()+TabWidget::splitVerticallyTag);
    newSplitter->setContentsMargins(0, 0, 0, 0);
    newSplitter->setOrientation(Qt::Vertical);
    setVisible(false);
    setParent(newSplitter);
    newSplitter->addWidget(this);
    setVisible(true);
    
    /*Adding now a new tab*/
    TabWidget* newTab = new TabWidget(_gui,TabWidget::CLOSABLE,newSplitter);
    newTab->setObjectName(objectName()+TabWidget::splitVerticallyTag);
    _gui->registerPane(newTab);
    newSplitter->addWidget(newTab);
    
    QSize splitterSize = newSplitter->sizeHint();
    QList<int> sizes; sizes <<   splitterSize.height()/2;
    sizes  << splitterSize.height()/2;
    newSplitter->setSizes(sizes);
    /*Inserting back the new splitter at the original index*/
    container->insertWidget(oldIndex,newSplitter);

    
    _userSplits.insert(std::make_pair(newTab,true));
    
    _gui->getApp()->triggerAutoSave();
}


bool TabWidget::appendTab(QWidget* widget){
    QString title = widget->objectName();
    if(_decorations!=NONE && title.isEmpty()) return false;
    
    /*registering this tab for drag&drop*/
    _gui->registerTab(widget);
    
    _tabs.push_back(widget);
    _tabBar->addTab(title);
    if(_tabs.size() == 1){
        for (int i =0; i < _mainLayout->count(); ++i) {
            QSpacerItem* item = dynamic_cast<QSpacerItem*>(_mainLayout->itemAt(i));
            if(item)
                _mainLayout->removeItem(item);
        }
    }
    makeCurrentTab(_tabs.size()-1);
    _tabBar->setCurrentIndex(_tabs.size()-1);
    _floatButton->setEnabled(true);
    return true;
}
bool TabWidget::appendTab(const QIcon& icon,QWidget* widget){
    QString title = widget->objectName();
    if(_decorations!=NONE && title.isEmpty()) return false;
    
    /*registering this tab for drag&drop*/
    _gui->registerTab(widget);
    
    _tabs.push_back(widget);
    _tabBar->addTab(icon,title);
    if(_tabs.size() == 1){
        for (int i =0; i < _mainLayout->count(); ++i) {
            QSpacerItem* item = dynamic_cast<QSpacerItem*>(_mainLayout->itemAt(i));
            if(item)
                _mainLayout->removeItem(item);
        }
    }
    makeCurrentTab(_tabs.size()-1);
    _tabBar->setCurrentIndex(_tabs.size()-1);
    _floatButton->setEnabled(true);
    return true;
}

void TabWidget::insertTab(int index,const QIcon& icon,QWidget* widget){
    QString title = widget->objectName();
    if ((U32)index < _tabs.size()) {
        /*registering this tab for drag&drop*/
        _gui->registerTab(widget);
        
        _tabs.insert(_tabs.begin()+index, widget);
        _tabBar->insertTab(index,icon ,title);
    }else{
        appendTab(widget);
    }
    _floatButton->setEnabled(true);
    
}

void TabWidget::insertTab(int index,QWidget* widget){
    QString title = widget->objectName();
    if (index < (int)_tabs.size()) {
        /*registering this tab for drag&drop*/
        _gui->registerTab(widget);
        
        _tabs.insert(_tabs.begin()+index, widget);
        _tabBar->insertTab(index,title);
    }else{
        appendTab(widget);
    }
    _floatButton->setEnabled(true);
}

QWidget*  TabWidget::removeTab(int index) {
    if (index < 0 || index >= (int)_tabs.size()) {
        return NULL;
    }
    QWidget* tab = _tabs[index];
    _tabs.erase(_tabs.begin()+index);
    _tabBar->removeTab(index);
    if (_tabs.size() > 0) {
        makeCurrentTab(0);
    } else {
        _currentWidget = 0;
        _mainLayout->addStretch();
        if (_isFloating) {
            closeFloatingPane();
        }
    }
    return tab;
}

void TabWidget::removeTab(QWidget* widget){
    for (U32 i = 0; i < _tabs.size(); ++i) {
        if (_tabs[i] == widget) {
            QWidget* tab = removeTab(i);
            assert(tab == widget);
            (void)tab;
            break;
        }
    }
}

void TabWidget::makeCurrentTab(int index){
    if(index < 0 || index >= (int)_tabs.size()) {
        return;
    }
    /*Removing previous widget if any*/
    if (_currentWidget) {
        _currentWidget->setVisible(false);
        _mainLayout->removeWidget(_currentWidget);
        // _currentWidget->setParent(0);
    }
    QWidget* tab = _tabs[index];
    _mainLayout->addWidget(tab);
    _currentWidget = tab;
    _currentWidget->setVisible(true);
    _currentWidget->setParent(this);
    _tabBar->setCurrentIndex(index);

}

void TabWidget::dragEnterEvent(QDragEnterEvent* event){
    // Only accept if it's an tab-reordering request
    const QMimeData* m = event->mimeData();
    QStringList formats = m->formats();
    if (formats.contains("Tab")) {
        event->acceptProposedAction();
        _drawDropRect = true;
        setFrameShape(QFrame::Box);
    }else{

    }
    repaint();
}

void TabWidget::dragLeaveEvent(QDragLeaveEvent*){
    _drawDropRect = false;
    setFrameShape(QFrame::NoFrame);
    repaint();
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


TabBar::TabBar(TabWidget* tabWidget,QWidget* parent): QTabBar(parent) , _tabWidget(tabWidget) /* ,_currentIndex(-1)*/{
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
    
    // initiate Drag
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    // a crude way to distinguish tab-reodering drags from other drags
    QWidget* selectedTab = 0;
    int selectedTabIndex = tabAt(event->pos());
    if(selectedTabIndex != -1){
        selectedTab = _tabWidget->tabAt(selectedTabIndex);
        assert(selectedTab);
        selectedTab->setParent(_tabWidget);
        QString text = _tabWidget->getTabName(selectedTab);
        mimeData->setData("Tab", text.toLatin1());
        drag->setMimeData(mimeData);
#if QT_VERSION < 0x050000
        QPixmap pix = QPixmap::grabWidget(_tabWidget);
#else
        QPixmap pix = _tabWidget->grab();
#endif
        drag->setPixmap(pix);
        drag->exec();
    }
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
void TabWidget::enterEvent(QEvent *event)
{   QWidget::enterEvent(event);
    setFocus();
}
void TabWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
}

void TabWidget::moveTab(QWidget* what,TabWidget *where){
    TabWidget* from = dynamic_cast<TabWidget*>(what->parentWidget());
    
    if(!from){
        return;
    }
    if(from == where){
        /*We check that even if it is the same TabWidget, it really exists.*/
        bool found = false;
        for (int i =0; i < from->count(); ++i) {
            if (what == from->tabAt(i)) {
                found = true;
                break;
            }
        }
        if (found) {
            return;
        }
        //it wasn't found somehow
    }
    
    from->removeTab(what);
    assert(where);
    where->appendTab(what);
    
    where->getGui()->getApp()->triggerAutoSave();
}

