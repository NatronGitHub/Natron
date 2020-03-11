/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef CURVE_WIDGET_H
#define CURVE_WIDGET_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtOpenGL/QGLWidget>
#include <QtCore/QMetaType>
#include <QtCore/QSize>
#include <QDialog>
#include <QtCore/QByteArray>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/OverlaySupport.h"
#include "Engine/Curve.h"

#include "Gui/CurveEditorUndoRedo.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class CurveWidgetPrivate;

class CurveWidget
    : public QGLWidget, public OverlaySupport
{
    friend class CurveGui;
    friend class CurveWidgetPrivate;

GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    /*Pass a null timeline ptr if you don't want interaction with the global timeline. */
    CurveWidget(Gui* gui,
                CurveSelection* selection,
                TimeLinePtr timeline = TimeLinePtr(),
                QWidget* parent = NULL,
                const QGLWidget* shareWidget = NULL);

    virtual ~CurveWidget() OVERRIDE;

    const QFont & getTextFont() const;

    void centerOn(double xmin, double xmax);
    void centerOn(double xmin, double xmax, double ymin, double ymax);

    void addCurveAndSetColor(const CurveGuiPtr& curve);

    void removeCurve(CurveGui* curve);

    void centerOn(const std::vector<CurveGuiPtr> & curves, bool useDisplayRange);

    void showCurvesAndHideOthers(const std::vector<CurveGuiPtr> & curves);

    void getVisibleCurves(std::vector<CurveGuiPtr>* curves) const;

    void setSelectedKeys(const SelectedKeys & keys);

    bool isSelectedKey(const CurveGuiPtr& curve, double time) const;

    void refreshSelectedKeysAndUpdate();

    double getZoomFactor() const;

    void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const;

    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio);

    /**
     * @brief Swap the OpenGL buffers.
     **/
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;

    /**
     * @brief Repaint
     **/
    virtual void redraw() OVERRIDE FINAL;

    /**
     * @brief Returns the width and height of the viewport in window coordinates.
     **/
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;

    /**
     * @brief Returns the pixel scale of the viewport.
     **/
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;

    /**
     * @brief Returns the colour of the background (i.e: clear color) of the viewport.
     **/
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL { return 0; }

    virtual RectD getViewportRect() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;

    /**
     * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
     **/
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE FINAL;

    /**
     * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
     **/
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE FINAL;

    /**
     * @brief Returns the font height, i.e: the height of the highest letter for this font
     **/
    virtual int getWidgetFontHeight() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns for a string the estimated pixel size it would take on the widget
     **/
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL WARN_UNUSED_RETURN;


    void updateSelectionAfterCurveChange(CurveGui* curve);

    void refreshCurveDisplayTangents(CurveGui* curve);

    QUndoStack* getUndoStack() const;
    void pushUndoCommand(QUndoCommand* cmd);

    // The interact will be drawn after the background and before any curve
    void setCustomInteract(const OfxParamOverlayInteractPtr & interactDesc);
    OfxParamOverlayInteractPtr getCustomInteract() const;

public Q_SLOTS:

    void refreshDisplayedTangents();

    void exportCurveToAscii();

    void importCurveFromAscii();

    void deleteSelectedKeyFrames();

    void copySelectedKeyFramesToClipBoard();

    void pasteKeyFramesFromClipBoardToSelectedCurve();

    void selectAllKeyFrames();

    void constantInterpForSelectedKeyFrames();

    void linearInterpForSelectedKeyFrames();

    void smoothForSelectedKeyFrames();

    void catmullromInterpForSelectedKeyFrames();

    void cubicInterpForSelectedKeyFrames();

    void horizontalInterpForSelectedKeyFrames();

    void breakDerivativesForSelectedKeyFrames();

    void frameAll();

    void frameSelectedCurve();

    void loopSelectedCurve();

    void negateSelectedCurve();

    void reverseSelectedCurve();

    void refreshSelectedKeysBbox();

    void onTimeLineFrameChanged(SequenceTime time, int reason);

    void onTimeLineBoundariesChanged(int, int);

    void onUpdateOnPenUpActionTriggered();

    void onEditKeyFrameDialogFinished();

private:

    virtual void initializeGL() OVERRIDE FINAL;
    virtual void resizeGL(int width, int height) OVERRIDE FINAL;
    virtual void paintGL() OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual bool renderText(double x, double y, const std::string &string, double r, double g, double b, int flags = 0) OVERRIDE FINAL;

    void renderText(double x, double y, const QString & text, const QColor & color, const QFont & font, int flags = 0) const;

    /**
     *@brief See toZoomCoordinates in ViewerGL.h
     **/
    QPointF toZoomCoordinates(double x, double y) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPointF toWidgetCoordinates(double x, double y) const;

    const QColor & getSelectedCurveColor() const;
    const QFont & getFont() const;
    const SelectedKeys & getSelectedKeyFrames() const;

    void addKey(const CurveGuiPtr& curve, double xCurve, double yCurve);

private:


    boost::scoped_ptr<CurveWidgetPrivate> _imp;
};


NATRON_NAMESPACE_EXIT

Q_DECLARE_METATYPE(NATRON_NAMESPACE::CurveWidget*)

#endif // CURVE_WIDGET_H
