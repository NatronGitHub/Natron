/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef EFFECTDESCRIPTION_H
#define EFFECTDESCRIPTION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/PropertiesHolder.h"
#include "Global/Enums.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief x1 RenderSafetyEnum property (optional) indicating whether if a plug-in render action is multi-thread safe or not
 * Default value - eRenderSafetyInstanceSafe
 **/
#define kEffectPropRenderThreadSafety "EffectPropRenderThreadSafety"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in may receive tiled images. Otherwise full size images must be returned.
 * Default value - false
 **/
#define kEffectPropSupportsTiles "EffectPropSupportsTiles"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in may request images at a different time that the one passed to the render action
 * Default value - false
 **/
#define kEffectPropTemporalImageAccess "EffectPropTemporalImageAccess"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in can handle images coming from different inputs with a different size.
 * Default value - false
 **/
#define kEffectPropSupportsMultiResolution "EffectPropSupportsMultiResolution"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in can handle actions being called with a render scale different of 1.
 * Default value - true
 **/
#define kEffectPropSupportsRenderScale "EffectPropSupportsRenderScale"

/**
 * @brief x1 PluginOpenGLRenderSupport property (optional) indicating whether the plug-in can or need to render with OpenGL
 * Default value - ePluginOpenGLRenderSupportNone
 **/
#define kEffectPropSupportsOpenGLRendering "EffectPropSupportsOpenGLRendering"

/**
 * @brief x1 SequentialPreferenceEnum property (optional) indicating whether the plug-in can only be rendered sequentially
 * Default value - eSequentialPreferenceNotSequential
 **/
#define kEffectPropSupportsSequentialRender "EffectPropSupportsSequentialRender"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in implements the getDistortion action.
 * Default value - false
 **/
#define kEffectPropSupportsCanReturnDistortion "EffectPropSupportsCanReturnDistortion"

/**
 * @brief [Deprecated] x1 bool property (optional) indicating whether the plug-in implements the getTransform action.
 * This is deprecated and the kEffectPropSupportsCanReturnDistortion property should be used instead, along with the new getDistortion action.
 * Default value - false
 **/
#define kEffectPropSupportsCanReturn3x3Transform "EffectPropSupportsCanReturn3x3Transform"

/**
 * @brief x1 bool property (optional) indicating that the plug-in wants to fill the alpha channel with 1 when fetching a source image
 * that is converted automatically from RGB to RGBA.
 * Default value - true
 **/
#define kEffectPropSupportsAlphaFillWith1 "EffectPropSupportsAlphaFillWith1"

/**
 * @brief x1 ImageBufferLayoutEnum property (optional) indicating the images format that the plug-in expects
 * Default value - eImageBufferLayoutRGBAPackedFullRect
 **/
#define kEffectPropImageBufferLayout "EffectPropImageBufferLayout"



class EffectDescription : public PropertiesHolder
{
public:

    EffectDescription();

    virtual ~EffectDescription();
private:

    virtual void initializeProperties() const OVERRIDE FINAL;

};

NATRON_NAMESPACE_EXIT
#endif // EFFECTDESCRIPTION_H
