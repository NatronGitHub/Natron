//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef BLOCKINGBACKGROUNDRENDER_H
#define BLOCKINGBACKGROUNDRENDER_H

#include <QMutex>
#include <QWaitCondition>

namespace Natron {
    class OutputEffectInstance;
}

class BlockingBackgroundRender{
    
    
    bool _running;
    QWaitCondition _runningCond;
    QMutex _runningMutex;
    Natron::OutputEffectInstance* _writer;
public:
    
    BlockingBackgroundRender(Natron::OutputEffectInstance* writer);
    
    virtual ~BlockingBackgroundRender(){}
    
    Natron::OutputEffectInstance* getWriter() const { return _writer; }
    
    void notifyFinished();
    
    void blockingRender();
    
};
#endif // BLOCKINGBACKGROUNDRENDER_H
