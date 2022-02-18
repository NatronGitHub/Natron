/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "BlockingBackgroundRender.h"

#include <cassert>
#include <stdexcept>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QtCore/QDebug>
CLANG_DIAG_ON(deprecated-register)

#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER


BlockingBackgroundRender::BlockingBackgroundRender(OutputEffectInstance* writer)
    : _running(false)
    , _writer(writer)
{
}

void
BlockingBackgroundRender::blockingRender(bool enableRenderStats,
                                         int first,
                                         int last,
                                         int frameStep)
{
    // avoid race condition: the code must work even if renderFullSequence() calls notifyFinished()
    // immediately.
    QMutexLocker locker(&_runningMutex);

    assert(_running == false);
    _running = true;
    _writer->renderFullSequence(true, enableRenderStats, this, first, last, frameStep);
    if (appPTR->getCurrentSettings()->getNumberOfThreads() == -1) {
        _running = false;
    } else {
        while (_running) {
            _runningCond.wait(locker.mutex());
        }
    }
}

void
BlockingBackgroundRender::notifyFinished()
{
    QMutexLocker locker(&_runningMutex);

    assert(_running == true);
    _running = false;
    _runningCond.wakeOne();
}

NATRON_NAMESPACE_EXIT
