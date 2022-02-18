/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#ifndef Engine_RotoDrawableItem_h
#define Engine_RotoDrawableItem_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated-declarations)
#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"
#include "Engine/CacheEntryHolder.h"
#include "Engine/RotoItem.h"
#include "Engine/Knob.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @class A base class for all items made by the roto context
 **/

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


    RotoDrawableItem(const RotoContextPtr& context,
                     const std::string & name,
                     const RotoLayerPtr& parent,
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
     * If isGloballyActivated() returns false, this function will return false as well.
     **/
    bool isActivated(double time) const;
    void setActivated(bool a, double time);

    /**
     * @brief The opacity of the curve
     **/
    double getOpacity(double time) const;
    void setOpacity(double o, double time);

    /**
     * @brief The distance of the feather is the distance from the control point to the feather point plus
     * the feather distance returned by this function.
     **/
    double getFeatherDistance(double time) const;
    void setFeatherDistance(double d, double time);
    int getNumKeyframesFeatherDistance() const;

    /**
     * @brief The fall-off rate: 0.5 means half color is faded at half distance.
     **/
    double getFeatherFallOff(double time) const;
    void setFeatherFallOff(double f, double time);

    /**
     * @brief The color that the GUI should use to draw the overlay of the shape
     **/
    void getOverlayColor(double* color) const;
    void setOverlayColor(const double* color);
    bool getInverted(double time) const;
    void getColor(double time, double* color) const;
    void setColor(double time, double r, double g, double b);

    int getCompositingOperator() const;

    void setCompositingOperator(int op);

    std::string getCompositingOperatorToolTip() const;
    KnobBoolPtr getActivatedKnob() const;
    KnobDoublePtr getFeatherKnob() const;
    KnobDoublePtr getFeatherFallOffKnob() const;
    KnobDoublePtr getOpacityKnob() const;
    KnobBoolPtr getInvertedKnob() const;
    KnobChoicePtr getOperatorKnob() const;
    KnobColorPtr getColorKnob() const;
    KnobDoublePtr getCenterKnob() const;
    KnobIntPtr getLifeTimeFrameKnob() const;
    KnobDoublePtr getBrushSizeKnob() const;
    KnobDoublePtr getBrushHardnessKnob() const;
    KnobDoublePtr getBrushSpacingKnob() const;
    KnobDoublePtr getBrushEffectKnob() const;
    KnobDoublePtr getBrushVisiblePortionKnob() const;
    KnobBoolPtr getPressureOpacityKnob() const;
    KnobBoolPtr getPressureSizeKnob() const;
    KnobBoolPtr getPressureHardnessKnob() const;
    KnobBoolPtr getBuildupKnob() const;
    KnobIntPtr getTimeOffsetKnob() const;
    KnobChoicePtr getTimeOffsetModeKnob() const;
    KnobChoicePtr getBrushSourceTypeKnob() const;
    KnobDoublePtr getBrushCloneTranslateKnob() const;
    KnobDoublePtr getMotionBlurAmountKnob() const;
    KnobDoublePtr getShutterOffsetKnob() const;
    KnobDoublePtr getShutterKnob() const;
    KnobChoicePtr getShutterTypeKnob() const;

    void setKeyframeOnAllTransformParameters(double time);

    const std::list<KnobIPtr>& getKnobs() const;

    KnobIPtr getKnobByName(const std::string& name) const;

    virtual RectD getBoundingBox(double time) const = 0;

    void getTransformAtTime(double time, Transform::Matrix3x3* matrix) const;

    /**
     * @brief Set the transform at the given time
     **/
    void setTransform(double time, double tx, double ty, double sx, double sy, double centerX, double centerY, double rot, double skewX, double skewY);

    void setExtraMatrix(bool setKeyframe, double time, const Transform::Matrix3x3& mat);

    NodePtr getEffectNode() const;
    NodePtr getMergeNode() const;
    NodePtr getTimeOffsetNode() const;
    NodePtr getFrameHoldNode() const;

    void resetNodesThreadSafety();
    void deactivateNodes();
    void activateNodes();
    void disconnectNodes();

    virtual std::string getCacheID() const OVERRIDE FINAL;

    void resetTransformCenter();

    ImagePtr renderMaskFromStroke(const ImagePlaneDesc& components,
                                                  const double time,
                                                  const ViewIdx view,
                                                  const ImageBitDepthEnum depth,
                                                  const unsigned int mipmapLevel,
                                                  const RectD& rotoNodeSrcRod);

private:

    ImagePtr renderMaskInternal(const RectI & roi,
                                                const ImagePlaneDesc& components,
                                                const double startTime,
                                                const double endTime,
                                                const double timeStep,
                                                const double time,
                                                const bool inverted,
                                                const ImageBitDepthEnum depth,
                                                const unsigned int mipmapLevel,
                                                const std::list<std::list<std::pair<Point, double> > >& strokes,
                                                const ImagePtr &image);

Q_SIGNALS:

    void invertedStateChanged();

    void overlayColorChanged();

    void shapeColorChanged();

    void compositingOperatorChanged(ViewSpec, int, int);

public Q_SLOTS:


    void onRotoKnobChanged(ViewSpec, int, int);

protected:

    void rotoKnobChanged(const KnobIPtr& knob, ValueChangedReasonEnum reason);

    virtual void onTransformSet(double /*time*/) {}

    void addKnob(const KnobIPtr& knob);

private:


    RotoDrawableItem* findPreviousInHierarchy();
    boost::scoped_ptr<RotoDrawableItemPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_RotoDrawableItem_h
