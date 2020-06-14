/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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


#ifndef GLSHADER_H
#define GLSHADER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"
#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER

struct GLShaderPrivate;
class GLShader
{
public:

    enum ShaderTypeEnum
    {
        eShaderTypeVertex,
        eShaderTypeFragment
    };

    /**
     * @brief Creates an empty shader. To actually use the shader, you must add a fragment shader and optionnally a vertex shader.
     * By default the vertex shader is:

       void main()
       {
        gl_TexCoord[0] = gl_MultiTexCoord0;
        gl_Position = ftransform();
       }

     *
     * Once programs have been added successfully, you can link() the programs together.
     * When done, the shader is ready to be used. Call bind() to activate the shader
     * and unbind() to deactivate it.
     * To set uniforms, call the setUniform function.
     **/
    GLShader();

    ~GLShader();

    bool addShader(ShaderTypeEnum type, const char* src, std::string* error = 0);

    void bind();

    bool link(std::string* error = 0);

    void unbind();

    U32 getShaderID() const;

    bool setUniform(const char* name, int value);
    bool setUniform(const char* name, float value);
    bool setUniform(const char* name, const OfxRGBAColourF& values);

private:

    boost::scoped_ptr<GLShaderPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // GLSHADER_H
