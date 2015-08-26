//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef BLOCKINGBACKGROUNDRENDER_H
#define BLOCKINGBACKGROUNDRENDER_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <QMutex>
#include <QWaitCondition>

namespace Natron {
class OutputEffectInstance;
}

class BlockingBackgroundRender
{
    bool _running;
    QWaitCondition _runningCond;
    QMutex _runningMutex;
    Natron::OutputEffectInstance* _writer;

public:

    BlockingBackgroundRender(Natron::OutputEffectInstance* writer);

    virtual ~BlockingBackgroundRender()
    {
    }

    Natron::OutputEffectInstance* getWriter() const
    {
        return _writer;
    }

    void notifyFinished();

    void blockingRender(bool enableRenderStats,int first,int last);
};

#endif // BLOCKINGBACKGROUNDRENDER_H
