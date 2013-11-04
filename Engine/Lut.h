//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_ENGINE_LUT_H_
#define POWITER_ENGINE_LUT_H_


/*
 *
 * High-speed conversion between 8 bit and floating point image data.
 *
 * Copyright 2002 Bill Spitzak and Digital Domain, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * For use in closed-source software please contact Digital Domain,
 * 300 Rose Avenue, Venice, CA 90291 310-314-2800
 *
 */

#include <cmath>
#include <QtGui/QRgb>
#include "Engine/RectI.h"
#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

namespace Powiter {
namespace Color {

/*This class implements : http://mysite.verizon.net/spitzak/conversion/algorithm.html*/

class Lut {
    // the fast lookup tables are mutable, because they are automatically initialized post-construction,
    // and never change afterwards
    mutable U16 to_byte_table[0x10000]; // contains  2^16 = 65536 values
	mutable float from_byte_table[256]; // values between 0-1.f.
	mutable bool init_;
    
public:
    Lut() : init_(false) {}
    virtual ~Lut() {}
    
    virtual bool linear() const = 0;
    
    /*useful for packed-pixels buffers operations*/
    enum PackedPixelsFormat{RGBA = 0,BGRA};
    
    /*! Convert an array of linear floating point pixel values to an
     array of destination lut values, with error diffusion to avoid posterizing
     artifacts.
     
     \a W is the number of pixels to convert.  \a delta is the distance
     between the output bytes (useful for interlacing them into a buffer
     for screen display).
     
     The input and output buffers must not overlap in memory.
     */
    void to_byte(uchar* to, const float* from, int W, int delta = 1) const;
    
    /*used to pre-multiply by alpha*/
    void to_byte(uchar* to, const float* from, const float* alpha, int W,int delta = -1) const;
    
    void to_short(U16* to, const float* from, int W, int bits = 16, int delta = 1) const;
    void to_short(U16* to, const float* from, const float* alpha, int W, int bits = 16, int delta = 1) const;
    
    void to_float(float* to, const float* from, int W, int delta = 1) const;
    void to_float(float* to, const float* from, const float* alpha, int W, int delta = 1) const;
    

    void from_byte(float* to, const uchar* from, int W, int delta = 1) const;
    
    /*un-premultiply by alpha before color-space converting and re-multiply by alpha
     afterwards*/
    void from_byte(float* to, const uchar* from, const uchar* alpha, int W, int delta = 1) const;
    void from_byteQt(float* to, const QRgb* from,Powiter::Channel z,bool premult,int W,int delta = 1) const;
    
    void from_short(float* to, const U16* from, int W, int bits = 16, int delta = 1) const;
    void from_short(float* to, const U16* from, const U16* alpha, int W, int bits = 16, int delta = 1) const;
    
    void from_float(float* to, const float* from, int W, int delta = 1) const;
    void from_float(float* to, const float* from, const float* alpha, int W, int delta = 1) const;
    
    
    
    
    /**
     * @brief These functions work exactly like to_byte but take in parameter 2 packed-buffers
     * pointing to (0,0) and rectangle to copy in image coordinates.
     * @param to[out] The buffer to write to. It must point to (rod.left(),rod.bottom()).
     * @param from[in] The buffer to read from. It must point to (rod.left(),rod.bottom()).
     * @param rect[in] The rectangle portion to copy over. It must be clipped to the rod.
     * @param rod[in] The region of definition of the input and output buffers.
     * @param invertY[in] If true, then it will use for the output scan-line y the input scan-line srcY = rod.top() - y - 1
     * @param premult[in] Should the output pixels be pre-multiplied by alpha ?
     * @param format[in] Specifies the pixel packing format of the OUTPUT buffer. This function assumes the 'from' buffer
     * is RGBA laid out.
     * WARNING: to and from must have exactly the same pixels count.
     **/
    void to_byte_rect(uchar* to, const float* from,
                      const RectI& rect,const RectI& rod,
                      bool invertY,bool premult,
                      Lut::PackedPixelsFormat format) const;
    void to_short_rect(U16* to, const float* from,
                       const RectI& rect,const RectI& rod,
                       bool invertY ,bool premult,
                       Lut::PackedPixelsFormat format) const;
    void to_float_rect(float* to, const float* from,
                       const RectI& rect,const RectI& rod,
                       bool invertY,bool premult,
                       Lut::PackedPixelsFormat format) const;
    
    
    /**
     * @brief These functions work exactly like from_byte but take in parameter 2 packed-buffers
     * pointing to (0,0) and rectangle to copy in image coordinates.
     * @param to[out] The buffer to write to. It must point to (rod.left(),rod.bottom()).
     * @param from[in] The buffer to read from. It must point to (rod.left(),rod.bottom()).
     * @param rect[in] The rectangle portion to copy over. It must be clipped to the rod.
     * @param rod[in] The region of definition of the input and output buffers.
     * @param invertY[in] If true, then it will use for the output scan-line y the input scan-line srcY = rod.top() - y - 1
     * @param premult[in] Are the input pixels pre-multiplied by alpha ? If yes it will un-premultiply by alpha before color-converting
     * and then multiply by alpha afterwards.
     * @param format[in] Specifies the pixel packing format of the INPUT buffer. This function assumes the 'to' buffer
     * is RGBA laid out.
     * WARNING: to and from must have exactly the same pixels count.
     **/

    void from_byte_rect(float* to, const uchar* from,
                        const RectI& rect,const RectI& rod,
                        bool invertY ,bool premult,
                        Lut::PackedPixelsFormat format) const;
    void from_short_rect(float* to, const U16* from,
                         const RectI& rect,const RectI& rod,
                         bool invertY ,bool premult,
                         Lut::PackedPixelsFormat format) const;
    void from_float_rect(float* to, const float* from,
                         const RectI& rect,const RectI& rod,
                         bool invertY ,bool premult,
                         Lut::PackedPixelsFormat format) const;
    
    

    
    float toFloatFast(float v) const;

    float fromFloatFast(float v) const;
    
    // post-constructor
    void validate() const {
        if (init_) {
            return;
        }
        if (!linear()) {
            fillTables();
        }
        init_=true;
    }

private:

    //takes in input a float clamped to [0 - 1.f] and  return
    // the value after color-space conversion is in [0 - 255.f]
    virtual float to_byte(float) const = 0;

    // takes in input a float clamped to [0 - 255.f] and do the math to convert it
    // to have the opposite effect of to_byte(float), returns it between [0 - 1.f]
    virtual float from_byte(float) const = 0;

    void fillTables() const;

    
    
};

// DEFAULT/GLOBAL LUTS
//
// luts : linear,srgb,rec709,cineon,gamma 1.8,gamma2.2,panalog,redlog,viperlog,alexaV3logC,ploglin,slog,redspace
// default : monitor = srgb,8bit :srgb, 16 bit:srgb, log :cineon,float : linear
enum LutType {
    LUT_DEFAULT_MONITOR = 0,
    LUT_DEFAULT_VIEWER = 1,
    LUT_DEFAULT_INT8 = 2,
    LUT_DEFAULT_INT16 = 3,
    LUT_LOG = 4,
    LUT_DEFAULT_FLOAT = 5,
    LUT_GAMMA1_8 = 6,
    LUT_GAMMA2_2 = 7,
    LUT_PANALOG = 8,
    LUT_REDLOG = 9,
    LUT_VIPERLOG = 10,
    LUT_ALEXAV3LOGC = 11,
    LUT_PLOGLIN = 12,
    LUT_SLOG = 13,
    LUT_TYPES_END = 14
};

void allocateLuts();

void deallocateLuts();

//     // Return a LUT associated to the datatype
const Lut* getLut(LutType);

inline void gamma_correct(float *c, float gamma)
{
	*c = powf((*c), gamma);
}

inline float rec709_to_linearrgb(float c)
{
	if (c < 0.081f)
		return (c < 0.0f) ? 0.0f : c * (1.0f / 4.5f);
	else
		return powf((c + 0.099f) * (1.0f / 1.099f), (1.0f / 0.45f));
}

inline float linearrgb_to_rec709(float c)
{
	if (c < 0.018f)
		return (c < 0.0f) ? 0.0f : c * 4.5f;
	else
		return 1.099f * powf(c, 0.45f) - 0.099f;
}

inline float srgb_to_linearrgb(float c)
{
	if (c < 0.04045f)
		return (c < 0.0f) ? 0.0f : c * (1.0f / 12.92f);
	else
		return powf((c + 0.055f) * (1.0f / 1.055f), 2.4f);
}

inline float linearrgb_to_srgb(float c)
{
	if (c < 0.0031308f)
		return (c < 0.0f) ? 0.0f : c * 12.92f;
	else
		return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}
    
void linear_from_byte(float* to, const uchar* from, int W, int delta = 1);
void linear_from_byteQt(float* to, const QRgb* from,Powiter::Channel z, int W, int delta = 1);
void linear_from_short(float* to, const U16* from, int W, int bits = 16, int delta = 1);
void linear_from_float(float* to, const float* from, int W, int delta = 1);

    
    /**
     * @brief These functions work exactly like from_byte but take in parameter 2 packed-buffers
     * pointing to (0,0) and rectangle to copy in image coordinates.
     * @param to[out] The buffer to write to. It must point to (rod.left(),rod.bottom()).
     * @param from[in] The buffer to read from. It must point to (rod.left(),rod.bottom()).
     * @param rect[in] The rectangle portion to copy over. It must be clipped to the rod.
     * @param rod[in] The region of definition of the input and output buffers.
     * @param invertY[in] If true, then it will use for the output scan-line y the input scan-line srcY = rod.top() - y - 1
     * @param format[in] Specifies the pixel packing format of the INPUT buffer. This function assumes the 'to' buffer
     * is RGBA laid out.
     * WARNING: to and from must have exactly the same pixels count.
     **/

void linear_from_byte_rect(float* to, const uchar* from,
                           const RectI& rect,const RectI& rod,
                           bool invertY,
                           Lut::PackedPixelsFormat format);

void linear_from_short_rect(float* to, const U16* from,
                            const RectI& rect,const RectI& rod,
                            bool invertY,
                            Lut::PackedPixelsFormat format);
void linear_from_float_rect(float* to, const float* from,
                            const RectI& rect,const RectI& rod,
                            bool invertY,
                            Lut::PackedPixelsFormat format);

/*! Convert an array of floating point pixel values to an array of
 bytes by multiplying by 255 and doing error diffusion to avoid
 banding. This should be used to convert mattes and alpha channels.

 \a W is the number of pixels to convert.  \a delta is the distance
 between the output bytes (useful for interlacing them into a buffer
 for screen display).

 The input and output buffers must not overlap in memory.
 */
void linear_to_byte(uchar* to, const float* from, int W, int delta = 1);
void linear_to_byte(uchar* to, const float* from,const float* alpha, int W, int delta = 1);
void linear_to_short(U16* to, const float* from, int W, int bits = 16, int delta = 1);
void linear_to_short(U16* to, const float* from,const float* alpha, int W, int bits = 16, int delta = 1);
void linear_to_float(float* to, const float* from, int W, int delta = 1);
void linear_to_float(float* to, const float* from,const float* alpha, int W, int delta = 1);
    
    
    /**
     * @brief These functions work exactly like to_byte but take in parameter 2 packed-buffers
     * pointing to (0,0) and rectangle to copy in image coordinates.
     * @param to[out] The buffer to write to. It must point to (rod.left(),rod.bottom()).
     * @param from[in] The buffer to read from. It must point to (rod.left(),rod.bottom()).
     * @param rect[in] The rectangle portion to copy over. It must be clipped to the rod.
     * @param rod[in] The region of definition of the input and output buffers.
     * @param invertY[in] If true, then it will use for the output scan-line y the input scan-line srcY = rod.top() - y - 1
     * @param premult[in] Should the output pixels be pre-multiplied by alpha ?
     * @param format[in] Specifies the pixel packing format of the OUTPUT buffer. This function assumes the 'from' buffer
     * is RGBA laid out.
     * WARNING: to and from must have exactly the same pixels count.
     **/
void linear_to_byte_rect(uchar* to, const float* from,
                         const RectI& rect,const RectI& rod,
                         bool invertY,bool premult,
                         Lut::PackedPixelsFormat format);
void linear_to_short_rect(U16* to, const float* from,
                          const RectI& rect,const RectI& rod,
                          bool invertY,bool premult,
                          Lut::PackedPixelsFormat format);
void linear_to_float_rect(float* to, const float* from,
                          const RectI& rect,const RectI& rod,
                          bool invertY,bool premult,
                          Lut::PackedPixelsFormat format);

} // namespace Powiter
} // namespace Color

#endif //POWITER_ENGINE_LUT_H_
