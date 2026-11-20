libtess was taken from the GLU repository, https://gitlab.freedesktop.org/mesa/glu (formerly https://cgit.freedesktop.org/mesa/glu/), at commit 2318f592b712724a48dd8561b8e7397bb91d7eb7 (2026-07-25). Its src/libtess only differs from commit 0bf42e41c8b63fc2488dd8d41f696310b5a5a6a7, which Natron used before, by f53d1f4, which drops the array bound in the definition of gluTessVertex().

`libtess-natron.patch` is the full set of Natron changes to the upstream files (src/libtess/* and src/include/gluos.h). The other files (`libtess.h`, `libtess_internal.h`, `CMakeLists.txt`, `libtess.pro` and this file) are Natron's.

Natron changes:

- `mesh.h`, `tess.h`: include `libtess_internal.h` instead of `<GL/glu.h>`, so that the API is prefixed with `libtess_` (see `libtess.h`) and does not clash with a system GLU.
- `priorityq-heap.c`: include `<limits.h>` (`LONG_MAX`).
- `gluos.h`: don't redefine `WIN32_LEAN_AND_MEAN`, `NOGDI`, `NOIME` and `NOMINMAX`.
- `tess.c`: a vertex with a NaN coordinate is skipped and reported as `GLU_TESS_COORD_TOO_LARGE` (the sweep used to crash on it).
- `tess.c`, `tess.h`, `sweep.c`, `sweep.h`: after an out-of-memory error, free the mesh, the edge dictionary and its regions, and the priority queue (`__gl_sweepCleanup()`), which leaked; if the error happened while adding vertices, drop the polygon instead of sweeping a mesh that may be inconsistent (that crashed).
- `render.c`: the single triangle fan fast path (`__gl_renderCache()`) is only used if the fan triangles don't overlap (`FanIsSimple()`): a contour winding more than once around its first vertex, or passing through it again, was filled where the odd winding rule makes a hole.
- `geom.c`: the intersection of two nearly parallel edges is kept close to both edges (`RefineIntersection()`): it could be far from both, which distorted the polygon and filled wrong areas.
- `sweep.c`, `tess.c`, `tess.h`: on degenerate contours (e.g. collinear points along a diagonal with rounding noise, or a self-loop edge made by merging the endpoints of a zero-length edge), the sweep deleted an edge whose other half was still the upper edge of an active region, and then used it (heap-use-after-free); or it marked inside a face which reaches the sentinel edges, whose vertices have NULL client data. The edges are now only deleted when unused (`DeleteUnusedEdge()`, `FixUpperEdge()`), the inside faces are checked (`CheckInsideFaces()`), and otherwise the polygon is dropped and `GLU_INVALID_OPERATION` is reported (`TESS_JMP_DEGENERATE`).
- `sweep.c`: the winding number and inside flag of a new active region are initialized (`AddRegionBelow()`): on degenerate input some regions kept uninitialized values.

These bugs are also in Mesa's GLU, and the last two in libtess2 (https://github.com/memononen/libtess2), which has no fan fast path. `Tests/LibTess_Test.cpp` has their regression tests (except for the out-of-memory errors).

To update, clone the repository, apply the Natron patch, and copy the files:

    git clone https://gitlab.freedesktop.org/mesa/glu.git
    cd glu
    patch -p1 < /path-to-natron/libs/libtess/libtess-natron.patch
    cp src/libtess/*.[ch] src/include/gluos.h /path-to-natron/libs/libtess/

After changing the Natron copy, regenerate the patch in a clean checkout of the upstream commit:

    cp /path-to-natron/libs/libtess/gluos.h src/include/
    for f in src/libtess/*.[ch]; do cp /path-to-natron/libs/libtess/$(basename $f) $f; done
    git diff -- src/include src/libtess > /path-to-natron/libs/libtess/libtess-natron.patch
    git checkout -- src
