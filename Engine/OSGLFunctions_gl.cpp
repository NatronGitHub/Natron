/*
 * THIS FILE WAS GENERATED AUTOMATICALLY FROM glad.h by tools/utils/generateGLIncludes, DO NOT EDIT
 */

#include "OSGLFunctions.h"

#include <glad/glad.h> // libs.pri sets the right include path. glads.h may set GLAD_DEBUG

extern "C" {
#ifdef GLAD_DEBUG
extern PFNGLCULLFACEPROC glad_debug_glCullFace;
#else
extern PFNGLCULLFACEPROC glad_glCullFace;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRONTFACEPROC glad_debug_glFrontFace;
#else
extern PFNGLFRONTFACEPROC glad_glFrontFace;
#endif
#ifdef GLAD_DEBUG
extern PFNGLHINTPROC glad_debug_glHint;
#else
extern PFNGLHINTPROC glad_glHint;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLINEWIDTHPROC glad_debug_glLineWidth;
#else
extern PFNGLLINEWIDTHPROC glad_glLineWidth;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOINTSIZEPROC glad_debug_glPointSize;
#else
extern PFNGLPOINTSIZEPROC glad_glPointSize;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOLYGONMODEPROC glad_debug_glPolygonMode;
#else
extern PFNGLPOLYGONMODEPROC glad_glPolygonMode;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSCISSORPROC glad_debug_glScissor;
#else
extern PFNGLSCISSORPROC glad_glScissor;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXPARAMETERFPROC glad_debug_glTexParameterf;
#else
extern PFNGLTEXPARAMETERFPROC glad_glTexParameterf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXPARAMETERFVPROC glad_debug_glTexParameterfv;
#else
extern PFNGLTEXPARAMETERFVPROC glad_glTexParameterfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXPARAMETERIPROC glad_debug_glTexParameteri;
#else
extern PFNGLTEXPARAMETERIPROC glad_glTexParameteri;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXPARAMETERIVPROC glad_debug_glTexParameteriv;
#else
extern PFNGLTEXPARAMETERIVPROC glad_glTexParameteriv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXIMAGE1DPROC glad_debug_glTexImage1D;
#else
extern PFNGLTEXIMAGE1DPROC glad_glTexImage1D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXIMAGE2DPROC glad_debug_glTexImage2D;
#else
extern PFNGLTEXIMAGE2DPROC glad_glTexImage2D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDRAWBUFFERPROC glad_debug_glDrawBuffer;
#else
extern PFNGLDRAWBUFFERPROC glad_glDrawBuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCLEARPROC glad_debug_glClear;
#else
extern PFNGLCLEARPROC glad_glClear;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCLEARCOLORPROC glad_debug_glClearColor;
#else
extern PFNGLCLEARCOLORPROC glad_glClearColor;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCLEARSTENCILPROC glad_debug_glClearStencil;
#else
extern PFNGLCLEARSTENCILPROC glad_glClearStencil;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCLEARDEPTHPROC glad_debug_glClearDepth;
#else
extern PFNGLCLEARDEPTHPROC glad_glClearDepth;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSTENCILMASKPROC glad_debug_glStencilMask;
#else
extern PFNGLSTENCILMASKPROC glad_glStencilMask;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLORMASKPROC glad_debug_glColorMask;
#else
extern PFNGLCOLORMASKPROC glad_glColorMask;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDEPTHMASKPROC glad_debug_glDepthMask;
#else
extern PFNGLDEPTHMASKPROC glad_glDepthMask;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDISABLEPROC glad_debug_glDisable;
#else
extern PFNGLDISABLEPROC glad_glDisable;
#endif
#ifdef GLAD_DEBUG
extern PFNGLENABLEPROC glad_debug_glEnable;
#else
extern PFNGLENABLEPROC glad_glEnable;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFINISHPROC glad_debug_glFinish;
#else
extern PFNGLFINISHPROC glad_glFinish;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFLUSHPROC glad_debug_glFlush;
#else
extern PFNGLFLUSHPROC glad_glFlush;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBLENDFUNCPROC glad_debug_glBlendFunc;
#else
extern PFNGLBLENDFUNCPROC glad_glBlendFunc;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLOGICOPPROC glad_debug_glLogicOp;
#else
extern PFNGLLOGICOPPROC glad_glLogicOp;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSTENCILFUNCPROC glad_debug_glStencilFunc;
#else
extern PFNGLSTENCILFUNCPROC glad_glStencilFunc;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSTENCILOPPROC glad_debug_glStencilOp;
#else
extern PFNGLSTENCILOPPROC glad_glStencilOp;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDEPTHFUNCPROC glad_debug_glDepthFunc;
#else
extern PFNGLDEPTHFUNCPROC glad_glDepthFunc;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPIXELSTOREFPROC glad_debug_glPixelStoref;
#else
extern PFNGLPIXELSTOREFPROC glad_glPixelStoref;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPIXELSTOREIPROC glad_debug_glPixelStorei;
#else
extern PFNGLPIXELSTOREIPROC glad_glPixelStorei;
#endif
#ifdef GLAD_DEBUG
extern PFNGLREADBUFFERPROC glad_debug_glReadBuffer;
#else
extern PFNGLREADBUFFERPROC glad_glReadBuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLREADPIXELSPROC glad_debug_glReadPixels;
#else
extern PFNGLREADPIXELSPROC glad_glReadPixels;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETBOOLEANVPROC glad_debug_glGetBooleanv;
#else
extern PFNGLGETBOOLEANVPROC glad_glGetBooleanv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETDOUBLEVPROC glad_debug_glGetDoublev;
#else
extern PFNGLGETDOUBLEVPROC glad_glGetDoublev;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETERRORPROC glad_debug_glGetError;
#else
extern PFNGLGETERRORPROC glad_glGetError;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETFLOATVPROC glad_debug_glGetFloatv;
#else
extern PFNGLGETFLOATVPROC glad_glGetFloatv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETINTEGERVPROC glad_debug_glGetIntegerv;
#else
extern PFNGLGETINTEGERVPROC glad_glGetIntegerv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETSTRINGPROC glad_debug_glGetString;
#else
extern PFNGLGETSTRINGPROC glad_glGetString;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXIMAGEPROC glad_debug_glGetTexImage;
#else
extern PFNGLGETTEXIMAGEPROC glad_glGetTexImage;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXPARAMETERFVPROC glad_debug_glGetTexParameterfv;
#else
extern PFNGLGETTEXPARAMETERFVPROC glad_glGetTexParameterfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXPARAMETERIVPROC glad_debug_glGetTexParameteriv;
#else
extern PFNGLGETTEXPARAMETERIVPROC glad_glGetTexParameteriv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXLEVELPARAMETERFVPROC glad_debug_glGetTexLevelParameterfv;
#else
extern PFNGLGETTEXLEVELPARAMETERFVPROC glad_glGetTexLevelParameterfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXLEVELPARAMETERIVPROC glad_debug_glGetTexLevelParameteriv;
#else
extern PFNGLGETTEXLEVELPARAMETERIVPROC glad_glGetTexLevelParameteriv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISENABLEDPROC glad_debug_glIsEnabled;
#else
extern PFNGLISENABLEDPROC glad_glIsEnabled;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDEPTHRANGEPROC glad_debug_glDepthRange;
#else
extern PFNGLDEPTHRANGEPROC glad_glDepthRange;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVIEWPORTPROC glad_debug_glViewport;
#else
extern PFNGLVIEWPORTPROC glad_glViewport;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNEWLISTPROC glad_debug_glNewList;
#else
extern PFNGLNEWLISTPROC glad_glNewList;
#endif
#ifdef GLAD_DEBUG
extern PFNGLENDLISTPROC glad_debug_glEndList;
#else
extern PFNGLENDLISTPROC glad_glEndList;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCALLLISTPROC glad_debug_glCallList;
#else
extern PFNGLCALLLISTPROC glad_glCallList;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCALLLISTSPROC glad_debug_glCallLists;
#else
extern PFNGLCALLLISTSPROC glad_glCallLists;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETELISTSPROC glad_debug_glDeleteLists;
#else
extern PFNGLDELETELISTSPROC glad_glDeleteLists;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENLISTSPROC glad_debug_glGenLists;
#else
extern PFNGLGENLISTSPROC glad_glGenLists;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLISTBASEPROC glad_debug_glListBase;
#else
extern PFNGLLISTBASEPROC glad_glListBase;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBEGINPROC glad_debug_glBegin;
#else
extern PFNGLBEGINPROC glad_glBegin;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBITMAPPROC glad_debug_glBitmap;
#else
extern PFNGLBITMAPPROC glad_glBitmap;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3BPROC glad_debug_glColor3b;
#else
extern PFNGLCOLOR3BPROC glad_glColor3b;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3BVPROC glad_debug_glColor3bv;
#else
extern PFNGLCOLOR3BVPROC glad_glColor3bv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3DPROC glad_debug_glColor3d;
#else
extern PFNGLCOLOR3DPROC glad_glColor3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3DVPROC glad_debug_glColor3dv;
#else
extern PFNGLCOLOR3DVPROC glad_glColor3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3FPROC glad_debug_glColor3f;
#else
extern PFNGLCOLOR3FPROC glad_glColor3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3FVPROC glad_debug_glColor3fv;
#else
extern PFNGLCOLOR3FVPROC glad_glColor3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3IPROC glad_debug_glColor3i;
#else
extern PFNGLCOLOR3IPROC glad_glColor3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3IVPROC glad_debug_glColor3iv;
#else
extern PFNGLCOLOR3IVPROC glad_glColor3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3SPROC glad_debug_glColor3s;
#else
extern PFNGLCOLOR3SPROC glad_glColor3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3SVPROC glad_debug_glColor3sv;
#else
extern PFNGLCOLOR3SVPROC glad_glColor3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3UBPROC glad_debug_glColor3ub;
#else
extern PFNGLCOLOR3UBPROC glad_glColor3ub;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3UBVPROC glad_debug_glColor3ubv;
#else
extern PFNGLCOLOR3UBVPROC glad_glColor3ubv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3UIPROC glad_debug_glColor3ui;
#else
extern PFNGLCOLOR3UIPROC glad_glColor3ui;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3UIVPROC glad_debug_glColor3uiv;
#else
extern PFNGLCOLOR3UIVPROC glad_glColor3uiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3USPROC glad_debug_glColor3us;
#else
extern PFNGLCOLOR3USPROC glad_glColor3us;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR3USVPROC glad_debug_glColor3usv;
#else
extern PFNGLCOLOR3USVPROC glad_glColor3usv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4BPROC glad_debug_glColor4b;
#else
extern PFNGLCOLOR4BPROC glad_glColor4b;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4BVPROC glad_debug_glColor4bv;
#else
extern PFNGLCOLOR4BVPROC glad_glColor4bv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4DPROC glad_debug_glColor4d;
#else
extern PFNGLCOLOR4DPROC glad_glColor4d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4DVPROC glad_debug_glColor4dv;
#else
extern PFNGLCOLOR4DVPROC glad_glColor4dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4FPROC glad_debug_glColor4f;
#else
extern PFNGLCOLOR4FPROC glad_glColor4f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4FVPROC glad_debug_glColor4fv;
#else
extern PFNGLCOLOR4FVPROC glad_glColor4fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4IPROC glad_debug_glColor4i;
#else
extern PFNGLCOLOR4IPROC glad_glColor4i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4IVPROC glad_debug_glColor4iv;
#else
extern PFNGLCOLOR4IVPROC glad_glColor4iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4SPROC glad_debug_glColor4s;
#else
extern PFNGLCOLOR4SPROC glad_glColor4s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4SVPROC glad_debug_glColor4sv;
#else
extern PFNGLCOLOR4SVPROC glad_glColor4sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4UBPROC glad_debug_glColor4ub;
#else
extern PFNGLCOLOR4UBPROC glad_glColor4ub;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4UBVPROC glad_debug_glColor4ubv;
#else
extern PFNGLCOLOR4UBVPROC glad_glColor4ubv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4UIPROC glad_debug_glColor4ui;
#else
extern PFNGLCOLOR4UIPROC glad_glColor4ui;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4UIVPROC glad_debug_glColor4uiv;
#else
extern PFNGLCOLOR4UIVPROC glad_glColor4uiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4USPROC glad_debug_glColor4us;
#else
extern PFNGLCOLOR4USPROC glad_glColor4us;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLOR4USVPROC glad_debug_glColor4usv;
#else
extern PFNGLCOLOR4USVPROC glad_glColor4usv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEDGEFLAGPROC glad_debug_glEdgeFlag;
#else
extern PFNGLEDGEFLAGPROC glad_glEdgeFlag;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEDGEFLAGVPROC glad_debug_glEdgeFlagv;
#else
extern PFNGLEDGEFLAGVPROC glad_glEdgeFlagv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLENDPROC glad_debug_glEnd;
#else
extern PFNGLENDPROC glad_glEnd;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXDPROC glad_debug_glIndexd;
#else
extern PFNGLINDEXDPROC glad_glIndexd;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXDVPROC glad_debug_glIndexdv;
#else
extern PFNGLINDEXDVPROC glad_glIndexdv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXFPROC glad_debug_glIndexf;
#else
extern PFNGLINDEXFPROC glad_glIndexf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXFVPROC glad_debug_glIndexfv;
#else
extern PFNGLINDEXFVPROC glad_glIndexfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXIPROC glad_debug_glIndexi;
#else
extern PFNGLINDEXIPROC glad_glIndexi;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXIVPROC glad_debug_glIndexiv;
#else
extern PFNGLINDEXIVPROC glad_glIndexiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXSPROC glad_debug_glIndexs;
#else
extern PFNGLINDEXSPROC glad_glIndexs;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXSVPROC glad_debug_glIndexsv;
#else
extern PFNGLINDEXSVPROC glad_glIndexsv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3BPROC glad_debug_glNormal3b;
#else
extern PFNGLNORMAL3BPROC glad_glNormal3b;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3BVPROC glad_debug_glNormal3bv;
#else
extern PFNGLNORMAL3BVPROC glad_glNormal3bv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3DPROC glad_debug_glNormal3d;
#else
extern PFNGLNORMAL3DPROC glad_glNormal3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3DVPROC glad_debug_glNormal3dv;
#else
extern PFNGLNORMAL3DVPROC glad_glNormal3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3FPROC glad_debug_glNormal3f;
#else
extern PFNGLNORMAL3FPROC glad_glNormal3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3FVPROC glad_debug_glNormal3fv;
#else
extern PFNGLNORMAL3FVPROC glad_glNormal3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3IPROC glad_debug_glNormal3i;
#else
extern PFNGLNORMAL3IPROC glad_glNormal3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3IVPROC glad_debug_glNormal3iv;
#else
extern PFNGLNORMAL3IVPROC glad_glNormal3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3SPROC glad_debug_glNormal3s;
#else
extern PFNGLNORMAL3SPROC glad_glNormal3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMAL3SVPROC glad_debug_glNormal3sv;
#else
extern PFNGLNORMAL3SVPROC glad_glNormal3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS2DPROC glad_debug_glRasterPos2d;
#else
extern PFNGLRASTERPOS2DPROC glad_glRasterPos2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS2DVPROC glad_debug_glRasterPos2dv;
#else
extern PFNGLRASTERPOS2DVPROC glad_glRasterPos2dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS2FPROC glad_debug_glRasterPos2f;
#else
extern PFNGLRASTERPOS2FPROC glad_glRasterPos2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS2FVPROC glad_debug_glRasterPos2fv;
#else
extern PFNGLRASTERPOS2FVPROC glad_glRasterPos2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS2IPROC glad_debug_glRasterPos2i;
#else
extern PFNGLRASTERPOS2IPROC glad_glRasterPos2i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS2IVPROC glad_debug_glRasterPos2iv;
#else
extern PFNGLRASTERPOS2IVPROC glad_glRasterPos2iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS2SPROC glad_debug_glRasterPos2s;
#else
extern PFNGLRASTERPOS2SPROC glad_glRasterPos2s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS2SVPROC glad_debug_glRasterPos2sv;
#else
extern PFNGLRASTERPOS2SVPROC glad_glRasterPos2sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS3DPROC glad_debug_glRasterPos3d;
#else
extern PFNGLRASTERPOS3DPROC glad_glRasterPos3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS3DVPROC glad_debug_glRasterPos3dv;
#else
extern PFNGLRASTERPOS3DVPROC glad_glRasterPos3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS3FPROC glad_debug_glRasterPos3f;
#else
extern PFNGLRASTERPOS3FPROC glad_glRasterPos3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS3FVPROC glad_debug_glRasterPos3fv;
#else
extern PFNGLRASTERPOS3FVPROC glad_glRasterPos3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS3IPROC glad_debug_glRasterPos3i;
#else
extern PFNGLRASTERPOS3IPROC glad_glRasterPos3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS3IVPROC glad_debug_glRasterPos3iv;
#else
extern PFNGLRASTERPOS3IVPROC glad_glRasterPos3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS3SPROC glad_debug_glRasterPos3s;
#else
extern PFNGLRASTERPOS3SPROC glad_glRasterPos3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS3SVPROC glad_debug_glRasterPos3sv;
#else
extern PFNGLRASTERPOS3SVPROC glad_glRasterPos3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS4DPROC glad_debug_glRasterPos4d;
#else
extern PFNGLRASTERPOS4DPROC glad_glRasterPos4d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS4DVPROC glad_debug_glRasterPos4dv;
#else
extern PFNGLRASTERPOS4DVPROC glad_glRasterPos4dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS4FPROC glad_debug_glRasterPos4f;
#else
extern PFNGLRASTERPOS4FPROC glad_glRasterPos4f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS4FVPROC glad_debug_glRasterPos4fv;
#else
extern PFNGLRASTERPOS4FVPROC glad_glRasterPos4fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS4IPROC glad_debug_glRasterPos4i;
#else
extern PFNGLRASTERPOS4IPROC glad_glRasterPos4i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS4IVPROC glad_debug_glRasterPos4iv;
#else
extern PFNGLRASTERPOS4IVPROC glad_glRasterPos4iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS4SPROC glad_debug_glRasterPos4s;
#else
extern PFNGLRASTERPOS4SPROC glad_glRasterPos4s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRASTERPOS4SVPROC glad_debug_glRasterPos4sv;
#else
extern PFNGLRASTERPOS4SVPROC glad_glRasterPos4sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRECTDPROC glad_debug_glRectd;
#else
extern PFNGLRECTDPROC glad_glRectd;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRECTDVPROC glad_debug_glRectdv;
#else
extern PFNGLRECTDVPROC glad_glRectdv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRECTFPROC glad_debug_glRectf;
#else
extern PFNGLRECTFPROC glad_glRectf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRECTFVPROC glad_debug_glRectfv;
#else
extern PFNGLRECTFVPROC glad_glRectfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRECTIPROC glad_debug_glRecti;
#else
extern PFNGLRECTIPROC glad_glRecti;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRECTIVPROC glad_debug_glRectiv;
#else
extern PFNGLRECTIVPROC glad_glRectiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRECTSPROC glad_debug_glRects;
#else
extern PFNGLRECTSPROC glad_glRects;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRECTSVPROC glad_debug_glRectsv;
#else
extern PFNGLRECTSVPROC glad_glRectsv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD1DPROC glad_debug_glTexCoord1d;
#else
extern PFNGLTEXCOORD1DPROC glad_glTexCoord1d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD1DVPROC glad_debug_glTexCoord1dv;
#else
extern PFNGLTEXCOORD1DVPROC glad_glTexCoord1dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD1FPROC glad_debug_glTexCoord1f;
#else
extern PFNGLTEXCOORD1FPROC glad_glTexCoord1f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD1FVPROC glad_debug_glTexCoord1fv;
#else
extern PFNGLTEXCOORD1FVPROC glad_glTexCoord1fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD1IPROC glad_debug_glTexCoord1i;
#else
extern PFNGLTEXCOORD1IPROC glad_glTexCoord1i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD1IVPROC glad_debug_glTexCoord1iv;
#else
extern PFNGLTEXCOORD1IVPROC glad_glTexCoord1iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD1SPROC glad_debug_glTexCoord1s;
#else
extern PFNGLTEXCOORD1SPROC glad_glTexCoord1s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD1SVPROC glad_debug_glTexCoord1sv;
#else
extern PFNGLTEXCOORD1SVPROC glad_glTexCoord1sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD2DPROC glad_debug_glTexCoord2d;
#else
extern PFNGLTEXCOORD2DPROC glad_glTexCoord2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD2DVPROC glad_debug_glTexCoord2dv;
#else
extern PFNGLTEXCOORD2DVPROC glad_glTexCoord2dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD2FPROC glad_debug_glTexCoord2f;
#else
extern PFNGLTEXCOORD2FPROC glad_glTexCoord2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD2FVPROC glad_debug_glTexCoord2fv;
#else
extern PFNGLTEXCOORD2FVPROC glad_glTexCoord2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD2IPROC glad_debug_glTexCoord2i;
#else
extern PFNGLTEXCOORD2IPROC glad_glTexCoord2i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD2IVPROC glad_debug_glTexCoord2iv;
#else
extern PFNGLTEXCOORD2IVPROC glad_glTexCoord2iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD2SPROC glad_debug_glTexCoord2s;
#else
extern PFNGLTEXCOORD2SPROC glad_glTexCoord2s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD2SVPROC glad_debug_glTexCoord2sv;
#else
extern PFNGLTEXCOORD2SVPROC glad_glTexCoord2sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD3DPROC glad_debug_glTexCoord3d;
#else
extern PFNGLTEXCOORD3DPROC glad_glTexCoord3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD3DVPROC glad_debug_glTexCoord3dv;
#else
extern PFNGLTEXCOORD3DVPROC glad_glTexCoord3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD3FPROC glad_debug_glTexCoord3f;
#else
extern PFNGLTEXCOORD3FPROC glad_glTexCoord3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD3FVPROC glad_debug_glTexCoord3fv;
#else
extern PFNGLTEXCOORD3FVPROC glad_glTexCoord3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD3IPROC glad_debug_glTexCoord3i;
#else
extern PFNGLTEXCOORD3IPROC glad_glTexCoord3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD3IVPROC glad_debug_glTexCoord3iv;
#else
extern PFNGLTEXCOORD3IVPROC glad_glTexCoord3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD3SPROC glad_debug_glTexCoord3s;
#else
extern PFNGLTEXCOORD3SPROC glad_glTexCoord3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD3SVPROC glad_debug_glTexCoord3sv;
#else
extern PFNGLTEXCOORD3SVPROC glad_glTexCoord3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD4DPROC glad_debug_glTexCoord4d;
#else
extern PFNGLTEXCOORD4DPROC glad_glTexCoord4d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD4DVPROC glad_debug_glTexCoord4dv;
#else
extern PFNGLTEXCOORD4DVPROC glad_glTexCoord4dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD4FPROC glad_debug_glTexCoord4f;
#else
extern PFNGLTEXCOORD4FPROC glad_glTexCoord4f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD4FVPROC glad_debug_glTexCoord4fv;
#else
extern PFNGLTEXCOORD4FVPROC glad_glTexCoord4fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD4IPROC glad_debug_glTexCoord4i;
#else
extern PFNGLTEXCOORD4IPROC glad_glTexCoord4i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD4IVPROC glad_debug_glTexCoord4iv;
#else
extern PFNGLTEXCOORD4IVPROC glad_glTexCoord4iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD4SPROC glad_debug_glTexCoord4s;
#else
extern PFNGLTEXCOORD4SPROC glad_glTexCoord4s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORD4SVPROC glad_debug_glTexCoord4sv;
#else
extern PFNGLTEXCOORD4SVPROC glad_glTexCoord4sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX2DPROC glad_debug_glVertex2d;
#else
extern PFNGLVERTEX2DPROC glad_glVertex2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX2DVPROC glad_debug_glVertex2dv;
#else
extern PFNGLVERTEX2DVPROC glad_glVertex2dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX2FPROC glad_debug_glVertex2f;
#else
extern PFNGLVERTEX2FPROC glad_glVertex2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX2FVPROC glad_debug_glVertex2fv;
#else
extern PFNGLVERTEX2FVPROC glad_glVertex2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX2IPROC glad_debug_glVertex2i;
#else
extern PFNGLVERTEX2IPROC glad_glVertex2i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX2IVPROC glad_debug_glVertex2iv;
#else
extern PFNGLVERTEX2IVPROC glad_glVertex2iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX2SPROC glad_debug_glVertex2s;
#else
extern PFNGLVERTEX2SPROC glad_glVertex2s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX2SVPROC glad_debug_glVertex2sv;
#else
extern PFNGLVERTEX2SVPROC glad_glVertex2sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX3DPROC glad_debug_glVertex3d;
#else
extern PFNGLVERTEX3DPROC glad_glVertex3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX3DVPROC glad_debug_glVertex3dv;
#else
extern PFNGLVERTEX3DVPROC glad_glVertex3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX3FPROC glad_debug_glVertex3f;
#else
extern PFNGLVERTEX3FPROC glad_glVertex3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX3FVPROC glad_debug_glVertex3fv;
#else
extern PFNGLVERTEX3FVPROC glad_glVertex3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX3IPROC glad_debug_glVertex3i;
#else
extern PFNGLVERTEX3IPROC glad_glVertex3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX3IVPROC glad_debug_glVertex3iv;
#else
extern PFNGLVERTEX3IVPROC glad_glVertex3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX3SPROC glad_debug_glVertex3s;
#else
extern PFNGLVERTEX3SPROC glad_glVertex3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX3SVPROC glad_debug_glVertex3sv;
#else
extern PFNGLVERTEX3SVPROC glad_glVertex3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX4DPROC glad_debug_glVertex4d;
#else
extern PFNGLVERTEX4DPROC glad_glVertex4d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX4DVPROC glad_debug_glVertex4dv;
#else
extern PFNGLVERTEX4DVPROC glad_glVertex4dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX4FPROC glad_debug_glVertex4f;
#else
extern PFNGLVERTEX4FPROC glad_glVertex4f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX4FVPROC glad_debug_glVertex4fv;
#else
extern PFNGLVERTEX4FVPROC glad_glVertex4fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX4IPROC glad_debug_glVertex4i;
#else
extern PFNGLVERTEX4IPROC glad_glVertex4i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX4IVPROC glad_debug_glVertex4iv;
#else
extern PFNGLVERTEX4IVPROC glad_glVertex4iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX4SPROC glad_debug_glVertex4s;
#else
extern PFNGLVERTEX4SPROC glad_glVertex4s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEX4SVPROC glad_debug_glVertex4sv;
#else
extern PFNGLVERTEX4SVPROC glad_glVertex4sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCLIPPLANEPROC glad_debug_glClipPlane;
#else
extern PFNGLCLIPPLANEPROC glad_glClipPlane;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLORMATERIALPROC glad_debug_glColorMaterial;
#else
extern PFNGLCOLORMATERIALPROC glad_glColorMaterial;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGFPROC glad_debug_glFogf;
#else
extern PFNGLFOGFPROC glad_glFogf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGFVPROC glad_debug_glFogfv;
#else
extern PFNGLFOGFVPROC glad_glFogfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGIPROC glad_debug_glFogi;
#else
extern PFNGLFOGIPROC glad_glFogi;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGIVPROC glad_debug_glFogiv;
#else
extern PFNGLFOGIVPROC glad_glFogiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLIGHTFPROC glad_debug_glLightf;
#else
extern PFNGLLIGHTFPROC glad_glLightf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLIGHTFVPROC glad_debug_glLightfv;
#else
extern PFNGLLIGHTFVPROC glad_glLightfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLIGHTIPROC glad_debug_glLighti;
#else
extern PFNGLLIGHTIPROC glad_glLighti;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLIGHTIVPROC glad_debug_glLightiv;
#else
extern PFNGLLIGHTIVPROC glad_glLightiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLIGHTMODELFPROC glad_debug_glLightModelf;
#else
extern PFNGLLIGHTMODELFPROC glad_glLightModelf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLIGHTMODELFVPROC glad_debug_glLightModelfv;
#else
extern PFNGLLIGHTMODELFVPROC glad_glLightModelfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLIGHTMODELIPROC glad_debug_glLightModeli;
#else
extern PFNGLLIGHTMODELIPROC glad_glLightModeli;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLIGHTMODELIVPROC glad_debug_glLightModeliv;
#else
extern PFNGLLIGHTMODELIVPROC glad_glLightModeliv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLINESTIPPLEPROC glad_debug_glLineStipple;
#else
extern PFNGLLINESTIPPLEPROC glad_glLineStipple;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMATERIALFPROC glad_debug_glMaterialf;
#else
extern PFNGLMATERIALFPROC glad_glMaterialf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMATERIALFVPROC glad_debug_glMaterialfv;
#else
extern PFNGLMATERIALFVPROC glad_glMaterialfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMATERIALIPROC glad_debug_glMateriali;
#else
extern PFNGLMATERIALIPROC glad_glMateriali;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMATERIALIVPROC glad_debug_glMaterialiv;
#else
extern PFNGLMATERIALIVPROC glad_glMaterialiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOLYGONSTIPPLEPROC glad_debug_glPolygonStipple;
#else
extern PFNGLPOLYGONSTIPPLEPROC glad_glPolygonStipple;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSHADEMODELPROC glad_debug_glShadeModel;
#else
extern PFNGLSHADEMODELPROC glad_glShadeModel;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXENVFPROC glad_debug_glTexEnvf;
#else
extern PFNGLTEXENVFPROC glad_glTexEnvf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXENVFVPROC glad_debug_glTexEnvfv;
#else
extern PFNGLTEXENVFVPROC glad_glTexEnvfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXENVIPROC glad_debug_glTexEnvi;
#else
extern PFNGLTEXENVIPROC glad_glTexEnvi;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXENVIVPROC glad_debug_glTexEnviv;
#else
extern PFNGLTEXENVIVPROC glad_glTexEnviv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXGENDPROC glad_debug_glTexGend;
#else
extern PFNGLTEXGENDPROC glad_glTexGend;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXGENDVPROC glad_debug_glTexGendv;
#else
extern PFNGLTEXGENDVPROC glad_glTexGendv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXGENFPROC glad_debug_glTexGenf;
#else
extern PFNGLTEXGENFPROC glad_glTexGenf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXGENFVPROC glad_debug_glTexGenfv;
#else
extern PFNGLTEXGENFVPROC glad_glTexGenfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXGENIPROC glad_debug_glTexGeni;
#else
extern PFNGLTEXGENIPROC glad_glTexGeni;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXGENIVPROC glad_debug_glTexGeniv;
#else
extern PFNGLTEXGENIVPROC glad_glTexGeniv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFEEDBACKBUFFERPROC glad_debug_glFeedbackBuffer;
#else
extern PFNGLFEEDBACKBUFFERPROC glad_glFeedbackBuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSELECTBUFFERPROC glad_debug_glSelectBuffer;
#else
extern PFNGLSELECTBUFFERPROC glad_glSelectBuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRENDERMODEPROC glad_debug_glRenderMode;
#else
extern PFNGLRENDERMODEPROC glad_glRenderMode;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINITNAMESPROC glad_debug_glInitNames;
#else
extern PFNGLINITNAMESPROC glad_glInitNames;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLOADNAMEPROC glad_debug_glLoadName;
#else
extern PFNGLLOADNAMEPROC glad_glLoadName;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPASSTHROUGHPROC glad_debug_glPassThrough;
#else
extern PFNGLPASSTHROUGHPROC glad_glPassThrough;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOPNAMEPROC glad_debug_glPopName;
#else
extern PFNGLPOPNAMEPROC glad_glPopName;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPUSHNAMEPROC glad_debug_glPushName;
#else
extern PFNGLPUSHNAMEPROC glad_glPushName;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCLEARACCUMPROC glad_debug_glClearAccum;
#else
extern PFNGLCLEARACCUMPROC glad_glClearAccum;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCLEARINDEXPROC glad_debug_glClearIndex;
#else
extern PFNGLCLEARINDEXPROC glad_glClearIndex;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXMASKPROC glad_debug_glIndexMask;
#else
extern PFNGLINDEXMASKPROC glad_glIndexMask;
#endif
#ifdef GLAD_DEBUG
extern PFNGLACCUMPROC glad_debug_glAccum;
#else
extern PFNGLACCUMPROC glad_glAccum;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOPATTRIBPROC glad_debug_glPopAttrib;
#else
extern PFNGLPOPATTRIBPROC glad_glPopAttrib;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPUSHATTRIBPROC glad_debug_glPushAttrib;
#else
extern PFNGLPUSHATTRIBPROC glad_glPushAttrib;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAP1DPROC glad_debug_glMap1d;
#else
extern PFNGLMAP1DPROC glad_glMap1d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAP1FPROC glad_debug_glMap1f;
#else
extern PFNGLMAP1FPROC glad_glMap1f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAP2DPROC glad_debug_glMap2d;
#else
extern PFNGLMAP2DPROC glad_glMap2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAP2FPROC glad_debug_glMap2f;
#else
extern PFNGLMAP2FPROC glad_glMap2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAPGRID1DPROC glad_debug_glMapGrid1d;
#else
extern PFNGLMAPGRID1DPROC glad_glMapGrid1d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAPGRID1FPROC glad_debug_glMapGrid1f;
#else
extern PFNGLMAPGRID1FPROC glad_glMapGrid1f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAPGRID2DPROC glad_debug_glMapGrid2d;
#else
extern PFNGLMAPGRID2DPROC glad_glMapGrid2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAPGRID2FPROC glad_debug_glMapGrid2f;
#else
extern PFNGLMAPGRID2FPROC glad_glMapGrid2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALCOORD1DPROC glad_debug_glEvalCoord1d;
#else
extern PFNGLEVALCOORD1DPROC glad_glEvalCoord1d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALCOORD1DVPROC glad_debug_glEvalCoord1dv;
#else
extern PFNGLEVALCOORD1DVPROC glad_glEvalCoord1dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALCOORD1FPROC glad_debug_glEvalCoord1f;
#else
extern PFNGLEVALCOORD1FPROC glad_glEvalCoord1f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALCOORD1FVPROC glad_debug_glEvalCoord1fv;
#else
extern PFNGLEVALCOORD1FVPROC glad_glEvalCoord1fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALCOORD2DPROC glad_debug_glEvalCoord2d;
#else
extern PFNGLEVALCOORD2DPROC glad_glEvalCoord2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALCOORD2DVPROC glad_debug_glEvalCoord2dv;
#else
extern PFNGLEVALCOORD2DVPROC glad_glEvalCoord2dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALCOORD2FPROC glad_debug_glEvalCoord2f;
#else
extern PFNGLEVALCOORD2FPROC glad_glEvalCoord2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALCOORD2FVPROC glad_debug_glEvalCoord2fv;
#else
extern PFNGLEVALCOORD2FVPROC glad_glEvalCoord2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALMESH1PROC glad_debug_glEvalMesh1;
#else
extern PFNGLEVALMESH1PROC glad_glEvalMesh1;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALPOINT1PROC glad_debug_glEvalPoint1;
#else
extern PFNGLEVALPOINT1PROC glad_glEvalPoint1;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALMESH2PROC glad_debug_glEvalMesh2;
#else
extern PFNGLEVALMESH2PROC glad_glEvalMesh2;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEVALPOINT2PROC glad_debug_glEvalPoint2;
#else
extern PFNGLEVALPOINT2PROC glad_glEvalPoint2;
#endif
#ifdef GLAD_DEBUG
extern PFNGLALPHAFUNCPROC glad_debug_glAlphaFunc;
#else
extern PFNGLALPHAFUNCPROC glad_glAlphaFunc;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPIXELZOOMPROC glad_debug_glPixelZoom;
#else
extern PFNGLPIXELZOOMPROC glad_glPixelZoom;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPIXELTRANSFERFPROC glad_debug_glPixelTransferf;
#else
extern PFNGLPIXELTRANSFERFPROC glad_glPixelTransferf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPIXELTRANSFERIPROC glad_debug_glPixelTransferi;
#else
extern PFNGLPIXELTRANSFERIPROC glad_glPixelTransferi;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPIXELMAPFVPROC glad_debug_glPixelMapfv;
#else
extern PFNGLPIXELMAPFVPROC glad_glPixelMapfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPIXELMAPUIVPROC glad_debug_glPixelMapuiv;
#else
extern PFNGLPIXELMAPUIVPROC glad_glPixelMapuiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPIXELMAPUSVPROC glad_debug_glPixelMapusv;
#else
extern PFNGLPIXELMAPUSVPROC glad_glPixelMapusv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOPYPIXELSPROC glad_debug_glCopyPixels;
#else
extern PFNGLCOPYPIXELSPROC glad_glCopyPixels;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDRAWPIXELSPROC glad_debug_glDrawPixels;
#else
extern PFNGLDRAWPIXELSPROC glad_glDrawPixels;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETCLIPPLANEPROC glad_debug_glGetClipPlane;
#else
extern PFNGLGETCLIPPLANEPROC glad_glGetClipPlane;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETLIGHTFVPROC glad_debug_glGetLightfv;
#else
extern PFNGLGETLIGHTFVPROC glad_glGetLightfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETLIGHTIVPROC glad_debug_glGetLightiv;
#else
extern PFNGLGETLIGHTIVPROC glad_glGetLightiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETMAPDVPROC glad_debug_glGetMapdv;
#else
extern PFNGLGETMAPDVPROC glad_glGetMapdv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETMAPFVPROC glad_debug_glGetMapfv;
#else
extern PFNGLGETMAPFVPROC glad_glGetMapfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETMAPIVPROC glad_debug_glGetMapiv;
#else
extern PFNGLGETMAPIVPROC glad_glGetMapiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETMATERIALFVPROC glad_debug_glGetMaterialfv;
#else
extern PFNGLGETMATERIALFVPROC glad_glGetMaterialfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETMATERIALIVPROC glad_debug_glGetMaterialiv;
#else
extern PFNGLGETMATERIALIVPROC glad_glGetMaterialiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETPIXELMAPFVPROC glad_debug_glGetPixelMapfv;
#else
extern PFNGLGETPIXELMAPFVPROC glad_glGetPixelMapfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETPIXELMAPUIVPROC glad_debug_glGetPixelMapuiv;
#else
extern PFNGLGETPIXELMAPUIVPROC glad_glGetPixelMapuiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETPIXELMAPUSVPROC glad_debug_glGetPixelMapusv;
#else
extern PFNGLGETPIXELMAPUSVPROC glad_glGetPixelMapusv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETPOLYGONSTIPPLEPROC glad_debug_glGetPolygonStipple;
#else
extern PFNGLGETPOLYGONSTIPPLEPROC glad_glGetPolygonStipple;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXENVFVPROC glad_debug_glGetTexEnvfv;
#else
extern PFNGLGETTEXENVFVPROC glad_glGetTexEnvfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXENVIVPROC glad_debug_glGetTexEnviv;
#else
extern PFNGLGETTEXENVIVPROC glad_glGetTexEnviv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXGENDVPROC glad_debug_glGetTexGendv;
#else
extern PFNGLGETTEXGENDVPROC glad_glGetTexGendv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXGENFVPROC glad_debug_glGetTexGenfv;
#else
extern PFNGLGETTEXGENFVPROC glad_glGetTexGenfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETTEXGENIVPROC glad_debug_glGetTexGeniv;
#else
extern PFNGLGETTEXGENIVPROC glad_glGetTexGeniv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISLISTPROC glad_debug_glIsList;
#else
extern PFNGLISLISTPROC glad_glIsList;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRUSTUMPROC glad_debug_glFrustum;
#else
extern PFNGLFRUSTUMPROC glad_glFrustum;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLOADIDENTITYPROC glad_debug_glLoadIdentity;
#else
extern PFNGLLOADIDENTITYPROC glad_glLoadIdentity;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLOADMATRIXFPROC glad_debug_glLoadMatrixf;
#else
extern PFNGLLOADMATRIXFPROC glad_glLoadMatrixf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLOADMATRIXDPROC glad_debug_glLoadMatrixd;
#else
extern PFNGLLOADMATRIXDPROC glad_glLoadMatrixd;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMATRIXMODEPROC glad_debug_glMatrixMode;
#else
extern PFNGLMATRIXMODEPROC glad_glMatrixMode;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTMATRIXFPROC glad_debug_glMultMatrixf;
#else
extern PFNGLMULTMATRIXFPROC glad_glMultMatrixf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTMATRIXDPROC glad_debug_glMultMatrixd;
#else
extern PFNGLMULTMATRIXDPROC glad_glMultMatrixd;
#endif
#ifdef GLAD_DEBUG
extern PFNGLORTHOPROC glad_debug_glOrtho;
#else
extern PFNGLORTHOPROC glad_glOrtho;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOPMATRIXPROC glad_debug_glPopMatrix;
#else
extern PFNGLPOPMATRIXPROC glad_glPopMatrix;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPUSHMATRIXPROC glad_debug_glPushMatrix;
#else
extern PFNGLPUSHMATRIXPROC glad_glPushMatrix;
#endif
#ifdef GLAD_DEBUG
extern PFNGLROTATEDPROC glad_debug_glRotated;
#else
extern PFNGLROTATEDPROC glad_glRotated;
#endif
#ifdef GLAD_DEBUG
extern PFNGLROTATEFPROC glad_debug_glRotatef;
#else
extern PFNGLROTATEFPROC glad_glRotatef;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSCALEDPROC glad_debug_glScaled;
#else
extern PFNGLSCALEDPROC glad_glScaled;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSCALEFPROC glad_debug_glScalef;
#else
extern PFNGLSCALEFPROC glad_glScalef;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTRANSLATEDPROC glad_debug_glTranslated;
#else
extern PFNGLTRANSLATEDPROC glad_glTranslated;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTRANSLATEFPROC glad_debug_glTranslatef;
#else
extern PFNGLTRANSLATEFPROC glad_glTranslatef;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDRAWARRAYSPROC glad_debug_glDrawArrays;
#else
extern PFNGLDRAWARRAYSPROC glad_glDrawArrays;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDRAWELEMENTSPROC glad_debug_glDrawElements;
#else
extern PFNGLDRAWELEMENTSPROC glad_glDrawElements;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETPOINTERVPROC glad_debug_glGetPointerv;
#else
extern PFNGLGETPOINTERVPROC glad_glGetPointerv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOLYGONOFFSETPROC glad_debug_glPolygonOffset;
#else
extern PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOPYTEXIMAGE1DPROC glad_debug_glCopyTexImage1D;
#else
extern PFNGLCOPYTEXIMAGE1DPROC glad_glCopyTexImage1D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOPYTEXIMAGE2DPROC glad_debug_glCopyTexImage2D;
#else
extern PFNGLCOPYTEXIMAGE2DPROC glad_glCopyTexImage2D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOPYTEXSUBIMAGE1DPROC glad_debug_glCopyTexSubImage1D;
#else
extern PFNGLCOPYTEXSUBIMAGE1DPROC glad_glCopyTexSubImage1D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOPYTEXSUBIMAGE2DPROC glad_debug_glCopyTexSubImage2D;
#else
extern PFNGLCOPYTEXSUBIMAGE2DPROC glad_glCopyTexSubImage2D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXSUBIMAGE1DPROC glad_debug_glTexSubImage1D;
#else
extern PFNGLTEXSUBIMAGE1DPROC glad_glTexSubImage1D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXSUBIMAGE2DPROC glad_debug_glTexSubImage2D;
#else
extern PFNGLTEXSUBIMAGE2DPROC glad_glTexSubImage2D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDTEXTUREPROC glad_debug_glBindTexture;
#else
extern PFNGLBINDTEXTUREPROC glad_glBindTexture;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETETEXTURESPROC glad_debug_glDeleteTextures;
#else
extern PFNGLDELETETEXTURESPROC glad_glDeleteTextures;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENTEXTURESPROC glad_debug_glGenTextures;
#else
extern PFNGLGENTEXTURESPROC glad_glGenTextures;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISTEXTUREPROC glad_debug_glIsTexture;
#else
extern PFNGLISTEXTUREPROC glad_glIsTexture;
#endif
#ifdef GLAD_DEBUG
extern PFNGLARRAYELEMENTPROC glad_debug_glArrayElement;
#else
extern PFNGLARRAYELEMENTPROC glad_glArrayElement;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOLORPOINTERPROC glad_debug_glColorPointer;
#else
extern PFNGLCOLORPOINTERPROC glad_glColorPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDISABLECLIENTSTATEPROC glad_debug_glDisableClientState;
#else
extern PFNGLDISABLECLIENTSTATEPROC glad_glDisableClientState;
#endif
#ifdef GLAD_DEBUG
extern PFNGLEDGEFLAGPOINTERPROC glad_debug_glEdgeFlagPointer;
#else
extern PFNGLEDGEFLAGPOINTERPROC glad_glEdgeFlagPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLENABLECLIENTSTATEPROC glad_debug_glEnableClientState;
#else
extern PFNGLENABLECLIENTSTATEPROC glad_glEnableClientState;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXPOINTERPROC glad_debug_glIndexPointer;
#else
extern PFNGLINDEXPOINTERPROC glad_glIndexPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINTERLEAVEDARRAYSPROC glad_debug_glInterleavedArrays;
#else
extern PFNGLINTERLEAVEDARRAYSPROC glad_glInterleavedArrays;
#endif
#ifdef GLAD_DEBUG
extern PFNGLNORMALPOINTERPROC glad_debug_glNormalPointer;
#else
extern PFNGLNORMALPOINTERPROC glad_glNormalPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXCOORDPOINTERPROC glad_debug_glTexCoordPointer;
#else
extern PFNGLTEXCOORDPOINTERPROC glad_glTexCoordPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXPOINTERPROC glad_debug_glVertexPointer;
#else
extern PFNGLVERTEXPOINTERPROC glad_glVertexPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLARETEXTURESRESIDENTPROC glad_debug_glAreTexturesResident;
#else
extern PFNGLARETEXTURESRESIDENTPROC glad_glAreTexturesResident;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPRIORITIZETEXTURESPROC glad_debug_glPrioritizeTextures;
#else
extern PFNGLPRIORITIZETEXTURESPROC glad_glPrioritizeTextures;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXUBPROC glad_debug_glIndexub;
#else
extern PFNGLINDEXUBPROC glad_glIndexub;
#endif
#ifdef GLAD_DEBUG
extern PFNGLINDEXUBVPROC glad_debug_glIndexubv;
#else
extern PFNGLINDEXUBVPROC glad_glIndexubv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOPCLIENTATTRIBPROC glad_debug_glPopClientAttrib;
#else
extern PFNGLPOPCLIENTATTRIBPROC glad_glPopClientAttrib;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPUSHCLIENTATTRIBPROC glad_debug_glPushClientAttrib;
#else
extern PFNGLPUSHCLIENTATTRIBPROC glad_glPushClientAttrib;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDRAWRANGEELEMENTSPROC glad_debug_glDrawRangeElements;
#else
extern PFNGLDRAWRANGEELEMENTSPROC glad_glDrawRangeElements;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXIMAGE3DPROC glad_debug_glTexImage3D;
#else
extern PFNGLTEXIMAGE3DPROC glad_glTexImage3D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLTEXSUBIMAGE3DPROC glad_debug_glTexSubImage3D;
#else
extern PFNGLTEXSUBIMAGE3DPROC glad_glTexSubImage3D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOPYTEXSUBIMAGE3DPROC glad_debug_glCopyTexSubImage3D;
#else
extern PFNGLCOPYTEXSUBIMAGE3DPROC glad_glCopyTexSubImage3D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLACTIVETEXTUREPROC glad_debug_glActiveTexture;
#else
extern PFNGLACTIVETEXTUREPROC glad_glActiveTexture;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSAMPLECOVERAGEPROC glad_debug_glSampleCoverage;
#else
extern PFNGLSAMPLECOVERAGEPROC glad_glSampleCoverage;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC glad_debug_glCompressedTexImage3D;
#else
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC glad_glCompressedTexImage3D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_debug_glCompressedTexImage2D;
#else
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_glCompressedTexImage2D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC glad_debug_glCompressedTexImage1D;
#else
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC glad_glCompressedTexImage1D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_debug_glCompressedTexSubImage3D;
#else
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glad_glCompressedTexSubImage3D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_debug_glCompressedTexSubImage2D;
#else
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_glCompressedTexSubImage2D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glad_debug_glCompressedTexSubImage1D;
#else
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glad_glCompressedTexSubImage1D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC glad_debug_glGetCompressedTexImage;
#else
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC glad_glGetCompressedTexImage;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCLIENTACTIVETEXTUREPROC glad_debug_glClientActiveTexture;
#else
extern PFNGLCLIENTACTIVETEXTUREPROC glad_glClientActiveTexture;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD1DPROC glad_debug_glMultiTexCoord1d;
#else
extern PFNGLMULTITEXCOORD1DPROC glad_glMultiTexCoord1d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD1DVPROC glad_debug_glMultiTexCoord1dv;
#else
extern PFNGLMULTITEXCOORD1DVPROC glad_glMultiTexCoord1dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD1FPROC glad_debug_glMultiTexCoord1f;
#else
extern PFNGLMULTITEXCOORD1FPROC glad_glMultiTexCoord1f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD1FVPROC glad_debug_glMultiTexCoord1fv;
#else
extern PFNGLMULTITEXCOORD1FVPROC glad_glMultiTexCoord1fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD1IPROC glad_debug_glMultiTexCoord1i;
#else
extern PFNGLMULTITEXCOORD1IPROC glad_glMultiTexCoord1i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD1IVPROC glad_debug_glMultiTexCoord1iv;
#else
extern PFNGLMULTITEXCOORD1IVPROC glad_glMultiTexCoord1iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD1SPROC glad_debug_glMultiTexCoord1s;
#else
extern PFNGLMULTITEXCOORD1SPROC glad_glMultiTexCoord1s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD1SVPROC glad_debug_glMultiTexCoord1sv;
#else
extern PFNGLMULTITEXCOORD1SVPROC glad_glMultiTexCoord1sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD2DPROC glad_debug_glMultiTexCoord2d;
#else
extern PFNGLMULTITEXCOORD2DPROC glad_glMultiTexCoord2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD2DVPROC glad_debug_glMultiTexCoord2dv;
#else
extern PFNGLMULTITEXCOORD2DVPROC glad_glMultiTexCoord2dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD2FPROC glad_debug_glMultiTexCoord2f;
#else
extern PFNGLMULTITEXCOORD2FPROC glad_glMultiTexCoord2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD2FVPROC glad_debug_glMultiTexCoord2fv;
#else
extern PFNGLMULTITEXCOORD2FVPROC glad_glMultiTexCoord2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD2IPROC glad_debug_glMultiTexCoord2i;
#else
extern PFNGLMULTITEXCOORD2IPROC glad_glMultiTexCoord2i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD2IVPROC glad_debug_glMultiTexCoord2iv;
#else
extern PFNGLMULTITEXCOORD2IVPROC glad_glMultiTexCoord2iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD2SPROC glad_debug_glMultiTexCoord2s;
#else
extern PFNGLMULTITEXCOORD2SPROC glad_glMultiTexCoord2s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD2SVPROC glad_debug_glMultiTexCoord2sv;
#else
extern PFNGLMULTITEXCOORD2SVPROC glad_glMultiTexCoord2sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD3DPROC glad_debug_glMultiTexCoord3d;
#else
extern PFNGLMULTITEXCOORD3DPROC glad_glMultiTexCoord3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD3DVPROC glad_debug_glMultiTexCoord3dv;
#else
extern PFNGLMULTITEXCOORD3DVPROC glad_glMultiTexCoord3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD3FPROC glad_debug_glMultiTexCoord3f;
#else
extern PFNGLMULTITEXCOORD3FPROC glad_glMultiTexCoord3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD3FVPROC glad_debug_glMultiTexCoord3fv;
#else
extern PFNGLMULTITEXCOORD3FVPROC glad_glMultiTexCoord3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD3IPROC glad_debug_glMultiTexCoord3i;
#else
extern PFNGLMULTITEXCOORD3IPROC glad_glMultiTexCoord3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD3IVPROC glad_debug_glMultiTexCoord3iv;
#else
extern PFNGLMULTITEXCOORD3IVPROC glad_glMultiTexCoord3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD3SPROC glad_debug_glMultiTexCoord3s;
#else
extern PFNGLMULTITEXCOORD3SPROC glad_glMultiTexCoord3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD3SVPROC glad_debug_glMultiTexCoord3sv;
#else
extern PFNGLMULTITEXCOORD3SVPROC glad_glMultiTexCoord3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD4DPROC glad_debug_glMultiTexCoord4d;
#else
extern PFNGLMULTITEXCOORD4DPROC glad_glMultiTexCoord4d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD4DVPROC glad_debug_glMultiTexCoord4dv;
#else
extern PFNGLMULTITEXCOORD4DVPROC glad_glMultiTexCoord4dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD4FPROC glad_debug_glMultiTexCoord4f;
#else
extern PFNGLMULTITEXCOORD4FPROC glad_glMultiTexCoord4f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD4FVPROC glad_debug_glMultiTexCoord4fv;
#else
extern PFNGLMULTITEXCOORD4FVPROC glad_glMultiTexCoord4fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD4IPROC glad_debug_glMultiTexCoord4i;
#else
extern PFNGLMULTITEXCOORD4IPROC glad_glMultiTexCoord4i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD4IVPROC glad_debug_glMultiTexCoord4iv;
#else
extern PFNGLMULTITEXCOORD4IVPROC glad_glMultiTexCoord4iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD4SPROC glad_debug_glMultiTexCoord4s;
#else
extern PFNGLMULTITEXCOORD4SPROC glad_glMultiTexCoord4s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTITEXCOORD4SVPROC glad_debug_glMultiTexCoord4sv;
#else
extern PFNGLMULTITEXCOORD4SVPROC glad_glMultiTexCoord4sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLOADTRANSPOSEMATRIXFPROC glad_debug_glLoadTransposeMatrixf;
#else
extern PFNGLLOADTRANSPOSEMATRIXFPROC glad_glLoadTransposeMatrixf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLOADTRANSPOSEMATRIXDPROC glad_debug_glLoadTransposeMatrixd;
#else
extern PFNGLLOADTRANSPOSEMATRIXDPROC glad_glLoadTransposeMatrixd;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTTRANSPOSEMATRIXFPROC glad_debug_glMultTransposeMatrixf;
#else
extern PFNGLMULTTRANSPOSEMATRIXFPROC glad_glMultTransposeMatrixf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTTRANSPOSEMATRIXDPROC glad_debug_glMultTransposeMatrixd;
#else
extern PFNGLMULTTRANSPOSEMATRIXDPROC glad_glMultTransposeMatrixd;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBLENDFUNCSEPARATEPROC glad_debug_glBlendFuncSeparate;
#else
extern PFNGLBLENDFUNCSEPARATEPROC glad_glBlendFuncSeparate;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTIDRAWARRAYSPROC glad_debug_glMultiDrawArrays;
#else
extern PFNGLMULTIDRAWARRAYSPROC glad_glMultiDrawArrays;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMULTIDRAWELEMENTSPROC glad_debug_glMultiDrawElements;
#else
extern PFNGLMULTIDRAWELEMENTSPROC glad_glMultiDrawElements;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOINTPARAMETERFPROC glad_debug_glPointParameterf;
#else
extern PFNGLPOINTPARAMETERFPROC glad_glPointParameterf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOINTPARAMETERFVPROC glad_debug_glPointParameterfv;
#else
extern PFNGLPOINTPARAMETERFVPROC glad_glPointParameterfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOINTPARAMETERIPROC glad_debug_glPointParameteri;
#else
extern PFNGLPOINTPARAMETERIPROC glad_glPointParameteri;
#endif
#ifdef GLAD_DEBUG
extern PFNGLPOINTPARAMETERIVPROC glad_debug_glPointParameteriv;
#else
extern PFNGLPOINTPARAMETERIVPROC glad_glPointParameteriv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGCOORDFPROC glad_debug_glFogCoordf;
#else
extern PFNGLFOGCOORDFPROC glad_glFogCoordf;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGCOORDFVPROC glad_debug_glFogCoordfv;
#else
extern PFNGLFOGCOORDFVPROC glad_glFogCoordfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGCOORDDPROC glad_debug_glFogCoordd;
#else
extern PFNGLFOGCOORDDPROC glad_glFogCoordd;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGCOORDDVPROC glad_debug_glFogCoorddv;
#else
extern PFNGLFOGCOORDDVPROC glad_glFogCoorddv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFOGCOORDPOINTERPROC glad_debug_glFogCoordPointer;
#else
extern PFNGLFOGCOORDPOINTERPROC glad_glFogCoordPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3BPROC glad_debug_glSecondaryColor3b;
#else
extern PFNGLSECONDARYCOLOR3BPROC glad_glSecondaryColor3b;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3BVPROC glad_debug_glSecondaryColor3bv;
#else
extern PFNGLSECONDARYCOLOR3BVPROC glad_glSecondaryColor3bv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3DPROC glad_debug_glSecondaryColor3d;
#else
extern PFNGLSECONDARYCOLOR3DPROC glad_glSecondaryColor3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3DVPROC glad_debug_glSecondaryColor3dv;
#else
extern PFNGLSECONDARYCOLOR3DVPROC glad_glSecondaryColor3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3FPROC glad_debug_glSecondaryColor3f;
#else
extern PFNGLSECONDARYCOLOR3FPROC glad_glSecondaryColor3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3FVPROC glad_debug_glSecondaryColor3fv;
#else
extern PFNGLSECONDARYCOLOR3FVPROC glad_glSecondaryColor3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3IPROC glad_debug_glSecondaryColor3i;
#else
extern PFNGLSECONDARYCOLOR3IPROC glad_glSecondaryColor3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3IVPROC glad_debug_glSecondaryColor3iv;
#else
extern PFNGLSECONDARYCOLOR3IVPROC glad_glSecondaryColor3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3SPROC glad_debug_glSecondaryColor3s;
#else
extern PFNGLSECONDARYCOLOR3SPROC glad_glSecondaryColor3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3SVPROC glad_debug_glSecondaryColor3sv;
#else
extern PFNGLSECONDARYCOLOR3SVPROC glad_glSecondaryColor3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3UBPROC glad_debug_glSecondaryColor3ub;
#else
extern PFNGLSECONDARYCOLOR3UBPROC glad_glSecondaryColor3ub;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3UBVPROC glad_debug_glSecondaryColor3ubv;
#else
extern PFNGLSECONDARYCOLOR3UBVPROC glad_glSecondaryColor3ubv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3UIPROC glad_debug_glSecondaryColor3ui;
#else
extern PFNGLSECONDARYCOLOR3UIPROC glad_glSecondaryColor3ui;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3UIVPROC glad_debug_glSecondaryColor3uiv;
#else
extern PFNGLSECONDARYCOLOR3UIVPROC glad_glSecondaryColor3uiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3USPROC glad_debug_glSecondaryColor3us;
#else
extern PFNGLSECONDARYCOLOR3USPROC glad_glSecondaryColor3us;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLOR3USVPROC glad_debug_glSecondaryColor3usv;
#else
extern PFNGLSECONDARYCOLOR3USVPROC glad_glSecondaryColor3usv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSECONDARYCOLORPOINTERPROC glad_debug_glSecondaryColorPointer;
#else
extern PFNGLSECONDARYCOLORPOINTERPROC glad_glSecondaryColorPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS2DPROC glad_debug_glWindowPos2d;
#else
extern PFNGLWINDOWPOS2DPROC glad_glWindowPos2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS2DVPROC glad_debug_glWindowPos2dv;
#else
extern PFNGLWINDOWPOS2DVPROC glad_glWindowPos2dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS2FPROC glad_debug_glWindowPos2f;
#else
extern PFNGLWINDOWPOS2FPROC glad_glWindowPos2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS2FVPROC glad_debug_glWindowPos2fv;
#else
extern PFNGLWINDOWPOS2FVPROC glad_glWindowPos2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS2IPROC glad_debug_glWindowPos2i;
#else
extern PFNGLWINDOWPOS2IPROC glad_glWindowPos2i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS2IVPROC glad_debug_glWindowPos2iv;
#else
extern PFNGLWINDOWPOS2IVPROC glad_glWindowPos2iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS2SPROC glad_debug_glWindowPos2s;
#else
extern PFNGLWINDOWPOS2SPROC glad_glWindowPos2s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS2SVPROC glad_debug_glWindowPos2sv;
#else
extern PFNGLWINDOWPOS2SVPROC glad_glWindowPos2sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS3DPROC glad_debug_glWindowPos3d;
#else
extern PFNGLWINDOWPOS3DPROC glad_glWindowPos3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS3DVPROC glad_debug_glWindowPos3dv;
#else
extern PFNGLWINDOWPOS3DVPROC glad_glWindowPos3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS3FPROC glad_debug_glWindowPos3f;
#else
extern PFNGLWINDOWPOS3FPROC glad_glWindowPos3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS3FVPROC glad_debug_glWindowPos3fv;
#else
extern PFNGLWINDOWPOS3FVPROC glad_glWindowPos3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS3IPROC glad_debug_glWindowPos3i;
#else
extern PFNGLWINDOWPOS3IPROC glad_glWindowPos3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS3IVPROC glad_debug_glWindowPos3iv;
#else
extern PFNGLWINDOWPOS3IVPROC glad_glWindowPos3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS3SPROC glad_debug_glWindowPos3s;
#else
extern PFNGLWINDOWPOS3SPROC glad_glWindowPos3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLWINDOWPOS3SVPROC glad_debug_glWindowPos3sv;
#else
extern PFNGLWINDOWPOS3SVPROC glad_glWindowPos3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBLENDCOLORPROC glad_debug_glBlendColor;
#else
extern PFNGLBLENDCOLORPROC glad_glBlendColor;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBLENDEQUATIONPROC glad_debug_glBlendEquation;
#else
extern PFNGLBLENDEQUATIONPROC glad_glBlendEquation;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENQUERIESPROC glad_debug_glGenQueries;
#else
extern PFNGLGENQUERIESPROC glad_glGenQueries;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETEQUERIESPROC glad_debug_glDeleteQueries;
#else
extern PFNGLDELETEQUERIESPROC glad_glDeleteQueries;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISQUERYPROC glad_debug_glIsQuery;
#else
extern PFNGLISQUERYPROC glad_glIsQuery;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBEGINQUERYPROC glad_debug_glBeginQuery;
#else
extern PFNGLBEGINQUERYPROC glad_glBeginQuery;
#endif
#ifdef GLAD_DEBUG
extern PFNGLENDQUERYPROC glad_debug_glEndQuery;
#else
extern PFNGLENDQUERYPROC glad_glEndQuery;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETQUERYIVPROC glad_debug_glGetQueryiv;
#else
extern PFNGLGETQUERYIVPROC glad_glGetQueryiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETQUERYOBJECTIVPROC glad_debug_glGetQueryObjectiv;
#else
extern PFNGLGETQUERYOBJECTIVPROC glad_glGetQueryObjectiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETQUERYOBJECTUIVPROC glad_debug_glGetQueryObjectuiv;
#else
extern PFNGLGETQUERYOBJECTUIVPROC glad_glGetQueryObjectuiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDBUFFERPROC glad_debug_glBindBuffer;
#else
extern PFNGLBINDBUFFERPROC glad_glBindBuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETEBUFFERSPROC glad_debug_glDeleteBuffers;
#else
extern PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENBUFFERSPROC glad_debug_glGenBuffers;
#else
extern PFNGLGENBUFFERSPROC glad_glGenBuffers;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISBUFFERPROC glad_debug_glIsBuffer;
#else
extern PFNGLISBUFFERPROC glad_glIsBuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBUFFERDATAPROC glad_debug_glBufferData;
#else
extern PFNGLBUFFERDATAPROC glad_glBufferData;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBUFFERSUBDATAPROC glad_debug_glBufferSubData;
#else
extern PFNGLBUFFERSUBDATAPROC glad_glBufferSubData;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETBUFFERSUBDATAPROC glad_debug_glGetBufferSubData;
#else
extern PFNGLGETBUFFERSUBDATAPROC glad_glGetBufferSubData;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAPBUFFERPROC glad_debug_glMapBuffer;
#else
extern PFNGLMAPBUFFERPROC glad_glMapBuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNMAPBUFFERPROC glad_debug_glUnmapBuffer;
#else
extern PFNGLUNMAPBUFFERPROC glad_glUnmapBuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETBUFFERPARAMETERIVPROC glad_debug_glGetBufferParameteriv;
#else
extern PFNGLGETBUFFERPARAMETERIVPROC glad_glGetBufferParameteriv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETBUFFERPOINTERVPROC glad_debug_glGetBufferPointerv;
#else
extern PFNGLGETBUFFERPOINTERVPROC glad_glGetBufferPointerv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBLENDEQUATIONSEPARATEPROC glad_debug_glBlendEquationSeparate;
#else
extern PFNGLBLENDEQUATIONSEPARATEPROC glad_glBlendEquationSeparate;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDRAWBUFFERSPROC glad_debug_glDrawBuffers;
#else
extern PFNGLDRAWBUFFERSPROC glad_glDrawBuffers;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSTENCILOPSEPARATEPROC glad_debug_glStencilOpSeparate;
#else
extern PFNGLSTENCILOPSEPARATEPROC glad_glStencilOpSeparate;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSTENCILFUNCSEPARATEPROC glad_debug_glStencilFuncSeparate;
#else
extern PFNGLSTENCILFUNCSEPARATEPROC glad_glStencilFuncSeparate;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSTENCILMASKSEPARATEPROC glad_debug_glStencilMaskSeparate;
#else
extern PFNGLSTENCILMASKSEPARATEPROC glad_glStencilMaskSeparate;
#endif
#ifdef GLAD_DEBUG
extern PFNGLATTACHSHADERPROC glad_debug_glAttachShader;
#else
extern PFNGLATTACHSHADERPROC glad_glAttachShader;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDATTRIBLOCATIONPROC glad_debug_glBindAttribLocation;
#else
extern PFNGLBINDATTRIBLOCATIONPROC glad_glBindAttribLocation;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCOMPILESHADERPROC glad_debug_glCompileShader;
#else
extern PFNGLCOMPILESHADERPROC glad_glCompileShader;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCREATEPROGRAMPROC glad_debug_glCreateProgram;
#else
extern PFNGLCREATEPROGRAMPROC glad_glCreateProgram;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCREATESHADERPROC glad_debug_glCreateShader;
#else
extern PFNGLCREATESHADERPROC glad_glCreateShader;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETEPROGRAMPROC glad_debug_glDeleteProgram;
#else
extern PFNGLDELETEPROGRAMPROC glad_glDeleteProgram;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETESHADERPROC glad_debug_glDeleteShader;
#else
extern PFNGLDELETESHADERPROC glad_glDeleteShader;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDETACHSHADERPROC glad_debug_glDetachShader;
#else
extern PFNGLDETACHSHADERPROC glad_glDetachShader;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_debug_glDisableVertexAttribArray;
#else
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray;
#endif
#ifdef GLAD_DEBUG
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glad_debug_glEnableVertexAttribArray;
#else
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETACTIVEATTRIBPROC glad_debug_glGetActiveAttrib;
#else
extern PFNGLGETACTIVEATTRIBPROC glad_glGetActiveAttrib;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETACTIVEUNIFORMPROC glad_debug_glGetActiveUniform;
#else
extern PFNGLGETACTIVEUNIFORMPROC glad_glGetActiveUniform;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETATTACHEDSHADERSPROC glad_debug_glGetAttachedShaders;
#else
extern PFNGLGETATTACHEDSHADERSPROC glad_glGetAttachedShaders;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETATTRIBLOCATIONPROC glad_debug_glGetAttribLocation;
#else
extern PFNGLGETATTRIBLOCATIONPROC glad_glGetAttribLocation;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETPROGRAMIVPROC glad_debug_glGetProgramiv;
#else
extern PFNGLGETPROGRAMIVPROC glad_glGetProgramiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETPROGRAMINFOLOGPROC glad_debug_glGetProgramInfoLog;
#else
extern PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETSHADERIVPROC glad_debug_glGetShaderiv;
#else
extern PFNGLGETSHADERIVPROC glad_glGetShaderiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETSHADERINFOLOGPROC glad_debug_glGetShaderInfoLog;
#else
extern PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETSHADERSOURCEPROC glad_debug_glGetShaderSource;
#else
extern PFNGLGETSHADERSOURCEPROC glad_glGetShaderSource;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETUNIFORMLOCATIONPROC glad_debug_glGetUniformLocation;
#else
extern PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETUNIFORMFVPROC glad_debug_glGetUniformfv;
#else
extern PFNGLGETUNIFORMFVPROC glad_glGetUniformfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETUNIFORMIVPROC glad_debug_glGetUniformiv;
#else
extern PFNGLGETUNIFORMIVPROC glad_glGetUniformiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETVERTEXATTRIBDVPROC glad_debug_glGetVertexAttribdv;
#else
extern PFNGLGETVERTEXATTRIBDVPROC glad_glGetVertexAttribdv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETVERTEXATTRIBFVPROC glad_debug_glGetVertexAttribfv;
#else
extern PFNGLGETVERTEXATTRIBFVPROC glad_glGetVertexAttribfv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETVERTEXATTRIBIVPROC glad_debug_glGetVertexAttribiv;
#else
extern PFNGLGETVERTEXATTRIBIVPROC glad_glGetVertexAttribiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETVERTEXATTRIBPOINTERVPROC glad_debug_glGetVertexAttribPointerv;
#else
extern PFNGLGETVERTEXATTRIBPOINTERVPROC glad_glGetVertexAttribPointerv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISPROGRAMPROC glad_debug_glIsProgram;
#else
extern PFNGLISPROGRAMPROC glad_glIsProgram;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISSHADERPROC glad_debug_glIsShader;
#else
extern PFNGLISSHADERPROC glad_glIsShader;
#endif
#ifdef GLAD_DEBUG
extern PFNGLLINKPROGRAMPROC glad_debug_glLinkProgram;
#else
extern PFNGLLINKPROGRAMPROC glad_glLinkProgram;
#endif
#ifdef GLAD_DEBUG
extern PFNGLSHADERSOURCEPROC glad_debug_glShaderSource;
#else
extern PFNGLSHADERSOURCEPROC glad_glShaderSource;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUSEPROGRAMPROC glad_debug_glUseProgram;
#else
extern PFNGLUSEPROGRAMPROC glad_glUseProgram;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM1FPROC glad_debug_glUniform1f;
#else
extern PFNGLUNIFORM1FPROC glad_glUniform1f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM2FPROC glad_debug_glUniform2f;
#else
extern PFNGLUNIFORM2FPROC glad_glUniform2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM3FPROC glad_debug_glUniform3f;
#else
extern PFNGLUNIFORM3FPROC glad_glUniform3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM4FPROC glad_debug_glUniform4f;
#else
extern PFNGLUNIFORM4FPROC glad_glUniform4f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM1IPROC glad_debug_glUniform1i;
#else
extern PFNGLUNIFORM1IPROC glad_glUniform1i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM2IPROC glad_debug_glUniform2i;
#else
extern PFNGLUNIFORM2IPROC glad_glUniform2i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM3IPROC glad_debug_glUniform3i;
#else
extern PFNGLUNIFORM3IPROC glad_glUniform3i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM4IPROC glad_debug_glUniform4i;
#else
extern PFNGLUNIFORM4IPROC glad_glUniform4i;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM1FVPROC glad_debug_glUniform1fv;
#else
extern PFNGLUNIFORM1FVPROC glad_glUniform1fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM2FVPROC glad_debug_glUniform2fv;
#else
extern PFNGLUNIFORM2FVPROC glad_glUniform2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM3FVPROC glad_debug_glUniform3fv;
#else
extern PFNGLUNIFORM3FVPROC glad_glUniform3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM4FVPROC glad_debug_glUniform4fv;
#else
extern PFNGLUNIFORM4FVPROC glad_glUniform4fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM1IVPROC glad_debug_glUniform1iv;
#else
extern PFNGLUNIFORM1IVPROC glad_glUniform1iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM2IVPROC glad_debug_glUniform2iv;
#else
extern PFNGLUNIFORM2IVPROC glad_glUniform2iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM3IVPROC glad_debug_glUniform3iv;
#else
extern PFNGLUNIFORM3IVPROC glad_glUniform3iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORM4IVPROC glad_debug_glUniform4iv;
#else
extern PFNGLUNIFORM4IVPROC glad_glUniform4iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORMMATRIX2FVPROC glad_debug_glUniformMatrix2fv;
#else
extern PFNGLUNIFORMMATRIX2FVPROC glad_glUniformMatrix2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORMMATRIX3FVPROC glad_debug_glUniformMatrix3fv;
#else
extern PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNIFORMMATRIX4FVPROC glad_debug_glUniformMatrix4fv;
#else
extern PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVALIDATEPROGRAMPROC glad_debug_glValidateProgram;
#else
extern PFNGLVALIDATEPROGRAMPROC glad_glValidateProgram;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB1DPROC glad_debug_glVertexAttrib1d;
#else
extern PFNGLVERTEXATTRIB1DPROC glad_glVertexAttrib1d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB1DVPROC glad_debug_glVertexAttrib1dv;
#else
extern PFNGLVERTEXATTRIB1DVPROC glad_glVertexAttrib1dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB1FPROC glad_debug_glVertexAttrib1f;
#else
extern PFNGLVERTEXATTRIB1FPROC glad_glVertexAttrib1f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB1FVPROC glad_debug_glVertexAttrib1fv;
#else
extern PFNGLVERTEXATTRIB1FVPROC glad_glVertexAttrib1fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB1SPROC glad_debug_glVertexAttrib1s;
#else
extern PFNGLVERTEXATTRIB1SPROC glad_glVertexAttrib1s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB1SVPROC glad_debug_glVertexAttrib1sv;
#else
extern PFNGLVERTEXATTRIB1SVPROC glad_glVertexAttrib1sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB2DPROC glad_debug_glVertexAttrib2d;
#else
extern PFNGLVERTEXATTRIB2DPROC glad_glVertexAttrib2d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB2DVPROC glad_debug_glVertexAttrib2dv;
#else
extern PFNGLVERTEXATTRIB2DVPROC glad_glVertexAttrib2dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB2FPROC glad_debug_glVertexAttrib2f;
#else
extern PFNGLVERTEXATTRIB2FPROC glad_glVertexAttrib2f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB2FVPROC glad_debug_glVertexAttrib2fv;
#else
extern PFNGLVERTEXATTRIB2FVPROC glad_glVertexAttrib2fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB2SPROC glad_debug_glVertexAttrib2s;
#else
extern PFNGLVERTEXATTRIB2SPROC glad_glVertexAttrib2s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB2SVPROC glad_debug_glVertexAttrib2sv;
#else
extern PFNGLVERTEXATTRIB2SVPROC glad_glVertexAttrib2sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB3DPROC glad_debug_glVertexAttrib3d;
#else
extern PFNGLVERTEXATTRIB3DPROC glad_glVertexAttrib3d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB3DVPROC glad_debug_glVertexAttrib3dv;
#else
extern PFNGLVERTEXATTRIB3DVPROC glad_glVertexAttrib3dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB3FPROC glad_debug_glVertexAttrib3f;
#else
extern PFNGLVERTEXATTRIB3FPROC glad_glVertexAttrib3f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB3FVPROC glad_debug_glVertexAttrib3fv;
#else
extern PFNGLVERTEXATTRIB3FVPROC glad_glVertexAttrib3fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB3SPROC glad_debug_glVertexAttrib3s;
#else
extern PFNGLVERTEXATTRIB3SPROC glad_glVertexAttrib3s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB3SVPROC glad_debug_glVertexAttrib3sv;
#else
extern PFNGLVERTEXATTRIB3SVPROC glad_glVertexAttrib3sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4NBVPROC glad_debug_glVertexAttrib4Nbv;
#else
extern PFNGLVERTEXATTRIB4NBVPROC glad_glVertexAttrib4Nbv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4NIVPROC glad_debug_glVertexAttrib4Niv;
#else
extern PFNGLVERTEXATTRIB4NIVPROC glad_glVertexAttrib4Niv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4NSVPROC glad_debug_glVertexAttrib4Nsv;
#else
extern PFNGLVERTEXATTRIB4NSVPROC glad_glVertexAttrib4Nsv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4NUBPROC glad_debug_glVertexAttrib4Nub;
#else
extern PFNGLVERTEXATTRIB4NUBPROC glad_glVertexAttrib4Nub;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4NUBVPROC glad_debug_glVertexAttrib4Nubv;
#else
extern PFNGLVERTEXATTRIB4NUBVPROC glad_glVertexAttrib4Nubv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4NUIVPROC glad_debug_glVertexAttrib4Nuiv;
#else
extern PFNGLVERTEXATTRIB4NUIVPROC glad_glVertexAttrib4Nuiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4NUSVPROC glad_debug_glVertexAttrib4Nusv;
#else
extern PFNGLVERTEXATTRIB4NUSVPROC glad_glVertexAttrib4Nusv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4BVPROC glad_debug_glVertexAttrib4bv;
#else
extern PFNGLVERTEXATTRIB4BVPROC glad_glVertexAttrib4bv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4DPROC glad_debug_glVertexAttrib4d;
#else
extern PFNGLVERTEXATTRIB4DPROC glad_glVertexAttrib4d;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4DVPROC glad_debug_glVertexAttrib4dv;
#else
extern PFNGLVERTEXATTRIB4DVPROC glad_glVertexAttrib4dv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4FPROC glad_debug_glVertexAttrib4f;
#else
extern PFNGLVERTEXATTRIB4FPROC glad_glVertexAttrib4f;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4FVPROC glad_debug_glVertexAttrib4fv;
#else
extern PFNGLVERTEXATTRIB4FVPROC glad_glVertexAttrib4fv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4IVPROC glad_debug_glVertexAttrib4iv;
#else
extern PFNGLVERTEXATTRIB4IVPROC glad_glVertexAttrib4iv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4SPROC glad_debug_glVertexAttrib4s;
#else
extern PFNGLVERTEXATTRIB4SPROC glad_glVertexAttrib4s;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4SVPROC glad_debug_glVertexAttrib4sv;
#else
extern PFNGLVERTEXATTRIB4SVPROC glad_glVertexAttrib4sv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4UBVPROC glad_debug_glVertexAttrib4ubv;
#else
extern PFNGLVERTEXATTRIB4UBVPROC glad_glVertexAttrib4ubv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4UIVPROC glad_debug_glVertexAttrib4uiv;
#else
extern PFNGLVERTEXATTRIB4UIVPROC glad_glVertexAttrib4uiv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIB4USVPROC glad_debug_glVertexAttrib4usv;
#else
extern PFNGLVERTEXATTRIB4USVPROC glad_glVertexAttrib4usv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLVERTEXATTRIBPOINTERPROC glad_debug_glVertexAttribPointer;
#else
extern PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDBUFFERARBPROC glad_debug_glBindBufferARB;
#else
extern PFNGLBINDBUFFERARBPROC glad_glBindBufferARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETEBUFFERSARBPROC glad_debug_glDeleteBuffersARB;
#else
extern PFNGLDELETEBUFFERSARBPROC glad_glDeleteBuffersARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENBUFFERSARBPROC glad_debug_glGenBuffersARB;
#else
extern PFNGLGENBUFFERSARBPROC glad_glGenBuffersARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISBUFFERARBPROC glad_debug_glIsBufferARB;
#else
extern PFNGLISBUFFERARBPROC glad_glIsBufferARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBUFFERDATAARBPROC glad_debug_glBufferDataARB;
#else
extern PFNGLBUFFERDATAARBPROC glad_glBufferDataARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBUFFERSUBDATAARBPROC glad_debug_glBufferSubDataARB;
#else
extern PFNGLBUFFERSUBDATAARBPROC glad_glBufferSubDataARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETBUFFERSUBDATAARBPROC glad_debug_glGetBufferSubDataARB;
#else
extern PFNGLGETBUFFERSUBDATAARBPROC glad_glGetBufferSubDataARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLMAPBUFFERARBPROC glad_debug_glMapBufferARB;
#else
extern PFNGLMAPBUFFERARBPROC glad_glMapBufferARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLUNMAPBUFFERARBPROC glad_debug_glUnmapBufferARB;
#else
extern PFNGLUNMAPBUFFERARBPROC glad_glUnmapBufferARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETBUFFERPARAMETERIVARBPROC glad_debug_glGetBufferParameterivARB;
#else
extern PFNGLGETBUFFERPARAMETERIVARBPROC glad_glGetBufferParameterivARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETBUFFERPOINTERVARBPROC glad_debug_glGetBufferPointervARB;
#else
extern PFNGLGETBUFFERPOINTERVARBPROC glad_glGetBufferPointervARB;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDVERTEXARRAYPROC glad_debug_glBindVertexArray;
#else
extern PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETEVERTEXARRAYSPROC glad_debug_glDeleteVertexArrays;
#else
extern PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENVERTEXARRAYSPROC glad_debug_glGenVertexArrays;
#else
extern PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISVERTEXARRAYPROC glad_debug_glIsVertexArray;
#else
extern PFNGLISVERTEXARRAYPROC glad_glIsVertexArray;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISRENDERBUFFERPROC glad_debug_glIsRenderbuffer;
#else
extern PFNGLISRENDERBUFFERPROC glad_glIsRenderbuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDRENDERBUFFERPROC glad_debug_glBindRenderbuffer;
#else
extern PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETERENDERBUFFERSPROC glad_debug_glDeleteRenderbuffers;
#else
extern PFNGLDELETERENDERBUFFERSPROC glad_glDeleteRenderbuffers;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENRENDERBUFFERSPROC glad_debug_glGenRenderbuffers;
#else
extern PFNGLGENRENDERBUFFERSPROC glad_glGenRenderbuffers;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRENDERBUFFERSTORAGEPROC glad_debug_glRenderbufferStorage;
#else
extern PFNGLRENDERBUFFERSTORAGEPROC glad_glRenderbufferStorage;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_debug_glGetRenderbufferParameteriv;
#else
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_glGetRenderbufferParameteriv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISFRAMEBUFFERPROC glad_debug_glIsFramebuffer;
#else
extern PFNGLISFRAMEBUFFERPROC glad_glIsFramebuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDFRAMEBUFFERPROC glad_debug_glBindFramebuffer;
#else
extern PFNGLBINDFRAMEBUFFERPROC glad_glBindFramebuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETEFRAMEBUFFERSPROC glad_debug_glDeleteFramebuffers;
#else
extern PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENFRAMEBUFFERSPROC glad_debug_glGenFramebuffers;
#else
extern PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_debug_glCheckFramebufferStatus;
#else
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERTEXTURE1DPROC glad_debug_glFramebufferTexture1D;
#else
extern PFNGLFRAMEBUFFERTEXTURE1DPROC glad_glFramebufferTexture1D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glad_debug_glFramebufferTexture2D;
#else
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERTEXTURE3DPROC glad_debug_glFramebufferTexture3D;
#else
extern PFNGLFRAMEBUFFERTEXTURE3DPROC glad_glFramebufferTexture3D;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_debug_glFramebufferRenderbuffer;
#else
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_debug_glGetFramebufferAttachmentParameteriv;
#else
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_glGetFramebufferAttachmentParameteriv;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENERATEMIPMAPPROC glad_debug_glGenerateMipmap;
#else
extern PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBLITFRAMEBUFFERPROC glad_debug_glBlitFramebuffer;
#else
extern PFNGLBLITFRAMEBUFFERPROC glad_glBlitFramebuffer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_debug_glRenderbufferStorageMultisample;
#else
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glad_glRenderbufferStorageMultisample;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glad_debug_glFramebufferTextureLayer;
#else
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glad_glFramebufferTextureLayer;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISRENDERBUFFEREXTPROC glad_debug_glIsRenderbufferEXT;
#else
extern PFNGLISRENDERBUFFEREXTPROC glad_glIsRenderbufferEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDRENDERBUFFEREXTPROC glad_debug_glBindRenderbufferEXT;
#else
extern PFNGLBINDRENDERBUFFEREXTPROC glad_glBindRenderbufferEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETERENDERBUFFERSEXTPROC glad_debug_glDeleteRenderbuffersEXT;
#else
extern PFNGLDELETERENDERBUFFERSEXTPROC glad_glDeleteRenderbuffersEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENRENDERBUFFERSEXTPROC glad_debug_glGenRenderbuffersEXT;
#else
extern PFNGLGENRENDERBUFFERSEXTPROC glad_glGenRenderbuffersEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glad_debug_glRenderbufferStorageEXT;
#else
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glad_glRenderbufferStorageEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glad_debug_glGetRenderbufferParameterivEXT;
#else
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glad_glGetRenderbufferParameterivEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISFRAMEBUFFEREXTPROC glad_debug_glIsFramebufferEXT;
#else
extern PFNGLISFRAMEBUFFEREXTPROC glad_glIsFramebufferEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDFRAMEBUFFEREXTPROC glad_debug_glBindFramebufferEXT;
#else
extern PFNGLBINDFRAMEBUFFEREXTPROC glad_glBindFramebufferEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glad_debug_glDeleteFramebuffersEXT;
#else
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glad_glDeleteFramebuffersEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENFRAMEBUFFERSEXTPROC glad_debug_glGenFramebuffersEXT;
#else
extern PFNGLGENFRAMEBUFFERSEXTPROC glad_glGenFramebuffersEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glad_debug_glCheckFramebufferStatusEXT;
#else
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glad_glCheckFramebufferStatusEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glad_debug_glFramebufferTexture1DEXT;
#else
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glad_glFramebufferTexture1DEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glad_debug_glFramebufferTexture2DEXT;
#else
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glad_glFramebufferTexture2DEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glad_debug_glFramebufferTexture3DEXT;
#else
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glad_glFramebufferTexture3DEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glad_debug_glFramebufferRenderbufferEXT;
#else
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glad_glFramebufferRenderbufferEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_debug_glGetFramebufferAttachmentParameterivEXT;
#else
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_glGetFramebufferAttachmentParameterivEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENERATEMIPMAPEXTPROC glad_debug_glGenerateMipmapEXT;
#else
extern PFNGLGENERATEMIPMAPEXTPROC glad_glGenerateMipmapEXT;
#endif
#ifdef GLAD_DEBUG
extern PFNGLBINDVERTEXARRAYAPPLEPROC glad_debug_glBindVertexArrayAPPLE;
#else
extern PFNGLBINDVERTEXARRAYAPPLEPROC glad_glBindVertexArrayAPPLE;
#endif
#ifdef GLAD_DEBUG
extern PFNGLDELETEVERTEXARRAYSAPPLEPROC glad_debug_glDeleteVertexArraysAPPLE;
#else
extern PFNGLDELETEVERTEXARRAYSAPPLEPROC glad_glDeleteVertexArraysAPPLE;
#endif
#ifdef GLAD_DEBUG
extern PFNGLGENVERTEXARRAYSAPPLEPROC glad_debug_glGenVertexArraysAPPLE;
#else
extern PFNGLGENVERTEXARRAYSAPPLEPROC glad_glGenVertexArraysAPPLE;
#endif
#ifdef GLAD_DEBUG
extern PFNGLISVERTEXARRAYAPPLEPROC glad_debug_glIsVertexArrayAPPLE;
#else
extern PFNGLISVERTEXARRAYAPPLEPROC glad_glIsVertexArrayAPPLE;
#endif
} // extern C

namespace Natron {

template <>
void OSGLFunctions<true>::load_functions() {
#ifdef GLAD_DEBUG
    _glCullFace = glad_debug_glCullFace;
#else
    _glCullFace = glad_glCullFace;
#endif
#ifdef GLAD_DEBUG
    _glFrontFace = glad_debug_glFrontFace;
#else
    _glFrontFace = glad_glFrontFace;
#endif
#ifdef GLAD_DEBUG
    _glHint = glad_debug_glHint;
#else
    _glHint = glad_glHint;
#endif
#ifdef GLAD_DEBUG
    _glLineWidth = glad_debug_glLineWidth;
#else
    _glLineWidth = glad_glLineWidth;
#endif
#ifdef GLAD_DEBUG
    _glPointSize = glad_debug_glPointSize;
#else
    _glPointSize = glad_glPointSize;
#endif
#ifdef GLAD_DEBUG
    _glPolygonMode = glad_debug_glPolygonMode;
#else
    _glPolygonMode = glad_glPolygonMode;
#endif
#ifdef GLAD_DEBUG
    _glScissor = glad_debug_glScissor;
#else
    _glScissor = glad_glScissor;
#endif
#ifdef GLAD_DEBUG
    _glTexParameterf = glad_debug_glTexParameterf;
#else
    _glTexParameterf = glad_glTexParameterf;
#endif
#ifdef GLAD_DEBUG
    _glTexParameterfv = glad_debug_glTexParameterfv;
#else
    _glTexParameterfv = glad_glTexParameterfv;
#endif
#ifdef GLAD_DEBUG
    _glTexParameteri = glad_debug_glTexParameteri;
#else
    _glTexParameteri = glad_glTexParameteri;
#endif
#ifdef GLAD_DEBUG
    _glTexParameteriv = glad_debug_glTexParameteriv;
#else
    _glTexParameteriv = glad_glTexParameteriv;
#endif
#ifdef GLAD_DEBUG
    _glTexImage1D = glad_debug_glTexImage1D;
#else
    _glTexImage1D = glad_glTexImage1D;
#endif
#ifdef GLAD_DEBUG
    _glTexImage2D = glad_debug_glTexImage2D;
#else
    _glTexImage2D = glad_glTexImage2D;
#endif
#ifdef GLAD_DEBUG
    _glDrawBuffer = glad_debug_glDrawBuffer;
#else
    _glDrawBuffer = glad_glDrawBuffer;
#endif
#ifdef GLAD_DEBUG
    _glClear = glad_debug_glClear;
#else
    _glClear = glad_glClear;
#endif
#ifdef GLAD_DEBUG
    _glClearColor = glad_debug_glClearColor;
#else
    _glClearColor = glad_glClearColor;
#endif
#ifdef GLAD_DEBUG
    _glClearStencil = glad_debug_glClearStencil;
#else
    _glClearStencil = glad_glClearStencil;
#endif
#ifdef GLAD_DEBUG
    _glClearDepth = glad_debug_glClearDepth;
#else
    _glClearDepth = glad_glClearDepth;
#endif
#ifdef GLAD_DEBUG
    _glStencilMask = glad_debug_glStencilMask;
#else
    _glStencilMask = glad_glStencilMask;
#endif
#ifdef GLAD_DEBUG
    _glColorMask = glad_debug_glColorMask;
#else
    _glColorMask = glad_glColorMask;
#endif
#ifdef GLAD_DEBUG
    _glDepthMask = glad_debug_glDepthMask;
#else
    _glDepthMask = glad_glDepthMask;
#endif
#ifdef GLAD_DEBUG
    _glDisable = glad_debug_glDisable;
#else
    _glDisable = glad_glDisable;
#endif
#ifdef GLAD_DEBUG
    _glEnable = glad_debug_glEnable;
#else
    _glEnable = glad_glEnable;
#endif
#ifdef GLAD_DEBUG
    _glFinish = glad_debug_glFinish;
#else
    _glFinish = glad_glFinish;
#endif
#ifdef GLAD_DEBUG
    _glFlush = glad_debug_glFlush;
#else
    _glFlush = glad_glFlush;
#endif
#ifdef GLAD_DEBUG
    _glBlendFunc = glad_debug_glBlendFunc;
#else
    _glBlendFunc = glad_glBlendFunc;
#endif
#ifdef GLAD_DEBUG
    _glLogicOp = glad_debug_glLogicOp;
#else
    _glLogicOp = glad_glLogicOp;
#endif
#ifdef GLAD_DEBUG
    _glStencilFunc = glad_debug_glStencilFunc;
#else
    _glStencilFunc = glad_glStencilFunc;
#endif
#ifdef GLAD_DEBUG
    _glStencilOp = glad_debug_glStencilOp;
#else
    _glStencilOp = glad_glStencilOp;
#endif
#ifdef GLAD_DEBUG
    _glDepthFunc = glad_debug_glDepthFunc;
#else
    _glDepthFunc = glad_glDepthFunc;
#endif
#ifdef GLAD_DEBUG
    _glPixelStoref = glad_debug_glPixelStoref;
#else
    _glPixelStoref = glad_glPixelStoref;
#endif
#ifdef GLAD_DEBUG
    _glPixelStorei = glad_debug_glPixelStorei;
#else
    _glPixelStorei = glad_glPixelStorei;
#endif
#ifdef GLAD_DEBUG
    _glReadBuffer = glad_debug_glReadBuffer;
#else
    _glReadBuffer = glad_glReadBuffer;
#endif
#ifdef GLAD_DEBUG
    _glReadPixels = glad_debug_glReadPixels;
#else
    _glReadPixels = glad_glReadPixels;
#endif
#ifdef GLAD_DEBUG
    _glGetBooleanv = glad_debug_glGetBooleanv;
#else
    _glGetBooleanv = glad_glGetBooleanv;
#endif
#ifdef GLAD_DEBUG
    _glGetDoublev = glad_debug_glGetDoublev;
#else
    _glGetDoublev = glad_glGetDoublev;
#endif
#ifdef GLAD_DEBUG
    _glGetError = glad_debug_glGetError;
#else
    _glGetError = glad_glGetError;
#endif
#ifdef GLAD_DEBUG
    _glGetFloatv = glad_debug_glGetFloatv;
#else
    _glGetFloatv = glad_glGetFloatv;
#endif
#ifdef GLAD_DEBUG
    _glGetIntegerv = glad_debug_glGetIntegerv;
#else
    _glGetIntegerv = glad_glGetIntegerv;
#endif
#ifdef GLAD_DEBUG
    _glGetString = glad_debug_glGetString;
#else
    _glGetString = glad_glGetString;
#endif
#ifdef GLAD_DEBUG
    _glGetTexImage = glad_debug_glGetTexImage;
#else
    _glGetTexImage = glad_glGetTexImage;
#endif
#ifdef GLAD_DEBUG
    _glGetTexParameterfv = glad_debug_glGetTexParameterfv;
#else
    _glGetTexParameterfv = glad_glGetTexParameterfv;
#endif
#ifdef GLAD_DEBUG
    _glGetTexParameteriv = glad_debug_glGetTexParameteriv;
#else
    _glGetTexParameteriv = glad_glGetTexParameteriv;
#endif
#ifdef GLAD_DEBUG
    _glGetTexLevelParameterfv = glad_debug_glGetTexLevelParameterfv;
#else
    _glGetTexLevelParameterfv = glad_glGetTexLevelParameterfv;
#endif
#ifdef GLAD_DEBUG
    _glGetTexLevelParameteriv = glad_debug_glGetTexLevelParameteriv;
#else
    _glGetTexLevelParameteriv = glad_glGetTexLevelParameteriv;
#endif
#ifdef GLAD_DEBUG
    _glIsEnabled = glad_debug_glIsEnabled;
#else
    _glIsEnabled = glad_glIsEnabled;
#endif
#ifdef GLAD_DEBUG
    _glDepthRange = glad_debug_glDepthRange;
#else
    _glDepthRange = glad_glDepthRange;
#endif
#ifdef GLAD_DEBUG
    _glViewport = glad_debug_glViewport;
#else
    _glViewport = glad_glViewport;
#endif
#ifdef GLAD_DEBUG
    _glNewList = glad_debug_glNewList;
#else
    _glNewList = glad_glNewList;
#endif
#ifdef GLAD_DEBUG
    _glEndList = glad_debug_glEndList;
#else
    _glEndList = glad_glEndList;
#endif
#ifdef GLAD_DEBUG
    _glCallList = glad_debug_glCallList;
#else
    _glCallList = glad_glCallList;
#endif
#ifdef GLAD_DEBUG
    _glCallLists = glad_debug_glCallLists;
#else
    _glCallLists = glad_glCallLists;
#endif
#ifdef GLAD_DEBUG
    _glDeleteLists = glad_debug_glDeleteLists;
#else
    _glDeleteLists = glad_glDeleteLists;
#endif
#ifdef GLAD_DEBUG
    _glGenLists = glad_debug_glGenLists;
#else
    _glGenLists = glad_glGenLists;
#endif
#ifdef GLAD_DEBUG
    _glListBase = glad_debug_glListBase;
#else
    _glListBase = glad_glListBase;
#endif
#ifdef GLAD_DEBUG
    _glBegin = glad_debug_glBegin;
#else
    _glBegin = glad_glBegin;
#endif
#ifdef GLAD_DEBUG
    _glBitmap = glad_debug_glBitmap;
#else
    _glBitmap = glad_glBitmap;
#endif
#ifdef GLAD_DEBUG
    _glColor3b = glad_debug_glColor3b;
#else
    _glColor3b = glad_glColor3b;
#endif
#ifdef GLAD_DEBUG
    _glColor3bv = glad_debug_glColor3bv;
#else
    _glColor3bv = glad_glColor3bv;
#endif
#ifdef GLAD_DEBUG
    _glColor3d = glad_debug_glColor3d;
#else
    _glColor3d = glad_glColor3d;
#endif
#ifdef GLAD_DEBUG
    _glColor3dv = glad_debug_glColor3dv;
#else
    _glColor3dv = glad_glColor3dv;
#endif
#ifdef GLAD_DEBUG
    _glColor3f = glad_debug_glColor3f;
#else
    _glColor3f = glad_glColor3f;
#endif
#ifdef GLAD_DEBUG
    _glColor3fv = glad_debug_glColor3fv;
#else
    _glColor3fv = glad_glColor3fv;
#endif
#ifdef GLAD_DEBUG
    _glColor3i = glad_debug_glColor3i;
#else
    _glColor3i = glad_glColor3i;
#endif
#ifdef GLAD_DEBUG
    _glColor3iv = glad_debug_glColor3iv;
#else
    _glColor3iv = glad_glColor3iv;
#endif
#ifdef GLAD_DEBUG
    _glColor3s = glad_debug_glColor3s;
#else
    _glColor3s = glad_glColor3s;
#endif
#ifdef GLAD_DEBUG
    _glColor3sv = glad_debug_glColor3sv;
#else
    _glColor3sv = glad_glColor3sv;
#endif
#ifdef GLAD_DEBUG
    _glColor3ub = glad_debug_glColor3ub;
#else
    _glColor3ub = glad_glColor3ub;
#endif
#ifdef GLAD_DEBUG
    _glColor3ubv = glad_debug_glColor3ubv;
#else
    _glColor3ubv = glad_glColor3ubv;
#endif
#ifdef GLAD_DEBUG
    _glColor3ui = glad_debug_glColor3ui;
#else
    _glColor3ui = glad_glColor3ui;
#endif
#ifdef GLAD_DEBUG
    _glColor3uiv = glad_debug_glColor3uiv;
#else
    _glColor3uiv = glad_glColor3uiv;
#endif
#ifdef GLAD_DEBUG
    _glColor3us = glad_debug_glColor3us;
#else
    _glColor3us = glad_glColor3us;
#endif
#ifdef GLAD_DEBUG
    _glColor3usv = glad_debug_glColor3usv;
#else
    _glColor3usv = glad_glColor3usv;
#endif
#ifdef GLAD_DEBUG
    _glColor4b = glad_debug_glColor4b;
#else
    _glColor4b = glad_glColor4b;
#endif
#ifdef GLAD_DEBUG
    _glColor4bv = glad_debug_glColor4bv;
#else
    _glColor4bv = glad_glColor4bv;
#endif
#ifdef GLAD_DEBUG
    _glColor4d = glad_debug_glColor4d;
#else
    _glColor4d = glad_glColor4d;
#endif
#ifdef GLAD_DEBUG
    _glColor4dv = glad_debug_glColor4dv;
#else
    _glColor4dv = glad_glColor4dv;
#endif
#ifdef GLAD_DEBUG
    _glColor4f = glad_debug_glColor4f;
#else
    _glColor4f = glad_glColor4f;
#endif
#ifdef GLAD_DEBUG
    _glColor4fv = glad_debug_glColor4fv;
#else
    _glColor4fv = glad_glColor4fv;
#endif
#ifdef GLAD_DEBUG
    _glColor4i = glad_debug_glColor4i;
#else
    _glColor4i = glad_glColor4i;
#endif
#ifdef GLAD_DEBUG
    _glColor4iv = glad_debug_glColor4iv;
#else
    _glColor4iv = glad_glColor4iv;
#endif
#ifdef GLAD_DEBUG
    _glColor4s = glad_debug_glColor4s;
#else
    _glColor4s = glad_glColor4s;
#endif
#ifdef GLAD_DEBUG
    _glColor4sv = glad_debug_glColor4sv;
#else
    _glColor4sv = glad_glColor4sv;
#endif
#ifdef GLAD_DEBUG
    _glColor4ub = glad_debug_glColor4ub;
#else
    _glColor4ub = glad_glColor4ub;
#endif
#ifdef GLAD_DEBUG
    _glColor4ubv = glad_debug_glColor4ubv;
#else
    _glColor4ubv = glad_glColor4ubv;
#endif
#ifdef GLAD_DEBUG
    _glColor4ui = glad_debug_glColor4ui;
#else
    _glColor4ui = glad_glColor4ui;
#endif
#ifdef GLAD_DEBUG
    _glColor4uiv = glad_debug_glColor4uiv;
#else
    _glColor4uiv = glad_glColor4uiv;
#endif
#ifdef GLAD_DEBUG
    _glColor4us = glad_debug_glColor4us;
#else
    _glColor4us = glad_glColor4us;
#endif
#ifdef GLAD_DEBUG
    _glColor4usv = glad_debug_glColor4usv;
#else
    _glColor4usv = glad_glColor4usv;
#endif
#ifdef GLAD_DEBUG
    _glEdgeFlag = glad_debug_glEdgeFlag;
#else
    _glEdgeFlag = glad_glEdgeFlag;
#endif
#ifdef GLAD_DEBUG
    _glEdgeFlagv = glad_debug_glEdgeFlagv;
#else
    _glEdgeFlagv = glad_glEdgeFlagv;
#endif
#ifdef GLAD_DEBUG
    _glEnd = glad_debug_glEnd;
#else
    _glEnd = glad_glEnd;
#endif
#ifdef GLAD_DEBUG
    _glIndexd = glad_debug_glIndexd;
#else
    _glIndexd = glad_glIndexd;
#endif
#ifdef GLAD_DEBUG
    _glIndexdv = glad_debug_glIndexdv;
#else
    _glIndexdv = glad_glIndexdv;
#endif
#ifdef GLAD_DEBUG
    _glIndexf = glad_debug_glIndexf;
#else
    _glIndexf = glad_glIndexf;
#endif
#ifdef GLAD_DEBUG
    _glIndexfv = glad_debug_glIndexfv;
#else
    _glIndexfv = glad_glIndexfv;
#endif
#ifdef GLAD_DEBUG
    _glIndexi = glad_debug_glIndexi;
#else
    _glIndexi = glad_glIndexi;
#endif
#ifdef GLAD_DEBUG
    _glIndexiv = glad_debug_glIndexiv;
#else
    _glIndexiv = glad_glIndexiv;
#endif
#ifdef GLAD_DEBUG
    _glIndexs = glad_debug_glIndexs;
#else
    _glIndexs = glad_glIndexs;
#endif
#ifdef GLAD_DEBUG
    _glIndexsv = glad_debug_glIndexsv;
#else
    _glIndexsv = glad_glIndexsv;
#endif
#ifdef GLAD_DEBUG
    _glNormal3b = glad_debug_glNormal3b;
#else
    _glNormal3b = glad_glNormal3b;
#endif
#ifdef GLAD_DEBUG
    _glNormal3bv = glad_debug_glNormal3bv;
#else
    _glNormal3bv = glad_glNormal3bv;
#endif
#ifdef GLAD_DEBUG
    _glNormal3d = glad_debug_glNormal3d;
#else
    _glNormal3d = glad_glNormal3d;
#endif
#ifdef GLAD_DEBUG
    _glNormal3dv = glad_debug_glNormal3dv;
#else
    _glNormal3dv = glad_glNormal3dv;
#endif
#ifdef GLAD_DEBUG
    _glNormal3f = glad_debug_glNormal3f;
#else
    _glNormal3f = glad_glNormal3f;
#endif
#ifdef GLAD_DEBUG
    _glNormal3fv = glad_debug_glNormal3fv;
#else
    _glNormal3fv = glad_glNormal3fv;
#endif
#ifdef GLAD_DEBUG
    _glNormal3i = glad_debug_glNormal3i;
#else
    _glNormal3i = glad_glNormal3i;
#endif
#ifdef GLAD_DEBUG
    _glNormal3iv = glad_debug_glNormal3iv;
#else
    _glNormal3iv = glad_glNormal3iv;
#endif
#ifdef GLAD_DEBUG
    _glNormal3s = glad_debug_glNormal3s;
#else
    _glNormal3s = glad_glNormal3s;
#endif
#ifdef GLAD_DEBUG
    _glNormal3sv = glad_debug_glNormal3sv;
#else
    _glNormal3sv = glad_glNormal3sv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos2d = glad_debug_glRasterPos2d;
#else
    _glRasterPos2d = glad_glRasterPos2d;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos2dv = glad_debug_glRasterPos2dv;
#else
    _glRasterPos2dv = glad_glRasterPos2dv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos2f = glad_debug_glRasterPos2f;
#else
    _glRasterPos2f = glad_glRasterPos2f;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos2fv = glad_debug_glRasterPos2fv;
#else
    _glRasterPos2fv = glad_glRasterPos2fv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos2i = glad_debug_glRasterPos2i;
#else
    _glRasterPos2i = glad_glRasterPos2i;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos2iv = glad_debug_glRasterPos2iv;
#else
    _glRasterPos2iv = glad_glRasterPos2iv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos2s = glad_debug_glRasterPos2s;
#else
    _glRasterPos2s = glad_glRasterPos2s;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos2sv = glad_debug_glRasterPos2sv;
#else
    _glRasterPos2sv = glad_glRasterPos2sv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos3d = glad_debug_glRasterPos3d;
#else
    _glRasterPos3d = glad_glRasterPos3d;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos3dv = glad_debug_glRasterPos3dv;
#else
    _glRasterPos3dv = glad_glRasterPos3dv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos3f = glad_debug_glRasterPos3f;
#else
    _glRasterPos3f = glad_glRasterPos3f;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos3fv = glad_debug_glRasterPos3fv;
#else
    _glRasterPos3fv = glad_glRasterPos3fv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos3i = glad_debug_glRasterPos3i;
#else
    _glRasterPos3i = glad_glRasterPos3i;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos3iv = glad_debug_glRasterPos3iv;
#else
    _glRasterPos3iv = glad_glRasterPos3iv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos3s = glad_debug_glRasterPos3s;
#else
    _glRasterPos3s = glad_glRasterPos3s;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos3sv = glad_debug_glRasterPos3sv;
#else
    _glRasterPos3sv = glad_glRasterPos3sv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos4d = glad_debug_glRasterPos4d;
#else
    _glRasterPos4d = glad_glRasterPos4d;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos4dv = glad_debug_glRasterPos4dv;
#else
    _glRasterPos4dv = glad_glRasterPos4dv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos4f = glad_debug_glRasterPos4f;
#else
    _glRasterPos4f = glad_glRasterPos4f;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos4fv = glad_debug_glRasterPos4fv;
#else
    _glRasterPos4fv = glad_glRasterPos4fv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos4i = glad_debug_glRasterPos4i;
#else
    _glRasterPos4i = glad_glRasterPos4i;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos4iv = glad_debug_glRasterPos4iv;
#else
    _glRasterPos4iv = glad_glRasterPos4iv;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos4s = glad_debug_glRasterPos4s;
#else
    _glRasterPos4s = glad_glRasterPos4s;
#endif
#ifdef GLAD_DEBUG
    _glRasterPos4sv = glad_debug_glRasterPos4sv;
#else
    _glRasterPos4sv = glad_glRasterPos4sv;
#endif
#ifdef GLAD_DEBUG
    _glRectd = glad_debug_glRectd;
#else
    _glRectd = glad_glRectd;
#endif
#ifdef GLAD_DEBUG
    _glRectdv = glad_debug_glRectdv;
#else
    _glRectdv = glad_glRectdv;
#endif
#ifdef GLAD_DEBUG
    _glRectf = glad_debug_glRectf;
#else
    _glRectf = glad_glRectf;
#endif
#ifdef GLAD_DEBUG
    _glRectfv = glad_debug_glRectfv;
#else
    _glRectfv = glad_glRectfv;
#endif
#ifdef GLAD_DEBUG
    _glRecti = glad_debug_glRecti;
#else
    _glRecti = glad_glRecti;
#endif
#ifdef GLAD_DEBUG
    _glRectiv = glad_debug_glRectiv;
#else
    _glRectiv = glad_glRectiv;
#endif
#ifdef GLAD_DEBUG
    _glRects = glad_debug_glRects;
#else
    _glRects = glad_glRects;
#endif
#ifdef GLAD_DEBUG
    _glRectsv = glad_debug_glRectsv;
#else
    _glRectsv = glad_glRectsv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord1d = glad_debug_glTexCoord1d;
#else
    _glTexCoord1d = glad_glTexCoord1d;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord1dv = glad_debug_glTexCoord1dv;
#else
    _glTexCoord1dv = glad_glTexCoord1dv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord1f = glad_debug_glTexCoord1f;
#else
    _glTexCoord1f = glad_glTexCoord1f;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord1fv = glad_debug_glTexCoord1fv;
#else
    _glTexCoord1fv = glad_glTexCoord1fv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord1i = glad_debug_glTexCoord1i;
#else
    _glTexCoord1i = glad_glTexCoord1i;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord1iv = glad_debug_glTexCoord1iv;
#else
    _glTexCoord1iv = glad_glTexCoord1iv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord1s = glad_debug_glTexCoord1s;
#else
    _glTexCoord1s = glad_glTexCoord1s;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord1sv = glad_debug_glTexCoord1sv;
#else
    _glTexCoord1sv = glad_glTexCoord1sv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord2d = glad_debug_glTexCoord2d;
#else
    _glTexCoord2d = glad_glTexCoord2d;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord2dv = glad_debug_glTexCoord2dv;
#else
    _glTexCoord2dv = glad_glTexCoord2dv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord2f = glad_debug_glTexCoord2f;
#else
    _glTexCoord2f = glad_glTexCoord2f;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord2fv = glad_debug_glTexCoord2fv;
#else
    _glTexCoord2fv = glad_glTexCoord2fv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord2i = glad_debug_glTexCoord2i;
#else
    _glTexCoord2i = glad_glTexCoord2i;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord2iv = glad_debug_glTexCoord2iv;
#else
    _glTexCoord2iv = glad_glTexCoord2iv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord2s = glad_debug_glTexCoord2s;
#else
    _glTexCoord2s = glad_glTexCoord2s;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord2sv = glad_debug_glTexCoord2sv;
#else
    _glTexCoord2sv = glad_glTexCoord2sv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord3d = glad_debug_glTexCoord3d;
#else
    _glTexCoord3d = glad_glTexCoord3d;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord3dv = glad_debug_glTexCoord3dv;
#else
    _glTexCoord3dv = glad_glTexCoord3dv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord3f = glad_debug_glTexCoord3f;
#else
    _glTexCoord3f = glad_glTexCoord3f;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord3fv = glad_debug_glTexCoord3fv;
#else
    _glTexCoord3fv = glad_glTexCoord3fv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord3i = glad_debug_glTexCoord3i;
#else
    _glTexCoord3i = glad_glTexCoord3i;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord3iv = glad_debug_glTexCoord3iv;
#else
    _glTexCoord3iv = glad_glTexCoord3iv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord3s = glad_debug_glTexCoord3s;
#else
    _glTexCoord3s = glad_glTexCoord3s;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord3sv = glad_debug_glTexCoord3sv;
#else
    _glTexCoord3sv = glad_glTexCoord3sv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord4d = glad_debug_glTexCoord4d;
#else
    _glTexCoord4d = glad_glTexCoord4d;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord4dv = glad_debug_glTexCoord4dv;
#else
    _glTexCoord4dv = glad_glTexCoord4dv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord4f = glad_debug_glTexCoord4f;
#else
    _glTexCoord4f = glad_glTexCoord4f;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord4fv = glad_debug_glTexCoord4fv;
#else
    _glTexCoord4fv = glad_glTexCoord4fv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord4i = glad_debug_glTexCoord4i;
#else
    _glTexCoord4i = glad_glTexCoord4i;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord4iv = glad_debug_glTexCoord4iv;
#else
    _glTexCoord4iv = glad_glTexCoord4iv;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord4s = glad_debug_glTexCoord4s;
#else
    _glTexCoord4s = glad_glTexCoord4s;
#endif
#ifdef GLAD_DEBUG
    _glTexCoord4sv = glad_debug_glTexCoord4sv;
#else
    _glTexCoord4sv = glad_glTexCoord4sv;
#endif
#ifdef GLAD_DEBUG
    _glVertex2d = glad_debug_glVertex2d;
#else
    _glVertex2d = glad_glVertex2d;
#endif
#ifdef GLAD_DEBUG
    _glVertex2dv = glad_debug_glVertex2dv;
#else
    _glVertex2dv = glad_glVertex2dv;
#endif
#ifdef GLAD_DEBUG
    _glVertex2f = glad_debug_glVertex2f;
#else
    _glVertex2f = glad_glVertex2f;
#endif
#ifdef GLAD_DEBUG
    _glVertex2fv = glad_debug_glVertex2fv;
#else
    _glVertex2fv = glad_glVertex2fv;
#endif
#ifdef GLAD_DEBUG
    _glVertex2i = glad_debug_glVertex2i;
#else
    _glVertex2i = glad_glVertex2i;
#endif
#ifdef GLAD_DEBUG
    _glVertex2iv = glad_debug_glVertex2iv;
#else
    _glVertex2iv = glad_glVertex2iv;
#endif
#ifdef GLAD_DEBUG
    _glVertex2s = glad_debug_glVertex2s;
#else
    _glVertex2s = glad_glVertex2s;
#endif
#ifdef GLAD_DEBUG
    _glVertex2sv = glad_debug_glVertex2sv;
#else
    _glVertex2sv = glad_glVertex2sv;
#endif
#ifdef GLAD_DEBUG
    _glVertex3d = glad_debug_glVertex3d;
#else
    _glVertex3d = glad_glVertex3d;
#endif
#ifdef GLAD_DEBUG
    _glVertex3dv = glad_debug_glVertex3dv;
#else
    _glVertex3dv = glad_glVertex3dv;
#endif
#ifdef GLAD_DEBUG
    _glVertex3f = glad_debug_glVertex3f;
#else
    _glVertex3f = glad_glVertex3f;
#endif
#ifdef GLAD_DEBUG
    _glVertex3fv = glad_debug_glVertex3fv;
#else
    _glVertex3fv = glad_glVertex3fv;
#endif
#ifdef GLAD_DEBUG
    _glVertex3i = glad_debug_glVertex3i;
#else
    _glVertex3i = glad_glVertex3i;
#endif
#ifdef GLAD_DEBUG
    _glVertex3iv = glad_debug_glVertex3iv;
#else
    _glVertex3iv = glad_glVertex3iv;
#endif
#ifdef GLAD_DEBUG
    _glVertex3s = glad_debug_glVertex3s;
#else
    _glVertex3s = glad_glVertex3s;
#endif
#ifdef GLAD_DEBUG
    _glVertex3sv = glad_debug_glVertex3sv;
#else
    _glVertex3sv = glad_glVertex3sv;
#endif
#ifdef GLAD_DEBUG
    _glVertex4d = glad_debug_glVertex4d;
#else
    _glVertex4d = glad_glVertex4d;
#endif
#ifdef GLAD_DEBUG
    _glVertex4dv = glad_debug_glVertex4dv;
#else
    _glVertex4dv = glad_glVertex4dv;
#endif
#ifdef GLAD_DEBUG
    _glVertex4f = glad_debug_glVertex4f;
#else
    _glVertex4f = glad_glVertex4f;
#endif
#ifdef GLAD_DEBUG
    _glVertex4fv = glad_debug_glVertex4fv;
#else
    _glVertex4fv = glad_glVertex4fv;
#endif
#ifdef GLAD_DEBUG
    _glVertex4i = glad_debug_glVertex4i;
#else
    _glVertex4i = glad_glVertex4i;
#endif
#ifdef GLAD_DEBUG
    _glVertex4iv = glad_debug_glVertex4iv;
#else
    _glVertex4iv = glad_glVertex4iv;
#endif
#ifdef GLAD_DEBUG
    _glVertex4s = glad_debug_glVertex4s;
#else
    _glVertex4s = glad_glVertex4s;
#endif
#ifdef GLAD_DEBUG
    _glVertex4sv = glad_debug_glVertex4sv;
#else
    _glVertex4sv = glad_glVertex4sv;
#endif
#ifdef GLAD_DEBUG
    _glClipPlane = glad_debug_glClipPlane;
#else
    _glClipPlane = glad_glClipPlane;
#endif
#ifdef GLAD_DEBUG
    _glColorMaterial = glad_debug_glColorMaterial;
#else
    _glColorMaterial = glad_glColorMaterial;
#endif
#ifdef GLAD_DEBUG
    _glFogf = glad_debug_glFogf;
#else
    _glFogf = glad_glFogf;
#endif
#ifdef GLAD_DEBUG
    _glFogfv = glad_debug_glFogfv;
#else
    _glFogfv = glad_glFogfv;
#endif
#ifdef GLAD_DEBUG
    _glFogi = glad_debug_glFogi;
#else
    _glFogi = glad_glFogi;
#endif
#ifdef GLAD_DEBUG
    _glFogiv = glad_debug_glFogiv;
#else
    _glFogiv = glad_glFogiv;
#endif
#ifdef GLAD_DEBUG
    _glLightf = glad_debug_glLightf;
#else
    _glLightf = glad_glLightf;
#endif
#ifdef GLAD_DEBUG
    _glLightfv = glad_debug_glLightfv;
#else
    _glLightfv = glad_glLightfv;
#endif
#ifdef GLAD_DEBUG
    _glLighti = glad_debug_glLighti;
#else
    _glLighti = glad_glLighti;
#endif
#ifdef GLAD_DEBUG
    _glLightiv = glad_debug_glLightiv;
#else
    _glLightiv = glad_glLightiv;
#endif
#ifdef GLAD_DEBUG
    _glLightModelf = glad_debug_glLightModelf;
#else
    _glLightModelf = glad_glLightModelf;
#endif
#ifdef GLAD_DEBUG
    _glLightModelfv = glad_debug_glLightModelfv;
#else
    _glLightModelfv = glad_glLightModelfv;
#endif
#ifdef GLAD_DEBUG
    _glLightModeli = glad_debug_glLightModeli;
#else
    _glLightModeli = glad_glLightModeli;
#endif
#ifdef GLAD_DEBUG
    _glLightModeliv = glad_debug_glLightModeliv;
#else
    _glLightModeliv = glad_glLightModeliv;
#endif
#ifdef GLAD_DEBUG
    _glLineStipple = glad_debug_glLineStipple;
#else
    _glLineStipple = glad_glLineStipple;
#endif
#ifdef GLAD_DEBUG
    _glMaterialf = glad_debug_glMaterialf;
#else
    _glMaterialf = glad_glMaterialf;
#endif
#ifdef GLAD_DEBUG
    _glMaterialfv = glad_debug_glMaterialfv;
#else
    _glMaterialfv = glad_glMaterialfv;
#endif
#ifdef GLAD_DEBUG
    _glMateriali = glad_debug_glMateriali;
#else
    _glMateriali = glad_glMateriali;
#endif
#ifdef GLAD_DEBUG
    _glMaterialiv = glad_debug_glMaterialiv;
#else
    _glMaterialiv = glad_glMaterialiv;
#endif
#ifdef GLAD_DEBUG
    _glPolygonStipple = glad_debug_glPolygonStipple;
#else
    _glPolygonStipple = glad_glPolygonStipple;
#endif
#ifdef GLAD_DEBUG
    _glShadeModel = glad_debug_glShadeModel;
#else
    _glShadeModel = glad_glShadeModel;
#endif
#ifdef GLAD_DEBUG
    _glTexEnvf = glad_debug_glTexEnvf;
#else
    _glTexEnvf = glad_glTexEnvf;
#endif
#ifdef GLAD_DEBUG
    _glTexEnvfv = glad_debug_glTexEnvfv;
#else
    _glTexEnvfv = glad_glTexEnvfv;
#endif
#ifdef GLAD_DEBUG
    _glTexEnvi = glad_debug_glTexEnvi;
#else
    _glTexEnvi = glad_glTexEnvi;
#endif
#ifdef GLAD_DEBUG
    _glTexEnviv = glad_debug_glTexEnviv;
#else
    _glTexEnviv = glad_glTexEnviv;
#endif
#ifdef GLAD_DEBUG
    _glTexGend = glad_debug_glTexGend;
#else
    _glTexGend = glad_glTexGend;
#endif
#ifdef GLAD_DEBUG
    _glTexGendv = glad_debug_glTexGendv;
#else
    _glTexGendv = glad_glTexGendv;
#endif
#ifdef GLAD_DEBUG
    _glTexGenf = glad_debug_glTexGenf;
#else
    _glTexGenf = glad_glTexGenf;
#endif
#ifdef GLAD_DEBUG
    _glTexGenfv = glad_debug_glTexGenfv;
#else
    _glTexGenfv = glad_glTexGenfv;
#endif
#ifdef GLAD_DEBUG
    _glTexGeni = glad_debug_glTexGeni;
#else
    _glTexGeni = glad_glTexGeni;
#endif
#ifdef GLAD_DEBUG
    _glTexGeniv = glad_debug_glTexGeniv;
#else
    _glTexGeniv = glad_glTexGeniv;
#endif
#ifdef GLAD_DEBUG
    _glFeedbackBuffer = glad_debug_glFeedbackBuffer;
#else
    _glFeedbackBuffer = glad_glFeedbackBuffer;
#endif
#ifdef GLAD_DEBUG
    _glSelectBuffer = glad_debug_glSelectBuffer;
#else
    _glSelectBuffer = glad_glSelectBuffer;
#endif
#ifdef GLAD_DEBUG
    _glRenderMode = glad_debug_glRenderMode;
#else
    _glRenderMode = glad_glRenderMode;
#endif
#ifdef GLAD_DEBUG
    _glInitNames = glad_debug_glInitNames;
#else
    _glInitNames = glad_glInitNames;
#endif
#ifdef GLAD_DEBUG
    _glLoadName = glad_debug_glLoadName;
#else
    _glLoadName = glad_glLoadName;
#endif
#ifdef GLAD_DEBUG
    _glPassThrough = glad_debug_glPassThrough;
#else
    _glPassThrough = glad_glPassThrough;
#endif
#ifdef GLAD_DEBUG
    _glPopName = glad_debug_glPopName;
#else
    _glPopName = glad_glPopName;
#endif
#ifdef GLAD_DEBUG
    _glPushName = glad_debug_glPushName;
#else
    _glPushName = glad_glPushName;
#endif
#ifdef GLAD_DEBUG
    _glClearAccum = glad_debug_glClearAccum;
#else
    _glClearAccum = glad_glClearAccum;
#endif
#ifdef GLAD_DEBUG
    _glClearIndex = glad_debug_glClearIndex;
#else
    _glClearIndex = glad_glClearIndex;
#endif
#ifdef GLAD_DEBUG
    _glIndexMask = glad_debug_glIndexMask;
#else
    _glIndexMask = glad_glIndexMask;
#endif
#ifdef GLAD_DEBUG
    _glAccum = glad_debug_glAccum;
#else
    _glAccum = glad_glAccum;
#endif
#ifdef GLAD_DEBUG
    _glPopAttrib = glad_debug_glPopAttrib;
#else
    _glPopAttrib = glad_glPopAttrib;
#endif
#ifdef GLAD_DEBUG
    _glPushAttrib = glad_debug_glPushAttrib;
#else
    _glPushAttrib = glad_glPushAttrib;
#endif
#ifdef GLAD_DEBUG
    _glMap1d = glad_debug_glMap1d;
#else
    _glMap1d = glad_glMap1d;
#endif
#ifdef GLAD_DEBUG
    _glMap1f = glad_debug_glMap1f;
#else
    _glMap1f = glad_glMap1f;
#endif
#ifdef GLAD_DEBUG
    _glMap2d = glad_debug_glMap2d;
#else
    _glMap2d = glad_glMap2d;
#endif
#ifdef GLAD_DEBUG
    _glMap2f = glad_debug_glMap2f;
#else
    _glMap2f = glad_glMap2f;
#endif
#ifdef GLAD_DEBUG
    _glMapGrid1d = glad_debug_glMapGrid1d;
#else
    _glMapGrid1d = glad_glMapGrid1d;
#endif
#ifdef GLAD_DEBUG
    _glMapGrid1f = glad_debug_glMapGrid1f;
#else
    _glMapGrid1f = glad_glMapGrid1f;
#endif
#ifdef GLAD_DEBUG
    _glMapGrid2d = glad_debug_glMapGrid2d;
#else
    _glMapGrid2d = glad_glMapGrid2d;
#endif
#ifdef GLAD_DEBUG
    _glMapGrid2f = glad_debug_glMapGrid2f;
#else
    _glMapGrid2f = glad_glMapGrid2f;
#endif
#ifdef GLAD_DEBUG
    _glEvalCoord1d = glad_debug_glEvalCoord1d;
#else
    _glEvalCoord1d = glad_glEvalCoord1d;
#endif
#ifdef GLAD_DEBUG
    _glEvalCoord1dv = glad_debug_glEvalCoord1dv;
#else
    _glEvalCoord1dv = glad_glEvalCoord1dv;
#endif
#ifdef GLAD_DEBUG
    _glEvalCoord1f = glad_debug_glEvalCoord1f;
#else
    _glEvalCoord1f = glad_glEvalCoord1f;
#endif
#ifdef GLAD_DEBUG
    _glEvalCoord1fv = glad_debug_glEvalCoord1fv;
#else
    _glEvalCoord1fv = glad_glEvalCoord1fv;
#endif
#ifdef GLAD_DEBUG
    _glEvalCoord2d = glad_debug_glEvalCoord2d;
#else
    _glEvalCoord2d = glad_glEvalCoord2d;
#endif
#ifdef GLAD_DEBUG
    _glEvalCoord2dv = glad_debug_glEvalCoord2dv;
#else
    _glEvalCoord2dv = glad_glEvalCoord2dv;
#endif
#ifdef GLAD_DEBUG
    _glEvalCoord2f = glad_debug_glEvalCoord2f;
#else
    _glEvalCoord2f = glad_glEvalCoord2f;
#endif
#ifdef GLAD_DEBUG
    _glEvalCoord2fv = glad_debug_glEvalCoord2fv;
#else
    _glEvalCoord2fv = glad_glEvalCoord2fv;
#endif
#ifdef GLAD_DEBUG
    _glEvalMesh1 = glad_debug_glEvalMesh1;
#else
    _glEvalMesh1 = glad_glEvalMesh1;
#endif
#ifdef GLAD_DEBUG
    _glEvalPoint1 = glad_debug_glEvalPoint1;
#else
    _glEvalPoint1 = glad_glEvalPoint1;
#endif
#ifdef GLAD_DEBUG
    _glEvalMesh2 = glad_debug_glEvalMesh2;
#else
    _glEvalMesh2 = glad_glEvalMesh2;
#endif
#ifdef GLAD_DEBUG
    _glEvalPoint2 = glad_debug_glEvalPoint2;
#else
    _glEvalPoint2 = glad_glEvalPoint2;
#endif
#ifdef GLAD_DEBUG
    _glAlphaFunc = glad_debug_glAlphaFunc;
#else
    _glAlphaFunc = glad_glAlphaFunc;
#endif
#ifdef GLAD_DEBUG
    _glPixelZoom = glad_debug_glPixelZoom;
#else
    _glPixelZoom = glad_glPixelZoom;
#endif
#ifdef GLAD_DEBUG
    _glPixelTransferf = glad_debug_glPixelTransferf;
#else
    _glPixelTransferf = glad_glPixelTransferf;
#endif
#ifdef GLAD_DEBUG
    _glPixelTransferi = glad_debug_glPixelTransferi;
#else
    _glPixelTransferi = glad_glPixelTransferi;
#endif
#ifdef GLAD_DEBUG
    _glPixelMapfv = glad_debug_glPixelMapfv;
#else
    _glPixelMapfv = glad_glPixelMapfv;
#endif
#ifdef GLAD_DEBUG
    _glPixelMapuiv = glad_debug_glPixelMapuiv;
#else
    _glPixelMapuiv = glad_glPixelMapuiv;
#endif
#ifdef GLAD_DEBUG
    _glPixelMapusv = glad_debug_glPixelMapusv;
#else
    _glPixelMapusv = glad_glPixelMapusv;
#endif
#ifdef GLAD_DEBUG
    _glCopyPixels = glad_debug_glCopyPixels;
#else
    _glCopyPixels = glad_glCopyPixels;
#endif
#ifdef GLAD_DEBUG
    _glDrawPixels = glad_debug_glDrawPixels;
#else
    _glDrawPixels = glad_glDrawPixels;
#endif
#ifdef GLAD_DEBUG
    _glGetClipPlane = glad_debug_glGetClipPlane;
#else
    _glGetClipPlane = glad_glGetClipPlane;
#endif
#ifdef GLAD_DEBUG
    _glGetLightfv = glad_debug_glGetLightfv;
#else
    _glGetLightfv = glad_glGetLightfv;
#endif
#ifdef GLAD_DEBUG
    _glGetLightiv = glad_debug_glGetLightiv;
#else
    _glGetLightiv = glad_glGetLightiv;
#endif
#ifdef GLAD_DEBUG
    _glGetMapdv = glad_debug_glGetMapdv;
#else
    _glGetMapdv = glad_glGetMapdv;
#endif
#ifdef GLAD_DEBUG
    _glGetMapfv = glad_debug_glGetMapfv;
#else
    _glGetMapfv = glad_glGetMapfv;
#endif
#ifdef GLAD_DEBUG
    _glGetMapiv = glad_debug_glGetMapiv;
#else
    _glGetMapiv = glad_glGetMapiv;
#endif
#ifdef GLAD_DEBUG
    _glGetMaterialfv = glad_debug_glGetMaterialfv;
#else
    _glGetMaterialfv = glad_glGetMaterialfv;
#endif
#ifdef GLAD_DEBUG
    _glGetMaterialiv = glad_debug_glGetMaterialiv;
#else
    _glGetMaterialiv = glad_glGetMaterialiv;
#endif
#ifdef GLAD_DEBUG
    _glGetPixelMapfv = glad_debug_glGetPixelMapfv;
#else
    _glGetPixelMapfv = glad_glGetPixelMapfv;
#endif
#ifdef GLAD_DEBUG
    _glGetPixelMapuiv = glad_debug_glGetPixelMapuiv;
#else
    _glGetPixelMapuiv = glad_glGetPixelMapuiv;
#endif
#ifdef GLAD_DEBUG
    _glGetPixelMapusv = glad_debug_glGetPixelMapusv;
#else
    _glGetPixelMapusv = glad_glGetPixelMapusv;
#endif
#ifdef GLAD_DEBUG
    _glGetPolygonStipple = glad_debug_glGetPolygonStipple;
#else
    _glGetPolygonStipple = glad_glGetPolygonStipple;
#endif
#ifdef GLAD_DEBUG
    _glGetTexEnvfv = glad_debug_glGetTexEnvfv;
#else
    _glGetTexEnvfv = glad_glGetTexEnvfv;
#endif
#ifdef GLAD_DEBUG
    _glGetTexEnviv = glad_debug_glGetTexEnviv;
#else
    _glGetTexEnviv = glad_glGetTexEnviv;
#endif
#ifdef GLAD_DEBUG
    _glGetTexGendv = glad_debug_glGetTexGendv;
#else
    _glGetTexGendv = glad_glGetTexGendv;
#endif
#ifdef GLAD_DEBUG
    _glGetTexGenfv = glad_debug_glGetTexGenfv;
#else
    _glGetTexGenfv = glad_glGetTexGenfv;
#endif
#ifdef GLAD_DEBUG
    _glGetTexGeniv = glad_debug_glGetTexGeniv;
#else
    _glGetTexGeniv = glad_glGetTexGeniv;
#endif
#ifdef GLAD_DEBUG
    _glIsList = glad_debug_glIsList;
#else
    _glIsList = glad_glIsList;
#endif
#ifdef GLAD_DEBUG
    _glFrustum = glad_debug_glFrustum;
#else
    _glFrustum = glad_glFrustum;
#endif
#ifdef GLAD_DEBUG
    _glLoadIdentity = glad_debug_glLoadIdentity;
#else
    _glLoadIdentity = glad_glLoadIdentity;
#endif
#ifdef GLAD_DEBUG
    _glLoadMatrixf = glad_debug_glLoadMatrixf;
#else
    _glLoadMatrixf = glad_glLoadMatrixf;
#endif
#ifdef GLAD_DEBUG
    _glLoadMatrixd = glad_debug_glLoadMatrixd;
#else
    _glLoadMatrixd = glad_glLoadMatrixd;
#endif
#ifdef GLAD_DEBUG
    _glMatrixMode = glad_debug_glMatrixMode;
#else
    _glMatrixMode = glad_glMatrixMode;
#endif
#ifdef GLAD_DEBUG
    _glMultMatrixf = glad_debug_glMultMatrixf;
#else
    _glMultMatrixf = glad_glMultMatrixf;
#endif
#ifdef GLAD_DEBUG
    _glMultMatrixd = glad_debug_glMultMatrixd;
#else
    _glMultMatrixd = glad_glMultMatrixd;
#endif
#ifdef GLAD_DEBUG
    _glOrtho = glad_debug_glOrtho;
#else
    _glOrtho = glad_glOrtho;
#endif
#ifdef GLAD_DEBUG
    _glPopMatrix = glad_debug_glPopMatrix;
#else
    _glPopMatrix = glad_glPopMatrix;
#endif
#ifdef GLAD_DEBUG
    _glPushMatrix = glad_debug_glPushMatrix;
#else
    _glPushMatrix = glad_glPushMatrix;
#endif
#ifdef GLAD_DEBUG
    _glRotated = glad_debug_glRotated;
#else
    _glRotated = glad_glRotated;
#endif
#ifdef GLAD_DEBUG
    _glRotatef = glad_debug_glRotatef;
#else
    _glRotatef = glad_glRotatef;
#endif
#ifdef GLAD_DEBUG
    _glScaled = glad_debug_glScaled;
#else
    _glScaled = glad_glScaled;
#endif
#ifdef GLAD_DEBUG
    _glScalef = glad_debug_glScalef;
#else
    _glScalef = glad_glScalef;
#endif
#ifdef GLAD_DEBUG
    _glTranslated = glad_debug_glTranslated;
#else
    _glTranslated = glad_glTranslated;
#endif
#ifdef GLAD_DEBUG
    _glTranslatef = glad_debug_glTranslatef;
#else
    _glTranslatef = glad_glTranslatef;
#endif
#ifdef GLAD_DEBUG
    _glDrawArrays = glad_debug_glDrawArrays;
#else
    _glDrawArrays = glad_glDrawArrays;
#endif
#ifdef GLAD_DEBUG
    _glDrawElements = glad_debug_glDrawElements;
#else
    _glDrawElements = glad_glDrawElements;
#endif
#ifdef GLAD_DEBUG
    _glGetPointerv = glad_debug_glGetPointerv;
#else
    _glGetPointerv = glad_glGetPointerv;
#endif
#ifdef GLAD_DEBUG
    _glPolygonOffset = glad_debug_glPolygonOffset;
#else
    _glPolygonOffset = glad_glPolygonOffset;
#endif
#ifdef GLAD_DEBUG
    _glCopyTexImage1D = glad_debug_glCopyTexImage1D;
#else
    _glCopyTexImage1D = glad_glCopyTexImage1D;
#endif
#ifdef GLAD_DEBUG
    _glCopyTexImage2D = glad_debug_glCopyTexImage2D;
#else
    _glCopyTexImage2D = glad_glCopyTexImage2D;
#endif
#ifdef GLAD_DEBUG
    _glCopyTexSubImage1D = glad_debug_glCopyTexSubImage1D;
#else
    _glCopyTexSubImage1D = glad_glCopyTexSubImage1D;
#endif
#ifdef GLAD_DEBUG
    _glCopyTexSubImage2D = glad_debug_glCopyTexSubImage2D;
#else
    _glCopyTexSubImage2D = glad_glCopyTexSubImage2D;
#endif
#ifdef GLAD_DEBUG
    _glTexSubImage1D = glad_debug_glTexSubImage1D;
#else
    _glTexSubImage1D = glad_glTexSubImage1D;
#endif
#ifdef GLAD_DEBUG
    _glTexSubImage2D = glad_debug_glTexSubImage2D;
#else
    _glTexSubImage2D = glad_glTexSubImage2D;
#endif
#ifdef GLAD_DEBUG
    _glBindTexture = glad_debug_glBindTexture;
#else
    _glBindTexture = glad_glBindTexture;
#endif
#ifdef GLAD_DEBUG
    _glDeleteTextures = glad_debug_glDeleteTextures;
#else
    _glDeleteTextures = glad_glDeleteTextures;
#endif
#ifdef GLAD_DEBUG
    _glGenTextures = glad_debug_glGenTextures;
#else
    _glGenTextures = glad_glGenTextures;
#endif
#ifdef GLAD_DEBUG
    _glIsTexture = glad_debug_glIsTexture;
#else
    _glIsTexture = glad_glIsTexture;
#endif
#ifdef GLAD_DEBUG
    _glArrayElement = glad_debug_glArrayElement;
#else
    _glArrayElement = glad_glArrayElement;
#endif
#ifdef GLAD_DEBUG
    _glColorPointer = glad_debug_glColorPointer;
#else
    _glColorPointer = glad_glColorPointer;
#endif
#ifdef GLAD_DEBUG
    _glDisableClientState = glad_debug_glDisableClientState;
#else
    _glDisableClientState = glad_glDisableClientState;
#endif
#ifdef GLAD_DEBUG
    _glEdgeFlagPointer = glad_debug_glEdgeFlagPointer;
#else
    _glEdgeFlagPointer = glad_glEdgeFlagPointer;
#endif
#ifdef GLAD_DEBUG
    _glEnableClientState = glad_debug_glEnableClientState;
#else
    _glEnableClientState = glad_glEnableClientState;
#endif
#ifdef GLAD_DEBUG
    _glIndexPointer = glad_debug_glIndexPointer;
#else
    _glIndexPointer = glad_glIndexPointer;
#endif
#ifdef GLAD_DEBUG
    _glInterleavedArrays = glad_debug_glInterleavedArrays;
#else
    _glInterleavedArrays = glad_glInterleavedArrays;
#endif
#ifdef GLAD_DEBUG
    _glNormalPointer = glad_debug_glNormalPointer;
#else
    _glNormalPointer = glad_glNormalPointer;
#endif
#ifdef GLAD_DEBUG
    _glTexCoordPointer = glad_debug_glTexCoordPointer;
#else
    _glTexCoordPointer = glad_glTexCoordPointer;
#endif
#ifdef GLAD_DEBUG
    _glVertexPointer = glad_debug_glVertexPointer;
#else
    _glVertexPointer = glad_glVertexPointer;
#endif
#ifdef GLAD_DEBUG
    _glAreTexturesResident = glad_debug_glAreTexturesResident;
#else
    _glAreTexturesResident = glad_glAreTexturesResident;
#endif
#ifdef GLAD_DEBUG
    _glPrioritizeTextures = glad_debug_glPrioritizeTextures;
#else
    _glPrioritizeTextures = glad_glPrioritizeTextures;
#endif
#ifdef GLAD_DEBUG
    _glIndexub = glad_debug_glIndexub;
#else
    _glIndexub = glad_glIndexub;
#endif
#ifdef GLAD_DEBUG
    _glIndexubv = glad_debug_glIndexubv;
#else
    _glIndexubv = glad_glIndexubv;
#endif
#ifdef GLAD_DEBUG
    _glPopClientAttrib = glad_debug_glPopClientAttrib;
#else
    _glPopClientAttrib = glad_glPopClientAttrib;
#endif
#ifdef GLAD_DEBUG
    _glPushClientAttrib = glad_debug_glPushClientAttrib;
#else
    _glPushClientAttrib = glad_glPushClientAttrib;
#endif
#ifdef GLAD_DEBUG
    _glDrawRangeElements = glad_debug_glDrawRangeElements;
#else
    _glDrawRangeElements = glad_glDrawRangeElements;
#endif
#ifdef GLAD_DEBUG
    _glTexImage3D = glad_debug_glTexImage3D;
#else
    _glTexImage3D = glad_glTexImage3D;
#endif
#ifdef GLAD_DEBUG
    _glTexSubImage3D = glad_debug_glTexSubImage3D;
#else
    _glTexSubImage3D = glad_glTexSubImage3D;
#endif
#ifdef GLAD_DEBUG
    _glCopyTexSubImage3D = glad_debug_glCopyTexSubImage3D;
#else
    _glCopyTexSubImage3D = glad_glCopyTexSubImage3D;
#endif
#ifdef GLAD_DEBUG
    _glActiveTexture = glad_debug_glActiveTexture;
#else
    _glActiveTexture = glad_glActiveTexture;
#endif
#ifdef GLAD_DEBUG
    _glSampleCoverage = glad_debug_glSampleCoverage;
#else
    _glSampleCoverage = glad_glSampleCoverage;
#endif
#ifdef GLAD_DEBUG
    _glCompressedTexImage3D = glad_debug_glCompressedTexImage3D;
#else
    _glCompressedTexImage3D = glad_glCompressedTexImage3D;
#endif
#ifdef GLAD_DEBUG
    _glCompressedTexImage2D = glad_debug_glCompressedTexImage2D;
#else
    _glCompressedTexImage2D = glad_glCompressedTexImage2D;
#endif
#ifdef GLAD_DEBUG
    _glCompressedTexImage1D = glad_debug_glCompressedTexImage1D;
#else
    _glCompressedTexImage1D = glad_glCompressedTexImage1D;
#endif
#ifdef GLAD_DEBUG
    _glCompressedTexSubImage3D = glad_debug_glCompressedTexSubImage3D;
#else
    _glCompressedTexSubImage3D = glad_glCompressedTexSubImage3D;
#endif
#ifdef GLAD_DEBUG
    _glCompressedTexSubImage2D = glad_debug_glCompressedTexSubImage2D;
#else
    _glCompressedTexSubImage2D = glad_glCompressedTexSubImage2D;
#endif
#ifdef GLAD_DEBUG
    _glCompressedTexSubImage1D = glad_debug_glCompressedTexSubImage1D;
#else
    _glCompressedTexSubImage1D = glad_glCompressedTexSubImage1D;
#endif
#ifdef GLAD_DEBUG
    _glGetCompressedTexImage = glad_debug_glGetCompressedTexImage;
#else
    _glGetCompressedTexImage = glad_glGetCompressedTexImage;
#endif
#ifdef GLAD_DEBUG
    _glClientActiveTexture = glad_debug_glClientActiveTexture;
#else
    _glClientActiveTexture = glad_glClientActiveTexture;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord1d = glad_debug_glMultiTexCoord1d;
#else
    _glMultiTexCoord1d = glad_glMultiTexCoord1d;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord1dv = glad_debug_glMultiTexCoord1dv;
#else
    _glMultiTexCoord1dv = glad_glMultiTexCoord1dv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord1f = glad_debug_glMultiTexCoord1f;
#else
    _glMultiTexCoord1f = glad_glMultiTexCoord1f;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord1fv = glad_debug_glMultiTexCoord1fv;
#else
    _glMultiTexCoord1fv = glad_glMultiTexCoord1fv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord1i = glad_debug_glMultiTexCoord1i;
#else
    _glMultiTexCoord1i = glad_glMultiTexCoord1i;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord1iv = glad_debug_glMultiTexCoord1iv;
#else
    _glMultiTexCoord1iv = glad_glMultiTexCoord1iv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord1s = glad_debug_glMultiTexCoord1s;
#else
    _glMultiTexCoord1s = glad_glMultiTexCoord1s;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord1sv = glad_debug_glMultiTexCoord1sv;
#else
    _glMultiTexCoord1sv = glad_glMultiTexCoord1sv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord2d = glad_debug_glMultiTexCoord2d;
#else
    _glMultiTexCoord2d = glad_glMultiTexCoord2d;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord2dv = glad_debug_glMultiTexCoord2dv;
#else
    _glMultiTexCoord2dv = glad_glMultiTexCoord2dv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord2f = glad_debug_glMultiTexCoord2f;
#else
    _glMultiTexCoord2f = glad_glMultiTexCoord2f;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord2fv = glad_debug_glMultiTexCoord2fv;
#else
    _glMultiTexCoord2fv = glad_glMultiTexCoord2fv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord2i = glad_debug_glMultiTexCoord2i;
#else
    _glMultiTexCoord2i = glad_glMultiTexCoord2i;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord2iv = glad_debug_glMultiTexCoord2iv;
#else
    _glMultiTexCoord2iv = glad_glMultiTexCoord2iv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord2s = glad_debug_glMultiTexCoord2s;
#else
    _glMultiTexCoord2s = glad_glMultiTexCoord2s;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord2sv = glad_debug_glMultiTexCoord2sv;
#else
    _glMultiTexCoord2sv = glad_glMultiTexCoord2sv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord3d = glad_debug_glMultiTexCoord3d;
#else
    _glMultiTexCoord3d = glad_glMultiTexCoord3d;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord3dv = glad_debug_glMultiTexCoord3dv;
#else
    _glMultiTexCoord3dv = glad_glMultiTexCoord3dv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord3f = glad_debug_glMultiTexCoord3f;
#else
    _glMultiTexCoord3f = glad_glMultiTexCoord3f;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord3fv = glad_debug_glMultiTexCoord3fv;
#else
    _glMultiTexCoord3fv = glad_glMultiTexCoord3fv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord3i = glad_debug_glMultiTexCoord3i;
#else
    _glMultiTexCoord3i = glad_glMultiTexCoord3i;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord3iv = glad_debug_glMultiTexCoord3iv;
#else
    _glMultiTexCoord3iv = glad_glMultiTexCoord3iv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord3s = glad_debug_glMultiTexCoord3s;
#else
    _glMultiTexCoord3s = glad_glMultiTexCoord3s;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord3sv = glad_debug_glMultiTexCoord3sv;
#else
    _glMultiTexCoord3sv = glad_glMultiTexCoord3sv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord4d = glad_debug_glMultiTexCoord4d;
#else
    _glMultiTexCoord4d = glad_glMultiTexCoord4d;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord4dv = glad_debug_glMultiTexCoord4dv;
#else
    _glMultiTexCoord4dv = glad_glMultiTexCoord4dv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord4f = glad_debug_glMultiTexCoord4f;
#else
    _glMultiTexCoord4f = glad_glMultiTexCoord4f;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord4fv = glad_debug_glMultiTexCoord4fv;
#else
    _glMultiTexCoord4fv = glad_glMultiTexCoord4fv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord4i = glad_debug_glMultiTexCoord4i;
#else
    _glMultiTexCoord4i = glad_glMultiTexCoord4i;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord4iv = glad_debug_glMultiTexCoord4iv;
#else
    _glMultiTexCoord4iv = glad_glMultiTexCoord4iv;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord4s = glad_debug_glMultiTexCoord4s;
#else
    _glMultiTexCoord4s = glad_glMultiTexCoord4s;
#endif
#ifdef GLAD_DEBUG
    _glMultiTexCoord4sv = glad_debug_glMultiTexCoord4sv;
#else
    _glMultiTexCoord4sv = glad_glMultiTexCoord4sv;
#endif
#ifdef GLAD_DEBUG
    _glLoadTransposeMatrixf = glad_debug_glLoadTransposeMatrixf;
#else
    _glLoadTransposeMatrixf = glad_glLoadTransposeMatrixf;
#endif
#ifdef GLAD_DEBUG
    _glLoadTransposeMatrixd = glad_debug_glLoadTransposeMatrixd;
#else
    _glLoadTransposeMatrixd = glad_glLoadTransposeMatrixd;
#endif
#ifdef GLAD_DEBUG
    _glMultTransposeMatrixf = glad_debug_glMultTransposeMatrixf;
#else
    _glMultTransposeMatrixf = glad_glMultTransposeMatrixf;
#endif
#ifdef GLAD_DEBUG
    _glMultTransposeMatrixd = glad_debug_glMultTransposeMatrixd;
#else
    _glMultTransposeMatrixd = glad_glMultTransposeMatrixd;
#endif
#ifdef GLAD_DEBUG
    _glBlendFuncSeparate = glad_debug_glBlendFuncSeparate;
#else
    _glBlendFuncSeparate = glad_glBlendFuncSeparate;
#endif
#ifdef GLAD_DEBUG
    _glMultiDrawArrays = glad_debug_glMultiDrawArrays;
#else
    _glMultiDrawArrays = glad_glMultiDrawArrays;
#endif
#ifdef GLAD_DEBUG
    _glMultiDrawElements = glad_debug_glMultiDrawElements;
#else
    _glMultiDrawElements = glad_glMultiDrawElements;
#endif
#ifdef GLAD_DEBUG
    _glPointParameterf = glad_debug_glPointParameterf;
#else
    _glPointParameterf = glad_glPointParameterf;
#endif
#ifdef GLAD_DEBUG
    _glPointParameterfv = glad_debug_glPointParameterfv;
#else
    _glPointParameterfv = glad_glPointParameterfv;
#endif
#ifdef GLAD_DEBUG
    _glPointParameteri = glad_debug_glPointParameteri;
#else
    _glPointParameteri = glad_glPointParameteri;
#endif
#ifdef GLAD_DEBUG
    _glPointParameteriv = glad_debug_glPointParameteriv;
#else
    _glPointParameteriv = glad_glPointParameteriv;
#endif
#ifdef GLAD_DEBUG
    _glFogCoordf = glad_debug_glFogCoordf;
#else
    _glFogCoordf = glad_glFogCoordf;
#endif
#ifdef GLAD_DEBUG
    _glFogCoordfv = glad_debug_glFogCoordfv;
#else
    _glFogCoordfv = glad_glFogCoordfv;
#endif
#ifdef GLAD_DEBUG
    _glFogCoordd = glad_debug_glFogCoordd;
#else
    _glFogCoordd = glad_glFogCoordd;
#endif
#ifdef GLAD_DEBUG
    _glFogCoorddv = glad_debug_glFogCoorddv;
#else
    _glFogCoorddv = glad_glFogCoorddv;
#endif
#ifdef GLAD_DEBUG
    _glFogCoordPointer = glad_debug_glFogCoordPointer;
#else
    _glFogCoordPointer = glad_glFogCoordPointer;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3b = glad_debug_glSecondaryColor3b;
#else
    _glSecondaryColor3b = glad_glSecondaryColor3b;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3bv = glad_debug_glSecondaryColor3bv;
#else
    _glSecondaryColor3bv = glad_glSecondaryColor3bv;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3d = glad_debug_glSecondaryColor3d;
#else
    _glSecondaryColor3d = glad_glSecondaryColor3d;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3dv = glad_debug_glSecondaryColor3dv;
#else
    _glSecondaryColor3dv = glad_glSecondaryColor3dv;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3f = glad_debug_glSecondaryColor3f;
#else
    _glSecondaryColor3f = glad_glSecondaryColor3f;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3fv = glad_debug_glSecondaryColor3fv;
#else
    _glSecondaryColor3fv = glad_glSecondaryColor3fv;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3i = glad_debug_glSecondaryColor3i;
#else
    _glSecondaryColor3i = glad_glSecondaryColor3i;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3iv = glad_debug_glSecondaryColor3iv;
#else
    _glSecondaryColor3iv = glad_glSecondaryColor3iv;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3s = glad_debug_glSecondaryColor3s;
#else
    _glSecondaryColor3s = glad_glSecondaryColor3s;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3sv = glad_debug_glSecondaryColor3sv;
#else
    _glSecondaryColor3sv = glad_glSecondaryColor3sv;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3ub = glad_debug_glSecondaryColor3ub;
#else
    _glSecondaryColor3ub = glad_glSecondaryColor3ub;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3ubv = glad_debug_glSecondaryColor3ubv;
#else
    _glSecondaryColor3ubv = glad_glSecondaryColor3ubv;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3ui = glad_debug_glSecondaryColor3ui;
#else
    _glSecondaryColor3ui = glad_glSecondaryColor3ui;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3uiv = glad_debug_glSecondaryColor3uiv;
#else
    _glSecondaryColor3uiv = glad_glSecondaryColor3uiv;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3us = glad_debug_glSecondaryColor3us;
#else
    _glSecondaryColor3us = glad_glSecondaryColor3us;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColor3usv = glad_debug_glSecondaryColor3usv;
#else
    _glSecondaryColor3usv = glad_glSecondaryColor3usv;
#endif
#ifdef GLAD_DEBUG
    _glSecondaryColorPointer = glad_debug_glSecondaryColorPointer;
#else
    _glSecondaryColorPointer = glad_glSecondaryColorPointer;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos2d = glad_debug_glWindowPos2d;
#else
    _glWindowPos2d = glad_glWindowPos2d;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos2dv = glad_debug_glWindowPos2dv;
#else
    _glWindowPos2dv = glad_glWindowPos2dv;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos2f = glad_debug_glWindowPos2f;
#else
    _glWindowPos2f = glad_glWindowPos2f;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos2fv = glad_debug_glWindowPos2fv;
#else
    _glWindowPos2fv = glad_glWindowPos2fv;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos2i = glad_debug_glWindowPos2i;
#else
    _glWindowPos2i = glad_glWindowPos2i;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos2iv = glad_debug_glWindowPos2iv;
#else
    _glWindowPos2iv = glad_glWindowPos2iv;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos2s = glad_debug_glWindowPos2s;
#else
    _glWindowPos2s = glad_glWindowPos2s;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos2sv = glad_debug_glWindowPos2sv;
#else
    _glWindowPos2sv = glad_glWindowPos2sv;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos3d = glad_debug_glWindowPos3d;
#else
    _glWindowPos3d = glad_glWindowPos3d;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos3dv = glad_debug_glWindowPos3dv;
#else
    _glWindowPos3dv = glad_glWindowPos3dv;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos3f = glad_debug_glWindowPos3f;
#else
    _glWindowPos3f = glad_glWindowPos3f;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos3fv = glad_debug_glWindowPos3fv;
#else
    _glWindowPos3fv = glad_glWindowPos3fv;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos3i = glad_debug_glWindowPos3i;
#else
    _glWindowPos3i = glad_glWindowPos3i;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos3iv = glad_debug_glWindowPos3iv;
#else
    _glWindowPos3iv = glad_glWindowPos3iv;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos3s = glad_debug_glWindowPos3s;
#else
    _glWindowPos3s = glad_glWindowPos3s;
#endif
#ifdef GLAD_DEBUG
    _glWindowPos3sv = glad_debug_glWindowPos3sv;
#else
    _glWindowPos3sv = glad_glWindowPos3sv;
#endif
#ifdef GLAD_DEBUG
    _glBlendColor = glad_debug_glBlendColor;
#else
    _glBlendColor = glad_glBlendColor;
#endif
#ifdef GLAD_DEBUG
    _glBlendEquation = glad_debug_glBlendEquation;
#else
    _glBlendEquation = glad_glBlendEquation;
#endif
#ifdef GLAD_DEBUG
    _glGenQueries = glad_debug_glGenQueries;
#else
    _glGenQueries = glad_glGenQueries;
#endif
#ifdef GLAD_DEBUG
    _glDeleteQueries = glad_debug_glDeleteQueries;
#else
    _glDeleteQueries = glad_glDeleteQueries;
#endif
#ifdef GLAD_DEBUG
    _glIsQuery = glad_debug_glIsQuery;
#else
    _glIsQuery = glad_glIsQuery;
#endif
#ifdef GLAD_DEBUG
    _glBeginQuery = glad_debug_glBeginQuery;
#else
    _glBeginQuery = glad_glBeginQuery;
#endif
#ifdef GLAD_DEBUG
    _glEndQuery = glad_debug_glEndQuery;
#else
    _glEndQuery = glad_glEndQuery;
#endif
#ifdef GLAD_DEBUG
    _glGetQueryiv = glad_debug_glGetQueryiv;
#else
    _glGetQueryiv = glad_glGetQueryiv;
#endif
#ifdef GLAD_DEBUG
    _glGetQueryObjectiv = glad_debug_glGetQueryObjectiv;
#else
    _glGetQueryObjectiv = glad_glGetQueryObjectiv;
#endif
#ifdef GLAD_DEBUG
    _glGetQueryObjectuiv = glad_debug_glGetQueryObjectuiv;
#else
    _glGetQueryObjectuiv = glad_glGetQueryObjectuiv;
#endif
#ifdef GLAD_DEBUG
    _glBindBuffer = glad_debug_glBindBuffer;
#else
    _glBindBuffer = glad_glBindBuffer;
#endif
#ifdef GLAD_DEBUG
    _glDeleteBuffers = glad_debug_glDeleteBuffers;
#else
    _glDeleteBuffers = glad_glDeleteBuffers;
#endif
#ifdef GLAD_DEBUG
    _glGenBuffers = glad_debug_glGenBuffers;
#else
    _glGenBuffers = glad_glGenBuffers;
#endif
#ifdef GLAD_DEBUG
    _glIsBuffer = glad_debug_glIsBuffer;
#else
    _glIsBuffer = glad_glIsBuffer;
#endif
#ifdef GLAD_DEBUG
    _glBufferData = glad_debug_glBufferData;
#else
    _glBufferData = glad_glBufferData;
#endif
#ifdef GLAD_DEBUG
    _glBufferSubData = glad_debug_glBufferSubData;
#else
    _glBufferSubData = glad_glBufferSubData;
#endif
#ifdef GLAD_DEBUG
    _glGetBufferSubData = glad_debug_glGetBufferSubData;
#else
    _glGetBufferSubData = glad_glGetBufferSubData;
#endif
#ifdef GLAD_DEBUG
    _glMapBuffer = glad_debug_glMapBuffer;
#else
    _glMapBuffer = glad_glMapBuffer;
#endif
#ifdef GLAD_DEBUG
    _glUnmapBuffer = glad_debug_glUnmapBuffer;
#else
    _glUnmapBuffer = glad_glUnmapBuffer;
#endif
#ifdef GLAD_DEBUG
    _glGetBufferParameteriv = glad_debug_glGetBufferParameteriv;
#else
    _glGetBufferParameteriv = glad_glGetBufferParameteriv;
#endif
#ifdef GLAD_DEBUG
    _glGetBufferPointerv = glad_debug_glGetBufferPointerv;
#else
    _glGetBufferPointerv = glad_glGetBufferPointerv;
#endif
#ifdef GLAD_DEBUG
    _glBlendEquationSeparate = glad_debug_glBlendEquationSeparate;
#else
    _glBlendEquationSeparate = glad_glBlendEquationSeparate;
#endif
#ifdef GLAD_DEBUG
    _glDrawBuffers = glad_debug_glDrawBuffers;
#else
    _glDrawBuffers = glad_glDrawBuffers;
#endif
#ifdef GLAD_DEBUG
    _glStencilOpSeparate = glad_debug_glStencilOpSeparate;
#else
    _glStencilOpSeparate = glad_glStencilOpSeparate;
#endif
#ifdef GLAD_DEBUG
    _glStencilFuncSeparate = glad_debug_glStencilFuncSeparate;
#else
    _glStencilFuncSeparate = glad_glStencilFuncSeparate;
#endif
#ifdef GLAD_DEBUG
    _glStencilMaskSeparate = glad_debug_glStencilMaskSeparate;
#else
    _glStencilMaskSeparate = glad_glStencilMaskSeparate;
#endif
#ifdef GLAD_DEBUG
    _glAttachShader = glad_debug_glAttachShader;
#else
    _glAttachShader = glad_glAttachShader;
#endif
#ifdef GLAD_DEBUG
    _glBindAttribLocation = glad_debug_glBindAttribLocation;
#else
    _glBindAttribLocation = glad_glBindAttribLocation;
#endif
#ifdef GLAD_DEBUG
    _glCompileShader = glad_debug_glCompileShader;
#else
    _glCompileShader = glad_glCompileShader;
#endif
#ifdef GLAD_DEBUG
    _glCreateProgram = glad_debug_glCreateProgram;
#else
    _glCreateProgram = glad_glCreateProgram;
#endif
#ifdef GLAD_DEBUG
    _glCreateShader = glad_debug_glCreateShader;
#else
    _glCreateShader = glad_glCreateShader;
#endif
#ifdef GLAD_DEBUG
    _glDeleteProgram = glad_debug_glDeleteProgram;
#else
    _glDeleteProgram = glad_glDeleteProgram;
#endif
#ifdef GLAD_DEBUG
    _glDeleteShader = glad_debug_glDeleteShader;
#else
    _glDeleteShader = glad_glDeleteShader;
#endif
#ifdef GLAD_DEBUG
    _glDetachShader = glad_debug_glDetachShader;
#else
    _glDetachShader = glad_glDetachShader;
#endif
#ifdef GLAD_DEBUG
    _glDisableVertexAttribArray = glad_debug_glDisableVertexAttribArray;
#else
    _glDisableVertexAttribArray = glad_glDisableVertexAttribArray;
#endif
#ifdef GLAD_DEBUG
    _glEnableVertexAttribArray = glad_debug_glEnableVertexAttribArray;
#else
    _glEnableVertexAttribArray = glad_glEnableVertexAttribArray;
#endif
#ifdef GLAD_DEBUG
    _glGetActiveAttrib = glad_debug_glGetActiveAttrib;
#else
    _glGetActiveAttrib = glad_glGetActiveAttrib;
#endif
#ifdef GLAD_DEBUG
    _glGetActiveUniform = glad_debug_glGetActiveUniform;
#else
    _glGetActiveUniform = glad_glGetActiveUniform;
#endif
#ifdef GLAD_DEBUG
    _glGetAttachedShaders = glad_debug_glGetAttachedShaders;
#else
    _glGetAttachedShaders = glad_glGetAttachedShaders;
#endif
#ifdef GLAD_DEBUG
    _glGetAttribLocation = glad_debug_glGetAttribLocation;
#else
    _glGetAttribLocation = glad_glGetAttribLocation;
#endif
#ifdef GLAD_DEBUG
    _glGetProgramiv = glad_debug_glGetProgramiv;
#else
    _glGetProgramiv = glad_glGetProgramiv;
#endif
#ifdef GLAD_DEBUG
    _glGetProgramInfoLog = glad_debug_glGetProgramInfoLog;
#else
    _glGetProgramInfoLog = glad_glGetProgramInfoLog;
#endif
#ifdef GLAD_DEBUG
    _glGetShaderiv = glad_debug_glGetShaderiv;
#else
    _glGetShaderiv = glad_glGetShaderiv;
#endif
#ifdef GLAD_DEBUG
    _glGetShaderInfoLog = glad_debug_glGetShaderInfoLog;
#else
    _glGetShaderInfoLog = glad_glGetShaderInfoLog;
#endif
#ifdef GLAD_DEBUG
    _glGetShaderSource = glad_debug_glGetShaderSource;
#else
    _glGetShaderSource = glad_glGetShaderSource;
#endif
#ifdef GLAD_DEBUG
    _glGetUniformLocation = glad_debug_glGetUniformLocation;
#else
    _glGetUniformLocation = glad_glGetUniformLocation;
#endif
#ifdef GLAD_DEBUG
    _glGetUniformfv = glad_debug_glGetUniformfv;
#else
    _glGetUniformfv = glad_glGetUniformfv;
#endif
#ifdef GLAD_DEBUG
    _glGetUniformiv = glad_debug_glGetUniformiv;
#else
    _glGetUniformiv = glad_glGetUniformiv;
#endif
#ifdef GLAD_DEBUG
    _glGetVertexAttribdv = glad_debug_glGetVertexAttribdv;
#else
    _glGetVertexAttribdv = glad_glGetVertexAttribdv;
#endif
#ifdef GLAD_DEBUG
    _glGetVertexAttribfv = glad_debug_glGetVertexAttribfv;
#else
    _glGetVertexAttribfv = glad_glGetVertexAttribfv;
#endif
#ifdef GLAD_DEBUG
    _glGetVertexAttribiv = glad_debug_glGetVertexAttribiv;
#else
    _glGetVertexAttribiv = glad_glGetVertexAttribiv;
#endif
#ifdef GLAD_DEBUG
    _glGetVertexAttribPointerv = glad_debug_glGetVertexAttribPointerv;
#else
    _glGetVertexAttribPointerv = glad_glGetVertexAttribPointerv;
#endif
#ifdef GLAD_DEBUG
    _glIsProgram = glad_debug_glIsProgram;
#else
    _glIsProgram = glad_glIsProgram;
#endif
#ifdef GLAD_DEBUG
    _glIsShader = glad_debug_glIsShader;
#else
    _glIsShader = glad_glIsShader;
#endif
#ifdef GLAD_DEBUG
    _glLinkProgram = glad_debug_glLinkProgram;
#else
    _glLinkProgram = glad_glLinkProgram;
#endif
#ifdef GLAD_DEBUG
    _glShaderSource = glad_debug_glShaderSource;
#else
    _glShaderSource = glad_glShaderSource;
#endif
#ifdef GLAD_DEBUG
    _glUseProgram = glad_debug_glUseProgram;
#else
    _glUseProgram = glad_glUseProgram;
#endif
#ifdef GLAD_DEBUG
    _glUniform1f = glad_debug_glUniform1f;
#else
    _glUniform1f = glad_glUniform1f;
#endif
#ifdef GLAD_DEBUG
    _glUniform2f = glad_debug_glUniform2f;
#else
    _glUniform2f = glad_glUniform2f;
#endif
#ifdef GLAD_DEBUG
    _glUniform3f = glad_debug_glUniform3f;
#else
    _glUniform3f = glad_glUniform3f;
#endif
#ifdef GLAD_DEBUG
    _glUniform4f = glad_debug_glUniform4f;
#else
    _glUniform4f = glad_glUniform4f;
#endif
#ifdef GLAD_DEBUG
    _glUniform1i = glad_debug_glUniform1i;
#else
    _glUniform1i = glad_glUniform1i;
#endif
#ifdef GLAD_DEBUG
    _glUniform2i = glad_debug_glUniform2i;
#else
    _glUniform2i = glad_glUniform2i;
#endif
#ifdef GLAD_DEBUG
    _glUniform3i = glad_debug_glUniform3i;
#else
    _glUniform3i = glad_glUniform3i;
#endif
#ifdef GLAD_DEBUG
    _glUniform4i = glad_debug_glUniform4i;
#else
    _glUniform4i = glad_glUniform4i;
#endif
#ifdef GLAD_DEBUG
    _glUniform1fv = glad_debug_glUniform1fv;
#else
    _glUniform1fv = glad_glUniform1fv;
#endif
#ifdef GLAD_DEBUG
    _glUniform2fv = glad_debug_glUniform2fv;
#else
    _glUniform2fv = glad_glUniform2fv;
#endif
#ifdef GLAD_DEBUG
    _glUniform3fv = glad_debug_glUniform3fv;
#else
    _glUniform3fv = glad_glUniform3fv;
#endif
#ifdef GLAD_DEBUG
    _glUniform4fv = glad_debug_glUniform4fv;
#else
    _glUniform4fv = glad_glUniform4fv;
#endif
#ifdef GLAD_DEBUG
    _glUniform1iv = glad_debug_glUniform1iv;
#else
    _glUniform1iv = glad_glUniform1iv;
#endif
#ifdef GLAD_DEBUG
    _glUniform2iv = glad_debug_glUniform2iv;
#else
    _glUniform2iv = glad_glUniform2iv;
#endif
#ifdef GLAD_DEBUG
    _glUniform3iv = glad_debug_glUniform3iv;
#else
    _glUniform3iv = glad_glUniform3iv;
#endif
#ifdef GLAD_DEBUG
    _glUniform4iv = glad_debug_glUniform4iv;
#else
    _glUniform4iv = glad_glUniform4iv;
#endif
#ifdef GLAD_DEBUG
    _glUniformMatrix2fv = glad_debug_glUniformMatrix2fv;
#else
    _glUniformMatrix2fv = glad_glUniformMatrix2fv;
#endif
#ifdef GLAD_DEBUG
    _glUniformMatrix3fv = glad_debug_glUniformMatrix3fv;
#else
    _glUniformMatrix3fv = glad_glUniformMatrix3fv;
#endif
#ifdef GLAD_DEBUG
    _glUniformMatrix4fv = glad_debug_glUniformMatrix4fv;
#else
    _glUniformMatrix4fv = glad_glUniformMatrix4fv;
#endif
#ifdef GLAD_DEBUG
    _glValidateProgram = glad_debug_glValidateProgram;
#else
    _glValidateProgram = glad_glValidateProgram;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib1d = glad_debug_glVertexAttrib1d;
#else
    _glVertexAttrib1d = glad_glVertexAttrib1d;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib1dv = glad_debug_glVertexAttrib1dv;
#else
    _glVertexAttrib1dv = glad_glVertexAttrib1dv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib1f = glad_debug_glVertexAttrib1f;
#else
    _glVertexAttrib1f = glad_glVertexAttrib1f;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib1fv = glad_debug_glVertexAttrib1fv;
#else
    _glVertexAttrib1fv = glad_glVertexAttrib1fv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib1s = glad_debug_glVertexAttrib1s;
#else
    _glVertexAttrib1s = glad_glVertexAttrib1s;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib1sv = glad_debug_glVertexAttrib1sv;
#else
    _glVertexAttrib1sv = glad_glVertexAttrib1sv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib2d = glad_debug_glVertexAttrib2d;
#else
    _glVertexAttrib2d = glad_glVertexAttrib2d;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib2dv = glad_debug_glVertexAttrib2dv;
#else
    _glVertexAttrib2dv = glad_glVertexAttrib2dv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib2f = glad_debug_glVertexAttrib2f;
#else
    _glVertexAttrib2f = glad_glVertexAttrib2f;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib2fv = glad_debug_glVertexAttrib2fv;
#else
    _glVertexAttrib2fv = glad_glVertexAttrib2fv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib2s = glad_debug_glVertexAttrib2s;
#else
    _glVertexAttrib2s = glad_glVertexAttrib2s;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib2sv = glad_debug_glVertexAttrib2sv;
#else
    _glVertexAttrib2sv = glad_glVertexAttrib2sv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib3d = glad_debug_glVertexAttrib3d;
#else
    _glVertexAttrib3d = glad_glVertexAttrib3d;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib3dv = glad_debug_glVertexAttrib3dv;
#else
    _glVertexAttrib3dv = glad_glVertexAttrib3dv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib3f = glad_debug_glVertexAttrib3f;
#else
    _glVertexAttrib3f = glad_glVertexAttrib3f;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib3fv = glad_debug_glVertexAttrib3fv;
#else
    _glVertexAttrib3fv = glad_glVertexAttrib3fv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib3s = glad_debug_glVertexAttrib3s;
#else
    _glVertexAttrib3s = glad_glVertexAttrib3s;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib3sv = glad_debug_glVertexAttrib3sv;
#else
    _glVertexAttrib3sv = glad_glVertexAttrib3sv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4Nbv = glad_debug_glVertexAttrib4Nbv;
#else
    _glVertexAttrib4Nbv = glad_glVertexAttrib4Nbv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4Niv = glad_debug_glVertexAttrib4Niv;
#else
    _glVertexAttrib4Niv = glad_glVertexAttrib4Niv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4Nsv = glad_debug_glVertexAttrib4Nsv;
#else
    _glVertexAttrib4Nsv = glad_glVertexAttrib4Nsv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4Nub = glad_debug_glVertexAttrib4Nub;
#else
    _glVertexAttrib4Nub = glad_glVertexAttrib4Nub;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4Nubv = glad_debug_glVertexAttrib4Nubv;
#else
    _glVertexAttrib4Nubv = glad_glVertexAttrib4Nubv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4Nuiv = glad_debug_glVertexAttrib4Nuiv;
#else
    _glVertexAttrib4Nuiv = glad_glVertexAttrib4Nuiv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4Nusv = glad_debug_glVertexAttrib4Nusv;
#else
    _glVertexAttrib4Nusv = glad_glVertexAttrib4Nusv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4bv = glad_debug_glVertexAttrib4bv;
#else
    _glVertexAttrib4bv = glad_glVertexAttrib4bv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4d = glad_debug_glVertexAttrib4d;
#else
    _glVertexAttrib4d = glad_glVertexAttrib4d;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4dv = glad_debug_glVertexAttrib4dv;
#else
    _glVertexAttrib4dv = glad_glVertexAttrib4dv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4f = glad_debug_glVertexAttrib4f;
#else
    _glVertexAttrib4f = glad_glVertexAttrib4f;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4fv = glad_debug_glVertexAttrib4fv;
#else
    _glVertexAttrib4fv = glad_glVertexAttrib4fv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4iv = glad_debug_glVertexAttrib4iv;
#else
    _glVertexAttrib4iv = glad_glVertexAttrib4iv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4s = glad_debug_glVertexAttrib4s;
#else
    _glVertexAttrib4s = glad_glVertexAttrib4s;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4sv = glad_debug_glVertexAttrib4sv;
#else
    _glVertexAttrib4sv = glad_glVertexAttrib4sv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4ubv = glad_debug_glVertexAttrib4ubv;
#else
    _glVertexAttrib4ubv = glad_glVertexAttrib4ubv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4uiv = glad_debug_glVertexAttrib4uiv;
#else
    _glVertexAttrib4uiv = glad_glVertexAttrib4uiv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttrib4usv = glad_debug_glVertexAttrib4usv;
#else
    _glVertexAttrib4usv = glad_glVertexAttrib4usv;
#endif
#ifdef GLAD_DEBUG
    _glVertexAttribPointer = glad_debug_glVertexAttribPointer;
#else
    _glVertexAttribPointer = glad_glVertexAttribPointer;
#endif
#ifdef GLAD_DEBUG
    _glBindBufferARB = glad_debug_glBindBufferARB;
#else
    _glBindBufferARB = glad_glBindBufferARB;
#endif
#ifdef GLAD_DEBUG
    _glDeleteBuffersARB = glad_debug_glDeleteBuffersARB;
#else
    _glDeleteBuffersARB = glad_glDeleteBuffersARB;
#endif
#ifdef GLAD_DEBUG
    _glGenBuffersARB = glad_debug_glGenBuffersARB;
#else
    _glGenBuffersARB = glad_glGenBuffersARB;
#endif
#ifdef GLAD_DEBUG
    _glIsBufferARB = glad_debug_glIsBufferARB;
#else
    _glIsBufferARB = glad_glIsBufferARB;
#endif
#ifdef GLAD_DEBUG
    _glBufferDataARB = glad_debug_glBufferDataARB;
#else
    _glBufferDataARB = glad_glBufferDataARB;
#endif
#ifdef GLAD_DEBUG
    _glBufferSubDataARB = glad_debug_glBufferSubDataARB;
#else
    _glBufferSubDataARB = glad_glBufferSubDataARB;
#endif
#ifdef GLAD_DEBUG
    _glGetBufferSubDataARB = glad_debug_glGetBufferSubDataARB;
#else
    _glGetBufferSubDataARB = glad_glGetBufferSubDataARB;
#endif
#ifdef GLAD_DEBUG
    _glMapBufferARB = glad_debug_glMapBufferARB;
#else
    _glMapBufferARB = glad_glMapBufferARB;
#endif
#ifdef GLAD_DEBUG
    _glUnmapBufferARB = glad_debug_glUnmapBufferARB;
#else
    _glUnmapBufferARB = glad_glUnmapBufferARB;
#endif
#ifdef GLAD_DEBUG
    _glGetBufferParameterivARB = glad_debug_glGetBufferParameterivARB;
#else
    _glGetBufferParameterivARB = glad_glGetBufferParameterivARB;
#endif
#ifdef GLAD_DEBUG
    _glGetBufferPointervARB = glad_debug_glGetBufferPointervARB;
#else
    _glGetBufferPointervARB = glad_glGetBufferPointervARB;
#endif
#ifdef GLAD_DEBUG
    _glBindVertexArray = glad_debug_glBindVertexArray;
#else
    _glBindVertexArray = glad_glBindVertexArray;
#endif
#ifdef GLAD_DEBUG
    _glDeleteVertexArrays = glad_debug_glDeleteVertexArrays;
#else
    _glDeleteVertexArrays = glad_glDeleteVertexArrays;
#endif
#ifdef GLAD_DEBUG
    _glGenVertexArrays = glad_debug_glGenVertexArrays;
#else
    _glGenVertexArrays = glad_glGenVertexArrays;
#endif
#ifdef GLAD_DEBUG
    _glIsVertexArray = glad_debug_glIsVertexArray;
#else
    _glIsVertexArray = glad_glIsVertexArray;
#endif
#ifdef GLAD_DEBUG
    _glIsRenderbuffer = glad_debug_glIsRenderbuffer;
#else
    _glIsRenderbuffer = glad_glIsRenderbuffer;
#endif
#ifdef GLAD_DEBUG
    _glBindRenderbuffer = glad_debug_glBindRenderbuffer;
#else
    _glBindRenderbuffer = glad_glBindRenderbuffer;
#endif
#ifdef GLAD_DEBUG
    _glDeleteRenderbuffers = glad_debug_glDeleteRenderbuffers;
#else
    _glDeleteRenderbuffers = glad_glDeleteRenderbuffers;
#endif
#ifdef GLAD_DEBUG
    _glGenRenderbuffers = glad_debug_glGenRenderbuffers;
#else
    _glGenRenderbuffers = glad_glGenRenderbuffers;
#endif
#ifdef GLAD_DEBUG
    _glRenderbufferStorage = glad_debug_glRenderbufferStorage;
#else
    _glRenderbufferStorage = glad_glRenderbufferStorage;
#endif
#ifdef GLAD_DEBUG
    _glGetRenderbufferParameteriv = glad_debug_glGetRenderbufferParameteriv;
#else
    _glGetRenderbufferParameteriv = glad_glGetRenderbufferParameteriv;
#endif
#ifdef GLAD_DEBUG
    _glIsFramebuffer = glad_debug_glIsFramebuffer;
#else
    _glIsFramebuffer = glad_glIsFramebuffer;
#endif
#ifdef GLAD_DEBUG
    _glBindFramebuffer = glad_debug_glBindFramebuffer;
#else
    _glBindFramebuffer = glad_glBindFramebuffer;
#endif
#ifdef GLAD_DEBUG
    _glDeleteFramebuffers = glad_debug_glDeleteFramebuffers;
#else
    _glDeleteFramebuffers = glad_glDeleteFramebuffers;
#endif
#ifdef GLAD_DEBUG
    _glGenFramebuffers = glad_debug_glGenFramebuffers;
#else
    _glGenFramebuffers = glad_glGenFramebuffers;
#endif
#ifdef GLAD_DEBUG
    _glCheckFramebufferStatus = glad_debug_glCheckFramebufferStatus;
#else
    _glCheckFramebufferStatus = glad_glCheckFramebufferStatus;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferTexture1D = glad_debug_glFramebufferTexture1D;
#else
    _glFramebufferTexture1D = glad_glFramebufferTexture1D;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferTexture2D = glad_debug_glFramebufferTexture2D;
#else
    _glFramebufferTexture2D = glad_glFramebufferTexture2D;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferTexture3D = glad_debug_glFramebufferTexture3D;
#else
    _glFramebufferTexture3D = glad_glFramebufferTexture3D;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferRenderbuffer = glad_debug_glFramebufferRenderbuffer;
#else
    _glFramebufferRenderbuffer = glad_glFramebufferRenderbuffer;
#endif
#ifdef GLAD_DEBUG
    _glGetFramebufferAttachmentParameteriv = glad_debug_glGetFramebufferAttachmentParameteriv;
#else
    _glGetFramebufferAttachmentParameteriv = glad_glGetFramebufferAttachmentParameteriv;
#endif
#ifdef GLAD_DEBUG
    _glGenerateMipmap = glad_debug_glGenerateMipmap;
#else
    _glGenerateMipmap = glad_glGenerateMipmap;
#endif
#ifdef GLAD_DEBUG
    _glBlitFramebuffer = glad_debug_glBlitFramebuffer;
#else
    _glBlitFramebuffer = glad_glBlitFramebuffer;
#endif
#ifdef GLAD_DEBUG
    _glRenderbufferStorageMultisample = glad_debug_glRenderbufferStorageMultisample;
#else
    _glRenderbufferStorageMultisample = glad_glRenderbufferStorageMultisample;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferTextureLayer = glad_debug_glFramebufferTextureLayer;
#else
    _glFramebufferTextureLayer = glad_glFramebufferTextureLayer;
#endif
#ifdef GLAD_DEBUG
    _glIsRenderbufferEXT = glad_debug_glIsRenderbufferEXT;
#else
    _glIsRenderbufferEXT = glad_glIsRenderbufferEXT;
#endif
#ifdef GLAD_DEBUG
    _glBindRenderbufferEXT = glad_debug_glBindRenderbufferEXT;
#else
    _glBindRenderbufferEXT = glad_glBindRenderbufferEXT;
#endif
#ifdef GLAD_DEBUG
    _glDeleteRenderbuffersEXT = glad_debug_glDeleteRenderbuffersEXT;
#else
    _glDeleteRenderbuffersEXT = glad_glDeleteRenderbuffersEXT;
#endif
#ifdef GLAD_DEBUG
    _glGenRenderbuffersEXT = glad_debug_glGenRenderbuffersEXT;
#else
    _glGenRenderbuffersEXT = glad_glGenRenderbuffersEXT;
#endif
#ifdef GLAD_DEBUG
    _glRenderbufferStorageEXT = glad_debug_glRenderbufferStorageEXT;
#else
    _glRenderbufferStorageEXT = glad_glRenderbufferStorageEXT;
#endif
#ifdef GLAD_DEBUG
    _glGetRenderbufferParameterivEXT = glad_debug_glGetRenderbufferParameterivEXT;
#else
    _glGetRenderbufferParameterivEXT = glad_glGetRenderbufferParameterivEXT;
#endif
#ifdef GLAD_DEBUG
    _glIsFramebufferEXT = glad_debug_glIsFramebufferEXT;
#else
    _glIsFramebufferEXT = glad_glIsFramebufferEXT;
#endif
#ifdef GLAD_DEBUG
    _glBindFramebufferEXT = glad_debug_glBindFramebufferEXT;
#else
    _glBindFramebufferEXT = glad_glBindFramebufferEXT;
#endif
#ifdef GLAD_DEBUG
    _glDeleteFramebuffersEXT = glad_debug_glDeleteFramebuffersEXT;
#else
    _glDeleteFramebuffersEXT = glad_glDeleteFramebuffersEXT;
#endif
#ifdef GLAD_DEBUG
    _glGenFramebuffersEXT = glad_debug_glGenFramebuffersEXT;
#else
    _glGenFramebuffersEXT = glad_glGenFramebuffersEXT;
#endif
#ifdef GLAD_DEBUG
    _glCheckFramebufferStatusEXT = glad_debug_glCheckFramebufferStatusEXT;
#else
    _glCheckFramebufferStatusEXT = glad_glCheckFramebufferStatusEXT;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferTexture1DEXT = glad_debug_glFramebufferTexture1DEXT;
#else
    _glFramebufferTexture1DEXT = glad_glFramebufferTexture1DEXT;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferTexture2DEXT = glad_debug_glFramebufferTexture2DEXT;
#else
    _glFramebufferTexture2DEXT = glad_glFramebufferTexture2DEXT;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferTexture3DEXT = glad_debug_glFramebufferTexture3DEXT;
#else
    _glFramebufferTexture3DEXT = glad_glFramebufferTexture3DEXT;
#endif
#ifdef GLAD_DEBUG
    _glFramebufferRenderbufferEXT = glad_debug_glFramebufferRenderbufferEXT;
#else
    _glFramebufferRenderbufferEXT = glad_glFramebufferRenderbufferEXT;
#endif
#ifdef GLAD_DEBUG
    _glGetFramebufferAttachmentParameterivEXT = glad_debug_glGetFramebufferAttachmentParameterivEXT;
#else
    _glGetFramebufferAttachmentParameterivEXT = glad_glGetFramebufferAttachmentParameterivEXT;
#endif
#ifdef GLAD_DEBUG
    _glGenerateMipmapEXT = glad_debug_glGenerateMipmapEXT;
#else
    _glGenerateMipmapEXT = glad_glGenerateMipmapEXT;
#endif
#ifdef GLAD_DEBUG
    _glBindVertexArrayAPPLE = glad_debug_glBindVertexArrayAPPLE;
#else
    _glBindVertexArrayAPPLE = glad_glBindVertexArrayAPPLE;
#endif
#ifdef GLAD_DEBUG
    _glDeleteVertexArraysAPPLE = glad_debug_glDeleteVertexArraysAPPLE;
#else
    _glDeleteVertexArraysAPPLE = glad_glDeleteVertexArraysAPPLE;
#endif
#ifdef GLAD_DEBUG
    _glGenVertexArraysAPPLE = glad_debug_glGenVertexArraysAPPLE;
#else
    _glGenVertexArraysAPPLE = glad_glGenVertexArraysAPPLE;
#endif
#ifdef GLAD_DEBUG
    _glIsVertexArrayAPPLE = glad_debug_glIsVertexArrayAPPLE;
#else
    _glIsVertexArrayAPPLE = glad_glIsVertexArrayAPPLE;
#endif
} // load_functions

template class OSGLFunctions<true>;

} // namespace Natron

