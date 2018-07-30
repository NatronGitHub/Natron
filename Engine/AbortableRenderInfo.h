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

#ifndef ABORTABLERENDERINFO_H
#define ABORTABLERENDERINFO_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QtCore/QObject>

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

/**
 * @brief Holds infos necessary to identify one render request and whether it was aborted or not.
 *
 * Typical usage is to start a render on a thread deriving the AbortableThread class, then make a new
 * AbortableRenderInfo object and set the info on the thread with the AbortableThread::setAbortInfo function.
 *
 * Whenever the GenericSchedulerThread::abortThreadedTask() function is called, this will set the aborted flag on this info
 * and the thread will be notified. The thread should then periodically call AbortableThread::getAbortInfo and call the AbortableRenderInfo::isAborted()
 * function to find out whether the render was aborted or not. Some renders relying on a 3rd party library might not peek the isAborted() function.
 * In this case, if this thread does not return from its task after some time that the user called setAborted(), then the user will be notified about a potentially
 * stalled render.
 **/
struct AbortableRenderInfoPrivate;
class AbortableRenderInfo
    : public QObject
    , public boost::enable_shared_from_this<AbortableRenderInfo>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    /**
     * @brief Create new infos for a specific render that can be aborted with the given age.
     * The render age identifies one single frame render in the Viewer. The older a render is, the smaller its render age is.
     * This is used in the Viewer keep and order on the render requests, even though each request runs concurrently of another.
     * This is not used by playback since in playback the ordering is controlled by the frame number.
     **/
    AbortableRenderInfo(bool canAbort,
                        U64 age);

    /**
     * @brief Same as AbortableRenderInfo(true, 0)
     **/
    AbortableRenderInfo();

public:
    // public constructors

    // Note: when switching to C++11, we can add variadic templates:
    //template<typename ... T>
    //static AbortableRenderInfoPtr create( T&& ... all ) {
    //    return AbortableRenderInfoPtr( new AbortableRenderInfo( std::forward<T>(all)... ) );
    //}

    static AbortableRenderInfoPtr create(bool canAbort,
                                                         U64 age)
    {
        return AbortableRenderInfoPtr( new AbortableRenderInfo(canAbort, age) );
    }

    static AbortableRenderInfoPtr create()
    {
        return AbortableRenderInfoPtr( new AbortableRenderInfo() );
    }

    ~AbortableRenderInfo();

    // Is this render abortable ?
    bool canAbort() const;

    // Is this render aborted ? This is extremely fast as it just dereferences an atomic integer
    bool isAborted() const;

    /**
     * @brief Set this render as aborted, cannot be reversed. This is call when the function GenericSchedulerThread::abortThreadedTask() is called
     **/
    void setAborted();

    /**
     * @brief Get the render age. The render age identifies one single frame render in the Viewer. The older a render is, the smaller its render age is.
     * This is used in the Viewer keep and order on the render requests, even though each request runs concurrently of another.
     * This is not used by playback since in playback the ordering is controlled by the frame number.
     **/
    U64 getRenderAge() const;

    /**
     * @brief Registers the thread as part of this render request. Whenever AbortableThread::setAbortInfo is called, the thread is automatically registered
     * in this class as to be part of this render. This is used to monitor running threads for a specific render and to know if a thread has stalled when
     * the user called setAborted()
     **/
    void registerThreadForRender(AbortableThread* thread);

    /**
     * @brief Unregister the thread as part of this render, returns true
     * if it was registered and found.
     * This is called whenever a thread calls cleanupTLSForThread(), i.e:
     * when its processing is finished.
     **/
    bool unregisterThreadForRender(AbortableThread* thread);

public Q_SLOTS:

    /**
     * @brief Triggered by a timer after some time that a thread called setAborted(). This is used to detect stalled threads that do not seem
     * to want to abort.
     **/
    void onAbortTimerTimeout();

    void onStartTimerInOriginalThreadTriggered();

Q_SIGNALS:

    void startTimerInOriginalThread();

private:

    boost::scoped_ptr<AbortableRenderInfoPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // ABORTABLERENDERINFO_H
