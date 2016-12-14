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

#ifndef EFFECTINSTANCETLSDATA_H
#define EFFECTINSTANCETLSDATA_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <map>
#include "Engine/EngineFwd.h"
#include "Engine/ViewIdx.h"
#include "Engine/RectI.h"
#include "Engine/EffectInstance.h"
#include "Global/GlobalDefines.h"


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

NATRON_NAMESPACE_ENTER;


// Generic arguments passed to the OpenFX actions which may be sometimes not passed-back
// to the A.P.I and that would result in loss of context.
// This is vital at least for these suite functions:
// clipGetRegionOfDefinition: it does not pass-us back the render-scale
// paramGetValue: it does not pass-us back the current action time and view

class GenericActionTLSArgs;
typedef boost::shared_ptr<GenericActionTLSArgs> GenericActionTLSArgsPtr;

class GenericActionTLSArgs
{
public:

    GenericActionTLSArgs()
    : time(0)
    , view()
    , scale()
    , validArgs(false)
    {

    }

    virtual ~GenericActionTLSArgs() {}


    // The time parameter passed to an OpenFX action
    double time;

    // the view parameter passed to an OpenFX action
    ViewIdx view;

    // the scale parameter passed to an OpenFX action
    RenderScale scale;

    // whether or not the data members of this struct are valid
    bool validArgs;


    virtual GenericActionTLSArgsPtr createCopy()
    {
        GenericActionTLSArgsPtr ret(new GenericActionTLSArgs);
        ret->time = time;
        ret->scale = scale;
        ret->view = view;
        ret->validArgs = validArgs;
        return ret;
    }
};

typedef boost::shared_ptr<GenericActionTLSArgs> GenericActionTLSArgsPtr;

// The render action requires extra data that are valid throughout a single call
// of a render action
class RenderActionTLSData;
typedef boost::shared_ptr<RenderActionTLSData> RenderActionTLSDataPtr;
class RenderActionTLSData : public GenericActionTLSArgs
{
public:

    RenderActionTLSData()
    : GenericActionTLSArgs()
    , renderWindowPixel()
    , inputImages()
    , outputPlanes()
    , compsNeeded()
    , transformRedirections()
    {

    }

    virtual ~RenderActionTLSData() OVERRIDE {}


    // The render window passed to the render action in pixel coordinates.
    // This is useful in the following cases:
    // - the clipGetImage function on the output clip should return an OfxImage with bounds
    // that are clipped to this render window so that if the plug-in attempts to write outside
    // the bounds, it may be caught easily
    // - When calling clipGetImage on an input clip: if the call at the given time/view was not
    // advertised properly in getFramesNeeded, we don't have an image pointer to the input image,
    // thus we have to re-call renderRoI: but we need to make sure we don't ask for more than what
    // getRegionsOfInterest on the renderwindow would compute.
    RectI renderWindowPixel;

    // Input images that were pre-fetched in renderRoI so that they can
    // be accessed from getImage(): this ensure that input images are not de-allocated until
    // we don't need them anymore.
    // As a general note, all expensive actions that may not be cached should always have a shared pointer
    // on the TLS throughout the action to ensure at least we don't cycle through the graph with exponential cost.
    InputImagesMap inputImages;

    // For each plane to render, the pointers to the internal images used during render: this is used when calling
    // clipGetImage on the output clip.
    std::map<ImageComponents, PlaneToRender> outputPlanes;

    // FIXME: this should be removed and the result of the action getComponentsNeeded should be cached much like any
    // other action
    ComponentsNeededMapPtr compsNeeded;

    // The result of the tryConcatenateTransforms function: it is used in the getImage function to concatenate the tree
    // and return the Transform Matrix
    InputMatrixMapPtr transformRedirections;

    virtual GenericActionTLSArgsPtr createCopy() OVERRIDE FINAL
    {
        RenderActionTLSDataPtr ret(new RenderActionTLSData);
        ret->time = time;
        ret->scale = scale;
        ret->view = view;
        ret->validArgs = validArgs;
        ret->renderWindowPixel = renderWindowPixel;
        ret->inputImages = inputImages;
        ret->outputPlanes  = outputPlanes;
        ret->compsNeeded = compsNeeded;
        ret->transformRedirections = transformRedirections;
        return ret;
    }


};


//these are per-node thread-local data
struct EffectTLSDataPrivate;
class EffectTLSData
{
public:

    EffectTLSData();

    EffectTLSData(const EffectTLSData& other);

private:
    
    boost::scoped_ptr<EffectTLSDataPrivate> _imp;

        
};

typedef boost::shared_ptr<EffectTLSData> EffectDataTLSPtr;

NATRON_NAMESPACE_EXIT;

#endif // EFFECTINSTANCETLSDATA_H
