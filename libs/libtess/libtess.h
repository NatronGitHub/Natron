#ifndef LIBTESS_H
#define LIBTESS_H


#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************/

/* ErrorCode */
#define LIBTESS_GLU_INVALID_ENUM                   100900
#define LIBTESS_GLU_INVALID_VALUE                  100901
#define LIBTESS_GLU_OUT_OF_MEMORY                  100902
#define LIBTESS_GLU_INCOMPATIBLE_GL_VERSION        100903
#define LIBTESS_GLU_INVALID_OPERATION              100904

/* TessCallback */
#define LIBTESS_GLU_TESS_BEGIN                     100100
#define LIBTESS_GLU_BEGIN                          100100
#define LIBTESS_GLU_TESS_VERTEX                    100101
#define LIBTESS_GLU_VERTEX                         100101
#define LIBTESS_GLU_TESS_END                       100102
#define LIBTESS_GLU_END                            100102
#define LIBTESS_GLU_TESS_ERROR                     100103
#define LIBTESS_GLU_TESS_EDGE_FLAG                 100104
#define LIBTESS_GLU_EDGE_FLAG                      100104
#define LIBTESS_GLU_TESS_COMBINE                   100105
#define LIBTESS_GLU_TESS_BEGIN_DATA                100106
#define LIBTESS_GLU_TESS_VERTEX_DATA               100107
#define LIBTESS_GLU_TESS_END_DATA                  100108
#define LIBTESS_GLU_TESS_ERROR_DATA                100109
#define LIBTESS_GLU_TESS_EDGE_FLAG_DATA            100110
#define LIBTESS_GLU_TESS_COMBINE_DATA              100111

/* TessProperty */
#define LIBTESS_GLU_TESS_WINDING_RULE              100140
#define LIBTESS_GLU_TESS_BOUNDARY_ONLY             100141
#define LIBTESS_GLU_TESS_TOLERANCE                 100142

/* TessError */
#define LIBTESS_GLU_TESS_MISSING_BEGIN_POLYGON     100151
#define LIBTESS_GLU_TESS_MISSING_BEGIN_CONTOUR     100152
#define LIBTESS_GLU_TESS_MISSING_END_POLYGON       100153
#define LIBTESS_GLU_TESS_MISSING_END_CONTOUR       100154
#define LIBTESS_GLU_TESS_COORD_TOO_LARGE           100155
#define LIBTESS_GLU_TESS_NEED_COMBINE_CALLBACK     100156

/* TessWinding */
#define LIBTESS_GLU_TESS_WINDING_ODD               100130
#define LIBTESS_GLU_TESS_WINDING_NONZERO           100131
#define LIBTESS_GLU_TESS_WINDING_POSITIVE          100132
#define LIBTESS_GLU_TESS_WINDING_NEGATIVE          100133
#define LIBTESS_GLU_TESS_WINDING_ABS_GEQ_TWO       100134

/* OpenGL primitives */
#define LIBTESS_GL_TRIANGLE_FAN                    0x0006
#define LIBTESS_GL_LINE_LOOP                       0x0002
#define LIBTESS_GL_TRIANGLES                       0x0004
#define LIBTESS_GL_TRIANGLE_STRIP                  0x0005
/*************************************************************/


#ifdef __cplusplus
class libtess_GLUtesselator;
#else
typedef struct libtess_GLUtesselator libtess_GLUtesselator;
#endif


#define LIBTESS_GLU_TESS_MAX_COORD 1.0e150

/* Internal convenience typedefs */
typedef void (LIBTESS__GLUfuncptr)(void);

void libtess_gluTessBeginContour (libtess_GLUtesselator* tess);
void libtess_gluTessBeginPolygon (libtess_GLUtesselator* tess, void* data);
void libtess_gluTessCallback (libtess_GLUtesselator* tess, unsigned int which, LIBTESS__GLUfuncptr CallBackFunc);
void libtess_gluTessEndContour (libtess_GLUtesselator* tess);
void libtess_gluTessEndPolygon (libtess_GLUtesselator* tess);
void libtess_gluTessNormal (libtess_GLUtesselator* tess, double valueX, double valueY, double valueZ);
void libtess_gluTessProperty (libtess_GLUtesselator* tess, unsigned int which, double data);
void libtess_gluTessVertex (libtess_GLUtesselator* tess, double *location, void* data);


#ifdef __cplusplus
}
#endif

#endif // LIBTESS_H
