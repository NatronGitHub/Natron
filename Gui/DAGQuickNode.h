#ifndef SMARTINPUTDIALOG_H
#define SMARTINPUTDIALOG_H

#include <QtGui/QFrame>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>
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
    SmartInputDialog(Controler* ctrl,NodeGraph* graph, QWidget *parent=0);
    virtual ~SmartInputDialog(){}
    void keyPressEvent(QKeyEvent *e);
    bool eventFilter(QObject * obj, QEvent * e);
private:
    NodeGraph* graph;
    QVBoxLayout* layout;
    QLabel* textLabel;
    QComboBox* textEdit;
    Controler* ctrl;
};

#endif // SMARTINPUTDIALOG_H
