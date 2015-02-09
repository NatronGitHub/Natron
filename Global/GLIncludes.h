//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GLOBAL_GLINCLUDES_H_
#define NATRON_GLOBAL_GLINCLUDES_H_

#include <GL/glew.h>
#define QT_NO_OPENGL_ES_2

#include "Global/Macros.h"

#ifdef DEBUG
#include <iostream>
#define glCheckError()                                                  \
    {                                                                     \
        GLenum _glerror_ = glGetError();                                    \
        if (_glerror_ != GL_NO_ERROR) {                                      \
            std::cout << "GL_ERROR :" << __FILE__ << " " << __LINE__ << " " << gluErrorString(_glerror_) << std::endl; \
        }                                                                   \
    }
#ifdef __APPLE__
#define glCheckErrorIgnoreOSXBug()                                      \
    {                                                                     \
        GLenum _glerror_ = glGetError();                                    \
        if (_glerror_ != GL_NO_ERROR && _glerror_ != GL_INVALID_FRAMEBUFFER_OPERATION) { \
            std::cout << "GL_ERROR :" << __FILE__ << " " << __LINE__ << " " << gluErrorString(_glerror_) << std::endl; \
        }                                                                   \
    }
#else
#define glCheckErrorIgnoreOSXBug() glCheckError()
#endif
#define glCheckErrorAssert()                                            \
    {                                                                     \
        GLenum _glerror_ = glGetError();                                    \
        if (_glerror_ != GL_NO_ERROR) {                                      \
            std::cout << "GL_ERROR :" << __FILE__ << " " << __LINE__ << " " << gluErrorString(_glerror_) << std::endl; abort(); \
        }                                                                   \
    }
#define glCheckFramebufferError()                                       \
    {                                                                     \
        GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);            \
        if (error != GL_FRAMEBUFFER_COMPLETE) {                             \
            std::cout << "GL_FRAMEBUFFER_ERROR :" << __FILE__ << " " << __LINE__ << " "; \
            if (error == GL_FRAMEBUFFER_UNDEFINED) {                            \
                std::cout << "Framebuffer undefined" << std::endl; }              \
            else if (error == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {            \
                std::cout << "Framebuffer incomplete attachment " << std::endl; } \
            else if (error == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {    \
                std::cout << "Framebuffer incomplete missing attachment" << std::endl; } \
            else if (error == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) {          \
                std::cout << "Framebuffer incomplete draw buffer" << std::endl; } \
            else if (error == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) {          \
                std::cout << "Framebuffer incomplete read buffer" << std::endl; } \
            else if (error == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE) {          \
                std::cout << "Framebuffer incomplete read buffer" << std::endl; } \
            else if (error == GL_FRAMEBUFFER_UNSUPPORTED) {                      \
                std::cout << "Framebuffer unsupported" << std::endl; }            \
            else if (error == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) {          \
                std::cout << "Framebuffer incomplete multisample" << std::endl; } \
            else if (error == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT) {    \
                std::cout << "Framebuffer incomplete layer targets" << std::endl; } \
            else if (error == 0) {                                             \
                std::cout << "an error occured determining the status of the framebuffer" <<  std::endl; } \
            else {                                                              \
                std::cout << "undefined framebuffer status (" << error << ")" << std::endl; } \
            glCheckError();                                                  \
        }                                                                  \
    }
#else // !DEBUG
#define glCheckErrorIgnoreOSXBug() ( (void)0 )
#define glCheckError() ( (void)0 )
#define glCheckErrorAssert() ( (void)0 )
#define glCheckFramebufferError() ( (void)0 )
#endif // ifdef DEBUG

CLANG_DIAG_OFF(deprecated)

// an RAII helper class to push/pop attribs
class GLProtectAttrib
{
public:
    GLProtectAttrib(GLbitfield mask = GL_ALL_ATTRIB_BITS)
    {
#ifdef DEBUG
        GLint d = -1, m = -1;
        glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &d);
        glGetIntegerv(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, &m);
        if (d == m) {
            overflow(m);
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
    void overflow(GLint m) {
        std::cout << "GLProtectAttrib: stack overflow (max depth is " << m << "). Add breakpoint in GLProtectAttrib::overflow()." << std::endl;
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
        GLint d = -1, m = -1;
        if (mode == GL_PROJECTION) {
            glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &d);
            glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &m);
        } else {
            glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &d);
            glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &m);
        }
        if (d == m) {
            overflow(m);
        }
#endif
        glMatrixMode(_mode);
        glPushMatrix();
    }

    ~GLProtectMatrix() { glMatrixMode(_mode); glPopMatrix(); }
private:
    GLenum _mode;

#ifdef DEBUG
    void overflow(GLint m) {
        std::cout << "GLProtectMatrix(GL_" << (_mode == GL_PROJECTION ? "PROJECTION": "MODELVIEW") << "): stack overflow (max depth is " << m << ") Add breakpoint in GLProtectMatrix::overflow()." << std::endl;
    }
#endif
};

CLANG_DIAG_ON(deprecated)

#endif // ifndef NATRON_GLOBAL_GLINCLUDES_H_
