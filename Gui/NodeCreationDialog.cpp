//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "NodeCreationDialog.h"

#include <QCompleter>
#include <QVBoxLayout>
#include <QStringList>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QTimer>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QListView>
#include <QStringListModel>

#include "Engine/Plugin.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiApplicationManager.h"

class ListView : public QListView
{
    /*NodeCreationDialog* dialog;*/
public:
    
    ListView(NodeCreationDialog* parent) : QListView(parent) /*, dialog(parent)*/ {}
    
private:
    
};


struct CompleterLineEditPrivate {
    NodeCreationDialog* dialog;
    QListView* listView;
    QStringListModel* model;
    QStringList words;
    
    CompleterLineEditPrivate(const QStringList& words,NodeCreationDialog* parent)
    : dialog(parent)
    , listView(NULL)
    , model(NULL)
    , words(words)
    {
        
    }
};



CompleterLineEdit::CompleterLineEdit(const QStringList& words,NodeCreationDialog* parent)
: LineEdit(parent)
, _imp(new CompleterLineEditPrivate(words,parent))
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
        for (int i = 0; i < _imp->words.size(); ++i) {
            if (_imp->words[i].contains(txt,Qt::CaseInsensitive)) {
                sl << _imp->words[i];
            }
        }
    }
    _imp->model->setStringList(sl);
    _imp->listView->setModel(_imp->model);
    
    if (_imp->model->rowCount() == 0) {
        return;
    }
    
    // Position the text edit
    _imp->listView->setFixedWidth(width());
    
    _imp->listView->move(mapToGlobal(QPoint(0,height())));
    _imp->listView->show();
}

void CompleterLineEdit::setTextFromIndex(const QModelIndex& index)
{
    QString text = index.data().toString();
    setText(text);
    _imp->listView->hide();
}

void CompleterLineEdit::keyPressEvent(QKeyEvent* e) {
    
    int key = e->key();
    bool viewVisible = !_imp->listView->isHidden();
    
    int count = _imp->listView->model()->rowCount();
    QModelIndex currentIndex = _imp->listView->currentIndex();
    
    if (key == Qt::Key_Escape) {
        _imp->dialog->close();
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
            _imp->dialog->accept();
            e->accept();
        } else {
            const QItemSelection selection = _imp->listView->selectionModel()->selection();
            QModelIndexList indexes = selection.indexes();
            if (indexes.size() == 1) {
                setText(_imp->model->index(indexes[0].row()).data().toString());
                _imp->dialog->accept();
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
    _imp->layout = new QVBoxLayout(this);
    _imp->layout->setContentsMargins(0, 0, 0, 0);

    
    
    QStringList strings;
    for (unsigned int i = 0; i < _imp->items.size(); ++i) {
        strings.push_back(_imp->items[i]->getPluginID());
    }
    strings.sort();
    
    _imp->textEdit = new CompleterLineEdit(strings,this);

    QPoint global = QCursor::pos();
    QSize sizeH = sizeHint();
    global.rx() -= sizeH.width() / 2;
    global.ry() -= sizeH.height() / 2;
    move(global.x(), global.y());
    
    _imp->layout->addWidget(_imp->textEdit);
    _imp->textEdit->setFocus(Qt::PopupFocusReason);
    QTimer::singleShot(25, _imp->textEdit, SLOT(showCompleter()));
}

NodeCreationDialog::~NodeCreationDialog()
{
    
}

bool NodeCreationDialog::determineIfAcceptNeeded()
{
    QCompleter* completer =  _imp->textEdit->completer();
    int count = completer->completionCount();
    const QItemSelection selection = completer->popup()->selectionModel()->selection();
    QModelIndexList indexes = selection.indexes();
    
    if (count == 1) {
        completer->setCurrentRow(0);
        _imp->textEdit->blockSignals(true);
        _imp->textEdit->setText(completer->currentCompletion());
        _imp->textEdit->blockSignals(false);
        accept();
        return true;
    } else if (indexes.size() == 1) {
        completer->setCurrentRow(indexes[0].row());
        _imp->textEdit->blockSignals(true);
        _imp->textEdit->setText(completer->currentCompletion());
        _imp->textEdit->blockSignals(false);
        accept();
        return true;
        
    } else {
        return false;
    }

}

QString NodeCreationDialog::getNodeName() const
{
    return _imp->textEdit->text();
}

void NodeCreationDialog::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}
