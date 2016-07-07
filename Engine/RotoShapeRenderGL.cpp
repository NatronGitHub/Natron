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
#include "Engine/RotoBezierTriangulation.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoShapeRenderNode.h"
#include "Engine/RotoShapeRenderNodePrivate.h"
#include "Engine/RotoContext.h"

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
"   gl_FragColor.a = pow(t,fallOff);\n"
"   gl_FragColor.rgb *= gl_FragColor.a;\n"
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

static const char* rotoDrawDot_FragmentShader =
"varying float outHardness;\n"
"uniform vec4 fillColor;\n"
"void main() {\n"
"	gl_FragColor = fillColor;\n"
"   float t = gl_Color.a;\n"
"   if (t == 0.0) {\n"
"       gl_FragColor.a = 0.0;\n"
"   } else {\n"
"       gl_FragColor.a = t;//pow(t, outHardness);\n"
"   }\n"
"   gl_FragColor.rgb *= gl_FragColor.a;\n"
"}"
;

RotoShapeRenderNodeOpenGLData::RotoShapeRenderNodeOpenGLData(bool isGPUContext)
: EffectOpenGLContextData(isGPUContext)
, _vboVerticesID(0)
, _vboColorsID(0)
, _vboHardnessID(0)
, _featherRampShader(5)
, _strokeDotShader()
{

}

void
RotoShapeRenderNodeOpenGLData::cleanup()
{
    _featherRampShader.clear();
    _strokeDotShader.reset();
    bool isGPU = isGPUContext();
    if (_vboVerticesID) {
        if (isGPU) {
            GL_GPU::glDeleteBuffers(1, &_vboVerticesID);
        } else {
            GL_CPU::glDeleteBuffers(1, &_vboVerticesID);
        }
    }

    if (_vboColorsID) {
        if (isGPU) {
            GL_GPU::glDeleteBuffers(1, &_vboColorsID);
        } else {
            GL_CPU::glDeleteBuffers(1, &_vboColorsID);
        }
    }

    if (_vboHardnessID) {
        if (isGPU) {
            GL_GPU::glDeleteBuffers(1, &_vboHardnessID);
        } else {
            GL_CPU::glDeleteBuffers(1, &_vboHardnessID);
        }
    }

    if (_iboID) {
        if (isGPU) {
            GL_GPU::glDeleteBuffers(1, &_iboID);
        } else {
            GL_CPU::glDeleteBuffers(1, &_iboID);
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
            GL_GPU::glGenBuffers(1, &_iboID);
        } else {
            GL_CPU::glGenBuffers(1, &_iboID);
        }
    }
    return _iboID;

}

unsigned int
RotoShapeRenderNodeOpenGLData::getOrCreateVBOVerticesID()
{
    if (!_vboVerticesID) {
        if (isGPUContext()) {
            GL_GPU::glGenBuffers(1, &_vboVerticesID);
        } else {
            GL_CPU::glGenBuffers(1, &_vboVerticesID);
        }
    }
    return _vboVerticesID;

}

unsigned int
RotoShapeRenderNodeOpenGLData::getOrCreateVBOColorsID()
{
    if (!_vboColorsID) {
        if (isGPUContext()) {
            GL_GPU::glGenBuffers(1, &_vboColorsID);
        } else {
            GL_CPU::glGenBuffers(1, &_vboColorsID);
        }
    }
    return _vboColorsID;
}

unsigned int
RotoShapeRenderNodeOpenGLData::getOrCreateVBOHardnessID()
{
    if (!_vboHardnessID) {
        if (isGPUContext()) {
            GL_GPU::glGenBuffers(1, &_vboHardnessID);
        } else {
            GL_CPU::glGenBuffers(1, &_vboHardnessID);
        }
    }
    return _vboHardnessID;

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
getOrCreateStrokeDotShaderInternal()
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
RotoShapeRenderNodeOpenGLData::getOrCreateStrokeDotShader()
{
    if (_strokeDotShader) {
        return _strokeDotShader;
    }
    if (isGPUContext()) {
        _strokeDotShader = getOrCreateStrokeDotShaderInternal<GL_GPU>();
    } else {
        _strokeDotShader = getOrCreateStrokeDotShaderInternal<GL_CPU>();
    }

    return _strokeDotShader;
}


template <typename GL>
void renderBezier_gl_bindFrameBuffer(const OSGLContextPtr& glContext, int target, int texID)
{
    GLuint fboID = glContext->getOrCreateFBOId();


    GL::glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    GL::glEnable(target);
    GL::glActiveTexture(GL_TEXTURE0);
    GL::glBindTexture( target, texID );

    GL::glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL::glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GL::glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL::glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GL::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, texID, 0 /*LoD*/);
    glCheckFramebufferError(GL);

}

template <typename GL>
void renderBezier_gl_internal_begin(const RectI& bounds, bool doClear, bool doBuildUp)
{


    Image::setupGLViewport<GL>(bounds, bounds);

    if (doClear) {
        GL::glClearColor(0, 0, 0, 0);
        GL::glClear(GL_COLOR_BUFFER_BIT);
    }
    glCheckError(GL);

    GL::glEnable(GL_BLEND);
    if (!doBuildUp) {
        GL::glBlendEquation(GL_MAX);
    } else {
        GL::glBlendEquation(GL_FUNC_ADD);
    }
    GL::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


}


template <typename GL>
void renderBezier_gl_singleDrawElements(int nbVertices, int nbIds, int vboVerticesID, int vboColorsID, int iboID, unsigned int primitiveType, const void* verticesData, const void* colorsData, const void* idsData, bool uploadVertices = true)
{

    GL::glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    if (uploadVertices) {
        GL::glBufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), verticesData, GL_DYNAMIC_DRAW);
    }
    GL::glEnableClientState(GL_VERTEX_ARRAY);
    GL::glVertexPointer(2, GL_FLOAT, 0, 0);

    GL::glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    if (uploadVertices) {
        GL::glBufferData(GL_ARRAY_BUFFER, nbVertices * 4 * sizeof(GLfloat), colorsData, GL_DYNAMIC_DRAW);
    }
    GL::glEnableClientState(GL_COLOR_ARRAY);
    GL::glColorPointer(4, GL_FLOAT, 0, 0);

    GL::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
    GL::glBufferData(GL_ELEMENT_ARRAY_BUFFER, nbIds * sizeof(GLuint), idsData, GL_DYNAMIC_DRAW);


    GL::glDrawElements(primitiveType, nbIds, GL_UNSIGNED_INT, 0);

    GL::glDisableClientState(GL_COLOR_ARRAY);
    GL::glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL::glDisableClientState(GL_VERTEX_ARRAY);

    GL::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError(GL);

}

template <typename GL>
void renderBezier_gl_multiDrawElements(int nbVertices, int vboVerticesID, unsigned int primitiveType, const void* verticesData, const int* perDrawCount, const void** perDrawIdsPtr, int drawCount, bool uploadVertices = true)
{

    GL::glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    if (uploadVertices) {
        GL::glBufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), verticesData, GL_DYNAMIC_DRAW);
    }
    GL::glEnableClientState(GL_VERTEX_ARRAY);
    GL::glVertexPointer(2, GL_FLOAT, 0, 0);

    GL::glMultiDrawElements(primitiveType, perDrawCount, GL_UNSIGNED_INT, perDrawIdsPtr, drawCount);

    GL::glDisableClientState(GL_COLOR_ARRAY);
    GL::glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL::glDisableClientState(GL_VERTEX_ARRAY);

    GL::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError(GL);

}


void
RotoShapeRenderGL::renderBezier_gl(const OSGLContextPtr& glContext,
                                   const boost::shared_ptr<RotoShapeRenderNodeOpenGLData>& glData,
                                   const Bezier* bezier,
                                   double opacity,
                                   double /*time*/,
                                   double startTime,
                                   double endTime,
                                   double mbFrameStep,
                                   unsigned int mipmapLevel,
                                   const RectI& bounds,
                                   int target,
                                   int texID)
{

    int vboVerticesID = glData->getOrCreateVBOVerticesID();
    int vboColorsID = glData->getOrCreateVBOColorsID();
    int iboID = glData->getOrCreateIBOID();

    RamBuffer<float> verticesArray;
    RamBuffer<float> colorsArray;
    RamBuffer<unsigned int> indicesArray;

    double mbOpacity = 1. / ((endTime - startTime + 1) / mbFrameStep);

    double shapeOpacity = opacity * mbOpacity;

    RampTypeEnum type;
    {
        boost::shared_ptr<KnobChoice> typeKnob = bezier->getFallOffRampTypeKnob();
        type = (RampTypeEnum)typeKnob->getValue();
    }
    GLShaderBasePtr rampShader = glData->getOrCreateFeatherRampShader(type);
    GLShaderBasePtr fillShader = glContext->getOrCreateFillShader();

    for (double t = startTime; t <= endTime; t+=mbFrameStep) {
        double fallOff = bezier->getFeatherFallOff(t);
        double featherDist = bezier->getFeatherDistance(t);
        double shapeColor[3];
        bezier->getColor(t, shapeColor);


        ///Adjust the feather distance so it takes the mipmap level into account
        if (mipmapLevel != 0) {
            featherDist /= (1 << mipmapLevel);
        }

        RotoBezierTriangulation::PolygonData data;
        RotoBezierTriangulation::computeTriangles(bezier, t, mipmapLevel, featherDist, &data);

        if (glContext->isGPUContext()) {
            renderBezier_gl_bindFrameBuffer<GL_GPU>(glContext, target, texID);
            renderBezier_gl_internal_begin<GL_GPU>(bounds, true, false);
        } else {
            renderBezier_gl_internal_begin<GL_CPU>(bounds, true, false);
        }


        rampShader->bind();
        OfxRGBAColourF fillColor = {shapeColor[0], shapeColor[1], shapeColor[2], shapeOpacity};
        rampShader->setUniform("fillColor", fillColor);
        rampShader->setUniform("fallOff", (float)fallOff);


        // First upload the feather mesh
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

                c_data[0] = shapeColor[0];
                c_data[1] = shapeColor[1];
                c_data[2] = shapeColor[2];
                if (data.featherMesh[i].isInner) {
                    c_data[3] = shapeOpacity;
                } else {
                    c_data[3] = 0.;
                }
            }

            if (glContext->isGPUContext()) {
                renderBezier_gl_singleDrawElements<GL_GPU>(nbVertices, nbVertices, vboVerticesID, vboColorsID, iboID, GL_TRIANGLES, (const void*)verticesArray.getData(), (const void*)colorsArray.getData(), (const void*)indicesArray.getData());
            } else {
                renderBezier_gl_singleDrawElements<GL_CPU>(nbVertices, nbVertices, vboVerticesID, vboColorsID, iboID, GL_TRIANGLES, (const void*)verticesArray.getData(), (const void*)colorsArray.getData(), (const void*)indicesArray.getData());
            }
        }

        rampShader->unbind();
        fillShader->bind();
        fillShader->setUniform("fillColor", fillColor);

        int nbVertices = data.bezierPolygonJoined.size();
        if (!nbVertices) {
            continue;
        }

        verticesArray.resize(nbVertices * 2);


        // Fill buffer
        float* v_data = verticesArray.getData();
        for (std::size_t i = 0; i < data.bezierPolygonJoined.size(); ++i, v_data += 2) {
            v_data[0] = data.bezierPolygonJoined[i].x;
            v_data[1] = data.bezierPolygonJoined[i].y;
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
                if (glContext->isGPUContext()) {
                    renderBezier_gl_multiDrawElements<GL_GPU>(nbVertices, vboVerticesID, GL_TRIANGLES, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount);
                } else {
                    renderBezier_gl_multiDrawElements<GL_CPU>(nbVertices, vboVerticesID, GL_TRIANGLES, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount);
                }
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
                if (glContext->isGPUContext()) {
                    renderBezier_gl_multiDrawElements<GL_GPU>(nbVertices, vboVerticesID, GL_TRIANGLE_FAN, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount, !hasUploadedVertices);
                } else {
                    renderBezier_gl_multiDrawElements<GL_CPU>(nbVertices, vboVerticesID, GL_TRIANGLE_FAN, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount, !hasUploadedVertices);
                }
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
                if (glContext->isGPUContext()) {
                    renderBezier_gl_multiDrawElements<GL_GPU>(nbVertices, vboVerticesID, GL_TRIANGLE_STRIP, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount, !hasUploadedVertices);
                } else {
                    renderBezier_gl_multiDrawElements<GL_CPU>(nbVertices, vboVerticesID, GL_TRIANGLE_STRIP, (const void*)verticesArray.getData(), (const int*)&perDrawCount[0], (const void**)(&perDrawsIDVec[0]), drawCount, !hasUploadedVertices);
                }
                hasUploadedVertices = true;
            }

        }


        if (glContext->isGPUContext()) {
            GL_GPU::glBindTexture(target, 0);
            GL_GPU::glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GL_GPU::glDisable(GL_BLEND);
            glCheckError(GL_GPU);
        } else {
            GL_CPU::glDisable(GL_BLEND);
            glCheckError(GL_CPU);
        }
        
    } // for (double t = startTime; t <= endTime; t+=mbFrameStep) {
} // RotoShapeRenderGL::renderBezier_gl



struct RenderStrokeGLData
{
    OSGLContextPtr glContext;
    boost::shared_ptr<RotoShapeRenderNodeOpenGLData> glData;

    int target;
    int texID;
    RectI bounds;

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
    
    RamBuffer<float> primitivesColors;
    RamBuffer<float> primitivesVertices;
    RamBuffer<float> primitivesHardness;
    std::vector<boost::shared_ptr<RamBuffer<unsigned int> > > indicesBuf;
};


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
                  RamBuffer<float>* vdata,
                  RamBuffer<float>* cdata,
                  RamBuffer<float>* hdata)
{
    int cSize = cdata->size();
    cdata->resize(cSize + (nbOutsideVertices + 2) * 4);
    int vSize = vdata->size();
    vdata->resize(vSize + (nbOutsideVertices + 2) * 2);
    int hSize = hdata->size();
    hdata->resize(hSize + (nbOutsideVertices + 2) * 1);

    float* cPtr = cdata->getData() + cSize;
    float* vPtr = vdata->getData() + vSize;
    float* hPtr = hdata->getData() + hSize;
    vPtr[0] = center.x;
    vPtr[1] = center.y;
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
        vPtr += 2;

        cPtr[0] = shapeColor[0];
        cPtr[1] = shapeColor[1];
        cPtr[2] = shapeColor[2];
        cPtr[3] = 0.;
        cPtr += 4;

        *hPtr = hardness;
        ++hdata;
    }

    vPtr[0] = center.x + radius;
    vPtr[1] = center.y;
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
    getDotTriangleFan(center, radius, data.nbPointsPerSegment, shapeColor, opacity, hardness, &data.primitivesVertices, &data.primitivesColors, &data.primitivesHardness);
    

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


    {
        /*
         * Approximate the necessary number of line segments to draw the ellipse, using http://antigrain.com/research/adaptive_bezier/
         */
        double radius = myData->brushSizePixel / 2.;
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
        myData->nbPointsPerSegment = (int)std::max(length * 0.1 /*0.25*/, 2.);
    }
    
    if (myData->glContext->isGPUContext()) {
        renderBezier_gl_bindFrameBuffer<GL_GPU>(myData->glContext, myData->target, myData->texID);
        renderBezier_gl_internal_begin<GL_GPU>(myData->bounds, false, buildUp);
    } else {
        renderBezier_gl_internal_begin<GL_CPU>(myData->bounds, false, buildUp);
    }

}


template <typename GL>
void renderStroke_gl_multiDrawElements(int nbVertices, int vboVerticesID, int vboColorsID, int vboHardnessID, const GLShaderBase& shader, unsigned int primitiveType, const void* verticesData, const void* colorsData, const void* hardnessData, const int* perDrawCount, const void** perDrawIdsPtr, int drawCount)
{
    GLint hardnessLoc;
    {
        bool ok = shader.getAttribLocation("inHardness", &hardnessLoc);
        assert(ok);
    }
    GL::glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    GL::glBufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), verticesData, GL_DYNAMIC_DRAW);

    GL::glEnableClientState(GL_VERTEX_ARRAY);
    GL::glVertexPointer(2, GL_FLOAT, 0, 0);


    GL::glBindBuffer(GL_ARRAY_BUFFER, vboHardnessID);
    GL::glBufferData(GL_ARRAY_BUFFER, nbVertices * sizeof(GLfloat), hardnessData, GL_DYNAMIC_DRAW);
    GL::glEnableVertexAttribArray(hardnessLoc);
    GL::glVertexAttribPointer(hardnessLoc, 1, GL_FLOAT, GL_FALSE ,0 /*stride*/, 0 /*data*/);


    GL::glBindBuffer(GL_ARRAY_BUFFER, vboColorsID);
    GL::glBufferData(GL_ARRAY_BUFFER, nbVertices * 4 * sizeof(GLfloat), colorsData, GL_DYNAMIC_DRAW);
    GL::glEnableClientState(GL_COLOR_ARRAY);
    GL::glColorPointer(4, GL_FLOAT, 0, 0);


    GL::glBindBuffer(GL_ARRAY_BUFFER, vboVerticesID);
    GL::glBufferData(GL_ARRAY_BUFFER, nbVertices * 2 * sizeof(GLfloat), verticesData, GL_DYNAMIC_DRAW);
    GL::glEnableClientState(GL_VERTEX_ARRAY);
    GL::glVertexPointer(2, GL_FLOAT, 0, 0);


    GL::glMultiDrawElements(primitiveType, perDrawCount, GL_UNSIGNED_INT, perDrawIdsPtr, drawCount);

    GL::glDisableClientState(GL_COLOR_ARRAY);
    GL::glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL::glDisableClientState(GL_VERTEX_ARRAY);
    GL::glDisableVertexAttribArray(hardnessLoc);
    GL::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glCheckError(GL);
    
}


static void
renderStrokeEnd_gl(RotoShapeRenderNodePrivate::RenderStrokeDataPtr userData)
{
    RenderStrokeGLData* myData = (RenderStrokeGLData*)userData;

    int nbVertices = (myData->primitivesVertices.size() / 2);
    int vboColorsID = myData->glData->getOrCreateVBOColorsID();
    int vboVerticesID = myData->glData->getOrCreateVBOVerticesID();
    int vboHardnessID = myData->glData->getOrCreateVBOHardnessID();
    GLShaderBasePtr strokeShader = myData->glData->getOrCreateStrokeDotShader();

    std::vector<unsigned int*> indicesVec(myData->indicesBuf.size());
    std::vector<int> perDrawCount(myData->indicesBuf.size());
    for (std::size_t i = 0; i < myData->indicesBuf.size(); ++i) {
        indicesVec[i] = myData->indicesBuf[i]->getData();
        perDrawCount[i] = myData->indicesBuf[i]->size();
    }

    strokeShader->bind();
    OfxRGBAColourF fillColor = {myData->shapeColor[0], myData->shapeColor[1], myData->shapeColor[2], myData->opacity};
    strokeShader->setUniform("fillColor", fillColor);


    if (myData->glContext->isGPUContext()) {

        renderStroke_gl_multiDrawElements<GL_GPU>(nbVertices, vboVerticesID, vboColorsID, vboHardnessID, *strokeShader, GL_TRIANGLE_FAN, (const void*)(myData->primitivesVertices.getData()), (const void*)(myData->primitivesColors.getData()), (const void*)(myData->primitivesHardness.getData()), (const int*)(&perDrawCount[0]), (const void**)(&indicesVec[0]), indicesVec.size());
        strokeShader->unbind();

        GL_GPU::glBindTexture(myData->target, 0);
        GL_GPU::glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_GPU::glDisable(GL_BLEND);
        glCheckError(GL_GPU);
    } else {
        renderStroke_gl_multiDrawElements<GL_CPU>(nbVertices, vboVerticesID, vboColorsID, vboHardnessID, *strokeShader, GL_TRIANGLE_FAN, (const void*)(myData->primitivesVertices.getData()), (const void*)(myData->primitivesColors.getData()), (const void*)(myData->primitivesHardness.getData()), (const int*)(&perDrawCount[0]), (const void**)(&indicesVec[0]), indicesVec.size());
        strokeShader->unbind();

        GL_CPU::glDisable(GL_BLEND);
        glCheckError(GL_CPU);
    }
}

static void
renderStrokeRenderDot_gl(RotoShapeRenderNodePrivate::RenderStrokeDataPtr userData,
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


    renderDot_gl(*myData, center, radius, myData->shapeColor, opacity, 1. / brushHardness);
}

double
RotoShapeRenderGL::renderStroke_gl(const OSGLContextPtr& glContext,
                                   const boost::shared_ptr<RotoShapeRenderNodeOpenGLData>& glData,
                                   int target,
                                   int texID,
                                   const RectI& bounds,
                                   const std::list<std::list<std::pair<Point, double> > >& strokes,
                                   double distToNext,
                                   const RotoDrawableItem* stroke,
                                   bool doBuildup,
                                   double opacity,
                                   double time,
                                   unsigned int mipmapLevel)
{
    RenderStrokeGLData data;
    data.glContext = glContext;
    data.glData = glData;
    data.target = target;
    data.texID = texID;
    data.bounds = bounds;
    return RotoShapeRenderNodePrivate::renderStroke_generic((RotoShapeRenderNodePrivate::RenderStrokeDataPtr)&data,
                                                            renderStrokeBegin_gl,
                                                            renderStrokeRenderDot_gl,
                                                            renderStrokeEnd_gl,
                                                            strokes,
                                                            distToNext,
                                                            stroke,
                                                            doBuildup,
                                                            opacity,
                                                            time,
                                                            mipmapLevel);
}

NATRON_NAMESPACE_EXIT;
