#ifndef DOPESHEETVIEW_H
#define DOPESHEETVIEW_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/OverlaySupport.h"

class DopeSheetEditor;
class DopeSheetViewPrivate;
class Gui;
class TimeLine;

class DopeSheetView : public QGLWidget, public OverlaySupport
{
    Q_OBJECT

public:
    friend class DopeSheetViewPrivate;

    explicit DopeSheetView(DopeSheetEditor *dopeSheetEditor, Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent = 0);
    ~DopeSheetView();

    void swapOpenGLBuffers() OVERRIDE FINAL;
    void redraw() OVERRIDE FINAL;
    void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    void getPixelScale(double &xScale, double &yScale) const OVERRIDE FINAL;
    void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    void saveOpenGLContext() OVERRIDE FINAL;
    void restoreOpenGLContext() OVERRIDE FINAL;
    unsigned int getCurrentRenderScale() const OVERRIDE FINAL;

protected:
    void initializeGL() OVERRIDE FINAL;
    void resizeGL(int w, int h) OVERRIDE FINAL;
    void paintGL() OVERRIDE FINAL;

private: /* functions */
    void renderText(double x, double y,
                    const QString &text,
                    const QColor &color,
                    const QFont &font) const;

private: /* attributes */
    boost::scoped_ptr<DopeSheetViewPrivate> _imp;
};

#endif // DOPESHEETVIEW_H
