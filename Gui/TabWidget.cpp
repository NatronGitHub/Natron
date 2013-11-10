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
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDragLeaveEvent>
#include <QtGui/QPaintEvent>
#include <QScrollArea>

#include "Gui/Button.h"
#include "Global/AppManager.h"
#include "Gui/Gui.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"
#include "Engine/ViewerInstance.h"

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
_drawDropRect(false){
    
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
    QImage imgC(NATRON_IMAGES_PATH"close.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC.scaled(15,15);
    QImage imgM(NATRON_IMAGES_PATH"maximize.png");
    QPixmap pixM=QPixmap::fromImage(imgM);
    pixM.scaled(15,15);
    QImage imgL(NATRON_IMAGES_PATH"layout.png");
    QPixmap pixL=QPixmap::fromImage(imgL);
    pixL.scaled(15,15);
    
    
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
    } else if(tab->objectName() != "DAG_GUI" && tab->objectName() != "Properties_GUI") {
        /*Otherwise delete it normally*/
        //delete tab;
        return true; // must be deleted
    } else {
        tab->setVisible(false);
    }
    return false; // must not be deleted
}

void TabWidget::createMenu(){
    QMenu *menu = new QMenu(_gui);
    QImage imgV(NATRON_IMAGES_PATH"splitVertically.png");
    QPixmap pixV=QPixmap::fromImage(imgV);
    pixV.scaled(12,12);
    QImage imgH(NATRON_IMAGES_PATH"splitHorizontally.png");
    QPixmap pixH=QPixmap::fromImage(imgH);
    QImage imgC(NATRON_IMAGES_PATH"close.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC.scaled(12,12);
    QImage imgM(NATRON_IMAGES_PATH"maximize.png");
    QPixmap pixM=QPixmap::fromImage(imgM);
    pixM.scaled(12,12);
    pixH.scaled(12,12);
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


void TabWidget::closePane(){
    _gui->closePane(this);
}

void TabWidget::floatPane(){
    _isFloating = true;
    _gui->floatWidget(this);
}

void TabWidget::addNewViewer(){
    _gui->setNewViewerAnchor(this);
    _gui->getApp()->createNode("Viewer");
}

void TabWidget::moveNodeGraphHere(){
    QWidget* what = dynamic_cast<QWidget*>(_gui->_nodeGraphTab->_nodeGraphArea);
    _gui->moveTab(what,this);
}
/*Get the header name of the tab at index "index".*/
QString TabWidget::getTabName(int index) const {
    if(index >= _tabBar->count()) return "";
    return _tabBar->tabText(index);
}

QString TabWidget::getTabName(QWidget* tab) const{
    for (U32 i = 0; i < _tabs.size(); ++i) {
        if (_tabs[i] == tab) {
            return _tabBar->tabText(i);
        }
    }
    return "";
}

void TabWidget::floatCurrentWidget(){
    if(!_currentWidget)
        return;
    TabWidget* newTab = new TabWidget(_gui,TabWidget::CLOSABLE);
    newTab->appendTab(getTabName(_currentWidget), _currentWidget);
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
}

void TabWidget::movePropertiesBinHere(){
    QWidget* what = dynamic_cast<QWidget*>(_gui->_propertiesScrollArea);
    _gui->moveTab(what, this);
}

void TabWidget::splitHorizontally(){
    _gui->splitPaneHorizontally(this);
}

void TabWidget::splitVertically(){
    _gui->splitPaneVertically(this);
}


bool TabWidget::appendTab(const QString& title,QWidget* widget){
    if(_decorations!=NONE && title.isEmpty()) return false;
    
    /*registering this tab for drag&drop*/
    _gui->registerTab(title.toStdString(), widget);
    
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
bool TabWidget::appendTab(const QString& title,const QIcon& icon,QWidget* widget){
    if(_decorations!=NONE && title.isEmpty()) return false;
    
    /*registering this tab for drag&drop*/
    _gui->registerTab(title.toStdString(), widget);
    
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

void TabWidget::insertTab(int index,const QIcon& icon,const QString &title,QWidget* widget){
    if ((U32)index < _tabs.size()) {
        /*registering this tab for drag&drop*/
        _gui->registerTab(title.toStdString(), widget);
        
        _tabs.insert(_tabs.begin()+index, widget);
        _tabBar->insertTab(index,icon ,title);
    }else{
        appendTab(title, widget);
    }
     _floatButton->setEnabled(true);
    
}

void TabWidget::insertTab(int index,const QString &title,QWidget* widget){
    if (index < (int)_tabs.size()) {
        /*registering this tab for drag&drop*/
        _gui->registerTab(title.toStdString(), widget);
        
        _tabs.insert(_tabs.begin()+index, widget);
        _tabBar->insertTab(index,title);
    }else{
        appendTab(title, widget);
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
        _currentWidget->setParent(0);
    }
    _mainLayout->addWidget(_tabs[index]);
    _currentWidget = _tabs[index];
    _currentWidget->setVisible(true);
    _currentWidget->setParent(this);
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
        _gui->moveTab(w, this);
    }
    _drawDropRect = false;
    setFrameShape(QFrame::NoFrame);
    repaint();
}


TabBar::TabBar(TabWidget* tabWidget,QWidget* parent): QTabBar(parent) , _tabWidget(tabWidget) /* ,_currentIndex(-1)*/{
    setTabsClosable(true);
    QObject::connect(this, SIGNAL(tabCloseRequested(int)), tabWidget, SLOT(closeTab(int)));
}

void TabBar::mousePressEvent(QMouseEvent* event){
    if (event->button() == Qt::LeftButton)
        _dragPos = event->pos(); // m_dragStartPos is a QPoint defined in the header
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
    QString text = _tabWidget->getTabName(_tabWidget->currentWidget());
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
