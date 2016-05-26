/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "AbortableRenderInfo.h"

#include <set>
#include <sstream>
#include <string>

#include <QtCore/QMutex>
#include <QtCore/QAtomicInt>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ThreadPool.h"
#include "Engine/Timer.h"

// After this amount of time, if any thread identified in this render is still remaining
// that means they are stuck probably doing a long processing that cannot be aborted or in a separate thread
// that we did not spawn. Anyway, report to the user that we cannot control this thread anymore and that it may
// waste resources.
#define NATRON_ABORT_TIMEOUT_MS 5000

NATRON_NAMESPACE_ENTER;

typedef std::set<AbortableThread*> ThreadSet;

struct AbortableRenderInfoPrivate
{
    AbortableRenderInfo* _p;
    bool canAbort;
    QAtomicInt aborted;
    U64 age;
    mutable QMutex threadsMutex;
    ThreadSet threadsForThisRender;

    mutable QMutex timerMutex;
    bool timerStarted;
    boost::scoped_ptr<QTimer> abortTimeoutTimer;


    // This is to check that we correctly stop the timer in the thread that created it
    QThread* timerThread;

    AbortableRenderInfoPrivate(AbortableRenderInfo* p,
                               bool canAbort,
                               U64 age)
        : _p(p)
        , canAbort(canAbort)
        , aborted()
        , age(age)
        , threadsMutex()
        , threadsForThisRender()
        , timerMutex()
        , timerStarted(false)
        , abortTimeoutTimer()
        , timerThread(0)
    {
        aborted.fetchAndStoreAcquire(0);

       // QObject::connect(p, SIGNAL(startTimerInOriginalThread()), p, SLOT(onStartTimerInOriginalThreadTriggered()));
    }
};

AbortableRenderInfo::AbortableRenderInfo()
    : _imp( new AbortableRenderInfoPrivate(this, true, 0) )
{
}

AbortableRenderInfo::AbortableRenderInfo(bool canAbort,
                                         U64 age)
    : _imp( new AbortableRenderInfoPrivate(this, canAbort, age) )
{



}

AbortableRenderInfo::~AbortableRenderInfo()
{
    // If the timer is still alive, we need to ensure it gets killed in the thread that created it, we block until in gets killed
    bool mustCallStopTimer = false;
    bool mustCallStopTimerInOtherThread = false;
    {
        QMutexLocker k(&_imp->timerMutex);
        if (_imp->abortTimeoutTimer) {
            assert(_imp->timerThread);
            mustCallStopTimerInOtherThread = _imp->timerThread != QThread::currentThread();
            mustCallStopTimer = true;
        }
    }
    if (mustCallStopTimer) {
        if (mustCallStopTimerInOtherThread) {
            Q_EMIT stopTimerInOriginalThread();
        } else {
            onStopTimerInOriginalThreadTriggered();
        }
    }
}

U64
AbortableRenderInfo::getRenderAge() const
{
    return _imp->age;
}

bool
AbortableRenderInfo::canAbort() const
{
    return _imp->canAbort;
}

bool
AbortableRenderInfo::isAborted() const
{
    return (int)_imp->aborted > 0;
}

void
AbortableRenderInfo::setAborted()
{
    int abortedValue = _imp->aborted.fetchAndAddAcquire(1);

    if (abortedValue > 0) {
        return;
    }
    {
        QMutexLocker k(&_imp->timerMutex);
        _imp->timerStarted = true;
    }
    onStartTimerInOriginalThreadTriggered();
}


void
AbortableRenderInfo::registerThreadForRender(AbortableThread* thread)
{
    QMutexLocker k(&_imp->threadsMutex);

    _imp->threadsForThisRender.insert(thread);
}

bool
AbortableRenderInfo::unregisterThreadForRender(AbortableThread* thread)
{
    bool ret = false;
    bool emitStopTimerSignal = false;
    {
        QMutexLocker k(&_imp->threadsMutex);
        ThreadSet::iterator found = _imp->threadsForThisRender.find(thread);

        if ( found != _imp->threadsForThisRender.end() ) {
            _imp->threadsForThisRender.erase(found);
            ret = true;
        }
        // Stop the timer has no more threads are running for this render
        if (_imp->threadsForThisRender.empty() && _imp->abortTimeoutTimer) {
            emitStopTimerSignal = true;
        }
    }
    if (emitStopTimerSignal) {
        bool mustCallStopTimerInOtherThread;
        {
            QMutexLocker k(&_imp->timerMutex);
            _imp->timerStarted = false;
            mustCallStopTimerInOtherThread = QThread::currentThread() != _imp->timerThread;
        }
        if (mustCallStopTimerInOtherThread) {
            Q_EMIT stopTimerInOriginalThread();
        } else {
            onStopTimerInOriginalThreadTriggered();
        }
    }
    return ret;
}

void
AbortableRenderInfo::onStartTimerInOriginalThreadTriggered()
{

    _imp->timerThread = QThread::currentThread();

    {
        QMutexLocker k(&_imp->timerMutex);
        _imp->abortTimeoutTimer.reset( new QTimer() );
        _imp->abortTimeoutTimer->setSingleShot(true);
        QObject::connect( _imp->abortTimeoutTimer.get(), SIGNAL(timeout()), this, SLOT(onAbortTimerTimeout()) );
        QObject::connect(this, SIGNAL(stopTimerInOriginalThread()), this, SLOT(onStopTimerInOriginalThreadTriggered()), Qt::BlockingQueuedConnection);
    }
    _imp->abortTimeoutTimer->start(NATRON_ABORT_TIMEOUT_MS);
}

void
AbortableRenderInfo::onStopTimerInOriginalThreadTriggered()
{
    assert(QThread::currentThread() == _imp->timerThread);
    QMutexLocker k(&_imp->timerMutex);
    if ( _imp->abortTimeoutTimer) {
        _imp->abortTimeoutTimer->stop();
        _imp->abortTimeoutTimer.reset();
        _imp->timerThread = 0;
    }

}

void
AbortableRenderInfo::onAbortTimerTimeout()
{
    {
        QMutexLocker k(&_imp->timerMutex);
        if (!_imp->timerStarted) {
            // The timer was stopped but the onStopTimerInOriginalThreadTriggered() handler has not been executed yet on the timerThreads
            return;
        }
    }

    // Runs in the thread that called setAborted()
    QMutexLocker k(&_imp->threadsMutex);
    if ( _imp->threadsForThisRender.empty() ) {
        return;
    }
    QString timeoutStr = Timer::printAsTime(NATRON_ABORT_TIMEOUT_MS / 1000, false);
    std::stringstream ss;
    ss << tr("One or multiple render seems to not be responding anymore after numerous attempt made by %1 to abort them for the last %2.").arg ( QString::fromUtf8( NATRON_APPLICATION_NAME) ).arg(timeoutStr).toStdString() << std::endl;
    ss << tr("This is likely due to a render taking too long in a plug-in.").toStdString() << std::endl << std::endl;
    ss << tr("List of stalled renders:").toStdString() << std::endl << std::endl;
    for (ThreadSet::const_iterator it = _imp->threadsForThisRender.begin(); it != _imp->threadsForThisRender.end(); ++it) {
        std::string actionName;
        NodePtr node;
        (*it)->getCurrentActionInfos(&actionName, &node);
        if (node) {
            // Don't show a dialog on timeout for writers since reading/writing from/to a file may be long and most libraries don't provide write callbacks anyway
            if ( node->getEffectInstance()->isReader() || node->getEffectInstance()->isWriter() ) {
                return;
            }
            std::string nodeName, pluginId;
            nodeName = node->getFullyQualifiedName();
            pluginId = node->getPluginID();

            ss << " - " << (*it)->getThreadName()  << tr(" stalled in:").toStdString() << std::endl << std::endl;

            if ( !nodeName.empty() ) {
                ss << "    Node: " << nodeName << std::endl;
            }
            if ( !pluginId.empty() ) {
                ss << "    Plugin: " << pluginId << std::endl;
            }
            if ( !actionName.empty() ) {
                ss << "    Action: " << actionName << std::endl;
            }
            ss << std::endl;
        }
    }
    ss << std::endl;

    if ( appPTR->isBackground() ) {
        qDebug() << ss.str().c_str();
    } else {
        ss << tr("Would you like to kill these renders?").toStdString() << std::endl << std::endl;
        ss << tr("WARNING: Killing them may not work or may leave %1 in a bad state. The application may crash or freeze as a consequence of this. It is advised to restart %1 instead.").arg( QString::fromUtf8( NATRON_APPLICATION_NAME) ).toStdString();

        std::string message = ss.str();
        StandardButtonEnum reply = Dialogs::questionDialog(tr("A Render is not responding anymore").toStdString(), ss.str(), false, StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonNo);
        if (reply == eStandardButtonYes) {
            // Kill threads
            for (ThreadSet::const_iterator it = _imp->threadsForThisRender.begin(); it != _imp->threadsForThisRender.end(); ++it) {
                (*it)->killThread();
            }
        }
    }
} // AbortableRenderInfo::onAbortTimerTimeout

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;

#include "moc_AbortableRenderInfo.cpp"
