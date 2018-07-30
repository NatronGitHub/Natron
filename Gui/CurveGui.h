/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef Gui_CurveGui_h
#define Gui_CurveGui_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject> // QObject
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Curve.h" // Curve
#include "Gui/AnimItemBase.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct CurveGuiPrivate;
class CurveGui
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    CurveGui(AnimationModuleView *curveWidget, const AnimItemBasePtr& item, DimIdx dimension, ViewIdx view);

    virtual ~CurveGui() OVERRIDE;
    
    AnimItemBasePtr getItem() const;
    
    ViewIdx getView() const;
    
    DimIdx getDimension() const;

    // Convenience for calling getItem + getView + getDimension
    AnimItemDimViewIndexID getCurveID() const;

    QString getName() const;

    void getColor(double* color) const;

    void setColor(const double* color);

    int getLineWidth() const;

    void setLineWidth(int t);

    /**
     * @brief Evaluates the curve and returns the y position corresponding to the given x.
     * The coordinates are those of the curve, not of the widget.
     **/
    double evaluate(bool useExpr, double x) const;

    CurvePtr getInternalCurve() const;

    void drawCurve(int curveIndex, int curvesCount);

    Curve::YRange getCurveYRange() const;

    bool areKeyFramesTimeClampedToIntegers() const;

    bool areKeyFramesValuesClampedToBooleans() const;

    bool areKeyFramesValuesClampedToIntegers() const;

    bool isYComponentMovable() const;

    KeyFrameSet getKeyFrames() const;

    int getKeyFrameIndex(TimeValue time) const ;

    KeyFrame setKeyFrameInterpolation(KeyframeTypeEnum interp, int index);

private:

    void nextPointForSegment(const double x,
                             const KeyFrameSet & keyframes,
                             const bool isPeriodic,
                             const double parametricXMin,
                             const double parametricXMax,
                             KeyFrameSet::const_iterator* lastUpperIt,
                             double* x2,
                             KeyFrame* key,
                             bool* isKey );


private:

    boost::scoped_ptr<CurveGuiPrivate> _imp;

};

NATRON_NAMESPACE_EXIT

#endif // Gui_CurveGui_h
