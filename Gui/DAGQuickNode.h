//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef SMARTINPUTDIALOG_H
#define SMARTINPUTDIALOG_H

#include <QDialog>

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
