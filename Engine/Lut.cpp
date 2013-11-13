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

#include "Lut.h"

#include <stdint.h>
#include <cassert>
#include <iostream>
#include <map>
#include <cstring>
#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

using std::cout;
using std::endl;
using std::make_pair;

#ifndef FLT_MAX
# define FLT_MAX 3.40282347e+38F
#endif

using namespace Natron;
using namespace Natron::Color;

namespace Natron
{
namespace Color
{

namespace Linear
{
inline float fromFloat(float v)
{
    return v;
}
inline float toFloat(float v)
{
    return v;
}
inline float toFloatFast(float v)
{
    return v;
}
} // namespace Linear


namespace
{

class sRGB: public Lut
{
public:
    sRGB(): Lut() {}

private:
    virtual bool linear() const {
        return false;
    }

    virtual float to_byte(float v) const {
        return linearrgb_to_srgb(v) * 255.f;
    }

    virtual float from_byte(float v) const {
        return srgb_to_linearrgb(v / 255.f);
    }
};

class Rec709: public Lut
{
public:
    Rec709() : Lut() {}

private:
    virtual bool linear() const {
        return false;
    }

    virtual float to_byte(float v) const {
        return linearrgb_to_rec709(v) * 255.f;
    }

    virtual float from_byte(float v) const {
        return rec709_to_linearrgb(v / 255.f);
    }
};

class LinearLut: public Lut
{
public:
    LinearLut() : Lut() {}

private:
    virtual bool linear() const {
        return true;
    }

    float to_byte(float v) const {
        return v * 255.f;
    }
    float from_byte(float v) const {
        return v / 255.f;
    }
};


// compile-time endianness checking found on:
// http://stackoverflow.com/questions/2100331/c-macro-definition-to-determine-big-endian-or-little-endian-machine
// if(O32_HOST_ORDER == O32_BIG_ENDIAN) will always be optimized by gcc -O2
enum {
    O32_LITTLE_ENDIAN = 0x03020100ul,
    O32_BIG_ENDIAN = 0x00010203ul,
    O32_PDP_ENDIAN = 0x01000302ul
};
static const union {
    uint8_t bytes[4];
    uint32_t value;
} o32_host_order = { { 0, 1, 2, 3 } };
#define O32_HOST_ORDER (o32_host_order.value)

static U16 hipart(const float f)
{
    union {
        float f;
        unsigned short us[2];
    } tmp;
    tmp.us[0] = tmp.us[1] = 0;
    tmp.f = f;

    if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
        return tmp.us[0];
    } else if (O32_HOST_ORDER == O32_LITTLE_ENDIAN) {
        return tmp.us[1];
    } else {
        assert((O32_HOST_ORDER == O32_LITTLE_ENDIAN) || (O32_HOST_ORDER == O32_BIG_ENDIAN));
        return 0;
    }
}

static float index_to_float(const U16 i)
{
    union {
        float f;
        unsigned short us[2];
    } tmp;
    /* positive and negative zeros, and all gradual underflow, turn into zero: */
    if (i < 0x80 || (i >= 0x8000 && i < 0x8080)) {
        return 0;
    }
    /* All NaN's and infinity turn into the largest possible legal float: */
    if (i >= 0x7f80 && i < 0x8000) {
        return FLT_MAX;
    }
    if (i >= 0xff80) {
        return -FLT_MAX;
    }
    if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
        tmp.us[0] = i;
        tmp.us[1] = 0x8000;
    } else if (O32_HOST_ORDER == O32_LITTLE_ENDIAN) {
        tmp.us[0] = 0x8000;
        tmp.us[1] = i;
    } else {
        assert((O32_HOST_ORDER == O32_LITTLE_ENDIAN) || (O32_HOST_ORDER == O32_BIG_ENDIAN));
    }

    return tmp.f;
}

} // namespace {

void Lut::fillTables() const
{
    // float increment = 1.f /65535.f;
    if (init_) {
        return;
    }
    //bool linearTrue = true; // linear() is a class property, no need to test it
    for (int i = 0; i < 0x10000; ++i) {
        float inp = index_to_float((U16)i);
        float f = to_byte(inp);
        //if(!isEqual_float(f, inp*255.f, 10000))
        //    linearTrue = false;
        if (f <= 0) {
            to_byte_table[i] = 0;
        } else if (f < 255) {
            to_byte_table[i] = (unsigned short)(f * 0x100 + .5);
        } else {
            to_byte_table[i] = 0xff00;
        }
    }

    for (int b = 0; b <= 255; ++b) {
        float f = from_byte((float)b);
        from_byte_table[b] = f;
        int i = hipart(f);
        to_byte_table[i] = (U16)(b * 0x100);
    }

}

float Lut::fromFloatFast(float v) const
{
    return from_byte_table[(int)v];
}

float Lut::toFloatFast(float v) const
{
    return (float)to_byte_table[hipart(v)];
}

void Lut::from_byte(float *to, const uchar *from, int W, int delta) const
{
    assert(!linear());
    validate();
    for (int i = 0 ; i < W ; i += delta) {
        to[i] = from_byte_table[(int)from[i]];
    }
}

void Lut::from_byte(float *to, const uchar *from, const uchar *alpha, int W, int delta) const
{
    assert(!linear());
    validate();
    for (int i = 0 ; i < W ; i += delta) {
        float a = (float)alpha[i];
        to[i] = from_byte_table[(int)from[i] / (int)a] * a;
    }
}

void Lut::from_byteQt(float *to, const QRgb *from, Natron::Channel z, bool premult, int W, int delta) const
{
    assert(!linear());
    validate();
    typedef int(*ChannelLookup)(QRgb);
    ChannelLookup lookup;
    if (z == Channel_alpha) {
        lookup = qAlpha;
    } else if (z == Channel_red) {
        lookup = qRed;
    }

    else if (z == Channel_green) {
        lookup = qGreen;
    }

    else if (z == Channel_blue) {
        lookup = qBlue;
    } else {
        lookup = qRed;
    }
    if (z == Natron::Channel_alpha) {
        linear_from_byteQt(to, from, z, W, delta);
    } else {
        if (premult) {
            for (int i = 0 ; i < W ; i += delta) {
                const QRgb c = from[i];
                int alpha = qAlpha(c);
                to[i] = from_byte_table[((*lookup)(c) / alpha)] * alpha;
            }
        } else {
            for (int i = 0 ; i < W ; i += delta) {
                const QRgb c = from[i];
                to[i] = from_byte_table[(*lookup)(c)];
            }
        }
    }
}

void Lut::from_short(float *to, const U16 *from, int W, int bits , int delta) const
{
    assert(!linear());
    validate();
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "Lut::from_short not yet implemented" << endl;
}

void Lut::from_short(float *to, const U16 *from, const U16 *alpha, int W, int bits, int delta) const
{
    assert(!linear());
    validate();
    (void)to;
    (void)from;
    (void)alpha;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "Lut::from_short not yet implemented" << endl;
}

void Lut::from_float(float *to, const float *from, int W, int delta) const
{
    assert(!linear());
    validate();
    for (int i = 0; i < W; i += delta) {
        to[i] = fromFloatFast(from[i] * 255.f);
    }
}

void Lut::from_float(float *to, const float *from, const float *alpha, int W, int delta) const
{
    assert(!linear());
    validate();
    for (int i = 0; i < W; i += delta) {
        float a = alpha[i];
        to[i] = fromFloatFast((from[i] / a) * 255.f) * a * 255.f;
    }

}

void getOffsetsForPacking(Lut::PackedPixelsFormat format, int *r, int *g, int *b, int *a)
{
    if (format == Lut::BGRA) {
        *b = 0;
        *g = 1;
        *r = 2;
        *a = 3;
    }else if(format == Lut::RGBA){
        *r = 0;
        *g = 1;
        *b = 2;
        *a = 3;
    }else{
        *r = -1;
        *g = -1;
        *b = -1;
        *a = -1;
        assert(false);
    }
}

void Lut::from_byte_rect(float *to, const uchar *from,
                         const RectI &rect, const RectI &rod,
                         bool invertY , bool premult,
                         Lut::PackedPixelsFormat format) const
{
    int rOffset, gOffset, bOffset, aOffset;
    getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
    assert(!linear());
    validate();
    if (!premult) {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int srcY = y;
            if (invertY) {
                srcY = rod.top() - y - 1;
            }
            const uchar *src_pixels = from + (srcY * rod.width() * 4);
            float *dst_pixels = to + (y * rod.width() * 4);
            for (int x = rect.left(); x < rect.right(); ++x) {
                int col = x * 4;
                dst_pixels[col] = from_byte_table[(int)src_pixels[col + rOffset]];
                dst_pixels[col + 1] = from_byte_table[(int)src_pixels[col + gOffset]];
                dst_pixels[col + 2] = from_byte_table[(int)src_pixels[col + bOffset]];
                dst_pixels[col + 3] = (float)src_pixels[col + aOffset] / 255.f;
            }
        }
    } else {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int srcY = y;
            if (invertY) {
                srcY = rod.top() - y - 1;
            }
            const uchar *src_pixels = from + (srcY * rod.width() * 4);
            float *dst_pixels = to + (y * rod.width() * 4);
            for (int x = rect.left(); x < rect.right(); ++x) {
                int col = x * 4;
                uchar a = src_pixels[col + aOffset];
                dst_pixels[col] = from_byte_table[(int)src_pixels[col + rOffset] / a] * a;
                dst_pixels[col + 1] = from_byte_table[(int)src_pixels[col + gOffset] / a] * a;
                dst_pixels[col + 2] = from_byte_table[(int)src_pixels[col + bOffset] / a] * a;
                dst_pixels[col + 3] = (float)a / 255.f;
            }
        }
    }


}

void Lut::from_short_rect(float *to, const U16 *from,
                          const RectI &rect, const RectI &rod,
                          bool invertY , bool premult,
                          Lut::PackedPixelsFormat format) const
{
    (void)to;
    (void)from;
    (void)rect;
    (void)rod;
    (void)invertY;
    (void)premult;
    (void)format;
    cout << "Lut::from_short_rect not yet implemented." << endl;
}

void Lut::from_float_rect(float *to, const float *from,
                          const RectI &rect, const RectI &rod,
                          bool invertY , bool premult,
                          Lut::PackedPixelsFormat format) const
{
    int rOffset, gOffset, bOffset, aOffset;
    getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
    assert(!linear());
    validate();
    if (!premult) {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int srcY = y;
            if (invertY) {
                srcY = rod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * rod.width() * 4);
            float *dst_pixels = to + (y * rod.width() * 4);
            for (int x = rect.left(); x < rect.right(); ++x) {
                int col = x * 4;
                dst_pixels[col] = fromFloatFast(src_pixels[col + rOffset] * 255.f);
                dst_pixels[col + 1] = fromFloatFast(src_pixels[col + gOffset] * 255.f);
                dst_pixels[col + 2] = fromFloatFast(src_pixels[col + bOffset] * 255.f);
                dst_pixels[col + 3] = src_pixels[col + aOffset];

            }
        }
    } else {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int srcY = y;
            if (invertY) {
                srcY = rod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * rod.width() * 4);
            float *dst_pixels = to + (y * rod.width() * 4);
            for (int x = rect.left(); x < rect.right(); ++x) {
                int col = x * 4;
                float a = src_pixels[col + aOffset];
                dst_pixels[col] = fromFloatFast(src_pixels[col + rOffset] / a * 255.f) * a * 255.f;
                dst_pixels[col + 1] = fromFloatFast(src_pixels[col + gOffset] / a * 255.f) * a * 255.f;
                dst_pixels[col + 2] = fromFloatFast(src_pixels[col + bOffset] / a * 255.f) * a * 255.f;
                dst_pixels[col + 3] = a;
            }
        }

    }
}

void Lut::to_byte(uchar *to, const float *from, int W, int delta) const
{
    assert(!linear());
    validate();
    uchar *end = to + W * delta;
    int start = rand() % W;
    const float *q;
    uchar *p;
    unsigned error;
    validate();

    /* go fowards from starting point to end of line: */
    error = 0x80;
    for (p = to + start * delta, q = from + start; p < end; p += delta) {
        error = (error & 0xff) + to_byte_table[hipart(*q)];
        ++q;
        *p = (uchar)(error >> 8);
    }
    /* go backwards from starting point to start of line: */
    error = 0x80;
    for (p = to + (start - 1) * delta, q = from + start; p >= to; p -= delta) {
        --q;
        error = (error & 0xff) + to_byte_table[hipart(*q)];
        *p = (uchar)(error >> 8);
    }
}

void Lut::to_byte(uchar *to, const float *from, const float *alpha, int W, int delta) const
{
    assert(!linear());
    validate();
    unsigned char *end = to + W * delta;
    int start = rand() % W;
    const float *q;
    const float *a = alpha;
    unsigned char *p;
    unsigned error;
    /* go fowards from starting point to end of line: */
    error = 0x80;
    for (p = to + start * delta, q = from + start, a += start; p < end; p += delta) {
        const float v = *q * *a;
        error = (error & 0xff) + to_byte_table[hipart(v)];
        ++q;
        ++a;
        *p = (uchar)(error >> 8);
    }
    /* go backwards from starting point to start of line: */
    error = 0x80;
    for (p = to + (start - 1) * delta, q = from + start , a = alpha + start; p >= to; p -= delta) {
        const float v = *q * *a;
        --q;
        --a;
        error = (error & 0xff) + to_byte_table[hipart(v)];
        *p = (uchar)(error >> 8);
    }

}

void Lut::to_short(U16 *to, const float *from, int W, int bits, int delta) const
{
    assert(!linear());
    validate();
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "Lut::to_short not implemented yet." << endl;
}

void Lut::to_short(U16 *to, const float *from, const float *alpha, int W, int bits, int delta) const
{
    assert(!linear());
    validate();
    (void)to;
    (void)from;
    (void)alpha;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "Lut::to_short not implemented yet." << endl;

}

void Lut::to_float(float *to, const float *from, int W, int delta) const
{
    assert(!linear());
    validate();
    for (int i = 0; i < W; i += delta) {
        to[i] = toFloatFast(from[i]);
    }
}

void Lut::to_float(float *to, const float *from, const float *alpha, int W, int delta) const
{
    assert(!linear());
    validate();
    for (int i = 0; i < W; i += delta) {
        to[i] = toFloatFast(from[i] * alpha[i]);
    }
}

void Lut::to_byte_rect(uchar *to, const float *from,
                       const RectI &rect, const RectI& srcRod,const RectI& dstRod,
                       bool invertY , bool premult,
                       Lut::PackedPixelsFormat format) const
{
    int rOffset, gOffset, bOffset, aOffset;
    getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
    assert(!linear());
    validate();
    if (!premult) {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int start = rand() % rect.width() + rect.left();
            unsigned error_r, error_g, error_b;
            error_r = error_g = error_b = 0x80;
            int srcY = y;
            if (invertY) {
                srcY = srcRod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * srcRod.width() * 4);
            uchar *dst_pixels = to + (y * dstRod.width() * 4);
            /* go fowards from starting point to end of line: */
            for (int x = start; x < rect.right(); ++x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                error_r = (error_r & 0xff) + to_byte_table[hipart(src_pixels[col])];
                error_g = (error_g & 0xff) + to_byte_table[hipart(src_pixels[col + 1])];
                error_b = (error_b & 0xff) + to_byte_table[hipart(src_pixels[col + 2])];
                dst_pixels[col + rOffset] = (uchar)(error_r >> 8);
                dst_pixels[col + gOffset] = (uchar)(error_g >> 8);
                dst_pixels[col + bOffset] = (uchar)(error_b >> 8);
                dst_pixels[col + aOffset] = (uchar)(std::min(a * 256.f, 255.f));
            }
            /* go backwards from starting point to start of line: */
            error_r = error_g = error_b = 0x80;
            for (int x = start - 1; x >= rect.left(); --x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                error_r = (error_r & 0xff) + to_byte_table[hipart(src_pixels[col])];
                error_g = (error_g & 0xff) + to_byte_table[hipart(src_pixels[col + 1])];
                error_b = (error_b & 0xff) + to_byte_table[hipart(src_pixels[col + 2])];
                dst_pixels[col + rOffset] = (uchar)(error_r >> 8);
                dst_pixels[col + gOffset] = (uchar)(error_g >> 8);
                dst_pixels[col + bOffset] = (uchar)(error_b >> 8);
                dst_pixels[col + aOffset] = (uchar)(std::min(a * 256.f, 255.f));
            }
        }
    } else {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int start = rand() % rect.width() + rect.left();
            unsigned error_r, error_g, error_b;
            error_r = error_g = error_b = 0x80;
            int srcY = y;
            if (invertY) {
                srcY = srcRod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * srcRod.width() * 4);
            uchar *dst_pixels = to + (y * dstRod.width() * 4);
            /* go fowards from starting point to end of line: */
            for (int x = start; x < rect.right(); ++x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                error_r = (error_r & 0xff) + to_byte_table[hipart(src_pixels[col] * a)];
                error_g = (error_g & 0xff) + to_byte_table[hipart(src_pixels[col + 1] * a)];
                error_b = (error_b & 0xff) + to_byte_table[hipart(src_pixels[col + 2] * a)];
                dst_pixels[col + rOffset] = (uchar)(error_r >> 8);
                dst_pixels[col + gOffset] = (uchar)(error_g >> 8);
                dst_pixels[col + bOffset] = (uchar)(error_b >> 8);
                dst_pixels[col + aOffset] = (uchar)(std::min(a * 256.f, 255.f));
            }
            /* go backwards from starting point to start of line: */
            error_r = error_g = error_b = 0x80;
            for (int x = start - 1; x >= rect.left(); --x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                error_r = (error_r & 0xff) + to_byte_table[hipart(src_pixels[col] * a)];
                error_g = (error_g & 0xff) + to_byte_table[hipart(src_pixels[col + 1] * a)];
                error_b = (error_b & 0xff) + to_byte_table[hipart(src_pixels[col + 2] * a)];
                dst_pixels[col + rOffset] = (uchar)(error_r >> 8);
                dst_pixels[col + gOffset] = (uchar)(error_g >> 8);
                dst_pixels[col + bOffset] = (uchar)(error_b >> 8);
                dst_pixels[col + aOffset] = (uchar)(std::min(a * 256.f, 255.f));
            }
        }

    }

}

void Lut::to_short_rect(U16 *to, const float *from,
                        const RectI &rect, const RectI& srcRod,const RectI& dstRod,
                        bool invertY , bool premult,
                        Lut::PackedPixelsFormat format) const
{
    (void)to;
    (void)from;
    (void)rect;
    (void)srcRod;
    (void)dstRod;
    (void)invertY;
    (void)premult;
    (void)format;
    cout << "Lut::to_short_rect not yet implemented." << endl;
}

void Lut::to_float_rect(float *to, const float *from,
                        const RectI &rect,const RectI& srcRod,const RectI& dstRod,
                        bool invertY , bool premult,
                        Lut::PackedPixelsFormat format) const
{
    int rOffset, gOffset, bOffset, aOffset;
    getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
    assert(!linear());
    validate();
    if (!premult) {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int srcY = y;
            if (invertY) {
                srcY = srcRod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * srcRod.width() * 4);
            float *dst_pixels = to + (y * dstRod.width() * 4);
            /* go fowards from starting point to end of line: */
            for (int x = rect.left(); x < rect.right(); ++x) {
                int col = x * 4;
                dst_pixels[col] = toFloatFast(src_pixels[col]);
                dst_pixels[col + rOffset] = toFloatFast(src_pixels[col + 1]);
                dst_pixels[col + gOffset] = toFloatFast(src_pixels[col + 2]);
                dst_pixels[col + bOffset] = src_pixels[col + 3];
            }
        }
    } else {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int srcY = y;
            if (invertY) {
                srcY = srcRod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * srcRod.width() * 4);
            float *dst_pixels = to + (y * dstRod.width() * 4);
            /* go fowards from starting point to end of line: */
            for (int x = rect.left(); x < rect.right(); ++x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                dst_pixels[col + rOffset] = toFloatFast(src_pixels[col] * a);
                dst_pixels[col + gOffset] = toFloatFast(src_pixels[col + 1] * a);
                dst_pixels[col + bOffset] = toFloatFast(src_pixels[col + 2] * a);
                dst_pixels[col + aOffset] = a;
            }
        }
    }
}

void linear_from_byte(float *to, const uchar *from, int W, int delta)
{
    from += (W - 1) * delta;
    to += W;
    for (; --W >= 0; from -= delta) {
        *--to = *from * (1.0f / 255.f);
    }
}

void linear_from_byteQt(float *to, const QRgb *from, Natron::Channel z, int W, int delta)
{
    typedef int(*ChannelLookup)(QRgb);
    ChannelLookup lookup;
    switch (z) {
    case Natron::Channel_alpha:
        lookup = qAlpha;
        break;
    case Natron::Channel_red:
        lookup = qRed;
        break;
    case Natron::Channel_green:
        lookup = qGreen;
        break;
    case Natron::Channel_blue:
        lookup = qBlue;
        break;
    default:
        lookup = qRed;
        break;
    }
    if (z == Natron::Channel_alpha) {
        for (int i = 0 ; i < W ; i += delta) {
            const QRgb c = from[i];
            to[i] = (float)qAlpha(c) / 255.f;
        }
    } else {

        for (int i = 0 ; i < W ; i += delta) {
            const QRgb c = from[i];
            to[i] = (float)(*lookup)(c);
        }
    }

}

void linear_from_short(float *to, const U16 *from, int W, int bits, int delta)
{
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "linear_from_short not yet implemented" << endl;
}

void linear_from_float(float *to, const float *from, int W, int delta)
{
    if(delta == 1){
        memcpy(to, from, W*sizeof(float));
    }else{
        for (int i = 0; i < W; i += delta) {
            to[i] = from[i];
        }
    }
}

void linear_from_byte_rect(float *to, const uchar *from,
                           const RectI &rect, const RectI &rod,
                           bool invertY ,
                           Lut::PackedPixelsFormat format)
{
    int rOffset, gOffset, bOffset, aOffset;
    getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
    for (int y = rect.bottom(); y < rect.top(); ++y) {
        int srcY = y;
        if (invertY) {
            srcY = rod.top() - y - 1;
        }
        const uchar *src_pixels = from + (srcY * rod.width() * 4);
        float *dst_pixels = to + (y * rod.width() * 4);
        for (int x = rect.left(); x < rect.right(); ++x) {
            int col = x * 4;
            dst_pixels[col] = src_pixels[col + rOffset] / 255.f;
            dst_pixels[col + 1] = src_pixels[col + gOffset] / 255.f;
            dst_pixels[col + 2] = src_pixels[col + bOffset] / 255.f;
            dst_pixels[col + 3] = src_pixels[col + aOffset] / 255.f;
        }
    }

}

void linear_from_short_rect(float *to, const U16 *from,
                            const RectI &rect, const RectI &rod,
                            bool invertY ,
                            Lut::PackedPixelsFormat format)
{
    (void)to;
    (void)from;
    (void)rect;
    (void)rod;
    (void)invertY;
    (void)format;
    cout << "linear_from_short_rect not yet implemented." << endl;
}

void linear_from_float_rect(float *to, const float *from,
                            const RectI &rect, const RectI &rod,
                            bool invertY ,
                            Lut::PackedPixelsFormat format)
{

    for (int y = rect.bottom(); y < rect.top(); ++y) {
        int srcY = y;
        if (invertY) {
            srcY = rod.top() - y - 1;
        }
        const float *src_pixels = from + (srcY * rod.width() * 4);
        float *dst_pixels = to + (y * rod.width() * 4);
        if (format == Lut::RGBA) {
            memcpy(dst_pixels, src_pixels, rect.width()*sizeof(float));
        } else {
            int rOffset, gOffset, bOffset, aOffset;
            getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
            for (int x = rect.left(); x < rect.right(); ++x) {
                int col = x * 4;
                dst_pixels[col] = src_pixels[col + rOffset];
                dst_pixels[col + 1] = src_pixels[col + gOffset];
                dst_pixels[col + 2] = src_pixels[col + bOffset];
                dst_pixels[col + 3] = src_pixels[col + aOffset];
            }
        }
    }
}

void linear_to_byte(uchar *to, const float *from, int W, int delta)
{
    unsigned char *end = to + W * delta;
    int start = rand() % W;
    const float *q;
    unsigned char *p;
    /* go fowards from starting point to end of line: */
    float error = .5;
    for (p = to + start * delta, q = from + start; p < end; p += delta) {
        float G = error + *q++ * 255.0f;
        if (G <= 0) {
            *p = 0;
        } else if (G < 255) {
            int i = (int)G;
            *p = (uchar)i;
            error = G - i;
        } else {
            *p = 255;
        }
    }
    /* go backwards from starting point to start of line: */
    error = .5;
    for (p = to + (start - 1) * delta, q = from + start; p >= to; p -= delta) {
        float G = error + *--q * 255.0f;
        if (G <= 0) {
            *p = 0;
        } else if (G < 255) {
            int i = (int)G;
            *p = (uchar)i;
            error = G - i;
        } else {
            *p = 255;
        }
    }
}

void linear_to_byte(uchar *to, const float *from, const float *alpha, int W, int delta)
{
    unsigned char *end = to + W * delta;
    int start = rand() % W;
    const float *q;
    const float *a = alpha;
    unsigned char *p;
    /* go fowards from starting point to end of line: */
    float error = .5;
    for (p = to + start * delta, q = from + start, a += start; p < end; p += delta) {
        float v = *q * *a;
        float G = error + v * 255.0f;
        ++q;
        ++a;
        if (G <= 0) {
            *p = 0;
        } else if (G < 255) {
            int i = (int)G;
            *p = (uchar)i;
            error = G - i;
        } else {
            *p = 255;
        }
    }
    /* go backwards from starting point to start of line: */
    error = .5;
    for (p = to + (start - 1) * delta, q = from + start, a = alpha + start; p >= to; p -= delta) {
        const float v = *q * *a;
        float G = error + v * 255.0f;
        --q;
        --a;
        if (G <= 0) {
            *p = 0;
        } else if (G < 255) {
            int i = (int)G;
            *p = (uchar)i;
            error = G - i;
        } else {
            *p = 255;
        }
    }
}

void linear_to_short(U16 *to, const float *from, int W, int bits, int delta)
{
    (void)to;
    (void)from;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "linear_to_short not implemented yet." << endl;
}

void linear_to_short(U16 *to, const float *from, const float *alpha, int W, int bits, int delta)
{
    (void)to;
    (void)from;
    (void)alpha;
    (void)W;
    (void)bits;
    (void)delta;
    cout << "linear_to_short not implemented yet." << endl;
}

void linear_to_float(float *to, const float *from, int W, int delta)
{
    (void)delta;
    if (delta == 1) {
        memcpy(to, from, W * sizeof(float));
    } else {
        for (int i = 0; i < W; i += delta) {
            to[i] = from[i];
        }
    }
}

void linear_to_float(float *to, const float *from, const float *alpha, int W, int delta)
{
    for (int i = 0; i < W; i += delta) {
        to[i] = from[i] * alpha[i];
    }
}

void linear_to_byte_rect(uchar *to, const float *from,
                         const RectI &rect, const RectI& srcRod,const RectI& dstRod,
                         bool invertY , bool premult,
                         Lut::PackedPixelsFormat format)
{
    int rOffset, gOffset, bOffset, aOffset;
    getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
    if (!premult) {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int start = rand() % rect.width() + rect.left();
            unsigned error_r, error_g, error_b;
            error_r = error_g = error_b = 0x80;
            int srcY = y;
            if (invertY) {
                srcY = srcRod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * srcRod.width() * 4);
            uchar *dst_pixels = to + (y * dstRod.width() * 4);
            /* go fowards from starting point to end of line: */
            for (int x = start; x < rect.right(); ++x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                error_r = (error_r & 0xff) + src_pixels[col] * 255.f;
                error_g = (error_g & 0xff) + src_pixels[col + 1] * 255.f;
                error_b = (error_b & 0xff) + src_pixels[col + 2] * 255.f;
                dst_pixels[col] = (uchar)(error_r >> 8);
                dst_pixels[col + rOffset] = (uchar)(error_g >> 8);
                dst_pixels[col + gOffset] = (uchar)(error_b >> 8);
                dst_pixels[col + aOffset] = (uchar)(std::min(a * 256.f, 255.f));
            }
            /* go backwards from starting point to start of line: */
            error_r = error_g = error_b = 0x80;
            for (int x = start - 1; x >= rect.left(); --x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                error_r = (error_r & 0xff) + src_pixels[col] * 255.f;
                error_g = (error_g & 0xff) + src_pixels[col + 1] * 255.f;
                error_b = (error_b & 0xff) + src_pixels[col + 2] * 255.f;
                dst_pixels[col + rOffset] = (uchar)(error_r >> 8);
                dst_pixels[col + gOffset] = (uchar)(error_g >> 8);
                dst_pixels[col + bOffset] = (uchar)(error_b >> 8);
                dst_pixels[col + aOffset] = (uchar)(std::min(a * 256.f, 255.f));
            }
        }
    } else {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int start = rand() % rect.width() + rect.left();
            unsigned error_r, error_g, error_b;
            error_r = error_g = error_b = 0x80;
            int srcY = y;
            if (invertY) {
                srcY = srcRod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * srcRod.width() * 4);
            uchar *dst_pixels = to + (y * dstRod.width() * 4);
            /* go fowards from starting point to end of line: */
            for (int x = start; x < rect.right(); ++x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                error_r = (error_r & 0xff) + src_pixels[col] * a * 255.f;
                error_g = (error_g & 0xff) + src_pixels[col + 1] * a * 255.f;
                error_b = (error_b & 0xff) + src_pixels[col + 2] * a * 255.f;
                dst_pixels[col + rOffset] = (uchar)(error_r >> 8);
                dst_pixels[col + gOffset] = (uchar)(error_g >> 8);
                dst_pixels[col + bOffset] = (uchar)(error_b >> 8);
                dst_pixels[col + aOffset] = (uchar)(std::min(a * 256.f, 255.f));
            }
            /* go backwards from starting point to start of line: */
            error_r = error_g = error_b = 0x80;
            for (int x = start - 1; x >= rect.left(); --x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                error_r = (error_r & 0xff) + src_pixels[col] * a * 255.f;
                error_g = (error_g & 0xff) + src_pixels[col + 1] * a * 255.f;
                error_b = (error_b & 0xff) + src_pixels[col + 2] * a * 255.f;
                dst_pixels[col + rOffset] = (uchar)(error_r >> 8);
                dst_pixels[col + gOffset] = (uchar)(error_g >> 8);
                dst_pixels[col + bOffset] = (uchar)(error_b >> 8);
                dst_pixels[col + aOffset] = (uchar)(std::min(a * 256.f, 255.f));
            }
        }

    }
}

void linear_to_short_rect(U16 *to, const float *from,
                          const RectI &rect, const RectI& srcRod,const RectI& dstRod,
                          bool invertY , bool premult,
                          Lut::PackedPixelsFormat format)
{
    (void)to;
    (void)from;
    (void)rect;
    (void)srcRod;
    (void)dstRod;
    (void)invertY;
    (void)premult;
    (void)format;
    cout << "linear_to_short_rect not yet implemented" << endl;
}

void linear_to_float_rect(float *to, const float *from,
                          const RectI &rect,const RectI& srcRod,const RectI& dstRod,
                          bool invertY , bool premult,
                          Lut::PackedPixelsFormat format)
{
    if (!premult) {
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int srcY = y;
            if (invertY) {
                srcY = srcRod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * srcRod.width() * 4);
            float *dst_pixels = to + (y * dstRod.width() * 4);
            if (format == Lut::RGBA) {
                memcpy(dst_pixels, src_pixels, rect.width()*sizeof(float));
            } else {
                int rOffset, gOffset, bOffset, aOffset;
                getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
                for (int x = rect.left(); x < rect.right(); ++x) {
                    int col = x * 4;
                    dst_pixels[col + rOffset] = src_pixels[col];
                    dst_pixels[col + gOffset] = src_pixels[col + 1];
                    dst_pixels[col + bOffset] = src_pixels[col + 2];
                    dst_pixels[col + aOffset] = src_pixels[col + 3];
                }
            }
        }
    } else {
        int rOffset, gOffset, bOffset, aOffset;
        getOffsetsForPacking(format, &rOffset, &gOffset, &bOffset, &aOffset);
        for (int y = rect.bottom(); y < rect.top(); ++y) {
            int srcY = y;
            if (invertY) {
                srcY = srcRod.top() - y - 1;
            }
            const float *src_pixels = from + (srcY * srcRod.width() * 4);
            float *dst_pixels = to + (y * dstRod.width() * 4);
            for (int x = rect.left(); x < rect.right(); ++x) {
                int col = x * 4;
                float a = src_pixels[col + 3];
                dst_pixels[col] = src_pixels[col] * a;
                dst_pixels[col + rOffset] = src_pixels[col + 1] * a;
                dst_pixels[col + gOffset] = src_pixels[col + 2] * a;
                dst_pixels[col + bOffset] = a;
            }
        }
    }
}

namespace
{
// a Singleton that holds precomputed LUTs for the whole application.
// The m_instance member is static and is thus built before the first call to Instance().
// After creation, is is always accessed read-only (no new LUTs should be added),
// thus it does not need to be protected by a mutex.
// To enforce this, the only public function (getLut()) is marked as const
class LutSingleton
{
public:
    static const LutSingleton &Instance() {
        return m_instance;
    };
    const Lut *getLut(LutType type) const {
        std::map<LutType, const Lut *>::const_iterator found = luts.find(type);
        if (found != luts.end()) {
            return found->second;
        }
        return NULL;
    }

private:
    LutSingleton &operator= (const LutSingleton &) {
        return *this;
    }
    LutSingleton(const LutSingleton &) {}

    static LutSingleton m_instance;
    LutSingleton();
    ~LutSingleton();

    std::map<LutType, const Lut *> luts;
};

LutSingleton::LutSingleton()
{
    Lut *srgb = new sRGB;
    Lut *lin = new LinearLut();
    Lut *rec709 = new Rec709;
    luts.insert(make_pair(LUT_DEFAULT_VIEWER, srgb));
    luts.insert(make_pair(LUT_DEFAULT_FLOAT, lin));
    luts.insert(make_pair(LUT_DEFAULT_INT8, srgb));
    luts.insert(make_pair(LUT_DEFAULT_INT16, srgb));
    luts.insert(make_pair(LUT_DEFAULT_MONITOR, rec709));
}

LutSingleton::~LutSingleton()
{
    for (std::map<LutType, const Lut *>::iterator it = luts.begin(); it != luts.end(); ++it) {
        if (it->second) {
            // now finding in map all other members that have the same pointer to avoid double free
            for (std::map<LutType, const Lut *>::iterator it2 = luts.begin(); it2 != luts.end(); ++it2) {
                if (it2->second == it->second && it->first != it2->first) {
                    it2->second = 0;
                }
            }
            delete it->second;
            it->second = 0;
        }
    }
    luts.clear();
}

LutSingleton LutSingleton::m_instance = LutSingleton();

} // namespace {

const Lut *getLut(LutType type)
{
    return LutSingleton::Instance().getLut(type);
}

void allocateLuts() {}

void deallocateLuts() {}

} // namespace Color
} // namespace Natron
