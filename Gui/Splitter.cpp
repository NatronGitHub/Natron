/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Splitter.h"

#include "Serialization/WorkspaceSerialization.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/TabWidget.h"
#include "Gui/Gui.h"
#include <cassert>
#include <stdexcept>

NATRON_NAMESPACE_ENTER


Splitter::Splitter(Qt::Orientation orientation,
                   Gui* gui,
                   QWidget * parent)
    : QSplitter(orientation, parent)
    , _lock()
    , _gui(gui)
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
        return QString::fromUtf8("%1 %2").arg(list[0]).arg(list[1]);
    }

    return QString();
}

void
Splitter::restoreNatron(const QString & serialization)
{
    QMutexLocker l(&_lock);
    QStringList list = serialization.split( QLatin1Char(' ') );

    assert(list.size() == 2);
    QList<int> s;
    s << list[0].toInt() << list[1].toInt();
    if ( (s[0] == 0) || (s[1] == 0) ) {
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

bool
Splitter::event(QEvent* e)
{
    return QSplitter::event(e);
}


OrientationEnum
Splitter::getNatronOrientation() const
{
    Qt::Orientation qor = orientation();
    switch (qor) {
        case Qt::Horizontal:
            return eOrientationHorizontal;
            break;
        case Qt::Vertical:
            return eOrientationVertical;
    }
    return eOrientationVertical;
}


int
Splitter::getLeftChildrenSize() const
{
    QList<int> list = sizes();
    assert(list.size() == 2);
    return list.front();
}

int
Splitter::getRightChildrenSize() const
{
    QList<int> list = sizes();
    assert(list.size() == 2);
    return list.back();
}

SplitterI*
Splitter::isLeftChildSplitter() const
{
    return dynamic_cast<Splitter*>(widget(0));
}

TabWidgetI*
Splitter::isLeftChildTabWidget() const
{
    return dynamic_cast<TabWidget*>(widget(0));
}

SplitterI*
Splitter::isRightChildSplitter() const
{
    return dynamic_cast<Splitter*>(widget(1));
}

TabWidgetI*
Splitter::isRightChildTabWidget() const
{
    return dynamic_cast<TabWidget*>(widget(1));
}

void
Splitter::setNatronOrientation(OrientationEnum orientation)
{
    switch (orientation) {
        case eOrientationHorizontal:
            setOrientation(Qt::Horizontal);
            break;
        case eOrientationVertical:
            setOrientation(Qt::Vertical);
            break;
    }
}

void
Splitter::setChildrenSize(int left, int right)
{
    QList<int> sizes;
    sizes.push_back(left);
    sizes.push_back(right);
    setSizes_mt_safe(sizes);
}

void
Splitter::restoreChildrenFromSerialization(const SERIALIZATION_NAMESPACE::WidgetSplitterSerialization& serialization)
{
    {
        if (serialization.leftChild.type == kSplitterChildTypeSplitter) {
            Qt::Orientation orientation = Qt::Horizontal;
            if (serialization.leftChild.childIsSplitter->orientation == kSplitterOrientationHorizontal) {
                orientation = Qt::Horizontal;
            } else if (serialization.leftChild.childIsSplitter->orientation == kSplitterOrientationVertical) {
                orientation = Qt::Vertical;
            }

            Splitter* splitter = new Splitter(orientation, _gui, this);
            splitter->fromSerialization(*serialization.leftChild.childIsSplitter);
            _gui->getApp()->registerSplitter(splitter);
            addWidget_mt_safe(splitter);
        } else if (serialization.leftChild.type == kSplitterChildTypeTabWidget) {
            TabWidget* tab = new TabWidget(_gui, this);
            tab->fromSerialization(*serialization.leftChild.childIsTabWidget);
            _gui->getApp()->registerTabWidget(tab);
            addWidget_mt_safe(tab);
        }
    }
    {
        if (serialization.rightChild.type == kSplitterChildTypeSplitter) {
            Qt::Orientation orientation = Qt::Horizontal;
            if (serialization.rightChild.childIsSplitter->orientation == kSplitterOrientationHorizontal) {
                orientation = Qt::Horizontal;
            } else if (serialization.rightChild.childIsSplitter->orientation == kSplitterOrientationVertical) {
                orientation = Qt::Vertical;
            }
            Splitter* splitter = new Splitter(orientation, _gui, this);
            splitter->fromSerialization(*serialization.rightChild.childIsSplitter);
            _gui->getApp()->registerSplitter(splitter);
            addWidget_mt_safe(splitter);
        } else if (serialization.rightChild.type == kSplitterChildTypeTabWidget) {
            TabWidget* tab = new TabWidget(_gui, this);
            tab->fromSerialization(*serialization.rightChild.childIsTabWidget);
            _gui->getApp()->registerTabWidget(tab);
            addWidget_mt_safe(tab);
        }
    }
}


NATRON_NAMESPACE_EXIT
