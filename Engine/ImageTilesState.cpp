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


#include "ImageTilesState.h"
#include <QDebug>
#include <QThread>
#include <iostream>

NATRON_NAMESPACE_ENTER



TileStateHeader::TileStateHeader()
: tileSizeX(0)
, tileSizeY(0)
, state(0)
, ownsState(false)
{

}

TileStateHeader::TileStateHeader(int tileSizeX, int tileSizeY, TilesState* state)
: tileSizeX(tileSizeX)
, tileSizeY(tileSizeY)
, state(state)
, ownsState(false)
{
    assert(state->tiles.empty() || ((int)state->tiles.size() == ((state->boundsRoundedToTileSize.width() / tileSizeX) * (state->boundsRoundedToTileSize.height() / tileSizeY))));
}

TileStateHeader::~TileStateHeader()
{
    if (ownsState) {
        delete state;
    }
}

void
TileStateHeader::init(int tileSizeXParam, int tileSizeYParam, const RectI& roi)
{
    tileSizeX = tileSizeXParam;
    tileSizeY = tileSizeYParam;


    if (state && ownsState) {
        delete state;
    }
    ownsState = true;

    state = new TilesState;
    state->bounds = roi;
    state->boundsRoundedToTileSize = roi;
    state->boundsRoundedToTileSize.roundToTileSize(tileSizeX, tileSizeY);

    state->tiles.resize((state->boundsRoundedToTileSize.width() / tileSizeX) * (state->boundsRoundedToTileSize.height() / tileSizeY));

    int tile_i = 0;
    for (int ty = state->boundsRoundedToTileSize.y1; ty < state->boundsRoundedToTileSize.y2; ty += tileSizeY) {
        for (int tx = state->boundsRoundedToTileSize.x1; tx < state->boundsRoundedToTileSize.x2; tx += tileSizeX, ++tile_i) {

            assert(tile_i < (int)state->tiles.size());

            state->tiles[tile_i].bounds.x1 = std::max(tx, state->bounds.x1);
            state->tiles[tile_i].bounds.y1 = std::max(ty, state->bounds.y1);
            state->tiles[tile_i].bounds.x2 = std::min(tx + tileSizeX, state->bounds.x2);
            state->tiles[tile_i].bounds.y2 = std::min(ty + tileSizeY, state->bounds.y2);
            
        }
    }

}

TileState*
TileStateHeader::getTileAt(int tx, int ty)
{
    assert(tx % tileSizeX == 0 && ty % tileSizeY == 0);
    if (ty < state->boundsRoundedToTileSize.y1 || ty > state->boundsRoundedToTileSize.y2 - tileSizeY || tx < state->boundsRoundedToTileSize.x1 || tx > state->boundsRoundedToTileSize.x2 - tileSizeX) {
        return NULL;
    }
    int index = (((ty - state->boundsRoundedToTileSize.y1) / tileSizeY) * (state->boundsRoundedToTileSize.width() / tileSizeX)) + (tx - state->boundsRoundedToTileSize.x1) / tileSizeX;
    if (index >= 0 && index < (int)state->tiles.size()) {
        return &state->tiles[index];
    } else {
        return NULL;
    }
}

const TileState*
TileStateHeader::getTileAt(int tx, int ty) const
{
    assert(tx % tileSizeX == 0 && ty % tileSizeY == 0);
    int index = (((ty - state->boundsRoundedToTileSize.y1) / tileSizeY) * (state->boundsRoundedToTileSize.width() / tileSizeX)) + (tx - state->boundsRoundedToTileSize.x1) / tileSizeX;
    if (ty < state->boundsRoundedToTileSize.y1 || ty > state->boundsRoundedToTileSize.y2 - tileSizeY || tx < state->boundsRoundedToTileSize.x1 || tx > state->boundsRoundedToTileSize.x2 - tileSizeX) {
        return NULL;
    }
    if (index >= 0 && index < (int)state->tiles.size()) {
        return &state->tiles[index];
    } else {
        return NULL;
    }
}

#ifdef DEBUG
void
TileStateHeader::printStateMap()
{
    std::cout << QThread::currentThread()->objectName().toStdString() << "=================DUMP=================" << std::endl;
    std::cout << "Bounds:" << state->bounds.x1 << " " << state->bounds.y1 << " " << state->bounds.x2 << " " << state->bounds.y2 << std::endl;
    std::cout << "BoundsRounded:" << state->boundsRoundedToTileSize.x1 << " "<< state->boundsRoundedToTileSize.y1 << " "<< state->boundsRoundedToTileSize.x2 << " " << state->boundsRoundedToTileSize.y2 << std::endl;
    for (int ty = state->boundsRoundedToTileSize.y1; ty < state->boundsRoundedToTileSize.y2; ty += tileSizeY) {
        for (int tx = state->boundsRoundedToTileSize.x1; tx < state->boundsRoundedToTileSize.x2; tx += tileSizeX) {
            TileState* tile = getTileAt(tx, ty);
            assert(tile);
            switch (tile->status) {
                case eTileStatusNotRendered:
                    std::cout << "0";
                    break;
                case eTileStatusPending:
                    std::cout << "P";
                    break;
                case eTileStatusRenderedLowQuality:
                case eTileStatusRenderedHighestQuality:
                    std::cout << "1";
                    break;
            }
            std::cout << ' ';
        }
        if (ty < state->boundsRoundedToTileSize.y2 - 1) {
            std::cout << std::endl;
        }
    }
}
#endif

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

RectI
ImageTilesState::getMinimalBboxToRenderFromTilesState(const RectI& roi, const TileStateHeader& stateMap)
{

    if (stateMap.state->tiles.empty()) {
        return RectI();
    }

    const RectI& imageBoundsRoundedToTileSize = stateMap.state->boundsRoundedToTileSize;

    assert(imageBoundsRoundedToTileSize.contains(roi));

    // The roi must be rounded to the tile size and clipped the pixel RoD
    assert(roi.x1 % stateMap.tileSizeX == 0 || roi.x1 == stateMap.state->bounds.x1);
    assert(roi.y1 % stateMap.tileSizeY == 0 || roi.y1 == stateMap.state->bounds.y1);
    assert(roi.x2 % stateMap.tileSizeX == 0 || roi.x2 == stateMap.state->bounds.x2);
    assert(roi.y2 % stateMap.tileSizeY == 0 || roi.y2 == stateMap.state->bounds.y2);

    RectI roiRoundedToTileSize = roi;
    roiRoundedToTileSize.roundToTileSize(stateMap.tileSizeX, stateMap.tileSizeY);

    // Search for rendered lines from bottom to top
    for (int y = roiRoundedToTileSize.y1; y < roiRoundedToTileSize.y2; y += stateMap.tileSizeY) {

        bool hasTileUnrenderedOnLine = false;
        for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += stateMap.tileSizeX) {
            assert(x % stateMap.tileSizeX == 0 && y % stateMap.tileSizeY == 0);
            const TileState* tile = stateMap.getTileAt(x, y);
            if (tile->status == eTileStatusNotRendered) {
                hasTileUnrenderedOnLine = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnLine) {
            roiRoundedToTileSize.y1 += stateMap.tileSizeY;
        } else {
            break;
        }
    }

    // Search for rendered lines from top to bottom
    for (int y = roiRoundedToTileSize.y2 - stateMap.tileSizeY; y >= roiRoundedToTileSize.y1; y -= stateMap.tileSizeY) {

        bool hasTileUnrenderedOnLine = false;
        for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += stateMap.tileSizeX) {
            assert(x % stateMap.tileSizeX == 0 && y % stateMap.tileSizeY == 0);
            const TileState* tile = stateMap.getTileAt(x, y);
            if (tile->status == eTileStatusNotRendered) {
                hasTileUnrenderedOnLine = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnLine) {
            roiRoundedToTileSize.y2 -= stateMap.tileSizeY;
        } else {
            break;
        }
    }

    // Avoid making roiRoundedToTileSize.width() iterations for nothing
    if (roiRoundedToTileSize.isNull()) {
        return roiRoundedToTileSize;
    }


    // Search for rendered columns from left to right
    for (int x = roiRoundedToTileSize.x1; x < roiRoundedToTileSize.x2; x += stateMap.tileSizeX) {

        bool hasTileUnrenderedOnCol = false;
        for (int y = roiRoundedToTileSize.y1; y < roiRoundedToTileSize.y2; y += stateMap.tileSizeY) {
            assert(x % stateMap.tileSizeX == 0 && y % stateMap.tileSizeY == 0);
            const TileState* tile = stateMap.getTileAt(x, y);
            if (tile->status == eTileStatusNotRendered) {
                hasTileUnrenderedOnCol = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnCol) {
            roiRoundedToTileSize.x1 += stateMap.tileSizeX;
        } else {
            break;
        }
    }

    // Avoid making roiRoundedToTileSize.width() iterations for nothing
    if (roiRoundedToTileSize.isNull()) {
        return roiRoundedToTileSize;
    }

    // Search for rendered columns from right to left
    for (int x = roiRoundedToTileSize.x2 - stateMap.tileSizeX; x >= roiRoundedToTileSize.x1; x -= stateMap.tileSizeX) {

        bool hasTileUnrenderedOnCol = false;
        for (int y = roiRoundedToTileSize.y1; y < roiRoundedToTileSize.y2; y += stateMap.tileSizeY) {
            assert(x % stateMap.tileSizeX == 0 && y % stateMap.tileSizeY == 0);
            const TileState* tile = stateMap.getTileAt(x, y);
            if (tile->status == eTileStatusNotRendered) {
                hasTileUnrenderedOnCol = true;
                break;
            }
        }
        if (!hasTileUnrenderedOnCol) {
            roiRoundedToTileSize.x2 -= stateMap.tileSizeX;
        } else {
            break;
        }
    }

    // Intersect the result to the bounds (because the tiles are rounded to tile size)
    RectI ret;
    roiRoundedToTileSize.intersect(stateMap.state->bounds, &ret);
    return ret;


} // getMinimalBboxToRenderFromTilesState

void
ImageTilesState::getMinimalRectsToRenderFromTilesState(const RectI& roi, const TileStateHeader& stateMap, std::list<RectI>* rectsToRender)
{
    if (stateMap.state->tiles.empty()) {
        return;
    }

    // The roi must be rounded to the tile size and clipped the pixel RoD
    assert(roi.x1 % stateMap.tileSizeX == 0 || roi.x1 == stateMap.state->bounds.x1);
    assert(roi.y1 % stateMap.tileSizeY == 0 || roi.y1 == stateMap.state->bounds.y1);
    assert(roi.x2 % stateMap.tileSizeX == 0 || roi.x2 == stateMap.state->bounds.x2);
    assert(roi.y2 % stateMap.tileSizeY == 0 || roi.y2 == stateMap.state->bounds.y2);

    RectI bboxM = getMinimalBboxToRenderFromTilesState(roi, stateMap);
    if (bboxM.isNull()) {
        return;
    }

    // Round to the tile size because the tile states is only valid for tiles
    bboxM.roundToTileSize(stateMap.tileSizeX, stateMap.tileSizeY);

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
    for (int y = bboxX.y1; y < bboxX.y2; y += stateMap.tileSizeY) {
        bool hasRenderedTileOnLine = false;
        for (int x = bboxX.x1; x < bboxX.x2; x += stateMap.tileSizeX) {
            assert(x % stateMap.tileSizeX == 0 && y % stateMap.tileSizeY == 0);
            const TileState* tile = stateMap.getTileAt(x, y);
            if (tile->status != eTileStatusNotRendered) {
                hasRenderedTileOnLine = true;
                break;
            }
        }
        if (hasRenderedTileOnLine) {
            break;
        } else {
            bboxX.y1 += stateMap.tileSizeY;
            bboxA.y2 = bboxX.y1;
        }
    }
    if ( !bboxA.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the bounds since we rounded to tile size earlier
        RectI bboxAIntersected;
        bboxA.intersect(stateMap.state->bounds, &bboxAIntersected);
        rectsToRender->push_back(bboxAIntersected);
    }

    // Now, find the "B" rectangle
    //find top
    RectI bboxB = bboxX;
    bboxB.y1 = bboxB.y2;

    for (int y = bboxX.y2 - stateMap.tileSizeY; y >= bboxX.y1; y -= stateMap.tileSizeY) {
        bool hasRenderedTileOnLine = false;
        for (int x = bboxX.x1; x < bboxX.x2; x += stateMap.tileSizeX) {
            assert(x % stateMap.tileSizeX == 0 && y % stateMap.tileSizeY == 0);
            const TileState* tile = stateMap.getTileAt(x, y);
            if (tile->status != eTileStatusNotRendered) {
                hasRenderedTileOnLine = true;
                break;
            }
        }
        if (hasRenderedTileOnLine) {
            break;
        } else {
            bboxX.y2 -= stateMap.tileSizeY;
            bboxB.y1 = bboxX.y2;
        }
    }

    if ( !bboxB.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the bounds since we rounded to tile size earlier
        RectI bboxBIntersected;
        bboxB.intersect(stateMap.state->bounds, &bboxBIntersected);
        rectsToRender->push_back(bboxBIntersected);
    }

    //find left
    RectI bboxC = bboxX;
    bboxC.x2 = bboxC.x1;
    if ( bboxX.y1 < bboxX.y2 ) {
        for (int x = bboxX.x1; x < bboxX.x2; x += stateMap.tileSizeX) {
            bool hasRenderedTileOnCol = false;
            for (int y = bboxX.y1; y < bboxX.y2; y += stateMap.tileSizeY) {
                assert(x % stateMap.tileSizeX == 0 && y % stateMap.tileSizeY == 0);
                const TileState* tile = stateMap.getTileAt(x, y);
                if (tile->status != eTileStatusNotRendered) {
                    hasRenderedTileOnCol = true;
                    break;
                }

            }
            if (hasRenderedTileOnCol) {
                break;
            } else {
                bboxX.x1 += stateMap.tileSizeX;
                bboxC.x2 = bboxX.x1;
            }
        }
    }
    if ( !bboxC.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the bounds since we rounded to tile size earlier
        RectI bboxCIntersected;
        bboxC.intersect(stateMap.state->bounds, &bboxCIntersected);
        rectsToRender->push_back(bboxCIntersected);
    }

    //find right
    RectI bboxD = bboxX;
    bboxD.x1 = bboxD.x2;
    if ( bboxX.y1 < bboxX.y2 ) {
        for (int x = bboxX.x2 - stateMap.tileSizeX; x >= bboxX.x1; x -= stateMap.tileSizeX) {
            bool hasRenderedTileOnCol = false;
            for (int y = bboxX.y1; y < bboxX.y2; y += stateMap.tileSizeY) {
                assert(x % stateMap.tileSizeX == 0 && y % stateMap.tileSizeY == 0);
                const TileState* tile = stateMap.getTileAt(x, y);
                if (tile->status != eTileStatusNotRendered) {
                    hasRenderedTileOnCol = true;
                    break;
                }
            }
            if (hasRenderedTileOnCol) {
                break;
            } else {
                bboxX.x2 -= stateMap.tileSizeX;
                bboxD.x1 = bboxX.x2;
            }
        }
    }
    if ( !bboxD.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the bounds since we rounded to tile size earlier
        RectI bboxDIntersected;
        bboxD.intersect(stateMap.state->bounds, &bboxDIntersected);
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
    bboxX = getMinimalBboxToRenderFromTilesState(bboxX, stateMap);

    if ( !bboxX.isNull() ) { // empty boxes should not be pushed
        // Ensure the bbox lies in the bounds since we rounded to tile size earlier
        RectI bboxXIntersected;
        bboxX.intersect(stateMap.state->bounds, &bboxXIntersected);
        rectsToRender->push_back(bboxXIntersected);
    }
    
} // getMinimalRectsToRenderFromTilesState


NATRON_NAMESPACE_EXIT

