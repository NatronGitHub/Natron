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
#include "OfxClipInstance.h"

#include <cfloat>
#include <limits>

#include "Engine/OfxNode.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/ImageFetcher.h"

#include "Global/AppManager.h"
#include "Global/Macros.h"

using namespace Powiter;
using namespace std;

OfxClipInstance::OfxClipInstance(OfxNode* nodeInstance
                                 ,Powiter::OfxImageEffectInstance* effect
                                 ,int /*index*/
                                 , OFX::Host::ImageEffect::ClipDescriptor* desc)
: OFX::Host::ImageEffect::ClipInstance(effect, *desc)
, _nodeInstance(nodeInstance)
, _effect(effect)
, _outputImage(NULL)
{
}

/// Get the Raw Unmapped Pixel Depth from the host. We are always 8 bits in our example
const std::string& OfxClipInstance::getUnmappedBitDepth() const
{
    // we always use floats
    static const std::string v(kOfxBitDepthFloat);
    return v;
}

/// Get the Raw Unmapped Components from the host. In our example we are always RGBA
const std::string &OfxClipInstance::getUnmappedComponents() const
{
    static const string rgbStr(kOfxImageComponentRGB);
    static const string noneStr(kOfxImageComponentNone);
    static const string rgbaStr(kOfxImageComponentRGBA);
    static const string alphaStr(kOfxImageComponentAlpha);
    
    //bool rgb = false;
    //bool alpha = false;
    
    //const ChannelSet& channels = _effect->info().channels();
    //if(channels & alpha) alpha = true;
    //if(channels & Mask_RGB) rgb = true;
    
//    if(!rgb && !alpha) return noneStr;
//    else if(rgb && !alpha) return rgbStr;
//    else if(!rgb && alpha) return alphaStr;
     return rgbaStr;
}


// PreMultiplication -
//
//  kOfxImageOpaque - the image is opaque and so has no premultiplication state
//  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
//  kOfxImageUnPreMultiplied - the image is unpremultiplied
const std::string &OfxClipInstance::getPremult() const
{
    static const std::string v(kOfxImageUnPreMultiplied);
    return v;
}


// Pixel Aspect Ratio -
//
//  The pixel aspect ratio of a clip or image.
double OfxClipInstance::getAspectRatio() const
{
    return _effect->getProjectPixelAspectRatio();
}

// Frame Rate -
double OfxClipInstance::getFrameRate() const
{
    return _effect->getFrameRate();
}

// Frame Range (startFrame, endFrame) -
//
//  The frame range over which a clip has images.
void OfxClipInstance::getFrameRange(double &startFrame, double &endFrame) const
{
    SequenceTime first = 0,last = 0;
    Node* n = getAssociatedNode();
    if(n)
        n->getFrameRange(&first, &last);
    startFrame = first;
    endFrame = last;
//    if (_effect->getContext() == kOfxImageEffectContextGenerator) {
//        startFrame = 0;
//        endFrame = 0;
//    }else{
//        startFrame = _nodeInstance->info().getFirstFrame();
//        endFrame = _nodeInstance->info().getLastFrame();
//    }
}

/// Field Order - Which spatial field occurs temporally first in a frame.
/// \returns
///  - kOfxImageFieldNone - the clip material is unfielded
///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
const std::string &OfxClipInstance::getFieldOrder() const
{
    return _effect->getDefaultOutputFielding();
}

// Connected -
//
//  Says whether the clip is actually connected at the moment.
bool OfxClipInstance::getConnected() const
{
    if(!_nodeInstance->isOutputNode())
        return _nodeInstance->hasOutputConnected();
    else
        return true;
}

// Unmapped Frame Rate -
//
//  The unmaped frame range over which an output clip has images.
double OfxClipInstance::getUnmappedFrameRate() const
{
    
    return 25;
}

// Unmapped Frame Range -
//
//  The unmaped frame range over which an output clip has images.
// this is applicable only to hosts and plugins that allow a plugin to change frame rates
void OfxClipInstance::getUnmappedFrameRange(double &unmappedStartFrame, double &unmappedEndFrame) const
{
    unmappedStartFrame = 1;
    unmappedEndFrame = 1;
}

// Continuous Samples -
//
//  0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
//  1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
bool OfxClipInstance::getContinuousSamples() const
{
    return false;
}


/// override this to return the rod on the clip cannoical coords!
OfxRectD OfxClipInstance::getRegionOfDefinition(OfxTime time) const
{
    OfxRectD ret;
    Box2D rod;
    Node* n = getAssociatedNode();
    if(n){
        n->getRegionOfDefinition(time,&rod);
        ret.x1 = rod.left();
        ret.x2 = rod.right();
        ret.y1 = rod.bottom();
        ret.y2 = rod.top();
    }
    else{
        _nodeInstance->effectInstance()->getProjectOffset(ret.x1, ret.y1);
        _nodeInstance->effectInstance()->getProjectExtent(ret.x2, ret.y2);
    }
    return ret;
}


/// override this to fill in the image at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Image* OfxClipInstance::getImage(OfxTime time, OfxRectD *optionalBounds)
{
    OfxRectD roi;
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    if(optionalBounds){
        roi = *optionalBounds;
    }else{
        assert(_nodeInstance->effectInstance());
        _nodeInstance->effectInstance()->getRegionOfDefinitionAction(time,renderScale,roi);
       
    }
    //cout << "Request image for clip " << getName() << endl;
    //cout << " l = " << roi.x1 << " b = " << roi.y1 << " r = " << roi.x2 << " t = " << roi.y2 << endl;
    /*SHOULD CHECK WHAT BIT DEPTH IS SUPPORTED BY THE PLUGIN INSTEAD OF GIVING FLOAT
     _effect->isPixelDepthSupported(...)
     */
    if(isOutput()){
        
        QMutexLocker locker(&_lock);
        if (!_outputImage) {
            _outputImage = new OfxImage(OfxImage::eBitDepthFloat,roi,*this,0);
        }else{
            OfxRectI outputImageBounds = _outputImage->getROD();
            if(outputImageBounds.x1 != roi.x1 ||
               outputImageBounds.x2 != roi.x2 ||
               outputImageBounds.y1 != roi.y1 ||
               outputImageBounds.y2 != roi.y2){
                delete _outputImage;
                _outputImage = new OfxImage(OfxImage::eBitDepthFloat,roi,*this,0);
            }
        }
        _outputImage->addReference();
        return _outputImage;
    }else{
        Node* input = getAssociatedNode();
        if(isOptional() && !input) {
            return  new OfxImage(OfxImage::eBitDepthFloat,roi,*this,0);
        }
        
        assert(input);
        if(input->isOpenFXNode()){
            OfxNode* ofxNode = dynamic_cast<OfxNode*>(input);
            OfxRectI roiInput;
            roiInput.x1 = (int)std::floor(roi.x1);
            roiInput.x2 = (int)std::ceil(roi.x2);
            roiInput.y1 = (int)std::floor(roi.y1);
            roiInput.y2 = (int)std::ceil(roi.y2);
            assert(ofxNode->effectInstance());
            OfxStatus stat = ofxNode->effectInstance()->renderAction(time, kOfxImageFieldNone, roiInput , renderScale);
            assert(stat == kOfxStatOK);
            OFX::Host::ImageEffect::ClipInstance* clip = ofxNode->effectInstance()->getClip(kOfxImageEffectOutputClipName);
            assert(clip);
            return clip->getImage(time, optionalBounds);
        } else {
            ChannelSet channels(Mask_RGBA);
            Box2D roiInput;
            input->getRegionOfDefinition(time, &roiInput);
            ImageFetcher srcImg(input,
                                time,
                                roiInput.left(),roiInput.bottom(),roiInput.right(),roiInput.top(),
                                channels);
            srcImg.claimInterest(true);
            OfxImage* ret = new OfxImage(OfxImage::eBitDepthFloat,roi,*this,0);
            assert(ret);
//            cout << "Input image: l = " << roiInput.left() << " b = " << roiInput.bottom() <<
//            " r = " << roiInput.right() << " t = " << roiInput.top() << endl;
//            cout << "Output image: l = " << roi.x1 << " b = " << roi.y1 <<
//            " r = " << roi.x2 << " t = " << roi.y2 << endl;
            /*Copying all rows living in the ImageFetcher to the ofx image*/
            for (int y = std::max((int)roi.y1,roiInput.bottom()); y < (std::min((int)roi.y2,roiInput.top())); ++y) {
                OfxRGBAColourF* dstImg = ret->pixelF(roi.x1, y);
                assert(dstImg);
                boost::shared_ptr<const Row> row = srcImg.at(y);
                assert(row);
                
                foreachChannels(z, channels){
                    rowPlaneToOfxPackedBuffer(z, row->begin(z), std::min(row->width(),(int)(roi.x2 - roi.x1)), dstImg);
                }
                srcImg.erase(y);
            }
            
            return ret;
        }
    }
    return NULL;
}

OfxImage::OfxImage(BitDepthEnum bitDepth,const OfxRectD& bounds,OfxClipInstance &clip, OfxTime /*t*/):
OFX::Host::ImageEffect::Image(clip),_bitDepth(bitDepth){
    size_t pixSize = 0;
    if(bitDepth == eBitDepthUByte){
        pixSize = 4;
    }else if(bitDepth == eBitDepthUShort){
        pixSize = 8;
    }else if(bitDepth == eBitDepthFloat){
        pixSize = 16;
    }
    assert(bounds.x1 != kOfxFlagInfiniteMin); // what should we do in this case?
    assert(bounds.y1 != kOfxFlagInfiniteMin);
    assert(bounds.x2 != kOfxFlagInfiniteMax);
    assert(bounds.y2 != kOfxFlagInfiniteMax);
    assert(bounds.x1 != -std::numeric_limits<double>::infinity()); // what should we do in this case?
    assert(bounds.y1 != -std::numeric_limits<double>::infinity());
    assert(bounds.x2 != std::numeric_limits<double>::infinity());
    assert(bounds.y2 != std::numeric_limits<double>::infinity());
    int xmin = (int)std::floor(bounds.x1);
    int xmax = (int)std::ceil(bounds.x2);
    _rowBytes = (xmax - xmin) * pixSize;
    int ymin = (int)std::floor(bounds.y1);
    int ymax = (int)std::ceil(bounds.y2);

    _data = malloc(_rowBytes * (ymax-ymin)) ;
    
    // render scale x and y of 1.0
    setDoubleProperty(kOfxImageEffectPropRenderScale, 1.0, 0);
    setDoubleProperty(kOfxImageEffectPropRenderScale, 1.0, 1);
    // data ptr
    setPointerProperty(kOfxImagePropData,_data);
    // bounds and rod
    setIntProperty(kOfxImagePropBounds, xmin, 0);
    setIntProperty(kOfxImagePropBounds, ymin, 1);
    setIntProperty(kOfxImagePropBounds, xmax, 2);
    setIntProperty(kOfxImagePropBounds, ymax, 3);
    setIntProperty(kOfxImagePropRegionOfDefinition, xmin, 0);
    setIntProperty(kOfxImagePropRegionOfDefinition, ymin, 1);
    setIntProperty(kOfxImagePropRegionOfDefinition, xmax, 2);
    setIntProperty(kOfxImagePropRegionOfDefinition, ymax, 3);
    // row bytes
    setIntProperty(kOfxImagePropRowBytes, _rowBytes);
    setStringProperty(kOfxImageEffectPropComponents, kOfxImageComponentRGBA);
}

OfxRGBAColourB* OfxImage::pixelB(int x, int y) const{
    assert(_bitDepth == eBitDepthUByte);
    OfxRectI bounds = getBounds();
    if ((x >= bounds.x1) && ( x< bounds.x2) && ( y >= bounds.y1) && ( y < bounds.y2) )
    {
        OfxRGBAColourB* p = reinterpret_cast<OfxRGBAColourB*>((U8*)_data + (y-bounds.y1)*_rowBytes);
        return &(p[x - bounds.x1]);
    }
    return 0;
}
OfxRGBAColourS* OfxImage::pixelS(int x, int y) const{
    assert(_bitDepth == eBitDepthUShort);
    OfxRectI bounds = getBounds();
    if ((x >= bounds.x1) && ( x< bounds.x2) && ( y >= bounds.y1) && ( y < bounds.y2) )
    {
        OfxRGBAColourS* p = reinterpret_cast<OfxRGBAColourS*>((U8*)_data + (y-bounds.y1)*_rowBytes);
        return &(p[x - bounds.x1]);
    }
    return 0;
}
OfxRGBAColourF* OfxImage::pixelF(int x, int y) const{
    assert(_bitDepth == eBitDepthFloat);
    OfxRectI bounds = getBounds();
    if ((x >= bounds.x1) && ( x< bounds.x2) && ( y >= bounds.y1) && ( y < bounds.y2) )
    {
        OfxRGBAColourF* p = reinterpret_cast<OfxRGBAColourF*>((U8*)_data + (y-bounds.y1)*_rowBytes);
        return &(p[x - bounds.x1]);
    }
    return 0;
}

Node* OfxClipInstance::getAssociatedNode() const {
    if(_isOutput)
        return _nodeInstance;
    else{
        int index = 0;
        OfxNode::MappedInputV inputs = _nodeInstance->inputClipsCopyWithoutOutput();
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i]->getName() == getName()) {
                index = i;
                break;
            }
        }
        return _nodeInstance->input(inputs.size()-1-index);
    }
}

