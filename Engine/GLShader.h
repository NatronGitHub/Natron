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


#ifndef GLSHADER_H
#define GLSHADER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/EngineFwd.h"
#include "Global/GlobalDefines.h"
#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER;

template <typename GL>
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
    GLShader()
    : _shaderID(0)
    , _vertexID(0)
    , _fragmentID(0)
    , _vertexAttached(false)
    , _fragmentAttached(false)
    , _firstTime(true)
    {

    }

    ~GLShader()
    {
        if (_vertexAttached) {
            GL::glDetachShader(_shaderID, _vertexID);
            GL::glDeleteShader(_vertexID);

        }
        if (_fragmentAttached) {
            GL::glDetachShader(_shaderID, _fragmentID);
            GL::glDeleteShader(_fragmentID);

        }

        if (_shaderID != 0) {
            GL::glDeleteProgram(_shaderID);
        }
    }

    bool addShader(ShaderTypeEnum type, const char* src, std::string* error = 0)
    {
        if (_firstTime) {
            _firstTime = false;
            _shaderID = GL::glCreateProgram();
        }

        GLuint shader = 0;
        if (type == eShaderTypeVertex) {
            _vertexID = GL::glCreateShader(GL_VERTEX_SHADER);

            shader = _vertexID;
        } else if (type == eShaderTypeFragment) {
            _fragmentID = GL::glCreateShader(GL_FRAGMENT_SHADER);

            shader = _fragmentID;
        } else {
            assert(false);
        }

        GL::glShaderSource(shader, 1, (const GLchar**)&src, 0);
        GL::glCompileShader(shader);
        GLint isCompiled;
        GL::glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) {
            if (error) {
                getShaderInfoLog(shader, error);
            }

            return false;
        }

        GL::glAttachShader(_shaderID, shader);
        if (type == eShaderTypeVertex) {
            _vertexAttached = true;
        } else {
            _fragmentAttached = true;
        }
        
        return true;
    }

    void bind()
    {
        GL::glUseProgram(_shaderID);
    }

    bool link(std::string* error = 0)
    {
        GL::glLinkProgram(_shaderID);
        GLint isLinked;
        GL::glGetProgramiv(_shaderID, GL_LINK_STATUS, &isLinked);
        if (isLinked == GL_FALSE) {
            if (error) {
                getShaderInfoLog(_shaderID, error);
            }

            return false;
        }
        
        return true;
    }

    void unbind()
    {
        GL::glUseProgram(0);
    }

    U32 getShaderID() const
    {
        return _shaderID;
    }

    bool setUniform(const char* name, int value)
    {
        GLint location = GL::glGetUniformLocation(_shaderID, (const GLchar*)name);

        if (location != -1) {
            GL::glUniform1iv(location, 1, &value);

            return true;
        }
        
        return false;
    }

    bool setUniform(const char* name, float value)
    {
        GLint location = GL::glGetUniformLocation(_shaderID, (const GLchar*)name);

        if (location != -1) {
            GL::glUniform1fv(location, 1, &value);

            return true;
        }
        
        return false;
    }

    bool setUniform(const char* name, const OfxRGBAColourF& values)
    {
        GLint location = GL::glGetUniformLocation(_shaderID, (const GLchar*)name);

        if (location != -1) {
            GL::glUniform4fv(location, 1, &values.r);

            return true;
        }
        
        return false;
    }

private:

    void getShaderInfoLog(GLuint shader,
                          std::string* error)
    {
        GLint maxLength;
        GL::glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        char* infoLog = (char*)malloc(maxLength);
        GL::glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog);
        error->append(infoLog);
        free(infoLog);
    }

    GLuint _shaderID;
    GLuint _vertexID;
    GLuint _fragmentID;
    bool _vertexAttached, _fragmentAttached;
    bool _firstTime;
};

NATRON_NAMESPACE_EXIT;

#endif // GLSHADER_H
