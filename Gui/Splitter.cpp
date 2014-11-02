//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#include "Splitter.h"
#include <cassert>
Splitter::Splitter(QWidget* parent)
    : QSplitter(parent)
      , _lock()
{
}

Splitter::Splitter(Qt::Orientation orientation,
                   QWidget * parent)
    : QSplitter(orientation,parent)
      , _lock()
{
}

void
Splitter::addWidget_mt_safe(QWidget * widget)
{
    QMutexLocker l(&_lock);

    widget->setParent(this);
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
    w->setParent(this);
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

