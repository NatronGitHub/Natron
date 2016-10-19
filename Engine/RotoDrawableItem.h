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


NATRON_NAMESPACE_ENTER;

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


    virtual ~RotoDrawableItem();

    /**
     * @brief Returns the default overlay color
     **/
    static void getDefaultOverlayColor(double *r, double *g, double *b);

    /**
     * @brief Create internal nodes used by the item. Does nothing if nodes are already created
     * @param connectNodes If true, this will also call refreshNodesConnections()
     **/
    void createNodes(bool connectNodes = true);

    /**
     * @brief Set the render thread safety to instance safe. This is called when user start drawing.
     **/
    void setNodesThreadSafetyForRotopainting();

    /**
     * @brief If setNodesThreadSafetyForRotopainting() was called, this will restore the thread safety of the internal 
     * nodes to their default thread safety for rendering.
     **/
    void resetNodesThreadSafety();

    /**
     * @brief Connects nodes used by this item in the rotopaint tree. createNodes() must have been called prior
     * to calling this function.
     **/
    void refreshNodesConnections(bool isTreeConcatenated);

    /**
     * @brief Deactivates all nodes used by this item
     **/
    void deactivateNodes();

    /**
     * @brief Activates all nodes used by this item
     **/
    void activateNodes();

    /**
     * @brief Deactivate all nodes used by this item
     **/
    void disconnectNodes();

    /**
     * @brief Clear the image pointers used by internal nodes of the item used to store previous computations
     * while rotopainting. This will force a render of the full image
     **/
    void clearPaintBuffers();

    /**
     * @brief When deactivated the spline will not be taken into account when rendering, neither will it be visible on the viewer.
     * If isGloballyActivated() returns false, this function will return false aswell.
     **/
    bool isActivated(double time, ViewGetSpec view) const;

    std::string getCompositingOperatorToolTip() const;

    KnobBoolPtr getActivatedKnob() const;

    KnobDoublePtr getOpacityKnob() const;
    KnobBoolPtr getInvertedKnob() const;
    KnobChoicePtr getOperatorKnob() const;
    KnobColorPtr getColorKnob() const;
    KnobDoublePtr getCenterKnob() const;
    KnobIntPtr getLifeTimeFrameKnob() const;

    KnobIntPtr getTimeOffsetKnob() const;
    KnobChoicePtr getTimeOffsetModeKnob() const;
    KnobChoicePtr getBrushSourceTypeKnob() const;

    KnobDoublePtr getBrushSizeKnob() const;
    KnobDoublePtr getBrushHardnessKnob() const;
    KnobDoublePtr getBrushSpacingKnob() const;
    KnobDoublePtr getBrushVisiblePortionKnob() const;



    void setKeyframeOnAllTransformParameters(double time);

    virtual RectD getBoundingBox(double time, ViewGetSpec view) const = 0;

    void getTransformAtTime(double time, ViewGetSpec view, Transform::Matrix3x3* matrix) const;
    
    void setExtraMatrix(bool setKeyframe, double time, ViewSetSpec view, const Transform::Matrix3x3& mat);

    /**
     * @brief Return pointer to internal node
     **/
    NodePtr getEffectNode() const;
    NodePtr getMergeNode() const;
    NodePtr getTimeOffsetNode() const;
    NodePtr getMaskNode() const;
    NodePtr getFrameHoldNode() const;


    void resetTransformCenter();

    virtual void initializeKnobs() OVERRIDE;


Q_SIGNALS:

    void invertedStateChanged();

    void shapeColorChanged();

    void compositingOperatorChanged(ViewSetSpec, DimIdx, ValueChangedReasonEnum);


    void onRotoKnobChanged(ViewSetSpec, DimIdx, ValueChangedReasonEnum);

protected:

    virtual bool onKnobValueChanged(const KnobIPtr& k,
                                    ValueChangedReasonEnum reason,
                                    double time,
                                    ViewSetSpec view,
                                    bool originatedFromMainThread) OVERRIDE;

    virtual void onTransformSet(double /*time*/, ViewSetSpec /*view*/) {}

private:

    virtual void onItemRemovedFromModel() OVERRIDE FINAL;

    boost::scoped_ptr<RotoDrawableItemPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // Engine_RotoDrawableItem_h
