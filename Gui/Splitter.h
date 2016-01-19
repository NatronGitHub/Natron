/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef SPLITTER_H
#define SPLITTER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QSplitter>
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @class A thread-safe wrapper over QSplitter
 **/
class Splitter
    : public QSplitter
{
public:

    Splitter(QWidget* parent = 0);

    Splitter(Qt::Orientation orientation,
             QWidget * parent = 0);

    virtual ~Splitter()
    {
    }

    void addWidget_mt_safe(QWidget * widget);

    QByteArray saveState_mt_safe() const;

    QString serializeNatron() const;

    void restoreNatron(const QString & serialization);

    void setSizes_mt_safe(const QList<int> & list);

    void setObjectName_mt_safe(const QString & str);

    QString objectName_mt_safe() const;

    void insertChild_mt_safe(int i,QWidget* w);

    void removeChild_mt_safe(QWidget* w);

    void getChildren_mt_safe(std::list<QWidget*> & children) const;

private:

    mutable QMutex _lock;
};

NATRON_NAMESPACE_EXIT;

#endif // SPLITTER_H
