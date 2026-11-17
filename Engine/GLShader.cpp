/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "GLShader.h"

#include "Global/GLIncludes.h"

NATRON_NAMESPACE_ENTER

struct GLShaderPrivate
{
    GLuint shaderID;
    GLuint vertexID;
    GLuint fragmentID;
    bool vertexAttached, fragmentAttached;
    bool firstTime;

    GLShaderPrivate()
        : shaderID(0)
        , vertexID(0)
        , fragmentID(0)
        , vertexAttached(false)
        , fragmentAttached(false)
        , firstTime(true)
    {
    }

    void getShaderInfoLog(GLuint shader,
                          std::string* error)
    {
        GLint maxLength;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        char* infoLog = (char*)malloc(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog);
        error->append(infoLog);
        free(infoLog);
    }
};

GLShader::GLShader()
    : _imp( new GLShaderPrivate() )
{
}

GLShader::~GLShader()
{
    if (_imp->vertexAttached) {
        glDetachShader(_imp->shaderID, _imp->vertexID);
        glDeleteShader(_imp->vertexID);
    }
    if (_imp->fragmentAttached) {
        glDetachShader(_imp->shaderID, _imp->fragmentID);
        glDeleteShader(_imp->fragmentID);
    }

    if (_imp->shaderID != 0) {
        glDeleteProgram(_imp->shaderID);
    }
}

bool
GLShader::addShader(GLShader::ShaderTypeEnum type,
                    const char* src,
                    std::string* error)
{
    if (_imp->firstTime) {
        _imp->firstTime = false;
        _imp->shaderID = glCreateProgram();
    }

    GLuint shader = 0;
    if (type == eShaderTypeVertex) {
        _imp->vertexID = glCreateShader(GL_VERTEX_SHADER);
        shader = _imp->vertexID;
    } else if (type == eShaderTypeFragment) {
        _imp->fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
        shader = _imp->fragmentID;
    } else {
        assert(false);
    }

    glShaderSource(shader, 1, (const GLchar**)&src, 0);
    glCompileShader(shader);
    GLint isCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        if (error) {
            _imp->getShaderInfoLog(shader, error);
        }

        return false;
    }

    glAttachShader(_imp->shaderID, shader);
    if (type == eShaderTypeVertex) {
        _imp->vertexAttached = true;
    } else {
        _imp->fragmentAttached = true;
    }

    return true;
}

void
GLShader::bind()
{
    glUseProgram(_imp->shaderID);
}

bool
GLShader::link(std::string* error)
{
    glLinkProgram(_imp->shaderID);
    GLint isLinked;
    glGetProgramiv(_imp->shaderID, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        if (error) {
            _imp->getShaderInfoLog(_imp->shaderID, error);
        }

        return false;
    }

    return true;
}

void
GLShader::unbind()
{
    glUseProgram(0);
}

U32
GLShader::getShaderID() const
{
    return _imp->shaderID;
}

bool
GLShader::setUniform(const char* name,
                     int value)
{
    GLint location = glGetUniformLocation(_imp->shaderID, (const GLchar*)name);

    if (location != -1) {
        glUniform1iv(location, 1, &value);

        return true;
    }

    return false;
}

bool
GLShader::setUniform(const char* name,
                     float value)
{
    GLint location = glGetUniformLocation(_imp->shaderID, (const GLchar*)name);

    if (location != -1) {
        glUniform1fv(location, 1, &value);

        return true;
    }

    return false;
}

bool
GLShader::setUniform(const char* name,
                     const OfxRGBAColourF& values)
{
    GLint location = glGetUniformLocation(_imp->shaderID, (const GLchar*)name);

    if (location != -1) {
        glUniform4fv(location, 1, &values.r);

        return true;
    }

    return false;
}

NATRON_NAMESPACE_EXIT
