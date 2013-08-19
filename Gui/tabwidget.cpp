//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 




#include "tabwidget.h"

#include <QLayout>
#include <QMenu>
#include <QApplication>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtCore/QMimeData>
#include <QtGui/QDrag>
#include <QScrollArea>

#include "Superviser/powiterFn.h"
#include "Gui/button.h"
#include "Superviser/controler.h"
#include "Gui/mainGui.h"
#include "Gui/DAG.h"
#include "Gui/viewerTab.h"
#include "Core/ViewerNode.h"

TabWidget::TabWidget(TabWidget::Decorations decorations,QWidget* parent):
QFrame(parent),
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
    QImage imgC(IMAGES_PATH"close.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC.scaled(15,15);
    QImage imgM(IMAGES_PATH"maximize.png");
    QPixmap pixM=QPixmap::fromImage(imgM);
    pixM.scaled(15,15);
    QImage imgL(IMAGES_PATH"layout.png");
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
        _floatButton->setToolTip(tr("Create a new window with the currently displayed tab."));
        _floatButton->setFixedSize(15,15);
        QObject::connect(_floatButton, SIGNAL(clicked()), this, SLOT(floatCurrentWidget()));
        _headerLayout->addWidget(_floatButton);
        
        _closeButton = new Button(QIcon(pixC),"",_header);
        _closeButton->setToolTip(tr("Close the currently displayed tab."));
        _closeButton->setFixedSize(15,15);
        QObject::connect(_closeButton, SIGNAL(clicked()), this, SLOT(closeCurrentWidget()));
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

void TabWidget::destroyTabs(){
    
    for (U32 i = 0; i < _tabs.size(); i++) {
        destroyTab(_tabs[i]);
    }
}
void TabWidget::destroyTab(QWidget* tab){
    /*special care is taken if this is a viewer: we also
     need to delete the viewer node.*/
    ViewerTab* isViewer = dynamic_cast<ViewerTab*>(tab);
    if (isViewer) {
        ctrlPTR->getGui()->removeViewerTab(isViewer,false);
    }else if(tab->objectName() != "DAG_GUI" && tab->objectName() != "Properties_GUI"){
        /*Otherwise delete it normally*/
        delete tab;
    }else{
        tab->setVisible(false);
    }
    
}

void TabWidget::createMenu(){
    QMenu *menu = new QMenu(_leftCornerButton);
    QImage imgV(IMAGES_PATH"splitVertically.png");
    QPixmap pixV=QPixmap::fromImage(imgV);
    pixV.scaled(12,12);
    QImage imgH(IMAGES_PATH"splitHorizontally.png");
    QPixmap pixH=QPixmap::fromImage(imgH);
    QImage imgC(IMAGES_PATH"close.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC.scaled(12,12);
    QImage imgM(IMAGES_PATH"maximize.png");
    QPixmap pixM=QPixmap::fromImage(imgM);
    pixM.scaled(12,12);
    pixH.scaled(12,12);
    menu->addAction(QIcon(pixV),tr("Split vertical"), this, SLOT(splitVertically()));
    menu->addAction(QIcon(pixH),tr("Split horizontal"), this, SLOT(splitHorizontally()));
    menu->addSeparator();
    menu->addAction(QIcon(pixM),tr("Float pane"), this, SLOT(floatPane()));
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
    ctrlPTR->getGui()->closePane(this);
}

void TabWidget::floatPane(){
    _isFloating = true;
    ctrlPTR->getGui()->floatWidget(this);
}

void TabWidget::addNewViewer(){
    ctrlPTR->getGui()->setNewViewerAnchor(this);
    ctrlPTR->createNode("Viewer");
}

void TabWidget::moveNodeGraphHere(){
    QWidget* what = dynamic_cast<QWidget*>(ctrlPTR->getGui()->_nodeGraphTab->_nodeGraphArea);
    ctrlPTR->getGui()->moveTab(what,this);
}
/*Get the header name of the tab at index "index".*/
QString TabWidget::getTabName(int index) const {
    if(index >= _tabBar->count()) return "";
    return _tabBar->tabText(index);
}

QString TabWidget::getTabName(QWidget* tab) const{
    for (U32 i = 0; i < _tabs.size(); i++) {
        if (_tabs[i] == tab) {
            return _tabBar->tabText(i);
        }
    }
    return "";
}

void TabWidget::floatCurrentWidget(){
    if(!_currentWidget) return;
    TabWidget* newTab = new TabWidget(TabWidget::CLOSABLE);
    newTab->appendTab(getTabName(_currentWidget), _currentWidget);
    newTab->floatPane();
    removeTab(_currentWidget);
}

void TabWidget::closeCurrentWidget(){
    if(!_currentWidget) return;
    destroyTab(_currentWidget);
    removeTab(_currentWidget);
}
void TabWidget::movePropertiesBinHere(){
    QWidget* what = dynamic_cast<QWidget*>(ctrlPTR->getGui()->_propertiesScrollArea);
    ctrlPTR->getGui()->moveTab(what, this);
}

void TabWidget::splitHorizontally(){
    ctrlPTR->getGui()->splitPaneHorizontally(this);
}

void TabWidget::splitVertically(){
    ctrlPTR->getGui()->splitPaneVertically(this);
}


bool TabWidget::appendTab(const QString& title,QWidget* widget){
    if(_decorations!=NONE && title.isEmpty()) return false;
    
    /*registering this tab for drag&drop*/
    ctrlPTR->getGui()->registerTab(title.toStdString(), widget);
    
    _tabs.push_back(widget);
    _tabBar->addTab(title);
    if(_tabs.size() == 1){
        for (int i =0; i < _mainLayout->count(); i++) {
            QSpacerItem* item = dynamic_cast<QSpacerItem*>(_mainLayout->itemAt(i));
            if(item)
                _mainLayout->removeItem(item);
        }
    }
    makeCurrentTab(_tabs.size()-1);
    _tabBar->setCurrentIndex(_tabs.size()-1);
    return true;
}
bool TabWidget::appendTab(const QString& title,const QIcon& icon,QWidget* widget){
    if(_decorations!=NONE && title.isEmpty()) return false;
    
    /*registering this tab for drag&drop*/
    ctrlPTR->getGui()->registerTab(title.toStdString(), widget);
    
    _tabs.push_back(widget);
    _tabBar->addTab(icon,title);
    if(_tabs.size() == 1){
        for (int i =0; i < _mainLayout->count(); i++) {
            QSpacerItem* item = dynamic_cast<QSpacerItem*>(_mainLayout->itemAt(i));
            if(item)
                _mainLayout->removeItem(item);
        }
    }
    makeCurrentTab(_tabs.size()-1);
    _tabBar->setCurrentIndex(_tabs.size()-1);
    return true;
}

void TabWidget::insertTab(int index,const QIcon& icon,const QString &title,QWidget* widget){
    if ((U32)index < _tabs.size()) {
        /*registering this tab for drag&drop*/
        ctrlPTR->getGui()->registerTab(title.toStdString(), widget);
        
        _tabs.insert(_tabs.begin()+index, widget);
        _tabBar->insertTab(index,icon ,title);
    }else{
        appendTab(title, widget);
    }
    
}

void TabWidget::insertTab(int index,const QString &title,QWidget* widget){
    if (index < (int)_tabs.size()) {
        /*registering this tab for drag&drop*/
        ctrlPTR->getGui()->registerTab(title.toStdString(), widget);
        
        _tabs.insert(_tabs.begin()+index, widget);
        _tabBar->insertTab(index,title);
    }else{
        appendTab(title, widget);
    }
    
}

QWidget*  TabWidget::removeTab(int index){
    if(index < (int)_tabs.size()){
        QWidget* tab = _tabs[index];
        _tabs.erase(_tabs.begin()+index);
        _tabBar->removeTab(index);
        if(_tabs.size() > 0){
            makeCurrentTab(0);
        }
        else{
            _currentWidget = 0;
            _mainLayout->addStretch();
            if (_isFloating) {
                closeFloatingPane();
            }
        }
        return tab;
    }else{
        return NULL;
    }
}
void TabWidget::removeTab(QWidget* widget){
    for (U32 i = 0; i < _tabs.size(); i++) {
        if (_tabs[i] == widget) {
            _tabs.erase(_tabs.begin()+i);
            _tabBar->removeTab(i);
            if(_tabs.size() > 0){
                makeCurrentTab(0);
            }else{
                _currentWidget = 0;
                _mainLayout->addStretch();
                if (_isFloating) {
                    closeFloatingPane();
                }
            }
            break;
        }
    }
}

void TabWidget::makeCurrentTab(int index){
    if((U32)index < _tabs.size()){
        /*Removing previous widget if any*/
        if(_currentWidget){
            _currentWidget->setVisible(false);
            _mainLayout->removeWidget(_currentWidget);
            _currentWidget->setParent(0);
        }
        _mainLayout->addWidget(_tabs[index]);
        _currentWidget = _tabs[index];
        _currentWidget->setVisible(true);
        _currentWidget->setParent(this);
    }
}

void TabWidget::dragEnterEvent(QDragEnterEvent* event){
    // Only accept if it's an tab-reordering request
    const QMimeData* m = event->mimeData();
    QStringList formats = m->formats();
    if (formats.contains("Tab")) {
        event->acceptProposedAction();
        _drawDropRect = true;
        setFrameShape(QFrame::Box);
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
    QWidget* w = ctrlPTR->getGui()->findExistingTab(name.toStdString());
    if(w){
        ctrlPTR->getGui()->moveTab(w, this);
    }
    _drawDropRect = false;
    setFrameShape(QFrame::NoFrame);
    repaint();
}


TabBar::TabBar(TabWidget* tabWidget,QWidget* parent): QTabBar(parent) , _tabWidget(tabWidget) /* ,_currentIndex(-1)*/{
    
    //    _mainLayout = new QHBoxLayout(this);
    //    _mainLayout->setContentsMargins(0, 0, 0, 0);
    //    _mainLayout->setSpacing(0);
    //    setLayout(_mainLayout);
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
//
//void TabBar::addTab(const QString& text){
//    _mainLayout->addWidget(new Tab(QIcon(),text,this));
//}
//
//void TabBar::addTab(const QIcon& icon,const QString& text){
//    _mainLayout->addWidget(new Tab(icon,text,this));
//}
//
//void TabBar::insertTab(int index,const QString& text){
//    _mainLayout->insertWidget(index,new Tab(QIcon(),text,this));
//}
//
//void TabBar::insertTab(int index,const QIcon& icon,const QString& text){
//    _mainLayout->insertWidget(index,new Tab(icon,text,this));
//}

//void TabBar::removeTab(int index){
//    _mainLayout->removeWidget(dynamic_cast<Tab*>(_mainLayout->itemAt(index)));
//    setCurrentIndex(index-1);
//}
//
//QString TabBar::tabText(int index) const{
//    Tab* tab = dynamic_cast<Tab*>(_mainLayout->itemAt(index));
//    QString ret;
//    if (tab) {
//        ret = tab->text();
//    }
//    return ret;
//}
//
//int TabBar::count() const {
//    return _mainLayout->count();
//}
//
//void TabBar::setCurrentIndex(int index){
//    if (index == -1) {
//        _currentIndex = -1;
//        emit currentChanged(-1);
//    }
//    for (int i =0 ; i < count(); i++) {
//        Tab* tab = dynamic_cast<Tab*>(_mainLayout->itemAt(index));
//        if (tab) {
//            if (i == index) {
//                _currentIndex = index;
//                emit currentChanged(index);
//                tab->setCurrent(true);
//            }else{
//                tab->setCurrent(false);
//            }
//        }
//    }
//}

//int TabBar::currentIndex() const{
//    return _currentIndex;
//}
//
//Tab::Tab(const QIcon& icon,const QString& text,QWidget* parent): QFrame(parent),isCurrent(false){
//    _layout = new QHBoxLayout(this);
//    _layout->setContentsMargins(0, 0, 5, 0);
//    setLayout(_layout);
//    _icon = new QLabel(this);
//    if (!icon.isNull()) {
//        _icon->setPixmap(icon.pixmap(15, 15));
//    }
//    _text = new QLabel(text,this);
//    _layout->addWidget(_icon);
//    _layout->addWidget(_text);
//    setFrameShape(QFrame::Box);
//}

//void Tab::setCurrent(bool cur){isCurrent = cur;}
//
//bool Tab::current() const {return isCurrent;}
//
//void Tab::paintEvent(QPaintEvent* e){
//
//    QFrame::paintEvent(e);
//}
//
//void Tab::setIcon(const QIcon& icon){
//    if (!icon.isNull()) {
//        _icon->setPixmap(icon.pixmap(15, 15));
//    }
//}
//
//void Tab::setText(const QString& text){
//    _text->setText(text);
//}

//QString Tab::text() const{
//    return _text->text();
//}
