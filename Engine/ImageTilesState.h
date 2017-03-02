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
    // The bounds of the image covered by this tile: this is not necessarily a full tile on the border
    RectI bounds;

    // Status of the tile (3 state)
    TileStatusEnum status;

    // The indices of the buffers in the cache
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
typedef boost::interprocess::allocator<TileState, ExternalSegmentType::segment_manager> TileState_Allocator_ExternalSegment;
typedef boost::interprocess::vector<TileState, TileState_Allocator_ExternalSegment> TileStateVector;

class TileStateMap
{
public:
    // The area covered by the tiles
    // The next tile is at tileSizeX/tileSizeY
    // There are boundsRoundedToTileSize / tileSizeX tiles per line
    int tileSizeX, tileSizeY;
    RectI bounds, boundsRoundedToTileSize;
    TileStateVector *tiles;


    // Do not fills the map
    TileStateMap();

    // Init from an external vector
    TileStateMap(int tileSizeX, int tileSizeY, const RectI& bounds, TileStateVector* tiles);

    ~TileStateMap();

    // fills the map with unrendered tiles
    void init(int tileSizeX, int tileSizeY, const RectI& bounds);

    // Get a tile
    TileState* getTileAt(int tx, int ty);
    const TileState* getTileAt(int tx, int ty) const;
};

typedef boost::interprocess::allocator<TileStateVector, ExternalSegmentType::segment_manager> TileStateVector_Allocator_ExternalSegment;
typedef boost::interprocess::vector<TileStateVector, TileStateVector_Allocator_ExternalSegment> PerMipMapTileStateVector;

struct ImageTilesStatePrivate;

/**
 * @brief Stores the state of the tiles of an image. This class is not thread safe.
 **/
class ImageTilesState
{
public:
    /**
     * @brief Returns the bounding box of the unrendered portion in the tiles map.
     * N.B: Tiles with a status of eTileStatusPending are treated as if they were
     * eTileStatusRendered.
     **/
    static RectI getMinimalBboxToRenderFromTilesState(const RectI& roi, const TileStateMap& stateMap);

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
    static void getMinimalRectsToRenderFromTilesState(const RectI& roi, const TileStateMap& stateMap, std::list<RectI>* rectsToRender);

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

};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_IMAGETILESSTATE_H
