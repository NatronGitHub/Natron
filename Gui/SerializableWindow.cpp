//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "SerializableWindow.h"
#include <QMutex>
SerializableWindow::SerializableWindow()
    : _lock(new QMutex)
      , _w(0), _h(0), _x(0), _y(0)
{
}

SerializableWindow::~SerializableWindow()
{
    delete _lock;
}

void
SerializableWindow::setMtSafeWindowSize(int w,
                                        int h)
{
    QMutexLocker k(_lock);

    _w = w;
    _h = h;
}

void
SerializableWindow::setMtSafePosition(int x,
                                      int y)
{
    QMutexLocker k(_lock);

    _x = x;
    _y = y;
}

void
SerializableWindow::getMtSafeWindowSize(int &w,
                                        int & h)
{
    QMutexLocker k(_lock);

    w = _w;
    h = _h;
}

void
SerializableWindow::getMtSafePosition(int &x,
                                      int &y)
{
    QMutexLocker k(_lock);

    x = _x;
    y = _y;
}

