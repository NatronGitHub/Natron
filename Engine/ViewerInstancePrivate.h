/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

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

#include "Engine/AbortableRenderInfo.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/FrameEntry.h"
#include "Engine/Settings.h"
#include "Engine/Image.h"
#include "Engine/TextureRect.h"
#include "Engine/EngineFwd.h"

#define GAMMA_LUT_NB_VALUES 1023

NATRON_NAMESPACE_ENTER


struct AbortableRenderInfo_CompareAge
{
    bool operator() (const AbortableRenderInfoPtr & lhs,
                     const AbortableRenderInfoPtr & rhs) const
    {
        return lhs->getRenderAge() < rhs->getRenderAge();
    }
};

typedef std::set<AbortableRenderInfoPtr, AbortableRenderInfo_CompareAge> OnGoingRenders;


struct RenderViewerArgs
{
    RenderViewerArgs(const ImageConstPtr &inputImage_,
                     const ImageConstPtr &matteImage_,
                     DisplayChannelsEnum channels_,
                     ImagePremultiplicationEnum srcPremult_,
                     int bitDepth_,
                     double gain_,
                     double gamma_,
                     double offset_,
                     const Color::Lut* srcColorSpace_,
                     const Color::Lut* colorSpace_,
                     int alphaChannelIndex_,
                     bool renderOnlyRoI_,
                     std::size_t tileRowElements_)
        : inputImage(inputImage_)
        , matteImage(matteImage_)
        , channels(channels_)
        , srcPremult(srcPremult_)
        , bitDepth(bitDepth_)
        , gain(gain_)
        , gamma(gamma_)
        , offset(offset_)
        , srcColorSpace(srcColorSpace_)
        , colorSpace(colorSpace_)
        , alphaChannelIndex(alphaChannelIndex_)
        , renderOnlyRoI(renderOnlyRoI_)
        , tileRowElements(tileRowElements_)
    {
    }

    ImageConstPtr inputImage;
    ImageConstPtr matteImage;
    DisplayChannelsEnum channels;
    ImagePremultiplicationEnum srcPremult;
    int bitDepth;
    double gain;
    double gamma;
    double offset;
    const Color::Lut* srcColorSpace;
    const Color::Lut* colorSpace;
    int alphaChannelIndex;
    bool renderOnlyRoI;
    std::size_t tileRowElements;
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
        , forceRender()
        , isViewerPausedMutex()
        , isViewerPaused()
        , viewerParamsMutex()
        , viewerParamsGain(1.)
        , viewerParamsGamma(1.)
        , viewerParamsLut(eViewerColorSpaceSRGB)
        , viewerParamsAutoContrast(false)
        , viewerParamsChannels()
        , viewerParamsLayer( ImagePlaneDesc::getRGBAComponents() )
        , viewerParamsAlphaLayer( ImagePlaneDesc::getRGBAComponents() )
        , viewerParamsAlphaChannelName("a")
        , viewerMipMapLevel(0)
        , fullFrameProcessingEnabled(false)
        , activateInputChangedFromViewer(false)
        , gammaLookupMutex()
        , gammaLookup()
        , lastRenderParamsMutex()
        , lastRenderParams()
        , partialUpdateRects()
        , viewportCenter()
        , viewportCenterSet(false)
        , isDoingPartialUpdates(false)
        , renderAgeMutex()
        , renderAge()
        , displayAge()
    {
        for (int i = 0; i < 2; ++i) {
            forceRender[i] = false;
            renderAge[i] = 1;
            displayAge[i] = 0;
            isViewerPaused[i] = false;
            viewerParamsChannels[i] = eDisplayChannelsRGB;
        }
    }

    void redrawViewer()
    {
        Q_EMIT mustRedrawViewer();
    }

public:

    virtual void lock(const FrameEntryPtr& entry) OVERRIDE FINAL
    {
        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<FrameEntryPtr>::iterator it =
            std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);

        while ( it != textureBeingRendered.end() ) {
            textureBeingRenderedCond.wait(&textureBeingRenderedMutex);
            it = std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);
        }
        ///Okay the image is not used by any other thread, claim that we want to use it
        assert( it == textureBeingRendered.end() );
        textureBeingRendered.push_back(entry);
    }

    virtual bool tryLock(const FrameEntryPtr& entry) OVERRIDE FINAL
    {
        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<FrameEntryPtr>::iterator it =
            std::find(textureBeingRendered.begin(), textureBeingRendered.end(), entry);

        if ( it != textureBeingRendered.end() ) {
            return false;
        }
        ///Okay the image is not used by any other thread, claim that we want to use it
        assert( it == textureBeingRendered.end() );
        textureBeingRendered.push_back(entry);

        return true;
    }

    virtual void unlock(const FrameEntryPtr& entry) OVERRIDE FINAL
    {
        QMutexLocker l(&textureBeingRenderedMutex);
        std::list<FrameEntryPtr>::iterator it =
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
    AbortableRenderInfoPtr createNewRenderRequest(int texIndex,
                                                  bool canAbort)
    {
        QMutexLocker k(&renderAgeMutex);
        U64 ret = renderAge[texIndex];

        if ( renderAge[texIndex] == std::numeric_limits<U64>::max() ) {
            renderAge[texIndex] = 0;
        } else {
            ++renderAge[texIndex];
        }

        AbortableRenderInfoPtr info = AbortableRenderInfo::create(canAbort, ret);

        return info;
    }

    /**
     * @brief We keep track of ongoing renders internally. This function is called only by non
     * abortable renders to determine if we should abort anyway because the render is no longer interesting.
     **/
    bool isLatestRender(int texIndex,
                        U64 age) const
    {
        QMutexLocker k(&renderAgeMutex);

        return !currentRenderAges[texIndex].empty() && ( *currentRenderAges[texIndex].rbegin() )->getRenderAge() == age;
    }

    /**
     * @brief To be called to check if there we are the last requested render (true) or if there were
     * more recent requests (false).
     **/
    bool checkAgeNoUpdate(int texIndex,
                          U64 age)
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
    bool checkAndUpdateDisplayAge(int texIndex,
                                  U64 age)
    {
        QMutexLocker k(&renderAgeMutex);

        assert(age <= renderAge[texIndex]);
        if (age <= displayAge[texIndex]) {
            return false;
        }
        displayAge[texIndex] = age;

        return true;
    }

    bool addOngoingRender(int texIndex,
                          const AbortableRenderInfoPtr& abortInfo)
    {
        QMutexLocker k(&renderAgeMutex);

        if ( !currentRenderAges[texIndex].empty() && ( ( *currentRenderAges[texIndex].rbegin() )->getRenderAge() >= abortInfo->getRenderAge() ) ) {
            return false;
        }
        currentRenderAges[texIndex].insert(abortInfo);

        return true;
    }

    bool removeOngoingRender(int texIndex,
                             U64 age)
    {
        QMutexLocker k(&renderAgeMutex);

        for (OnGoingRenders::iterator it = currentRenderAges[texIndex].begin(); it != currentRenderAges[texIndex].end(); ++it) {
            if ( (*it)->getRenderAge() == age ) {
                currentRenderAges[texIndex].erase(it);

                return true;
            }
        }

        return false;
    }

    void fillGammaLut(double gamma)
    {
        // gammaLookupMutex should already be locked
        gammaLookup.resize(GAMMA_LUT_NB_VALUES + 1);
        if (gamma <= 0) {
            // gamma = 0: everything is zero, except gamma(1)=1
            std::fill(gammaLookup.begin(), gammaLookup.begin() + GAMMA_LUT_NB_VALUES, 0.f);
            gammaLookup[GAMMA_LUT_NB_VALUES] = 1.f;
            return;
        }
        for (int position = 0; position <= GAMMA_LUT_NB_VALUES; ++position) {
            double parametricPos = double(position) / GAMMA_LUT_NB_VALUES;
            double value = std::pow(parametricPos, 1. / gamma);
            // set that in the lut
            gammaLookup[position] = (float)std::max( 0., std::min(1., value) );
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
            float alpha = std::max( 0.f, std::min(value * GAMMA_LUT_NB_VALUES - i, 1.f) );
            float a = gammaLookup[i];
            float b = (i  < GAMMA_LUT_NB_VALUES) ? gammaLookup[i + 1] : 0.f;

            return a * (1.f - alpha) + b * alpha;
        }
    }

public Q_SLOTS:

    /**
     * @brief Slot called internally by the renderViewer() function when it wants to refresh the OpenGL viewer.
     * Do not call this yourself.
     **/
    void updateViewer(UpdateViewerParamsPtr params);

Q_SIGNALS:

    void mustRedrawViewer();

public:
    const ViewerInstance* const instance;
    OpenGLViewerI* uiContext; // written in the main thread before render thread creation, accessed from render thread
    mutable QMutex forceRenderMutex;
    bool forceRender[2]; /*!< true when we want to by-pass the cache*/
    mutable QMutex isViewerPausedMutex;
    bool isViewerPaused[2]; /*!< When true we should no longer refresh the viewer */

    // updateViewer: stuff for handling the execution of updateViewer() in the main thread, @see UpdateViewerParams
    //is always called on the main thread, but the thread running renderViewer MUST
    //wait the entire time. This flag is here to make the renderViewer() thread wait
    //until the texture upload is finished by the main thread.


    // viewerParams: The viewer parameters that may be accessed from the GUI
    mutable QMutex viewerParamsMutex;   //< protects viewerParamsGain, viewerParamsLut, viewerParamsAutoContrast, viewerParamsChannels
    double viewerParamsGain;           /*!< Current gain setting in the GUI. Not affected by autoContrast. */
    double viewerParamsGamma;          /*!< Current gamma setting in the GUI. Not affected by autoContrast. */
    ViewerColorSpaceEnum viewerParamsLut; /*!< a value coding the current color-space used to render.
                                                 0 = sRGB ,  1 = linear , 2 = Rec 709*/
    bool viewerParamsAutoContrast;
    DisplayChannelsEnum viewerParamsChannels[2];
    ImagePlaneDesc viewerParamsLayer;
    ImagePlaneDesc viewerParamsAlphaLayer;
    std::string viewerParamsAlphaChannelName;
    unsigned int viewerMipMapLevel; //< the mipmap level the viewer should render at (0 == no downscaling)
    bool fullFrameProcessingEnabled;

    ///Only accessed from MT
    bool activateInputChangedFromViewer;
    mutable QMutex textureBeingRenderedMutex;
    QWaitCondition textureBeingRenderedCond;
    std::list<FrameEntryPtr> textureBeingRendered; ///< a list of all the texture being rendered simultaneously
    mutable QReadWriteLock gammaLookupMutex;
    std::vector<float> gammaLookup; // protected by gammaLookupMutex

    //When painting, this is the last texture we've drawn onto so that we can update only the specific portion needed
    mutable QMutex lastRenderParamsMutex;
    UpdateViewerParamsPtr lastRenderParams[2];

    /*
     * @brief If this list is not empty, this is the list of canonical rectangles we should update on the viewer, completely
     * disregarding the RoI. This is protected by viewerParamsMutex
     */
    std::list<RectD> partialUpdateRects;

    /*
     * @brief If set, the viewport center will be updated to this point upon the next update of the texture, this is protected by
     * viewerParamsMutex
     */
    Point viewportCenter;
    bool viewportCenterSet;

    //True if during tracking
    bool isDoingPartialUpdates;
    mutable QMutex renderAgeMutex; // protects renderAge lastRenderAge currentRenderAges
    U64 renderAge[2];
    U64 displayAge[2];

    //A priority list recording the ongoing renders. This is used for abortable renders (i.e: when moving a slider or scrubbing the timeline)
    //The purpose of this is to always at least keep 1 active render (non abortable) and abort more recent renders that do no longer make sense
    OnGoingRenders currentRenderAges[2];
};

NATRON_NAMESPACE_EXIT


#endif // ifndef Natron_Engine_ViewerInstancePrivate_h
