/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef NATRON_ENGINE_EFFECTINSTANCETLSDATA_H
#define NATRON_ENGINE_EFFECTINSTANCETLSDATA_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/ViewIdx.h"
#include "Engine/RectI.h"
#include "Engine/EffectInstance.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER


// Generic arguments passed to the OpenFX actions which may be sometimes not passed-back
// to the A.P.I and that would result in loss of context.
// This is vital at least for these suite functions:
// clipGetRegionOfDefinition: it does not pass-us back the render-scale
// paramGetValue: it does not pass-us back the current action time and view


class GenericActionTLSArgs
{
public:

    GenericActionTLSArgs()
    : time(0)
    , view()
    , scale()
    , actionName()
#ifdef DEBUG
    , canSetValue(true)
#endif
    {

    }

    virtual ~GenericActionTLSArgs() {}


    // The time parameter passed to an OpenFX action
    TimeValue time;

    // the view parameter passed to an OpenFX action
    ViewIdx view;

    // the scale parameter passed to an OpenFX action
    RenderScale scale;

    // Identifier of the action to prevent infinite recursion
    std::string actionName;

#ifdef DEBUG
    // whether or not this action can set value
    bool canSetValue;
#endif

};

class RenderActionTLSData : public GenericActionTLSArgs
{
public:

    RenderActionTLSData()
    : GenericActionTLSArgs()
    , renderWindow()
    , mipMapLevel(0)
    , outputPlanes()
    {

    }

    virtual ~RenderActionTLSData() OVERRIDE {}

    // The render window in pixel coordinates that was passed to render
    RectI renderWindow;

    unsigned int mipMapLevel;

    // For each plane to render, the pointers to the internal images used during render: this is used when calling
    // clipGetImage on the output clip.
    std::map<ImagePlaneDesc, ImagePtr> outputPlanes;

};


//these are per-node thread-local data
struct EffectInstanceTLSDataPrivate;
class EffectInstanceTLSData
{
public:

    EffectInstanceTLSData();

    virtual ~EffectInstanceTLSData();

    /**
     * @brief Push TLS for an action that is not the render action. Mainly this will be needed by OfxClipInstance::getRegionOfDefinition and OfxClipInstance::getImage
     * This should only be needed for OpenFX plug-ins, as the Natron A.P.I already pass all required arguments in parameter.
     **/
    void pushActionArgs(const std::string& actionName, TimeValue time, ViewIdx view, const RenderScale& scale
#ifdef DEBUG
                        , bool canSetValue
                        , bool canBeCalledRecursively
#endif
                        );

    /**
     * @brief Push TLS for the render action for any plug-in (not only OpenFX). This will be needed in EffectInstance::getImage
     **/
    void pushRenderActionArgs(TimeValue time, ViewIdx view, const RenderScale& proxyScale, unsigned int mipMapLevel,
                              const RectI& renderWindow,
                              const std::map<ImagePlaneDesc, ImagePtr>& outputPlanes);

    /**
     * @brief Pop the current action TLS. This call must match a call to one of the push functions above.
     * The push/pop functions should be encapsulated in a RAII style class to ensure the TLS is correctly popped
     * in all cases.
     **/
    void popArgs();


#ifdef DEBUG
    /**
     * @brief Returns true if the action cannot have its knobs calling setValue
     **/
    bool isDuringActionThatCannotSetValue() const;
#endif

    /**
     * @brief Retrieve the current thread action args. This will return true on success, false if we are not
     * between a push/pop bracket.
     **/
    bool getCurrentActionArgs(TimeValue* time, ViewIdx* view, RenderScale* scale, std::string* actionName) const;

    /**
     * @brief Returns true if this given action is the current one
     **/
    bool isCurrentAction(const std::string& actionName) const;

    /**
     * @brief Returns true if one of the actions in the stack is the given action
     **/
    bool hasActionInStack(const std::string& actionName) const;

    /**
     * @brief Clears the stack
     **/
    void clearActionStack();

    /**
     * @brief Same as above execpt for the render action. Any field can be set to NULL if you do not need to retrieve it
     **/
    bool getCurrentRenderActionArgs(TimeValue* time, ViewIdx* view, RenderScale* proxyScale, unsigned int* mipMapLevel,
                                    RectI* renderWindow,
                                    std::map<ImagePlaneDesc, ImagePtr>* outputPlanes) const;

    std::vector<std::string>& getUserPlanesVector();

    
private:
    
    boost::scoped_ptr<EffectInstanceTLSDataPrivate> _imp;

        
};

class EffectActionArgsSetter_RAII
{
    EffectInstanceTLSDataPtr tls;
public:

    EffectActionArgsSetter_RAII(const EffectInstanceTLSDataPtr& tls,
                                const std::string& actionName,
                                TimeValue time, ViewIdx view, const RenderScale& scale
#ifdef DEBUG
                                , bool canSetValue
                                , bool canBeCalledRecursively
#endif
    )
    : tls(tls)
    {
        tls->pushActionArgs(actionName, time, view, scale
#ifdef DEBUG
                            , canSetValue
                            , canBeCalledRecursively
#endif
                            );
    }

    ~EffectActionArgsSetter_RAII()
    {
        tls->popArgs();
    }

};

class RenderActionArgsSetter_RAII
{
    EffectInstanceTLSDataPtr tls;
public:

    RenderActionArgsSetter_RAII(const EffectInstanceTLSDataPtr& tls,
                                TimeValue time, ViewIdx view, const RenderScale& proxyScale, unsigned int mipMapLevel,
                                const RectI& renderWindow,
                                const std::map<ImagePlaneDesc, ImagePtr>& outputPlanes)
    : tls(tls)
    {
        tls->pushRenderActionArgs(time, view, proxyScale, mipMapLevel, renderWindow, outputPlanes);
    }

    ~RenderActionArgsSetter_RAII()
    {
        tls->popArgs();
    }
    
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_EFFECTINSTANCETLSDATA_H
