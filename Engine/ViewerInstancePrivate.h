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

#ifndef Natron_Engine_ViewerInstancePrivate_h
#define Natron_Engine_ViewerInstancePrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ViewerInstance.h"

#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <cassert>
#include <algorithm> // min, max

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QReadWriteLock>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/OutputSchedulerThread.h"
#include "Engine/ImageComponents.h"
#include "Engine/FrameEntry.h"
#include "Engine/Settings.h"
#include "Engine/Image.h"
#include "Engine/TextureRect.h"
#include "Engine/EngineFwd.h"

#define GAMMA_LUT_NB_VALUES 1023

NATRON_NAMESPACE_ENTER;

struct OnGoingRenderInfo
{
    bool aborted;
};


typedef std::map<U64,OnGoingRenderInfo> OnGoingRenders;


struct RenderViewerArgs
{
    RenderViewerArgs(const boost::shared_ptr<const Image> &inputImage_,
                     const boost::shared_ptr<const Image> &matteImage_,
                     const TextureRect & texRect_,
                     DisplayChannelsEnum channels_,
                     ImagePremultiplicationEnum srcPremult_,
                     int bitDepth_,
                     double gain_,
                     double gamma_,
                     double offset_,
                     const Color::Lut* srcColorSpace_,
                     const Color::Lut* colorSpace_,
                     int alphaChannelIndex_)
    : inputImage(inputImage_)
    , matteImage(matteImage_)
    , texRect(texRect_)
    , channels(channels_)
    , srcPremult(srcPremult_)
    , bitDepth(bitDepth_)
    , gain(gain_)
    , gamma(gamma_)
    , offset(offset_)
    , srcColorSpace(srcColorSpace_)
    , colorSpace(colorSpace_)
    , alphaChannelIndex(alphaChannelIndex_)
    {
    }

    boost::shared_ptr<const Image> inputImage;
    boost::shared_ptr<const Image> matteImage;
    TextureRect texRect;
    DisplayChannelsEnum channels;
    ImagePremultiplicationEnum srcPremult;
    int bitDepth;
    double gain;
    double gamma;
    double offset;
    const Color::Lut* srcColorSpace;
    const Color::Lut* colorSpace;
    int alphaChannelIndex;
};

/// parameters send from the scheduler thread to updateViewer() (which runs in the main thread)
class UpdateViewerParams : public BufferableObject
{
    
public:
    
    UpdateViewerParams()
    : ramBuffer(NULL)
    , mustFreeRamBuffer(false)
    , textureIndex(0)
    , time(0)
    , textureRect()
    , srcPremult(eImagePremultiplicationOpaque)
    , bytesCount(0)
    , depth()
    , gain(1.)
    , gamma(1.)
    , offset(0.)
    , mipMapLevel(0)
    , premult(eImagePremultiplicationOpaque)
    , lut(eViewerColorSpaceSRGB)
    , layer()
    , alphaLayer()
    , alphaChannelName()
    , cachedFrame()
    , tiles()
    , rod()
    , renderAge(0)
    , isSequential(false)
    , roi()
    , updateOnlyRoi(false)
    {
    }
    
    virtual ~UpdateViewerParams() {
        if (mustFreeRamBuffer) {
            free(ramBuffer);
        }
    }
    
    virtual std::size_t sizeInRAM() const OVERRIDE FINAL
    {
        return bytesCount;
    }

    unsigned char* ramBuffer;
    bool mustFreeRamBuffer; //< set to true when !cachedFrame
    int textureIndex;
    int time;
    TextureRect textureRect;
    ImagePremultiplicationEnum srcPremult;
    size_t bytesCount;
    ImageBitDepthEnum depth;
    double gain;
    double gamma;
    double offset;
    unsigned int mipMapLevel;
    ImagePremultiplicationEnum premult;
    ViewerColorSpaceEnum lut;
    ImageComponents layer;
    ImageComponents alphaLayer;
    std::string alphaChannelName;
    
    // put a shared_ptr here, so that the cache entry is never released before the end of updateViewer()
    boost::shared_ptr<FrameEntry> cachedFrame;
    std::list<boost::shared_ptr<Image> > tiles;
    RectD rod;
    U64 renderAge;
    bool isSequential;
    RectI roi;
    bool updateOnlyRoi;
};

struct ViewerInstance::ViewerInstancePrivate
: public QObject, public LockManagerI<FrameEntry>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    ViewerInstancePrivate(const ViewerInstance* parent)
    : instance(parent)
    , uiContext(NULL)
    , forceRenderMutex()
    , forceRender(false)
    , updateViewerPboIndex(0)
    , viewerParamsMutex()
    , viewerParamsGain(1.)
    , viewerParamsGamma(1.)
    , viewerParamsLut(eViewerColorSpaceSRGB)
    , viewerParamsAutoContrast(false)
    , viewerParamsChannels()
    , viewerParamsLayer(ImageComponents::getRGBAComponents())
    , viewerParamsAlphaLayer(ImageComponents::getRGBAComponents())
    , viewerParamsAlphaChannelName("a")
    , viewerMipMapLevel(0)
    , activeInputsMutex(QMutex::Recursive)
    , activeInputs()
    , activateInputChangedFromViewer(false)
    , gammaLookupMutex()
    , gammaLookup()
    , lastRotoPaintTickParamsMutex()
    , lastRotoPaintTickParams()
    , currentlyUpdatingOpenGLViewerMutex()
    , currentlyUpdatingOpenGLViewer(false)
    , renderAgeMutex()
    , renderAge()
    , displayAge()
    {

        for (int i = 0; i < 2; ++i) {
            activeInputs[i] = -1;
            renderAge[i] = 1;
            displayAge[i] = 0;
            viewerParamsChannels[i] = eDisplayChannelsRGB;
        }
        
    }
    
    

    void redrawViewer()
    {
        Q_EMIT mustRedrawViewer();
    }
    
public:
    
    virtual void lock(const boost::shared_ptr<FrameEntry>& entry) OVERRIDE FINAL
    {


        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<boost::shared_ptr<FrameEntry> >::iterator it =
                std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);
        while ( it != textureBeingRendered.end() ) {
            textureBeingRenderedCond.wait(&textureBeingRenderedMutex);
            it = std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);
        }
        ///Okay the image is not used by any other thread, claim that we want to use it
        assert( it == textureBeingRendered.end() );
        textureBeingRendered.push_back(entry);
    }
    
    virtual bool tryLock(const boost::shared_ptr<FrameEntry>& entry) OVERRIDE FINAL
    {
        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<boost::shared_ptr<FrameEntry> >::iterator it =
        std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);
        if ( it != textureBeingRendered.end() ) {
            return false;
        }
        ///Okay the image is not used by any other thread, claim that we want to use it
        assert( it == textureBeingRendered.end() );
        textureBeingRendered.push_back(entry);
        return true;
    }
    
    virtual void unlock(const boost::shared_ptr<FrameEntry>& entry) OVERRIDE FINAL
    {

        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<boost::shared_ptr<FrameEntry> >::iterator it =
                std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);
        ///The image must exist, otherwise this is a bug
        assert( it != textureBeingRendered.end() );
        textureBeingRendered.erase(it);
        ///Notify all waiting threads that we're finished
        textureBeingRenderedCond.wakeAll();
    }

    /**
     * @brief Returns the current render age of the viewer (a simple counter incrementing at each request).
     * The age is then incremented so the next call to getRenderAge will return the current value plus one.
     **/
    U64 getRenderAgeAndIncrement(int texIndex)
    {
        QMutexLocker k(&renderAgeMutex);
        
        U64 ret = renderAge[texIndex];
        if (renderAge[texIndex] == std::numeric_limits<U64>::max()) {
            renderAge[texIndex] = 0;
        } else {
            ++renderAge[texIndex];
        }
        return ret;
        
    }
    
    /**
     * @brief We keep track of ongoing renders internally. This function is called only by non 
     * abortable renders to determine if we should abort anyway because the render is no longer interesting.
     **/
    bool isRenderAbortable(int texIndex,U64 age) const
    {
        QMutexLocker k(&renderAgeMutex);
      
        
        for (OnGoingRenders::const_iterator it = currentRenderAges[texIndex].begin(); it!=currentRenderAges[texIndex].end();++it) {
            if (it->first == age) {
                return it->second.aborted;
            }
        }
         //hmm something is wrong the render doesn't exist
        return true;
    }
    
    /**
     * @brief To be called to check if there we are the last requested render (true) or if there were
     * more recent requests (false).
     **/
    bool checkAgeNoUpdate(int texIndex,U64 age)
    {
        QMutexLocker k(&renderAgeMutex);
        assert(age <= renderAge[texIndex]);
        if (age >= displayAge[texIndex]) {
            return true;
        } else {
            return false;
        }
    }
    
    /**
     * @brief To be called when a render is about to end (either because of a failure or of success).
     * If there was already a more recent render that had finished rendering, we return false meaning
     * it should not continue further for the current render and not redraw the viewer.
     * On the other hand, if we're the most recent render request, we return true and update the last
     * render age, meaning we should redraw the viewer.
     **/
    bool checkAndUpdateDisplayAge(int texIndex,U64 age)
    {
        QMutexLocker k(&renderAgeMutex);
        assert(age <= renderAge[texIndex]);
        assert(age != displayAge[texIndex]);
        if (age <= displayAge[texIndex]) {
            return false;
        }
        displayAge[texIndex] = age;
        return true;
    }
    
    bool addOngoingRender(int texIndex, U64 age) {
        QMutexLocker k(&renderAgeMutex);
        if (!currentRenderAges[texIndex].empty() && currentRenderAges[texIndex].rbegin()->first >= age) {
            return false;
        }
       
        OnGoingRenderInfo info;
        info.aborted = false;
        currentRenderAges[texIndex][age] = info;
        return true;
    }
    
    bool removeOngoingRender(int texIndex, U64 age) {
        QMutexLocker k(&renderAgeMutex);
        for (OnGoingRenders::iterator it = currentRenderAges[texIndex].begin(); it!=currentRenderAges[texIndex].end();++it) {
            if (it->first == age) {
                currentRenderAges[texIndex].erase(it);
                return true;
            }
        }

        return false;
    }

    
    void fillGammaLut(double gamma) {
        gammaLookup.resize(GAMMA_LUT_NB_VALUES + 1);
        for (int position = 0; position <= GAMMA_LUT_NB_VALUES; ++position) {
            
            double parametricPos = double(position) / GAMMA_LUT_NB_VALUES;
            double value = std::pow(parametricPos, gamma);
            // set that in the lut
            gammaLookup[position] = (float)std::max(0.,std::min(1.,value));
        }
    }
    
    float lookupGammaLut(float value) const
    {
        if (value < 0.) {
            return 0.;
        } else if (value > 1.) {
            return 1.;
        } else {
            int i = (int)(value * GAMMA_LUT_NB_VALUES);
            assert(0 <= i && i <= GAMMA_LUT_NB_VALUES);
            float alpha = std::max(0.f,std::min(value * GAMMA_LUT_NB_VALUES - i, 1.f));
            float a = gammaLookup[i];
            float b = (i  < GAMMA_LUT_NB_VALUES) ? gammaLookup[i + 1] : 0.f;
            return a * (1.f - alpha) + b * alpha;
        }

    }
    
    void reportProgress(const boost::shared_ptr<UpdateViewerParams>& originalParams,
                        const std::list<RectI>& rectangles,
                        const boost::shared_ptr<RenderStats>& stats,
                        const boost::shared_ptr<RequestedFrame>& request);

public Q_SLOTS:

    /**
     * @brief Slot called internally by the renderViewer() function when it wants to refresh the OpenGL viewer.
     * Do not call this yourself.
     **/
    void updateViewer(boost::shared_ptr<UpdateViewerParams> params);

Q_SIGNALS:
   
    void mustRedrawViewer();

public:
    const ViewerInstance* const instance;
    OpenGLViewerI* uiContext; // written in the main thread before render thread creation, accessed from render thread
    mutable QMutex forceRenderMutex;
    bool forceRender; /*!< true when we want to by-pass the cache*/


    // updateViewer: stuff for handling the execution of updateViewer() in the main thread, @see UpdateViewerParams
    //is always called on the main thread, but the thread running renderViewer MUST
    //wait the entire time. This flag is here to make the renderViewer() thread wait
    //until the texture upload is finished by the main thread.
    int updateViewerPboIndex;                // always accessed in the main thread: initialized in the constructor, then always accessed and modified by updateViewer()


    // viewerParams: The viewer parameters that may be accessed from the GUI
    mutable QMutex viewerParamsMutex;   //< protects viewerParamsGain, viewerParamsLut, viewerParamsAutoContrast, viewerParamsChannels
    double viewerParamsGain;           /*!< Current gain setting in the GUI. Not affected by autoContrast. */
    double viewerParamsGamma;          /*!< Current gamma setting in the GUI. Not affected by autoContrast. */
    ViewerColorSpaceEnum viewerParamsLut; /*!< a value coding the current color-space used to render.
                                                 0 = sRGB ,  1 = linear , 2 = Rec 709*/
    bool viewerParamsAutoContrast;
    DisplayChannelsEnum viewerParamsChannels[2];
    ImageComponents viewerParamsLayer;
    ImageComponents viewerParamsAlphaLayer;
    std::string viewerParamsAlphaChannelName;
    unsigned int viewerMipMapLevel; //< the mipmap level the viewer should render at (0 == no downscaling)

    mutable QMutex activeInputsMutex;
    int activeInputs[2]; //< indexes of the inputs used for the wipe
    
    ///Only accessed from MT
    bool activateInputChangedFromViewer;
    
    mutable QMutex textureBeingRenderedMutex;
    QWaitCondition textureBeingRenderedCond;
    std::list<boost::shared_ptr<FrameEntry> > textureBeingRendered; ///< a list of all the texture being rendered simultaneously
    
    mutable QReadWriteLock gammaLookupMutex;
    std::vector<float> gammaLookup; // protected by gammaLookupMutex
    
    //When painting, this is the last texture we've drawn onto so that we can update only the specific portion needed
    mutable QMutex lastRotoPaintTickParamsMutex;
    boost::shared_ptr<UpdateViewerParams> lastRotoPaintTickParams[2];
    
    mutable QMutex currentlyUpdatingOpenGLViewerMutex;
    bool currentlyUpdatingOpenGLViewer;
    
    mutable QMutex renderAgeMutex; // protects renderAge lastRenderAge currentRenderAges
    U64 renderAge[2];
    U64 displayAge[2];
    
    //A priority list recording the ongoing renders. This is used for abortable renders (i.e: when moving a slider or scrubbing the timeline)
    //The purpose of this is to always at least keep 1 active render (non abortable) and abort more recent renders that do no longer make sense
    
    OnGoingRenders currentRenderAges[2];
    
};

NATRON_NAMESPACE_EXIT;


#endif // ifndef Natron_Engine_ViewerInstancePrivate_h
