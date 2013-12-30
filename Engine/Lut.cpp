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

#include "Engine/Rect.h"

namespace Natron {
    namespace Color {
        
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
        
        static unsigned short hipart(const float f)
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
        
        static float index_to_float(const unsigned short i)
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
                assert((O32_HOST_ORDER == O32_LITTLE_ENDIAN) || (O32_HOST_ORDER == O32_BIG_ENDIAN));
            }
            
            return tmp.f;
        }
        
        
        ///initialize the singleton
        LutManager LutManager::m_instance = LutManager();
        
        LutManager::LutManager()
        : luts()
        {
        }
        
        
        const Lut *LutManager::getLut(const std::string& name,fromColorSpaceFunctionV1 fromFunc,toColorSpaceFunctionV1 toFunc) {
            
            LutsMap::iterator found = LutManager::m_instance.luts.find(name);
            if (found != LutManager::m_instance.luts.end()) {
                return found->second;
            }else{
                std::pair<LutsMap::iterator,bool> ret =
                LutManager::m_instance.luts.insert(std::make_pair(name,new Lut(name,fromFunc,toFunc)));
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
            for (LutsMap::iterator it = luts.begin(); it!=luts.end(); ++it) {
                delete it->second;
            }
        }
        
        
        void clip(RectI* what,const RectI& to){
            if(what->x1 < to.x1){
                what->x1 = to.x1;
            }
            if(what->x2 > to.x2){
                what->x2 = to.x2;
            }
            if(what->y1 < to.y1){
                what->y1 = to.y1;
            }
            if(what->y2 > to.y2){
                what->y2 = to.y2;
            }
        }
        
        bool intersects(const RectI& what,const RectI& other){
            return (what.x2 >= other.x1 && what.x2 <= other.x2 ) ||
            ( what.x1 < other.x2 && what.x1 >= other.x1) ||
            ( what.y2 >= other.y1 && what.y2 <= other.y2) ||
            ( what.y1 < other.y2 && what.y1 >= other.y1);
        }
        
        void getOffsetsForPacking(PixelPacking format, int *r, int *g, int *b, int *a)
        {
            if (format == PACKING_BGRA) {
                *b = 0;
                *g = 1;
                *r = 2;
                *a = 3;
            }else if(format == PACKING_RGBA){
                *r = 0;
                *g = 1;
                *b = 2;
                *a = 3;
            }else if(format == PACKING_RGB){
                *r = 0;
                *g = 1;
                *b = 2;
                *a = -1;
            }else if(format == PACKING_BGR){
                *r = 0;
                *g = 1;
                *b = 2;
                *a = -1;
            }else if(format == PACKING_PLANAR){
                *r = 0;
                *g = 1;
                *b = 2;
                *a = -1;
            }else{
                *r = -1;
                *g = -1;
                *b = -1;
                *a = -1;
                throw std::runtime_error("Unsupported pixel packing format");
            }
        }
        
        
        float Lut::fromColorSpaceFloatToLinearFloatFast(float v) const
        {
            validate();
            return from_byte_table[(int)(v * 255)];
        }
        
        float Lut::toColorSpaceFloatFromLinearFloatFast(float v) const
        {
            validate();
            return (float)to_byte_table[hipart(v)] / 65535.f;
        }
        
        unsigned char Lut::toColorSpaceByteFromLinearFloatFast(float v) const
        {
            validate();
            return to_byte_table[hipart(v)] / 255;
        }
        
        unsigned short Lut::toColorSpaceShortFromLinearFloatFast(float v) const
        {
            validate();
            return to_byte_table[hipart(v)];
        }
        
        
        
        void Lut::fillTables() const
        {
            if (init_) {
                return;
            }
            for (int i = 0; i < 0x10000; ++i) {
                float inp = index_to_float((unsigned short)i);
                float f = _toFunc(inp);
                if (f <= 0) {
                    to_byte_table[i] = 0;
                } else if (f < 255) {
                    to_byte_table[i] = (unsigned short)(f * 0x100 + .5);
                } else {
                    to_byte_table[i] = 0xff00;
                }
            }
            
            for (int b = 0; b <= 255; ++b) {
                float f = _fromFunc((float)b / 255.f);
                from_byte_table[b] = f;
                int i = hipart(f);
                to_byte_table[i] = (unsigned short)(b * 0x100);
            }
            
        }
        
        void Lut::to_byte_planar(unsigned char* to, const float* from,int W,const float* alpha,int inDelta,int outDelta) const {
            validate();
            unsigned char *end = to + W * outDelta;
            int start = rand() % W;
            const float *q;
            unsigned char *p;
            unsigned error;
            if(!alpha){
                /* go fowards from starting point to end of line: */
                error = 0x80;
                for (p = to + start * outDelta, q = from + start * inDelta; p < end; p += outDelta , q += inDelta) {
                    error = (error & 0xff) + to_byte_table[hipart(*q)];
                    *p = (unsigned char)(error >> 8);
                }
                /* go backwards from starting point to start of line: */
                error = 0x80;
                for (p = to + (start - 1) * outDelta, q = from + start * inDelta; p >= to; p -= outDelta) {
                    q -= inDelta;
                    error = (error & 0xff) + to_byte_table[hipart(*q)];
                    *p = (unsigned char)(error >> 8);
                }
            }else{
                const float *a = alpha;
                /* go fowards from starting point to end of line: */
                error = 0x80;
                for (p = to + start * outDelta, q = from + start * inDelta, a += start * inDelta; p < end; p += outDelta, q += inDelta, a+= inDelta) {
                    const float v = *q * *a;
                    error = (error & 0xff) + to_byte_table[hipart(v)];
                    ++a;
                    *p = (unsigned char)(error >> 8);
                }
                /* go backwards from starting point to start of line: */
                error = 0x80;
                for (p = to + (start - 1) * outDelta, q = from + start * inDelta , a = alpha + start * inDelta; p >= to; p -= outDelta) {
                    const float v = *q * *a;
                    q -= inDelta;
                    q -= inDelta;
                    error = (error & 0xff) + to_byte_table[hipart(v)];
                    *p = (unsigned char)(error >> 8);
                }
                
            }
            
        }
        
        void Lut::to_short_planar(unsigned short* /*to*/, const float* /*from*/,int /*W*/,const float* /*alpha*/ ,
                                  int /*inDelta*/,int /*outDelta*/) const {
            
            throw std::runtime_error("Lut::to_short_planar not implemented yet.");
        }
        
        void Lut::to_float_planar(float* to, const float* from,int W,const float* alpha ,int inDelta,int outDelta) const {
            
            validate();
            if(!alpha){
                for (int f = 0,t = 0; f < W; f += inDelta, t+= outDelta) {
                    to[t] = toColorSpaceFloatFromLinearFloatFast(from[f]);
                }
            }else{
                for (int f = 0,t = 0; f < W; f += inDelta, t+= outDelta) {
                    to[t] = toColorSpaceFloatFromLinearFloatFast(from[f] * alpha[f]);
                }
            }
        }
        
        void Lut::to_byte_packed(unsigned char* to, const float* from,const RectI& conversionRect,
                                 const RectI& srcRoD,const RectI& dstRoD,
                                 PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const {
            
            ///if the conversion rectangle is out of the src rod and the dst rod, just return
            if(!intersects(conversionRect, srcRoD) || !intersects(conversionRect, dstRoD)){
                return;
            }
            
            ///clip the conversion rect to srcRoD and dstRoD
            RectI rect = conversionRect;
            clip(&rect,srcRoD);
            clip(&rect,dstRoD);
            
            bool inputHasAlpha = inputPacking == PACKING_BGRA || inputPacking == PACKING_RGBA;
            bool outputHasAlpha = outputPacking == PACKING_BGRA || outputPacking == PACKING_RGBA;
            
            int inROffset, inGOffset, inBOffset, inAOffset;
            int outROffset, outGOffset, outBOffset, outAOffset;
            getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
            getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);
            
            int inPackingSize,outPackingSize;
            inPackingSize = inputHasAlpha ? 4 : 3;
            outPackingSize = outputHasAlpha ? 4 : 3;
            
            validate();
            
            for (int y = rect.y1; y < rect.y2; ++y) {
                int start = rand() % (rect.x2 - rect.x1) + rect.x1;
                unsigned error_r, error_g, error_b;
                error_r = error_g = error_b = 0x80;
                int srcY = y;
                if (!invertY) {
                    srcY = srcRoD.y2 - y - 1;
                }
                
                
                int dstY = dstRoD.y2 - y - 1;
                
                const float *src_pixels = from + (srcY * (srcRoD.x2 - srcRoD.x1) * inPackingSize);
                unsigned char *dst_pixels = to + (dstY * (dstRoD.x2 - srcRoD.x1) * outPackingSize);
                /* go fowards from starting point to end of line: */
                for (int x = start; x < rect.x2; ++x) {
                    
                    int inCol = x * inPackingSize;
                    int outCol = x * outPackingSize;
                    
                    float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;
                    error_r = (error_r & 0xff) + to_byte_table[hipart(src_pixels[inCol + inROffset] * a)];
                    error_g = (error_g & 0xff) + to_byte_table[hipart(src_pixels[inCol + inGOffset] * a)];
                    error_b = (error_b & 0xff) + to_byte_table[hipart(src_pixels[inCol + inBOffset] * a)];
                    dst_pixels[outCol + outROffset] = (unsigned char)(error_r >> 8);
                    dst_pixels[outCol + outGOffset] = (unsigned char)(error_g >> 8);
                    dst_pixels[outCol + outBOffset] = (unsigned char)(error_b >> 8);
                    if(outputHasAlpha){
                        dst_pixels[outCol + outAOffset] = (unsigned char)(std::min(a * 256.f, 255.f));
                    }
                }
                /* go backwards from starting point to start of line: */
                error_r = error_g = error_b = 0x80;
                for (int x = start - 1; x >= rect.x1; --x) {
                    
                    int inCol = x * inPackingSize;
                    int outCol = x * outPackingSize;
                    
                    float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;
                    error_r = (error_r & 0xff) + to_byte_table[hipart(src_pixels[inCol + inROffset] * a)];
                    error_g = (error_g & 0xff) + to_byte_table[hipart(src_pixels[inCol + inGOffset] * a)];
                    error_b = (error_b & 0xff) + to_byte_table[hipart(src_pixels[inCol + inBOffset] * a)];
                    dst_pixels[outCol + outROffset] = (unsigned char)(error_r >> 8);
                    dst_pixels[outCol + outGOffset] = (unsigned char)(error_g >> 8);
                    dst_pixels[outCol + outBOffset] = (unsigned char)(error_b >> 8);
                    if(outputHasAlpha){
                        dst_pixels[outCol + outAOffset] = (unsigned char)(std::min(a * 256.f, 255.f));
                    }
                }
            }
            
            
        }
        
        void Lut::to_short_packed(unsigned short* /*to*/, const float* /*from*/,const RectI& /*conversionRect*/,
                                  const RectI& /*srcRoD*/,const RectI& /*dstRoD*/,
                                  PixelPacking /*inputPacking*/,PixelPacking /*outputPacking*/,bool /*invertY*/,bool /*premult*/) const {
            
            throw std::runtime_error("Lut::to_short_packed not implemented yet.");
            
        }
        
        void Lut::to_float_packed(float* to, const float* from,const RectI& conversionRect,
                                  const RectI& srcRoD,const RectI& dstRoD,
                                  PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const {
            
            ///if the conversion rectangle is out of the src rod and the dst rod, just return
            if(!intersects(conversionRect, srcRoD) || !intersects(conversionRect, dstRoD)){
                return;
            }
            
            ///clip the conversion rect to srcRoD and dstRoD
            RectI rect = conversionRect;
            clip(&rect,srcRoD);
            clip(&rect,dstRoD);
            
            bool inputHasAlpha = inputPacking == PACKING_BGRA || inputPacking == PACKING_RGBA;
            bool outputHasAlpha = outputPacking == PACKING_BGRA || outputPacking == PACKING_RGBA;
            
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
                    srcY = srcRoD.y2 - y - 1;
                }
                
                int dstY = dstRoD.y2 - y - 1;
                const float *src_pixels = from + (srcY * (srcRoD.x2 - srcRoD.x1) * inPackingSize);
                float *dst_pixels = to + (dstY * (dstRoD.x2 - dstRoD.x1) * outPackingSize);
                /* go fowards from starting point to end of line: */
                for (int x = rect.x1; x < rect.x2; ++x) {
                    int inCol = x * inPackingSize;
                    int outCol = x * outPackingSize;
                    float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;;
                    dst_pixels[outCol + outROffset] = toColorSpaceFloatFromLinearFloatFast(src_pixels[inCol + inROffset] * a);
                    dst_pixels[outCol + outGOffset] = toColorSpaceFloatFromLinearFloatFast(src_pixels[inCol + inGOffset] * a);
                    dst_pixels[outCol + outBOffset] = toColorSpaceFloatFromLinearFloatFast(src_pixels[inCol + inBOffset] * a);
                    if(outputHasAlpha) {
                        dst_pixels[outCol + outAOffset] = a;
                    }
                }
            }
            
        }
        
        
        
        void Lut::from_byte_planar(float* to,const unsigned char* from,int W,const unsigned char* alpha ,int inDelta,int outDelta) const {
            
            validate();
            if(!alpha){
                for (int f = 0,t = 0 ; f < W ; f += inDelta, t += outDelta) {
                    to[f] = from_byte_table[(int)from[f]];
                }
            }else{
                for (int f = 0,t = 0 ; f < W ; f += inDelta, t += outDelta) {
                    to[t] = from_byte_table[(from[f]*255 + 128) / alpha[f]] * alpha[f] / 255.;
                }
            }
            
        }
        
        void Lut::from_short_planar(float* /*to*/,const unsigned short* /*from*/,int /*W*/,
                                    const unsigned short*/* alpha*/ ,int /*inDelta*/,int/* outDelta*/) const {
            
            throw std::runtime_error("Lut::from_short_planar not implemented yet.");
        }
        
        void Lut::from_float_planar(float* to,const float* from,int W,const float* alpha ,int inDelta,int outDelta) const {
            
            validate();
            if(!alpha){
                for (int f = 0,t = 0 ; f < W ; f += inDelta, t += outDelta) {
                    to[t] = fromColorSpaceFloatToLinearFloatFast(from[f]);
                }
            }else{
                for (int f = 0,t = 0 ; f < W ; f += inDelta, t += outDelta) {
                    float a = alpha[f];
                    to[t] = fromColorSpaceFloatToLinearFloatFast(from[f] / a) * a;
                }
            }
        }
        
        void Lut::from_byte_packed(float* to, const unsigned char* from,const RectI& conversionRect,
                                   const RectI& srcRoD,const RectI& dstRoD,
                                   PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const {
            
            if(inputPacking == PACKING_PLANAR || outputPacking == PACKING_PLANAR){
                throw std::runtime_error("Invalid pixel format.");
            }
            
            ///if the conversion rectangle is out of the src rod and the dst rod, just return
            if(!intersects(conversionRect, srcRoD) || !intersects(conversionRect, dstRoD)){
                return;
            }
            
            ///clip the conversion rect to srcRoD and dstRoD
            RectI rect = conversionRect;
            clip(&rect,srcRoD);
            clip(&rect,dstRoD);
            
            bool inputHasAlpha = inputPacking == PACKING_BGRA || inputPacking == PACKING_RGBA;
            bool outputHasAlpha = outputPacking == PACKING_BGRA || outputPacking == PACKING_RGBA;
            
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
                    srcY = srcRoD.y2 - y - 1;
                }
                
                const unsigned char *src_pixels = from + (srcY * (srcRoD.x2 - srcRoD.x1) * inPackingSize);
                float *dst_pixels = to + (y * (dstRoD.x2 - dstRoD.x1) * outPackingSize);
                for (int x = rect.x1; x < rect.x2; ++x) {
                    int inCol = x * inPackingSize;
                    int outCol = x * outPackingSize;
                    float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] / 255.f : 1.f;
                    dst_pixels[outCol + outROffset] = from_byte_table[(int)(((src_pixels[inCol + inROffset] / 255.f) / a) * 255)] * a;
                    dst_pixels[outCol + outGOffset] = from_byte_table[(int)(((src_pixels[inCol + inGOffset] / 255.f) / a) * 255)] * a;
                    dst_pixels[outCol + outBOffset] = from_byte_table[(int)(((src_pixels[inCol + inBOffset] / 255.f) / a) * 255)] * a;
                    if (outputHasAlpha) {
                        dst_pixels[outCol + outAOffset] = a;
                    }
                }
            }
            
        }
        
        void Lut::from_short_packed(float* /*to*/, const unsigned short* /*from*/,const RectI& /*conversionRect*/,
                                    const RectI& /*srcRoD*/,const RectI& /*dstRoD*/,
                                    PixelPacking /*inputPacking*/,PixelPacking /*outputPacking*/,bool /*invertY*/,bool /*premult*/) const {
            
            throw std::runtime_error("Lut::from_short_packed not implemented yet.");
        }
        
        void Lut::from_float_packed(float* to, const float* from,const RectI& conversionRect,
                                    const RectI& srcRoD,const RectI& dstRoD,
                                    PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult) const {
            
            if(inputPacking == PACKING_PLANAR || outputPacking == PACKING_PLANAR){
                throw std::runtime_error("Invalid pixel format.");
            }
            
            ///if the conversion rectangle is out of the src rod and the dst rod, just return
            if(!intersects(conversionRect, srcRoD) || !intersects(conversionRect, dstRoD)){
                return;
            }
            
            ///clip the conversion rect to srcRoD and dstRoD
            RectI rect = conversionRect;
            clip(&rect,srcRoD);
            clip(&rect,dstRoD);
            
            bool inputHasAlpha = inputPacking == PACKING_BGRA || inputPacking == PACKING_RGBA;
            bool outputHasAlpha = outputPacking == PACKING_BGRA || outputPacking == PACKING_RGBA;
            
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
                    srcY = srcRoD.y2 - y - 1;
                }
                const float *src_pixels = from + (srcY * (srcRoD.x2 - srcRoD.x1) * inPackingSize);
                float *dst_pixels = to + (y * (dstRoD.x2 - dstRoD.x1) * outPackingSize);
                for (int x = rect.x1; x < rect.x2; ++x) {
                    int inCol = x * inPackingSize;
                    int outCol = x * outPackingSize;
                    float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;;
                    dst_pixels[outCol + outROffset] = fromColorSpaceFloatToLinearFloatFast((src_pixels[inCol + inROffset] / a) * 255.f) * a;
                    dst_pixels[outCol + outGOffset] = fromColorSpaceFloatToLinearFloatFast((src_pixels[inCol + inGOffset] / a) * 255.f) * a;
                    dst_pixels[outCol + outBOffset] = fromColorSpaceFloatToLinearFloatFast((src_pixels[inCol + inBOffset] /a) * 255.f) * a;
                    if(outputHasAlpha) {
                        dst_pixels[outCol + outAOffset] = a;
                    }
                    
                }
            }
        }
        
        ///////////////////////
        /////////////////////////////////////////// LINEAR //////////////////////////////////////////////
        ///////////////////////
        
        namespace Linear {
            
            void from_byte_planar(float *to, const unsigned char *from, int W, int inDelta,int outDelta)
            {
                from += (W - 1) * inDelta;
                to += W * outDelta;
                for (; --W >= 0; from -= inDelta) {
                    to -= outDelta;
                    *to = Linear::toFloat(*from);
                }
            }
            
            void from_short_planar(float *to, const unsigned short *from, int W,  int inDelta,int outDelta)
            {
                for (int f = 0,t = 0 ; f < W ; f += inDelta, t += outDelta) {
                    to[t] = Linear::toFloat(from[f]);
                }
                
            }
            
            void from_float_planar(float *to, const float *from, int W, int inDelta,int outDelta)
            {
                if(inDelta == 1 && outDelta == 1){
                    memcpy(to, from, W*sizeof(float));
                }else{
                    for (int f = 0,t = 0 ; f < W ; f += inDelta, t += outDelta) {
                        to[t] = from[f];
                    }
                }
            }
            
            
            void from_byte_packed(float *to, const unsigned char *from,
                                  const RectI &conversionRect,const RectI &srcRoD, const RectI &dstRoD,
                                  PixelPacking inputPacking,PixelPacking outputPacking,
                                  bool invertY )
            
            {
                
                if(inputPacking == PACKING_PLANAR || outputPacking == PACKING_PLANAR){
                    throw std::runtime_error("Invalid pixel format.");
                }
                
                ///if the conversion rectangle is out of the src rod and the dst rod, just return
                if(!intersects(conversionRect, srcRoD) || !intersects(conversionRect, dstRoD)){
                    return;
                }
                
                ///clip the conversion rect to srcRoD and dstRoD
                RectI rect = conversionRect;
                clip(&rect,srcRoD);
                clip(&rect,dstRoD);
                
                bool inputHasAlpha = inputPacking == PACKING_BGRA || inputPacking == PACKING_RGBA;
                bool outputHasAlpha = outputPacking == PACKING_BGRA || outputPacking == PACKING_RGBA;
                
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
                        srcY = srcRoD.y2 - y - 1;
                    }
                    const unsigned char *src_pixels = from + (srcY * (srcRoD.x2 - srcRoD.x1) * inPackingSize);
                    float *dst_pixels = to + (y * (dstRoD.x2 - dstRoD.x1) * outPackingSize);
                    for (int x = rect.x1; x < rect.x2; ++x) {
                        int inCol = x * inPackingSize;
                        int outCol = x * outPackingSize;
                        unsigned char a = inputHasAlpha ? src_pixels[inCol + inAOffset] : 255;
                        dst_pixels[outCol + outROffset] = src_pixels[inCol + inROffset] / 255.f;
                        dst_pixels[outCol + outGOffset] = src_pixels[inCol + inGOffset] / 255.f;
                        dst_pixels[outCol + outBOffset] = src_pixels[inCol + inBOffset] / 255.f;
                        if(outputHasAlpha) {
                            dst_pixels[outCol + outAOffset] = a / 255.f;
                        }
                    }
                }
                
            }
            
            void from_short_packed(float */*to*/, const unsigned short */*from*/,
                                   const RectI &/*rect*/,const RectI &/*srcRod*/, const RectI &/*rod*/,
                                   PixelPacking /*inputFormat*/,PixelPacking /*format*/,
                                   bool /*invertY*/)
            {
                throw std::runtime_error("Linear::from_short_packed not yet implemented.");
                
            }
            
            void from_float_packed(float *to, const float *from,
                                   const RectI &conversionRect,const RectI &srcRoD, const RectI &dstRoD,
                                   PixelPacking inputPacking,PixelPacking outputPacking,
                                   bool invertY)
            {
                if(inputPacking == PACKING_PLANAR || outputPacking == PACKING_PLANAR){
                    throw std::runtime_error("This function is not meant for planar buffers.");
                }
                
                
                ///if the conversion rectangle is out of the src rod and the dst rod, just return
                if(!intersects(conversionRect, srcRoD) || !intersects(conversionRect, dstRoD)){
                    return;
                }
                
                ///clip the conversion rect to srcRoD and dstRoD
                RectI rect = conversionRect;
                clip(&rect,srcRoD);
                clip(&rect,dstRoD);
                
                if(inputPacking == PACKING_PLANAR || outputPacking == PACKING_PLANAR){
                    throw std::runtime_error("Invalid pixel format.");
                }
                
                bool inputHasAlpha = inputPacking == PACKING_BGRA || inputPacking == PACKING_RGBA;
                bool outputHasAlpha = outputPacking == PACKING_BGRA || outputPacking == PACKING_RGBA;
                
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
                        srcY = srcRoD.y2 - y - 1;
                    }
                    const float *src_pixels = from + (srcY * (srcRoD.x2 - srcRoD.x1) * inPackingSize);
                    float *dst_pixels = to + (y * (dstRoD.x2 - dstRoD.x1) * outPackingSize);
                    if (inputPacking == outputPacking) {
                        memcpy(dst_pixels, src_pixels,(rect.x2 - rect.x1) * sizeof(float));
                    } else {
                        for (int x = rect.x1; x < rect.x2; ++x) {
                            int inCol = x * inPackingSize;
                            int outCol = x * outPackingSize;
                            float a = inputHasAlpha ? src_pixels[inCol + inAOffset] : 1.f;
                            dst_pixels[outCol + outROffset] = src_pixels[inCol + inROffset];
                            dst_pixels[outCol + outGOffset] = src_pixels[inCol + inGOffset];
                            dst_pixels[outCol + outBOffset] = src_pixels[inCol + inBOffset];
                            if(outputHasAlpha) {
                                dst_pixels[outCol + outAOffset] = a;
                            }
                        }
                    }
                }
            }
            
            void to_byte_planar(unsigned char *to, const float *from, int W,const float* alpha, int inDelta,int outDelta)
            {
                if(!alpha){
                    unsigned char *end = to + W * outDelta;
                    int start = rand() % W;
                    const float *q;
                    unsigned char *p;
                    /* go fowards from starting point to end of line: */
                    float error = .5;
                    for (p = to + start * outDelta, q = from + start * inDelta; p < end; p += outDelta , q += inDelta) {
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
                }else{
                    unsigned char *end = to + W * outDelta;
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
            }
            
            
            
            void to_short_planar(unsigned short *to, const float *from, int W, const float* alpha ,int inDelta,int outDelta)
            {
                (void)to;
                (void)from;
                (void)W;
                (void)alpha;
                (void)inDelta;
                (void)outDelta;
                throw std::runtime_error("Linear::to_short_planar not yet implemented.");
            }
            
            
            void to_float_planar(float *to, const float *from, int W,const float* alpha ,int inDelta,int outDelta)
            {
                if(!alpha){
                    if (inDelta == 1 && outDelta == 1) {
                        memcpy(to, from, W * sizeof(float));
                    } else {
                        for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
                            to[t] = from[f];
                        }
                    }
                }else{
                    for (int f = 0,t = 0; f < W; f += inDelta, t += outDelta) {
                        to[t] = from[f] * alpha[f];
                    }

                }
            }
            
            void to_byte_packed(unsigned char* to, const float* from,const RectI& conversionRect,
                                const RectI& srcRoD,const RectI& dstRoD,
                                PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult)
            {
                
                if(inputPacking == PACKING_PLANAR || outputPacking == PACKING_PLANAR){
                    throw std::runtime_error("This function is not meant for planar buffers.");
                }
                
                ///if the conversion rectangle is out of the src rod and the dst rod, just return
                if(!intersects(conversionRect, srcRoD) || !intersects(conversionRect, dstRoD)){
                    return;
                }
                
                ///clip the conversion rect to srcRoD and dstRoD
                RectI rect = conversionRect;
                clip(&rect,srcRoD);
                
                bool inputHasAlpha = inputPacking == PACKING_BGRA || inputPacking == PACKING_RGBA;
                bool outputHasAlpha = outputPacking == PACKING_BGRA || outputPacking == PACKING_RGBA;
                
                int inROffset, inGOffset, inBOffset, inAOffset;
                int outROffset, outGOffset, outBOffset, outAOffset;
                getOffsetsForPacking(inputPacking, &inROffset, &inGOffset, &inBOffset, &inAOffset);
                getOffsetsForPacking(outputPacking, &outROffset, &outGOffset, &outBOffset, &outAOffset);
                
                int inPackingSize,outPackingSize;
                inPackingSize = inputHasAlpha ? 4 : 3;
                outPackingSize = outputHasAlpha ? 4 : 3;
                
                for (int y = rect.y1; y < rect.y2; ++y) {
                    int start = rand() % (rect.x2 - rect.x1) + rect.x1;
                    unsigned error_r, error_g, error_b;
                    error_r = error_g = error_b = 0x80;
                    int srcY = y;
                    if (invertY) {
                        srcY = srcRoD.y2 - y - 1;
                    }
                    
                    const float *src_pixels = from + (srcY * (srcRoD.x2 - srcRoD.x1) * inPackingSize);
                    unsigned char *dst_pixels = to + (y * (dstRoD.x2 - dstRoD.x1) * outPackingSize);
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
                        if(outputHasAlpha) {
                            dst_pixels[outCol + outAOffset] = (unsigned char)(std::min(a * 256.f, 255.f));
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
                        if(outputHasAlpha) {
                            dst_pixels[outCol + outAOffset] = (unsigned char)(std::min(a * 256.f, 255.f));
                        }
                    }
                }
            }
            
            
            
            void to_short_packed(unsigned short* to, const float* from,const RectI& conversionRect,
                                 const RectI& srcRoD,const RectI& dstRoD,
                                 PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult)
            {
                (void)to;
                (void)from;
                (void)conversionRect;
                (void)srcRoD;
                (void)dstRoD;
                (void)invertY;
                (void)premult;
                (void)inputPacking;
                (void)outputPacking;
                throw std::runtime_error("Linear::to_short_packed not yet implemented.");
            }
            
            void to_float_packed(float* to, const float* from,const RectI& conversionRect,
                                 const RectI& srcRoD,const RectI& dstRoD,
                                 PixelPacking inputPacking,PixelPacking outputPacking,bool invertY,bool premult)
            {
                if(inputPacking == PACKING_PLANAR || outputPacking == PACKING_PLANAR){
                    throw std::runtime_error("Invalid pixel format.");
                }
                
                ///if the conversion rectangle is out of the src rod and the dst rod, just return
                if(!intersects(conversionRect, srcRoD) || !intersects(conversionRect, dstRoD)){
                    return;
                }
                
                ///clip the conversion rect to srcRoD and dstRoD
                RectI rect = conversionRect;
                clip(&rect,srcRoD);
                
                bool inputHasAlpha = inputPacking == PACKING_BGRA || inputPacking == PACKING_RGBA;
                bool outputHasAlpha = outputPacking == PACKING_BGRA || outputPacking == PACKING_RGBA;
                
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
                        srcY = srcRoD.y2 - y - 1;
                    }
                    
                    const float *src_pixels = from + (srcY * (srcRoD.x2 - srcRoD.x1) * inPackingSize);
                    float *dst_pixels = to + (y * (dstRoD.x2 - dstRoD.x1) * outPackingSize);
                    if (inputPacking == outputPacking && !premult) {
                        memcpy(dst_pixels, src_pixels, (rect.x2 - rect.x1) *sizeof(float));
                    } else {
                        for (int x = rect.x1; x < conversionRect.x2; ++x) {
                            
                            int inCol = x * inPackingSize;
                            int outCol = x * outPackingSize;
                            
                            float a = (inputHasAlpha && premult) ? src_pixels[inCol + inAOffset] : 1.f;
                            
                            dst_pixels[outCol + outROffset] = src_pixels[inCol + inROffset] * a;
                            dst_pixels[outCol + outGOffset] = src_pixels[inCol + inGOffset] * a;
                            dst_pixels[outCol + outBOffset] = src_pixels[inCol + inBOffset] * a;
                            if(outputHasAlpha){
                                dst_pixels[outCol + outAOffset] = a;
                            }
                        }
                    }
                }
            }
        }
        
        const Lut* LutManager::sRGBLut(){
            return LutManager::m_instance.getLut("sRGB",from_func_srgb,to_func_srgb);
        }
        
        float from_func_Rec709(float v){
            if (v < 0.081f)
                return (v < 0.0f) ? 0.0f : v * (1.0f / 4.5f);
            else
                return std::pow((v + 0.099f) * (1.0f / 1.099f), (1.0f / 0.45f));
        }
        
        float to_func_Rec709(float v){
            if (v < 0.018f)
                return (v < 0.0f) ? 0.0f : v * 4.5f;
            else
                return 1.099f * std::pow(v, 0.45f) - 0.099f;
        }
        
        const Lut* LutManager::Rec709Lut(){
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
        float from_func_Cineon(float v){
            return (1.f / (1.f - std::pow(10.f,1.97f))) * std::pow(10.f,((1023.f * v) - 685.f) * 0.002f / 0.6f);
        }
        
        float to_func_Cineon(float v){
            float offset = std::pow(10.f,1.97f);
            return (std::log10((v + offset) / (1.f / (1.f - offset))) / 0.0033f + 685.0f) / 1023.f;
        }
        
        const Lut* LutManager::CineonLut(){
            return LutManager::m_instance.getLut("Cineon",from_func_Cineon,to_func_Cineon);
        }
        
        float from_func_Gamma1_8(float v){
            return std::pow(v, 0.55f);
        }
        
        float to_func_Gamma1_8(float v){
            return std::pow(v, 1.8f);
        }
        
        const Lut* LutManager::Gamma1_8Lut(){
            return LutManager::m_instance.getLut("Gamma1_8",from_func_Gamma1_8,to_func_Gamma1_8);
        }
        
        float from_func_Gamma2_2(float v){
            return std::pow(v, 0.45f);
        }
        
        float to_func_Gamma2_2(float v){
            return std::pow(v, 2.2f);
        }
        
        const Lut* LutManager::Gamma2_2Lut(){
            return LutManager::m_instance.getLut("Gamma2_2",from_func_Gamma2_2,to_func_Gamma2_2);
        }
        
        float from_func_Panalog(float v){
            return (std::pow(10.f,(1023.f * v - 681.f) / 444.f) - 0.0408) / 0.96f;
        }
        
        float to_func_Panalog(float v){
            return (444.f * std::log10(0.0408 + 0.96f * v) + 681.f) / 1023.f;
        }
        
        const Lut* LutManager::PanaLogLut(){
            return LutManager::m_instance.getLut("PanaLog",from_func_Panalog,to_func_Panalog);
        }
        
        float from_func_ViperLog(float v){
            return std::pow(10.f,(1023.f * v - 1023.f) / 500.f);
        }
        
        float to_func_ViperLog(float v){
            return (500.f * std::log10(v) + 1023.f) / 1023.f;
        }
        
        const Lut* LutManager::ViperLogLut(){
            return LutManager::m_instance.getLut("ViperLog",from_func_ViperLog,to_func_ViperLog);
        }
        
        float from_func_RedLog(float v){
            return (std::pow(10.f,( 1023.f * v - 1023.f ) / 511.f) - 0.01f) / 0.99f;
        }
        
        float to_func_RedLog(float v){
            return (511.f * std::log10(0.01f + 0.99f * v) + 1023.f) / 1023.f;
        }
        
        const Lut* LutManager::RedLogLut(){
            return LutManager::m_instance.getLut("RedLog",from_func_RedLog,to_func_RedLog);
        }
        
        float from_func_AlexaV3LogC(float v){
            return v > 0.1496582f ? std::pow(10.f,(v - 0.385537f) / 0.2471896f) * 0.18f - 0.00937677f
            : ( v / 0.9661776f - 0.04378604) * 0.18f - 0.00937677f;
        }
        
        float to_func_AlexaV3LogC(float v){
            return v > 0.010591f ?  0.247190f * std::log10(5.555556f * v + 0.052272f) + 0.385537f
            : v * 5.367655f + 0.092809f;
        }
        
        const Lut* LutManager::AlexaV3LogCLut(){
            return LutManager::m_instance.getLut("AlexaV3LogC",from_func_AlexaV3LogC,to_func_AlexaV3LogC);
        }
        
    }
}
