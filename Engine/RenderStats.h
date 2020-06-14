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

#ifndef RENDERSTATS_H
#define RENDERSTATS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>
#include <set>
#include <string>
#include <bitset>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/RectI.h"
#include "Engine/RectD.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief Holds render infos for one frame for one node. Not MT-safe: MT-safety is handled by RenderStats.
 **/
struct NodeRenderStatsPrivate;
class NodeRenderStats
{
public:

    NodeRenderStats();

    NodeRenderStats(const NodeRenderStats& other);

    ~NodeRenderStats();

    void operator=(const NodeRenderStats& other);

    void addTimeSpentRendering(double time);
    double getTotalTimeSpentRendering() const;


private:

    boost::scoped_ptr<NodeRenderStatsPrivate> _imp;
};

/**
 * @brief Holds render infos for all nodes in a compositing tree for a frame.
 **/
struct RenderStatsPrivate;
class RenderStats
{
public:

    /**
     * @brief If enableInDepthProfiling is true, a detailed breakdown for each node will be available in getStats()
     * otherwise just the totalTimeSpent for the frame will be computed.
     **/
    RenderStats(bool enableInDepthProfiling);

    ~RenderStats();

    bool isInDepthProfilingEnabled() const;

    void addRenderInfosForNode(const NodePtr& node, double timeSpent);

    std::map<NodePtr, NodeRenderStats > getStats(double *totalTimeSpent) const;

private:

    boost::scoped_ptr<RenderStatsPrivate> _imp;
};

NATRON_NAMESPACE_EXIT


#endif // RENDERSTATS_H
