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
#include "Engine/ImageLocker.h"
#include "Engine/Knob.h" // for KnobHolder
#include "Engine/RectD.h"
#include "Engine/RectI.h"
#include "Engine/RenderStats.h"
#include "Engine/EngineFwd.h"
#include "Engine/ParallelRenderArgs.h"

// Various useful plugin IDs, @see EffectInstance::getPluginID()
#define PLUGINID_OFX_MERGE        "net.sf.openfx.MergePlugin"
#define PLUGINID_OFX_TRACKERPM    "net.sf.openfx.TrackerPM"
#define PLUGINID_OFX_DOTEXAMPLE   "net.sf.openfx.dotexample"
#define PLUGINID_OFX_READOIIO     "fr.inria.openfx.ReadOIIO"
#define PLUGINID_OFX_WRITEOIIO    "fr.inria.openfx.WriteOIIO"
#define PLUGINID_OFX_ROTO         "net.sf.openfx.RotoPlugin"
#define CLIP_OFX_ROTO             "Roto" // The Roto input clip from the Roto plugin
#define PLUGINID_OFX_TRANSFORM    "net.sf.openfx.TransformPlugin"
#define PLUGINID_OFX_GRADE        "net.sf.openfx.GradePlugin"
#define PLUGINID_OFX_COLORCORRECT "net.sf.openfx.ColorCorrectPlugin"
#define PLUGINID_OFX_BLURCIMG     "net.sf.cimg.CImgBlur"
#define PLUGINID_OFX_CORNERPIN    "net.sf.openfx.CornerPinPlugin"
#define PLUGINID_OFX_CONSTANT    "net.sf.openfx.ConstantPlugin"
#define PLUGINID_OFX_TIMEOFFSET "net.sf.openfx.timeOffset"
#define PLUGINID_OFX_FRAMEHOLD    "net.sf.openfx.FrameHold"
#define PLUGINID_OFX_RETIME "net.sf.openfx.Retime"
#define PLUGINID_OFX_FRAMERANGE "net.sf.openfx.FrameRange"
#define PLUGINID_OFX_RUNSCRIPT    "fr.inria.openfx.RunScript"
#define PLUGINID_OFX_READFFMPEG     "fr.inria.openfx.ReadFFmpeg"
#define PLUGINID_OFX_SHUFFLE     "net.sf.openfx.ShufflePlugin"
#define PLUGINID_OFX_WRITEFFMPEG     "fr.inria.openfx.WriteFFmpeg"
#define PLUGINID_OFX_READPFM     "fr.inria.openfx.ReadPFM"
#define PLUGINID_OFX_WRITEPFM     "fr.inria.openfx.WritePFM"

#define PLUGINID_NATRON_VIEWER    (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Viewer")
#define PLUGINID_NATRON_DISKCACHE (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.DiskCache")
#define PLUGINID_NATRON_DOT       (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Dot")
#define PLUGINID_NATRON_READQT    (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.ReadQt")
#define PLUGINID_NATRON_WRITEQT   (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.WriteQt")
#define PLUGINID_NATRON_GROUP     (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Group")
#define PLUGINID_NATRON_INPUT     (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Input")
#define PLUGINID_NATRON_OUTPUT    (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Output")
#define PLUGINID_NATRON_BACKDROP  (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.BackDrop")
#define PLUGINID_NATRON_ROTOPAINT (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.RotoPaint")
#define PLUGINID_NATRON_ROTO (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Roto")
#define PLUGINID_NATRON_ROTOSMEAR (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.RotoSmear")
#define PLUGINID_NATRON_PRECOMP     (NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.Precomp")

#define kNatronTLSEffectPointerProperty "NatronTLSEffectPointerProperty"

class QThread;

namespace Natron {

/**
 * @brief This is the base class for visual effects.
 * A live instance is always living throughout the lifetime of a Node and other copies are
 * created on demand when a render is needed.
 **/
class EffectInstance
    : public NamedKnobHolder
      , public LockManagerI<Natron::Image>
      , public boost::enable_shared_from_this<Natron::EffectInstance>
{
public:


    typedef std::map<Natron::ImageComponents, boost::weak_ptr<Natron::Node> > ComponentsAvailableMap;
    typedef std::list<std::pair<Natron::ImageComponents, boost::weak_ptr<Natron::Node> > > ComponentsAvailableList;
    typedef std::map<int, std::list< boost::shared_ptr<Natron::Image> > > InputImagesMap;
    typedef std::map<int, std::vector<Natron::ImageComponents> > ComponentsNeededMap;
    
    struct RenderRoIArgs
    {
        // Developper note: the fields were reordered to optimize packing.
        // see http://www.catb.org/esr/structure-packing/
        
        double time; //< the time at which to render
        RenderScale scale; //< the scale at which to render
        unsigned int mipMapLevel; //< the mipmap level (redundant with the scale, stored here to avoid refetching it everytimes)
        int view; //< the view to render
        RectI roi; //< the renderWindow (in pixel coordinates) , watch out OpenFX action getRegionsOfInterest expects canonical coords!
        RectD preComputedRoD; //<  pre-computed region of definition in canonical coordinates for this effect to speed-up the call to renderRoi
        std::list<Natron::ImageComponents> components; //< the requested image components (per plane)

        ///When called from getImage() the calling node  will have already computed input images, hence the image of this node
        ///might already be in this list
        EffectInstance::InputImagesMap inputImagesList;
        const EffectInstance* caller;
        Natron::ImageBitDepthEnum bitdepth; //< the requested bit depth
        bool byPassCache;
        bool calledFromGetImage;

        RenderRoIArgs()
        : time(0)
        , scale(1.)
        , mipMapLevel(0)
        , view(0)
        , roi()
        , preComputedRoD()
        , components()
        , inputImagesList()
        , caller(0)
        , bitdepth(eImageBitDepthFloat)
        , byPassCache(false)
        , calledFromGetImage(false)
        {
        }
        
        RenderRoIArgs(double time_,
                      const RenderScale & scale_,
                      unsigned int mipMapLevel_,
                      int view_,
                      bool byPassCache_,
                      const RectI & roi_,
                      const RectD & preComputedRoD_,
                      const std::list<Natron::ImageComponents> & components_,
                      Natron::ImageBitDepthEnum bitdepth_,
                      bool calledFromGetImage,
                      const EffectInstance* caller,
                      const EffectInstance::InputImagesMap & inputImages = EffectInstance::InputImagesMap() )
        : time(time_)
        , scale(scale_)
        , mipMapLevel(mipMapLevel_)
        , view(view_)
        , roi(roi_)
        , preComputedRoD(preComputedRoD_)
        , components(components_)
        , inputImagesList(inputImages)
        , caller(caller)
        , bitdepth(bitdepth_)
        , byPassCache(byPassCache_)
        , calledFromGetImage(calledFromGetImage)
        {
        }
    };

    enum SupportsEnum
    {
        eSupportsMaybe = -1,
        eSupportsNo = 0,
        eSupportsYes = 1
    };

public:


    /**
     * @brief Constructor used once for each node created. Its purpose is to create the "live instance".
     * You shouldn't do any heavy processing here nor lengthy initialization as the constructor is often
     * called just to be able to call a few virtuals fonctions.
     * The constructor is always called by the main thread of the application.
     **/
    explicit EffectInstance(boost::shared_ptr<Natron::Node> node);

    virtual ~EffectInstance();


    virtual bool canHandleEvaluateOnChangeInOtherThread() const OVERRIDE
    {
        return true;
    }

    /**
     * @brief Returns true once the effect has been fully initialized and is ready to have its actions called apart from
     * the createInstanceAction
     **/
    virtual bool isEffectCreated() const
    {
        return true;
    }

    /**
     * @brief Returns a pointer to the node holding this effect.
     **/
    boost::shared_ptr<Natron::Node> getNode() const WARN_UNUSED_RETURN
    {
        return _node.lock();
    }


    /**
     * @brief Returns the "real" hash of the node synchronized with the gui state
     **/
    U64 getHash() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns the hash the node had at the start of renderRoI. This will return the same value
     * at any time during the same render call.
     * @returns This function returns true if case of success, false otherwise.
     **/
    U64 getRenderHash() const WARN_UNUSED_RETURN;

    U64 getKnobsAge() const WARN_UNUSED_RETURN;

    /**
     * @brief Set the knobs age of this node to be 'age'. Note that this can be called
     * for 2 reasons:
     * - loading a project
     * - If this node is a clone and the master node changed its hash.
     **/
    void setKnobsAge(U64 age);

    /**
     * @brief Forwarded to the node's name
     **/
    const std::string & getScriptName() const WARN_UNUSED_RETURN;
    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Forwarded to the node's render format
     **/
    void getRenderFormat(Format *f) const;

    /**
     * @brief Forwarded to the node's render views count
     **/
    int getRenderViewsCount() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns input n. It might be NULL if the input is not connected.
     * MT-Safe
     **/
    EffectInstance* getInput(int n) const WARN_UNUSED_RETURN;

    /**
     * @brief Forwarded to the node holding the effect
     **/
    bool hasOutputConnected() const WARN_UNUSED_RETURN;

    /**
     * @brief Must return the plugin's major version.
     **/
    virtual int getMajorVersion() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Must return the plugin's minor version.
     **/
    virtual int getMinorVersion() const WARN_UNUSED_RETURN = 0;

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
     * @brief Is this node an output node ? An output node means
     * it has no output.
     **/
    virtual bool isOutput() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Returns true if this node is a tracker
     **/
    virtual bool isTrackerNode() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Returns true if this node is a tracker
     **/
    virtual bool isRotoPaintNode() const WARN_UNUSED_RETURN
    {
        return false;
    }

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
     * @brief Is this node an OpenFX node?
     **/
    virtual bool isOpenFX() const WARN_UNUSED_RETURN
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

    /**
     * @brief Is the input a roto brush ?
     **/
    virtual bool isInputRotoBrush(int /*inputNb*/) const WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual int getRotoBrushInputIndex() const WARN_UNUSED_RETURN
    {
        return -1;
    }

    /**
     * @brief Returns the index of the channel to use to produce the mask and the components.
     * None = -1
     * R = 0
     * G = 1
     * B = 2
     * A = 3
     **/
    int getMaskChannel(int inputNb, Natron::ImageComponents* comps, boost::shared_ptr<Natron::Node>* maskInput) const;

    /**
     * @brief Returns whether masking is enabled or not
     **/
    bool isMaskEnabled(int inputNb) const;

    /**
     * @brief Routine called after the creation of an effect. This function must
     * fill for the given input what image components we can feed it with.
     * This function is also called to specify what image components this effect can output.
     * In that case inputNb equals -1.
     **/
    virtual void addAcceptedComponents(int inputNb, std::list<Natron::ImageComponents>* comps) = 0;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const = 0;

    /**
     * @brief Must return the deepest bit depth that this plug-in can support.
     * If 32 float is supported then return Natron::eImageBitDepthFloat, otherwise
     * return eImageBitDepthShort if 16 bits is supported, and as a last resort, return
     * eImageBitDepthByte. At least one must be returned.
     **/
    Natron::ImageBitDepthEnum getBestSupportedBitDepth() const;

    bool isSupportedBitDepth(Natron::ImageBitDepthEnum depth) const;

    /**
     * @brief Returns true if the given input supports the given components. If inputNb equals -1
     * then this function will check whether the effect can produce the given components.
     **/
    bool isSupportedComponent(int inputNb, const Natron::ImageComponents & comp) const;

    /**
     * @brief Returns the most appropriate components that can be supported by the inputNb.
     * If inputNb equals -1 then this function will check the output components.
     **/
    Natron::ImageComponents findClosestSupportedComponents(int inputNb, const Natron::ImageComponents & comp) const WARN_UNUSED_RETURN;

    /**
     * @brief Can be derived to give a more meaningful label to the input 'inputNb'
     **/
    virtual std::string getInputLabel(int inputNb) const WARN_UNUSED_RETURN;

    /**
     * @brief Must be implemented to give the plugin internal id(i.e: net.sf.openfx:invertPlugin)
     **/
    virtual std::string getPluginID() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Must be implemented to give the plugin a label that will be used by the graphical
     * user interface.
     **/
    virtual std::string getPluginLabel() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief The grouping of the plug-in. For instance Views/Stereo/MyStuff
     * Each string being one level of the grouping. The first one being the name
     * of one group that will appear in the user interface.
     **/
    virtual void getPluginGrouping(std::list<std::string>* grouping) const = 0;

    /**
     * @brief Must be implemented to give a desription of the effect that this node does. This is typically
     * what you'll see displayed when the user clicks the '?' button on the node's panel in the user interface.
     **/
    virtual std::string getPluginDescription() const WARN_UNUSED_RETURN = 0;


    /**
     * @bried Returns the effect render order preferences:
     * eSequentialPreferenceNotSequential: The effect does not need to be run in a sequential order
     * eSequentialPreferenceOnlySequential: The effect can only be run in a sequential order (i.e like the background render would do)
     * eSequentialPreferencePreferSequential: This indicates that the effect would work better by rendering sequential. This is merely
     * a hint to Natron but for now we just consider it as eSequentialPreferenceNotSequential.
     **/
    virtual Natron::SequentialPreferenceEnum getSequentialPreference() const
    {
        return Natron::eSequentialPreferenceNotSequential;
    }

    enum RenderRoIRetCode
    {
        eRenderRoIRetCodeOk = 0,
        eRenderRoIRetCodeAborted,
        eRenderRoIRetCodeFailed
    };

    /**
     * @brief Renders the image planes at the given time,scale and for the given view & render window.
     * This returns a list of all planes requested in the args.
     * @param args See the definition of the class for comments on each argument.
     * The return code indicates whether the render succeeded or failed. Note that this function may succeed
     * and return 0 plane if the RoI does not intersect the RoD of the effect.
     **/
    RenderRoIRetCode renderRoI(const RenderRoIArgs & args, std::list<boost::shared_ptr<Image> >* outputPlanes) WARN_UNUSED_RETURN;


    void getImageFromCacheAndConvertIfNeeded(bool useCache,
                                             bool useDiskCache,
                                             const Natron::ImageKey & key,
                                             unsigned int mipMapLevel,
                                             const RectI* boundsParam,
                                             const RectD* rodParam,
                                             Natron::ImageBitDepthEnum bitdepth,
                                             const Natron::ImageComponents & components,
                                             Natron::ImageBitDepthEnum nodeBitDepthPref,
                                             const Natron::ImageComponents & nodeComponentsPref,
                                             const EffectInstance::InputImagesMap & inputImages,
                                             const boost::shared_ptr<RenderStats> & stats,
                                             boost::shared_ptr<Natron::Image>* image);


    /**
     * @brief This function is to be called by getImage() when the plug-ins renders more planes than the ones suggested
     * by the render action. We allocate those extra planes and cache them so they were not rendered for nothing.
     * Note that the plug-ins may call this only while in the render action, and there must be other planes to render.
     **/
    boost::shared_ptr<Natron::Image> allocateImagePlaneAndSetInThreadLocalStorage(const Natron::ImageComponents & plane);


    class NotifyRenderingStarted_RAII
    {
        Node* _node;
        bool _didEmit;

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

        NotifyInputNRenderingStarted_RAII(Node* node, int inputNumber);

        ~NotifyInputNRenderingStarted_RAII();
    };


    /**
     * @brief Sets render preferences for the rendering of a frame for the current thread.
     * This is thread local storage. This is NOT local to a call to renderRoI
     **/
    void setParallelRenderArgsTLS(double time,
                                  int view,
                                  bool isRenderUserInteraction,
                                  bool isSequential,
                                  bool canAbort,
                                  U64 nodeHash,
                                  U64 renderAge,
                                  const boost::shared_ptr<Natron::Node> & treeRoot,
                                  const boost::shared_ptr<NodeFrameRequest> & nodeRequest,
                                  int textureIndex,
                                  const TimeLine* timeline,
                                  bool isAnalysis,
                                  bool isDuringPaintStrokeCreation,
                                  const std::list<boost::shared_ptr<Natron::Node> > & rotoPaintNodes,
                                  Natron::RenderSafetyEnum currentThreadSafety,
                                  bool doNanHandling,
                                  bool draftMode,
                                  bool viewerProgressReportEnabled,
                                  const boost::shared_ptr<RenderStats> & stats);

    void setDuringPaintStrokeCreationThreadLocal(bool duringPaintStroke);

    void setParallelRenderArgsTLS(const ParallelRenderArgs & args);

    /**
     *@returns whether the effect was flagged with canSetValue = true or false
     **/
    void invalidateParallelRenderArgsTLS();

    const ParallelRenderArgs* getParallelRenderArgsTLS() const;

    //Implem in ParallelRenderArgs.cpp
    static Natron::StatusEnum getInputsRoIsFunctor(bool useTransforms,
                                                   double time,
                                                   int view,
                                                   unsigned originalMipMapLevel,
                                                   const boost::shared_ptr<Natron::Node> & node,
                                                   const boost::shared_ptr<Natron::Node> & treeRoot,
                                                   const RectD & canonicalRenderWindow,
                                                   FrameRequestMap & requests);

    /**
     * @brief Visit recursively the compositing tree and computes required informations about region of interests for each node and
     * for each frame/view pair. This helps to call render a single time per frame/view pair for a node.
     * Implem is in ParallelRenderArgs.cpp
     **/
    static Natron::StatusEnum computeRequestPass(double time,
                                                 int view,
                                                 unsigned int mipMapLevel,
                                                 const RectD & renderWindow,
                                                 const boost::shared_ptr<Natron::Node> & treeRoot,
                                                 FrameRequestMap & request);

    // Implem is in ParallelRenderArgs.cpp
    static Natron::EffectInstance::RenderRoIRetCode treeRecurseFunctor(bool isRenderFunctor,
                                                                       const boost::shared_ptr<Natron::Node> & node,
                                                                       const FramesNeededMap & framesNeeded,
                                                                       const RoIMap & inputRois,
                                                                       const boost::shared_ptr<InputMatrixMap> & reroutesMap,
                                                                       bool useTransforms, // roi functor specific
                                                                       unsigned int originalMipMapLevel, // roi functor specific
                                                                       double time,
                                                                       int view,
                                                                       const boost::shared_ptr<Natron::Node> & treeRoot,
                                                                       FrameRequestMap* requests,  // roi functor specific
                                                                       EffectInstance::InputImagesMap* inputImages, // render functor specific
                                                                       const EffectInstance::ComponentsNeededMap* neededComps, // render functor specific
                                                                       bool useScaleOneInputs, // render functor specific
                                                                       bool byPassCache); // render functor specific


    /**
     * @brief If the current thread is rendering and this was started by the knobChanged (instanceChangedAction) function
     * then this will return true
     **/
    bool isCurrentRenderInAnalysis() const;

    /**
     * @breif Don't override this one, override onKnobValueChanged instead.
     **/
    virtual void onKnobValueChanged_public(KnobI* k, Natron::ValueChangedReasonEnum reason, double time, bool originatedFromMainThread) OVERRIDE FINAL;

    /**
     * @brief Returns a pointer to the first non disabled upstream node.
     * When cycling through the tree, we prefer non optional inputs and we span inputs
     * from last to first.
     * If this not is not disabled, it will return a pointer to this.
     **/
    Natron::EffectInstance* getNearestNonDisabled() const;

    /**
     * @brief Same as getNearestNonDisabled() except that it returns the *last* disabled node before the nearest non disabled node.
     * @param inputNb[out] The inputNb of the node that is non disabled.
     **/
    Natron::EffectInstance* getNearestNonDisabledPrevious(int* inputNb);

    /**
     * @brief Same as getNearestNonDisabled except that it looks for the nearest non identity node.
     * This function calls the action isIdentity and getRegionOfDefinition and can be expensive!
     **/
    Natron::EffectInstance* getNearestNonIdentity(double time);

    /**
     * @brief This is purely for the OfxEffectInstance derived class, but passed here for the sake of abstraction
     **/
    void refreshClipPreferences_public(double time,
                                        const RenderScale & scale,
                                        Natron::ValueChangedReasonEnum reason,
                                        bool forceGetClipPrefAction,
                                        bool recurse);

    virtual void onChannelsSelectorRefreshed() {}


    virtual bool refreshClipPreferences(double time,
                                        const RenderScale & scale,
                                        Natron::ValueChangedReasonEnum reason,
                                        bool forceGetClipPrefAction);

protected:

    void refreshClipPreferences_recursive(double time,
                                           const RenderScale & scale,
                                          Natron::ValueChangedReasonEnum reason,
                                           bool forceGetClipPrefAction,
                                           std::list<Natron::Node*> & markedNodes);



public:


    ///////////////////////Clip preferences related////////////////////////
    // These data are refreshed during a call to getClipPreferences to optimize the getters below
    // which would involve cycling through the tree
    struct DefaultClipPreferencesData;

    /**
     * @brief Returns the output aspect ratio to render with
     **/
    virtual double getPreferredAspectRatio() const;
    virtual double getPreferredFrameRate() const;

    /**
    * @brief Returns the preferred depth and components for the given input.
    * If inputNb equals -1 then this function will check the output components.
    **/
    virtual void getPreferredDepthAndComponents(int inputNb,
                                            std::list<Natron::ImageComponents>* comp,
                                            Natron::ImageBitDepthEnum* depth) const;


    /**
    * @brief Override to get the preffered premultiplication flag for the output image
    **/
    virtual Natron::ImagePremultiplicationEnum getOutputPremultiplication() const;
    ///////////////////////End Clip preferences related////////////////////////


    virtual void lock(const boost::shared_ptr<Natron::Image> & entry) OVERRIDE FINAL;
    virtual bool tryLock(const boost::shared_ptr<Natron::Image> & entry) OVERRIDE FINAL;
    virtual void unlock(const boost::shared_ptr<Natron::Image> & entry) OVERRIDE FINAL;
    virtual bool canSetValue() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void abortAnyEvaluation() OVERRIDE FINAL;
    virtual double getCurrentTime() const OVERRIDE WARN_UNUSED_RETURN;
    virtual int getCurrentView() const OVERRIDE WARN_UNUSED_RETURN;
    virtual bool getCanTransform() const
    {
        return false;
    }

    virtual RenderScale getOverlayInteractRenderScale() const;

    SequenceTime getFrameRenderArgsCurrentTime() const;

    int getFrameRenderArgsCurrentView() const;

    virtual bool getInputsHoldingTransform(std::list<int>* /*inputs*/) const
    {
        return false;
    }


    bool getThreadLocalRegionsOfInterests(RoIMap & roiMap) const;


    void getThreadLocalInputImages(InputImagesMap* images) const;

    void addThreadLocalInputImageTempPointer(int inputNb, const boost::shared_ptr<Natron::Image> & img);

    bool getThreadLocalRotoPaintTreeNodes(std::list<boost::shared_ptr<Natron::Node> >* nodes) const;

    /**
     * @brief Returns whether the effect is frame-varying (i.e: a Reader with different images in the sequence)
     **/
    virtual bool isFrameVarying() const
    {
        return false;
    }

    /**
     * @brief Returns whether the current node and/or the tree upstream is frame varying or animated.
     * It is frame varying/animated if at least one of the node is animated/varying
     **/
    bool isFrameVaryingOrAnimated_Recursive() const;
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

    struct RenderActionArgs
    {
        double time;
        RenderScale originalScale;
        RenderScale mappedScale;
        RectI roi;
        std::list<std::pair<Natron::ImageComponents, boost::shared_ptr<Natron::Image> > > outputPlanes;
        EffectInstance::InputImagesMap inputImages;
        int view;
        bool isSequentialRender;
        bool isRenderResponseToUserInteraction;
        bool byPassCache;
        bool draftMode;
        std::bitset<4> processChannels;
    };

protected:
    /**
     * @brief Must fill the image 'output' for the region of interest 'roi' at the given time and
     * at the given scale.
     * Pre-condition: render() has been called for all inputs so the portion of the image contained
     * in output corresponding to the roi is valid.
     * Note that this function can be called concurrently for the same output image but with different
     * rois, depending on the threading-affinity of the plug-in.
     **/
    virtual Natron::StatusEnum render(const RenderActionArgs & /*args*/) WARN_UNUSED_RETURN
    {
        return Natron::eStatusOK;
    }

    virtual Natron::StatusEnum getTransform(double /*time*/,
                                            const RenderScale & /*renderScale*/,
                                            int /*view*/,
                                            Natron::EffectInstance** /*inputToTransform*/,
                                            Transform::Matrix3x3* /*transform*/) WARN_UNUSED_RETURN
    {
        return Natron::eStatusReplyDefault;
    }

public:


    Natron::StatusEnum render_public(const RenderActionArgs & args) WARN_UNUSED_RETURN;
    Natron::StatusEnum getTransform_public(double time,
                                           const RenderScale & renderScale,
                                           int view,
                                           Natron::EffectInstance** inputToTransform,
                                           Transform::Matrix3x3* transform) WARN_UNUSED_RETURN;

protected:
/**
 * @brief Can be overloaded to indicates whether the effect is an identity, i.e it doesn't produce
 * any change in output.
 * @param time The time of interest
 * @param scale The scale of interest
 * @param rod The image region of definition, in canonical coordinates
 * @param view The view we 're interested in
 * @param inputTime[out] the input time to which this plugin is identity of
 * @param inputNb[out] the input number of the effect that is identity of.
 * The special value of -2 indicates that the plugin is identity of itself at another time
 **/
    virtual bool isIdentity(double /*time*/,
                            const RenderScale & /*scale*/,
                            const RectI & /*roi*/,
                            int /*view*/,
                            double* /*inputTime*/,
                            int* /*inputNb*/) WARN_UNUSED_RETURN
    {
        return false;
    }

public:

    bool isIdentity_public(bool useIdentityCache, // only set to true when calling for the whole image (not for a subrect)
                           U64 hash,
                           double time,
                           const RenderScale & scale,
                           const RectI & renderWindow,
                           int view,
                           double* inputTime,
                           int* inputNb) WARN_UNUSED_RETURN;

    /**
     * @brief Indicates how many simultaneous renders the plugin can deal with.
     * RenderSafetyEnum::eRenderSafetyUnsafe - indicating that only a single 'render' call can be made at any time amoung all instances,
     * RenderSafetyEnum::eRenderSafetyInstanceSafe - indicating that any instance can have a single 'render' call at any one time,
     * RenderSafetyEnum::eRenderSafetyFullySafe - indicating that any instance of a plugin can have multiple renders running simultaneously
     * RenderSafetyEnum::eRenderSafetyFullySafeFrame - Same as eRenderSafetyFullySafe but the plug-in also flagged  kOfxImageEffectPluginPropHostFrameThreading to true.
     **/
    virtual Natron::RenderSafetyEnum renderThreadSafety() const WARN_UNUSED_RETURN = 0;

    /*@brief The derived class should query this to abort any long process
       in the engine function.*/
    bool aborted() const WARN_UNUSED_RETURN;


    /** @brief Returns the image computed by the input 'inputNb' at the given time and scale for the given view.
     * @param dontUpscale If the image is retrieved is downscaled but the plug-in doesn't support the user of
     * downscaled images by default we upscale the image. If dontUpscale is true then we don't do this upscaling.
     *
     * @param roiPixel If non NULL will be set to the render window used to render the image, that is, either the
     * region of interest of this effect on the input effect we want to render or the optionalBounds if set, but
     * converted to pixel coordinates
     */
    boost::shared_ptr<Image> getImage(int inputNb,
                                      const double time,
                                      const RenderScale & scale,
                                      const int view,
                                      const RectD *optionalBounds, //!< optional region in canonical coordinates
                                      const Natron::ImageComponents & requestComps,
                                      const Natron::ImageBitDepthEnum depth,
                                      const double par,
                                      const bool dontUpscale,
                                      const bool mapImageToPreferredComps,
                                      RectI* roiPixel,
                                      boost::shared_ptr<Transform::Matrix3x3>* transform = 0) WARN_UNUSED_RETURN;
    virtual void aboutToRestoreDefaultValues() OVERRIDE FINAL;
    virtual bool shouldCacheOutput(bool isFrameVaryingOrAnimated, double time, int view) const;

    /**
     * @brief Can be derived to get the region that the plugin is capable of filling.
     * This is meaningful for plugins that generate images or transform images.
     * By default it returns in rod the union of all inputs RoD and eStatusReplyDefault is returned.
     * @param isProjectFormat[out] If set to true, then rod is taken to be equal to the current project format.
     * In case of failure the plugin should return eStatusFailed.
     * @returns eStatusOK, eStatusReplyDefault, or eStatusFailed. rod is set except if return value is eStatusOK or eStatusReplyDefault.
    **/
    virtual Natron::StatusEnum getRegionOfDefinition(U64 hash, double time, const RenderScale & scale, int view, RectD* rod) WARN_UNUSED_RETURN;

protected:



    virtual void calcDefaultRegionOfDefinition(U64 hash, double time, const RenderScale & scale, int view, RectD *rod);

    /**
     * @brief If the instance rod is infinite, returns the union of all connected inputs. If there's no input this returns the
     * project format.
     * @returns true if the rod is set to the project format.
     **/
    bool ifInfiniteApplyHeuristic(U64 hash,
                                  double time,
                                  const RenderScale & scale,
                                  int view,
                                  RectD* rod); //!< input/output

    /**
     * @brief Can be derived to indicate for each input node what is the region of interest
     * of the node at time 'time' and render scale 'scale' given a render window.
     * For exemple a blur plugin would specify what it needs
     * from inputs in order to do a blur taking into account the size of the blurring kernel.
     * By default, it returns renderWindow for each input.
     **/
    virtual void getRegionsOfInterest(double time,
                                      const RenderScale & scale,
                                      const RectD & outputRoD,   //!< the RoD of the effect, in canonical coordinates
                                      const RectD & renderWindow,   //!< the region to be rendered in the output image, in Canonical Coordinates
                                      int view,
                                      RoIMap* ret);

    /**
     * @brief Can be derived to indicate for each input node what is the frame range(s) (which can be discontinuous)
     * that this effects need in order to render the frame at the given time.
     **/
    virtual FramesNeededMap getFramesNeeded(double time, int view) WARN_UNUSED_RETURN;


    /**
     * @brief Can be derived to get the frame range wherein the plugin is capable of producing frames.
     * By default it merges the frame range of the inputs.
     * In case of failure the plugin should return eStatusFailed.
     **/
    virtual void getFrameRange(double *first, double *last);

public:

    Natron::StatusEnum getRegionOfDefinition_public(U64 hash,
                                                    double time,
                                                    const RenderScale & scale,
                                                    int view,
                                                    RectD* rod,
                                                    bool* isProjectFormat) WARN_UNUSED_RETURN;
    Natron::StatusEnum getRegionOfDefinitionWithIdentityCheck_public(U64 hash,
                                                                     double time,
                                                                     const RenderScale & scale,
                                                                     const RectI & renderWindow,
                                                                     int view,
                                                                     RectD* rod,
                                                                     bool* isProjectFormat) WARN_UNUSED_RETURN;

private:

    Natron::StatusEnum getRegionOfDefinition_publicInternal(U64 hash,
                                                            double time,
                                                            const RenderScale & scale,
                                                            const RectI & renderWindow,
                                                            bool useRenderWindow,
                                                            int view,
                                                            RectD* rod,
                                                            bool* isProjectFormat) WARN_UNUSED_RETURN;

public:


    void getRegionsOfInterest_public(double time,
                                     const RenderScale & scale,
                                     const RectD & outputRoD,
                                     const RectD & renderWindow,   //!< the region to be rendered in the output image, in Canonical Coordinates
                                     int view,
                                     RoIMap* ret);

    FramesNeededMap getFramesNeeded_public(U64 hash, double time, int view, unsigned int mipMapLevel) WARN_UNUSED_RETURN;

    void getFrameRange_public(U64 hash, double *first, double *last, bool bypasscache = false);

    /**
     * @brief Override to initialize the overlay interact. It is called only on the
     * live instance.
     **/
    virtual void initializeOverlayInteract()
    {
    }

    void setCurrentViewportForOverlays_public(OverlaySupport* viewport);

protected:

    virtual void setCurrentViewportForOverlays(OverlaySupport* /*viewport*/)
    {
    }

public:


    /**
     * @brief Overload this and return true if your operator should dislay a preview image by default.
     **/
    virtual bool makePreviewByDefault() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Called on generator effects upon creation if they have an image input file field.
     **/
    void openImageFileKnob();


    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(Natron::ValueChangedReasonEnum /*reason*/) OVERRIDE
    {
    }

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(Natron::ValueChangedReasonEnum /*reason*/) OVERRIDE
    {
    }

    /**
     * @brief Can be overloaded to clear any cache the plugin might be
     * handling on his side.
     **/
    virtual void purgeCaches()
    {
    };
    virtual void clearLastRenderedImage();

    void clearActionsCache();

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
    bool message(Natron::MessageTypeEnum type, const std::string & content) const;

    /**
     * @brief Use this function to post a persistent message to the user. It will be displayed on the
     * node's graphical interface and on any connected viewer. The message can be of 3 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * @param content The message you want to pass.
     **/
    void setPersistentMessage(Natron::MessageTypeEnum type, const std::string & content);

    /**
     * @brief Test if a persistent message is set.
     **/
    bool hasPersistentMessage();

    /**
     * @brief Clears any message posted previously by setPersistentMessage.
     **/
    void clearPersistentMessage(bool recurse);

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
     * @brief Does this effect supports rendering at a different scale than 1 ?
     * There is no OFX property for this purpose. The only solution found for OFX is that if a render
     * or isIdentity with renderscale != 1 fails, the host retries with renderscale = 1 (and upscaled images).
     * If the renderScale support was not set, this throws an exception.
     **/
    bool supportsRenderScale() const;

    SupportsEnum supportsRenderScaleMaybe() const;

    /// should be set during effect initialization, but may also be set by the first getRegionOfDefinition with scale != 1 that succeeds
    void setSupportsRenderScaleMaybe(EffectInstance::SupportsEnum s) const;

    virtual bool supportsRenderQuality() const { return false; }

    /**
     * @brief Does this effect can support multiple clips PAR ?
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultipleClipPARs
     * If a plugin does not accept clips of differing PARs, then the host must resample all images fed to that effect to agree with the output's PAR.
     * If a plugin does accept clips of differing PARs, it will need to specify the output clip's PAR in the kOfxImageEffectActionGetClipPreferences action.
     **/
    virtual bool supportsMultipleClipsPAR() const
    {
        return false;
    }

    virtual bool doesTemporalClipAccess() const
    {
        return false;
    }

    virtual Natron::PluginOpenGLRenderSupport supportsOpenGLRender() const
    {
        return Natron::ePluginOpenGLRenderSupportNone;
    }

    /**
     * @brief If this effect is a writer then the file path corresponding to the output images path will be fed
     * with the content of pattern.
     **/
    void setOutputFilesForWriter(const std::string & pattern);

    /**
     * @brief Constructs a new memory holder, with nBytes allocated. If the allocation failed, bad_alloc is thrown
     **/
    PluginMemory* newMemoryInstance(size_t nBytes) WARN_UNUSED_RETURN;

    /// used to count the memory used by a plugin
    /// Don't call these, they're called by PluginMemory automatically
    void registerPluginMemory(size_t nBytes);
    void unregisterPluginMemory(size_t nBytes);

    void addPluginMemoryPointer(PluginMemory* mem);
    void removePluginMemoryPointer(PluginMemory* mem);

    void clearPluginMemoryChunks();

    /**
     * @brief Called right away when the user first opens the settings panel of the node.
     * This is called after each params had its default value set.
     **/
    virtual void beginEditKnobs()
    {
    }

    virtual std::vector<std::string> supportedFileFormats() const
    {
        return std::vector<std::string>();
    }

    /**
     * @brief Called everytimes an input connection is changed
     **/
    virtual void onInputChanged(int inputNo);

    /**
     * @brief If the plug-in calls timelineGoTo and we're during a render/instance changed action,
     * then all the knobs will retrieve the current time as being the one in the last render args thread-storage.
     * This function is here to update the last render args thread storage.
     **/
    void updateThreadLocalRenderTime(double time);

    bool isDuringPaintStrokeCreationThreadLocal() const;

    struct PlaneToRender
    {
        //Points to the fullscale image if render scale is not supported by the plug-in, or downscaleImage otherwise
        boost::shared_ptr<Natron::Image> fullscaleImage;
        
        //Points to the image to be rendered
        boost::shared_ptr<Natron::Image> downscaleImage;
        
        //Points to the image that the plug-in can render (either fullScale or downscale)
        boost::shared_ptr<Natron::Image> renderMappedImage;
        
        //Points to a temporary image that the plug-in will render
        boost::shared_ptr<Natron::Image> tmpImage;
        
        /*
         In the event where the fullScaleImage is in the cache but we must resize it to render a portion unallocated yet and
         if the render is issues directly from getImage() we swap image in cache instead of taking the write lock of fullScaleImage
         */
        boost::shared_ptr<Natron::Image> cacheSwapImage;
        
        void* originalCachedImage;
        
        /**
         * This is set to true if this plane is allocated with allocateImagePlaneAndSetInThreadLocalStorage()
         **/
        bool isAllocatedOnTheFly;
        
        PlaneToRender()
        : fullscaleImage()
        , downscaleImage()
        , renderMappedImage()
        , tmpImage()
        , cacheSwapImage()
        , originalCachedImage(0)
        , isAllocatedOnTheFly(false)
        {
        }
    };

    struct RectToRender
    {
        Natron::EffectInstance* identityInput;
        RectI rect;
        RoIMap inputRois;
        EffectInstance::InputImagesMap imgs;
        double identityTime;
        bool isIdentity;
    };

    struct ImagePlanesToRender
    {
        std::list<RectToRender> rectsToRender;
        std::map<Natron::ImageComponents, PlaneToRender> planes;
        std::map<int, Natron::ImagePremultiplicationEnum> inputPremult;
        Natron::ImagePremultiplicationEnum outputPremult;
        bool isBeingRenderedElsewhere;

        ImagePlanesToRender()
            : rectsToRender()
            , planes()
            , inputPremult()
            , outputPremult(eImagePremultiplicationPremultiplied)
            , isBeingRenderedElsewhere(false)
        {
        }
    };


    /**
     * @brief If the caller thread is currently rendering an image, it will return a pointer to it
     * otherwise it will return NULL.
     * This function also returns the current renderWindow that is being rendered on that image
     * To be called exclusively on a render thread.
     *
     * WARNING: This call isexpensive and this function should not be called many times.
     **/
    bool getThreadLocalRenderedPlanes(std::map<Natron::ImageComponents, EffectInstance::PlaneToRender >*  planes,
                                      Natron::ImageComponents* planeBeingRendered,
                                      RectI* renderWindow) const;

    bool getThreadLocalNeededComponents(boost::shared_ptr<ComponentsNeededMap>* neededComps) const;

    /**
     * @brief Called when the associated node's hash has changed.
     * This is always called on the main-thread.
     **/
    void onNodeHashChanged(U64 hash);

    void resetTotalTimeSpentRendering();

    virtual void initializeData()
    {
    }

#ifdef DEBUG
    void checkCanSetValueAndWarn() const;
#endif

protected:


    virtual void resetTimeSpentRenderingInfos()
    {
    }

#ifdef DEBUG

    /*
     Debug helper to track plug-in that do setValue calls that are forbidden
     
     http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#SettingParams
     Officially, setValue calls are allowed during the following actions:
     
     The Create Instance Action
     The The Begin Instance Changed Action
     The The Instance Changed Action
     The The End Instance Changed Action
     The The Sync Private Data Action
     
     
 */

    void setCanSetValue(bool can);

    void invalidateCanSetValueFlag();


    bool isDuringActionThatCanSetValue() const;

    class CanSetSetValueFlag_RAII
    {
        EffectInstance* effect;
        
    public:
        
        CanSetSetValueFlag_RAII(EffectInstance* effect,bool canSetValue)
        : effect(effect)
        {
            effect->setCanSetValue(canSetValue);
        }
        
        ~CanSetSetValueFlag_RAII()
        {
            effect->invalidateCanSetValueFlag();
        }
    } ;

    bool checkCanSetValue() const
    {
        return isDuringActionThatCanSetValue();
    }


#define SET_CAN_SET_VALUE(canSetValue) Natron::EffectInstance::CanSetSetValueFlag_RAII canSetValueSetter(this,canSetValue)
#else
#define SET_CAN_SET_VALUE(canSetValue) ( (void)0 )
#endif // DEBUG

    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void knobChanged(KnobI* /*k*/,
                             Natron::ValueChangedReasonEnum /*reason*/,
                             int /*view*/,
                             double /*time*/,
                             bool /*originatedFromMainThread*/)
    {
    }

    virtual Natron::StatusEnum beginSequenceRender(double /*first*/,
                                                   double /*last*/,
                                                   double /*step*/,
                                                   bool /*interactive*/,
                                                   const RenderScale & /*scale*/,
                                                   bool /*isSequentialRender*/,
                                                   bool /*isRenderResponseToUserInteraction*/,
                                                   bool /*draftMode*/,
                                                   int /*view*/)
    {
        return Natron::eStatusOK;
    }

    virtual Natron::StatusEnum endSequenceRender(double /*first*/,
                                                 double /*last*/,
                                                 double /*step*/,
                                                 bool /*interactive*/,
                                                 const RenderScale & /*scale*/,
                                                 bool /*isSequentialRender*/,
                                                 bool /*isRenderResponseToUserInteraction*/,
                                                 bool /*draftMode*/,
                                                 int /*view*/)
    {
        return Natron::eStatusOK;
    }

public:

    ///Doesn't do anything, instead we overriden onKnobValueChanged_public
    virtual void onKnobValueChanged(KnobI* k, Natron::ValueChangedReasonEnum reason, double time,
                                    bool originatedFromMainThread) OVERRIDE FINAL;
    Natron::StatusEnum beginSequenceRender_public(double first, double last,
                                                  double step, bool interactive, const RenderScale & scale,
                                                  bool isSequentialRender, bool isRenderResponseToUserInteraction,
                                                  bool draftMode,
                                                  int view);
    Natron::StatusEnum endSequenceRender_public(double first, double last,
                                                double step, bool interactive, const RenderScale & scale,
                                                bool isSequentialRender, bool isRenderResponseToUserInteraction,
                                                bool draftMode,
                                                int view);


    void drawOverlay_public(double time, const RenderScale & renderScale, int view);

bool onOverlayPenDown_public(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;

    bool onOverlayPenMotion_public(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;

    bool onOverlayPenUp_public(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;

    bool onOverlayKeyDown_public(double time, const RenderScale & renderScale, int view, Natron::Key key, Natron::KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyUp_public(double time, const RenderScale & renderScale, int view, Natron::Key key, Natron::KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyRepeat_public(double time, const RenderScale & renderScale, int view, Natron::Key key, Natron::KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayFocusGained_public(double time, const RenderScale & renderScale, int view) WARN_UNUSED_RETURN;

    bool onOverlayFocusLost_public(double time, const RenderScale & renderScale, int view) WARN_UNUSED_RETURN;

    virtual bool isDoingInteractAction() const OVERRIDE FINAL WARN_UNUSED_RETURN;

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

    virtual void redrawOverlayInteract();

    /**
     * @brief Returns the components available on each input for this effect at the given time.
     **/
    void getComponentsAvailable(bool useLayerChoice, bool useThisNodeComponentsNeeded, double time, ComponentsAvailableMap* comps);
    void getComponentsAvailable(bool useLayerChoice, bool useThisNodeComponentsNeeded, double time, ComponentsAvailableMap* comps, std::list<Natron::EffectInstance*>* markedNodes);

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

    virtual void onKnobsLoaded() {}


private:

    void getComponentsAvailableRecursive(bool useLayerChoice,
                                         bool useThisNodeComponentsNeeded,
                                         double time,
                                         int view,
                                         ComponentsAvailableMap* comps,
                                         std::list<Natron::EffectInstance*>* markedNodes);

public:

    void getComponentsNeededAndProduced_public(bool useLayerChoice,
                                               bool useThisNodeComponentsNeeded,
                                               double time, int view,
                                              EffectInstance::ComponentsNeededMap* comps,
                                               bool* processAllRequested,
                                               SequenceTime* passThroughTime,
                                               int* passThroughView,
                                               std::bitset<4> *processChannels,
                                               boost::shared_ptr<Natron::Node>* passThroughInput);

    void setComponentsAvailableDirty(bool dirty);


    /**
     * @brief Check if Transform effects concatenation is possible on the current node and node upstream.
     **/
    void tryConcatenateTransforms(double time,
                                  int view,
                                  const RenderScale & scale,
                                  InputMatrixMap* inputTransforms);


    static void transformInputRois(Natron::EffectInstance* self,
                                   const boost::shared_ptr<InputMatrixMap>& inputTransforms,
                                   double par,
                                   const RenderScale & scale,
                                   RoIMap* inputRois,
                                   std::map<int, EffectInstance*>* reroutesMap);

    struct RenderArgs
    {
        RectD rod; //!< the effect's RoD in CANONICAL coordinates
        RoIMap regionOfInterestResults; //< the input RoI's in CANONICAL coordinates
        RectI renderWindowPixel; //< the current renderWindow in PIXEL coordinates
        double time; //< the time to render
        int view; //< the view to render
        bool validArgs; //< are the args valid ?
        bool isIdentity;
        double identityTime;
        Natron::EffectInstance* identityInput;
        
        EffectInstance::InputImagesMap inputImages;
        std::map<Natron::ImageComponents, PlaneToRender> outputPlanes;
        
        //This is set only when the plug-in has set ePassThroughRenderAllRequestedPlanes
        Natron::ImageComponents outputPlaneBeingRendered;
        boost::shared_ptr<ComponentsNeededMap>  compsNeeded;
        double firstFrame, lastFrame;
        
        boost::shared_ptr<InputMatrixMap> transformRedirections;
        
        RenderArgs();
        
        RenderArgs(const RenderArgs & o);
    };

    //these are per-node thread-local data
    struct EffectTLSData {
        
        //Used to count the begin/endRenderAction recursion
        int beginEndRenderCount;
        
        
        ///Used to count the recursion in the function calls
        /* The image effect actions which may trigger a recursive action call on a single instance are...
         
         kOfxActionBeginInstanceChanged
         kOfxActionInstanceChanged
         kOfxActionEndInstanceChanged
         The interact actions which may trigger a recursive action to be called on the associated plugin instance are...
         
         kOfxInteractActionGainFocus
         kOfxInteractActionKeyDown
         kOfxInteractActionKeyRepeat
         kOfxInteractActionKeyUp
         kOfxInteractActionLoseFocus
         kOfxInteractActionPenDown
         kOfxInteractActionPenMotion
         kOfxInteractActionPenUp
         
         The image effect actions which may be called recursively are...
         
         kOfxActionBeginInstanceChanged
         kOfxActionInstanceChanged
         kOfxActionEndInstanceChanged
         kOfxImageEffectActionGetClipPreferences
         The interact actions which may be called recursively are...
         
         kOfxInteractActionDraw
         
         */
        int actionRecursionLevel;
#ifdef DEBUG
        std::list<bool> canSetValue;
#endif
        
        ParallelRenderArgs frameArgs;
        EffectInstance::RenderArgs currentRenderArgs;
        
        EffectTLSData()
        : beginEndRenderCount(0)
        , actionRecursionLevel(0)
#ifdef DEBUG
        , canSetValue()
#endif
        , frameArgs()
        , currentRenderArgs()
        {
            
        }
    };

    typedef boost::shared_ptr<EffectTLSData> EffectDataTLSPtr;

protected:

    /**
     * @brief Equivalent to assert(actionsRecursionLevel == 0).
     * In release mode an exception is thrown instead.
     * This should be called in all actions except in the following recursive actions...
     *
     * kOfxActionBeginInstanceChanged
     * kOfxActionInstanceChanged
     * kOfxActionEndInstanceChanged
     * kOfxImageEffectActionGetClipPreferences
     * kOfxInteractActionDraw
     *
     * We also allow recursion in some other actions such as getRegionOfDefinition or isIdentity
     **/
    virtual void assertActionIsNotRecursive() const OVERRIDE FINAL;

    /**
     * @brief Should be called in the begining of an action.
     * Right after assertActionIsNotRecursive() for non recursive actions.
     **/
    virtual void incrementRecursionLevel() OVERRIDE FINAL;

    /**
     * @brief Should be called at the end of an action.
     **/
    virtual void decrementRecursionLevel() OVERRIDE FINAL;

public:

    virtual int getRecursionLevel() const OVERRIDE FINAL WARN_UNUSED_RETURN;

protected:

    virtual void refreshExtraStateAfterTimeChanged(double time)  OVERRIDE;

    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() OVERRIDE
    {
    };




    /**
     * @brief Returns a map of the components produced by this effect and the components needed by the inputs of this effect.
     * The output is mapped against -1. For all components not produced and if this effect is passthrough, it should use the
     * passThroughInput to fetch the components needed.
     **/
    virtual void getComponentsNeededAndProduced(double time, int view,
                                               EffectInstance::ComponentsNeededMap* comps,
                                                SequenceTime* passThroughTime,
                                                int* passThroughView,
                                                boost::shared_ptr<Natron::Node>* passThroughInput);


    /**
     * @brief This function is provided for means to copy more data than just the knobs from the live instance
     * to the render clones.
     **/
    virtual void cloneExtras()
    {
    }

    virtual void drawOverlay(double /*time*/,
                             const RenderScale & /*renderScale*/,
                             int /*view*/)
    {
    }

    virtual bool onOverlayPenDown(double /*time*/,
                                  const RenderScale & /*renderScale*/,
                                  int /*view*/,
                                  const QPointF & /*viewportPos*/,
                                  const QPointF & /*pos*/,
                                  double /*pressure*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayPenMotion(double /*time*/,
                                    const RenderScale & /*renderScale*/,
                                    int /*view*/,
                                    const QPointF & /*viewportPos*/,
                                    const QPointF & /*pos*/,
                                    double /*pressure*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayPenUp(double /*time*/,
                                const RenderScale & /*renderScale*/,
                                int /*view*/,
                                const QPointF & /*viewportPos*/,
                                const QPointF & /*pos*/,
                                double /*pressure*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayKeyDown(double /*time*/,
                                  const RenderScale & /*renderScale*/,
                                  int /*view*/,
                                  Natron::Key /*key*/,
                                  Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayKeyUp(double /*time*/,
                                const RenderScale & /*renderScale*/,
                                int /*view*/,
                                Natron::Key /*key*/,
                                Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayKeyRepeat(double /*time*/,
                                    const RenderScale & /*renderScale*/,
                                    int /*view*/,
                                    Natron::Key /*key*/,
                                    Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayFocusGained(double /*time*/,
                                      const RenderScale & /*renderScale*/,
                                      int /*view*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayFocusLost(double /*time*/,
                                    const RenderScale & /*renderScale*/,
                                    int /*view*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    boost::weak_ptr<Node> _node; //< the node holding this effect

private:

    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
    enum RenderRoIStatusEnum
    {
        eRenderRoIStatusImageAlreadyRendered = 0, // there was nothing left to render
        eRenderRoIStatusImageRendered, // we rendered what was missing
        eRenderRoIStatusRenderFailed // render failed
    };


    /**
     * @brief The internal of renderRoI, mainly it calls render and handles the thread safety of the effect.
     * @param time The time at which to render
     * @param scale The scale at which to render
     * @param mipMapLevel Redundant with scale
     * @param view The view on which to render
     * @param renderWindow The rectangle to render of the image, in pixel coordinates
     * @param cachedImgParams The parameters of the image to render as they are in the cache.
     * @param image This is the "full-scale" image, if the effect does support the render scale, then
     * image and downscaledImage are pointing to the SAME image.
     * @param downscaledImage If the effect doesn't support the render scale, then this is a pointer to the
     * downscaled image. If the effect doesn't support render scale then it will render in the "image" parameter
     * and then downscale the results into downscaledImage.
     * @param isSequentialRender True when the render is sequential
     * @param isRenderMadeInResponseToUserInteraction True when the render is made due to user interaction
     * @param byPassCache Cache look-ups have been already handled by renderRoI(...) but we pass it here because
     * we need to call renderRoI() on the input effects with this parameter too.
     * @param nodeHash The hash of the node used to render. This might no longer be equal to the value returned by
     * getHash() because the user might have changed something in the project (parameters...links..)
     * @param channelForAlpha This is passed here so that we can remember it later when converting the mask
     * which channel we wanted for the alpha channel.
     * @param renderFullScaleThenDownscale means that rendering should be done at full resolution and then
     * downscaled, because the plugin does not support render scale.
     * @returns True if the render call succeeded, false otherwise.
     **/
    RenderRoIStatusEnum renderRoIInternal(double time,
                                          const ParallelRenderArgs & frameArgs,
                                          Natron::RenderSafetyEnum safety,
                                          unsigned int mipMapLevel,
                                          int view,
                                          const RectD & rod, //!< rod in canonical coordinates
                                          const double par,
                                          const boost::shared_ptr<ImagePlanesToRender> & planes,
                                          bool isSequentialRender,
                                          bool isRenderMadeInResponseToUserInteraction,
                                          U64 nodeHash,
                                          bool renderFullScaleThenDownscale,
                                          bool byPassCache,
                                          Natron::ImageBitDepthEnum outputClipPrefDepth,
                                          const std::list<Natron::ImageComponents> & outputClipPrefsComps,
                                          const boost::shared_ptr<ComponentsNeededMap> & compsNeeded,
                                          std::bitset<4> processChannels);


    /// \returns false if rendering was aborted
    RenderRoIRetCode renderInputImagesForRoI(const FrameViewRequest* request,
                                             bool useTransforms,
                                             double time,
                                             int view,
                                             const RectD & rod,
                                             const RectD & canonicalRenderWindow,
                                             const boost::shared_ptr<InputMatrixMap> & transformMatrix,
                                             unsigned int mipMapLevel,
                                             const RenderScale & renderMappedScale,
                                             bool useScaleOneInputImages,
                                             bool byPassCache,
                                             const FramesNeededMap & framesNeeded,
                                             const EffectInstance::ComponentsNeededMap & compsNeeded,
                                             EffectInstance::InputImagesMap *inputImages,
                                             RoIMap* inputsRoI);

    static boost::shared_ptr<Natron::Image> convertPlanesFormatsIfNeeded(const AppInstance* app,
                                                                         const boost::shared_ptr<Natron::Image>& inputImage,
                                                                         const RectI& roi,
                                                                         const ImageComponents& targetComponents,
                                                                         ImageBitDepthEnum targetDepth,
                                                                         bool useAlpha0ForRGBToRGBAConversion,
                                                                         ImagePremultiplicationEnum outputPremult,
                                                                         int channelForAlpha);


    /**
     * @brief Called by getImage when the thread-storage was not set by the caller thread (mostly because this is a thread that is not
     * a thread controlled by Natron).
     **/
    // TODO: shouldn't this be documented a bit more? (parameters?)
    bool retrieveGetImageDataUponFailure(const double time,
                                         const int view,
                                         const RenderScale & scale,
                                         const RectD* optionalBoundsParam,
                                         U64* nodeHash_p,
                                         bool* isIdentity_p,
                                         double* identityTime,
                                         Natron::EffectInstance** identityInput_p,
                                         bool* duringPaintStroke_p,
                                         RectD* rod_p,
                                         RoIMap* inputRois_p, //!< output, only set if optionalBoundsParam != NULL
                                         RectD* optionalBounds_p); //!< output, only set if optionalBoundsParam != NULL


    bool allocateImagePlane(const ImageKey & key,
                            const RectD & rod,
                            const RectI & downscaleImageBounds,
                            const RectI & fullScaleImageBounds,
                            bool isProjectFormat,
                            const FramesNeededMap & framesNeeded,
                            const Natron::ImageComponents & components,
                            Natron::ImageBitDepthEnum depth,
                            double par,
                            unsigned int mipmapLevel,
                            bool renderFullScaleThenDownscale,
                            bool useDiskCache,
                            bool createInCache,
                            boost::shared_ptr<Natron::Image>* fullScaleImage,
                            boost::shared_ptr<Natron::Image>* downscaleImage);

    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    void evaluate(KnobI* knob, bool isSignificant, Natron::ValueChangedReasonEnum reason) OVERRIDE;


    virtual void onAllKnobsSlaved(bool isSlave, KnobHolder* master) OVERRIDE FINAL;
    virtual void onKnobSlaved(KnobI* slave, KnobI* master,
                              int dimension,
                              bool isSlave) OVERRIDE FINAL;
    enum RenderingFunctorRetEnum
    {
        eRenderingFunctorRetFailed, //< must stop rendering
        eRenderingFunctorRetOK, //< ok, move on
        eRenderingFunctorRetTakeImageLock, //< take the image lock because another thread is rendering part of something we need
        eRenderingFunctorRetAborted // we were aborted
    };


    /**
     * @brief Returns the index of the input if inputEffect is a valid input connected to this effect, otherwise returns -1.
     **/
    int getInputNumber(Natron::EffectInstance* inputEffect) const;


};



/**
 * @typedef Any plug-in should have a static function called BuildEffect with the following signature.
 * It is used to build a new instance of an effect. Basically it should just call the constructor.
 **/
typedef Natron::EffectInstance* (*EffectBuilder)(boost::shared_ptr<Node>);

} // Natron
#endif // NATRON_ENGINE_EFFECTINSTANCE_H
