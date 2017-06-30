/*
 * THIS FILE WAS GENERATED AUTOMATICALLY FROM glad.h by tools/utils/generateGLIncludes, DO NOT EDIT
 */

#ifndef OSGLFUNCTIONS_H
#define OSGLFUNCTIONS_H

#include <cstring> // memset

#include <glad/glad.h> // libs.pri sets the right include path. glads.h may set GLAD_DEBUG
#include "Global/GLObfuscate.h"

#include "Global/Macros.h"

#ifdef GLAD_DEBUG
#define glad_defined(glFunc) glad_debug_ ## glFunc
#else
#define glad_defined(glFunc) glad_ ## glFunc
#endif


NATRON_NAMESPACE_ENTER

template <bool USEOPENGL>
class OSGLFunctions
{
    static OSGLFunctions<USEOPENGL>& getInstance()
    {
        static OSGLFunctions<USEOPENGL> instance;

        return instance;
    }

    // load function, implemented in _gl.h and _mesa.h
    void load_functions();

    // private constructor
    OSGLFunctions() { std::memset(this, 0, sizeof(*this)); load_functions(); }

    PFNGLCULLFACEPROC _glCullFace;
    PFNGLFRONTFACEPROC _glFrontFace;
    PFNGLHINTPROC _glHint;
    PFNGLLINEWIDTHPROC _glLineWidth;
    PFNGLPOINTSIZEPROC _glPointSize;
    PFNGLPOLYGONMODEPROC _glPolygonMode;
    PFNGLSCISSORPROC _glScissor;
    PFNGLTEXPARAMETERFPROC _glTexParameterf;
    PFNGLTEXPARAMETERFVPROC _glTexParameterfv;
    PFNGLTEXPARAMETERIPROC _glTexParameteri;
    PFNGLTEXPARAMETERIVPROC _glTexParameteriv;
    PFNGLTEXIMAGE1DPROC _glTexImage1D;
    PFNGLTEXIMAGE2DPROC _glTexImage2D;
    PFNGLDRAWBUFFERPROC _glDrawBuffer;
    PFNGLCLEARPROC _glClear;
    PFNGLCLEARCOLORPROC _glClearColor;
    PFNGLCLEARSTENCILPROC _glClearStencil;
    PFNGLCLEARDEPTHPROC _glClearDepth;
    PFNGLSTENCILMASKPROC _glStencilMask;
    PFNGLCOLORMASKPROC _glColorMask;
    PFNGLDEPTHMASKPROC _glDepthMask;
    PFNGLDISABLEPROC _glDisable;
    PFNGLENABLEPROC _glEnable;
    PFNGLFINISHPROC _glFinish;
    PFNGLFLUSHPROC _glFlush;
    PFNGLBLENDFUNCPROC _glBlendFunc;
    PFNGLLOGICOPPROC _glLogicOp;
    PFNGLSTENCILFUNCPROC _glStencilFunc;
    PFNGLSTENCILOPPROC _glStencilOp;
    PFNGLDEPTHFUNCPROC _glDepthFunc;
    PFNGLPIXELSTOREFPROC _glPixelStoref;
    PFNGLPIXELSTOREIPROC _glPixelStorei;
    PFNGLREADBUFFERPROC _glReadBuffer;
    PFNGLREADPIXELSPROC _glReadPixels;
    PFNGLGETBOOLEANVPROC _glGetBooleanv;
    PFNGLGETDOUBLEVPROC _glGetDoublev;
    PFNGLGETERRORPROC _glGetError;
    PFNGLGETFLOATVPROC _glGetFloatv;
    PFNGLGETINTEGERVPROC _glGetIntegerv;
    PFNGLGETSTRINGPROC _glGetString;
    PFNGLGETTEXIMAGEPROC _glGetTexImage;
    PFNGLGETTEXPARAMETERFVPROC _glGetTexParameterfv;
    PFNGLGETTEXPARAMETERIVPROC _glGetTexParameteriv;
    PFNGLGETTEXLEVELPARAMETERFVPROC _glGetTexLevelParameterfv;
    PFNGLGETTEXLEVELPARAMETERIVPROC _glGetTexLevelParameteriv;
    PFNGLISENABLEDPROC _glIsEnabled;
    PFNGLDEPTHRANGEPROC _glDepthRange;
    PFNGLVIEWPORTPROC _glViewport;
    PFNGLNEWLISTPROC _glNewList;
    PFNGLENDLISTPROC _glEndList;
    PFNGLCALLLISTPROC _glCallList;
    PFNGLCALLLISTSPROC _glCallLists;
    PFNGLDELETELISTSPROC _glDeleteLists;
    PFNGLGENLISTSPROC _glGenLists;
    PFNGLLISTBASEPROC _glListBase;
    PFNGLBEGINPROC _glBegin;
    PFNGLBITMAPPROC _glBitmap;
    PFNGLCOLOR3BPROC _glColor3b;
    PFNGLCOLOR3BVPROC _glColor3bv;
    PFNGLCOLOR3DPROC _glColor3d;
    PFNGLCOLOR3DVPROC _glColor3dv;
    PFNGLCOLOR3FPROC _glColor3f;
    PFNGLCOLOR3FVPROC _glColor3fv;
    PFNGLCOLOR3IPROC _glColor3i;
    PFNGLCOLOR3IVPROC _glColor3iv;
    PFNGLCOLOR3SPROC _glColor3s;
    PFNGLCOLOR3SVPROC _glColor3sv;
    PFNGLCOLOR3UBPROC _glColor3ub;
    PFNGLCOLOR3UBVPROC _glColor3ubv;
    PFNGLCOLOR3UIPROC _glColor3ui;
    PFNGLCOLOR3UIVPROC _glColor3uiv;
    PFNGLCOLOR3USPROC _glColor3us;
    PFNGLCOLOR3USVPROC _glColor3usv;
    PFNGLCOLOR4BPROC _glColor4b;
    PFNGLCOLOR4BVPROC _glColor4bv;
    PFNGLCOLOR4DPROC _glColor4d;
    PFNGLCOLOR4DVPROC _glColor4dv;
    PFNGLCOLOR4FPROC _glColor4f;
    PFNGLCOLOR4FVPROC _glColor4fv;
    PFNGLCOLOR4IPROC _glColor4i;
    PFNGLCOLOR4IVPROC _glColor4iv;
    PFNGLCOLOR4SPROC _glColor4s;
    PFNGLCOLOR4SVPROC _glColor4sv;
    PFNGLCOLOR4UBPROC _glColor4ub;
    PFNGLCOLOR4UBVPROC _glColor4ubv;
    PFNGLCOLOR4UIPROC _glColor4ui;
    PFNGLCOLOR4UIVPROC _glColor4uiv;
    PFNGLCOLOR4USPROC _glColor4us;
    PFNGLCOLOR4USVPROC _glColor4usv;
    PFNGLEDGEFLAGPROC _glEdgeFlag;
    PFNGLEDGEFLAGVPROC _glEdgeFlagv;
    PFNGLENDPROC _glEnd;
    PFNGLINDEXDPROC _glIndexd;
    PFNGLINDEXDVPROC _glIndexdv;
    PFNGLINDEXFPROC _glIndexf;
    PFNGLINDEXFVPROC _glIndexfv;
    PFNGLINDEXIPROC _glIndexi;
    PFNGLINDEXIVPROC _glIndexiv;
    PFNGLINDEXSPROC _glIndexs;
    PFNGLINDEXSVPROC _glIndexsv;
    PFNGLNORMAL3BPROC _glNormal3b;
    PFNGLNORMAL3BVPROC _glNormal3bv;
    PFNGLNORMAL3DPROC _glNormal3d;
    PFNGLNORMAL3DVPROC _glNormal3dv;
    PFNGLNORMAL3FPROC _glNormal3f;
    PFNGLNORMAL3FVPROC _glNormal3fv;
    PFNGLNORMAL3IPROC _glNormal3i;
    PFNGLNORMAL3IVPROC _glNormal3iv;
    PFNGLNORMAL3SPROC _glNormal3s;
    PFNGLNORMAL3SVPROC _glNormal3sv;
    PFNGLRASTERPOS2DPROC _glRasterPos2d;
    PFNGLRASTERPOS2DVPROC _glRasterPos2dv;
    PFNGLRASTERPOS2FPROC _glRasterPos2f;
    PFNGLRASTERPOS2FVPROC _glRasterPos2fv;
    PFNGLRASTERPOS2IPROC _glRasterPos2i;
    PFNGLRASTERPOS2IVPROC _glRasterPos2iv;
    PFNGLRASTERPOS2SPROC _glRasterPos2s;
    PFNGLRASTERPOS2SVPROC _glRasterPos2sv;
    PFNGLRASTERPOS3DPROC _glRasterPos3d;
    PFNGLRASTERPOS3DVPROC _glRasterPos3dv;
    PFNGLRASTERPOS3FPROC _glRasterPos3f;
    PFNGLRASTERPOS3FVPROC _glRasterPos3fv;
    PFNGLRASTERPOS3IPROC _glRasterPos3i;
    PFNGLRASTERPOS3IVPROC _glRasterPos3iv;
    PFNGLRASTERPOS3SPROC _glRasterPos3s;
    PFNGLRASTERPOS3SVPROC _glRasterPos3sv;
    PFNGLRASTERPOS4DPROC _glRasterPos4d;
    PFNGLRASTERPOS4DVPROC _glRasterPos4dv;
    PFNGLRASTERPOS4FPROC _glRasterPos4f;
    PFNGLRASTERPOS4FVPROC _glRasterPos4fv;
    PFNGLRASTERPOS4IPROC _glRasterPos4i;
    PFNGLRASTERPOS4IVPROC _glRasterPos4iv;
    PFNGLRASTERPOS4SPROC _glRasterPos4s;
    PFNGLRASTERPOS4SVPROC _glRasterPos4sv;
    PFNGLRECTDPROC _glRectd;
    PFNGLRECTDVPROC _glRectdv;
    PFNGLRECTFPROC _glRectf;
    PFNGLRECTFVPROC _glRectfv;
    PFNGLRECTIPROC _glRecti;
    PFNGLRECTIVPROC _glRectiv;
    PFNGLRECTSPROC _glRects;
    PFNGLRECTSVPROC _glRectsv;
    PFNGLTEXCOORD1DPROC _glTexCoord1d;
    PFNGLTEXCOORD1DVPROC _glTexCoord1dv;
    PFNGLTEXCOORD1FPROC _glTexCoord1f;
    PFNGLTEXCOORD1FVPROC _glTexCoord1fv;
    PFNGLTEXCOORD1IPROC _glTexCoord1i;
    PFNGLTEXCOORD1IVPROC _glTexCoord1iv;
    PFNGLTEXCOORD1SPROC _glTexCoord1s;
    PFNGLTEXCOORD1SVPROC _glTexCoord1sv;
    PFNGLTEXCOORD2DPROC _glTexCoord2d;
    PFNGLTEXCOORD2DVPROC _glTexCoord2dv;
    PFNGLTEXCOORD2FPROC _glTexCoord2f;
    PFNGLTEXCOORD2FVPROC _glTexCoord2fv;
    PFNGLTEXCOORD2IPROC _glTexCoord2i;
    PFNGLTEXCOORD2IVPROC _glTexCoord2iv;
    PFNGLTEXCOORD2SPROC _glTexCoord2s;
    PFNGLTEXCOORD2SVPROC _glTexCoord2sv;
    PFNGLTEXCOORD3DPROC _glTexCoord3d;
    PFNGLTEXCOORD3DVPROC _glTexCoord3dv;
    PFNGLTEXCOORD3FPROC _glTexCoord3f;
    PFNGLTEXCOORD3FVPROC _glTexCoord3fv;
    PFNGLTEXCOORD3IPROC _glTexCoord3i;
    PFNGLTEXCOORD3IVPROC _glTexCoord3iv;
    PFNGLTEXCOORD3SPROC _glTexCoord3s;
    PFNGLTEXCOORD3SVPROC _glTexCoord3sv;
    PFNGLTEXCOORD4DPROC _glTexCoord4d;
    PFNGLTEXCOORD4DVPROC _glTexCoord4dv;
    PFNGLTEXCOORD4FPROC _glTexCoord4f;
    PFNGLTEXCOORD4FVPROC _glTexCoord4fv;
    PFNGLTEXCOORD4IPROC _glTexCoord4i;
    PFNGLTEXCOORD4IVPROC _glTexCoord4iv;
    PFNGLTEXCOORD4SPROC _glTexCoord4s;
    PFNGLTEXCOORD4SVPROC _glTexCoord4sv;
    PFNGLVERTEX2DPROC _glVertex2d;
    PFNGLVERTEX2DVPROC _glVertex2dv;
    PFNGLVERTEX2FPROC _glVertex2f;
    PFNGLVERTEX2FVPROC _glVertex2fv;
    PFNGLVERTEX2IPROC _glVertex2i;
    PFNGLVERTEX2IVPROC _glVertex2iv;
    PFNGLVERTEX2SPROC _glVertex2s;
    PFNGLVERTEX2SVPROC _glVertex2sv;
    PFNGLVERTEX3DPROC _glVertex3d;
    PFNGLVERTEX3DVPROC _glVertex3dv;
    PFNGLVERTEX3FPROC _glVertex3f;
    PFNGLVERTEX3FVPROC _glVertex3fv;
    PFNGLVERTEX3IPROC _glVertex3i;
    PFNGLVERTEX3IVPROC _glVertex3iv;
    PFNGLVERTEX3SPROC _glVertex3s;
    PFNGLVERTEX3SVPROC _glVertex3sv;
    PFNGLVERTEX4DPROC _glVertex4d;
    PFNGLVERTEX4DVPROC _glVertex4dv;
    PFNGLVERTEX4FPROC _glVertex4f;
    PFNGLVERTEX4FVPROC _glVertex4fv;
    PFNGLVERTEX4IPROC _glVertex4i;
    PFNGLVERTEX4IVPROC _glVertex4iv;
    PFNGLVERTEX4SPROC _glVertex4s;
    PFNGLVERTEX4SVPROC _glVertex4sv;
    PFNGLCLIPPLANEPROC _glClipPlane;
    PFNGLCOLORMATERIALPROC _glColorMaterial;
    PFNGLFOGFPROC _glFogf;
    PFNGLFOGFVPROC _glFogfv;
    PFNGLFOGIPROC _glFogi;
    PFNGLFOGIVPROC _glFogiv;
    PFNGLLIGHTFPROC _glLightf;
    PFNGLLIGHTFVPROC _glLightfv;
    PFNGLLIGHTIPROC _glLighti;
    PFNGLLIGHTIVPROC _glLightiv;
    PFNGLLIGHTMODELFPROC _glLightModelf;
    PFNGLLIGHTMODELFVPROC _glLightModelfv;
    PFNGLLIGHTMODELIPROC _glLightModeli;
    PFNGLLIGHTMODELIVPROC _glLightModeliv;
    PFNGLLINESTIPPLEPROC _glLineStipple;
    PFNGLMATERIALFPROC _glMaterialf;
    PFNGLMATERIALFVPROC _glMaterialfv;
    PFNGLMATERIALIPROC _glMateriali;
    PFNGLMATERIALIVPROC _glMaterialiv;
    PFNGLPOLYGONSTIPPLEPROC _glPolygonStipple;
    PFNGLSHADEMODELPROC _glShadeModel;
    PFNGLTEXENVFPROC _glTexEnvf;
    PFNGLTEXENVFVPROC _glTexEnvfv;
    PFNGLTEXENVIPROC _glTexEnvi;
    PFNGLTEXENVIVPROC _glTexEnviv;
    PFNGLTEXGENDPROC _glTexGend;
    PFNGLTEXGENDVPROC _glTexGendv;
    PFNGLTEXGENFPROC _glTexGenf;
    PFNGLTEXGENFVPROC _glTexGenfv;
    PFNGLTEXGENIPROC _glTexGeni;
    PFNGLTEXGENIVPROC _glTexGeniv;
    PFNGLFEEDBACKBUFFERPROC _glFeedbackBuffer;
    PFNGLSELECTBUFFERPROC _glSelectBuffer;
    PFNGLRENDERMODEPROC _glRenderMode;
    PFNGLINITNAMESPROC _glInitNames;
    PFNGLLOADNAMEPROC _glLoadName;
    PFNGLPASSTHROUGHPROC _glPassThrough;
    PFNGLPOPNAMEPROC _glPopName;
    PFNGLPUSHNAMEPROC _glPushName;
    PFNGLCLEARACCUMPROC _glClearAccum;
    PFNGLCLEARINDEXPROC _glClearIndex;
    PFNGLINDEXMASKPROC _glIndexMask;
    PFNGLACCUMPROC _glAccum;
    PFNGLPOPATTRIBPROC _glPopAttrib;
    PFNGLPUSHATTRIBPROC _glPushAttrib;
    PFNGLMAP1DPROC _glMap1d;
    PFNGLMAP1FPROC _glMap1f;
    PFNGLMAP2DPROC _glMap2d;
    PFNGLMAP2FPROC _glMap2f;
    PFNGLMAPGRID1DPROC _glMapGrid1d;
    PFNGLMAPGRID1FPROC _glMapGrid1f;
    PFNGLMAPGRID2DPROC _glMapGrid2d;
    PFNGLMAPGRID2FPROC _glMapGrid2f;
    PFNGLEVALCOORD1DPROC _glEvalCoord1d;
    PFNGLEVALCOORD1DVPROC _glEvalCoord1dv;
    PFNGLEVALCOORD1FPROC _glEvalCoord1f;
    PFNGLEVALCOORD1FVPROC _glEvalCoord1fv;
    PFNGLEVALCOORD2DPROC _glEvalCoord2d;
    PFNGLEVALCOORD2DVPROC _glEvalCoord2dv;
    PFNGLEVALCOORD2FPROC _glEvalCoord2f;
    PFNGLEVALCOORD2FVPROC _glEvalCoord2fv;
    PFNGLEVALMESH1PROC _glEvalMesh1;
    PFNGLEVALPOINT1PROC _glEvalPoint1;
    PFNGLEVALMESH2PROC _glEvalMesh2;
    PFNGLEVALPOINT2PROC _glEvalPoint2;
    PFNGLALPHAFUNCPROC _glAlphaFunc;
    PFNGLPIXELZOOMPROC _glPixelZoom;
    PFNGLPIXELTRANSFERFPROC _glPixelTransferf;
    PFNGLPIXELTRANSFERIPROC _glPixelTransferi;
    PFNGLPIXELMAPFVPROC _glPixelMapfv;
    PFNGLPIXELMAPUIVPROC _glPixelMapuiv;
    PFNGLPIXELMAPUSVPROC _glPixelMapusv;
    PFNGLCOPYPIXELSPROC _glCopyPixels;
    PFNGLDRAWPIXELSPROC _glDrawPixels;
    PFNGLGETCLIPPLANEPROC _glGetClipPlane;
    PFNGLGETLIGHTFVPROC _glGetLightfv;
    PFNGLGETLIGHTIVPROC _glGetLightiv;
    PFNGLGETMAPDVPROC _glGetMapdv;
    PFNGLGETMAPFVPROC _glGetMapfv;
    PFNGLGETMAPIVPROC _glGetMapiv;
    PFNGLGETMATERIALFVPROC _glGetMaterialfv;
    PFNGLGETMATERIALIVPROC _glGetMaterialiv;
    PFNGLGETPIXELMAPFVPROC _glGetPixelMapfv;
    PFNGLGETPIXELMAPUIVPROC _glGetPixelMapuiv;
    PFNGLGETPIXELMAPUSVPROC _glGetPixelMapusv;
    PFNGLGETPOLYGONSTIPPLEPROC _glGetPolygonStipple;
    PFNGLGETTEXENVFVPROC _glGetTexEnvfv;
    PFNGLGETTEXENVIVPROC _glGetTexEnviv;
    PFNGLGETTEXGENDVPROC _glGetTexGendv;
    PFNGLGETTEXGENFVPROC _glGetTexGenfv;
    PFNGLGETTEXGENIVPROC _glGetTexGeniv;
    PFNGLISLISTPROC _glIsList;
    PFNGLFRUSTUMPROC _glFrustum;
    PFNGLLOADIDENTITYPROC _glLoadIdentity;
    PFNGLLOADMATRIXFPROC _glLoadMatrixf;
    PFNGLLOADMATRIXDPROC _glLoadMatrixd;
    PFNGLMATRIXMODEPROC _glMatrixMode;
    PFNGLMULTMATRIXFPROC _glMultMatrixf;
    PFNGLMULTMATRIXDPROC _glMultMatrixd;
    PFNGLORTHOPROC _glOrtho;
    PFNGLPOPMATRIXPROC _glPopMatrix;
    PFNGLPUSHMATRIXPROC _glPushMatrix;
    PFNGLROTATEDPROC _glRotated;
    PFNGLROTATEFPROC _glRotatef;
    PFNGLSCALEDPROC _glScaled;
    PFNGLSCALEFPROC _glScalef;
    PFNGLTRANSLATEDPROC _glTranslated;
    PFNGLTRANSLATEFPROC _glTranslatef;
    PFNGLDRAWARRAYSPROC _glDrawArrays;
    PFNGLDRAWELEMENTSPROC _glDrawElements;
    PFNGLGETPOINTERVPROC _glGetPointerv;
    PFNGLPOLYGONOFFSETPROC _glPolygonOffset;
    PFNGLCOPYTEXIMAGE1DPROC _glCopyTexImage1D;
    PFNGLCOPYTEXIMAGE2DPROC _glCopyTexImage2D;
    PFNGLCOPYTEXSUBIMAGE1DPROC _glCopyTexSubImage1D;
    PFNGLCOPYTEXSUBIMAGE2DPROC _glCopyTexSubImage2D;
    PFNGLTEXSUBIMAGE1DPROC _glTexSubImage1D;
    PFNGLTEXSUBIMAGE2DPROC _glTexSubImage2D;
    PFNGLBINDTEXTUREPROC _glBindTexture;
    PFNGLDELETETEXTURESPROC _glDeleteTextures;
    PFNGLGENTEXTURESPROC _glGenTextures;
    PFNGLISTEXTUREPROC _glIsTexture;
    PFNGLARRAYELEMENTPROC _glArrayElement;
    PFNGLCOLORPOINTERPROC _glColorPointer;
    PFNGLDISABLECLIENTSTATEPROC _glDisableClientState;
    PFNGLEDGEFLAGPOINTERPROC _glEdgeFlagPointer;
    PFNGLENABLECLIENTSTATEPROC _glEnableClientState;
    PFNGLINDEXPOINTERPROC _glIndexPointer;
    PFNGLINTERLEAVEDARRAYSPROC _glInterleavedArrays;
    PFNGLNORMALPOINTERPROC _glNormalPointer;
    PFNGLTEXCOORDPOINTERPROC _glTexCoordPointer;
    PFNGLVERTEXPOINTERPROC _glVertexPointer;
    PFNGLARETEXTURESRESIDENTPROC _glAreTexturesResident;
    PFNGLPRIORITIZETEXTURESPROC _glPrioritizeTextures;
    PFNGLINDEXUBPROC _glIndexub;
    PFNGLINDEXUBVPROC _glIndexubv;
    PFNGLPOPCLIENTATTRIBPROC _glPopClientAttrib;
    PFNGLPUSHCLIENTATTRIBPROC _glPushClientAttrib;
    PFNGLDRAWRANGEELEMENTSPROC _glDrawRangeElements;
    PFNGLTEXIMAGE3DPROC _glTexImage3D;
    PFNGLTEXSUBIMAGE3DPROC _glTexSubImage3D;
    PFNGLCOPYTEXSUBIMAGE3DPROC _glCopyTexSubImage3D;
    PFNGLACTIVETEXTUREPROC _glActiveTexture;
    PFNGLSAMPLECOVERAGEPROC _glSampleCoverage;
    PFNGLCOMPRESSEDTEXIMAGE3DPROC _glCompressedTexImage3D;
    PFNGLCOMPRESSEDTEXIMAGE2DPROC _glCompressedTexImage2D;
    PFNGLCOMPRESSEDTEXIMAGE1DPROC _glCompressedTexImage1D;
    PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC _glCompressedTexSubImage3D;
    PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC _glCompressedTexSubImage2D;
    PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC _glCompressedTexSubImage1D;
    PFNGLGETCOMPRESSEDTEXIMAGEPROC _glGetCompressedTexImage;
    PFNGLCLIENTACTIVETEXTUREPROC _glClientActiveTexture;
    PFNGLMULTITEXCOORD1DPROC _glMultiTexCoord1d;
    PFNGLMULTITEXCOORD1DVPROC _glMultiTexCoord1dv;
    PFNGLMULTITEXCOORD1FPROC _glMultiTexCoord1f;
    PFNGLMULTITEXCOORD1FVPROC _glMultiTexCoord1fv;
    PFNGLMULTITEXCOORD1IPROC _glMultiTexCoord1i;
    PFNGLMULTITEXCOORD1IVPROC _glMultiTexCoord1iv;
    PFNGLMULTITEXCOORD1SPROC _glMultiTexCoord1s;
    PFNGLMULTITEXCOORD1SVPROC _glMultiTexCoord1sv;
    PFNGLMULTITEXCOORD2DPROC _glMultiTexCoord2d;
    PFNGLMULTITEXCOORD2DVPROC _glMultiTexCoord2dv;
    PFNGLMULTITEXCOORD2FPROC _glMultiTexCoord2f;
    PFNGLMULTITEXCOORD2FVPROC _glMultiTexCoord2fv;
    PFNGLMULTITEXCOORD2IPROC _glMultiTexCoord2i;
    PFNGLMULTITEXCOORD2IVPROC _glMultiTexCoord2iv;
    PFNGLMULTITEXCOORD2SPROC _glMultiTexCoord2s;
    PFNGLMULTITEXCOORD2SVPROC _glMultiTexCoord2sv;
    PFNGLMULTITEXCOORD3DPROC _glMultiTexCoord3d;
    PFNGLMULTITEXCOORD3DVPROC _glMultiTexCoord3dv;
    PFNGLMULTITEXCOORD3FPROC _glMultiTexCoord3f;
    PFNGLMULTITEXCOORD3FVPROC _glMultiTexCoord3fv;
    PFNGLMULTITEXCOORD3IPROC _glMultiTexCoord3i;
    PFNGLMULTITEXCOORD3IVPROC _glMultiTexCoord3iv;
    PFNGLMULTITEXCOORD3SPROC _glMultiTexCoord3s;
    PFNGLMULTITEXCOORD3SVPROC _glMultiTexCoord3sv;
    PFNGLMULTITEXCOORD4DPROC _glMultiTexCoord4d;
    PFNGLMULTITEXCOORD4DVPROC _glMultiTexCoord4dv;
    PFNGLMULTITEXCOORD4FPROC _glMultiTexCoord4f;
    PFNGLMULTITEXCOORD4FVPROC _glMultiTexCoord4fv;
    PFNGLMULTITEXCOORD4IPROC _glMultiTexCoord4i;
    PFNGLMULTITEXCOORD4IVPROC _glMultiTexCoord4iv;
    PFNGLMULTITEXCOORD4SPROC _glMultiTexCoord4s;
    PFNGLMULTITEXCOORD4SVPROC _glMultiTexCoord4sv;
    PFNGLLOADTRANSPOSEMATRIXFPROC _glLoadTransposeMatrixf;
    PFNGLLOADTRANSPOSEMATRIXDPROC _glLoadTransposeMatrixd;
    PFNGLMULTTRANSPOSEMATRIXFPROC _glMultTransposeMatrixf;
    PFNGLMULTTRANSPOSEMATRIXDPROC _glMultTransposeMatrixd;
    PFNGLBLENDFUNCSEPARATEPROC _glBlendFuncSeparate;
    PFNGLMULTIDRAWARRAYSPROC _glMultiDrawArrays;
    PFNGLMULTIDRAWELEMENTSPROC _glMultiDrawElements;
    PFNGLPOINTPARAMETERFPROC _glPointParameterf;
    PFNGLPOINTPARAMETERFVPROC _glPointParameterfv;
    PFNGLPOINTPARAMETERIPROC _glPointParameteri;
    PFNGLPOINTPARAMETERIVPROC _glPointParameteriv;
    PFNGLFOGCOORDFPROC _glFogCoordf;
    PFNGLFOGCOORDFVPROC _glFogCoordfv;
    PFNGLFOGCOORDDPROC _glFogCoordd;
    PFNGLFOGCOORDDVPROC _glFogCoorddv;
    PFNGLFOGCOORDPOINTERPROC _glFogCoordPointer;
    PFNGLSECONDARYCOLOR3BPROC _glSecondaryColor3b;
    PFNGLSECONDARYCOLOR3BVPROC _glSecondaryColor3bv;
    PFNGLSECONDARYCOLOR3DPROC _glSecondaryColor3d;
    PFNGLSECONDARYCOLOR3DVPROC _glSecondaryColor3dv;
    PFNGLSECONDARYCOLOR3FPROC _glSecondaryColor3f;
    PFNGLSECONDARYCOLOR3FVPROC _glSecondaryColor3fv;
    PFNGLSECONDARYCOLOR3IPROC _glSecondaryColor3i;
    PFNGLSECONDARYCOLOR3IVPROC _glSecondaryColor3iv;
    PFNGLSECONDARYCOLOR3SPROC _glSecondaryColor3s;
    PFNGLSECONDARYCOLOR3SVPROC _glSecondaryColor3sv;
    PFNGLSECONDARYCOLOR3UBPROC _glSecondaryColor3ub;
    PFNGLSECONDARYCOLOR3UBVPROC _glSecondaryColor3ubv;
    PFNGLSECONDARYCOLOR3UIPROC _glSecondaryColor3ui;
    PFNGLSECONDARYCOLOR3UIVPROC _glSecondaryColor3uiv;
    PFNGLSECONDARYCOLOR3USPROC _glSecondaryColor3us;
    PFNGLSECONDARYCOLOR3USVPROC _glSecondaryColor3usv;
    PFNGLSECONDARYCOLORPOINTERPROC _glSecondaryColorPointer;
    PFNGLWINDOWPOS2DPROC _glWindowPos2d;
    PFNGLWINDOWPOS2DVPROC _glWindowPos2dv;
    PFNGLWINDOWPOS2FPROC _glWindowPos2f;
    PFNGLWINDOWPOS2FVPROC _glWindowPos2fv;
    PFNGLWINDOWPOS2IPROC _glWindowPos2i;
    PFNGLWINDOWPOS2IVPROC _glWindowPos2iv;
    PFNGLWINDOWPOS2SPROC _glWindowPos2s;
    PFNGLWINDOWPOS2SVPROC _glWindowPos2sv;
    PFNGLWINDOWPOS3DPROC _glWindowPos3d;
    PFNGLWINDOWPOS3DVPROC _glWindowPos3dv;
    PFNGLWINDOWPOS3FPROC _glWindowPos3f;
    PFNGLWINDOWPOS3FVPROC _glWindowPos3fv;
    PFNGLWINDOWPOS3IPROC _glWindowPos3i;
    PFNGLWINDOWPOS3IVPROC _glWindowPos3iv;
    PFNGLWINDOWPOS3SPROC _glWindowPos3s;
    PFNGLWINDOWPOS3SVPROC _glWindowPos3sv;
    PFNGLBLENDCOLORPROC _glBlendColor;
    PFNGLBLENDEQUATIONPROC _glBlendEquation;
    PFNGLGENQUERIESPROC _glGenQueries;
    PFNGLDELETEQUERIESPROC _glDeleteQueries;
    PFNGLISQUERYPROC _glIsQuery;
    PFNGLBEGINQUERYPROC _glBeginQuery;
    PFNGLENDQUERYPROC _glEndQuery;
    PFNGLGETQUERYIVPROC _glGetQueryiv;
    PFNGLGETQUERYOBJECTIVPROC _glGetQueryObjectiv;
    PFNGLGETQUERYOBJECTUIVPROC _glGetQueryObjectuiv;
    PFNGLBINDBUFFERPROC _glBindBuffer;
    PFNGLDELETEBUFFERSPROC _glDeleteBuffers;
    PFNGLGENBUFFERSPROC _glGenBuffers;
    PFNGLISBUFFERPROC _glIsBuffer;
    PFNGLBUFFERDATAPROC _glBufferData;
    PFNGLBUFFERSUBDATAPROC _glBufferSubData;
    PFNGLGETBUFFERSUBDATAPROC _glGetBufferSubData;
    PFNGLMAPBUFFERPROC _glMapBuffer;
    PFNGLUNMAPBUFFERPROC _glUnmapBuffer;
    PFNGLGETBUFFERPARAMETERIVPROC _glGetBufferParameteriv;
    PFNGLGETBUFFERPOINTERVPROC _glGetBufferPointerv;
    PFNGLBLENDEQUATIONSEPARATEPROC _glBlendEquationSeparate;
    PFNGLDRAWBUFFERSPROC _glDrawBuffers;
    PFNGLSTENCILOPSEPARATEPROC _glStencilOpSeparate;
    PFNGLSTENCILFUNCSEPARATEPROC _glStencilFuncSeparate;
    PFNGLSTENCILMASKSEPARATEPROC _glStencilMaskSeparate;
    PFNGLATTACHSHADERPROC _glAttachShader;
    PFNGLBINDATTRIBLOCATIONPROC _glBindAttribLocation;
    PFNGLCOMPILESHADERPROC _glCompileShader;
    PFNGLCREATEPROGRAMPROC _glCreateProgram;
    PFNGLCREATESHADERPROC _glCreateShader;
    PFNGLDELETEPROGRAMPROC _glDeleteProgram;
    PFNGLDELETESHADERPROC _glDeleteShader;
    PFNGLDETACHSHADERPROC _glDetachShader;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC _glDisableVertexAttribArray;
    PFNGLENABLEVERTEXATTRIBARRAYPROC _glEnableVertexAttribArray;
    PFNGLGETACTIVEATTRIBPROC _glGetActiveAttrib;
    PFNGLGETACTIVEUNIFORMPROC _glGetActiveUniform;
    PFNGLGETATTACHEDSHADERSPROC _glGetAttachedShaders;
    PFNGLGETATTRIBLOCATIONPROC _glGetAttribLocation;
    PFNGLGETPROGRAMIVPROC _glGetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC _glGetProgramInfoLog;
    PFNGLGETSHADERIVPROC _glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC _glGetShaderInfoLog;
    PFNGLGETSHADERSOURCEPROC _glGetShaderSource;
    PFNGLGETUNIFORMLOCATIONPROC _glGetUniformLocation;
    PFNGLGETUNIFORMFVPROC _glGetUniformfv;
    PFNGLGETUNIFORMIVPROC _glGetUniformiv;
    PFNGLGETVERTEXATTRIBDVPROC _glGetVertexAttribdv;
    PFNGLGETVERTEXATTRIBFVPROC _glGetVertexAttribfv;
    PFNGLGETVERTEXATTRIBIVPROC _glGetVertexAttribiv;
    PFNGLGETVERTEXATTRIBPOINTERVPROC _glGetVertexAttribPointerv;
    PFNGLISPROGRAMPROC _glIsProgram;
    PFNGLISSHADERPROC _glIsShader;
    PFNGLLINKPROGRAMPROC _glLinkProgram;
    PFNGLSHADERSOURCEPROC _glShaderSource;
    PFNGLUSEPROGRAMPROC _glUseProgram;
    PFNGLUNIFORM1FPROC _glUniform1f;
    PFNGLUNIFORM2FPROC _glUniform2f;
    PFNGLUNIFORM3FPROC _glUniform3f;
    PFNGLUNIFORM4FPROC _glUniform4f;
    PFNGLUNIFORM1IPROC _glUniform1i;
    PFNGLUNIFORM2IPROC _glUniform2i;
    PFNGLUNIFORM3IPROC _glUniform3i;
    PFNGLUNIFORM4IPROC _glUniform4i;
    PFNGLUNIFORM1FVPROC _glUniform1fv;
    PFNGLUNIFORM2FVPROC _glUniform2fv;
    PFNGLUNIFORM3FVPROC _glUniform3fv;
    PFNGLUNIFORM4FVPROC _glUniform4fv;
    PFNGLUNIFORM1IVPROC _glUniform1iv;
    PFNGLUNIFORM2IVPROC _glUniform2iv;
    PFNGLUNIFORM3IVPROC _glUniform3iv;
    PFNGLUNIFORM4IVPROC _glUniform4iv;
    PFNGLUNIFORMMATRIX2FVPROC _glUniformMatrix2fv;
    PFNGLUNIFORMMATRIX3FVPROC _glUniformMatrix3fv;
    PFNGLUNIFORMMATRIX4FVPROC _glUniformMatrix4fv;
    PFNGLVALIDATEPROGRAMPROC _glValidateProgram;
    PFNGLVERTEXATTRIB1DPROC _glVertexAttrib1d;
    PFNGLVERTEXATTRIB1DVPROC _glVertexAttrib1dv;
    PFNGLVERTEXATTRIB1FPROC _glVertexAttrib1f;
    PFNGLVERTEXATTRIB1FVPROC _glVertexAttrib1fv;
    PFNGLVERTEXATTRIB1SPROC _glVertexAttrib1s;
    PFNGLVERTEXATTRIB1SVPROC _glVertexAttrib1sv;
    PFNGLVERTEXATTRIB2DPROC _glVertexAttrib2d;
    PFNGLVERTEXATTRIB2DVPROC _glVertexAttrib2dv;
    PFNGLVERTEXATTRIB2FPROC _glVertexAttrib2f;
    PFNGLVERTEXATTRIB2FVPROC _glVertexAttrib2fv;
    PFNGLVERTEXATTRIB2SPROC _glVertexAttrib2s;
    PFNGLVERTEXATTRIB2SVPROC _glVertexAttrib2sv;
    PFNGLVERTEXATTRIB3DPROC _glVertexAttrib3d;
    PFNGLVERTEXATTRIB3DVPROC _glVertexAttrib3dv;
    PFNGLVERTEXATTRIB3FPROC _glVertexAttrib3f;
    PFNGLVERTEXATTRIB3FVPROC _glVertexAttrib3fv;
    PFNGLVERTEXATTRIB3SPROC _glVertexAttrib3s;
    PFNGLVERTEXATTRIB3SVPROC _glVertexAttrib3sv;
    PFNGLVERTEXATTRIB4NBVPROC _glVertexAttrib4Nbv;
    PFNGLVERTEXATTRIB4NIVPROC _glVertexAttrib4Niv;
    PFNGLVERTEXATTRIB4NSVPROC _glVertexAttrib4Nsv;
    PFNGLVERTEXATTRIB4NUBPROC _glVertexAttrib4Nub;
    PFNGLVERTEXATTRIB4NUBVPROC _glVertexAttrib4Nubv;
    PFNGLVERTEXATTRIB4NUIVPROC _glVertexAttrib4Nuiv;
    PFNGLVERTEXATTRIB4NUSVPROC _glVertexAttrib4Nusv;
    PFNGLVERTEXATTRIB4BVPROC _glVertexAttrib4bv;
    PFNGLVERTEXATTRIB4DPROC _glVertexAttrib4d;
    PFNGLVERTEXATTRIB4DVPROC _glVertexAttrib4dv;
    PFNGLVERTEXATTRIB4FPROC _glVertexAttrib4f;
    PFNGLVERTEXATTRIB4FVPROC _glVertexAttrib4fv;
    PFNGLVERTEXATTRIB4IVPROC _glVertexAttrib4iv;
    PFNGLVERTEXATTRIB4SPROC _glVertexAttrib4s;
    PFNGLVERTEXATTRIB4SVPROC _glVertexAttrib4sv;
    PFNGLVERTEXATTRIB4UBVPROC _glVertexAttrib4ubv;
    PFNGLVERTEXATTRIB4UIVPROC _glVertexAttrib4uiv;
    PFNGLVERTEXATTRIB4USVPROC _glVertexAttrib4usv;
    PFNGLVERTEXATTRIBPOINTERPROC _glVertexAttribPointer;
    PFNGLBINDBUFFERARBPROC _glBindBufferARB;
    PFNGLDELETEBUFFERSARBPROC _glDeleteBuffersARB;
    PFNGLGENBUFFERSARBPROC _glGenBuffersARB;
    PFNGLISBUFFERARBPROC _glIsBufferARB;
    PFNGLBUFFERDATAARBPROC _glBufferDataARB;
    PFNGLBUFFERSUBDATAARBPROC _glBufferSubDataARB;
    PFNGLGETBUFFERSUBDATAARBPROC _glGetBufferSubDataARB;
    PFNGLMAPBUFFERARBPROC _glMapBufferARB;
    PFNGLUNMAPBUFFERARBPROC _glUnmapBufferARB;
    PFNGLGETBUFFERPARAMETERIVARBPROC _glGetBufferParameterivARB;
    PFNGLGETBUFFERPOINTERVARBPROC _glGetBufferPointervARB;
    PFNGLBINDVERTEXARRAYPROC _glBindVertexArray;
    PFNGLDELETEVERTEXARRAYSPROC _glDeleteVertexArrays;
    PFNGLGENVERTEXARRAYSPROC _glGenVertexArrays;
    PFNGLISVERTEXARRAYPROC _glIsVertexArray;
    PFNGLISRENDERBUFFERPROC _glIsRenderbuffer;
    PFNGLBINDRENDERBUFFERPROC _glBindRenderbuffer;
    PFNGLDELETERENDERBUFFERSPROC _glDeleteRenderbuffers;
    PFNGLGENRENDERBUFFERSPROC _glGenRenderbuffers;
    PFNGLRENDERBUFFERSTORAGEPROC _glRenderbufferStorage;
    PFNGLGETRENDERBUFFERPARAMETERIVPROC _glGetRenderbufferParameteriv;
    PFNGLISFRAMEBUFFERPROC _glIsFramebuffer;
    PFNGLBINDFRAMEBUFFERPROC _glBindFramebuffer;
    PFNGLDELETEFRAMEBUFFERSPROC _glDeleteFramebuffers;
    PFNGLGENFRAMEBUFFERSPROC _glGenFramebuffers;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC _glCheckFramebufferStatus;
    PFNGLFRAMEBUFFERTEXTURE1DPROC _glFramebufferTexture1D;
    PFNGLFRAMEBUFFERTEXTURE2DPROC _glFramebufferTexture2D;
    PFNGLFRAMEBUFFERTEXTURE3DPROC _glFramebufferTexture3D;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC _glFramebufferRenderbuffer;
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC _glGetFramebufferAttachmentParameteriv;
    PFNGLGENERATEMIPMAPPROC _glGenerateMipmap;
    PFNGLBLITFRAMEBUFFERPROC _glBlitFramebuffer;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC _glRenderbufferStorageMultisample;
    PFNGLFRAMEBUFFERTEXTURELAYERPROC _glFramebufferTextureLayer;
    PFNGLISRENDERBUFFEREXTPROC _glIsRenderbufferEXT;
    PFNGLBINDRENDERBUFFEREXTPROC _glBindRenderbufferEXT;
    PFNGLDELETERENDERBUFFERSEXTPROC _glDeleteRenderbuffersEXT;
    PFNGLGENRENDERBUFFERSEXTPROC _glGenRenderbuffersEXT;
    PFNGLRENDERBUFFERSTORAGEEXTPROC _glRenderbufferStorageEXT;
    PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC _glGetRenderbufferParameterivEXT;
    PFNGLISFRAMEBUFFEREXTPROC _glIsFramebufferEXT;
    PFNGLBINDFRAMEBUFFEREXTPROC _glBindFramebufferEXT;
    PFNGLDELETEFRAMEBUFFERSEXTPROC _glDeleteFramebuffersEXT;
    PFNGLGENFRAMEBUFFERSEXTPROC _glGenFramebuffersEXT;
    PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC _glCheckFramebufferStatusEXT;
    PFNGLFRAMEBUFFERTEXTURE1DEXTPROC _glFramebufferTexture1DEXT;
    PFNGLFRAMEBUFFERTEXTURE2DEXTPROC _glFramebufferTexture2DEXT;
    PFNGLFRAMEBUFFERTEXTURE3DEXTPROC _glFramebufferTexture3DEXT;
    PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC _glFramebufferRenderbufferEXT;
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC _glGetFramebufferAttachmentParameterivEXT;
    PFNGLGENERATEMIPMAPEXTPROC _glGenerateMipmapEXT;
    PFNGLBINDVERTEXARRAYAPPLEPROC _glBindVertexArrayAPPLE;
    PFNGLDELETEVERTEXARRAYSAPPLEPROC _glDeleteVertexArraysAPPLE;
    PFNGLGENVERTEXARRAYSAPPLEPROC _glGenVertexArraysAPPLE;
    PFNGLISVERTEXARRAYAPPLEPROC _glIsVertexArrayAPPLE;

public:

    // static non MT-safe load function that must be called once to initialize functions
    static void load()
    {
        (void)getInstance();
    }

    static bool isGPU()
    {
        return USEOPENGL;
    }

    static void CullFace(GLenum mode)
    {
        getInstance()._glCullFace(mode);
    }

    static void FrontFace(GLenum mode)
    {
        getInstance()._glFrontFace(mode);
    }

    static void Hint(GLenum target,
                       GLenum mode)
    {
        getInstance()._glHint(target, mode);
    }

    static void LineWidth(GLfloat width)
    {
        getInstance()._glLineWidth(width);
    }

    static void PointSize(GLfloat size)
    {
        getInstance()._glPointSize(size);
    }

    static void PolygonMode(GLenum face,
                              GLenum mode)
    {
        getInstance()._glPolygonMode(face, mode);
    }

    static void Scissor(GLint x,
                          GLint y,
                          GLsizei width,
                          GLsizei height)
    {
        getInstance()._glScissor(x, y, width, height);
    }

    static void TexParameterf(GLenum target,
                                GLenum pname,
                                GLfloat param)
    {
        getInstance()._glTexParameterf(target, pname, param);
    }

    static void TexParameterfv(GLenum target,
                                 GLenum pname,
                                 const GLfloat* params)
    {
        getInstance()._glTexParameterfv(target, pname, params);
    }

    static void TexParameteri(GLenum target,
                                GLenum pname,
                                GLint param)
    {
        getInstance()._glTexParameteri(target, pname, param);
    }

    static void TexParameteriv(GLenum target,
                                 GLenum pname,
                                 const GLint* params)
    {
        getInstance()._glTexParameteriv(target, pname, params);
    }

    static void TexImage1D(GLenum target,
                             GLint level,
                             GLint internalformat,
                             GLsizei width,
                             GLint border,
                             GLenum format,
                             GLenum type,
                             const void* pixels)
    {
        getInstance()._glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
    }

    static void TexImage2D(GLenum target,
                             GLint level,
                             GLint internalformat,
                             GLsizei width,
                             GLsizei height,
                             GLint border,
                             GLenum format,
                             GLenum type,
                             const void* pixels)
    {
        getInstance()._glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    }

    static void DrawBuffer(GLenum buf)
    {
        getInstance()._glDrawBuffer(buf);
    }

    static void Clear(GLbitfield mask)
    {
        getInstance()._glClear(mask);
    }

    static void ClearColor(GLfloat red,
                             GLfloat green,
                             GLfloat blue,
                             GLfloat alpha)
    {
        getInstance()._glClearColor(red, green, blue, alpha);
    }

    static void ClearStencil(GLint s)
    {
        getInstance()._glClearStencil(s);
    }

    static void ClearDepth(GLdouble depth)
    {
        getInstance()._glClearDepth(depth);
    }

    static void StencilMask(GLuint mask)
    {
        getInstance()._glStencilMask(mask);
    }

    static void ColorMask(GLboolean red,
                            GLboolean green,
                            GLboolean blue,
                            GLboolean alpha)
    {
        getInstance()._glColorMask(red, green, blue, alpha);
    }

    static void DepthMask(GLboolean flag)
    {
        getInstance()._glDepthMask(flag);
    }

    static void Disable(GLenum cap)
    {
        getInstance()._glDisable(cap);
    }

    static void Enable(GLenum cap)
    {
        getInstance()._glEnable(cap);
    }

    static void Finish()
    {
        getInstance()._glFinish();
    }

    static void Flush()
    {
        getInstance()._glFlush();
    }

    static void BlendFunc(GLenum sfactor,
                            GLenum dfactor)
    {
        getInstance()._glBlendFunc(sfactor, dfactor);
    }

    static void LogicOp(GLenum opcode)
    {
        getInstance()._glLogicOp(opcode);
    }

    static void StencilFunc(GLenum func,
                              GLint ref,
                              GLuint mask)
    {
        getInstance()._glStencilFunc(func, ref, mask);
    }

    static void StencilOp(GLenum fail,
                            GLenum zfail,
                            GLenum zpass)
    {
        getInstance()._glStencilOp(fail, zfail, zpass);
    }

    static void DepthFunc(GLenum func)
    {
        getInstance()._glDepthFunc(func);
    }

    static void PixelStoref(GLenum pname,
                              GLfloat param)
    {
        getInstance()._glPixelStoref(pname, param);
    }

    static void PixelStorei(GLenum pname,
                              GLint param)
    {
        getInstance()._glPixelStorei(pname, param);
    }

    static void ReadBuffer(GLenum src)
    {
        getInstance()._glReadBuffer(src);
    }

    static void ReadPixels(GLint x,
                             GLint y,
                             GLsizei width,
                             GLsizei height,
                             GLenum format,
                             GLenum type,
                             void* pixels)
    {
        getInstance()._glReadPixels(x, y, width, height, format, type, pixels);
    }

    static void GetBooleanv(GLenum pname,
                              GLboolean* data)
    {
        getInstance()._glGetBooleanv(pname, data);
    }

    static void GetDoublev(GLenum pname,
                             GLdouble* data)
    {
        getInstance()._glGetDoublev(pname, data);
    }

    static GLenum GetError()
    {
        return getInstance()._glGetError();
    }

    static void GetFloatv(GLenum pname,
                            GLfloat* data)
    {
        getInstance()._glGetFloatv(pname, data);
    }

    static void GetIntegerv(GLenum pname,
                              GLint* data)
    {
        getInstance()._glGetIntegerv(pname, data);
    }

    static const GLubyte* GetString(GLenum name)
    {
        return getInstance()._glGetString(name);
    }

    static void GetTexImage(GLenum target,
                              GLint level,
                              GLenum format,
                              GLenum type,
                              void* pixels)
    {
        getInstance()._glGetTexImage(target, level, format, type, pixels);
    }

    static void GetTexParameterfv(GLenum target,
                                    GLenum pname,
                                    GLfloat* params)
    {
        getInstance()._glGetTexParameterfv(target, pname, params);
    }

    static void GetTexParameteriv(GLenum target,
                                    GLenum pname,
                                    GLint* params)
    {
        getInstance()._glGetTexParameteriv(target, pname, params);
    }

    static void GetTexLevelParameterfv(GLenum target,
                                         GLint level,
                                         GLenum pname,
                                         GLfloat* params)
    {
        getInstance()._glGetTexLevelParameterfv(target, level, pname, params);
    }

    static void GetTexLevelParameteriv(GLenum target,
                                         GLint level,
                                         GLenum pname,
                                         GLint* params)
    {
        getInstance()._glGetTexLevelParameteriv(target, level, pname, params);
    }

    static GLboolean IsEnabled(GLenum cap)
    {
        return getInstance()._glIsEnabled(cap);
    }

    static void DepthRange(GLdouble nearVal,
                             GLdouble farVal)
    {
        getInstance()._glDepthRange(nearVal, farVal);
    }

    static void Viewport(GLint x,
                           GLint y,
                           GLsizei width,
                           GLsizei height)
    {
        getInstance()._glViewport(x, y, width, height);
    }

    static void NewList(GLuint list,
                          GLenum mode)
    {
        getInstance()._glNewList(list, mode);
    }

    static void EndList()
    {
        getInstance()._glEndList();
    }

    static void CallList(GLuint list)
    {
        getInstance()._glCallList(list);
    }

    static void CallLists(GLsizei n,
                            GLenum type,
                            const void* lists)
    {
        getInstance()._glCallLists(n, type, lists);
    }

    static void DeleteLists(GLuint list,
                              GLsizei range)
    {
        getInstance()._glDeleteLists(list, range);
    }

    static GLuint GenLists(GLsizei range)
    {
        return getInstance()._glGenLists(range);
    }

    static void ListBase(GLuint base)
    {
        getInstance()._glListBase(base);
    }

    static void Begin(GLenum mode)
    {
        getInstance()._glBegin(mode);
    }

    static void Bitmap(GLsizei width,
                         GLsizei height,
                         GLfloat xorig,
                         GLfloat yorig,
                         GLfloat xmove,
                         GLfloat ymove,
                         const GLubyte* bitmap)
    {
        getInstance()._glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
    }

    static void Color3b(GLbyte red,
                          GLbyte green,
                          GLbyte blue)
    {
        getInstance()._glColor3b(red, green, blue);
    }

    static void Color3bv(const GLbyte* v)
    {
        getInstance()._glColor3bv(v);
    }

    static void Color3d(GLdouble red,
                          GLdouble green,
                          GLdouble blue)
    {
        getInstance()._glColor3d(red, green, blue);
    }

    static void Color3dv(const GLdouble* v)
    {
        getInstance()._glColor3dv(v);
    }

    static void Color3f(GLfloat red,
                          GLfloat green,
                          GLfloat blue)
    {
        getInstance()._glColor3f(red, green, blue);
    }

    static void Color3fv(const GLfloat* v)
    {
        getInstance()._glColor3fv(v);
    }

    static void Color3i(GLint red,
                          GLint green,
                          GLint blue)
    {
        getInstance()._glColor3i(red, green, blue);
    }

    static void Color3iv(const GLint* v)
    {
        getInstance()._glColor3iv(v);
    }

    static void Color3s(GLshort red,
                          GLshort green,
                          GLshort blue)
    {
        getInstance()._glColor3s(red, green, blue);
    }

    static void Color3sv(const GLshort* v)
    {
        getInstance()._glColor3sv(v);
    }

    static void Color3ub(GLubyte red,
                           GLubyte green,
                           GLubyte blue)
    {
        getInstance()._glColor3ub(red, green, blue);
    }

    static void Color3ubv(const GLubyte* v)
    {
        getInstance()._glColor3ubv(v);
    }

    static void Color3ui(GLuint red,
                           GLuint green,
                           GLuint blue)
    {
        getInstance()._glColor3ui(red, green, blue);
    }

    static void Color3uiv(const GLuint* v)
    {
        getInstance()._glColor3uiv(v);
    }

    static void Color3us(GLushort red,
                           GLushort green,
                           GLushort blue)
    {
        getInstance()._glColor3us(red, green, blue);
    }

    static void Color3usv(const GLushort* v)
    {
        getInstance()._glColor3usv(v);
    }

    static void Color4b(GLbyte red,
                          GLbyte green,
                          GLbyte blue,
                          GLbyte alpha)
    {
        getInstance()._glColor4b(red, green, blue, alpha);
    }

    static void Color4bv(const GLbyte* v)
    {
        getInstance()._glColor4bv(v);
    }

    static void Color4d(GLdouble red,
                          GLdouble green,
                          GLdouble blue,
                          GLdouble alpha)
    {
        getInstance()._glColor4d(red, green, blue, alpha);
    }

    static void Color4dv(const GLdouble* v)
    {
        getInstance()._glColor4dv(v);
    }

    static void Color4f(GLfloat red,
                          GLfloat green,
                          GLfloat blue,
                          GLfloat alpha)
    {
        getInstance()._glColor4f(red, green, blue, alpha);
    }

    static void Color4fv(const GLfloat* v)
    {
        getInstance()._glColor4fv(v);
    }

    static void Color4i(GLint red,
                          GLint green,
                          GLint blue,
                          GLint alpha)
    {
        getInstance()._glColor4i(red, green, blue, alpha);
    }

    static void Color4iv(const GLint* v)
    {
        getInstance()._glColor4iv(v);
    }

    static void Color4s(GLshort red,
                          GLshort green,
                          GLshort blue,
                          GLshort alpha)
    {
        getInstance()._glColor4s(red, green, blue, alpha);
    }

    static void Color4sv(const GLshort* v)
    {
        getInstance()._glColor4sv(v);
    }

    static void Color4ub(GLubyte red,
                           GLubyte green,
                           GLubyte blue,
                           GLubyte alpha)
    {
        getInstance()._glColor4ub(red, green, blue, alpha);
    }

    static void Color4ubv(const GLubyte* v)
    {
        getInstance()._glColor4ubv(v);
    }

    static void Color4ui(GLuint red,
                           GLuint green,
                           GLuint blue,
                           GLuint alpha)
    {
        getInstance()._glColor4ui(red, green, blue, alpha);
    }

    static void Color4uiv(const GLuint* v)
    {
        getInstance()._glColor4uiv(v);
    }

    static void Color4us(GLushort red,
                           GLushort green,
                           GLushort blue,
                           GLushort alpha)
    {
        getInstance()._glColor4us(red, green, blue, alpha);
    }

    static void Color4usv(const GLushort* v)
    {
        getInstance()._glColor4usv(v);
    }

    static void EdgeFlag(GLboolean flag)
    {
        getInstance()._glEdgeFlag(flag);
    }

    static void EdgeFlagv(const GLboolean* flag)
    {
        getInstance()._glEdgeFlagv(flag);
    }

    static void End()
    {
        getInstance()._glEnd();
    }

    static void Indexd(GLdouble c)
    {
        getInstance()._glIndexd(c);
    }

    static void Indexdv(const GLdouble* c)
    {
        getInstance()._glIndexdv(c);
    }

    static void Indexf(GLfloat c)
    {
        getInstance()._glIndexf(c);
    }

    static void Indexfv(const GLfloat* c)
    {
        getInstance()._glIndexfv(c);
    }

    static void Indexi(GLint c)
    {
        getInstance()._glIndexi(c);
    }

    static void Indexiv(const GLint* c)
    {
        getInstance()._glIndexiv(c);
    }

    static void Indexs(GLshort c)
    {
        getInstance()._glIndexs(c);
    }

    static void Indexsv(const GLshort* c)
    {
        getInstance()._glIndexsv(c);
    }

    static void Normal3b(GLbyte nx,
                           GLbyte ny,
                           GLbyte nz)
    {
        getInstance()._glNormal3b(nx, ny, nz);
    }

    static void Normal3bv(const GLbyte* v)
    {
        getInstance()._glNormal3bv(v);
    }

    static void Normal3d(GLdouble nx,
                           GLdouble ny,
                           GLdouble nz)
    {
        getInstance()._glNormal3d(nx, ny, nz);
    }

    static void Normal3dv(const GLdouble* v)
    {
        getInstance()._glNormal3dv(v);
    }

    static void Normal3f(GLfloat nx,
                           GLfloat ny,
                           GLfloat nz)
    {
        getInstance()._glNormal3f(nx, ny, nz);
    }

    static void Normal3fv(const GLfloat* v)
    {
        getInstance()._glNormal3fv(v);
    }

    static void Normal3i(GLint nx,
                           GLint ny,
                           GLint nz)
    {
        getInstance()._glNormal3i(nx, ny, nz);
    }

    static void Normal3iv(const GLint* v)
    {
        getInstance()._glNormal3iv(v);
    }

    static void Normal3s(GLshort nx,
                           GLshort ny,
                           GLshort nz)
    {
        getInstance()._glNormal3s(nx, ny, nz);
    }

    static void Normal3sv(const GLshort* v)
    {
        getInstance()._glNormal3sv(v);
    }

    static void RasterPos2d(GLdouble x,
                              GLdouble y)
    {
        getInstance()._glRasterPos2d(x, y);
    }

    static void RasterPos2dv(const GLdouble* v)
    {
        getInstance()._glRasterPos2dv(v);
    }

    static void RasterPos2f(GLfloat x,
                              GLfloat y)
    {
        getInstance()._glRasterPos2f(x, y);
    }

    static void RasterPos2fv(const GLfloat* v)
    {
        getInstance()._glRasterPos2fv(v);
    }

    static void RasterPos2i(GLint x,
                              GLint y)
    {
        getInstance()._glRasterPos2i(x, y);
    }

    static void RasterPos2iv(const GLint* v)
    {
        getInstance()._glRasterPos2iv(v);
    }

    static void RasterPos2s(GLshort x,
                              GLshort y)
    {
        getInstance()._glRasterPos2s(x, y);
    }

    static void RasterPos2sv(const GLshort* v)
    {
        getInstance()._glRasterPos2sv(v);
    }

    static void RasterPos3d(GLdouble x,
                              GLdouble y,
                              GLdouble z)
    {
        getInstance()._glRasterPos3d(x, y, z);
    }

    static void RasterPos3dv(const GLdouble* v)
    {
        getInstance()._glRasterPos3dv(v);
    }

    static void RasterPos3f(GLfloat x,
                              GLfloat y,
                              GLfloat z)
    {
        getInstance()._glRasterPos3f(x, y, z);
    }

    static void RasterPos3fv(const GLfloat* v)
    {
        getInstance()._glRasterPos3fv(v);
    }

    static void RasterPos3i(GLint x,
                              GLint y,
                              GLint z)
    {
        getInstance()._glRasterPos3i(x, y, z);
    }

    static void RasterPos3iv(const GLint* v)
    {
        getInstance()._glRasterPos3iv(v);
    }

    static void RasterPos3s(GLshort x,
                              GLshort y,
                              GLshort z)
    {
        getInstance()._glRasterPos3s(x, y, z);
    }

    static void RasterPos3sv(const GLshort* v)
    {
        getInstance()._glRasterPos3sv(v);
    }

    static void RasterPos4d(GLdouble x,
                              GLdouble y,
                              GLdouble z,
                              GLdouble w)
    {
        getInstance()._glRasterPos4d(x, y, z, w);
    }

    static void RasterPos4dv(const GLdouble* v)
    {
        getInstance()._glRasterPos4dv(v);
    }

    static void RasterPos4f(GLfloat x,
                              GLfloat y,
                              GLfloat z,
                              GLfloat w)
    {
        getInstance()._glRasterPos4f(x, y, z, w);
    }

    static void RasterPos4fv(const GLfloat* v)
    {
        getInstance()._glRasterPos4fv(v);
    }

    static void RasterPos4i(GLint x,
                              GLint y,
                              GLint z,
                              GLint w)
    {
        getInstance()._glRasterPos4i(x, y, z, w);
    }

    static void RasterPos4iv(const GLint* v)
    {
        getInstance()._glRasterPos4iv(v);
    }

    static void RasterPos4s(GLshort x,
                              GLshort y,
                              GLshort z,
                              GLshort w)
    {
        getInstance()._glRasterPos4s(x, y, z, w);
    }

    static void RasterPos4sv(const GLshort* v)
    {
        getInstance()._glRasterPos4sv(v);
    }

    static void Rectd(GLdouble x1,
                        GLdouble y1,
                        GLdouble x2,
                        GLdouble y2)
    {
        getInstance()._glRectd(x1, y1, x2, y2);
    }

    static void Rectdv(const GLdouble* v1,
                         const GLdouble* v2)
    {
        getInstance()._glRectdv(v1, v2);
    }

    static void Rectf(GLfloat x1,
                        GLfloat y1,
                        GLfloat x2,
                        GLfloat y2)
    {
        getInstance()._glRectf(x1, y1, x2, y2);
    }

    static void Rectfv(const GLfloat* v1,
                         const GLfloat* v2)
    {
        getInstance()._glRectfv(v1, v2);
    }

    static void Recti(GLint x1,
                        GLint y1,
                        GLint x2,
                        GLint y2)
    {
        getInstance()._glRecti(x1, y1, x2, y2);
    }

    static void Rectiv(const GLint* v1,
                         const GLint* v2)
    {
        getInstance()._glRectiv(v1, v2);
    }

    static void Rects(GLshort x1,
                        GLshort y1,
                        GLshort x2,
                        GLshort y2)
    {
        getInstance()._glRects(x1, y1, x2, y2);
    }

    static void Rectsv(const GLshort* v1,
                         const GLshort* v2)
    {
        getInstance()._glRectsv(v1, v2);
    }

    static void TexCoord1d(GLdouble s)
    {
        getInstance()._glTexCoord1d(s);
    }

    static void TexCoord1dv(const GLdouble* v)
    {
        getInstance()._glTexCoord1dv(v);
    }

    static void TexCoord1f(GLfloat s)
    {
        getInstance()._glTexCoord1f(s);
    }

    static void TexCoord1fv(const GLfloat* v)
    {
        getInstance()._glTexCoord1fv(v);
    }

    static void TexCoord1i(GLint s)
    {
        getInstance()._glTexCoord1i(s);
    }

    static void TexCoord1iv(const GLint* v)
    {
        getInstance()._glTexCoord1iv(v);
    }

    static void TexCoord1s(GLshort s)
    {
        getInstance()._glTexCoord1s(s);
    }

    static void TexCoord1sv(const GLshort* v)
    {
        getInstance()._glTexCoord1sv(v);
    }

    static void TexCoord2d(GLdouble s,
                             GLdouble t)
    {
        getInstance()._glTexCoord2d(s, t);
    }

    static void TexCoord2dv(const GLdouble* v)
    {
        getInstance()._glTexCoord2dv(v);
    }

    static void TexCoord2f(GLfloat s,
                             GLfloat t)
    {
        getInstance()._glTexCoord2f(s, t);
    }

    static void TexCoord2fv(const GLfloat* v)
    {
        getInstance()._glTexCoord2fv(v);
    }

    static void TexCoord2i(GLint s,
                             GLint t)
    {
        getInstance()._glTexCoord2i(s, t);
    }

    static void TexCoord2iv(const GLint* v)
    {
        getInstance()._glTexCoord2iv(v);
    }

    static void TexCoord2s(GLshort s,
                             GLshort t)
    {
        getInstance()._glTexCoord2s(s, t);
    }

    static void TexCoord2sv(const GLshort* v)
    {
        getInstance()._glTexCoord2sv(v);
    }

    static void TexCoord3d(GLdouble s,
                             GLdouble t,
                             GLdouble r)
    {
        getInstance()._glTexCoord3d(s, t, r);
    }

    static void TexCoord3dv(const GLdouble* v)
    {
        getInstance()._glTexCoord3dv(v);
    }

    static void TexCoord3f(GLfloat s,
                             GLfloat t,
                             GLfloat r)
    {
        getInstance()._glTexCoord3f(s, t, r);
    }

    static void TexCoord3fv(const GLfloat* v)
    {
        getInstance()._glTexCoord3fv(v);
    }

    static void TexCoord3i(GLint s,
                             GLint t,
                             GLint r)
    {
        getInstance()._glTexCoord3i(s, t, r);
    }

    static void TexCoord3iv(const GLint* v)
    {
        getInstance()._glTexCoord3iv(v);
    }

    static void TexCoord3s(GLshort s,
                             GLshort t,
                             GLshort r)
    {
        getInstance()._glTexCoord3s(s, t, r);
    }

    static void TexCoord3sv(const GLshort* v)
    {
        getInstance()._glTexCoord3sv(v);
    }

    static void TexCoord4d(GLdouble s,
                             GLdouble t,
                             GLdouble r,
                             GLdouble q)
    {
        getInstance()._glTexCoord4d(s, t, r, q);
    }

    static void TexCoord4dv(const GLdouble* v)
    {
        getInstance()._glTexCoord4dv(v);
    }

    static void TexCoord4f(GLfloat s,
                             GLfloat t,
                             GLfloat r,
                             GLfloat q)
    {
        getInstance()._glTexCoord4f(s, t, r, q);
    }

    static void TexCoord4fv(const GLfloat* v)
    {
        getInstance()._glTexCoord4fv(v);
    }

    static void TexCoord4i(GLint s,
                             GLint t,
                             GLint r,
                             GLint q)
    {
        getInstance()._glTexCoord4i(s, t, r, q);
    }

    static void TexCoord4iv(const GLint* v)
    {
        getInstance()._glTexCoord4iv(v);
    }

    static void TexCoord4s(GLshort s,
                             GLshort t,
                             GLshort r,
                             GLshort q)
    {
        getInstance()._glTexCoord4s(s, t, r, q);
    }

    static void TexCoord4sv(const GLshort* v)
    {
        getInstance()._glTexCoord4sv(v);
    }

    static void Vertex2d(GLdouble x,
                           GLdouble y)
    {
        getInstance()._glVertex2d(x, y);
    }

    static void Vertex2dv(const GLdouble* v)
    {
        getInstance()._glVertex2dv(v);
    }

    static void Vertex2f(GLfloat x,
                           GLfloat y)
    {
        getInstance()._glVertex2f(x, y);
    }

    static void Vertex2fv(const GLfloat* v)
    {
        getInstance()._glVertex2fv(v);
    }

    static void Vertex2i(GLint x,
                           GLint y)
    {
        getInstance()._glVertex2i(x, y);
    }

    static void Vertex2iv(const GLint* v)
    {
        getInstance()._glVertex2iv(v);
    }

    static void Vertex2s(GLshort x,
                           GLshort y)
    {
        getInstance()._glVertex2s(x, y);
    }

    static void Vertex2sv(const GLshort* v)
    {
        getInstance()._glVertex2sv(v);
    }

    static void Vertex3d(GLdouble x,
                           GLdouble y,
                           GLdouble z)
    {
        getInstance()._glVertex3d(x, y, z);
    }

    static void Vertex3dv(const GLdouble* v)
    {
        getInstance()._glVertex3dv(v);
    }

    static void Vertex3f(GLfloat x,
                           GLfloat y,
                           GLfloat z)
    {
        getInstance()._glVertex3f(x, y, z);
    }

    static void Vertex3fv(const GLfloat* v)
    {
        getInstance()._glVertex3fv(v);
    }

    static void Vertex3i(GLint x,
                           GLint y,
                           GLint z)
    {
        getInstance()._glVertex3i(x, y, z);
    }

    static void Vertex3iv(const GLint* v)
    {
        getInstance()._glVertex3iv(v);
    }

    static void Vertex3s(GLshort x,
                           GLshort y,
                           GLshort z)
    {
        getInstance()._glVertex3s(x, y, z);
    }

    static void Vertex3sv(const GLshort* v)
    {
        getInstance()._glVertex3sv(v);
    }

    static void Vertex4d(GLdouble x,
                           GLdouble y,
                           GLdouble z,
                           GLdouble w)
    {
        getInstance()._glVertex4d(x, y, z, w);
    }

    static void Vertex4dv(const GLdouble* v)
    {
        getInstance()._glVertex4dv(v);
    }

    static void Vertex4f(GLfloat x,
                           GLfloat y,
                           GLfloat z,
                           GLfloat w)
    {
        getInstance()._glVertex4f(x, y, z, w);
    }

    static void Vertex4fv(const GLfloat* v)
    {
        getInstance()._glVertex4fv(v);
    }

    static void Vertex4i(GLint x,
                           GLint y,
                           GLint z,
                           GLint w)
    {
        getInstance()._glVertex4i(x, y, z, w);
    }

    static void Vertex4iv(const GLint* v)
    {
        getInstance()._glVertex4iv(v);
    }

    static void Vertex4s(GLshort x,
                           GLshort y,
                           GLshort z,
                           GLshort w)
    {
        getInstance()._glVertex4s(x, y, z, w);
    }

    static void Vertex4sv(const GLshort* v)
    {
        getInstance()._glVertex4sv(v);
    }

    static void ClipPlane(GLenum plane,
                            const GLdouble* equation)
    {
        getInstance()._glClipPlane(plane, equation);
    }

    static void ColorMaterial(GLenum face,
                                GLenum mode)
    {
        getInstance()._glColorMaterial(face, mode);
    }

    static void Fogf(GLenum pname,
                       GLfloat param)
    {
        getInstance()._glFogf(pname, param);
    }

    static void Fogfv(GLenum pname,
                        const GLfloat* params)
    {
        getInstance()._glFogfv(pname, params);
    }

    static void Fogi(GLenum pname,
                       GLint param)
    {
        getInstance()._glFogi(pname, param);
    }

    static void Fogiv(GLenum pname,
                        const GLint* params)
    {
        getInstance()._glFogiv(pname, params);
    }

    static void Lightf(GLenum light,
                         GLenum pname,
                         GLfloat param)
    {
        getInstance()._glLightf(light, pname, param);
    }

    static void Lightfv(GLenum light,
                          GLenum pname,
                          const GLfloat* params)
    {
        getInstance()._glLightfv(light, pname, params);
    }

    static void Lighti(GLenum light,
                         GLenum pname,
                         GLint param)
    {
        getInstance()._glLighti(light, pname, param);
    }

    static void Lightiv(GLenum light,
                          GLenum pname,
                          const GLint* params)
    {
        getInstance()._glLightiv(light, pname, params);
    }

    static void LightModelf(GLenum pname,
                              GLfloat param)
    {
        getInstance()._glLightModelf(pname, param);
    }

    static void LightModelfv(GLenum pname,
                               const GLfloat* params)
    {
        getInstance()._glLightModelfv(pname, params);
    }

    static void LightModeli(GLenum pname,
                              GLint param)
    {
        getInstance()._glLightModeli(pname, param);
    }

    static void LightModeliv(GLenum pname,
                               const GLint* params)
    {
        getInstance()._glLightModeliv(pname, params);
    }

    static void LineStipple(GLint factor,
                              GLushort pattern)
    {
        getInstance()._glLineStipple(factor, pattern);
    }

    static void Materialf(GLenum face,
                            GLenum pname,
                            GLfloat param)
    {
        getInstance()._glMaterialf(face, pname, param);
    }

    static void Materialfv(GLenum face,
                             GLenum pname,
                             const GLfloat* params)
    {
        getInstance()._glMaterialfv(face, pname, params);
    }

    static void Materiali(GLenum face,
                            GLenum pname,
                            GLint param)
    {
        getInstance()._glMateriali(face, pname, param);
    }

    static void Materialiv(GLenum face,
                             GLenum pname,
                             const GLint* params)
    {
        getInstance()._glMaterialiv(face, pname, params);
    }

    static void PolygonStipple(const GLubyte* mask)
    {
        getInstance()._glPolygonStipple(mask);
    }

    static void ShadeModel(GLenum mode)
    {
        getInstance()._glShadeModel(mode);
    }

    static void TexEnvf(GLenum target,
                          GLenum pname,
                          GLfloat param)
    {
        getInstance()._glTexEnvf(target, pname, param);
    }

    static void TexEnvfv(GLenum target,
                           GLenum pname,
                           const GLfloat* params)
    {
        getInstance()._glTexEnvfv(target, pname, params);
    }

    static void TexEnvi(GLenum target,
                          GLenum pname,
                          GLint param)
    {
        getInstance()._glTexEnvi(target, pname, param);
    }

    static void TexEnviv(GLenum target,
                           GLenum pname,
                           const GLint* params)
    {
        getInstance()._glTexEnviv(target, pname, params);
    }

    static void TexGend(GLenum coord,
                          GLenum pname,
                          GLdouble param)
    {
        getInstance()._glTexGend(coord, pname, param);
    }

    static void TexGendv(GLenum coord,
                           GLenum pname,
                           const GLdouble* params)
    {
        getInstance()._glTexGendv(coord, pname, params);
    }

    static void TexGenf(GLenum coord,
                          GLenum pname,
                          GLfloat param)
    {
        getInstance()._glTexGenf(coord, pname, param);
    }

    static void TexGenfv(GLenum coord,
                           GLenum pname,
                           const GLfloat* params)
    {
        getInstance()._glTexGenfv(coord, pname, params);
    }

    static void TexGeni(GLenum coord,
                          GLenum pname,
                          GLint param)
    {
        getInstance()._glTexGeni(coord, pname, param);
    }

    static void TexGeniv(GLenum coord,
                           GLenum pname,
                           const GLint* params)
    {
        getInstance()._glTexGeniv(coord, pname, params);
    }

    static void FeedbackBuffer(GLsizei size,
                                 GLenum type,
                                 GLfloat* buffer)
    {
        getInstance()._glFeedbackBuffer(size, type, buffer);
    }

    static void SelectBuffer(GLsizei size,
                               GLuint* buffer)
    {
        getInstance()._glSelectBuffer(size, buffer);
    }

    static GLint RenderMode(GLenum mode)
    {
        return getInstance()._glRenderMode(mode);
    }

    static void InitNames()
    {
        getInstance()._glInitNames();
    }

    static void LoadName(GLuint name)
    {
        getInstance()._glLoadName(name);
    }

    static void PassThrough(GLfloat token)
    {
        getInstance()._glPassThrough(token);
    }

    static void PopName()
    {
        getInstance()._glPopName();
    }

    static void PushName(GLuint name)
    {
        getInstance()._glPushName(name);
    }

    static void ClearAccum(GLfloat red,
                             GLfloat green,
                             GLfloat blue,
                             GLfloat alpha)
    {
        getInstance()._glClearAccum(red, green, blue, alpha);
    }

    static void ClearIndex(GLfloat c)
    {
        getInstance()._glClearIndex(c);
    }

    static void IndexMask(GLuint mask)
    {
        getInstance()._glIndexMask(mask);
    }

    static void Accum(GLenum op,
                        GLfloat value)
    {
        getInstance()._glAccum(op, value);
    }

    static void PopAttrib()
    {
        getInstance()._glPopAttrib();
    }

    static void PushAttrib(GLbitfield mask)
    {
        getInstance()._glPushAttrib(mask);
    }

    static void Map1d(GLenum target,
                        GLdouble u1,
                        GLdouble u2,
                        GLint stride,
                        GLint order,
                        const GLdouble* points)
    {
        getInstance()._glMap1d(target, u1, u2, stride, order, points);
    }

    static void Map1f(GLenum target,
                        GLfloat u1,
                        GLfloat u2,
                        GLint stride,
                        GLint order,
                        const GLfloat* points)
    {
        getInstance()._glMap1f(target, u1, u2, stride, order, points);
    }

    static void Map2d(GLenum target,
                        GLdouble u1,
                        GLdouble u2,
                        GLint ustride,
                        GLint uorder,
                        GLdouble v1,
                        GLdouble v2,
                        GLint vstride,
                        GLint vorder,
                        const GLdouble* points)
    {
        getInstance()._glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
    }

    static void Map2f(GLenum target,
                        GLfloat u1,
                        GLfloat u2,
                        GLint ustride,
                        GLint uorder,
                        GLfloat v1,
                        GLfloat v2,
                        GLint vstride,
                        GLint vorder,
                        const GLfloat* points)
    {
        getInstance()._glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
    }

    static void MapGrid1d(GLint un,
                            GLdouble u1,
                            GLdouble u2)
    {
        getInstance()._glMapGrid1d(un, u1, u2);
    }

    static void MapGrid1f(GLint un,
                            GLfloat u1,
                            GLfloat u2)
    {
        getInstance()._glMapGrid1f(un, u1, u2);
    }

    static void MapGrid2d(GLint un,
                            GLdouble u1,
                            GLdouble u2,
                            GLint vn,
                            GLdouble v1,
                            GLdouble v2)
    {
        getInstance()._glMapGrid2d(un, u1, u2, vn, v1, v2);
    }

    static void MapGrid2f(GLint un,
                            GLfloat u1,
                            GLfloat u2,
                            GLint vn,
                            GLfloat v1,
                            GLfloat v2)
    {
        getInstance()._glMapGrid2f(un, u1, u2, vn, v1, v2);
    }

    static void EvalCoord1d(GLdouble u)
    {
        getInstance()._glEvalCoord1d(u);
    }

    static void EvalCoord1dv(const GLdouble* u)
    {
        getInstance()._glEvalCoord1dv(u);
    }

    static void EvalCoord1f(GLfloat u)
    {
        getInstance()._glEvalCoord1f(u);
    }

    static void EvalCoord1fv(const GLfloat* u)
    {
        getInstance()._glEvalCoord1fv(u);
    }

    static void EvalCoord2d(GLdouble u,
                              GLdouble v)
    {
        getInstance()._glEvalCoord2d(u, v);
    }

    static void EvalCoord2dv(const GLdouble* u)
    {
        getInstance()._glEvalCoord2dv(u);
    }

    static void EvalCoord2f(GLfloat u,
                              GLfloat v)
    {
        getInstance()._glEvalCoord2f(u, v);
    }

    static void EvalCoord2fv(const GLfloat* u)
    {
        getInstance()._glEvalCoord2fv(u);
    }

    static void EvalMesh1(GLenum mode,
                            GLint i1,
                            GLint i2)
    {
        getInstance()._glEvalMesh1(mode, i1, i2);
    }

    static void EvalPoint1(GLint i)
    {
        getInstance()._glEvalPoint1(i);
    }

    static void EvalMesh2(GLenum mode,
                            GLint i1,
                            GLint i2,
                            GLint j1,
                            GLint j2)
    {
        getInstance()._glEvalMesh2(mode, i1, i2, j1, j2);
    }

    static void EvalPoint2(GLint i,
                             GLint j)
    {
        getInstance()._glEvalPoint2(i, j);
    }

    static void AlphaFunc(GLenum func,
                            GLfloat ref)
    {
        getInstance()._glAlphaFunc(func, ref);
    }

    static void PixelZoom(GLfloat xfactor,
                            GLfloat yfactor)
    {
        getInstance()._glPixelZoom(xfactor, yfactor);
    }

    static void PixelTransferf(GLenum pname,
                                 GLfloat param)
    {
        getInstance()._glPixelTransferf(pname, param);
    }

    static void PixelTransferi(GLenum pname,
                                 GLint param)
    {
        getInstance()._glPixelTransferi(pname, param);
    }

    static void PixelMapfv(GLenum map,
                             GLsizei mapsize,
                             const GLfloat* values)
    {
        getInstance()._glPixelMapfv(map, mapsize, values);
    }

    static void PixelMapuiv(GLenum map,
                              GLsizei mapsize,
                              const GLuint* values)
    {
        getInstance()._glPixelMapuiv(map, mapsize, values);
    }

    static void PixelMapusv(GLenum map,
                              GLsizei mapsize,
                              const GLushort* values)
    {
        getInstance()._glPixelMapusv(map, mapsize, values);
    }

    static void CopyPixels(GLint x,
                             GLint y,
                             GLsizei width,
                             GLsizei height,
                             GLenum type)
    {
        getInstance()._glCopyPixels(x, y, width, height, type);
    }

    static void DrawPixels(GLsizei width,
                             GLsizei height,
                             GLenum format,
                             GLenum type,
                             const void* pixels)
    {
        getInstance()._glDrawPixels(width, height, format, type, pixels);
    }

    static void GetClipPlane(GLenum plane,
                               GLdouble* equation)
    {
        getInstance()._glGetClipPlane(plane, equation);
    }

    static void GetLightfv(GLenum light,
                             GLenum pname,
                             GLfloat* params)
    {
        getInstance()._glGetLightfv(light, pname, params);
    }

    static void GetLightiv(GLenum light,
                             GLenum pname,
                             GLint* params)
    {
        getInstance()._glGetLightiv(light, pname, params);
    }

    static void GetMapdv(GLenum target,
                           GLenum query,
                           GLdouble* v)
    {
        getInstance()._glGetMapdv(target, query, v);
    }

    static void GetMapfv(GLenum target,
                           GLenum query,
                           GLfloat* v)
    {
        getInstance()._glGetMapfv(target, query, v);
    }

    static void GetMapiv(GLenum target,
                           GLenum query,
                           GLint* v)
    {
        getInstance()._glGetMapiv(target, query, v);
    }

    static void GetMaterialfv(GLenum face,
                                GLenum pname,
                                GLfloat* params)
    {
        getInstance()._glGetMaterialfv(face, pname, params);
    }

    static void GetMaterialiv(GLenum face,
                                GLenum pname,
                                GLint* params)
    {
        getInstance()._glGetMaterialiv(face, pname, params);
    }

    static void GetPixelMapfv(GLenum map,
                                GLfloat* values)
    {
        getInstance()._glGetPixelMapfv(map, values);
    }

    static void GetPixelMapuiv(GLenum map,
                                 GLuint* values)
    {
        getInstance()._glGetPixelMapuiv(map, values);
    }

    static void GetPixelMapusv(GLenum map,
                                 GLushort* values)
    {
        getInstance()._glGetPixelMapusv(map, values);
    }

    static void GetPolygonStipple(GLubyte* mask)
    {
        getInstance()._glGetPolygonStipple(mask);
    }

    static void GetTexEnvfv(GLenum target,
                              GLenum pname,
                              GLfloat* params)
    {
        getInstance()._glGetTexEnvfv(target, pname, params);
    }

    static void GetTexEnviv(GLenum target,
                              GLenum pname,
                              GLint* params)
    {
        getInstance()._glGetTexEnviv(target, pname, params);
    }

    static void GetTexGendv(GLenum coord,
                              GLenum pname,
                              GLdouble* params)
    {
        getInstance()._glGetTexGendv(coord, pname, params);
    }

    static void GetTexGenfv(GLenum coord,
                              GLenum pname,
                              GLfloat* params)
    {
        getInstance()._glGetTexGenfv(coord, pname, params);
    }

    static void GetTexGeniv(GLenum coord,
                              GLenum pname,
                              GLint* params)
    {
        getInstance()._glGetTexGeniv(coord, pname, params);
    }

    static GLboolean IsList(GLuint list)
    {
        return getInstance()._glIsList(list);
    }

    static void Frustum(GLdouble left,
                          GLdouble right,
                          GLdouble bottom,
                          GLdouble top,
                          GLdouble zNear,
                          GLdouble zFar)
    {
        getInstance()._glFrustum(left, right, bottom, top, zNear, zFar);
    }

    static void LoadIdentity()
    {
        getInstance()._glLoadIdentity();
    }

    static void LoadMatrixf(const GLfloat* m)
    {
        getInstance()._glLoadMatrixf(m);
    }

    static void LoadMatrixd(const GLdouble* m)
    {
        getInstance()._glLoadMatrixd(m);
    }

    static void MatrixMode(GLenum mode)
    {
        getInstance()._glMatrixMode(mode);
    }

    static void MultMatrixf(const GLfloat* m)
    {
        getInstance()._glMultMatrixf(m);
    }

    static void MultMatrixd(const GLdouble* m)
    {
        getInstance()._glMultMatrixd(m);
    }

    static void Ortho(GLdouble left,
                        GLdouble right,
                        GLdouble bottom,
                        GLdouble top,
                        GLdouble zNear,
                        GLdouble zFar)
    {
        getInstance()._glOrtho(left, right, bottom, top, zNear, zFar);
    }

    static void PopMatrix()
    {
        getInstance()._glPopMatrix();
    }

    static void PushMatrix()
    {
        getInstance()._glPushMatrix();
    }

    static void Rotated(GLdouble angle,
                          GLdouble x,
                          GLdouble y,
                          GLdouble z)
    {
        getInstance()._glRotated(angle, x, y, z);
    }

    static void Rotatef(GLfloat angle,
                          GLfloat x,
                          GLfloat y,
                          GLfloat z)
    {
        getInstance()._glRotatef(angle, x, y, z);
    }

    static void Scaled(GLdouble x,
                         GLdouble y,
                         GLdouble z)
    {
        getInstance()._glScaled(x, y, z);
    }

    static void Scalef(GLfloat x,
                         GLfloat y,
                         GLfloat z)
    {
        getInstance()._glScalef(x, y, z);
    }

    static void Translated(GLdouble x,
                             GLdouble y,
                             GLdouble z)
    {
        getInstance()._glTranslated(x, y, z);
    }

    static void Translatef(GLfloat x,
                             GLfloat y,
                             GLfloat z)
    {
        getInstance()._glTranslatef(x, y, z);
    }

    static void DrawArrays(GLenum mode,
                             GLint first,
                             GLsizei count)
    {
        getInstance()._glDrawArrays(mode, first, count);
    }

    static void DrawElements(GLenum mode,
                               GLsizei count,
                               GLenum type,
                               const void* indices)
    {
        getInstance()._glDrawElements(mode, count, type, indices);
    }

    static void GetPointerv(GLenum pname,
                              void** params)
    {
        getInstance()._glGetPointerv(pname, params);
    }

    static void PolygonOffset(GLfloat factor,
                                GLfloat units)
    {
        getInstance()._glPolygonOffset(factor, units);
    }

    static void CopyTexImage1D(GLenum target,
                                 GLint level,
                                 GLenum internalformat,
                                 GLint x,
                                 GLint y,
                                 GLsizei width,
                                 GLint border)
    {
        getInstance()._glCopyTexImage1D(target, level, internalformat, x, y, width, border);
    }

    static void CopyTexImage2D(GLenum target,
                                 GLint level,
                                 GLenum internalformat,
                                 GLint x,
                                 GLint y,
                                 GLsizei width,
                                 GLsizei height,
                                 GLint border)
    {
        getInstance()._glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
    }

    static void CopyTexSubImage1D(GLenum target,
                                    GLint level,
                                    GLint xoffset,
                                    GLint x,
                                    GLint y,
                                    GLsizei width)
    {
        getInstance()._glCopyTexSubImage1D(target, level, xoffset, x, y, width);
    }

    static void CopyTexSubImage2D(GLenum target,
                                    GLint level,
                                    GLint xoffset,
                                    GLint yoffset,
                                    GLint x,
                                    GLint y,
                                    GLsizei width,
                                    GLsizei height)
    {
        getInstance()._glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
    }

    static void TexSubImage1D(GLenum target,
                                GLint level,
                                GLint xoffset,
                                GLsizei width,
                                GLenum format,
                                GLenum type,
                                const void* pixels)
    {
        getInstance()._glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
    }

    static void TexSubImage2D(GLenum target,
                                GLint level,
                                GLint xoffset,
                                GLint yoffset,
                                GLsizei width,
                                GLsizei height,
                                GLenum format,
                                GLenum type,
                                const void* pixels)
    {
        getInstance()._glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    }

    static void BindTexture(GLenum target,
                              GLuint texture)
    {
        getInstance()._glBindTexture(target, texture);
    }

    static void DeleteTextures(GLsizei n,
                                 const GLuint* textures)
    {
        getInstance()._glDeleteTextures(n, textures);
    }

    static void GenTextures(GLsizei n,
                              GLuint* textures)
    {
        getInstance()._glGenTextures(n, textures);
    }

    static GLboolean IsTexture(GLuint texture)
    {
        return getInstance()._glIsTexture(texture);
    }

    static void ArrayElement(GLint i)
    {
        getInstance()._glArrayElement(i);
    }

    static void ColorPointer(GLint size,
                               GLenum type,
                               GLsizei stride,
                               const void* pointer)
    {
        getInstance()._glColorPointer(size, type, stride, pointer);
    }

    static void DisableClientState(GLenum array)
    {
        getInstance()._glDisableClientState(array);
    }

    static void EdgeFlagPointer(GLsizei stride,
                                  const void* pointer)
    {
        getInstance()._glEdgeFlagPointer(stride, pointer);
    }

    static void EnableClientState(GLenum array)
    {
        getInstance()._glEnableClientState(array);
    }

    static void IndexPointer(GLenum type,
                               GLsizei stride,
                               const void* pointer)
    {
        getInstance()._glIndexPointer(type, stride, pointer);
    }

    static void InterleavedArrays(GLenum format,
                                    GLsizei stride,
                                    const void* pointer)
    {
        getInstance()._glInterleavedArrays(format, stride, pointer);
    }

    static void NormalPointer(GLenum type,
                                GLsizei stride,
                                const void* pointer)
    {
        getInstance()._glNormalPointer(type, stride, pointer);
    }

    static void TexCoordPointer(GLint size,
                                  GLenum type,
                                  GLsizei stride,
                                  const void* pointer)
    {
        getInstance()._glTexCoordPointer(size, type, stride, pointer);
    }

    static void VertexPointer(GLint size,
                                GLenum type,
                                GLsizei stride,
                                const void* pointer)
    {
        getInstance()._glVertexPointer(size, type, stride, pointer);
    }

    static GLboolean AreTexturesResident(GLsizei n,
                                           const GLuint* textures,
                                           GLboolean* residences)
    {
        return getInstance()._glAreTexturesResident(n, textures, residences);
    }

    static void PrioritizeTextures(GLsizei n,
                                     const GLuint* textures,
                                     const GLfloat* priorities)
    {
        getInstance()._glPrioritizeTextures(n, textures, priorities);
    }

    static void Indexub(GLubyte c)
    {
        getInstance()._glIndexub(c);
    }

    static void Indexubv(const GLubyte* c)
    {
        getInstance()._glIndexubv(c);
    }

    static void PopClientAttrib()
    {
        getInstance()._glPopClientAttrib();
    }

    static void PushClientAttrib(GLbitfield mask)
    {
        getInstance()._glPushClientAttrib(mask);
    }

    static void DrawRangeElements(GLenum mode,
                                    GLuint start,
                                    GLuint end,
                                    GLsizei count,
                                    GLenum type,
                                    const void* indices)
    {
        getInstance()._glDrawRangeElements(mode, start, end, count, type, indices);
    }

    static void TexImage3D(GLenum target,
                             GLint level,
                             GLint internalformat,
                             GLsizei width,
                             GLsizei height,
                             GLsizei depth,
                             GLint border,
                             GLenum format,
                             GLenum type,
                             const void* pixels)
    {
        getInstance()._glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
    }

    static void TexSubImage3D(GLenum target,
                                GLint level,
                                GLint xoffset,
                                GLint yoffset,
                                GLint zoffset,
                                GLsizei width,
                                GLsizei height,
                                GLsizei depth,
                                GLenum format,
                                GLenum type,
                                const void* pixels)
    {
        getInstance()._glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    }

    static void CopyTexSubImage3D(GLenum target,
                                    GLint level,
                                    GLint xoffset,
                                    GLint yoffset,
                                    GLint zoffset,
                                    GLint x,
                                    GLint y,
                                    GLsizei width,
                                    GLsizei height)
    {
        getInstance()._glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    }

    static void ActiveTexture(GLenum texture)
    {
        getInstance()._glActiveTexture(texture);
    }

    static void SampleCoverage(GLfloat value,
                                 GLboolean invert)
    {
        getInstance()._glSampleCoverage(value, invert);
    }

    static void CompressedTexImage3D(GLenum target,
                                       GLint level,
                                       GLenum internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLsizei depth,
                                       GLint border,
                                       GLsizei imageSize,
                                       const void* data)
    {
        getInstance()._glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
    }

    static void CompressedTexImage2D(GLenum target,
                                       GLint level,
                                       GLenum internalformat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border,
                                       GLsizei imageSize,
                                       const void* data)
    {
        getInstance()._glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
    }

    static void CompressedTexImage1D(GLenum target,
                                       GLint level,
                                       GLenum internalformat,
                                       GLsizei width,
                                       GLint border,
                                       GLsizei imageSize,
                                       const void* data)
    {
        getInstance()._glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
    }

    static void CompressedTexSubImage3D(GLenum target,
                                          GLint level,
                                          GLint xoffset,
                                          GLint yoffset,
                                          GLint zoffset,
                                          GLsizei width,
                                          GLsizei height,
                                          GLsizei depth,
                                          GLenum format,
                                          GLsizei imageSize,
                                          const void* data)
    {
        getInstance()._glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    }

    static void CompressedTexSubImage2D(GLenum target,
                                          GLint level,
                                          GLint xoffset,
                                          GLint yoffset,
                                          GLsizei width,
                                          GLsizei height,
                                          GLenum format,
                                          GLsizei imageSize,
                                          const void* data)
    {
        getInstance()._glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    }

    static void CompressedTexSubImage1D(GLenum target,
                                          GLint level,
                                          GLint xoffset,
                                          GLsizei width,
                                          GLenum format,
                                          GLsizei imageSize,
                                          const void* data)
    {
        getInstance()._glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
    }

    static void GetCompressedTexImage(GLenum target,
                                        GLint level,
                                        void* img)
    {
        getInstance()._glGetCompressedTexImage(target, level, img);
    }

    static void ClientActiveTexture(GLenum texture)
    {
        getInstance()._glClientActiveTexture(texture);
    }

    static void MultiTexCoord1d(GLenum target,
                                  GLdouble s)
    {
        getInstance()._glMultiTexCoord1d(target, s);
    }

    static void MultiTexCoord1dv(GLenum target,
                                   const GLdouble* v)
    {
        getInstance()._glMultiTexCoord1dv(target, v);
    }

    static void MultiTexCoord1f(GLenum target,
                                  GLfloat s)
    {
        getInstance()._glMultiTexCoord1f(target, s);
    }

    static void MultiTexCoord1fv(GLenum target,
                                   const GLfloat* v)
    {
        getInstance()._glMultiTexCoord1fv(target, v);
    }

    static void MultiTexCoord1i(GLenum target,
                                  GLint s)
    {
        getInstance()._glMultiTexCoord1i(target, s);
    }

    static void MultiTexCoord1iv(GLenum target,
                                   const GLint* v)
    {
        getInstance()._glMultiTexCoord1iv(target, v);
    }

    static void MultiTexCoord1s(GLenum target,
                                  GLshort s)
    {
        getInstance()._glMultiTexCoord1s(target, s);
    }

    static void MultiTexCoord1sv(GLenum target,
                                   const GLshort* v)
    {
        getInstance()._glMultiTexCoord1sv(target, v);
    }

    static void MultiTexCoord2d(GLenum target,
                                  GLdouble s,
                                  GLdouble t)
    {
        getInstance()._glMultiTexCoord2d(target, s, t);
    }

    static void MultiTexCoord2dv(GLenum target,
                                   const GLdouble* v)
    {
        getInstance()._glMultiTexCoord2dv(target, v);
    }

    static void MultiTexCoord2f(GLenum target,
                                  GLfloat s,
                                  GLfloat t)
    {
        getInstance()._glMultiTexCoord2f(target, s, t);
    }

    static void MultiTexCoord2fv(GLenum target,
                                   const GLfloat* v)
    {
        getInstance()._glMultiTexCoord2fv(target, v);
    }

    static void MultiTexCoord2i(GLenum target,
                                  GLint s,
                                  GLint t)
    {
        getInstance()._glMultiTexCoord2i(target, s, t);
    }

    static void MultiTexCoord2iv(GLenum target,
                                   const GLint* v)
    {
        getInstance()._glMultiTexCoord2iv(target, v);
    }

    static void MultiTexCoord2s(GLenum target,
                                  GLshort s,
                                  GLshort t)
    {
        getInstance()._glMultiTexCoord2s(target, s, t);
    }

    static void MultiTexCoord2sv(GLenum target,
                                   const GLshort* v)
    {
        getInstance()._glMultiTexCoord2sv(target, v);
    }

    static void MultiTexCoord3d(GLenum target,
                                  GLdouble s,
                                  GLdouble t,
                                  GLdouble r)
    {
        getInstance()._glMultiTexCoord3d(target, s, t, r);
    }

    static void MultiTexCoord3dv(GLenum target,
                                   const GLdouble* v)
    {
        getInstance()._glMultiTexCoord3dv(target, v);
    }

    static void MultiTexCoord3f(GLenum target,
                                  GLfloat s,
                                  GLfloat t,
                                  GLfloat r)
    {
        getInstance()._glMultiTexCoord3f(target, s, t, r);
    }

    static void MultiTexCoord3fv(GLenum target,
                                   const GLfloat* v)
    {
        getInstance()._glMultiTexCoord3fv(target, v);
    }

    static void MultiTexCoord3i(GLenum target,
                                  GLint s,
                                  GLint t,
                                  GLint r)
    {
        getInstance()._glMultiTexCoord3i(target, s, t, r);
    }

    static void MultiTexCoord3iv(GLenum target,
                                   const GLint* v)
    {
        getInstance()._glMultiTexCoord3iv(target, v);
    }

    static void MultiTexCoord3s(GLenum target,
                                  GLshort s,
                                  GLshort t,
                                  GLshort r)
    {
        getInstance()._glMultiTexCoord3s(target, s, t, r);
    }

    static void MultiTexCoord3sv(GLenum target,
                                   const GLshort* v)
    {
        getInstance()._glMultiTexCoord3sv(target, v);
    }

    static void MultiTexCoord4d(GLenum target,
                                  GLdouble s,
                                  GLdouble t,
                                  GLdouble r,
                                  GLdouble q)
    {
        getInstance()._glMultiTexCoord4d(target, s, t, r, q);
    }

    static void MultiTexCoord4dv(GLenum target,
                                   const GLdouble* v)
    {
        getInstance()._glMultiTexCoord4dv(target, v);
    }

    static void MultiTexCoord4f(GLenum target,
                                  GLfloat s,
                                  GLfloat t,
                                  GLfloat r,
                                  GLfloat q)
    {
        getInstance()._glMultiTexCoord4f(target, s, t, r, q);
    }

    static void MultiTexCoord4fv(GLenum target,
                                   const GLfloat* v)
    {
        getInstance()._glMultiTexCoord4fv(target, v);
    }

    static void MultiTexCoord4i(GLenum target,
                                  GLint s,
                                  GLint t,
                                  GLint r,
                                  GLint q)
    {
        getInstance()._glMultiTexCoord4i(target, s, t, r, q);
    }

    static void MultiTexCoord4iv(GLenum target,
                                   const GLint* v)
    {
        getInstance()._glMultiTexCoord4iv(target, v);
    }

    static void MultiTexCoord4s(GLenum target,
                                  GLshort s,
                                  GLshort t,
                                  GLshort r,
                                  GLshort q)
    {
        getInstance()._glMultiTexCoord4s(target, s, t, r, q);
    }

    static void MultiTexCoord4sv(GLenum target,
                                   const GLshort* v)
    {
        getInstance()._glMultiTexCoord4sv(target, v);
    }

    static void LoadTransposeMatrixf(const GLfloat* m)
    {
        getInstance()._glLoadTransposeMatrixf(m);
    }

    static void LoadTransposeMatrixd(const GLdouble* m)
    {
        getInstance()._glLoadTransposeMatrixd(m);
    }

    static void MultTransposeMatrixf(const GLfloat* m)
    {
        getInstance()._glMultTransposeMatrixf(m);
    }

    static void MultTransposeMatrixd(const GLdouble* m)
    {
        getInstance()._glMultTransposeMatrixd(m);
    }

    static void BlendFuncSeparate(GLenum sfactorRGB,
                                    GLenum dfactorRGB,
                                    GLenum sfactorAlpha,
                                    GLenum dfactorAlpha)
    {
        getInstance()._glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    }

    static void MultiDrawArrays(GLenum mode,
                                  const GLint* first,
                                  const GLsizei* count,
                                  GLsizei drawcount)
    {
        getInstance()._glMultiDrawArrays(mode, first, count, drawcount);
    }

    static void MultiDrawElements(GLenum mode,
                                    const GLsizei* count,
                                    GLenum type,
                                    const void** indices,
                                    GLsizei drawcount)
    {
        getInstance()._glMultiDrawElements(mode, count, type, indices, drawcount);
    }

    static void PointParameterf(GLenum pname,
                                  GLfloat param)
    {
        getInstance()._glPointParameterf(pname, param);
    }

    static void PointParameterfv(GLenum pname,
                                   const GLfloat* params)
    {
        getInstance()._glPointParameterfv(pname, params);
    }

    static void PointParameteri(GLenum pname,
                                  GLint param)
    {
        getInstance()._glPointParameteri(pname, param);
    }

    static void PointParameteriv(GLenum pname,
                                   const GLint* params)
    {
        getInstance()._glPointParameteriv(pname, params);
    }

    static void FogCoordf(GLfloat coord)
    {
        getInstance()._glFogCoordf(coord);
    }

    static void FogCoordfv(const GLfloat* coord)
    {
        getInstance()._glFogCoordfv(coord);
    }

    static void FogCoordd(GLdouble coord)
    {
        getInstance()._glFogCoordd(coord);
    }

    static void FogCoorddv(const GLdouble* coord)
    {
        getInstance()._glFogCoorddv(coord);
    }

    static void FogCoordPointer(GLenum type,
                                  GLsizei stride,
                                  const void* pointer)
    {
        getInstance()._glFogCoordPointer(type, stride, pointer);
    }

    static void SecondaryColor3b(GLbyte red,
                                   GLbyte green,
                                   GLbyte blue)
    {
        getInstance()._glSecondaryColor3b(red, green, blue);
    }

    static void SecondaryColor3bv(const GLbyte* v)
    {
        getInstance()._glSecondaryColor3bv(v);
    }

    static void SecondaryColor3d(GLdouble red,
                                   GLdouble green,
                                   GLdouble blue)
    {
        getInstance()._glSecondaryColor3d(red, green, blue);
    }

    static void SecondaryColor3dv(const GLdouble* v)
    {
        getInstance()._glSecondaryColor3dv(v);
    }

    static void SecondaryColor3f(GLfloat red,
                                   GLfloat green,
                                   GLfloat blue)
    {
        getInstance()._glSecondaryColor3f(red, green, blue);
    }

    static void SecondaryColor3fv(const GLfloat* v)
    {
        getInstance()._glSecondaryColor3fv(v);
    }

    static void SecondaryColor3i(GLint red,
                                   GLint green,
                                   GLint blue)
    {
        getInstance()._glSecondaryColor3i(red, green, blue);
    }

    static void SecondaryColor3iv(const GLint* v)
    {
        getInstance()._glSecondaryColor3iv(v);
    }

    static void SecondaryColor3s(GLshort red,
                                   GLshort green,
                                   GLshort blue)
    {
        getInstance()._glSecondaryColor3s(red, green, blue);
    }

    static void SecondaryColor3sv(const GLshort* v)
    {
        getInstance()._glSecondaryColor3sv(v);
    }

    static void SecondaryColor3ub(GLubyte red,
                                    GLubyte green,
                                    GLubyte blue)
    {
        getInstance()._glSecondaryColor3ub(red, green, blue);
    }

    static void SecondaryColor3ubv(const GLubyte* v)
    {
        getInstance()._glSecondaryColor3ubv(v);
    }

    static void SecondaryColor3ui(GLuint red,
                                    GLuint green,
                                    GLuint blue)
    {
        getInstance()._glSecondaryColor3ui(red, green, blue);
    }

    static void SecondaryColor3uiv(const GLuint* v)
    {
        getInstance()._glSecondaryColor3uiv(v);
    }

    static void SecondaryColor3us(GLushort red,
                                    GLushort green,
                                    GLushort blue)
    {
        getInstance()._glSecondaryColor3us(red, green, blue);
    }

    static void SecondaryColor3usv(const GLushort* v)
    {
        getInstance()._glSecondaryColor3usv(v);
    }

    static void SecondaryColorPointer(GLint size,
                                        GLenum type,
                                        GLsizei stride,
                                        const void* pointer)
    {
        getInstance()._glSecondaryColorPointer(size, type, stride, pointer);
    }

    static void WindowPos2d(GLdouble x,
                              GLdouble y)
    {
        getInstance()._glWindowPos2d(x, y);
    }

    static void WindowPos2dv(const GLdouble* v)
    {
        getInstance()._glWindowPos2dv(v);
    }

    static void WindowPos2f(GLfloat x,
                              GLfloat y)
    {
        getInstance()._glWindowPos2f(x, y);
    }

    static void WindowPos2fv(const GLfloat* v)
    {
        getInstance()._glWindowPos2fv(v);
    }

    static void WindowPos2i(GLint x,
                              GLint y)
    {
        getInstance()._glWindowPos2i(x, y);
    }

    static void WindowPos2iv(const GLint* v)
    {
        getInstance()._glWindowPos2iv(v);
    }

    static void WindowPos2s(GLshort x,
                              GLshort y)
    {
        getInstance()._glWindowPos2s(x, y);
    }

    static void WindowPos2sv(const GLshort* v)
    {
        getInstance()._glWindowPos2sv(v);
    }

    static void WindowPos3d(GLdouble x,
                              GLdouble y,
                              GLdouble z)
    {
        getInstance()._glWindowPos3d(x, y, z);
    }

    static void WindowPos3dv(const GLdouble* v)
    {
        getInstance()._glWindowPos3dv(v);
    }

    static void WindowPos3f(GLfloat x,
                              GLfloat y,
                              GLfloat z)
    {
        getInstance()._glWindowPos3f(x, y, z);
    }

    static void WindowPos3fv(const GLfloat* v)
    {
        getInstance()._glWindowPos3fv(v);
    }

    static void WindowPos3i(GLint x,
                              GLint y,
                              GLint z)
    {
        getInstance()._glWindowPos3i(x, y, z);
    }

    static void WindowPos3iv(const GLint* v)
    {
        getInstance()._glWindowPos3iv(v);
    }

    static void WindowPos3s(GLshort x,
                              GLshort y,
                              GLshort z)
    {
        getInstance()._glWindowPos3s(x, y, z);
    }

    static void WindowPos3sv(const GLshort* v)
    {
        getInstance()._glWindowPos3sv(v);
    }

    static void BlendColor(GLfloat red,
                             GLfloat green,
                             GLfloat blue,
                             GLfloat alpha)
    {
        getInstance()._glBlendColor(red, green, blue, alpha);
    }

    static void BlendEquation(GLenum mode)
    {
        getInstance()._glBlendEquation(mode);
    }

    static void GenQueries(GLsizei n,
                             GLuint* ids)
    {
        getInstance()._glGenQueries(n, ids);
    }

    static void DeleteQueries(GLsizei n,
                                const GLuint* ids)
    {
        getInstance()._glDeleteQueries(n, ids);
    }

    static GLboolean IsQuery(GLuint id)
    {
        return getInstance()._glIsQuery(id);
    }

    static void BeginQuery(GLenum target,
                             GLuint id)
    {
        getInstance()._glBeginQuery(target, id);
    }

    static void EndQuery(GLenum target)
    {
        getInstance()._glEndQuery(target);
    }

    static void GetQueryiv(GLenum target,
                             GLenum pname,
                             GLint* params)
    {
        getInstance()._glGetQueryiv(target, pname, params);
    }

    static void GetQueryObjectiv(GLuint id,
                                   GLenum pname,
                                   GLint* params)
    {
        getInstance()._glGetQueryObjectiv(id, pname, params);
    }

    static void GetQueryObjectuiv(GLuint id,
                                    GLenum pname,
                                    GLuint* params)
    {
        getInstance()._glGetQueryObjectuiv(id, pname, params);
    }

    static void BindBuffer(GLenum target,
                             GLuint buffer)
    {
        getInstance()._glBindBuffer(target, buffer);
    }

    static void DeleteBuffers(GLsizei n,
                                const GLuint* buffers)
    {
        getInstance()._glDeleteBuffers(n, buffers);
    }

    static void GenBuffers(GLsizei n,
                             GLuint* buffers)
    {
        getInstance()._glGenBuffers(n, buffers);
    }

    static GLboolean IsBuffer(GLuint buffer)
    {
        return getInstance()._glIsBuffer(buffer);
    }

    static void BufferData(GLenum target,
                             GLsizeiptr size,
                             const void* data,
                             GLenum usage)
    {
        getInstance()._glBufferData(target, size, data, usage);
    }

    static void BufferSubData(GLenum target,
                                GLintptr offset,
                                GLsizeiptr size,
                                const void* data)
    {
        getInstance()._glBufferSubData(target, offset, size, data);
    }

    static void GetBufferSubData(GLenum target,
                                   GLintptr offset,
                                   GLsizeiptr size,
                                   void* data)
    {
        getInstance()._glGetBufferSubData(target, offset, size, data);
    }

    static void* MapBuffer(GLenum target,
                             GLenum access)
    {
        return getInstance()._glMapBuffer(target, access);
    }

    static GLboolean UnmapBuffer(GLenum target)
    {
        return getInstance()._glUnmapBuffer(target);
    }

    static void GetBufferParameteriv(GLenum target,
                                       GLenum pname,
                                       GLint* params)
    {
        getInstance()._glGetBufferParameteriv(target, pname, params);
    }

    static void GetBufferPointerv(GLenum target,
                                    GLenum pname,
                                    void** params)
    {
        getInstance()._glGetBufferPointerv(target, pname, params);
    }

    static void BlendEquationSeparate(GLenum modeRGB,
                                        GLenum modeAlpha)
    {
        getInstance()._glBlendEquationSeparate(modeRGB, modeAlpha);
    }

    static void DrawBuffers(GLsizei n,
                              const GLenum* bufs)
    {
        getInstance()._glDrawBuffers(n, bufs);
    }

    static void StencilOpSeparate(GLenum face,
                                    GLenum sfail,
                                    GLenum dpfail,
                                    GLenum dppass)
    {
        getInstance()._glStencilOpSeparate(face, sfail, dpfail, dppass);
    }

    static void StencilFuncSeparate(GLenum face,
                                      GLenum func,
                                      GLint ref,
                                      GLuint mask)
    {
        getInstance()._glStencilFuncSeparate(face, func, ref, mask);
    }

    static void StencilMaskSeparate(GLenum face,
                                      GLuint mask)
    {
        getInstance()._glStencilMaskSeparate(face, mask);
    }

    static void AttachShader(GLuint program,
                               GLuint shader)
    {
        getInstance()._glAttachShader(program, shader);
    }

    static void BindAttribLocation(GLuint program,
                                     GLuint index,
                                     const GLchar* name)
    {
        getInstance()._glBindAttribLocation(program, index, name);
    }

    static void CompileShader(GLuint shader)
    {
        getInstance()._glCompileShader(shader);
    }

    static GLuint CreateProgram()
    {
        return getInstance()._glCreateProgram();
    }

    static GLuint CreateShader(GLenum type)
    {
        return getInstance()._glCreateShader(type);
    }

    static void DeleteProgram(GLuint program)
    {
        getInstance()._glDeleteProgram(program);
    }

    static void DeleteShader(GLuint shader)
    {
        getInstance()._glDeleteShader(shader);
    }

    static void DetachShader(GLuint program,
                               GLuint shader)
    {
        getInstance()._glDetachShader(program, shader);
    }

    static void DisableVertexAttribArray(GLuint index)
    {
        getInstance()._glDisableVertexAttribArray(index);
    }

    static void EnableVertexAttribArray(GLuint index)
    {
        getInstance()._glEnableVertexAttribArray(index);
    }

    static void GetActiveAttrib(GLuint program,
                                  GLuint index,
                                  GLsizei bufSize,
                                  GLsizei* length,
                                  GLint* size,
                                  GLenum* type,
                                  GLchar* name)
    {
        getInstance()._glGetActiveAttrib(program, index, bufSize, length, size, type, name);
    }

    static void GetActiveUniform(GLuint program,
                                   GLuint index,
                                   GLsizei bufSize,
                                   GLsizei* length,
                                   GLint* size,
                                   GLenum* type,
                                   GLchar* name)
    {
        getInstance()._glGetActiveUniform(program, index, bufSize, length, size, type, name);
    }

    static void GetAttachedShaders(GLuint program,
                                     GLsizei maxCount,
                                     GLsizei* count,
                                     GLuint* shaders)
    {
        getInstance()._glGetAttachedShaders(program, maxCount, count, shaders);
    }

    static GLint GetAttribLocation(GLuint program,
                                     const GLchar* name)
    {
        return getInstance()._glGetAttribLocation(program, name);
    }

    static void GetProgramiv(GLuint program,
                               GLenum pname,
                               GLint* params)
    {
        getInstance()._glGetProgramiv(program, pname, params);
    }

    static void GetProgramInfoLog(GLuint program,
                                    GLsizei bufSize,
                                    GLsizei* length,
                                    GLchar* infoLog)
    {
        getInstance()._glGetProgramInfoLog(program, bufSize, length, infoLog);
    }

    static void GetShaderiv(GLuint shader,
                              GLenum pname,
                              GLint* params)
    {
        getInstance()._glGetShaderiv(shader, pname, params);
    }

    static void GetShaderInfoLog(GLuint shader,
                                   GLsizei bufSize,
                                   GLsizei* length,
                                   GLchar* infoLog)
    {
        getInstance()._glGetShaderInfoLog(shader, bufSize, length, infoLog);
    }

    static void GetShaderSource(GLuint shader,
                                  GLsizei bufSize,
                                  GLsizei* length,
                                  GLchar* source)
    {
        getInstance()._glGetShaderSource(shader, bufSize, length, source);
    }

    static GLint GetUniformLocation(GLuint program,
                                      const GLchar* name)
    {
        return getInstance()._glGetUniformLocation(program, name);
    }

    static void GetUniformfv(GLuint program,
                               GLint location,
                               GLfloat* params)
    {
        getInstance()._glGetUniformfv(program, location, params);
    }

    static void GetUniformiv(GLuint program,
                               GLint location,
                               GLint* params)
    {
        getInstance()._glGetUniformiv(program, location, params);
    }

    static void GetVertexAttribdv(GLuint index,
                                    GLenum pname,
                                    GLdouble* params)
    {
        getInstance()._glGetVertexAttribdv(index, pname, params);
    }

    static void GetVertexAttribfv(GLuint index,
                                    GLenum pname,
                                    GLfloat* params)
    {
        getInstance()._glGetVertexAttribfv(index, pname, params);
    }

    static void GetVertexAttribiv(GLuint index,
                                    GLenum pname,
                                    GLint* params)
    {
        getInstance()._glGetVertexAttribiv(index, pname, params);
    }

    static void GetVertexAttribPointerv(GLuint index,
                                          GLenum pname,
                                          void** pointer)
    {
        getInstance()._glGetVertexAttribPointerv(index, pname, pointer);
    }

    static GLboolean IsProgram(GLuint program)
    {
        return getInstance()._glIsProgram(program);
    }

    static GLboolean IsShader(GLuint shader)
    {
        return getInstance()._glIsShader(shader);
    }

    static void LinkProgram(GLuint program)
    {
        getInstance()._glLinkProgram(program);
    }

    static void ShaderSource(GLuint shader,
                               GLsizei count,
                               const GLchar** string,
                               const GLint* length)
    {
        getInstance()._glShaderSource(shader, count, string, length);
    }

    static void UseProgram(GLuint program)
    {
        getInstance()._glUseProgram(program);
    }

    static void Uniform1f(GLint location,
                            GLfloat v0)
    {
        getInstance()._glUniform1f(location, v0);
    }

    static void Uniform2f(GLint location,
                            GLfloat v0,
                            GLfloat v1)
    {
        getInstance()._glUniform2f(location, v0, v1);
    }

    static void Uniform3f(GLint location,
                            GLfloat v0,
                            GLfloat v1,
                            GLfloat v2)
    {
        getInstance()._glUniform3f(location, v0, v1, v2);
    }

    static void Uniform4f(GLint location,
                            GLfloat v0,
                            GLfloat v1,
                            GLfloat v2,
                            GLfloat v3)
    {
        getInstance()._glUniform4f(location, v0, v1, v2, v3);
    }

    static void Uniform1i(GLint location,
                            GLint v0)
    {
        getInstance()._glUniform1i(location, v0);
    }

    static void Uniform2i(GLint location,
                            GLint v0,
                            GLint v1)
    {
        getInstance()._glUniform2i(location, v0, v1);
    }

    static void Uniform3i(GLint location,
                            GLint v0,
                            GLint v1,
                            GLint v2)
    {
        getInstance()._glUniform3i(location, v0, v1, v2);
    }

    static void Uniform4i(GLint location,
                            GLint v0,
                            GLint v1,
                            GLint v2,
                            GLint v3)
    {
        getInstance()._glUniform4i(location, v0, v1, v2, v3);
    }

    static void Uniform1fv(GLint location,
                             GLsizei count,
                             const GLfloat* value)
    {
        getInstance()._glUniform1fv(location, count, value);
    }

    static void Uniform2fv(GLint location,
                             GLsizei count,
                             const GLfloat* value)
    {
        getInstance()._glUniform2fv(location, count, value);
    }

    static void Uniform3fv(GLint location,
                             GLsizei count,
                             const GLfloat* value)
    {
        getInstance()._glUniform3fv(location, count, value);
    }

    static void Uniform4fv(GLint location,
                             GLsizei count,
                             const GLfloat* value)
    {
        getInstance()._glUniform4fv(location, count, value);
    }

    static void Uniform1iv(GLint location,
                             GLsizei count,
                             const GLint* value)
    {
        getInstance()._glUniform1iv(location, count, value);
    }

    static void Uniform2iv(GLint location,
                             GLsizei count,
                             const GLint* value)
    {
        getInstance()._glUniform2iv(location, count, value);
    }

    static void Uniform3iv(GLint location,
                             GLsizei count,
                             const GLint* value)
    {
        getInstance()._glUniform3iv(location, count, value);
    }

    static void Uniform4iv(GLint location,
                             GLsizei count,
                             const GLint* value)
    {
        getInstance()._glUniform4iv(location, count, value);
    }

    static void UniformMatrix2fv(GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat* value)
    {
        getInstance()._glUniformMatrix2fv(location, count, transpose, value);
    }

    static void UniformMatrix3fv(GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat* value)
    {
        getInstance()._glUniformMatrix3fv(location, count, transpose, value);
    }

    static void UniformMatrix4fv(GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat* value)
    {
        getInstance()._glUniformMatrix4fv(location, count, transpose, value);
    }

    static void ValidateProgram(GLuint program)
    {
        getInstance()._glValidateProgram(program);
    }

    static void VertexAttrib1d(GLuint index,
                                 GLdouble x)
    {
        getInstance()._glVertexAttrib1d(index, x);
    }

    static void VertexAttrib1dv(GLuint index,
                                  const GLdouble* v)
    {
        getInstance()._glVertexAttrib1dv(index, v);
    }

    static void VertexAttrib1f(GLuint index,
                                 GLfloat x)
    {
        getInstance()._glVertexAttrib1f(index, x);
    }

    static void VertexAttrib1fv(GLuint index,
                                  const GLfloat* v)
    {
        getInstance()._glVertexAttrib1fv(index, v);
    }

    static void VertexAttrib1s(GLuint index,
                                 GLshort x)
    {
        getInstance()._glVertexAttrib1s(index, x);
    }

    static void VertexAttrib1sv(GLuint index,
                                  const GLshort* v)
    {
        getInstance()._glVertexAttrib1sv(index, v);
    }

    static void VertexAttrib2d(GLuint index,
                                 GLdouble x,
                                 GLdouble y)
    {
        getInstance()._glVertexAttrib2d(index, x, y);
    }

    static void VertexAttrib2dv(GLuint index,
                                  const GLdouble* v)
    {
        getInstance()._glVertexAttrib2dv(index, v);
    }

    static void VertexAttrib2f(GLuint index,
                                 GLfloat x,
                                 GLfloat y)
    {
        getInstance()._glVertexAttrib2f(index, x, y);
    }

    static void VertexAttrib2fv(GLuint index,
                                  const GLfloat* v)
    {
        getInstance()._glVertexAttrib2fv(index, v);
    }

    static void VertexAttrib2s(GLuint index,
                                 GLshort x,
                                 GLshort y)
    {
        getInstance()._glVertexAttrib2s(index, x, y);
    }

    static void VertexAttrib2sv(GLuint index,
                                  const GLshort* v)
    {
        getInstance()._glVertexAttrib2sv(index, v);
    }

    static void VertexAttrib3d(GLuint index,
                                 GLdouble x,
                                 GLdouble y,
                                 GLdouble z)
    {
        getInstance()._glVertexAttrib3d(index, x, y, z);
    }

    static void VertexAttrib3dv(GLuint index,
                                  const GLdouble* v)
    {
        getInstance()._glVertexAttrib3dv(index, v);
    }

    static void VertexAttrib3f(GLuint index,
                                 GLfloat x,
                                 GLfloat y,
                                 GLfloat z)
    {
        getInstance()._glVertexAttrib3f(index, x, y, z);
    }

    static void VertexAttrib3fv(GLuint index,
                                  const GLfloat* v)
    {
        getInstance()._glVertexAttrib3fv(index, v);
    }

    static void VertexAttrib3s(GLuint index,
                                 GLshort x,
                                 GLshort y,
                                 GLshort z)
    {
        getInstance()._glVertexAttrib3s(index, x, y, z);
    }

    static void VertexAttrib3sv(GLuint index,
                                  const GLshort* v)
    {
        getInstance()._glVertexAttrib3sv(index, v);
    }

    static void VertexAttrib4Nbv(GLuint index,
                                   const GLbyte* v)
    {
        getInstance()._glVertexAttrib4Nbv(index, v);
    }

    static void VertexAttrib4Niv(GLuint index,
                                   const GLint* v)
    {
        getInstance()._glVertexAttrib4Niv(index, v);
    }

    static void VertexAttrib4Nsv(GLuint index,
                                   const GLshort* v)
    {
        getInstance()._glVertexAttrib4Nsv(index, v);
    }

    static void VertexAttrib4Nub(GLuint index,
                                   GLubyte x,
                                   GLubyte y,
                                   GLubyte z,
                                   GLubyte w)
    {
        getInstance()._glVertexAttrib4Nub(index, x, y, z, w);
    }

    static void VertexAttrib4Nubv(GLuint index,
                                    const GLubyte* v)
    {
        getInstance()._glVertexAttrib4Nubv(index, v);
    }

    static void VertexAttrib4Nuiv(GLuint index,
                                    const GLuint* v)
    {
        getInstance()._glVertexAttrib4Nuiv(index, v);
    }

    static void VertexAttrib4Nusv(GLuint index,
                                    const GLushort* v)
    {
        getInstance()._glVertexAttrib4Nusv(index, v);
    }

    static void VertexAttrib4bv(GLuint index,
                                  const GLbyte* v)
    {
        getInstance()._glVertexAttrib4bv(index, v);
    }

    static void VertexAttrib4d(GLuint index,
                                 GLdouble x,
                                 GLdouble y,
                                 GLdouble z,
                                 GLdouble w)
    {
        getInstance()._glVertexAttrib4d(index, x, y, z, w);
    }

    static void VertexAttrib4dv(GLuint index,
                                  const GLdouble* v)
    {
        getInstance()._glVertexAttrib4dv(index, v);
    }

    static void VertexAttrib4f(GLuint index,
                                 GLfloat x,
                                 GLfloat y,
                                 GLfloat z,
                                 GLfloat w)
    {
        getInstance()._glVertexAttrib4f(index, x, y, z, w);
    }

    static void VertexAttrib4fv(GLuint index,
                                  const GLfloat* v)
    {
        getInstance()._glVertexAttrib4fv(index, v);
    }

    static void VertexAttrib4iv(GLuint index,
                                  const GLint* v)
    {
        getInstance()._glVertexAttrib4iv(index, v);
    }

    static void VertexAttrib4s(GLuint index,
                                 GLshort x,
                                 GLshort y,
                                 GLshort z,
                                 GLshort w)
    {
        getInstance()._glVertexAttrib4s(index, x, y, z, w);
    }

    static void VertexAttrib4sv(GLuint index,
                                  const GLshort* v)
    {
        getInstance()._glVertexAttrib4sv(index, v);
    }

    static void VertexAttrib4ubv(GLuint index,
                                   const GLubyte* v)
    {
        getInstance()._glVertexAttrib4ubv(index, v);
    }

    static void VertexAttrib4uiv(GLuint index,
                                   const GLuint* v)
    {
        getInstance()._glVertexAttrib4uiv(index, v);
    }

    static void VertexAttrib4usv(GLuint index,
                                   const GLushort* v)
    {
        getInstance()._glVertexAttrib4usv(index, v);
    }

    static void VertexAttribPointer(GLuint index,
                                      GLint size,
                                      GLenum type,
                                      GLboolean normalized,
                                      GLsizei stride,
                                      const void* pointer)
    {
        getInstance()._glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    }

    static void BindBufferARB(GLenum target,
                                GLuint buffer)
    {
        getInstance()._glBindBufferARB(target, buffer);
    }

    static void DeleteBuffersARB(GLsizei n,
                                   const GLuint* buffers)
    {
        getInstance()._glDeleteBuffersARB(n, buffers);
    }

    static void GenBuffersARB(GLsizei n,
                                GLuint* buffers)
    {
        getInstance()._glGenBuffersARB(n, buffers);
    }

    static GLboolean IsBufferARB(GLuint buffer)
    {
        return getInstance()._glIsBufferARB(buffer);
    }

    static void BufferDataARB(GLenum target,
                                GLsizeiptrARB size,
                                const void* data,
                                GLenum usage)
    {
        getInstance()._glBufferDataARB(target, size, data, usage);
    }

    static void BufferSubDataARB(GLenum target,
                                   GLintptrARB offset,
                                   GLsizeiptrARB size,
                                   const void* data)
    {
        getInstance()._glBufferSubDataARB(target, offset, size, data);
    }

    static void GetBufferSubDataARB(GLenum target,
                                      GLintptrARB offset,
                                      GLsizeiptrARB size,
                                      void* data)
    {
        getInstance()._glGetBufferSubDataARB(target, offset, size, data);
    }

    static void* MapBufferARB(GLenum target,
                                GLenum access)
    {
        return getInstance()._glMapBufferARB(target, access);
    }

    static GLboolean UnmapBufferARB(GLenum target)
    {
        return getInstance()._glUnmapBufferARB(target);
    }

    static void GetBufferParameterivARB(GLenum target,
                                          GLenum pname,
                                          GLint* params)
    {
        getInstance()._glGetBufferParameterivARB(target, pname, params);
    }

    static void GetBufferPointervARB(GLenum target,
                                       GLenum pname,
                                       void** params)
    {
        getInstance()._glGetBufferPointervARB(target, pname, params);
    }

    static void BindVertexArray(GLuint array)
    {
        getInstance()._glBindVertexArray(array);
    }

    static void DeleteVertexArrays(GLsizei n,
                                     const GLuint* arrays)
    {
        getInstance()._glDeleteVertexArrays(n, arrays);
    }

    static void GenVertexArrays(GLsizei n,
                                  GLuint* arrays)
    {
        getInstance()._glGenVertexArrays(n, arrays);
    }

    static GLboolean IsVertexArray(GLuint array)
    {
        return getInstance()._glIsVertexArray(array);
    }

    static GLboolean IsRenderbuffer(GLuint renderbuffer)
    {
        return getInstance()._glIsRenderbuffer(renderbuffer);
    }

    static void BindRenderbuffer(GLenum target,
                                   GLuint renderbuffer)
    {
        getInstance()._glBindRenderbuffer(target, renderbuffer);
    }

    static void DeleteRenderbuffers(GLsizei n,
                                      const GLuint* renderbuffers)
    {
        getInstance()._glDeleteRenderbuffers(n, renderbuffers);
    }

    static void GenRenderbuffers(GLsizei n,
                                   GLuint* renderbuffers)
    {
        getInstance()._glGenRenderbuffers(n, renderbuffers);
    }

    static void RenderbufferStorage(GLenum target,
                                      GLenum internalformat,
                                      GLsizei width,
                                      GLsizei height)
    {
        getInstance()._glRenderbufferStorage(target, internalformat, width, height);
    }

    static void GetRenderbufferParameteriv(GLenum target,
                                             GLenum pname,
                                             GLint* params)
    {
        getInstance()._glGetRenderbufferParameteriv(target, pname, params);
    }

    static GLboolean IsFramebuffer(GLuint framebuffer)
    {
        return getInstance()._glIsFramebuffer(framebuffer);
    }

    static void BindFramebuffer(GLenum target,
                                  GLuint framebuffer)
    {
        getInstance()._glBindFramebuffer(target, framebuffer);
    }

    static void DeleteFramebuffers(GLsizei n,
                                     const GLuint* framebuffers)
    {
        getInstance()._glDeleteFramebuffers(n, framebuffers);
    }

    static void GenFramebuffers(GLsizei n,
                                  GLuint* framebuffers)
    {
        getInstance()._glGenFramebuffers(n, framebuffers);
    }

    static GLenum CheckFramebufferStatus(GLenum target)
    {
        return getInstance()._glCheckFramebufferStatus(target);
    }

    static void FramebufferTexture1D(GLenum target,
                                       GLenum attachment,
                                       GLenum textarget,
                                       GLuint texture,
                                       GLint level)
    {
        getInstance()._glFramebufferTexture1D(target, attachment, textarget, texture, level);
    }

    static void FramebufferTexture2D(GLenum target,
                                       GLenum attachment,
                                       GLenum textarget,
                                       GLuint texture,
                                       GLint level)
    {
        getInstance()._glFramebufferTexture2D(target, attachment, textarget, texture, level);
    }

    static void FramebufferTexture3D(GLenum target,
                                       GLenum attachment,
                                       GLenum textarget,
                                       GLuint texture,
                                       GLint level,
                                       GLint zoffset)
    {
        getInstance()._glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
    }

    static void FramebufferRenderbuffer(GLenum target,
                                          GLenum attachment,
                                          GLenum renderbuffertarget,
                                          GLuint renderbuffer)
    {
        getInstance()._glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
    }

    static void GetFramebufferAttachmentParameteriv(GLenum target,
                                                      GLenum attachment,
                                                      GLenum pname,
                                                      GLint* params)
    {
        getInstance()._glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
    }

    static void GenerateMipmap(GLenum target)
    {
        getInstance()._glGenerateMipmap(target);
    }

    static void BlitFramebuffer(GLint srcX0,
                                  GLint srcY0,
                                  GLint srcX1,
                                  GLint srcY1,
                                  GLint dstX0,
                                  GLint dstY0,
                                  GLint dstX1,
                                  GLint dstY1,
                                  GLbitfield mask,
                                  GLenum filter)
    {
        getInstance()._glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    }

    static void RenderbufferStorageMultisample(GLenum target,
                                                 GLsizei samples,
                                                 GLenum internalformat,
                                                 GLsizei width,
                                                 GLsizei height)
    {
        getInstance()._glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
    }

    static void FramebufferTextureLayer(GLenum target,
                                          GLenum attachment,
                                          GLuint texture,
                                          GLint level,
                                          GLint layer)
    {
        getInstance()._glFramebufferTextureLayer(target, attachment, texture, level, layer);
    }

    static GLboolean IsRenderbufferEXT(GLuint renderbuffer)
    {
        return getInstance()._glIsRenderbufferEXT(renderbuffer);
    }

    static void BindRenderbufferEXT(GLenum target,
                                      GLuint renderbuffer)
    {
        getInstance()._glBindRenderbufferEXT(target, renderbuffer);
    }

    static void DeleteRenderbuffersEXT(GLsizei n,
                                         const GLuint* renderbuffers)
    {
        getInstance()._glDeleteRenderbuffersEXT(n, renderbuffers);
    }

    static void GenRenderbuffersEXT(GLsizei n,
                                      GLuint* renderbuffers)
    {
        getInstance()._glGenRenderbuffersEXT(n, renderbuffers);
    }

    static void RenderbufferStorageEXT(GLenum target,
                                         GLenum internalformat,
                                         GLsizei width,
                                         GLsizei height)
    {
        getInstance()._glRenderbufferStorageEXT(target, internalformat, width, height);
    }

    static void GetRenderbufferParameterivEXT(GLenum target,
                                                GLenum pname,
                                                GLint* params)
    {
        getInstance()._glGetRenderbufferParameterivEXT(target, pname, params);
    }

    static GLboolean IsFramebufferEXT(GLuint framebuffer)
    {
        return getInstance()._glIsFramebufferEXT(framebuffer);
    }

    static void BindFramebufferEXT(GLenum target,
                                     GLuint framebuffer)
    {
        getInstance()._glBindFramebufferEXT(target, framebuffer);
    }

    static void DeleteFramebuffersEXT(GLsizei n,
                                        const GLuint* framebuffers)
    {
        getInstance()._glDeleteFramebuffersEXT(n, framebuffers);
    }

    static void GenFramebuffersEXT(GLsizei n,
                                     GLuint* framebuffers)
    {
        getInstance()._glGenFramebuffersEXT(n, framebuffers);
    }

    static GLenum CheckFramebufferStatusEXT(GLenum target)
    {
        return getInstance()._glCheckFramebufferStatusEXT(target);
    }

    static void FramebufferTexture1DEXT(GLenum target,
                                          GLenum attachment,
                                          GLenum textarget,
                                          GLuint texture,
                                          GLint level)
    {
        getInstance()._glFramebufferTexture1DEXT(target, attachment, textarget, texture, level);
    }

    static void FramebufferTexture2DEXT(GLenum target,
                                          GLenum attachment,
                                          GLenum textarget,
                                          GLuint texture,
                                          GLint level)
    {
        getInstance()._glFramebufferTexture2DEXT(target, attachment, textarget, texture, level);
    }

    static void FramebufferTexture3DEXT(GLenum target,
                                          GLenum attachment,
                                          GLenum textarget,
                                          GLuint texture,
                                          GLint level,
                                          GLint zoffset)
    {
        getInstance()._glFramebufferTexture3DEXT(target, attachment, textarget, texture, level, zoffset);
    }

    static void FramebufferRenderbufferEXT(GLenum target,
                                             GLenum attachment,
                                             GLenum renderbuffertarget,
                                             GLuint renderbuffer)
    {
        getInstance()._glFramebufferRenderbufferEXT(target, attachment, renderbuffertarget, renderbuffer);
    }

    static void GetFramebufferAttachmentParameterivEXT(GLenum target,
                                                         GLenum attachment,
                                                         GLenum pname,
                                                         GLint* params)
    {
        getInstance()._glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, params);
    }

    static void GenerateMipmapEXT(GLenum target)
    {
        getInstance()._glGenerateMipmapEXT(target);
    }

    static void BindVertexArrayAPPLE(GLuint array)
    {
        getInstance()._glBindVertexArrayAPPLE(array);
    }

    static void DeleteVertexArraysAPPLE(GLsizei n,
                                          const GLuint* arrays)
    {
        getInstance()._glDeleteVertexArraysAPPLE(n, arrays);
    }

    static void GenVertexArraysAPPLE(GLsizei n,
                                       GLuint* arrays)
    {
        getInstance()._glGenVertexArraysAPPLE(n, arrays);
    }

    static GLboolean IsVertexArrayAPPLE(GLuint array)
    {
        return getInstance()._glIsVertexArrayAPPLE(array);
    }
};

typedef OSGLFunctions<true> GL_GPU;
typedef OSGLFunctions<false> GL_CPU;

NATRON_NAMESPACE_EXIT

#endif // OSGLFUNCTIONS_H
