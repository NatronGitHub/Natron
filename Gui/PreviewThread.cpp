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
#include <list>
#include <vector>

#include "PreviewThread.h"

#include <QWaitCondition>
#include <QMutex>

#include "Gui/GuiDefines.h"
#include "Gui/NodeGui.h"

#include "Engine/Node.h"


using namespace Natron;

struct ComputePreviewRequest
{
    double time;
    boost::shared_ptr<NodeGui> node;
};

struct PreviewThreadPrivate
{
    mutable QMutex previewQueueMutex;
    std::list<ComputePreviewRequest> previewQueue;
    QWaitCondition previewQueueNotEmptyCond;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;
    
    std::vector<unsigned int> data;
    
    PreviewThreadPrivate()
    : previewQueueMutex()
    , previewQueue()
    , previewQueueNotEmptyCond()
    , mustQuitMutex()
    , mustQuitCond()
    , mustQuit(false)
    , data(NATRON_PREVIEW_HEIGHT * NATRON_PREVIEW_WIDTH * sizeof(unsigned int))
    {
        
    }
    
};

PreviewThread::PreviewThread()
: QThread()
, _imp(new PreviewThreadPrivate())
{
    setObjectName("PreviewThread");
}

PreviewThread::~PreviewThread()
{
    
}

void
PreviewThread::appendToQueue(const boost::shared_ptr<NodeGui>& node, double time)
{

    {
        QMutexLocker k(&_imp->previewQueueMutex);
        ComputePreviewRequest r;
        r.node = node;
        r.time = time;
        _imp->previewQueue.push_back(r);
    }
    if (!isRunning()) {
        start();
    } else {
        QMutexLocker k(&_imp->previewQueueMutex);
        _imp->previewQueueNotEmptyCond.wakeOne();
    }
}

void
PreviewThread::quitThread()
{
    if (!isRunning()) {
        return;
    }
    QMutexLocker k(&_imp->mustQuitMutex);
    _imp->mustQuit = true;
    
    {
        QMutexLocker k2(&_imp->previewQueueMutex);
        ComputePreviewRequest r;
        r.node = boost::shared_ptr<NodeGui>();
        r.time = 0;
        _imp->previewQueue.push_back(r);
        _imp->previewQueueNotEmptyCond.wakeOne();
    }
    while (_imp->mustQuit) {
        _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
    }
    
    {
        QMutexLocker k2(&_imp->previewQueueMutex);
        _imp->previewQueue.clear();
    }
}

bool
PreviewThread::isWorking() const
{
    QMutexLocker k(&_imp->previewQueueMutex);
    return !_imp->previewQueue.empty();
}

void
PreviewThread::run()
{
    for (;;) {
        
        //Check for exit
        {
            QMutexLocker k(&_imp->mustQuitMutex);
            if (_imp->mustQuit) {
                _imp->mustQuit = false;
                _imp->mustQuitCond.wakeAll();
                return;
            }
        }
        
        
        ComputePreviewRequest front;
        {
            //Wait until we get a request
            QMutexLocker k(&_imp->previewQueueMutex);
            while (_imp->previewQueue.empty()) {
                _imp->previewQueueNotEmptyCond.wait(&_imp->previewQueueMutex);
            }
            
            assert(!_imp->previewQueue.empty());
            front = _imp->previewQueue.front();
            _imp->previewQueue.pop_front();
        }
        
        
        if (front.node) {
            //process the request if valid
            int w = NATRON_PREVIEW_WIDTH;
            int h = NATRON_PREVIEW_HEIGHT;
            
            //set buffer to 0
#ifndef __NATRON_WIN32__
            memset(&_imp->data.front(), 0, _imp->data.size() * sizeof(unsigned int));
#else
            for (std::size_t i = 0; i < _imp->data.size(); ++i) {
                _imp->data[i] = qRgba(0,0,0,255);
            }
#endif
            boost::shared_ptr<Natron::Node> internalNode = front.node->getNode();
            if (internalNode) {
                bool success = internalNode->makePreviewImage(front.time, &w, &h, _imp->data.data());
                if (success) {
                    front.node->copyPreviewImageBuffer(_imp->data, w, h);
                }
            }
        }
        
        
    } // for(;;)
}

