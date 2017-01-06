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

    ViewerInstancePrivate(ViewerInstance* publicInterface)
        : _publicInterface(publicInterface)
        , viewerParamsMutex()
        , viewerParamsLayer( ImageComponents::getRGBAComponents() )
        , viewerParamsAlphaLayer( ImageComponents::getRGBAComponents() )
        , viewerParamsAlphaChannelName("a")
        , viewerChannelsAutoswitchedToAlpha(false)
        , activateInputChangedFromViewer(false)
        , gammaLookupMutex()
        , gammaLookup()
        , partialUpdateRects()
        , viewportCenter()
        , viewportCenterSet(false)
        , isDoingPartialUpdates(false)
    {

    }

public:




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




public:

    ViewerInstance* _publicInterface;


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


};

NATRON_NAMESPACE_EXIT;


#endif // ifndef Natron_Engine_ViewerInstancePrivate_h
