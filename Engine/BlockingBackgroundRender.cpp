//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#include "BlockingBackgroundRender.h"
#include <QDebug>
#include "Engine/EffectInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Settings.h"

BlockingBackgroundRender::BlockingBackgroundRender(boost::shared_ptr<Natron::OutputEffectInstance> writer)
: _running(false)
,_writer(writer)
{
    
}

void BlockingBackgroundRender::blockingRender(){
    _writer->renderFullSequence(this);
    if (appPTR->getCurrentSettings()->getNumberOfThreads() != -1) {
        QMutexLocker locker(&_runningMutex);
        _running = true;
        while (_running) {
            _runningCond.wait(&_runningMutex);
        }
    }
}

void BlockingBackgroundRender::notifyFinished() {
    qDebug() << "Blocking render finished.";
    appPTR->writeToOutputPipe(kRenderingFinishedStringLong,kRenderingFinishedStringShort);
    QMutexLocker locker(&_runningMutex);
    _running = false;
    _runningCond.wakeOne();
}

