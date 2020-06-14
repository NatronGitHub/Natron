/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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
#include "Engine/RotoItem.h"
#include "Engine/Knob.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#define kMergeParamOutputChannelsR      "OutputChannelsR"
#define kMergeParamOutputChannelsG      "OutputChannelsG"
#define kMergeParamOutputChannelsB      "OutputChannelsB"
#define kMergeParamOutputChannelsA      "OutputChannelsA"

#define kMergeOFXParamMix "mix"
#define kMergeOFXParamOperation "operation"
#define kMergeOFXParamInvertMask "maskInvert"
#define kBlurCImgParamSize "size"
#define kTimeOffsetParamOffset "timeOffset"
#define kFrameHoldParamFirstFrame "firstFrame"

#define kTransformParamTranslate "translate"
#define kTransformParamRotate "rotate"
#define kTransformParamScale "scale"
#define kTransformParamUniform "uniform"
#define kTransformParamSkewX "skewX"
#define kTransformParamSkewY "skewY"
#define kTransformParamSkewOrder "skewOrder"
#define kTransformParamCenter "center"
#define kTransformParamFilter "filter"
#define kTransformParamResetCenter "resetCenter"
#define kTransformParamBlackOutside "black_outside"

#define kTimeBlurParamDivisions "division"
#define kTimeBlurParamShutter "shutter"
#define kTimeBlurParamShutterOffset "shutterOffset"
#define kTimeBlurParamCustomOffset "shutterCustomOffset"

#define kConstantParamColor "color"
#define kConstantParamOutputComponents "outputComponents"

/**
 * @class A base class for all items made by the roto context
 **/

/**
 * @brief Base class for all drawable items
 **/
struct RotoDrawableItemPrivate;
class RotoDrawableItem
    : public RotoItem
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    RotoDrawableItem(const KnobItemsTablePtr& model);

    // The copy constructor makes a shallow copy and only copy knob pointers
    RotoDrawableItem(const RotoDrawableItemPtr& other, const FrameViewRenderKey& key);

    virtual ~RotoDrawableItem();

    /**
     * @brief Returns the default overlay color
     **/
    static void getDefaultOverlayColor(double *r, double *g, double *b);

    const NodesList& getItemNodes() const;

    /**
     * @brief Create internal nodes used by the item. Does nothing if nodes are already created
     * @param connectNodes If true, this will also call refreshNodesConnections()
     **/
    void createNodes(bool connectNodes = true);

protected:
    /**
     * @brief Set the render thread safety to instance safe. This is called when user start drawing.
     **/
    void setNodesThreadSafetyForRotopainting();

    /**
     * @brief If setNodesThreadSafetyForRotopainting() was called, this will restore the thread safety of the internal 
     * nodes to their default thread safety for rendering.
     **/
    void resetNodesThreadSafety();

public:
    /**
     * @brief Connects nodes used by this item in the rotopaint tree. createNodes() must have been called prior
     * to calling this function.
     **/
    void refreshNodesConnections(const RotoDrawableItemPtr& previousItem);

    /**
     * @brief Refresh the internal tree of the item so that it's nodes are centered around the point at (x,y)
     **/
    void refreshNodesPositions(double x, double y);


    /**
     * @brief Deactivate all nodes used by this item
     **/
    void disconnectNodes();

    /**
     * @brief When deactivated the spline will not be taken into account when rendering, neither will it be visible on the viewer.
     * If isGloballyActivated() returns false, this function will return false aswell.
     **/
    bool isActivated(TimeValue time, ViewIdx view) const;

    /**
     * @brief Get the frame-range(s) through which the item is activated
     **/
    std::vector<RangeD> getActivatedRanges(ViewIdx view) const;

    std::string getCompositingOperatorToolTip() const;

    KnobBoolPtr getCustomRangeKnob() const;

    KnobDoublePtr getOpacityKnob() const;
    KnobButtonPtr getInvertedKnob() const;
    KnobChoicePtr getOperatorKnob() const;
    KnobColorPtr getColorKnob() const;
    KnobColorPtr getOverlayColorKnob() const;
    KnobDoublePtr getCenterKnob() const;
    KnobIntPtr getLifeTimeFrameKnob() const;

    KnobIntPtr getTimeOffsetKnob() const;
    KnobChoicePtr getTimeOffsetModeKnob() const;
    KnobChoicePtr getMergeInputAChoiceKnob() const;
    KnobChoicePtr getMergeMaskChoiceKnob() const;
    KnobDoublePtr getMixKnob() const;

    KnobDoublePtr getBrushSizeKnob() const;
    KnobDoublePtr getBrushHardnessKnob() const;
    KnobDoublePtr getBrushSpacingKnob() const;
    KnobDoublePtr getBrushVisiblePortionKnob() const;

    KnobChoicePtr getMotionBlurModeKnob() const;


    void getMotionBlurSettings(const TimeValue time,
                               ViewIdx view,
                               RangeD* range,
                               int* divisions) const;

    void setKeyframeOnAllTransformParameters(TimeValue time);

    virtual RectD getBoundingBox(TimeValue time, ViewIdx view) const = 0;


    /**
     * @brief Return pointer to internal node
     **/
    NodePtr getEffectNode() const;
    NodePtr getMergeNode() const;
    NodePtr getTimeOffsetNode() const;
    NodePtr getMaskNode() const;
    NodePtr getFrameHoldNode() const;
    NodePtr getBackgroundNode() const;

    void resetTransformCenter();

    virtual void initializeKnobs() OVERRIDE;

    virtual void fetchRenderCloneKnobs() OVERRIDE;

    virtual RotoStrokeType getBrushType() const = 0;

Q_SIGNALS:

    void invertedStateChanged();

    void shapeColorChanged();

    void compositingOperatorChanged(ViewSetSpec, DimIdx, ValueChangedReasonEnum);


    void onRotoKnobChanged(ViewSetSpec, DimIdx, ValueChangedReasonEnum);

protected:

    virtual bool getTransformAtTimeInternal(TimeValue time, ViewIdx view, Transform::Matrix3x3* matrix) const OVERRIDE ;

    virtual bool onKnobValueChanged(const KnobIPtr& k,
                                    ValueChangedReasonEnum reason,
                                    TimeValue time,
                                    ViewSetSpec view) OVERRIDE;

    virtual void refreshRightClickMenu(const KnobChoicePtr& refreshRightClickMenuInternal) OVERRIDE;


private:

    virtual void onItemRemovedFromModel() OVERRIDE FINAL;

    virtual void onItemInsertedInModel() OVERRIDE FINAL;

    boost::scoped_ptr<RotoDrawableItemPrivate> _imp;
};


class CompNodeItem : public RotoDrawableItem
{
public:

    CompNodeItem(const KnobItemsTablePtr& model)
    : RotoDrawableItem(model)
    {

    }


    virtual ~CompNodeItem()
    {

    }

    virtual RotoStrokeType getBrushType() const OVERRIDE FINAL
    {
        return eRotoStrokeTypeComp;
    }

    virtual RectD getBoundingBox(TimeValue time, ViewIdx view) const OVERRIDE FINAL;

    virtual std::string getBaseItemName() const OVERRIDE FINAL;

    virtual std::string getSerializationClassName() const OVERRIDE FINAL;
};


inline CompNodeItemPtr
toCompNodeItem(const KnobHolderPtr& item)
{
    return boost::dynamic_pointer_cast<CompNodeItem>(item);
}


NATRON_NAMESPACE_EXIT

#endif // Engine_RotoDrawableItem_h
