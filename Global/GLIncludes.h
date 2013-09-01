//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GLOBAL_GLINCLUDES_H_
#define POWITER_GLOBAL_GLINCLUDES_H_

#ifdef _WIN32

#include <GL/glew.h>
#define QT_NO_OPENGL_ES_2

#else
    #define GL_GLEXT_PROTOTYPES
#endif

#if defined __APPLE__

    
    #include <OpenGL/glu.h>
    #ifndef GL_HALF_FLOAT_ARB
	#define USE_APPLE_FLOAT_PIXELS
    #endif

    #ifndef GL_LUMINANCE32F_ARB
	#define GL_LUMINANCE32F_ARB GL_LUMINANCE_FLOAT32_APPLE
    #endif

    #ifndef GL_RGBA32F_ARB
	#define GL_RGBA32F_ARB GL_RGBA_FLOAT32_APPLE
    #endif
    
    #ifndef GL_HALF_FLOAT_ARB
	#define GL_HALF_FLOAT_ARB GL_HALF_APPLE
    #endif


#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	#include <GL/glu.h>
#endif



#endif
