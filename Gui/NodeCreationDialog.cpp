/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeCreationDialog.h"

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QtCore/QStringList>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QtCore/QTimer>
#include <QApplication>
#include <QListView>
#include <QSettings>
#include <QDesktopWidget>
#include <QtCore/QRegExp>
#include <QApplication>
#include <QStringListModel>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AppManager.h"
#include "Engine/Plugin.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiApplicationManager.h"

/*
   If defined, the list will contain matches ordered by increasing match length of the regexp
   otherwise they will be ordered with their score
 */
//#define NODE_TAB_DIALOG_USE_MATCHED_LENGTH

NATRON_NAMESPACE_ENTER


struct CompleterLineEditPrivate
{
    QDialog* dialog;
    QListView* listView;
    QStringListModel* model;
    CompleterLineEdit::PluginsNamesMap names;
    bool quickExitEnabled;

    CompleterLineEditPrivate(const CompleterLineEdit::PluginsNamesMap& plugins,
                             bool quickExit,
                             QDialog* parent)
        : dialog(parent)
        , listView(NULL)
        , model(NULL)
        , names(plugins)
        , quickExitEnabled(quickExit)
    {
    }

    CompleterLineEditPrivate(const QStringList & displayWords,
                             const QStringList & internalIds,
                             bool quickExit,
                             QDialog* parent)
        : dialog(parent)
        , listView(NULL)
        , model(NULL)
        , quickExitEnabled(quickExit)
    {
        assert( displayWords.size() == internalIds.size() );

        for (int i = 0; i < displayWords.size(); ++i) {
            names.insert( std::make_pair( 0, std::make_pair(internalIds[i], displayWords[i]) ) );
        }
    }
};

CompleterLineEdit::CompleterLineEdit(const PluginsNamesMap& plugins,
                                     bool quickExit,
                                     QDialog* parent)
    : LineEdit(parent)
    , _imp( new CompleterLineEditPrivate(plugins, quickExit, parent) )
{
    _imp->listView = new QListView(parent);
    _imp->listView->setObjectName("listView");
    _imp->listView->setAlternatingRowColors(true);
    _imp->model = new QStringListModel(this);
    _imp->listView->setWindowFlags(Qt::ToolTip);
    _imp->listView->setModel(_imp->model);

    connect( this, SIGNAL(textChanged(QString)), this, SLOT(filterText(QString)) );
    connect( _imp->listView, SIGNAL(clicked(QModelIndex)), this, SLOT(setTextFromIndex(QModelIndex)) );
}

CompleterLineEdit::CompleterLineEdit(const QStringList & displayWords,
                                     const QStringList & internalIds,
                                     bool quickExit,
                                     QDialog* parent)
    : LineEdit(parent)
    , _imp( new CompleterLineEditPrivate(displayWords, internalIds, quickExit, parent) )
{
    _imp->listView = new QListView(parent);
    _imp->listView->setObjectName("listView");
    _imp->listView->setAlternatingRowColors(true);  
    _imp->model = new QStringListModel(this);
    _imp->listView->setWindowFlags(Qt::ToolTip);
    _imp->listView->setModel(_imp->model);

    connect( this, SIGNAL(textChanged(QString)), this, SLOT(filterText(QString)) );
    connect( _imp->listView, SIGNAL(clicked(QModelIndex)), this, SLOT(setTextFromIndex(QModelIndex)) );
}

CompleterLineEdit::~CompleterLineEdit()
{
}

QListView*
CompleterLineEdit::getView() const
{
    return _imp->listView;
}

void
CompleterLineEdit::filterText(const QString & txt)
{
    QStringList sl;

    if ( txt.isEmpty() ) {
        for (PluginsNamesMap::iterator it = _imp->names.begin(); it != _imp->names.end(); ++it) {
            sl.push_front(it->second.second);
        }
    } else {
        QString pattern;
        for (int i = 0; i < txt.size(); ++i) {
            pattern.push_back( QLatin1Char('*') );
            pattern.push_back(txt[i]);
        }
        pattern.push_back( QLatin1Char('*') );
        QRegExp expr(pattern, Qt::CaseInsensitive, QRegExp::WildcardUnix);

#ifdef NODE_TAB_DIALOG_USE_MATCHED_LENGTH
        std::map<int, QStringList> matchOrdered;
        for (PluginsNamesMap::iterator it = _imp->names.begin(); it != _imp->names.end(); ++it) {
            if ( expr.exactMatch(it->second.first) ) {
                QStringList& matchedForLength =  matchOrdered[expr.matchedLength()];
                matchedForLength.push_front(it->second.second);
            }
        }
        for (std::map<int, QStringList>::iterator it = matchOrdered.begin(); it != matchOrdered.end(); ++it) {
            sl.append(it->second);
        }
#else

        for (PluginsNamesMap::iterator it = _imp->names.begin(); it != _imp->names.end(); ++it) {
            if ( it->second.first.contains(expr) ) {
                sl.push_front(it->second.second);
            }
        }
#endif
    }
    _imp->model->setStringList(sl);
    _imp->listView->setModel(_imp->model);

    int rowCount = _imp->model->rowCount();
    if (rowCount == 0) {
        return;
    }

    QPoint p = mapToGlobal( QPoint( 0, height() ) );
    //QDesktopWidget* desktop = QApplication::desktop();
    //QRect screen = desktop->screenGeometry();
    //double maxHeight = ( screen.height() - p.y() ) * 0.8;
    //QFontMetrics fm = _imp->listView->fontMetrics();
    //maxHeight = std::min( maxHeight, ( rowCount * fm.height() * 1.2 + fm.height() ) );

    // Position the text edit
    //   _imp->listView->setFixedSize(width(),maxHeight);

    _imp->listView->move(p);
    _imp->listView->show();
} // CompleterLineEdit::filterText

void
CompleterLineEdit::setTextFromIndex(const QModelIndex & index)
{
    QString text = index.data().toString();

    setText(text);
    Q_EMIT itemCompletionChosen();
    _imp->listView->hide();
    if (_imp->quickExitEnabled) {
        NodeCreationDialog* isNodeDialog = dynamic_cast<NodeCreationDialog*>(_imp->dialog);
        if (isNodeDialog) {
            isNodeDialog->finish(true);
        } else {
            if (_imp->dialog) {
                _imp->dialog->accept();
            }
        }
    }
}

void
CompleterLineEdit::keyPressEvent(QKeyEvent* e)
{
    int key = e->key();
    bool viewVisible = !_imp->listView->isHidden();

    assert( _imp->listView->model() );
    int count = _imp->listView->model()->rowCount();
    QModelIndex currentIndex = _imp->listView->currentIndex();

    if (key == Qt::Key_Escape) {
        if (_imp->quickExitEnabled) {
            NodeCreationDialog* isNodeDialog = dynamic_cast<NodeCreationDialog*>(_imp->dialog);
            if (isNodeDialog) {
                isNodeDialog->finish(false);
            } else {
                if (_imp->dialog) {
                    _imp->dialog->accept();
                }
            }
        } else {
            if ( _imp->listView->isVisible() ) {
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
    } else if ( (key == Qt::Key_Enter) || (key == Qt::Key_Return) ) {
        _imp->listView->hide();
        if (_imp->model->rowCount() == 1) {
            setText( _imp->model->index(0).data().toString() );
            Q_EMIT itemCompletionChosen();
            if (_imp->quickExitEnabled) {
                NodeCreationDialog* isNodeDialog = dynamic_cast<NodeCreationDialog*>(_imp->dialog);
                if (isNodeDialog) {
                    isNodeDialog->finish(true);
                } else {
                    if (_imp->dialog) {
                        _imp->dialog->accept();
                    }
                }
            }
            e->accept();
        } else {
            const QItemSelection selection = _imp->listView->selectionModel()->selection();
            QModelIndexList indexes = selection.indexes();
            bool doDialogEnd = false;
            if (indexes.size() == 1) {
                setText( _imp->model->index( indexes[0].row() ).data().toString() );
                doDialogEnd = true;
            } else if ( indexes.isEmpty() ) {
                if (_imp->model->rowCount() > 1) {
                    doDialogEnd = true;
                    setText( _imp->model->index(0).data().toString() );
                }
            }
            if (doDialogEnd) {
                Q_EMIT itemCompletionChosen();
                if (_imp->quickExitEnabled) {
                    NodeCreationDialog* isNodeDialog = dynamic_cast<NodeCreationDialog*>(_imp->dialog);
                    if (isNodeDialog) {
                        isNodeDialog->finish(true);
                    } else {
                        if (_imp->dialog) {
                            _imp->dialog->accept();
                        }
                    }
                }
                e->accept();
            } else {
                QLineEdit::keyPressEvent(e);
            }
        }
    } else {
        QLineEdit::keyPressEvent(e);
    }
} // keyPressEvent

void
CompleterLineEdit::showCompleter()
{
    _imp->listView->setFixedWidth( width() );
    filterText( text() );
}

struct NodeCreationDialogPrivate
{
    QVBoxLayout* layout;
    CompleterLineEdit* textEdit;
    PluginsMap items;
    bool isFinished;

    NodeCreationDialogPrivate()
        : layout(NULL)
        , textEdit(NULL)
        , items( appPTR->getPluginsList() )
        , isFinished(false)
    {
    }
};

static int
getPluginWeight(const QString& pluginID,
                int majVersion)
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    QString propName( pluginID + QString::number(majVersion) + QString::fromUtf8("_tab_weight") );
    int curValue = settings.value( propName, QVariant(0) ).toInt();

    return curValue;
}

static void
incrementPluginWeight(const QString& pluginID,
                      int majVersion)
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    QString propName( pluginID + QString::number(majVersion) + QString::fromUtf8("_tab_weight") );
    int curValue = settings.value( propName, QVariant(0) ).toInt();

    ++curValue;
    settings.setValue( propName, QVariant(curValue) );
}

NodeCreationDialog::NodeCreationDialog(const QString& initialFilter,
                                       QWidget* parent)
    : QDialog(parent)
    , _imp( new NodeCreationDialogPrivate() )
{
    setWindowTitle( tr("Node Creation Tool") );
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setObjectName( QString::fromUtf8("nodeCreationDialog") );
    setAttribute(Qt::WA_DeleteOnClose, true);
    _imp->layout = new QVBoxLayout(this);
    _imp->layout->setContentsMargins(0, 0, 0, 0);


    CompleterLineEdit::PluginsNamesMap pluginsMap;
    QString initialFilterName;
    std::string stdInitialFilter = initialFilter.toStdString();
    int i = 0;
    for (PluginsMap::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if ( it->second.empty() ) {
            continue;
        }


        if (it->second.size() == 1) {
            std::pair<QString, QString> idNamePair;
            Plugin* p = ( *it->second.begin() );
            if ( !p->getIsUserCreatable() ) {
                continue;
            }

            idNamePair.second = p->generateUserFriendlyPluginID();

            int indexOfBracket = idNamePair.second.lastIndexOf( QString::fromUtf8("  [") );
            if (indexOfBracket != -1) {
                idNamePair.first = idNamePair.second.left(indexOfBracket);
            }

            int weight = getPluginWeight( p->getPluginID(), p->getMajorVersion() );
            pluginsMap.insert( std::make_pair(weight, idNamePair) );

            if (it->first == stdInitialFilter) {
                initialFilterName = idNamePair.first;
            }
            ++i;
        } else {
            QString bestMajorName;
            for (PluginVersionsOrdered::reverse_iterator it2 = it->second.rbegin(); it2 != it->second.rend(); ++it2) {
                if ( !(*it2)->getIsUserCreatable() ) {
                    continue;
                }
                std::pair<QString, QString> idNamePair;
                if ( it2 == it->second.rbegin() ) {
                    idNamePair.second = (*it2)->generateUserFriendlyPluginID();
                    bestMajorName = idNamePair.second;
                } else {
                    idNamePair.second = (*it2)->generateUserFriendlyPluginIDMajorEncoded();
                }


                int indexOfBracket = idNamePair.second.lastIndexOf( QString::fromUtf8("  [") );
                if (indexOfBracket != -1) {
                    idNamePair.first = idNamePair.second.left(indexOfBracket);
                }

                ++i;

                int weight = getPluginWeight( (*it2)->getPluginID(), (*it2)->getMajorVersion() );
                pluginsMap.insert( std::make_pair(weight, idNamePair) );
            }
            if (it->first == stdInitialFilter) {
                initialFilterName = bestMajorName;
            }
        }
    }

    _imp->textEdit = new CompleterLineEdit(pluginsMap, true, this);
    if ( !initialFilterName.isEmpty() ) {
        _imp->textEdit->setText(initialFilterName);
    }

    QPoint global = QCursor::pos();
    QSize sizeH = sizeHint();
    global.rx() -= sizeH.width() / 2;
    global.ry() -= sizeH.height() / 2;
    move( global.x(), global.y() );

    _imp->layout->addWidget(_imp->textEdit);
    _imp->textEdit->setFocus();
    _imp->textEdit->selectAll();
    QTimer::singleShot( 20, _imp->textEdit, SLOT(showCompleter()) );
}

NodeCreationDialog::~NodeCreationDialog()
{
}

QString
NodeCreationDialog::getNodeName(int *major) const
{
    QString name = _imp->textEdit->text();


    for (PluginsMap::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->second.size() == 1) {
            if ( ( *it->second.begin() )->generateUserFriendlyPluginID() == name ) {
                Plugin* p = ( *it->second.begin() );
                *major = p->getMajorVersion();
                const QString& ret = p->getPluginID();
                incrementPluginWeight(ret, *major);

                return ret;
            }
        } else {
            for (PluginVersionsOrdered::reverse_iterator it2 = it->second.rbegin(); it2 != it->second.rend(); ++it2) {
                if ( it2 == it->second.rbegin() ) {
                    if ( (*it2)->generateUserFriendlyPluginID() == name ) {
                        *major = (*it2)->getMajorVersion();
                        const QString& ret = (*it2)->getPluginID();
                        incrementPluginWeight(ret, *major);

                        return ret;
                    }
                } else {
                    if ( (*it2)->generateUserFriendlyPluginIDMajorEncoded() == name ) {
                        *major = (*it2)->getMajorVersion();
                        const QString& ret = (*it2)->getPluginID();
                        incrementPluginWeight(ret, *major);

                        return ret;
                    }
                }
            }
        }
    }

    return QString();
}

void
NodeCreationDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        finish(true);
    } else if (e->key() == Qt::Key_Escape) {
        finish(false);
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
NodeCreationDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange) {
        if ( !isActiveWindow() && ( qApp->activeWindow() != _imp->textEdit->getView() ) ) {
            finish(false);

            return;
        }
    }
    QDialog::changeEvent(e);
}

void
NodeCreationDialog::finish(bool accepted)
{
    if (_imp->isFinished) {
        return;
    }
    _imp->isFinished = true;
    Q_EMIT dialogFinished(accepted);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_NodeCreationDialog.cpp"
