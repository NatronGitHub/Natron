/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QSize>
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @class An histogram view in the histograms gui.
 **/
struct HistogramPrivate;
class Histogram
    : public QGLWidget
      , public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum DisplayModeEnum
    {
        eDisplayModeRGB = 0,
        eDisplayModeA,
        eDisplayModeY,
        eDisplayModeR,
        eDisplayModeG,
        eDisplayModeB
    };

    Histogram(Gui* gui,
              const QGLWidget* shareWidget = NULL);

    virtual ~Histogram();

    void renderText(double x, double y, const QString & text, const QColor & color, const QFont & font, int flags = 0) const;

    void setViewerCursor(const std::vector<double>& pickerColor);
    void hideViewerCursor();

    int getViewerTextureInputDisplayed() const;
    double getScreenPixelRatio() const;

public Q_SLOTS:

#ifndef NATRON_HISTOGRAM_USING_OPENGL

    void onCPUHistogramComputed();

    void computeHistogramAndRefresh(bool forceEvenIfNotVisible = false);

    void populateViewersChoices();

#endif

    void onDisplayModeChanged(QAction*);

    void onFilterChanged(QAction*);

    void onCurrentViewerChanged(QAction*);

    void onViewerImageChanged(ViewerGL* viewer, int texIndex, bool hasImageBackend);

private:

    virtual void initializeGL() OVERRIDE FINAL;
    virtual void paintGL() OVERRIDE FINAL;
    virtual void resizeGL(int w, int h) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void showEvent(QShowEvent* e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    boost::scoped_ptr<HistogramPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // HISTOGRAM_H
