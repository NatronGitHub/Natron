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
#include <QRegularExpression>

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

/* Convert a wildcard pattern to an anchored QRegularExpression pattern, with
 * the semantics of the QRegExp::Wildcard syntax, which is gone in Qt6:
 * - '*' matches any sequence of characters, and '?' any single character,
 *   including '/';
 * - "[...]" is a character set and "[^...]" a negated set ('!' is literal);
 * - every other character is literal, including the backslash, which never
 *   escapes anything: "\\*" is a backslash followed by a wildcard (for a
 *   backslash followed by another character, Qt5's QRegExp wanted two
 *   backslashes in the string, which was a bug).
 * QRegularExpression::wildcardToRegularExpression() is not a drop-in
 * replacement: it is path-aware ('*' and '?' do not match '/'), and it reads
 * "[!...]" as a negated set, even with Qt 6.6's NonPathWildcardConversion.
 * As with QRegExp, an unterminated '[' gives an invalid expression. */
inline QString
wildcardToRegularExpression(const QString & wildcard)
{
    QString rx;
    QString literal; // run of literal characters, escaped when flushed
    const int n = int( wildcard.size() );
    int i = 0;

    while (i < n) {
        const QChar c = wildcard.at(i++);
        if ( ( c != QLatin1Char('*') ) && ( c != QLatin1Char('?') ) && ( c != QLatin1Char('[') ) ) {
            literal += c;
            continue;
        }
        rx += QRegularExpression::escape(literal);
        literal.clear();
        if ( c == QLatin1Char('*') ) {
            rx += QLatin1String(".*");
        } else if ( c == QLatin1Char('?') ) {
            rx += QLatin1Char('.');
        } else {
            // Character set: copy it, escaping the punctuation, which may be
            // special inside a PCRE set ('\\', '[', "[:alpha:]"...) but was
            // literal for QRegExp. '-' still denotes a range.
            rx += QLatin1Char('[');
            if ( ( i < n ) && ( wildcard.at(i) == QLatin1Char('^') ) ) {
                rx += QLatin1Char('^');
                ++i;
            }
            if ( ( i < n ) && ( wildcard.at(i) == QLatin1Char(']') ) ) {
                // a leading ']' is a member of the set
                rx += QLatin1String("\\]");
                ++i;
            }
            while ( ( i < n ) && ( wildcard.at(i) != QLatin1Char(']') ) ) {
                const QChar s = wildcard.at(i++);
                if ( !s.isLetterOrNumber() && !s.isSurrogate() && ( s != QLatin1Char('-') ) ) {
                    rx += QLatin1Char('\\');
                }
                rx += s;
            }
            if (i < n) {
                rx += QLatin1Char(']');
                ++i;
            }
        }
    }
    rx += QRegularExpression::escape(literal);

    // "(?s)": '.' matches newlines too, as '*' and '?' did with QRegExp
    return QRegularExpression::anchoredPattern(QLatin1String("(?s)") + rx);
}

// Define compatibility typedefs so code builds with Qt5 & Qt6
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
typedef ::QEnterEvent QEnterEvent;
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
typedef QEvent QEnterEvent;
#else
#error "Unsupported version of QT"
#endif

/*Holds a lock on either kind of mutex for the duration of a scope.

  Qt5 derived QRecursiveMutex from QMutex, so a single QMutexLocker could hold
  whichever one a code path happened to pick. Qt6 made the two types unrelated
  and turned QMutexLocker into a template on the mutex type, which leaves no
  one type able to hold both. Callers hold it in a std::unique_ptr that is only
  set on some branches, and a null mutex locks nothing, as with QMutexLocker.*/
class MutexLock
{
public:
    explicit MutexLock(QMutex* mutex)
        : _mutex(mutex)
        , _recursiveMutex(0)
    {
        // like QMutexLocker, a null mutex means "don't lock"
        if (_mutex) {
            _mutex->lock();
        }
    }

    explicit MutexLock(QRecursiveMutex* mutex)
        : _mutex(0)
        , _recursiveMutex(mutex)
    {
        if (_recursiveMutex) {
            _recursiveMutex->lock();
        }
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
