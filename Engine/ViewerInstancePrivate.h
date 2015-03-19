//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
//  Created by Frédéric Devernay on 20/03/2014.
//
//

#ifndef Natron_Engine_ViewerInstancePrivate_h
#define Natron_Engine_ViewerInstancePrivate_h

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ViewerInstance.h"

#include <map>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/OutputSchedulerThread.h"
#include "Engine/ImageComponents.h"
#include "Engine/FrameEntry.h"
#include "Engine/Settings.h"
#include "Engine/TextureRect.h"

namespace Natron {
class FrameEntry;
class FrameParams;
}

//namespace Natron {

struct RenderViewerArgs
{
    RenderViewerArgs(boost::shared_ptr<const Natron::Image> inputImage_,
                     const TextureRect & texRect_,
                     Natron::DisplayChannelsEnum channels_,
                     Natron::ImagePremultiplicationEnum srcPremult_,
                     int bitDepth_,
                     double gain_,
                     double offset_,
                     const Natron::Color::Lut* srcColorSpace_,
                     const Natron::Color::Lut* colorSpace_,
                     int alphaChannelIndex_)
    : inputImage(inputImage_)
    , texRect(texRect_)
    , channels(channels_)
    , srcPremult(srcPremult_)
    , bitDepth(bitDepth_)
    , gain(gain_)
    , offset(offset_)
    , srcColorSpace(srcColorSpace_)
    , colorSpace(colorSpace_)
    , alphaChannelIndex(alphaChannelIndex_)
    {
    }

    boost::shared_ptr<const Natron::Image> inputImage;
    TextureRect texRect;
    Natron::DisplayChannelsEnum channels;
    Natron::ImagePremultiplicationEnum srcPremult;
    int bitDepth;
    double gain;
    double offset;
    const Natron::Color::Lut* srcColorSpace;
    const Natron::Color::Lut* colorSpace;
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
    , srcPremult(Natron::eImagePremultiplicationOpaque)
    , bytesCount(0)
    , gain(1.)
    , offset(0.)
    , mipMapLevel(0)
    , premult(Natron::eImagePremultiplicationOpaque)
    , lut(Natron::eViewerColorSpaceSRGB)
    , layer()
    , alphaLayer()
    , alphaChannelName()
    , cachedFrame()
    , image()
    , rod()
    , renderAge(0)
    , isSequential(false)
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
    Natron::ImagePremultiplicationEnum srcPremult;
    size_t bytesCount;
    double gain;
    double offset;
    unsigned int mipMapLevel;
    Natron::ImagePremultiplicationEnum premult;
    Natron::ViewerColorSpaceEnum lut;
    Natron::ImageComponents layer;
    Natron::ImageComponents alphaLayer;
    std::string alphaChannelName;
    
    // put a shared_ptr here, so that the cache entry is never released before the end of updateViewer()
    boost::shared_ptr<Natron::FrameEntry> cachedFrame;
    boost::shared_ptr<Natron::Image> image;
    RectD rod;
    U64 renderAge;
    bool isSequential;
};

struct ViewerInstance::ViewerInstancePrivate
: public QObject, public LockManagerI<Natron::FrameEntry>
{
    Q_OBJECT

public:
    
    ViewerInstancePrivate(const ViewerInstance* parent)
    : instance(parent)
    , uiContext(NULL)
    , forceRenderMutex()
    , forceRender(false)
    , updateViewerPboIndex(0)
    , viewerParamsMutex()
    , viewerParamsGain(1.)
    , viewerParamsLut(Natron::eViewerColorSpaceSRGB)
    , viewerParamsAutoContrast(false)
    , viewerParamsChannels(Natron::eDisplayChannelsRGB)
    , viewerParamsLayer(Natron::ImageComponents::getRGBAComponents())
    , viewerParamsAlphaLayer(Natron::ImageComponents::getRGBAComponents())
    , viewerParamsAlphaChannelName("a")
    , viewerMipMapLevel(0)
    , activeInputsMutex()
    , activeInputs()
    , lastRenderedHashMutex()
    , lastRenderedHash(0)
    , lastRenderedHashValid(false)
    , renderAgeMutex()
    , renderAge()
    , displayAge()
    {

        for (int i = 0;i < 2; ++i) {
            activeInputs[i] = -1;
            renderAge[i] = 1;
            displayAge[i] = 0;
        }
    }
    
    

    void redrawViewer()
    {
        Q_EMIT mustRedrawViewer();
    }
    
public:
    
    virtual void lock(const boost::shared_ptr<Natron::FrameEntry>& entry) OVERRIDE FINAL
    {


        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<boost::shared_ptr<Natron::FrameEntry> >::iterator it =
                std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);
        while ( it != textureBeingRendered.end() ) {
            textureBeingRenderedCond.wait(&textureBeingRenderedMutex);
            it = std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);
        }
        ///Okay the image is not used by any other thread, claim that we want to use it
        assert( it == textureBeingRendered.end() );
        textureBeingRendered.push_back(entry);
    }
    
    virtual bool tryLock(const boost::shared_ptr<Natron::FrameEntry>& entry) OVERRIDE FINAL
    {
        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<boost::shared_ptr<Natron::FrameEntry> >::iterator it =
        std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);
        if ( it != textureBeingRendered.end() ) {
            return false;
        }
        ///Okay the image is not used by any other thread, claim that we want to use it
        assert( it == textureBeingRendered.end() );
        textureBeingRendered.push_back(entry);
        return true;
    }
    
    virtual void unlock(const boost::shared_ptr<Natron::FrameEntry>& entry) OVERRIDE FINAL
    {

        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<boost::shared_ptr<Natron::FrameEntry> >::iterator it =
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
    U64 getRenderAge(int texIndex)
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
     * @brief We keep track of ongoing renders internally. This is used for abortable renders 
     * (scrubbing the timeline, moving a slider...) to keep always at least 1 thread computing
     * so that not all threads are always aborted.
     **/
    bool isRenderAbortable(int texIndex,U64 age) const
    {
        QMutexLocker k(&renderAgeMutex);
        if (!currentRenderAges[texIndex].empty() && currentRenderAges[texIndex].front() == age) {
            return false;
        }
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
        if (age < displayAge[texIndex]) {
            return false;
        }
        displayAge[texIndex] = age;
        return true;
    }
    
    bool addOngoingRender(int texIndex, U64 age) {
        QMutexLocker k(&renderAgeMutex);
        if (!currentRenderAges[texIndex].empty() && currentRenderAges[texIndex].back() >= age) {
            return false;
        }
        if (currentRenderAges[texIndex].size() > 1) {
            currentRenderAges[texIndex].resize(1);
        }
        currentRenderAges[texIndex].push_back(age);
        return true;
    }
    
    bool removeOngoingRender(int texIndex, U64 age) {
        QMutexLocker k(&renderAgeMutex);
        int i = 0;
        for (std::list<U64>::iterator it = currentRenderAges[texIndex].begin(); it != currentRenderAges[texIndex].end(); ++it, ++i) {
            if (*it == age) {
                currentRenderAges[texIndex].erase(it);
                return true;
            }
        }
        return false;
    }


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
    Natron::ViewerColorSpaceEnum viewerParamsLut; /*!< a value coding the current color-space used to render.
                                                 0 = sRGB ,  1 = linear , 2 = Rec 709*/
    bool viewerParamsAutoContrast;
    Natron::DisplayChannelsEnum viewerParamsChannels;
    Natron::ImageComponents viewerParamsLayer;
    Natron::ImageComponents viewerParamsAlphaLayer;
    std::string viewerParamsAlphaChannelName;
    unsigned int viewerMipMapLevel; //< the mipmap level the viewer should render at (0 == no downscaling)

    mutable QMutex activeInputsMutex;
    int activeInputs[2]; //< indexes of the inputs used for the wipe
    
    QMutex lastRenderedHashMutex;
    U64 lastRenderedHash;
    bool lastRenderedHashValid;
    
    mutable QMutex textureBeingRenderedMutex;
    QWaitCondition textureBeingRenderedCond;
    std::list<boost::shared_ptr<Natron::FrameEntry> > textureBeingRendered; ///< a list of all the texture being rendered simultaneously
    
private:
    
    mutable QMutex renderAgeMutex; // protects renderAge lastRenderAge currentRenderAges
    U64 renderAge[2];
    U64 displayAge[2];
    
    //A priority list recording the ongoing renders. This is used for abortable renders (i.e: when moving a slider or scrubbing the timeline)
    //The purpose of this is to always at least keep 1 active render (non abortable) and abort more recent renders that do no longer make sense
    std::list<U64> currentRenderAges[2];
};


//} // namespace Natron


#endif // ifndef Natron_Engine_ViewerInstancePrivate_h
