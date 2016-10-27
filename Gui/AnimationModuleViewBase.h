/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_ANIMATIONMODULEVIEWBASE_H
#define NATRON_GUI_ANIMATIONMODULEVIEWBASE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui/GuiFwd.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h

#include <QGLWidget>
#include <QMutex>

#include "Engine/RectD.h"
#include "Engine/OverlaySupport.h"

#include "Gui/AnimItemBase.h"
#include "Gui/TextRenderer.h"
#include "Gui/ZoomContext.h"

#define KF_TEXTURES_COUNT 18
#define KF_PIXMAP_SIZE 14
#define KF_X_OFFSET 7

NATRON_NAMESPACE_ENTER;

class AnimationModuleViewPrivateBase;
class AnimationViewBase
: public QGLWidget
, public OverlaySupport
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:

    AnimationViewBase(QWidget* parent);

    virtual ~AnimationViewBase();

    void initialize(Gui* gui, AnimationModuleBasePtr& model);

    bool hasDrawnOnce() const;

    void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const;

    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio);

    virtual void refreshSelectionBoundingBox() = 0;

    void refreshSelectionBboxAndRedraw();

    //// Overriden from OverlaySupport
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;
    virtual void redraw() OVERRIDE FINAL;
    virtual void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const OVERRIDE FINAL;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE = 0;
    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL { return 0; }
    virtual RectD getViewportRect() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE FINAL;
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE FINAL;
    virtual int getWidgetFontHeight() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool renderText(double x, double y, const std::string &string, double r, double g, double b, int flags = 0) OVERRIDE FINAL;
    /////


    QPointF toZoomCoordinates(double x, double y) const;
    QPointF toWidgetCoordinates(double x, double y) const;


    /**
     * @brief Make the view fill the given rectangle.
     **/
    void centerOn(double xmin, double xmax, double ymin, double ymax);

    /**
     * @brief Convenience function: make the view fill the given x range while keeping the existing y range
     **/
    void centerOn(double xmin, double xmax);

    virtual void centerOnAllItems() = 0;

public Q_SLOTS:

    void onRemoveSelectedKeyFramesActionTriggered();

    void onCopySelectedKeyFramesToClipBoardActionTriggered();

    void onPasteClipBoardKeyFramesActionTriggered();

    void onPasteClipBoardKeyFramesAbsoluteActionTriggered();

    void onSelectAllKeyFramesActionTriggered();

    void onSetInterpolationActionTriggered();

    void onCenterAllCurvesActionTriggered();

    void onCenterOnSelectedCurvesActionTriggered();


    void onTimeLineFrameChanged(SequenceTime time, int reason);

    void onUpdateOnPenUpActionTriggered();
    

protected:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;
    virtual void initializeGL() OVERRIDE;

private:

    virtual void resizeGL(int width, int height) OVERRIDE FINAL;
    virtual void paintGL() OVERRIDE FINAL;

protected:

    void setImplementation(const boost::shared_ptr<AnimationModuleViewPrivateBase>& imp)
    {
        _imp = imp;
    }

    virtual void initializeImplementation(Gui* gui, AnimationModuleBasePtr& model, AnimationViewBase* publicInterface) = 0;

    virtual void drawView() = 0;
private:

    boost::shared_ptr<AnimationModuleViewPrivateBase> _imp;

};


NATRON_NAMESPACE_EXIT;
#endif // NATRON_GUI_ANIMATIONMODULEVIEWBASE_H
