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

#include <bitset>

#include "RenderStats.h"

#include <cassert>
#include <stdexcept>

#include <QMutex>

#include "Engine/Node.h"
#include "Engine/Timer.h"
#include "Engine/RectI.h"
#include "Engine/RectD.h"

NATRON_NAMESPACE_ENTER;

struct NodeRenderStatsPrivate
{
    //The accumulated time spent in the EffectInstance::renderHandler function
    double totalTimeSpentRendering;
    
    //The region of definition of the node for this frame
    RectD rod;
    
    //Is identity
    NodeWPtr isWholeImageIdentity;
    
    //The list of all tiles rendered
    std::list<RectI> rectanglesRendered;
    
    //The list of rectangles for which isIdentity returned true
    std::list<std::pair<RectI,NodeWPtr > > identityRectangles;
    
    //The different mipmaplevels rendered
    std::set<unsigned int> mipmapLevelsAccessed;
    
    //The different planes rendered
    std::set<std::string> planesRendered;
    
    //Cache access infos
    int nbCacheMisses;
    int nbCacheHit;
    int nbCacheHitButDownscaledImages;
    
    //Is tile support enabled for this render
    bool tileSupportEnabled;
    
    //Is render scale support enabled for this render
    bool renderScaleSupportEnabled;
    
    //Channels rendered
    std::bitset<4> channelsEnabled;
    
    //Premultiplication of the output imge
    ImagePremultiplicationEnum outputPremult;
    
    NodeRenderStatsPrivate()
    : totalTimeSpentRendering(0)
    , rod()
    , isWholeImageIdentity()
    , rectanglesRendered()
    , identityRectangles()
    , mipmapLevelsAccessed()
    , planesRendered()
    , nbCacheMisses(0)
    , nbCacheHit(0)
    , nbCacheHitButDownscaledImages(0)
    , tileSupportEnabled(false)
    , renderScaleSupportEnabled(false)
    , channelsEnabled()
    , outputPremult(eImagePremultiplicationOpaque)
    {
        for (int i = 0; i < 4; ++i) {
            channelsEnabled[i] = false;
        }
    }
};

NodeRenderStats::NodeRenderStats()
: _imp(new NodeRenderStatsPrivate())
{
    
}

NodeRenderStats::~NodeRenderStats()
{
    
}

NodeRenderStats::NodeRenderStats(const NodeRenderStats& other)
: _imp(new NodeRenderStatsPrivate())
{
    *this = other;
}

void
NodeRenderStats::operator=(const NodeRenderStats& other)
{
    _imp->totalTimeSpentRendering = other._imp->totalTimeSpentRendering;
    _imp->rod = other._imp->rod;
    _imp->isWholeImageIdentity = other._imp->isWholeImageIdentity;
    _imp->rectanglesRendered = other._imp->rectanglesRendered;
    _imp->identityRectangles  = other._imp->identityRectangles;
    _imp->mipmapLevelsAccessed = other._imp->mipmapLevelsAccessed;
    _imp->planesRendered = other._imp->planesRendered;
    _imp->nbCacheMisses = other._imp->nbCacheMisses;
    _imp->nbCacheHit = other._imp->nbCacheHit;
    _imp->nbCacheHitButDownscaledImages = other._imp->nbCacheHitButDownscaledImages;
    _imp->tileSupportEnabled = other._imp->tileSupportEnabled;
    _imp->renderScaleSupportEnabled = other._imp->renderScaleSupportEnabled;
    for (int i = 0; i < 4; ++i) {
        _imp->channelsEnabled[i] = other._imp->channelsEnabled[i];
    }
    _imp->outputPremult = other._imp->outputPremult;
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

const RectD&
NodeRenderStats::getRoD() const
{
    return _imp->rod;
}

void
NodeRenderStats::setRoD(const RectD& rod)
{
    _imp->rod = rod;
}

void
NodeRenderStats::setInputImageIdentity(const NodePtr& identity)
{
    _imp->isWholeImageIdentity = identity;
}

NodePtr
NodeRenderStats::getInputImageIdentity() const
{
    return _imp->isWholeImageIdentity.lock();
}

void
NodeRenderStats::addRenderedRectangle(const RectI& rectangle)
{
    _imp->rectanglesRendered.push_back(rectangle);
}

const std::list<RectI>&
NodeRenderStats::getRenderedRectangles() const
{
    return _imp->rectanglesRendered;
}

void
NodeRenderStats::addIdentityRectangle(const NodePtr& identity, const RectI& rectangle)
{
    _imp->identityRectangles.push_back(std::make_pair(rectangle,identity));
}

std::list<std::pair<RectI,NodePtr > >
NodeRenderStats::getIdentityRectangles() const
{
    std::list<std::pair<RectI,NodePtr > > ret;
    for (std::list<std::pair<RectI,NodeWPtr > >::const_iterator it = _imp->identityRectangles.begin(); it!=_imp->identityRectangles.end(); ++it) {
        NodePtr n = it->second.lock();
        if (n) {
            ret.push_back(std::make_pair(it->first, n));
        }
    }
    return ret;
}

void
NodeRenderStats::addMipMapLevelRendered(unsigned int level)
{
    _imp->mipmapLevelsAccessed.insert(level);
}

const std::set<unsigned int>&
NodeRenderStats::getMipMapLevelsRendered() const
{
    return _imp->mipmapLevelsAccessed;
}

void
NodeRenderStats::addPlaneRendered(const std::string& plane)
{
    _imp->planesRendered.insert(plane);
}

const std::set<std::string>&
NodeRenderStats::getPlanesRendered() const
{
    return _imp->planesRendered;
}

void
NodeRenderStats::addCacheAccessInfo(bool isCacheMiss, bool hasDownscaled)
{
    if (isCacheMiss) {
        ++_imp->nbCacheMisses;
    } else {
        ++_imp->nbCacheHit;
        if (hasDownscaled) {
            ++_imp->nbCacheHitButDownscaledImages;
        }
    }
    
}

void
NodeRenderStats::getCacheAccessInfos(int* nbCacheMisses, int* nbCacheHits, int* nbCacheHitButDownscaledImages) const
{
    *nbCacheMisses = _imp->nbCacheMisses;
    *nbCacheHits = _imp->nbCacheHit;
    *nbCacheHitButDownscaledImages = _imp->nbCacheHitButDownscaledImages;
}

void
NodeRenderStats::setTilesSupported(bool tilesSupported)
{
    _imp->tileSupportEnabled = tilesSupported;
}

bool
NodeRenderStats::isTilesSupportEnabled() const
{
    return _imp->tileSupportEnabled;
}

void
NodeRenderStats::setRenderScaleSupported(bool rsSupported)
{
    _imp->renderScaleSupportEnabled = rsSupported;
}

bool
NodeRenderStats::isRenderScaleSupportEnabled() const
{
    return _imp->renderScaleSupportEnabled;
}

void
NodeRenderStats::setChannelsRendered(const std::bitset<4> channelsRendered)
{
    _imp->channelsEnabled = channelsRendered;
}

std::bitset<4>
NodeRenderStats::getChannelsRendered() const
{
    return _imp->channelsEnabled;
}

void
NodeRenderStats::setOutputPremult(ImagePremultiplicationEnum premult)
{
    _imp->outputPremult = premult;
}

ImagePremultiplicationEnum
NodeRenderStats::getOutputPremult() const
{
    return _imp->outputPremult;
}

struct RenderStatsPrivate
{
    mutable QMutex lock;
    
    //Timer recording time spent for the whole frame
    TimeLapse totalTimeSpentForFrameTimer;
    
    //When true in-depth profiling will be enabled for all Nodes with detailed infos
    bool doNodesProfiling;
    
    typedef std::map<NodeWPtr,NodeRenderStats > NodeInfosMap;
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
        assert(!lock.tryLock());
        
        for (NodeInfosMap::iterator it = nodeInfos.begin(); it!=nodeInfos.end(); ++it) {
            if (it->first.lock() == node) {
                return it;
            }
        }
        return nodeInfos.end();
    }
    
    NodeRenderStats& findOrCreateNodeStats(const NodePtr& node)
    {
      
        NodeInfosMap::iterator found = findNode(node);
        if (found != nodeInfos.end()) {
            return found->second;
        }
        std::pair<NodeInfosMap::iterator,bool> ret = nodeInfos.insert(std::make_pair(node, NodeRenderStats()));
        assert(ret.second);
        return ret.first->second;
    }
    
};

RenderStats::RenderStats(bool enableInDepthProfiling)
: _imp(new RenderStatsPrivate())
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
RenderStats::setNodeIdentity(const NodePtr& node, const NodePtr& identity)
{
    QMutexLocker k(&_imp->lock);
    assert(_imp->doNodesProfiling);
    
    NodeRenderStats& stats = _imp->findOrCreateNodeStats(node);
    stats.setInputImageIdentity(identity);
}

void
RenderStats::setGlobalRenderInfosForNode(const NodePtr& node,
                                         const RectD& rod,
                                         ImagePremultiplicationEnum outputPremult,
                                         std::bitset<4> channelsRendered,
                                         bool tilesSupported,
                                         bool renderScaleSupported,
                                         unsigned int mipmapLevel)
{
    QMutexLocker k(&_imp->lock);
    assert(_imp->doNodesProfiling);
    
    NodeRenderStats& stats = _imp->findOrCreateNodeStats(node);
    stats.setOutputPremult(outputPremult);
    stats.setTilesSupported(tilesSupported);
    stats.setRenderScaleSupported(renderScaleSupported);
    stats.addMipMapLevelRendered(mipmapLevel);
    stats.setChannelsRendered(channelsRendered);
    stats.setRoD(rod);
}

void
RenderStats::addCacheInfosForNode(const NodePtr& node,
                          bool isCacheMiss,
                          bool hasDownscaled)
{
    QMutexLocker k(&_imp->lock);
    assert(_imp->doNodesProfiling);
    
    NodeRenderStats& stats = _imp->findOrCreateNodeStats(node);
    stats.addCacheAccessInfo(isCacheMiss, hasDownscaled);
}

void
RenderStats::addRenderInfosForNode(const NodePtr& node,
                           const NodePtr& identity,
                           const std::string& plane,
                           const RectI& rectangle,
                           double timeSpent)
{
    QMutexLocker k(&_imp->lock);
    assert(_imp->doNodesProfiling);
    
    NodeRenderStats& stats = _imp->findOrCreateNodeStats(node);
    if (identity) {
        stats.addIdentityRectangle(identity, rectangle);
    } else {
        stats.addRenderedRectangle(rectangle);
    }
    stats.addTimeSpentRendering(timeSpent);
    stats.addPlaneRendered(plane);
}

std::map<NodePtr,NodeRenderStats >
RenderStats::getStats(double *totalTimeSpent) const
{
    QMutexLocker k(&_imp->lock);
    
    std::map<NodePtr,NodeRenderStats > ret;
    for (RenderStatsPrivate::NodeInfosMap::const_iterator it = _imp->nodeInfos.begin(); it!=_imp->nodeInfos.end(); ++it) {
        NodePtr node = it->first.lock();
        if (node) {
            ret.insert(std::make_pair(node, it->second));
        }
    }
    
    *totalTimeSpent = _imp->totalTimeSpentForFrameTimer.getTimeSinceCreation();
    
    return ret;
}

NATRON_NAMESPACE_EXIT;
