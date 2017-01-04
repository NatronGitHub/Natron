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

#include "Engine/OutputSchedulerThread.h"
#include "Engine/ImageComponents.h"
#include "Engine/KnobTypes.h"
#include "Engine/Settings.h"
#include "Engine/Image.h"
#include "Engine/TextureRect.h"
#include "Engine/EngineFwd.h"

#define GAMMA_LUT_NB_VALUES 1023

NATRON_NAMESPACE_ENTER;



struct RenderViewerArgs
{
    RenderViewerArgs(const boost::shared_ptr<const Image> &inputImage_,
                     const boost::shared_ptr<const Image> &matteImage_,
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

    boost::shared_ptr<const Image> inputImage;
    boost::shared_ptr<const Image> matteImage;
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
    : public QObject
{

public:

    ViewerInstancePrivate(const ViewerInstance* parent)
        : instance(parent)
        , forceRenderMutex()
        , forceRender()
        , viewerParamsMutex()
        , viewerParamsLayer( ImageComponents::getRGBAComponents() )
        , viewerParamsAlphaLayer( ImageComponents::getRGBAComponents() )
        , viewerParamsAlphaChannelName("a")
        , viewerChannelsAutoswitchedToAlpha(false)
        , activateInputChangedFromViewer(false)
        , gammaLookupMutex()
        , gammaLookup()
        , lastRenderParamsMutex()
        , lastRenderParams()
        , partialUpdateRects()
        , viewportCenter()
        , viewportCenterSet(false)
        , isDoingPartialUpdates(false)
    {

    }

public:


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
        
        // Now that we increment the display age, check if ongoing renders are still useful, otherwise abort them
        for (OnGoingRenders::iterator it = currentRenderAges[texIndex].begin(); it != currentRenderAges[texIndex].end(); ++it) {
            if ( (*it)->getRenderAge() < displayAge[texIndex] ) {
                (*it)->setAborted();                
            }
        }

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


    void updateViewer(boost::shared_ptr<UpdateViewerParams> params);


public:
    const ViewerInstance* const instance;
    mutable QMutex forceRenderMutex;
    bool forceRender[2]; /*!< true when we want to by-pass the cache*/

    // updateViewer: stuff for handling the execution of updateViewer() in the main thread, @see UpdateViewerParams
    //is always called on the main thread, but the thread running renderViewer MUST
    //wait the entire time. This flag is here to make the renderViewer() thread wait
    //until the texture upload is finished by the main thread.


    // viewerParams: The viewer parameters that may be accessed from the GUI
    mutable QMutex viewerParamsMutex;   //< protects viewerParamsGain, viewerParamsLut, viewerParamsAutoContrast, viewerParamsChannels
    ImageComponents viewerParamsLayer;
    ImageComponents viewerParamsAlphaLayer;
    std::string viewerParamsAlphaChannelName;
    bool viewerChannelsAutoswitchedToAlpha;

    ///Only accessed from MT
    bool activateInputChangedFromViewer;
    mutable QReadWriteLock gammaLookupMutex;
    std::vector<float> gammaLookup; // protected by gammaLookupMutex

    //When painting, this is the last texture we've drawn onto so that we can update only the specific portion needed
    mutable QMutex lastRenderParamsMutex;
    boost::shared_ptr<UpdateViewerParams> lastRenderParams[2];

    /*
     * @brief If this list is not empty, this is the list of canonical rectangles we should update on the viewer, completly
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

};

NATRON_NAMESPACE_EXIT;


#endif // ifndef Natron_Engine_ViewerInstancePrivate_h
