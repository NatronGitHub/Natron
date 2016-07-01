/*THIS FILE WAS GENERATED AUTOMATICALLY FROM glad.h, DO NOT EDIT*/



#ifndef OSGLFUNCTIONS_gl_H
#define OSGLFUNCTIONS_gl_H

#include "OSGLFunctions.h"





extern "C" {
#ifdef DEBUG
extern PFNGLCULLFACEPROC glad_debug_glCullFace;
#else
void extern PFNGLCULLFACEPROC glad_glCullFace;
#endif
#ifdef DEBUG
extern PFNGLFRONTFACEPROC glad_debug_glFrontFace;
#else
void extern PFNGLFRONTFACEPROC glad_glFrontFace;
#endif
#ifdef DEBUG
extern PFNGLHINTPROC glad_debug_glHint;
#else
void extern PFNGLHINTPROC glad_glHint;
#endif
#ifdef DEBUG
extern PFNGLLINEWIDTHPROC glad_debug_glLineWidth;
#else
void extern PFNGLLINEWIDTHPROC glad_glLineWidth;
#endif
#ifdef DEBUG
extern PFNGLPOINTSIZEPROC glad_debug_glPointSize;
#else
void extern PFNGLPOINTSIZEPROC glad_glPointSize;
#endif
#ifdef DEBUG
extern PFNGLPOLYGONMODEPROC glad_debug_glPolygonMode;
#else
void extern PFNGLPOLYGONMODEPROC glad_glPolygonMode;
#endif
#ifdef DEBUG
extern PFNGLSCISSORPROC glad_debug_glScissor;
#else
void extern PFNGLSCISSORPROC glad_glScissor;
#endif
#ifdef DEBUG
extern PFNGLTEXPARAMETERFPROC glad_debug_glTexParameterf;
#else
void extern PFNGLTEXPARAMETERFPROC glad_glTexParameterf;
#endif
#ifdef DEBUG
extern PFNGLTEXPARAMETERFVPROC glad_debug_glTexParameterfv;
#else
void extern PFNGLTEXPARAMETERFVPROC glad_glTexParameterfv;
#endif
#ifdef DEBUG
extern PFNGLTEXPARAMETERIPROC glad_debug_glTexParameteri;
#else
void extern PFNGLTEXPARAMETERIPROC glad_glTexParameteri;
#endif
#ifdef DEBUG
extern PFNGLTEXPARAMETERIVPROC glad_debug_glTexParameteriv;
#else
void extern PFNGLTEXPARAMETERIVPROC glad_glTexParameteriv;
#endif
#ifdef DEBUG
extern PFNGLTEXIMAGE1DPROC glad_debug_glTexImage1D;
#else
void extern PFNGLTEXIMAGE1DPROC glad_glTexImage1D;
#endif
#ifdef DEBUG
extern PFNGLTEXIMAGE2DPROC glad_debug_glTexImage2D;
#else
void extern PFNGLTEXIMAGE2DPROC glad_glTexImage2D;
#endif
#ifdef DEBUG
extern PFNGLDRAWBUFFERPROC glad_debug_glDrawBuffer;
#else
void extern PFNGLDRAWBUFFERPROC glad_glDrawBuffer;
#endif
#ifdef DEBUG
extern PFNGLCLEARPROC glad_debug_glClear;
#else
void extern PFNGLCLEARPROC glad_glClear;
#endif
#ifdef DEBUG
extern PFNGLCLEARCOLORPROC glad_debug_glClearColor;
#else
void extern PFNGLCLEARCOLORPROC glad_glClearColor;
#endif
#ifdef DEBUG
extern PFNGLCLEARSTENCILPROC glad_debug_glClearStencil;
#else
void extern PFNGLCLEARSTENCILPROC glad_glClearStencil;
#endif
#ifdef DEBUG
extern PFNGLCLEARDEPTHPROC glad_debug_glClearDepth;
#else
void extern PFNGLCLEARDEPTHPROC glad_glClearDepth;
#endif
#ifdef DEBUG
extern PFNGLSTENCILMASKPROC glad_debug_glStencilMask;
#else
void extern PFNGLSTENCILMASKPROC glad_glStencilMask;
#endif
#ifdef DEBUG
extern PFNGLCOLORMASKPROC glad_debug_glColorMask;
#else
void extern PFNGLCOLORMASKPROC glad_glColorMask;
#endif
#ifdef DEBUG
extern PFNGLDEPTHMASKPROC glad_debug_glDepthMask;
#else
void extern PFNGLDEPTHMASKPROC glad_glDepthMask;
#endif
#ifdef DEBUG
extern PFNGLDISABLEPROC glad_debug_glDisable;
#else
void extern PFNGLDISABLEPROC glad_glDisable;
#endif
#ifdef DEBUG
extern PFNGLENABLEPROC glad_debug_glEnable;
#else
void extern PFNGLENABLEPROC glad_glEnable;
#endif
#ifdef DEBUG
extern PFNGLFINISHPROC glad_debug_glFinish;
#else
void extern PFNGLFINISHPROC glad_glFinish;
#endif
#ifdef DEBUG
extern PFNGLFLUSHPROC glad_debug_glFlush;
#else
void extern PFNGLFLUSHPROC glad_glFlush;
#endif
#ifdef DEBUG
extern PFNGLBLENDFUNCPROC glad_debug_glBlendFunc;
#else
void extern PFNGLBLENDFUNCPROC glad_glBlendFunc;
#endif
#ifdef DEBUG
extern PFNGLLOGICOPPROC glad_debug_glLogicOp;
#else
void extern PFNGLLOGICOPPROC glad_glLogicOp;
#endif
#ifdef DEBUG
extern PFNGLSTENCILFUNCPROC glad_debug_glStencilFunc;
#else
void extern PFNGLSTENCILFUNCPROC glad_glStencilFunc;
#endif
#ifdef DEBUG
extern PFNGLSTENCILOPPROC glad_debug_glStencilOp;
#else
void extern PFNGLSTENCILOPPROC glad_glStencilOp;
#endif
#ifdef DEBUG
extern PFNGLDEPTHFUNCPROC glad_debug_glDepthFunc;
#else
void extern PFNGLDEPTHFUNCPROC glad_glDepthFunc;
#endif
#ifdef DEBUG
extern PFNGLPIXELSTOREFPROC glad_debug_glPixelStoref;
#else
void extern PFNGLPIXELSTOREFPROC glad_glPixelStoref;
#endif
#ifdef DEBUG
extern PFNGLPIXELSTOREIPROC glad_debug_glPixelStorei;
#else
void extern PFNGLPIXELSTOREIPROC glad_glPixelStorei;
#endif
#ifdef DEBUG
extern PFNGLREADBUFFERPROC glad_debug_glReadBuffer;
#else
void extern PFNGLREADBUFFERPROC glad_glReadBuffer;
#endif
#ifdef DEBUG
extern PFNGLREADPIXELSPROC glad_debug_glReadPixels;
#else
void extern PFNGLREADPIXELSPROC glad_glReadPixels;
#endif
#ifdef DEBUG
extern PFNGLGETBOOLEANVPROC glad_debug_glGetBooleanv;
#else
void extern PFNGLGETBOOLEANVPROC glad_glGetBooleanv;
#endif
#ifdef DEBUG
extern PFNGLGETDOUBLEVPROC glad_debug_glGetDoublev;
#else
void extern PFNGLGETDOUBLEVPROC glad_glGetDoublev;
#endif
#ifdef DEBUG
extern PFNGLGETERRORPROC glad_debug_glGetError;
#else
void extern PFNGLGETERRORPROC glad_glGetError;
#endif
#ifdef DEBUG
extern PFNGLGETFLOATVPROC glad_debug_glGetFloatv;
#else
void extern PFNGLGETFLOATVPROC glad_glGetFloatv;
#endif
#ifdef DEBUG
extern PFNGLGETINTEGERVPROC glad_debug_glGetIntegerv;
#else
void extern PFNGLGETINTEGERVPROC glad_glGetIntegerv;
#endif
#ifdef DEBUG
extern PFNGLGETSTRINGPROC glad_debug_glGetString;
#else
void extern PFNGLGETSTRINGPROC glad_glGetString;
#endif
#ifdef DEBUG
extern PFNGLGETTEXIMAGEPROC glad_debug_glGetTexImage;
#else
void extern PFNGLGETTEXIMAGEPROC glad_glGetTexImage;
#endif
#ifdef DEBUG
extern PFNGLGETTEXPARAMETERFVPROC glad_debug_glGetTexParameterfv;
#else
void extern PFNGLGETTEXPARAMETERFVPROC glad_glGetTexParameterfv;
#endif
#ifdef DEBUG
extern PFNGLGETTEXPARAMETERIVPROC glad_debug_glGetTexParameteriv;
#else
void extern PFNGLGETTEXPARAMETERIVPROC glad_glGetTexParameteriv;
#endif
#ifdef DEBUG
extern PFNGLGETTEXLEVELPARAMETERFVPROC glad_debug_glGetTexLevelParameterfv;
#else
void extern PFNGLGETTEXLEVELPARAMETERFVPROC glad_glGetTexLevelParameterfv;
#endif
#ifdef DEBUG
extern PFNGLGETTEXLEVELPARAMETERIVPROC glad_debug_glGetTexLevelParameteriv;
#else
void extern PFNGLGETTEXLEVELPARAMETERIVPROC glad_glGetTexLevelParameteriv;
#endif
#ifdef DEBUG
extern PFNGLISENABLEDPROC glad_debug_glIsEnabled;
#else
void extern PFNGLISENABLEDPROC glad_glIsEnabled;
#endif
#ifdef DEBUG
extern PFNGLDEPTHRANGEPROC glad_debug_glDepthRange;
#else
void extern PFNGLDEPTHRANGEPROC glad_glDepthRange;
#endif
#ifdef DEBUG
extern PFNGLVIEWPORTPROC glad_debug_glViewport;
#else
void extern PFNGLVIEWPORTPROC glad_glViewport;
#endif
#ifdef DEBUG
extern PFNGLNEWLISTPROC glad_debug_glNewList;
#else
void extern PFNGLNEWLISTPROC glad_glNewList;
#endif
#ifdef DEBUG
extern PFNGLENDLISTPROC glad_debug_glEndList;
#else
void extern PFNGLENDLISTPROC glad_glEndList;
#endif
#ifdef DEBUG
extern PFNGLCALLLISTPROC glad_debug_glCallList;
#else
void extern PFNGLCALLLISTPROC glad_glCallList;
#endif
#ifdef DEBUG
extern PFNGLCALLLISTSPROC glad_debug_glCallLists;
#else
void extern PFNGLCALLLISTSPROC glad_glCallLists;
#endif
#ifdef DEBUG
extern PFNGLDELETELISTSPROC glad_debug_glDeleteLists;
#else
void extern PFNGLDELETELISTSPROC glad_glDeleteLists;
#endif
#ifdef DEBUG
extern PFNGLGENLISTSPROC glad_debug_glGenLists;
#else
void extern PFNGLGENLISTSPROC glad_glGenLists;
#endif
#ifdef DEBUG
extern PFNGLLISTBASEPROC glad_debug_glListBase;
#else
void extern PFNGLLISTBASEPROC glad_glListBase;
#endif
#ifdef DEBUG
extern PFNGLBEGINPROC glad_debug_glBegin;
#else
void extern PFNGLBEGINPROC glad_glBegin;
#endif
#ifdef DEBUG
extern PFNGLBITMAPPROC glad_debug_glBitmap;
#else
void extern PFNGLBITMAPPROC glad_glBitmap;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3BPROC glad_debug_glColor3b;
#else
void extern PFNGLCOLOR3BPROC glad_glColor3b;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3BVPROC glad_debug_glColor3bv;
#else
void extern PFNGLCOLOR3BVPROC glad_glColor3bv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3DPROC glad_debug_glColor3d;
#else
void extern PFNGLCOLOR3DPROC glad_glColor3d;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3DVPROC glad_debug_glColor3dv;
#else
void extern PFNGLCOLOR3DVPROC glad_glColor3dv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3FPROC glad_debug_glColor3f;
#else
void extern PFNGLCOLOR3FPROC glad_glColor3f;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3FVPROC glad_debug_glColor3fv;
#else
void extern PFNGLCOLOR3FVPROC glad_glColor3fv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3IPROC glad_debug_glColor3i;
#else
void extern PFNGLCOLOR3IPROC glad_glColor3i;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3IVPROC glad_debug_glColor3iv;
#else
void extern PFNGLCOLOR3IVPROC glad_glColor3iv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3SPROC glad_debug_glColor3s;
#else
void extern PFNGLCOLOR3SPROC glad_glColor3s;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3SVPROC glad_debug_glColor3sv;
#else
void extern PFNGLCOLOR3SVPROC glad_glColor3sv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3UBPROC glad_debug_glColor3ub;
#else
void extern PFNGLCOLOR3UBPROC glad_glColor3ub;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3UBVPROC glad_debug_glColor3ubv;
#else
void extern PFNGLCOLOR3UBVPROC glad_glColor3ubv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3UIPROC glad_debug_glColor3ui;
#else
void extern PFNGLCOLOR3UIPROC glad_glColor3ui;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3UIVPROC glad_debug_glColor3uiv;
#else
void extern PFNGLCOLOR3UIVPROC glad_glColor3uiv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3USPROC glad_debug_glColor3us;
#else
void extern PFNGLCOLOR3USPROC glad_glColor3us;
#endif
#ifdef DEBUG
extern PFNGLCOLOR3USVPROC glad_debug_glColor3usv;
#else
void extern PFNGLCOLOR3USVPROC glad_glColor3usv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4BPROC glad_debug_glColor4b;
#else
void extern PFNGLCOLOR4BPROC glad_glColor4b;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4BVPROC glad_debug_glColor4bv;
#else
void extern PFNGLCOLOR4BVPROC glad_glColor4bv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4DPROC glad_debug_glColor4d;
#else
void extern PFNGLCOLOR4DPROC glad_glColor4d;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4DVPROC glad_debug_glColor4dv;
#else
void extern PFNGLCOLOR4DVPROC glad_glColor4dv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4FPROC glad_debug_glColor4f;
#else
void extern PFNGLCOLOR4FPROC glad_glColor4f;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4FVPROC glad_debug_glColor4fv;
#else
void extern PFNGLCOLOR4FVPROC glad_glColor4fv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4IPROC glad_debug_glColor4i;
#else
void extern PFNGLCOLOR4IPROC glad_glColor4i;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4IVPROC glad_debug_glColor4iv;
#else
void extern PFNGLCOLOR4IVPROC glad_glColor4iv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4SPROC glad_debug_glColor4s;
#else
void extern PFNGLCOLOR4SPROC glad_glColor4s;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4SVPROC glad_debug_glColor4sv;
#else
void extern PFNGLCOLOR4SVPROC glad_glColor4sv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4UBPROC glad_debug_glColor4ub;
#else
void extern PFNGLCOLOR4UBPROC glad_glColor4ub;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4UBVPROC glad_debug_glColor4ubv;
#else
void extern PFNGLCOLOR4UBVPROC glad_glColor4ubv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4UIPROC glad_debug_glColor4ui;
#else
void extern PFNGLCOLOR4UIPROC glad_glColor4ui;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4UIVPROC glad_debug_glColor4uiv;
#else
void extern PFNGLCOLOR4UIVPROC glad_glColor4uiv;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4USPROC glad_debug_glColor4us;
#else
void extern PFNGLCOLOR4USPROC glad_glColor4us;
#endif
#ifdef DEBUG
extern PFNGLCOLOR4USVPROC glad_debug_glColor4usv;
#else
void extern PFNGLCOLOR4USVPROC glad_glColor4usv;
#endif
#ifdef DEBUG
extern PFNGLEDGEFLAGPROC glad_debug_glEdgeFlag;
#else
void extern PFNGLEDGEFLAGPROC glad_glEdgeFlag;
#endif
#ifdef DEBUG
extern PFNGLEDGEFLAGVPROC glad_debug_glEdgeFlagv;
#else
void extern PFNGLEDGEFLAGVPROC glad_glEdgeFlagv;
#endif
#ifdef DEBUG
extern PFNGLENDPROC glad_debug_glEnd;
#else
void extern PFNGLENDPROC glad_glEnd;
#endif
#ifdef DEBUG
extern PFNGLINDEXDPROC glad_debug_glIndexd;
#else
void extern PFNGLINDEXDPROC glad_glIndexd;
#endif
#ifdef DEBUG
extern PFNGLINDEXDVPROC glad_debug_glIndexdv;
#else
void extern PFNGLINDEXDVPROC glad_glIndexdv;
#endif
#ifdef DEBUG
extern PFNGLINDEXFPROC glad_debug_glIndexf;
#else
void extern PFNGLINDEXFPROC glad_glIndexf;
#endif
#ifdef DEBUG
extern PFNGLINDEXFVPROC glad_debug_glIndexfv;
#else
void extern PFNGLINDEXFVPROC glad_glIndexfv;
#endif
#ifdef DEBUG
extern PFNGLINDEXIPROC glad_debug_glIndexi;
#else
void extern PFNGLINDEXIPROC glad_glIndexi;
#endif
#ifdef DEBUG
extern PFNGLINDEXIVPROC glad_debug_glIndexiv;
#else
void extern PFNGLINDEXIVPROC glad_glIndexiv;
#endif
#ifdef DEBUG
extern PFNGLINDEXSPROC glad_debug_glIndexs;
#else
void extern PFNGLINDEXSPROC glad_glIndexs;
#endif
#ifdef DEBUG
extern PFNGLINDEXSVPROC glad_debug_glIndexsv;
#else
void extern PFNGLINDEXSVPROC glad_glIndexsv;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3BPROC glad_debug_glNormal3b;
#else
void extern PFNGLNORMAL3BPROC glad_glNormal3b;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3BVPROC glad_debug_glNormal3bv;
#else
void extern PFNGLNORMAL3BVPROC glad_glNormal3bv;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3DPROC glad_debug_glNormal3d;
#else
void extern PFNGLNORMAL3DPROC glad_glNormal3d;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3DVPROC glad_debug_glNormal3dv;
#else
void extern PFNGLNORMAL3DVPROC glad_glNormal3dv;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3FPROC glad_debug_glNormal3f;
#else
void extern PFNGLNORMAL3FPROC glad_glNormal3f;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3FVPROC glad_debug_glNormal3fv;
#else
void extern PFNGLNORMAL3FVPROC glad_glNormal3fv;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3IPROC glad_debug_glNormal3i;
#else
void extern PFNGLNORMAL3IPROC glad_glNormal3i;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3IVPROC glad_debug_glNormal3iv;
#else
void extern PFNGLNORMAL3IVPROC glad_glNormal3iv;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3SPROC glad_debug_glNormal3s;
#else
void extern PFNGLNORMAL3SPROC glad_glNormal3s;
#endif
#ifdef DEBUG
extern PFNGLNORMAL3SVPROC glad_debug_glNormal3sv;
#else
void extern PFNGLNORMAL3SVPROC glad_glNormal3sv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS2DPROC glad_debug_glRasterPos2d;
#else
void extern PFNGLRASTERPOS2DPROC glad_glRasterPos2d;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS2DVPROC glad_debug_glRasterPos2dv;
#else
void extern PFNGLRASTERPOS2DVPROC glad_glRasterPos2dv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS2FPROC glad_debug_glRasterPos2f;
#else
void extern PFNGLRASTERPOS2FPROC glad_glRasterPos2f;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS2FVPROC glad_debug_glRasterPos2fv;
#else
void extern PFNGLRASTERPOS2FVPROC glad_glRasterPos2fv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS2IPROC glad_debug_glRasterPos2i;
#else
void extern PFNGLRASTERPOS2IPROC glad_glRasterPos2i;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS2IVPROC glad_debug_glRasterPos2iv;
#else
void extern PFNGLRASTERPOS2IVPROC glad_glRasterPos2iv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS2SPROC glad_debug_glRasterPos2s;
#else
void extern PFNGLRASTERPOS2SPROC glad_glRasterPos2s;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS2SVPROC glad_debug_glRasterPos2sv;
#else
void extern PFNGLRASTERPOS2SVPROC glad_glRasterPos2sv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS3DPROC glad_debug_glRasterPos3d;
#else
void extern PFNGLRASTERPOS3DPROC glad_glRasterPos3d;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS3DVPROC glad_debug_glRasterPos3dv;
#else
void extern PFNGLRASTERPOS3DVPROC glad_glRasterPos3dv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS3FPROC glad_debug_glRasterPos3f;
#else
void extern PFNGLRASTERPOS3FPROC glad_glRasterPos3f;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS3FVPROC glad_debug_glRasterPos3fv;
#else
void extern PFNGLRASTERPOS3FVPROC glad_glRasterPos3fv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS3IPROC glad_debug_glRasterPos3i;
#else
void extern PFNGLRASTERPOS3IPROC glad_glRasterPos3i;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS3IVPROC glad_debug_glRasterPos3iv;
#else
void extern PFNGLRASTERPOS3IVPROC glad_glRasterPos3iv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS3SPROC glad_debug_glRasterPos3s;
#else
void extern PFNGLRASTERPOS3SPROC glad_glRasterPos3s;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS3SVPROC glad_debug_glRasterPos3sv;
#else
void extern PFNGLRASTERPOS3SVPROC glad_glRasterPos3sv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS4DPROC glad_debug_glRasterPos4d;
#else
void extern PFNGLRASTERPOS4DPROC glad_glRasterPos4d;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS4DVPROC glad_debug_glRasterPos4dv;
#else
void extern PFNGLRASTERPOS4DVPROC glad_glRasterPos4dv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS4FPROC glad_debug_glRasterPos4f;
#else
void extern PFNGLRASTERPOS4FPROC glad_glRasterPos4f;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS4FVPROC glad_debug_glRasterPos4fv;
#else
void extern PFNGLRASTERPOS4FVPROC glad_glRasterPos4fv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS4IPROC glad_debug_glRasterPos4i;
#else
void extern PFNGLRASTERPOS4IPROC glad_glRasterPos4i;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS4IVPROC glad_debug_glRasterPos4iv;
#else
void extern PFNGLRASTERPOS4IVPROC glad_glRasterPos4iv;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS4SPROC glad_debug_glRasterPos4s;
#else
void extern PFNGLRASTERPOS4SPROC glad_glRasterPos4s;
#endif
#ifdef DEBUG
extern PFNGLRASTERPOS4SVPROC glad_debug_glRasterPos4sv;
#else
void extern PFNGLRASTERPOS4SVPROC glad_glRasterPos4sv;
#endif
#ifdef DEBUG
extern PFNGLRECTDPROC glad_debug_glRectd;
#else
void extern PFNGLRECTDPROC glad_glRectd;
#endif
#ifdef DEBUG
extern PFNGLRECTDVPROC glad_debug_glRectdv;
#else
void extern PFNGLRECTDVPROC glad_glRectdv;
#endif
#ifdef DEBUG
extern PFNGLRECTFPROC glad_debug_glRectf;
#else
void extern PFNGLRECTFPROC glad_glRectf;
#endif
#ifdef DEBUG
extern PFNGLRECTFVPROC glad_debug_glRectfv;
#else
void extern PFNGLRECTFVPROC glad_glRectfv;
#endif
#ifdef DEBUG
extern PFNGLRECTIPROC glad_debug_glRecti;
#else
void extern PFNGLRECTIPROC glad_glRecti;
#endif
#ifdef DEBUG
extern PFNGLRECTIVPROC glad_debug_glRectiv;
#else
void extern PFNGLRECTIVPROC glad_glRectiv;
#endif
#ifdef DEBUG
extern PFNGLRECTSPROC glad_debug_glRects;
#else
void extern PFNGLRECTSPROC glad_glRects;
#endif
#ifdef DEBUG
extern PFNGLRECTSVPROC glad_debug_glRectsv;
#else
void extern PFNGLRECTSVPROC glad_glRectsv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD1DPROC glad_debug_glTexCoord1d;
#else
void extern PFNGLTEXCOORD1DPROC glad_glTexCoord1d;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD1DVPROC glad_debug_glTexCoord1dv;
#else
void extern PFNGLTEXCOORD1DVPROC glad_glTexCoord1dv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD1FPROC glad_debug_glTexCoord1f;
#else
void extern PFNGLTEXCOORD1FPROC glad_glTexCoord1f;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD1FVPROC glad_debug_glTexCoord1fv;
#else
void extern PFNGLTEXCOORD1FVPROC glad_glTexCoord1fv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD1IPROC glad_debug_glTexCoord1i;
#else
void extern PFNGLTEXCOORD1IPROC glad_glTexCoord1i;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD1IVPROC glad_debug_glTexCoord1iv;
#else
void extern PFNGLTEXCOORD1IVPROC glad_glTexCoord1iv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD1SPROC glad_debug_glTexCoord1s;
#else
void extern PFNGLTEXCOORD1SPROC glad_glTexCoord1s;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD1SVPROC glad_debug_glTexCoord1sv;
#else
void extern PFNGLTEXCOORD1SVPROC glad_glTexCoord1sv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD2DPROC glad_debug_glTexCoord2d;
#else
void extern PFNGLTEXCOORD2DPROC glad_glTexCoord2d;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD2DVPROC glad_debug_glTexCoord2dv;
#else
void extern PFNGLTEXCOORD2DVPROC glad_glTexCoord2dv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD2FPROC glad_debug_glTexCoord2f;
#else
void extern PFNGLTEXCOORD2FPROC glad_glTexCoord2f;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD2FVPROC glad_debug_glTexCoord2fv;
#else
void extern PFNGLTEXCOORD2FVPROC glad_glTexCoord2fv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD2IPROC glad_debug_glTexCoord2i;
#else
void extern PFNGLTEXCOORD2IPROC glad_glTexCoord2i;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD2IVPROC glad_debug_glTexCoord2iv;
#else
void extern PFNGLTEXCOORD2IVPROC glad_glTexCoord2iv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD2SPROC glad_debug_glTexCoord2s;
#else
void extern PFNGLTEXCOORD2SPROC glad_glTexCoord2s;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD2SVPROC glad_debug_glTexCoord2sv;
#else
void extern PFNGLTEXCOORD2SVPROC glad_glTexCoord2sv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD3DPROC glad_debug_glTexCoord3d;
#else
void extern PFNGLTEXCOORD3DPROC glad_glTexCoord3d;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD3DVPROC glad_debug_glTexCoord3dv;
#else
void extern PFNGLTEXCOORD3DVPROC glad_glTexCoord3dv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD3FPROC glad_debug_glTexCoord3f;
#else
void extern PFNGLTEXCOORD3FPROC glad_glTexCoord3f;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD3FVPROC glad_debug_glTexCoord3fv;
#else
void extern PFNGLTEXCOORD3FVPROC glad_glTexCoord3fv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD3IPROC glad_debug_glTexCoord3i;
#else
void extern PFNGLTEXCOORD3IPROC glad_glTexCoord3i;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD3IVPROC glad_debug_glTexCoord3iv;
#else
void extern PFNGLTEXCOORD3IVPROC glad_glTexCoord3iv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD3SPROC glad_debug_glTexCoord3s;
#else
void extern PFNGLTEXCOORD3SPROC glad_glTexCoord3s;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD3SVPROC glad_debug_glTexCoord3sv;
#else
void extern PFNGLTEXCOORD3SVPROC glad_glTexCoord3sv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD4DPROC glad_debug_glTexCoord4d;
#else
void extern PFNGLTEXCOORD4DPROC glad_glTexCoord4d;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD4DVPROC glad_debug_glTexCoord4dv;
#else
void extern PFNGLTEXCOORD4DVPROC glad_glTexCoord4dv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD4FPROC glad_debug_glTexCoord4f;
#else
void extern PFNGLTEXCOORD4FPROC glad_glTexCoord4f;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD4FVPROC glad_debug_glTexCoord4fv;
#else
void extern PFNGLTEXCOORD4FVPROC glad_glTexCoord4fv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD4IPROC glad_debug_glTexCoord4i;
#else
void extern PFNGLTEXCOORD4IPROC glad_glTexCoord4i;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD4IVPROC glad_debug_glTexCoord4iv;
#else
void extern PFNGLTEXCOORD4IVPROC glad_glTexCoord4iv;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD4SPROC glad_debug_glTexCoord4s;
#else
void extern PFNGLTEXCOORD4SPROC glad_glTexCoord4s;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORD4SVPROC glad_debug_glTexCoord4sv;
#else
void extern PFNGLTEXCOORD4SVPROC glad_glTexCoord4sv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX2DPROC glad_debug_glVertex2d;
#else
void extern PFNGLVERTEX2DPROC glad_glVertex2d;
#endif
#ifdef DEBUG
extern PFNGLVERTEX2DVPROC glad_debug_glVertex2dv;
#else
void extern PFNGLVERTEX2DVPROC glad_glVertex2dv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX2FPROC glad_debug_glVertex2f;
#else
void extern PFNGLVERTEX2FPROC glad_glVertex2f;
#endif
#ifdef DEBUG
extern PFNGLVERTEX2FVPROC glad_debug_glVertex2fv;
#else
void extern PFNGLVERTEX2FVPROC glad_glVertex2fv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX2IPROC glad_debug_glVertex2i;
#else
void extern PFNGLVERTEX2IPROC glad_glVertex2i;
#endif
#ifdef DEBUG
extern PFNGLVERTEX2IVPROC glad_debug_glVertex2iv;
#else
void extern PFNGLVERTEX2IVPROC glad_glVertex2iv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX2SPROC glad_debug_glVertex2s;
#else
void extern PFNGLVERTEX2SPROC glad_glVertex2s;
#endif
#ifdef DEBUG
extern PFNGLVERTEX2SVPROC glad_debug_glVertex2sv;
#else
void extern PFNGLVERTEX2SVPROC glad_glVertex2sv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX3DPROC glad_debug_glVertex3d;
#else
void extern PFNGLVERTEX3DPROC glad_glVertex3d;
#endif
#ifdef DEBUG
extern PFNGLVERTEX3DVPROC glad_debug_glVertex3dv;
#else
void extern PFNGLVERTEX3DVPROC glad_glVertex3dv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX3FPROC glad_debug_glVertex3f;
#else
void extern PFNGLVERTEX3FPROC glad_glVertex3f;
#endif
#ifdef DEBUG
extern PFNGLVERTEX3FVPROC glad_debug_glVertex3fv;
#else
void extern PFNGLVERTEX3FVPROC glad_glVertex3fv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX3IPROC glad_debug_glVertex3i;
#else
void extern PFNGLVERTEX3IPROC glad_glVertex3i;
#endif
#ifdef DEBUG
extern PFNGLVERTEX3IVPROC glad_debug_glVertex3iv;
#else
void extern PFNGLVERTEX3IVPROC glad_glVertex3iv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX3SPROC glad_debug_glVertex3s;
#else
void extern PFNGLVERTEX3SPROC glad_glVertex3s;
#endif
#ifdef DEBUG
extern PFNGLVERTEX3SVPROC glad_debug_glVertex3sv;
#else
void extern PFNGLVERTEX3SVPROC glad_glVertex3sv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX4DPROC glad_debug_glVertex4d;
#else
void extern PFNGLVERTEX4DPROC glad_glVertex4d;
#endif
#ifdef DEBUG
extern PFNGLVERTEX4DVPROC glad_debug_glVertex4dv;
#else
void extern PFNGLVERTEX4DVPROC glad_glVertex4dv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX4FPROC glad_debug_glVertex4f;
#else
void extern PFNGLVERTEX4FPROC glad_glVertex4f;
#endif
#ifdef DEBUG
extern PFNGLVERTEX4FVPROC glad_debug_glVertex4fv;
#else
void extern PFNGLVERTEX4FVPROC glad_glVertex4fv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX4IPROC glad_debug_glVertex4i;
#else
void extern PFNGLVERTEX4IPROC glad_glVertex4i;
#endif
#ifdef DEBUG
extern PFNGLVERTEX4IVPROC glad_debug_glVertex4iv;
#else
void extern PFNGLVERTEX4IVPROC glad_glVertex4iv;
#endif
#ifdef DEBUG
extern PFNGLVERTEX4SPROC glad_debug_glVertex4s;
#else
void extern PFNGLVERTEX4SPROC glad_glVertex4s;
#endif
#ifdef DEBUG
extern PFNGLVERTEX4SVPROC glad_debug_glVertex4sv;
#else
void extern PFNGLVERTEX4SVPROC glad_glVertex4sv;
#endif
#ifdef DEBUG
extern PFNGLCLIPPLANEPROC glad_debug_glClipPlane;
#else
void extern PFNGLCLIPPLANEPROC glad_glClipPlane;
#endif
#ifdef DEBUG
extern PFNGLCOLORMATERIALPROC glad_debug_glColorMaterial;
#else
void extern PFNGLCOLORMATERIALPROC glad_glColorMaterial;
#endif
#ifdef DEBUG
extern PFNGLFOGFPROC glad_debug_glFogf;
#else
void extern PFNGLFOGFPROC glad_glFogf;
#endif
#ifdef DEBUG
extern PFNGLFOGFVPROC glad_debug_glFogfv;
#else
void extern PFNGLFOGFVPROC glad_glFogfv;
#endif
#ifdef DEBUG
extern PFNGLFOGIPROC glad_debug_glFogi;
#else
void extern PFNGLFOGIPROC glad_glFogi;
#endif
#ifdef DEBUG
extern PFNGLFOGIVPROC glad_debug_glFogiv;
#else
void extern PFNGLFOGIVPROC glad_glFogiv;
#endif
#ifdef DEBUG
extern PFNGLLIGHTFPROC glad_debug_glLightf;
#else
void extern PFNGLLIGHTFPROC glad_glLightf;
#endif
#ifdef DEBUG
extern PFNGLLIGHTFVPROC glad_debug_glLightfv;
#else
void extern PFNGLLIGHTFVPROC glad_glLightfv;
#endif
#ifdef DEBUG
extern PFNGLLIGHTIPROC glad_debug_glLighti;
#else
void extern PFNGLLIGHTIPROC glad_glLighti;
#endif
#ifdef DEBUG
extern PFNGLLIGHTIVPROC glad_debug_glLightiv;
#else
void extern PFNGLLIGHTIVPROC glad_glLightiv;
#endif
#ifdef DEBUG
extern PFNGLLIGHTMODELFPROC glad_debug_glLightModelf;
#else
void extern PFNGLLIGHTMODELFPROC glad_glLightModelf;
#endif
#ifdef DEBUG
extern PFNGLLIGHTMODELFVPROC glad_debug_glLightModelfv;
#else
void extern PFNGLLIGHTMODELFVPROC glad_glLightModelfv;
#endif
#ifdef DEBUG
extern PFNGLLIGHTMODELIPROC glad_debug_glLightModeli;
#else
void extern PFNGLLIGHTMODELIPROC glad_glLightModeli;
#endif
#ifdef DEBUG
extern PFNGLLIGHTMODELIVPROC glad_debug_glLightModeliv;
#else
void extern PFNGLLIGHTMODELIVPROC glad_glLightModeliv;
#endif
#ifdef DEBUG
extern PFNGLLINESTIPPLEPROC glad_debug_glLineStipple;
#else
void extern PFNGLLINESTIPPLEPROC glad_glLineStipple;
#endif
#ifdef DEBUG
extern PFNGLMATERIALFPROC glad_debug_glMaterialf;
#else
void extern PFNGLMATERIALFPROC glad_glMaterialf;
#endif
#ifdef DEBUG
extern PFNGLMATERIALFVPROC glad_debug_glMaterialfv;
#else
void extern PFNGLMATERIALFVPROC glad_glMaterialfv;
#endif
#ifdef DEBUG
extern PFNGLMATERIALIPROC glad_debug_glMateriali;
#else
void extern PFNGLMATERIALIPROC glad_glMateriali;
#endif
#ifdef DEBUG
extern PFNGLMATERIALIVPROC glad_debug_glMaterialiv;
#else
void extern PFNGLMATERIALIVPROC glad_glMaterialiv;
#endif
#ifdef DEBUG
extern PFNGLPOLYGONSTIPPLEPROC glad_debug_glPolygonStipple;
#else
void extern PFNGLPOLYGONSTIPPLEPROC glad_glPolygonStipple;
#endif
#ifdef DEBUG
extern PFNGLSHADEMODELPROC glad_debug_glShadeModel;
#else
void extern PFNGLSHADEMODELPROC glad_glShadeModel;
#endif
#ifdef DEBUG
extern PFNGLTEXENVFPROC glad_debug_glTexEnvf;
#else
void extern PFNGLTEXENVFPROC glad_glTexEnvf;
#endif
#ifdef DEBUG
extern PFNGLTEXENVFVPROC glad_debug_glTexEnvfv;
#else
void extern PFNGLTEXENVFVPROC glad_glTexEnvfv;
#endif
#ifdef DEBUG
extern PFNGLTEXENVIPROC glad_debug_glTexEnvi;
#else
void extern PFNGLTEXENVIPROC glad_glTexEnvi;
#endif
#ifdef DEBUG
extern PFNGLTEXENVIVPROC glad_debug_glTexEnviv;
#else
void extern PFNGLTEXENVIVPROC glad_glTexEnviv;
#endif
#ifdef DEBUG
extern PFNGLTEXGENDPROC glad_debug_glTexGend;
#else
void extern PFNGLTEXGENDPROC glad_glTexGend;
#endif
#ifdef DEBUG
extern PFNGLTEXGENDVPROC glad_debug_glTexGendv;
#else
void extern PFNGLTEXGENDVPROC glad_glTexGendv;
#endif
#ifdef DEBUG
extern PFNGLTEXGENFPROC glad_debug_glTexGenf;
#else
void extern PFNGLTEXGENFPROC glad_glTexGenf;
#endif
#ifdef DEBUG
extern PFNGLTEXGENFVPROC glad_debug_glTexGenfv;
#else
void extern PFNGLTEXGENFVPROC glad_glTexGenfv;
#endif
#ifdef DEBUG
extern PFNGLTEXGENIPROC glad_debug_glTexGeni;
#else
void extern PFNGLTEXGENIPROC glad_glTexGeni;
#endif
#ifdef DEBUG
extern PFNGLTEXGENIVPROC glad_debug_glTexGeniv;
#else
void extern PFNGLTEXGENIVPROC glad_glTexGeniv;
#endif
#ifdef DEBUG
extern PFNGLFEEDBACKBUFFERPROC glad_debug_glFeedbackBuffer;
#else
void extern PFNGLFEEDBACKBUFFERPROC glad_glFeedbackBuffer;
#endif
#ifdef DEBUG
extern PFNGLSELECTBUFFERPROC glad_debug_glSelectBuffer;
#else
void extern PFNGLSELECTBUFFERPROC glad_glSelectBuffer;
#endif
#ifdef DEBUG
extern PFNGLRENDERMODEPROC glad_debug_glRenderMode;
#else
void extern PFNGLRENDERMODEPROC glad_glRenderMode;
#endif
#ifdef DEBUG
extern PFNGLINITNAMESPROC glad_debug_glInitNames;
#else
void extern PFNGLINITNAMESPROC glad_glInitNames;
#endif
#ifdef DEBUG
extern PFNGLLOADNAMEPROC glad_debug_glLoadName;
#else
void extern PFNGLLOADNAMEPROC glad_glLoadName;
#endif
#ifdef DEBUG
extern PFNGLPASSTHROUGHPROC glad_debug_glPassThrough;
#else
void extern PFNGLPASSTHROUGHPROC glad_glPassThrough;
#endif
#ifdef DEBUG
extern PFNGLPOPNAMEPROC glad_debug_glPopName;
#else
void extern PFNGLPOPNAMEPROC glad_glPopName;
#endif
#ifdef DEBUG
extern PFNGLPUSHNAMEPROC glad_debug_glPushName;
#else
void extern PFNGLPUSHNAMEPROC glad_glPushName;
#endif
#ifdef DEBUG
extern PFNGLCLEARACCUMPROC glad_debug_glClearAccum;
#else
void extern PFNGLCLEARACCUMPROC glad_glClearAccum;
#endif
#ifdef DEBUG
extern PFNGLCLEARINDEXPROC glad_debug_glClearIndex;
#else
void extern PFNGLCLEARINDEXPROC glad_glClearIndex;
#endif
#ifdef DEBUG
extern PFNGLINDEXMASKPROC glad_debug_glIndexMask;
#else
void extern PFNGLINDEXMASKPROC glad_glIndexMask;
#endif
#ifdef DEBUG
extern PFNGLACCUMPROC glad_debug_glAccum;
#else
void extern PFNGLACCUMPROC glad_glAccum;
#endif
#ifdef DEBUG
extern PFNGLPOPATTRIBPROC glad_debug_glPopAttrib;
#else
void extern PFNGLPOPATTRIBPROC glad_glPopAttrib;
#endif
#ifdef DEBUG
extern PFNGLPUSHATTRIBPROC glad_debug_glPushAttrib;
#else
void extern PFNGLPUSHATTRIBPROC glad_glPushAttrib;
#endif
#ifdef DEBUG
extern PFNGLMAP1DPROC glad_debug_glMap1d;
#else
void extern PFNGLMAP1DPROC glad_glMap1d;
#endif
#ifdef DEBUG
extern PFNGLMAP1FPROC glad_debug_glMap1f;
#else
void extern PFNGLMAP1FPROC glad_glMap1f;
#endif
#ifdef DEBUG
extern PFNGLMAP2DPROC glad_debug_glMap2d;
#else
void extern PFNGLMAP2DPROC glad_glMap2d;
#endif
#ifdef DEBUG
extern PFNGLMAP2FPROC glad_debug_glMap2f;
#else
void extern PFNGLMAP2FPROC glad_glMap2f;
#endif
#ifdef DEBUG
extern PFNGLMAPGRID1DPROC glad_debug_glMapGrid1d;
#else
void extern PFNGLMAPGRID1DPROC glad_glMapGrid1d;
#endif
#ifdef DEBUG
extern PFNGLMAPGRID1FPROC glad_debug_glMapGrid1f;
#else
void extern PFNGLMAPGRID1FPROC glad_glMapGrid1f;
#endif
#ifdef DEBUG
extern PFNGLMAPGRID2DPROC glad_debug_glMapGrid2d;
#else
void extern PFNGLMAPGRID2DPROC glad_glMapGrid2d;
#endif
#ifdef DEBUG
extern PFNGLMAPGRID2FPROC glad_debug_glMapGrid2f;
#else
void extern PFNGLMAPGRID2FPROC glad_glMapGrid2f;
#endif
#ifdef DEBUG
extern PFNGLEVALCOORD1DPROC glad_debug_glEvalCoord1d;
#else
void extern PFNGLEVALCOORD1DPROC glad_glEvalCoord1d;
#endif
#ifdef DEBUG
extern PFNGLEVALCOORD1DVPROC glad_debug_glEvalCoord1dv;
#else
void extern PFNGLEVALCOORD1DVPROC glad_glEvalCoord1dv;
#endif
#ifdef DEBUG
extern PFNGLEVALCOORD1FPROC glad_debug_glEvalCoord1f;
#else
void extern PFNGLEVALCOORD1FPROC glad_glEvalCoord1f;
#endif
#ifdef DEBUG
extern PFNGLEVALCOORD1FVPROC glad_debug_glEvalCoord1fv;
#else
void extern PFNGLEVALCOORD1FVPROC glad_glEvalCoord1fv;
#endif
#ifdef DEBUG
extern PFNGLEVALCOORD2DPROC glad_debug_glEvalCoord2d;
#else
void extern PFNGLEVALCOORD2DPROC glad_glEvalCoord2d;
#endif
#ifdef DEBUG
extern PFNGLEVALCOORD2DVPROC glad_debug_glEvalCoord2dv;
#else
void extern PFNGLEVALCOORD2DVPROC glad_glEvalCoord2dv;
#endif
#ifdef DEBUG
extern PFNGLEVALCOORD2FPROC glad_debug_glEvalCoord2f;
#else
void extern PFNGLEVALCOORD2FPROC glad_glEvalCoord2f;
#endif
#ifdef DEBUG
extern PFNGLEVALCOORD2FVPROC glad_debug_glEvalCoord2fv;
#else
void extern PFNGLEVALCOORD2FVPROC glad_glEvalCoord2fv;
#endif
#ifdef DEBUG
extern PFNGLEVALMESH1PROC glad_debug_glEvalMesh1;
#else
void extern PFNGLEVALMESH1PROC glad_glEvalMesh1;
#endif
#ifdef DEBUG
extern PFNGLEVALPOINT1PROC glad_debug_glEvalPoint1;
#else
void extern PFNGLEVALPOINT1PROC glad_glEvalPoint1;
#endif
#ifdef DEBUG
extern PFNGLEVALMESH2PROC glad_debug_glEvalMesh2;
#else
void extern PFNGLEVALMESH2PROC glad_glEvalMesh2;
#endif
#ifdef DEBUG
extern PFNGLEVALPOINT2PROC glad_debug_glEvalPoint2;
#else
void extern PFNGLEVALPOINT2PROC glad_glEvalPoint2;
#endif
#ifdef DEBUG
extern PFNGLALPHAFUNCPROC glad_debug_glAlphaFunc;
#else
void extern PFNGLALPHAFUNCPROC glad_glAlphaFunc;
#endif
#ifdef DEBUG
extern PFNGLPIXELZOOMPROC glad_debug_glPixelZoom;
#else
void extern PFNGLPIXELZOOMPROC glad_glPixelZoom;
#endif
#ifdef DEBUG
extern PFNGLPIXELTRANSFERFPROC glad_debug_glPixelTransferf;
#else
void extern PFNGLPIXELTRANSFERFPROC glad_glPixelTransferf;
#endif
#ifdef DEBUG
extern PFNGLPIXELTRANSFERIPROC glad_debug_glPixelTransferi;
#else
void extern PFNGLPIXELTRANSFERIPROC glad_glPixelTransferi;
#endif
#ifdef DEBUG
extern PFNGLPIXELMAPFVPROC glad_debug_glPixelMapfv;
#else
void extern PFNGLPIXELMAPFVPROC glad_glPixelMapfv;
#endif
#ifdef DEBUG
extern PFNGLPIXELMAPUIVPROC glad_debug_glPixelMapuiv;
#else
void extern PFNGLPIXELMAPUIVPROC glad_glPixelMapuiv;
#endif
#ifdef DEBUG
extern PFNGLPIXELMAPUSVPROC glad_debug_glPixelMapusv;
#else
void extern PFNGLPIXELMAPUSVPROC glad_glPixelMapusv;
#endif
#ifdef DEBUG
extern PFNGLCOPYPIXELSPROC glad_debug_glCopyPixels;
#else
void extern PFNGLCOPYPIXELSPROC glad_glCopyPixels;
#endif
#ifdef DEBUG
extern PFNGLDRAWPIXELSPROC glad_debug_glDrawPixels;
#else
void extern PFNGLDRAWPIXELSPROC glad_glDrawPixels;
#endif
#ifdef DEBUG
extern PFNGLGETCLIPPLANEPROC glad_debug_glGetClipPlane;
#else
void extern PFNGLGETCLIPPLANEPROC glad_glGetClipPlane;
#endif
#ifdef DEBUG
extern PFNGLGETLIGHTFVPROC glad_debug_glGetLightfv;
#else
void extern PFNGLGETLIGHTFVPROC glad_glGetLightfv;
#endif
#ifdef DEBUG
extern PFNGLGETLIGHTIVPROC glad_debug_glGetLightiv;
#else
void extern PFNGLGETLIGHTIVPROC glad_glGetLightiv;
#endif
#ifdef DEBUG
extern PFNGLGETMAPDVPROC glad_debug_glGetMapdv;
#else
void extern PFNGLGETMAPDVPROC glad_glGetMapdv;
#endif
#ifdef DEBUG
extern PFNGLGETMAPFVPROC glad_debug_glGetMapfv;
#else
void extern PFNGLGETMAPFVPROC glad_glGetMapfv;
#endif
#ifdef DEBUG
extern PFNGLGETMAPIVPROC glad_debug_glGetMapiv;
#else
void extern PFNGLGETMAPIVPROC glad_glGetMapiv;
#endif
#ifdef DEBUG
extern PFNGLGETMATERIALFVPROC glad_debug_glGetMaterialfv;
#else
void extern PFNGLGETMATERIALFVPROC glad_glGetMaterialfv;
#endif
#ifdef DEBUG
extern PFNGLGETMATERIALIVPROC glad_debug_glGetMaterialiv;
#else
void extern PFNGLGETMATERIALIVPROC glad_glGetMaterialiv;
#endif
#ifdef DEBUG
extern PFNGLGETPIXELMAPFVPROC glad_debug_glGetPixelMapfv;
#else
void extern PFNGLGETPIXELMAPFVPROC glad_glGetPixelMapfv;
#endif
#ifdef DEBUG
extern PFNGLGETPIXELMAPUIVPROC glad_debug_glGetPixelMapuiv;
#else
void extern PFNGLGETPIXELMAPUIVPROC glad_glGetPixelMapuiv;
#endif
#ifdef DEBUG
extern PFNGLGETPIXELMAPUSVPROC glad_debug_glGetPixelMapusv;
#else
void extern PFNGLGETPIXELMAPUSVPROC glad_glGetPixelMapusv;
#endif
#ifdef DEBUG
extern PFNGLGETPOLYGONSTIPPLEPROC glad_debug_glGetPolygonStipple;
#else
void extern PFNGLGETPOLYGONSTIPPLEPROC glad_glGetPolygonStipple;
#endif
#ifdef DEBUG
extern PFNGLGETTEXENVFVPROC glad_debug_glGetTexEnvfv;
#else
void extern PFNGLGETTEXENVFVPROC glad_glGetTexEnvfv;
#endif
#ifdef DEBUG
extern PFNGLGETTEXENVIVPROC glad_debug_glGetTexEnviv;
#else
void extern PFNGLGETTEXENVIVPROC glad_glGetTexEnviv;
#endif
#ifdef DEBUG
extern PFNGLGETTEXGENDVPROC glad_debug_glGetTexGendv;
#else
void extern PFNGLGETTEXGENDVPROC glad_glGetTexGendv;
#endif
#ifdef DEBUG
extern PFNGLGETTEXGENFVPROC glad_debug_glGetTexGenfv;
#else
void extern PFNGLGETTEXGENFVPROC glad_glGetTexGenfv;
#endif
#ifdef DEBUG
extern PFNGLGETTEXGENIVPROC glad_debug_glGetTexGeniv;
#else
void extern PFNGLGETTEXGENIVPROC glad_glGetTexGeniv;
#endif
#ifdef DEBUG
extern PFNGLISLISTPROC glad_debug_glIsList;
#else
void extern PFNGLISLISTPROC glad_glIsList;
#endif
#ifdef DEBUG
extern PFNGLFRUSTUMPROC glad_debug_glFrustum;
#else
void extern PFNGLFRUSTUMPROC glad_glFrustum;
#endif
#ifdef DEBUG
extern PFNGLLOADIDENTITYPROC glad_debug_glLoadIdentity;
#else
void extern PFNGLLOADIDENTITYPROC glad_glLoadIdentity;
#endif
#ifdef DEBUG
extern PFNGLLOADMATRIXFPROC glad_debug_glLoadMatrixf;
#else
void extern PFNGLLOADMATRIXFPROC glad_glLoadMatrixf;
#endif
#ifdef DEBUG
extern PFNGLLOADMATRIXDPROC glad_debug_glLoadMatrixd;
#else
void extern PFNGLLOADMATRIXDPROC glad_glLoadMatrixd;
#endif
#ifdef DEBUG
extern PFNGLMATRIXMODEPROC glad_debug_glMatrixMode;
#else
void extern PFNGLMATRIXMODEPROC glad_glMatrixMode;
#endif
#ifdef DEBUG
extern PFNGLMULTMATRIXFPROC glad_debug_glMultMatrixf;
#else
void extern PFNGLMULTMATRIXFPROC glad_glMultMatrixf;
#endif
#ifdef DEBUG
extern PFNGLMULTMATRIXDPROC glad_debug_glMultMatrixd;
#else
void extern PFNGLMULTMATRIXDPROC glad_glMultMatrixd;
#endif
#ifdef DEBUG
extern PFNGLORTHOPROC glad_debug_glOrtho;
#else
void extern PFNGLORTHOPROC glad_glOrtho;
#endif
#ifdef DEBUG
extern PFNGLPOPMATRIXPROC glad_debug_glPopMatrix;
#else
void extern PFNGLPOPMATRIXPROC glad_glPopMatrix;
#endif
#ifdef DEBUG
extern PFNGLPUSHMATRIXPROC glad_debug_glPushMatrix;
#else
void extern PFNGLPUSHMATRIXPROC glad_glPushMatrix;
#endif
#ifdef DEBUG
extern PFNGLROTATEDPROC glad_debug_glRotated;
#else
void extern PFNGLROTATEDPROC glad_glRotated;
#endif
#ifdef DEBUG
extern PFNGLROTATEFPROC glad_debug_glRotatef;
#else
void extern PFNGLROTATEFPROC glad_glRotatef;
#endif
#ifdef DEBUG
extern PFNGLSCALEDPROC glad_debug_glScaled;
#else
void extern PFNGLSCALEDPROC glad_glScaled;
#endif
#ifdef DEBUG
extern PFNGLSCALEFPROC glad_debug_glScalef;
#else
void extern PFNGLSCALEFPROC glad_glScalef;
#endif
#ifdef DEBUG
extern PFNGLTRANSLATEDPROC glad_debug_glTranslated;
#else
void extern PFNGLTRANSLATEDPROC glad_glTranslated;
#endif
#ifdef DEBUG
extern PFNGLTRANSLATEFPROC glad_debug_glTranslatef;
#else
void extern PFNGLTRANSLATEFPROC glad_glTranslatef;
#endif
#ifdef DEBUG
extern PFNGLDRAWARRAYSPROC glad_debug_glDrawArrays;
#else
void extern PFNGLDRAWARRAYSPROC glad_glDrawArrays;
#endif
#ifdef DEBUG
extern PFNGLDRAWELEMENTSPROC glad_debug_glDrawElements;
#else
void extern PFNGLDRAWELEMENTSPROC glad_glDrawElements;
#endif
#ifdef DEBUG
extern PFNGLGETPOINTERVPROC glad_debug_glGetPointerv;
#else
void extern PFNGLGETPOINTERVPROC glad_glGetPointerv;
#endif
#ifdef DEBUG
extern PFNGLPOLYGONOFFSETPROC glad_debug_glPolygonOffset;
#else
void extern PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset;
#endif
#ifdef DEBUG
extern PFNGLCOPYTEXIMAGE1DPROC glad_debug_glCopyTexImage1D;
#else
void extern PFNGLCOPYTEXIMAGE1DPROC glad_glCopyTexImage1D;
#endif
#ifdef DEBUG
extern PFNGLCOPYTEXIMAGE2DPROC glad_debug_glCopyTexImage2D;
#else
void extern PFNGLCOPYTEXIMAGE2DPROC glad_glCopyTexImage2D;
#endif
#ifdef DEBUG
extern PFNGLCOPYTEXSUBIMAGE1DPROC glad_debug_glCopyTexSubImage1D;
#else
void extern PFNGLCOPYTEXSUBIMAGE1DPROC glad_glCopyTexSubImage1D;
#endif
#ifdef DEBUG
extern PFNGLCOPYTEXSUBIMAGE2DPROC glad_debug_glCopyTexSubImage2D;
#else
void extern PFNGLCOPYTEXSUBIMAGE2DPROC glad_glCopyTexSubImage2D;
#endif
#ifdef DEBUG
extern PFNGLTEXSUBIMAGE1DPROC glad_debug_glTexSubImage1D;
#else
void extern PFNGLTEXSUBIMAGE1DPROC glad_glTexSubImage1D;
#endif
#ifdef DEBUG
extern PFNGLTEXSUBIMAGE2DPROC glad_debug_glTexSubImage2D;
#else
void extern PFNGLTEXSUBIMAGE2DPROC glad_glTexSubImage2D;
#endif
#ifdef DEBUG
extern PFNGLBINDTEXTUREPROC glad_debug_glBindTexture;
#else
void extern PFNGLBINDTEXTUREPROC glad_glBindTexture;
#endif
#ifdef DEBUG
extern PFNGLDELETETEXTURESPROC glad_debug_glDeleteTextures;
#else
void extern PFNGLDELETETEXTURESPROC glad_glDeleteTextures;
#endif
#ifdef DEBUG
extern PFNGLGENTEXTURESPROC glad_debug_glGenTextures;
#else
void extern PFNGLGENTEXTURESPROC glad_glGenTextures;
#endif
#ifdef DEBUG
extern PFNGLISTEXTUREPROC glad_debug_glIsTexture;
#else
void extern PFNGLISTEXTUREPROC glad_glIsTexture;
#endif
#ifdef DEBUG
extern PFNGLARRAYELEMENTPROC glad_debug_glArrayElement;
#else
void extern PFNGLARRAYELEMENTPROC glad_glArrayElement;
#endif
#ifdef DEBUG
extern PFNGLCOLORPOINTERPROC glad_debug_glColorPointer;
#else
void extern PFNGLCOLORPOINTERPROC glad_glColorPointer;
#endif
#ifdef DEBUG
extern PFNGLDISABLECLIENTSTATEPROC glad_debug_glDisableClientState;
#else
void extern PFNGLDISABLECLIENTSTATEPROC glad_glDisableClientState;
#endif
#ifdef DEBUG
extern PFNGLEDGEFLAGPOINTERPROC glad_debug_glEdgeFlagPointer;
#else
void extern PFNGLEDGEFLAGPOINTERPROC glad_glEdgeFlagPointer;
#endif
#ifdef DEBUG
extern PFNGLENABLECLIENTSTATEPROC glad_debug_glEnableClientState;
#else
void extern PFNGLENABLECLIENTSTATEPROC glad_glEnableClientState;
#endif
#ifdef DEBUG
extern PFNGLINDEXPOINTERPROC glad_debug_glIndexPointer;
#else
void extern PFNGLINDEXPOINTERPROC glad_glIndexPointer;
#endif
#ifdef DEBUG
extern PFNGLINTERLEAVEDARRAYSPROC glad_debug_glInterleavedArrays;
#else
void extern PFNGLINTERLEAVEDARRAYSPROC glad_glInterleavedArrays;
#endif
#ifdef DEBUG
extern PFNGLNORMALPOINTERPROC glad_debug_glNormalPointer;
#else
void extern PFNGLNORMALPOINTERPROC glad_glNormalPointer;
#endif
#ifdef DEBUG
extern PFNGLTEXCOORDPOINTERPROC glad_debug_glTexCoordPointer;
#else
void extern PFNGLTEXCOORDPOINTERPROC glad_glTexCoordPointer;
#endif
#ifdef DEBUG
extern PFNGLVERTEXPOINTERPROC glad_debug_glVertexPointer;
#else
void extern PFNGLVERTEXPOINTERPROC glad_glVertexPointer;
#endif
#ifdef DEBUG
extern PFNGLARETEXTURESRESIDENTPROC glad_debug_glAreTexturesResident;
#else
void extern PFNGLARETEXTURESRESIDENTPROC glad_glAreTexturesResident;
#endif
#ifdef DEBUG
extern PFNGLPRIORITIZETEXTURESPROC glad_debug_glPrioritizeTextures;
#else
void extern PFNGLPRIORITIZETEXTURESPROC glad_glPrioritizeTextures;
#endif
#ifdef DEBUG
extern PFNGLINDEXUBPROC glad_debug_glIndexub;
#else
void extern PFNGLINDEXUBPROC glad_glIndexub;
#endif
#ifdef DEBUG
extern PFNGLINDEXUBVPROC glad_debug_glIndexubv;
#else
void extern PFNGLINDEXUBVPROC glad_glIndexubv;
#endif
#ifdef DEBUG
extern PFNGLPOPCLIENTATTRIBPROC glad_debug_glPopClientAttrib;
#else
void extern PFNGLPOPCLIENTATTRIBPROC glad_glPopClientAttrib;
#endif
#ifdef DEBUG
extern PFNGLPUSHCLIENTATTRIBPROC glad_debug_glPushClientAttrib;
#else
void extern PFNGLPUSHCLIENTATTRIBPROC glad_glPushClientAttrib;
#endif
#ifdef DEBUG
extern PFNGLDRAWRANGEELEMENTSPROC glad_debug_glDrawRangeElements;
#else
void extern PFNGLDRAWRANGEELEMENTSPROC glad_glDrawRangeElements;
#endif
#ifdef DEBUG
extern PFNGLTEXIMAGE3DPROC glad_debug_glTexImage3D;
#else
void extern PFNGLTEXIMAGE3DPROC glad_glTexImage3D;
#endif
#ifdef DEBUG
extern PFNGLTEXSUBIMAGE3DPROC glad_debug_glTexSubImage3D;
#else
void extern PFNGLTEXSUBIMAGE3DPROC glad_glTexSubImage3D;
#endif
#ifdef DEBUG
extern PFNGLCOPYTEXSUBIMAGE3DPROC glad_debug_glCopyTexSubImage3D;
#else
void extern PFNGLCOPYTEXSUBIMAGE3DPROC glad_glCopyTexSubImage3D;
#endif
#ifdef DEBUG
extern PFNGLACTIVETEXTUREPROC glad_debug_glActiveTexture;
#else
void extern PFNGLACTIVETEXTUREPROC glad_glActiveTexture;
#endif
#ifdef DEBUG
extern PFNGLSAMPLECOVERAGEPROC glad_debug_glSampleCoverage;
#else
void extern PFNGLSAMPLECOVERAGEPROC glad_glSampleCoverage;
#endif
#ifdef DEBUG
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC glad_debug_glCompressedTexImage3D;
#else
void extern PFNGLCOMPRESSEDTEXIMAGE3DPROC glad_glCompressedTexImage3D;
#endif
#ifdef DEBUG
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_debug_glCompressedTexImage2D;
#else
void extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_glCompressedTexImage2D;
#endif
#ifdef DEBUG
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC glad_debug_glCompressedTexImage1D;
#else
void extern PFNGLCOMPRESSEDTEXIMAGE1DPROC glad_glCompressedTexImage1D;
#endif
#ifdef DEBUG
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_debug_glCompressedTexSubImage3D;
#else
void extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_glCompressedTexSubImage3D;
#endif
#ifdef DEBUG
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_debug_glCompressedTexSubImage2D;
#else
void extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_glCompressedTexSubImage2D;
#endif
#ifdef DEBUG
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glad_debug_glCompressedTexSubImage1D;
#else
void extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glad_glCompressedTexSubImage1D;
#endif
#ifdef DEBUG
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC glad_debug_glGetCompressedTexImage;
#else
void extern PFNGLGETCOMPRESSEDTEXIMAGEPROC glad_glGetCompressedTexImage;
#endif
#ifdef DEBUG
extern PFNGLCLIENTACTIVETEXTUREPROC glad_debug_glClientActiveTexture;
#else
void extern PFNGLCLIENTACTIVETEXTUREPROC glad_glClientActiveTexture;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD1DPROC glad_debug_glMultiTexCoord1d;
#else
void extern PFNGLMULTITEXCOORD1DPROC glad_glMultiTexCoord1d;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD1DVPROC glad_debug_glMultiTexCoord1dv;
#else
void extern PFNGLMULTITEXCOORD1DVPROC glad_glMultiTexCoord1dv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD1FPROC glad_debug_glMultiTexCoord1f;
#else
void extern PFNGLMULTITEXCOORD1FPROC glad_glMultiTexCoord1f;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD1FVPROC glad_debug_glMultiTexCoord1fv;
#else
void extern PFNGLMULTITEXCOORD1FVPROC glad_glMultiTexCoord1fv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD1IPROC glad_debug_glMultiTexCoord1i;
#else
void extern PFNGLMULTITEXCOORD1IPROC glad_glMultiTexCoord1i;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD1IVPROC glad_debug_glMultiTexCoord1iv;
#else
void extern PFNGLMULTITEXCOORD1IVPROC glad_glMultiTexCoord1iv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD1SPROC glad_debug_glMultiTexCoord1s;
#else
void extern PFNGLMULTITEXCOORD1SPROC glad_glMultiTexCoord1s;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD1SVPROC glad_debug_glMultiTexCoord1sv;
#else
void extern PFNGLMULTITEXCOORD1SVPROC glad_glMultiTexCoord1sv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD2DPROC glad_debug_glMultiTexCoord2d;
#else
void extern PFNGLMULTITEXCOORD2DPROC glad_glMultiTexCoord2d;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD2DVPROC glad_debug_glMultiTexCoord2dv;
#else
void extern PFNGLMULTITEXCOORD2DVPROC glad_glMultiTexCoord2dv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD2FPROC glad_debug_glMultiTexCoord2f;
#else
void extern PFNGLMULTITEXCOORD2FPROC glad_glMultiTexCoord2f;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD2FVPROC glad_debug_glMultiTexCoord2fv;
#else
void extern PFNGLMULTITEXCOORD2FVPROC glad_glMultiTexCoord2fv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD2IPROC glad_debug_glMultiTexCoord2i;
#else
void extern PFNGLMULTITEXCOORD2IPROC glad_glMultiTexCoord2i;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD2IVPROC glad_debug_glMultiTexCoord2iv;
#else
void extern PFNGLMULTITEXCOORD2IVPROC glad_glMultiTexCoord2iv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD2SPROC glad_debug_glMultiTexCoord2s;
#else
void extern PFNGLMULTITEXCOORD2SPROC glad_glMultiTexCoord2s;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD2SVPROC glad_debug_glMultiTexCoord2sv;
#else
void extern PFNGLMULTITEXCOORD2SVPROC glad_glMultiTexCoord2sv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD3DPROC glad_debug_glMultiTexCoord3d;
#else
void extern PFNGLMULTITEXCOORD3DPROC glad_glMultiTexCoord3d;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD3DVPROC glad_debug_glMultiTexCoord3dv;
#else
void extern PFNGLMULTITEXCOORD3DVPROC glad_glMultiTexCoord3dv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD3FPROC glad_debug_glMultiTexCoord3f;
#else
void extern PFNGLMULTITEXCOORD3FPROC glad_glMultiTexCoord3f;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD3FVPROC glad_debug_glMultiTexCoord3fv;
#else
void extern PFNGLMULTITEXCOORD3FVPROC glad_glMultiTexCoord3fv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD3IPROC glad_debug_glMultiTexCoord3i;
#else
void extern PFNGLMULTITEXCOORD3IPROC glad_glMultiTexCoord3i;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD3IVPROC glad_debug_glMultiTexCoord3iv;
#else
void extern PFNGLMULTITEXCOORD3IVPROC glad_glMultiTexCoord3iv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD3SPROC glad_debug_glMultiTexCoord3s;
#else
void extern PFNGLMULTITEXCOORD3SPROC glad_glMultiTexCoord3s;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD3SVPROC glad_debug_glMultiTexCoord3sv;
#else
void extern PFNGLMULTITEXCOORD3SVPROC glad_glMultiTexCoord3sv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD4DPROC glad_debug_glMultiTexCoord4d;
#else
void extern PFNGLMULTITEXCOORD4DPROC glad_glMultiTexCoord4d;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD4DVPROC glad_debug_glMultiTexCoord4dv;
#else
void extern PFNGLMULTITEXCOORD4DVPROC glad_glMultiTexCoord4dv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD4FPROC glad_debug_glMultiTexCoord4f;
#else
void extern PFNGLMULTITEXCOORD4FPROC glad_glMultiTexCoord4f;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD4FVPROC glad_debug_glMultiTexCoord4fv;
#else
void extern PFNGLMULTITEXCOORD4FVPROC glad_glMultiTexCoord4fv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD4IPROC glad_debug_glMultiTexCoord4i;
#else
void extern PFNGLMULTITEXCOORD4IPROC glad_glMultiTexCoord4i;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD4IVPROC glad_debug_glMultiTexCoord4iv;
#else
void extern PFNGLMULTITEXCOORD4IVPROC glad_glMultiTexCoord4iv;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD4SPROC glad_debug_glMultiTexCoord4s;
#else
void extern PFNGLMULTITEXCOORD4SPROC glad_glMultiTexCoord4s;
#endif
#ifdef DEBUG
extern PFNGLMULTITEXCOORD4SVPROC glad_debug_glMultiTexCoord4sv;
#else
void extern PFNGLMULTITEXCOORD4SVPROC glad_glMultiTexCoord4sv;
#endif
#ifdef DEBUG
extern PFNGLLOADTRANSPOSEMATRIXFPROC glad_debug_glLoadTransposeMatrixf;
#else
void extern PFNGLLOADTRANSPOSEMATRIXFPROC glad_glLoadTransposeMatrixf;
#endif
#ifdef DEBUG
extern PFNGLLOADTRANSPOSEMATRIXDPROC glad_debug_glLoadTransposeMatrixd;
#else
void extern PFNGLLOADTRANSPOSEMATRIXDPROC glad_glLoadTransposeMatrixd;
#endif
#ifdef DEBUG
extern PFNGLMULTTRANSPOSEMATRIXFPROC glad_debug_glMultTransposeMatrixf;
#else
void extern PFNGLMULTTRANSPOSEMATRIXFPROC glad_glMultTransposeMatrixf;
#endif
#ifdef DEBUG
extern PFNGLMULTTRANSPOSEMATRIXDPROC glad_debug_glMultTransposeMatrixd;
#else
void extern PFNGLMULTTRANSPOSEMATRIXDPROC glad_glMultTransposeMatrixd;
#endif
#ifdef DEBUG
extern PFNGLBLENDFUNCSEPARATEPROC glad_debug_glBlendFuncSeparate;
#else
void extern PFNGLBLENDFUNCSEPARATEPROC glad_glBlendFuncSeparate;
#endif
#ifdef DEBUG
extern PFNGLMULTIDRAWARRAYSPROC glad_debug_glMultiDrawArrays;
#else
void extern PFNGLMULTIDRAWARRAYSPROC glad_glMultiDrawArrays;
#endif
#ifdef DEBUG
extern PFNGLMULTIDRAWELEMENTSPROC glad_debug_glMultiDrawElements;
#else
void extern PFNGLMULTIDRAWELEMENTSPROC glad_glMultiDrawElements;
#endif
#ifdef DEBUG
extern PFNGLPOINTPARAMETERFPROC glad_debug_glPointParameterf;
#else
void extern PFNGLPOINTPARAMETERFPROC glad_glPointParameterf;
#endif
#ifdef DEBUG
extern PFNGLPOINTPARAMETERFVPROC glad_debug_glPointParameterfv;
#else
void extern PFNGLPOINTPARAMETERFVPROC glad_glPointParameterfv;
#endif
#ifdef DEBUG
extern PFNGLPOINTPARAMETERIPROC glad_debug_glPointParameteri;
#else
void extern PFNGLPOINTPARAMETERIPROC glad_glPointParameteri;
#endif
#ifdef DEBUG
extern PFNGLPOINTPARAMETERIVPROC glad_debug_glPointParameteriv;
#else
void extern PFNGLPOINTPARAMETERIVPROC glad_glPointParameteriv;
#endif
#ifdef DEBUG
extern PFNGLFOGCOORDFPROC glad_debug_glFogCoordf;
#else
void extern PFNGLFOGCOORDFPROC glad_glFogCoordf;
#endif
#ifdef DEBUG
extern PFNGLFOGCOORDFVPROC glad_debug_glFogCoordfv;
#else
void extern PFNGLFOGCOORDFVPROC glad_glFogCoordfv;
#endif
#ifdef DEBUG
extern PFNGLFOGCOORDDPROC glad_debug_glFogCoordd;
#else
void extern PFNGLFOGCOORDDPROC glad_glFogCoordd;
#endif
#ifdef DEBUG
extern PFNGLFOGCOORDDVPROC glad_debug_glFogCoorddv;
#else
void extern PFNGLFOGCOORDDVPROC glad_glFogCoorddv;
#endif
#ifdef DEBUG
extern PFNGLFOGCOORDPOINTERPROC glad_debug_glFogCoordPointer;
#else
void extern PFNGLFOGCOORDPOINTERPROC glad_glFogCoordPointer;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3BPROC glad_debug_glSecondaryColor3b;
#else
void extern PFNGLSECONDARYCOLOR3BPROC glad_glSecondaryColor3b;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3BVPROC glad_debug_glSecondaryColor3bv;
#else
void extern PFNGLSECONDARYCOLOR3BVPROC glad_glSecondaryColor3bv;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3DPROC glad_debug_glSecondaryColor3d;
#else
void extern PFNGLSECONDARYCOLOR3DPROC glad_glSecondaryColor3d;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3DVPROC glad_debug_glSecondaryColor3dv;
#else
void extern PFNGLSECONDARYCOLOR3DVPROC glad_glSecondaryColor3dv;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3FPROC glad_debug_glSecondaryColor3f;
#else
void extern PFNGLSECONDARYCOLOR3FPROC glad_glSecondaryColor3f;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3FVPROC glad_debug_glSecondaryColor3fv;
#else
void extern PFNGLSECONDARYCOLOR3FVPROC glad_glSecondaryColor3fv;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3IPROC glad_debug_glSecondaryColor3i;
#else
void extern PFNGLSECONDARYCOLOR3IPROC glad_glSecondaryColor3i;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3IVPROC glad_debug_glSecondaryColor3iv;
#else
void extern PFNGLSECONDARYCOLOR3IVPROC glad_glSecondaryColor3iv;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3SPROC glad_debug_glSecondaryColor3s;
#else
void extern PFNGLSECONDARYCOLOR3SPROC glad_glSecondaryColor3s;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3SVPROC glad_debug_glSecondaryColor3sv;
#else
void extern PFNGLSECONDARYCOLOR3SVPROC glad_glSecondaryColor3sv;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3UBPROC glad_debug_glSecondaryColor3ub;
#else
void extern PFNGLSECONDARYCOLOR3UBPROC glad_glSecondaryColor3ub;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3UBVPROC glad_debug_glSecondaryColor3ubv;
#else
void extern PFNGLSECONDARYCOLOR3UBVPROC glad_glSecondaryColor3ubv;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3UIPROC glad_debug_glSecondaryColor3ui;
#else
void extern PFNGLSECONDARYCOLOR3UIPROC glad_glSecondaryColor3ui;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3UIVPROC glad_debug_glSecondaryColor3uiv;
#else
void extern PFNGLSECONDARYCOLOR3UIVPROC glad_glSecondaryColor3uiv;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3USPROC glad_debug_glSecondaryColor3us;
#else
void extern PFNGLSECONDARYCOLOR3USPROC glad_glSecondaryColor3us;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLOR3USVPROC glad_debug_glSecondaryColor3usv;
#else
void extern PFNGLSECONDARYCOLOR3USVPROC glad_glSecondaryColor3usv;
#endif
#ifdef DEBUG
extern PFNGLSECONDARYCOLORPOINTERPROC glad_debug_glSecondaryColorPointer;
#else
void extern PFNGLSECONDARYCOLORPOINTERPROC glad_glSecondaryColorPointer;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS2DPROC glad_debug_glWindowPos2d;
#else
void extern PFNGLWINDOWPOS2DPROC glad_glWindowPos2d;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS2DVPROC glad_debug_glWindowPos2dv;
#else
void extern PFNGLWINDOWPOS2DVPROC glad_glWindowPos2dv;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS2FPROC glad_debug_glWindowPos2f;
#else
void extern PFNGLWINDOWPOS2FPROC glad_glWindowPos2f;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS2FVPROC glad_debug_glWindowPos2fv;
#else
void extern PFNGLWINDOWPOS2FVPROC glad_glWindowPos2fv;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS2IPROC glad_debug_glWindowPos2i;
#else
void extern PFNGLWINDOWPOS2IPROC glad_glWindowPos2i;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS2IVPROC glad_debug_glWindowPos2iv;
#else
void extern PFNGLWINDOWPOS2IVPROC glad_glWindowPos2iv;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS2SPROC glad_debug_glWindowPos2s;
#else
void extern PFNGLWINDOWPOS2SPROC glad_glWindowPos2s;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS2SVPROC glad_debug_glWindowPos2sv;
#else
void extern PFNGLWINDOWPOS2SVPROC glad_glWindowPos2sv;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS3DPROC glad_debug_glWindowPos3d;
#else
void extern PFNGLWINDOWPOS3DPROC glad_glWindowPos3d;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS3DVPROC glad_debug_glWindowPos3dv;
#else
void extern PFNGLWINDOWPOS3DVPROC glad_glWindowPos3dv;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS3FPROC glad_debug_glWindowPos3f;
#else
void extern PFNGLWINDOWPOS3FPROC glad_glWindowPos3f;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS3FVPROC glad_debug_glWindowPos3fv;
#else
void extern PFNGLWINDOWPOS3FVPROC glad_glWindowPos3fv;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS3IPROC glad_debug_glWindowPos3i;
#else
void extern PFNGLWINDOWPOS3IPROC glad_glWindowPos3i;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS3IVPROC glad_debug_glWindowPos3iv;
#else
void extern PFNGLWINDOWPOS3IVPROC glad_glWindowPos3iv;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS3SPROC glad_debug_glWindowPos3s;
#else
void extern PFNGLWINDOWPOS3SPROC glad_glWindowPos3s;
#endif
#ifdef DEBUG
extern PFNGLWINDOWPOS3SVPROC glad_debug_glWindowPos3sv;
#else
void extern PFNGLWINDOWPOS3SVPROC glad_glWindowPos3sv;
#endif
#ifdef DEBUG
extern PFNGLBLENDCOLORPROC glad_debug_glBlendColor;
#else
void extern PFNGLBLENDCOLORPROC glad_glBlendColor;
#endif
#ifdef DEBUG
extern PFNGLBLENDEQUATIONPROC glad_debug_glBlendEquation;
#else
void extern PFNGLBLENDEQUATIONPROC glad_glBlendEquation;
#endif
#ifdef DEBUG
extern PFNGLGENQUERIESPROC glad_debug_glGenQueries;
#else
void extern PFNGLGENQUERIESPROC glad_glGenQueries;
#endif
#ifdef DEBUG
extern PFNGLDELETEQUERIESPROC glad_debug_glDeleteQueries;
#else
void extern PFNGLDELETEQUERIESPROC glad_glDeleteQueries;
#endif
#ifdef DEBUG
extern PFNGLISQUERYPROC glad_debug_glIsQuery;
#else
void extern PFNGLISQUERYPROC glad_glIsQuery;
#endif
#ifdef DEBUG
extern PFNGLBEGINQUERYPROC glad_debug_glBeginQuery;
#else
void extern PFNGLBEGINQUERYPROC glad_glBeginQuery;
#endif
#ifdef DEBUG
extern PFNGLENDQUERYPROC glad_debug_glEndQuery;
#else
void extern PFNGLENDQUERYPROC glad_glEndQuery;
#endif
#ifdef DEBUG
extern PFNGLGETQUERYIVPROC glad_debug_glGetQueryiv;
#else
void extern PFNGLGETQUERYIVPROC glad_glGetQueryiv;
#endif
#ifdef DEBUG
extern PFNGLGETQUERYOBJECTIVPROC glad_debug_glGetQueryObjectiv;
#else
void extern PFNGLGETQUERYOBJECTIVPROC glad_glGetQueryObjectiv;
#endif
#ifdef DEBUG
extern PFNGLGETQUERYOBJECTUIVPROC glad_debug_glGetQueryObjectuiv;
#else
void extern PFNGLGETQUERYOBJECTUIVPROC glad_glGetQueryObjectuiv;
#endif
#ifdef DEBUG
extern PFNGLBINDBUFFERPROC glad_debug_glBindBuffer;
#else
void extern PFNGLBINDBUFFERPROC glad_glBindBuffer;
#endif
#ifdef DEBUG
extern PFNGLDELETEBUFFERSPROC glad_debug_glDeleteBuffers;
#else
void extern PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers;
#endif
#ifdef DEBUG
extern PFNGLGENBUFFERSPROC glad_debug_glGenBuffers;
#else
void extern PFNGLGENBUFFERSPROC glad_glGenBuffers;
#endif
#ifdef DEBUG
extern PFNGLISBUFFERPROC glad_debug_glIsBuffer;
#else
void extern PFNGLISBUFFERPROC glad_glIsBuffer;
#endif
#ifdef DEBUG
extern PFNGLBUFFERDATAPROC glad_debug_glBufferData;
#else
void extern PFNGLBUFFERDATAPROC glad_glBufferData;
#endif
#ifdef DEBUG
extern PFNGLBUFFERSUBDATAPROC glad_debug_glBufferSubData;
#else
void extern PFNGLBUFFERSUBDATAPROC glad_glBufferSubData;
#endif
#ifdef DEBUG
extern PFNGLGETBUFFERSUBDATAPROC glad_debug_glGetBufferSubData;
#else
void extern PFNGLGETBUFFERSUBDATAPROC glad_glGetBufferSubData;
#endif
#ifdef DEBUG
extern PFNGLMAPBUFFERPROC glad_debug_glMapBuffer;
#else
void extern PFNGLMAPBUFFERPROC glad_glMapBuffer;
#endif
#ifdef DEBUG
extern PFNGLUNMAPBUFFERPROC glad_debug_glUnmapBuffer;
#else
void extern PFNGLUNMAPBUFFERPROC glad_glUnmapBuffer;
#endif
#ifdef DEBUG
extern PFNGLGETBUFFERPARAMETERIVPROC glad_debug_glGetBufferParameteriv;
#else
void extern PFNGLGETBUFFERPARAMETERIVPROC glad_glGetBufferParameteriv;
#endif
#ifdef DEBUG
extern PFNGLGETBUFFERPOINTERVPROC glad_debug_glGetBufferPointerv;
#else
void extern PFNGLGETBUFFERPOINTERVPROC glad_glGetBufferPointerv;
#endif
#ifdef DEBUG
extern PFNGLBLENDEQUATIONSEPARATEPROC glad_debug_glBlendEquationSeparate;
#else
void extern PFNGLBLENDEQUATIONSEPARATEPROC glad_glBlendEquationSeparate;
#endif
#ifdef DEBUG
extern PFNGLDRAWBUFFERSPROC glad_debug_glDrawBuffers;
#else
void extern PFNGLDRAWBUFFERSPROC glad_glDrawBuffers;
#endif
#ifdef DEBUG
extern PFNGLSTENCILOPSEPARATEPROC glad_debug_glStencilOpSeparate;
#else
void extern PFNGLSTENCILOPSEPARATEPROC glad_glStencilOpSeparate;
#endif
#ifdef DEBUG
extern PFNGLSTENCILFUNCSEPARATEPROC glad_debug_glStencilFuncSeparate;
#else
void extern PFNGLSTENCILFUNCSEPARATEPROC glad_glStencilFuncSeparate;
#endif
#ifdef DEBUG
extern PFNGLSTENCILMASKSEPARATEPROC glad_debug_glStencilMaskSeparate;
#else
void extern PFNGLSTENCILMASKSEPARATEPROC glad_glStencilMaskSeparate;
#endif
#ifdef DEBUG
extern PFNGLATTACHSHADERPROC glad_debug_glAttachShader;
#else
void extern PFNGLATTACHSHADERPROC glad_glAttachShader;
#endif
#ifdef DEBUG
extern PFNGLBINDATTRIBLOCATIONPROC glad_debug_glBindAttribLocation;
#else
void extern PFNGLBINDATTRIBLOCATIONPROC glad_glBindAttribLocation;
#endif
#ifdef DEBUG
extern PFNGLCOMPILESHADERPROC glad_debug_glCompileShader;
#else
void extern PFNGLCOMPILESHADERPROC glad_glCompileShader;
#endif
#ifdef DEBUG
extern PFNGLCREATEPROGRAMPROC glad_debug_glCreateProgram;
#else
void extern PFNGLCREATEPROGRAMPROC glad_glCreateProgram;
#endif
#ifdef DEBUG
extern PFNGLCREATESHADERPROC glad_debug_glCreateShader;
#else
void extern PFNGLCREATESHADERPROC glad_glCreateShader;
#endif
#ifdef DEBUG
extern PFNGLDELETEPROGRAMPROC glad_debug_glDeleteProgram;
#else
void extern PFNGLDELETEPROGRAMPROC glad_glDeleteProgram;
#endif
#ifdef DEBUG
extern PFNGLDELETESHADERPROC glad_debug_glDeleteShader;
#else
void extern PFNGLDELETESHADERPROC glad_glDeleteShader;
#endif
#ifdef DEBUG
extern PFNGLDETACHSHADERPROC glad_debug_glDetachShader;
#else
void extern PFNGLDETACHSHADERPROC glad_glDetachShader;
#endif
#ifdef DEBUG
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_debug_glDisableVertexAttribArray;
#else
void extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray;
#endif
#ifdef DEBUG
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glad_debug_glEnableVertexAttribArray;
#else
void extern PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray;
#endif
#ifdef DEBUG
extern PFNGLGETACTIVEATTRIBPROC glad_debug_glGetActiveAttrib;
#else
void extern PFNGLGETACTIVEATTRIBPROC glad_glGetActiveAttrib;
#endif
#ifdef DEBUG
extern PFNGLGETACTIVEUNIFORMPROC glad_debug_glGetActiveUniform;
#else
void extern PFNGLGETACTIVEUNIFORMPROC glad_glGetActiveUniform;
#endif
#ifdef DEBUG
extern PFNGLGETATTACHEDSHADERSPROC glad_debug_glGetAttachedShaders;
#else
void extern PFNGLGETATTACHEDSHADERSPROC glad_glGetAttachedShaders;
#endif
#ifdef DEBUG
extern PFNGLGETATTRIBLOCATIONPROC glad_debug_glGetAttribLocation;
#else
void extern PFNGLGETATTRIBLOCATIONPROC glad_glGetAttribLocation;
#endif
#ifdef DEBUG
extern PFNGLGETPROGRAMIVPROC glad_debug_glGetProgramiv;
#else
void extern PFNGLGETPROGRAMIVPROC glad_glGetProgramiv;
#endif
#ifdef DEBUG
extern PFNGLGETPROGRAMINFOLOGPROC glad_debug_glGetProgramInfoLog;
#else
void extern PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog;
#endif
#ifdef DEBUG
extern PFNGLGETSHADERIVPROC glad_debug_glGetShaderiv;
#else
void extern PFNGLGETSHADERIVPROC glad_glGetShaderiv;
#endif
#ifdef DEBUG
extern PFNGLGETSHADERINFOLOGPROC glad_debug_glGetShaderInfoLog;
#else
void extern PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog;
#endif
#ifdef DEBUG
extern PFNGLGETSHADERSOURCEPROC glad_debug_glGetShaderSource;
#else
void extern PFNGLGETSHADERSOURCEPROC glad_glGetShaderSource;
#endif
#ifdef DEBUG
extern PFNGLGETUNIFORMLOCATIONPROC glad_debug_glGetUniformLocation;
#else
void extern PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation;
#endif
#ifdef DEBUG
extern PFNGLGETUNIFORMFVPROC glad_debug_glGetUniformfv;
#else
void extern PFNGLGETUNIFORMFVPROC glad_glGetUniformfv;
#endif
#ifdef DEBUG
extern PFNGLGETUNIFORMIVPROC glad_debug_glGetUniformiv;
#else
void extern PFNGLGETUNIFORMIVPROC glad_glGetUniformiv;
#endif
#ifdef DEBUG
extern PFNGLGETVERTEXATTRIBDVPROC glad_debug_glGetVertexAttribdv;
#else
void extern PFNGLGETVERTEXATTRIBDVPROC glad_glGetVertexAttribdv;
#endif
#ifdef DEBUG
extern PFNGLGETVERTEXATTRIBFVPROC glad_debug_glGetVertexAttribfv;
#else
void extern PFNGLGETVERTEXATTRIBFVPROC glad_glGetVertexAttribfv;
#endif
#ifdef DEBUG
extern PFNGLGETVERTEXATTRIBIVPROC glad_debug_glGetVertexAttribiv;
#else
void extern PFNGLGETVERTEXATTRIBIVPROC glad_glGetVertexAttribiv;
#endif
#ifdef DEBUG
extern PFNGLGETVERTEXATTRIBPOINTERVPROC glad_debug_glGetVertexAttribPointerv;
#else
void extern PFNGLGETVERTEXATTRIBPOINTERVPROC glad_glGetVertexAttribPointerv;
#endif
#ifdef DEBUG
extern PFNGLISPROGRAMPROC glad_debug_glIsProgram;
#else
void extern PFNGLISPROGRAMPROC glad_glIsProgram;
#endif
#ifdef DEBUG
extern PFNGLISSHADERPROC glad_debug_glIsShader;
#else
void extern PFNGLISSHADERPROC glad_glIsShader;
#endif
#ifdef DEBUG
extern PFNGLLINKPROGRAMPROC glad_debug_glLinkProgram;
#else
void extern PFNGLLINKPROGRAMPROC glad_glLinkProgram;
#endif
#ifdef DEBUG
extern PFNGLSHADERSOURCEPROC glad_debug_glShaderSource;
#else
void extern PFNGLSHADERSOURCEPROC glad_glShaderSource;
#endif
#ifdef DEBUG
extern PFNGLUSEPROGRAMPROC glad_debug_glUseProgram;
#else
void extern PFNGLUSEPROGRAMPROC glad_glUseProgram;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM1FPROC glad_debug_glUniform1f;
#else
void extern PFNGLUNIFORM1FPROC glad_glUniform1f;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM2FPROC glad_debug_glUniform2f;
#else
void extern PFNGLUNIFORM2FPROC glad_glUniform2f;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM3FPROC glad_debug_glUniform3f;
#else
void extern PFNGLUNIFORM3FPROC glad_glUniform3f;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM4FPROC glad_debug_glUniform4f;
#else
void extern PFNGLUNIFORM4FPROC glad_glUniform4f;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM1IPROC glad_debug_glUniform1i;
#else
void extern PFNGLUNIFORM1IPROC glad_glUniform1i;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM2IPROC glad_debug_glUniform2i;
#else
void extern PFNGLUNIFORM2IPROC glad_glUniform2i;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM3IPROC glad_debug_glUniform3i;
#else
void extern PFNGLUNIFORM3IPROC glad_glUniform3i;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM4IPROC glad_debug_glUniform4i;
#else
void extern PFNGLUNIFORM4IPROC glad_glUniform4i;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM1FVPROC glad_debug_glUniform1fv;
#else
void extern PFNGLUNIFORM1FVPROC glad_glUniform1fv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM2FVPROC glad_debug_glUniform2fv;
#else
void extern PFNGLUNIFORM2FVPROC glad_glUniform2fv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM3FVPROC glad_debug_glUniform3fv;
#else
void extern PFNGLUNIFORM3FVPROC glad_glUniform3fv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM4FVPROC glad_debug_glUniform4fv;
#else
void extern PFNGLUNIFORM4FVPROC glad_glUniform4fv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM1IVPROC glad_debug_glUniform1iv;
#else
void extern PFNGLUNIFORM1IVPROC glad_glUniform1iv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM2IVPROC glad_debug_glUniform2iv;
#else
void extern PFNGLUNIFORM2IVPROC glad_glUniform2iv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM3IVPROC glad_debug_glUniform3iv;
#else
void extern PFNGLUNIFORM3IVPROC glad_glUniform3iv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORM4IVPROC glad_debug_glUniform4iv;
#else
void extern PFNGLUNIFORM4IVPROC glad_glUniform4iv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORMMATRIX2FVPROC glad_debug_glUniformMatrix2fv;
#else
void extern PFNGLUNIFORMMATRIX2FVPROC glad_glUniformMatrix2fv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORMMATRIX3FVPROC glad_debug_glUniformMatrix3fv;
#else
void extern PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv;
#endif
#ifdef DEBUG
extern PFNGLUNIFORMMATRIX4FVPROC glad_debug_glUniformMatrix4fv;
#else
void extern PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv;
#endif
#ifdef DEBUG
extern PFNGLVALIDATEPROGRAMPROC glad_debug_glValidateProgram;
#else
void extern PFNGLVALIDATEPROGRAMPROC glad_glValidateProgram;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB1DPROC glad_debug_glVertexAttrib1d;
#else
void extern PFNGLVERTEXATTRIB1DPROC glad_glVertexAttrib1d;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB1DVPROC glad_debug_glVertexAttrib1dv;
#else
void extern PFNGLVERTEXATTRIB1DVPROC glad_glVertexAttrib1dv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB1FPROC glad_debug_glVertexAttrib1f;
#else
void extern PFNGLVERTEXATTRIB1FPROC glad_glVertexAttrib1f;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB1FVPROC glad_debug_glVertexAttrib1fv;
#else
void extern PFNGLVERTEXATTRIB1FVPROC glad_glVertexAttrib1fv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB1SPROC glad_debug_glVertexAttrib1s;
#else
void extern PFNGLVERTEXATTRIB1SPROC glad_glVertexAttrib1s;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB1SVPROC glad_debug_glVertexAttrib1sv;
#else
void extern PFNGLVERTEXATTRIB1SVPROC glad_glVertexAttrib1sv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB2DPROC glad_debug_glVertexAttrib2d;
#else
void extern PFNGLVERTEXATTRIB2DPROC glad_glVertexAttrib2d;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB2DVPROC glad_debug_glVertexAttrib2dv;
#else
void extern PFNGLVERTEXATTRIB2DVPROC glad_glVertexAttrib2dv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB2FPROC glad_debug_glVertexAttrib2f;
#else
void extern PFNGLVERTEXATTRIB2FPROC glad_glVertexAttrib2f;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB2FVPROC glad_debug_glVertexAttrib2fv;
#else
void extern PFNGLVERTEXATTRIB2FVPROC glad_glVertexAttrib2fv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB2SPROC glad_debug_glVertexAttrib2s;
#else
void extern PFNGLVERTEXATTRIB2SPROC glad_glVertexAttrib2s;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB2SVPROC glad_debug_glVertexAttrib2sv;
#else
void extern PFNGLVERTEXATTRIB2SVPROC glad_glVertexAttrib2sv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB3DPROC glad_debug_glVertexAttrib3d;
#else
void extern PFNGLVERTEXATTRIB3DPROC glad_glVertexAttrib3d;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB3DVPROC glad_debug_glVertexAttrib3dv;
#else
void extern PFNGLVERTEXATTRIB3DVPROC glad_glVertexAttrib3dv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB3FPROC glad_debug_glVertexAttrib3f;
#else
void extern PFNGLVERTEXATTRIB3FPROC glad_glVertexAttrib3f;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB3FVPROC glad_debug_glVertexAttrib3fv;
#else
void extern PFNGLVERTEXATTRIB3FVPROC glad_glVertexAttrib3fv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB3SPROC glad_debug_glVertexAttrib3s;
#else
void extern PFNGLVERTEXATTRIB3SPROC glad_glVertexAttrib3s;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB3SVPROC glad_debug_glVertexAttrib3sv;
#else
void extern PFNGLVERTEXATTRIB3SVPROC glad_glVertexAttrib3sv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4NBVPROC glad_debug_glVertexAttrib4Nbv;
#else
void extern PFNGLVERTEXATTRIB4NBVPROC glad_glVertexAttrib4Nbv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4NIVPROC glad_debug_glVertexAttrib4Niv;
#else
void extern PFNGLVERTEXATTRIB4NIVPROC glad_glVertexAttrib4Niv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4NSVPROC glad_debug_glVertexAttrib4Nsv;
#else
void extern PFNGLVERTEXATTRIB4NSVPROC glad_glVertexAttrib4Nsv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4NUBPROC glad_debug_glVertexAttrib4Nub;
#else
void extern PFNGLVERTEXATTRIB4NUBPROC glad_glVertexAttrib4Nub;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4NUBVPROC glad_debug_glVertexAttrib4Nubv;
#else
void extern PFNGLVERTEXATTRIB4NUBVPROC glad_glVertexAttrib4Nubv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4NUIVPROC glad_debug_glVertexAttrib4Nuiv;
#else
void extern PFNGLVERTEXATTRIB4NUIVPROC glad_glVertexAttrib4Nuiv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4NUSVPROC glad_debug_glVertexAttrib4Nusv;
#else
void extern PFNGLVERTEXATTRIB4NUSVPROC glad_glVertexAttrib4Nusv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4BVPROC glad_debug_glVertexAttrib4bv;
#else
void extern PFNGLVERTEXATTRIB4BVPROC glad_glVertexAttrib4bv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4DPROC glad_debug_glVertexAttrib4d;
#else
void extern PFNGLVERTEXATTRIB4DPROC glad_glVertexAttrib4d;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4DVPROC glad_debug_glVertexAttrib4dv;
#else
void extern PFNGLVERTEXATTRIB4DVPROC glad_glVertexAttrib4dv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4FPROC glad_debug_glVertexAttrib4f;
#else
void extern PFNGLVERTEXATTRIB4FPROC glad_glVertexAttrib4f;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4FVPROC glad_debug_glVertexAttrib4fv;
#else
void extern PFNGLVERTEXATTRIB4FVPROC glad_glVertexAttrib4fv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4IVPROC glad_debug_glVertexAttrib4iv;
#else
void extern PFNGLVERTEXATTRIB4IVPROC glad_glVertexAttrib4iv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4SPROC glad_debug_glVertexAttrib4s;
#else
void extern PFNGLVERTEXATTRIB4SPROC glad_glVertexAttrib4s;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4SVPROC glad_debug_glVertexAttrib4sv;
#else
void extern PFNGLVERTEXATTRIB4SVPROC glad_glVertexAttrib4sv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4UBVPROC glad_debug_glVertexAttrib4ubv;
#else
void extern PFNGLVERTEXATTRIB4UBVPROC glad_glVertexAttrib4ubv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4UIVPROC glad_debug_glVertexAttrib4uiv;
#else
void extern PFNGLVERTEXATTRIB4UIVPROC glad_glVertexAttrib4uiv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIB4USVPROC glad_debug_glVertexAttrib4usv;
#else
void extern PFNGLVERTEXATTRIB4USVPROC glad_glVertexAttrib4usv;
#endif
#ifdef DEBUG
extern PFNGLVERTEXATTRIBPOINTERPROC glad_debug_glVertexAttribPointer;
#else
void extern PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer;
#endif
#ifdef DEBUG
extern PFNGLBINDBUFFERARBPROC glad_debug_glBindBufferARB;
#else
void extern PFNGLBINDBUFFERARBPROC glad_glBindBufferARB;
#endif
#ifdef DEBUG
extern PFNGLDELETEBUFFERSARBPROC glad_debug_glDeleteBuffersARB;
#else
void extern PFNGLDELETEBUFFERSARBPROC glad_glDeleteBuffersARB;
#endif
#ifdef DEBUG
extern PFNGLGENBUFFERSARBPROC glad_debug_glGenBuffersARB;
#else
void extern PFNGLGENBUFFERSARBPROC glad_glGenBuffersARB;
#endif
#ifdef DEBUG
extern PFNGLISBUFFERARBPROC glad_debug_glIsBufferARB;
#else
void extern PFNGLISBUFFERARBPROC glad_glIsBufferARB;
#endif
#ifdef DEBUG
extern PFNGLBUFFERDATAARBPROC glad_debug_glBufferDataARB;
#else
void extern PFNGLBUFFERDATAARBPROC glad_glBufferDataARB;
#endif
#ifdef DEBUG
extern PFNGLBUFFERSUBDATAARBPROC glad_debug_glBufferSubDataARB;
#else
void extern PFNGLBUFFERSUBDATAARBPROC glad_glBufferSubDataARB;
#endif
#ifdef DEBUG
extern PFNGLGETBUFFERSUBDATAARBPROC glad_debug_glGetBufferSubDataARB;
#else
void extern PFNGLGETBUFFERSUBDATAARBPROC glad_glGetBufferSubDataARB;
#endif
#ifdef DEBUG
extern PFNGLMAPBUFFERARBPROC glad_debug_glMapBufferARB;
#else
void extern PFNGLMAPBUFFERARBPROC glad_glMapBufferARB;
#endif
#ifdef DEBUG
extern PFNGLUNMAPBUFFERARBPROC glad_debug_glUnmapBufferARB;
#else
void extern PFNGLUNMAPBUFFERARBPROC glad_glUnmapBufferARB;
#endif
#ifdef DEBUG
extern PFNGLGETBUFFERPARAMETERIVARBPROC glad_debug_glGetBufferParameterivARB;
#else
void extern PFNGLGETBUFFERPARAMETERIVARBPROC glad_glGetBufferParameterivARB;
#endif
#ifdef DEBUG
extern PFNGLGETBUFFERPOINTERVARBPROC glad_debug_glGetBufferPointervARB;
#else
void extern PFNGLGETBUFFERPOINTERVARBPROC glad_glGetBufferPointervARB;
#endif
#ifdef DEBUG
extern PFNGLBINDVERTEXARRAYPROC glad_debug_glBindVertexArray;
#else
void extern PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray;
#endif
#ifdef DEBUG
extern PFNGLDELETEVERTEXARRAYSPROC glad_debug_glDeleteVertexArrays;
#else
void extern PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays;
#endif
#ifdef DEBUG
extern PFNGLGENVERTEXARRAYSPROC glad_debug_glGenVertexArrays;
#else
void extern PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays;
#endif
#ifdef DEBUG
extern PFNGLISVERTEXARRAYPROC glad_debug_glIsVertexArray;
#else
void extern PFNGLISVERTEXARRAYPROC glad_glIsVertexArray;
#endif
#ifdef DEBUG
extern PFNGLISRENDERBUFFERPROC glad_debug_glIsRenderbuffer;
#else
void extern PFNGLISRENDERBUFFERPROC glad_glIsRenderbuffer;
#endif
#ifdef DEBUG
extern PFNGLBINDRENDERBUFFERPROC glad_debug_glBindRenderbuffer;
#else
void extern PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer;
#endif
#ifdef DEBUG
extern PFNGLDELETERENDERBUFFERSPROC glad_debug_glDeleteRenderbuffers;
#else
void extern PFNGLDELETERENDERBUFFERSPROC glad_glDeleteRenderbuffers;
#endif
#ifdef DEBUG
extern PFNGLGENRENDERBUFFERSPROC glad_debug_glGenRenderbuffers;
#else
void extern PFNGLGENRENDERBUFFERSPROC glad_glGenRenderbuffers;
#endif
#ifdef DEBUG
extern PFNGLRENDERBUFFERSTORAGEPROC glad_debug_glRenderbufferStorage;
#else
void extern PFNGLRENDERBUFFERSTORAGEPROC glad_glRenderbufferStorage;
#endif
#ifdef DEBUG
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_debug_glGetRenderbufferParameteriv;
#else
void extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_glGetRenderbufferParameteriv;
#endif
#ifdef DEBUG
extern PFNGLISFRAMEBUFFERPROC glad_debug_glIsFramebuffer;
#else
void extern PFNGLISFRAMEBUFFERPROC glad_glIsFramebuffer;
#endif
#ifdef DEBUG
extern PFNGLBINDFRAMEBUFFERPROC glad_debug_glBindFramebuffer;
#else
void extern PFNGLBINDFRAMEBUFFERPROC glad_glBindFramebuffer;
#endif
#ifdef DEBUG
extern PFNGLDELETEFRAMEBUFFERSPROC glad_debug_glDeleteFramebuffers;
#else
void extern PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers;
#endif
#ifdef DEBUG
extern PFNGLGENFRAMEBUFFERSPROC glad_debug_glGenFramebuffers;
#else
void extern PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers;
#endif
#ifdef DEBUG
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_debug_glCheckFramebufferStatus;
#else
void extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERTEXTURE1DPROC glad_debug_glFramebufferTexture1D;
#else
void extern PFNGLFRAMEBUFFERTEXTURE1DPROC glad_glFramebufferTexture1D;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glad_debug_glFramebufferTexture2D;
#else
void extern PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERTEXTURE3DPROC glad_debug_glFramebufferTexture3D;
#else
void extern PFNGLFRAMEBUFFERTEXTURE3DPROC glad_glFramebufferTexture3D;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_debug_glFramebufferRenderbuffer;
#else
void extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer;
#endif
#ifdef DEBUG
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_debug_glGetFramebufferAttachmentParameteriv;
#else
void extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_glGetFramebufferAttachmentParameteriv;
#endif
#ifdef DEBUG
extern PFNGLGENERATEMIPMAPPROC glad_debug_glGenerateMipmap;
#else
void extern PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap;
#endif
#ifdef DEBUG
extern PFNGLBLITFRAMEBUFFERPROC glad_debug_glBlitFramebuffer;
#else
void extern PFNGLBLITFRAMEBUFFERPROC glad_glBlitFramebuffer;
#endif
#ifdef DEBUG
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_debug_glRenderbufferStorageMultisample;
#else
void extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_glRenderbufferStorageMultisample;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glad_debug_glFramebufferTextureLayer;
#else
void extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glad_glFramebufferTextureLayer;
#endif
#ifdef DEBUG
extern PFNGLISRENDERBUFFEREXTPROC glad_debug_glIsRenderbufferEXT;
#else
void extern PFNGLISRENDERBUFFEREXTPROC glad_glIsRenderbufferEXT;
#endif
#ifdef DEBUG
extern PFNGLBINDRENDERBUFFEREXTPROC glad_debug_glBindRenderbufferEXT;
#else
void extern PFNGLBINDRENDERBUFFEREXTPROC glad_glBindRenderbufferEXT;
#endif
#ifdef DEBUG
extern PFNGLDELETERENDERBUFFERSEXTPROC glad_debug_glDeleteRenderbuffersEXT;
#else
void extern PFNGLDELETERENDERBUFFERSEXTPROC glad_glDeleteRenderbuffersEXT;
#endif
#ifdef DEBUG
extern PFNGLGENRENDERBUFFERSEXTPROC glad_debug_glGenRenderbuffersEXT;
#else
void extern PFNGLGENRENDERBUFFERSEXTPROC glad_glGenRenderbuffersEXT;
#endif
#ifdef DEBUG
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glad_debug_glRenderbufferStorageEXT;
#else
void extern PFNGLRENDERBUFFERSTORAGEEXTPROC glad_glRenderbufferStorageEXT;
#endif
#ifdef DEBUG
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glad_debug_glGetRenderbufferParameterivEXT;
#else
void extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glad_glGetRenderbufferParameterivEXT;
#endif
#ifdef DEBUG
extern PFNGLISFRAMEBUFFEREXTPROC glad_debug_glIsFramebufferEXT;
#else
void extern PFNGLISFRAMEBUFFEREXTPROC glad_glIsFramebufferEXT;
#endif
#ifdef DEBUG
extern PFNGLBINDFRAMEBUFFEREXTPROC glad_debug_glBindFramebufferEXT;
#else
void extern PFNGLBINDFRAMEBUFFEREXTPROC glad_glBindFramebufferEXT;
#endif
#ifdef DEBUG
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glad_debug_glDeleteFramebuffersEXT;
#else
void extern PFNGLDELETEFRAMEBUFFERSEXTPROC glad_glDeleteFramebuffersEXT;
#endif
#ifdef DEBUG
extern PFNGLGENFRAMEBUFFERSEXTPROC glad_debug_glGenFramebuffersEXT;
#else
void extern PFNGLGENFRAMEBUFFERSEXTPROC glad_glGenFramebuffersEXT;
#endif
#ifdef DEBUG
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glad_debug_glCheckFramebufferStatusEXT;
#else
void extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glad_glCheckFramebufferStatusEXT;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glad_debug_glFramebufferTexture1DEXT;
#else
void extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glad_glFramebufferTexture1DEXT;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glad_debug_glFramebufferTexture2DEXT;
#else
void extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glad_glFramebufferTexture2DEXT;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glad_debug_glFramebufferTexture3DEXT;
#else
void extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glad_glFramebufferTexture3DEXT;
#endif
#ifdef DEBUG
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glad_debug_glFramebufferRenderbufferEXT;
#else
void extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glad_glFramebufferRenderbufferEXT;
#endif
#ifdef DEBUG
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_debug_glGetFramebufferAttachmentParameterivEXT;
#else
void extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_glGetFramebufferAttachmentParameterivEXT;
#endif
#ifdef DEBUG
extern PFNGLGENERATEMIPMAPEXTPROC glad_debug_glGenerateMipmapEXT;
#else
void extern PFNGLGENERATEMIPMAPEXTPROC glad_glGenerateMipmapEXT;
#endif
#ifdef DEBUG
extern PFNGLBINDVERTEXARRAYAPPLEPROC glad_debug_glBindVertexArrayAPPLE;
#else
void extern PFNGLBINDVERTEXARRAYAPPLEPROC glad_glBindVertexArrayAPPLE;
#endif
#ifdef DEBUG
extern PFNGLDELETEVERTEXARRAYSAPPLEPROC glad_debug_glDeleteVertexArraysAPPLE;
#else
void extern PFNGLDELETEVERTEXARRAYSAPPLEPROC glad_glDeleteVertexArraysAPPLE;
#endif
#ifdef DEBUG
extern PFNGLGENVERTEXARRAYSAPPLEPROC glad_debug_glGenVertexArraysAPPLE;
#else
void extern PFNGLGENVERTEXARRAYSAPPLEPROC glad_glGenVertexArraysAPPLE;
#endif
#ifdef DEBUG
extern PFNGLISVERTEXARRAYAPPLEPROC glad_debug_glIsVertexArrayAPPLE;
#else
void extern PFNGLISVERTEXARRAYAPPLEPROC glad_glIsVertexArrayAPPLE;
#endif
} // extern C


namespace Natron {

template <>
void OSGLFunctions<true>::load_functions() {
#ifdef DEBUG
    _mglCullFace = glad_debug_glCullFace;
#else
    _mglCullFace = glad_glCullFace;
#endif
#ifdef DEBUG
    _mglFrontFace = glad_debug_glFrontFace;
#else
    _mglFrontFace = glad_glFrontFace;
#endif
#ifdef DEBUG
    _mglHint = glad_debug_glHint;
#else
    _mglHint = glad_glHint;
#endif
#ifdef DEBUG
    _mglLineWidth = glad_debug_glLineWidth;
#else
    _mglLineWidth = glad_glLineWidth;
#endif
#ifdef DEBUG
    _mglPointSize = glad_debug_glPointSize;
#else
    _mglPointSize = glad_glPointSize;
#endif
#ifdef DEBUG
    _mglPolygonMode = glad_debug_glPolygonMode;
#else
    _mglPolygonMode = glad_glPolygonMode;
#endif
#ifdef DEBUG
    _mglScissor = glad_debug_glScissor;
#else
    _mglScissor = glad_glScissor;
#endif
#ifdef DEBUG
    _mglTexParameterf = glad_debug_glTexParameterf;
#else
    _mglTexParameterf = glad_glTexParameterf;
#endif
#ifdef DEBUG
    _mglTexParameterfv = glad_debug_glTexParameterfv;
#else
    _mglTexParameterfv = glad_glTexParameterfv;
#endif
#ifdef DEBUG
    _mglTexParameteri = glad_debug_glTexParameteri;
#else
    _mglTexParameteri = glad_glTexParameteri;
#endif
#ifdef DEBUG
    _mglTexParameteriv = glad_debug_glTexParameteriv;
#else
    _mglTexParameteriv = glad_glTexParameteriv;
#endif
#ifdef DEBUG
    _mglTexImage1D = glad_debug_glTexImage1D;
#else
    _mglTexImage1D = glad_glTexImage1D;
#endif
#ifdef DEBUG
    _mglTexImage2D = glad_debug_glTexImage2D;
#else
    _mglTexImage2D = glad_glTexImage2D;
#endif
#ifdef DEBUG
    _mglDrawBuffer = glad_debug_glDrawBuffer;
#else
    _mglDrawBuffer = glad_glDrawBuffer;
#endif
#ifdef DEBUG
    _mglClear = glad_debug_glClear;
#else
    _mglClear = glad_glClear;
#endif
#ifdef DEBUG
    _mglClearColor = glad_debug_glClearColor;
#else
    _mglClearColor = glad_glClearColor;
#endif
#ifdef DEBUG
    _mglClearStencil = glad_debug_glClearStencil;
#else
    _mglClearStencil = glad_glClearStencil;
#endif
#ifdef DEBUG
    _mglClearDepth = glad_debug_glClearDepth;
#else
    _mglClearDepth = glad_glClearDepth;
#endif
#ifdef DEBUG
    _mglStencilMask = glad_debug_glStencilMask;
#else
    _mglStencilMask = glad_glStencilMask;
#endif
#ifdef DEBUG
    _mglColorMask = glad_debug_glColorMask;
#else
    _mglColorMask = glad_glColorMask;
#endif
#ifdef DEBUG
    _mglDepthMask = glad_debug_glDepthMask;
#else
    _mglDepthMask = glad_glDepthMask;
#endif
#ifdef DEBUG
    _mglDisable = glad_debug_glDisable;
#else
    _mglDisable = glad_glDisable;
#endif
#ifdef DEBUG
    _mglEnable = glad_debug_glEnable;
#else
    _mglEnable = glad_glEnable;
#endif
#ifdef DEBUG
    _mglFinish = glad_debug_glFinish;
#else
    _mglFinish = glad_glFinish;
#endif
#ifdef DEBUG
    _mglFlush = glad_debug_glFlush;
#else
    _mglFlush = glad_glFlush;
#endif
#ifdef DEBUG
    _mglBlendFunc = glad_debug_glBlendFunc;
#else
    _mglBlendFunc = glad_glBlendFunc;
#endif
#ifdef DEBUG
    _mglLogicOp = glad_debug_glLogicOp;
#else
    _mglLogicOp = glad_glLogicOp;
#endif
#ifdef DEBUG
    _mglStencilFunc = glad_debug_glStencilFunc;
#else
    _mglStencilFunc = glad_glStencilFunc;
#endif
#ifdef DEBUG
    _mglStencilOp = glad_debug_glStencilOp;
#else
    _mglStencilOp = glad_glStencilOp;
#endif
#ifdef DEBUG
    _mglDepthFunc = glad_debug_glDepthFunc;
#else
    _mglDepthFunc = glad_glDepthFunc;
#endif
#ifdef DEBUG
    _mglPixelStoref = glad_debug_glPixelStoref;
#else
    _mglPixelStoref = glad_glPixelStoref;
#endif
#ifdef DEBUG
    _mglPixelStorei = glad_debug_glPixelStorei;
#else
    _mglPixelStorei = glad_glPixelStorei;
#endif
#ifdef DEBUG
    _mglReadBuffer = glad_debug_glReadBuffer;
#else
    _mglReadBuffer = glad_glReadBuffer;
#endif
#ifdef DEBUG
    _mglReadPixels = glad_debug_glReadPixels;
#else
    _mglReadPixels = glad_glReadPixels;
#endif
#ifdef DEBUG
    _mglGetBooleanv = glad_debug_glGetBooleanv;
#else
    _mglGetBooleanv = glad_glGetBooleanv;
#endif
#ifdef DEBUG
    _mglGetDoublev = glad_debug_glGetDoublev;
#else
    _mglGetDoublev = glad_glGetDoublev;
#endif
#ifdef DEBUG
    _mglGetError = glad_debug_glGetError;
#else
    _mglGetError = glad_glGetError;
#endif
#ifdef DEBUG
    _mglGetFloatv = glad_debug_glGetFloatv;
#else
    _mglGetFloatv = glad_glGetFloatv;
#endif
#ifdef DEBUG
    _mglGetIntegerv = glad_debug_glGetIntegerv;
#else
    _mglGetIntegerv = glad_glGetIntegerv;
#endif
#ifdef DEBUG
    _mglGetString = glad_debug_glGetString;
#else
    _mglGetString = glad_glGetString;
#endif
#ifdef DEBUG
    _mglGetTexImage = glad_debug_glGetTexImage;
#else
    _mglGetTexImage = glad_glGetTexImage;
#endif
#ifdef DEBUG
    _mglGetTexParameterfv = glad_debug_glGetTexParameterfv;
#else
    _mglGetTexParameterfv = glad_glGetTexParameterfv;
#endif
#ifdef DEBUG
    _mglGetTexParameteriv = glad_debug_glGetTexParameteriv;
#else
    _mglGetTexParameteriv = glad_glGetTexParameteriv;
#endif
#ifdef DEBUG
    _mglGetTexLevelParameterfv = glad_debug_glGetTexLevelParameterfv;
#else
    _mglGetTexLevelParameterfv = glad_glGetTexLevelParameterfv;
#endif
#ifdef DEBUG
    _mglGetTexLevelParameteriv = glad_debug_glGetTexLevelParameteriv;
#else
    _mglGetTexLevelParameteriv = glad_glGetTexLevelParameteriv;
#endif
#ifdef DEBUG
    _mglIsEnabled = glad_debug_glIsEnabled;
#else
    _mglIsEnabled = glad_glIsEnabled;
#endif
#ifdef DEBUG
    _mglDepthRange = glad_debug_glDepthRange;
#else
    _mglDepthRange = glad_glDepthRange;
#endif
#ifdef DEBUG
    _mglViewport = glad_debug_glViewport;
#else
    _mglViewport = glad_glViewport;
#endif
#ifdef DEBUG
    _mglNewList = glad_debug_glNewList;
#else
    _mglNewList = glad_glNewList;
#endif
#ifdef DEBUG
    _mglEndList = glad_debug_glEndList;
#else
    _mglEndList = glad_glEndList;
#endif
#ifdef DEBUG
    _mglCallList = glad_debug_glCallList;
#else
    _mglCallList = glad_glCallList;
#endif
#ifdef DEBUG
    _mglCallLists = glad_debug_glCallLists;
#else
    _mglCallLists = glad_glCallLists;
#endif
#ifdef DEBUG
    _mglDeleteLists = glad_debug_glDeleteLists;
#else
    _mglDeleteLists = glad_glDeleteLists;
#endif
#ifdef DEBUG
    _mglGenLists = glad_debug_glGenLists;
#else
    _mglGenLists = glad_glGenLists;
#endif
#ifdef DEBUG
    _mglListBase = glad_debug_glListBase;
#else
    _mglListBase = glad_glListBase;
#endif
#ifdef DEBUG
    _mglBegin = glad_debug_glBegin;
#else
    _mglBegin = glad_glBegin;
#endif
#ifdef DEBUG
    _mglBitmap = glad_debug_glBitmap;
#else
    _mglBitmap = glad_glBitmap;
#endif
#ifdef DEBUG
    _mglColor3b = glad_debug_glColor3b;
#else
    _mglColor3b = glad_glColor3b;
#endif
#ifdef DEBUG
    _mglColor3bv = glad_debug_glColor3bv;
#else
    _mglColor3bv = glad_glColor3bv;
#endif
#ifdef DEBUG
    _mglColor3d = glad_debug_glColor3d;
#else
    _mglColor3d = glad_glColor3d;
#endif
#ifdef DEBUG
    _mglColor3dv = glad_debug_glColor3dv;
#else
    _mglColor3dv = glad_glColor3dv;
#endif
#ifdef DEBUG
    _mglColor3f = glad_debug_glColor3f;
#else
    _mglColor3f = glad_glColor3f;
#endif
#ifdef DEBUG
    _mglColor3fv = glad_debug_glColor3fv;
#else
    _mglColor3fv = glad_glColor3fv;
#endif
#ifdef DEBUG
    _mglColor3i = glad_debug_glColor3i;
#else
    _mglColor3i = glad_glColor3i;
#endif
#ifdef DEBUG
    _mglColor3iv = glad_debug_glColor3iv;
#else
    _mglColor3iv = glad_glColor3iv;
#endif
#ifdef DEBUG
    _mglColor3s = glad_debug_glColor3s;
#else
    _mglColor3s = glad_glColor3s;
#endif
#ifdef DEBUG
    _mglColor3sv = glad_debug_glColor3sv;
#else
    _mglColor3sv = glad_glColor3sv;
#endif
#ifdef DEBUG
    _mglColor3ub = glad_debug_glColor3ub;
#else
    _mglColor3ub = glad_glColor3ub;
#endif
#ifdef DEBUG
    _mglColor3ubv = glad_debug_glColor3ubv;
#else
    _mglColor3ubv = glad_glColor3ubv;
#endif
#ifdef DEBUG
    _mglColor3ui = glad_debug_glColor3ui;
#else
    _mglColor3ui = glad_glColor3ui;
#endif
#ifdef DEBUG
    _mglColor3uiv = glad_debug_glColor3uiv;
#else
    _mglColor3uiv = glad_glColor3uiv;
#endif
#ifdef DEBUG
    _mglColor3us = glad_debug_glColor3us;
#else
    _mglColor3us = glad_glColor3us;
#endif
#ifdef DEBUG
    _mglColor3usv = glad_debug_glColor3usv;
#else
    _mglColor3usv = glad_glColor3usv;
#endif
#ifdef DEBUG
    _mglColor4b = glad_debug_glColor4b;
#else
    _mglColor4b = glad_glColor4b;
#endif
#ifdef DEBUG
    _mglColor4bv = glad_debug_glColor4bv;
#else
    _mglColor4bv = glad_glColor4bv;
#endif
#ifdef DEBUG
    _mglColor4d = glad_debug_glColor4d;
#else
    _mglColor4d = glad_glColor4d;
#endif
#ifdef DEBUG
    _mglColor4dv = glad_debug_glColor4dv;
#else
    _mglColor4dv = glad_glColor4dv;
#endif
#ifdef DEBUG
    _mglColor4f = glad_debug_glColor4f;
#else
    _mglColor4f = glad_glColor4f;
#endif
#ifdef DEBUG
    _mglColor4fv = glad_debug_glColor4fv;
#else
    _mglColor4fv = glad_glColor4fv;
#endif
#ifdef DEBUG
    _mglColor4i = glad_debug_glColor4i;
#else
    _mglColor4i = glad_glColor4i;
#endif
#ifdef DEBUG
    _mglColor4iv = glad_debug_glColor4iv;
#else
    _mglColor4iv = glad_glColor4iv;
#endif
#ifdef DEBUG
    _mglColor4s = glad_debug_glColor4s;
#else
    _mglColor4s = glad_glColor4s;
#endif
#ifdef DEBUG
    _mglColor4sv = glad_debug_glColor4sv;
#else
    _mglColor4sv = glad_glColor4sv;
#endif
#ifdef DEBUG
    _mglColor4ub = glad_debug_glColor4ub;
#else
    _mglColor4ub = glad_glColor4ub;
#endif
#ifdef DEBUG
    _mglColor4ubv = glad_debug_glColor4ubv;
#else
    _mglColor4ubv = glad_glColor4ubv;
#endif
#ifdef DEBUG
    _mglColor4ui = glad_debug_glColor4ui;
#else
    _mglColor4ui = glad_glColor4ui;
#endif
#ifdef DEBUG
    _mglColor4uiv = glad_debug_glColor4uiv;
#else
    _mglColor4uiv = glad_glColor4uiv;
#endif
#ifdef DEBUG
    _mglColor4us = glad_debug_glColor4us;
#else
    _mglColor4us = glad_glColor4us;
#endif
#ifdef DEBUG
    _mglColor4usv = glad_debug_glColor4usv;
#else
    _mglColor4usv = glad_glColor4usv;
#endif
#ifdef DEBUG
    _mglEdgeFlag = glad_debug_glEdgeFlag;
#else
    _mglEdgeFlag = glad_glEdgeFlag;
#endif
#ifdef DEBUG
    _mglEdgeFlagv = glad_debug_glEdgeFlagv;
#else
    _mglEdgeFlagv = glad_glEdgeFlagv;
#endif
#ifdef DEBUG
    _mglEnd = glad_debug_glEnd;
#else
    _mglEnd = glad_glEnd;
#endif
#ifdef DEBUG
    _mglIndexd = glad_debug_glIndexd;
#else
    _mglIndexd = glad_glIndexd;
#endif
#ifdef DEBUG
    _mglIndexdv = glad_debug_glIndexdv;
#else
    _mglIndexdv = glad_glIndexdv;
#endif
#ifdef DEBUG
    _mglIndexf = glad_debug_glIndexf;
#else
    _mglIndexf = glad_glIndexf;
#endif
#ifdef DEBUG
    _mglIndexfv = glad_debug_glIndexfv;
#else
    _mglIndexfv = glad_glIndexfv;
#endif
#ifdef DEBUG
    _mglIndexi = glad_debug_glIndexi;
#else
    _mglIndexi = glad_glIndexi;
#endif
#ifdef DEBUG
    _mglIndexiv = glad_debug_glIndexiv;
#else
    _mglIndexiv = glad_glIndexiv;
#endif
#ifdef DEBUG
    _mglIndexs = glad_debug_glIndexs;
#else
    _mglIndexs = glad_glIndexs;
#endif
#ifdef DEBUG
    _mglIndexsv = glad_debug_glIndexsv;
#else
    _mglIndexsv = glad_glIndexsv;
#endif
#ifdef DEBUG
    _mglNormal3b = glad_debug_glNormal3b;
#else
    _mglNormal3b = glad_glNormal3b;
#endif
#ifdef DEBUG
    _mglNormal3bv = glad_debug_glNormal3bv;
#else
    _mglNormal3bv = glad_glNormal3bv;
#endif
#ifdef DEBUG
    _mglNormal3d = glad_debug_glNormal3d;
#else
    _mglNormal3d = glad_glNormal3d;
#endif
#ifdef DEBUG
    _mglNormal3dv = glad_debug_glNormal3dv;
#else
    _mglNormal3dv = glad_glNormal3dv;
#endif
#ifdef DEBUG
    _mglNormal3f = glad_debug_glNormal3f;
#else
    _mglNormal3f = glad_glNormal3f;
#endif
#ifdef DEBUG
    _mglNormal3fv = glad_debug_glNormal3fv;
#else
    _mglNormal3fv = glad_glNormal3fv;
#endif
#ifdef DEBUG
    _mglNormal3i = glad_debug_glNormal3i;
#else
    _mglNormal3i = glad_glNormal3i;
#endif
#ifdef DEBUG
    _mglNormal3iv = glad_debug_glNormal3iv;
#else
    _mglNormal3iv = glad_glNormal3iv;
#endif
#ifdef DEBUG
    _mglNormal3s = glad_debug_glNormal3s;
#else
    _mglNormal3s = glad_glNormal3s;
#endif
#ifdef DEBUG
    _mglNormal3sv = glad_debug_glNormal3sv;
#else
    _mglNormal3sv = glad_glNormal3sv;
#endif
#ifdef DEBUG
    _mglRasterPos2d = glad_debug_glRasterPos2d;
#else
    _mglRasterPos2d = glad_glRasterPos2d;
#endif
#ifdef DEBUG
    _mglRasterPos2dv = glad_debug_glRasterPos2dv;
#else
    _mglRasterPos2dv = glad_glRasterPos2dv;
#endif
#ifdef DEBUG
    _mglRasterPos2f = glad_debug_glRasterPos2f;
#else
    _mglRasterPos2f = glad_glRasterPos2f;
#endif
#ifdef DEBUG
    _mglRasterPos2fv = glad_debug_glRasterPos2fv;
#else
    _mglRasterPos2fv = glad_glRasterPos2fv;
#endif
#ifdef DEBUG
    _mglRasterPos2i = glad_debug_glRasterPos2i;
#else
    _mglRasterPos2i = glad_glRasterPos2i;
#endif
#ifdef DEBUG
    _mglRasterPos2iv = glad_debug_glRasterPos2iv;
#else
    _mglRasterPos2iv = glad_glRasterPos2iv;
#endif
#ifdef DEBUG
    _mglRasterPos2s = glad_debug_glRasterPos2s;
#else
    _mglRasterPos2s = glad_glRasterPos2s;
#endif
#ifdef DEBUG
    _mglRasterPos2sv = glad_debug_glRasterPos2sv;
#else
    _mglRasterPos2sv = glad_glRasterPos2sv;
#endif
#ifdef DEBUG
    _mglRasterPos3d = glad_debug_glRasterPos3d;
#else
    _mglRasterPos3d = glad_glRasterPos3d;
#endif
#ifdef DEBUG
    _mglRasterPos3dv = glad_debug_glRasterPos3dv;
#else
    _mglRasterPos3dv = glad_glRasterPos3dv;
#endif
#ifdef DEBUG
    _mglRasterPos3f = glad_debug_glRasterPos3f;
#else
    _mglRasterPos3f = glad_glRasterPos3f;
#endif
#ifdef DEBUG
    _mglRasterPos3fv = glad_debug_glRasterPos3fv;
#else
    _mglRasterPos3fv = glad_glRasterPos3fv;
#endif
#ifdef DEBUG
    _mglRasterPos3i = glad_debug_glRasterPos3i;
#else
    _mglRasterPos3i = glad_glRasterPos3i;
#endif
#ifdef DEBUG
    _mglRasterPos3iv = glad_debug_glRasterPos3iv;
#else
    _mglRasterPos3iv = glad_glRasterPos3iv;
#endif
#ifdef DEBUG
    _mglRasterPos3s = glad_debug_glRasterPos3s;
#else
    _mglRasterPos3s = glad_glRasterPos3s;
#endif
#ifdef DEBUG
    _mglRasterPos3sv = glad_debug_glRasterPos3sv;
#else
    _mglRasterPos3sv = glad_glRasterPos3sv;
#endif
#ifdef DEBUG
    _mglRasterPos4d = glad_debug_glRasterPos4d;
#else
    _mglRasterPos4d = glad_glRasterPos4d;
#endif
#ifdef DEBUG
    _mglRasterPos4dv = glad_debug_glRasterPos4dv;
#else
    _mglRasterPos4dv = glad_glRasterPos4dv;
#endif
#ifdef DEBUG
    _mglRasterPos4f = glad_debug_glRasterPos4f;
#else
    _mglRasterPos4f = glad_glRasterPos4f;
#endif
#ifdef DEBUG
    _mglRasterPos4fv = glad_debug_glRasterPos4fv;
#else
    _mglRasterPos4fv = glad_glRasterPos4fv;
#endif
#ifdef DEBUG
    _mglRasterPos4i = glad_debug_glRasterPos4i;
#else
    _mglRasterPos4i = glad_glRasterPos4i;
#endif
#ifdef DEBUG
    _mglRasterPos4iv = glad_debug_glRasterPos4iv;
#else
    _mglRasterPos4iv = glad_glRasterPos4iv;
#endif
#ifdef DEBUG
    _mglRasterPos4s = glad_debug_glRasterPos4s;
#else
    _mglRasterPos4s = glad_glRasterPos4s;
#endif
#ifdef DEBUG
    _mglRasterPos4sv = glad_debug_glRasterPos4sv;
#else
    _mglRasterPos4sv = glad_glRasterPos4sv;
#endif
#ifdef DEBUG
    _mglRectd = glad_debug_glRectd;
#else
    _mglRectd = glad_glRectd;
#endif
#ifdef DEBUG
    _mglRectdv = glad_debug_glRectdv;
#else
    _mglRectdv = glad_glRectdv;
#endif
#ifdef DEBUG
    _mglRectf = glad_debug_glRectf;
#else
    _mglRectf = glad_glRectf;
#endif
#ifdef DEBUG
    _mglRectfv = glad_debug_glRectfv;
#else
    _mglRectfv = glad_glRectfv;
#endif
#ifdef DEBUG
    _mglRecti = glad_debug_glRecti;
#else
    _mglRecti = glad_glRecti;
#endif
#ifdef DEBUG
    _mglRectiv = glad_debug_glRectiv;
#else
    _mglRectiv = glad_glRectiv;
#endif
#ifdef DEBUG
    _mglRects = glad_debug_glRects;
#else
    _mglRects = glad_glRects;
#endif
#ifdef DEBUG
    _mglRectsv = glad_debug_glRectsv;
#else
    _mglRectsv = glad_glRectsv;
#endif
#ifdef DEBUG
    _mglTexCoord1d = glad_debug_glTexCoord1d;
#else
    _mglTexCoord1d = glad_glTexCoord1d;
#endif
#ifdef DEBUG
    _mglTexCoord1dv = glad_debug_glTexCoord1dv;
#else
    _mglTexCoord1dv = glad_glTexCoord1dv;
#endif
#ifdef DEBUG
    _mglTexCoord1f = glad_debug_glTexCoord1f;
#else
    _mglTexCoord1f = glad_glTexCoord1f;
#endif
#ifdef DEBUG
    _mglTexCoord1fv = glad_debug_glTexCoord1fv;
#else
    _mglTexCoord1fv = glad_glTexCoord1fv;
#endif
#ifdef DEBUG
    _mglTexCoord1i = glad_debug_glTexCoord1i;
#else
    _mglTexCoord1i = glad_glTexCoord1i;
#endif
#ifdef DEBUG
    _mglTexCoord1iv = glad_debug_glTexCoord1iv;
#else
    _mglTexCoord1iv = glad_glTexCoord1iv;
#endif
#ifdef DEBUG
    _mglTexCoord1s = glad_debug_glTexCoord1s;
#else
    _mglTexCoord1s = glad_glTexCoord1s;
#endif
#ifdef DEBUG
    _mglTexCoord1sv = glad_debug_glTexCoord1sv;
#else
    _mglTexCoord1sv = glad_glTexCoord1sv;
#endif
#ifdef DEBUG
    _mglTexCoord2d = glad_debug_glTexCoord2d;
#else
    _mglTexCoord2d = glad_glTexCoord2d;
#endif
#ifdef DEBUG
    _mglTexCoord2dv = glad_debug_glTexCoord2dv;
#else
    _mglTexCoord2dv = glad_glTexCoord2dv;
#endif
#ifdef DEBUG
    _mglTexCoord2f = glad_debug_glTexCoord2f;
#else
    _mglTexCoord2f = glad_glTexCoord2f;
#endif
#ifdef DEBUG
    _mglTexCoord2fv = glad_debug_glTexCoord2fv;
#else
    _mglTexCoord2fv = glad_glTexCoord2fv;
#endif
#ifdef DEBUG
    _mglTexCoord2i = glad_debug_glTexCoord2i;
#else
    _mglTexCoord2i = glad_glTexCoord2i;
#endif
#ifdef DEBUG
    _mglTexCoord2iv = glad_debug_glTexCoord2iv;
#else
    _mglTexCoord2iv = glad_glTexCoord2iv;
#endif
#ifdef DEBUG
    _mglTexCoord2s = glad_debug_glTexCoord2s;
#else
    _mglTexCoord2s = glad_glTexCoord2s;
#endif
#ifdef DEBUG
    _mglTexCoord2sv = glad_debug_glTexCoord2sv;
#else
    _mglTexCoord2sv = glad_glTexCoord2sv;
#endif
#ifdef DEBUG
    _mglTexCoord3d = glad_debug_glTexCoord3d;
#else
    _mglTexCoord3d = glad_glTexCoord3d;
#endif
#ifdef DEBUG
    _mglTexCoord3dv = glad_debug_glTexCoord3dv;
#else
    _mglTexCoord3dv = glad_glTexCoord3dv;
#endif
#ifdef DEBUG
    _mglTexCoord3f = glad_debug_glTexCoord3f;
#else
    _mglTexCoord3f = glad_glTexCoord3f;
#endif
#ifdef DEBUG
    _mglTexCoord3fv = glad_debug_glTexCoord3fv;
#else
    _mglTexCoord3fv = glad_glTexCoord3fv;
#endif
#ifdef DEBUG
    _mglTexCoord3i = glad_debug_glTexCoord3i;
#else
    _mglTexCoord3i = glad_glTexCoord3i;
#endif
#ifdef DEBUG
    _mglTexCoord3iv = glad_debug_glTexCoord3iv;
#else
    _mglTexCoord3iv = glad_glTexCoord3iv;
#endif
#ifdef DEBUG
    _mglTexCoord3s = glad_debug_glTexCoord3s;
#else
    _mglTexCoord3s = glad_glTexCoord3s;
#endif
#ifdef DEBUG
    _mglTexCoord3sv = glad_debug_glTexCoord3sv;
#else
    _mglTexCoord3sv = glad_glTexCoord3sv;
#endif
#ifdef DEBUG
    _mglTexCoord4d = glad_debug_glTexCoord4d;
#else
    _mglTexCoord4d = glad_glTexCoord4d;
#endif
#ifdef DEBUG
    _mglTexCoord4dv = glad_debug_glTexCoord4dv;
#else
    _mglTexCoord4dv = glad_glTexCoord4dv;
#endif
#ifdef DEBUG
    _mglTexCoord4f = glad_debug_glTexCoord4f;
#else
    _mglTexCoord4f = glad_glTexCoord4f;
#endif
#ifdef DEBUG
    _mglTexCoord4fv = glad_debug_glTexCoord4fv;
#else
    _mglTexCoord4fv = glad_glTexCoord4fv;
#endif
#ifdef DEBUG
    _mglTexCoord4i = glad_debug_glTexCoord4i;
#else
    _mglTexCoord4i = glad_glTexCoord4i;
#endif
#ifdef DEBUG
    _mglTexCoord4iv = glad_debug_glTexCoord4iv;
#else
    _mglTexCoord4iv = glad_glTexCoord4iv;
#endif
#ifdef DEBUG
    _mglTexCoord4s = glad_debug_glTexCoord4s;
#else
    _mglTexCoord4s = glad_glTexCoord4s;
#endif
#ifdef DEBUG
    _mglTexCoord4sv = glad_debug_glTexCoord4sv;
#else
    _mglTexCoord4sv = glad_glTexCoord4sv;
#endif
#ifdef DEBUG
    _mglVertex2d = glad_debug_glVertex2d;
#else
    _mglVertex2d = glad_glVertex2d;
#endif
#ifdef DEBUG
    _mglVertex2dv = glad_debug_glVertex2dv;
#else
    _mglVertex2dv = glad_glVertex2dv;
#endif
#ifdef DEBUG
    _mglVertex2f = glad_debug_glVertex2f;
#else
    _mglVertex2f = glad_glVertex2f;
#endif
#ifdef DEBUG
    _mglVertex2fv = glad_debug_glVertex2fv;
#else
    _mglVertex2fv = glad_glVertex2fv;
#endif
#ifdef DEBUG
    _mglVertex2i = glad_debug_glVertex2i;
#else
    _mglVertex2i = glad_glVertex2i;
#endif
#ifdef DEBUG
    _mglVertex2iv = glad_debug_glVertex2iv;
#else
    _mglVertex2iv = glad_glVertex2iv;
#endif
#ifdef DEBUG
    _mglVertex2s = glad_debug_glVertex2s;
#else
    _mglVertex2s = glad_glVertex2s;
#endif
#ifdef DEBUG
    _mglVertex2sv = glad_debug_glVertex2sv;
#else
    _mglVertex2sv = glad_glVertex2sv;
#endif
#ifdef DEBUG
    _mglVertex3d = glad_debug_glVertex3d;
#else
    _mglVertex3d = glad_glVertex3d;
#endif
#ifdef DEBUG
    _mglVertex3dv = glad_debug_glVertex3dv;
#else
    _mglVertex3dv = glad_glVertex3dv;
#endif
#ifdef DEBUG
    _mglVertex3f = glad_debug_glVertex3f;
#else
    _mglVertex3f = glad_glVertex3f;
#endif
#ifdef DEBUG
    _mglVertex3fv = glad_debug_glVertex3fv;
#else
    _mglVertex3fv = glad_glVertex3fv;
#endif
#ifdef DEBUG
    _mglVertex3i = glad_debug_glVertex3i;
#else
    _mglVertex3i = glad_glVertex3i;
#endif
#ifdef DEBUG
    _mglVertex3iv = glad_debug_glVertex3iv;
#else
    _mglVertex3iv = glad_glVertex3iv;
#endif
#ifdef DEBUG
    _mglVertex3s = glad_debug_glVertex3s;
#else
    _mglVertex3s = glad_glVertex3s;
#endif
#ifdef DEBUG
    _mglVertex3sv = glad_debug_glVertex3sv;
#else
    _mglVertex3sv = glad_glVertex3sv;
#endif
#ifdef DEBUG
    _mglVertex4d = glad_debug_glVertex4d;
#else
    _mglVertex4d = glad_glVertex4d;
#endif
#ifdef DEBUG
    _mglVertex4dv = glad_debug_glVertex4dv;
#else
    _mglVertex4dv = glad_glVertex4dv;
#endif
#ifdef DEBUG
    _mglVertex4f = glad_debug_glVertex4f;
#else
    _mglVertex4f = glad_glVertex4f;
#endif
#ifdef DEBUG
    _mglVertex4fv = glad_debug_glVertex4fv;
#else
    _mglVertex4fv = glad_glVertex4fv;
#endif
#ifdef DEBUG
    _mglVertex4i = glad_debug_glVertex4i;
#else
    _mglVertex4i = glad_glVertex4i;
#endif
#ifdef DEBUG
    _mglVertex4iv = glad_debug_glVertex4iv;
#else
    _mglVertex4iv = glad_glVertex4iv;
#endif
#ifdef DEBUG
    _mglVertex4s = glad_debug_glVertex4s;
#else
    _mglVertex4s = glad_glVertex4s;
#endif
#ifdef DEBUG
    _mglVertex4sv = glad_debug_glVertex4sv;
#else
    _mglVertex4sv = glad_glVertex4sv;
#endif
#ifdef DEBUG
    _mglClipPlane = glad_debug_glClipPlane;
#else
    _mglClipPlane = glad_glClipPlane;
#endif
#ifdef DEBUG
    _mglColorMaterial = glad_debug_glColorMaterial;
#else
    _mglColorMaterial = glad_glColorMaterial;
#endif
#ifdef DEBUG
    _mglFogf = glad_debug_glFogf;
#else
    _mglFogf = glad_glFogf;
#endif
#ifdef DEBUG
    _mglFogfv = glad_debug_glFogfv;
#else
    _mglFogfv = glad_glFogfv;
#endif
#ifdef DEBUG
    _mglFogi = glad_debug_glFogi;
#else
    _mglFogi = glad_glFogi;
#endif
#ifdef DEBUG
    _mglFogiv = glad_debug_glFogiv;
#else
    _mglFogiv = glad_glFogiv;
#endif
#ifdef DEBUG
    _mglLightf = glad_debug_glLightf;
#else
    _mglLightf = glad_glLightf;
#endif
#ifdef DEBUG
    _mglLightfv = glad_debug_glLightfv;
#else
    _mglLightfv = glad_glLightfv;
#endif
#ifdef DEBUG
    _mglLighti = glad_debug_glLighti;
#else
    _mglLighti = glad_glLighti;
#endif
#ifdef DEBUG
    _mglLightiv = glad_debug_glLightiv;
#else
    _mglLightiv = glad_glLightiv;
#endif
#ifdef DEBUG
    _mglLightModelf = glad_debug_glLightModelf;
#else
    _mglLightModelf = glad_glLightModelf;
#endif
#ifdef DEBUG
    _mglLightModelfv = glad_debug_glLightModelfv;
#else
    _mglLightModelfv = glad_glLightModelfv;
#endif
#ifdef DEBUG
    _mglLightModeli = glad_debug_glLightModeli;
#else
    _mglLightModeli = glad_glLightModeli;
#endif
#ifdef DEBUG
    _mglLightModeliv = glad_debug_glLightModeliv;
#else
    _mglLightModeliv = glad_glLightModeliv;
#endif
#ifdef DEBUG
    _mglLineStipple = glad_debug_glLineStipple;
#else
    _mglLineStipple = glad_glLineStipple;
#endif
#ifdef DEBUG
    _mglMaterialf = glad_debug_glMaterialf;
#else
    _mglMaterialf = glad_glMaterialf;
#endif
#ifdef DEBUG
    _mglMaterialfv = glad_debug_glMaterialfv;
#else
    _mglMaterialfv = glad_glMaterialfv;
#endif
#ifdef DEBUG
    _mglMateriali = glad_debug_glMateriali;
#else
    _mglMateriali = glad_glMateriali;
#endif
#ifdef DEBUG
    _mglMaterialiv = glad_debug_glMaterialiv;
#else
    _mglMaterialiv = glad_glMaterialiv;
#endif
#ifdef DEBUG
    _mglPolygonStipple = glad_debug_glPolygonStipple;
#else
    _mglPolygonStipple = glad_glPolygonStipple;
#endif
#ifdef DEBUG
    _mglShadeModel = glad_debug_glShadeModel;
#else
    _mglShadeModel = glad_glShadeModel;
#endif
#ifdef DEBUG
    _mglTexEnvf = glad_debug_glTexEnvf;
#else
    _mglTexEnvf = glad_glTexEnvf;
#endif
#ifdef DEBUG
    _mglTexEnvfv = glad_debug_glTexEnvfv;
#else
    _mglTexEnvfv = glad_glTexEnvfv;
#endif
#ifdef DEBUG
    _mglTexEnvi = glad_debug_glTexEnvi;
#else
    _mglTexEnvi = glad_glTexEnvi;
#endif
#ifdef DEBUG
    _mglTexEnviv = glad_debug_glTexEnviv;
#else
    _mglTexEnviv = glad_glTexEnviv;
#endif
#ifdef DEBUG
    _mglTexGend = glad_debug_glTexGend;
#else
    _mglTexGend = glad_glTexGend;
#endif
#ifdef DEBUG
    _mglTexGendv = glad_debug_glTexGendv;
#else
    _mglTexGendv = glad_glTexGendv;
#endif
#ifdef DEBUG
    _mglTexGenf = glad_debug_glTexGenf;
#else
    _mglTexGenf = glad_glTexGenf;
#endif
#ifdef DEBUG
    _mglTexGenfv = glad_debug_glTexGenfv;
#else
    _mglTexGenfv = glad_glTexGenfv;
#endif
#ifdef DEBUG
    _mglTexGeni = glad_debug_glTexGeni;
#else
    _mglTexGeni = glad_glTexGeni;
#endif
#ifdef DEBUG
    _mglTexGeniv = glad_debug_glTexGeniv;
#else
    _mglTexGeniv = glad_glTexGeniv;
#endif
#ifdef DEBUG
    _mglFeedbackBuffer = glad_debug_glFeedbackBuffer;
#else
    _mglFeedbackBuffer = glad_glFeedbackBuffer;
#endif
#ifdef DEBUG
    _mglSelectBuffer = glad_debug_glSelectBuffer;
#else
    _mglSelectBuffer = glad_glSelectBuffer;
#endif
#ifdef DEBUG
    _mglRenderMode = glad_debug_glRenderMode;
#else
    _mglRenderMode = glad_glRenderMode;
#endif
#ifdef DEBUG
    _mglInitNames = glad_debug_glInitNames;
#else
    _mglInitNames = glad_glInitNames;
#endif
#ifdef DEBUG
    _mglLoadName = glad_debug_glLoadName;
#else
    _mglLoadName = glad_glLoadName;
#endif
#ifdef DEBUG
    _mglPassThrough = glad_debug_glPassThrough;
#else
    _mglPassThrough = glad_glPassThrough;
#endif
#ifdef DEBUG
    _mglPopName = glad_debug_glPopName;
#else
    _mglPopName = glad_glPopName;
#endif
#ifdef DEBUG
    _mglPushName = glad_debug_glPushName;
#else
    _mglPushName = glad_glPushName;
#endif
#ifdef DEBUG
    _mglClearAccum = glad_debug_glClearAccum;
#else
    _mglClearAccum = glad_glClearAccum;
#endif
#ifdef DEBUG
    _mglClearIndex = glad_debug_glClearIndex;
#else
    _mglClearIndex = glad_glClearIndex;
#endif
#ifdef DEBUG
    _mglIndexMask = glad_debug_glIndexMask;
#else
    _mglIndexMask = glad_glIndexMask;
#endif
#ifdef DEBUG
    _mglAccum = glad_debug_glAccum;
#else
    _mglAccum = glad_glAccum;
#endif
#ifdef DEBUG
    _mglPopAttrib = glad_debug_glPopAttrib;
#else
    _mglPopAttrib = glad_glPopAttrib;
#endif
#ifdef DEBUG
    _mglPushAttrib = glad_debug_glPushAttrib;
#else
    _mglPushAttrib = glad_glPushAttrib;
#endif
#ifdef DEBUG
    _mglMap1d = glad_debug_glMap1d;
#else
    _mglMap1d = glad_glMap1d;
#endif
#ifdef DEBUG
    _mglMap1f = glad_debug_glMap1f;
#else
    _mglMap1f = glad_glMap1f;
#endif
#ifdef DEBUG
    _mglMap2d = glad_debug_glMap2d;
#else
    _mglMap2d = glad_glMap2d;
#endif
#ifdef DEBUG
    _mglMap2f = glad_debug_glMap2f;
#else
    _mglMap2f = glad_glMap2f;
#endif
#ifdef DEBUG
    _mglMapGrid1d = glad_debug_glMapGrid1d;
#else
    _mglMapGrid1d = glad_glMapGrid1d;
#endif
#ifdef DEBUG
    _mglMapGrid1f = glad_debug_glMapGrid1f;
#else
    _mglMapGrid1f = glad_glMapGrid1f;
#endif
#ifdef DEBUG
    _mglMapGrid2d = glad_debug_glMapGrid2d;
#else
    _mglMapGrid2d = glad_glMapGrid2d;
#endif
#ifdef DEBUG
    _mglMapGrid2f = glad_debug_glMapGrid2f;
#else
    _mglMapGrid2f = glad_glMapGrid2f;
#endif
#ifdef DEBUG
    _mglEvalCoord1d = glad_debug_glEvalCoord1d;
#else
    _mglEvalCoord1d = glad_glEvalCoord1d;
#endif
#ifdef DEBUG
    _mglEvalCoord1dv = glad_debug_glEvalCoord1dv;
#else
    _mglEvalCoord1dv = glad_glEvalCoord1dv;
#endif
#ifdef DEBUG
    _mglEvalCoord1f = glad_debug_glEvalCoord1f;
#else
    _mglEvalCoord1f = glad_glEvalCoord1f;
#endif
#ifdef DEBUG
    _mglEvalCoord1fv = glad_debug_glEvalCoord1fv;
#else
    _mglEvalCoord1fv = glad_glEvalCoord1fv;
#endif
#ifdef DEBUG
    _mglEvalCoord2d = glad_debug_glEvalCoord2d;
#else
    _mglEvalCoord2d = glad_glEvalCoord2d;
#endif
#ifdef DEBUG
    _mglEvalCoord2dv = glad_debug_glEvalCoord2dv;
#else
    _mglEvalCoord2dv = glad_glEvalCoord2dv;
#endif
#ifdef DEBUG
    _mglEvalCoord2f = glad_debug_glEvalCoord2f;
#else
    _mglEvalCoord2f = glad_glEvalCoord2f;
#endif
#ifdef DEBUG
    _mglEvalCoord2fv = glad_debug_glEvalCoord2fv;
#else
    _mglEvalCoord2fv = glad_glEvalCoord2fv;
#endif
#ifdef DEBUG
    _mglEvalMesh1 = glad_debug_glEvalMesh1;
#else
    _mglEvalMesh1 = glad_glEvalMesh1;
#endif
#ifdef DEBUG
    _mglEvalPoint1 = glad_debug_glEvalPoint1;
#else
    _mglEvalPoint1 = glad_glEvalPoint1;
#endif
#ifdef DEBUG
    _mglEvalMesh2 = glad_debug_glEvalMesh2;
#else
    _mglEvalMesh2 = glad_glEvalMesh2;
#endif
#ifdef DEBUG
    _mglEvalPoint2 = glad_debug_glEvalPoint2;
#else
    _mglEvalPoint2 = glad_glEvalPoint2;
#endif
#ifdef DEBUG
    _mglAlphaFunc = glad_debug_glAlphaFunc;
#else
    _mglAlphaFunc = glad_glAlphaFunc;
#endif
#ifdef DEBUG
    _mglPixelZoom = glad_debug_glPixelZoom;
#else
    _mglPixelZoom = glad_glPixelZoom;
#endif
#ifdef DEBUG
    _mglPixelTransferf = glad_debug_glPixelTransferf;
#else
    _mglPixelTransferf = glad_glPixelTransferf;
#endif
#ifdef DEBUG
    _mglPixelTransferi = glad_debug_glPixelTransferi;
#else
    _mglPixelTransferi = glad_glPixelTransferi;
#endif
#ifdef DEBUG
    _mglPixelMapfv = glad_debug_glPixelMapfv;
#else
    _mglPixelMapfv = glad_glPixelMapfv;
#endif
#ifdef DEBUG
    _mglPixelMapuiv = glad_debug_glPixelMapuiv;
#else
    _mglPixelMapuiv = glad_glPixelMapuiv;
#endif
#ifdef DEBUG
    _mglPixelMapusv = glad_debug_glPixelMapusv;
#else
    _mglPixelMapusv = glad_glPixelMapusv;
#endif
#ifdef DEBUG
    _mglCopyPixels = glad_debug_glCopyPixels;
#else
    _mglCopyPixels = glad_glCopyPixels;
#endif
#ifdef DEBUG
    _mglDrawPixels = glad_debug_glDrawPixels;
#else
    _mglDrawPixels = glad_glDrawPixels;
#endif
#ifdef DEBUG
    _mglGetClipPlane = glad_debug_glGetClipPlane;
#else
    _mglGetClipPlane = glad_glGetClipPlane;
#endif
#ifdef DEBUG
    _mglGetLightfv = glad_debug_glGetLightfv;
#else
    _mglGetLightfv = glad_glGetLightfv;
#endif
#ifdef DEBUG
    _mglGetLightiv = glad_debug_glGetLightiv;
#else
    _mglGetLightiv = glad_glGetLightiv;
#endif
#ifdef DEBUG
    _mglGetMapdv = glad_debug_glGetMapdv;
#else
    _mglGetMapdv = glad_glGetMapdv;
#endif
#ifdef DEBUG
    _mglGetMapfv = glad_debug_glGetMapfv;
#else
    _mglGetMapfv = glad_glGetMapfv;
#endif
#ifdef DEBUG
    _mglGetMapiv = glad_debug_glGetMapiv;
#else
    _mglGetMapiv = glad_glGetMapiv;
#endif
#ifdef DEBUG
    _mglGetMaterialfv = glad_debug_glGetMaterialfv;
#else
    _mglGetMaterialfv = glad_glGetMaterialfv;
#endif
#ifdef DEBUG
    _mglGetMaterialiv = glad_debug_glGetMaterialiv;
#else
    _mglGetMaterialiv = glad_glGetMaterialiv;
#endif
#ifdef DEBUG
    _mglGetPixelMapfv = glad_debug_glGetPixelMapfv;
#else
    _mglGetPixelMapfv = glad_glGetPixelMapfv;
#endif
#ifdef DEBUG
    _mglGetPixelMapuiv = glad_debug_glGetPixelMapuiv;
#else
    _mglGetPixelMapuiv = glad_glGetPixelMapuiv;
#endif
#ifdef DEBUG
    _mglGetPixelMapusv = glad_debug_glGetPixelMapusv;
#else
    _mglGetPixelMapusv = glad_glGetPixelMapusv;
#endif
#ifdef DEBUG
    _mglGetPolygonStipple = glad_debug_glGetPolygonStipple;
#else
    _mglGetPolygonStipple = glad_glGetPolygonStipple;
#endif
#ifdef DEBUG
    _mglGetTexEnvfv = glad_debug_glGetTexEnvfv;
#else
    _mglGetTexEnvfv = glad_glGetTexEnvfv;
#endif
#ifdef DEBUG
    _mglGetTexEnviv = glad_debug_glGetTexEnviv;
#else
    _mglGetTexEnviv = glad_glGetTexEnviv;
#endif
#ifdef DEBUG
    _mglGetTexGendv = glad_debug_glGetTexGendv;
#else
    _mglGetTexGendv = glad_glGetTexGendv;
#endif
#ifdef DEBUG
    _mglGetTexGenfv = glad_debug_glGetTexGenfv;
#else
    _mglGetTexGenfv = glad_glGetTexGenfv;
#endif
#ifdef DEBUG
    _mglGetTexGeniv = glad_debug_glGetTexGeniv;
#else
    _mglGetTexGeniv = glad_glGetTexGeniv;
#endif
#ifdef DEBUG
    _mglIsList = glad_debug_glIsList;
#else
    _mglIsList = glad_glIsList;
#endif
#ifdef DEBUG
    _mglFrustum = glad_debug_glFrustum;
#else
    _mglFrustum = glad_glFrustum;
#endif
#ifdef DEBUG
    _mglLoadIdentity = glad_debug_glLoadIdentity;
#else
    _mglLoadIdentity = glad_glLoadIdentity;
#endif
#ifdef DEBUG
    _mglLoadMatrixf = glad_debug_glLoadMatrixf;
#else
    _mglLoadMatrixf = glad_glLoadMatrixf;
#endif
#ifdef DEBUG
    _mglLoadMatrixd = glad_debug_glLoadMatrixd;
#else
    _mglLoadMatrixd = glad_glLoadMatrixd;
#endif
#ifdef DEBUG
    _mglMatrixMode = glad_debug_glMatrixMode;
#else
    _mglMatrixMode = glad_glMatrixMode;
#endif
#ifdef DEBUG
    _mglMultMatrixf = glad_debug_glMultMatrixf;
#else
    _mglMultMatrixf = glad_glMultMatrixf;
#endif
#ifdef DEBUG
    _mglMultMatrixd = glad_debug_glMultMatrixd;
#else
    _mglMultMatrixd = glad_glMultMatrixd;
#endif
#ifdef DEBUG
    _mglOrtho = glad_debug_glOrtho;
#else
    _mglOrtho = glad_glOrtho;
#endif
#ifdef DEBUG
    _mglPopMatrix = glad_debug_glPopMatrix;
#else
    _mglPopMatrix = glad_glPopMatrix;
#endif
#ifdef DEBUG
    _mglPushMatrix = glad_debug_glPushMatrix;
#else
    _mglPushMatrix = glad_glPushMatrix;
#endif
#ifdef DEBUG
    _mglRotated = glad_debug_glRotated;
#else
    _mglRotated = glad_glRotated;
#endif
#ifdef DEBUG
    _mglRotatef = glad_debug_glRotatef;
#else
    _mglRotatef = glad_glRotatef;
#endif
#ifdef DEBUG
    _mglScaled = glad_debug_glScaled;
#else
    _mglScaled = glad_glScaled;
#endif
#ifdef DEBUG
    _mglScalef = glad_debug_glScalef;
#else
    _mglScalef = glad_glScalef;
#endif
#ifdef DEBUG
    _mglTranslated = glad_debug_glTranslated;
#else
    _mglTranslated = glad_glTranslated;
#endif
#ifdef DEBUG
    _mglTranslatef = glad_debug_glTranslatef;
#else
    _mglTranslatef = glad_glTranslatef;
#endif
#ifdef DEBUG
    _mglDrawArrays = glad_debug_glDrawArrays;
#else
    _mglDrawArrays = glad_glDrawArrays;
#endif
#ifdef DEBUG
    _mglDrawElements = glad_debug_glDrawElements;
#else
    _mglDrawElements = glad_glDrawElements;
#endif
#ifdef DEBUG
    _mglGetPointerv = glad_debug_glGetPointerv;
#else
    _mglGetPointerv = glad_glGetPointerv;
#endif
#ifdef DEBUG
    _mglPolygonOffset = glad_debug_glPolygonOffset;
#else
    _mglPolygonOffset = glad_glPolygonOffset;
#endif
#ifdef DEBUG
    _mglCopyTexImage1D = glad_debug_glCopyTexImage1D;
#else
    _mglCopyTexImage1D = glad_glCopyTexImage1D;
#endif
#ifdef DEBUG
    _mglCopyTexImage2D = glad_debug_glCopyTexImage2D;
#else
    _mglCopyTexImage2D = glad_glCopyTexImage2D;
#endif
#ifdef DEBUG
    _mglCopyTexSubImage1D = glad_debug_glCopyTexSubImage1D;
#else
    _mglCopyTexSubImage1D = glad_glCopyTexSubImage1D;
#endif
#ifdef DEBUG
    _mglCopyTexSubImage2D = glad_debug_glCopyTexSubImage2D;
#else
    _mglCopyTexSubImage2D = glad_glCopyTexSubImage2D;
#endif
#ifdef DEBUG
    _mglTexSubImage1D = glad_debug_glTexSubImage1D;
#else
    _mglTexSubImage1D = glad_glTexSubImage1D;
#endif
#ifdef DEBUG
    _mglTexSubImage2D = glad_debug_glTexSubImage2D;
#else
    _mglTexSubImage2D = glad_glTexSubImage2D;
#endif
#ifdef DEBUG
    _mglBindTexture = glad_debug_glBindTexture;
#else
    _mglBindTexture = glad_glBindTexture;
#endif
#ifdef DEBUG
    _mglDeleteTextures = glad_debug_glDeleteTextures;
#else
    _mglDeleteTextures = glad_glDeleteTextures;
#endif
#ifdef DEBUG
    _mglGenTextures = glad_debug_glGenTextures;
#else
    _mglGenTextures = glad_glGenTextures;
#endif
#ifdef DEBUG
    _mglIsTexture = glad_debug_glIsTexture;
#else
    _mglIsTexture = glad_glIsTexture;
#endif
#ifdef DEBUG
    _mglArrayElement = glad_debug_glArrayElement;
#else
    _mglArrayElement = glad_glArrayElement;
#endif
#ifdef DEBUG
    _mglColorPointer = glad_debug_glColorPointer;
#else
    _mglColorPointer = glad_glColorPointer;
#endif
#ifdef DEBUG
    _mglDisableClientState = glad_debug_glDisableClientState;
#else
    _mglDisableClientState = glad_glDisableClientState;
#endif
#ifdef DEBUG
    _mglEdgeFlagPointer = glad_debug_glEdgeFlagPointer;
#else
    _mglEdgeFlagPointer = glad_glEdgeFlagPointer;
#endif
#ifdef DEBUG
    _mglEnableClientState = glad_debug_glEnableClientState;
#else
    _mglEnableClientState = glad_glEnableClientState;
#endif
#ifdef DEBUG
    _mglIndexPointer = glad_debug_glIndexPointer;
#else
    _mglIndexPointer = glad_glIndexPointer;
#endif
#ifdef DEBUG
    _mglInterleavedArrays = glad_debug_glInterleavedArrays;
#else
    _mglInterleavedArrays = glad_glInterleavedArrays;
#endif
#ifdef DEBUG
    _mglNormalPointer = glad_debug_glNormalPointer;
#else
    _mglNormalPointer = glad_glNormalPointer;
#endif
#ifdef DEBUG
    _mglTexCoordPointer = glad_debug_glTexCoordPointer;
#else
    _mglTexCoordPointer = glad_glTexCoordPointer;
#endif
#ifdef DEBUG
    _mglVertexPointer = glad_debug_glVertexPointer;
#else
    _mglVertexPointer = glad_glVertexPointer;
#endif
#ifdef DEBUG
    _mglAreTexturesResident = glad_debug_glAreTexturesResident;
#else
    _mglAreTexturesResident = glad_glAreTexturesResident;
#endif
#ifdef DEBUG
    _mglPrioritizeTextures = glad_debug_glPrioritizeTextures;
#else
    _mglPrioritizeTextures = glad_glPrioritizeTextures;
#endif
#ifdef DEBUG
    _mglIndexub = glad_debug_glIndexub;
#else
    _mglIndexub = glad_glIndexub;
#endif
#ifdef DEBUG
    _mglIndexubv = glad_debug_glIndexubv;
#else
    _mglIndexubv = glad_glIndexubv;
#endif
#ifdef DEBUG
    _mglPopClientAttrib = glad_debug_glPopClientAttrib;
#else
    _mglPopClientAttrib = glad_glPopClientAttrib;
#endif
#ifdef DEBUG
    _mglPushClientAttrib = glad_debug_glPushClientAttrib;
#else
    _mglPushClientAttrib = glad_glPushClientAttrib;
#endif
#ifdef DEBUG
    _mglDrawRangeElements = glad_debug_glDrawRangeElements;
#else
    _mglDrawRangeElements = glad_glDrawRangeElements;
#endif
#ifdef DEBUG
    _mglTexImage3D = glad_debug_glTexImage3D;
#else
    _mglTexImage3D = glad_glTexImage3D;
#endif
#ifdef DEBUG
    _mglTexSubImage3D = glad_debug_glTexSubImage3D;
#else
    _mglTexSubImage3D = glad_glTexSubImage3D;
#endif
#ifdef DEBUG
    _mglCopyTexSubImage3D = glad_debug_glCopyTexSubImage3D;
#else
    _mglCopyTexSubImage3D = glad_glCopyTexSubImage3D;
#endif
#ifdef DEBUG
    _mglActiveTexture = glad_debug_glActiveTexture;
#else
    _mglActiveTexture = glad_glActiveTexture;
#endif
#ifdef DEBUG
    _mglSampleCoverage = glad_debug_glSampleCoverage;
#else
    _mglSampleCoverage = glad_glSampleCoverage;
#endif
#ifdef DEBUG
    _mglCompressedTexImage3D = glad_debug_glCompressedTexImage3D;
#else
    _mglCompressedTexImage3D = glad_glCompressedTexImage3D;
#endif
#ifdef DEBUG
    _mglCompressedTexImage2D = glad_debug_glCompressedTexImage2D;
#else
    _mglCompressedTexImage2D = glad_glCompressedTexImage2D;
#endif
#ifdef DEBUG
    _mglCompressedTexImage1D = glad_debug_glCompressedTexImage1D;
#else
    _mglCompressedTexImage1D = glad_glCompressedTexImage1D;
#endif
#ifdef DEBUG
    _mglCompressedTexSubImage3D = glad_debug_glCompressedTexSubImage3D;
#else
    _mglCompressedTexSubImage3D = glad_glCompressedTexSubImage3D;
#endif
#ifdef DEBUG
    _mglCompressedTexSubImage2D = glad_debug_glCompressedTexSubImage2D;
#else
    _mglCompressedTexSubImage2D = glad_glCompressedTexSubImage2D;
#endif
#ifdef DEBUG
    _mglCompressedTexSubImage1D = glad_debug_glCompressedTexSubImage1D;
#else
    _mglCompressedTexSubImage1D = glad_glCompressedTexSubImage1D;
#endif
#ifdef DEBUG
    _mglGetCompressedTexImage = glad_debug_glGetCompressedTexImage;
#else
    _mglGetCompressedTexImage = glad_glGetCompressedTexImage;
#endif
#ifdef DEBUG
    _mglClientActiveTexture = glad_debug_glClientActiveTexture;
#else
    _mglClientActiveTexture = glad_glClientActiveTexture;
#endif
#ifdef DEBUG
    _mglMultiTexCoord1d = glad_debug_glMultiTexCoord1d;
#else
    _mglMultiTexCoord1d = glad_glMultiTexCoord1d;
#endif
#ifdef DEBUG
    _mglMultiTexCoord1dv = glad_debug_glMultiTexCoord1dv;
#else
    _mglMultiTexCoord1dv = glad_glMultiTexCoord1dv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord1f = glad_debug_glMultiTexCoord1f;
#else
    _mglMultiTexCoord1f = glad_glMultiTexCoord1f;
#endif
#ifdef DEBUG
    _mglMultiTexCoord1fv = glad_debug_glMultiTexCoord1fv;
#else
    _mglMultiTexCoord1fv = glad_glMultiTexCoord1fv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord1i = glad_debug_glMultiTexCoord1i;
#else
    _mglMultiTexCoord1i = glad_glMultiTexCoord1i;
#endif
#ifdef DEBUG
    _mglMultiTexCoord1iv = glad_debug_glMultiTexCoord1iv;
#else
    _mglMultiTexCoord1iv = glad_glMultiTexCoord1iv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord1s = glad_debug_glMultiTexCoord1s;
#else
    _mglMultiTexCoord1s = glad_glMultiTexCoord1s;
#endif
#ifdef DEBUG
    _mglMultiTexCoord1sv = glad_debug_glMultiTexCoord1sv;
#else
    _mglMultiTexCoord1sv = glad_glMultiTexCoord1sv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord2d = glad_debug_glMultiTexCoord2d;
#else
    _mglMultiTexCoord2d = glad_glMultiTexCoord2d;
#endif
#ifdef DEBUG
    _mglMultiTexCoord2dv = glad_debug_glMultiTexCoord2dv;
#else
    _mglMultiTexCoord2dv = glad_glMultiTexCoord2dv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord2f = glad_debug_glMultiTexCoord2f;
#else
    _mglMultiTexCoord2f = glad_glMultiTexCoord2f;
#endif
#ifdef DEBUG
    _mglMultiTexCoord2fv = glad_debug_glMultiTexCoord2fv;
#else
    _mglMultiTexCoord2fv = glad_glMultiTexCoord2fv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord2i = glad_debug_glMultiTexCoord2i;
#else
    _mglMultiTexCoord2i = glad_glMultiTexCoord2i;
#endif
#ifdef DEBUG
    _mglMultiTexCoord2iv = glad_debug_glMultiTexCoord2iv;
#else
    _mglMultiTexCoord2iv = glad_glMultiTexCoord2iv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord2s = glad_debug_glMultiTexCoord2s;
#else
    _mglMultiTexCoord2s = glad_glMultiTexCoord2s;
#endif
#ifdef DEBUG
    _mglMultiTexCoord2sv = glad_debug_glMultiTexCoord2sv;
#else
    _mglMultiTexCoord2sv = glad_glMultiTexCoord2sv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord3d = glad_debug_glMultiTexCoord3d;
#else
    _mglMultiTexCoord3d = glad_glMultiTexCoord3d;
#endif
#ifdef DEBUG
    _mglMultiTexCoord3dv = glad_debug_glMultiTexCoord3dv;
#else
    _mglMultiTexCoord3dv = glad_glMultiTexCoord3dv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord3f = glad_debug_glMultiTexCoord3f;
#else
    _mglMultiTexCoord3f = glad_glMultiTexCoord3f;
#endif
#ifdef DEBUG
    _mglMultiTexCoord3fv = glad_debug_glMultiTexCoord3fv;
#else
    _mglMultiTexCoord3fv = glad_glMultiTexCoord3fv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord3i = glad_debug_glMultiTexCoord3i;
#else
    _mglMultiTexCoord3i = glad_glMultiTexCoord3i;
#endif
#ifdef DEBUG
    _mglMultiTexCoord3iv = glad_debug_glMultiTexCoord3iv;
#else
    _mglMultiTexCoord3iv = glad_glMultiTexCoord3iv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord3s = glad_debug_glMultiTexCoord3s;
#else
    _mglMultiTexCoord3s = glad_glMultiTexCoord3s;
#endif
#ifdef DEBUG
    _mglMultiTexCoord3sv = glad_debug_glMultiTexCoord3sv;
#else
    _mglMultiTexCoord3sv = glad_glMultiTexCoord3sv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord4d = glad_debug_glMultiTexCoord4d;
#else
    _mglMultiTexCoord4d = glad_glMultiTexCoord4d;
#endif
#ifdef DEBUG
    _mglMultiTexCoord4dv = glad_debug_glMultiTexCoord4dv;
#else
    _mglMultiTexCoord4dv = glad_glMultiTexCoord4dv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord4f = glad_debug_glMultiTexCoord4f;
#else
    _mglMultiTexCoord4f = glad_glMultiTexCoord4f;
#endif
#ifdef DEBUG
    _mglMultiTexCoord4fv = glad_debug_glMultiTexCoord4fv;
#else
    _mglMultiTexCoord4fv = glad_glMultiTexCoord4fv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord4i = glad_debug_glMultiTexCoord4i;
#else
    _mglMultiTexCoord4i = glad_glMultiTexCoord4i;
#endif
#ifdef DEBUG
    _mglMultiTexCoord4iv = glad_debug_glMultiTexCoord4iv;
#else
    _mglMultiTexCoord4iv = glad_glMultiTexCoord4iv;
#endif
#ifdef DEBUG
    _mglMultiTexCoord4s = glad_debug_glMultiTexCoord4s;
#else
    _mglMultiTexCoord4s = glad_glMultiTexCoord4s;
#endif
#ifdef DEBUG
    _mglMultiTexCoord4sv = glad_debug_glMultiTexCoord4sv;
#else
    _mglMultiTexCoord4sv = glad_glMultiTexCoord4sv;
#endif
#ifdef DEBUG
    _mglLoadTransposeMatrixf = glad_debug_glLoadTransposeMatrixf;
#else
    _mglLoadTransposeMatrixf = glad_glLoadTransposeMatrixf;
#endif
#ifdef DEBUG
    _mglLoadTransposeMatrixd = glad_debug_glLoadTransposeMatrixd;
#else
    _mglLoadTransposeMatrixd = glad_glLoadTransposeMatrixd;
#endif
#ifdef DEBUG
    _mglMultTransposeMatrixf = glad_debug_glMultTransposeMatrixf;
#else
    _mglMultTransposeMatrixf = glad_glMultTransposeMatrixf;
#endif
#ifdef DEBUG
    _mglMultTransposeMatrixd = glad_debug_glMultTransposeMatrixd;
#else
    _mglMultTransposeMatrixd = glad_glMultTransposeMatrixd;
#endif
#ifdef DEBUG
    _mglBlendFuncSeparate = glad_debug_glBlendFuncSeparate;
#else
    _mglBlendFuncSeparate = glad_glBlendFuncSeparate;
#endif
#ifdef DEBUG
    _mglMultiDrawArrays = glad_debug_glMultiDrawArrays;
#else
    _mglMultiDrawArrays = glad_glMultiDrawArrays;
#endif
#ifdef DEBUG
    _mglMultiDrawElements = glad_debug_glMultiDrawElements;
#else
    _mglMultiDrawElements = glad_glMultiDrawElements;
#endif
#ifdef DEBUG
    _mglPointParameterf = glad_debug_glPointParameterf;
#else
    _mglPointParameterf = glad_glPointParameterf;
#endif
#ifdef DEBUG
    _mglPointParameterfv = glad_debug_glPointParameterfv;
#else
    _mglPointParameterfv = glad_glPointParameterfv;
#endif
#ifdef DEBUG
    _mglPointParameteri = glad_debug_glPointParameteri;
#else
    _mglPointParameteri = glad_glPointParameteri;
#endif
#ifdef DEBUG
    _mglPointParameteriv = glad_debug_glPointParameteriv;
#else
    _mglPointParameteriv = glad_glPointParameteriv;
#endif
#ifdef DEBUG
    _mglFogCoordf = glad_debug_glFogCoordf;
#else
    _mglFogCoordf = glad_glFogCoordf;
#endif
#ifdef DEBUG
    _mglFogCoordfv = glad_debug_glFogCoordfv;
#else
    _mglFogCoordfv = glad_glFogCoordfv;
#endif
#ifdef DEBUG
    _mglFogCoordd = glad_debug_glFogCoordd;
#else
    _mglFogCoordd = glad_glFogCoordd;
#endif
#ifdef DEBUG
    _mglFogCoorddv = glad_debug_glFogCoorddv;
#else
    _mglFogCoorddv = glad_glFogCoorddv;
#endif
#ifdef DEBUG
    _mglFogCoordPointer = glad_debug_glFogCoordPointer;
#else
    _mglFogCoordPointer = glad_glFogCoordPointer;
#endif
#ifdef DEBUG
    _mglSecondaryColor3b = glad_debug_glSecondaryColor3b;
#else
    _mglSecondaryColor3b = glad_glSecondaryColor3b;
#endif
#ifdef DEBUG
    _mglSecondaryColor3bv = glad_debug_glSecondaryColor3bv;
#else
    _mglSecondaryColor3bv = glad_glSecondaryColor3bv;
#endif
#ifdef DEBUG
    _mglSecondaryColor3d = glad_debug_glSecondaryColor3d;
#else
    _mglSecondaryColor3d = glad_glSecondaryColor3d;
#endif
#ifdef DEBUG
    _mglSecondaryColor3dv = glad_debug_glSecondaryColor3dv;
#else
    _mglSecondaryColor3dv = glad_glSecondaryColor3dv;
#endif
#ifdef DEBUG
    _mglSecondaryColor3f = glad_debug_glSecondaryColor3f;
#else
    _mglSecondaryColor3f = glad_glSecondaryColor3f;
#endif
#ifdef DEBUG
    _mglSecondaryColor3fv = glad_debug_glSecondaryColor3fv;
#else
    _mglSecondaryColor3fv = glad_glSecondaryColor3fv;
#endif
#ifdef DEBUG
    _mglSecondaryColor3i = glad_debug_glSecondaryColor3i;
#else
    _mglSecondaryColor3i = glad_glSecondaryColor3i;
#endif
#ifdef DEBUG
    _mglSecondaryColor3iv = glad_debug_glSecondaryColor3iv;
#else
    _mglSecondaryColor3iv = glad_glSecondaryColor3iv;
#endif
#ifdef DEBUG
    _mglSecondaryColor3s = glad_debug_glSecondaryColor3s;
#else
    _mglSecondaryColor3s = glad_glSecondaryColor3s;
#endif
#ifdef DEBUG
    _mglSecondaryColor3sv = glad_debug_glSecondaryColor3sv;
#else
    _mglSecondaryColor3sv = glad_glSecondaryColor3sv;
#endif
#ifdef DEBUG
    _mglSecondaryColor3ub = glad_debug_glSecondaryColor3ub;
#else
    _mglSecondaryColor3ub = glad_glSecondaryColor3ub;
#endif
#ifdef DEBUG
    _mglSecondaryColor3ubv = glad_debug_glSecondaryColor3ubv;
#else
    _mglSecondaryColor3ubv = glad_glSecondaryColor3ubv;
#endif
#ifdef DEBUG
    _mglSecondaryColor3ui = glad_debug_glSecondaryColor3ui;
#else
    _mglSecondaryColor3ui = glad_glSecondaryColor3ui;
#endif
#ifdef DEBUG
    _mglSecondaryColor3uiv = glad_debug_glSecondaryColor3uiv;
#else
    _mglSecondaryColor3uiv = glad_glSecondaryColor3uiv;
#endif
#ifdef DEBUG
    _mglSecondaryColor3us = glad_debug_glSecondaryColor3us;
#else
    _mglSecondaryColor3us = glad_glSecondaryColor3us;
#endif
#ifdef DEBUG
    _mglSecondaryColor3usv = glad_debug_glSecondaryColor3usv;
#else
    _mglSecondaryColor3usv = glad_glSecondaryColor3usv;
#endif
#ifdef DEBUG
    _mglSecondaryColorPointer = glad_debug_glSecondaryColorPointer;
#else
    _mglSecondaryColorPointer = glad_glSecondaryColorPointer;
#endif
#ifdef DEBUG
    _mglWindowPos2d = glad_debug_glWindowPos2d;
#else
    _mglWindowPos2d = glad_glWindowPos2d;
#endif
#ifdef DEBUG
    _mglWindowPos2dv = glad_debug_glWindowPos2dv;
#else
    _mglWindowPos2dv = glad_glWindowPos2dv;
#endif
#ifdef DEBUG
    _mglWindowPos2f = glad_debug_glWindowPos2f;
#else
    _mglWindowPos2f = glad_glWindowPos2f;
#endif
#ifdef DEBUG
    _mglWindowPos2fv = glad_debug_glWindowPos2fv;
#else
    _mglWindowPos2fv = glad_glWindowPos2fv;
#endif
#ifdef DEBUG
    _mglWindowPos2i = glad_debug_glWindowPos2i;
#else
    _mglWindowPos2i = glad_glWindowPos2i;
#endif
#ifdef DEBUG
    _mglWindowPos2iv = glad_debug_glWindowPos2iv;
#else
    _mglWindowPos2iv = glad_glWindowPos2iv;
#endif
#ifdef DEBUG
    _mglWindowPos2s = glad_debug_glWindowPos2s;
#else
    _mglWindowPos2s = glad_glWindowPos2s;
#endif
#ifdef DEBUG
    _mglWindowPos2sv = glad_debug_glWindowPos2sv;
#else
    _mglWindowPos2sv = glad_glWindowPos2sv;
#endif
#ifdef DEBUG
    _mglWindowPos3d = glad_debug_glWindowPos3d;
#else
    _mglWindowPos3d = glad_glWindowPos3d;
#endif
#ifdef DEBUG
    _mglWindowPos3dv = glad_debug_glWindowPos3dv;
#else
    _mglWindowPos3dv = glad_glWindowPos3dv;
#endif
#ifdef DEBUG
    _mglWindowPos3f = glad_debug_glWindowPos3f;
#else
    _mglWindowPos3f = glad_glWindowPos3f;
#endif
#ifdef DEBUG
    _mglWindowPos3fv = glad_debug_glWindowPos3fv;
#else
    _mglWindowPos3fv = glad_glWindowPos3fv;
#endif
#ifdef DEBUG
    _mglWindowPos3i = glad_debug_glWindowPos3i;
#else
    _mglWindowPos3i = glad_glWindowPos3i;
#endif
#ifdef DEBUG
    _mglWindowPos3iv = glad_debug_glWindowPos3iv;
#else
    _mglWindowPos3iv = glad_glWindowPos3iv;
#endif
#ifdef DEBUG
    _mglWindowPos3s = glad_debug_glWindowPos3s;
#else
    _mglWindowPos3s = glad_glWindowPos3s;
#endif
#ifdef DEBUG
    _mglWindowPos3sv = glad_debug_glWindowPos3sv;
#else
    _mglWindowPos3sv = glad_glWindowPos3sv;
#endif
#ifdef DEBUG
    _mglBlendColor = glad_debug_glBlendColor;
#else
    _mglBlendColor = glad_glBlendColor;
#endif
#ifdef DEBUG
    _mglBlendEquation = glad_debug_glBlendEquation;
#else
    _mglBlendEquation = glad_glBlendEquation;
#endif
#ifdef DEBUG
    _mglGenQueries = glad_debug_glGenQueries;
#else
    _mglGenQueries = glad_glGenQueries;
#endif
#ifdef DEBUG
    _mglDeleteQueries = glad_debug_glDeleteQueries;
#else
    _mglDeleteQueries = glad_glDeleteQueries;
#endif
#ifdef DEBUG
    _mglIsQuery = glad_debug_glIsQuery;
#else
    _mglIsQuery = glad_glIsQuery;
#endif
#ifdef DEBUG
    _mglBeginQuery = glad_debug_glBeginQuery;
#else
    _mglBeginQuery = glad_glBeginQuery;
#endif
#ifdef DEBUG
    _mglEndQuery = glad_debug_glEndQuery;
#else
    _mglEndQuery = glad_glEndQuery;
#endif
#ifdef DEBUG
    _mglGetQueryiv = glad_debug_glGetQueryiv;
#else
    _mglGetQueryiv = glad_glGetQueryiv;
#endif
#ifdef DEBUG
    _mglGetQueryObjectiv = glad_debug_glGetQueryObjectiv;
#else
    _mglGetQueryObjectiv = glad_glGetQueryObjectiv;
#endif
#ifdef DEBUG
    _mglGetQueryObjectuiv = glad_debug_glGetQueryObjectuiv;
#else
    _mglGetQueryObjectuiv = glad_glGetQueryObjectuiv;
#endif
#ifdef DEBUG
    _mglBindBuffer = glad_debug_glBindBuffer;
#else
    _mglBindBuffer = glad_glBindBuffer;
#endif
#ifdef DEBUG
    _mglDeleteBuffers = glad_debug_glDeleteBuffers;
#else
    _mglDeleteBuffers = glad_glDeleteBuffers;
#endif
#ifdef DEBUG
    _mglGenBuffers = glad_debug_glGenBuffers;
#else
    _mglGenBuffers = glad_glGenBuffers;
#endif
#ifdef DEBUG
    _mglIsBuffer = glad_debug_glIsBuffer;
#else
    _mglIsBuffer = glad_glIsBuffer;
#endif
#ifdef DEBUG
    _mglBufferData = glad_debug_glBufferData;
#else
    _mglBufferData = glad_glBufferData;
#endif
#ifdef DEBUG
    _mglBufferSubData = glad_debug_glBufferSubData;
#else
    _mglBufferSubData = glad_glBufferSubData;
#endif
#ifdef DEBUG
    _mglGetBufferSubData = glad_debug_glGetBufferSubData;
#else
    _mglGetBufferSubData = glad_glGetBufferSubData;
#endif
#ifdef DEBUG
    _mglMapBuffer = glad_debug_glMapBuffer;
#else
    _mglMapBuffer = glad_glMapBuffer;
#endif
#ifdef DEBUG
    _mglUnmapBuffer = glad_debug_glUnmapBuffer;
#else
    _mglUnmapBuffer = glad_glUnmapBuffer;
#endif
#ifdef DEBUG
    _mglGetBufferParameteriv = glad_debug_glGetBufferParameteriv;
#else
    _mglGetBufferParameteriv = glad_glGetBufferParameteriv;
#endif
#ifdef DEBUG
    _mglGetBufferPointerv = glad_debug_glGetBufferPointerv;
#else
    _mglGetBufferPointerv = glad_glGetBufferPointerv;
#endif
#ifdef DEBUG
    _mglBlendEquationSeparate = glad_debug_glBlendEquationSeparate;
#else
    _mglBlendEquationSeparate = glad_glBlendEquationSeparate;
#endif
#ifdef DEBUG
    _mglDrawBuffers = glad_debug_glDrawBuffers;
#else
    _mglDrawBuffers = glad_glDrawBuffers;
#endif
#ifdef DEBUG
    _mglStencilOpSeparate = glad_debug_glStencilOpSeparate;
#else
    _mglStencilOpSeparate = glad_glStencilOpSeparate;
#endif
#ifdef DEBUG
    _mglStencilFuncSeparate = glad_debug_glStencilFuncSeparate;
#else
    _mglStencilFuncSeparate = glad_glStencilFuncSeparate;
#endif
#ifdef DEBUG
    _mglStencilMaskSeparate = glad_debug_glStencilMaskSeparate;
#else
    _mglStencilMaskSeparate = glad_glStencilMaskSeparate;
#endif
#ifdef DEBUG
    _mglAttachShader = glad_debug_glAttachShader;
#else
    _mglAttachShader = glad_glAttachShader;
#endif
#ifdef DEBUG
    _mglBindAttribLocation = glad_debug_glBindAttribLocation;
#else
    _mglBindAttribLocation = glad_glBindAttribLocation;
#endif
#ifdef DEBUG
    _mglCompileShader = glad_debug_glCompileShader;
#else
    _mglCompileShader = glad_glCompileShader;
#endif
#ifdef DEBUG
    _mglCreateProgram = glad_debug_glCreateProgram;
#else
    _mglCreateProgram = glad_glCreateProgram;
#endif
#ifdef DEBUG
    _mglCreateShader = glad_debug_glCreateShader;
#else
    _mglCreateShader = glad_glCreateShader;
#endif
#ifdef DEBUG
    _mglDeleteProgram = glad_debug_glDeleteProgram;
#else
    _mglDeleteProgram = glad_glDeleteProgram;
#endif
#ifdef DEBUG
    _mglDeleteShader = glad_debug_glDeleteShader;
#else
    _mglDeleteShader = glad_glDeleteShader;
#endif
#ifdef DEBUG
    _mglDetachShader = glad_debug_glDetachShader;
#else
    _mglDetachShader = glad_glDetachShader;
#endif
#ifdef DEBUG
    _mglDisableVertexAttribArray = glad_debug_glDisableVertexAttribArray;
#else
    _mglDisableVertexAttribArray = glad_glDisableVertexAttribArray;
#endif
#ifdef DEBUG
    _mglEnableVertexAttribArray = glad_debug_glEnableVertexAttribArray;
#else
    _mglEnableVertexAttribArray = glad_glEnableVertexAttribArray;
#endif
#ifdef DEBUG
    _mglGetActiveAttrib = glad_debug_glGetActiveAttrib;
#else
    _mglGetActiveAttrib = glad_glGetActiveAttrib;
#endif
#ifdef DEBUG
    _mglGetActiveUniform = glad_debug_glGetActiveUniform;
#else
    _mglGetActiveUniform = glad_glGetActiveUniform;
#endif
#ifdef DEBUG
    _mglGetAttachedShaders = glad_debug_glGetAttachedShaders;
#else
    _mglGetAttachedShaders = glad_glGetAttachedShaders;
#endif
#ifdef DEBUG
    _mglGetAttribLocation = glad_debug_glGetAttribLocation;
#else
    _mglGetAttribLocation = glad_glGetAttribLocation;
#endif
#ifdef DEBUG
    _mglGetProgramiv = glad_debug_glGetProgramiv;
#else
    _mglGetProgramiv = glad_glGetProgramiv;
#endif
#ifdef DEBUG
    _mglGetProgramInfoLog = glad_debug_glGetProgramInfoLog;
#else
    _mglGetProgramInfoLog = glad_glGetProgramInfoLog;
#endif
#ifdef DEBUG
    _mglGetShaderiv = glad_debug_glGetShaderiv;
#else
    _mglGetShaderiv = glad_glGetShaderiv;
#endif
#ifdef DEBUG
    _mglGetShaderInfoLog = glad_debug_glGetShaderInfoLog;
#else
    _mglGetShaderInfoLog = glad_glGetShaderInfoLog;
#endif
#ifdef DEBUG
    _mglGetShaderSource = glad_debug_glGetShaderSource;
#else
    _mglGetShaderSource = glad_glGetShaderSource;
#endif
#ifdef DEBUG
    _mglGetUniformLocation = glad_debug_glGetUniformLocation;
#else
    _mglGetUniformLocation = glad_glGetUniformLocation;
#endif
#ifdef DEBUG
    _mglGetUniformfv = glad_debug_glGetUniformfv;
#else
    _mglGetUniformfv = glad_glGetUniformfv;
#endif
#ifdef DEBUG
    _mglGetUniformiv = glad_debug_glGetUniformiv;
#else
    _mglGetUniformiv = glad_glGetUniformiv;
#endif
#ifdef DEBUG
    _mglGetVertexAttribdv = glad_debug_glGetVertexAttribdv;
#else
    _mglGetVertexAttribdv = glad_glGetVertexAttribdv;
#endif
#ifdef DEBUG
    _mglGetVertexAttribfv = glad_debug_glGetVertexAttribfv;
#else
    _mglGetVertexAttribfv = glad_glGetVertexAttribfv;
#endif
#ifdef DEBUG
    _mglGetVertexAttribiv = glad_debug_glGetVertexAttribiv;
#else
    _mglGetVertexAttribiv = glad_glGetVertexAttribiv;
#endif
#ifdef DEBUG
    _mglGetVertexAttribPointerv = glad_debug_glGetVertexAttribPointerv;
#else
    _mglGetVertexAttribPointerv = glad_glGetVertexAttribPointerv;
#endif
#ifdef DEBUG
    _mglIsProgram = glad_debug_glIsProgram;
#else
    _mglIsProgram = glad_glIsProgram;
#endif
#ifdef DEBUG
    _mglIsShader = glad_debug_glIsShader;
#else
    _mglIsShader = glad_glIsShader;
#endif
#ifdef DEBUG
    _mglLinkProgram = glad_debug_glLinkProgram;
#else
    _mglLinkProgram = glad_glLinkProgram;
#endif
#ifdef DEBUG
    _mglShaderSource = glad_debug_glShaderSource;
#else
    _mglShaderSource = glad_glShaderSource;
#endif
#ifdef DEBUG
    _mglUseProgram = glad_debug_glUseProgram;
#else
    _mglUseProgram = glad_glUseProgram;
#endif
#ifdef DEBUG
    _mglUniform1f = glad_debug_glUniform1f;
#else
    _mglUniform1f = glad_glUniform1f;
#endif
#ifdef DEBUG
    _mglUniform2f = glad_debug_glUniform2f;
#else
    _mglUniform2f = glad_glUniform2f;
#endif
#ifdef DEBUG
    _mglUniform3f = glad_debug_glUniform3f;
#else
    _mglUniform3f = glad_glUniform3f;
#endif
#ifdef DEBUG
    _mglUniform4f = glad_debug_glUniform4f;
#else
    _mglUniform4f = glad_glUniform4f;
#endif
#ifdef DEBUG
    _mglUniform1i = glad_debug_glUniform1i;
#else
    _mglUniform1i = glad_glUniform1i;
#endif
#ifdef DEBUG
    _mglUniform2i = glad_debug_glUniform2i;
#else
    _mglUniform2i = glad_glUniform2i;
#endif
#ifdef DEBUG
    _mglUniform3i = glad_debug_glUniform3i;
#else
    _mglUniform3i = glad_glUniform3i;
#endif
#ifdef DEBUG
    _mglUniform4i = glad_debug_glUniform4i;
#else
    _mglUniform4i = glad_glUniform4i;
#endif
#ifdef DEBUG
    _mglUniform1fv = glad_debug_glUniform1fv;
#else
    _mglUniform1fv = glad_glUniform1fv;
#endif
#ifdef DEBUG
    _mglUniform2fv = glad_debug_glUniform2fv;
#else
    _mglUniform2fv = glad_glUniform2fv;
#endif
#ifdef DEBUG
    _mglUniform3fv = glad_debug_glUniform3fv;
#else
    _mglUniform3fv = glad_glUniform3fv;
#endif
#ifdef DEBUG
    _mglUniform4fv = glad_debug_glUniform4fv;
#else
    _mglUniform4fv = glad_glUniform4fv;
#endif
#ifdef DEBUG
    _mglUniform1iv = glad_debug_glUniform1iv;
#else
    _mglUniform1iv = glad_glUniform1iv;
#endif
#ifdef DEBUG
    _mglUniform2iv = glad_debug_glUniform2iv;
#else
    _mglUniform2iv = glad_glUniform2iv;
#endif
#ifdef DEBUG
    _mglUniform3iv = glad_debug_glUniform3iv;
#else
    _mglUniform3iv = glad_glUniform3iv;
#endif
#ifdef DEBUG
    _mglUniform4iv = glad_debug_glUniform4iv;
#else
    _mglUniform4iv = glad_glUniform4iv;
#endif
#ifdef DEBUG
    _mglUniformMatrix2fv = glad_debug_glUniformMatrix2fv;
#else
    _mglUniformMatrix2fv = glad_glUniformMatrix2fv;
#endif
#ifdef DEBUG
    _mglUniformMatrix3fv = glad_debug_glUniformMatrix3fv;
#else
    _mglUniformMatrix3fv = glad_glUniformMatrix3fv;
#endif
#ifdef DEBUG
    _mglUniformMatrix4fv = glad_debug_glUniformMatrix4fv;
#else
    _mglUniformMatrix4fv = glad_glUniformMatrix4fv;
#endif
#ifdef DEBUG
    _mglValidateProgram = glad_debug_glValidateProgram;
#else
    _mglValidateProgram = glad_glValidateProgram;
#endif
#ifdef DEBUG
    _mglVertexAttrib1d = glad_debug_glVertexAttrib1d;
#else
    _mglVertexAttrib1d = glad_glVertexAttrib1d;
#endif
#ifdef DEBUG
    _mglVertexAttrib1dv = glad_debug_glVertexAttrib1dv;
#else
    _mglVertexAttrib1dv = glad_glVertexAttrib1dv;
#endif
#ifdef DEBUG
    _mglVertexAttrib1f = glad_debug_glVertexAttrib1f;
#else
    _mglVertexAttrib1f = glad_glVertexAttrib1f;
#endif
#ifdef DEBUG
    _mglVertexAttrib1fv = glad_debug_glVertexAttrib1fv;
#else
    _mglVertexAttrib1fv = glad_glVertexAttrib1fv;
#endif
#ifdef DEBUG
    _mglVertexAttrib1s = glad_debug_glVertexAttrib1s;
#else
    _mglVertexAttrib1s = glad_glVertexAttrib1s;
#endif
#ifdef DEBUG
    _mglVertexAttrib1sv = glad_debug_glVertexAttrib1sv;
#else
    _mglVertexAttrib1sv = glad_glVertexAttrib1sv;
#endif
#ifdef DEBUG
    _mglVertexAttrib2d = glad_debug_glVertexAttrib2d;
#else
    _mglVertexAttrib2d = glad_glVertexAttrib2d;
#endif
#ifdef DEBUG
    _mglVertexAttrib2dv = glad_debug_glVertexAttrib2dv;
#else
    _mglVertexAttrib2dv = glad_glVertexAttrib2dv;
#endif
#ifdef DEBUG
    _mglVertexAttrib2f = glad_debug_glVertexAttrib2f;
#else
    _mglVertexAttrib2f = glad_glVertexAttrib2f;
#endif
#ifdef DEBUG
    _mglVertexAttrib2fv = glad_debug_glVertexAttrib2fv;
#else
    _mglVertexAttrib2fv = glad_glVertexAttrib2fv;
#endif
#ifdef DEBUG
    _mglVertexAttrib2s = glad_debug_glVertexAttrib2s;
#else
    _mglVertexAttrib2s = glad_glVertexAttrib2s;
#endif
#ifdef DEBUG
    _mglVertexAttrib2sv = glad_debug_glVertexAttrib2sv;
#else
    _mglVertexAttrib2sv = glad_glVertexAttrib2sv;
#endif
#ifdef DEBUG
    _mglVertexAttrib3d = glad_debug_glVertexAttrib3d;
#else
    _mglVertexAttrib3d = glad_glVertexAttrib3d;
#endif
#ifdef DEBUG
    _mglVertexAttrib3dv = glad_debug_glVertexAttrib3dv;
#else
    _mglVertexAttrib3dv = glad_glVertexAttrib3dv;
#endif
#ifdef DEBUG
    _mglVertexAttrib3f = glad_debug_glVertexAttrib3f;
#else
    _mglVertexAttrib3f = glad_glVertexAttrib3f;
#endif
#ifdef DEBUG
    _mglVertexAttrib3fv = glad_debug_glVertexAttrib3fv;
#else
    _mglVertexAttrib3fv = glad_glVertexAttrib3fv;
#endif
#ifdef DEBUG
    _mglVertexAttrib3s = glad_debug_glVertexAttrib3s;
#else
    _mglVertexAttrib3s = glad_glVertexAttrib3s;
#endif
#ifdef DEBUG
    _mglVertexAttrib3sv = glad_debug_glVertexAttrib3sv;
#else
    _mglVertexAttrib3sv = glad_glVertexAttrib3sv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4Nbv = glad_debug_glVertexAttrib4Nbv;
#else
    _mglVertexAttrib4Nbv = glad_glVertexAttrib4Nbv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4Niv = glad_debug_glVertexAttrib4Niv;
#else
    _mglVertexAttrib4Niv = glad_glVertexAttrib4Niv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4Nsv = glad_debug_glVertexAttrib4Nsv;
#else
    _mglVertexAttrib4Nsv = glad_glVertexAttrib4Nsv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4Nub = glad_debug_glVertexAttrib4Nub;
#else
    _mglVertexAttrib4Nub = glad_glVertexAttrib4Nub;
#endif
#ifdef DEBUG
    _mglVertexAttrib4Nubv = glad_debug_glVertexAttrib4Nubv;
#else
    _mglVertexAttrib4Nubv = glad_glVertexAttrib4Nubv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4Nuiv = glad_debug_glVertexAttrib4Nuiv;
#else
    _mglVertexAttrib4Nuiv = glad_glVertexAttrib4Nuiv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4Nusv = glad_debug_glVertexAttrib4Nusv;
#else
    _mglVertexAttrib4Nusv = glad_glVertexAttrib4Nusv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4bv = glad_debug_glVertexAttrib4bv;
#else
    _mglVertexAttrib4bv = glad_glVertexAttrib4bv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4d = glad_debug_glVertexAttrib4d;
#else
    _mglVertexAttrib4d = glad_glVertexAttrib4d;
#endif
#ifdef DEBUG
    _mglVertexAttrib4dv = glad_debug_glVertexAttrib4dv;
#else
    _mglVertexAttrib4dv = glad_glVertexAttrib4dv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4f = glad_debug_glVertexAttrib4f;
#else
    _mglVertexAttrib4f = glad_glVertexAttrib4f;
#endif
#ifdef DEBUG
    _mglVertexAttrib4fv = glad_debug_glVertexAttrib4fv;
#else
    _mglVertexAttrib4fv = glad_glVertexAttrib4fv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4iv = glad_debug_glVertexAttrib4iv;
#else
    _mglVertexAttrib4iv = glad_glVertexAttrib4iv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4s = glad_debug_glVertexAttrib4s;
#else
    _mglVertexAttrib4s = glad_glVertexAttrib4s;
#endif
#ifdef DEBUG
    _mglVertexAttrib4sv = glad_debug_glVertexAttrib4sv;
#else
    _mglVertexAttrib4sv = glad_glVertexAttrib4sv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4ubv = glad_debug_glVertexAttrib4ubv;
#else
    _mglVertexAttrib4ubv = glad_glVertexAttrib4ubv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4uiv = glad_debug_glVertexAttrib4uiv;
#else
    _mglVertexAttrib4uiv = glad_glVertexAttrib4uiv;
#endif
#ifdef DEBUG
    _mglVertexAttrib4usv = glad_debug_glVertexAttrib4usv;
#else
    _mglVertexAttrib4usv = glad_glVertexAttrib4usv;
#endif
#ifdef DEBUG
    _mglVertexAttribPointer = glad_debug_glVertexAttribPointer;
#else
    _mglVertexAttribPointer = glad_glVertexAttribPointer;
#endif
#ifdef DEBUG
    _mglBindBufferARB = glad_debug_glBindBufferARB;
#else
    _mglBindBufferARB = glad_glBindBufferARB;
#endif
#ifdef DEBUG
    _mglDeleteBuffersARB = glad_debug_glDeleteBuffersARB;
#else
    _mglDeleteBuffersARB = glad_glDeleteBuffersARB;
#endif
#ifdef DEBUG
    _mglGenBuffersARB = glad_debug_glGenBuffersARB;
#else
    _mglGenBuffersARB = glad_glGenBuffersARB;
#endif
#ifdef DEBUG
    _mglIsBufferARB = glad_debug_glIsBufferARB;
#else
    _mglIsBufferARB = glad_glIsBufferARB;
#endif
#ifdef DEBUG
    _mglBufferDataARB = glad_debug_glBufferDataARB;
#else
    _mglBufferDataARB = glad_glBufferDataARB;
#endif
#ifdef DEBUG
    _mglBufferSubDataARB = glad_debug_glBufferSubDataARB;
#else
    _mglBufferSubDataARB = glad_glBufferSubDataARB;
#endif
#ifdef DEBUG
    _mglGetBufferSubDataARB = glad_debug_glGetBufferSubDataARB;
#else
    _mglGetBufferSubDataARB = glad_glGetBufferSubDataARB;
#endif
#ifdef DEBUG
    _mglMapBufferARB = glad_debug_glMapBufferARB;
#else
    _mglMapBufferARB = glad_glMapBufferARB;
#endif
#ifdef DEBUG
    _mglUnmapBufferARB = glad_debug_glUnmapBufferARB;
#else
    _mglUnmapBufferARB = glad_glUnmapBufferARB;
#endif
#ifdef DEBUG
    _mglGetBufferParameterivARB = glad_debug_glGetBufferParameterivARB;
#else
    _mglGetBufferParameterivARB = glad_glGetBufferParameterivARB;
#endif
#ifdef DEBUG
    _mglGetBufferPointervARB = glad_debug_glGetBufferPointervARB;
#else
    _mglGetBufferPointervARB = glad_glGetBufferPointervARB;
#endif
#ifdef DEBUG
    _mglBindVertexArray = glad_debug_glBindVertexArray;
#else
    _mglBindVertexArray = glad_glBindVertexArray;
#endif
#ifdef DEBUG
    _mglDeleteVertexArrays = glad_debug_glDeleteVertexArrays;
#else
    _mglDeleteVertexArrays = glad_glDeleteVertexArrays;
#endif
#ifdef DEBUG
    _mglGenVertexArrays = glad_debug_glGenVertexArrays;
#else
    _mglGenVertexArrays = glad_glGenVertexArrays;
#endif
#ifdef DEBUG
    _mglIsVertexArray = glad_debug_glIsVertexArray;
#else
    _mglIsVertexArray = glad_glIsVertexArray;
#endif
#ifdef DEBUG
    _mglIsRenderbuffer = glad_debug_glIsRenderbuffer;
#else
    _mglIsRenderbuffer = glad_glIsRenderbuffer;
#endif
#ifdef DEBUG
    _mglBindRenderbuffer = glad_debug_glBindRenderbuffer;
#else
    _mglBindRenderbuffer = glad_glBindRenderbuffer;
#endif
#ifdef DEBUG
    _mglDeleteRenderbuffers = glad_debug_glDeleteRenderbuffers;
#else
    _mglDeleteRenderbuffers = glad_glDeleteRenderbuffers;
#endif
#ifdef DEBUG
    _mglGenRenderbuffers = glad_debug_glGenRenderbuffers;
#else
    _mglGenRenderbuffers = glad_glGenRenderbuffers;
#endif
#ifdef DEBUG
    _mglRenderbufferStorage = glad_debug_glRenderbufferStorage;
#else
    _mglRenderbufferStorage = glad_glRenderbufferStorage;
#endif
#ifdef DEBUG
    _mglGetRenderbufferParameteriv = glad_debug_glGetRenderbufferParameteriv;
#else
    _mglGetRenderbufferParameteriv = glad_glGetRenderbufferParameteriv;
#endif
#ifdef DEBUG
    _mglIsFramebuffer = glad_debug_glIsFramebuffer;
#else
    _mglIsFramebuffer = glad_glIsFramebuffer;
#endif
#ifdef DEBUG
    _mglBindFramebuffer = glad_debug_glBindFramebuffer;
#else
    _mglBindFramebuffer = glad_glBindFramebuffer;
#endif
#ifdef DEBUG
    _mglDeleteFramebuffers = glad_debug_glDeleteFramebuffers;
#else
    _mglDeleteFramebuffers = glad_glDeleteFramebuffers;
#endif
#ifdef DEBUG
    _mglGenFramebuffers = glad_debug_glGenFramebuffers;
#else
    _mglGenFramebuffers = glad_glGenFramebuffers;
#endif
#ifdef DEBUG
    _mglCheckFramebufferStatus = glad_debug_glCheckFramebufferStatus;
#else
    _mglCheckFramebufferStatus = glad_glCheckFramebufferStatus;
#endif
#ifdef DEBUG
    _mglFramebufferTexture1D = glad_debug_glFramebufferTexture1D;
#else
    _mglFramebufferTexture1D = glad_glFramebufferTexture1D;
#endif
#ifdef DEBUG
    _mglFramebufferTexture2D = glad_debug_glFramebufferTexture2D;
#else
    _mglFramebufferTexture2D = glad_glFramebufferTexture2D;
#endif
#ifdef DEBUG
    _mglFramebufferTexture3D = glad_debug_glFramebufferTexture3D;
#else
    _mglFramebufferTexture3D = glad_glFramebufferTexture3D;
#endif
#ifdef DEBUG
    _mglFramebufferRenderbuffer = glad_debug_glFramebufferRenderbuffer;
#else
    _mglFramebufferRenderbuffer = glad_glFramebufferRenderbuffer;
#endif
#ifdef DEBUG
    _mglGetFramebufferAttachmentParameteriv = glad_debug_glGetFramebufferAttachmentParameteriv;
#else
    _mglGetFramebufferAttachmentParameteriv = glad_glGetFramebufferAttachmentParameteriv;
#endif
#ifdef DEBUG
    _mglGenerateMipmap = glad_debug_glGenerateMipmap;
#else
    _mglGenerateMipmap = glad_glGenerateMipmap;
#endif
#ifdef DEBUG
    _mglBlitFramebuffer = glad_debug_glBlitFramebuffer;
#else
    _mglBlitFramebuffer = glad_glBlitFramebuffer;
#endif
#ifdef DEBUG
    _mglRenderbufferStorageMultisample = glad_debug_glRenderbufferStorageMultisample;
#else
    _mglRenderbufferStorageMultisample = glad_glRenderbufferStorageMultisample;
#endif
#ifdef DEBUG
    _mglFramebufferTextureLayer = glad_debug_glFramebufferTextureLayer;
#else
    _mglFramebufferTextureLayer = glad_glFramebufferTextureLayer;
#endif
#ifdef DEBUG
    _mglIsRenderbufferEXT = glad_debug_glIsRenderbufferEXT;
#else
    _mglIsRenderbufferEXT = glad_glIsRenderbufferEXT;
#endif
#ifdef DEBUG
    _mglBindRenderbufferEXT = glad_debug_glBindRenderbufferEXT;
#else
    _mglBindRenderbufferEXT = glad_glBindRenderbufferEXT;
#endif
#ifdef DEBUG
    _mglDeleteRenderbuffersEXT = glad_debug_glDeleteRenderbuffersEXT;
#else
    _mglDeleteRenderbuffersEXT = glad_glDeleteRenderbuffersEXT;
#endif
#ifdef DEBUG
    _mglGenRenderbuffersEXT = glad_debug_glGenRenderbuffersEXT;
#else
    _mglGenRenderbuffersEXT = glad_glGenRenderbuffersEXT;
#endif
#ifdef DEBUG
    _mglRenderbufferStorageEXT = glad_debug_glRenderbufferStorageEXT;
#else
    _mglRenderbufferStorageEXT = glad_glRenderbufferStorageEXT;
#endif
#ifdef DEBUG
    _mglGetRenderbufferParameterivEXT = glad_debug_glGetRenderbufferParameterivEXT;
#else
    _mglGetRenderbufferParameterivEXT = glad_glGetRenderbufferParameterivEXT;
#endif
#ifdef DEBUG
    _mglIsFramebufferEXT = glad_debug_glIsFramebufferEXT;
#else
    _mglIsFramebufferEXT = glad_glIsFramebufferEXT;
#endif
#ifdef DEBUG
    _mglBindFramebufferEXT = glad_debug_glBindFramebufferEXT;
#else
    _mglBindFramebufferEXT = glad_glBindFramebufferEXT;
#endif
#ifdef DEBUG
    _mglDeleteFramebuffersEXT = glad_debug_glDeleteFramebuffersEXT;
#else
    _mglDeleteFramebuffersEXT = glad_glDeleteFramebuffersEXT;
#endif
#ifdef DEBUG
    _mglGenFramebuffersEXT = glad_debug_glGenFramebuffersEXT;
#else
    _mglGenFramebuffersEXT = glad_glGenFramebuffersEXT;
#endif
#ifdef DEBUG
    _mglCheckFramebufferStatusEXT = glad_debug_glCheckFramebufferStatusEXT;
#else
    _mglCheckFramebufferStatusEXT = glad_glCheckFramebufferStatusEXT;
#endif
#ifdef DEBUG
    _mglFramebufferTexture1DEXT = glad_debug_glFramebufferTexture1DEXT;
#else
    _mglFramebufferTexture1DEXT = glad_glFramebufferTexture1DEXT;
#endif
#ifdef DEBUG
    _mglFramebufferTexture2DEXT = glad_debug_glFramebufferTexture2DEXT;
#else
    _mglFramebufferTexture2DEXT = glad_glFramebufferTexture2DEXT;
#endif
#ifdef DEBUG
    _mglFramebufferTexture3DEXT = glad_debug_glFramebufferTexture3DEXT;
#else
    _mglFramebufferTexture3DEXT = glad_glFramebufferTexture3DEXT;
#endif
#ifdef DEBUG
    _mglFramebufferRenderbufferEXT = glad_debug_glFramebufferRenderbufferEXT;
#else
    _mglFramebufferRenderbufferEXT = glad_glFramebufferRenderbufferEXT;
#endif
#ifdef DEBUG
    _mglGetFramebufferAttachmentParameterivEXT = glad_debug_glGetFramebufferAttachmentParameterivEXT;
#else
    _mglGetFramebufferAttachmentParameterivEXT = glad_glGetFramebufferAttachmentParameterivEXT;
#endif
#ifdef DEBUG
    _mglGenerateMipmapEXT = glad_debug_glGenerateMipmapEXT;
#else
    _mglGenerateMipmapEXT = glad_glGenerateMipmapEXT;
#endif
#ifdef DEBUG
    _mglBindVertexArrayAPPLE = glad_debug_glBindVertexArrayAPPLE;
#else
    _mglBindVertexArrayAPPLE = glad_glBindVertexArrayAPPLE;
#endif
#ifdef DEBUG
    _mglDeleteVertexArraysAPPLE = glad_debug_glDeleteVertexArraysAPPLE;
#else
    _mglDeleteVertexArraysAPPLE = glad_glDeleteVertexArraysAPPLE;
#endif
#ifdef DEBUG
    _mglGenVertexArraysAPPLE = glad_debug_glGenVertexArraysAPPLE;
#else
    _mglGenVertexArraysAPPLE = glad_glGenVertexArraysAPPLE;
#endif
#ifdef DEBUG
    _mglIsVertexArrayAPPLE = glad_debug_glIsVertexArrayAPPLE;
#else
    _mglIsVertexArrayAPPLE = glad_glIsVertexArrayAPPLE;
#endif
} // load_functions 

} // namespace Natron 

#endif // OSGLFUNCTIONS_gl_H
