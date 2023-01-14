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

#include <QtCore/QtGlobal> // for Q_OS_*
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtCore/QDir>
#include <QtCore/QString>

NATRON_NAMESPACE_ENTER

namespace QtCompat {
inline bool
removeRecursively(const QString & dirName)
{
    bool result = false;
    QDir dir(dirName);

    if ( dir.exists(dirName) ) {
        Q_FOREACH( QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst) ) {
            if ( info.isDir() ) {
                result = removeRecursively( info.absoluteFilePath() );
            } else {
                result = QFile::remove( info.absoluteFilePath() );
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
}
} // namespace QtCompat

NATRON_NAMESPACE_EXIT

#endif

#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>

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

// in Qt 4.8 QUrl is broken on mac, it returns /.file/id= for local files
// See https://bugreports.qt.io/browse/QTBUG-40449
#if defined(Q_OS_DARWIN) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
//Implementation is in QUrlFix.mm
QUrl toLocalFileUrlFixed(const QUrl& url);
#else // #if defined(Q_OS_DARWIN) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
inline QUrl
toLocalFileUrlFixed(const QUrl& url) { return url; }

#endif // #if defined(Q_OS_DARWIN) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
} // namespace QtCompat

NATRON_NAMESPACE_EXIT

#endif // NATRON_GLOBAL_QTCOMPAT_H
