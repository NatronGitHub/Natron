//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Engine/FrameEntry.h"

using namespace Natron;



boost::shared_ptr<FrameParams>
FrameEntry::makeParams(const RectI & rod,
                       int bitDepth,
                       int texW,
                       int texH)
{
    return boost::shared_ptr<FrameParams>( new FrameParams(rod, bitDepth, texW, texH) );
}



FrameKey
FrameEntry::makeKey(SequenceTime time,
                    U64 treeVersion,
                    double gain,
                    int lut,
                    int bitDepth,
                    int channels,
                    int view,
                    const TextureRect & textureRect,
                    const RenderScale & scale,
                    const std::string & inputName)
{
    return FrameKey(time,treeVersion,gain,lut,bitDepth,channels,view,textureRect,scale,inputName);
}