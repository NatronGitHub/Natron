//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef SMARTINPUTDIALOG_H
#define SMARTINPUTDIALOG_H

#include <QtWidgets/QDialog>

class NodeGraph;
class Controler;
class QComboBox;
class QLabel;
class QVBoxLayout;
class QEvent;
class QKeyEvent;
class SmartInputDialog:public QDialog
{
Q_OBJECT

public:
    SmartInputDialog(NodeGraph* graph);
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
