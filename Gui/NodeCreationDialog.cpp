//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "NodeCreationDialog.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QStringList>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QTimer>
#include <QApplication>
#include <QListView>
#include <QDesktopWidget>
#include <QApplication>
#include <QStringListModel>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Plugin.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiApplicationManager.h"



struct CompleterLineEditPrivate {
    QDialog* dialog;
    QListView* listView;
    QStringListModel* model;
    QStringList words,ids;
    bool quickExitEnabled;
    
    CompleterLineEditPrivate(const QStringList& displayWords,const QStringList& internalIds,bool quickExit,QDialog* parent)
    : dialog(parent)
    , listView(NULL)
    , model(NULL)
    , words(displayWords)
    , ids(internalIds)
    , quickExitEnabled(quickExit)
    {
        assert(displayWords.size() == internalIds.size());
    }
};



CompleterLineEdit::CompleterLineEdit(const QStringList& displayWords,const QStringList& internalIds,bool quickExit,QDialog* parent)
: LineEdit(parent)
, _imp(new CompleterLineEditPrivate(displayWords,internalIds,quickExit,parent))
{
    _imp->listView = new QListView(this);
    _imp->model = new QStringListModel(this);
    _imp->listView->setWindowFlags(Qt::ToolTip);
    
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(filterText(QString)));
    connect(_imp->listView, SIGNAL(clicked(QModelIndex)), this, SLOT(setTextFromIndex(QModelIndex)));
}

CompleterLineEdit::~CompleterLineEdit() {}

QListView* CompleterLineEdit::getView() const {return _imp->listView;}

void CompleterLineEdit::filterText(const QString& txt)
{
    
    QStringList sl;
    if (txt.isEmpty()) {
        sl = _imp->words;
    } else {
        for (int i = 0; i < _imp->ids.size(); ++i) {
            if (_imp->ids[i].contains(txt,Qt::CaseInsensitive)) {
                sl << _imp->words[i];
            }
        }
    }
    _imp->model->setStringList(sl);
    _imp->listView->setModel(_imp->model);
    
    int rowCount = _imp->model->rowCount();
    if (rowCount == 0) {
        return;
    }
    
    QPoint p = mapToGlobal(QPoint(0,height()));
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screen = desktop->screenGeometry();
    
    double maxHeight = (screen.height() - p.y()) * 0.8;
    QFontMetrics fm = _imp->listView->fontMetrics();
    maxHeight = std::min(maxHeight, (rowCount * fm.height() * 1.2 + fm.height()));
    
    // Position the text edit
    _imp->listView->resize(width(),maxHeight);
    
    _imp->listView->move(p);
    _imp->listView->show();
}

void CompleterLineEdit::setTextFromIndex(const QModelIndex& index)
{
    QString text = index.data().toString();
    setText(text);
    emit itemCompletionChosen();
    _imp->listView->hide();
    if (_imp->quickExitEnabled) {
        _imp->dialog->accept();
    }
}

void CompleterLineEdit::keyPressEvent(QKeyEvent* e) {
    
    int key = e->key();
    bool viewVisible = !_imp->listView->isHidden();
    
    int count = _imp->listView->model()->rowCount();
    QModelIndex currentIndex = _imp->listView->currentIndex();
    
    if (key == Qt::Key_Escape) {
        if (_imp->quickExitEnabled) {
            _imp->dialog->reject();
        } else {
            if (_imp->listView->isVisible()) {
                _imp->listView->hide();
            } else {
                _imp->dialog->reject();
            }
        }
        e->accept();
    } else if (key == Qt::Key_Down) {
        if (viewVisible) {
            int row = currentIndex.row() + 1;
            
            ///Handle circular selection
            if (row >= count) {
                row = 0;
            }
            
            QModelIndex index = _imp->listView->model()->index(row, 0);
            _imp->listView->setCurrentIndex(index);
        }
    } else if (key == Qt::Key_Up) {
        if (viewVisible) {
            int row = currentIndex.row() - 1;
            
            ///Handle circular selection
            if (row < 0) {
                row = count - 1;
            }
            
            QModelIndex index = _imp->listView->model()->index(row, 0);
            _imp->listView->setCurrentIndex(index);
        }
    } else if (key == Qt::Key_Enter || key == Qt::Key_Return) {
        if (_imp->model->rowCount() == 1) {
            setText(_imp->model->index(0).data().toString());
            emit itemCompletionChosen();
            if (_imp->quickExitEnabled) {
                _imp->dialog->accept();
            }
            e->accept();
                
        } else {
            const QItemSelection selection = _imp->listView->selectionModel()->selection();
            QModelIndexList indexes = selection.indexes();
            if (indexes.size() == 1) {
                setText(_imp->model->index(indexes[0].row()).data().toString());
                emit itemCompletionChosen();
                if (_imp->quickExitEnabled) {
                    _imp->dialog->accept();
                }
                e->accept();
            } else {
                QLineEdit::keyPressEvent(e);
            }
        }
   
        _imp->listView->hide();
        
        
    } else {
        QLineEdit::keyPressEvent(e);
    }
    
}

void CompleterLineEdit::showCompleter()
{
    filterText("");
}

struct NodeCreationDialogPrivate
{
    QVBoxLayout* layout;
    CompleterLineEdit* textEdit;
    

    
    std::vector<Natron::Plugin*> items;
    
    NodeCreationDialogPrivate()
    : layout(NULL)
    , textEdit(NULL)
    , items(appPTR->getPluginsList())
    {
        
    }
    
};

NodeCreationDialog::NodeCreationDialog(QWidget* parent)
: QDialog(parent)
, _imp(new NodeCreationDialogPrivate())
{
    setWindowTitle(tr("Node creation tool"));
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    setObjectName("nodeCreationDialog");
    setAttribute(Qt::WA_DeleteOnClose,false);
    _imp->layout = new QVBoxLayout(this);
    _imp->layout->setContentsMargins(0, 0, 0, 0);

    
    
    QStringList ids;
    QStringList names;
    for (unsigned int i = 0; i < _imp->items.size(); ++i) {
        ids.push_back(_imp->items[i]->getPluginID());
        names.push_back(_imp->items[i]->getPluginLabel());
    }
    ids.sort();
    names.sort();
    _imp->textEdit = new CompleterLineEdit(ids,names,true,this);

    QPoint global = QCursor::pos();
    QSize sizeH = sizeHint();
    global.rx() -= sizeH.width() / 2;
    global.ry() -= sizeH.height() / 2;
    move(global.x(), global.y());
    
    _imp->layout->addWidget(_imp->textEdit);
    _imp->textEdit->setFocus();
    QTimer::singleShot(25, _imp->textEdit, SLOT(showCompleter()));
}

NodeCreationDialog::~NodeCreationDialog()
{
}


QString NodeCreationDialog::getNodeName() const
{
    return _imp->textEdit->text();
}

void NodeCreationDialog::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void NodeCreationDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            reject();
            return;
        }
    }
    QDialog::changeEvent(e);
}

