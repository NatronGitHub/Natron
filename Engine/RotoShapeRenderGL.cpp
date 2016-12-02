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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "RotoShapeRenderGL.h"

#include "Engine/OSGLContext.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/RotoBezierTriangulation.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoShapeRenderNode.h"
#include "Engine/RotoShapeRenderNodePrivate.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER;

static const char* rotoRamp_FragmentShader =
"uniform vec4 fillColor;\n"
"uniform float fallOff;\n"
"\n"
"void main() {\n"
"	gl_FragColor = fillColor;\n"
"   float t = gl_Color.a;\n"
"#ifdef RAMP_P_LINEAR\n"
"   t = t * t * t;\n"
"#endif\n"
"#ifdef RAMP_EASE_IN\n"
"   t = t * t * (2.0 - t);\n"
"#endif\n"
"#ifdef RAMP_EASE_OUT\n"
"   t = t * (1.0 + t * (1.0 - t));\n"
"#endif\n"
"#ifdef RAMP_SMOOTH\n"
"   t = t * t * (3.0 - 2.0 * t); \n"
"#endif\n"
"   vec4 outColor = gl_FragColor;\n"
"   outColor.a = pow(t,fallOff);\n"
"   outColor.rgb *= gl_FragColor.a;\n"
"}";


static const char* rotoDrawDot_VertexShader =
"attribute float inHardness;\n"
"varying float outHardness;\n"
"void main() {\n"
"   outHardness = inHardness;\n"
"   gl_Position = ftransform();\n"
"   gl_FrontColor = gl_Color;\n"
"}"
;


static const char* roto_AccumulateShader =
"uniform sampler2D sampleTex;\n"
"#ifdef ACCUMULATE\n"
"uniform sampler2D accumTex;\n"
"#endif\n"
"\n"
"void main() {\n"
"   vec4 sampleColor = texture2D(sampleTex,gl_TexCoord[0].st);\n"
"#ifndef ACCUMULATE\n"
"   gl_FragColor = sampleColor;\n"
"#else\n"
"   vec4 accumColor = texture2D(accumTex,gl_TexCoord[0].st);\n"
"   gl_FragColor.r = accumColor.r + sampleColor.r;\n"
"   gl_FragColor.g = accumColor.g + sampleColor.g;\n"
"   gl_FragColor.b = accumColor.b + sampleColor.b;\n"
"   gl_FragColor.a = accumColor.a + sampleColor.a;\n"
"#endif\n"
"}";

static const char* roto_DivideShader =
"uniform sampler2D srcTex;\n"
"uniform float nDivisions;\n"
"\n"
"void main() {\n"
"   vec4 srcColor = texture2D(srcTex,gl_TexCoord[0].st);\n"
"   srcColor.r /= nDivisions; \n"
"   srcColor.g /= nDivisions; \n"
"   srcColor.b /= nDivisions; \n"
"   srcColor.a /= nDivisions; \n"
"   gl_FragColor = srcColor; \n"
"}";

static const char* rotoDrawDot_FragmentShader =
"varying float outHardness;\n"
"uniform vec4 fillColor;\n"
"float gaussLookup(float t) {\n"
"   if (t < -0.5) {\n"
"       t = -1.0 - t;\n"
"       return (2.0 * t * t);\n"
"   }\n"
"   if (t < 0.5) {\n"
"       return (1.0 - 2.0 * t * t);\n"
"   }\n"
"   t = 1.0 - t;\n"
"   return (2.0 * t * t);\n"
"}\n"
"void main() {\n"
"#ifndef BUILDUP\n"
"	gl_FragColor = fillColor;\n"
"#else\n"
"   gl_FragColor.rgb = vec3(1.0, 1.0, 1.0);\n"
"#endif\n"
"   if (outHardness == 1.0) {\n"
"       gl_FragColor.a = 1.0;\n"
"   } else {\n"
"       float exp = 0.4 / (1.0 - outHardness);\n"
"       gl_FragColor.a = clamp(gaussLookup(pow(1.0 - gl_Color.a, exp)), 0.0, 1.0);\n"
"   }\n"
"#ifndef BUILDUP\n"
"   gl_FragColor.a *= fillColor.a; \n"
"   gl_FragColor.rgb *= gl_FragColor.a;\n"
"#endif\n"
"}"
;

static const char* rotoDrawDotBuildUpSecondPass_FragmentShader =
"uniform sampler2D tex;"
"uniform vec4 fillColor;\n"
"void main() {\n"
"   vec4 srcColor = texture2D(tex,gl_TexCoord[0].st);\n"
"   gl_FragColor.r = srcColor.r * fillColor.r;\n"
"   gl_FragColor.g = srcColor.r * fillColor.g;\n"
"   gl_FragColor.b = srcColor.r * fillColor.b;\n"
"   gl_FragColor.a = srcColor.r * fillColor.a;\n"
"}";


static const char* rotoSmearDot_VertexShader =
"attribute float inHardness;\n"
"varying float outHardness;\n"
"void main() {\n"
"   outHardness = inHardness;\n"
"   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"   gl_Position = ftransform();\n"
"   gl_FrontColor = gl_Color;\n"
"}"
;

static const char* rotoSmearDot_FragmentShader =
"varying float outHardness;\n"
"uniform sampler2D tex;\n"
"uniform vec4 fillColor;\n"
"float gaussLookup(float t) {\n"
"   if (t < -0.5) {\n"
"       t = -1.0 - t;\n"
"       return (2.0 * t * t);\n"
"   }\n"
"   if (t < 0.5) {\n"
"       return (1.0 - 2.0 * t * t);\n"
"   }\n"
"   t = 1.0 - t;\n"
"   return (2.0 * t * t);\n"
"}\n"
"void main() {\n"
"   gl_FragColor = texture2D(tex,gl_TexCoord[0].st);\n"
"   float alpha;\n"
"   if (outHardness == 1.0) {\n"
"       alpha = 1.0;\n"
"   } else {\n"
"       float exp = 0.4 / (1.0 - outHardness);\n"
"       alpha = clamp(gaussLookup(pow(1.0 - gl_Color.a, exp)), 0.0, 1.0);\n"
"   }\n"
"   alpha *= fillColor.a; \n"
"   gl_FragColor.a = alpha;\n"
"   //gl_FragColor.rgb *= alpha;\n"
"}"
;

RotoShapeRenderNodeOpenGLData::RotoShapeRenderNodeOpenGLData(bool isGPUContext)
: EffectOpenGLContextData(isGPUContext)
, _iboID(0)
, _vboVerticesID(0)
, _vboColorsID(0)
, _vboHardnessID(0)
, _vboTexID(0)
, _featherRampShader(5)
, _strokeDotShader(2)
, _accumShader(2)
{

}

void
RotoShapeRenderNodeOpenGLData::cleanup()
{
    _featherRampShader.clear();
    _strokeDotShader.clear();
    bool isGPU = isGPUContext();
    if (_vboVerticesID) {
        if (isGPU) {
            GL_GPU::DeleteBuffers(1, &_vboVerticesID);
        } else {
            GL_CPU::DeleteBuffers(1, &_vboVerticesID);
        }
    }

    if (_vboColorsID) {
        if (isGPU) {
            GL_GPU::DeleteBuffers(1, &_vboColorsID);
        } else {
            GL_CPU::DeleteBuffers(1, &_vboColorsID);
        }
    }

    if (_vboHardnessID) {
        if (isGPU) {
            GL_GPU::DeleteBuffers(1, &_vboHardnessID);
        } else {
            GL_CPU::DeleteBuffers(1, &_vboHardnessID);
        }
    }

    if (_iboID) {
        if (isGPU) {
            GL_GPU::DeleteBuffers(1, &_iboID);
        } else {
            GL_CPU::DeleteBuffers(1, &_iboID);
        }
    }
}


RotoShapeRenderNodeOpenGLData::~RotoShapeRenderNodeOpenGLData()
{

}


unsigned int
RotoShapeRenderNodeOpenGLData::getOrCreateIBOID()
{

    if (!_iboID) {
        if (isGPUContext()) {
            GL_GPU::GenBuffers(1, &_iboID);
        } else {
            GL_CPU::GenBuffers(1, &_iboID);
        }
    }
    return _iboID;

}

unsigned int
RotoShapeRenderNodeOpenGLData::getOrCreateVBOVerticesID()
{
    if (!_vboVerticesID) {
        if (isGPUContext()) {
            GL_GPU::GenBuffers(1, &_vboVerticesID);
        } else {
            GL_CPU::GenBuffers(1, &_vboVerticesID);
        }
    }
    return _vboVerticesID;

}

unsigned int
RotoShapeRenderNodeOpenGLData::getOrCreateVBOColorsID()
{
    if (!_vboColorsID) {
        if (isGPUContext()) {
            GL_GPU::GenBuffers(1, &_vboColorsID);
        } else {
            GL_CPU::GenBuffers(1, &_vboColorsID);
        }
    }
    return _vboColorsID;
}

unsigned int
RotoShapeRenderNodeOpenGLData::getOrCreateVBOHardnessID()
{
    if (!_vboHardnessID) {
        if (isGPUContext()) {
            GL_GPU::GenBuffers(1, &_vboHardnessID);
        } else {
            GL_CPU::GenBuffers(1, &_vboHardnessID);
        }
    }
    return _vboHardnessID;

}

unsigned int
RotoShapeRenderNodeOpenGLData::getOrCreateVBOTexID()
{
    if (!_vboTexID) {
        if (isGPUContext()) {
            GL_GPU::GenBuffers(1, &_vboTexID);
        } else {
            GL_CPU::GenBuffers(1, &_vboTexID);
        }
    }
    return _vboTexID;

}

template <typename GL>
static GLShaderBasePtr
getOrCreateFeatherRampShaderInternal(RampTypeEnum type)
{
    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );

    std::string fragmentSource;

    switch (type) {
        case eRampTypeLinear:
            break;
        case eRampTypePLinear:
            fragmentSource += "#define RAMP_P_LINEAR\n";
            break;
        case eRampTypeEaseIn:
            fragmentSource += "#define RAMP_EASE_IN\n";
            break;
        case eRampTypeEaseOut:
            fragmentSource += "#define RAMP_EASE_OUT\n";
            break;
        case eRampTypeSmooth:
            fragmentSource += "#define RAMP_SMOOTH\n";
            break;
    }


    fragmentSource += std::string(rotoRamp_FragmentShader);

#ifdef DEBUG
    std::string error;
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);
    
    return shader;
}

GLShaderBasePtr
RotoShapeRenderNodeOpenGLData::getOrCreateFeatherRampShader(RampTypeEnum type)
{
    int type_i = (int)type;
    if (_featherRampShader[type_i]) {
        return _featherRampShader[type_i];
    }
    if (isGPUContext()) {
        _featherRampShader[type_i] = getOrCreateFeatherRampShaderInternal<GL_GPU>(type);
    } else {
        _featherRampShader[type_i] = getOrCreateFeatherRampShaderInternal<GL_CPU>(type);
    }

    return _featherRampShader[type_i];
}


template <typename GL>
static GLShaderBasePtr
getOrCreateStrokeDotShaderInternal(bool buildUp)
{
    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );

    std::string vertexSource;
    vertexSource += std::string(rotoDrawDot_VertexShader);

#ifdef DEBUG
    std::string error;
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeVertex, vertexSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeVertex, vertexSource.c_str(), 0);
#endif
    assert(ok);


    std::string fragmentSource;
    if (buildUp) {
        fragmentSource += "#define BUILDUP\n";
    }
    fragmentSource += std::string(rotoDrawDot_FragmentShader);
#ifdef DEBUG
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
}


GLShaderBasePtr
RotoShapeRenderNodeOpenGLData::getOrCreateStrokeDotShader(bool buildUp)
{
    int index = (int)buildUp;
    if (_strokeDotShader[index]) {
        return _strokeDotShader[index];
    }
    if (isGPUContext()) {
        _strokeDotShader[index] = getOrCreateStrokeDotShaderInternal<GL_GPU>(buildUp);
    } else {
        _strokeDotShader[index] = getOrCreateStrokeDotShaderInternal<GL_CPU>(buildUp);
    }

    return _strokeDotShader[index];
}

template <typename GL>
static GLShaderBasePtr
getOrCreateAccumShaderInternal(bool accum)
{
    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );
    bool ok;
    std::string fragmentSource;

    if (accum) {
        fragmentSource += "#define ACCUMULATE\n";
    }
    fragmentSource += std::string(roto_AccumulateShader);
#ifdef DEBUG
    std::string error;
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
}

template <typename GL>
static GLShaderBasePtr
getOrCreateDivideShaderInternal()
{
    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );
    bool ok;
    std::string fragmentSource;

    fragmentSource += std::string(roto_DivideShader);
#ifdef DEBUG
    std::string error;
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
}

GLShaderBasePtr
RotoShapeRenderNodeOpenGLData::getOrCreateAccumulateShader(bool accum)
{
    int index = (int)accum;
    if (_accumShader[index]) {
        return _accumShader[index];
    }
    if (isGPUContext()) {
        _accumShader[index] = getOrCreateAccumShaderInternal<GL_GPU>(accum);
    } else {
        _accumShader[index] = getOrCreateAccumShaderInternal<GL_GPU>(accum);
    }
    return _accumShader[index];
}

GLShaderBasePtr
RotoShapeRenderNodeOpenGLData::getOrCreateDivideShader()
{
    if (_divideShader) {
        return _divideShader;
    }
    if (isGPUContext()) {
        _divideShader = getOrCreateDivideShaderInternal<GL_GPU>();
    } else {
        _divideShader = getOrCreateDivideShaderInternal<GL_CPU>();
    }
    return _divideShader;
}

template <typename GL>
static GLShaderBasePtr
getOrCreateStrokeSecondPassShaderInternal()
{
    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );

    bool ok;
    std::string fragmentSource;
    fragmentSource += std::string(rotoDrawDotBuildUpSecondPass_FragmentShader);
#ifdef DEBUG
    std::string error;
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
}


GLShaderBasePtr
RotoShapeRenderNodeOpenGLData::getOrCreateStrokeSecondPassShader()
{
    if (_strokeDotSecondPassShader) {
        return _strokeDotSecondPassShader;
    }
    if (isGPUContext()) {
        _strokeDotSecondPassShader = getOrCreateStrokeSecondPassShaderInternal<GL_GPU>();
    } else {
        _strokeDotSecondPassShader = getOrCreateStrokeSecondPassShaderInternal<GL_CPU>();
    }

    return _strokeDotSecondPassShader;
}


template <typename GL>
static GLShaderBasePtr
getOrCreateSmearShaderInternal()
{
    boost::shared_ptr<GLShader<GL> > shader( new GLShader<GL>() );

    std::string vertexSource;
    vertexSource += std::string(rotoSmearDot_VertexShader);

#ifdef DEBUG
    std::string error;
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeVertex, vertexSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    bool ok = shader->addShader(GLShader<GL>::eShaderTypeVertex, vertexSource.c_str(), 0);
#endif
    assert(ok);


    std::string fragmentSource;
    fragmentSource += std::string(rotoSmearDot_FragmentShader);
#ifdef DEBUG
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), &error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->addShader(GLShader<GL>::eShaderTypeFragment, fragmentSource.c_str(), 0);
#endif
    assert(ok);
#ifdef DEBUG
    ok = shader->link(&error);
    if (!ok) {
        qDebug() << error.c_str();
    }
#else
    ok = shader->link();
#endif
    assert(ok);
    Q_UNUSED(ok);

    return shader;
}


GLShaderBasePtr
RotoShapeRenderNodeOpenGLData::getOrCreateSmearShader()
{
    if (_smearShader) {
        return _smearShader;
    }
    if (isGPUContext()) {
        _smearShader = getOrCreateSmearShaderInternal<GL_GPU>();
    } else {
        _smearShader = getOrCreateSmearShaderInternal<GL_CPU>();
    }

    return _smearShader;
}



template <typename GL>
void setupTexParams(int target)
{

    GL::TexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL::TexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GL::TexParameteri (target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    GL::TexParameteri (target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

}

template <typename GL>
void renderBezier_gl_internal_begin(bool doBuildUp)
{



    GL::Enable(GL_BLEND);
    GL::BlendColor(1, 1, 1, 1);
    if (!doBuildUp) {
        GL::BlendEquation(GL_MAX);
        GL::BlendFunc(GL_ONE, GL_ZERO);

    } else {
        GL::BlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
        GL::BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    }


}


template <typename GL>
void renderBezier_gl_singleDrawElements(int nbVertices, int nbIds, int vboVerticesID, int vboColorsID, int iboID, unsigned int primitiveType, const void* verticesData, const void* colorsData, const void* idsData, bool uploadVertices = true)
{

    GL::BindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    if (uploadVertices) {
        GL::BufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), verticesData, GL_DYNAMIC_DRAW);
    }
    GL::EnableClientState(GL_VERTEX_ARRAY);
    GL::VertexPointer(2, GL_FLOAT, 0, 0);

    GL::BindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    if (uploadVertices) {
        GL::BufferData(GL_ARRAY_BUFFER, nbVertices * 4 * sizeof(GLfloat), colorsData, GL_DYNAMIC_DRAW);
    }
    GL::EnableClientState(GL_COLOR_ARRAY);
    GL::ColorPointer(4, GL_FLOAT, 0, 0);

    GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
    GL::BufferData(GL_ELEMENT_ARRAY_BUFFER, nbIds * sizeof(GLuint), idsData, GL_DYNAMIC_DRAW);


    GL::DrawElements(primitiveType, nbIds, GL_UNSIGNED_INT, 0);

    GL::DisableClientState(GL_COLOR_ARRAY);
    GL::BindBuffer(GL_ARRAY_BUFFER, 0);
    GL::DisableClientState(GL_VERTEX_ARRAY);

    GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError(GL);

}

template <typename GL>
void renderBezier_gl_multiDrawElements(int nbVertices, int vboVerticesID, unsigned int primitiveType, const void* verticesData, const int* perDrawCount, const void** perDrawIdsPtr, int drawCount, bool uploadVertices = true)
{

    GL::BindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    if (uploadVertices) {
        GL::BufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), verticesData, GL_DYNAMIC_DRAW);
    }
    GL::EnableClientState(GL_VERTEX_ARRAY);
    GL::VertexPointer(2, GL_FLOAT, 0, 0);

    GL::MultiDrawElements(primitiveType, perDrawCount, GL_UNSIGNED_INT, perDrawIdsPtr, drawCount);

    GL::DisableClientState(GL_COLOR_ARRAY);
    GL::BindBuffer(GL_ARRAY_BUFFER, 0);
    GL::DisableClientState(GL_VERTEX_ARRAY);

    GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError(GL);

}

template <typename GL>
void
renderBezier_gl_internal(const OSGLContextPtr& glContext,
                         const RotoShapeRenderNodeOpenGLDataPtr& glData,
                         const RectI& roi,
                         const BezierPtr& bezier,
                         const ImagePtr& dstImage,
                         double opacity,
                         double time,
                         ViewIdx view,
                         const RangeD& shutterRange,
                         int nDivisions,
                         unsigned int mipmapLevel,
                         int target)
{
    Q_UNUSED(roi);
    int vboVerticesID = glData->getOrCreateVBOVerticesID();
    int vboColorsID = glData->getOrCreateVBOColorsID();
    int iboID = glData->getOrCreateIBOID();

    RamBuffer<float> verticesArray;
    RamBuffer<float> colorsArray;
    RamBuffer<unsigned int> indicesArray;


    RampTypeEnum type;
    {
        KnobChoicePtr typeKnob = bezier->getFallOffRampTypeKnob();
        type = (RampTypeEnum)typeKnob->getValue();
    }
    GLShaderBasePtr rampShader = glData->getOrCreateFeatherRampShader(type);
    GLShaderBasePtr fillShader = glContext->getOrCreateFillShader();
    GLShaderBasePtr accumShader[2];
    accumShader[0] = glData->getOrCreateAccumulateShader(false);
    accumShader[1] = glData->getOrCreateAccumulateShader(true);
    GLShaderBasePtr divideShader = glData->getOrCreateDivideShader();

    GLuint fboID = glContext->getOrCreateFBOId();

    // If we do more than 1 pass, do motion blur
    // When motion-blur is enabled, draw each sample to a temporary texture in a first pass.
    // On the first sample, just copy the sample texture to the accumulation texture.
    // Then for other samples, copy the accumulation texture so we can read it in input
    // and add the sample texture to the accumulation copy and write to the original accumulation texture.
    // After the last sample, copy the accumulation texture to the dst framebuffer by dividing by the number of steps.

    double interval = nDivisions >= 1 ? (shutterRange.max - shutterRange.min) / nDivisions : 1.;

    ImagePtr perSampleRenderTexture, accumulationTexture, tmpAccumulationCpy;
    if (nDivisions > 1) {

        // Make 2 textures of the same size as the dst image.
        ImagePtr* tmpTex[3] = {&perSampleRenderTexture, &accumulationTexture, &tmpAccumulationCpy};
        for (int i = 0; i < 3; ++i) {
            ImageParamsPtr params(new ImageParams(*dstImage->getParams()));
            params->setBounds(roi);
            {
                CacheEntryStorageInfo& info = params->getStorageInfo();
                info.textureTarget = target;
                info.isGPUTexture = glContext->isGPUContext();
                info.mode = eStorageModeGLTex;
            }
            tmpTex[i]->reset( new Image(dstImage->getKey(), params) );
        }
        assert(perSampleRenderTexture && accumulationTexture);
    }

    for (int d = 0; d < nDivisions; ++d) {

        // If motion-blur enabled, bind the per sample texture so we render on it
        if (perSampleRenderTexture) {
            GL::BindTexture( target, perSampleRenderTexture->getGLTextureID() );
            setupTexParams<GL>(target);

            GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
            GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, perSampleRenderTexture->getGLTextureID(), 0 /*LoD*/);
            glCheckFramebufferError(GL);
            GL::BindTexture( target, perSampleRenderTexture->getGLTextureID() );
        }

        double t = nDivisions > 1 ? shutterRange.min + d * interval : time;

        double fallOff = bezier->getFeatherFallOffKnob()->getValueAtTime(t, DimIdx(0), view);
        double featherDist = bezier->getFeatherKnob()->getValueAtTime(t, DimIdx(0), view);
        std::vector<double> shapeColor(3);
        {
            KnobColorPtr colorKnob = bezier->getColorKnob();
            for (int i = 0; i < 3; ++i) {
                shapeColor[i] = colorKnob->getValueAtTime(t, DimIdx(i), view);
            }
        }


        ///Adjust the feather distance so it takes the mipmap level into account
        if (mipmapLevel != 0) {
            featherDist /= (1 << mipmapLevel);
        }

        // Compute the feather triangles as well as the internal shape triangles.
        RotoBezierTriangulation::PolygonData data;
        RotoBezierTriangulation::computeTriangles(bezier, t, view, mipmapLevel, featherDist, &data);

        // Tex parameters may not have been set yet in GPU mode if motion blur is disabled
        if (GL::isGPU() && !perSampleRenderTexture) {
            setupTexParams<GL>(target);
        }

        renderBezier_gl_internal_begin<GL>(false);


        rampShader->bind();
        OfxRGBAColourF fillColor = {(float)shapeColor[0], (float)shapeColor[1], (float)shapeColor[2], (float)opacity};
        rampShader->setUniform("fillColor", fillColor);
        rampShader->setUniform("fallOff", (float)fallOff);


        // First upload and render the feather mesh which is composed of GL_TRIANGLES
        {

            int nbVertices = data.featherMesh.size();
            if (!nbVertices) {
                continue;
            }

            verticesArray.resize(nbVertices * 2);
            colorsArray.resize(nbVertices * 4);
            indicesArray.resize(nbVertices);

            // Fill buffer
            float* v_data = verticesArray.getData();
            float* c_data = colorsArray.getData();
            unsigned int* i_data = indicesArray.getData();

            for (std::size_t i = 0; i < data.featherMesh.size(); ++i,
                 v_data += 2,
                 c_data += 4,
                 ++i_data) {
                v_data[0] = data.featherMesh[i].x;
                v_data[1] = data.featherMesh[i].y;
                i_data[0] = i;

                // The roi was computed from the RoD, it must include the feather points.
                // If this crashes here, this is likely because either the computation of the RoD is wrong
                // or the Knobs of the Bezier have changed since the RoD computation or the Bezier shape itself
                // has changed since the RoD computation. In all cases datas should remain the same thoughout the render
                // since we are operating on a thread-local render clone.
                assert(roi.contains(v_data[0], v_data[1]));

                c_data[0] = shapeColor[0];
                c_data[1] = shapeColor[1];
                c_data[2] = shapeColor[2];
                if (data.featherMesh[i].isInner) {
                    c_data[3] = opacity;
                } else {
                    c_data[3] = 0.;
                }
            }
            renderBezier_gl_singleDrawElements<GL>(nbVertices, nbVertices, vboVerticesID, vboColorsID, iboID, GL_TRIANGLES, (const void*)verticesArray.getData(), (const void*)colorsArray.getData(), (const void*)indicesArray.getData());

        }

        // Unbind the Ramp shader used for the feather and bind the Fill shader for the Roto
        rampShader->unbind();
        fillShader->bind();
        fillShader->setUniform("fillColor", fillColor);

        int nbVertices = data.bezierPolygonJoined.size();
        if (!nbVertices) {
            continue;
        }

        verticesArray.resize(nbVertices * 2);


        // Fill the vertices buffer with all the points of the internal shape polygon
        // Each 3 pass of render (the triangles pass, the triangles fan pass, and the triangles strip pass)
        // will reference these vertices.
        {
            float* v_data = verticesArray.getData();
            for (std::size_t i = 0; i < data.bezierPolygonJoined.size(); ++i, v_data += 2) {
                v_data[0] = data.bezierPolygonJoined[i].x;
                v_data[1] = data.bezierPolygonJoined[i].y;

                // The roi was computed from the bounds, it must include the internal shape points.
                assert(roi.contains(v_data[0], v_data[1]));
            }
        }
        bool hasUploadedVertices = false;
        {
            // Render internal triangles
            // Merge all set of GL_TRIANGLES into a single call of glMultiDrawElements
            int drawCount = (int)data.internalTriangles.size();

            if (drawCount) {
                std::vector<const void*> perDrawsIDVec(drawCount);
                std::vector<int> perDrawCount(drawCount);
                for (std::size_t i = 0; i < data.internalTriangles.size(); ++i) {
                    perDrawsIDVec[i] = (const void*)(&data.internalTriangles[i].indices[0]);
                    perDrawCount[i] = (int)data.internalTriangles[i].indices.size();
                }

                renderBezier_gl_multiDrawElements<GL>(nbVertices, vboVerticesID, GL_TRIANGLES, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount);

                hasUploadedVertices = true;

            }


        }
        {
            // Render internal triangle fans

            int drawCount = (int)data.internalFans.size();

            if (drawCount) {
                std::vector<const void*> perDrawsIDVec(drawCount);
                std::vector<int> perDrawCount(drawCount);
                for (std::size_t i = 0; i < data.internalFans.size(); ++i) {
                    perDrawsIDVec[i] = (const void*)(&data.internalFans[i].indices[0]);
                    perDrawCount[i] = (int)data.internalFans[i].indices.size();
                }
                renderBezier_gl_multiDrawElements<GL>(nbVertices, vboVerticesID, GL_TRIANGLE_FAN, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount, !hasUploadedVertices);
                hasUploadedVertices = true;
            }

        }
        {
            // Render internal triangle strips

            int drawCount = (int)data.internalStrips.size();

            if (drawCount) {
                std::vector<const void*> perDrawsIDVec(drawCount);
                std::vector<int> perDrawCount(drawCount);
                for (std::size_t i = 0; i < data.internalStrips.size(); ++i) {
                    perDrawsIDVec[i] = (const void*)(&data.internalStrips[i].indices[0]);
                    perDrawCount[i] = (int)data.internalStrips[i].indices.size();
                }
                renderBezier_gl_multiDrawElements<GL>(nbVertices, vboVerticesID, GL_TRIANGLE_STRIP, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount, !hasUploadedVertices);
                hasUploadedVertices = true;
            }
            
        }
        Q_UNUSED(hasUploadedVertices);

        GL::Disable(GL_BLEND);
        glCheckError(GL);

        // If motion-blur is enabled, we still have to accumulate, otherwise we are done.
        if (nDivisions > 1) {

            if (d == 0) {
                // On the first sample, just copy the sample to the accumulation texture as we need to initialize it.

                GL::BindTexture( target, accumulationTexture->getGLTextureID() );
                setupTexParams<GL>(target);

                GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
                GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, accumulationTexture->getGLTextureID(), 0 /*LoD*/);
                glCheckFramebufferError(GL);

                GL::BindTexture( target, perSampleRenderTexture->getGLTextureID() );
                accumShader[0]->bind();
                accumShader[0]->setUniform("sampleTex", 0);
                Image::applyTextureMapping<GL>(roi, roi, roi);
                accumShader[0]->unbind();

            } else {
                // If this is not the first sample, duplicate the accumulation texture so we can read the duplicate in input and write to the
                // accumulation texture in output.

                GL::BindTexture( target, tmpAccumulationCpy->getGLTextureID() );
                setupTexParams<GL>(target);

                GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
                GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, tmpAccumulationCpy->getGLTextureID(), 0 /*LoD*/);
                glCheckFramebufferError(GL);

                GL::BindTexture( target, accumulationTexture->getGLTextureID() );
                accumShader[0]->bind();
                accumShader[0]->setUniform("sampleTex", 0);
                Image::applyTextureMapping<GL>(roi, roi, roi);
                accumShader[0]->unbind();


                // Now do the accumulation
                GL::BindTexture( target, accumulationTexture->getGLTextureID() );
                setupTexParams<GL>(target);

                GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
                GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, accumulationTexture->getGLTextureID(), 0 /*LoD*/);
                glCheckFramebufferError(GL);

                GL::ActiveTexture(GL_TEXTURE0);
                GL::BindTexture( target, tmpAccumulationCpy->getGLTextureID() );
                GL::ActiveTexture(GL_TEXTURE1);
                GL::BindTexture( target, perSampleRenderTexture->getGLTextureID() );
                accumShader[1]->bind();
                accumShader[1]->setUniform("accumTex", 0);
                accumShader[1]->setUniform("sampleTex", 1);
                Image::applyTextureMapping<GL>(roi, roi, roi);
                accumShader[1]->unbind();
                GL::ActiveTexture(GL_TEXTURE0);
            }
        }
    } // for all samples

    if (nDivisions > 1) {
        // Copy the accumulation buffer to the destination framebuffer and divide by the number of samples.
        RectI outputBounds;
        if (GL::isGPU()) {
            outputBounds = dstImage->getBounds();
            GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
            GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, dstImage->getGLTextureID(), 0 /*LoD*/);
            glCheckFramebufferError(GL);
        } else {
            outputBounds = roi;
            // In CPU mode render to the default framebuffer that is already backed by the dstImage
            GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        GL::BindTexture( target, accumulationTexture->getGLTextureID() );
        divideShader->bind();
        divideShader->setUniform("srcTex", 0);
        divideShader->setUniform("nDivisions", (float)nDivisions);
        Image::applyTextureMapping<GL>(roi, outputBounds, roi);
        divideShader->unbind();
        glCheckError(GL);
        

    }
} // renderBezier_gl_internal


void
RotoShapeRenderGL::renderBezier_gl(const OSGLContextPtr& glContext,
                                   const RotoShapeRenderNodeOpenGLDataPtr& glData,
                                   const RectI& roi,
                                   const BezierPtr& bezier,
                                   const ImagePtr& dstImage,
                                   double opacity,
                                   double time,
                                   ViewIdx view,
                                   const RangeD& shutterRange,
                                   int nDivisions,
                                   unsigned int mipmapLevel,
                                   int target)
{
    if (glContext->isGPUContext()) {
        renderBezier_gl_internal<GL_GPU>(glContext, glData, roi, bezier, dstImage, opacity, time, view, shutterRange, nDivisions, mipmapLevel, target);
    } else {
        renderBezier_gl_internal<GL_CPU>(glContext, glData, roi, bezier, dstImage, opacity, time, view, shutterRange, nDivisions, mipmapLevel, target);
    }
} // RotoShapeRenderGL::renderBezier_gl



struct RenderStrokeGLData
{
    OSGLContextPtr glContext;
    RotoShapeRenderNodeOpenGLDataPtr glData;

    ImagePtr dstImage;

    double brushSizePixel;
    double brushSpacing;
    double brushHardness;
    bool pressureAffectsOpacity;
    bool pressureAffectsHardness;
    bool pressureAffectsSize;
    bool buildUp;
    double shapeColor[3];
    double opacity;

    int nbPointsPerSegment;

    RectI roi;

    RamBuffer<float> primitivesColors;
    RamBuffer<float> primitivesVertices;
    RamBuffer<float> primitivesHardness;
    std::vector<boost::shared_ptr<RamBuffer<unsigned int> > > indicesBuf;
};

static void toTexCoords(const RectI& texBounds, const float xi, const float yi, float* xo, float* yo)
{
    *xo = (xi - texBounds.x1) / (double)texBounds.width();
    *yo = (yi - texBounds.y1) / (double)texBounds.height();
}

/**
 * @brief Makes up a single dot that will be send to the GPU
 **/
static void
getDotTriangleFan(const Point& center,
                  double radius,
                  const int nbOutsideVertices,
                  double shapeColor[3],
                  double opacity,
                  double hardness,
                  bool doBuildUp,
                  const RectI& texBounds,
                  bool appendToData,
                  RamBuffer<float>* vdata,
                  RamBuffer<float>* cdata,
                  RamBuffer<float>* hdata,
                  RamBuffer<float>* tdata)
{
    /*
     To render a dot we make a triangle fan. Along each edge linking an outter vertex and the center of the dot the ramp is interpolated by OpenGL.
     The function in the shader is the same as the one we use to fill the radial gradient used in Cairo. Since in Cairo we use stops that do not make linear curve,
     instead we slightly alter the hardness by a very sharp log curve so it mimics correctly the radial gradient used in CPU mode.
     */
    if (doBuildUp) {
        hardness = std::pow(hardness, 0.7f);
    } else {
        hardness = std::pow(hardness, 0.3f);
    }

    int cSize = appendToData ? cdata->size() : 0;
    cdata->resizeAndPreserve(cSize + (nbOutsideVertices + 2) * 4);
    int vSize = appendToData ? vdata->size() : 0;
    vdata->resizeAndPreserve(vSize + (nbOutsideVertices + 2) * 2);
    int hSize = appendToData ? hdata->size() : 0;
    hdata->resizeAndPreserve(hSize + (nbOutsideVertices + 2) * 1);

    int tSize = 0;
    if (tdata) {
        tSize = appendToData ? tdata->size() : 0;
        tdata->resizeAndPreserve(tSize + (nbOutsideVertices + 2) * 2);
    }

    float* cPtr = cdata->getData() + cSize;
    float* vPtr = vdata->getData() + vSize;
    float* hPtr = hdata->getData() + hSize;
    float* tPtr = tdata ? tdata->getData() + tSize : 0;
    vPtr[0] = center.x;
    vPtr[1] = center.y;
    assert(texBounds.contains(vPtr[0],vPtr[1]));
    if (tPtr) {
        toTexCoords(texBounds, center.x, center.y, &tPtr[0], &tPtr[1]);
        tPtr += 2;
    }
    cPtr[0] = shapeColor[0];
    cPtr[1] = shapeColor[1];
    cPtr[2] = shapeColor[2];
    cPtr[3] = opacity;
    *hPtr = hardness;
    vPtr += 2;
    cPtr += 4;
    ++hPtr;

    double m = 2. * M_PI / (double)nbOutsideVertices;
    for (int i = 0; i < nbOutsideVertices; ++i) {
        double theta = i * m;
        vPtr[0] = center.x + radius * std::cos(theta);
        vPtr[1] = center.y + radius * std::sin(theta);
        assert(texBounds.contains(vPtr[0],vPtr[1]));
        if (tPtr) {
            toTexCoords(texBounds, vPtr[0], vPtr[1], &tPtr[0], &tPtr[1]);
            tPtr += 2;
        }
        vPtr += 2;



        cPtr[0] = shapeColor[0];
        cPtr[1] = shapeColor[1];
        cPtr[2] = shapeColor[2];
        cPtr[3] = 0.;
        cPtr += 4;

        *hPtr = hardness;
        ++hPtr;
    }

    vPtr[0] = center.x + radius;
    vPtr[1] = center.y;
    assert(texBounds.contains(vPtr[0],vPtr[1]));
    if (tPtr) {
        toTexCoords(texBounds, vPtr[0], vPtr[1], &tPtr[0], &tPtr[1]);
    }
    cPtr[0] = shapeColor[0];
    cPtr[1] = shapeColor[1];
    cPtr[2] = shapeColor[2];
    cPtr[3] = 0.;
    *hPtr = hardness;

}

static void renderDot_gl(RenderStrokeGLData& data, const Point &center, double radius, double shapeColor[3], double opacity, double hardness)
{

    // Create the indices buffer for this triangle fan
    {
        boost::shared_ptr<RamBuffer<unsigned int> > vec(new RamBuffer<unsigned int>);
        data.indicesBuf.push_back(vec);
        unsigned int vSize = data.primitivesVertices.size() / 2;
        vec->resize(data.nbPointsPerSegment + 2);
        unsigned int* idxData = vec->getData();
        for (std::size_t i = 0; i < vec->size(); ++i, ++vSize) {
            idxData[i] = vSize;
        }
    }

    getDotTriangleFan(center, radius, data.nbPointsPerSegment, shapeColor, opacity, hardness, data.buildUp, data.roi, true, &data.primitivesVertices, &data.primitivesColors, &data.primitivesHardness, 0);
    

}

/*
 * Approximate the necessary number of line segments to draw a dot, using http://antigrain.com/research/adaptive_bezier/
 */
static int getNbPointPerSegment(const double brushSizePixel)
{
    double radius = brushSizePixel / 2.;
    Point p0 = {radius, 0.};
    Point p3 = {0., radius};
    Point p1 = {1. / .3 * p3.x + 2. / 3 * p0.x, 1. / .3 * p3.y + 2. / 3 * p0.y};
    Point p2 = {1. / .3 * p0.x + 2. / 3 * p3.x, 1. / .3 * p0.y + 2. / 3 * p3.y};
    double dx1, dy1, dx2, dy2, dx3, dy3;
    dx1 = p1.x - p0.x;
    dy1 = p1.y - p0.y;
    dx2 = p2.x - p1.x;
    dy2 = p2.y - p1.y;
    dx3 = p3.x - p2.x;
    dy3 = p3.y - p2.y;
    double length = std::sqrt(dx1 * dx1 + dy1 * dy1) +
    std::sqrt(dx2 * dx2 + dy2 * dy2) +
    std::sqrt(dx3 * dx3 + dy3 * dy3);
    return (int)std::max(length * 0.1 /*0.25*/, 2.);
}

static void
renderStrokeBegin_gl(RotoShapeRenderNodePrivate::RenderStrokeDataPtr userData,
                     double brushSizePixel,
                     double brushSpacing,
                     double brushHardness,
                     bool pressureAffectsOpacity,
                     bool pressureAffectsHardness,
                     bool pressureAffectsSize,
                     bool buildUp,
                     double shapeColor[3],
                     double opacity)
{
    RenderStrokeGLData* myData = (RenderStrokeGLData*)userData;

    myData->brushSizePixel = brushSizePixel;
    myData->brushSpacing = brushSpacing;
    myData->brushHardness = brushHardness;
    myData->pressureAffectsOpacity = pressureAffectsOpacity;
    myData->pressureAffectsHardness = pressureAffectsHardness;
    myData->pressureAffectsSize = pressureAffectsSize;
    myData->buildUp = buildUp;
    memcpy(myData->shapeColor, shapeColor, sizeof(double) * 3);
    myData->opacity = opacity;
    myData->nbPointsPerSegment = getNbPointPerSegment(myData->brushSizePixel);
}


template <typename GL>
void renderStroke_gl_multiDrawElements(int nbVertices,
                                       int vboVerticesID,
                                       int vboColorsID,
                                       int vboHardnessID,
                                       const OSGLContextPtr& glContext,
                                       const GLShaderBasePtr& strokeShader,
                                       const GLShaderBasePtr& strokeSecondPassShader,
                                       bool doBuildUp,
                                       const OfxRGBAColourF& fillColor,
                                       const RectI& roi,
                                       const ImagePtr& dstImage,
                                       unsigned int primitiveType,
                                       const void* verticesData,
                                       const void* colorsData,
                                       const void* hardnessData,
                                       const int* perDrawCount,
                                       const void** perDrawIdsPtr,
                                       int drawCount)
{

    int target = GL_TEXTURE_2D;
    // Disable scissors since we are going to do texture ping-pong with different frame buffer texture size
    GL::Disable(GL_SCISSOR_TEST);
    GL::Enable(target);
    GL::ActiveTexture(GL_TEXTURE0);
    
    // With OSMesa we must copy the dstImage which already contains the previous drawing to a tmpTexture because the first
    // time we draw to the default framebuffer, mesa will clear out the framebuffer...
    ImagePtr tmpTexture;
    if (!GL::isGPU()) {
        // Since we need to render to texture in any case, upload the content of dstImage to a temporary texture
        tmpTexture = EffectInstance::convertRAMImageRoIToOpenGLTexture(dstImage, roi, glContext);

        GL::BindTexture( target, tmpTexture->getGLTextureID() );
        setupTexParams<GL>(target);

        
    }
    /*
     In build-up mode the only way to properly render is in 2 pass:
     A first shader computes what should be the appropriate output an alpha ramp using OpenGL blending and stores it in the RGB channels
     A second shader multiplies this ramp with the actual fill color.
     In non-build up mode we can do it all at once since we don't use the GL_FUNC_ADD equation so no actual alpha blending occurs, we control
     the premultiplication.
     */
    GLuint fboID = glContext->getOrCreateFBOId();

    if (GL::isGPU()) {
        GL::BindTexture( target, dstImage->getGLTextureID() );
        setupTexParams<GL>(target);
    }
    RectI dstBounds = dstImage->getBounds();




    // In build-up mode we have to copy the dstImage to a temporary texture anyway because we cannot render to a texture
    // and use it as source.
    if (doBuildUp
        && GL::isGPU() // < with OSmesa slow path we already upload the dstImage to the tmpTexture
        ) {


        ImageParamsPtr params(new ImageParams(*dstImage->getParams()));
        params->setBounds(roi);
        {
            CacheEntryStorageInfo& info = params->getStorageInfo();
            info.textureTarget = target;
            info.isGPUTexture = GL::isGPU();
            info.mode = eStorageModeGLTex;
        }
        tmpTexture.reset( new Image(dstImage->getKey(), params) );
        // Copy the content of the existing dstImage

        GL::BindTexture( target, tmpTexture->getGLTextureID() );
        setupTexParams<GL>(target);

        GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
        GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, tmpTexture->getGLTextureID(), 0 /*LoD*/);
        glCheckFramebufferError(GL);
        GL::BindTexture( target, dstImage->getGLTextureID() );

        GLShaderBasePtr shader = glContext->getOrCreateCopyTexShader();
        assert(shader);
        shader->bind();
        shader->setUniform("srcTex", 0);
        Image::applyTextureMapping<GL>(dstBounds, roi, roi);
        shader->unbind();

    } // if (doBuildUp) {

    // Do the actual rendering of the geometry, this is very fast and optimized
    ImagePtr firstPassDstImage;
    if (tmpTexture) {
        firstPassDstImage = tmpTexture;
    } else {
        firstPassDstImage = dstImage;
    }

    GL::BindTexture( target, 0 );

    renderBezier_gl_internal_begin<GL>(doBuildUp);

    if (!GL::isGPU() && firstPassDstImage == dstImage) {
        GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
        GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, firstPassDstImage->getGLTextureID(), 0 /*LoD*/);
        glCheckFramebufferError(GL);
    }
    glCheckError(GL);

    Image::setupGLViewport<GL>(firstPassDstImage->getBounds(), roi);

    strokeShader->bind();
    strokeShader->setUniform("fillColor", fillColor);

    GLint hardnessLoc;
    {
        bool ok = strokeShader->getAttribLocation("inHardness", &hardnessLoc);
        assert(ok);
        Q_UNUSED(ok);
    }
#if 1
    GL::BindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    GL::BufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), verticesData, GL_DYNAMIC_DRAW);
    GL::EnableClientState(GL_VERTEX_ARRAY);
    GL::VertexPointer(2, GL_FLOAT, 0, 0);


    GL::BindBuffer(GL_ARRAY_BUFFER, vboHardnessID);
    GL::BufferData(GL_ARRAY_BUFFER, nbVertices * sizeof(GLfloat), hardnessData, GL_DYNAMIC_DRAW);
    GL::EnableVertexAttribArray(hardnessLoc);
    GL::VertexAttribPointer(hardnessLoc, 1, GL_FLOAT, GL_FALSE ,0, 0);


    GL::BindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    GL::BufferData(GL_ARRAY_BUFFER, nbVertices * 4 * sizeof(GLfloat), colorsData, GL_DYNAMIC_DRAW);
    GL::EnableClientState(GL_COLOR_ARRAY);
    GL::ColorPointer(4, GL_FLOAT, 0, 0);


    GL::MultiDrawElements(primitiveType, perDrawCount, GL_UNSIGNED_INT, perDrawIdsPtr, drawCount);

    GL::DisableClientState(GL_COLOR_ARRAY);
    GL::BindBuffer(GL_ARRAY_BUFFER, 0);
    GL::DisableClientState(GL_VERTEX_ARRAY);
    GL::DisableVertexAttribArray(hardnessLoc);
    GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#else
    // old gl pipeline for debug
    const float* vptr = (const float*)verticesData;
    const float* cptr = (const float*)colorsData;
    const float* hptr = (const float*)hardnessData;
    for (int c = 0; c < drawCount; ++c) {
        GL::Begin(primitiveType);
        const unsigned int* iptr = (const unsigned int*)perDrawIdsPtr[c];
        for (int i = 0; i < perDrawCount[c]; ++i, ++iptr) {
            int vindex = *iptr;
            double hardness = hptr[vindex];
            double r = cptr[vindex * 4 + 0];
            double g = cptr[vindex * 4 + 1];
            double b = cptr[vindex * 4 + 2];
            double a = cptr[vindex * 4 + 3];
            double x = vptr[vindex * 2 + 0];
            double y = vptr[vindex * 2 + 1];
            GL::VertexAttrib1f(hardnessLoc, hardness);
            GL::Color4f(r, g, b, a);
            GL::Vertex2f(x, y);
        }

        GL::End();

    }
#endif
    glCheckError(GL);

    strokeShader->unbind();
    GL::Disable(GL_BLEND);

    // Geometry is rendered, in build-up mode we still have to apply a second shader
    // to correctly set alpha channel
    if (doBuildUp) {

        // If OSMesa slow path is enabled or we are rendering with OpenGL we need to
        // bind the dstImage to the FBO. In CPU mode we need to allocate a copy of tmpTexture
        // because we cannot render to dstImage (which is not a texture) and we cannot use tmpTexture
        // both as input and output.
        RectI outputBounds;
        if (GL::isGPU()) {
            outputBounds = dstBounds;
            GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
            GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, dstImage->getGLTextureID(), 0 /*LoD*/);
            glCheckFramebufferError(GL);
        } else {
            outputBounds = roi;
            // In CPU mode render to the default framebuffer that is already backed by the dstImage
            GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        GL::BindTexture( target, tmpTexture->getGLTextureID() );
        strokeSecondPassShader->bind();
        strokeSecondPassShader->setUniform("tex", 0);
        strokeSecondPassShader->setUniform("fillColor", fillColor);
        Image::applyTextureMapping<GL>(roi, outputBounds, roi);
        strokeSecondPassShader->unbind();
        glCheckError(GL);

    } else if (tmpTexture) {
        // With OSMesa  we copy back the content of the tmpTexture to the dstImage.
        assert(!GL::isGPU());
        GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
        GL::BindTexture( target, tmpTexture->getGLTextureID() );
        Image::applyTextureMapping<GL>(roi, roi, roi);
    }
    GL::BindTexture( target, 0);

} // void renderStroke_gl_multiDrawElements


static void
renderStrokeEnd_gl(RotoShapeRenderNodePrivate::RenderStrokeDataPtr userData)
{
    RenderStrokeGLData* myData = (RenderStrokeGLData*)userData;

    int nbVertices = (myData->primitivesVertices.size() / 2);
    int vboColorsID = myData->glData->getOrCreateVBOColorsID();
    int vboVerticesID = myData->glData->getOrCreateVBOVerticesID();
    int vboHardnessID = myData->glData->getOrCreateVBOHardnessID();
    GLShaderBasePtr strokeShader = myData->glData->getOrCreateStrokeDotShader(myData->buildUp);
    GLShaderBasePtr buildUpPassShader = myData->glData->getOrCreateStrokeSecondPassShader();

    std::vector<const void*> indicesVec(myData->indicesBuf.size());
    std::vector<int> perDrawCount(myData->indicesBuf.size());
    for (std::size_t i = 0; i < myData->indicesBuf.size(); ++i) {
        indicesVec[i] = (const void*)myData->indicesBuf[i]->getData();
        assert((int)myData->indicesBuf[i]->size() == (myData->nbPointsPerSegment + 2));
        perDrawCount[i] = myData->indicesBuf[i]->size();
    }

    OfxRGBAColourF fillColor = {(float)myData->shapeColor[0], (float)myData->shapeColor[1], (float)myData->shapeColor[2], (float)myData->opacity};



    if (myData->glContext->isGPUContext()) {

        renderStroke_gl_multiDrawElements<GL_GPU>(nbVertices, vboVerticesID, vboColorsID, vboHardnessID, myData->glContext, strokeShader, buildUpPassShader, myData->buildUp, fillColor, myData->roi, myData->dstImage, GL_TRIANGLE_FAN, (const void*)(myData->primitivesVertices.getData()), (const void*)(myData->primitivesColors.getData()), (const void*)(myData->primitivesHardness.getData()), (const int*)(&perDrawCount[0]), (const void**)(&indicesVec[0]), indicesVec.size());


    } else {
        renderStroke_gl_multiDrawElements<GL_CPU>(nbVertices, vboVerticesID, vboColorsID, vboHardnessID, myData->glContext, strokeShader, buildUpPassShader, myData->buildUp, fillColor, myData->roi, myData->dstImage, GL_TRIANGLE_FAN, (const void*)(myData->primitivesVertices.getData()), (const void*)(myData->primitivesColors.getData()), (const void*)(myData->primitivesHardness.getData()), (const int*)(&perDrawCount[0]), (const void**)(&indicesVec[0]), indicesVec.size());

    }
}

static bool
renderStrokeRenderDot_gl(RotoShapeRenderNodePrivate::RenderStrokeDataPtr userData,
                         const Point &/*prevCenter*/,
                         const Point &center,
                         double pressure,
                         double *spacing)
{
    RenderStrokeGLData* myData = (RenderStrokeGLData*)userData;

    double brushSizePixel = myData->brushSizePixel;
    if (myData->pressureAffectsSize) {
        brushSizePixel *= pressure;
    }
    double brushHardness = myData->brushHardness;
    if (myData->pressureAffectsHardness) {
        brushHardness *= pressure;
    }
    double opacity = myData->opacity;
    if (myData->pressureAffectsOpacity) {
        opacity *= pressure;
    }
    double brushSpacing = myData->brushSpacing;

    double radius = std::max(brushSizePixel, 1.) / 2.;
    *spacing = radius * 2. * brushSpacing;


    renderDot_gl(*myData, center, radius, myData->shapeColor, opacity,  brushHardness);
    return true;
}

void
RotoShapeRenderGL::renderStroke_gl(const OSGLContextPtr& glContext,
                                   const RotoShapeRenderNodeOpenGLDataPtr& glData,
                                   const RectI& roi,
                                   const ImagePtr& dstImage,
                                   const std::list<std::list<std::pair<Point, double> > >& strokes,
                                   const double distToNextIn,
                                   const Point& lastCenterPointIn,
                                   const RotoDrawableItemPtr& stroke,
                                   bool doBuildup,
                                   double opacity,
                                   double time,
                                   ViewIdx view,
                                   unsigned int mipmapLevel,
                                   double *distToNextOut,
                                   Point* lastCenterPointOut)
{
    RenderStrokeGLData data;
    data.glContext = glContext;
    data.glData = glData;
    data.dstImage = dstImage;
    data.roi = roi;
    RotoShapeRenderNodePrivate::renderStroke_generic((RotoShapeRenderNodePrivate::RenderStrokeDataPtr)&data,
                                                     renderStrokeBegin_gl,
                                                     renderStrokeRenderDot_gl,
                                                     renderStrokeEnd_gl,
                                                     strokes,
                                                     distToNextIn,
                                                     lastCenterPointIn,
                                                     stroke,
                                                     doBuildup,
                                                     opacity,
                                                     time,
                                                     view,
                                                     mipmapLevel,
                                                     distToNextOut,
                                                     lastCenterPointOut);
}


struct RenderSmearGLData
{
    OSGLContextPtr glContext;
    RotoShapeRenderNodeOpenGLDataPtr glData;

    ImagePtr dstImage;

    double brushSizePixel;
    double brushSpacing;
    double brushHardness;
    bool pressureAffectsOpacity;
    bool pressureAffectsHardness;
    bool pressureAffectsSize;
    double opacity;

    int nbPointsPerSegment;

    RamBuffer<float> primitivesColors;
    RamBuffer<float> primitivesVertices;
    RamBuffer<float> primitivesHardness;
    RamBuffer<float> primitivesTexCoords;
    RamBuffer<unsigned int> indices;

    RectI roi;

};

static void
renderSmearBegin_gl(RotoShapeRenderNodePrivate::RenderStrokeDataPtr userData,
                     double brushSizePixel,
                     double brushSpacing,
                     double brushHardness,
                     bool pressureAffectsOpacity,
                     bool pressureAffectsHardness,
                     bool pressureAffectsSize,
                     bool /*buildUp*/,
                     double /*shapeColor*/[3],
                     double opacity)
{
    RenderSmearGLData* myData = (RenderSmearGLData*)userData;

    myData->brushSizePixel = brushSizePixel;
    myData->brushSpacing = brushSpacing;
    myData->brushHardness = brushHardness;
    myData->pressureAffectsOpacity = pressureAffectsOpacity;
    myData->pressureAffectsHardness = pressureAffectsHardness;
    myData->pressureAffectsSize = pressureAffectsSize;
    myData->opacity = opacity;
    myData->nbPointsPerSegment = getNbPointPerSegment(myData->brushSizePixel);
    
}

template <typename GL>
static bool renderSmearDotInternal(RenderSmearGLData* myData,
                                   const Point &prevCenter,
                                   const Point &center,
                                   double pressure,
                                   double* spacing)
{

    double brushSizePixels = myData->brushSizePixel;
    if (myData->pressureAffectsSize) {
        brushSizePixels *= pressure;
    }
    double brushHardness = myData->brushHardness;
    if (myData->pressureAffectsHardness) {
        brushHardness *= pressure;
    }
    double opacity = myData->opacity;
    if (myData->pressureAffectsOpacity) {
        opacity *= pressure;
    }
    double brushSpacing = myData->brushSpacing;

    double radius = std::max(brushSizePixels, 1.) / 2.;
    *spacing = radius * 2. * brushSpacing;

    // Check for initialization cases
    if (prevCenter.x == INT_MIN || prevCenter.y == INT_MIN) {
        return false;
    }
    if (prevCenter.x == center.x && prevCenter.y == center.y) {
        return false;
    }

    // If we were to copy exactly the portion in prevCenter, the smear would leave traces
    // too long. To dampen the effect of the smear, we clamp the spacing
    Point prevPoint = RotoShapeRenderNodePrivate::dampenSmearEffect(prevCenter, center, *spacing);
    const OSGLContextPtr& glContext = myData->glContext;
    const ImagePtr& dstImage = myData->dstImage;
    RectI dstBounds = dstImage->getBounds();

    GLuint fboID = glContext->getOrCreateFBOId();

    int target = GL_TEXTURE_2D;

    // Disable scissors because we are going to use opengl outside of RoI
    GL::Disable(GL_SCISSOR_TEST);

    GL::Enable(target);
    GL::ActiveTexture(GL_TEXTURE0);

    // This is the output texture
    GL::BindTexture( target, dstImage->getGLTextureID() );
    setupTexParams<GL>(target);

    // Specifies the src and dst rectangle
    RectI prevDotBounds(prevPoint.x - brushSizePixels / 2., prevPoint.y - brushSizePixels / 2., prevPoint.x + brushSizePixels / 2. + 1, prevPoint.y + brushSizePixels / 2. + 1);
    RectI nextDotBounds(center.x - brushSizePixels / 2., center.y - brushSizePixels / 2., center.x + brushSizePixels / 2. + 1, center.y + brushSizePixels / 2.+ 1);

    qDebug() << "Prev:" << prevDotBounds.x1<<prevDotBounds.y1<<prevDotBounds.x2<<prevDotBounds.y2;
    qDebug() << "Next:" << nextDotBounds.x1<<nextDotBounds.y1<<nextDotBounds.x2<<nextDotBounds.y2;

    int nbVertices = myData->nbPointsPerSegment + 2;
    double shapeColor[3] = {1., 1., 1.};

    // Get the dot vertices, colors, hardness and tex coords to premultiply the src rect
    bool wasIndicesValid = false;
    {

        // Since the number of vertices does not change for every dot, fill the indices array only once
        if (myData->indices.size() == 0) {
            myData->indices.resize(nbVertices);
            unsigned int* idxData = myData->indices.getData();
            for (std::size_t i = 0; i < myData->indices.size(); ++i) {
                idxData[i] = i;
            }
        } else {
            wasIndicesValid = true;
        }

        Point dotCenter = {(prevDotBounds.x1 + prevDotBounds.x2) / 2., (prevDotBounds.y1 + prevDotBounds.y2) / 2.};
        getDotTriangleFan(dotCenter , radius, myData->nbPointsPerSegment, shapeColor, opacity, brushHardness, true, dstBounds, false, &myData->primitivesVertices, &myData->primitivesColors, &myData->primitivesHardness, &myData->primitivesTexCoords);
    }


    // Copy the original rectangle to a tmp texture and premultiply by an alpha mask with a dot shape
    ImagePtr tmpTexture;
    {


        // In GPU mode, we copy directly from dstImage because it is a texture already.
        // In CPU mode we must upload the portion we are interested in to OSmesa first
        RectI roi;
        prevDotBounds.intersect(dstBounds, &roi);

        ImageParamsPtr params(new ImageParams(*dstImage->getParams()));
        params->setBounds(prevDotBounds);
        {
            CacheEntryStorageInfo& info = params->getStorageInfo();
            info.textureTarget = target;
            info.isGPUTexture = GL::isGPU();
            info.mode = eStorageModeGLTex;
        }
        tmpTexture.reset( new Image(dstImage->getKey(), params) );
        // Copy the content of the existing dstImage

        GL::BindTexture( target, tmpTexture->getGLTextureID() );
        setupTexParams<GL>(target);
        GL::BindFramebuffer(GL_FRAMEBUFFER, fboID);
        GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, tmpTexture->getGLTextureID(), 0 /*LoD*/);
        glCheckFramebufferError(GL);

        Image::setupGLViewport<GL>(prevDotBounds, prevDotBounds);

        // First clear to black the texture because the prevDotBounds might not be contained in the dstBounds
        GL::ClearColor(0., 0., 0., 0.);
        GL::Clear(GL_COLOR_BUFFER_BIT);

        // Now draw onto the intersection with the dstBOunds with the smear shader

        Image::setupGLViewport<GL>(prevDotBounds, roi);


        GL::BindTexture( target, dstImage->getGLTextureID() );


        OfxRGBAColourF fillColor = {(float)shapeColor[0], (float)shapeColor[1], (float)shapeColor[2], (float)opacity};

        GLShaderBasePtr smearShader = myData->glData->getOrCreateSmearShader();
        unsigned int iboID = myData->glData->getOrCreateIBOID();
        unsigned int vboVerticesID = myData->glData->getOrCreateVBOVerticesID();
        unsigned int vboColorsID = myData->glData->getOrCreateVBOColorsID();
        unsigned int vboTexID = myData->glData->getOrCreateVBOTexID();
        unsigned int vboHardnessID = myData->glData->getOrCreateVBOHardnessID();

        smearShader->bind();
        smearShader->setUniform("tex", 0);
        smearShader->setUniform("fillColor", fillColor);
        GLint hardnessLoc;
        {
            bool ok = smearShader->getAttribLocation("inHardness", &hardnessLoc);
            assert(ok);
            Q_UNUSED(ok);
        }
        GL::BindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
        GL::BufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), myData->primitivesVertices.getData(), GL_DYNAMIC_DRAW);
        GL::EnableClientState(GL_VERTEX_ARRAY);
        GL::VertexPointer(2, GL_FLOAT, 0, 0);

        GL::BindBuffer(GL_ARRAY_BUFFER, vboTexID);
        GL::BufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), myData->primitivesTexCoords.getData(), GL_DYNAMIC_DRAW);
        GL::EnableClientState(GL_TEXTURE_COORD_ARRAY);
        GL::TexCoordPointer(2, GL_FLOAT, 0, 0);

        GL::BindBuffer(GL_ARRAY_BUFFER, vboHardnessID);
        GL::BufferData(GL_ARRAY_BUFFER, nbVertices * sizeof(GLfloat), myData->primitivesHardness.getData(), GL_DYNAMIC_DRAW);
        GL::EnableVertexAttribArray(hardnessLoc);
        GL::VertexAttribPointer(hardnessLoc, 1, GL_FLOAT, GL_FALSE ,0 /*stride*/, 0 /*data*/);


        GL::BindBuffer(GL_ARRAY_BUFFER, vboColorsID);
        GL::BufferData(GL_ARRAY_BUFFER, nbVertices * 4 * sizeof(GLfloat), myData->primitivesColors.getData(), GL_DYNAMIC_DRAW);
        GL::EnableClientState(GL_COLOR_ARRAY);
        GL::ColorPointer(4, GL_FLOAT, 0, 0);


        GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
        if (!wasIndicesValid) {
            GL::BufferData(GL_ELEMENT_ARRAY_BUFFER, nbVertices * sizeof(GLuint), myData->indices.getData(), GL_STATIC_DRAW);
        }


        GL::DrawElements(GL_TRIANGLE_FAN, nbVertices, GL_UNSIGNED_INT, 0);

        GL::DisableClientState(GL_COLOR_ARRAY);
        GL::BindBuffer(GL_ARRAY_BUFFER, 0);
        GL::DisableClientState(GL_VERTEX_ARRAY);
        GL::DisableClientState(GL_TEXTURE_COORD_ARRAY);
        GL::DisableVertexAttribArray(hardnessLoc);
        GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glCheckError(GL);
        
        smearShader->unbind();

        if (!GL::isGPU()) {
            GL::Flush();
            GL::Finish();
        }
    }

    // Now copy to the destination rect with blending on
    GL::Enable(GL_BLEND);
    GL::BlendEquation(GL_FUNC_ADD);
    GL::BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);


    GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, dstImage->getGLTextureID(), 0 /*LoD*/);
    glCheckFramebufferError(GL);

    // Use a shader that does not copy the alpha
    GL::BindTexture( target, tmpTexture->getGLTextureID() );
    Image::applyTextureMapping<GL>(nextDotBounds, dstBounds, nextDotBounds);
    GL::BindTexture( target, 0);
    GL::Disable(GL_BLEND);
    glCheckError(GL);
    if (!GL::isGPU()) {
        GL::Flush();
        GL::Finish();

    }


    return true;
} // renderSmearDotInternal

static bool
renderSmearRenderDot_gl(RotoShapeRenderNodePrivate::RenderStrokeDataPtr userData,
                         const Point &prevCenter,
                         const Point &center,
                         double pressure,
                         double *spacing)
{
    RenderSmearGLData* myData = (RenderSmearGLData*)userData;
    /*
     To smear we need to copy the portion of the texture around prevCenter to the portion around center. We cannot use the same
     texture in input and output. The only solution to correctly smear is to first copy the original rectangle to a temporary texture  which
     we premultiply with an alpha mask drawn by a dot and then copy the tmp texture to the final image at the center point with alpha blending
     enabled.
     */
    if (myData->glContext->isGPUContext()) {
        return renderSmearDotInternal<GL_GPU>(myData, prevCenter, center, pressure, spacing);
    } else {
        return renderSmearDotInternal<GL_CPU>(myData, prevCenter, center, pressure, spacing);
    }
}

static void
renderSmearEnd_gl(RotoShapeRenderNodePrivate::RenderStrokeDataPtr /*userData*/)
{

}

bool
RotoShapeRenderGL::renderSmear_gl(const OSGLContextPtr& glContext,
                                  const RotoShapeRenderNodeOpenGLDataPtr& glData,
                                  const RectI& roi,
                                  const ImagePtr& dstImage,
                                  const std::list<std::list<std::pair<Point, double> > >& strokes,
                                  const double distToNextIn,
                                  const Point& lastCenterPointIn,
                                  const RotoStrokeItemPtr& stroke,
                                  double opacity,
                                  double time,
                                  ViewIdx view,
                                  unsigned int mipmapLevel,
                                  double *distToNextOut,
                                  Point* lastCenterPointOut)
{
    RenderSmearGLData data;
    data.glContext = glContext;
    data.glData = glData;
    data.dstImage = dstImage;
    data.roi = roi;
    bool hasRenderedDot = RotoShapeRenderNodePrivate::renderStroke_generic((RotoShapeRenderNodePrivate::RenderStrokeDataPtr)&data,
                                                                           renderSmearBegin_gl,
                                                                           renderSmearRenderDot_gl,
                                                                           renderSmearEnd_gl,
                                                                           strokes,
                                                                           distToNextIn,
                                                                           lastCenterPointIn,
                                                                           stroke,
                                                                           false,
                                                                           opacity,
                                                                           time,
                                                                           view,
                                                                           mipmapLevel,
                                                                           distToNextOut,
                                                                           lastCenterPointOut);

    // Also report the results to the dst image on the default framebuffer
    if (!glContext->isGPUContext()) {
        // Disable scissors because we are going to use opengl outside of RoI
        GL_CPU::Disable(GL_SCISSOR_TEST);
        GL_CPU::BindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_CPU::BindTexture( GL_TEXTURE_2D, dstImage->getGLTextureID());
        setupTexParams<GL_CPU>(GL_TEXTURE_2D);
        RectI bounds = dstImage->getBounds();
        Image::applyTextureMapping<GL_CPU>(bounds, bounds, bounds);
        GL_CPU::BindTexture( GL_TEXTURE_2D, 0);
    }
    return hasRenderedDot;
} // RotoShapeRenderGL::renderSmear_gl

NATRON_NAMESPACE_EXIT;
