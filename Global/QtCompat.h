/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#ifndef NATRON_GLOBAL_QTCOMPAT_H
#define NATRON_GLOBAL_QTCOMPAT_H

#include "Global/Macros.h"

#include <QtGlobal> // for Q_OS_*
#include <QMutex>
#include <QRecursiveMutex>
#include <QString>
#include <QUrl>
#include <QFileInfo>

// Qt6 declares QWidget::enterEvent(QEnterEvent*) where Qt5 used QEvent*. Only
// Gui ever needs the definition and Engine does not link QtGui, so forward
// declare it the way EngineFwd.h handles the rest of Qt.
class QEnterEvent;

NATRON_NAMESPACE_ENTER

namespace QtCompat {
/*Removes the . and the extension from the filename and also
 * returns the extension as a string.*/
inline QString
removeFileExtension(QString & filename)
{
    //qDebug() << "remove file ext from" << filename;
    QFileInfo fi(filename);
    QString extension = fi.suffix();

    if ( !extension.isEmpty() ) {
        filename.truncate(filename.size() - extension.size() - 1);
    }

    //qDebug() << "->" << filename << fi.suffix();
    return extension;
}

typedef ::QEnterEvent QEnterEvent;

/*Holds a lock on either kind of mutex for the duration of a scope.

  QRecursiveMutex does not derive from QMutex and QMutexLocker is a template on
  the mutex type, so no one type can hold both. Deferred locking is the point
  here: these lockers are declared unlocked and only engaged on some branches.*/
class MutexLock
{
public:
    explicit MutexLock(QMutex* mutex)
        : _mutex(mutex)
        , _recursiveMutex(0)
    {
        _mutex->lock();
    }

    explicit MutexLock(QRecursiveMutex* mutex)
        : _mutex(0)
        , _recursiveMutex(mutex)
    {
        _recursiveMutex->lock();
    }

    ~MutexLock()
    {
        if (_mutex) {
            _mutex->unlock();
        }
        if (_recursiveMutex) {
            _recursiveMutex->unlock();
        }
    }

private:
    Q_DISABLE_COPY(MutexLock)

    QMutex* _mutex;
    QRecursiveMutex* _recursiveMutex;
};

} // namespace QtCompat

NATRON_NAMESPACE_EXIT

#endif // NATRON_GLOBAL_QTCOMPAT_H
