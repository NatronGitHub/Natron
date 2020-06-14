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
#include <boost/uuid/uuid.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)



#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

NATRON_NAMESPACE_ENTER


struct TileInternalIndexImpl
{

    // The memory file index onto which the tile is mapped. We don't expect the Cache to create more than 2^16 files
    // of storage: each file is of size NATRON_TILE_STORAGE_FILE_SIZE.
    U16 fileIndex;

    // The index of the tile in the file: Since there are 256 buckets and each bucket has 256 tiles per file, the
    // number of adressable tiles per bucket per file is 256
#ifndef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
    U8 tileIndex;
#else
    U16 tileIndex;
#endif

};

/**
 * @brief The index of a tile in the Cache. This is a 64 value encoding 2 32bit values
 * of the file index containing the tile and the index of the tile in that file.
 * @see getTileIndex function in the cpp
 **/
struct TileInternalIndex
{
#ifndef NATRON_CACHE_TILES_MEMORY_ALLOCATOR_CENTRALIZED
    // The bucket index in which the tile belongs to. This is a byte since there are 256 buckets.
    U8 bucketIndex;
#endif

    TileInternalIndexImpl index;

};

/**
 * @brief The hash of a tile: this is the identifier of a tile in the Cache and what is passed
 * in the retrieveAndLockTiles function. The hash is produced with the function CacheBase::makeTileCacheIndex
 **/
struct TileHash
{
    U64 index;
};


enum TileStatusEnum
{
    // The tile is rendered with the highest quality possible
    eTileStatusRenderedHighestQuality,

    // The tile is rendered but in a low quality version
    eTileStatusRenderedLowQuality,

    // The tile is not rendered
    eTileStatusNotRendered,

    // The tile is being rendered by a thread
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
    TileInternalIndex channelsTileStorageIndex[4];

    // If the tile status is eTileStatusPending, this is the uuid of the process that is in charge of computing
    // the tile. This can be used to detect tile abandonnement.
    boost::uuids::uuid uuid;

    TileState()
    : bounds()
    , status(eTileStatusNotRendered)
    , channelsTileStorageIndex()
    , uuid()
    {
#if 0
        TileInternalIndex i;
        i.index = (U64)-1;
        channelsTileStorageIndex[0] = channelsTileStorageIndex[1] = channelsTileStorageIndex[2] = channelsTileStorageIndex[3] = i;
#endif
    }

    TileState(const TileState& other)
    {
        *this = other;
    }

    void operator=(const TileState& other)
    {
        bounds = other.bounds;
        status = other.status;
        uuid = other.uuid;
        memcpy(channelsTileStorageIndex, other.channelsTileStorageIndex, sizeof(U64) * 4);
    }
};


// Tiles are orderedby y axis then by x such that the first tile in the vector
// has its bottom left corner being the bottom left corner of the image and
// the last tile in the vector has its top right corner being the top right corner
// of the image.
typedef std::vector<TileState> TileStateVector;

/**
 * @brief A tiles vector + 2 members to speed lookup of the full vector
 **/
struct TilesState
{
    TileStateVector tiles;

    // The area covered by the tiles
    // The next tile is at tileSizeX/tileSizeY
    // There are boundsRoundedToTileSize / tileSizeX tiles per line
    RectI bounds;
    RectI boundsRoundedToTileSize;

    TilesState()
    : tiles()
    , bounds()
    , boundsRoundedToTileSize()
    {

    }
    
};

class TileStateHeader
{
public:
       int tileSizeX, tileSizeY;
    TilesState *state;
    bool ownsState;


    // Do not fills the map
    TileStateHeader();

    // Init from an external vector
    TileStateHeader(int tileSizeX, int tileSizeY,  TilesState* tiles);

    ~TileStateHeader();

    // fills the map with unrendered tiles
    void init(int tileSizeX, int tileSizeY, const RectI& bounds);

    // Get a tile
    TileState* getTileAt(int tx, int ty);
    const TileState* getTileAt(int tx, int ty) const;

#ifdef DEBUG
    void printStateMap();
#endif
};


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
    static RectI getMinimalBboxToRenderFromTilesState(const RectI& roi, const TileStateHeader& stateMap);

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
    static void getMinimalRectsToRenderFromTilesState(const RectI& roi, const TileStateHeader& stateMap, std::list<RectI>* rectsToRender);

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

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_IMAGETILESSTATE_H
