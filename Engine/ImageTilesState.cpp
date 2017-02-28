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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "ImageTilesState.h"


NATRON_NAMESPACE_ENTER;


struct ImageTilesStatePrivate
{
    TileStateMap stateMap;
    RectI originalBounds, boundsRoundedToTileSize;
    int tileSizeX;
    int tileSizeY;
    
    ImageTilesStatePrivate(const RectI& originalBounds, int tileSizeX, int tileSizeY)
    : stateMap()
    , originalBounds(originalBounds)
    , boundsRoundedToTileSize(originalBounds)
    , tileSizeX(tileSizeX)
    , tileSizeY(tileSizeY)
    {
        boundsRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);

        for (int ty = boundsRoundedToTileSize.y1; ty < boundsRoundedToTileSize.y2; ty += tileSizeY) {
            for (int tx = boundsRoundedToTileSize.x1; tx < boundsRoundedToTileSize.x2; tx += tileSizeX) {
                assert(tx % tileSizeX == 0 && ty % tileSizeY == 0);

                TileCoord c = {tx, ty};
                TileState s;
                s.bounds.x1 = std::max(c.tx, originalBounds.x1);
                s.bounds.y1 = std::max(c.ty, originalBounds.y1);
                s.bounds.x2 = std::min(c.tx + tileSizeX, originalBounds.x2);
                s.bounds.y2 = std::min(c.ty + tileSizeY, originalBounds.y2);
                s.status = eTileStatusNotRendered;
                s.tiledStorageIndex = -1;

                stateMap.insert(std::make_pair(c, s));
            }
        }
    }
};

ImageTilesState::ImageTilesState(const RectI& originalBounds, int tileSizeX, int tileSizeY)
: _imp(new ImageTilesStatePrivate(originalBounds, tileSizeX, tileSizeY))
{

}

ImageTilesState::~ImageTilesState()
{

}

const TileStateMap&
ImageTilesState::getTilesMap() const
{
    return _imp->stateMap;
}

void
ImageTilesState::setTilesMap(const TileStateMap& tilesMap)
{
    assert(tilesMap.size() == _imp->stateMap.size());
    _imp->stateMap = tilesMap;
}

const RectI&
ImageTilesState::getOriginalBounds() const
{
    return _imp->originalBounds;
}

const RectI&
ImageTilesState::getBoundsRoundedToTileSize() const
{
    return _imp->boundsRoundedToTileSize;
}

void
ImageTilesState::getTileSize(int* tileSizeX, int* tileSizeY) const
{
    *tileSizeX = _imp->tileSizeX;
    *tileSizeY = _imp->tileSizeY;
}

void
ImageTilesState::getABCDRectangles(const RectI& srcBounds, const RectI& biggerBounds, RectI& aRect, RectI& bRect, RectI& cRect, RectI& dRect)
{
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
    aRect.x1 = biggerBounds.x1;
    aRect.y1 = srcBounds.y2;
    aRect.y2 = biggerBounds.y2;
    aRect.x2 = biggerBounds.x2;

    bRect.x1 = srcBounds.x2;
    bRect.y1 = srcBounds.y1;
    bRect.x2 = biggerBounds.x2;
    bRect.y2 = srcBounds.y2;

    cRect.x1 = biggerBounds.x1;
    cRect.y1 = biggerBounds.y1;
    cRect.x2 = biggerBounds.x2;
    cRect.y2 = srcBounds.y1;

    dRect.x1 = biggerBounds.x1;
    dRect.y1 = srcBounds.y1;
    dRect.x2 = srcBounds.x1;
    dRect.y2 = srcBounds.y2;

} // getABCDRectangles

static void getImageBoundsFromTilesState(const TileStateMap& tiles, int tileSizeX, int tileSizeY,
                                         RectI& imageBoundsRoundedToTileSize,
                                         RectI& imageBoundsNotRounded)
{
    {
        TileStateMap::const_iterator bottomLeft = tiles.begin();
        TileStateMap::const_reverse_iterator topRight = tiles.rbegin();
        imageBoundsRoundedToTileSize.x1 = bottomLeft->first.tx;
        imageBoundsRoundedToTileSize.y1 = bottomLeft->first.ty;
        imageBoundsRoundedToTileSize.x2 = topRight->first.tx + tileSizeX;
        imageBoundsRoundedToTileSize.y2 = topRight->first.ty + tileSizeY;

        imageBoundsNotRounded.x1 = bottomLeft->second.bounds.x1;
        imageBoundsNotRounded.y1 = bottomLeft->second.bounds.y1;
        imageBoundsNotRounded.x2 = topRight->second.bounds.x2;
        imageBoundsNotRounded.y2 = topRight->second.bounds.y2;
    }

    assert(imageBoundsRoundedToTileSize.x1 % tileSizeX == 0);
    assert(imageBoundsRoundedToTileSize.y1 % tileSizeY == 0);
    assert(imageBoundsRoundedToTileSize.x2 % tileSizeX == 0);
    assert(imageBoundsRoundedToTileSize.y2 % tileSizeY == 0);
}


RectI
ImageTilesState::getMinimalBboxToRenderFromTilesState(const RectI& roi, int tileSizeX, int tileSizeY)
{

    if (_imp->stateMap.empty()) {
        return RectI();
    }

    RectI imageBoundsRoundedToTileSize;
    RectI imageBoundsNotRounded;
    getImageBoundsFromTilesState(_imp->stateMap, tileSizeX, tileSizeY, imageBoundsRoundedToTileSize, imageBoundsNotRounded);
    assert(imageBoundsRoundedToTileSize.contains(roi));

    RectI roiRoundedToTileSize = roi;
    roiRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);

    // Search for rendered lines from bottom to top
    for (int y = roiRoundedToTileSize.y1; y < roiRoundedToTileSize.y2; y += tileSizeY) {

        bool hasTileUnrenderedOnLine = false;
        for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += tileSizeX) {

            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = _imp->stateMap.find(c);
            assert(foundTile != _imp->stateMap.end());
            if (foundTile->second.status == eTileStatusNotRendered) {
                hasTileUnrenderedOnLine = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnLine) {
            roiRoundedToTileSize.y1 += tileSizeY;
        }
    }

    // Search for rendered lines from top to bottom
    for (int y = roiRoundedToTileSize.y2 - tileSizeY; y >= roiRoundedToTileSize.y1; y -= tileSizeY) {

        bool hasTileUnrenderedOnLine = false;
        for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += tileSizeX) {

            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = _imp->stateMap.find(c);
            assert(foundTile != _imp->stateMap.end());
            if (foundTile->second.status == eTileStatusNotRendered) {
                hasTileUnrenderedOnLine = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnLine) {
            roiRoundedToTileSize.y2 -= tileSizeY;
        }
    }

    // Avoid making roiRoundedToTileSize.width() iterations for nothing
    if (roiRoundedToTileSize.isNull()) {
        return roiRoundedToTileSize;
    }


    // Search for rendered columns from left to right
    for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += tileSizeX) {

        bool hasTileUnrenderedOnCol = false;
        for (int y = roiRoundedToTileSize.y1; y < roiRoundedToTileSize.y2; y += tileSizeY) {

            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = _imp->stateMap.find(c);
            assert(foundTile != _imp->stateMap.end());
            if (foundTile->second.status == eTileStatusNotRendered) {
                hasTileUnrenderedOnCol = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnCol) {
            roiRoundedToTileSize.x1 += tileSizeX;
        }
    }

    // Avoid making roiRoundedToTileSize.width() iterations for nothing
    if (roiRoundedToTileSize.isNull()) {
        return roiRoundedToTileSize;
    }

    // Search for rendered columns from right to left
    for (int x = roiRoundedToTileSize.x2 - tileSizeX; x >= roiRoundedToTileSize.x1; x -= tileSizeX) {

        bool hasTileUnrenderedOnCol = false;
        for (int y = roiRoundedToTileSize.y1; y < roiRoundedToTileSize.y2; y += tileSizeY) {

            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = _imp->stateMap.find(c);
            assert(foundTile != _imp->stateMap.end());
            if (foundTile->second.status == eTileStatusNotRendered) {
                hasTileUnrenderedOnCol = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnCol) {
            roiRoundedToTileSize.x2 -= tileSizeX;
        }
    }

    // Intersect the result to the actual image bounds (because the tiles are rounded to tile size)
    RectI ret;
    roiRoundedToTileSize.intersect(imageBoundsNotRounded, &ret);
    return ret;


} // getMinimalBboxToRenderFromTilesState

void
ImageTilesState::getMinimalRectsToRenderFromTilesState(const RectI& roi, int tileSizeX, int tileSizeY, std::list<RectI>* rectsToRender)
{
    if (_imp->stateMap.empty()) {
        return;
    }

    RectI roiRoundedToTileSize = roi;
    roiRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);

    RectI bboxM = getMinimalBboxToRenderFromTilesState(roi, tileSizeX, tileSizeY);
    if (bboxM.isNull()) {
        return;
    }

    bboxM.roundToTileSize(tileSizeX, tileSizeY);

    // optimization by Fred, Jan 31, 2014
    //
    // Now that we have the smallest enclosing bounding box,
    // let's try to find rectangles for the bottom, the top,
    // the left and the right part.
    // This happens quite often, for example when zooming out
    // (in this case the area to compute is formed of A, B, C and D,
    // and X is already rendered), or when panning (in this case the area
    // is just two rectangles, e.g. A and C, and the rectangles B, D and
    // X are already rendered).
    // The rectangles A, B, C and D from the following drawing are just
    // zeroes, and X contains zeroes and ones.
    //
    // BBBBBBBBBBBBBB
    // BBBBBBBBBBBBBB
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // CXXXXXXXXXXDDD
    // AAAAAAAAAAAAAA

    // First, find if there's an "A" rectangle, and push it to the result
    //find bottom
    RectI bboxX = bboxM;
    RectI bboxA = bboxX;
    bboxA.y2 = bboxA.y1;
    for (int y = bboxX.y1; y < bboxX.y2; y += tileSizeY) {
        bool hasRenderedTileOnLine = false;
        for (int x = bboxX.x1; x < bboxX.x2; x += tileSizeX) {
            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = _imp->stateMap.find(c);
            assert(foundTile != _imp->stateMap.end());
            if (foundTile->second.status != eTileStatusNotRendered) {
                hasRenderedTileOnLine = true;
                break;
            }
        }
        if (hasRenderedTileOnLine) {
            break;
        } else {
            bboxX.y1 += tileSizeY;
            bboxA.y2 = bboxX.y1;
        }
    }
    if ( !bboxA.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the RoI since we rounded to tile size earlier
        RectI bboxAIntersected;
        bboxA.intersect(roi, &bboxAIntersected);
        rectsToRender->push_back(bboxAIntersected);
    }

    // Now, find the "B" rectangle
    //find top
    RectI bboxB = bboxX;
    bboxB.y1 = bboxB.y2;

    for (int y = bboxX.y2 - tileSizeY; y >= bboxX.y1; y -= tileSizeY) {
        bool hasRenderedTileOnLine = false;
        for (int x = bboxX.x1; x < bboxX.x2; x += tileSizeX) {
            TileCoord c = {x, y};
            TileStateMap::const_iterator foundTile = _imp->stateMap.find(c);
            assert(foundTile != _imp->stateMap.end());
            if (foundTile->second.status != eTileStatusNotRendered) {
                hasRenderedTileOnLine = true;
                break;
            }
        }
        if (hasRenderedTileOnLine) {
            break;
        } else {
            bboxX.y2 -= tileSizeY;
            bboxB.y1 = bboxX.y2;
        }
    }

    if ( !bboxB.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the RoI since we rounded to tile size earlier
        RectI bboxBIntersected;
        bboxB.intersect(roi, &bboxBIntersected);
        rectsToRender->push_back(bboxBIntersected);
    }

    //find left
    RectI bboxC = bboxX;
    bboxC.x2 = bboxC.x1;
    if ( bboxX.y1 < bboxX.y2 ) {
        for (int x = bboxX.x1; x < bboxX.x2; x += tileSizeX) {
            bool hasRenderedTileOnCol = false;
            for (int y = bboxX.y1; y < bboxX.y2; y += tileSizeY) {
                TileCoord c = {x, y};
                TileStateMap::const_iterator foundTile = _imp->stateMap.find(c);
                assert(foundTile != _imp->stateMap.end());
                if (foundTile->second.status != eTileStatusNotRendered) {
                    hasRenderedTileOnCol = true;
                    break;
                }

            }
            if (hasRenderedTileOnCol) {
                break;
            } else {
                bboxX.x1 += tileSizeX;
                bboxC.x2 = bboxX.x1;
            }
        }
    }
    if ( !bboxC.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the RoI since we rounded to tile size earlier
        RectI bboxCIntersected;
        bboxC.intersect(roi, &bboxCIntersected);
        rectsToRender->push_back(bboxCIntersected);
    }

    //find right
    RectI bboxD = bboxX;
    bboxD.x1 = bboxD.x2;
    if ( bboxX.y1 < bboxX.y2 ) {
        for (int x = bboxX.x2 - tileSizeX; x >= bboxX.x1; x -= tileSizeX) {
            bool hasRenderedTileOnCol = false;
            for (int y = bboxX.y1; y < bboxX.y2; y += tileSizeY) {
                TileCoord c = {x, y};
                TileStateMap::const_iterator foundTile = _imp->stateMap.find(c);
                assert(foundTile != _imp->stateMap.end());
                if (foundTile->second.status != eTileStatusNotRendered) {
                    hasRenderedTileOnCol = true;
                    break;
                }
            }
            if (hasRenderedTileOnCol) {
                break;
            } else {
                bboxX.x2 -= tileSizeX;
                bboxD.x1 = bboxX.x2;
            }
        }
    }
    if ( !bboxD.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the RoI since we rounded to tile size earlier
        RectI bboxDIntersected;
        bboxD.intersect(roi, &bboxDIntersected);
        rectsToRender->push_back(bboxDIntersected);
    }

    assert( bboxA.bottom() == bboxM.bottom() );
    assert( bboxA.left() == bboxM.left() );
    assert( bboxA.right() == bboxM.right() );
    assert( bboxA.top() == bboxX.bottom() );

    assert( bboxB.top() == bboxM.top() );
    assert( bboxB.left() == bboxM.left() );
    assert( bboxB.right() == bboxM.right() );
    assert( bboxB.bottom() == bboxX.top() );

    assert( bboxC.top() == bboxX.top() );
    assert( bboxC.left() == bboxM.left() );
    assert( bboxC.right() == bboxX.left() );
    assert( bboxC.bottom() == bboxX.bottom() );

    assert( bboxD.top() == bboxX.top() );
    assert( bboxD.left() == bboxX.right() );
    assert( bboxD.right() == bboxM.right() );
    assert( bboxD.bottom() == bboxX.bottom() );

    // get the bounding box of what's left (the X rectangle in the drawing above)
    bboxX = getMinimalBboxToRenderFromTilesState(bboxX, tileSizeX, tileSizeY);

    if ( !bboxX.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the RoI since we rounded to tile size earlier
        RectI bboxXIntersected;
        bboxX.intersect(roi, &bboxXIntersected);
        rectsToRender->push_back(bboxXIntersected);
    }
    
} // getMinimalRectsToRenderFromTilesState


NATRON_NAMESPACE_EXIT;

