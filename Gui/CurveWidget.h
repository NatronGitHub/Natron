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

#ifndef CURVE_WIDGET_H
#define CURVE_WIDGET_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"


#include "Gui/AnimationModuleViewBase.h"

NATRON_NAMESPACE_ENTER;

class CurveWidgetPrivate;

class CurveWidget : public AnimationViewBase
{


GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    CurveWidget(QWidget* parent);

    virtual ~CurveWidget() OVERRIDE;

    // Overriden from AnimationViewBase
    virtual void centerOnAllItems() OVERRIDE FINAL;
    virtual void centerOnSelection() OVERRIDE FINAL;
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual void refreshSelectionBoundingBox() OVERRIDE FINAL;
private:
    virtual void initializeImplementation(Gui* gui, const AnimationModuleBasePtr& model, AnimationViewBase* publicInterface) OVERRIDE FINAL;
    virtual void drawView() OVERRIDE FINAL;
public:

    
    /**
     * @brief Convenience function: fill the view according to the bounding box of all given curves.
     * @param curves The curves to compute the bounding box from. If empty, uses all selected curves
     * in the tree view.
     * @param useDisplayRange If true, then if the curves have a display range, it uses it in order
     * to compute the bounding box of a curve, otherwise it uses the bounding box of its keyframes.
     **/
    void centerOnCurves(const std::vector<CurveGuiPtr> & curves, bool useDisplayRange);


    // The interact will be drawn after the background and before any curve
    void setCustomInteract(const OfxParamOverlayInteractPtr & interactDesc);
    OfxParamOverlayInteractPtr getCustomInteract() const;

  
    
    /**
     * @brief Computes the position of the right and left tangents handle in curve coordinates
     **/
    void getKeyTangentPoints(KeyFrameSet::const_iterator it,
                             const KeyFrameSet& keys,
                             QPointF* leftTanPos,
                             QPointF* rightTanPos);


public Q_SLOTS:

    void onEditKeyFrameDialogFinished(bool accepted);

    void onExportCurveToAsciiActionTriggered();

    void onImportCurveFromAsciiActionTriggered();

    void onApplyLoopExpressionOnSelectedCurveActionTriggered();

    void onApplyNegateExpressionOnSelectedCurveActionTriggered();

    void onApplyReverseExpressionOnSelectedCurveActionTriggered();
private:

    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;


    void addKey(const CurveGuiPtr& curve, double xCurve, double yCurve);

private:


    boost::shared_ptr<CurveWidgetPrivate> _imp;
};


NATRON_NAMESPACE_EXIT;

Q_DECLARE_METATYPE(NATRON_NAMESPACE::CurveWidget*)

#endif // CURVE_WIDGET_H
