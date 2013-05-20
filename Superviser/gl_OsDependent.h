//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef INCLUDED_OS_DEPENDENT_H
#define INCLUDED_OS_DEPENDENT_H
#include <iostream>
using std::cout;
using std::endl;


#ifdef _WIN32

#include <GL/glew.h>
#define QT_NO_OPENGL_ES_2

#else
    #define GL_GLEXT_PROTOTYPES
#endif

#if defined __APPLE__

    
 //   #include <OpenGL/gl.h>
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

    //#include <GL/gl.h>

#endif

#include "Superviser/powiterFn.h"


#ifdef __POWITER_LINUX__
    #include <GL/glu.h>
#endif

#endif
