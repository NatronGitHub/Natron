/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Natron_Global_GLObfuscate_h
#define Natron_Global_GLObfuscate_h

#define glObfuscate ERROR_use_GL_CPU_or_GL_GPU::FuncName_instead_of_glFuncName

// remove global macro definitions of OpenGL functions by glad.h
// fgrep "#define gl" glad.h |sed -e 's/#define/#undef/g' |awk '{print $1, $2}'
#undef glCullFace
#undef glFrontFace
#undef glHint
#undef glLineWidth
#undef glPointSize
#undef glPolygonMode
#undef glScissor
#undef glTexParameterf
#undef glTexParameterfv
#undef glTexParameteri
#undef glTexParameteriv
#undef glTexImage1D
#undef glTexImage2D
#undef glDrawBuffer
#undef glClear
#undef glClearColor
#undef glClearStencil
#undef glClearDepth
#undef glStencilMask
#undef glColorMask
#undef glDepthMask
#undef glDisable
#undef glEnable
#undef glFinish
#undef glFlush
#undef glBlendFunc
#undef glLogicOp
#undef glStencilFunc
#undef glStencilOp
#undef glDepthFunc
#undef glPixelStoref
#undef glPixelStorei
#undef glReadBuffer
#undef glReadPixels
#undef glGetBooleanv
#undef glGetDoublev
#undef glGetError
#undef glGetFloatv
#undef glGetIntegerv
#undef glGetString
#undef glGetTexImage
#undef glGetTexParameterfv
#undef glGetTexParameteriv
#undef glGetTexLevelParameterfv
#undef glGetTexLevelParameteriv
#undef glIsEnabled
#undef glDepthRange
#undef glViewport
#undef glNewList
#undef glEndList
#undef glCallList
#undef glCallLists
#undef glDeleteLists
#undef glGenLists
#undef glListBase
#undef glBegin
#undef glBitmap
#undef glColor3b
#undef glColor3bv
#undef glColor3d
#undef glColor3dv
#undef glColor3f
#undef glColor3fv
#undef glColor3i
#undef glColor3iv
#undef glColor3s
#undef glColor3sv
#undef glColor3ub
#undef glColor3ubv
#undef glColor3ui
#undef glColor3uiv
#undef glColor3us
#undef glColor3usv
#undef glColor4b
#undef glColor4bv
#undef glColor4d
#undef glColor4dv
#undef glColor4f
#undef glColor4fv
#undef glColor4i
#undef glColor4iv
#undef glColor4s
#undef glColor4sv
#undef glColor4ub
#undef glColor4ubv
#undef glColor4ui
#undef glColor4uiv
#undef glColor4us
#undef glColor4usv
#undef glEdgeFlag
#undef glEdgeFlagv
#undef glEnd
#undef glIndexd
#undef glIndexdv
#undef glIndexf
#undef glIndexfv
#undef glIndexi
#undef glIndexiv
#undef glIndexs
#undef glIndexsv
#undef glNormal3b
#undef glNormal3bv
#undef glNormal3d
#undef glNormal3dv
#undef glNormal3f
#undef glNormal3fv
#undef glNormal3i
#undef glNormal3iv
#undef glNormal3s
#undef glNormal3sv
#undef glRasterPos2d
#undef glRasterPos2dv
#undef glRasterPos2f
#undef glRasterPos2fv
#undef glRasterPos2i
#undef glRasterPos2iv
#undef glRasterPos2s
#undef glRasterPos2sv
#undef glRasterPos3d
#undef glRasterPos3dv
#undef glRasterPos3f
#undef glRasterPos3fv
#undef glRasterPos3i
#undef glRasterPos3iv
#undef glRasterPos3s
#undef glRasterPos3sv
#undef glRasterPos4d
#undef glRasterPos4dv
#undef glRasterPos4f
#undef glRasterPos4fv
#undef glRasterPos4i
#undef glRasterPos4iv
#undef glRasterPos4s
#undef glRasterPos4sv
#undef glRectd
#undef glRectdv
#undef glRectf
#undef glRectfv
#undef glRecti
#undef glRectiv
#undef glRects
#undef glRectsv
#undef glTexCoord1d
#undef glTexCoord1dv
#undef glTexCoord1f
#undef glTexCoord1fv
#undef glTexCoord1i
#undef glTexCoord1iv
#undef glTexCoord1s
#undef glTexCoord1sv
#undef glTexCoord2d
#undef glTexCoord2dv
#undef glTexCoord2f
#undef glTexCoord2fv
#undef glTexCoord2i
#undef glTexCoord2iv
#undef glTexCoord2s
#undef glTexCoord2sv
#undef glTexCoord3d
#undef glTexCoord3dv
#undef glTexCoord3f
#undef glTexCoord3fv
#undef glTexCoord3i
#undef glTexCoord3iv
#undef glTexCoord3s
#undef glTexCoord3sv
#undef glTexCoord4d
#undef glTexCoord4dv
#undef glTexCoord4f
#undef glTexCoord4fv
#undef glTexCoord4i
#undef glTexCoord4iv
#undef glTexCoord4s
#undef glTexCoord4sv
#undef glVertex2d
#undef glVertex2dv
#undef glVertex2f
#undef glVertex2fv
#undef glVertex2i
#undef glVertex2iv
#undef glVertex2s
#undef glVertex2sv
#undef glVertex3d
#undef glVertex3dv
#undef glVertex3f
#undef glVertex3fv
#undef glVertex3i
#undef glVertex3iv
#undef glVertex3s
#undef glVertex3sv
#undef glVertex4d
#undef glVertex4dv
#undef glVertex4f
#undef glVertex4fv
#undef glVertex4i
#undef glVertex4iv
#undef glVertex4s
#undef glVertex4sv
#undef glClipPlane
#undef glColorMaterial
#undef glFogf
#undef glFogfv
#undef glFogi
#undef glFogiv
#undef glLightf
#undef glLightfv
#undef glLighti
#undef glLightiv
#undef glLightModelf
#undef glLightModelfv
#undef glLightModeli
#undef glLightModeliv
#undef glLineStipple
#undef glMaterialf
#undef glMaterialfv
#undef glMateriali
#undef glMaterialiv
#undef glPolygonStipple
#undef glShadeModel
#undef glTexEnvf
#undef glTexEnvfv
#undef glTexEnvi
#undef glTexEnviv
#undef glTexGend
#undef glTexGendv
#undef glTexGenf
#undef glTexGenfv
#undef glTexGeni
#undef glTexGeniv
#undef glFeedbackBuffer
#undef glSelectBuffer
#undef glRenderMode
#undef glInitNames
#undef glLoadName
#undef glPassThrough
#undef glPopName
#undef glPushName
#undef glClearAccum
#undef glClearIndex
#undef glIndexMask
#undef glAccum
#undef glPopAttrib
#undef glPushAttrib
#undef glMap1d
#undef glMap1f
#undef glMap2d
#undef glMap2f
#undef glMapGrid1d
#undef glMapGrid1f
#undef glMapGrid2d
#undef glMapGrid2f
#undef glEvalCoord1d
#undef glEvalCoord1dv
#undef glEvalCoord1f
#undef glEvalCoord1fv
#undef glEvalCoord2d
#undef glEvalCoord2dv
#undef glEvalCoord2f
#undef glEvalCoord2fv
#undef glEvalMesh1
#undef glEvalPoint1
#undef glEvalMesh2
#undef glEvalPoint2
#undef glAlphaFunc
#undef glPixelZoom
#undef glPixelTransferf
#undef glPixelTransferi
#undef glPixelMapfv
#undef glPixelMapuiv
#undef glPixelMapusv
#undef glCopyPixels
#undef glDrawPixels
#undef glGetClipPlane
#undef glGetLightfv
#undef glGetLightiv
#undef glGetMapdv
#undef glGetMapfv
#undef glGetMapiv
#undef glGetMaterialfv
#undef glGetMaterialiv
#undef glGetPixelMapfv
#undef glGetPixelMapuiv
#undef glGetPixelMapusv
#undef glGetPolygonStipple
#undef glGetTexEnvfv
#undef glGetTexEnviv
#undef glGetTexGendv
#undef glGetTexGenfv
#undef glGetTexGeniv
#undef glIsList
#undef glFrustum
#undef glLoadIdentity
#undef glLoadMatrixf
#undef glLoadMatrixd
#undef glMatrixMode
#undef glMultMatrixf
#undef glMultMatrixd
#undef glOrtho
#undef glPopMatrix
#undef glPushMatrix
#undef glRotated
#undef glRotatef
#undef glScaled
#undef glScalef
#undef glTranslated
#undef glTranslatef
#undef glDrawArrays
#undef glDrawElements
#undef glGetPointerv
#undef glPolygonOffset
#undef glCopyTexImage1D
#undef glCopyTexImage2D
#undef glCopyTexSubImage1D
#undef glCopyTexSubImage2D
#undef glTexSubImage1D
#undef glTexSubImage2D
#undef glBindTexture
#undef glDeleteTextures
#undef glGenTextures
#undef glIsTexture
#undef glArrayElement
#undef glColorPointer
#undef glDisableClientState
#undef glEdgeFlagPointer
#undef glEnableClientState
#undef glIndexPointer
#undef glInterleavedArrays
#undef glNormalPointer
#undef glTexCoordPointer
#undef glVertexPointer
#undef glAreTexturesResident
#undef glPrioritizeTextures
#undef glIndexub
#undef glIndexubv
#undef glPopClientAttrib
#undef glPushClientAttrib
#undef glDrawRangeElements
#undef glTexImage3D
#undef glTexSubImage3D
#undef glCopyTexSubImage3D
#undef glActiveTexture
#undef glSampleCoverage
#undef glCompressedTexImage3D
#undef glCompressedTexImage2D
#undef glCompressedTexImage1D
#undef glCompressedTexSubImage3D
#undef glCompressedTexSubImage2D
#undef glCompressedTexSubImage1D
#undef glGetCompressedTexImage
#undef glClientActiveTexture
#undef glMultiTexCoord1d
#undef glMultiTexCoord1dv
#undef glMultiTexCoord1f
#undef glMultiTexCoord1fv
#undef glMultiTexCoord1i
#undef glMultiTexCoord1iv
#undef glMultiTexCoord1s
#undef glMultiTexCoord1sv
#undef glMultiTexCoord2d
#undef glMultiTexCoord2dv
#undef glMultiTexCoord2f
#undef glMultiTexCoord2fv
#undef glMultiTexCoord2i
#undef glMultiTexCoord2iv
#undef glMultiTexCoord2s
#undef glMultiTexCoord2sv
#undef glMultiTexCoord3d
#undef glMultiTexCoord3dv
#undef glMultiTexCoord3f
#undef glMultiTexCoord3fv
#undef glMultiTexCoord3i
#undef glMultiTexCoord3iv
#undef glMultiTexCoord3s
#undef glMultiTexCoord3sv
#undef glMultiTexCoord4d
#undef glMultiTexCoord4dv
#undef glMultiTexCoord4f
#undef glMultiTexCoord4fv
#undef glMultiTexCoord4i
#undef glMultiTexCoord4iv
#undef glMultiTexCoord4s
#undef glMultiTexCoord4sv
#undef glLoadTransposeMatrixf
#undef glLoadTransposeMatrixd
#undef glMultTransposeMatrixf
#undef glMultTransposeMatrixd
#undef glBlendFuncSeparate
#undef glMultiDrawArrays
#undef glMultiDrawElements
#undef glPointParameterf
#undef glPointParameterfv
#undef glPointParameteri
#undef glPointParameteriv
#undef glFogCoordf
#undef glFogCoordfv
#undef glFogCoordd
#undef glFogCoorddv
#undef glFogCoordPointer
#undef glSecondaryColor3b
#undef glSecondaryColor3bv
#undef glSecondaryColor3d
#undef glSecondaryColor3dv
#undef glSecondaryColor3f
#undef glSecondaryColor3fv
#undef glSecondaryColor3i
#undef glSecondaryColor3iv
#undef glSecondaryColor3s
#undef glSecondaryColor3sv
#undef glSecondaryColor3ub
#undef glSecondaryColor3ubv
#undef glSecondaryColor3ui
#undef glSecondaryColor3uiv
#undef glSecondaryColor3us
#undef glSecondaryColor3usv
#undef glSecondaryColorPointer
#undef glWindowPos2d
#undef glWindowPos2dv
#undef glWindowPos2f
#undef glWindowPos2fv
#undef glWindowPos2i
#undef glWindowPos2iv
#undef glWindowPos2s
#undef glWindowPos2sv
#undef glWindowPos3d
#undef glWindowPos3dv
#undef glWindowPos3f
#undef glWindowPos3fv
#undef glWindowPos3i
#undef glWindowPos3iv
#undef glWindowPos3s
#undef glWindowPos3sv
#undef glBlendColor
#undef glBlendEquation
#undef glGenQueries
#undef glDeleteQueries
#undef glIsQuery
#undef glBeginQuery
#undef glEndQuery
#undef glGetQueryiv
#undef glGetQueryObjectiv
#undef glGetQueryObjectuiv
#undef glBindBuffer
#undef glDeleteBuffers
#undef glGenBuffers
#undef glIsBuffer
#undef glBufferData
#undef glBufferSubData
#undef glGetBufferSubData
#undef glMapBuffer
#undef glUnmapBuffer
#undef glGetBufferParameteriv
#undef glGetBufferPointerv
#undef glBlendEquationSeparate
#undef glDrawBuffers
#undef glStencilOpSeparate
#undef glStencilFuncSeparate
#undef glStencilMaskSeparate
#undef glAttachShader
#undef glBindAttribLocation
#undef glCompileShader
#undef glCreateProgram
#undef glCreateShader
#undef glDeleteProgram
#undef glDeleteShader
#undef glDetachShader
#undef glDisableVertexAttribArray
#undef glEnableVertexAttribArray
#undef glGetActiveAttrib
#undef glGetActiveUniform
#undef glGetAttachedShaders
#undef glGetAttribLocation
#undef glGetProgramiv
#undef glGetProgramInfoLog
#undef glGetShaderiv
#undef glGetShaderInfoLog
#undef glGetShaderSource
#undef glGetUniformLocation
#undef glGetUniformfv
#undef glGetUniformiv
#undef glGetVertexAttribdv
#undef glGetVertexAttribfv
#undef glGetVertexAttribiv
#undef glGetVertexAttribPointerv
#undef glIsProgram
#undef glIsShader
#undef glLinkProgram
#undef glShaderSource
#undef glUseProgram
#undef glUniform1f
#undef glUniform2f
#undef glUniform3f
#undef glUniform4f
#undef glUniform1i
#undef glUniform2i
#undef glUniform3i
#undef glUniform4i
#undef glUniform1fv
#undef glUniform2fv
#undef glUniform3fv
#undef glUniform4fv
#undef glUniform1iv
#undef glUniform2iv
#undef glUniform3iv
#undef glUniform4iv
#undef glUniformMatrix2fv
#undef glUniformMatrix3fv
#undef glUniformMatrix4fv
#undef glValidateProgram
#undef glVertexAttrib1d
#undef glVertexAttrib1dv
#undef glVertexAttrib1f
#undef glVertexAttrib1fv
#undef glVertexAttrib1s
#undef glVertexAttrib1sv
#undef glVertexAttrib2d
#undef glVertexAttrib2dv
#undef glVertexAttrib2f
#undef glVertexAttrib2fv
#undef glVertexAttrib2s
#undef glVertexAttrib2sv
#undef glVertexAttrib3d
#undef glVertexAttrib3dv
#undef glVertexAttrib3f
#undef glVertexAttrib3fv
#undef glVertexAttrib3s
#undef glVertexAttrib3sv
#undef glVertexAttrib4Nbv
#undef glVertexAttrib4Niv
#undef glVertexAttrib4Nsv
#undef glVertexAttrib4Nub
#undef glVertexAttrib4Nubv
#undef glVertexAttrib4Nuiv
#undef glVertexAttrib4Nusv
#undef glVertexAttrib4bv
#undef glVertexAttrib4d
#undef glVertexAttrib4dv
#undef glVertexAttrib4f
#undef glVertexAttrib4fv
#undef glVertexAttrib4iv
#undef glVertexAttrib4s
#undef glVertexAttrib4sv
#undef glVertexAttrib4ubv
#undef glVertexAttrib4uiv
#undef glVertexAttrib4usv
#undef glVertexAttribPointer
#undef glBindBufferARB
#undef glDeleteBuffersARB
#undef glGenBuffersARB
#undef glIsBufferARB
#undef glBufferDataARB
#undef glBufferSubDataARB
#undef glGetBufferSubDataARB
#undef glMapBufferARB
#undef glUnmapBufferARB
#undef glGetBufferParameterivARB
#undef glGetBufferPointervARB
#undef glBindVertexArray
#undef glDeleteVertexArrays
#undef glGenVertexArrays
#undef glIsVertexArray
#undef glIsRenderbuffer
#undef glBindRenderbuffer
#undef glDeleteRenderbuffers
#undef glGenRenderbuffers
#undef glRenderbufferStorage
#undef glGetRenderbufferParameteriv
#undef glIsFramebuffer
#undef glBindFramebuffer
#undef glDeleteFramebuffers
#undef glGenFramebuffers
#undef glCheckFramebufferStatus
#undef glFramebufferTexture1D
#undef glFramebufferTexture2D
#undef glFramebufferTexture3D
#undef glFramebufferRenderbuffer
#undef glGetFramebufferAttachmentParameteriv
#undef glGenerateMipmap
#undef glBlitFramebuffer
#undef glRenderbufferStorageMultisample
#undef glFramebufferTextureLayer
#undef glIsRenderbufferEXT
#undef glBindRenderbufferEXT
#undef glDeleteRenderbuffersEXT
#undef glGenRenderbuffersEXT
#undef glRenderbufferStorageEXT
#undef glGetRenderbufferParameterivEXT
#undef glIsFramebufferEXT
#undef glBindFramebufferEXT
#undef glDeleteFramebuffersEXT
#undef glGenFramebuffersEXT
#undef glCheckFramebufferStatusEXT
#undef glFramebufferTexture1DEXT
#undef glFramebufferTexture2DEXT
#undef glFramebufferTexture3DEXT
#undef glFramebufferRenderbufferEXT
#undef glGetFramebufferAttachmentParameterivEXT
#undef glGenerateMipmapEXT
#undef glBindVertexArrayAPPLE
#undef glDeleteVertexArraysAPPLE
#undef glGenVertexArraysAPPLE
#undef glIsVertexArrayAPPLE

// redefine OpenGL functions by glad.h
// fgrep "#define gl" glad.h | awk '{print $1, $2, "glObfuscate"}'
#define glCullFace glObfuscate
#define glFrontFace glObfuscate
#define glHint glObfuscate
#define glLineWidth glObfuscate
#define glPointSize glObfuscate
#define glPolygonMode glObfuscate
#define glScissor glObfuscate
#define glTexParameterf glObfuscate
#define glTexParameterfv glObfuscate
#define glTexParameteri glObfuscate
#define glTexParameteriv glObfuscate
#define glTexImage1D glObfuscate
#define glTexImage2D glObfuscate
#define glDrawBuffer glObfuscate
#define glClear glObfuscate
#define glClearColor glObfuscate
#define glClearStencil glObfuscate
#define glClearDepth glObfuscate
#define glStencilMask glObfuscate
#define glColorMask glObfuscate
#define glDepthMask glObfuscate
#define glDisable glObfuscate
#define glEnable glObfuscate
#define glFinish glObfuscate
#define glFlush glObfuscate
#define glBlendFunc glObfuscate
#define glLogicOp glObfuscate
#define glStencilFunc glObfuscate
#define glStencilOp glObfuscate
#define glDepthFunc glObfuscate
#define glPixelStoref glObfuscate
#define glPixelStorei glObfuscate
#define glReadBuffer glObfuscate
#define glReadPixels glObfuscate
#define glGetBooleanv glObfuscate
#define glGetDoublev glObfuscate
#define glGetError glObfuscate
#define glGetFloatv glObfuscate
#define glGetIntegerv glObfuscate
#define glGetString glObfuscate
#define glGetTexImage glObfuscate
#define glGetTexParameterfv glObfuscate
#define glGetTexParameteriv glObfuscate
#define glGetTexLevelParameterfv glObfuscate
#define glGetTexLevelParameteriv glObfuscate
#define glIsEnabled glObfuscate
#define glDepthRange glObfuscate
#define glViewport glObfuscate
#define glNewList glObfuscate
#define glEndList glObfuscate
#define glCallList glObfuscate
#define glCallLists glObfuscate
#define glDeleteLists glObfuscate
#define glGenLists glObfuscate
#define glListBase glObfuscate
#define glBegin glObfuscate
#define glBitmap glObfuscate
#define glColor3b glObfuscate
#define glColor3bv glObfuscate
#define glColor3d glObfuscate
#define glColor3dv glObfuscate
#define glColor3f glObfuscate
#define glColor3fv glObfuscate
#define glColor3i glObfuscate
#define glColor3iv glObfuscate
#define glColor3s glObfuscate
#define glColor3sv glObfuscate
#define glColor3ub glObfuscate
#define glColor3ubv glObfuscate
#define glColor3ui glObfuscate
#define glColor3uiv glObfuscate
#define glColor3us glObfuscate
#define glColor3usv glObfuscate
#define glColor4b glObfuscate
#define glColor4bv glObfuscate
#define glColor4d glObfuscate
#define glColor4dv glObfuscate
#define glColor4f glObfuscate
#define glColor4fv glObfuscate
#define glColor4i glObfuscate
#define glColor4iv glObfuscate
#define glColor4s glObfuscate
#define glColor4sv glObfuscate
#define glColor4ub glObfuscate
#define glColor4ubv glObfuscate
#define glColor4ui glObfuscate
#define glColor4uiv glObfuscate
#define glColor4us glObfuscate
#define glColor4usv glObfuscate
#define glEdgeFlag glObfuscate
#define glEdgeFlagv glObfuscate
#define glEnd glObfuscate
#define glIndexd glObfuscate
#define glIndexdv glObfuscate
#define glIndexf glObfuscate
#define glIndexfv glObfuscate
#define glIndexi glObfuscate
#define glIndexiv glObfuscate
#define glIndexs glObfuscate
#define glIndexsv glObfuscate
#define glNormal3b glObfuscate
#define glNormal3bv glObfuscate
#define glNormal3d glObfuscate
#define glNormal3dv glObfuscate
#define glNormal3f glObfuscate
#define glNormal3fv glObfuscate
#define glNormal3i glObfuscate
#define glNormal3iv glObfuscate
#define glNormal3s glObfuscate
#define glNormal3sv glObfuscate
#define glRasterPos2d glObfuscate
#define glRasterPos2dv glObfuscate
#define glRasterPos2f glObfuscate
#define glRasterPos2fv glObfuscate
#define glRasterPos2i glObfuscate
#define glRasterPos2iv glObfuscate
#define glRasterPos2s glObfuscate
#define glRasterPos2sv glObfuscate
#define glRasterPos3d glObfuscate
#define glRasterPos3dv glObfuscate
#define glRasterPos3f glObfuscate
#define glRasterPos3fv glObfuscate
#define glRasterPos3i glObfuscate
#define glRasterPos3iv glObfuscate
#define glRasterPos3s glObfuscate
#define glRasterPos3sv glObfuscate
#define glRasterPos4d glObfuscate
#define glRasterPos4dv glObfuscate
#define glRasterPos4f glObfuscate
#define glRasterPos4fv glObfuscate
#define glRasterPos4i glObfuscate
#define glRasterPos4iv glObfuscate
#define glRasterPos4s glObfuscate
#define glRasterPos4sv glObfuscate
#define glRectd glObfuscate
#define glRectdv glObfuscate
#define glRectf glObfuscate
#define glRectfv glObfuscate
#define glRecti glObfuscate
#define glRectiv glObfuscate
#define glRects glObfuscate
#define glRectsv glObfuscate
#define glTexCoord1d glObfuscate
#define glTexCoord1dv glObfuscate
#define glTexCoord1f glObfuscate
#define glTexCoord1fv glObfuscate
#define glTexCoord1i glObfuscate
#define glTexCoord1iv glObfuscate
#define glTexCoord1s glObfuscate
#define glTexCoord1sv glObfuscate
#define glTexCoord2d glObfuscate
#define glTexCoord2dv glObfuscate
#define glTexCoord2f glObfuscate
#define glTexCoord2fv glObfuscate
#define glTexCoord2i glObfuscate
#define glTexCoord2iv glObfuscate
#define glTexCoord2s glObfuscate
#define glTexCoord2sv glObfuscate
#define glTexCoord3d glObfuscate
#define glTexCoord3dv glObfuscate
#define glTexCoord3f glObfuscate
#define glTexCoord3fv glObfuscate
#define glTexCoord3i glObfuscate
#define glTexCoord3iv glObfuscate
#define glTexCoord3s glObfuscate
#define glTexCoord3sv glObfuscate
#define glTexCoord4d glObfuscate
#define glTexCoord4dv glObfuscate
#define glTexCoord4f glObfuscate
#define glTexCoord4fv glObfuscate
#define glTexCoord4i glObfuscate
#define glTexCoord4iv glObfuscate
#define glTexCoord4s glObfuscate
#define glTexCoord4sv glObfuscate
#define glVertex2d glObfuscate
#define glVertex2dv glObfuscate
#define glVertex2f glObfuscate
#define glVertex2fv glObfuscate
#define glVertex2i glObfuscate
#define glVertex2iv glObfuscate
#define glVertex2s glObfuscate
#define glVertex2sv glObfuscate
#define glVertex3d glObfuscate
#define glVertex3dv glObfuscate
#define glVertex3f glObfuscate
#define glVertex3fv glObfuscate
#define glVertex3i glObfuscate
#define glVertex3iv glObfuscate
#define glVertex3s glObfuscate
#define glVertex3sv glObfuscate
#define glVertex4d glObfuscate
#define glVertex4dv glObfuscate
#define glVertex4f glObfuscate
#define glVertex4fv glObfuscate
#define glVertex4i glObfuscate
#define glVertex4iv glObfuscate
#define glVertex4s glObfuscate
#define glVertex4sv glObfuscate
#define glClipPlane glObfuscate
#define glColorMaterial glObfuscate
#define glFogf glObfuscate
#define glFogfv glObfuscate
#define glFogi glObfuscate
#define glFogiv glObfuscate
#define glLightf glObfuscate
#define glLightfv glObfuscate
#define glLighti glObfuscate
#define glLightiv glObfuscate
#define glLightModelf glObfuscate
#define glLightModelfv glObfuscate
#define glLightModeli glObfuscate
#define glLightModeliv glObfuscate
#define glLineStipple glObfuscate
#define glMaterialf glObfuscate
#define glMaterialfv glObfuscate
#define glMateriali glObfuscate
#define glMaterialiv glObfuscate
#define glPolygonStipple glObfuscate
#define glShadeModel glObfuscate
#define glTexEnvf glObfuscate
#define glTexEnvfv glObfuscate
#define glTexEnvi glObfuscate
#define glTexEnviv glObfuscate
#define glTexGend glObfuscate
#define glTexGendv glObfuscate
#define glTexGenf glObfuscate
#define glTexGenfv glObfuscate

#endif
