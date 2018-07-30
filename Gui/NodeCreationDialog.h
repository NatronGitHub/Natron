/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NODECREATIONDIALOG_H
#define NODECREATIONDIALOG_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/LineEdit.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct CompleterLineEditPrivate;

class CompleterLineEdit
    : public LineEdit
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    struct PluginDesc
    {
        PluginWPtr plugin;
        QString comboLabel;
        QString lineEditLabel;
        QString presetName;
    };
    // Map each plug-in by a weight
    typedef std::multimap<int, PluginDesc> PluginsNamesMap;

    CompleterLineEdit(const PluginsNamesMap& plugins,
                      bool quickExit,
                      QDialog* parent);

    CompleterLineEdit(const QStringList & displayWords,
                      const QStringList & internalIds,
                      bool quickExit,
                      QDialog* parent);


    virtual ~CompleterLineEdit();

    const PluginsNamesMap& getPluginNamesMap() const;

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
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit NodeCreationDialog(const QString& defaultPluginID,
                                QWidget* parent);

    virtual ~NodeCreationDialog() OVERRIDE;

    PluginPtr getPlugin(QString* presetName) const;

    void finish(bool accepted);

Q_SIGNALS:

    // Do not use the base class accept and reject
    // because here we do not use a modal dialog.
    // If we do then some complicated recursion due to the
    // completelerlineedit can make Qt crash.
    void dialogFinished(bool accepted);

private:

    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<NodeCreationDialogPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NODECREATIONDIALOG_H
