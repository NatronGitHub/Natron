//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Engine/FrameEntry.h"
#include "Engine/FrameParams.h"

using namespace Natron;


FrameKey::FrameKey()
: KeyHelper<U64>()
, _time(0)
, _treeVersion(0)
, _gain(0)
, _lut(0)
, _bitDepth(0)
, _channels(0)
, _view(0)
, _textureRect()
, _scale()
{
    _scale.x = _scale.y = 0.;
}


FrameKey::FrameKey(SequenceTime time,U64 treeVersion,double gain,
         int lut,int bitDepth,int channels,int view,const TextureRect& textureRect,const RenderScale& scale,const std::string& inputName):
KeyHelper<U64>()
,_time(time)
,_treeVersion(treeVersion)
,_gain(gain)
,_lut(lut)
,_bitDepth(bitDepth)
,_channels(channels)
,_view(view)
,_textureRect(textureRect)
,_scale(scale)
,_inputName(inputName)
{
    
}

void FrameKey::fillHash(Hash64* hash) const {
    hash->append(_time);
    hash->append(_treeVersion);
    hash->append(_gain);
    hash->append(_lut);
    hash->append(_bitDepth);
    hash->append(_channels);
    hash->append(_view);
    hash->append(_textureRect.x1);
    hash->append(_textureRect.y1);
    hash->append(_textureRect.x2);
    hash->append(_textureRect.y2);
    hash->append(_textureRect.w);
    hash->append(_textureRect.h);
    hash->append(_textureRect.closestPo2);
    hash->append(_scale.x);
    hash->append(_scale.y); 
    for (size_t i = 0; i < _inputName.size(); ++i) {
        hash->append(_inputName.at(i));
    }
}

bool FrameKey::operator==(const FrameKey& other) const {
    return  _time == other._time &&
    _treeVersion == other._treeVersion &&
    _gain == other._gain &&
    _lut == other._lut &&
    _bitDepth == other._bitDepth &&
    _channels == other._channels &&
    _view == other._view &&
    _textureRect == other._textureRect &&
    _scale.x == other._scale.x &&
    _scale.y == other._scale.y &&
    _inputName == other._inputName;
}

FrameKey FrameEntry::makeKey(SequenceTime time,U64 treeVersion,double gain,
                        int lut,int bitDepth,int channels,int view,const TextureRect& textureRect,
                             const RenderScale& scale,const std::string& inputName){
    return FrameKey(time,treeVersion,gain,lut,bitDepth,channels,view,textureRect,scale,inputName);
}

boost::shared_ptr<const FrameParams> FrameEntry::makeParams(const RectI rod,int bitDepth,int texW,int texH)
{
    return boost::shared_ptr<const FrameParams>(new FrameParams(rod,bitDepth,texW,texH));
}

