/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "RenderStats.h"

#include <bitset>
#include <cassert>
#include <stdexcept>

#include <QtCore/QMutex>

#include "Engine/Node.h"
#include "Engine/Timer.h"
#include "Engine/RectI.h"
#include "Engine/RectD.h"

NATRON_NAMESPACE_ENTER

struct NodeRenderStatsPrivate
{
    //The accumulated time spent in the EffectInstance::renderHandler function
    double totalTimeSpentRendering;


    NodeRenderStatsPrivate()
    : totalTimeSpentRendering(0)
    {

    }
};

NodeRenderStats::NodeRenderStats()
    : _imp( new NodeRenderStatsPrivate() )
{
}

NodeRenderStats::~NodeRenderStats()
{
}

NodeRenderStats::NodeRenderStats(const NodeRenderStats& other)
    : _imp( new NodeRenderStatsPrivate() )
{
    *this = other;
}

void
NodeRenderStats::operator=(const NodeRenderStats& other)
{
    _imp->totalTimeSpentRendering = other._imp->totalTimeSpentRendering;
}

void
NodeRenderStats::addTimeSpentRendering(double time)
{
    _imp->totalTimeSpentRendering += time;
}

double
NodeRenderStats::getTotalTimeSpentRendering() const
{
    return _imp->totalTimeSpentRendering;
}


struct RenderStatsPrivate
{
    mutable QMutex lock;

    //Timer recording time spent for the whole frame
    TimeLapse totalTimeSpentForFrameTimer;

    //When true in-depth profiling will be enabled for all Nodes with detailed infos
    bool doNodesProfiling;

    typedef std::map<NodeWPtr, NodeRenderStats > NodeInfosMap;
    NodeInfosMap nodeInfos;


    RenderStatsPrivate()
        : lock()
        , totalTimeSpentForFrameTimer()
        , doNodesProfiling(false)
        , nodeInfos()
    {
    }

    NodeInfosMap::iterator findNode(const NodePtr& node)
    {
        //Private, shouldn't lock
        assert( !lock.tryLock() );

        for (NodeInfosMap::iterator it = nodeInfos.begin(); it != nodeInfos.end(); ++it) {
            if (it->first.lock() == node) {
                return it;
            }
        }

        return nodeInfos.end();
    }

    NodeRenderStats& findOrCreateNodeStats(const NodePtr& node)
    {
        NodeInfosMap::iterator found = findNode(node);

        if ( found != nodeInfos.end() ) {
            return found->second;
        }
        std::pair<NodeInfosMap::iterator, bool> ret = nodeInfos.insert( std::make_pair( node, NodeRenderStats() ) );
        assert(ret.second);

        return ret.first->second;
    }
};

RenderStats::RenderStats(bool enableInDepthProfiling)
    : _imp( new RenderStatsPrivate() )
{
    _imp->doNodesProfiling = enableInDepthProfiling;
}

RenderStats::~RenderStats()
{
}

bool
RenderStats::isInDepthProfilingEnabled() const
{
    return _imp->doNodesProfiling;
}



void
RenderStats::addRenderInfosForNode(const NodePtr& node, double timeSpent)
{
    QMutexLocker k(&_imp->lock);

    assert(_imp->doNodesProfiling);

    NodeRenderStats& stats = _imp->findOrCreateNodeStats(node);
    stats.addTimeSpentRendering(timeSpent);
}

std::map<NodePtr, NodeRenderStats >
RenderStats::getStats(double *totalTimeSpent) const
{
    QMutexLocker k(&_imp->lock);
    std::map<NodePtr, NodeRenderStats > ret;

    for (RenderStatsPrivate::NodeInfosMap::const_iterator it = _imp->nodeInfos.begin(); it != _imp->nodeInfos.end(); ++it) {
        NodePtr node = it->first.lock();
        if (node) {
            ret.insert( std::make_pair(node, it->second) );
        }
    }

    *totalTimeSpent = _imp->totalTimeSpentForFrameTimer.getTimeSinceCreation();

    return ret;
}

NATRON_NAMESPACE_EXIT
