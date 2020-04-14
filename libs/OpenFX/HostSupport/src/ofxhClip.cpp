/*
Software License :

Copyright (c) 2007-2009, The Open Effects Association Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Open Effects Association Ltd, nor the names of its 
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>

// ofx
#include "ofxCore.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhImageEffect.h"
#ifdef OFX_EXTENSIONS_VEGAS
#include "ofxSonyVegas.h"
#endif
#ifdef OFX_EXTENSIONS_NUKE
#include <nuke/fnOfxExtensions.h>
#endif
#ifdef OFX_EXTENSIONS_NATRON
#include "ofxNatron.h"
#endif
#ifdef OFX_SUPPORTS_OPENGLRENDER
#include "ofxOpenGLRender.h"
#endif

namespace OFX {

  namespace Host {

    namespace ImageEffect {

      /// properties common to the desciptor and instance
      /// the desc and set them, the instance cannot
      static const Property::PropSpec clipDescriptorStuffs[] = {
        { kOfxPropType, Property::eString, 1, true, kOfxTypeClip },
        { kOfxPropName, Property::eString, 1, true, "SET ME ON CONSTRUCTION" },
        { kOfxPropLabel, Property::eString, 1, false, "" } ,
#ifdef OFX_EXTENSIONS_NATRON
        { kOfxParamPropSecret, Property::eInt, 1, false, "0"},
        { kOfxParamPropHint, Property::eString, 1, false, "" } ,
#endif
        { kOfxPropShortLabel, Property::eString, 1, false, "" },
        { kOfxPropLongLabel, Property::eString, 1, false, "" },        
        { kOfxImageEffectPropSupportedComponents, Property::eString, 0, false, "" },
        { kOfxImageEffectPropTemporalClipAccess,   Property::eInt, 1, false, "0" },
        { kOfxImageClipPropOptional, Property::eInt, 1, false, "0" },
        { kOfxImageClipPropIsMask,   Property::eInt, 1, false, "0" },
        { kOfxImageClipPropFieldExtraction, Property::eString, 1, false, kOfxImageFieldDoubled },
        { kOfxImageEffectPropSupportsTiles,   Property::eInt, 1, false, "1" },
#ifdef OFX_EXTENSIONS_NUKE
        { kFnOfxImageEffectCanTransform,   Property::eInt, 1, false, "0" }, // can a kFnOfxPropMatrix2D be attached to images on this clip
#endif
#ifdef OFX_EXTENSIONS_NATRON
        { kOfxImageEffectPropCanDistort,   Property::eInt, 1, false, "0" }, // can a distortion function be attached to images on this clip
#endif
        Property::propSpecEnd,
      };
      
      ////////////////////////////////////////////////////////////////////////////////
      // props to clips descriptors and instances

      // base ctor, for a descriptor
      ClipBase::ClipBase()
        : _properties(clipDescriptorStuffs) 
      {
      }

      /// props to clips and 
      ClipBase::ClipBase(const ClipBase &v)
        : _properties(v._properties) 
      {
        /// we are an instance, we need to reset the props to read only
        const Property::PropertyMap &map = _properties.getProperties();
        Property::PropertyMap::const_iterator i;
        for(i = map.begin(); i != map.end(); ++i) {
          (*i).second->setPluginReadOnly(false);
        } 
      }

      /// name of the clip
      const std::string &ClipBase::getShortLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropShortLabel);
        if(s == "") {
          return getLabel();
        }
        return s;
      }
      
      /// name of the clip
      const std::string &ClipBase::getLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropLabel);
        if(s == "") {
          return getName();
        }
        return s;
      }
      
      /// name of the clip
      const std::string &ClipBase::getLongLabel() const
      {
        const std::string &s = _properties.getStringProperty(kOfxPropLongLabel);
        if(s == "") {
          return getLabel();
        }
        return s;
      }
      
#ifdef OFX_EXTENSIONS_NATRON
      /// doc of the clip
      const std::string &ClipBase::getHint() const
      {
        const std::string &s = _properties.getStringProperty(kOfxParamPropHint);
        return s;
      }

      /// is the clip secret
      bool ClipBase::isSecret() const
      {
        return _properties.getIntProperty(kOfxParamPropSecret) != 0;
      }
      

#endif
      /// return a std::vector of supported comp
      const std::vector<std::string> &ClipBase::getSupportedComponents() const
      {
        Property::String *p =  _properties.fetchStringProperty(kOfxImageEffectPropSupportedComponents);
        assert(p != NULL);
        return p->getValues();
      }
      
      /// is the given component supported
      bool ClipBase::isSupportedComponent(const std::string &comp) const
      {
        return _properties.findStringPropValueIndex(kOfxImageEffectPropSupportedComponents, comp) != -1;
      }
      
      /// does the clip do random temporal access
      bool ClipBase::temporalAccess() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropTemporalClipAccess) != 0;
      }
      
      /// is the clip optional
      bool ClipBase::isOptional() const
      {
        return _properties.getIntProperty(kOfxImageClipPropOptional) != 0;
      }
      
      /// is the clip a nominal 'mask' clip
      bool ClipBase::isMask() const
      {
        return _properties.getIntProperty(kOfxImageClipPropIsMask) != 0;
      }
      
      /// how does this clip like fielded images to be presented to it
      const std::string &ClipBase::getFieldExtraction() const
      {
        return _properties.getStringProperty(kOfxImageClipPropFieldExtraction);
      }
      
      /// is the clip a nominal 'mask' clip
      bool ClipBase::supportsTiles() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropSupportsTiles) != 0;
      }

#ifdef OFX_EXTENSIONS_NUKE
      /// can a kFnOfxPropMatrix2D be attached to images on this clip
      bool ClipBase::canTransform() const
      {
        return _properties.getIntProperty(kFnOfxImageEffectCanTransform) != 0;
      }
#endif

#ifdef OFX_EXTENSIONS_NATRON
      /// can a distortion function be attached to images on this clip
      bool ClipBase::canDistort() const
      {
        return _properties.getIntProperty(kOfxImageEffectPropCanDistort) != 0;
      }
#endif

      const Property::Set& ClipBase::getProps() const
      {
        return _properties;
      }

      Property::Set& ClipBase::getProps() 
      {
        return _properties;
      }

      /// get a handle on the properties of the clip descriptor for the C api
      OfxPropertySetHandle ClipBase::getPropHandle() const
      {
        return _properties.getHandle();
      }

      /// get a handle on the clip descriptor for the C api
      OfxImageClipHandle ClipBase::getHandle() const
      {
        return (OfxImageClipHandle)this;
      }

      ////////////////////////////////////////////////////////////////////////////////
      /// descriptor
      ClipDescriptor::ClipDescriptor(const std::string &name)
        : ClipBase()
      {
        _properties.setStringProperty(kOfxPropName,name);
      }
      
      /// extra properties for the instance, these are fetched from the host
      /// via a get hook and some virtuals
      static const Property::PropSpec clipInstanceStuffs[] = {
        { kOfxImageEffectPropPixelDepth, Property::eString, 1, true, kOfxBitDepthNone },
        { kOfxImageEffectPropComponents, Property::eString, 1, true, kOfxImageComponentNone },
        { kOfxImageClipPropUnmappedPixelDepth, Property::eString, 1, true, kOfxBitDepthNone },
        { kOfxImageClipPropUnmappedComponents, Property::eString, 1, true, kOfxImageComponentNone },
        { kOfxImageEffectPropPreMultiplication, Property::eString, 1, true, kOfxImageOpaque },
        { kOfxImagePropPixelAspectRatio, Property::eDouble, 1, true, "1.0" },
        { kOfxImageEffectPropFrameRate, Property::eDouble, 1, true, "25.0" },
        { kOfxImageEffectPropFrameRange, Property::eDouble, 2, true, "0" },
        { kOfxImageClipPropFieldOrder, Property::eString, 1, true, kOfxImageFieldNone },
        { kOfxImageClipPropConnected, Property::eInt, 1, true, "0" },
        { kOfxImageEffectPropUnmappedFrameRange, Property::eDouble, 2, true, "0" },
        { kOfxImageEffectPropUnmappedFrameRate, Property::eDouble, 1, true, "25.0" },
        { kOfxImageClipPropContinuousSamples, Property::eInt, 1, true, "0" },
#ifdef OFX_EXTENSIONS_RESOLVE
        { kOfxImageClipPropThumbnail, Property::eInt, 1, true, "0" },
#endif
#ifdef OFX_EXTENSIONS_VEGAS
        { kOfxImagePropPixelOrder, Property::eInt, 1, true, kOfxImagePixelOrderRGBA },
#endif
#ifdef OFX_EXTENSIONS_NUKE
        { kFnOfxImageEffectPropComponentsPresent, Property::eString, 0, true, kOfxImageComponentNone},
#endif
#ifdef OFX_EXTENSIONS_NATRON
        { kOfxImageClipPropFormat, Property::eInt, 4, true, "0"},
#endif
        Property::propSpecEnd,
      };


      ////////////////////////////////////////////////////////////////////////////////
      // instance
      ClipInstance::ClipInstance(ImageEffect::Instance* effectInstance, ClipDescriptor& desc) 
        : ClipBase(desc)
        , _effectInstance(effectInstance)
        , _isOutput(desc.isOutput())
        , _pixelDepth(kOfxBitDepthNone) 
        , _components(kOfxImageComponentNone)
      {
        // this will add parameters that are needed in an instance but not a
        // Descriptor
        _properties.addProperties(clipInstanceStuffs);
        int i = 0;
        while(clipInstanceStuffs[i].name) {
          const Property::PropSpec& spec = clipInstanceStuffs[i];

          switch (spec.type) {
          case Property::eDouble:
          case Property::eString:
          case Property::eInt:
            _properties.setGetHook(spec.name, this);
            break;
          default:
            break;
          }

          i++;
        }
#     ifdef OFX_EXTENSIONS_NATRON
        _properties.addNotifyHook(kOfxPropLabel, this);
        _properties.addNotifyHook(kOfxParamPropSecret, this);
        _properties.addNotifyHook(kOfxParamPropHint, this);
#     endif
      }

#     ifdef OFX_EXTENSIONS_NATRON
      // callback which should update label
      void ClipInstance::setLabel()
      {
      }
      
      // callback which should set secret state as appropriate
      void ClipInstance::setSecret()
      {
      }
      
      // callback which should update hint
      void ClipInstance::setHint()
      {
      }
#     endif

      // do nothing
      int ClipInstance::getDimension(const std::string &name) const OFX_EXCEPTION_SPEC 
      {
        if(name == kOfxImageEffectPropUnmappedFrameRange || name == kOfxImageEffectPropFrameRange)
          return 2;
        return 1;
      }

      // don't know what to do
      void ClipInstance::reset(const std::string &/*name*/) OFX_EXCEPTION_SPEC {
        //printf("failing in %s\n", __PRETTY_FUNCTION__);
        throw Property::Exception(kOfxStatErrMissingHostFeature);
      }

      const std::string &ClipInstance::getPixelDepth() const
      {
        return _pixelDepth;
      }


      void ClipInstance::setPixelDepth(const std::string &s)
      {
        _pixelDepth =  s;
      }

      const std::string &ClipInstance::getComponents() const
      {
        return _components;
      }
      
      /// set the current set of components
      /// called by clip preferences action 
      void ClipInstance::setComponents(const std::string &s)
      {
        _components = s;
      }
       
#ifdef OFX_EXTENSIONS_NUKE
      const std::vector<std::string>& ClipInstance::getComponentsPresent() const OFX_EXCEPTION_SPEC
      {
          static std::vector<std::string> components;
          if (components.empty()) {
              components.resize(1);
          }
          components[0] = _components;
          return components;
      }
#endif
      // get the virutals for viewport size, pixel scale, background colour
      void ClipInstance::getDoublePropertyN(const std::string &name, double *values, int n) const OFX_EXCEPTION_SPEC
      {
        if(n<=0) throw Property::Exception(kOfxStatErrValue);
        if(name==kOfxImagePropPixelAspectRatio){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          *values = getAspectRatio();
        }
        else if(name==kOfxImageEffectPropFrameRate){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          *values = getFrameRate();
        }
        else if(name==kOfxImageEffectPropFrameRange){
          if(n>2) throw Property::Exception(kOfxStatErrValue);
          getFrameRange(values[0], values[1]);
        }
        else if(name==kOfxImageEffectPropUnmappedFrameRate){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          *values =  getUnmappedFrameRate();
        }
        else if(name==kOfxImageEffectPropUnmappedFrameRange){
          if(n>2) throw Property::Exception(kOfxStatErrValue);
          getUnmappedFrameRange(values[0], values[1]);
        }
        else
          throw Property::Exception(kOfxStatErrValue);
      }

      // get the virutals for viewport size, pixel scale, background colour
      double ClipInstance::getDoubleProperty(const std::string &name, int n) const OFX_EXCEPTION_SPEC
      {
        if(name==kOfxImagePropPixelAspectRatio){
          if(n!=0) throw Property::Exception(kOfxStatErrValue);
          return getAspectRatio();
        }
        else if(name==kOfxImageEffectPropFrameRate){
          if(n!=0) throw Property::Exception(kOfxStatErrValue);
          return getFrameRate();
        }
        else if(name==kOfxImageEffectPropFrameRange){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          double range[2];
          getFrameRange(range[0], range[1]);
          return range[n];
        }
        else if(name==kOfxImageEffectPropUnmappedFrameRate){
          if(n>0) throw Property::Exception(kOfxStatErrValue);
          return getUnmappedFrameRate();
        }
        else if(name==kOfxImageEffectPropUnmappedFrameRange){
          if(n>1) throw Property::Exception(kOfxStatErrValue);
          double range[2];
          getUnmappedFrameRange(range[0], range[1]);
          return range[n];
        }
        else
          throw Property::Exception(kOfxStatErrValue);
      }

      // get the virutals for viewport size, pixel scale, background colour
      int ClipInstance::getIntProperty(const std::string &name, int n) const OFX_EXCEPTION_SPEC
      {
        if(name==kOfxImageClipPropConnected){
          return getConnected();
        }
        else if(name==kOfxImageClipPropContinuousSamples){
          return getContinuousSamples();
        }
#ifdef OFX_EXTENSIONS_NATRON
        else if(name==kOfxImageClipPropFormat){
          OfxRectI format = getFormat();
          if (n == 0) {
            return format.x1;
          } else if (n == 1) {
            return format.y1;
          } else if (n == 2) {
            return format.x2;
          } else if (n == 3) {
            return format.y2;
          } else {
            throw Property::Exception(kOfxStatErrBadIndex);
          }
        }
#endif
        else {
          throw Property::Exception(kOfxStatErrValue);
        }
      }

      // get the virutals for viewport size, pixel scale, background colour
      void ClipInstance::getIntPropertyN(const std::string &name, int *values, int n) const OFX_EXCEPTION_SPEC
      {
        if(n<=0) throw Property::Exception(kOfxStatErrValue);
#ifdef OFX_EXTENSIONS_NATRON
        if(name==kOfxImageClipPropFormat){
          if (n != 4) {
            throw Property::Exception(kOfxStatErrBadIndex);
          }
          OfxRectI format = getFormat();
          values[0] = format.x1;
          values[1] = format.y1;
          values[2] = format.x2;
          values[3] = format.y2;
        } else
#endif
        {
          *values = getIntProperty(name, 0);
        }
      }

      // get the virutals for viewport size, pixel scale, background colour
      const std::string &ClipInstance::getStringProperty(const std::string &name, int n) const OFX_EXCEPTION_SPEC
      {
#ifdef OFX_EXTENSIONS_NUKE
        if (name==kFnOfxImageEffectPropComponentsPresent) {
            const std::vector<std::string>& componentsPresents = getComponentsPresent();
            if (n >= 0 && n < (int)componentsPresents.size()) {
                return componentsPresents[n];
            } else {
                throw Property::Exception(kOfxStatErrBadIndex);
            }
        }
#endif
        if(n!=0) throw Property::Exception(kOfxStatErrValue);
        if(name==kOfxImageEffectPropPixelDepth){
          return getPixelDepth();
        }
        else if(name==kOfxImageEffectPropComponents){
          return getComponents();
        }
        else if(name==kOfxImageClipPropUnmappedComponents){
          return getUnmappedComponents();
        }
        else if(name==kOfxImageClipPropUnmappedPixelDepth){
          return getUnmappedBitDepth();
        }
        else if(name==kOfxImageEffectPropPreMultiplication){
          return getPremult();
        }
        else if(name==kOfxImageClipPropFieldOrder){
          return getFieldOrder();
        }
        else
          throw Property::Exception(kOfxStatErrValue);
      }
       
      // fetch  multiple values in a multi-dimension property
      void ClipInstance::getStringPropertyN(const std::string &name, const char** values, int count) const OFX_EXCEPTION_SPEC
      {
          if (count <= 0) throw Property::Exception(kOfxStatErrValue);
#ifdef OFX_EXTENSIONS_NUKE
          if (name==kFnOfxImageEffectPropComponentsPresent) {
              const std::vector<std::string>& componentsPresents = getComponentsPresent();
              int minCount = (int)componentsPresents.size() < count ? (int)componentsPresents.size() : count;
              for (int i = 0; i < minCount; ++i) {
                  values[i] = componentsPresents[i].c_str();
              }
              return;
          }
#endif
          if(count!=1) throw Property::Exception(kOfxStatErrValue);
          if(name==kOfxImageEffectPropPixelDepth){
              values[0] = getPixelDepth().c_str();
          }
          else if(name==kOfxImageEffectPropComponents){
              values[0] = getComponents().c_str();
          }
          else if(name==kOfxImageClipPropUnmappedComponents){
              values[0] = getUnmappedComponents().c_str();
          }
          else if(name==kOfxImageClipPropUnmappedPixelDepth){
              values[0] = getUnmappedBitDepth().c_str();
          }
          else if(name==kOfxImageEffectPropPreMultiplication){
              values[0] = getPremult().c_str();
          }
          else if(name==kOfxImageClipPropFieldOrder){
              values[0] = getFieldOrder().c_str();
          }
          else
              throw Property::Exception(kOfxStatErrValue);
      }

      // notify override properties
      void ClipInstance::notify(const std::string &name, bool /*isSingle*/, int /*indexOrN*/)  OFX_EXCEPTION_SPEC
      {
#     ifdef OFX_EXTENSIONS_NATRON
        if (name == kOfxPropLabel) {
          setLabel();
        }
        else if (name == kOfxParamPropHint) {
          setHint();
        }
        else if (name == kOfxParamPropSecret) {
          setSecret();
        }
#     endif
      }

      OfxStatus ClipInstance::instanceChangedAction(const std::string &why,
                                                OfxTime     time,
                                                OfxPointD   renderScale)
      {
        Property::PropSpec stuff[] = {
          { kOfxPropType, Property::eString, 1, true, kOfxTypeClip },
          { kOfxPropName, Property::eString, 1, true, getName().c_str() },
          { kOfxPropChangeReason, Property::eString, 1, true, why.c_str() },
          { kOfxPropTime, Property::eDouble, 1, true, "0" },
          { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "0" },
          Property::propSpecEnd
        };

        Property::Set inArgs(stuff);

        // add the second dimension of the render scale
        inArgs.setDoubleProperty(kOfxPropTime,time);
        inArgs.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)_effectInstance<<"->"<<kOfxActionInstanceChanged<<"("<<kOfxTypeClip<<","<<getName()<<","<<why<<","<<time<<",("<<renderScale.x<<","<<renderScale.y<<"))"<<std::endl;
#       endif

        OfxStatus st;
        if(_effectInstance){
          st = _effectInstance->mainEntry(kOfxActionInstanceChanged, _effectInstance->getHandle(), &inArgs, 0);
        } else {
          st = kOfxStatFailed;
        }
#       ifdef OFX_DEBUG_ACTIONS
          std::cout << "OFX: "<<(void*)_effectInstance<<"->"<<kOfxActionInstanceChanged<<"("<<kOfxTypeClip<<","<<getName()<<","<<why<<","<<time<<",("<<renderScale.x<<","<<renderScale.y<<"))->"<<StatStr(st)<<std::endl;
#       endif
        return st;
      }

      /// given the colour component, find the nearest set of supported colour components
      const std::string &ClipInstance::findSupportedComp(const std::string &s) const
      { 
        static const std::string none(kOfxImageComponentNone);
        static const std::string rgba(kOfxImageComponentRGBA);
        static const std::string rgb(kOfxImageComponentRGB);
        static const std::string alpha(kOfxImageComponentAlpha);
#ifdef OFX_EXTENSIONS_NATRON
        static const std::string xy(kNatronOfxImageComponentXY);
#endif
        /// is it there
        if(isSupportedComponent(s))
          return s;
          
#ifdef OFX_EXTENSIONS_NATRON
        if (s == xy) {
          if (isSupportedComponent(rgb)) {
            return rgb;
          } else if (isSupportedComponent(rgba)) {
            return rgba;
          } else if (isSupportedComponent(alpha)) {
            return alpha;
          }
        }
#endif

        /// were we fed some custom non chromatic component by getUnmappedComponents? Return it.
        /// we should never be here mind, so a bit weird
        if(!_effectInstance->isChromaticComponent(s))
          return s;

        /// Means we have RGBA or Alpha being passed in and the clip
        /// only supports the other one, so return that
        if(s == rgba) {
          if(isSupportedComponent(rgb))
            return rgb;
          if(isSupportedComponent(alpha))
            return alpha;
        } else if(s == alpha) {
          if(isSupportedComponent(rgba))
            return rgba;
          if(isSupportedComponent(rgb))
            return rgb;
        }

        /// wierd, must be some custom bit , if only one, choose that, otherwise no idea
        /// how to map, you need to derive to do so.
        const std::vector<std::string> &supportedComps = getSupportedComponents();
        if(supportedComps.size() == 1)
          return supportedComps[0];

        return none;
      }
      
      
      ////////////////////////////////////////////////////////////////////////////////
      // Image
      //

      static const Property::PropSpec imageBaseStuffs[] = {
        { kOfxPropType, Property::eString, 1, false, kOfxTypeImage },
        { kOfxImageEffectPropPixelDepth, Property::eString, 1, true, kOfxBitDepthNone  },
        { kOfxImageEffectPropComponents, Property::eString, 1, true, kOfxImageComponentNone },
        { kOfxImageEffectPropPreMultiplication, Property::eString, 1, true, kOfxImageOpaque  },
        { kOfxImageEffectPropRenderScale, Property::eDouble, 2, true, "1.0" },
        { kOfxImagePropPixelAspectRatio, Property::eDouble, 1, true, "1.0"  },
        { kOfxImagePropBounds, Property::eInt, 4, true, "0" },
        { kOfxImagePropRegionOfDefinition, Property::eInt, 4, true, "0", },
        { kOfxImagePropRowBytes, Property::eInt, 1, true, "0", },
        { kOfxImagePropField, Property::eString, 1, true, "", },
        { kOfxImagePropUniqueIdentifier, Property::eString, 1, true, "" },
#if defined(OFX_EXTENSIONS_NUKE) || defined(OFX_EXTENSIONS_NATRON)
        // The following properties should be added using OFX::Host::Property::Set::addProperties()
        // when attaching a transform or a distortion to an image. if they are not present, there
        // is no transform/distortion
        /*
#ifdef OFX_EXTENSIONS_NUKE
        { kFnOfxPropMatrix2D, Property::eDouble, 9, true, "0" }, // If the clip descriptor has kFnOfxImageEffectCanTransform set to 1, this property contains a 3x3 matrix corresponding to a transform in pixel coordinate space, going from the source image to the destination, defaults to the identity matrix. A matrix filled with zeroes is considered as the identity matrix (i.e. no transform)
#endif
#ifdef OFX_EXTENSIONS_NATRON
        { kOfxPropMatrix3x3, OFX::Host::Property::eDouble, 9, true, "0" }, // If the clip descriptor has kOfxImageEffectPropCanDistort set to 1, this property contains a 3x3 matrix corresponding to a transform in canonical coordinate space, going from the source image to the destination, defaults to the identity matrix. A matrix filled with zeroes is considered as the identity matrix (i.e. no transform)
        { kOfxPropInverseDistortionFunction, Property::ePointer, 1, true, NULL }, // If the clip descriptor has kOfxImageEffectPropCanDistort set to 1, this property contains a pointer to a distortion function going from a position in the output distorted image in canonical coordinates to a position in the source image.
        { kOfxPropInverseDistortionFunctionData, Property::ePointer, 1, true, NULL }, // if kOfxPropDistortionFunction is set, this a pointer to the data that must be passed to the distortion function
#endif
          */
#endif
        Property::propSpecEnd
      };

      ImageBase::ImageBase()
        : Property::Set(imageBaseStuffs)
        , _referenceCount(1)
      {
      }

      /// called during ctor to get bits from the clip props into ours
      void ImageBase::getClipBits(ClipInstance& instance)
      {
        Property::Set& clipProperties = instance.getProps();
        
        // get and set the clip instance pixel depth
        const std::string &depth = clipProperties.getStringProperty(kOfxImageEffectPropPixelDepth);
        setStringProperty(kOfxImageEffectPropPixelDepth, depth); 
        
        // get and set the clip instance components
        const std::string &comps = clipProperties.getStringProperty(kOfxImageEffectPropComponents);
        setStringProperty(kOfxImageEffectPropComponents, comps);
        
        // get and set the clip instance premultiplication
        setStringProperty(kOfxImageEffectPropPreMultiplication, clipProperties.getStringProperty(kOfxImageEffectPropPreMultiplication));

        // get and set the clip instance pixel aspect ratio
        setDoubleProperty(kOfxImagePropPixelAspectRatio, clipProperties.getDoubleProperty(kOfxImagePropPixelAspectRatio));
      }

      /// make an image from a clip instance
      ImageBase::ImageBase(ClipInstance& instance)
        : Property::Set(imageBaseStuffs)
        , _referenceCount(1)
      {
        getClipBits(instance);
      }      

      // construction based on clip instance
      ImageBase::ImageBase(ClipInstance& instance,
                   double renderScaleX, 
                   double renderScaleY,
                   const OfxRectI &bounds,
                   const OfxRectI &rod,
                   int rowBytes,
                   std::string field,
                   std::string uniqueIdentifier) 
        : Property::Set(imageBaseStuffs)
        , _referenceCount(1)
      {
        getClipBits(instance);

        // set other data
        setDoubleProperty(kOfxImageEffectPropRenderScale,renderScaleX, 0);    
        setDoubleProperty(kOfxImageEffectPropRenderScale,renderScaleY, 1);        
        setIntProperty(kOfxImagePropBounds,bounds.x1, 0);
        setIntProperty(kOfxImagePropBounds,bounds.y1, 1);
        setIntProperty(kOfxImagePropBounds,bounds.x2, 2);
        setIntProperty(kOfxImagePropBounds,bounds.y2, 3);
        setIntProperty(kOfxImagePropRegionOfDefinition,rod.x1, 0);
        setIntProperty(kOfxImagePropRegionOfDefinition,rod.y1, 1);
        setIntProperty(kOfxImagePropRegionOfDefinition,rod.x2, 2);
        setIntProperty(kOfxImagePropRegionOfDefinition,rod.y2, 3);        
        setIntProperty(kOfxImagePropRowBytes,rowBytes);
        
        setStringProperty(kOfxImagePropField,field);
        setStringProperty(kOfxImageClipPropFieldOrder,field);
        setStringProperty(kOfxImagePropUniqueIdentifier,uniqueIdentifier);
      }

      OfxRectI ImageBase::getBounds() const
      {
        OfxRectI bounds = {0, 0, 0, 0};
        getIntPropertyN(kOfxImagePropBounds, &bounds.x1, 4);
        return bounds;
      }

      OfxRectI ImageBase::getROD() const
      {
        OfxRectI rod = {0, 0, 0, 0};
        getIntPropertyN(kOfxImagePropRegionOfDefinition, &rod.x1, 4);
        return rod;
      }

      ImageBase::~ImageBase() {
        //assert(_referenceCount <= 0);
      }

      // release the reference 
      void ImageBase::releaseReference()
      {
        _referenceCount -= 1;
        if(_referenceCount <= 0)
          delete this;
      }


      static const Property::PropSpec imageStuffs[] = {
        { kOfxImagePropData, Property::ePointer, 1, true, NULL },
        Property::propSpecEnd
      };

      Image::Image()
        : ImageBase()
      {
        addProperties(imageStuffs);
      }

      /// make an image from a clip instance
      Image::Image(ClipInstance& instance)
        : ImageBase(instance)
      {
        addProperties(imageStuffs);
      }

      // construction based on clip instance
      Image::Image(ClipInstance& instance,
                   double renderScaleX, 
                   double renderScaleY,
                   void* data,
                   const OfxRectI &bounds,
                   const OfxRectI &rod,
                   int rowBytes,
                   std::string field,
                   std::string uniqueIdentifier) 
        : ImageBase(instance, renderScaleX, renderScaleY, bounds, rod, rowBytes, field, uniqueIdentifier)
      {
        addProperties(imageStuffs);

        // set other data
        setPointerProperty(kOfxImagePropData,data);
      }

      Image::~Image() {
        //assert(_referenceCount <= 0);
      }
#   ifdef OFX_SUPPORTS_OPENGLRENDER
      static const Property::PropSpec textureStuffs[] = {
        { kOfxImageEffectPropOpenGLTextureIndex, Property::eInt, 1, true, "-1" },
        { kOfxImageEffectPropOpenGLTextureTarget, Property::eInt, 1, true, "-1" },
        Property::propSpecEnd
      };

      Texture::Texture()
        : ImageBase()
      {
        addProperties(textureStuffs);
      }

      /// make an image from a clip instance
      Texture::Texture(ClipInstance& instance)
        : ImageBase(instance)
      {
        addProperties(textureStuffs);
      }

      // construction based on clip instance
      Texture::Texture(ClipInstance& instance,
                   double renderScaleX, 
                   double renderScaleY,
                   int index,
                   int target,
                   const OfxRectI &bounds,
                   const OfxRectI &rod,
                   int rowBytes,
                   std::string field,
                   std::string uniqueIdentifier) 
        : ImageBase(instance, renderScaleX, renderScaleY, bounds, rod, rowBytes, field, uniqueIdentifier)
      {
        addProperties(textureStuffs);

        // set other data
        setIntProperty(kOfxImageEffectPropOpenGLTextureIndex, index);
        setIntProperty(kOfxImageEffectPropOpenGLTextureTarget, target);
      }


      Texture::~Texture() {
        //assert(_referenceCount <= 0);
      }
#   endif
    } // Clip

  } // Host

} // OFX
