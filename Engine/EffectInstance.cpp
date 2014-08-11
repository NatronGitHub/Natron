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

#include "EffectInstance.h"

#include <sstream>
#include <QtConcurrentMap>
#include <QReadWriteLock>
#include <QCoreApplication>
#include <QtConcurrentRun>

#include <boost/bind.hpp>

#include "Global/MemoryInfo.h"
#include "Engine/AppManager.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Log.h"
#include "Engine/VideoEngine.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/KnobFile.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/AppInstance.h"
#include "Engine/ThreadStorage.h"
#include "Engine/Settings.h"
#include "Engine/RotoContext.h"
using namespace Natron;


class File_Knob;
class OutputFile_Knob;


OutputImageLocker::OutputImageLocker(Natron::Node* node,const boost::shared_ptr<Natron::Image>& image)
: n (node) , img(image)
{
    assert(n && img);
    n->addImageBeingRendered(img);
}

OutputImageLocker::~OutputImageLocker()
{
    n->removeImageBeingRendered(img);
}




struct EffectInstance::RenderArgs
{
    RectD _rod; //!< the effect's RoD in canonical coordinates
    RoIMap _regionOfInterestResults; //< the input RoI's in CANONICAL coordinates
    SequenceTime _time; //< the time to render
    RenderScale _scale; //< the scale to render
    unsigned int _mipMapLevel; //< the mipmap level to render (redundant with scale)
    int _view; //< the view to render
    bool _isSequentialRender; //< is this sequential ?
    bool _isRenderResponseToUserInteraction; //< is this a render due to user interaction ?
    bool _byPassCache; //< use cache lookups  ?
    bool _validArgs; //< are the args valid ?
    U64 _nodeHash;
    U64 _rotoAge;
    int _channelForAlpha;
    bool _isIdentity;
    SequenceTime _identityTime;
    int _identityInputNb;
    
    RenderArgs()
    : _rod()
    , _regionOfInterestResults()
    , _time(0)
    , _scale()
    , _mipMapLevel(0)
    , _view(0)
    , _isSequentialRender(false)
    , _isRenderResponseToUserInteraction(false)
    , _byPassCache(false)
    , _validArgs(false)
    , _nodeHash(0)
    , _rotoAge(0)
    , _channelForAlpha(3)
    , _isIdentity(false)
    , _identityTime(0)
    , _identityInputNb(-1)
    {}
};

struct EffectInstance::Implementation {
    Implementation()
    : renderAbortedMutex()
    , renderAborted(false)
    , renderArgs()
    , beginEndRenderCount()
    , inputImages()
    , lastRenderArgsMutex()
    , lastRenderHash(0)
    , lastImage()
    , duringInteractActionMutex()
    , duringInteractAction(false)
    , pluginMemoryChunksMutex()
    , pluginMemoryChunks()
    , supportsRenderScale(eSupportsMaybe)
    {
    }

    mutable QReadWriteLock renderAbortedMutex;
    bool renderAborted; //< was rendering aborted ?
    
    ThreadStorage<RenderArgs> renderArgs;
    ThreadStorage<int> beginEndRenderCount;
    
    ///Whenever a render thread is running, it stores here a temp copy used in getImage
    ///to make sure these images aren't cleared from the cache.
    ThreadStorage< std::list< boost::shared_ptr<Natron::Image> > > inputImages;
    
    QMutex lastRenderArgsMutex; //< protects lastRenderArgs & lastImageKey
    U64  lastRenderHash; //< the last hash given to render
    boost::shared_ptr<Natron::Image> lastImage; //< the last image rendered
    
    mutable QReadWriteLock duringInteractActionMutex; //< protects duringInteractAction
    bool duringInteractAction; //< true when we're running inside an interact action

    mutable QMutex pluginMemoryChunksMutex;
    std::list<PluginMemory*> pluginMemoryChunks;

    QMutex supportsRenderScaleMutex;
    SupportsEnum supportsRenderScale;

    void setDuringInteractAction(bool b) {
        QWriteLocker l(&duringInteractActionMutex);
        duringInteractAction = b;
    }
    
    
    class ScopedInstanceChangedArgs {
        RenderArgs args;
        ThreadStorage<RenderArgs>* _dst;
    public:
        
        ScopedInstanceChangedArgs(ThreadStorage<RenderArgs>* dst,
                                  SequenceTime time,
                                  int view,
                                  unsigned int mipMapLevel,
                                  const RectD& rod, //!< effect output rod in canonical coordinates
                                  const RoIMap& inputsRoI,
                                  bool isSequential,
                                  bool byPassCache,
                                  U64 nodeHash,
                                  U64 rotoAge,
                                  bool isIdentity,
                                  SequenceTime identityTime,
                                  int inputNbIdentity)
        : args()
        , _dst(dst)
        {
            assert(_dst);
            args._rod = rod;
            args._regionOfInterestResults = inputsRoI;
            args._time = time;
            args._view = view;
            args._mipMapLevel = mipMapLevel;
            args._scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
            args._scale.y = args._scale.x;
            args._isSequentialRender = isSequential;
            args._isRenderResponseToUserInteraction = true;
            args._byPassCache = byPassCache;
            args._nodeHash = nodeHash;
            args._rotoAge = rotoAge;
            args._channelForAlpha = 3;
            args._isIdentity = isIdentity;
            args._identityTime = identityTime;
            args._identityInputNb = inputNbIdentity;
            args._validArgs = true;
            _dst->setLocalData(args);
        }
        
        
        ~ScopedInstanceChangedArgs()
        {
            assert(_dst->hasLocalData());
            args._validArgs = false;
            _dst->setLocalData(args);
        }
        

        
    };
    
    /**
     * @brief Small helper class that set the render args and
     * invalidate them when it is destroyed.
     **/
    class ScopedRenderArgs {
        
        RenderArgs args;
        ThreadStorage<RenderArgs>* _dst;
    public:
        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,
                         const RoIMap& roiMap,
                         const RectD& rod,
                         SequenceTime time,
                         int view,
                         const RenderScale& scale,
                         unsigned int mipMapLevel,
                         bool sequential,
                         bool userInteraction,
                         bool bypassCache,
                         U64 nodeHash,
                         U64 rotoAge,
                         int channelForAlpha,
                         bool isIdentity,
                         SequenceTime identityTime,
                         int inputNbIdentity)
        : args()
        , _dst(dst)
        {
            assert(_dst);
            args._regionOfInterestResults = roiMap;
            args._rod = rod;
            args._time = time;
            args._view = view;
            args._scale = scale;
            args._mipMapLevel = mipMapLevel;
            args._isSequentialRender = sequential;
            args._isRenderResponseToUserInteraction = userInteraction;
            args._byPassCache = bypassCache;
            args._nodeHash = nodeHash;
            args._rotoAge = rotoAge;
            args._channelForAlpha = channelForAlpha;
            args._isIdentity = isIdentity;
            args._identityTime = identityTime;
            args._identityInputNb = inputNbIdentity;
            args._validArgs = true;
            _dst->setLocalData(args);
        }
        
        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,const RenderArgs& a)
        : args(a)
        , _dst(dst)
        {
            args._validArgs = true;
            _dst->setLocalData(args);
        }
        
        ~ScopedRenderArgs()
        {
            assert(_dst->hasLocalData());
            args._validArgs = false;
            _dst->setLocalData(args);
        }
        
        /**
         * @brief WARNING: Returns the args that have been passed to the constructor.
         **/
        const RenderArgs& getArgs() const { return args; }
    };
    
    void addInputImageTempPointer(const boost::shared_ptr<Natron::Image>& img) {
        if (inputImages.hasLocalData()) {
            inputImages.localData().push_back(img);
        } else {
            std::list< boost::shared_ptr<Natron::Image> > newList;
            newList.push_back(img);
            inputImages.localData() = newList;
        }
    }
    
    void clearInputImagePointers() {
        if (inputImages.hasLocalData()) {
            inputImages.localData().clear();
        }
    }
    
    
};

EffectInstance::EffectInstance(boost::shared_ptr<Node> node)
: NamedKnobHolder(node ? node->getApp() : NULL)
, _node(node)
, _imp(new Implementation)
{
}

EffectInstance::~EffectInstance()
{
    if (_node) {
        appPTR->removeAllImagesFromCacheWithMatchingKey(hash());
    }
    clearPluginMemoryChunks();
}

void
EffectInstance::clearPluginMemoryChunks()
{
    int toRemove;
    {
        QMutexLocker l(&_imp->pluginMemoryChunksMutex);
        toRemove = (int)_imp->pluginMemoryChunks.size();
    }
    while (toRemove > 0) {
        PluginMemory* mem;
        {
            QMutexLocker l(&_imp->pluginMemoryChunksMutex);
            mem = (*_imp->pluginMemoryChunks.begin());
        }
        delete mem;
        --toRemove;
    }
}

U64
EffectInstance::hash() const
{
    return _node->getHashValue();
}

bool
EffectInstance::getRenderHash(U64* hash) const
{
    if (!_imp->renderArgs.hasLocalData() || !_imp->renderArgs.localData()._validArgs) {
        return false;
    } else {
        *hash = _imp->renderArgs.localData()._nodeHash;
        return true;
    }
}

bool
EffectInstance::aborted() const
{
    QReadLocker l(&_imp->renderAbortedMutex);
    return _imp->renderAborted;
}

void
EffectInstance::setAborted(bool b)
{
    QWriteLocker l(&_imp->renderAbortedMutex);
    _imp->renderAborted = b;
}

U64
EffectInstance::knobsAge() const {
    return _node->getKnobsAge();
}

void
EffectInstance::setKnobsAge(U64 age) {
    _node->setKnobsAge(age);
}

const std::string&
EffectInstance::getName() const{
    return _node->getName();
}

std::string EffectInstance::getName_mt_safe() const
{
    return _node->getName_mt_safe();
}

void
EffectInstance::getRenderFormat(Format *f) const
{
    assert(f);
    getApp()->getProject()->getProjectDefaultFormat(f);
}

int
EffectInstance::getRenderViewsCount() const{
    return getApp()->getProject()->getProjectViewsCount();
}


bool
EffectInstance::hasOutputConnected() const
{
    return _node->hasOutputConnected();
}

EffectInstance*
EffectInstance::input(int n) const
{
    
    ///Only called by the main-thread
    
    boost::shared_ptr<Natron::Node> inputNode = _node->input(n);
    if (inputNode) {
        return inputNode->getLiveInstance();
    }
    return NULL;
}

EffectInstance*
EffectInstance::input_other_thread(int n) const
{
    boost::shared_ptr<Natron::Node> inputNode = _node->input_other_thread(n);
    if (inputNode) {
        return inputNode->getLiveInstance();
    }
    return NULL;
}

std::string
EffectInstance::inputLabel(int inputNb) const
{
    std::string out;
    out.append(1,(char)(inputNb+65));
    return out;
}

boost::shared_ptr<Natron::Image>
EffectInstance::getImage(int inputNb,
                         const SequenceTime time,
                         const RenderScale& scale,
                         const int view,
                         const RectD *optionalBoundsParam, //!< optional region in canonical coordinates
                         const Natron::ImageComponents comp,
                         const Natron::ImageBitDepth depth,
                         const bool dontUpscale)
{
    bool isMask = isInputMask(inputNb);

    if (isMask && !isMaskEnabled(inputNb)) {
        ///This is last resort, the plug-in should've checked getConnected() before which would have returned false.
        return boost::shared_ptr<Natron::Image>();
    }
    
    EffectInstance* n;
    if (QThread::currentThread() == qApp->thread()) {
        n = input(inputNb);
    } else {
        n  = input_other_thread(inputNb);
    }
    
   
    boost::shared_ptr<RotoContext> roto = _node->getRotoContext();
    bool useRotoInput = false;
    if (roto) {
        useRotoInput = isInputRotoBrush(inputNb);
    }
    if ((!roto || (roto && !useRotoInput)) && !n) {
        return boost::shared_ptr<Natron::Image>();
    }
    
    RectD optionalBounds;
    if (optionalBoundsParam) {
        optionalBounds = *optionalBoundsParam;
    }
    bool isSequentialRender,isRenderUserInteraction,byPassCache;
    unsigned int mipMapLevel = 0;
    RoIMap inputsRoI;
    RectD rod;
    bool isIdentity;
    SequenceTime identityTime;
    int inputNbIdentity;
    ///The caller thread MUST be a thread owned by Natron. It cannot be a thread from the multi-thread suite.
    ///A call to getImage is forbidden outside an action running in a thread launched by Natron.
    
    /// From http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsImagesAndClipsUsingClips
    //    Images may be fetched from an attached clip in the following situations...
    //    in the kOfxImageEffectActionRender action
    //    in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason of kOfxChangeUserEdited
    if(!_imp->renderArgs.hasLocalData() || !_imp->renderArgs.localData()._validArgs) {
        ///This is a bad plug-in
        qDebug() << getNode()->getName_mt_safe().c_str() << " is trying to call clipGetImage during an unauthorized time. "
        "Developers of that plug-in should fix it. \n Reminder from the OpenFX spec: \n "
        "Images may be fetched from an attached clip in the following situations... \n"
        "- in the kOfxImageEffectActionRender action\n"
        "- in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason or kOfxChangeUserEdited";
        ///Try to compensate for the mistale
        isSequentialRender = true;
        isRenderUserInteraction = true;
        byPassCache = false;
    
        Natron::Status stat = getRegionOfDefinition(time, scale, view, &rod);
        if (stat == StatFailed) {
            return boost::shared_ptr<Natron::Image>();
        }

        ///// This code is wrong but executed ONLY IF THE PLUG-IN DOESN'T RESPECT THE SPECIFICATIONS. Recursive actions
        ///// should never happen.
        if (!optionalBounds) {
            ///// We cannot recover the RoI, we just assume the plug-in wants to render the full RoD.
            optionalBounds = rod;
            (void)ifInfiniteApplyHeuristic(time, scale, view, &optionalBounds);


            /// If the region parameter is not set to NULL, then it will be clipped to the clip's
            /// Region of Definition for the given time. The returned image will be m at m least as big as this region.
            /// If the region parameter is not set, then the region fetched will be at least the Region of Interest
            /// the effect has previously specified, clipped the clip's Region of Definition.
            /// (renderRoI will do the clipping for us).
            

            ///// This code is wrong but executed ONLY IF THE PLUG-IN DOESN'T RESPECT THE SPECIFICATIONS. Recursive actions
            ///// should never happen.
            inputsRoI = getRegionsOfInterest(time, scale, optionalBounds, optionalBounds, 0);
        }

        isIdentity = isIdentity_public(time, scale, rod, view, &identityTime, &inputNbIdentity);
    } else {
        isSequentialRender = _imp->renderArgs.localData()._isSequentialRender;
        isRenderUserInteraction = _imp->renderArgs.localData()._isRenderResponseToUserInteraction;
        byPassCache = _imp->renderArgs.localData()._byPassCache;
        mipMapLevel = _imp->renderArgs.localData()._mipMapLevel;
        inputsRoI = _imp->renderArgs.localData()._regionOfInterestResults;
        rod = _imp->renderArgs.localData()._rod;
        isIdentity = _imp->renderArgs.localData()._isIdentity;
        identityTime = _imp->renderArgs.localData()._identityTime;
        inputNbIdentity = _imp->renderArgs.localData()._identityInputNb;
    }
    
    RectD roi;
    if (!optionalBounds) {
        RoIMap::iterator found = inputsRoI.find(useRotoInput ? this : n);
        assert(found != inputsRoI.end());
        ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
        roi = found->second;
    } else {
        roi = optionalBounds;
    }
    
    
    ///If the effect is an identity but it didn't ask for the effect's image of which it is identity
    ///return a null image
    if (isIdentity && inputNbIdentity != inputNb) {
        return boost::shared_ptr<Image>();
    }
    
    ///Both the result of getRegionsOfInterest and optionalBounds are in canonical coordinates, we have to convert in both cases
    ///Convert to pixel coordinates
    RectI roiPixel;
    roi.toPixelEnclosing(scale, &roiPixel);

    int channelForAlpha = !isMask ? 3 : getMaskChannel(inputNb);
    
    if (useRotoInput) {
        U64 nodeHash = _imp->renderArgs.localData()._nodeHash;
        U64 rotoAge = _imp->renderArgs.localData()._rotoAge;
        Natron::ImageComponents outputComps;
        Natron::ImageBitDepth outputDepth;
        getPreferredDepthAndComponents(-1, &outputComps, &outputDepth);
        boost::shared_ptr<Natron::Image> mask =  roto->renderMask(roiPixel, outputComps, nodeHash,rotoAge,
                                                                  RectD(), time, depth, view, mipMapLevel, byPassCache);
        _imp->addInputImageTempPointer(mask);
        return mask;
    }
    
    
    //if the node is not connected, return a NULL pointer!
    if (!n) {
        return boost::shared_ptr<Natron::Image>();
    }
    
    ///Launch in another thread as the current thread might already have been created by the multi-thread suite,
    ///hence it might have a thread-id.
    QThreadPool::globalInstance()->reserveThread();
    U64 inputNodeHash;
    QFuture< boost::shared_ptr<Image > > future = QtConcurrent::run(n,&Natron::EffectInstance::renderRoI,
                                                                    RenderRoIArgs(time,
                                                                                  scale,
                                                                                  mipMapLevel,
                                                                                  view,
                                                                                  roiPixel,
                                                                                  isSequentialRender,
                                                                                  isRenderUserInteraction,
                                                                                  byPassCache,
                                                                                  RectD(),
                                                                                  comp,
                                                                                  depth,
                                                                                  channelForAlpha)
                                                                    ,&inputNodeHash);
    future.waitForFinished();
    QThreadPool::globalInstance()->releaseThread();
    boost::shared_ptr<Natron::Image> inputImg = future.result();
	if (!inputImg) {
		return inputImg;
	}
        
    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();

    ///If the plug-in doesn't support the render scale, but the image is downscale, up-scale it.
    ///Note that we do NOT cache it
#pragma message WARN("supportsRenderScale() always returns true for OFX - 'upscale' should be a parameter maybe?") // see doc for OfxEffectInstance::supportsRenderScale()
    if (!dontUpscale && inputImgMipMapLevel > 0 && !supportsRenderScale()) {
#pragma message WARN("wrong: it should use the originial RoD of the full-res image, not an upscaled one (which may be larger)")
        Natron::ImageBitDepth bitdepth = inputImg->getBitDepth();
        int mipMapLevel = 0;
        RectI bounds;
        rod.toPixelEnclosing(mipMapLevel, &bounds);
        boost::shared_ptr<Natron::Image> upscaledImg(new Natron::Image(inputImg->getComponents(), rod, bounds, mipMapLevel, bitdepth));
        inputImg->upscaleMipMap(inputImg->getBounds(), inputImgMipMapLevel, mipMapLevel, upscaledImg.get());
        return upscaledImg;
    } else {
        _imp->addInputImageTempPointer(inputImg);
        return inputImg;
    }
}

RectD
EffectInstance::calcDefaultRegionOfDefinition(SequenceTime /*time*/,
                                              const RenderScale &/*scale*/) const
{
    Format projectDefault;
    getRenderFormat(&projectDefault);
    return RectD(projectDefault.left(), projectDefault.bottom(), projectDefault.right(), projectDefault.top());
}

Natron::Status
EffectInstance::getRegionOfDefinition(SequenceTime time,
                                      const RenderScale& scale,
                                      int view,
                                      RectD* rod) //!< rod is in canonical coordinates
{
    bool firstInput = true;
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
        if (input) {
            RectD inputRod;
            bool isProjectFormat;
            Status st = input->getRegionOfDefinition_public(time, scale, view, &inputRod, &isProjectFormat);
            assert(inputRod.x2 >= inputRod.x1 && inputRod.y2 >= inputRod.y1);
            if (st == StatFailed) {
                return st;
            }
            
            if (firstInput) {
                *rod = inputRod;
                firstInput = false;
            } else {
                rod->merge(inputRod);
            }
            assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
        }
    }

    return StatReplyDefault;
}

bool
EffectInstance::ifInfiniteApplyHeuristic(SequenceTime time,
                                         const RenderScale& scale,
                                         int view,
                                         RectD* rod) const
{
    /*If the rod is infinite clip it to the project's default*/
    
    Format projectDefault;
    getRenderFormat(&projectDefault);
    /// FIXME: before removing the assert() (I know you are tempted) please explain (here: document!) if the format rectangle can be empty and in what situation(s)
    assert(!projectDefault.isNull());
    
    assert(rod);
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
    bool x1Infinite = rod->x1 <= kOfxFlagInfiniteMin;
    bool y1Infinite = rod->y1 <= kOfxFlagInfiniteMin;
    bool x2Infinite = rod->x2 >= kOfxFlagInfiniteMax;
    bool y2Infinite = rod->y2 >= kOfxFlagInfiniteMax;
    
    ///Get the union of the inputs.
    RectD inputsUnion;

    ///Do the following only if one coordinate is infinite otherwise we wont need the RoD of the input
    if (x1Infinite || y1Infinite || x2Infinite || y2Infinite) {
        // initialize with the effect's default RoD, because inputs may not be connected to other effects (e.g. Roto)
        inputsUnion = calcDefaultRegionOfDefinition(time, scale);
        bool first = true;
        for (int i = 0; i < maximumInputs(); ++i) {
            Natron::EffectInstance* input = input_other_thread(i);
            if (input) {
                RectD inputRod;
                bool isProjectFormat;
                Status st = input->getRegionOfDefinition_public(time, scale, view, &inputRod, &isProjectFormat);
                if (st != StatFailed) {
                    if (first) {
                        inputsUnion = inputRod;
                        first = false;
                    } else {
                        inputsUnion.merge(inputRod);
                    }
                }
            }
        }
    }
    ///If infinite : clip to inputsUnion if not null, otherwise to project default
    
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    bool isProjectFormat = false;
    if (x1Infinite) {
        if (!inputsUnion.isNull()) {
            rod->x1 = std::min(inputsUnion.x1, projectDefault.x1);
        } else {
            rod->x1 = projectDefault.x1;
            isProjectFormat = true;
        }
    }
    if (y1Infinite) {
        if (!inputsUnion.isNull()) {
            rod->y1 = std::min(inputsUnion.y1, projectDefault.y1);
        } else {
            rod->y1 = projectDefault.y1;
            isProjectFormat = true;
        }
    }
    if (x2Infinite) {
        if (!inputsUnion.isNull()) {
            rod->x2 = std::max(inputsUnion.x2, projectDefault.x2);
        } else {
            rod->x2 = projectDefault.x2;
            isProjectFormat = true;
        }
    }
    if (y2Infinite) {
        if (!inputsUnion.isNull()) {
            rod->y2 = std::max(inputsUnion.y2, projectDefault.y2);
        } else {
            rod->y2 = projectDefault.y2;
            isProjectFormat = true;
        }
    }
    if (isProjectFormat && !isGenerator()) {
        isProjectFormat = false;
    }
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
    return isProjectFormat;
}


EffectInstance::RoIMap
EffectInstance::getRegionsOfInterest(SequenceTime /*time*/,
                                     const RenderScale& /*scale*/,
                                     const RectD& /*outputRoD*/, //!< the RoD of the effect, in canonical coordinates
                                     const RectD& renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                     int /*view*/)
{
    RoIMap ret;
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
        if (input) {
            ret.insert(std::make_pair(input, renderWindow));
        }
    }
    return ret;
}

EffectInstance::FramesNeededMap
EffectInstance::getFramesNeeded(SequenceTime time)
{
    EffectInstance::FramesNeededMap ret;
    RangeD defaultRange;
    defaultRange.min = defaultRange.max = time;
    std::vector<RangeD> ranges;
    ranges.push_back(defaultRange);
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
        if (input) {
            ret.insert(std::make_pair(i, ranges));
        }
    }
    return ret;
}

void
EffectInstance::getFrameRange(SequenceTime *first,SequenceTime *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
        if (input) {
            SequenceTime inpFirst,inpLast;
            input->getFrameRange(&inpFirst, &inpLast);
            if (i == 0) {
                *first = inpFirst;
                *last = inpLast;
            } else {
                if (inpFirst < *first) {
                    *first = inpFirst;
                }
                if (inpLast > *last) {
                    *last = inpLast;
                }
            }

        }
    }
}



boost::shared_ptr<Natron::Image>
EffectInstance::renderRoI(const RenderRoIArgs& args,
                          U64* hashUsed)
{
#ifdef NATRON_LOG
    Natron::Log::beginFunction(getName(),"renderRoI");
    Natron::Log::print(QString("Time "+QString::number(time)+
                                                      " Scale ("+QString::number(scale.x)+
                                                      ","+QString::number(scale.y)
                        +") View " + QString::number(view) + " RoI: xmin= "+ QString::number(renderWindow.left()) +
                        " ymin= " + QString::number(renderWindow.bottom()) + " xmax= " + QString::number(renderWindow.right())
                        + " ymax= " + QString::number(renderWindow.top())).toStdString());
#endif
    
    ///For writer we never want to cache otherwise the next time we want to render it will skip writing the image on disk!
    bool byPassCache = args.byPassCache;
    if (isWriter()) {
        byPassCache = true;
    }
    
    ///Use the hash at this time, and then copy it to the clips in the thread local storage to use the same value
    ///through all the rendering of this frame.
    U64 nodeHash = hash();
    if (hashUsed) {
        *hashUsed = nodeHash;
    }
    
    boost::shared_ptr<const ImageParams> cachedImgParams;
    boost::shared_ptr<Image> image;
    

    RectD rod; //!< rod is in canonical coordinates
    bool isProjectFormat = false;

    ///if the rod is already passed as parameter, just use it and don't call getRegionOfDefinition
    if (!args.preComputedRoD.isNull()) {
        rod = args.preComputedRoD;
    } else {
        ///before allocating it we must fill the RoD of the image we want to render
        Status stat = getRegionOfDefinition_public(args.time, args.scale, args.view, &rod, &isProjectFormat);

        ///The rod might be NULL for a roto that has no beziers and no input
        if (stat == StatFailed || rod.isNull()) {
            ///if getRoD fails, just return a NULL ptr
            return boost::shared_ptr<Natron::Image>();
        }
    }

    Natron::ImageKey key = Natron::Image::makeKey(nodeHash, args.time,args.mipMapLevel,args.view);
    {
        ///If the last rendered image had a different hash key (i.e a parameter changed or an input changed)
        ///just remove the old image from the cache to recycle memory.
        ///We also do this if the mipmap level is different (e.g: the user is zooming in/out) because
        ///anyway the ViewerCache will have the texture cached and it would be redundant to keep this image
        ///in the cache since the ViewerCache already has it ready.
        boost::shared_ptr<Image> lastRenderedImage;
        U64 lastRenderHash;
        {
            QMutexLocker l(&_imp->lastRenderArgsMutex);
            lastRenderedImage = _imp->lastImage;
            lastRenderHash = _imp->lastRenderHash;
        }
        if (lastRenderedImage &&
            (lastRenderHash != nodeHash)) {
            ///try to obtain the lock for the last rendered image as another thread might still rely on it in the cache
            OutputImageLocker imgLocker(_node.get(),lastRenderedImage);
            ///once we got it remove it from the cache
            appPTR->removeAllImagesFromCacheWithMatchingKey(lastRenderHash);
            {
                QMutexLocker l(&_imp->lastRenderArgsMutex);
                _imp->lastImage.reset();
            }
        }
    }
    
    /// First-off look-up the cache and see if we can find the cached actions results and cached image.
    bool isCached = Natron::getImageFromCache(key, &cachedImgParams,&image);
    
    ////Lock the output image so that multiple threads do not access for writing at the same time.
    ////When it goes out of scope the lock will be released automatically
    boost::shared_ptr<OutputImageLocker> imageLock;
    
    if (isCached) {
        assert(cachedImgParams && image);
        
        imageLock.reset(new OutputImageLocker(_node.get(),image));
        
        if (cachedImgParams->isRodProjectFormat()) {
            ////If the image was cached with a RoD dependent on the project format, but the project format changed,
            ////just discard this entry
            Format projectFormat;
            getRenderFormat(&projectFormat);
            if (dynamic_cast<RectI&>(projectFormat) != cachedImgParams->getRoD()) {
                isCached = false;
                appPTR->removeFromNodeCache(image);
                cachedImgParams.reset();
                imageLock.reset(); //< release the lock after cleaning it from the cache
                image.reset();
            }
        }
        
        //Do the following only if we're not an identity
        if (image && cachedImgParams->getInputNbIdentity() == -1) {
            ///If components are different but convertible without damage, or bit depth is different, keep this image, convert it
            ///and continue render on it. This is in theory still faster than ignoring the image and doing a full render again.
            if ((image->getComponents() != args.components && Image::hasEnoughDataToConvert(image->getComponents(),args.components)) ||
                image->getBitDepth() != args.bitdepth) {
                ///Convert the image to the requested components
                assert(!rod.isNull());
                RectI bounds;
                rod.toPixelEnclosing(args.mipMapLevel, &bounds);
                boost::shared_ptr<Image> remappedImage(new Image(args.components, rod, bounds, args.mipMapLevel, args.bitdepth));
                if (!byPassCache) {
                    image->convertToFormat(image->getBounds(),
                                           getApp()->getDefaultColorSpaceForBitDepth(image->getBitDepth()),
                                           getApp()->getDefaultColorSpaceForBitDepth(args.bitdepth),
                                           args.channelForAlpha, false, true,
                                           remappedImage.get());
                }
                ///switch the pointer
                image = remappedImage;
            } else if (image->getComponents() != args.components) {
                assert(!Image::hasEnoughDataToConvert(image->getComponents(),args.components));
                ///we cannot convert without loosing data of some channels, we better off render everything again
                isCached = false;
                appPTR->removeFromNodeCache(image);
                cachedImgParams.reset();
                imageLock.reset(); //< release the lock after cleaning it from the cache
                image.reset();
            }
            
            if (isCached && byPassCache) {
                ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
                ///we're sure renderRoIInternal will compute the whole image again.
                ///We must use the cache facility anyway because we rely on it for caching the results
                ///of actions which is necessary to avoid recursive actions.
                image->clearBitmap();
            }
        }
    }
    
    boost::shared_ptr<Natron::Image> downscaledImage = image;

    if (!isCached) {
        
        ///first-off check whether the effect is identity, in which case we don't want
        /// to cache anything or render anything for this effect.
        SequenceTime inputTimeIdentity = 0.;
        int inputNbIdentity;
        FramesNeededMap framesNeeded;

        bool identity = isIdentity_public(args.time, args.scale, rod, args.view, &inputTimeIdentity, &inputNbIdentity);
    
        if (identity) {
            
            ///The effect is an identity but it has no inputs
            if (inputNbIdentity == -1) {
                return boost::shared_ptr<Natron::Image>();
            } else if (inputNbIdentity == -2) {
                ///This special value of -2 indicates that the plugin is identity of itself at another time
                RenderRoIArgs argCpy = args;
                argCpy.time = inputTimeIdentity;
                return renderRoI(argCpy);
                
            } else {
                RectD canonicalRoI;
                args.roi.toCanonical(args.mipMapLevel, rod, &canonicalRoI);
                RoIMap inputsRoI;
                inputsRoI.insert(std::make_pair(input_other_thread(inputNbIdentity), canonicalRoI));
                Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,
                                                            inputsRoI,
                                                            rod,
                                                            args.time,
                                                            args.view,
                                                            args.scale,
                                                            args.mipMapLevel,
                                                            args.isSequentialRender,
                                                            args.isRenderUserInteraction,
                                                            byPassCache,
                                                            nodeHash,
                                                            0,
                                                            args.channelForAlpha,
                                                            identity,
                                                            inputTimeIdentity,
                                                            inputNbIdentity);
                Natron::ImageComponents inputPrefComps;
                Natron::ImageBitDepth inputPrefDepth;
                Natron::EffectInstance* inputEffectIdentity = input_other_thread(inputNbIdentity);
                if (inputEffectIdentity) {
                    inputEffectIdentity->getPreferredDepthAndComponents(-1, &inputPrefComps, &inputPrefDepth);
                    ///we don't need to call getRegionOfDefinition and getFramesNeeded if the effect is an identity
                    image = getImage(inputNbIdentity, inputTimeIdentity, args.scale, args.view, NULL, inputPrefComps, inputPrefDepth, true);
                    ///Clear input images pointer because getImage has stored the image .
                    _imp->clearInputImagePointers();
                } else {
                    return image;
                }
                
                ///if we bypass the cache, don't cache the result of isIdentity
                if (byPassCache) {
                    return image;
                }
            }
        } else {
            ///set it to -1 so the cache knows it's not an identity
            inputNbIdentity = -1;

            // why should the rod be empty here?
            assert(!rod.isNull());
            
            framesNeeded = getFramesNeeded_public(args.time);
          
        }
    
        
        int cost = 0;
        /*should data be stored on a physical device ?*/
        if (shouldRenderedDataBePersistent()) {
            cost = 1;
        }
        
        if (identity) {
            cost = -1;
        }
      
        
        ///Cache the image with the requested components instead of the remapped ones
        cachedImgParams = Natron::Image::makeParams(cost,
                                                    rod,
                                                    args.mipMapLevel,
                                                    isProjectFormat,
                                                    args.components,
                                                    args.bitdepth,
                                                    inputNbIdentity,
                                                    inputTimeIdentity,
                                                    framesNeeded);
    
        ///even though we called getImage before and it returned false, it may now
        ///return true if another thread created the image in the cache, so we can't
        ///make any assumption on the return value of this function call.
        ///
        ///!!!Note that if isIdentity is true it will allocate an empty image object with 0 bytes of data.
        boost::shared_ptr<Image> newImage;
        bool cached = appPTR->getImageOrCreate(key, cachedImgParams, &newImage);
        if (!newImage) {
            std::stringstream ss;
            ss << "Failed to allocate an image of ";
            ss << printAsRAM(cachedImgParams->getElementsCount() * sizeof(Image::data_t)).toStdString();
            Natron::errorDialog(QObject::tr("Out of memory").toStdString(),ss.str());
            return newImage;
        }
        assert(newImage);
        imageLock.reset(new OutputImageLocker(_node.get(),newImage));
        
        if (cached && byPassCache) {
            ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
            ///we're sure renderRoIInternal will compute the whole image again.
            ///We must use the cache facility anyway because we rely on it for caching the results
            ///of actions which is necessary to avoid recursive actions.
            newImage->clearBitmap();
        }
        
        ///if the plugin is an identity we just inserted in the cache the identity params, we can now return.
        if (identity) {
            ///don't return the empty allocated image but the input effect image instead!
            return image;
        }
        image = newImage;
        downscaledImage = image;
        
#pragma message WARN("supportsRenderScale() always returns true for OFX") // see doc for OfxEffectInstance::supportsRenderScale()
        if (!supportsRenderScale() && args.mipMapLevel != 0) {
            ///Allocate the upscaled image
            int mipMapLevel = 0;
            RectI bounds;
            rod.toPixelEnclosing(mipMapLevel, &bounds);
            image.reset(new Natron::Image(args.components, rod, bounds, mipMapLevel, args.bitdepth));
        }
        
        
        assert(cachedImgParams);

    } else {
#ifdef NATRON_LOG
        Natron::Log::print(QString("The image was found in the NodeCache with the following hash key: "+
                                   QString::number(key.getHash())).toStdString());
#endif
        assert(cachedImgParams);
        assert(image);
        

        ///if it was cached, first thing to check is to see if it is an identity
        int inputNbIdentity = cachedImgParams->getInputNbIdentity();
        if (inputNbIdentity != -1) {
            SequenceTime inputTimeIdentity = cachedImgParams->getInputTimeIdentity();
            RectD canonicalRoI;
            assert(!rod.isNull());
            args.roi.toCanonical(args.mipMapLevel, rod, &canonicalRoI);
            RoIMap inputsRoI;
            inputsRoI.insert(std::make_pair(input_other_thread(inputNbIdentity), canonicalRoI));
            Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,
                                                        inputsRoI,
                                                        rod,
                                                        args.time,
                                                        args.view,
                                                        args.scale,
                                                        args.mipMapLevel,
                                                        args.isSequentialRender,
                                                        args.isRenderUserInteraction,
                                                        byPassCache,
                                                        nodeHash,
                                                        0,
                                                        args.channelForAlpha,
                                                        true,
                                                        inputTimeIdentity,
                                                        inputNbIdentity);
            Natron::ImageComponents inputPrefComps;
            Natron::ImageBitDepth inputPrefDepth;
            Natron::EffectInstance* inputEffectIdentity = input_other_thread(inputNbIdentity);
            if (inputEffectIdentity) {
                inputEffectIdentity->getPreferredDepthAndComponents(-1, &inputPrefComps, &inputPrefDepth);
                boost::shared_ptr<Image> ret =  getImage(inputNbIdentity, inputTimeIdentity,
                                                         args.scale, args.view, NULL, inputPrefComps, inputPrefDepth, true);
                ///Clear input images pointer because getImage has stored ret .
                _imp->clearInputImagePointers();
                return ret;
            } else {
                return boost::shared_ptr<Image>();
            }
        }
        
#ifdef NATRON_DEBUG
        ///If the precomputed rod parameter was set, assert that it is the same than the image in cache.
        if (inputNbIdentity != -1 && !args.preComputedRoD.isNull()) {
            assert(args.preComputedRoD == cachedImgParams->getRoD());
        }
#endif

        ///For effects that don't support the render scale we have to upscale this cached image,
        ///render the parts we are interested in and then downscale again
        ///Before doing that we verify if everything we want is already rendered in which case we
        ///dont degrade the image
        //#pragma message WARN("supportsRenderScale() always returns true for OFX") // see doc for OfxEffectInstance::supportsRenderScale()
        if (!supportsRenderScale() && args.mipMapLevel != 0) {
            
            RectI intersection;
            args.roi.intersect(image->getBounds(), &intersection);
            std::list<RectI> rectsRendered = image->getRestToRender(intersection);
            if (rectsRendered.empty()) {
                return image;
            }
        
            downscaledImage = image;

            RectD rod = cachedImgParams->getRoD();
            int mipMapLevel = 0;
            RectI bounds;
            rod.toPixelEnclosing(mipMapLevel, &bounds);
            ///Allocate the upscaled image
            boost::shared_ptr<Natron::Image> upscaledImage(new Natron::Image(args.components, rod, bounds, mipMapLevel, args.bitdepth));
            downscaledImage->scaleBox(downscaledImage->getBounds(),upscaledImage.get());
            image = upscaledImage;
        }
        
    }
    
    ////The lock must be taken here otherwise this could lead to corruption if multiple threads
    ////are accessing the output image.
    assert(imageLock);

    ///If we reach here, it can be either because the image is cached or not, either way
    ///the image is NOT an identity, and it may have some content left to render.
    EffectInstance::RenderRoIStatus renderRetCode = renderRoIInternal(args.time, args.scale,args.mipMapLevel,
                                                                      args.view, args.roi, rod, cachedImgParams, image,
                                                                      downscaledImage,args.isSequentialRender,
                                                                      args.isRenderUserInteraction ,byPassCache,nodeHash,
                                                                      args.channelForAlpha);
    
    
    if (aborted()) {
        //if render was aborted, remove the frame from the cache as it contains only garbage
        appPTR->removeFromNodeCache(image);
    } else if (renderRetCode == eImageRenderFailed) {
        throw std::runtime_error("Rendering Failed");
    }
    
    {
        ///flag that this is the last image we rendered
        QMutexLocker l(&_imp->lastRenderArgsMutex);
        _imp->lastRenderHash= nodeHash;
        _imp->lastImage = downscaledImage;
    }
    
#ifdef NATRON_LOG
    Natron::Log::endFunction(getName(),"renderRoI");
#endif

    return downscaledImage;
}


void
EffectInstance::renderRoI(SequenceTime time,
                          const RenderScale& scale,
                          unsigned int mipMapLevel,
                          int view,
                          const RectI& renderWindow,
                          const RectD& rod, //!< effect rod in canonical coords
                          const boost::shared_ptr<const ImageParams>& cachedImgParams,
                          const boost::shared_ptr<Image>& image,
                          const boost::shared_ptr<Image>& downscaledImage,
                          bool isSequentialRender,
                          bool isRenderMadeInResponseToUserInteraction,
                          bool byPassCache,
                          U64 nodeHash)
{
   EffectInstance::RenderRoIStatus renderRetCode = renderRoIInternal(time, scale,mipMapLevel, view, renderWindow, rod, cachedImgParams, image,downscaledImage,isSequentialRender,isRenderMadeInResponseToUserInteraction, byPassCache,nodeHash,3);
    if (renderRetCode == eImageRenderFailed && !aborted()) {
        throw std::runtime_error("Rendering Failed");
    }
}

EffectInstance::RenderRoIStatus
EffectInstance::renderRoIInternal(SequenceTime time,
                                  const RenderScale& scale,
                                  unsigned int mipMapLevel,
                                  int view,
                                  const RectI& renderWindow, //!< seems to be in downscaledImage's pixel coordinates ??
                                  const RectD& rod, //!< effect rod in canonical coords
                                  const boost::shared_ptr<const ImageParams>& cachedImgParams,
                                  const boost::shared_ptr<Image>& image,
                                  const boost::shared_ptr<Image>& downscaledImage,
                                  bool isSequentialRender,
                                  bool isRenderMadeInResponseToUserInteraction,
                                  bool byPassCache,
                                  U64 nodeHash,
                                  int channelForAlpha)
{
    EffectInstance::RenderRoIStatus retCode;

    ///First off check if the requested components and bitdepth are supported by the output clip
    Natron::ImageBitDepth outputDepth ;
    Natron::ImageComponents outputComponents;
    getPreferredDepthAndComponents(-1, &outputComponents, &outputDepth);
    bool imageConversionNeeded = outputComponents != image->getComponents() || outputDepth != image->getBitDepth();
    
    assert(isSupportedBitDepth(outputDepth) && isSupportedComponent(-1, outputComponents));
    
    ///This flag is relevant only when the mipMapLevel is different than 0. We use it to determine
    ///wether the plug-in should render in the full scale image, and then we downscale afterwards or
    ///if the plug-in can just use the downscaled image to render.
#pragma message WARN("supportsRenderScale() always returns true for OFX - renderROI should try rendering twice instead (once with rs<1, once with rs=1)") // see doc for OfxEffectInstance::supportsRenderScale()
    bool useFullResImage = (!supportsRenderScale() && mipMapLevel != 0);
    
    ///The image and downscaled image are pointing to the same image in 2 cases:
    ///1) Proxy mode is turned off
    ///2) Proxy mode is turned on but plug-in supports render scale
    ///Subsequently the image and downscaled image are different only if the plug-in
    ///does not support the render scale and the proxy mode is turned on.
    assert((image == downscaledImage && (supportsRenderScale() || mipMapLevel == 0)) ||
           (image != downscaledImage && !supportsRenderScale() && mipMapLevel != 0));
    
    
    ///Add the window to the project's available formats if the effect is a reader
    ///This is the only reliable place where I could put these lines...which don't seem to feel right here.
    ///Plus setOrAddProjectFormat will actually set the project format the first time we read an image in the project
    ///hence ask for a new render... which can be expensive!
    ///Any solution how to work around this ?
    if (isReader()) {
        Format frmt;
        frmt.set(cachedImgParams->getRoD());
        ///FIXME: what about the pixel aspect ratio ?
        getApp()->getProject()->setOrAddProjectFormat(frmt);
    }


    
    ///We check what is left to render.
    
    ///intersect the image render window to the actual image region of definition.
    ///Intersection will be in pixel coordinates (downscaled).
    RectI intersection;
    
    // make sure the renderWindow falls within the image bounds
    renderWindow.intersect(image->getBounds(), &intersection);
 
    /// If the list is empty then we already rendered it all
    /// Each rect in this list is in pixel coordinate (downscaled)
    std::list<RectI> rectsToRender = downscaledImage->getRestToRender(intersection);
    
    ///if the effect doesn't support tiles and it has something left to render, just render the rod again
    ///note that it should NEVER happen because if it doesn't support tiles in the first place, it would
    ///have rendered the rod already.
    if (!supportsTiles() && !rectsToRender.empty()) {
        ///if the effect doesn't support tiles, just render the whole rod again even though
        rectsToRender.clear();
        rectsToRender.push_back(downscaledImage->getBounds());
    }
#ifdef NATRON_LOG
    else if (rectsToRender.empty()) {
        Natron::Log::print(QString("Everything is already rendered in this image.").toStdString());
    }
#endif
    
    ///Here we fetch the age of the roto context if there's any to pass it through
    ///the tree and remember it when the plugin calls getImage() afterwards
    boost::shared_ptr<RotoContext> rotoContext = _node->getRotoContext();
    U64 rotoAge = rotoContext ? rotoContext->getAge() : 0;
    
    Natron::Status renderStatus = StatOK;
    
    if (rectsToRender.empty()) {
        retCode = EffectInstance::eImageAlreadyRendered;
    } else {
        retCode = EffectInstance::eImageRendered;
    }

    
    ///These are the image passed to the plug-in to render
    /// - fullscaleMappedImage is the fullscale image remapped to what the plugin can support (components/bitdepth)
    /// - downscaledMappedImage is the downscaled image remapped to what the plugin can support (components/bitdepth wise)
    /// - fullscaleMappedImage is pointing to "image" if the plug-in does support the renderscale, meaning we don't use it.
    /// - Similarily downscaledMappedImage is pointing to "downscaledImage" if the plug-in doesn't support the render scale.
    ///
    /// - renderMappedImage is what is given to the plug-in to render the image into,it is mapped to an image that the plug-in
    ///can render onto (good scale, good components, good bitdepth)
    ///
    /// These are the possible scenarios:
    /// - 1) Plugin doesn't need remapping and doesn't need downscaling
    ///    * We render in downscaledImage always, all image pointers point to it.
    /// - 2) Plugin doesn't need remapping but needs downscaling (doesn't support the renderscale)
    ///    * We render in fullScaleImage, fullscaleMappedImage points to it and then we downscale into downscaledImage.
    ///    * renderMappedImage points to fullScaleImage
    /// - 3) Plugin needs remapping (doesn't support requested components or bitdepth) but doesn't need downscaling
    ///    * renderMappedImage points to downscaledMappedImage
    ///    * We render in downscaledMappedImage and then convert back to downscaledImage with requested comps/bitdepth
    /// - 4) Plugin needs remapping and downscaling
    ///    * renderMappedImage points to fullScaleMappedImage
    ///    * We render in fullScaledMappedImage, then convert into "image" and then downscale into downscaledImage.
    boost::shared_ptr<Image> fullScaleMappedImage, downscaledMappedImage, renderMappedImage;
    if (!rectsToRender.empty()) {
        if (imageConversionNeeded) {
            if (useFullResImage) {
                // TODO: as soon as partial images are supported (RoD != bounds): no need to allocate a full image...
                // one rectangle should be allocated for each rendered rectangle
                int mipMapLevel = image->getMipMapLevel();
                RectD rod = image->getRoD();
                RectI bounds;
                rod.toPixelEnclosing(mipMapLevel, &bounds);
                fullScaleMappedImage.reset(new Image(outputComponents, rod, bounds, mipMapLevel, outputDepth));
                downscaledMappedImage = downscaledImage;
            } else {
                int mipMapLevel = downscaledImage->getMipMapLevel();
                RectD rod = downscaledImage->getRoD();
                RectI bounds;
                rod.toPixelEnclosing(mipMapLevel, &bounds);
                downscaledMappedImage.reset(new Image(outputComponents, rod, bounds, mipMapLevel, outputDepth));
                fullScaleMappedImage = image;
            }
        } else {
            fullScaleMappedImage = image;
            downscaledMappedImage = downscaledImage;
        }
        renderMappedImage = useFullResImage ? fullScaleMappedImage : downscaledMappedImage;
    }
    
    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        const RectI& downscaledRectToRender = *it; // please leave it as const, copy it if necessary
        
        ///Upscale the RoI to a region in the full scale image so it is in canonical coordinates
        RectD canonicalRectToRender;
        downscaledRectToRender.toCanonical(mipMapLevel, rod, &canonicalRectToRender);

        
#ifdef NATRON_LOG
        Natron::Log::print(QString("Rect left to render in the image... xmin= "+
                                   QString::number(downscaledRectToRender.left())+" ymin= "+
                                   QString::number(downscaledRectToRender.bottom())+ " xmax= "+
                                   QString::number(downscaledRectToRender.right())+ " ymax= "+
                                   QString::number(downscaledRectToRender.top())).toStdString());
#endif
        
        ///the getRegionsOfInterest call will not be cached because it would be unnecessary
        ///To put that information (which depends on the RoI) into the cache. That's why we
        ///store it into the render args (thread-storage) so the getImage() function can retrieve the results.
#pragma message WARN("of course, it should use the effect RoD instead of image->getRoD()!")
        RoIMap inputsRoi = getRegionsOfInterest_public(time, scale, image->getRoD(), canonicalRectToRender, view);
        
        ///There cannot be the same thread running 2 concurrent instances of renderRoI on the same effect.
        assert(!_imp->renderArgs.hasLocalData() || !_imp->renderArgs.localData()._validArgs);

        ///If the effect is a writer, byPassCache was set to true to make sure the image
        ///we got from the cache would get its bitmap cleared (indicating we would have to call render again)
        ///but for the render args, we set byPassCache to false otherwise each input will not benefit of the
        ///cache, which would drastically reduce effiency.
        if (isWriter()) {
            assert(byPassCache);
            byPassCache = false;
        }
        
        ///The scoped args will maintain the args set for this thread during the
        ///whole time the render action is called, so they can be fetched in the
        ///getImage() call.
        /// @see EffectInstance::getImage
        Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,
                                                    inputsRoi,
                                                    rod,
                                                    time,
                                                    view,
                                                    scale,
                                                    mipMapLevel,
                                                    isSequentialRender,
                                                    isRenderMadeInResponseToUserInteraction,
                                                    byPassCache,
                                                    nodeHash,
                                                    rotoAge,
                                                    channelForAlpha,
                                                    false, //< if we reached here the node is not an identity!
                                                    0.,
                                                    -1);
        const RenderArgs& args = scopedArgs.getArgs();
    
       
        ///Get the frames needed.
        const FramesNeededMap& framesNeeeded = cachedImgParams->getFramesNeeded();
        
        ///We render each input first and stash their image in the inputImages list
        ///in order to maintain a shared_ptr use_count > 1 so the cache doesn't attempt
        ///to remove them.
        std::list< boost::shared_ptr<Natron::Image> > inputImages;

        for (FramesNeededMap::const_iterator it2 = framesNeeeded.begin(); it2 != framesNeeeded.end(); ++it2) {
            
            ///We have to do this here because the enabledness of a mask is a feature added by Natron.
            bool inputIsMask = isInputMask(it2->first);
            if (inputIsMask && !isMaskEnabled(it2->first)) {
                continue;
            }
            
            EffectInstance* inputEffect = input_other_thread(it2->first);
            if (inputEffect) {
                
                ///What region are we interested in for this input effect ? (This is in Canonical coords)
                RoIMap::iterator foundInputRoI = inputsRoi.find(inputEffect);
                assert(foundInputRoI != inputsRoi.end());
                
                ///Convert to pixel coords the RoI
                if (foundInputRoI->second.isInfinite()) {
                    throw std::runtime_error(std::string("Plugin ") + this->pluginLabel() + " asked for an infinite region of interest!");
                }

                RectI inputRoIPixelCoords;
                foundInputRoI->second.toPixelEnclosing(scale, &inputRoIPixelCoords);
                
                ///Notify the node that we're going to render something with the input
                assert(it2->first != -1); //< see getInputNumber
                _node->notifyInputNIsRendering(it2->first);
                
                ///For all frames requested for this node, render the RoI requested.
                for (U32 range = 0; range < it2->second.size(); ++range) {
                    for (U32 f = it2->second[range].min; f <= it2->second[range].max; ++f) {
                        
                        Natron::ImageComponents inputPrefComps;
                        Natron::ImageBitDepth inputPrefDepth;
                        inputEffect->getPreferredDepthAndComponents(-1, &inputPrefComps, &inputPrefDepth);
                        
                        int channelForAlphaInput = inputIsMask ? getMaskChannel(it2->first) : 3;
                        
                        boost::shared_ptr<Natron::Image> inputImg =
                        inputEffect->renderRoI(RenderRoIArgs(f, //< time
                                                             scale, //< scale
                                                             mipMapLevel, //< mipmapLevel (redundant with the scale)
                                                             view, //< view
                                                             inputRoIPixelCoords, //< roi in pixel coordinates
                                                             isSequentialRender, //< sequential render ?
                                                             isRenderMadeInResponseToUserInteraction, // < user interaction ?
                                                             byPassCache, //< look-up the cache for existing images ?
                                                             RectD(),// < did we precompute any RoD to speed-up the call ?
                                                             inputPrefComps, //< requested comps
                                                             inputPrefDepth,
                                                             channelForAlphaInput)); //< requested bitdepth
                        
#pragma message WARN("TODO: if supportsMultiResolution is false, check that all inputs have the same rod and their (x1,y1) is at the origin")
                        if (inputImg) {
                            inputImages.push_back(inputImg);
                        }
                    }
                }
                _node->notifyInputNIsFinishedRendering(it2->first);
                
                if (aborted()) {
                    //if render was aborted, remove the frame from the cache as it contains only garbage
                    appPTR->removeFromNodeCache(image);
                    return eImageRendered;
                }
            }
        }
        
        ///if the node has a roto context, pre-render the roto mask too
        boost::shared_ptr<RotoContext> rotoCtx = _node->getRotoContext();
        if (rotoCtx) {

            boost::shared_ptr<Natron::Image> mask = rotoCtx->renderMask(downscaledRectToRender,
                                                                        image->getComponents(),
                                                                        nodeHash,
                                                                        rotoAge,
                                                                        rod,
                                                                        time,
                                                                        getBitDepth(),
                                                                        view,
                                                                        mipMapLevel,
                                                                        byPassCache);
            assert(mask);
            inputImages.push_back(mask);
        }
        
        ///notify the node we're starting a render
        _node->notifyRenderingStarted();
        
        ///We only need to call begin if we've not already called it.
        bool callBegin = false;
        
        ///neer call beginsequenceRender here if the render is sequential
        if (!args._isSequentialRender)
        {
            if (!_imp->beginEndRenderCount.hasLocalData()) {
                callBegin = true;
                _imp->beginEndRenderCount.localData() = 0;
            } else if (_imp->beginEndRenderCount.localData() == 0) {
                callBegin = true;
            }
        }
        if (callBegin) {
            if (beginSequenceRender_public(time, time, 1, !appPTR->isBackground(), scale,isSequentialRender,
                                           isRenderMadeInResponseToUserInteraction,view) == StatFailed) {
                renderStatus = StatFailed;
                break;
            }
        }
        
        /*depending on the thread-safety of the plug-in we render with a different
         amount of threads*/
        EffectInstance::RenderSafety safety = renderThreadSafety();
        
        ///if the project lock is already locked at this point, don't start any other thread
        ///as it would lead to a deadlock when the project is loading.
        ///Just fall back to Fully_safe
        int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
        if (safety == FULLY_SAFE_FRAME) {
            
            if (nbThreads == -1 || nbThreads == 1 || (nbThreads == 0 && QThread::idealThreadCount() == 1) ||
                QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount()) {
                safety = FULLY_SAFE;
            } else {
                if (!getApp()->getProject()->tryLock()) {
                    safety = FULLY_SAFE;
                } else {
                    getApp()->getProject()->unlock();
                }
            }
        }

        switch (safety) {
            case FULLY_SAFE_FRAME: { // the plugin will not perform any per frame SMP threading
                // we can split the frame in tiles and do per frame SMP threading (see kOfxImageEffectPluginPropHostFrameThreading)
                if (nbThreads == 0) {
                    nbThreads = QThreadPool::globalInstance()->maxThreadCount();
                }
                std::vector<RectI> splitRects = RectI::splitRectIntoSmallerRect(downscaledRectToRender, nbThreads);
                // the bitmap is checked again at the beginning of EffectInstance::tiledRenderingFunctor()
                QFuture<Natron::Status> ret = QtConcurrent::mapped(splitRects,
                                                                   boost::bind(&EffectInstance::tiledRenderingFunctor, this,
                                                                               args,
                                                                               _1,
                                                                               downscaledImage,
                                                                               image,
                                                                               downscaledMappedImage,
                                                                               fullScaleMappedImage,
                                                                               renderMappedImage));
                ret.waitForFinished();
                
                bool callEndRender = false;
                ///never call endsequence render here if the render is sequential
                if (!args._isSequentialRender) {
                    assert(_imp->beginEndRenderCount.hasLocalData());
                    if (_imp->beginEndRenderCount.localData() ==  1) {
                        callEndRender = true;
                    }
                }
                if (callEndRender) {
                    if (endSequenceRender_public(time, time, time, false, scale,
                                                 isSequentialRender,isRenderMadeInResponseToUserInteraction,view) == StatFailed) {
                        renderStatus = StatFailed;
                        break;
                    }
                }
                for (QFuture<Natron::Status>::const_iterator it2 = ret.begin(); it2!=ret.end(); ++it2) {
                    if ((*it2) == Natron::StatFailed) {
                        renderStatus = *it2;
                        break;
                    }
                }
            }   break;
                
            case INSTANCE_SAFE: // indicating that any instance can have a single 'render' call at any one time,
            case FULLY_SAFE:    // indicating that any instance of a plugin can have multiple renders running simultaneously
            case UNSAFE: { // indicating that only a single 'render' call can be made at any time amoung all instances
                // INSTANCE_SAFE means that there is at most one render per instance
                // NOTE: the per-instance lock should probably be shared between
                // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
                // and read-write on it.
                // It is probably safer to assume that several clones may write to the same output image only in the FULLY_SAFE case.

                // FULLY_SAFE means that there is only one render per FRAME for a given instance take a per-frame lock here (the map of per-frame
                ///locks belongs to an instance)

                QMutexLocker l((safety == INSTANCE_SAFE) ? &getNode()->getRenderInstancesSharedMutex() :
                               ((safety == FULLY_SAFE) ? &getNode()->getFrameMutex(time) :
                                appPTR->getMutexForPlugin(pluginID().c_str())));

                renderStatus = tiledRenderingFunctor(args,
                                                     downscaledRectToRender,
                                                     downscaledImage,
                                                     image,
                                                     downscaledMappedImage,
                                                     fullScaleMappedImage,
                                                     renderMappedImage);
            }   break;
        }

        ///notify the node we've finished rendering
        _node->notifyRenderingEnded();

        if (renderStatus != StatOK) {
            break;
        }
    }
    
    if (renderStatus != StatOK) {
        retCode = eImageRenderFailed;
    }
    //we released the input images and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();
    return retCode;

}

Natron::Status
EffectInstance::tiledRenderingFunctor(const RenderArgs& args,
                                      const RectI& downscaledRectToRender,
                                      const boost::shared_ptr<Natron::Image>& downscaledImage,
                                      const boost::shared_ptr<Natron::Image>& fullScaleImage,
                                      const boost::shared_ptr<Natron::Image>& downscaledMappedImage,
                                      const boost::shared_ptr<Natron::Image>& fullScaleMappedImage,
                                      const boost::shared_ptr<Natron::Image>& renderMappedImage)
{
    assert(downscaledMappedImage && fullScaleMappedImage && renderMappedImage);
    Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,args);
    const SequenceTime time = args._time;
    const RenderScale scale = args._scale;
    int mipMapLevel = args._mipMapLevel;
    const int view = args._view;
    const bool isSequentialRender = args._isSequentialRender;
    const bool isRenderResponseToUserInteraction = args._isRenderResponseToUserInteraction;
    const int channelForAlpha = args._channelForAlpha;

    //#pragma message WARN("supportsRenderScale() always returns true for OFX") // see doc for OfxEffectInstance::supportsRenderScale()
    const bool useFullResImage = (!supportsRenderScale() && mipMapLevel != 0);

    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
    const RectI& renderBounds = renderMappedImage->getBounds();
    assert(renderBounds.x1 <= downscaledRectToRender.x1 && downscaledRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= downscaledRectToRender.y1 && downscaledRectToRender.y2 <= renderBounds.y2);

    // check the bitmap!
    const RectI downscaledRectToRenderMinimal = downscaledMappedImage->getMinimalRect(downscaledRectToRender);

    assert(renderBounds.x1 <= downscaledRectToRenderMinimal.x1 && downscaledRectToRenderMinimal.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= downscaledRectToRenderMinimal.y1 && downscaledRectToRenderMinimal.y2 <= renderBounds.y2);


    RectI renderRectToRender; // rectangle to render, in renderMappedImage's pixel coordinates
    if (useFullResImage) {
        RectD canonicalrenderRectToRender;
        downscaledRectToRenderMinimal.toCanonical(mipMapLevel, args._rod, &canonicalrenderRectToRender);
        canonicalrenderRectToRender.toPixelEnclosing(0, &renderRectToRender);
        renderRectToRender.intersect(renderMappedImage->getBounds(), &renderRectToRender);
    } else {
        renderRectToRender = downscaledRectToRenderMinimal;
    }
    
    if (!renderRectToRender.isNull()) {
        
        Natron::Status st = render_public(time, scale, renderRectToRender, view,
                                          isSequentialRender,
                                          isRenderResponseToUserInteraction,
                                          renderMappedImage);
        if (st != StatOK) {
            return st;
        }

        if (!aborted()) {
            renderMappedImage->markForRendered(renderRectToRender);
        }

        ///copy the rectangle rendered in the full scale image to the downscaled output
        if (useFullResImage) {
            ///First demap the fullScaleMappedImage to the original comps/bitdepth if it needs to
            if (renderMappedImage == fullScaleMappedImage && fullScaleMappedImage != fullScaleImage) {
                renderMappedImage->convertToFormat(renderRectToRender,
                                                   getApp()->getDefaultColorSpaceForBitDepth(renderMappedImage->getBitDepth()),
                                                   getApp()->getDefaultColorSpaceForBitDepth(fullScaleMappedImage->getBitDepth()),
                                                   channelForAlpha, false, true,
                                                   fullScaleImage.get());
            }
            if (mipMapLevel != 0) {
                assert(fullScaleImage != downscaledImage);
                fullScaleImage->downscaleMipMap(renderRectToRender, 0, mipMapLevel, downscaledImage.get());
            }
        } else {
            assert(renderMappedImage == downscaledMappedImage);
            assert(renderRectToRender == downscaledRectToRenderMinimal);
            if (renderMappedImage != downscaledImage) {
                renderMappedImage->convertToFormat(renderRectToRender,
                                                   getApp()->getDefaultColorSpaceForBitDepth(renderMappedImage->getBitDepth()),
                                                   getApp()->getDefaultColorSpaceForBitDepth(downscaledMappedImage->getBitDepth()),
                                                   channelForAlpha, false, true,
                                                   downscaledImage.get());
            }
        }
    }

    return StatOK;
}

void
EffectInstance::openImageFileKnob()
{
    const std::vector< boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
            boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
            assert(fk);
            if (fk->isInputImageFile()) {
                std::string file = fk->getValue();
                if (file.empty()) {
                    fk->open_file();
                }
                break;
            }
        } else if(knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if (fk->isOutputImageFile()) {
                std::string file = fk->getValue();
                if(file.empty()){
                    fk->open_file();
                }
                break;
            }

        }
    }
}


void
EffectInstance::createKnobDynamically()
{
    _node->createKnobDynamically();
}

void
EffectInstance::evaluate(KnobI* knob,
                         bool isSignificant,
                         Natron::ValueChangedReason reason)
{
    
    assert(_node);
    
    ////If the node is currently modifying its input, to ask for a render
    ////because at then end of the inputChanged handler, it will ask for a refresh
    ////and a rebuild of the inputs tree.
    if (_node->duringInputChangedAction()) {
        return;
    }
    
    if (getApp()->getProject()->isLoadingProject()) {
        return;
    }
    
    
    Button_Knob* button = dynamic_cast<Button_Knob*>(knob);

    /*if this is a writer (openfx or built-in writer)*/
    if (isWriter()) {
        /*if this is a button and it is a render button,we're safe to assume the plug-ins wants to start rendering.*/
        if (button) {
            if (button->isRenderButton()) {
                QStringList list;
                list << getName().c_str();
                
                std::string sequentialNode;
                if (_node->hasSequentialOnlyNodeUpstream(sequentialNode)) {
                    if (_node->getApp()->getProject()->getProjectViewsCount() > 1) {
                        Natron::StandardButton answer = Natron::questionDialog(QObject::tr("Render").toStdString(), sequentialNode + QObject::tr(" can only "
                                                                               "render in sequential mode. Due to limitations in the "
                                                                               "OpenFX standard that means that %1"
                                                                               " will not be able "
                                                                               "to render all the views of the project. "
                                                                               "Only the main view of the project will be rendered, you can "
                                                                               "change the main view in the project settings. Would you like "
                                                                               "to continue ?").arg(NATRON_APPLICATION_NAME).toStdString());
                        if (answer != Natron::Yes) {
                            return;
                        }
                    }
                }
                
                getApp()->startWritersRendering(list);
                return;
            }
        }
    }
    
    ///increments the knobs age following a change
    if (!button) {
        _node->incrementKnobsAge();
    }
    
    std::list<ViewerInstance* > viewers;
    _node->hasViewersConnected(&viewers);
    bool forcePreview = getApp()->getProject()->isAutoPreviewEnabled();
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it) {
        if (isSignificant) {
            if (button) {
                ///if the parameter is a button, force an update of the tree since it could be an analysis
                (*it)->updateTreeAndRender();
            } else {
                (*it)->refreshAndContinueRender(forcePreview,reason == Natron::USER_EDITED);
            }
            
        } else {
            (*it)->redrawViewer();
        }
    }
    
    ///if the parameter is a button, force an update of the tree since it could be an analysis
    if (button && viewers.empty()) {
        _node->updateRenderInputsRecursive();
    }
    
    getNode()->refreshPreviewsRecursively();
}

bool
EffectInstance::message(Natron::MessageType type,
                        const std::string& content) const
{
    return _node->message(type,content);
}

void
EffectInstance::setPersistentMessage(Natron::MessageType type,
                                     const std::string& content)
{
    _node->setPersistentMessage(type, content);
}

void
EffectInstance::clearPersistentMessage()
{
    _node->clearPersistentMessage();
}

int
EffectInstance::getInputNumber(Natron::EffectInstance* inputEffect) const
{
    for (int i = 0; i < maximumInputs(); ++i) {
        if (input_other_thread(i) == inputEffect) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Does this effect supports rendering at a different scale than 1 ?
 * There is no OFX property for this purpose. The only solution found for OFX is that if a isIdentity
 * with renderscale != 1 fails, the host retries with renderscale = 1 (and upscaled images).
 * If the renderScale support was not set, this throws an exception.
 **/
bool
EffectInstance::supportsRenderScale() const
{
    if (_imp->supportsRenderScale == eSupportsMaybe) {
        qDebug() << "EffectInstance::supportsRenderScale not set";
#pragma message WARN("EffectInstance::supportsRenderScale should be set before testing, or use supportsRenderScaleMaybe()")
        // uncomment the following as soon as it is fixed:
        //throw std::runtime_error("supportsRenderScale not set");
        return true;
    }
    return _imp->supportsRenderScale == eSupportsYes;
}

EffectInstance::SupportsEnum
EffectInstance::supportsRenderScaleMaybe() const
{
    QMutexLocker l(&_imp->supportsRenderScaleMutex);
    return _imp->supportsRenderScale;
}

void
EffectInstance::setSupportsRenderScaleMaybe(EffectInstance::SupportsEnum s) const
{
    QMutexLocker l(&_imp->supportsRenderScaleMutex);
    _imp->supportsRenderScale = s;
}

void
EffectInstance::setOutputFilesForWriter(const std::string& pattern)
{
    if (!isWriter()) {
        return;
    }
    
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if (fk->isOutputImageFile()) {
                fk->setValue(pattern,0);
                break;
            }
        }
    }
}

PluginMemory*
EffectInstance::newMemoryInstance(size_t nBytes)
{
    
    PluginMemory* ret = new PluginMemory(_node->getLiveInstance()); //< hack to get "this" as a shared ptr
    bool wasntLocked = ret->alloc(nBytes);
    assert(wasntLocked);
    return ret;
}

void
EffectInstance::addPluginMemoryPointer(PluginMemory* mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);
    _imp->pluginMemoryChunks.push_back(mem);
}

void
EffectInstance::removePluginMemoryPointer(PluginMemory* mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);
    std::list<PluginMemory*>::iterator it = std::find(_imp->pluginMemoryChunks.begin(),_imp->pluginMemoryChunks.end(),mem);
    if (it != _imp->pluginMemoryChunks.end()) {
        _imp->pluginMemoryChunks.erase(it);
    }
}


void
EffectInstance::registerPluginMemory(size_t nBytes)
{
    _node->registerPluginMemory(nBytes);
}

void
EffectInstance::unregisterPluginMemory(size_t nBytes)
{
    _node->unregisterPluginMemory(nBytes);
}

void
EffectInstance::onAllKnobsSlaved(bool isSlave,
                                 KnobHolder* master)
{
    _node->onAllKnobsSlaved(isSlave,master);
}

void
EffectInstance::onKnobSlaved(const boost::shared_ptr<KnobI>& knob,int dimension,bool isSlave,KnobHolder* master)
{
    _node->onKnobSlaved(knob,dimension,isSlave,master);
}

void
EffectInstance::drawOverlay_public(double scaleX,
                                   double scaleY)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    
    ///Explanation: if the gui is showing a dialog, it was probably during an instanceChanged action
    ///and then this action would be recursive. this is safe to assume the previous action already set
    ///the clip thread storage.
    ///The same applies to when a plug-in set a persistent message, the viewer will repaint overlays as
    ///a result of this action.
    if (getRecursionLevel() == 0) {
        getClipThreadStorageData(time, &view, &mipMapLevel, &rod);
    }
    ///Recursive action, must not call assertActionIsNotRecursive()
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    drawOverlay(scaleX,scaleY,rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
}

bool
EffectInstance::onOverlayPenDown_public(double scaleX,
                                        double scaleY,
                                        const QPointF& viewportPos,
                                        const QPointF& pos)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return false;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    getClipThreadStorageData(time, &view, &mipMapLevel, &rod);

    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenDown(scaleX,scaleY,viewportPos, pos,rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayPenMotion_public(double scaleX,
                                          double scaleY,
                                          const QPointF& viewportPos,
                                          const QPointF& pos)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return false;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    
    ///Explanation: if the gui is showing a dialog, it was probably during an instanceChanged action
    ///and then this action would be recursive. this is safe to assume the previous action already set
    ///the clip thread storage
    ///The same applies to when a plug-in set a persistent message, the viewer will repaint overlays as
    ///a result of this action.
    if (getRecursionLevel() == 0) {
        getClipThreadStorageData(time, &view, &mipMapLevel, &rod);
    }

    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenMotion(scaleX,scaleY,viewportPos, pos,rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    //Don't chek if render is needed on pen motion, wait for the pen up
    //checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayPenUp_public(double scaleX,
                                      double scaleY,
                                      const QPointF& viewportPos,
                                      const QPointF& pos)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return false;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    getClipThreadStorageData(time, &view, &mipMapLevel, &rod);

    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenUp(scaleX,scaleY,viewportPos, pos,rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayKeyDown_public(double scaleX,
                                        double scaleY,
                                        Natron::Key key,
                                        Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return false;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    getClipThreadStorageData(time, &view, &mipMapLevel, &rod);

    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyDown(scaleX,scaleY,key, modifiers,rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayKeyUp_public(double scaleX,
                                      double scaleY,
                                      Natron::Key key,
                                      Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return false;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    getClipThreadStorageData(time, &view, &mipMapLevel, &rod);

    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyUp(scaleX, scaleY, key, modifiers, rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;

}

bool
EffectInstance::onOverlayKeyRepeat_public(double scaleX,
                                          double scaleY,
                                          Natron::Key key,
                                          Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return false;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    getClipThreadStorageData(time, &view, &mipMapLevel, &rod);

    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyRepeat(scaleX,scaleY,key, modifiers,rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;

}

bool
EffectInstance::onOverlayFocusGained_public(double scaleX,
                                            double scaleY)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return false;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    
    ///Explanation: if the gui is showing a dialog, it was probably during an instanceChanged action
    ///and then this action would be recursive. this is safe to assume the previous action already set
    ///the clip thread storage
    ///The same applies to when a plug-in set a persistent message, the viewer will repaint overlays as
    ///a result of this action.
    if (getRecursionLevel() == 0) {
        getClipThreadStorageData(time, &view, &mipMapLevel, &rod);
    }

    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayFocusGained(scaleX,scaleY,rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;

}

bool
EffectInstance::onOverlayFocusLost_public(double scaleX,
                                          double scaleY)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    if (!hasOverlay()) {
        return false;
    }
    SequenceTime time = getApp()->getTimeLine()->currentFrame();
    int view ;
    unsigned int mipMapLevel;
    RectD rod;
    ///Explanation: if the gui is showing a dialog, it was probably during an instanceChanged action
    ///and then this action would be recursive. this is safe to assume the previous action already set
    ///the clip thread storage
    ///The same applies to when a plug-in set a persistent message, the viewer will repaint overlays as
    ///a result of this action.
    if (getRecursionLevel() == 0) {
        getClipThreadStorageData(time, &view, &mipMapLevel, &rod);
    }

    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayFocusLost(scaleX,scaleY,rod);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;

}

bool
EffectInstance::isDoingInteractAction() const
{
    QReadLocker l(&_imp->duringInteractActionMutex);
    return _imp->duringInteractAction;
}

Natron::Status
EffectInstance::render_public(SequenceTime time,
                              const RenderScale& scale,
                              const RectI& roi,
                              int view,
                              bool isSequentialRender,
                              bool isRenderResponseToUserInteraction,
                              boost::shared_ptr<Natron::Image> output)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    
    ///Clear any previous input image which may be left
    _imp->clearInputImagePointers();
    
    Natron::Status stat;
    
    try {
        stat = render(time, scale, roi, view, isSequentialRender, isRenderResponseToUserInteraction, output);
    } catch (const std::exception & e) {
        ///Also clear images when catching an exception
        _imp->clearInputImagePointers();
        decrementRecursionLevel();
        throw e;
    }
    
    ///Clear any previous input image which may be left
    _imp->clearInputImagePointers();

    decrementRecursionLevel();
    return stat;
}

bool
EffectInstance::isIdentity_public(SequenceTime time,
                                  const RenderScale& scale,
                                  const RectD& rod, //!< image rod in canonical coordinates
                                  int view,
                                  SequenceTime* inputTime,
                                  int* inputNb)
{
    if (supportsRenderScaleMaybe() == eSupportsNo && (scale.x != 1. || scale.y != 1.)) {
        qDebug() << "EffectInstance::isIdentity_public() called with renderscale !=1, but plugin does not support it";
    }
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    bool ret = false;
    if (_node->isNodeDisabled()) {
        ret = true;
        *inputTime = time;
        *inputNb = -1;
        ///we forward this node to the last connected non-optional input
        ///if there's only optional inputs connected, we return the last optional input
        int lastOptionalInput = -1;
        for (int i = maximumInputs() - 1; i >= 0; --i) {
            bool optional = isInputOptional(i);
            if (!optional && _node->input_other_thread(i)) {
                *inputNb = i;
                break;
            } else if (optional && lastOptionalInput == -1) {
                lastOptionalInput = i;
            }
        }
        if (*inputNb == -1) {
            *inputNb = lastOptionalInput;
        }
        
    } else {
        /// Don't call isIdentity if plugin is sequential only.
        if (getSequentialPreference() != Natron::EFFECT_ONLY_SEQUENTIAL) {
            ret = isIdentity(time, scale, rod, view, inputTime, inputNb);
        }
    }
    decrementRecursionLevel();
    return ret;
}

Natron::Status
EffectInstance::getRegionOfDefinition_public(SequenceTime time,
                                             const RenderScale& scale,
                                             int view,
                                             RectD* rod,
                                             bool* isProjectFormat)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    Natron::Status ret;

    try {
        ret = getRegionOfDefinition(time, scale, view, rod);
    } catch (const std::exception& e) {
        decrementRecursionLevel();
        throw e;
    }
    assert(ret != StatOK || (rod->x1 <= rod->x2 && rod->y1 <= rod->y2));
    
    decrementRecursionLevel();
    *isProjectFormat = ifInfiniteApplyHeuristic(time, scale, view, rod);
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
    return ret;
}

EffectInstance::RoIMap
EffectInstance::getRegionsOfInterest_public(SequenceTime time,
                                            const RenderScale& scale,
                                            const RectD& outputRoD, //!< effect RoD in canonical coordinates
                                            const RectD& renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            int view)
{
    assertActionIsNotRecursive();
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);
    incrementRecursionLevel();
    EffectInstance::RoIMap ret;
    try {
        ret = getRegionsOfInterest(time, scale, outputRoD, renderWindow, view);
    } catch (const std::exception& e) {
        decrementRecursionLevel();
        throw e;
    }
    decrementRecursionLevel();
    return ret;
}

EffectInstance::FramesNeededMap
EffectInstance::getFramesNeeded_public(SequenceTime time)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    EffectInstance::FramesNeededMap ret = getFramesNeeded(time);
    decrementRecursionLevel();
    return ret;
}

void
EffectInstance::getFrameRange_public(SequenceTime *first,
                                     SequenceTime *last)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    getFrameRange(first, last);
    decrementRecursionLevel();
}

Natron::Status
EffectInstance::beginSequenceRender_public(SequenceTime first,
                                           SequenceTime last,
                                           SequenceTime step,
                                           bool interactive,
                                           const RenderScale& scale,
                                           bool isSequentialRender,
                                           bool isRenderResponseToUserInteraction,
                                           int view)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    {
        if (!_imp->beginEndRenderCount.hasLocalData()) {
            _imp->beginEndRenderCount.localData() = 1;
        } else {
            ++_imp->beginEndRenderCount.localData();
        }
    }
    Natron::Status ret = beginSequenceRender(first, last, step, interactive, scale,
                                             isSequentialRender, isRenderResponseToUserInteraction, view);
    decrementRecursionLevel();
    return ret;
}

Natron::Status
EffectInstance::endSequenceRender_public(SequenceTime first,
                                         SequenceTime last,
                                         SequenceTime step,
                                         bool interactive,
                                         const RenderScale& scale,
                                         bool isSequentialRender,
                                         bool isRenderResponseToUserInteraction,
                                         int view)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    {
        assert(_imp->beginEndRenderCount.hasLocalData());
        --_imp->beginEndRenderCount.localData();
        assert(_imp->beginEndRenderCount.localData() >= 0);
    }
    Natron::Status ret = endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, view);
    decrementRecursionLevel();
    return ret;
}


bool
EffectInstance::isSupportedComponent(int inputNb,
                                     Natron::ImageComponents comp) const
{
    return _node->isSupportedComponent(inputNb, comp);
}

Natron::ImageBitDepth
EffectInstance::getBitDepth() const
{
    return _node->getBitDepth();
}

bool
EffectInstance::isSupportedBitDepth(Natron::ImageBitDepth depth) const
{
    return _node->isSupportedBitDepth(depth);
}

Natron::ImageComponents
EffectInstance::findClosestSupportedComponents(int inputNb,
                                               Natron::ImageComponents comp) const
{
    return _node->findClosestSupportedComponents(inputNb,comp);
}

void
EffectInstance::getPreferredDepthAndComponents(int inputNb,
                                               Natron::ImageComponents* comp,
                                               Natron::ImageBitDepth* depth) const
{
    ///find closest to RGBA
    *comp = findClosestSupportedComponents(inputNb, Natron::ImageComponentRGBA);

    ///find deepest bitdepth
    *depth = getBitDepth();
}

int
EffectInstance::getMaskChannel(int inputNb) const
{
    return _node->getMaskChannel(inputNb);
}


bool
EffectInstance::isMaskEnabled(int inputNb) const
{
    return _node->isMaskEnabled(inputNb);
}


void
EffectInstance::onKnobValueChanged(KnobI* /*k*/,
                                   Natron::ValueChangedReason /*reason*/,
                                   SequenceTime /*time*/)
{
}

int
EffectInstance::getCurrentFrameRecursive() const
{
    if (_imp->renderArgs.hasLocalData() && _imp->renderArgs.localData()._validArgs) {
        return _imp->renderArgs.localData()._time;
    }
    return getApp()->getTimeLine()->currentFrame();
}


int
EffectInstance::getCurrentViewRecursive() const
{
    if (_imp->renderArgs.hasLocalData() && _imp->renderArgs.localData()._validArgs) {
        return _imp->renderArgs.localData()._view;
    }
    ///If we reach here, that's means this is not during a call to renderRoI
    ///hence this is probably asked but doesn't matter so just return 0
    return 0;

}


int
EffectInstance::getCurrentMipMapLevelRecursive() const
{
    if (_imp->renderArgs.hasLocalData() && _imp->renderArgs.localData()._validArgs) {
        return _imp->renderArgs.localData()._mipMapLevel;
    }
    
    ///If we reach here, that's means this is not during a call to renderRoI
    ///hence this is probably asked but doesn't matter so just return 0
    return 0;

}

void
EffectInstance::updateCurrentFrameRecursive(int time)
{
    if (_imp->renderArgs.hasLocalData() && _imp->renderArgs.localData()._validArgs) {
        _imp->renderArgs.localData()._time = time;
    }
}

void
EffectInstance::getClipThreadStorageData(SequenceTime time,
                                         int *view,
                                         unsigned int *mipMapLevel,
                                         RectD *outputRoD)
{
    assert(view && mipMapLevel && outputRoD);
    *view = getCurrentViewRecursive();
    *mipMapLevel = getCurrentMipMapLevelRecursive();

    RenderScale scale;
    scale.x = Image::getScaleFromMipMapLevel(*mipMapLevel);
    scale.y = scale.x;
    bool isProjectFormat;
    
    ///we don't care if it fails
    (void)getRegionOfDefinition_public(time, scale, *view, outputRoD, &isProjectFormat);
    assert(outputRoD->x2 >= outputRoD->x1 && outputRoD->y2 >= outputRoD->y1);
}

void
EffectInstance::onKnobValueChanged_public(KnobI* k,
                                          Natron::ValueChangedReason reason,
                                          SequenceTime time)
{
    ///cannot run in another thread.
    assert(QThread::currentThread() == qApp->thread());


    if (isEvaluationBlocked()) {
        return;
    }
    
    _node->onEffectKnobValueChanged(k, reason);
    if (dynamic_cast<KnobHelper*>(k)->isDeclaredByPlugin()) {

        ////We set the thread storage render args so that if the instance changed action
        ////tries to call getImage it can render with good parameters.
        
        RectD rod;
        if (getRecursionLevel() == 0) {
            int view ;
            unsigned int mipMapLevel;
            getClipThreadStorageData(time, &view, &mipMapLevel, &rod);
            assert(rod.x2 >= rod.x1 && rod.y2 >= rod.y1);
            RenderScale scale;
            scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
            scale.y = scale.x;
            
            U64 nodeHash = hash();
            RoIMap inputRois = getRegionsOfInterest_public(time, scale, rod, rod, view);
            
            boost::shared_ptr<RotoContext> roto = _node->getRotoContext();
            U64 rotoAge = 0;
            if (roto) {
                rotoAge = roto->getAge();
            }
            
            SequenceTime identityTime;
            int identityNb;

            bool isIdentity = isIdentity_public(time, scale, rod, view, &identityTime, &identityNb);
            
            ///These args remain valid on the thread storage 'til it gets out of scope
            Implementation::ScopedInstanceChangedArgs args(&_imp->renderArgs,
                                                           time,
                                                           view,
                                                           mipMapLevel,
                                                           rod,
                                                           inputRois,
                                                           false,
                                                           false,
                                                           nodeHash,
                                                           rotoAge,
                                                           isIdentity,
                                                           identityTime,
                                                           identityNb);
            ///Recursive action, must not call assertActionIsNotRecursive()
            incrementRecursionLevel();
            knobChanged(k, reason, rod, view, time);
            decrementRecursionLevel();
        
        } else {
            int view = getCurrentViewRecursive();
            ///Recursive action, must not call assertActionIsNotRecursive()
            incrementRecursionLevel();
            knobChanged(k, reason, rod, view, time);
            decrementRecursionLevel();
        }
        
        ///Clear input images pointers that were stored in getImage() for the main-thread.
        ///This is safe to do so because if this is called while in render() it won't clear the input images
        ///pointers for the render thread. This is helpful for analysis effects which call getImage() on the main-thread
        ///and whose render() function is never called.
        _imp->clearInputImagePointers();
        
    }
    
   
}

void
EffectInstance::clearLastRenderedImage()
{
    {
        QMutexLocker l(&_imp->lastRenderArgsMutex);
        _imp->lastImage.reset();
    }
}


void
EffectInstance::aboutToRestoreDefaultValues()
{
    ///Invalidate the cache by incrementing the age
    _node->incrementKnobsAge();
    
    if (_node->areKeyframesVisibleOnTimeline()) {
        _node->hideKeyframesFromTimeline(true);
    }
    
}

OutputEffectInstance::OutputEffectInstance(boost::shared_ptr<Node> node)
: Natron::EffectInstance(node)
, _videoEngine(node ? new VideoEngine(this) : 0)
, _writerCurrentFrame(0)
, _writerFirstFrame(0)
, _writerLastFrame(0)
, _doingFullSequenceRender()
, _outputEffectDataLock(new QMutex)
, _renderController(0)
{
}

OutputEffectInstance::~OutputEffectInstance()
{
    
    if (_videoEngine) {
        ///Thread must have been killed before.
        assert(!_videoEngine->isThreadRunning());
    }
    delete _outputEffectDataLock;
}

void
OutputEffectInstance::updateTreeAndRender()
{
    _videoEngine->updateTreeAndContinueRender();
}

void
OutputEffectInstance::refreshAndContinueRender(bool forcePreview,
                                               bool abortRender)
{
    _videoEngine->refreshAndContinueRender(forcePreview,abortRender);
}

bool
OutputEffectInstance::ifInfiniteclipRectToProjectDefault(RectD* rod) const
{
    if(!getApp()->getProject()){
        return false;
    }
    /*If the rod is infinite clip it to the project's default*/
    Format projectDefault;
    getRenderFormat(&projectDefault);
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    // an int can not be equal to (or compared to) std::numeric_limits<double>::infinity()
    bool isRodProjctFormat = false;
    if (rod->left() <= kOfxFlagInfiniteMin) {
        rod->set_left(projectDefault.left());
        isRodProjctFormat = true;
    }
    if (rod->bottom() <= kOfxFlagInfiniteMin) {
        rod->set_bottom(projectDefault.bottom());
        isRodProjctFormat = true;
    }
    if (rod->right() >= kOfxFlagInfiniteMax) {
        rod->set_right(projectDefault.right());
        isRodProjctFormat = true;
    }
    if (rod->top() >= kOfxFlagInfiniteMax) {
        rod->set_top(projectDefault.top());
        isRodProjctFormat = true;
    }
    return isRodProjctFormat;
}

void
OutputEffectInstance::renderFullSequence(BlockingBackgroundRender* renderController)
{
    _renderController = renderController;
    assert(pluginID() != "Viewer"); //< this function is not meant to be called for rendering on the viewer
    getVideoEngine()->refreshTree();
    getVideoEngine()->render(-1, //< frame count
                             true, //< seek timeline
                             true, //< refresh tree
                             true, //< forward
                             false,
                             false); //< same frame
    
}

void
OutputEffectInstance::notifyRenderFinished()
{
    if (_renderController) {
        _renderController->notifyFinished();
        _renderController = 0;
    }
}

int
OutputEffectInstance::getCurrentFrame() const
{
    QMutexLocker l(_outputEffectDataLock);
    return _writerCurrentFrame;
}

void
OutputEffectInstance::setCurrentFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);
    _writerCurrentFrame = f;
}

int
OutputEffectInstance::getFirstFrame() const
{
    QMutexLocker l(_outputEffectDataLock);
    return _writerFirstFrame;
}

void
OutputEffectInstance::setFirstFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);
    _writerFirstFrame = f;
}

int
OutputEffectInstance::getLastFrame() const
{
    QMutexLocker l(_outputEffectDataLock);
    return _writerLastFrame;
}

void
OutputEffectInstance::setLastFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);
    _writerLastFrame = f;
}

void
OutputEffectInstance::setDoingFullSequenceRender(bool b)
{
    QMutexLocker l(_outputEffectDataLock);
    _doingFullSequenceRender = b;
}

bool
OutputEffectInstance::isDoingFullSequenceRender() const
{
    QMutexLocker l(_outputEffectDataLock);
    return _doingFullSequenceRender;
}
