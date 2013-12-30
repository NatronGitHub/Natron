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

#include "Decoder.h"

#include <cstdlib>
#include <QtGui/qrgb.h>

#include "Global/AppManager.h"

#include "Engine/Row.h"
#include "Engine/ImageInfo.h"

#include "Readers/Reader.h"
#include "Readers/ExrDecoder.h"

using namespace Natron;

Decoder::Decoder(Reader* op_):
_premult(false)
, _autoCreateAlpha(false)
, _reader(op_)
, _lut(0)
, _readerInfo(new ImageInfo)
, _filename()
{
}

Decoder::~Decoder(){

}


void Decoder::createKnobDynamically(){
    _reader->createKnobDynamically();
}

void Decoder::setReaderInfo(Format dispW,
	const RectI& dataW,
    Natron::ChannelSet channels) {
    _readerInfo->setDisplayWindow(dispW);
    _readerInfo->setRoD(dataW);
    _readerInfo->setChannels(channels);
}

