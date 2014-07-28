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

#include "Engine/Plugin.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiApplicationManager.h"

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

    
    _imp->textEdit = new LineEdit(this);
    
    QStringList strings;
    for (unsigned int i = 0; i < _imp->items.size(); ++i) {
        strings.push_back(_imp->items[i]->getPluginID());
    }
    strings.sort();
    QCompleter* completer = new QCompleter(strings,this);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    
    ///QCompleter sets a custom QAbstractItemDelegate on it's model and unfortunately this
    ///custom item delegate does not inherit QStyledItemDelegate but simply QItemDelegate
    ///(and then overrides the paintmethod to show the selected state).
    
    ///EDIT : It doesnt seem to work here anyway...
    QStyledItemDelegate* itemDelegate = new QStyledItemDelegate();
    completer->popup()->setItemDelegate(itemDelegate);
    
    _imp->textEdit->setCompleter(completer);
    _imp->layout->addWidget(_imp->textEdit);
    _imp->textEdit->setFocus(Qt::TabFocusReason);
    QTimer::singleShot(25, _imp->textEdit->completer(), SLOT(complete()));
}

NodeCreationDialog::~NodeCreationDialog()
{
    
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
