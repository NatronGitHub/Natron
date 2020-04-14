
/*
Software License :

Copyright (c) 2007-2009, The Open Effects Association Ltd.  All Rights Reserved.

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

#ifndef OFX_INTERACT_H
#define OFX_INTERACT_H

#include "ofxOld.h" // old plugins may rely on deprecated properties being present
#ifdef OFX_EXTENSIONS_NATRON
#include "ofxPixels.h" // for OfxRGBAColourD
#endif

namespace OFX {

  namespace Host {

    namespace Interact {
      
      /// fetch a versioned suite for our interact
      const void *GetSuite(int version);
      
      class Base {
      public:
        virtual ~Base() {
        }

        /// grab a handle on the parameter for passing to the C API
        OfxInteractHandle getHandle() {return (OfxInteractHandle)this;}
        
        /// get the property handle for this instance/descriptor
        virtual OfxPropertySetHandle getPropHandle() = 0;
      };

      /// state the interact can be in
      enum State {
        eUninitialised,
        eDescribed,
        eCreated,
        eFailed
      };
  
      /// Descriptor for an interact. Interacts all share a single description
      class Descriptor : public Base {
      protected:
        Property::Set        _properties;  ///< its props
        State                _state;       ///< how is it feeling today
        OfxPluginEntryPoint *_entryPoint;  ///< the entry point for this overlay

      public:
        /// CTOR
        Descriptor();

        /// dtor
        virtual ~Descriptor();

        /// set the main entry points
        void setEntryPoint(OfxPluginEntryPoint *entryPoint) {_entryPoint = entryPoint;}

        /// call describe on this descriptor, returns true if all went well
        bool describe(int bitDepthPerComponent, bool hasAlpha);

        /// grab a handle on the properties of this parameter for the C api
        OfxPropertySetHandle getPropHandle() {return _properties.getHandle();}

        /// get prop set
        const Property::Set &getProperties() const {return _properties;}

        /// get a non const prop set
        Property::Set &getProperties() {return _properties;}

        /// call the entry point with action and the given args
        OfxStatus callEntry(const char *action, 
                            void *handle,
                            OfxPropertySetHandle inArgs, 
                            OfxPropertySetHandle outArgs);      

        /// what is it's state?
        State getState() const {return _state;}
      };

      /// a generic interact, it doesn't belong to anything in particular
      /// we need to generify this slighty more and remove the renderscale args
      /// into a derived class, as they only belong to image effect plugins
      class Instance : public Base, protected Property::GetHook {
      protected:
        Descriptor   &_descriptor;  ///< who we are
        Property::Set _properties;  ///< its props
        State         _state;       ///< how is it feeling today
        void         *_effectInstance; ///< this is ugly, we need a base class to all plugin instances at some point.
        Property::Set _argProperties;

        /// initialise the argument properties
        void initArgProp(Property::Set& props,
                         OfxTime time,
                         const OfxPointD   &renderScale
#ifdef OFX_EXTENSIONS_NUKE
                         , int view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                         , const OfxRGBAColourD* colourPicker
#endif
        );

        /// set pen props in the args
        void setPenArgProps(const OfxPointD &penPos,
                            const OfxPointI &penPosViewport,
                            double  pressure);
        
        /// set key args in the props
        void setKeyArgProps(int     key,
                            char*   keyString);
        
      public:
        Instance(Descriptor &desc, void *effectInstance);

        virtual ~Instance();

        /// what is it's state?
        State getState() const {return _state;}

        /// grab a handle on the properties of this parameter for the C api
        OfxPropertySetHandle getPropHandle() {return _properties.getHandle();}

        /// get prop set
        const Property::Set &getProperties() const {return _properties;}

        /// call the entry point in the descriptor with action and the given args
        virtual OfxStatus callEntry(const char *action,
                                    Property::Set *inArgs);
        
#ifdef kOfxInteractPropViewportSize // removed in OFX 1.4
        /// hooks to kOfxInteractPropViewportSize in the property set
        /// this is actually redundant and is to be deprecated
        virtual void getViewportSize(double &width, double &height) const = 0;
#endif

        // hooks to live kOfxInteractPropPixelScale in the property set
        virtual void getPixelScale(double& xScale, double& yScale) const = 0;

#ifdef OFX_EXTENSIONS_NATRON
        // hooks to live kOfxInteractPropScreenPixelRatio in the property set
        virtual double getScreenPixelRatio() const = 0;
#endif

        // hooks to kOfxInteractPropBackgroundColour in the property set
        virtual void getBackgroundColour(double &r, double &g, double &b) const = 0;

        // hooks to kOfxInteractPropSuggestedColour and kOfxPropOverlayColour in the property set
        // return false if there is no color suggestion by the host.
        virtual bool getSuggestedColour(double &r, double &g, double &b) const = 0;

        /// implement
        virtual OfxStatus swapBuffers() = 0;

        /// implement this
        virtual OfxStatus redraw() = 0;

        /// returns the params the interact uses
        virtual void getSlaveToParam(std::vector<std::string>& params) const;

        // do nothing
        virtual int  getDimension(const std::string &name) const OFX_EXCEPTION_SPEC;
        
        // don't know what to do
        virtual void reset(const std::string &name) OFX_EXCEPTION_SPEC;

        /// the gethook virutals for  pixel scale, background colour
        virtual double getDoubleProperty(const std::string &name, int index) const OFX_EXCEPTION_SPEC;

        /// for pixel scale and background colour
        virtual void getDoublePropertyN(const std::string &name, double *first, int n) const OFX_EXCEPTION_SPEC;

        /// call create instance
        virtual OfxStatus createInstanceAction();
        
        // interact action - kOfxInteractActionDraw 
        // 
        // Params -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        virtual OfxStatus drawAction(OfxTime time,
                                     const OfxPointD &renderScale
#ifdef OFX_EXTENSIONS_NUKE
                                     , int view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                     , const OfxRGBAColourD* colourPicker
#endif
                                     );

        // interact action - kOfxInteractActionPenMotion
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    penX              - the X position
        //    penY              - the Y position
        //    pressure          - the pen pressue 0 to 1
        virtual OfxStatus penMotionAction(OfxTime time,
                                          const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                          int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                          const OfxRGBAColourD* colourPicker,
#endif
                                          const OfxPointD &penPos,
                                          const OfxPointI &penPosViewport,
                                          double pressure);

        // interact action - kOfxInteractActionPenUp
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    penX              - the X position
        //    penY              - the Y position
        //    pressure          - the pen pressue 0 to 1
        virtual OfxStatus penUpAction(OfxTime time,
                                      const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                      int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                      const OfxRGBAColourD* colourPicker,
#endif
                                      const OfxPointD &penPos,
                                      const OfxPointI &penPosViewport,
                                      double  pressure);

        // interact action - kOfxInteractActionPenDown
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    penX              - the X position
        //    penY              - the Y position
        //    pressure          - the pen pressue 0 to 1
        virtual OfxStatus penDownAction(OfxTime time,
                                        const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                        int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                        const OfxRGBAColourD* colourPicker,
#endif
                                        const OfxPointD &penPos,
                                        const OfxPointI &penPosViewport,
                                        double  pressure);

        // interact action - kOfxInteractActionkeyDown
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    key               - the pressed key
        //    keyString         - the pressed key string
        virtual OfxStatus keyDownAction(OfxTime time,
                                        const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                        int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                        const OfxRGBAColourD* colourPicker,
#endif
                                        int     key,
                                        char*   keyString);

        // interact action - kOfxInteractActionkeyUp
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    key               - the pressed key
        //    keyString         - the pressed key string
        virtual OfxStatus keyUpAction(OfxTime time,
                                      const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                      int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                      const OfxRGBAColourD* colourPicker,
#endif
                                      int     key,
                                      char*   keyString);

        // interact action - kOfxInteractActionkeyRepeat
        //
        // Params  -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        //    key               - the pressed key
        //    keyString         - the pressed key string
        virtual OfxStatus keyRepeatAction(OfxTime time,
                                          const OfxPointD &renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                          int view,
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                          const OfxRGBAColourD* colourPicker,
#endif
                                          int     key,
                                          char*   keyString);

        // interact action - kOfxInteractActionLoseFocus
        // 
        // Params -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        virtual OfxStatus gainFocusAction(OfxTime time,
                                          const OfxPointD &renderScale
#ifdef OFX_EXTENSIONS_NUKE
                                          , int view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                          , const OfxRGBAColourD* colourPicker
#endif
                                          );

        // interact action - kOfxInteractActionLoseFocus
        // 
        // Params -
        //
        //    time              - the effect time at which changed occured
        //    renderScale       - the render scale
        virtual OfxStatus loseFocusAction(OfxTime  time,
                                          const OfxPointD &renderScale
#ifdef OFX_EXTENSIONS_NUKE
                                          , int view
#endif
#ifdef OFX_EXTENSIONS_NATRON
                                          , const OfxRGBAColourD* colourPicker
#endif
                                          );
      };

    } // Interact

  } // Host

} // OFX

#endif // OFX_INTERACT_H
