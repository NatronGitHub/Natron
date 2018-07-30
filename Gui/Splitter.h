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
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/SplitterI.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @class A thread-safe wrapper over QSplitter
 **/
class Splitter
    : public QSplitter
    , public SplitterI
{
public:

    Splitter(Qt::Orientation orientation,
             Gui* gui, 
             QWidget * parent);

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

    void insertChild_mt_safe(int i, QWidget* w);

    void removeChild_mt_safe(QWidget* w);

    void getChildren_mt_safe(std::list<QWidget*> & children) const;

    virtual OrientationEnum getNatronOrientation() const OVERRIDE FINAL;

    virtual int getLeftChildrenSize() const OVERRIDE FINAL;

    virtual int getRightChildrenSize() const OVERRIDE FINAL;

    virtual SplitterI* isLeftChildSplitter() const OVERRIDE FINAL;

    virtual TabWidgetI* isLeftChildTabWidget() const OVERRIDE FINAL;

    virtual SplitterI* isRightChildSplitter() const OVERRIDE FINAL;

    virtual TabWidgetI* isRightChildTabWidget() const OVERRIDE FINAL;

    virtual void setNatronOrientation(OrientationEnum orientation) OVERRIDE FINAL;

    virtual void setChildrenSize(int left, int right) OVERRIDE FINAL;

private:

    virtual void restoreChildrenFromSerialization(const SERIALIZATION_NAMESPACE::WidgetSplitterSerialization& serialization) OVERRIDE FINAL;

    virtual bool event(QEvent* e) OVERRIDE FINAL;
    mutable QMutex _lock;
    Gui* _gui;
};

NATRON_NAMESPACE_EXIT

#endif // SPLITTER_H
