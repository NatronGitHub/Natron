#ifndef LIBTESS_INTERNAL_H
#define LIBTESS_INTERNAL_H

#include "libtess.h"

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifndef GLAPIENTRYP
#define GLAPIENTRYP
#endif

#ifndef GLAPI
#define GLAPI
#endif

/*************************************************************/

/* ErrorCode */
#define GLU_INVALID_ENUM LIBTESS_GLU_INVALID_ENUM
#define GLU_INVALID_VALUE LIBTESS_GLU_INVALID_VALUE
#define GLU_OUT_OF_MEMORY LIBTESS_GLU_OUT_OF_MEMORY
#define GLU_INCOMPATIBLE_GL_VERSION LIBTESS_GLU_INCOMPATIBLE_GL_VERSION
#define GLU_INVALID_OPERATION LIBTESS_GLU_INVALID_OPERATION

/* TessCallback */
#define GLU_TESS_BEGIN LIBTESS_GLU_TESS_BEGIN
#define GLU_BEGIN LIBTESS_GLU_BEGIN
#define GLU_TESS_VERTEX LIBTESS_GLU_TESS_VERTEX
#define GLU_VERTEX LIBTESS_GLU_VERTEX
#define GLU_TESS_END LIBTESS_GLU_TESS_END
#define GLU_END LIBTESS_GLU_END
#define GLU_TESS_ERROR LIBTESS_GLU_TESS_ERROR
#define GLU_TESS_EDGE_FLAG LIBTESS_GLU_TESS_EDGE_FLAG
#define GLU_EDGE_FLAG LIBTESS_GLU_EDGE_FLAG
#define GLU_TESS_COMBINE LIBTESS_GLU_TESS_COMBINE
#define GLU_TESS_BEGIN_DATA LIBTESS_GLU_TESS_BEGIN_DATA
#define GLU_TESS_VERTEX_DATA LIBTESS_GLU_TESS_VERTEX_DATA
#define GLU_TESS_END_DATA LIBTESS_GLU_TESS_END_DATA
#define GLU_TESS_ERROR_DATA LIBTESS_GLU_TESS_ERROR_DATA
#define GLU_TESS_EDGE_FLAG_DATA LIBTESS_GLU_TESS_EDGE_FLAG_DATA
#define GLU_TESS_COMBINE_DATA LIBTESS_GLU_TESS_COMBINE_DATA

/* TessProperty */
#define GLU_TESS_WINDING_RULE LIBTESS_GLU_TESS_WINDING_RULE
#define GLU_TESS_BOUNDARY_ONLY LIBTESS_GLU_TESS_BOUNDARY_ONLY
#define GLU_TESS_TOLERANCE LIBTESS_GLU_TESS_TOLERANCE

/* TessError */
#define GLU_TESS_MISSING_BEGIN_POLYGON LIBTESS_GLU_TESS_MISSING_BEGIN_POLYGON
#define GLU_TESS_MISSING_BEGIN_CONTOUR LIBTESS_GLU_TESS_MISSING_BEGIN_CONTOUR
#define GLU_TESS_MISSING_END_POLYGON LIBTESS_GLU_TESS_MISSING_END_POLYGON
#define GLU_TESS_MISSING_END_CONTOUR LIBTESS_GLU_TESS_MISSING_END_CONTOUR
#define GLU_TESS_COORD_TOO_LARGE LIBTESS_GLU_TESS_COORD_TOO_LARGE
#define GLU_TESS_NEED_COMBINE_CALLBACK LIBTESS_GLU_TESS_NEED_COMBINE_CALLBACK

/* TessWinding */
#define GLU_TESS_WINDING_ODD LIBTESS_GLU_TESS_WINDING_ODD
#define GLU_TESS_WINDING_NONZERO LIBTESS_GLU_TESS_WINDING_NONZERO
#define GLU_TESS_WINDING_POSITIVE LIBTESS_GLU_TESS_WINDING_POSITIVE
#define GLU_TESS_WINDING_NEGATIVE LIBTESS_GLU_TESS_WINDING_NEGATIVE
#define GLU_TESS_WINDING_ABS_GEQ_TWO LIBTESS_GLU_TESS_WINDING_ABS_GEQ_TWO

/* OpenGL primitives */
#define GL_TRIANGLE_FAN LIBTESS_GL_TRIANGLE_FAN
#define GL_LINE_LOOP LIBTESS_GL_LINE_LOOP
#define GL_TRIANGLES LIBTESS_GL_TRIANGLES
#define GL_TRIANGLE_STRIP LIBTESS_GL_TRIANGLE_STRIP
/*************************************************************/


#define GLUtesselator libtess_GLUtesselator


#define GLvoid void
#define GLenum unsigned int
#define GLdouble double
#define GLboolean unsigned char
#define GLfloat float

#define GL_FALSE 0

#define GLU_TESS_MAX_COORD LIBTESS_GLU_TESS_MAX_COORD

/* Internal convenience typedefs */
#define _GLUfuncptr LIBTESS__GLUfuncptr

#define gluTessBeginContour libtess_gluTessBeginContour
#define gluTessBeginPolygon libtess_gluTessBeginPolygon
#define gluTessCallback libtess_gluTessCallback
#define gluTessEndContour libtess_gluTessEndContour
#define gluTessEndPolygon libtess_gluTessEndPolygon
#define gluTessNormal libtess_gluTessNormal
#define gluTessProperty libtess_gluTessProperty
#define gluTessVertex libtess_gluTessVertex 
#define gluGetTessProperty libtess_gluGetTessProperty
#define gluNewTess libtess_gluNewTess
#define gluDeleteTess libtess_gluDeleteTess

/*backward compatibility, not exposed in libtess.h*/
void gluBeginPolygon( GLUtesselator *tess );
void gluNextContour( GLUtesselator *tess, GLenum type );
void gluEndPolygon( GLUtesselator *tess );


#endif // LIBTESS_INTERNAL_H
