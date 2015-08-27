/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef NATRON_GLOBAL_GLINCLUDES_H_
#define NATRON_GLOBAL_GLINCLUDES_H_

#include <GL/glew.h>
#define QT_NO_OPENGL_ES_2

#include "Global/Macros.h"

#ifdef DEBUG
#include <iostream>

// put a breakpoint in glError to halt the debugger
inline void glError() {}

#define glCheckError()                                                  \
    {                                                                   \
        GLenum _glerror_ = glGetError();                                \
        if (_glerror_ != GL_NO_ERROR) {                                 \
            std::cout << "GL_ERROR :" << __FILE__ << " " << __LINE__ << " " << gluErrorString(_glerror_) << std::endl; \
            glError();                                                  \
        }                                                               \
    }
#ifdef __APPLE__
#define glCheckErrorIgnoreOSXBug()                                      \
    {                                                                   \
        GLenum _glerror_ = glGetError();                                \
        if (_glerror_ != GL_NO_ERROR && _glerror_ != GL_INVALID_FRAMEBUFFER_OPERATION) { \
            std::cout << "GL_ERROR :" << __FILE__ << " " << __LINE__ << " " << gluErrorString(_glerror_) << std::endl; \
            glError();                                                  \
        }                                                               \
    }
#else
#define glCheckErrorIgnoreOSXBug() glCheckError()
#endif
#define glCheckErrorAssert()                                            \
    {                                                                   \
        GLenum _glerror_ = glGetError();                                \
        if (_glerror_ != GL_NO_ERROR) {                                 \
            std::cout << "GL_ERROR :" << __FILE__ << " " << __LINE__ << " " << gluErrorString(_glerror_) << std::endl; abort(); \
            glError();                                                  \
        }                                                               \
    }
#define glCheckFramebufferError()                                       \
    {                                                                   \
        GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);        \
        if (error != GL_FRAMEBUFFER_COMPLETE) {                         \
            std::cout << "GL_FRAMEBUFFER_ERROR :" << __FILE__ << " " << __LINE__ << " "; \
            if (error == GL_FRAMEBUFFER_UNDEFINED) {                    \
                std::cout << "Framebuffer undefined" << std::endl;      \
                glError();                                              \
            } else if (error == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) { \
                std::cout << "Framebuffer incomplete attachment " << std::endl; \
                glError();                                              \
            } else if (error == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) { \
                std::cout << "Framebuffer incomplete missing attachment" << std::endl; \
                glError();                                              \
            } else if (error == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) { \
                std::cout << "Framebuffer incomplete draw buffer" << std::endl; \
                glError();                                              \
            } else if (error == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) { \
                std::cout << "Framebuffer incomplete read buffer" << std::endl; \
                glError();                                              \
            } else if (error == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE) { \
                std::cout << "Framebuffer incomplete read buffer" << std::endl; \
                glError();                                              \
            } else if (error == GL_FRAMEBUFFER_UNSUPPORTED) {           \
                std::cout << "Framebuffer unsupported" << std::endl;    \
                glError();                                              \
            } else if (error == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) { \
                std::cout << "Framebuffer incomplete multisample" << std::endl; \
                glError();                                              \
            } else if (error == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT) { \
                std::cout << "Framebuffer incomplete layer targets" << std::endl; \
                glError();                                              \
            } else if (error == 0) {                                    \
                std::cout << "an error occured determining the status of the framebuffer" <<  std::endl; \
                glError();                                              \
            } else {                                                    \
                std::cout << "undefined framebuffer status (" << error << ")" << std::endl; \
                glError();                                              \
            }                                                           \
            glCheckError();                                             \
        }                                                               \
    }
#define glCheckAttribStack()                                            \
    {                                                                   \
        GLint d = -1;                                                   \
        glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &d);                       \
        if (d >= 16) {                                                  \
            std::cout << "GL_ATTRIB_STACK_DEPTH:" << __FILE__ << " " << __LINE__ << " stack may overflow on a basic OpenGL system (depth is " << d << " >= 16)\n"; \
            glError();                                                  \
        }                                                               \
    }
#define glCheckClientAttribStack()                                      \
    {                                                                   \
        GLint d = -1;                                                   \
        glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH, &d);                \
        if (d >= 16) {                                                  \
            std::cout << "GL_CLIENT_ATTRIB_STACK_DEPTH:" << __FILE__ << " " << __LINE__ << " stack may overflow on a basic OpenGL system (depth is " << d << " >= 16)\n"; \
            glError();                                                  \
        }                                                               \
    }
#define glCheckProjectionStack()                                        \
    {                                                                   \
        GLint d = -1;                                                   \
        glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &d);                   \
        if (d >= 2) {                                                   \
            std::cout << "GL_PROJECTION_STACK_DEPTH:" << __FILE__ << " " << __LINE__ << " stack may overflow on a basic OpenGL system (depth is " << d << " >= 2)\n"; \
            glError();                                                  \
        }                                                               \
    }
#define glCheckModelviewStack()                                         \
    {                                                                   \
        GLint d = -1;                                                   \
        glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &d);                    \
        if (d >= 32) {                                                  \
            std::cout << "GL_PROJECTION_STACK_DEPTH:" << __FILE__ << " " << __LINE__ << " stack may overflow on a basic OpenGL system (depth is " << d << " >= 32)\n"; \
            glError();                                                  \
       }                                                                \
    }
#else // !DEBUG
#define glCheckErrorIgnoreOSXBug() ( (void)0 )
#define glCheckError() ( (void)0 )
#define glCheckErrorAssert() ( (void)0 )
#define glCheckFramebufferError() ( (void)0 )
#define glCheckAttribStack() ( (void)0 )
#define glCheckClientAttribStack() ( (void)0 )
#define glCheckProjectionStack() ( (void)0 )
#define glCheckModelviewStack() ( (void)0 )
#endif // ifdef DEBUG

CLANG_DIAG_OFF(deprecated)

// an RAII helper class to push/pop attribs
class GLProtectAttrib
{
public:
    GLProtectAttrib(GLbitfield mask = GL_ALL_ATTRIB_BITS)
    {
#ifdef DEBUG
        GLint d = -1, m0 = -1, m = -1;
        m0 = 16; // https://www.opengl.org/sdk/docs/man2/xhtml/glPushAttrib.xml
        glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &d);
        glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, &m);
        assert(m >= m0);
        if (d >= m0) {
            overflow(d, m0, m);
        }
#endif
        glPushAttrib(mask);
#ifdef DEBUG
        glCheckError();
#endif
    }

    ~GLProtectAttrib() { glPopAttrib(); }
private:
#ifdef DEBUG
    void overflow(GLint d, GLint m0, GLint m) {
        if (d >= m) {
            std::cout << "GLProtectAttrib: stack overflow (max depth is " << m << ")";
        } else {
            std::cout << "GLProtectAttrib: stack may overflow on a minimal OpenGL system (depth is " << d << ", max depth is " << m << " - but at least " << m0 << ")";
        }
        std::cout << " Add breakpoint in GLProtectAttrib::overflow()." << std::endl;
    }
#endif
};

// an RAII helper class to push/pop matrix
class GLProtectMatrix
{
public:
    GLProtectMatrix(GLenum mode) : _mode(mode)
    {
#ifdef DEBUG
        GLint d = -1, m0 = -1, m = -1;
        if (mode == GL_PROJECTION) {
            m0 = 2; // https://www.opengl.org/sdk/docs/man2/xhtml/glPushMatrix.xml
            glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &d);
            //std::cout << "GLProtectMatrix(GL_PROJECTION): depth is " << d << "\n";
            glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &m);
        } else {
            m0 = 32; // https://www.opengl.org/sdk/docs/man2/xhtml/glPushMatrix.xml
            glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &d);
            glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &m);
        }
        assert(m >= m0);
        if (d >= m0) {
            overflow(d, m0, m);
        }
#endif
        glMatrixMode(_mode);
        glPushMatrix();
    }

    ~GLProtectMatrix() { glMatrixMode(_mode); glPopMatrix(); }
private:
    GLenum _mode;

#ifdef DEBUG
    void overflow(GLint d, GLint m0, GLint m) {
        if (d >= m) {
            std::cout << "GLProtectMatrix(GL_" << (_mode == GL_PROJECTION ? "PROJECTION": "MODELVIEW") << "): stack overflow (max depth is " << m << ")";
        } else {
            std::cout << "GLProtectMatrix(GL_" << (_mode == GL_PROJECTION ? "PROJECTION": "MODELVIEW") << "): stack may overflow on a minimal OpenGL system (depth is " << d << ", max depth is " << m << " - but at least " << m0 << ")";
        }
        std::cout << " Add breakpoint in GLProtectMatrix::overflow()." << std::endl;
    }
#endif
};

CLANG_DIAG_ON(deprecated)

#endif // ifndef NATRON_GLOBAL_GLINCLUDES_H_
