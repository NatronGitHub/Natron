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

#ifndef RENDERSTATS_H
#define RENDERSTATS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

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
    
    const RectD& getRoD() const;
    void setRoD(const RectD& rod);
    
    void setInputImageIdentity(const boost::shared_ptr<Node>& identity);
    boost::shared_ptr<Node> getInputImageIdentity() const;
    
    void addRenderedRectangle(const RectI& rectangle);
    const std::list<RectI>& getRenderedRectangles() const;
    
    void addIdentityRectangle(const boost::shared_ptr<Node>& identity, const RectI& rectangle);
    std::list<std::pair<RectI,boost::shared_ptr<Node> > > getIdentityRectangles() const;
    
    void addMipMapLevelRendered(unsigned int level);
    const std::set<unsigned int>& getMipMapLevelsRendered() const;
    
    void addPlaneRendered(const std::string& plane);
    const std::set<std::string>& getPlanesRendered() const;
    
    void addCacheAccessInfo(bool isCacheMiss, bool hasDownscaled);
    void getCacheAccessInfos(int* nbCacheMisses, int* nbCacheHits, int* nbCacheHitButDownscaledImages) const;
    
    void setTilesSupported(bool tilesSupported);
    bool isTilesSupportEnabled() const;
    
    void setRenderScaleSupported(bool rsSupported);
    bool isRenderScaleSupportEnabled() const;
    
    void setChannelsRendered(std::bitset<4> channelsRendered);
    std::bitset<4> getChannelsRendered() const;
    
    void setOutputPremult(ImagePremultiplicationEnum premult);
    ImagePremultiplicationEnum getOutputPremult() const;
    
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
    
    void setNodeIdentity(const boost::shared_ptr<Node>& node, const boost::shared_ptr<Node>& identity);
    
    void setGlobalRenderInfosForNode(const boost::shared_ptr<Node>& node,
                                     const RectD& rod,
                                     ImagePremultiplicationEnum outputPremult,
                                     std::bitset<4> channelsRendered,
                                     bool tilesSupported,
                                     bool renderScaleSupported,
                                     unsigned int mipmapLevel);
    
    void addCacheInfosForNode(const boost::shared_ptr<Node>& node,
                              bool isCacheMiss,
                              bool hasDownscaled);
    
    void addRenderInfosForNode(const boost::shared_ptr<Node>& node,
                        const boost::shared_ptr<Node>& identity,
                        const std::string& plane,
                        const RectI& rectangle,
                        double timeSpent);
    
    std::map<boost::shared_ptr<Node>,NodeRenderStats > getStats(double *totalTimeSpent) const;
    
private:
    
    boost::scoped_ptr<RenderStatsPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;


#endif // RENDERSTATS_H
