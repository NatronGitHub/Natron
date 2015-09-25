/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_RotoDrawableItem_h
#define Engine_RotoDrawableItem_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <set>
#include <string>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"
#include "Engine/CacheEntryHolder.h"
#include "Engine/RotoItem.h"

CLANG_DIAG_OFF(deprecated-declarations)
#include <QObject>
#include <QMutex>
#include <QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#define kRotoLayerBaseName "Layer"
#define kRotoBezierBaseName "Bezier"
#define kRotoOpenBezierBaseName "Pencil"
#define kRotoBSplineBaseName "BSpline"
#define kRotoEllipseBaseName "Ellipse"
#define kRotoRectangleBaseName "Rectangle"
#define kRotoPaintBrushBaseName "Brush"
#define kRotoPaintEraserBaseName "Eraser"
#define kRotoPaintBlurBaseName "Blur"
#define kRotoPaintSmearBaseName "Smear"
#define kRotoPaintSharpenBaseName "Sharpen"
#define kRotoPaintCloneBaseName "Clone"
#define kRotoPaintRevealBaseName "Reveal"
#define kRotoPaintDodgeBaseName "Dodge"
#define kRotoPaintBurnBaseName "Burn"

namespace Natron {
class Image;
class ImageComponents;
class Node;
}
namespace boost { namespace serialization { class access; } }

class RectI;
class RectD;
class KnobI;
class KnobBool;
class KnobDouble;
class KnobInt;
class KnobChoice;
class KnobColor;
typedef struct _cairo_pattern cairo_pattern_t;

class Curve;
class Bezier;
class RotoItemSerialization;
class BezierCP;

/**
 * @class A base class for all items made by the roto context
 **/
class RotoContext;
class RotoLayer;

namespace Transform {
struct Matrix3x3;
}

/**
 * @brief Base class for all drawable items
 **/
struct RotoDrawableItemPrivate;
class RotoDrawableItem
    : public RotoItem
    , public CacheEntryHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    RotoDrawableItem(const boost::shared_ptr<RotoContext>& context,
                     const std::string & name,
                     const boost::shared_ptr<RotoLayer>& parent,
                     bool isStroke);

    virtual ~RotoDrawableItem();
    
    void createNodes(bool connectNodes = true);
    
    void setNodesThreadSafetyForRotopainting();
    
    void incrementNodesAge();
    
    void refreshNodesConnections();

    virtual void clone(const RotoItem*  other) OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj) OVERRIDE;

    /**
     * @brief When deactivated the spline will not be taken into account when rendering, neither will it be visible on the viewer.
     * If isGloballyActivated() returns false, this function will return false aswell.
     **/
    bool isActivated(double time) const;
    void setActivated(bool a, double time);

    /**
     * @brief The opacity of the curve
     **/
    double getOpacity(double time) const;
    void setOpacity(double o,double time);

    /**
     * @brief The distance of the feather is the distance from the control point to the feather point plus
     * the feather distance returned by this function.
     **/
    double getFeatherDistance(double time) const;
    void setFeatherDistance(double d,double time);
    int getNumKeyframesFeatherDistance() const;

    /**
     * @brief The fall-off rate: 0.5 means half color is faded at half distance.
     **/
    double getFeatherFallOff(double time) const;
    void setFeatherFallOff(double f,double time);

    /**
     * @brief The color that the GUI should use to draw the overlay of the shape
     **/
    void getOverlayColor(double* color) const;
    void setOverlayColor(const double* color);

    bool getInverted(double time) const;

    void getColor(double time,double* color) const;
    void setColor(double time,double r,double g,double b);
    
    int getCompositingOperator() const;
    
    void setCompositingOperator(int op);

    std::string getCompositingOperatorToolTip() const;
    boost::shared_ptr<KnobBool> getActivatedKnob() const;
    boost::shared_ptr<KnobDouble> getFeatherKnob() const;
    boost::shared_ptr<KnobDouble> getFeatherFallOffKnob() const;
    boost::shared_ptr<KnobDouble> getOpacityKnob() const;
#ifdef NATRON_ROTO_INVERTIBLE
    boost::shared_ptr<KnobBool> getInvertedKnob() const;
#endif
    boost::shared_ptr<KnobChoice> getOperatorKnob() const;
    boost::shared_ptr<KnobColor> getColorKnob() const;
    boost::shared_ptr<KnobDouble> getCenterKnob() const;
    boost::shared_ptr<KnobInt> getLifeTimeFrameKnob() const;
    boost::shared_ptr<KnobDouble> getBrushSizeKnob() const;
    boost::shared_ptr<KnobDouble> getBrushHardnessKnob() const;
    boost::shared_ptr<KnobDouble> getBrushSpacingKnob() const;
    boost::shared_ptr<KnobDouble> getBrushEffectKnob() const;
    boost::shared_ptr<KnobDouble> getBrushVisiblePortionKnob() const;
    boost::shared_ptr<KnobBool> getPressureOpacityKnob() const;
    boost::shared_ptr<KnobBool> getPressureSizeKnob() const;
    boost::shared_ptr<KnobBool> getPressureHardnessKnob() const;
    boost::shared_ptr<KnobBool> getBuildupKnob() const;
    boost::shared_ptr<KnobInt> getTimeOffsetKnob() const;
    boost::shared_ptr<KnobChoice> getTimeOffsetModeKnob() const;
    boost::shared_ptr<KnobChoice> getBrushSourceTypeKnob() const;
    boost::shared_ptr<KnobDouble> getBrushCloneTranslateKnob() const;
    
    void setKeyframeOnAllTransformParameters(double time);

    const std::list<boost::shared_ptr<KnobI> >& getKnobs() const;
    
    virtual RectD getBoundingBox(double time) const = 0;

    void getTransformAtTime(double time,Transform::Matrix3x3* matrix) const;
    
    /**
     * @brief Set the transform at the given time
     **/
    void setTransform(double time, double tx, double ty, double sx, double sy, double centerX, double centerY, double rot, double skewX, double skewY);
    
    boost::shared_ptr<Natron::Node> getEffectNode() const;
    boost::shared_ptr<Natron::Node> getMergeNode() const;
    boost::shared_ptr<Natron::Node> getTimeOffsetNode() const;
    boost::shared_ptr<Natron::Node> getFrameHoldNode() const;
    
    void resetNodesThreadSafety();
    void deactivateNodes();
    void activateNodes();
    void disconnectNodes();

    virtual std::string getCacheID() const OVERRIDE FINAL;
    
Q_SIGNALS:

#ifdef NATRON_ROTO_INVERTIBLE
    void invertedStateChanged();
#endif

    void overlayColorChanged();

    void shapeColorChanged();

    void compositingOperatorChanged(int,int);

public Q_SLOTS:
    
    void onRotoKnobChanged(int,int);
    
protected:
    
    void rotoKnobChanged(const boost::shared_ptr<KnobI>& knob, Natron::ValueChangedReasonEnum reason);
    
    virtual void onTransformSet(double /*time*/) {}
    
    void addKnob(const boost::shared_ptr<KnobI>& knob);

private:
    
    void resetCloneTransformCenter();
    
    RotoDrawableItem* findPreviousInHierarchy();


    boost::scoped_ptr<RotoDrawableItemPrivate> _imp;
};



#endif // Engine_RotoDrawableItem_h
