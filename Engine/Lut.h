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

///
///// This namespace is kept is synch with what can be found in openfx-io repository. It is used here in Natron for the viewer essentially.
///



#ifndef NATRON_ENGINE_LUT_H_
#define NATRON_ENGINE_LUT_H_

#include <cmath>
#include <map>
#include <string>

#include <QMutex>


class RectI;

namespace Natron {
    namespace Color {
        
        /// @enum An enum describing supported pixels packing formats
        enum PixelPacking {
            PACKING_RGBA = 0,
            PACKING_BGRA,
            PACKING_RGB,
            PACKING_BGR,
            PACKING_PLANAR
        };
        
        
        
        /* @brief Converts a float ranging in [0 - 1.f] in the desired color-space to linear color-space also ranging in [0 - 1.f]*/
        typedef float (*fromColorSpaceFunctionV1)(float v);
        
        /* @brief Converts a float ranging in [0 - 1.f] in  linear color-space to the desired color-space to also ranging in [0 - 1.f]*/
        typedef float (*toColorSpaceFunctionV1)(float v);
        
        
        // a Singleton that holds precomputed LUTs for the whole application.
        // The m_instance member is static and is thus built before the first call to Instance().
        // WARNING : This class is not thread-safe and calling getLut must not be done in a function called
        // by multiple thread at once!
        class Lut;
        class LutManager
        {
        public:
            static LutManager &Instance() {
                return m_instance;
            };
            
            /**
             * @brief Returns a pointer to a lut with the given name and the given from and to functions.
             * If a lut with the same name didn't already exist, then it will create one.
             * WARNING : NOT THREAD-SAFE
             **/
            static const Lut *getLut(const std::string& name,fromColorSpaceFunctionV1 fromFunc,toColorSpaceFunctionV1 toFunc);
            
            ///buit-ins color-spaces
            static const Lut* sRGBLut();
            
            static const Lut* Rec709Lut();
            
            static const Lut* CineonLut();
            
            static const Lut* Gamma1_8Lut();
            
            static const Lut* Gamma2_2Lut();
            
            static const Lut* PanaLogLut();
            
            static const Lut* ViperLogLut();
            
            static const Lut* RedLogLut();
            
            static const Lut* AlexaV3LogCLut();
            
        private:
            LutManager &operator= (const LutManager &) {
                return *this;
            }
            LutManager(const LutManager &) {}
            
            static LutManager m_instance;
            LutManager();
            
            ////the luts must all have been released before!
            ////This is because the Lut holds a OFX::MultiThread::Mutex and it can't be deleted
            //// by this singleton because it makes their destruction time uncertain regarding to
            ///the host multi-thread suite.
            ~LutManager();
            
            //each lut with a ref count mapped against their name
            typedef std::map<std::string,const Lut * > LutsMap;
            LutsMap luts;
        };
        
        
        
        /**
         * @brief A Lut (look-up table) used to speed-up color-spaces conversions.
         * If you plan on doing linear conversion, you should just use the Linear class instead.
         **/
        class Lut {
            
            std::string _name; ///< name of the lut
            fromColorSpaceFunctionV1 _fromFunc;
            toColorSpaceFunctionV1 _toFunc;
            
            /// the fast lookup tables are mutable, because they are automatically initialized post-construction,
            /// and never change afterwards
            mutable unsigned short to_byte_table[0x10000]; /// contains  2^16 = 65536 values between 0-255
            mutable float from_byte_table[256]; /// values between 0-1.f
            mutable bool init_; ///< false if the tables are not yet initialized
            mutable QMutex _lock; ///< protects init_
            
            friend class LutManager;
            ///private constructor, used by LutManager
            Lut(const std::string& name,fromColorSpaceFunctionV1 fromFunc,toColorSpaceFunctionV1 toFunc)
            : _name(name)
            , _fromFunc(fromFunc)
            , _toFunc(toFunc)
            , init_(false)
            , _lock()
            {
                
            }
            
            ///init luts
            ///it uses fromFloat(float) and toFloat(float)
            ///Called by validate()
            void fillTables() const;
            
            
            /* @brief Converts a float ranging in [0 - 1.f] in the desired color-space to linear color-space also ranging in [0 - 1.f]
             * This function is not fast!
             * @see fromFloatFast(float)
             */
            float fromFloat(float v) const { return _fromFunc(v); }
            
            /* @brief Converts a float ranging in [0 - 1.f] in  linear color-space to the desired color-space to also ranging in [0 - 1.f]
             * This function is not fast!
             * @see toFloatFast(float)
             */
            float toFloat(float v) const { return _toFunc(v); }
            
        public:
            
            //Called by all public members
            void validate() const {
                QMutexLocker g(&_lock);
                if (init_) {
                    return;
                }
                fillTables();
                init_=true;
            }

            
            const std::string& getName() const { return _name; }
            
            /* @brief Converts a float ranging in [0 - 1.f] in linear color-space using the look-up tables.
             * @return A float in [0 - 1.f] in the destination color-space.
             */
            float toColorSpaceFloatFromLinearFloatFast(float v) const;
            
            /* @brief Converts a float ranging in [0 - 1.f] in linear color-space using the look-up tables.
             * @return A byte in [0 - 255] in the destination color-space.
             */
            unsigned char toColorSpaceByteFromLinearFloatFast(float v) const;
            
            /* @brief Converts a float ranging in [0 - 1.f] in linear color-space using the look-up tables.
             * @return A byte in [0 - 255] in the destination color-space.
             */
            unsigned short toColorSpaceShortFromLinearFloatFast(float v) const;

            
            /* @brief Converts a float ranging in [0 - 1.f] in the destination color-space using the look-up tables.
             * @return A float in [0 - 1.f] in linear color-space.
             */
            float fromColorSpaceFloatToLinearFloatFast(float v) const;
            
            
            
            /////@TODO the following functions expects a float input buffer, one could extend it to cover all bitdepths.
            
            /**
             * @brief Convert an array of linear floating point pixel values to an
             * array of destination lut values, with error diffusion to avoid posterizing
             * artifacts.
             *
             * \a W is the number of pixels to convert.
             * \a inDelta is the distance between the input elements
             * \a outDelta is the distance between the output elements
             * The delta parameters are useful for:
             - interlacing a planar input buffer into a packed buffer.
             - deinterlacing a packed input buffer into a planar buffer.
             
             * \a alpha is a pointer to an extra alpha planar buffer if you want to premultiply by alpha the from channel.
             * The input and output buffers must not overlap in memory.
             **/
            void to_byte_planar(unsigned char* to, const float* from,int W,const float* alpha = NULL,
                                int inDelta = 1, int outDelta = 1) const;
            void to_short_planar(unsigned short* to, const float* from,int W,const float* alpha = NULL,
                                 int inDelta = 1, int outDelta = 1) const;
            void to_float_planar(float* to, const float* from,int W,const float* alpha = NULL,
                                 int inDelta = 1, int outDelta = 1) const;
            
            
            /**
             * @brief These functions work exactly like the to_X_planar functions but expect 2 buffers
             * pointing at (0,0) in an image and convert a rectangle of the image. It also supports
             * several pixel packing commonly used by openfx images.
             * Note that the conversionRect will be clipped to srcRoD and dstRoD to prevent bad memory access
             
             \arg 	- from - A pointer to the input buffer, pointing at (0,0) in the image.
             \arg 	- srcRoD - The region of definition of the input buffer.
             \arg 	- inputPacking - The pixel packing of the input buffer.
             
             \arg 	- conversionRect - The region we want to convert. This will be clipped
             agains srcRoD and dstRoD.
             
             \arg 	- to - A pointer to the output buffer, pointing at (0,0) in the image.
             \arg 	- dstRoD - The region of definition of the output buffer.
             \arg 	- outputPacking - The pixel packing of the output buffer.
             
             \arg premult - If true, it indicates we want to premultiply by the alpha channel
             the R,G and B channels if they exist.
             
             \arg invertY - If true then the output scan-line y of the output buffer
             should be converted with the scan-line (srcRoD.y2 - y - 1) of the
             input buffer.
             
             **/
            void to_byte_packed(unsigned char* to, const float* from,const RectI& conversionRect,
                                const RectI& srcRoD,const RectI& dstRoD,
                                PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const;
            void to_short_packed(unsigned short* to, const float* from,const RectI& conversionRect,
                                 const RectI& srcRoD,const RectI& dstRoD,
                                 PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const;
            void to_float_packed(float* to, const float* from,const RectI& conversionRect,
                                 const RectI& srcRoD,const RectI& dstRoD,
                                 PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const;
            
            
            
            /////@TODO the following functions expects a float output buffer, one could extend it to cover all bitdepths.
            
            /**
             * @brief Convert from a buffer in the input color-space to the output color-space.
             *
             * \a W is the number of pixels to convert.
             * \a inDelta is the distance between the input elements
             * \a outDelta is the distance between the output elements
             * The delta parameters are useful for:
             - interlacing a planar input buffer into a packed buffer.
             - deinterlacing a packed input buffer into a planar buffer.
             
             * \a alpha is a pointer to an extra planar buffer being the alpha channel of the image.
             * If the image was stored with premultiplication on, it will then unpremultiply by alpha
             * before doing the color-space conversion, and the multiply back by alpha.
             * The input and output buffers must not overlap in memory.
             **/
            void from_byte_planar(float* to,const unsigned char* from,
                                  int W,const unsigned char* alpha = NULL,int inDelta = 1, int outDelta = 1) const;
            void from_short_planar(float* to,const unsigned short* from,
                                   int W,const unsigned short* alpha = NULL,int inDelta = 1, int outDelta = 1) const;
            void from_float_planar(float* to,const float* from,
                                   int W,const float* alpha = NULL,int inDelta = 1, int outDelta = 1) const;
            
            
            /**
             * @brief These functions work exactly like the to_X_planar functions but expect 2 buffers
             * pointing at (0,0) in an image and convert a rectangle of the image. It also supports
             * several pixel packing commonly used by openfx images.
             * Note that the conversionRect will be clipped to srcRoD and dstRoD to prevent bad memory access
             
             \arg 	- from - A pointer to the input buffer, pointing at (0,0) in the image.
             \arg 	- srcRoD - The region of definition of the input buffer.
             \arg 	- inputPacking - The pixel packing of the input buffer.
             
             \arg 	- conversionRect - The region we want to convert. This will be clipped
             agains srcRoD and dstRoD.
             
             \arg 	- to - A pointer to the output buffer, pointing at (0,0) in the image.
             \arg 	- dstRoD - The region of definition of the output buffer.
             \arg 	- outputPacking - The pixel packing of the output buffer.
             
             \arg premult - If true, it indicates we want to unpremultiply the R,G,B channels (if they exist) by the alpha channel
             before doing the color-space conversion, and multiply back by alpha.
             
             \arg invertY - If true then the output scan-line y of the output buffer
             should be converted with the scan-line (srcRoD.y2 - y - 1) of the
             input buffer.
             
             **/
            void from_byte_packed(float* to, const unsigned char* from,const RectI& conversionRect,
                                  const RectI& srcRoD,const RectI& dstRoD,
                                  PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const;
            
            void from_short_packed(float* to, const unsigned short* from,const RectI& conversionRect,
                                   const RectI& srcRoD,const RectI& dstRoD,
                                   PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const;
            
            void from_float_packed(float* to, const float* from,const RectI& conversionRect,
                                   const RectI& srcRoD,const RectI& dstRoD,
                                   PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const;
            
            
        };
        
        
        namespace Linear {
            
            ///utility functions
            inline float toFloat(unsigned char v) { return (float)v / 255.f; }
            inline float toFloat(unsigned short v) { return (float)v / 65535.f; }
            inline float toFloat(float v) { return v; }
            
            inline unsigned char fromFloatB(float v) { return (unsigned char)(v * 255.f); }
            inline unsigned short fromFloatS(float v) { return (unsigned short)(v * 65535.f); }
            inline float fromFloatF(float v) { return v; }
            
            /////the following functions expects a float input buffer, one could extend it to cover all bitdepths.
            
            /**
             * @brief Convert an array of linear floating point pixel values to an
             * array of destination lut values, with error diffusion to avoid posterizing
             * artifacts.
             *
             * \a W is the number of pixels to convert.
             * \a inDelta is the distance between the input elements
             * \a outDelta is the distance between the output elements
             * The delta parameters are useful for:
             - interlacing a planar input buffer into a packed buffer.
             - deinterlacing a packed input buffer into a planar buffer.
             
             * \a alpha is a pointer to an extra alpha planar buffer if you want to premultiply by alpha the from channel.
             * The input and output buffers must not overlap in memory.
             **/
            void to_byte_planar(unsigned char* to, const float* from,int W,const float* alpha = NULL,int inDelta = 1, int outDelta = 1);
            void to_short_planar(unsigned short* to, const float* from,int W,const float* alpha = NULL,int inDelta = 1, int outDelta = 1);
            void to_float_planar(float* to, const float* from,int W,const float* alpha = NULL,int inDelta = 1, int outDelta = 1);
            
            
            /**
             * @brief These functions work exactly like the to_X_planar functions but expect 2 buffers
             * pointing at (0,0) in the image and convert a rectangle of the image. It also supports
             * several pixel packing commonly used by openfx images.
             * Note that the conversionRect will be clipped to srcRoD and dstRoD to prevent bad memory access
             \arg 	- from - A pointer to the input buffer, pointing at (0,0) in the image.
             \arg 	- srcRoD - The region of definition of the input buffer.
             \arg 	- inputPacking - The pixel packing of the input buffer.
             
             \arg 	- conversionRect - The region we want to convert. This will be clipped
             agains srcRoD and dstRoD.
             
             \arg 	- to - A pointer to the output buffer, pointing at (0,0) in the image.
             \arg 	- dstRoD - The region of definition of the output buffer.
             \arg 	- outputPacking - The pixel packing of the output buffer.
             
             \arg premult - If true, it indicates we want to premultiply by the alpha channel
             the R,G and B channels if they exist.
             
             \arg invertY - If true then the output scan-line y of the output buffer
             should be converted with the scan-line (srcRoD.y2 - y - 1) of the
             input buffer.
             **/
            void to_byte_packed(unsigned char* to, const float* from,const RectI& conversionRect,
                                const RectI& srcRoD,const RectI& dstRoD,
                                PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult);
            void to_short_packed(unsigned short* to, const float* from,const RectI& conversionRect,
                                 const RectI& srcRoD,const RectI& dstRoD,
                                 PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult);
            void to_float_packed(float* to, const float* from,const RectI& conversionRect,
                                 const RectI& srcRoD,const RectI& dstRoD,
                                 PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult);
            
            
            
            /////the following functions expects a float output buffer, one could extend it to cover all bitdepths.
            
            /**
             * @brief Convert from a buffer in the input color-space to the output color-space.
             *
             * \a W is the number of pixels to convert.
             * \a inDelta is the distance between the input elements
             * \a outDelta is the distance between the output elements
             * The delta parameters are useful for:
             - interlacing a planar input buffer into a packed buffer.
             - deinterlacing a packed input buffer into a planar buffer.
             
             * \a alpha is a pointer to an extra planar buffer being the alpha channel of the image.
             * If the image was stored with premultiplication on, it will then unpremultiply by alpha
             * before doing the color-space conversion, and the multiply back by alpha.
             * The input and output buffers must not overlap in memory.
             **/
            
            void from_byte_planar(float* to,const unsigned char* from, int W,int inDelta = 1, int outDelta = 1);
            void from_short_planar(float* to,const unsigned short* from,int W,int inDelta = 1, int outDelta = 1);
            void from_float_planar(float* to,const float* from,int W,int inDelta = 1, int outDelta = 1);
            
            
            /**
             * @brief These functions work exactly like the from_X_planar functions but expect 2 buffers
             * pointing at (0,0) in the image and convert a rectangle of the image. It also supports
             * several pixel packing commonly used by openfx images.
             * Note that the conversionRect will be clipped to srcRoD and dstRoD to prevent bad memory access
             
             \arg 	- from - A pointer to the input buffer, pointing at (0,0) in the image.
             \arg 	- srcRoD - The region of definition of the input buffer.
             \arg 	- inputPacking - The pixel packing of the input buffer.
             
             \arg 	- conversionRect - The region we want to convert. This will be clipped
             agains srcRoD and dstRoD.
             
             \arg 	- to - A pointer to the output buffer, pointing at (0,0) in the image.
             \arg 	- dstRoD - The region of definition of the output buffer.
             \arg 	- outputPacking - The pixel packing of the output buffer.
             
             \arg invertY - If true then the output scan-line y of the output buffer
             should be converted with the scan-line (srcRoD.y2 - y - 1) of the
             input buffer.
             **/
            void from_byte_packed(float* to, const unsigned char* from,const RectI& conversionRect,
                                  const RectI& srcRoD,const RectI& dstRoD,
                                  PixelPacking inputPacking,PixelPacking outputPacking,bool invertY);
            
            void from_short_packed(float* to, const unsigned short* from,const RectI& conversionRect,
                                   const RectI& srcRoD,const RectI& dstRoD,
                                   PixelPacking inputPacking,PixelPacking outputPacking,bool invertY);
            
            void from_float_packed(float* to, const float* from,const RectI& conversionRect,
                                   const RectI& srcRoD,const RectI& dstRoD,
                                   PixelPacking inputPacking,PixelPacking outputPacking,bool invertY);
            
        }//namespace Linear
        
        
        ///these are put in the header as they are used elsewhere
        
        inline float from_func_srgb(float v){
            if (v < 0.04045f)
                return (v < 0.0f) ? 0.0f : v * (1.0f / 12.92f);
            else
                return std::pow((v + 0.055f) * (1.0f / 1.055f), 2.4f);
        }
        
        inline float to_func_srgb(float v){
            if (v < 0.0031308f)
                return (v < 0.0f) ? 0.0f : v * 12.92f;
            else
                return 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
        }
        
        
        inline float clamp(float v,float min,float max){
            return v > max ? max : v;
            return v < min ? min : v;
        }
        
    } //namespace Color
} //namespace Natron




#endif //NATRON_ENGINE_LUT_H_
