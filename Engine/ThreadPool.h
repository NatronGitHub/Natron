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

#ifndef Natron_Engine_ThreadPool_h
#define Natron_Engine_ThreadPool_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include <QtCore/QThreadPool> // defines QT_CUSTOM_THREADPOOL (or not)

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


/**
 * @brief This class provides a fast way to determine whether a render thread
 * is aborted or not.
 **/
struct AbortableThreadPrivate;
class AbortableThread
{
public:

    AbortableThread(QThread* thread);

    virtual ~AbortableThread();

    /**
     * @brief Set the information related to a specific render so we know if it was aborted or not in getAbortInfo()
     **/
    void setAbortInfo(bool isRenderResponseToUserInteraction,
                      const AbortableRenderInfoPtr& abortInfo,
                      const EffectInstancePtr& treeRoot);

    /**
     * @brief Clear any render-specific abort info held on this thread
     **/
    void clearAbortInfo();

    /**
     * @brief Returns the abort info related on the specific render ongoing on this thread.
     * This is used in EffectInstance::aborted() to figure out if a render thread was aborted or not.
     **/
    bool getAbortInfo(bool* isRenderResponseToUserInteraction,
                      AbortableRenderInfoPtr* abortInfo,
                      EffectInstancePtr* treeRoot) const;

    // For debug purposes, so that the debugger can display the thread name
    void setThreadName(const std::string& threadName);

    // The name of the thread
    const std::string& getThreadName() const;

    /**
     * @brief Flag that the thread is currently entering the given actionName of the given node. This is used
     * to report to the user if a thread seems to be unresponding after a long time that the user aborted it.
     * @see AbortableRenderInfo::onAbortTimerTimeout()
     **/
    void setCurrentActionInfos(const std::string& actionName, const NodePtr& node);
    void getCurrentActionInfos(std::string* actionName, NodePtr* node) const;

    /**
     * @brief Used with caution! Terminates the execution of the thread.
     * When the thread is terminated, all threads waiting for the thread to finish will be woken up.
     *
     * ||||WARNING|||: This function is dangerous and its use is discouraged.
     * The thread can be terminated at any point in its code path. Threads can be terminated while modifying data.
     * There is no chance for the thread to clean up after itself, unlock any held mutexes, etc. In short, use this function only if absolutely necessary.
     **/
    void killThread();

    QThread* getThread() const;

    virtual bool isThreadPoolThread() const { return false; }

private:

    boost::scoped_ptr<AbortableThreadPrivate> _imp;
};

#define REPORT_CURRENT_THREAD_ACTION(actionName, node) \
    { \
        QThread* thread = QThread::currentThread(); \
        AbortableThread* isAbortable = dynamic_cast<AbortableThread*>(thread); \
        if (isAbortable) {  \
            isAbortable->setCurrentActionInfos(actionName, node); \
        } \
    } \

// We patched Qt to be able to derive QThreadPool to control the threads that are spawned to improve performances
// of the EffectInstance::aborted() function. This is done by enabling QThreadPoolThread* to derive AbortableThread.
#ifdef QT_CUSTOM_THREADPOOL

class ThreadPool
    : public QThreadPool
{
public:

    ThreadPool();

    virtual ~ThreadPool();

private:

    virtual QThreadPoolThread* createThreadPoolThread() const OVERRIDE FINAL;
};

#endif // QT_CUSTOM_THREADPOOL

NATRON_NAMESPACE_EXIT

#endif // Natron_Engine_ThreadPool_h
