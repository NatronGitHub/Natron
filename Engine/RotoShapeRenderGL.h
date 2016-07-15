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


#ifndef ROTOSHAPERENDERGL_H
#define ROTOSHAPERENDERGL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include "Engine/EngineFwd.h"
#include "Global/GlobalDefines.h"
#include "Engine/EffectOpenGLContextData.h"

NATRON_NAMESPACE_ENTER;


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
    unsigned int _iboID, _vboVerticesID, _vboColorsID;
    unsigned int _vboHardnessID;

    std::vector<GLShaderBasePtr> _featherRampShader;
    std::vector<GLShaderBasePtr> _strokeDotShader;
    GLShaderBasePtr _strokeDotSecondPassShader;

public:

    void cleanup();

    RotoShapeRenderNodeOpenGLData(bool isGPUContext);

    unsigned int getOrCreateIBOID() ;

    unsigned int getOrCreateVBOVerticesID() ;

    unsigned int getOrCreateVBOColorsID() ;

    unsigned int getOrCreateVBOHardnessID() ;

    GLShaderBasePtr getOrCreateFeatherRampShader(RampTypeEnum type);

    GLShaderBasePtr getOrCreateStrokeDotShader(bool doPremult);

    GLShaderBasePtr getOrCreateStrokeSecondPassShader();

    virtual ~RotoShapeRenderNodeOpenGLData();
    
};

class RotoShapeRenderGL
{
public:
    RotoShapeRenderGL()
    {
        
    }



    static void renderStroke_gl(const OSGLContextPtr& glContext,
                                const boost::shared_ptr<RotoShapeRenderNodeOpenGLData>& glData,
                                const RectI& roi,
                                const ImagePtr& dstImage,
                                const std::list<std::list<std::pair<Point, double> > >& strokes,
                                const double distToNextIn,
                                const Point& lastCenterPointIn,
                                const RotoDrawableItem* stroke,
                                bool doBuildup,
                                double opacity,
                                double time,
                                unsigned int mipmapLevel,
                                double *distToNextOut,
                                Point* lastCenterPointOut);

    static void renderSmear_gl(const OSGLContextPtr& glContext,
                               const boost::shared_ptr<RotoShapeRenderNodeOpenGLData>& glData,
                               const RectI& roi,
                               const ImagePtr& dstImage,
                               const std::list<std::list<std::pair<Point, double> > >& strokes,
                               const double distToNextIn,
                               const Point& lastCenterPointIn,
                               const RotoDrawableItem* stroke,
                               double opacity,
                               double time,
                               unsigned int mipmapLevel,
                               double *distToNextOut,
                               Point* lastCenterPointOut);




    static void renderBezier_gl(const OSGLContextPtr& glContext,
                                const boost::shared_ptr<RotoShapeRenderNodeOpenGLData>& glData,
                                const RectI& roi,
                                const Bezier* bezier,
                                double opacity,
                                double time,
                                double startTime,
                                double endTime,
                                double mbFrameStep,
                                unsigned int mipmapLevel,
                                int target);

};

NATRON_NAMESPACE_EXIT;

#endif // ROTOSHAPERENDERGL_H
