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

#include "Global/GlobalDefines.h"
#include "Global/GLIncludes.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class GLShaderBase
{



public:

    enum ShaderTypeEnum
    {
        eShaderTypeVertex,
        eShaderTypeFragment
    };


    GLShaderBase()
    {

    }

    virtual ~GLShaderBase()
    {

    }

    virtual bool addShader(ShaderTypeEnum type, const char* src, std::string* error = 0) = 0;
    virtual void bind() = 0;
    virtual void unbind() = 0;
    virtual bool link(std::string* error = 0) = 0;
    virtual U32 getShaderID() const = 0;
    virtual bool setUniform(const char* name, int value) = 0;
    virtual bool setUniform(const char* name, float value) = 0;
    virtual bool setUniform(const char* name, const OfxRGBAColourF& values) = 0;
    virtual bool getAttribLocation(const char* name, GLint* loc) const = 0;
};

template <typename GL>
class GLShader : public GLShaderBase
{
public:

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
    GLShader()
    : _shaderID(0)
    , _vertexID(0)
    , _fragmentID(0)
    , _vertexAttached(false)
    , _fragmentAttached(false)
    , _firstTime(true)
    {

    }

    virtual ~GLShader()
    {
        if (_vertexAttached) {
            GL::DetachShader(_shaderID, _vertexID);
            GL::DeleteShader(_vertexID);

        }
        if (_fragmentAttached) {
            GL::DetachShader(_shaderID, _fragmentID);
            GL::DeleteShader(_fragmentID);

        }

        if (_shaderID != 0) {
            GL::DeleteProgram(_shaderID);
        }
    }

    virtual bool addShader(ShaderTypeEnum type, const char* src, std::string* error = 0) OVERRIDE FINAL
    {
        if (_firstTime) {
            _shaderID = GL::CreateProgram();
            if (!_shaderID) {
                return false;
            }
            _firstTime = false;

        }

        GLuint shader = 0;
        if (type == eShaderTypeVertex) {
            _vertexID = GL::CreateShader(GL_VERTEX_SHADER);

            shader = _vertexID;
        } else if (type == eShaderTypeFragment) {
            _fragmentID = GL::CreateShader(GL_FRAGMENT_SHADER);

            shader = _fragmentID;
        } else {
            assert(false);
        }

        GL::ShaderSource(shader, 1, (const GLchar**)&src, 0);
        GL::CompileShader(shader);
        GLint isCompiled;
        GL::GetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) {
            if (error) {
                getShaderInfoLog(shader, error);
            }

            return false;
        }

        GL::AttachShader(_shaderID, shader);
        if (type == eShaderTypeVertex) {
            _vertexAttached = true;
        } else {
            _fragmentAttached = true;
        }
        
        return true;
    }

    virtual void bind() OVERRIDE FINAL
    {
        GL::UseProgram(_shaderID);
    }

    virtual bool link(std::string* error = 0) OVERRIDE FINAL
    {
        GL::LinkProgram(_shaderID);
        GLint isLinked;
        GL::GetProgramiv(_shaderID, GL_LINK_STATUS, &isLinked);
        if (isLinked == GL_FALSE) {
            if (error) {
                getShaderInfoLog(_shaderID, error);
            }

            return false;
        }
        
        return true;
    }

    virtual void unbind() OVERRIDE FINAL
    {
        GL::UseProgram(0);
    }

    virtual U32 getShaderID() const OVERRIDE FINAL
    {
        return _shaderID;
    }

    virtual bool setUniform(const char* name, int value) OVERRIDE FINAL
    {
        GLint location = GL::GetUniformLocation(_shaderID, (const GLchar*)name);

        if (location != -1) {
            GL::Uniform1iv(location, 1, &value);

            return true;
        }
        assert(false);
        return false;
    }

    virtual bool setUniform(const char* name, float value) OVERRIDE FINAL
    {
        GLint location = GL::GetUniformLocation(_shaderID, (const GLchar*)name);

        if (location != -1) {
            GL::Uniform1fv(location, 1, &value);

            return true;
        }
        assert(false);
        return false;
    }

    virtual bool setUniform(const char* name, const OfxRGBAColourF& values) OVERRIDE FINAL
    {
        GLint location = GL::GetUniformLocation(_shaderID, (const GLchar*)name);

        if (location != -1) {
            GL::Uniform4fv(location, 1, &values.r);

            return true;
        }
        assert(false);
        return false;
    }

    virtual bool getAttribLocation(const char* name, GLint* loc) const OVERRIDE FINAL
    {
        *loc = GL::GetAttribLocation(_shaderID, (const GLchar*)name);
        return *loc != -1;
    }

private:

    void getShaderInfoLog(GLuint shader,
                          std::string* error)
    {
        GLint maxLength;
        GL::GetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        char* infoLog = (char*)malloc(maxLength);
        GL::GetShaderInfoLog(shader, maxLength, &maxLength, infoLog);
        error->append(infoLog);
        free(infoLog);
    }

    GLuint _shaderID;
    GLuint _vertexID;
    GLuint _fragmentID;
    bool _vertexAttached, _fragmentAttached;
    bool _firstTime;
};

NATRON_NAMESPACE_EXIT

#endif // GLSHADER_H
