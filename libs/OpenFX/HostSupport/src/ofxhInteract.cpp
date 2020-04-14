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

// ofx 
#include "ofxKeySyms.h"
#include "ofxCore.h"
#include "ofxImageEffect.h"
#ifdef OFX_EXTENSIONS_NUKE
#include "nuke/fnPublicOfxExtensions.h"
#include "nuke/fnOfxExtensions.h"
#endif
#ifdef OFX_EXTENSIONS_NATRON
#include "ofxNatron.h"
#endif

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhImageEffect.h"
#include "ofxhInteract.h"
#include "ofxOld.h" // old plugins may rely on deprecated properties being present

namespace OFX {

  namespace Host {

    namespace Interact {

      //
      // descriptor
      //
      static const Property::PropSpec interactDescriptorStuffs[] = {
        { kOfxInteractPropHasAlpha , Property::eInt, 1, true, "0" },
        { kOfxInteractPropBitDepth , Property::eInt, 1, true, "0" },
#     ifdef OFX_EXTENSIONS_NATRON
        { kNatronOfxInteractColourPicking, Property::eInt, 1, false, "0" },
#     endif
        Property::propSpecEnd
      };

      Descriptor::Descriptor()
        : _properties(interactDescriptorStuffs) 
        , _state(eUninitialised)
        , _entryPoint(NULL)
      {
      }

      Descriptor::~Descriptor()
      {
      }

      /// call describe on this descriptor
      bool Descriptor::describe(int bitDepthPerComponent, bool hasAlpha)
      {
        if(_state == eUninitialised) {
          _properties.setIntProperty(kOfxInteractPropBitDepth, bitDepthPerComponent);
          _properties.setIntProperty(kOfxInteractPropHasAlpha, (int)(hasAlpha));

          OfxStatus stat = callEntry(kOfxActionDescribe, getHandle(), NULL, NULL);
          if(stat == kOfxStatOK || stat == kOfxStatReplyDefault) {
            _state = eDescribed;
          }
          else {
            _state = eFailed;
          }
        }
        return _state == eDescribed;
      }

      // call the interactive entry point
      OfxStatus Descriptor::callEntry(const char *action, 
                                      void *handle,
                                      OfxPropertySetHandle inArgs, 
                                      OfxPropertySetHandle outArgs)
      {
        if(_entryPoint && _state != eFailed) {
          return _entryPoint(action, handle, inArgs, outArgs);
        }
        else
          return kOfxStatFailed;
        
        return kOfxStatOK;
      }


      ////////////////////////////////////////////////////////////////////////////////
      static const Property::PropSpec interactInstanceStuffs[] = {
        { kOfxPropEffectInstance, Property::ePointer, 1, true, NULL },
        { kOfxPropInstanceData, Property::ePointer, 1, false, NULL },
        { kOfxInteractPropPixelScale, Property::eDouble, 2, true, "1.0f" },
        { kOfxInteractPropBackgroundColour , Property::eDouble, 3, true, "0.0f" },
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        { kOfxInteractPropViewportSize, Property::eDouble, 2, true, "100.0f" },
#endif
        { kOfxInteractPropSlaveToParam , Property::eString, 0, false, ""},
        { kOfxInteractPropSuggestedColour , Property::eDouble, 0, true, "1.0f" }, // dimension may be 0 or 3
#ifdef OFX_EXTENSIONS_NUKE
        { kOfxPropOverlayColour , Property::eDouble, 0, true, "1.0f" }, // dimension may be 0 or 3
#endif
#     ifdef OFX_EXTENSIONS_NATRON
        { kNatronOfxInteractColourPicking, Property::eInt, 1, false, "0" },
#     endif
        Property::propSpecEnd
      };

      static const Property::PropSpec interactArgsStuffs[] = {
        { kOfxPropEffectInstance, Property::ePointer, 1, false, NULL },
        { kOfxPropTime, Property::eDouble, 1, false, "0.0" },
        { kOfxImageEffectPropRenderScale, Property::eDouble, 2, false, "0.0" },
#ifdef OFX_EXTENSIONS_NUKE
        { kFnOfxImageEffectPropView, Property::eInt, 1, false, "0" },
#endif
#ifdef OFX_EXTENSIONS_NATRON
        { kNatronOfxPropPickerColour, Property::eDouble, 4, false, "-1.0f" },
        { kOfxInteractPropScreenPixelRatio, Property::eDouble, 1, false, "1.f" },
#endif
        { kOfxInteractPropBackgroundColour , Property::eDouble, 3, false, "0.0f" },
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        { kOfxInteractPropViewportSize, Property::eDouble, 2, false, "0.0f" },
#endif
        { kOfxInteractPropPixelScale, Property::eDouble, 2, false, "1.0f" },
        { kOfxInteractPropPenPosition, Property::eDouble, 2, false, "0.0" },
        { kOfxInteractPropPenViewportPosition, Property::eInt, 2, false, "0" }, // new in OFX 1.2
        { kOfxInteractPropPenPressure, Property::eDouble, 1, false, "0.0" },
        { kOfxPropKeyString, Property::eString, 1, false, "" },
        { kOfxPropKeySym, Property::eInt, 1, false, "0" },
        Property::propSpecEnd
      };

      // instance

      Instance::Instance(Descriptor& desc, void *effectInstance) 
        : _descriptor(desc)
        , _properties(interactInstanceStuffs)
        , _state(desc.getState())
        , _effectInstance(effectInstance)
        , _argProperties(interactArgsStuffs)
      {
        _properties.setPointerProperty(kOfxPropEffectInstance, effectInstance);
        _properties.setChainedSet(&desc.getProperties()); /// chain it into the descriptor props
        _properties.setGetHook(kOfxInteractPropPixelScale, this);
        _properties.setGetHook(kOfxInteractPropBackgroundColour,this);
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        _properties.setGetHook(kOfxInteractPropViewportSize,this);
#endif
        _properties.setGetHook(kOfxInteractPropSuggestedColour,this);
#ifdef OFX_EXTENSIONS_NUKE
        _properties.setGetHook(kOfxPropOverlayColour,this);
#endif
#ifdef OFX_EXTENSIONS_NATRON
        _properties.setGetHook(kOfxInteractPropScreenPixelRatio, this);
#endif
        _argProperties.setGetHook(kOfxInteractPropPixelScale, this);
        _argProperties.setGetHook(kOfxInteractPropBackgroundColour,this);
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        _argProperties.setGetHook(kOfxInteractPropViewportSize,this);
#endif
      }

      Instance::~Instance()
      {
        /// call it directly incase CI failed and we should always tidy up after create instance
        callEntry(kOfxActionDestroyInstance,  NULL);
      }

      /// call the entry point in the descriptor with action and the given args
      OfxStatus Instance::callEntry(const char *action, Property::Set *inArgs)
      {
        if(_state != eFailed) {
          OfxPropertySetHandle inHandle = inArgs ? inArgs->getHandle() : NULL ;
          return _descriptor.callEntry(action, getHandle(), inHandle, NULL);
        }
        return kOfxStatFailed;
      }
      
      // do nothing
      int Instance::getDimension(const std::string &name) const OFX_EXCEPTION_SPEC
      {
        if(name == kOfxInteractPropPixelScale){
          return 2;
        }
        else if(name == kOfxInteractPropBackgroundColour){
          return 3;
        }
        else if(name == kOfxInteractPropSuggestedColour
#ifdef OFX_EXTENSIONS_NUKE
                || name == kOfxPropOverlayColour
#endif
                ){
          double r, g, b;
          bool stat = getSuggestedColour(r, g, b);
          if (stat) {
            return 3;
          } else {
            return 0;
          }
        }
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        else if(name == kOfxInteractPropViewportSize){
          return 2;
        }
#endif
        else
          throw Property::Exception(kOfxStatErrValue);
      }
        
      // do nothing function
      void Instance::reset(const std::string &/*name*/) OFX_EXCEPTION_SPEC
      {
        // no-op
      }

      double Instance::getDoubleProperty(const std::string &name, int index) const OFX_EXCEPTION_SPEC
      {   
        if(name == kOfxInteractPropPixelScale){
          if(index>=2) throw Property::Exception(kOfxStatErrBadIndex);
          double first[2];
          getPixelScale(first[0],first[1]);
          return first[index];
        }
        else if(name == kOfxInteractPropBackgroundColour){
          if(index>=3) throw Property::Exception(kOfxStatErrBadIndex);
          double first[3];
          getBackgroundColour(first[0],first[1],first[2]);
          return first[index];
        }
        else if(name == kOfxInteractPropSuggestedColour
#ifdef OFX_EXTENSIONS_NUKE
                || name == kOfxPropOverlayColour
#endif
                ){
          if(index>=3) throw Property::Exception(kOfxStatErrBadIndex);
          double first[3];
          bool stat = getSuggestedColour(first[0],first[1],first[2]);
          if (!stat) throw Property::Exception(kOfxStatReplyDefault);
          return first[index];
        }
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        else if(name == kOfxInteractPropViewportSize){
          if(index>=2) throw Property::Exception(kOfxStatErrBadIndex);
          double first[2];
          getViewportSize(first[0],first[1]);
          return first[index];
        }
#endif
#ifdef OFX_EXTENSIONS_NATRON
        else if (name == kOfxInteractPropScreenPixelRatio) {
          return getScreenPixelRatio();
        }
#endif
        else
          throw Property::Exception(kOfxStatErrUnknown);
      }

      void Instance::getDoublePropertyN(const std::string &name, double *first, int n) const OFX_EXCEPTION_SPEC
      {
        if(name == kOfxInteractPropPixelScale){
          if(n>2) throw Property::Exception(kOfxStatErrBadIndex);
          getPixelScale(first[0],first[1]);
        }
        else if(name == kOfxInteractPropBackgroundColour){
          if(n>3) throw Property::Exception(kOfxStatErrBadIndex);
          getBackgroundColour(first[0],first[1],first[2]);
        }
        else if(name == kOfxInteractPropSuggestedColour
#ifdef OFX_EXTENSIONS_NUKE
                || name == kOfxPropOverlayColour
#endif
                ){
          if(n>3) throw Property::Exception(kOfxStatErrBadIndex);
          bool stat = getSuggestedColour(first[0],first[1],first[2]);
          if (!stat) throw Property::Exception(kOfxStatReplyDefault);
        }
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        else if(name == kOfxInteractPropViewportSize){
          if(n>2) throw Property::Exception(kOfxStatErrBadIndex);
          getViewportSize(first[0],first[1]);
        }
#endif
        else
          throw Property::Exception(kOfxStatErrUnknown);
      }

      void Instance::getSlaveToParam(std::vector<std::string>& params) const
      {        
        int nSlaveParams = _properties.getDimension(kOfxInteractPropSlaveToParam);
                    
        for (int i=0; i<nSlaveParams; i++) {
          std::string param = _properties.getStringProperty(kOfxInteractPropSlaveToParam, i);
          params.push_back(param);
        }
      }    
      
      /// initialise the argument properties
      void Instance::initArgProp(Property::Set& props,
                                 OfxTime time,
                                 const OfxPointD &renderScale
#ifdef OFX_EXTENSIONS_NUKE
                                 , int view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                 , const OfxRGBAColourD* colourPicker
#endif
                                 )
      {
        if ( OFX::IsNaN(time) ) {
          throw Property::Exception(kOfxStatErrValue);
        }
        double pixelScale[2];
        getPixelScale(pixelScale[0], pixelScale[1]);  
        props.setDoublePropertyN(kOfxInteractPropPixelScale, pixelScale, 2);
        props.setPointerProperty(kOfxPropEffectInstance, _effectInstance);
        props.setPointerProperty(kOfxPropInstanceData, _properties.getPointerProperty(kOfxPropInstanceData));
        props.setDoubleProperty(kOfxPropTime,time);
        props.setDoublePropertyN(kOfxImageEffectPropRenderScale, &renderScale.x, 2);
#ifdef OFX_EXTENSIONS_NUKE
        props.setIntProperty(kFnOfxImageEffectPropView,view);
#endif
#ifdef OFX_EXTENSIONS_NATRON
        if (colourPicker) {
          props.setDoublePropertyN(kNatronOfxPropPickerColour, (const double*)colourPicker, 4);
        } else {
          const double noColor[4] = { -1., -1., -1., -1.};
          props.setDoublePropertyN(kNatronOfxPropPickerColour, noColor, 4);
        }
        props.setDoubleProperty(kOfxInteractPropScreenPixelRatio, getScreenPixelRatio());
#endif
      }

      void Instance::setPenArgProps(const OfxPointD &penPos,
                                    const OfxPointI &penPosViewport,
                                    double  pressure)
      {
        _argProperties.setDoublePropertyN(kOfxInteractPropPenPosition, &penPos.x, 2);
        _argProperties.setIntPropertyN(kOfxInteractPropPenViewportPosition, &penPosViewport.x, 2); // new in OFX 1.2
        _argProperties.setDoubleProperty(kOfxInteractPropPenPressure, pressure);
      }

      void Instance::setKeyArgProps(int     key,
                                    char*   keyString)
      {
        _argProperties.setIntProperty(kOfxPropKeySym,key);
        _argProperties.setStringProperty(kOfxPropKeyString,keyString);
      }

      OfxStatus Instance::createInstanceAction()
      {        
        OfxStatus stat = callEntry(kOfxActionCreateInstance, NULL);
        if(stat == kOfxStatOK || stat == kOfxStatReplyDefault) {
          _state = eCreated;
        }
        else {
          _state = eFailed;
        }
        return stat;
      }

      OfxStatus Instance::drawAction(OfxTime time,
                                     const OfxPointD &renderScale
#ifdef OFX_EXTENSIONS_NUKE
                                     , int view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                     , const OfxRGBAColourD* colourPicker
#endif
                                     )
      {        
        if ( OFX::IsNaN(time) ) {
          return kOfxStatErrValue;
        }
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        return callEntry(kOfxInteractActionDraw, &_argProperties);
      }

      OfxStatus Instance::penMotionAction(OfxTime time,
                                          const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                          int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                          const OfxRGBAColourD* colourPicker,
#endif
                                          const OfxPointD &penPos,
                                          const OfxPointI &penPosViewport,
                                          double  pressure)
      {
        if ( OFX::IsNaN(time) ) {
          return kOfxStatErrValue;
        }
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        setPenArgProps(penPos, penPosViewport, pressure);
        return callEntry(kOfxInteractActionPenMotion,&_argProperties);
      }

      OfxStatus Instance::penUpAction(OfxTime time,
                                      const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                      int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                      const OfxRGBAColourD* colourPicker,
#endif
                                      const OfxPointD &penPos,
                                      const OfxPointI &penPosViewport,
                                      double pressure)
      {
        if ( OFX::IsNaN(time) ) {
          return kOfxStatErrValue;
        }
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        setPenArgProps(penPos, penPosViewport, pressure);
        return callEntry(kOfxInteractActionPenUp,&_argProperties);
      }

      OfxStatus Instance::penDownAction(OfxTime time,
                                        const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                        int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                        const OfxRGBAColourD* colourPicker,
#endif
                                        const OfxPointD &penPos,
                                        const OfxPointI &penPosViewport,
                                        double pressure)
      {
        if ( OFX::IsNaN(time) ) {
          return kOfxStatErrValue;
        }
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        setPenArgProps(penPos, penPosViewport, pressure);
        return callEntry(kOfxInteractActionPenDown,&_argProperties);
      }

      OfxStatus Instance::keyDownAction(OfxTime time,
                                        const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                        int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                        const OfxRGBAColourD* colourPicker,
#endif
                                        int     key,
                                        char*   keyString)
      {
        if ( OFX::IsNaN(time) ) {
          return kOfxStatErrValue;
        }
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        setKeyArgProps(key, keyString);
        return callEntry(kOfxInteractActionKeyDown,&_argProperties);
      }

      OfxStatus Instance::keyUpAction(OfxTime time,
                                      const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                      int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                      const OfxRGBAColourD* colourPicker,
#endif
                                      int     key,
                                      char*   keyString)
      {
        if ( OFX::IsNaN(time) ) {
          return kOfxStatErrValue;
        }
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        setKeyArgProps(key, keyString);
        return callEntry(kOfxInteractActionKeyUp,&_argProperties);
      }

      OfxStatus Instance::keyRepeatAction(OfxTime time,
                                          const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                          int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                          const OfxRGBAColourD* colourPicker,
#endif
                                          int     key,
                                          char*   keyString)
      {
        if ( OFX::IsNaN(time) ) {
          return kOfxStatErrValue;
        }
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        setKeyArgProps(key, keyString);
        return callEntry(kOfxInteractActionKeyRepeat,&_argProperties);
      }
      
      OfxStatus Instance::gainFocusAction(OfxTime time,
                                          const OfxPointD &renderScale
#ifdef OFX_EXTENSIONS_NUKE
                                          , int view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                          , const OfxRGBAColourD* colourPicker
#endif
                                          )
      {
        if ( OFX::IsNaN(time) ) {
          return kOfxStatErrValue;
        }
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        return callEntry(kOfxInteractActionGainFocus,&_argProperties);
      }

      OfxStatus Instance::loseFocusAction(OfxTime  time,
                                          const OfxPointD &renderScale
#ifdef OFX_EXTENSIONS_NUKE
                                          , int view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                          , const OfxRGBAColourD* colourPicker
#endif
        )
      {
        initArgProp(_argProperties,
                    time,
                    renderScale
#ifdef OFX_EXTENSIONS_NUKE
                    , view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                    , colourPicker
#endif
                    );
        return callEntry(kOfxInteractActionLoseFocus,&_argProperties);
      }

      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////////////////////////////////////////////////
      // Interact suite functions

      static OfxStatus interactSwapBuffers(OfxInteractHandle handle)
      {
        try {
        Interact::Instance *interactInstance = reinterpret_cast<Interact::Instance*>(handle);
        if(interactInstance)
          return interactInstance->swapBuffers();
        else
          return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatFailed;
        }
      }
      
      static OfxStatus interactRedraw(OfxInteractHandle handle)
      {
        try {
        Interact::Instance *interactInstance = reinterpret_cast<Interact::Instance*>(handle);
        if(interactInstance)
          return interactInstance->redraw();
        else
          return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatFailed;
        }
      }
      
      static OfxStatus interactGetPropertySet(OfxInteractHandle handle, OfxPropertySetHandle *property)
      {
        try {
        Interact::Base *interact = reinterpret_cast<Interact::Base*>(handle);
        if (!property) {
          return kOfxStatErrBadHandle;
        }

        if (interact) {
          *property = interact->getPropHandle();

          return kOfxStatOK;
        }
        *property = NULL;

        return kOfxStatErrBadHandle;
        } catch (...) {
          return kOfxStatFailed;
        }
      }
      
      /// the interact suite
      static const OfxInteractSuiteV1 gSuite = {
        interactSwapBuffers,
        interactRedraw,
        interactGetPropertySet
      };

      /// function to get the sutie
      const void *GetSuite(int version) {
        if(version == 1)
          return (void *) &gSuite;
        return NULL;
      }


    } // Interact

  } // Host

} // OFX
