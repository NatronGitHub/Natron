/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_AnimationModuleView_H
#define NATRON_GUI_AnimationModuleView_H

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

#include "Global/GLIncludes.h" //!<must be included before QGLWidget
#include <QtOpenGL/QGLWidget>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget
#include <QMutex>

#include "Engine/RectD.h"
#include "Engine/OverlaySupport.h"

#include "Gui/AnimItemBase.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/TextRenderer.h"
#include "Gui/ZoomContext.h"

NATRON_NAMESPACE_ENTER

class AnimationModuleViewPrivate;
class AnimationModuleView
: public QGLWidget
, public OverlaySupport
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:

    AnimationModuleView(QWidget* parent);

    virtual ~AnimationModuleView();

    void initialize(Gui* gui, const AnimationModuleBasePtr& model);

    bool hasDrawnOnce() const;

    //void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const;

    //void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio);

    void refreshSelectionBoundingBox();

    void refreshSelectionBboxAndRedraw();

    //// Overriden from OverlaySupport
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;
    virtual void redraw() OVERRIDE FINAL;
    virtual void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const OVERRIDE FINAL;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;
#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE FINAL;
#endif
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE;
    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL { return 0; }
    virtual RectD getViewportRect() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE FINAL;
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE FINAL;
    virtual int getWidgetFontHeight() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool renderText(double x, double y, const std::string &string, double r, double g, double b, double a, int flags = 0) OVERRIDE FINAL;
    /////


    /**
     * @brief Make the view fill the given rectangle.
     **/
    void centerOn(double xmin, double xmax, double ymin, double ymax);

    /**
     * @brief Convenience function: make the view fill the given x range while keeping the existing y range
     **/
    void centerOn(double xmin, double xmax);

    void centerOnAllItems();

    void centerOnSelection();

private:

    void centerOnItemsInternal(const std::vector<CurveGuiPtr>& curves,
                               const std::vector<NodeAnimPtr >& nodes,
                               const std::vector<TableItemAnimPtr>& tableItems);
public:


    RectD getSelectionRectangle() const;


    /**
     * @brief Returns the bounding box of the given curves
     */
    static RectD getCurvesKeyframeBbox(const std::vector<CurveGuiPtr> & curves);
    static RectD getCurvesDisplayRangesBbox(const std::vector<CurveGuiPtr> & curves);
    static std::pair<double,double> getCurvesKeyframeTimesBbox(const std::vector<CurveGuiPtr> & curves);


    /**
     * @brief Convenience function: fill the view according to the bounding box of all given curves.
     * @param curves The curves to compute the bounding box from. If empty, uses all selected curves
     * in the tree view.
     * @param useDisplayRange If true, then if the curves have a display range, it uses it in order
     * to compute the bounding box of a curve, otherwise it uses the bounding box of its keyframes.
     **/
    void centerOnCurves(const std::vector<CurveGuiPtr> & curves, bool useDisplayRange);

    // The interact will be drawn after the background and before any curve
    void setCustomInteract(const OverlayInteractBasePtr & interactDesc);

    OverlayInteractBasePtr getCustomInteract() const;

    /**
     * @brief Computes the position of the right and left tangents handle in curve coordinates
     **/
    void getKeyTangentPoints(KeyFrameSet::const_iterator it,
                             const KeyFrameSet& keys,
                             QPointF* leftTanPos,
                             QPointF* rightTanPos);

    // Uses curve editor zoom context
    QPointF toZoomCoordinates(double x, double y) const;
    QPointF toWidgetCoordinates(double x, double y) const;

    void setSelectedDerivative(MoveTangentCommand::SelectedTangentEnum tangent, AnimItemDimViewKeyFrame key);
    
public Q_SLOTS:

    

    void onTimeLineFrameChanged(SequenceTime time, int reason);

    void onUpdateOnPenUpActionTriggered();

    void onSelectionModelKeyframeSelectionChanged();

    void onRemoveSelectedKeyFramesActionTriggered();

    void onCopySelectedKeyFramesToClipBoardActionTriggered();

    void onPasteClipBoardKeyFramesActionTriggered();

    void onPasteClipBoardKeyFramesAbsoluteActionTriggered();

    void onSelectAllKeyFramesActionTriggered();

    void onSetInterpolationActionTriggered();

    void onCenterAllCurvesActionTriggered();

    void onCenterOnSelectedCurvesActionTriggered();

    void onAnimationTreeViewItemExpandedOrCollapsed(QTreeWidgetItem *item);

    void onAnimationTreeViewScrollbarMoved(int);

    void onEditKeyFrameDialogFinished(bool accepted);

    void onExportCurveToAsciiActionTriggered();

    void onImportCurveFromAsciiActionTriggered();

    void onApplyLoopExpressionOnSelectedCurveActionTriggered();

    void onApplyNegateExpressionOnSelectedCurveActionTriggered();

    void onApplyReverseExpressionOnSelectedCurveActionTriggered();

protected:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;
    virtual void initializeGL() OVERRIDE;

private:

    virtual void resizeGL(int width, int height) OVERRIDE FINAL;
    virtual void paintGL() OVERRIDE FINAL;

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;

public:
    
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;

private:


    friend class CurveGui;
    boost::shared_ptr<AnimationModuleViewPrivate> _imp;

};


NATRON_NAMESPACE_EXIT
#endif // NATRON_GUI_AnimationModuleView_H
