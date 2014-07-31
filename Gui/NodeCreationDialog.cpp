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


class CustomLineEdit : public LineEdit
{
    NodeCreationDialog* dialog;
public:
    
    CustomLineEdit(NodeCreationDialog* parent) : LineEdit(parent), dialog(parent) {}
    
    virtual void keyPressEvent(QKeyEvent* e) {
        if (e->key() == Qt::Key_Escape) {
            dialog->close();
            e->accept();
        } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            if (dialog->determineIfAcceptNeeded()) {
                e->accept();
            } else {
                LineEdit::keyPressEvent(e);
            }
        } else {
            LineEdit::keyPressEvent(e);
        }
    }
    
    virtual void mousePressEvent(QMouseEvent* e) {
        LineEdit::mousePressEvent(e);
    }
    
};

struct NodeCreationDialogPrivate
{
    QVBoxLayout* layout;
    LineEdit* textEdit;
    
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

    
    _imp->textEdit = new CustomLineEdit(this);
    
    QStringList strings;
    for (unsigned int i = 0; i < _imp->items.size(); ++i) {
        strings.push_back(_imp->items[i]->getPluginID());
    }
    strings.sort();
    QCompleter* completer = new QCompleter(strings,this);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    
    ListView* customView = new ListView(this);
    completer->setPopup(customView);
    ///QCompleter sets a custom QAbstractItemDelegate on it's model and unfortunately this
    ///custom item delegate does not inherit QStyledItemDelegate but simply QItemDelegate
    ///(and then overrides the paintmethod to show the selected state).
    
    ///EDIT : It doesnt seem to work here anyway...
    QStyledItemDelegate* itemDelegate = new QStyledItemDelegate(customView);
    customView->setItemDelegate(itemDelegate);

    _imp->textEdit->setCompleter(completer);
    _imp->layout->addWidget(_imp->textEdit);
    _imp->textEdit->setFocus(Qt::PopupFocusReason);
    QTimer::singleShot(25, _imp->textEdit->completer(), SLOT(complete()));
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
