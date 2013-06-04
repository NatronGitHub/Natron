//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef SMARTINPUTDIALOG_H
#define SMARTINPUTDIALOG_H

#include <QtWidgets/QFrame>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include "Superviser/controler.h"
#include "QtCore/QStringList"
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
class NodeGraph;
class Controler;
class SmartInputDialog:public QFrame
{
Q_OBJECT

public:
    SmartInputDialog(NodeGraph* graph, QWidget *parent=0);
    virtual ~SmartInputDialog(){}
    void keyPressEvent(QKeyEvent *e);
    bool eventFilter(QObject * obj, QEvent * e);
private:
    NodeGraph* graph;
    QVBoxLayout* layout;
    QLabel* textLabel;
    QComboBox* textEdit;
};

#endif // SMARTINPUTDIALOG_H
