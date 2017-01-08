/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_EFFECTINSTANCE_H
#define NATRON_ENGINE_EFFECTINSTANCE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <bitset>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Global/KeySymbols.h"

#include "Engine/ImageComponents.h"
#include "Engine/Knob.h" // for KnobHolder
#include "Engine/RectD.h"
#include "Engine/RectI.h"
#include "Engine/RenderStats.h"
#include "Engine/EffectInstanceActionResults.h"
#include "Engine/TreeRenderNodeArgs.h"
#include "Engine/PluginActionShortcut.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

// Various useful plugin IDs, @see EffectInstance::getPluginID()
#define PLUGINID_OFX_MERGE        "net.sf.openfx.MergePlugin"
#define PLUGINID_OFX_TRACKERPM    "net.sf.openfx.TrackerPM"
#define PLUGINID_OFX_DOTEXAMPLE   "net.sf.openfx.dotexample"
#define PLUGINID_OFX_RADIAL       "net.sf.openfx.Radial"
#define PLUGINID_OFX_SENOISE      "net.sf.openfx.SeNoise"
#define PLUGINID_OFX_READOIIO     "fr.inria.openfx.ReadOIIO"
#define PLUGINID_OFX_WRITEOIIO    "fr.inria.openfx.WriteOIIO"
#define PLUGINID_OFX_ROTO         "net.sf.openfx.RotoPlugin"
#define PLUGINID_OFX_TRANSFORM    "net.sf.openfx.TransformPlugin"
#define PLUGINID_OFX_TIMEBLUR     "net.sf.openfx.TimeBlur"
#define PLUGINID_OFX_GRADE        "net.sf.openfx.GradePlugin"
#define PLUGINID_OFX_COLORCORRECT "net.sf.openfx.ColorCorrectPlugin"
#define PLUGINID_OFX_COLORLOOKUP  "net.sf.openfx.ColorLookupPlugin"
#define PLUGINID_OFX_BLURCIMG     "net.sf.cimg.CImgBlur"
#define PLUGINID_OFX_INVERT       "net.sf.openfx.Invert"
#define PLUGINID_OFX_SHARPENCIMG  "net.sf.cimg.CImgSharpen"
#define PLUGINID_OFX_SOFTENCIMG   "net.sf.cimg.CImgSoften"
#define PLUGINID_OFX_CORNERPIN    "net.sf.openfx.CornerPinPlugin"
#define PLUGINID_OFX_CONSTANT     "net.sf.openfx.ConstantPlugin"
#define PLUGINID_OFX_TIMEOFFSET   "net.sf.openfx.timeOffset"
#define PLUGINID_OFX_FRAMEHOLD    "net.sf.openfx.FrameHold"
#define PLUGINID_OFX_NOOP         "net.sf.openfx.NoOpPlugin"
#define PLUGINID_OFX_PREMULT      "net.sf.openfx.Premult"
#define PLUGINID_OFX_UNPREMULT    "net.sf.openfx.Unpremult"
#define PLUGINID_OFX_RETIME       "net.sf.openfx.Retime"
#define PLUGINID_OFX_FRAMERANGE   "net.sf.openfx.FrameRange"
#define PLUGINID_OFX_RUNSCRIPT    "fr.inria.openfx.RunScript"
#define PLUGINID_OFX_READFFMPEG   "fr.inria.openfx.ReadFFmpeg"
#define PLUGINID_OFX_SHUFFLE      "net.sf.openfx.ShufflePlugin"
#define PLUGINID_OFX_TIMEDISSOLVE "net.sf.openfx.TimeDissolvePlugin"
#define PLUGINID_OFX_WRITEFFMPEG  "fr.inria.openfx.WriteFFmpeg"
#define PLUGINID_OFX_READPFM      "fr.inria.openfx.ReadPFM"
#define PLUGINID_OFX_WRITEPFM     "fr.inria.openfx.WritePFM"
#define PLUGINID_OFX_READMISC     "fr.inria.openfx.ReadMisc"
#define PLUGINID_OFX_READPSD      "net.fxarena.openfx.ReadPSD"
#define PLUGINID_OFX_READKRITA    "fr.inria.openfx.ReadKrita"
#define PLUGINID_OFX_READSVG      "net.fxarena.openfx.ReadSVG"
#define PLUGINID_OFX_READORA      "fr.inria.openfx.OpenRaster"
#define PLUGINID_OFX_READCDR      "fr.inria.openfx.ReadCDR"
#define PLUGINID_OFX_READPNG      "fr.inria.openfx.ReadPNG"
#define PLUGINID_OFX_WRITEPNG     "fr.inria.openfx.WritePNG"
#define PLUGINID_OFX_READPDF      "fr.inria.openfx.ReadPDF"

#define PLUGINID_NATRON_VIEWER_GROUP        (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Viewer")
#define PLUGINID_NATRON_VIEWER_INTERNAL     (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.ViewerInternal")
#define PLUGINID_NATRON_DISKCACHE           (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.DiskCache")
#define PLUGINID_NATRON_DOT                 (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Dot")
#define PLUGINID_NATRON_READQT              (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.ReadQt")
#define PLUGINID_NATRON_WRITEQT             (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.WriteQt")
#define PLUGINID_NATRON_GROUP               (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Group")
#define PLUGINID_NATRON_INPUT               (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Input")
#define PLUGINID_NATRON_OUTPUT              (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Output")
#define PLUGINID_NATRON_BACKDROP            (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.BackDrop") // DO NOT change the capitalization, even if it's wrong
#define PLUGINID_NATRON_ROTOPAINT           (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.RotoPaint")
#define PLUGINID_NATRON_ROTO                (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Roto")
#define PLUGINID_NATRON_ROTOSHAPE           (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.RotoShape")
#define PLUGINID_NATRON_LAYEREDCOMP         (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.LayeredComp")
#define PLUGINID_NATRON_PRECOMP             (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Precomp")
#define PLUGINID_NATRON_TRACKER             (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Tracker")
#define PLUGINID_NATRON_JOINVIEWS           (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.JoinViews")
#define PLUGINID_NATRON_READ                (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Read")
#define PLUGINID_NATRON_WRITE               (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Write")
#define PLUGINID_NATRON_ONEVIEW             (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.OneView")
#define PLUGINID_NATRON_STUB                (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Stub")

#define kReaderParamNameOriginalFrameRange "originalFrameRange"
#define kReaderParamNameFirstFrame "firstFrame"
#define kReaderParamNameLastFrame "lastFrame"

struct PlaneToRender
{
    // Points to the image composed of mono channel tiles stored with mmap 
    ImagePtr cacheImage;

    // Points to a temporary image that the plug-in will render
    ImagePtr tmpImage;

    PlaneToRender()
    : cacheImage()
    , tmpImage()
    {
    }
};


/**
 * @brief This is the base class for visual effects.
 **/
class EffectInstance
    : public NamedKnobHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    struct RenderActionArgs
    {
        // The time at which to render
        TimeValue time;

        // The view to render
        ViewIdx view;

        // The scale at which this effect is rendering
        RenderScale renderScale;

        // The render window: this is the portion to render for each output plane
        RectI roi;

        // The list of output planes: these are the images to write to in output
        std::list<std::pair<ImageComponents, ImagePtr > > outputPlanes;

        // The render args for this node.
        TreeRenderNodeArgsPtr renderArgs;

        // Should render use OpenGL or CPU
        RenderBackendTypeEnum backendType;

        // The OpenGL context to used to render if backend type is set to eRenderBackendTypeOpenGL
        // or eRenderBackendTypeOSMesa
        OSGLContextAttacherPtr glContextAttacher;

        // The effect data attached to the current OpenGL context. These are the data that were returned by
        // attachOpenGLContext.
        EffectOpenGLContextDataPtr glContextData;

        // The RGBA channels to process. This can optimize render times for un-needed channels.
        std::bitset<4> processChannels;
        
    };


    enum RenderRoITypeEnum
    {
        // The frame view was only requested via getFramesNeeded by a node downstream the usual way.
        // In this case we know exactly how many times a frame is requested and can correctly union
        // all region of interests to do a single render pass.
        // In this case we just have to lock the image in the cache until all downstream nodes have rendered
        // their image.
        eRenderRoITypeKnownFrame,

        // If the plug-in called getImage without advertising first that it would need the image from
        // getFramesNeeded, then this frame is considered "unknown":
        // We don't know if it's going to be requested again by another branch of the tree and we don't know
        // if the RoI so far is going to enclose all requests.
        // This flag indicates that we MUST lock the image in the cache through the whole render of the frame.
        eRenderRoITypeUnknownFrame
    };
    



    struct RenderRoIArgs
    {
        // Developper note: the fields were reordered to optimize packing.
        // see http://www.catb.org/esr/structure-packing/

        // The time at which to render
        TimeValue time;

        // The view to render
        ViewIdx view;

        // The rectangle to render (in pixel coordinates)
        RectI roi;

        // The image planes to render
        std::list<ImageComponents> components;

        // The effect calling renderRoI if within getImage
        EffectInstancePtr caller;

        // The input number of this node in the caller node
        int inputNbInCaller;

        // Set to false if you don't want the node to render using the GPU at all
        // This is useful if the previous render failed because GPU memory was maxed out
        bool allowGPURendering;

        // the time that was passed to the original renderRoI call of the caller node
        TimeValue callerRenderTime;

        // Is this a known frame or not
        RenderRoITypeEnum type;

        TreeRenderNodeArgsPtr renderArgs;

        RenderRoIArgs()
        : time(0)
        , view(0)
        , roi()
        , components()
        , caller()
        , inputNbInCaller(-1)
        , allowGPURendering(true)
        , callerRenderTime(0.)
        , type(eRenderRoITypeKnownFrame)
        , renderArgs()
        {
        }

        RenderRoIArgs(TimeValue time_,
                      ViewIdx view_,
                      const RectI & roi_,
                      const std::list<ImageComponents> & components_,
                      EffectInstancePtr caller,
                      int inputNbInCaller,
                      TimeValue callerRenderTime,
                      const TreeRenderNodeArgsPtr& renderArgs )
        : time(time_)
        , view(view_)
        , roi(roi_)
        , components(components_)
        , caller(caller)
        , inputNbInCaller(inputNbInCaller)
        , allowGPURendering(true)
        , callerRenderTime(callerRenderTime)
        , renderArgs(renderArgs)
        {
        }
    };


protected: // derives from KnobHolder, parent of JoinViewsNode, OneViewNode, PrecompNode, ReadNode, RotoPaint
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    /**
     * @brief Constructor used once for each node created. Its purpose is to create the "live instance".
     * You shouldn't do any heavy processing here nor lengthy initialization as the constructor is often
     * called just to be able to call a few virtuals fonctions.
     * The constructor is always called by the main thread of the application.
     **/
    explicit EffectInstance(const NodePtr& node);

protected:
    EffectInstance(const EffectInstance& other);

public:

    EffectInstancePtr shared_from_this() {
        return boost::dynamic_pointer_cast<EffectInstance>(KnobHolder::shared_from_this());
    }

    EffectInstanceConstPtr shared_from_this() const {
        return boost::dynamic_pointer_cast<const EffectInstance>(KnobHolder::shared_from_this());
    }

    // dtor
    virtual ~EffectInstance();


    struct GetImageInArgs
    {
        // The input nb to fetch the image from
        //
        // Default - 0
        int inputNb;

        // The time to sample on the input
        //
        // Default - 0
        TimeValue inputTime;

        // The view to sample on the input
        //
        // Default - 0
        ViewIdx inputView;

        // The current action scale
        // - Must be set
        RenderScale currentScale;

        // The current action time
        // - Must be set
        TimeValue currentTime;

        // The current action view
        // - Must be set
        ViewIdx currentView;

        // When calling getImage while not during a render, these are the bounds to render in canonical coordinates.
        // If not specified, this will ask to render the full region of definition.
        //
        // Default - NULL
        const RectD* optionalBounds;

        // If set this is the layers to fetch, otherwise we use the result of
        // the getClipComponents action
        //
        // Default - NULL
        const std::list<ImageComponents>* layers;

        // The backend that should be used to return the image. E.G: the input may compute a OpenGL texture but the effect pulling
        // the image may not support OpenGL. In this case setting storage to eRenderBackendTypeCPU would convert the OpenGL texture to
        // a RAM image. If NULL, the image returned from the upstream plug-in will not be changed.
        //
        // Default - NULL
        const RenderBackendTypeEnum* renderBackend;

        // A pointer to the render args of this node. MUST BE SET
        //
        // Default - NULL
        TreeRenderNodeArgsPtr renderArgs;

        GetImageInArgs();

        // Initialize the inArgs with the current render action args
        GetImageInArgs(const RenderActionArgs& args);
    };

    struct GetImageOutArgs
    {
        // For each plane requested the associated image.
        std::map<ImageComponents, ImagePtr> imagePlanes;

        // The roi of the input effect effect on the image. This may be useful e.g to limit the bounds accessed by the plug-in
        RectI roiPixel;

        // Whether we have a distorsion stack to apply
        Distorsion2DStackPtr distorsionStack;
    };

    /** @brief This is the main-entry point to pull images from up-stream of this node.
     * This function may be used in 2 different situations:
     *
     * 1) We are currently in the render of a plug-in and wish to fetch images from above.
     * In this case the image in input has probably already been pre-computed and cached away using the
     * getFramesNeeded and getRegionsOfInterest actions to drive the algorithm.
     *
     * 2) We are not currently rendering: for example we may be in the knobChanged or inputChanged or getDistorsion or drawOverlay action:
     * This image will pull images from upstream that have not yet be pre-computed.
     * Internally it will call TreeRender::launchRender.
     * In this case the inArgs have an extra
     * optionalBounds parameter that can be set to specify which portion of the input image is needed.
     *
     * Note that this is very important in the case 1) that all images that are accessed via getImage() are
     * declared to Natron with getFramesNeeded otherwise Natron has to fallback to a less optimized pipeline 
     * that keeps image longer in memory and it may risk to render twice an effect.
     *
     * @returns This function returns true if the image set in the outArgs is valid.
     * Note that in the outArgs is returned the distorsion stack to apply if this effect was marked
     * as getCanDistort().
     *
     **/
    bool getImagePlanes(const GetImageInArgs& inArgs, GetImageOutArgs* outArgs) WARN_UNUSED_RETURN;

private:

    bool resolveRoIForGetImage(const GetImageInArgs& inArgs,
                               TimeValue inputTime,
                               RectD* roiCanonical,
                               RenderRoITypeEnum* type);

public:

    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    /////////////// Actions to be overriden by the effect   //////////////

    /**
     * @brief Wrapper around getComponentsNeededAndProduced, see getComponentsNeededAndProduced.
     **/
    ActionRetCodeEnum getComponents_public(TimeValue time, ViewIdx view, const TreeRenderNodeArgsPtr& render, GetComponentsResultsPtr* results);

    /**
     * @brief Returns the layer availables for the given inputNb. If inputNb is -1, this returns the layers available in output.
     * The layers available are the layers produced by the corresponding node, plus the pass-through layers plus the default project
     * layers plus the user created layers at this node.
     **/
    ActionRetCodeEnum getAvailableLayers(TimeValue time, ViewIdx view, int inputNb, const TreeRenderNodeArgsPtr& render, std::list<ImageComponents>* availableLayers) ;

protected:

    /**
     * @brief Called by getComponents_public, to be overriden.
     * Must return the layers needed for each input and the layers produced in output of the effect.
     * If this effect allows some layers to pass-through the effect (i.e: does this effect inherits layers from upstream?)
     * then it must specify the pass-through inputNb and the time and view at which to fetch the pass-through plane.
     * Default implementation returns the components returned by getTimeInvariantMetadatas on the color plane.
     **/
    virtual ActionRetCodeEnum getComponentsAction(TimeValue time,
                                                  ViewIdx view,
                                                  const TreeRenderNodeArgsPtr& render,
                                                  std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                                  std::list<ImageComponents>* layersProduced,
                                                  TimeValue* passThroughTime,
                                                  ViewIdx* passThroughView,
                                                  int* passThroughInputNb);

private:

    void getDefaultComponentsNeededAndProduced(TimeValue time,
                                               ViewIdx view,
                                               const TreeRenderNodeArgsPtr& render,
                                               std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                               std::list<ImageComponents>* layersProduced,
                                               TimeValue* passThroughTime,
                                               ViewIdx* passThroughView,
                                               int* passThroughInputNb,
                                               bool* processAll,
                                               std::bitset<4>* processChannels);


    ActionRetCodeEnum getComponentsNeededInternal(TimeValue time,
                                                  ViewIdx view,
                                                  const TreeRenderNodeArgsPtr& render,
                                                  std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                                  std::list<ImageComponents>* layersProduced,
                                                  TimeValue* passThroughTime,
                                                  ViewIdx* passThroughView,
                                                  int* passThroughInputNb,
                                                  bool* processAll,
                                                  std::bitset<4>* processChannels);
    
    

public:



    /**
     * @brief Wrapper around attachOpenGLContext, see attachOpenGLContext
     **/
    ActionRetCodeEnum attachOpenGLContext_public(TimeValue time, ViewIdx view, const RenderScale& scale, const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data);

    /**
     * @brief Wrapper around dettachOpenGLContext, see dettachOpenGLContext
     **/
    ActionRetCodeEnum dettachOpenGLContext_public(const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, const EffectOpenGLContextDataPtr& data);

    /**
     * @brief Called for plug-ins that support concurrent OpenGL renders when the effect is about to be destroyed to release all contexts data.
     **/
    void dettachAllOpenGLContexts();


protected:

    /**
     * @brief This function must initialize all OpenGL context related data such as shaders, LUTs, etc...
     * This function will be called once per context. The function dettachOpenGLContext() will be called
     * on the value returned by this function to deinitialize all context related data.
     *
     * If the function supportsConcurrentOpenGLRenders() returns false, each call to attachOpenGLContext must be followed by
     * a call to dettachOpenGLContext before attaching a DIFFERENT context, meaning the plug-in thread-safety is instance safe at most.
     *
     **/
    virtual ActionRetCodeEnum attachOpenGLContext(TimeValue time, ViewIdx view, const RenderScale& scale, const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data);

    /**
     * @brief This function must free all OpenGL context related data that were allocated previously in a call to attachOpenGLContext().
     **/
    virtual ActionRetCodeEnum dettachOpenGLContext(const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, const EffectOpenGLContextDataPtr& data);


public:

    /// Overlay actions

    void drawOverlay_public(TimeValue time, const RenderScale & renderScale, ViewIdx view);

    bool onOverlayPenDown_public(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp, PenType pen) WARN_UNUSED_RETURN;

    bool onOverlayPenMotion_public(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) WARN_UNUSED_RETURN;

    bool onOverlayPenDoubleClicked_public(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos) WARN_UNUSED_RETURN;

    bool onOverlayPenUp_public(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) WARN_UNUSED_RETURN;

    bool onOverlayKeyDown_public(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyUp_public(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyRepeat_public(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayFocusGained_public(TimeValue time, const RenderScale & renderScale, ViewIdx view) WARN_UNUSED_RETURN;

    bool onOverlayFocusLost_public(TimeValue time, const RenderScale & renderScale, ViewIdx view) WARN_UNUSED_RETURN;

protected:

    virtual void drawOverlay(TimeValue time,
                             const RenderScale & renderScale,
                             ViewIdx view);

    virtual bool onOverlayPenDown(TimeValue time,
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  const QPointF & viewportPos,
                                  const QPointF & pos,
                                  double pressure,
                                  TimeValue timestamp,
                                  PenType pen) WARN_UNUSED_RETURN;

    virtual bool onOverlayPenDoubleClicked(TimeValue time,
                                           const RenderScale & renderScale,
                                           ViewIdx view,
                                           const QPointF & viewportPos,
                                           const QPointF & pos) WARN_UNUSED_RETURN;

    virtual bool onOverlayPenMotion(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    const QPointF & viewportPos,
                                    const QPointF & pos,
                                    double pressure,
                                    TimeValue timestamp) WARN_UNUSED_RETURN;

    virtual bool onOverlayPenUp(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double pressure,
                                TimeValue timestamp) WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyDown(TimeValue time,
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  Key key,
                                  KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyUp(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                Key key,
                                KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyRepeat(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    Key key,
                                    KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    virtual bool onOverlayFocusGained(TimeValue time,
                                      const RenderScale & renderScale,
                                      ViewIdx view) WARN_UNUSED_RETURN;

    virtual bool onOverlayFocusLost(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view) WARN_UNUSED_RETURN;

public:

    /**
     * @brief For sequential effects, this is called before the first call to render .
     * For non sequential effects, this is called before each call to render.
     **/
    ActionRetCodeEnum beginSequenceRender_public(double first, double last,
                                          double step, bool interactive, const RenderScale & scale,
                                          bool isSequentialRender, bool isRenderResponseToUserInteraction,
                                          bool draftMode,
                                          ViewIdx view,
                                          RenderBackendTypeEnum backendType,
                                          const EffectOpenGLContextDataPtr& glContextData,
                                          const TreeRenderNodeArgsPtr& render);


    /**
     * @brief For sequential effects, this is called after the last call to render in a sequence.
     * For non sequential effects, this is called after each call to render.
     **/
    ActionRetCodeEnum endSequenceRender_public(double first, double last,
                                        double step, bool interactive, const RenderScale & scale,
                                        bool isSequentialRender, bool isRenderResponseToUserInteraction,
                                        bool draftMode,
                                        ViewIdx view,
                                        RenderBackendTypeEnum backendType,
                                        const EffectOpenGLContextDataPtr& glContextData,
                                        const TreeRenderNodeArgsPtr& render);



    /**
     * @brief The main render action. This should render onto the given output planes.
     **/
    ActionRetCodeEnum render_public(const RenderActionArgs & args) WARN_UNUSED_RETURN;


protected:

    virtual ActionRetCodeEnum beginSequenceRender(double first,
                                           double last,
                                           double step,
                                           bool interactive,
                                           const RenderScale & scale,
                                           bool isSequentialRender,
                                           bool isRenderResponseToUserInteraction,
                                           bool draftMode,
                                           ViewIdx view,
                                           RenderBackendTypeEnum backendType,
                                           const EffectOpenGLContextDataPtr& glContextData,
                                           const TreeRenderNodeArgsPtr& render);

    virtual ActionRetCodeEnum endSequenceRender(double first,
                                         double last,
                                         double step,
                                         bool interactive,
                                         const RenderScale & scale,
                                         bool isSequentialRender,
                                         bool isRenderResponseToUserInteraction,
                                         bool draftMode,
                                         ViewIdx view,
                                         RenderBackendTypeEnum backendType,
                                         const EffectOpenGLContextDataPtr& glContextData,
                                         const TreeRenderNodeArgsPtr& render);

    virtual ActionRetCodeEnum render(const RenderActionArgs & /*args*/) WARN_UNUSED_RETURN;

public:

    /**
     * @brief For effects that can return a distorsion function (e.g an affine Transform), then they 
     * may flag so with the getCanDistort() function. In this case this function will be called prior to
     * calling render. If possible, Natron will concatenate distorsion effects and only the effect at
     * the bottom will do the final rendering.
     **/
    ActionRetCodeEnum getDistorsion_public(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    const TreeRenderNodeArgsPtr& render,
                                    GetDistorsionResultsPtr* distorsion) WARN_UNUSED_RETURN;



protected:



    virtual ActionRetCodeEnum getDistorsion(TimeValue time,
                                     const RenderScale & renderScale,
                                     ViewIdx view,
                                     const TreeRenderNodeArgsPtr& render,
                                     DistorsionFunction2D* distorsion) WARN_UNUSED_RETURN;
    
public:

    /**
     * @brief Indicates whether the effect is an identity, i.e: it doesn't produce
     * any change in output and is a pass-through.
     * @param time The time that should be passed to render()
     * @param scale The scale of the renderWindow
     * @param renderWindow The portion of the image over which the effect should be identity
     * @param view The view that should be passed to render()
     * @param inputTime[out] the input time to which this plugin is identity of
     * @param inputNb[out] the input number of the effect that is identity of.
     * The special value of -2 indicates that the plugin is identity of itself at another time
     * @param inputView[out] the input view of the effect that is identity of.
     **/
    ActionRetCodeEnum isIdentity_public(bool useIdentityCache, // only set to true when calling for the whole image (not for a subrect)
                           TimeValue time,
                           const RenderScale & scale,
                           const RectI & renderWindow,
                           ViewIdx view,
                           const TreeRenderNodeArgsPtr& render,
                           IsIdentityResultsPtr* results) WARN_UNUSED_RETURN;
protected:


    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                                  const RenderScale & scale,
                                  const RectI & roi,
                                  ViewIdx view,
                                  const TreeRenderNodeArgsPtr& render,
                                  TimeValue* inputTime,
                                  ViewIdx* inputView,
                                  int* inputNb) WARN_UNUSED_RETURN;
    
public:

    /**
     * @brief Return the region that the plugin is capable of filling.
     * This is meaningful for plugins that generate images or transform images.
     * By default it returns in rod the union of all inputs RoD and eStatusReplyDefault is returned.
     * In case of failure the plugin should return eStatusFailed.
     **/
    ActionRetCodeEnum getRegionOfDefinition_public(TimeValue time,
                                            const RenderScale & scale,
                                            ViewIdx view,
                                            const TreeRenderNodeArgsPtr& render,
                                            GetRegionOfDefinitionResultsPtr* results) WARN_UNUSED_RETURN;

    ActionRetCodeEnum getRegionOfDefinitionFromCache(TimeValue time,
                                              const RenderScale & scale,
                                              ViewIdx view,
                                              RectD* rod) WARN_UNUSED_RETURN;

    void calcDefaultRegionOfDefinition_public(TimeValue time, const RenderScale & scale, ViewIdx view, const TreeRenderNodeArgsPtr& render, RectD *rod);


    /**
     * @brief If the instance rod is infinite, returns the union of all connected inputs. If there's no input this returns the
     * project format.
     **/
    void ifInfiniteApplyHeuristic(TimeValue time,
                                  const RenderScale & scale,
                                  ViewIdx view,
                                  const TreeRenderNodeArgsPtr& render,
                                  RectD* rod); //!< input/output


protected:


    virtual ActionRetCodeEnum getRegionOfDefinition(TimeValue time, const RenderScale & scale, ViewIdx view,
                                             const TreeRenderNodeArgsPtr& render,
                                             RectD* rod) WARN_UNUSED_RETURN;

    virtual void calcDefaultRegionOfDefinition(TimeValue time, const RenderScale & scale, ViewIdx view, const TreeRenderNodeArgsPtr& render,RectD *rod);

public:

    /**
     * @brief Indicates for each input node what is the region of interest
     * of the node at time 'time' and render scale 'scale' given a render window.
     * For exemple a blur plugin would specify what it needs
     * from inputs in order to do a blur taking into account the size of the blurring kernel.
     * By default, it returns renderWindow for each input.
     **/
    ActionRetCodeEnum getRegionsOfInterest_public(TimeValue time,
                                     const RenderScale & scale,
                                     const RectD & renderWindow,   //!< the region to be rendered in the output image, in Canonical Coordinates
                                     ViewIdx view,
                                     const TreeRenderNodeArgsPtr& render,
                                     RoIMap* ret);

protected:



    virtual ActionRetCodeEnum getRegionsOfInterest(TimeValue time,
                                      const RenderScale & scale,
                                      const RectD & renderWindow,
                                      ViewIdx view,
                                      const TreeRenderNodeArgsPtr& render,
                                      RoIMap* ret);

public:

    /**
     * @brief Computes the frame/view pairs needed by this effect in input for the render action.
     * @param time The time at which the input should be sampled
     * @param view the view at which the input should be sampled
     * @param hash If set this will return the hash of the node for the given time view. In a
     * render thread, this hash should be cached away
     **/
    ActionRetCodeEnum getFramesNeeded_public(TimeValue time, ViewIdx view, const TreeRenderNodeArgsPtr& render, GetFramesNeededResultsPtr* results);

protected:


    virtual ActionRetCodeEnum getFramesNeeded(TimeValue time, ViewIdx view, const TreeRenderNodeArgsPtr& render, FramesNeededMap* results) ;

public:

    /**
     * @brief Can be derived to get the frame range wherein the plugin is capable of producing frames.
     * By default it merges the frame range of the inputs.
     * In case of failure the plugin should return eStatusFailed.
     **/
    ActionRetCodeEnum getFrameRange_public(const TreeRenderNodeArgsPtr& render, GetFrameRangeResultsPtr* results);


protected:


    virtual ActionRetCodeEnum getFrameRange(const TreeRenderNodeArgsPtr& render, double *first, double *last);

public:

    virtual void beginKnobsValuesChanged_public(ValueChangedReasonEnum reason) OVERRIDE FINAL;

    virtual void endKnobsValuesChanged_public(ValueChangedReasonEnum reason) OVERRIDE FINAL;

    /**
     * @breif Don't override this one, override onKnobValueChanged instead.
     **/
    virtual bool onKnobValueChanged_public(const KnobIPtr& k, ValueChangedReasonEnum reason, TimeValue time, ViewSetSpec view) OVERRIDE FINAL;

    void beginEditKnobs_public();

protected:

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once.
     **/
    virtual void beginKnobsValuesChanged(ValueChangedReasonEnum /*reason*/) OVERRIDE
    {
    }

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once.
     **/
    virtual void endKnobsValuesChanged(ValueChangedReasonEnum /*reason*/) OVERRIDE
    {
    }

    /**
     * @brief Called right away when the user first opens the settings panel of the node.
     * This is called after each params had its default value set.
     **/
    virtual void beginEditKnobs()
    {
    }


    /**
     * @brief Called whenever a param changes
     **/
    virtual bool knobChanged(const KnobIPtr& knob,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time);

public:

    void onInputChanged_public(int inputNo, const NodePtr& oldNode, const NodePtr& newNode);

protected:


    /**
     * @brief Called anytime an input connection is changed
     **/
    virtual void onInputChanged(int inputNo, const NodePtr& oldNode, const NodePtr& newNode);


public:

    void createInstanceAction_public();

protected:

    virtual void createInstanceAction() {}

public:


    /**
     * @brief Returns the preferred metadata to render with
     * This should only be called to compute the clip preferences, call the appropriate
     * getters to get the actual values.
     * The default implementation gives reasonable values appropriate to the context of the node (the inputs)
     * and the values reported by the supported components/bitdepth
     *
     * This should not be reimplemented except for OpenFX which already has its default specification
     * for clip Preferences, see setDefaultClipPreferences()
     *
     **/
    ActionRetCodeEnum getTimeInvariantMetaDatas_public(const TreeRenderNodeArgsPtr& render, GetTimeInvariantMetaDatasResultsPtr* results);

    ActionRetCodeEnum getDefaultMetadata(const TreeRenderNodeArgsPtr& render, NodeMetadata& metadata);

protected:

    virtual ActionRetCodeEnum getTimeInvariantMetaDatas(NodeMetadata& /*metadata*/) { return eActionStatusReplyDefault; }


public:

    void purgeCaches_public();

protected:

    /**
     * @brief Can be overloaded to clear any cache the plugin might be
     * handling on his side.
     **/
    virtual void purgeCaches()
    {
    };


public:



    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////

    
    /**
     * @brief Returns a pointer to the node holding this effect.
     **/
    NodePtr getNode() const WARN_UNUSED_RETURN
    {
        return _node.lock();
    }

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash) OVERRIDE;


public:

    /**
     * @brief Recursively invalidates the hash of this node and the nodes downstream.
     **/
    virtual bool invalidateHashCacheInternal(std::set<HashableObject*>* invalidatedObjects) OVERRIDE ;

private:

    bool invalidateHashCacheImplementation(const bool recurse, std::set<HashableObject*>* invalidatedObjects);

public:

    virtual TimeValue getCurrentTime_TLS() const OVERRIDE;
    virtual ViewIdx getCurrentView_TLS() const OVERRIDE;
    RenderValuesCachePtr getRenderValuesCache_TLS(TimeValue* currentTime, ViewIdx* currentView) const;
    void setCurrentRender_TLS(const TreeRenderNodeArgsPtr& render);
    TreeRenderNodeArgsPtr getCurrentRender_TLS() const;
    EffectInstanceTLSDataPtr getTLSObject() const;

    /**
     * @brief Forwarded to the node's name
     **/
    const std::string & getScriptName() const WARN_UNUSED_RETURN;
    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onScriptNameChanged(const std::string& /*fullyQualifiedName*/) {}


    /**
     * @brief Forwarded to the node's render views count
     **/
    int getRenderViewsCount() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns input n. It might be NULL if the input is not connected.
     * MT-Safe
     **/
    EffectInstancePtr getInput(int n) const WARN_UNUSED_RETURN;


    /**
     * @brief Implement to returns what input should be used by default.
     * @param connected If true, should return in inputIndex an input that is connected, otherwise -1
     * (e.g: when the node is disabled).
     * If false, should return an input that is not yet connected, otherwise -1 (e.g: when auto-connecting
     * a node into an existing pipe).
     * By default this function does nothing and the implementation of Node::getPreferredInputInternal is used.
     **/
    virtual bool getDefaultInput(bool connected, int* inputIndex) const
    {
        Q_UNUSED(connected);
        Q_UNUSED(inputIndex);
        return false;
    }

    /**
     * @brief Forwarded to the node holding the effect
     **/
    bool hasOutputConnected() const WARN_UNUSED_RETURN;

    /**
     * @brief Is this node an input node ? An input node means
     * it has no input.
     **/
    virtual bool isGenerator() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Is the node a reader ?
     **/
    virtual bool isReader() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Is the node a writer ?
     **/
    virtual bool isWriter() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Basically returns true for WRITE_FFMPEG
     **/
    virtual bool isVideoWriter() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Is this node an output node ? An output node means
     * that a RenderEngine can be created.
     * If returning true, you may subclass createRenderEngine() to 
     * create a custom render engine if needed. The basic render engine
     * handles writers & disk cache.
     **/
    virtual bool isOutput() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Return the render engine created by createRenderEngine
     **/
    RenderEnginePtr getRenderEngine() const;

    /**
     * @brief To be implemented if this effect has the isOutput() function retuning true.
     * Creates the engine that will control the output rendering.
     * The default render engine just renders the requested tree and doesn't do
     * extra-stuff.
     **/
    virtual RenderEngine* createRenderEngine();



    virtual bool isPaintingOverItselfEnabled() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns true if the node is capable of generating
     * data and process data on the input as well
     **/
    virtual bool isGeneratorAndFilter() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief How many input can we have at most. (i.e: how many input arrows)
     * This function should be MT-safe and should never change the value returned.
     **/
    virtual int getMaxInputCount() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Is inputNb optional ? In which case the render can be made without it.
     **/
    virtual bool isInputOptional(int inputNb) const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Is inputNb a mask ? In which case the effect will have an additionnal mask parameter.
     **/
    virtual bool isInputMask(int /*inputNb*/) const WARN_UNUSED_RETURN
    {
        return false;
    };


    virtual bool getMakeSettingsPanel() const { return true; }


    virtual bool getCreateChannelSelectorKnob() const;

    /**
     * @brief Returns whether masking is enabled or not
     **/
    bool isMaskEnabled(int inputNb) const;

    /**
     * @brief Routine called after the creation of an effect. This function must
     * fill for the given input what number of image components is supported by the plug-in.
     * Each bit of the bitset indicate whether image of N components are supported.
     *
     * This function is also called to specify what image components this effect can output.
     * In that case inputNb equals -1.
     **/
    virtual void addAcceptedComponents(int inputNb, std::bitset<4> * comps) = 0;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const = 0;


    /**
     * @brief Can be derived to give a more meaningful label to the input 'inputNb'
     **/
    virtual std::string getInputLabel(int inputNb) const WARN_UNUSED_RETURN;

    /**
     * @brief Return a string indicating the purpose of the given input. It is used for the user documentation.
     **/
    virtual std::string getInputHint(int inputNb) const WARN_UNUSED_RETURN;


    /**
     * @bried Returns the effect render order preferences:
     * eSequentialPreferenceNotSequential: The effect does not need to be run in a sequential order
     * eSequentialPreferenceOnlySequential: The effect can only be run in a sequential order (i.e like the background render would do)
     * eSequentialPreferencePreferSequential: This indicates that the effect would work better by rendering sequential. This is merely
     * a hint to Natron but for now we just consider it as eSequentialPreferenceNotSequential.
     **/
    virtual SequentialPreferenceEnum getSequentialPreference() const
    {
        return eSequentialPreferenceNotSequential;
    }


    struct RenderRoIResults
    {
        // For each component requested, the image plane rendered
        std::map<ImageComponents, ImagePtr> outputPlanes;

        // If this effect can apply distorsion, this is the stack of distorsions upstream to apply
        Distorsion2DStackPtr distorsionStack;
    };

    /**
     * @brief Renders the image planes at the given time,scale and for the given view & render window.
     * This returns a list of all planes requested in the args.
     * @param args See the definition of the class for comments on each argument.
     * The return code indicates whether the render succeeded or failed. Note that this function may succeed
     * and return 0 plane if the RoI does not intersect the RoD of the effect.
     **/
    ActionRetCodeEnum renderRoI(const RenderRoIArgs & args, RenderRoIResults* results) WARN_UNUSED_RETURN;

    
    struct RectToRender
    {
        int identityInputNumber;
        RectI rect;
        TimeValue identityTime;
        ViewIdx identityView;
    };

    struct ImagePlanesToRender
    {
        // The list of rectangles to pass to the render action.
        // identity rectangles will not have render called but directly
        // copy the identity input image
        std::list<RectToRender> rectsToRender;

        // The planes to render. Fully cached planes are not in this list
        std::map<ImageComponents, PlaneToRender> planes;

        // The render device (CPU, OpenGL...)
        RenderBackendTypeEnum backendType;

        // For OpenGL this is the effect context dependent data
        EffectOpenGLContextDataPtr glContextData;


        ImagePlanesToRender()
        : rectsToRender()
        , planes()
        , backendType(eRenderBackendTypeCPU)
        , glContextData()
        {
        }
    };

    typedef boost::shared_ptr<ImagePlanesToRender> ImagePlanesToRenderPtr;
    
private:

    void optimizeRectsToRender(const TreeRenderNodeArgsPtr& renderArgs,
                               const RectI & inputsRoDIntersection,
                               const std::list<RectI> & rectsToRender,
                               const TimeValue time,
                               const ViewIdx view,
                               const RenderScale & renderMappedScale,
                               std::list<EffectInstance::RectToRender>* finalRectsToRender);
    
    
public:
    


    class NotifyRenderingStarted_RAII
    {
        Node* _node;
        bool _didEmit;
        bool _didGroupEmit;

public:

        NotifyRenderingStarted_RAII(Node* node);

        ~NotifyRenderingStarted_RAII();
    };

    class NotifyInputNRenderingStarted_RAII
    {
        Node* _node;
        int _inputNumber;
        bool _didEmit;

public:

        NotifyInputNRenderingStarted_RAII(Node* node,
                                          int inputNumber);

        ~NotifyInputNRenderingStarted_RAII();
    };


public:

    virtual bool getActionsRecursionLevel() const OVERRIDE FINAL;


    EffectInstancePtr resolveInputEffectForFrameNeeded(const int inputNb, int* channelForMask);

public:

    /**
     * @brief Returns the node output format
     **/
    RectI getOutputFormat(const TreeRenderNodeArgsPtr& render);

    /**
     * @brief Returns whether the effect is frame-varying (i.e: a Reader with different images in the sequence)
     **/
    bool isFrameVarying(const TreeRenderNodeArgsPtr& render);

    /**
     * @brief Returns the preferred output frame rate to render with
     **/
    double getFrameRate(const TreeRenderNodeArgsPtr& render);

    /**
     * @brief Returns the preferred premultiplication flag for the output image
     **/
    ImagePremultiplicationEnum getPremult(const TreeRenderNodeArgsPtr& render);

    /**
     * @brief If true, the plug-in knows how to render frames at non integer times. If false
     * this is the hint indicating that the plug-ins can only render integer frame times (such as a Reader)
     **/
    bool canRenderContinuously(const TreeRenderNodeArgsPtr& render);

    /**
     * @brief Returns the field ordering of images produced by this plug-in
     **/
    ImageFieldingOrderEnum getFieldingOrder(const TreeRenderNodeArgsPtr& render);

    /**
     * @brief Returns the pixel aspect ratio, depth and components for the given input.
     * If inputNb equals -1 then this function will check the output components.
     **/
    double getAspectRatio(const TreeRenderNodeArgsPtr& render, int inputNb);

    /**
     * @brief Returns the expected color plane components by this node on the given input.
     * If inputNb is -1, it returns the expected components for the color plane in output.
     * For multi-planar effects, they must support all kind of components.
     **/
    ImageComponents getColorPlaneComponents(const TreeRenderNodeArgsPtr& render, int inputNb);

    ImageBitDepthEnum getBitDepth(const TreeRenderNodeArgsPtr& render,int inputNb);


    /**
     * @brief Return true if this effect can return a distorsion 2D. In this case
     * you have to implement getDistorsion.
     **/
    virtual bool getCanDistort() const
    {
        return false;
    }

    /**
     * @brief Return true if the given input should also attempt to return a distorsion function
     * along with the image when possible
     **/
    virtual bool getInputCanReceiveDistorsion(int /*inputNb*/) const
    {
        return false;
    }


    virtual bool isMultiPlanar() const
    {
        return false;
    }

    enum PassThroughEnum
    {
        ePassThroughBlockNonRenderedPlanes,
        ePassThroughPassThroughNonRenderedPlanes,
        ePassThroughRenderAllRequestedPlanes
    };

    virtual EffectInstance::PassThroughEnum isPassThroughForNonRenderedPlanes() const
    {
        return ePassThroughPassThroughNonRenderedPlanes;
    }

    virtual bool isViewAware() const
    {
        return false;
    }

    enum ViewInvarianceLevel
    {
        eViewInvarianceAllViewsVariant,
        eViewInvarianceOnlyPassThroughPlanesVariant,
        eViewInvarianceAllViewsInvariant,
    };

    virtual ViewInvarianceLevel isViewInvariant() const
    {
        return eViewInvarianceAllViewsVariant;
    }

public:




    /**
     * @brief Indicates how many simultaneous renders the plugin can deal with.
     * By default it returns the plug-in safety.
     * RenderSafetyEnum::eRenderSafetyUnsafe - indicating that only a single 'render' call can be made at any time amoung all instances,
     * RenderSafetyEnum::eRenderSafetyInstanceSafe - indicating that any instance can have a single 'render' call at any one time,
     * RenderSafetyEnum::eRenderSafetyFullySafe - indicating that any instance of a plugin can have multiple renders running simultaneously
     * RenderSafetyEnum::eRenderSafetyFullySafeFrame - Same as eRenderSafetyFullySafe but the plug-in also flagged  kOfxImageEffectPluginPropHostFrameThreading to true.
     **/
    virtual RenderSafetyEnum getCurrentRenderThreadSafety() const WARN_UNUSED_RETURN;

    virtual PluginOpenGLRenderSupport getCurrentOpenGLSupport() const WARN_UNUSED_RETURN;



    virtual bool shouldCacheOutput(bool isFrameVaryingOrAnimated, const TreeRenderNodeArgsPtr& render, int visitsCount) const;




public:


    /**
     * @brief Override to initialize the overlay interact. It is called only on the
     * live instance.
     **/
    virtual void initializeOverlayInteract()
    {
    }

    virtual void setCurrentViewportForOverlays(OverlaySupport* /*viewport*/)
    {
    }

public:


    /**
     * @brief Overload this and return true if your operator should dislay a preview image by default.
     **/
    virtual bool makePreviewByDefault() const WARN_UNUSED_RETURN
    {
        return isReader();
    }


    virtual void clearLastRenderedImage();


    /**
     * @brief Use this function to post a transient message to the user. It will be displayed using
     * a dialog. The message can be of 4 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * eMessageTypeQuestion : you want to ask the user about something.
     * The function will return true always except for a message of type eMessageTypeQuestion, in which
     * case the function may return false if the user pressed the 'No' button.
     * @param content The message you want to pass.
     **/
    bool message(MessageTypeEnum type, const std::string & content) const;

    /**
     * @brief Use this function to post a persistent message to the user. It will be displayed on the
     * node's graphical interface and on any connected viewer. The message can be of 3 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * @param content The message you want to pass.
     **/
    void setPersistentMessage(MessageTypeEnum type, const std::string & content);

    /**
     * @brief Test if a persistent message is set.
     **/
    bool hasPersistentMessage();

    /**
     * @brief Clears any message posted previously by setPersistentMessage.
     **/
    void clearPersistentMessage(bool recurse);

    /**
     * @brief Must return the preferred layout of images that are received with getImage
     **/
    virtual ImageBufferLayoutEnum getPreferredBufferLayout() const
    {
        return eImageBufferLayoutRGBAPackedFullRect;
    }

    /**
     * @brief Does this effect supports tiling ?
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
     * If a clip or plugin does not support tiled images, then the host should supply
     * full RoD images to the effect whenever it fetches one.
     **/
    virtual bool supportsTiles() const
    {
        return false;
    }

    /**
     * @brief Does this effect supports multiresolution ?
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
     * Multiple resolution images mean...
     * input and output images can be of any size
     * input and output images can be offset from the origin
     **/
    virtual bool supportsMultiResolution() const
    {
        return true;
    }

    /**
     * @brief Hint the cache whether this effect can make draft mode renders or not
     **/
    virtual bool supportsRenderQuality() const { return false; }

    /**
     * @brief Does this effect can support multiple clips PAR ?
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultipleClipPARs
     * If a plugin does not accept clips of differing PARs, then the host must resample all images fed to that effect to agree with the output's PAR.
     * If a plugin does accept clips of differing PARs, it will need to specify the output clip's PAR in the kOfxImageEffectActionGetClipPreferences action.
     **/
    virtual bool supportsMultipleClipPARs() const
    {
        return false;
    }

    virtual bool supportsMultipleClipDepths() const
    {
        return false;
    }

    virtual bool supportsMultipleClipFPSs() const
    {
        return false;
    }

    virtual bool doesTemporalClipAccess() const
    {
        return false;
    }

    virtual bool canCPUImplementationSupportOSMesa() const
    {
        return false;
    }

    virtual void onEnableOpenGLKnobValueChanged(bool /*activated*/)
    {

    }

    /**
     * @brief If this effect is a writer then the file path corresponding to the output images path will be fed
     * with the content of pattern.
     **/
    void setOutputFilesForWriter(const std::string & pattern);

    /**
     * @brief Constructs a new memory holder, with nBytes allocated. If the allocation failed, bad_alloc is thrown
     **/
    PluginMemoryPtr newMemoryInstance(size_t nBytes) WARN_UNUSED_RETURN;





    /**
     * @brief Callback called to fetch data from the plug-in before creating an instance.
     **/
    virtual void describePlugin()
    {
    }



#ifdef DEBUG
    void checkCanSetValueAndWarn() const;
#endif




public:

    /**
     * @brief Push a new undo command to the undo/redo stack associated to this node.
     * The stack takes ownership of the shared pointer, so you should not hold a strong reference to the passed pointer.
     * If no undo/redo stack is present, the command will just be redone once then destroyed.
     **/
    void pushUndoCommand(const UndoCommandPtr& command);

    // Same as the version above, do NOT derefence command after this call as it will be destroyed already, usually this call is
    // made as such: pushUndoCommand(new MyCommand(...))
    void pushUndoCommand(UndoCommand* command);

    /**
     * @brief Set the cursor to be one of the default cursor.
     * @returns True if it successfully set the cursor, false otherwise.
     * Note: this can only be called during an overlay interact action.
     **/
    bool setCurrentCursor(CursorEnum defaultCursor);
    bool setCurrentCursor(const QString& customCursorFilePath);

    ///Doesn't do anything, instead we overriden onKnobValueChanged_public
    virtual bool onKnobValueChanged(const KnobIPtr& k,
                                    ValueChangedReasonEnum reason,
                                    TimeValue time,
                                    ViewSetSpec view) OVERRIDE FINAL;


    virtual bool canHandleRenderScaleForOverlays() const { return true; }



    void setInteractColourPicker_public(const OfxRGBAColourD& color, bool setColor, bool hasColor);

    virtual bool isDoingInteractAction() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onInteractViewportSelectionCleared() {}

    virtual void onInteractViewportSelectionUpdated(const RectD& /*rectangle*/,
                                                    bool /*onRelease*/) {}


    /* @brief Overlay support:
     * Just overload this function in your operator.
     * No need to include any OpenGL related header.
     * The coordinate space is  defined by the displayWindow
     * (i.e: (0,0) = bottomLeft and  width() and height() being
     * respectivly the width and height of the frame.)
     */
    virtual bool hasOverlay() const
    {
        return false;
    }


    /**
     * @brief Reimplement to control how the host adds the RGBA checkboxes.
     * @returns True if you want the host to add the RGBA checkboxes, false otherwise.
     **/
    virtual bool isHostChannelSelectorSupported(bool* defaultR,
                                                bool* defaultG,
                                                bool* defaultB,
                                                bool* defaultA) const
    {
        *defaultR = true;
        *defaultG = true;
        *defaultB = true;
        *defaultA = true;

        return true;
    }

    /**
     * @brief Reimplement to activate host masking
     * Note that in this case this is expected that getMaxInputCount returns the number of inputs *with* the mask.
     * The function getInputLabel should also return the appropriate label for the mask.
     * The function isInputMask should also return true for this mask index.
     * The mask will be the last input, i.e its index will be getMaxInputCount() - 1.
     **/
    virtual bool isHostMaskingEnabled() const
    {
        return false;
    }

    /**
     * @brief Reimplement to activate host mixing
     **/
    virtual bool isHostMixingEnabled() const
    {
        return false;
    }

    virtual void onKnobsAboutToBeLoaded(const SERIALIZATION_NAMESPACE::NodeSerialization& /*serialization*/) {}

    virtual void onKnobsLoaded() {}

    /**
     * @brief Called after all knobs have been loaded and the node has been created
     **/
    virtual void onEffectCreated(const CreateNodeArgs& /*args*/) {}



    /**
     * @brief Must return whether the plug-in handles concurrent OpenGL renders or not.
     * By default the OpenFX OpenGL render suite cannot support concurrent OpenGL renders, but version 2 specified by natron allows to do so.
     **/
    virtual bool supportsConcurrentOpenGLRenders() const { return true; }


    void clearRenderInstances();


    /**
     * @brief Called when something that should force a new evaluation (render) is done.
     * @param isSignificant If false the viewers will only be redrawn and nothing will be rendered.
     * @param refreshMetadatas If true the meta-datas on this node and all nodes downstream recursively will be re-computed.
     **/
    virtual void evaluate(bool isSignificant, bool refreshMetadatas) OVERRIDE;


    bool ifInfiniteclipRectToProjectDefault(RectD* rod) const;


protected:


    /**
     * @brief Plug-ins that are flagged eRenderSafetyInstanceSafe or lower can implement this function to make a copy
     * of the effect that will be used to render. The copy should be as fast as possible, meaning any clip or parameter should
     * share pointers (since internally these classes are thread-safe) and ensure that any private data member is copied.
     **/
    virtual EffectInstancePtr createRenderClone() { return EffectInstancePtr(); }


private:

    EffectInstancePtr getOrCreateRenderInstance();


    void releaseRenderInstance(const EffectInstancePtr& instance);



protected:

    virtual void refreshExtraStateAfterTimeChanged(bool isPlayback, TimeValue time)  OVERRIDE;

    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() OVERRIDE
    {
    };



    virtual void setInteractColourPicker(const OfxRGBAColourD& /*color*/, bool /*setColor*/, bool /*hasColor*/)
    {

    }

    virtual bool shouldPreferPluginOverlayOverHostOverlay() const
    {
        return true;
    }

    virtual bool shouldDrawHostOverlay() const
    {
        return true;
    }


    NodeWPtr _node; //< the node holding this effect

private:

    class Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details

    friend class ReadNode;
    friend class WriteNode;




    /**
     * @brief Launch the render action for a given render clone
     **/
    ActionRetCodeEnum renderForClone(const OSGLContextAttacherPtr& glContext,
                                     const RenderRoIArgs& args,
                                     const RenderScale& renderMappedScale,
                                     const ImagePlanesToRenderPtr & planes,
                                     const std::bitset<4> processChannels,
                                     const std::map<int, std::list<ImageComponents> >& neededInputLayers);



private:

    /**
     * @brief Returns the index of the input if inputEffect is a valid input connected to this effect, otherwise returns -1.
     **/
    int getInputNumber(const EffectInstancePtr& inputEffect) const;
};



/**
 * @typedef Any plug-in should have a static function called BuildEffect with the following signature.
 * It is used to build a new instance of an effect. Basically it should just call the constructor.
 **/
typedef EffectInstancePtr (*EffectBuilder)(const NodePtr&);

inline EffectInstancePtr
toEffectInstance(const KnobHolderPtr& effect)
{
    return boost::dynamic_pointer_cast<EffectInstance>(effect);
}


NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_EFFECTINSTANCE_H
