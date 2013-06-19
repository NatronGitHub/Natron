//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "tabwidget.h"
#include <QtWidgets/QtWidgets>
#include "Superviser/powiterFn.h"
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
TabWidget::TabWidget(QWidget* parent):QWidget(parent),_currentWidget(0){
    
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding,
                              QSizePolicy::TabWidget));
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    _mainLayout->setSpacing(0);
    setLayout(_mainLayout);
    _header = new QWidget(this);
    
    _mainLayout->addWidget(_header);
    
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
    
    _leftCornerButton = new QPushButton(_header);
    _leftCornerButton->setFixedSize(15,15);
    _headerLayout->addWidget(_leftCornerButton);
    
    _tabBar = new QTabBar(_header);
    _tabBar->setShape(QTabBar::RoundedNorth);
    _tabBar->setDrawBase(false);
    QObject::connect(_tabBar, SIGNAL(currentChanged(int)), this, SLOT(makeCurrentTab(int)));
    _headerLayout->addWidget(_tabBar);
    
    _floatButton = new QPushButton(QIcon(pixM),"",_header);
    _floatButton->setFixedSize(15,15);
    _headerLayout->addWidget(_floatButton);
    
    _closeButton = new QPushButton(QIcon(pixC),"",_header);
    _closeButton->setFixedSize(15,15);
    _headerLayout->addWidget(_closeButton);
    
}

TabWidget::~TabWidget(){}

void TabWidget::appendTab(const QString& title,QWidget* widget){
    _tabs.push_back(widget);
    _tabBar->addTab(title);
    if(_tabs.size() == 1){
        makeCurrentTab(0);
    }
}
void TabWidget::appendTab(const QString& title,const QIcon& icon,QWidget* widget){
    _tabs.push_back(widget);
    _tabBar->addTab(icon,title);
    if(_tabs.size() == 1){
        makeCurrentTab(0);
    }

}

void TabWidget::insertTab(int index,const QIcon& icon,const QString &title,QWidget* widget){
    if (index < _tabs.size()) {
        _tabs.insert(_tabs.begin()+index, widget);
        _tabBar->insertTab(index,icon ,title);
    }else{
        appendTab(title, widget);
    }
    
}

void TabWidget::insertTab(int index,const QString &title,QWidget* widget){
    if (index < _tabs.size()) {
        _tabs.insert(_tabs.begin()+index, widget);
        _tabBar->insertTab(index,title);
    }else{
        appendTab(title, widget);
    }

}

QWidget*  TabWidget::removeTab(int index){
    if(index < _tabs.size()){
        QWidget* tab = _tabs[index];
        _tabs.erase(_tabs.begin()+index);
        _tabBar->removeTab(index);
        if(_tabs.size() > 0){
            makeCurrentTab(0);
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
            }
            break;
        }
    }
}

void TabWidget::makeCurrentTab(int index){
    if(index < _tabs.size()){
        /*Removing previous widget if any*/
        if(_currentWidget)
            _mainLayout->removeWidget(_currentWidget);
        _mainLayout->addWidget(_tabs[index]);
        _currentWidget = _tabs[index];
    }
}