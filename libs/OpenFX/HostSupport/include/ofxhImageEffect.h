
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

#ifndef OFX_IMAGE_EFFECT_H
#define OFX_IMAGE_EFFECT_H

#include "ofxCore.h"
#include "ofxImageEffect.h"

#include "ofxhHost.h"
#include "ofxhClip.h"
#include "ofxhProgress.h"
#include "ofxhTimeLine.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhInteract.h"
#ifdef OFX_EXTENSIONS_NATRON
#include "ofxNatron.h"
#endif

#ifdef _MSC_VER
//Use visual studio extension
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace OFX {

  namespace Host {

    // forward declare    
    class Plugin;

    namespace Memory {
      class Instance;
    }

    namespace ImageEffect {

      // forward declare
      class ImageEffectPlugin;
      class OverlayInstance;
      class Instance;
      class Descriptor;

      /// An image effect host, passed to the setHost function of all image effect plugins
      class Host : public OFX::Host::Host {
      public :
        Host();

        /// fetch a suite
        virtual const void *fetchSuite(const char *suiteName, int suiteVersion);

        /// Create a new instance of an image effect plug-in.
        ///
        /// It is called by ImageEffectPlugin::createInstance which the
        /// client code calls when it wants to make a new instance.
        /// 
        ///   \arg clientData - the clientData passed into the ImageEffectPlugin::createInstance
        ///   \arg plugin - the plugin being created
        ///   \arg desc - the descriptor for that plugin
        ///   \arg context - the context to be created in
        virtual Instance* newInstance(void* clientData,
                                      ImageEffectPlugin* plugin,
                                      Descriptor& desc,
                                      const std::string& context) = 0;

        /// Function called as each plugin binary is found and loaded from disk
        /// or when it is unloaded.
        ///
        /// Use this in any dialogue etc... showing progress
        /// \arg loading - whether this plugin is loading or unloading
        /// \arg id - the plugin ID
        /// \arg versionMajor - the plugin major version number
        /// \arg versionMinor - the plugin minor version number
        virtual void loadingStatus(bool loading, const std::string &id, int versionMajor, int versionMinor);
        
        /// Override this to filter out plugins which the host can't support for whatever reason
        ///
        ///   \arg plugin - the plugin to examine
        ///   \arg reason - set this to report the reason the plugin was not loaded
        virtual bool pluginSupported(ImageEffectPlugin *plugin, std::string &reason) const;

        /// Override this to create a descriptor, this makes the 'root' descriptor
        virtual Descriptor *makeDescriptor(ImageEffectPlugin* plugin) = 0;

        /// used to construct a context description, rootContext is the main context
        virtual Descriptor *makeDescriptor(const Descriptor &rootContext, ImageEffectPlugin *plug) = 0;        

        /// used to construct populate the cache
        virtual Descriptor *makeDescriptor(const std::string &bundlePath, ImageEffectPlugin *plug) = 0;

        /// Override this to initialise an image effect descriptor after it has been
        /// created.
        virtual void initDescriptor(Descriptor* desc);

#ifdef OFX_SUPPORTS_MULTITHREAD
        // these functions must be implemented if the host supports OfxMultiThreadSuiteV1
        // all the following functions are described in ofxMultiThread.h
        //

        /// @see OfxMultiThreadSuiteV1.multiThread()
        virtual OfxStatus multiThread(OfxThreadFunctionV1 func,unsigned int nThreads, void *customArg) = 0;
          
        /// @see OfxMultiThreadSuiteV1.multiThreadNumCPUS()
        virtual OfxStatus multiThreadNumCPUS(unsigned int *nCPUs) const = 0;

        /// @see OfxMultiThreadSuiteV1.multiThreadIndex()
        virtual OfxStatus multiThreadIndex(unsigned int *threadIndex) const = 0;
          
        /// @see OfxMultiThreadSuiteV1.multiThreadIsSpawnedThread()
        virtual int multiThreadIsSpawnedThread() const = 0;
          
        /// @see OfxMultiThreadSuiteV1.mutexCreate()
        virtual OfxStatus mutexCreate(OfxMutexHandle *mutex, int lockCount) = 0;
          
        /// @see OfxMultiThreadSuiteV1.mutexDestroy()
        virtual OfxStatus mutexDestroy(const OfxMutexHandle mutex) = 0;

        /// @see OfxMultiThreadSuiteV1.mutexLock()
        virtual OfxStatus mutexLock(const OfxMutexHandle mutex) = 0;
          
        /// @see OfxMultiThreadSuiteV1.mutexUnLock()
        virtual OfxStatus mutexUnLock(const OfxMutexHandle mutex) = 0;
          
        /// @see OfxMultiThreadSuiteV1.mutexTryLock()
        virtual OfxStatus mutexTryLock(const OfxMutexHandle mutex) = 0;
#endif // OFX_SUPPORTS_MULTITHREAD

#ifdef OFX_SUPPORTS_DIALOG
        // dialog suite
        // In OfxDialogSuiteV1, only the host can figure out which effect instance triggered
        // that request.

        /// @see OfxDialogSuite.requestDialog()
        virtual OfxStatus requestDialog(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs, void *instanceData) = 0;

        /// @see OfxDialogSuite.notifyRedrawPending()
        virtual OfxStatus notifyRedrawPending(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs) = 0;
#endif

#     ifdef OFX_SUPPORTS_OPENGLRENDER
        /// @see OfxImageEffectOpenGLRenderSuiteV1.flushResources()
        virtual OfxStatus flushOpenGLResources() const = 0;
#     endif

        /// override this to use your own memory instance - must inherrit from memory::instance
        virtual Memory::Instance* newMemoryInstance(size_t nBytes);

        // return an memory::instance calls makeMemoryInstance that can be overriden
        Memory::Instance* imageMemoryAlloc(size_t nBytes);
      };

      /// our global host object, set when the plugin cache is created
      extern Host *gImageEffectHost;

      ////////////////////////////////////////////////////////////////////////////////
      /// base class to both effect descriptors and instances
      class Base {
      protected:        
        Property::Set   _properties;

      public:
        Base(const Property::Set &set);
        Base(const Property::PropSpec * propSpec);
        virtual ~Base();

        /// is my magic number valid?
        virtual bool verifyMagic() { return true; }

        /// obtain a handle on this for passing to the C api
        OfxImageEffectHandle getHandle() const;

        /// get the properties set
        Property::Set &getProps();

        /// get the properties set, const version
        const Property::Set &getProps() const;

        /// name of the clip
        const std::string &getShortLabel() const;
        
        /// name of the clip
        const std::string &getLabel() const;
        
        /// name of the clip
        const std::string &getLongLabel() const;

        /// is the given context supported
        bool isContextSupported(const std::string &s) const;

        /// what is the name of the group the plug-in belongs to
        const std::string &getPluginGrouping() const;

        /// is the effect single instance
        bool isSingleInstance() const;

        /// what is the thread safety on this effect
        const std::string &getRenderThreadSafety() const;

        /// should the host attempt to managed multi-threaded rendering if it can
        /// via tiling or some such
        bool getHostFrameThreading() const;

#ifdef OFX_EXTENSIONS_NATRON
        // Indicates if a plug-in can make use of multiple CPU threads within a single call
        // of the kOfxImageEffectActionRender action, for instance
        // by using the multi-thread suite.
        // The host should only account this as a hint to better organize its multi-threading priority queue.
        bool getUsesMultiThreading() const;
#endif

        /// get the overlay interact main entry if it exists
        OfxPluginEntryPoint *getOverlayInteractMainEntry() const;

        /// does the effect support images of differing sizes
        bool supportsMultiResolution() const;

        /// does the effect support tiled rendering
        bool supportsTiles() const;

        /// does this effect need random temporal access
        bool temporalAccess() const;

        /// is the given RGBA/A pixel depth supported by the effect
        bool isPixelDepthSupported(const std::string &s) const;

#ifdef OFX_SUPPORTS_OPENGLRENDER
        /// is the given RGBA/A OpenGL pixel depth supported by the effect
        bool isOpenGLPixelDepthSupported(const std::string &s) const;
#endif
        /// when field rendering, does the effect need to be called
        /// twice to render a frame in all circumstances (with different fields)
        bool fieldRenderTwiceAlways() const;
        
        /// does the effect support multiple clip depths
        bool supportsMultipleClipDepths() const;
        
        /// does the effect support multiple clip pixel aspect ratios
        bool supportsMultipleClipPARs() const;
        
        /// does changing the named param re-tigger a clip preferences action
        bool isClipPreferencesSlaveParam(const std::string &s) const;

        /// does the effect require sequential render
        bool requiresSequentialRender() const;
        
        /// does the effect prefer sequential render
        bool prefersSequentialRender() const;
        
        /// does the effect support render quality
        bool supportsRenderQuality() const;

#ifdef OFX_EXTENSIONS_NUKE
        /// does this effect handle transform effects
        bool canTransform() const;
      
        /// Indicates that a host or plugin can fetch more than a type of image from a clip
        bool isMultiPlanar() const;
        
        ///Indicates that the plug-in would like to have a mask automatically handled by the host if possible
        bool isHostMaskingEnabled() const;
          
        ///Indicates that the plug-in would like to have a "Mix" parameter handled by the host if possible
        bool isHostMixingEnabled() const;
        
        enum OfxPassThroughLevelEnum {
            ePassThroughLevelEnumBlockAllNonRenderedPlanes = 0,
            ePassThroughLevelEnumPassThroughAllNonRenderedPlanes,
            ePassThroughLevelEnumRenderAllRequestedPlanes,
        };
          
        /// If true, the plug-in is asking to pass-through all non rendered planes in output, otherwise they should be blocked and non
        /// accessible from below
        OfxPassThroughLevelEnum getPassThroughForNonRenderedPlanes() const;
          
        /// Indicates to the host that the plugin is view aware, in which case it will have to use the view calls
        bool isViewAware() const;
          
        /// Indicates to the host that a view aware plugin produces the same image independent of the view being rendered
        int getViewInvariance() const;
#endif

#ifdef OFX_EXTENSIONS_NATRON
        /// does this effect handle distortion functions
        bool canDistort() const;

        /// is this effect deprecated
        bool isDeprecated() const;

        // Is the plug-in description string encoded in markdown instead of plain-text ?
        bool isPluginDescriptionInMarkdown() const;

        struct PluginShortcut
        {
          std::string shortcutID;
          int symbol;
          bool hasCtrlModifier;
          bool hasAltModifier;
          bool hasShiftModifier;
          bool hasMetaModifier;
          bool hasKeypadModifier;
        };

        // Get the default shortcuts defined for in viewport parameters
        void getPluginDefaultShortcuts(std::list<PluginShortcut>* shortcuts) const;

        // Get a list of the parameters name that are to be displayed in the viewport
        void getInViewportParametersName(std::list<std::string>* parameterNames) const;

        // Get a list of the cursors used by this plug-in
        void getDefaultCursors(std::list<std::string>* cursors) const;

        // Update the selection rectangle property
        void setSelectionRectangleState(double x1, double y1, double x2, double y2);
#endif
      };

      /// an image effect plugin descriptor
      class Descriptor 
        : public Base
        , public Param::SetDescriptor {
      private :
        // private CC
        Descriptor(const Descriptor &other)
          : Base(other._properties)
          , Param::SetDescriptor()
          , _plugin(other._plugin)
        {}

      protected:
        Plugin                                 *_plugin;       ///< the plugin I belong to
        std::map<std::string, ClipDescriptor*>  _clips;        ///< clips descriptors by name
        std::vector<ClipDescriptor*>            _clipsByOrder; ///< clip descriptors in order of declaration
        mutable Interact::Descriptor            _overlayDescriptor; ///< descriptor to use for overlays, it has delayed description

      public:
        /// used to construct the global description
        Descriptor(Plugin *plug);        
        
        /// used to construct a context description, 'other' is the main context
        Descriptor(const Descriptor &rootContext, Plugin *plug);        
        
        /// used to construct populate the cache
        Descriptor(const std::string &bundlePath, Plugin *plug);

        /// dtor
        virtual ~Descriptor();

        /// implemented for Param::SetDescriptor
        virtual Property::Set &getParamSetProps();

        /// get the plugin I belong to
        Plugin *getPlugin() const {return _plugin;}

        /// create a new clip and add this to the clip map
        virtual ClipDescriptor *defineClip(const std::string &name);

        /// get the clips
        const std::map<std::string, ClipDescriptor*> &getClips() const;

        /// add a new clip
        void addClip(const std::string &name, ClipDescriptor *clip);

        /// get the clips in order of construction
        const std::vector<ClipDescriptor*> &getClipsByOrder() const
        {
          return _clipsByOrder;
        }

        /// Get the interact description, this will also call describe on the interact
        /// This will return NULL if there is not main entry point or if the description failed
        /// otherwise it will return the described overlay
        Interact::Descriptor &getOverlayDescriptor(int bitDepthPerComponent = 8, bool hasAlpha = false);
      };      

      /// a map used to specify needed frame ranges on set of clips
      typedef std::map<ClipInstance *, std::vector<OfxRangeD> > RangeMap;

#ifdef OFX_EXTENSIONS_NUKE
      /// a map used to indicate needed frame/views ranges for all input clips
      typedef std::map<ClipInstance*, std::map<int, std::vector<OfxRangeD> > > ViewsRangeMap;
        
      /// a map used to specify clip components on clips
      typedef std::map<ClipInstance*,std::list<std::string> > ComponentsMap;
#endif
      /// an image effect plugin instance.
      ///
      /// Client code needs to filling the pure virtuals in this.
      class Instance : public Base,
                       public Param::SetInstance,
                       public Progress::ProgressI,
                       public TimeLine::TimeLineI,
                       private Property::NotifyHook, 
                       private Property::GetHook
      {
      protected:
        OFX::Host::ImageEffect::ImageEffectPlugin    *_plugin;
        std::string                                   _context;
        Descriptor                                   *_descriptor;
        std::map<std::string, ClipInstance*>          _clips;
        bool                                          _interactive;
        bool                                          _created;
        bool                                          _ownsData; ///<false if this instance was created with the copy constructor

        bool                                          _clipPrefsDirty; ///< do we need to re-run the clip prefs action
        bool                                          _continuousSamples; ///< set by clip prefs
        bool                                          _frameVarying; ///< set by clip prefs
        std::string                                   _outputPreMultiplication;  ///< set by clip prefs
        std::string                                   _outputFielding;  ///< set by clip prefs
        double                                        _outputFrameRate; ///< set by clip prefs

      public:        
        /// constructor based on effect descriptor
        Instance(ImageEffectPlugin* plugin,
                 Descriptor         &other, 
                 const std::string  &context,
                 bool               interactive);

        /// copy ctor, all clips and parameters will hold a reference (that they do not own)
        /// to clips/parameters from the other instance. The other instance is expected to live
        /// at least as long as this instance
        Instance(const Instance& other);

        virtual ~Instance();

        /// implemented for Param::SetInstance
        virtual Property::Set &getParamSetProps();

        /// implemented for Param::SetInstance
        virtual void paramChangedByPlugin(Param::Instance *param);

        /// get the descriptor for this instance
        const Descriptor &getDescriptor() const {return *_descriptor;}

        /// return the plugin this instance was created with
        OFX::Host::ImageEffect::ImageEffectPlugin*getPlugin() const { return _plugin; }

        /// return the context this instance was created with
        const std::string &getContext() const { return _context; }

        /// get the descriptor for this instance
        Descriptor &getDescriptor() {return *_descriptor;}

        /// get default output fielding. This is passed into the clip prefs action
        /// and  might be mapped (if the host allows such a thing)
        virtual const std::string &getDefaultOutputFielding() const = 0;

        /// get output fielding as set in the clip preferences action.
        const std::string &getOutputFielding() const {return _outputFielding; }

        /// get output fielding as set in the clip preferences action.
        const std::string &getOutputPreMultiplication() const {return _outputPreMultiplication; }

        /// get the output frame rate, as set in the clip prefences action.
        double getOutputFrameRate() const {return _outputFrameRate;}


        /// called after construction to populate the various members
        /// ideally should be called in the ctor, but it relies on 
        /// virtuals so has to be delayed until after the effect is
        /// constructed
        OfxStatus populate();

        /// get the nth clip, in order of declaration
        ClipInstance* getNthClip(int index);

        /// get the nth clip, in order of declaration
        int getNClips() const
        {
          return int(_clips.size());
        }

        /// are the clip preferences currently dirty
        bool areClipPrefsDirty() const {return _clipPrefsDirty;}

        /// are all the non optional clips connected
        bool checkClipConnectionStatus() const;

        /// can this this instance render images at arbitrary times, not just frame boundaries
        /// set by getClipPreferenceAction()
        bool continuousSamples() const {return _continuousSamples;}

        /// does this instance generate a different picture on a frame change, even if the
        /// params and input images are exactly the same. eg: random noise generator
        bool isFrameVarying() const {return _frameVarying;}

        /// pure virtuals that must  be overriden
        virtual ClipInstance* getClip(const std::string& name) const;

        /// override this to make processing abort, return 1 to abort processing
        virtual int abort();

        /// override this to use your own memory instance - must inherrit from memory::instance
        virtual Memory::Instance* newMemoryInstance(size_t nBytes);

        // return an memory::instance calls makeMemoryInstance that can be overriden
        Memory::Instance* imageMemoryAlloc(size_t nBytes);

        /// make a clip
        virtual ClipInstance* newClipInstance(ImageEffect::Instance* plugin,
                                              ClipDescriptor* descriptor, 
                                              int index) = 0;

        /// message suite
        virtual OfxStatus vmessage(const char* type,
                                   const char* id,
                                   const char* format,	
                                   va_list args) = 0;  

        virtual OfxStatus setPersistentMessage(const char* type,
                                               const char* id,
                                               const char* format,
                                               va_list args) = 0;

        virtual OfxStatus clearPersistentMessage() = 0;  

        /// call the effect entry point
        virtual OfxStatus mainEntry(const char *action, 
                                    const void *handle, 
                                    Property::Set *inArgs,
                                    Property::Set *outArgs);

        int upperGetDimension(const std::string &name);

        /// overridden from Property::Notify
        virtual void notify(const std::string &name, bool singleValue, int indexOrN) OFX_EXCEPTION_SPEC;

        /// overridden from gethook,  get the virutals for viewport size, pixel scale, background colour
        virtual double getDoubleProperty(const std::string &name, int index) const OFX_EXCEPTION_SPEC;

        /// overridden from gethook,  get the virutals for viewport size, pixel scale, background colour
        virtual void getDoublePropertyN(const std::string &name, double *values, int count) const OFX_EXCEPTION_SPEC;

        virtual const std::string &getStringProperty(const std::string &name, int index) const  OFX_EXCEPTION_SPEC;

        virtual void getStringPropertyN(const std::string &name, const char** values, int count) const OFX_EXCEPTION_SPEC;

        /// overridden from gethook, don't know what to do
        virtual void reset(const std::string &name) OFX_EXCEPTION_SPEC;

        //// overridden from gethook
        virtual int getDimension(const std::string &name)  const OFX_EXCEPTION_SPEC;

        //
        // live parameters
        //
#ifdef OFX_EXTENSIONS_NATRON
        virtual const std::vector<std::string>& getUserCreatedPlanes() const;
#endif

        // The size of the current project in canonical coordinates.
        // The size of a project is a sub set of the kOfxImageEffectPropProjectExtent. For example a 
        // project may be a PAL SD project, but only be a letter-box within that. The project size is 
        // the size of this sub window. 
        virtual void getProjectSize(double& xSize, double& ySize) const = 0;

        // The offset of the current project in canonical coordinates. 
        // The offset is related to the kOfxImageEffectPropProjectSize and is the offset from the origin 
        // of the project 'subwindow'. For example for a PAL SD project that is in letterbox form, the
        // project offset is the offset to the bottom left hand corner of the letter box. The project 
        // offset is in canonical coordinates. 
        virtual void getProjectOffset(double& xOffset, double& yOffset) const = 0;

        // The extent of the current project in canonical coordinates. 
        // The extent is the size of the 'output' for the current project. See ProjectCoordinateSystems 
        // for more infomation on the project extent. The extent is in canonical coordinates and only 
        // returns the top right position, as the extent is always rooted at 0,0. For example a PAL SD 
        // project would have an extent of 768, 576. 
        virtual void getProjectExtent(double& xSize, double& ySize) const = 0;

        // The pixel aspect ratio of the current project 
        virtual double getProjectPixelAspectRatio() const = 0;

        // The duration of the effect 
        // This contains the duration of the plug-in effect, in frames. 
        virtual double getEffectDuration() const = 0;

        // For an instance, this is the frame rate of the project the effect is in. 
        virtual double getFrameRate() const = 0;

        /// This is called whenever a param is changed by the plugin so that
        /// the recursive instanceChangedAction will be fed the correct frame 
        virtual double getFrameRecursive() const = 0;

        /// This is called whenever a param is changed by the plugin so that
        /// the recursive instanceChangedAction will be fed the correct
        /// renderScale
        virtual void getRenderScaleRecursive(double &x, double &y) const = 0;

        /// Get whether the component is a supported 'chromatic' component (RGBA or alpha) in
        /// the base API.
        /// Override this if you have extended your chromatic colour types (eg RGB) and want
        /// the clip preferences logic to still work
        virtual bool isChromaticComponent(const std::string &str) const;

        /// function to check for multiple bit depth support
        /// The answer will depend on host, plugin and context
        virtual bool canCurrentlyHandleMultipleClipDepths() const;

        /// calculate the default rod for this effect instance
        virtual OfxRectD calcDefaultRegionOfDefinition(OfxTime  time,
                                                       OfxPointD   renderScale
#                                                      ifdef OFX_EXTENSIONS_NUKE
                                                       ,
                                                       int view
#                                                      endif
                                                       ) const;

        //
        // actions
        //

        /// this is used to populate with any extra action in argumnents that may be needed
        virtual void setCustomInArgs(const std::string &action, Property::Set &inArgs);
        
        /// this is used to populate with any extra action out argumnents that may be needed
        virtual void setCustomOutArgs(const std::string &action, Property::Set &outArgs);
        
        /// this is used retrieve any out args after the action was called in mainEntry
        virtual void examineOutArgs(const std::string &action, OfxStatus stat, const Property::Set &outArgs);
        
        /// create an instance. This needs to be called _after_ construction and
        /// _after_ the host populates it's params and clips with the 'correct'
        /// values (either persisted ones or the defaults)
        virtual OfxStatus createInstanceAction();

        /// destroy an instance. This needs to be called before the destructor,
        /// since it may call virtual functions
        virtual OfxStatus destroyInstanceAction();

        // begin/change/end instance changed

        //
        // why -
        //
        // kOfxChangeUserEdited   - the user or host changed the instance somehow and 
        //                          caused a change to something, this includes undo/redos, 
        //                          resets and loading values from files or presets,
        // kOfxChangePluginEdited - the plugin itself has changed the value of the instance 
        //                          in some action
        // kOfxChangeTime         - the time has changed and this has affected the value 
        //                          of the object because it varies over time
        //
        virtual OfxStatus beginInstanceChangedAction(const std::string &why);

        virtual OfxStatus paramInstanceChangedAction(const std::string &paramName,
                                                     const std::string & why,
                                                     OfxTime     time,
                                                     OfxPointD   renderScale);

        virtual OfxStatus clipInstanceChangedAction(const std::string &clipName,
                                                     const std::string & why,
                                                    OfxTime     time,
                                                    OfxPointD   renderScale);

        virtual OfxStatus endInstanceChangedAction(const std::string &why);

        // purge your caches
        virtual OfxStatus purgeCachesAction();

        // sync your private data
        virtual OfxStatus syncPrivateDataAction();

        // begin/end edit instance
        virtual OfxStatus beginInstanceEditAction();
        virtual OfxStatus endInstanceEditAction();

#     ifdef OFX_SUPPORTS_OPENGLRENDER
        // attach/detach OpenGL context
        virtual OfxStatus contextAttachedAction(
#                                              ifdef OFX_EXTENSIONS_NATRON
                                                void* &contextData
#                                              endif
                                                );
        virtual OfxStatus contextDetachedAction(
#                                              ifdef OFX_EXTENSIONS_NATRON
                                                void* contextData
#                                              endif
                                                );
#     endif

        // render action
        virtual OfxStatus beginRenderAction(OfxTime  startFrame,
                                            OfxTime  endFrame,
                                            OfxTime  step,
                                            bool     interactive,
                                            OfxPointD   renderScale,
                                            bool     sequentialRender,
                                            bool     interactiveRender,
#                                    ifdef OFX_SUPPORTS_OPENGLRENDER
                                            bool     openGLRender,
#                                         ifdef OFX_EXTENSIONS_NATRON
                                            void*    contextData,
#                                         endif
#                                    endif
                                            bool     draftRender
#                                         ifdef OFX_EXTENSIONS_NUKE
                                            ,
                                            int view
#                                         endif
                                            );

        virtual OfxStatus renderAction(OfxTime      time,
                                       const std::string &  field,
                                       const OfxRectI &renderRoI,
                                       OfxPointD   renderScale,
                                       bool     sequentialRender,
                                       bool     interactiveRender,
#                                    ifdef OFX_SUPPORTS_OPENGLRENDER
                                       bool     openGLRender,
#                                     ifdef OFX_EXTENSIONS_NATRON
                                       void*    contextData,
#                                     endif
#                                    endif
                                       bool     draftRender
#                                    if defined(OFX_EXTENSIONS_VEGAS) || defined(OFX_EXTENSIONS_NUKE)
                                       ,
                                       int view
#                                    endif
#                                    ifdef OFX_EXTENSIONS_VEGAS
                                       ,
                                       int nViews
#                                    endif
#                                    ifdef OFX_EXTENSIONS_NUKE
                                       ,
                                       const std::list<std::string>& planes
#                                    endif
                                       );

        virtual OfxStatus endRenderAction(OfxTime  startFrame,
                                          OfxTime  endFrame,
                                          OfxTime  step,
                                          bool     interactive,
                                          OfxPointD   renderScale,
                                          bool     sequentialRender,
                                          bool     interactiveRender,
#                                    ifdef OFX_SUPPORTS_OPENGLRENDER
                                          bool     openGLRender,
#                                       ifdef OFX_EXTENSIONS_NATRON
                                          void*    contextData,
#                                       endif
#                                    endif
                                          bool     draftRender
#                                       ifdef OFX_EXTENSIONS_NUKE
                                          ,
                                          int view
#                                       endif
                                          );

#ifdef OFX_EXTENSIONS_NUKE
        virtual OfxStatus getTransformAction(OfxTime time,
                                             const std::string& field,
                                             OfxPointD renderScale,
                                             bool draftRender,
                                             int view,
                                             std::string& clip,
                                             double transform[9]);
#endif

#ifdef OFX_EXTENSIONS_NATRON
        /// call the get distortion action at the given time
        virtual OfxStatus getInverseDistortionAction(OfxTime time,
                                              const std::string& field,
                                              OfxPointD renderScale,
                                              bool draftRender,
                                              int view,
                                              std::string& clip,
                                              double transform[9],
                                              OfxInverseDistortionFunctionV1* distortionFunc,
                                              void** distortionFunctionData,
                                              int* distortionFunctionDataSize,
                                              OfxInverseDistortionDataFreeFunctionV1* freeDataFunction);
#endif

        /// Call the region of definition action the plugin at the given time
        /// and with the given render scales. The value is returned in rod.
        /// Note that if the plugin does not trap the action the default
        /// RoD is calculated and returned. 
        virtual OfxStatus getRegionOfDefinitionAction(OfxTime  time,
                                                      OfxPointD   renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                                      int view,
#endif
                                                      OfxRectD &rod);
        
        /// call the get region of interest action on the plugin for the 
        /// given frame and renderscale. The render RoI is passed in in
        /// roi, the std::map will contain the requested rois. Note
        /// That this call will check for tiling support and for
        /// default replies and set up the correct rois in these cases
        /// as well
        virtual OfxStatus getRegionOfInterestAction(OfxTime  time,
                                                    OfxPointD   renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                                    int view,
#endif
                                                    const OfxRectD &roi,
                                                    std::map<ClipInstance *, OfxRectD> &rois);
          
#ifdef OFX_EXTENSIONS_NUKE
          /// Call the clipComponents action on the plug-in for the given frame and view.
          /// Enquires which components are needed on input and produced on output
          /// For each clip a list of all components. For input clips the components needed, for output clips the components produced)
          /// The passThroughClip will be set to an input clip to use as pass-through for all non rendered planes
          /// The passThroughTime is the time at which to pass-through, must have been registered by getFramesNeeded
          /// The passThroughView is the time at which to pass-through, must have been registered by getFrameViewsNeeded
        virtual OfxStatus getClipComponentsAction(OfxTime time,
                                                  int view,
                                                  ComponentsMap& clipComponents,
                                                  ClipInstance*& passThroughClip,
                                                  OfxTime& passThroughTime,
                                                  int& passThroughView);
          /// get frames/views needed for the given frame/view
        virtual OfxStatus getFrameViewsNeeded(OfxTime time,
                                              int view,
                                              ViewsRangeMap& rangeMap);
#endif

        // get frames needed to render the given frame
        virtual OfxStatus getFrameNeededAction(OfxTime time, 
                                               RangeMap &rangeMap);
 
        // is identity
        virtual OfxStatus isIdentityAction(OfxTime     &time,
                                           const std::string &  field,
                                           const OfxRectI  &renderRoI,
                                           OfxPointD   renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                           int& view,
                                           std::string& plane,
#endif
                                           std::string &clip);

        // time domain
        virtual OfxStatus getTimeDomainAction(OfxRangeD& range);

#ifdef OFX_SUPPORTS_DIALOG
        // OfxDialogSuiteV1
        /// @see kOfxActionDialog
        virtual OfxStatus dialog(void *instanceData);
#endif

        /// Get the interact description, this will also call describe on the interact
        /// This will return NULL if there is not main entry point or if the description failed
        /// otherwise it will return the described overlay
        /// This is called by the CTOR of OverlayInteract to get the descriptor to do things with
        Interact::Descriptor &getOverlayDescriptor(int bitDepthPerComponent = 8, bool hasAlpha = false);
       
        /// Setup the default clip preferences on the clips
        virtual void setDefaultClipPreferences();

        /// Initialise the clip preferences arguments, override this to do
        /// stuff with wierd components etc... Calls setDefaultClipPreferences
        virtual void setupClipPreferencesArgs(Property::Set &args);

        /// Run the clip preferences action from the effect.
        /// 
        /// This will look into the input clips and output clip
        /// and set the following properties that the effect should 
        /// fetch the image at.
        ///     - pixel depth
        ///     - components
        ///     - pixel aspect ratio
        /// It will also set on the effect itselff
        ///     - whether it is continuously samplable
        ///     - the premult state of the output
        ///     - whether the effect is frame varying
        ///     - the fielding of the output clip
        ///
        /// This will be run automatically by the effect in the following situations...
        ///     - an input clip is changed
        ///     - a clip preferences slave param is changed
        /// 
        /// The host still needs to call this explicitly just after the effect is wired
        /// up.
        virtual bool getClipPreferences();

        /// calls getClipPreferences only if the prefs are dirty
        ///
        /// returns whether the clips prefs were dirty or not
        bool runGetClipPrefsConditionally()
        {
          if(areClipPrefsDirty()) {
            getClipPreferences();
            return true;
          }
          return false;
        }
        
        /// find the best supported bit depth for the given one. Override this if you define
        /// more depths
        virtual const std::string &bestSupportedDepth(const std::string &depth) const;

        /// find the most chromatic components out of the two. Override this if you define
        /// more chromatic components
        virtual const std::string &findMostChromaticComponents(const std::string &a, const std::string &b) const;
          
#ifdef OFX_EXTENSIONS_NUKE
        /// Returns the number of views
        virtual OfxStatus getViewCount(int *nViews) const = 0;
          
        /// Returns the view textual representation
        /// The string is owned by the host and must be valid throughout the calling action
        virtual OfxStatus getViewName(int viewIndex, const char** name) const = 0;
#endif

#ifdef OFX_EXTENSIONS_NATRON
        // get the list of parameters (name) in the order they should appear in the host viewport
        virtual bool isInViewportParam(const std::string& paramName) const;
#endif
      };

      ////////////////////////////////////////////////////////////////////////////////
      /// An overlay interact for image effects, derived from one of these to
      /// be an overlay interact
      class OverlayInteract : public Interact::Instance {
      protected :
        /// our image effect instance
        ImageEffect::Instance &_instance;

      public    :
        /// ctor this calls Instance->getOverlayDescriptor to get the descriptor
        OverlayInteract(ImageEffect::Instance &v, int bitDepthPerComponent = 8, bool hasAlpha = false);
      };


    } // namespace ImageEffect

  } // namespace Host

} // namespace OFX

#endif // OFX_IMAGE_EFFECT_H
