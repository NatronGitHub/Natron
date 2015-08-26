//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "RenderStats.h"

#include <QMutex>

#include "Engine/Node.h"
#include "Engine/Timer.h"
#include "Engine/RectI.h"
#include "Engine/RectD.h"


struct NodeRenderStatsPrivate
{
    //The accumulated time spent in the EffectInstance::renderHandler function
    double totalTimeSpentRendering;
    
    //The region of definition of the node for this frame
    RectD rod;
    
    //Is identity
    boost::weak_ptr<Natron::Node> isWholeImageIdentity;
    
    //The list of all tiles rendered
    std::list<RectI> rectanglesRendered;
    
    //The list of rectangles for which isIdentity returned true
    std::list<std::pair<RectI,boost::weak_ptr<Natron::Node> > > identityRectangles;
    
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
    bool channelsEnabled[4];
    
    //Premultiplication of the output imge
    Natron::ImagePremultiplicationEnum outputPremult;
    
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
    , outputPremult(Natron::eImagePremultiplicationOpaque)
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
NodeRenderStats::setInputImageIdentity(const boost::shared_ptr<Natron::Node>& identity)
{
    _imp->isWholeImageIdentity = identity;
}

boost::shared_ptr<Natron::Node>
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
NodeRenderStats::addIdentityRectangle(const boost::shared_ptr<Natron::Node>& identity, const RectI& rectangle)
{
    _imp->identityRectangles.push_back(std::make_pair(rectangle,identity));
}

std::list<std::pair<RectI,boost::shared_ptr<Natron::Node> > >
NodeRenderStats::getIdentityRectangles() const
{
    std::list<std::pair<RectI,boost::shared_ptr<Natron::Node> > > ret;
    for (std::list<std::pair<RectI,boost::weak_ptr<Natron::Node> > >::const_iterator it = _imp->identityRectangles.begin(); it!=_imp->identityRectangles.end(); ++it) {
        boost::shared_ptr<Natron::Node> n = it->second.lock();
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
NodeRenderStats::setChannelsRendered(bool r, bool g, bool b, bool a)
{
    _imp->channelsEnabled[0] = r;
    _imp->channelsEnabled[1] = g;
    _imp->channelsEnabled[2] = b;
    _imp->channelsEnabled[3] = a;
}

void
NodeRenderStats::getChannelsRendered(bool* r, bool* g, bool* b, bool* a) const
{
    *r = _imp->channelsEnabled[0];
    *g = _imp->channelsEnabled[1];
    *b = _imp->channelsEnabled[2];
    *a = _imp->channelsEnabled[3];
}

void
NodeRenderStats::setOutputPremult(Natron::ImagePremultiplicationEnum premult)
{
    _imp->outputPremult = premult;
}

Natron::ImagePremultiplicationEnum
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
    
    typedef std::map<boost::weak_ptr<Natron::Node>,NodeRenderStats > NodeInfosMap;
    NodeInfosMap nodeInfos;
    
    
    
    RenderStatsPrivate()
    : lock()
    , totalTimeSpentForFrameTimer()
    , doNodesProfiling(false)
    , nodeInfos()
    {
        
    }
    
    NodeInfosMap::iterator findNode(const boost::shared_ptr<Natron::Node>& node)
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
    
    NodeRenderStats& findOrCreateNodeStats(const boost::shared_ptr<Natron::Node>& node)
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

void
RenderStats::setNodeIdentity(const boost::shared_ptr<Natron::Node>& node, const boost::shared_ptr<Natron::Node>& identity)
{
    QMutexLocker k(&_imp->lock);
    assert(_imp->doNodesProfiling);
    
    NodeRenderStats& stats = _imp->findOrCreateNodeStats(node);
    stats.setInputImageIdentity(identity);
}

void
RenderStats::setGlobalRenderInfosForNode(const boost::shared_ptr<Natron::Node>& node,
                                 const RectD& rod,
                                 Natron::ImagePremultiplicationEnum outputPremult,
                                 bool channelsRendered[4],
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
    stats.setChannelsRendered(channelsRendered[0], channelsRendered[1], channelsRendered[2], channelsRendered[3]);
    stats.setRoD(rod);
}

void
RenderStats::addCacheInfosForNode(const boost::shared_ptr<Natron::Node>& node,
                          bool isCacheMiss,
                          bool hasDownscaled)
{
    QMutexLocker k(&_imp->lock);
    assert(_imp->doNodesProfiling);
    
    NodeRenderStats& stats = _imp->findOrCreateNodeStats(node);
    stats.addCacheAccessInfo(isCacheMiss, hasDownscaled);
}

void
RenderStats::addRenderInfosForNode(const boost::shared_ptr<Natron::Node>& node,
                           const boost::shared_ptr<Natron::Node>& identity,
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

std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >
RenderStats::getStats(double *totalTimeSpent) const
{
    QMutexLocker k(&_imp->lock);
    
    std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats > ret;
    for (RenderStatsPrivate::NodeInfosMap::const_iterator it = _imp->nodeInfos.begin(); it!=_imp->nodeInfos.end(); ++it) {
        boost::shared_ptr<Natron::Node> node = it->first.lock();
        if (node) {
            ret.insert(std::make_pair(node, it->second));
        }
    }
    
    *totalTimeSpent = _imp->totalTimeSpentForFrameTimer.getTimeSinceCreation();
    
    return ret;
}