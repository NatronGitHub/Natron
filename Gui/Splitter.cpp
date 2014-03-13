//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/


#include "Splitter.h"

Splitter::Splitter(QWidget* parent)
: QSplitter(parent)
, _lock(QMutex::Recursive)
{
    
}

Splitter::Splitter(Qt::Orientation orientation, QWidget * parent)
: QSplitter(orientation,parent)
, _lock(QMutex::Recursive)
{
    
}

void Splitter::addWidget_mt_safe(QWidget * widget) {
    QMutexLocker l(&_lock);
    addWidget(widget);
}

QByteArray Splitter::saveState_mt_safe() const {
    QMutexLocker l(&_lock);
    return saveState();
}

void Splitter::setSizes_mt_safe(const QList<int> & list) {
    QMutexLocker l(&_lock);
    setSizes(list);
}

bool Splitter::event(QEvent * e) {
    QMutexLocker l(&_lock);
    return QSplitter::event(e);
}

void Splitter::setObjectName_mt_safe(const QString& str) {
    QMutexLocker l(&_lock);
    setObjectName(str);
}

QString Splitter::objectName_mt_safe() const {
    QMutexLocker l(&_lock);
    return objectName();
}