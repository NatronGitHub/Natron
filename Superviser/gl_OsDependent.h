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
#else
    #define GL_GLEXT_PROTOTYPES
#endif

#if defined __APPLE__

    
    #include <OpenGL/gl.h>
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

#else

    #include <GL/gl.h>

#endif

#include "Superviser/powiterFn.h"


#ifdef __POWITER_LINUX__
    #include <GL/glu.h>
#endif




#ifndef PW_DEBUG
#define checkGLErrors() ((void)0)
#else
#define checkGLErrors() \
{ \
GLenum error = glGetError(); \
if(error != GL_NO_ERROR) { \
std::cout << "GL_ERROR :" << __FILE__ << " "<< __LINE__ << " " << gluErrorString(error) << std::endl; \
} \
}
#endif



static void checkFrameBufferCompleteness(const char where[],bool silent=true){
	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( error == GL_FRAMEBUFFER_UNDEFINED)
		cout << where << ": Framebuffer undefined" << endl;
	else if(error == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		cout << where << ": Framebuffer incomplete attachment " << endl;
	else if(error == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
		cout << where << ": Framebuffer incomplete missing attachment" << endl;
	else if( error == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
		cout << where << ": Framebuffer incomplete draw buffer" << endl;
	else if( error == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
		cout << where << ": Framebuffer incomplete read buffer" << endl;
	else if( error == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE)
		cout << where << ": Framebuffer incomplete read buffer" << endl;
	else if( error== GL_FRAMEBUFFER_UNSUPPORTED)
		cout << where << ": Framebuffer unsupported" << endl;
	else if( error == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
		cout << where << ": Framebuffer incomplete multisample" << endl;
	else if( error == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT)
		cout << where << ": Framebuffer incomplete layer targets" << endl;
	else if(error == GL_FRAMEBUFFER_COMPLETE ){
		if(!silent)
			cout << where << ": Framebuffer complete" << endl;
	}
	else if ( error == 0)
		cout << where << ": an error occured determining the status of the framebuffer" << endl;
	else
		cout << where << ": UNDEFINED FRAMEBUFFER STATUS" << endl;
	checkGLErrors();
}


#endif
