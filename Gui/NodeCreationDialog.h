//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef NODECREATIONDIALOG_H
#define NODECREATIONDIALOG_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Gui/LineEdit.h"

class QStringList;
class QModelIndex;
class QListView;
struct CompleterLineEditPrivate;
class CompleterLineEdit
    : public LineEdit
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    
    /// map<weight, pair<ID,Label> >
    typedef std::multimap<int,std::pair<QString,QString> > PluginsNamesMap;

    CompleterLineEdit(const PluginsNamesMap& plugins,
                      bool quickExit,
                      QDialog* parent);
    
    CompleterLineEdit(const QStringList & displayWords,
                      const QStringList & internalIds,
                      bool quickExit,
                      QDialog* parent);


    virtual ~CompleterLineEdit();

    QListView* getView() const;

public Q_SLOTS:

    void filterText(const QString & txt);
    void setTextFromIndex(const QModelIndex & index);
    void showCompleter();

Q_SIGNALS:

    void itemCompletionChosen();

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<CompleterLineEditPrivate> _imp;
};

struct NodeCreationDialogPrivate;
class NodeCreationDialog
    : public QDialog
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:

    explicit NodeCreationDialog(const QString& initialFilter,QWidget* parent);

    virtual ~NodeCreationDialog() OVERRIDE;

    QString getNodeName(int *major) const;

private:

    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<NodeCreationDialogPrivate> _imp;
};

#endif // NODECREATIONDIALOG_H
