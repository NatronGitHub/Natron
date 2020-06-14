/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#ifndef Engine_ExistenceCheckThread_h
#define Engine_ExistenceCheckThread_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <QtCore/QThread>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct MappedProcessWatcherThreadPrivate;
class MappedProcessWatcherThread
: public QThread
{


public:

    MappedProcessWatcherThread();

    virtual ~MappedProcessWatcherThread();

    void quitThread();

    void startWatching();

private:

    virtual void run() OVERRIDE FINAL;
    boost::scoped_ptr<MappedProcessWatcherThreadPrivate> _imp;
};

struct ExistenceCheckerThreadPrivate;
class ExistenceCheckerThread
    : public QThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    ExistenceCheckerThread(const QString& checkMessage,
                           const QString& acknowledgementMessage,
                           const QString& comServerPipePath);

    virtual ~ExistenceCheckerThread();

    void quitThread();

Q_SIGNALS:

    /**
     * @brief Emitted whenever the other process do no longer seem to exist
     **/
    void otherProcessUnreachable();

private:

    virtual void run() OVERRIDE FINAL;
    boost::scoped_ptr<ExistenceCheckerThreadPrivate> _imp;
};

NATRON_NAMESPACE_EXIT


#endif // Engine_ExistenceCheckThread_h
