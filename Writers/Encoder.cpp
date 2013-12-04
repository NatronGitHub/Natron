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

#include "Encoder.h"

#include "Engine/Row.h"
#include "Writers/Writer.h"
//#include "Gui/KnobGui.h"

using namespace Natron;

/*Constructors should initialize variables, but shouldn't do any heavy computations, as these objects
 are oftenly re-created. To initialize the input color-space , you can do so by overloading
 initializeColorSpace. This function is called after the constructor and before any
 reading occurs.*/
Encoder::Encoder(Writer* writer):_premult(false),_lut(0),_writer(writer),_optionalKnobs(0),_filename()
{
}

Encoder::~Encoder()
{
}

void Encoder::to_byte(Channel z, uchar* to, const float* from, const float* alpha, int W, int delta )
{
    if ( z <= 3 && !_lut->linear()) {
        if (alpha && _premult) {
            _lut->to_byte_premult(to, from, alpha, W,delta);
        } else {
            _lut->to_byte(to, from, W,delta);
        }
    } else {
        Color::linear_to_byte(to, from, W,delta);
    }
}

void Encoder::to_short(Channel z, U16* to, const float* from, const float* alpha, int W, int /* bits=16 */, int delta)
{
    if ( z <= 3 && !_lut->linear()) {
        if(alpha && _premult) {
            _lut->to_short_premult(to, from, alpha, W,delta);
        } else {
            _lut->to_short(to, from, W,delta);
        }
    } else {
        Color::linear_to_short(to, from, W,delta);
    }
}

void Encoder::to_float(Channel z, float* to, const float* from, const float* alpha, int W, int delta )
{
    if ( z <= 3 && !_lut->linear()) {
        if (alpha && _premult) {
            _lut->to_float_premult(to, from, alpha, W,delta);
        } else {
            _lut->to_float(to, from, W,delta);
        }
    } else {
        Color::linear_to_float(to, from, W,delta);
    }
}

void Encoder::to_byte_rect(uchar* to, const float* from,
                           const RectI& rect, const RectI& srcRod, const RectI& dstRod,
                           Natron::Color::Lut::PackedPixelsFormat outputPacking, int invertY)
{
    if (!_lut->linear()) {
        if (_premult) {
            _lut->to_byte_rect_premult(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        } else {
            _lut->to_byte_rect(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        }
    } else {
        if (_premult) {
            Color::linear_to_byte_rect_premult(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        } else {
            Color::linear_to_byte_rect(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        }
    }
}

void Encoder::to_short_rect(U16* to, const float* from,
                            const RectI& rect, const RectI& srcRod, const RectI& dstRod,
                            Natron::Color::Lut::PackedPixelsFormat outputPacking, int invertY)
{
    if (!_lut->linear()) {
        if (_premult) {
            _lut->to_short_rect_premult(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        } else {
            _lut->to_short_rect(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        }
    } else {
        if (_premult) {
            Color::linear_to_short_rect_premult(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        } else {
            Color::linear_to_short_rect(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        }
    }
}

void Encoder::to_float_rect(float* to, const float* from,
                            const RectI& rect, const RectI& srcRod, const RectI& dstRod,
                            Natron::Color::Lut::PackedPixelsFormat outputPacking, int invertY)
{
    if (!_lut->linear()) {
        if (_premult) {
            _lut->to_float_rect_premult(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        } else {
            _lut->to_float_rect(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        }
    } else {
        if (_premult) {
            Color::linear_to_float_rect_premult(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        } else {
            Color::linear_to_float_rect(to, from, rect, srcRod, dstRod, invertY, outputPacking);
        }
    }
}

void EncoderKnobs::initKnobs(const std::string&)
{
    _writer->createKnobDynamically();
}

