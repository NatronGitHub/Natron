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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PreviewThread.h"

#include <list>
#include <vector>
#include <stdexcept>
#include <cstring> // for std::memcpy, std::memset

#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>

#include "Gui/GuiDefines.h"
#include "Gui/NodeGui.h"

#include "Engine/Node.h"


NATRON_NAMESPACE_ENTER

class ComputePreviewRequest
    : public GenericThreadStartArgs
{
public:

    TimeValue time;
    NodeGuiWPtr node;

    ComputePreviewRequest()
        : GenericThreadStartArgs()
        , time(0)
        , node()
    {}

    virtual ~ComputePreviewRequest()
    {
    }
};

typedef boost::shared_ptr<ComputePreviewRequest> ComputePreviewRequestPtr;

struct PreviewThreadPrivate
{
    std::vector<unsigned int> data;

    PreviewThreadPrivate()
        : data( NATRON_PREVIEW_HEIGHT * NATRON_PREVIEW_WIDTH * sizeof(unsigned int) )
    {
    }
};

PreviewThread::PreviewThread()
    : GenericSchedulerThread()
    , _imp( new PreviewThreadPrivate() )
{
    setThreadName("PreviewThread");
}

PreviewThread::~PreviewThread()
{
    quitThread(false);
    waitForThreadToQuit_enforce_blocking();
}

void
PreviewThread::appendToQueue(const NodeGuiPtr& node,
                             TimeValue time)
{
    ComputePreviewRequestPtr r = boost::make_shared<ComputePreviewRequest>();

    r->node = node;
    r->time = time;
    startTask(r);
}

GenericSchedulerThread::ThreadStateEnum
PreviewThread::threadLoopOnce(const GenericThreadStartArgsPtr& inArgs)
{
    ComputePreviewRequestPtr args = boost::dynamic_pointer_cast<ComputePreviewRequest>(inArgs);

    assert(args);


    NodeGuiPtr node = args->node.lock();
    if (node) {

        //process the request if valid
        int w = NATRON_PREVIEW_WIDTH;
        int h = NATRON_PREVIEW_HEIGHT;

        //set buffer to 0
#ifndef __NATRON_WIN32__
        std::memset( &_imp->data.front(), 0, _imp->data.size() * sizeof(unsigned int) );
#else
        for (std::size_t i = 0; i < _imp->data.size(); ++i) {
            _imp->data[i] = qRgba(0, 0, 0, 255);
        }
#endif
        NodePtr internalNode = node->getNode();
        if (internalNode) {
            bool ok = internalNode->makePreviewImage( args->time, w, h, &_imp->data.front() );
            Q_UNUSED(ok);
            node->copyPreviewImageBuffer(_imp->data, w, h);
        }

    }

    return eThreadStateActive;
} // PreviewThread::threadLoopOnce

NATRON_NAMESPACE_EXIT

