//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "BlockingBackgroundRender.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QDebug>
CLANG_DIAG_ON(deprecated-register)
#include "Engine/EffectInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Settings.h"

BlockingBackgroundRender::BlockingBackgroundRender(Natron::OutputEffectInstance* writer)
    : _running(false)
      ,_writer(writer)
{
}

void
BlockingBackgroundRender::blockingRender(bool enableRenderStats,int first,int last)
{
    _writer->renderFullSequence(enableRenderStats,this,first,last);
    if (appPTR->getCurrentSettings()->getNumberOfThreads() != -1) {
        QMutexLocker locker(&_runningMutex);
        _running = true;
        while (_running) {
            _runningCond.wait(&_runningMutex);
        }
    }
}

void
BlockingBackgroundRender::notifyFinished()
{
#ifdef DEBUG
    std::cout << "Blocking render finished." << std::endl;
#endif
    appPTR->writeToOutputPipe(kRenderingFinishedStringLong,kRenderingFinishedStringShort);
    QMutexLocker locker(&_runningMutex);
    _running = false;
    _runningCond.wakeOne();
}

