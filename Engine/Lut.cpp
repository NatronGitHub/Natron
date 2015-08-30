/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "Lut.h"

#include <cstring> // for memcpy
#include <algorithm> // min, max
#include <stdexcept>

#include "Engine/RectI.h"

/*
 * The to_byte* and from_byte* functions implement and generalize the algorithm
 * described in:
 *
 * http://dx.doi.org/10.1145/1242073.1242206 Spitzak, B. (2002, July). High-speed conversion of floating point images to 8-bit. In ACM SIGGRAPH 2002 conference abstracts and applications (pp. 193-193). ACM.
 * https://spitzak.github.io/conversion/sketches_0265.pdf
 *
 * Related patents:
 * http://www.google.com/patents/US5528741 "Method and apparatus for converting floating-point pixel values to byte pixel values by table lookup" (Caduc)
 * http://www.google.com/patents/US20040100475 http://www.google.com/patents/US6999098 "Apparatus for converting floating point values to gamma corrected fixed point values "
 * https://www.google.fr/patents/US7456845 https://www.google.fr/patents/US7405736 "Efficient perceptual/physical color space conversion "
 */


namespace Natron {
namespace Color {
// compile-time endianness checking found on:
// http://stackoverflow.com/questions/2100331/c-macro-definition-to-determine-big-endian-or-little-endian-machine
// if(O32_HOST_ORDER == O32_BIG_ENDIAN) will always be optimized by gcc -O2
enum
{
    O32_LITTLE_ENDIAN = 0x03020100ul,
    O32_BIG_ENDIAN = 0x00010203ul,
    O32_PDP_ENDIAN = 0x01000302ul
};

static const union
{
    uint8_t bytes[4];
    uint32_t value;
}

o32_host_order = {
    { 0, 1, 2, 3 }
};
#define O32_HOST_ORDER (o32_host_order.value)
static unsigned short
hipart(const float f)
{
    union
    {
        float f;
        unsigned short us[2];
    }

    tmp;

    tmp.us[0] = tmp.us[1] = 0;
    tmp.f = f;

    if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
        return tmp.us[0];
    } else if (O32_HOST_ORDER == O32_LITTLE_ENDIAN) {
        return tmp.us[1];
    } else {
        assert( (O32_HOST_ORDER == O32_LITTLE_ENDIAN) || (O32_HOST_ORDER == O32_BIG_ENDIAN) );

        return 0;
    }
}

static float
index_to_float(const unsigned short i)
{
    union
    {
        float f;
        unsigned short us[2];
    }

    tmp;

    /* positive and negative zeros, and all gradual underflow, turn into zero: */
    if ( ( i < 0x80) || ( ( i >= 0x8000) && ( i < 0x8080) ) ) {
        return 0;
    }
    /* All NaN's and infinity turn into the largest possible legal float: */
    if ( ( i >= 0x7f80) && ( i < 0x8000) ) {
        return std::numeric_limits<float>::max();
    }
    if (i >= 0xff80) {
        return -std::numeric_limits<float>::max();
    }
    if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
        tmp.us[0] = i;
        tmp.us[1] = 0x8000;
    } else if (O32_HOST_ORDER == O32_LITTLE_ENDIAN) {
        tmp.us[0] = 0x8000;
        tmp.us[1] = i;
    } else {
        assert( (O32_HOST_ORDER == O32_LITTLE_ENDIAN) || (O32_HOST_ORDER == O32_BIG_ENDIAN) );
    }

    return tmp.f;
}

///initialize the singleton
LutManager LutManager::m_instance = LutManager();
LutManager::LutManager()
    : luts()
{
}

const Lut*
LutManager::getLut(const std::string & name,
                   fromColorSpaceFunctionV1 fromFunc,
                   toColorSpaceFunctionV1 toFunc)
{
    LutsMap::iterator found = LutManager::m_instance.luts.find(name);

    if ( found != LutManager::m_instance.luts.end() ) {
        return found->second;
    } else {
        std::pair<LutsMap::iterator,bool> ret =
            LutManager::m_instance.luts.insert( std::make_pair( name,new Lut(name,fromFunc,toFunc) ) );
        assert(ret.second);

        return ret.first->second;
    }

    return NULL;
}

LutManager::~LutManager()
{
    ////the luts must all have been released before!
    ////This is because the Lut holds a OFX::MultiThread::Mutex and it can't be deleted
    //// by this singleton because it makes their destruction time uncertain regarding to
    ///the host multi-thread suite.
    for (LutsMap::iterator it = luts.begin(); it != luts.end(); ++it) {
        delete it->second;
    }
}

static bool
clip(RectI* what,
     const RectI & to)
{
    return what->intersect(to, what);
}

#ifdef DEAD_CODE
static bool
intersects(const RectI & what,
           const RectI & other)
{
    return what.intersects(other);
}
#endif // DEAD_CODE

static void
getOffsetsForPacking(PixelPackingEnum format,
                     int *r,
                     int *g,
                     int *b,
                     int *a)
{
    if (format == ePixelPackingBGRA) {
        *b = 0;
        *g = 1;
        *r = 2;
        *a = 3;
    } else if (format == ePixelPackingRGBA) {
        *r = 0;
        *g = 1;
        *b = 2;
        *a = 3;
    } else if (format == ePixelPackingRGB) {
        *r = 0;
        *g = 1;
        *b = 2;
        *a = -1;
    } else if (format == ePixelPackingBGR) {
        *r = 0;
        *g = 1;
        *b = 2;
        *a = -1;
    } else if (format == ePixelPackingPLANAR) {
        *r = 0;
        *g = 1;
        *b = 2;
        *a = -1;
    } else {
        *r = -1;
        *g = -1;
        *b = -1;
        *a = -1;
        throw std::runtime_error("Unsupported pixel packing format");
    }
}

float
Lut::fromColorSpaceUint8ToLinearFloatFast(unsigned char v) const
{
    assert(init_);

    return fromFunc_uint8_to_float[v];
}

#ifdef DEAD_CODE
// It is not recommended to use this function, because the output is quantized
// If one really needs float, one has to use the full function (or OpenColorIO)
float
Lut::toColorSpaceFloatFromLinearFloatFast(float v) const
{
    assert(init_);

    return Color::intToFloat<0xff01>(toFunc_hipart_to_uint8xx[hipart(v)]);
}
#endif // DEAD_CODE

unsigned char
Lut::toColorSpaceUint8FromLinearFloatFast(float v) const
{
    assert(init_);

    return Color::uint8xxToChar(toFunc_hipart_to_uint8xx[hipart(v)]);
}

unsigned short
Lut::toColorSpaceUint8xxFromLinearFloatFast(float v) const
{
    assert(init_);

    return toFunc_hipart_to_uint8xx[hipart(v)];
}

// the following only works for increasing LUTs
unsigned short
Lut::toColorSpaceUint16FromLinearFloatFast(float v) const
{
    assert(init_);
    // algorithm:
    // - convert to 8 bits -> val8u
    // - convert val8u-1, val8u and val8u+1 to float
    // - interpolate linearly in the right interval
    unsigned char v8u = toColorSpaceUint8FromLinearFloatFast(v);
    unsigned char v8u_next, v8u_prev;
    float v32f_next, v32f_prev;
    if (v8u == 0) {
        v8u_prev = 0;
        v8u_next = 1;
        v32f_prev = fromColorSpaceUint8ToLinearFloatFast(0);
        v32f_next = fromColorSpaceUint8ToLinearFloatFast(1);
    } else if (v8u == 255) {
        v8u_prev = 254;
        v8u_next = 255;
        v32f_prev = fromColorSpaceUint8ToLinearFloatFast(254);
        v32f_next = fromColorSpaceUint8ToLinearFloatFast(255);
    } else {
        float v32f = fromColorSpaceUint8ToLinearFloatFast(v8u);
        // we suppose the LUT is an increasing func
        if (v < v32f) {
            v8u_prev = v8u - 1;
            v32f_prev = fromColorSpaceUint8ToLinearFloatFast(v8u_prev);
            v8u_next = v8u;
            v32f_next = v32f;
        } else {
            v8u_prev = v8u;
            v32f_prev = v32f;
            v8u_next = v8u + 1;
            v32f_next = fromColorSpaceUint8ToLinearFloatFast(v8u_next);
        }
    }

    // interpolate linearly
    return (v8u_prev << 8) + v8u_prev + (v - v32f_prev) * ( ( (v8u_next - v8u_prev) << 8 ) + (v8u_next + v8u_prev) ) / (v32f_next - v32f_prev) + 0.5;
}

float
Lut::fromColorSpaceUint16ToLinearFloatFast(unsigned short v) const
{
    assert(init_);
    // the following is from ImageMagick's quantum.h
    unsigned char v8u_prev = ( v - (v >> 8) ) >> 8;
    unsigned char v8u_next = v8u_prev + 1;
    unsigned short v16u_prev = (v8u_prev << 8) + v8u_prev;
    unsigned short v16u_next = (v8u_next << 8) + v8u_next;
    float v32f_prev = fromColorSpaceUint8ToLinearFloatFast(v8u_prev);
    float v32f_next = fromColorSpaceUint8ToLinearFloatFast(v8u_next);

    // interpolate linearly
    return v32f_prev + (v - v16u_prev) * (v32f_next - v32f_prev) / (v16u_next - v16u_prev);
}

void
Lut::fillTables() const
{
    if (init_) {
        return;
    }
    // fill all
    for (int i = 0; i < 0x10000; ++i) {
        float inp = index_to_float( (unsigned short)i );
        float f = _toFunc(inp);
        toFunc_hipart_to_uint8xx[i] = Color::floatToInt<0xff01>(f);
    }
    // fill fromFunc_uint8_to_float, and make sure that
    // the entries of toFunc_hipart_to_uint8xx corresponding
    // to the transform of each byte value contain the same value,
    // so that toFunc(fromFunc(b)) is identity
    //
    for (int b = 0; b < 256; ++b) {
        float f = _fromFunc( Color::intToFloat<256>(b) );
        fromFunc_uint8_to_float[b] = f;
        int i = hipart(f);
        toFunc_hipart_to_uint8xx[i] = Color::charToUint8xx(b);
    }
}

#ifdef DEAD_CODE
void
Lut::to_byte_planar(unsigned char* to,
                    const float* from,
                    int W,
                    const float* alpha,
                    int inDelta,
                    int outDelta) const
{
    validate();
    unsigned char *end = to + W * outDelta;
    // coverity[dont_call]
    int start = rand() % W;
    const float *q;
    unsigned char *p;
    unsigned error;
    if (!alpha) {
        /* go fowards from starting point to end of line: */
        error = 0x80;
        for (p = to + start * outDelta, q = from + start * inDelta; p < end; p += outDelta, q += inDelta) {
            error = (error & 0xff) + toFunc_hipart_to_uint8xx[hipart(*q)];
            *p = (unsigned char)(error >> 8);
        }
        /* go backwards from starting point to start of line: */
        error = 0x80;
        for (p = to + (start - 1) * outDelta, q = from + start * inDelta; p >= to; p -= outDelta) {
            q -= inDelta;
            error = (error & 0xff) + toFunc_hipart_to_uint8xx[hipart(*q)];
            *p = (unsigned char)(error >> 8);
        }
    } else {
        const float *a = alpha;
        /* go fowards from starting point to end of line: */
        error = 0x80;
        for (p = to + start * outDelta, q = from + start * inDelta, a += start * inDelta; p < end; p += outDelta, q += inDelta, a += inDelta) {
            const float v = *q * *a;
            error = (error & 0xff) + toFunc_hipart_to_uint8xx[hipart(v)];
            ++a;
            *p = (unsigned char)(error >> 8);
        }
        /* go backwards from starting point to start of line: */
        error = 0x80;
        for (p = to + (start - 1) * outDelta, q = from + start * inDelta, a = alpha + start * inDelta; p >= to; p -= outDelta) {
            const float v = *q * *a;
            q -= inDelta;
            q -= inDelta;
            error = (error & 0xff) + toFunc_hipart_to_uint8xx[hipart(v)];
            *p = (unsigned char)(error >> 8);
        }
    }
}
#endif // DEAD_CODE

#ifdef DEAD_CODE
void
Lut::to_short_planar(unsigned short* /*to*/,
                     const float* /*from*/,
                     int /*W*/,
                     const float* /*alpha*/,
                     int /*inDelta*/,
                     int /*outDelta*/) const
{
    throw std::runtime_error("Lut::to_short_planar not implemented yet.");
}
#endif // DEAD_CODE

void
Lut::to_float_planar(float* to,
                     const float* from,
                     int W,
                     const float* alpha,
                     int inDelta,
                     int outDelta) const
{
    validate();
    if (!alpha) {
        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
            to[t] = toColorSpaceFloatFromLinearFloat(from[f]);
        }
    } else {
        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
            to[t] = toColorSpaceFloatFromLinearFloat(from[f] * alpha[f]);
        }
    }
}

void
Lut::to_byte_packed(unsigned char* to,
                    const float* from,
                    const RectI & conversionRect,
                    const RectI & srcBounds,
                    const RectI & dstBounds,
                    PixelPackingEnum inputPacking,
                    PixelPackingEnum outputPacking,
                    bool invertY,
                    bool premult) const
{
    ///clip the conversion rect to srcBounds and dstBounds
    RectI rect = conversionRect;

    if ( !clip(&rect,srcBounds) || !clip(&rect,dstBounds) ) {
        return;
    }


    bool inputHasAlpha = inputPacking == ePixelPackingBGRA || inputPacking == ePixelPackingRGBA;
    bool outputHasAlpha = outputPacking == ePixelPackingBGRA || outputPacking == ePixelPackingRGBA;
    int inROffset, inGOffset, inBOffset, inAOffset;
    int outROffset, outGOffset, outBOffset, outAOffset;
    getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
    getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);

    int inPackingSize,outPackingSize;
    inPackingSize = inputHasAlpha ? 4 : 3;
    outPackingSize = outputHasAlpha ? 4 : 3;

    validate();

    for (int y = rect.y1; y < rect.y2; ++y) {
        // coverity[dont_call]
        int start = rand() % (rect.x2 - rect.x1) + rect.x1;
        unsigned error_r, error_g, error_b;
        error_r = error_g = error_b = 0x80;
        int srcY = y;
        if (!invertY) {
            srcY = srcBounds.y2 - y - 1;
        }


        int dstY = dstBounds.y2 - y - 1;
        const float *src_pixels = from + (srcY * (srcBounds.x2 - srcBounds.x1) * inPackingSize);
        unsigned char *dst_pixels = to + (dstY * (dstBounds.x2 - dstBounds.x1) * outPackingSize);
        /* go fowards from starting point to end of line: */
        for (int x = start; x < rect.x2; ++x) {
            int inCol = x * inPackingSize;
            int outCol = x * outPackingSize;
            float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;
            error_r = (error_r & 0xff) + toFunc_hipart_to_uint8xx[hipart(src_pixels[inCol + inROffset] * a)];
            error_g = (error_g & 0xff) + toFunc_hipart_to_uint8xx[hipart(src_pixels[inCol + inGOffset] * a)];
            error_b = (error_b & 0xff) + toFunc_hipart_to_uint8xx[hipart(src_pixels[inCol + inBOffset] * a)];
            assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
            dst_pixels[outCol + outROffset] = (unsigned char)(error_r >> 8);
            dst_pixels[outCol + outGOffset] = (unsigned char)(error_g >> 8);
            dst_pixels[outCol + outBOffset] = (unsigned char)(error_b >> 8);
            if (outputHasAlpha) {
                // alpha is linear and should not be dithered
                dst_pixels[outCol + outAOffset] = floatToInt<256>(a);
            }
        }
        /* go backwards from starting point to start of line: */
        error_r = error_g = error_b = 0x80;
        for (int x = start - 1; x >= rect.x1; --x) {
            int inCol = x * inPackingSize;
            int outCol = x * outPackingSize;
            float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;
            error_r = (error_r & 0xff) + toFunc_hipart_to_uint8xx[hipart(src_pixels[inCol + inROffset] * a)];
            error_g = (error_g & 0xff) + toFunc_hipart_to_uint8xx[hipart(src_pixels[inCol + inGOffset] * a)];
            error_b = (error_b & 0xff) + toFunc_hipart_to_uint8xx[hipart(src_pixels[inCol + inBOffset] * a)];
            assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
            dst_pixels[outCol + outROffset] = (unsigned char)(error_r >> 8);
            dst_pixels[outCol + outGOffset] = (unsigned char)(error_g >> 8);
            dst_pixels[outCol + outBOffset] = (unsigned char)(error_b >> 8);
            if (outputHasAlpha) {
                // alpha is linear and should not be dithered
                dst_pixels[outCol + outAOffset] = floatToInt<256>(a);
            }
        }
    }
} // to_byte_packed

#ifdef DEAD_CODE
void
Lut::to_short_packed(unsigned short* /*to*/,
                     const float* /*from*/,
                     const RectI & /*conversionRect*/,
                     const RectI & /*srcBounds*/,
                     const RectI & /*dstBounds*/,
                     PixelPackingEnum /*inputPacking*/,
                     PixelPackingEnum /*outputPacking*/,
                     bool /*invertY*/,
                     bool /*premult*/) const
{
    throw std::runtime_error("Lut::to_short_packed not implemented yet.");
}
#endif // DEAD_CODE

void
Lut::to_float_packed(float* to,
                     const float* from,
                     const RectI & conversionRect,
                     const RectI & srcBounds,
                     const RectI & dstBounds,
                     PixelPackingEnum inputPacking,
                     PixelPackingEnum outputPacking,
                     bool invertY,
                     bool premult) const
{
    ///clip the conversion rect to srcBounds and dstBounds
    RectI rect = conversionRect;

    if ( !clip(&rect,srcBounds) || !clip(&rect,dstBounds) ) {
        return;
    }

    bool inputHasAlpha = inputPacking == ePixelPackingBGRA || inputPacking == ePixelPackingRGBA;
    bool outputHasAlpha = outputPacking == ePixelPackingBGRA || outputPacking == ePixelPackingRGBA;
    int inROffset, inGOffset, inBOffset, inAOffset;
    int outROffset, outGOffset, outBOffset, outAOffset;
    getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
    getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);

    int inPackingSize,outPackingSize;
    inPackingSize = inputHasAlpha ? 4 : 3;
    outPackingSize = outputHasAlpha ? 4 : 3;

    validate();

    for (int y = rect.y1; y < rect.y2; ++y) {
        int srcY = y;
        if (invertY) {
            srcY = srcBounds.y2 - y - 1;
        }

        int dstY = dstBounds.y2 - y - 1;
        const float *src_pixels = from + (srcY * (srcBounds.x2 - srcBounds.x1) * inPackingSize);
        float *dst_pixels = to + (dstY * (dstBounds.x2 - dstBounds.x1) * outPackingSize);
        /* go fowards from starting point to end of line: */
        for (int x = rect.x1; x < rect.x2; ++x) {
            int inCol = x * inPackingSize;
            int outCol = x * outPackingSize;
            float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;;
            dst_pixels[outCol + outROffset] = toColorSpaceFloatFromLinearFloat(src_pixels[inCol + inROffset] * a);
            dst_pixels[outCol + outGOffset] = toColorSpaceFloatFromLinearFloat(src_pixels[inCol + inGOffset] * a);
            dst_pixels[outCol + outBOffset] = toColorSpaceFloatFromLinearFloat(src_pixels[inCol + inBOffset] * a);
            if (outputHasAlpha) {
                // alpha is linear and should not be dithered
                dst_pixels[outCol + outAOffset] = a;
            }
        }
    }
}

void
Lut::from_byte_planar(float* to,
                      const unsigned char* from,
                      int W,
                      const unsigned char* alpha,
                      int inDelta,
                      int outDelta) const
{
    validate();
    if (!alpha) {
        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
            to[f] = fromFunc_uint8_to_float[(int)from[f]];
        }
    } else {
        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
            to[t] = alpha[f] <= 0 ? 0 : Color::intToFloat<256>(fromFunc_uint8_to_float[(from[f] * 255 + 128) / alpha[f]] * alpha[f]);
        }
    }
}

void
Lut::from_short_planar(float* /*to*/,
                       const unsigned short* /*from*/,
                       int /*W*/,
                       const unsigned short*/* alpha*/,
                       int /*inDelta*/,
                       int /* outDelta*/) const
{
    throw std::runtime_error("Lut::from_short_planar not implemented yet.");
}

void
Lut::from_float_planar(float* to,
                       const float* from,
                       int W,
                       const float* alpha,
                       int inDelta,
                       int outDelta) const
{
    validate();
    if (!alpha) {
        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
            to[t] = fromColorSpaceFloatToLinearFloat(from[f]);
        }
    } else {
        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
            float a = alpha[f];
            to[t] = a <= 0. ? 0. : fromColorSpaceFloatToLinearFloat(from[f] / a) * a;
        }
    }
}

void
Lut::from_byte_packed(float* to,
                      const unsigned char* from,
                      const RectI & conversionRect,
                      const RectI & srcBounds,
                      const RectI & dstBounds,
                      PixelPackingEnum inputPacking,
                      PixelPackingEnum outputPacking,
                      bool invertY,
                      bool premult) const
{
    if ( ( inputPacking == ePixelPackingPLANAR) || ( outputPacking == ePixelPackingPLANAR) ) {
        throw std::runtime_error("Invalid pixel format.");
    }

    ///clip the conversion rect to srcBounds and dstBounds
    RectI rect = conversionRect;
    if ( !clip(&rect,srcBounds) || !clip(&rect,dstBounds) ) {
        return;
    }


    bool inputHasAlpha = inputPacking == ePixelPackingBGRA || inputPacking == ePixelPackingRGBA;
    bool outputHasAlpha = outputPacking == ePixelPackingBGRA || outputPacking == ePixelPackingRGBA;
    int inROffset, inGOffset, inBOffset, inAOffset;
    int outROffset, outGOffset, outBOffset, outAOffset;
    getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
    getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);

    int inPackingSize,outPackingSize;
    inPackingSize = inputHasAlpha ? 4 : 3;
    outPackingSize = outputHasAlpha ? 4 : 3;

    validate();
    for (int y = rect.y1; y < rect.y2; ++y) {
        int srcY = y;
        if (invertY) {
            srcY = srcBounds.y2 - y - 1;
        }

        const unsigned char *src_pixels = from + (srcY * (srcBounds.x2 - srcBounds.x1) * inPackingSize);
        float *dst_pixels = to + (y * (dstBounds.x2 - dstBounds.x1) * outPackingSize);
        for (int x = rect.x1; x < rect.x2; ++x) {
            int inCol = x * inPackingSize;
            int outCol = x * outPackingSize;
            if (inputHasAlpha && premult) {
                float rf = 0., gf = 0., bf = 0.;
                float a = Color::intToFloat<256>(src_pixels[inCol + inAOffset]);
                if (a > 0) {
                    rf = Color::intToFloat<256>(src_pixels[inCol + inROffset]) / a;
                    gf = Color::intToFloat<256>(src_pixels[inCol + inGOffset]) / a;
                    bf = Color::intToFloat<256>(src_pixels[inCol + inBOffset]) / a;
                }
                // we may lose a bit of information, but hey, it's 8-bits anyway, who cares?
                dst_pixels[outCol + outROffset] = fromColorSpaceUint8ToLinearFloatFast( Color::floatToInt<256>(rf) ) * a;
                dst_pixels[outCol + outGOffset] = fromColorSpaceUint8ToLinearFloatFast( Color::floatToInt<256>(gf) ) * a;
                dst_pixels[outCol + outBOffset] = fromColorSpaceUint8ToLinearFloatFast( Color::floatToInt<256>(bf) ) * a;
                if (outputHasAlpha) {
                    // alpha is linear
                    dst_pixels[outCol + outAOffset] = a;
                }
            } else {
                int r8 = 0, g8 = 0, b8 = 0;
                r8 = src_pixels[inCol + inROffset];
                g8 = src_pixels[inCol + inGOffset];
                b8 = src_pixels[inCol + inBOffset];
                assert(r8 >= 0 && r8 < 256 && g8 >= 0 && g8 < 256 && b8 >= 0 && b8 < 256);
                dst_pixels[outCol + outROffset] = fromFunc_uint8_to_float[r8];
                dst_pixels[outCol + outGOffset] = fromFunc_uint8_to_float[g8];
                dst_pixels[outCol + outBOffset] = fromFunc_uint8_to_float[b8];
                if (outputHasAlpha) {
                    // alpha is linear
                    float a = Color::intToFloat<256>(src_pixels[inCol + inAOffset]);
                    dst_pixels[outCol + outAOffset] = a;
                }
            }
        }
    }
} // from_byte_packed

void
Lut::from_short_packed(float* /*to*/,
                       const unsigned short* /*from*/,
                       const RectI & /*conversionRect*/,
                       const RectI & /*srcBounds*/,
                       const RectI & /*dstBounds*/,
                       PixelPackingEnum /*inputPacking*/,
                       PixelPackingEnum /*outputPacking*/,
                       bool /*invertY*/,
                       bool /*premult*/) const
{
    throw std::runtime_error("Lut::from_short_packed not implemented yet.");
}

void
Lut::from_float_packed(float* to,
                       const float* from,
                       const RectI & conversionRect,
                       const RectI & srcBounds,
                       const RectI & dstBounds,
                       PixelPackingEnum inputPacking,
                       PixelPackingEnum outputPacking,
                       bool invertY,
                       bool premult) const
{
    if ( ( inputPacking == ePixelPackingPLANAR) || ( outputPacking == ePixelPackingPLANAR) ) {
        throw std::runtime_error("Invalid pixel format.");
    }

    ///clip the conversion rect to srcBounds and dstBounds
    RectI rect = conversionRect;
    if ( !clip(&rect,srcBounds) || !clip(&rect,dstBounds) ) {
        return;
    }


    bool inputHasAlpha = inputPacking == ePixelPackingBGRA || inputPacking == ePixelPackingRGBA;
    bool outputHasAlpha = outputPacking == ePixelPackingBGRA || outputPacking == ePixelPackingRGBA;
    int inROffset, inGOffset, inBOffset, inAOffset;
    int outROffset, outGOffset, outBOffset, outAOffset;
    getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
    getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);

    int inPackingSize,outPackingSize;
    inPackingSize = inputHasAlpha ? 4 : 3;
    outPackingSize = outputHasAlpha ? 4 : 3;

    validate();

    for (int y = rect.y1; y < rect.y2; ++y) {
        int srcY = y;
        if (invertY) {
            srcY = srcBounds.y2 - y - 1;
        }
        const float *src_pixels = from + (srcY * (srcBounds.x2 - srcBounds.x1) * inPackingSize);
        float *dst_pixels = to + (y * (dstBounds.x2 - dstBounds.x1) * outPackingSize);
        for (int x = rect.x1; x < rect.x2; ++x) {
            int inCol = x * inPackingSize;
            int outCol = x * outPackingSize;
            float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;;
            float rf = 0., gf = 0., bf = 0.;
            if (a > 0.) {
                rf = src_pixels[inCol + inROffset] / a;
                gf = src_pixels[inCol + inGOffset] / a;
                bf = src_pixels[inCol + inBOffset] / a;
            }
            dst_pixels[outCol + outROffset] = fromColorSpaceFloatToLinearFloat(rf) * a;
            dst_pixels[outCol + outGOffset] = fromColorSpaceFloatToLinearFloat(gf) * a;
            dst_pixels[outCol + outBOffset] = fromColorSpaceFloatToLinearFloat(bf) * a;
            if (outputHasAlpha) {
                // alpha is linear
                dst_pixels[outCol + outAOffset] = a;
            }
        }
    }
} // from_float_packed

///////////////////////
/////////////////////////////////////////// LINEAR //////////////////////////////////////////////
///////////////////////

namespace Linear {
void
from_byte_planar(float *to,
                 const unsigned char *from,
                 int W,
                 int inDelta,
                 int outDelta)
{
    from += (W - 1) * inDelta;
    to += W * outDelta;
    for (; --W >= 0; from -= inDelta) {
        to -= outDelta;
        *to = intToFloat<256>(*from);
    }
}

void
from_short_planar(float *to,
                  const unsigned short *from,
                  int W,
                  int inDelta,
                  int outDelta)
{
    for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
        to[t] = intToFloat<65536>(from[f]);
    }
}

void
from_float_planar(float *to,
                  const float *from,
                  int W,
                  int inDelta,
                  int outDelta)
{
    if ( ( inDelta == 1) && ( outDelta == 1) ) {
        memcpy( to, from, W * sizeof(float) );
    } else {
        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
            to[t] = from[f];
        }
    }
}

void
from_byte_packed(float *to,
                 const unsigned char *from,
                 const RectI &conversionRect,
                 const RectI &srcBounds,
                 const RectI &dstBounds,
                 PixelPackingEnum inputPacking,
                 PixelPackingEnum outputPacking,
                 bool invertY )

{
    if ( ( inputPacking == ePixelPackingPLANAR) || ( outputPacking == ePixelPackingPLANAR) ) {
        throw std::runtime_error("Invalid pixel format.");
    }

    ///clip the conversion rect to srcBounds and dstBounds
    RectI rect = conversionRect;
    if ( !clip(&rect,srcBounds) || !clip(&rect,dstBounds) ) {
        return;
    }


    bool inputHasAlpha = inputPacking == ePixelPackingBGRA || inputPacking == ePixelPackingRGBA;
    bool outputHasAlpha = outputPacking == ePixelPackingBGRA || outputPacking == ePixelPackingRGBA;
    int inROffset, inGOffset, inBOffset, inAOffset;
    int outROffset, outGOffset, outBOffset, outAOffset;
    getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
    getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);


    int inPackingSize,outPackingSize;
    inPackingSize = inputHasAlpha ? 4 : 3;
    outPackingSize = outputHasAlpha ? 4 : 3;


    for (int y = rect.y1; y < rect.y2; ++y) {
        int srcY = y;
        if (invertY) {
            srcY = srcBounds.y2 - y - 1;
        }
        const unsigned char *src_pixels = from + (srcY * (srcBounds.x2 - srcBounds.x1) * inPackingSize);
        float *dst_pixels = to + (y * (dstBounds.x2 - dstBounds.x1) * outPackingSize);
        for (int x = rect.x1; x < rect.x2; ++x) {
            int inCol = x * inPackingSize;
            int outCol = x * outPackingSize;
            unsigned char a = inputHasAlpha ? src_pixels[inCol + inAOffset] : 255;
            dst_pixels[outCol + outROffset] = Color::intToFloat<256>(src_pixels[inCol + inROffset]);
            dst_pixels[outCol + outGOffset] = Color::intToFloat<256>(src_pixels[inCol + inGOffset]);
            dst_pixels[outCol + outBOffset] = Color::intToFloat<256>(src_pixels[inCol + inBOffset]);
            if (outputHasAlpha) {
                // alpha is linear
                dst_pixels[outCol + outAOffset] = Color::intToFloat<256>(a);
            }
        }
    }
}

void
from_short_packed(float */*to*/,
                  const unsigned short */*from*/,
                  const RectI & /*rect*/,
                  const RectI & /*srcRod*/,
                  const RectI & /*rod*/,
                  PixelPackingEnum /*inputFormat*/,
                  PixelPackingEnum /*format*/,
                  bool /*invertY*/)
{
    throw std::runtime_error("Linear::from_short_packed not yet implemented.");
}

void
from_float_packed(float *to,
                  const float *from,
                  const RectI &conversionRect,
                  const RectI &srcBounds,
                  const RectI &dstBounds,
                  PixelPackingEnum inputPacking,
                  PixelPackingEnum outputPacking,
                  bool invertY)
{
    if ( ( inputPacking == ePixelPackingPLANAR) || ( outputPacking == ePixelPackingPLANAR) ) {
        throw std::runtime_error("This function is not meant for planar buffers.");
    }


    ///clip the conversion rect to srcBounds and dstBounds
    RectI rect = conversionRect;
    if ( !clip(&rect,srcBounds) || !clip(&rect,dstBounds) ) {
        return;
    }


    if ( ( inputPacking == ePixelPackingPLANAR) || ( outputPacking == ePixelPackingPLANAR) ) {
        throw std::runtime_error("Invalid pixel format.");
    }

    bool inputHasAlpha = inputPacking == ePixelPackingBGRA || inputPacking == ePixelPackingRGBA;
    bool outputHasAlpha = outputPacking == ePixelPackingBGRA || outputPacking == ePixelPackingRGBA;
    int inROffset, inGOffset, inBOffset, inAOffset;
    int outROffset, outGOffset, outBOffset, outAOffset;
    getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
    getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);

    int inPackingSize,outPackingSize;
    inPackingSize = inputHasAlpha ? 4 : 3;
    outPackingSize = outputHasAlpha ? 4 : 3;

    for (int y = rect.y1; y < rect.y2; ++y) {
        int srcY = y;
        if (invertY) {
            srcY = srcBounds.y2 - y - 1;
        }
        const float *src_pixels = from + (srcY * (srcBounds.x2 - srcBounds.x1) * inPackingSize);
        float *dst_pixels = to + (y * (dstBounds.x2 - dstBounds.x1) * outPackingSize);
        if (inputPacking == outputPacking) {
            memcpy( dst_pixels, src_pixels,(rect.x2 - rect.x1) * sizeof(float) );
        } else {
            for (int x = rect.x1; x < rect.x2; ++x) {
                int inCol = x * inPackingSize;
                int outCol = x * outPackingSize;
                float a = inputHasAlpha ? src_pixels[inCol + inAOffset] : 1.f;
                dst_pixels[outCol + outROffset] = src_pixels[inCol + inROffset];
                dst_pixels[outCol + outGOffset] = src_pixels[inCol + inGOffset];
                dst_pixels[outCol + outBOffset] = src_pixels[inCol + inBOffset];
                if (outputHasAlpha) {
                    // alpha is linear
                    dst_pixels[outCol + outAOffset] = a;
                }
            }
        }
    }
} // from_float_packed

#if 0
void
to_byte_planar(unsigned char *to,
               const float *from,
               int W,
               const float* alpha,
               int inDelta,
               int outDelta)
{
    if (!alpha) {
        unsigned char *end = to + W * outDelta;
        // coverity[dont_call]
        int start = rand() % W;
        const float *q;
        unsigned char *p;
        /* go fowards from starting point to end of line: */
        float error = .5;
        for (p = to + start * outDelta, q = from + start * inDelta; p < end; p += outDelta, q += inDelta) {
            float G = error + *q * 255.0f;
            if (G <= 0) {
                *p = 0;
            } else if (G < 255) {
                int i = (int)G;
                *p = (unsigned char)i;
                error = G - i;
            } else {
                *p = 255;
            }
        }
        /* go backwards from starting point to start of line: */
        error = .5;
        for (p = to + (start - 1) * outDelta, q = from + start * inDelta; p >= to; p -= outDelta) {
            q -= inDelta;
            float G = error + *q * 255.0f;
            if (G <= 0) {
                *p = 0;
            } else if (G < 255) {
                int i = (int)G;
                *p = (unsigned char)i;
                error = G - i;
            } else {
                *p = 255;
            }
        }
    } else {
        unsigned char *end = to + W * outDelta;
        // coverity[dont_call]
        int start = rand() % W;
        const float *q;
        const float *a = alpha;
        unsigned char *p;
        /* go fowards from starting point to end of line: */
        float error = .5;
        for (p = to + start * outDelta, q = from + start * inDelta, a += start * inDelta; p < end;
             p += outDelta, q += inDelta, a += inDelta) {
            float v = *q * *a;
            float G = error + v * 255.0f;
            if (G <= 0) {
                *p = 0;
            } else if (G < 255) {
                int i = (int)G;
                *p = (unsigned char)i;
                error = G - i;
            } else {
                *p = 255;
            }
        }
        /* go backwards from starting point to start of line: */
        error = .5;
        for (p = to + (start - 1) * outDelta, q = from + start * inDelta, a = alpha + start * inDelta; p >= to; p -= outDelta) {
            q -= inDelta;
            a -= inDelta;
            const float v = *q * *a;
            float G = error + v * 255.0f;
            if (G <= 0) {
                *p = 0;
            } else if (G < 255) {
                int i = (int)G;
                *p = (unsigned char)i;
                error = G - i;
            } else {
                *p = 255;
            }
        }
    }
} // to_byte_planar
#endif // DEAD_CODE

#ifdef DEAD_CODE
void
to_short_planar(unsigned short *to,
                const float *from,
                int W,
                const float* alpha,
                int inDelta,
                int outDelta)
{
    Q_UNUSED(to);
    Q_UNUSED(from);
    Q_UNUSED(W);
    Q_UNUSED(alpha);
    Q_UNUSED(inDelta);
    Q_UNUSED(outDelta);
    throw std::runtime_error("Linear::to_short_planar not yet implemented.");
}
#endif // DEAD_CODE

#ifdef DEAD_CODE
void
to_float_planar(float *to,
                const float *from,
                int W,
                const float* alpha,
                int inDelta,
                int outDelta)
{
    if (!alpha) {
        if ( ( inDelta == 1) && ( outDelta == 1) ) {
            memcpy( to, from, W * sizeof(float) );
        } else {
            for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
                to[t] = from[f];
            }
        }
    } else {
        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
            to[t] = from[f] * alpha[f];
        }
    }
}
#endif // DEAD_CODE

#ifdef DEAD_CODE
void
to_byte_packed(unsigned char* to,
               const float* from,
               const RectI & conversionRect,
               const RectI & srcBounds,
               const RectI & dstBounds,
               PixelPackingEnum inputPacking,
               PixelPackingEnum outputPacking,
               bool invertY,
               bool premult)
{
    if ( ( inputPacking == ePixelPackingPLANAR) || ( outputPacking == ePixelPackingPLANAR) ) {
        throw std::runtime_error("This function is not meant for planar buffers.");
    }

    ///clip the conversion rect to srcBounds and dstBounds
    RectI rect = conversionRect;
    if ( !clip(&rect,srcBounds) || !clip(&rect,dstBounds) ) {
        return;
    }


    bool inputHasAlpha = inputPacking == ePixelPackingBGRA || inputPacking == ePixelPackingRGBA;
    bool outputHasAlpha = outputPacking == ePixelPackingBGRA || outputPacking == ePixelPackingRGBA;
    int inROffset, inGOffset, inBOffset, inAOffset;
    int outROffset, outGOffset, outBOffset, outAOffset;
    getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
    getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);

    int inPackingSize,outPackingSize;
    inPackingSize = inputHasAlpha ? 4 : 3;
    outPackingSize = outputHasAlpha ? 4 : 3;

    for (int y = rect.y1; y < rect.y2; ++y) {
        // coverity[dont_call]
        int start = rand() % (rect.x2 - rect.x1) + rect.x1;
        unsigned error_r, error_g, error_b;
        error_r = error_g = error_b = 0x80;
        int srcY = y;
        if (invertY) {
            srcY = srcBounds.y2 - y - 1;
        }

        const float *src_pixels = from + (srcY * (srcBounds.x2 - srcBounds.x1) * inPackingSize);
        unsigned char *dst_pixels = to + (y * (dstBounds.x2 - dstBounds.x1) * outPackingSize);
        /* go fowards from starting point to end of line: */
        for (int x = start; x < rect.x2; ++x) {
            int inCol = x * inPackingSize;
            int outCol = x * outPackingSize;
            float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;

            error_r = (error_r & 0xff) + src_pixels[inCol + inROffset] * a * 255.f;
            error_g = (error_g & 0xff) + src_pixels[inCol + inGOffset] * a * 255.f;
            error_b = (error_b & 0xff) + src_pixels[inCol + inBOffset] * a * 255.f;
            dst_pixels[outCol + outROffset] = (unsigned char)(error_r >> 8);
            dst_pixels[outCol + outGOffset] = (unsigned char)(error_g >> 8);
            dst_pixels[outCol + outBOffset] = (unsigned char)(error_b >> 8);
            if (outputHasAlpha) {
                dst_pixels[outCol + outAOffset] = floatToInt<256>(a);
            }
        }
        /* go backwards from starting point to start of line: */
        error_r = error_g = error_b = 0x80;
        for (int x = start - 1; x >= rect.x1; --x) {
            int inCol = x * inPackingSize;
            int outCol = x * outPackingSize;
            float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;

            error_r = (error_r & 0xff) + src_pixels[inCol + inROffset] * a * 255.f;
            error_g = (error_g & 0xff) + src_pixels[inCol + inGOffset] * a * 255.f;
            error_b = (error_b & 0xff) + src_pixels[inCol + inBOffset] * a * 255.f;
            dst_pixels[outCol + outROffset] = (unsigned char)(error_r >> 8);
            dst_pixels[outCol + outGOffset] = (unsigned char)(error_g >> 8);
            dst_pixels[outCol + outBOffset] = (unsigned char)(error_b >> 8);
            if (outputHasAlpha) {
                dst_pixels[outCol + outAOffset] = floatToInt<256>(a);
            }
        }
    }
} // to_byte_packed
#endif // DEAD_CODE

#ifdef DEAD_CODE
void
to_short_packed(unsigned short* to,
                const float* from,
                const RectI & conversionRect,
                const RectI & srcBounds,
                const RectI & dstBounds,
                PixelPackingEnum inputPacking,
                PixelPackingEnum outputPacking,
                bool invertY,
                bool premult)
{
    Q_UNUSED(to);
    Q_UNUSED(from);
    Q_UNUSED(conversionRect);
    Q_UNUSED(srcBounds);
    Q_UNUSED(dstBounds);
    Q_UNUSED(invertY);
    Q_UNUSED(premult);
    Q_UNUSED(inputPacking);
    Q_UNUSED(outputPacking);
    throw std::runtime_error("Linear::to_short_packed not yet implemented.");
}
#endif // DEAD_CODE

void
to_float_packed(float* to,
                const float* from,
                const RectI & conversionRect,
                const RectI & srcBounds,
                const RectI & dstBounds,
                PixelPackingEnum inputPacking,
                PixelPackingEnum outputPacking,
                bool invertY,
                bool premult)
{
    if ( ( inputPacking == ePixelPackingPLANAR) || ( outputPacking == ePixelPackingPLANAR) ) {
        throw std::runtime_error("Invalid pixel format.");
    }

    ///clip the conversion rect to srcBounds and dstBounds
    RectI rect = conversionRect;
    if ( !clip(&rect,srcBounds) || !clip(&rect,dstBounds) ) {
        return;
    }


    bool inputHasAlpha = inputPacking == ePixelPackingBGRA || inputPacking == ePixelPackingRGBA;
    bool outputHasAlpha = outputPacking == ePixelPackingBGRA || outputPacking == ePixelPackingRGBA;
    int inROffset, inGOffset, inBOffset, inAOffset;
    int outROffset, outGOffset, outBOffset, outAOffset;
    getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
    getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);


    int inPackingSize,outPackingSize;
    inPackingSize = inputHasAlpha ? 4 : 3;
    outPackingSize = outputHasAlpha ? 4 : 3;

    for (int y = rect.y1; y < rect.y2; ++y) {
        int srcY = y;
        if (invertY) {
            srcY = srcBounds.y2 - y - 1;
        }

        const float *src_pixels = from + (srcY * (srcBounds.x2 - srcBounds.x1) * inPackingSize);
        float *dst_pixels = to + (y * (dstBounds.x2 - dstBounds.x1) * outPackingSize);
        if ( ( inputPacking == outputPacking) && !premult ) {
            memcpy( dst_pixels, src_pixels, (rect.x2 - rect.x1) * sizeof(float) );
        } else {
            for (int x = rect.x1; x < conversionRect.x2; ++x) {
                int inCol = x * inPackingSize;
                int outCol = x * outPackingSize;
                float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;

                dst_pixels[outCol + outROffset] = src_pixels[inCol + inROffset] * a;
                dst_pixels[outCol + outGOffset] = src_pixels[inCol + inGOffset] * a;
                dst_pixels[outCol + outBOffset] = src_pixels[inCol + inBOffset] * a;
                if (outputHasAlpha) {
                    dst_pixels[outCol + outAOffset] = a;
                }
            }
        }
    }
} // to_float_packed
}

const Lut*
LutManager::sRGBLut()
{
    return LutManager::m_instance.getLut("sRGB",from_func_srgb,to_func_srgb);
}

static
float
from_func_Rec709(float v)
{
    if (v < 0.081f) {
        return (v < 0.0f) ? 0.0f : v * (1.0f / 4.5f);
    } else {
        return std::pow( (v + 0.099f) * (1.0f / 1.099f), (1.0f / 0.45f) );
    }
}

static
float
to_func_Rec709(float v)
{
    if (v < 0.018f) {
        return (v < 0.0f) ? 0.0f : v * 4.5f;
    } else {
        return 1.099f * std::pow(v, 0.45f) - 0.099f;
    }
}

const Lut*
LutManager::Rec709Lut()
{
    return LutManager::m_instance.getLut("Rec709",from_func_Rec709,to_func_Rec709);
}

/*
   Following the formula:
   offset = pow(10,(blackpoint - whitepoint) * 0.002 / gammaSensito)
   gain = 1/(1-offset)
   linear = gain * pow(10,(1023*v - whitepoint)*0.002/gammaSensito)
   cineon = (log10((v + offset) /gain)/ (0.002 / gammaSensito) + whitepoint)/1023
   Here we're using: blackpoint = 95.0
   whitepoint = 685.0
   gammasensito = 0.6
 */
static
float
from_func_Cineon(float v)
{
    return ( 1.f / ( 1.f - std::pow(10.f,1.97f) ) ) * std::pow(10.f,( (1023.f * v) - 685.f ) * 0.002f / 0.6f);
}

static
float
to_func_Cineon(float v)
{
    float offset = std::pow(10.f,1.97f);

    return (std::log10( (v + offset) / ( 1.f / (1.f - offset) ) ) / 0.0033f + 685.0f) / 1023.f;
}

const Lut*
LutManager::CineonLut()
{
    return LutManager::m_instance.getLut("Cineon",from_func_Cineon,to_func_Cineon);
}

static
float
from_func_Gamma1_8(float v)
{
    return std::pow(v, 0.55f);
}

static
float
to_func_Gamma1_8(float v)
{
    return std::pow(v, 1.8f);
}

const Lut*
LutManager::Gamma1_8Lut()
{
    return LutManager::m_instance.getLut("Gamma1_8",from_func_Gamma1_8,to_func_Gamma1_8);
}

static
float
from_func_Gamma2_2(float v)
{
    return std::pow(v, 0.45f);
}

static
float
to_func_Gamma2_2(float v)
{
    return std::pow(v, 2.2f);
}

const Lut*
LutManager::Gamma2_2Lut()
{
    return LutManager::m_instance.getLut("Gamma2_2",from_func_Gamma2_2,to_func_Gamma2_2);
}

static
float
from_func_Panalog(float v)
{
    return (std::pow(10.f,(1023.f * v - 681.f) / 444.f) - 0.0408) / 0.96f;
}

static
float
to_func_Panalog(float v)
{
    return (444.f * std::log10(0.0408 + 0.96f * v) + 681.f) / 1023.f;
}

const Lut*
LutManager::PanaLogLut()
{
    return LutManager::m_instance.getLut("PanaLog",from_func_Panalog,to_func_Panalog);
}

static
float
from_func_ViperLog(float v)
{
    return std::pow(10.f,(1023.f * v - 1023.f) / 500.f);
}

static
float
to_func_ViperLog(float v)
{
    return (500.f * std::log10(v) + 1023.f) / 1023.f;
}

const Lut*
LutManager::ViperLogLut()
{
    return LutManager::m_instance.getLut("ViperLog",from_func_ViperLog,to_func_ViperLog);
}

static
float
from_func_RedLog(float v)
{
    return (std::pow(10.f,( 1023.f * v - 1023.f ) / 511.f) - 0.01f) / 0.99f;
}

static
float
to_func_RedLog(float v)
{
    return (511.f * std::log10(0.01f + 0.99f * v) + 1023.f) / 1023.f;
}

const Lut*
LutManager::RedLogLut()
{
    return LutManager::m_instance.getLut("RedLog",from_func_RedLog,to_func_RedLog);
}

static
float
from_func_AlexaV3LogC(float v)
{
    return v > 0.1496582f ? std::pow(10.f,(v - 0.385537f) / 0.2471896f) * 0.18f - 0.00937677f
           : ( v / 0.9661776f - 0.04378604) * 0.18f - 0.00937677f;
}

static
float
to_func_AlexaV3LogC(float v)
{
    return v > 0.010591f ?  0.247190f * std::log10(5.555556f * v + 0.052272f) + 0.385537f
           : v * 5.367655f + 0.092809f;
}

const Lut*
LutManager::AlexaV3LogCLut()
{
    return LutManager::m_instance.getLut("AlexaV3LogC",from_func_AlexaV3LogC,to_func_AlexaV3LogC);
}

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = 0 (undefined)
void
rgb_to_hsv( float r,
            float g,
            float b,
            float *h,
            float *s,
            float *v )
{
    float min, max, delta;

    min = std::min(std::min(r, g), b);
    max = std::max(std::max(r, g), b);
    *v = max;                       // v

    delta = max - min;

    if (max != 0.) {
        *s = delta / max;               // s
    } else {
        // r = g = b = 0		// s = 0, v is undefined
        *s = 0.;
        *h = 0.;

        return;
    }

    if (delta == 0.) {
        *h = 0.;         // gray
    } else if (r == max) {
        *h = (g - b) / delta;               // between yellow & magenta
    } else if (g == max) {
        *h = 2 + (b - r) / delta;           // between cyan & yellow
    } else {
        *h = 4 + (r - g) / delta;           // between magenta & cyan
    }
    *h *= 60;                       // degrees
    if (*h < 0) {
        *h += 360;
    }
}
}     // namespace Color {
} // namespace Natron {

