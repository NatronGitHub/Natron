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
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject> // QObject
#include <QtGui/QColor> // QColor
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Curve.h" // Curve

#include "Gui/CurveGui.h" // Curves
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class CurveGui
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    CurveGui(CurveWidget *curveWidget,
             CurvePtr  curve,
             const QString & name,
             const QColor & color,
             int thickness = 1);

    virtual ~CurveGui() OVERRIDE;

    const QString & getName() const
    {
        return _name;
    }

    void setName(const QString & name)
    {
        _name = name;
    }

    const QColor & getColor() const
    {
        return _color;
    }

    void setColor(const QColor & color)
    {
        _color = color;
    }

    int getThickness() const
    {
        return _thickness;
    }

    void setThickness(int t)
    {
        _thickness = t;
    }

    bool isVisible() const
    {
        return _visible;
    }

    void setVisibleAndRefresh(bool visible);

    /**
     * @brief same as setVisibleAndRefresh() but doesn't Q_EMIT any signal for a repaint
     **/
    void setVisible(bool visible);

    bool isSelected() const
    {
        return _selected;
    }

    void setSelected(bool s)
    {
        _selected = s;
    }

    /**
     * @brief Evaluates the curve and returns the y position corresponding to the given x.
     * The coordinates are those of the curve, not of the widget.
     **/
    virtual double evaluate(bool useExpr, double x) const = 0;
    virtual CurvePtr  getInternalCurve() const;

    void drawCurve(int curveIndex, int curvesCount);

    virtual Curve::YRange getCurveYRange() const;
    virtual bool areKeyFramesTimeClampedToIntegers() const;
    virtual bool areKeyFramesValuesClampedToBooleans() const;
    virtual bool areKeyFramesValuesClampedToIntegers() const;
    virtual bool isYComponentMovable() const;
    virtual KeyFrameSet getKeyFrames() const;
    virtual int getKeyFrameIndex(double time) const = 0;
    virtual KeyFrame setKeyFrameInterpolation(KeyframeTypeEnum interp, int index) = 0;

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

protected:

    CurvePtr _internalCurve; ///ptr to the internal curve
    CurveWidget* _curveWidget;

private:

    QString _name; /// the name of the curve
    QColor _color; /// the color that must be used to draw the curve
    int _thickness; /// its thickness
    bool _visible; /// should we draw this curve ?
    bool _selected; /// is this curve selected
};

typedef std::list<CurveGuiPtr> Curves;

class KnobCurveGui
    : public CurveGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    KnobCurveGui(CurveWidget *curveWidget,
                 CurvePtr  curve,
                 const KnobGuiPtr& knob,
                 int dimension,
                 const QString & name,
                 const QColor & color,
                 int thickness = 1);


    KnobCurveGui(CurveWidget *curveWidget,
                 CurvePtr  curve,
                 const KnobIPtr& knob,
                 const RotoContextPtr& roto,
                 int dimension,
                 const QString & name,
                 const QColor & color,
                 int thickness = 1);

    virtual ~KnobCurveGui();

    virtual CurvePtr  getInternalCurve() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    KnobGuiPtr getKnobGui() const
    {
        return _knob.lock();
    }

    virtual double evaluate(bool useExpr, double x) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    RotoContextPtr getRotoContext() const { return _roto; }

    KnobIPtr getInternalKnob() const;

    int getDimension() const
    {
        return _dimension;
    }

    virtual int getKeyFrameIndex(double time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual KeyFrame setKeyFrameInterpolation(KeyframeTypeEnum interp, int index) OVERRIDE FINAL;

public Q_SLOTS:

    void onKnobInternalCurveChanged();
    void onKnobInterpolationChanged();

private:

    RotoContextPtr _roto;
    KnobIPtr _internalKnob;
    KnobGuiWPtr _knob; //< ptr to the knob holding this curve
    int _dimension; //< which dimension is this curve representing
};

class BezierCPCurveGui
    : public CurveGui
{
public:

    BezierCPCurveGui(CurveWidget *curveWidget,
                     const BezierPtr& bezier,
                     const RotoContextPtr& roto,
                     const QString & name,
                     const QColor & color,
                     int thickness = 1);

    virtual ~BezierCPCurveGui();

    RotoContextPtr getRotoContext() const { return _rotoContext; }

    BezierPtr getBezier() const;
    virtual double evaluate(bool useExpr, double x) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual Curve::YRange getCurveYRange() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool areKeyFramesTimeClampedToIntegers() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }

    virtual bool areKeyFramesValuesClampedToBooleans() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual bool areKeyFramesValuesClampedToIntegers() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }

    virtual bool isYComponentMovable() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual KeyFrameSet getKeyFrames() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getKeyFrameIndex(double time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual KeyFrame setKeyFrameInterpolation(KeyframeTypeEnum interp, int index) OVERRIDE FINAL;

private:


    BezierPtr _bezier;
    RotoContextPtr _rotoContext;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_CurveGui_h
