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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Splitter.h"

#include <cassert>
#include <stdexcept>

NATRON_NAMESPACE_ENTER;

Splitter::Splitter(QWidget* parent)
    : QSplitter(parent)
      , _lock()
{
    setMouseTracking(true);
}

Splitter::Splitter(Qt::Orientation orientation,
                   QWidget * parent)
    : QSplitter(orientation,parent)
      , _lock()
{
    setMouseTracking(true);
}

void
Splitter::addWidget_mt_safe(QWidget * widget)
{
    QMutexLocker l(&_lock);

    //widget->setParent(this);
    addWidget(widget);
}

QString
Splitter::serializeNatron() const
{
    QMutexLocker l(&_lock);

    QList<int> list = sizes();
    if (list.size() == 2) {
        return QString("%1 %2").arg(list[0]).arg(list[1]);
    }

    return "";
}

void
Splitter::restoreNatron(const QString & serialization)
{
    QMutexLocker l(&_lock);
    QStringList list = serialization.split( QChar(' ') );

    assert(list.size() == 2);
    QList<int> s;
    s << list[0].toInt() << list[1].toInt();
    if (s[0] == 0 || s[1] == 0) {
        int mean = (s[0] + s[1]) / 2;
        s[0] = s[1] = mean;
    }
    setSizes(s);
}

QByteArray
Splitter::saveState_mt_safe() const
{
    QMutexLocker l(&_lock);

    return saveState();
}

void
Splitter::setSizes_mt_safe(const QList<int> & list)
{
    QMutexLocker l(&_lock);

    setSizes(list);
}

void
Splitter::setObjectName_mt_safe(const QString & str)
{
    QMutexLocker l(&_lock);

    setObjectName(str);
}

QString
Splitter::objectName_mt_safe() const
{
    QMutexLocker l(&_lock);

    return objectName();
}

void
Splitter::insertChild_mt_safe(int i,
                              QWidget* w)
{
    QMutexLocker l(&_lock);

    insertWidget(i, w);
    //w->setParent(this);
}

void
Splitter::removeChild_mt_safe(QWidget* w)
{
    QMutexLocker l(&_lock);

    w->setParent(NULL);
}

void
Splitter::getChildren_mt_safe(std::list<QWidget*> & children) const
{
    QMutexLocker l(&_lock);

    for (int i = 0; i < count(); ++i) {
        children.push_back( widget(i) );
    }
}

NATRON_NAMESPACE_EXIT;
