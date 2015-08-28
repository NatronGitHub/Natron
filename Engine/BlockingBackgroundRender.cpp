/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

