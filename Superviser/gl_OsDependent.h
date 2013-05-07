///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_OS_DEPENDENT_H
#define INCLUDED_OS_DEPENDENT_H
#include <iostream>
using std::cout;
using std::endl;
//----------------------------------------------------------------------------
//
//	OpenGL related code and definitions
//	that depend on the operating system.
//
//----------------------------------------------------------------------------


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
//static void checkGlErrors (const char where[])
//{
//	GLenum error = glGetError();
//	if (error != GL_NO_ERROR)
//	{
//		cout << where << ": " << gluErrorString (error) << endl;
//	}
//}


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
