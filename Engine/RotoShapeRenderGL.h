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

#ifndef ROTOSHAPERENDERGL_H
#define ROTOSHAPERENDERGL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/EffectOpenGLContextData.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER


enum RampTypeEnum
{
    // from http://www.comp-fu.com/2012/01/nukes-smooth-ramp-functions/
    // linear
    //y = x
    // plinear: perceptually linear in rec709
    //y = pow(x, 3)
    // smooth: traditional smoothstep
    //y = x*x*(3 - 2*x)
    // smooth0: Catmull-Rom spline, smooth start, linear end
    //y = x*x*(2 - x)
    // smooth1: Catmull-Rom spline, linear start, smooth end
    //y = x*(1 + x*(1 - x))

    eRampTypeLinear,

    // plinear: perceptually linear in rec709
    eRampTypePLinear,

    // smooth0: Catmull-Rom spline, smooth start, linear end
    eRampTypeEaseIn,

    // smooth1: Catmull-Rom spline, linear start, smooth end
    eRampTypeEaseOut,

    // smooth: traditional smoothstep
    eRampTypeSmooth,

};

class RotoShapeRenderNodeOpenGLData : public EffectOpenGLContextData
{
    unsigned int _iboID;
    unsigned int _vboVerticesID;
    unsigned int _vboColorsID;
    unsigned int _vboHardnessID;
    unsigned int _vboTexID;

    std::vector<GLShaderBasePtr> _featherRampShader;
    std::vector<GLShaderBasePtr> _strokeDotShader;
    GLShaderBasePtr _accumShader;
    GLShaderBasePtr _strokeDotSecondPassShader;
    GLShaderBasePtr _smearShader;
    GLShaderBasePtr _divideShader;

public:

    void cleanup();

    RotoShapeRenderNodeOpenGLData(bool isGPUContext);

    unsigned int getOrCreateIBOID();

    unsigned int getOrCreateVBOVerticesID();

    unsigned int getOrCreateVBOColorsID();

    unsigned int getOrCreateVBOHardnessID();

    unsigned int getOrCreateVBOTexID();

    GLShaderBasePtr getOrCreateFeatherRampShader(RampTypeEnum type);

    GLShaderBasePtr getOrCreateAccumulateShader();

    GLShaderBasePtr getOrCreateDivideShader();

    GLShaderBasePtr getOrCreateStrokeDotShader(bool doPremult);

    GLShaderBasePtr getOrCreateStrokeSecondPassShader();

    GLShaderBasePtr getOrCreateSmearShader();

    virtual ~RotoShapeRenderNodeOpenGLData();
    
};

class RotoShapeRenderGL
{
public:
    RotoShapeRenderGL()
    {
        
    }



    static void renderStroke_gl(const OSGLContextPtr& glContext,
                                const RotoShapeRenderNodeOpenGLDataPtr& glData,
                                const RectI& roi,
                                const ImagePtr& dstImage,
                                bool isDuringPaintStrokeDrawing,
                                const double distToNextIn,
                                const Point& lastCenterPointIn,
                                const RotoDrawableItemPtr& stroke,
                                bool doBuildup,
                                double opacity,
                                TimeValue time,
                                ViewIdx view,
                                const RangeD& shutterRange,
                                int nDivisions,
                                const RenderScale& scale,
                                double *distToNextOut,
                                Point* lastCenterPointOut);

    static bool renderSmear_gl(const OSGLContextPtr& glContext,
                               const RotoShapeRenderNodeOpenGLDataPtr& glData,
                               const RectI& roi,
                               const ImagePtr& dstImage,
                               const double distToNextIn,
                               const Point& lastCenterPointIn,
                               const RotoStrokeItemPtr& stroke,
                               double opacity,
                               TimeValue time,
                               ViewIdx view,
                               const RenderScale& scale,
                               double *distToNextOut,
                               Point* lastCenterPointOut);




    static void renderBezier_gl(const OSGLContextPtr& glContext,
                                const RotoShapeRenderNodeOpenGLDataPtr& glData,
                                const RectI& roi,
                                const BezierPtr& bezier,
                                const ImagePtr& dstImage,
                                bool clipToFormat,
                                double opacity,
                                TimeValue time,
                                ViewIdx view,
                                const RangeD& shutterRange,
                                int nDivisions,
                                const RenderScale& scale,
                                int target);

};

NATRON_NAMESPACE_EXIT

#endif // ROTOSHAPERENDERGL_H
