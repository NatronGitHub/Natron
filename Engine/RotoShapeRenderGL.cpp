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
#include "Engine/RotoContext.h"

NATRON_NAMESPACE_ENTER;



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
void renderBezier_gl_internal_begin(const RectI& bounds)
{


    Image::setupGLViewport<GL>(bounds, bounds);


    GL::glClearColor(0, 0, 0, 0);
    GL::glClear(GL_COLOR_BUFFER_BIT);
    glCheckError(GL);

    GL::glEnable(GL_BLEND);
    GL::glBlendEquation(GL_MAX);
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
RotoShapeRenderGL::renderBezier_gl(const OSGLContextPtr& glContext, const Bezier* bezier, double opacity, double /*time*/, double startTime, double endTime, double mbFrameStep, unsigned int mipmapLevel, const RectI& bounds, int target, int texID)
{

    int vboVerticesID = glContext->getOrCreateVBOVerticesId();
    int vboColorsID = glContext->getOrCreateVBOColorsId();
    int iboID = glContext->getOrCreateIBOId();

    RamBuffer<float> verticesArray;
    RamBuffer<float> colorsArray;
    RamBuffer<unsigned int> indicesArray;

    double mbOpacity = 1. / ((endTime - startTime + 1) / mbFrameStep);

    double shapeOpacity = opacity * mbOpacity;

    OSGLContext::RampTypeEnum type;
    {
        boost::shared_ptr<KnobChoice> typeKnob = bezier->getFallOffRampTypeKnob();
        type = (OSGLContext::RampTypeEnum)typeKnob->getValue();
    }
    boost::shared_ptr<GLShaderBase> rampShader;
    boost::shared_ptr<GLShaderBase> fillShader;
    if (glContext->isGPUContext()) {
        rampShader = glContext->getOrCreateRampShader<GL_GPU>(type);
        fillShader = glContext->getOrCreateFillShader<GL_GPU>();
    } else {
        rampShader = glContext->getOrCreateRampShader<GL_CPU>(type);
        fillShader = glContext->getOrCreateFillShader<GL_CPU>();
    }



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
            renderBezier_gl_internal_begin<GL_GPU>(bounds);
        } else {
            renderBezier_gl_internal_begin<GL_CPU>(bounds);
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


NATRON_NAMESPACE_EXIT;
