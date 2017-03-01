/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_IMAGETILESSTATE_H
#define NATRON_ENGINE_IMAGETILESSTATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/EngineFwd.h"
#include "Engine/RectI.h"

#include "IPCCommon.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)



#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

NATRON_NAMESPACE_ENTER;

/**
 * @brief The coordinates in pixel of the bottom left corner of a tile.
 **/
struct TileCoord
{
    int tx,ty;
};

struct TileCoord_Compare
{
    bool operator() (const TileCoord& lhs, const TileCoord& rhs) const
    {
        if (lhs.ty < rhs.ty) {
            return true;
        } else if (lhs.ty > rhs.ty) {
            return false;
        } else {
            return lhs.tx < rhs.tx;
        }
    }
};

enum TileStatusEnum
{
    eTileStatusRendered,
    eTileStatusNotRendered,
    eTileStatusPending
};

/**
 * @brief The state of a tile is shared accross all channels since OpenFX does not currently allows to render a single channel
 **/
struct TileState
{
    RectI bounds;
    TileStatusEnum status;
    int channelsTileStorageIndex[4];

    TileState()
    : bounds()
    , status(eTileStatusNotRendered)
    , channelsTileStorageIndex()
    {
        channelsTileStorageIndex[0] = channelsTileStorageIndex[1] = channelsTileStorageIndex[2] = channelsTileStorageIndex[3] = -1;
    }
};


// Tiles are orderedby y axis then by x such that the first tile in the map
// has its bottom left corner being the bottom left corner of the image and
// the last tile in the map has its top right corner being the top right corner
// of the image.
typedef std::pair<const TileCoord, TileState > TileStateValueType;
typedef boost::interprocess::allocator<TileStateValueType, ExternalSegmentType::segment_manager> TileStateValueType_Allocator_ExternalSegment;
typedef boost::interprocess::map<TileCoord, TileState, TileCoord_Compare, TileStateValueType_Allocator_ExternalSegment> TileStateMap;
typedef boost::interprocess::allocator<TileStateMap, ExternalSegmentType::segment_manager> TileStateMap_Allocator_ExternalSegment;
typedef boost::interprocess::vector<TileStateMap, TileStateMap_Allocator_ExternalSegment> TileStateMapVector;

struct ImageTilesStatePrivate;

/**
 * @brief Stores the state of the tiles of an image. This class is not thread safe.
 **/
class ImageTilesState
{
public:


    ImageTilesState(const RectI& originalBounds, int tileSizeX, int tileSizeY);

    ~ImageTilesState();

    TileStateMap& getTilesMap();

    const TileStateMap& getTilesMap() const;

    void setTilesMap(const TileStateMap& tilesMap);

    const RectI& getOriginalBounds() const;

    const RectI& getBoundsRoundedToTileSize() const;

    void getTileSize(int* tileSizeX, int* tileSizeY) const;

    /**
     * @brief Returns the bounding box of the unrendered portion in the tiles map.
     * N.B: Tiles with a status of eTileStatusPending are treated as if they were
     * eTileStatusRendered.
     **/
    RectI getMinimalBboxToRenderFromTilesState(const RectI& roi, int tileSizeX, int tileSizeY);

    /**
     * @brief Refines a region to render in potentially 4 smaller rectangles. This function makes use of
     * getMinimalBboxToRenderFromTilesState to get the smallest enclosing bbox to render.
     * Then it tries to find rectangles for the bottom, the top,
     * the left and the right part.
     * This happens quite often, for example when zooming out
     * (in this case the area to compute is formed of A, B, C and D,
     * and X is already rendered), or when panning (in this case the area
     * is just two rectangles, e.g. A and C, and the rectangles B, D and
     * X are already rendered).
     * The rectangles A, B, C and D from the following drawing are just
     * zeroes, and X contains zeroes and ones.
     *
     * BBBBBBBBBBBBBB
     * BBBBBBBBBBBBBB
     * CXXXXXXXXXXDDD
     * CXXXXXXXXXXDDD
     * CXXXXXXXXXXDDD
     * CXXXXXXXXXXDDD
     * AAAAAAAAAAAAAA
     **/
    void getMinimalRectsToRenderFromTilesState(const RectI& roi, int tileSizeX, int tileSizeY, std::list<RectI>* rectsToRender);

    /*
     Compute the rectangles (A,B,C,D) where to set the image to 0

     AAAAAAAAAAAAAAAAAAAAAAAAAAAA
     AAAAAAAAAAAAAAAAAAAAAAAAAAAA
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     DDDDDXXXXXXXXXXXXXXXXXXBBBBB
     CCCCCCCCCCCCCCCCCCCCCCCCCCCC
     CCCCCCCCCCCCCCCCCCCCCCCCCCCC
     */
    static void getABCDRectangles(const RectI& srcBounds, const RectI& biggerBounds, RectI& aRect, RectI& bRect, RectI& cRect, RectI& dRect);
    
private:

    boost::scoped_ptr<ImageTilesStatePrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGETILESSTATE_H
