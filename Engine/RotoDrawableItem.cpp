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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "RotoDrawableItem.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cassert>
#include <cstring> // for std::memcpy
#include <stdexcept>

#include <QtCore/QLineF>
#include <QtCore/QDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Global/MemoryInfo.h"

#include "Engine/AppInstance.h"
#include "Engine/BezierCP.h"
#include "Engine/Bezier.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/ImageParams.h"
#include "Engine/MergingEnum.h"
#include "Engine/Interpolation.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoShapeRenderNode.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/NodeSerialization.h"



NATRON_NAMESPACE_ENTER;


struct RotoDrawableItemPrivate
{
    Q_DECLARE_TR_FUNCTIONS(RotoDrawableItem)

public:
    /*
     * The effect node corresponds to the following given the selected tool:
     * Stroke= RotoOFX
     * Blur = BlurCImg
     * Clone = TransformOFX
     * Sharpen = SharpenCImg
     * Smear = hand made tool
     * Reveal = Merge(over) with A being color type and B the tree upstream
     * Dodge/Burn = Merge(color-dodge/color-burn) with A being the tree upstream and B the color type
     *
     * Each effect is followed by a merge (except for the ones that use a merge) with the user given operator
     * onto the previous tree upstream of the effectNode.
     */
    NodePtr effectNode, maskNode;
    NodePtr mergeNode;
    NodePtr timeOffsetNode, frameHoldNode;
    KnobColorWPtr overlayColor; //< the color the shape overlay should be drawn with, defaults to smooth red
    KnobDoubleWPtr opacity; //< opacity of the rendered shape between 0 and 1

    KnobChoiceWPtr lifeTime;
    KnobBoolWPtr activated; //< should the curve be visible/rendered ? (animable)
    KnobIntWPtr lifeTimeFrame;
    KnobBoolWPtr invertKnob; //< invert the rendering
    KnobColorWPtr color;
    KnobChoiceWPtr compOperator;

    KnobDoubleWPtr brushSize;
    KnobDoubleWPtr brushSpacing;
    KnobDoubleWPtr brushHardness;
    KnobDoubleWPtr visiblePortion; // [0,1] by default

    // Transform
    KnobDoubleWPtr translate;
    KnobDoubleWPtr rotate;
    KnobDoubleWPtr scale;
    KnobBoolWPtr scaleUniform;
    KnobDoubleWPtr skewX;
    KnobDoubleWPtr skewY;
    KnobChoiceWPtr skewOrder;
    KnobDoubleWPtr center;
    KnobDoubleWPtr extraMatrix;
    
    KnobChoiceWPtr sourceColor;
    KnobIntWPtr timeOffset;
    KnobChoiceWPtr timeOffsetMode;


    RotoDrawableItemPrivate()
    : effectNode()
    , maskNode()
    , mergeNode()
    , timeOffsetNode()
    , frameHoldNode()
    {

    }

};

RotoDrawableItem::RotoDrawableItem(const KnobItemsTablePtr& model)
    : RotoItem(model)
    , _imp( new RotoDrawableItemPrivate() )
{
}

RotoDrawableItem::~RotoDrawableItem()
{
}


void
RotoDrawableItem::setNodesThreadSafetyForRotopainting()
{
    assert( toRotoStrokeItem( boost::dynamic_pointer_cast<RotoDrawableItem>( shared_from_this() ) ) );

    KnobItemsTablePtr model = getModel();
    if (!model) {
        return;
    }
    NodePtr node = model->getNode();
    if (!node) {
        return;
    }
    RotoPaintPtr isRotopaint = toRotoPaint(node->getEffectInstance());
    node->setRenderThreadSafety(eRenderSafetyInstanceSafe);
    if (isRotopaint) {
        isRotopaint->setWhileCreatingPaintStrokeOnMergeNodes(true);
    }
    if (_imp->effectNode) {
        _imp->effectNode->setWhileCreatingPaintStroke(true);
        _imp->effectNode->setRenderThreadSafety(eRenderSafetyInstanceSafe);
    }
    if (_imp->maskNode) {
        _imp->maskNode->setWhileCreatingPaintStroke(true);
        _imp->maskNode->setRenderThreadSafety(eRenderSafetyInstanceSafe);
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->setWhileCreatingPaintStroke(true);
        _imp->mergeNode->setRenderThreadSafety(eRenderSafetyInstanceSafe);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->setWhileCreatingPaintStroke(true);
        _imp->timeOffsetNode->setRenderThreadSafety(eRenderSafetyInstanceSafe);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->setWhileCreatingPaintStroke(true);
        _imp->frameHoldNode->setRenderThreadSafety(eRenderSafetyInstanceSafe);
    }
}

static void attachStrokeToNode(const NodePtr& node, const NodePtr& rotopaintNode, const RotoDrawableItemPtr& item)
{
    assert(rotopaintNode);
    assert(node);
    assert(item);
    node->attachRotoItem(item);

    // Link OpenGL enabled knob to the one on the Rotopaint so the user can control if GPU rendering is used in the roto internal node graph
    KnobChoicePtr glRenderKnob = node->getOrCreateOpenGLEnabledKnob();
    if (glRenderKnob) {
        KnobChoicePtr rotoPaintGLRenderKnob = rotopaintNode->getOrCreateOpenGLEnabledKnob();
        assert(rotoPaintGLRenderKnob);
        ignore_result(glRenderKnob->slaveTo(rotoPaintGLRenderKnob));
    }
}

void
RotoDrawableItem::createNodes(bool connectNodes)
{

    KnobItemsTablePtr model = getModel();
    if (!model) {
        return;
    }
    NodePtr node = model->getNode();
    if (!node) {
        return;
    }

    RotoPaintPtr rotoPaintEffect = toRotoPaint(node->getEffectInstance());
    AppInstancePtr app = rotoPaintEffect->getApp();

    QString fixedNamePrefix = QString::fromUtf8( rotoPaintEffect->getNode()->getScriptName_mt_safe().c_str() );
    fixedNamePrefix.append( QLatin1Char('_') );
    fixedNamePrefix.append( QString::fromUtf8( getScriptName_mt_safe().c_str() ) );

    QString pluginId;
    RotoStrokeType type;
    RotoDrawableItemPtr thisShared = boost::dynamic_pointer_cast<RotoDrawableItem>( shared_from_this() );
    assert(thisShared);
    RotoStrokeItemPtr isStroke = toRotoStrokeItem(thisShared);

    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }

    const QString maskPluginID = QString::fromUtf8(PLUGINID_NATRON_ROTOSHAPE);

    switch (type) {
    case eRotoStrokeTypeBlur:
        pluginId = QString::fromUtf8(PLUGINID_OFX_BLURCIMG);
        break;
    case eRotoStrokeTypeEraser:
        pluginId = QString::fromUtf8(PLUGINID_OFX_CONSTANT);
        break;
    case eRotoStrokeTypeSolid:
    case eRotoStrokeTypeSmear:
        pluginId = maskPluginID;
        break;
    case eRotoStrokeTypeClone:
    case eRotoStrokeTypeReveal:
        pluginId = QString::fromUtf8(PLUGINID_OFX_TRANSFORM);
        break;
    case eRotoStrokeTypeBurn:
    case eRotoStrokeTypeDodge:
        //uses merge
        break;
    case eRotoStrokeTypeSharpen:
        pluginId = QString::fromUtf8(PLUGINID_OFX_SHARPENCIMG);
        break;
    case eRotoStrokeTypeComp:
        // No node
        break;
    }

    QString baseFixedName = fixedNamePrefix;
    if ( !pluginId.isEmpty() ) {
        fixedNamePrefix.append( QString::fromUtf8("Effect") );

        CreateNodeArgsPtr args(CreateNodeArgs::create( pluginId.toStdString(), rotoPaintEffect ));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

        _imp->effectNode = app->createNode(args);
        if (!_imp->effectNode) {
            throw std::runtime_error("Rotopaint requires the plug-in " + pluginId.toStdString() + " in order to work");
        }
        assert(_imp->effectNode);


        if (type == eRotoStrokeTypeBlur) {
            assert(isStroke);
            // Link effect knob to size
            KnobIPtr knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
            knob->slaveTo(isStroke->getBrushEffectKnob());

        } else if ( (type == eRotoStrokeTypeClone) || (type == eRotoStrokeTypeReveal) ) {
            // Link transform knobs
            KnobIPtr translateKnob = _imp->effectNode->getKnobByName(kTransformParamTranslate);
            translateKnob->slaveTo(isStroke->getBrushCloneTranslateKnob());

            KnobIPtr rotateKnob = _imp->effectNode->getKnobByName(kTransformParamRotate);
            rotateKnob->slaveTo(isStroke->getBrushCloneRotateKnob());

            KnobIPtr scaleKnob = _imp->effectNode->getKnobByName(kTransformParamScale);
            scaleKnob->slaveTo(isStroke->getBrushCloneScaleKnob());

            KnobIPtr uniformKnob = _imp->effectNode->getKnobByName(kTransformParamUniform);
            uniformKnob->slaveTo(isStroke->getBrushCloneScaleUniformKnob());

            KnobIPtr skewxKnob = _imp->effectNode->getKnobByName(kTransformParamSkewX);
            skewxKnob->slaveTo(isStroke->getBrushCloneSkewXKnob());

            KnobIPtr skewyKnob = _imp->effectNode->getKnobByName(kTransformParamSkewY);
            skewyKnob->slaveTo(isStroke->getBrushCloneSkewYKnob());

            KnobIPtr skewOrderKnob = _imp->effectNode->getKnobByName(kTransformParamSkewOrder);
            skewOrderKnob->slaveTo(isStroke->getBrushCloneSkewOrderKnob());

            KnobIPtr centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
            centerKnob->slaveTo(isStroke->getBrushCloneCenterKnob());

            KnobIPtr filterKnob = _imp->effectNode->getKnobByName(kTransformParamFilter);
            filterKnob->slaveTo(isStroke->getBrushCloneFilterKnob());

            KnobIPtr boKnob = _imp->effectNode->getKnobByName(kTransformParamBlackOutside);
            boKnob->slaveTo(isStroke->getBrushCloneBlackOutsideKnob());


        }

        if (type == eRotoStrokeTypeSmear) {
            // For smear setup the type parameter
            KnobIPtr knob = _imp->effectNode->getKnobByName(kRotoShapeRenderNodeParamType);
            assert(knob);
            KnobChoicePtr typeChoice = toKnobChoice(knob);
            assert(typeChoice);
            typeChoice->setValue(1);
        }

        if ( (type == eRotoStrokeTypeClone) || (type == eRotoStrokeTypeReveal) ) {
            {
                fixedNamePrefix = baseFixedName;
                fixedNamePrefix.append( QString::fromUtf8("TimeOffset") );
                CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_OFX_TIMEOFFSET, rotoPaintEffect ));
                args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
                args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
                args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());

                _imp->timeOffsetNode = app->createNode(args);
                assert(_imp->timeOffsetNode);
                if (!_imp->timeOffsetNode) {
                    throw std::runtime_error("Rotopaint requires the plug-in " PLUGINID_OFX_TIMEOFFSET " in order to work");
                }

                // Link time offset knob
                KnobIPtr offsetKnob = _imp->timeOffsetNode->getKnobByName(kTimeOffsetParamOffset);
                offsetKnob->slaveTo(_imp->timeOffset.lock());
            }
            {
                fixedNamePrefix = baseFixedName;
                fixedNamePrefix.append( QString::fromUtf8("FrameHold") );
                CreateNodeArgsPtr args(CreateNodeArgs::create( PLUGINID_OFX_FRAMEHOLD, rotoPaintEffect ));
                args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
                args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
                args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
                _imp->frameHoldNode = app->createNode(args);
                assert(_imp->frameHoldNode);
                if (!_imp->frameHoldNode) {
                    throw std::runtime_error("Rotopaint requires the plug-in " PLUGINID_OFX_FRAMEHOLD " in order to work");
                }
                // Link time offset knob
                KnobIPtr offsetKnob = _imp->frameHoldNode->getKnobByName(kFrameHoldParamFirstFrame);
                offsetKnob->slaveTo(_imp->timeOffset.lock());
            }
        }
    }

    fixedNamePrefix = baseFixedName;
    fixedNamePrefix.append( QString::fromUtf8("Merge") );

    CreateNodeArgsPtr args(CreateNodeArgs::create( PLUGINID_OFX_MERGE, rotoPaintEffect ));
    args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
    args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());

    _imp->mergeNode = app->createNode(args);
    assert(_imp->mergeNode);
    if (!_imp->mergeNode) {
        throw std::runtime_error("Rotopaint requires the plug-in " PLUGINID_OFX_MERGE " in order to work");
    }

    {
        // Link the RGBA enabled checkbox of the Rotopaint to the merge output RGBA
        KnobBoolPtr rotoPaintRGBA[4];
        KnobBoolPtr mergeRGBA[4];
        rotoPaintEffect->getEnabledChannelKnobs(&rotoPaintRGBA[0], &rotoPaintRGBA[1], &rotoPaintRGBA[2], &rotoPaintRGBA[3]);
        mergeRGBA[0] = toKnobBool(_imp->mergeNode->getKnobByName(kMergeParamOutputChannelsR));
        mergeRGBA[1] = toKnobBool(_imp->mergeNode->getKnobByName(kMergeParamOutputChannelsG));
        mergeRGBA[2] = toKnobBool(_imp->mergeNode->getKnobByName(kMergeParamOutputChannelsB));
        mergeRGBA[3] = toKnobBool(_imp->mergeNode->getKnobByName(kMergeParamOutputChannelsA));
        for (int i = 0; i < 4; ++i) {
            ignore_result(mergeRGBA[i]->slaveTo(rotoPaintRGBA[i]));
        }



        // Link the compositing operator to this knob
        KnobIPtr mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
        mergeOperatorKnob->slaveTo(_imp->compOperator.lock());

        KnobIPtr thisInvertKnob = _imp->invertKnob.lock();
        if (thisInvertKnob) {
            // Link mask invert knob
            KnobIPtr mergeMaskInvertKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamInvertMask);
            mergeMaskInvertKnob->slaveTo(_imp->invertKnob.lock());
        }

        // Link mix
        KnobIPtr rotoPaintMix = rotoPaintEffect->getNode()->getOrCreateHostMixKnob(rotoPaintEffect->getNode()->getOrCreateMainPage());
        KnobIPtr mergeMix = _imp->mergeNode->getKnobByName(kMergeOFXParamMix);
        mergeMix->slaveTo(rotoPaintMix);
    }

    if ( (type != eRotoStrokeTypeSolid) && (type != eRotoStrokeTypeSmear) ) {
        // Create the mask plug-in

        {
            fixedNamePrefix = baseFixedName;
            fixedNamePrefix.append( QString::fromUtf8("Mask") );
            CreateNodeArgsPtr args(CreateNodeArgs::create( maskPluginID.toStdString(), rotoPaintEffect ));
            args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
            args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
            args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
            args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
            _imp->maskNode = app->createNode(args);
            assert(_imp->maskNode);
            if (!_imp->maskNode) {
                throw std::runtime_error("Rotopaint requires the plug-in " + maskPluginID.toStdString() + " in order to work");
            }


            {
                // For masks set the output components to alpha
                KnobIPtr knob = _imp->maskNode->getKnobByName(kRotoShapeRenderNodeParamOutputComponents);
                assert(knob);
                KnobChoicePtr typeChoice = toKnobChoice(knob);
                assert(typeChoice);
                typeChoice->setValue(1);
            }
        }
    }

    // For drawn masks, the hash depend on the draw bezier/stroke, hence we need to add this item as a child of the RotoShapeRenderNode
    if (_imp->maskNode) {
        setHashParent(_imp->maskNode->getEffectInstance());
    } else if (type == eRotoStrokeTypeSolid || type == eRotoStrokeTypeSmear) {
        setHashParent(_imp->effectNode->getEffectInstance());
    }

    KnobIPtr mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
    assert(mergeOperatorKnob);
    KnobChoicePtr mergeOp = toKnobChoice(mergeOperatorKnob);
    assert(mergeOp);

    KnobChoicePtr compOp = getOperatorKnob();
    MergingFunctionEnum op;
    if ( (type == eRotoStrokeTypeDodge) || (type == eRotoStrokeTypeBurn) ) {
        op = (type == eRotoStrokeTypeDodge ? eMergeColorDodge : eMergeColorBurn);
    } else if (type == eRotoStrokeTypeSolid) {
        op = eMergeOver;
    } else {
        op = eMergeCopy;
    }
    if (mergeOp) {
        mergeOp->setValueFromLabel(Merge::getOperatorString(op));
    }
    compOp->setDefaultValueFromLabel(Merge::getOperatorString(op));

    // Make sure it is not serialized
    compOp->setCurrentDefaultValueAsInitialValue();

    if (isStroke) {
        if (type == eRotoStrokeTypeSmear) {
            KnobDoublePtr spacingKnob = isStroke->getBrushSpacingKnob();
            assert(spacingKnob);
            spacingKnob->setValue(0.05);
        }

        setNodesThreadSafetyForRotopainting();
    }

    ///Attach this stroke to the underlying nodes used
    if (_imp->effectNode) {
        attachStrokeToNode(_imp->effectNode, rotoPaintEffect->getNode(), thisShared);
    }
    if (_imp->maskNode) {
        attachStrokeToNode(_imp->maskNode, rotoPaintEffect->getNode(), thisShared);
    }
    if (_imp->mergeNode) {
        attachStrokeToNode(_imp->mergeNode, rotoPaintEffect->getNode(), thisShared);
    }
    if (_imp->timeOffsetNode) {
        attachStrokeToNode(_imp->timeOffsetNode, rotoPaintEffect->getNode(), thisShared);
    }
    if (_imp->frameHoldNode) {
        attachStrokeToNode(_imp->frameHoldNode, rotoPaintEffect->getNode(), thisShared);
    }


    if (connectNodes) {
        refreshNodesConnections(false);
    }
} // RotoDrawableItem::createNodes


void
RotoDrawableItem::disconnectNodes()
{
    _imp->mergeNode->disconnectInput(0);
    _imp->mergeNode->disconnectInput(1);
    if (_imp->effectNode) {
        _imp->effectNode->disconnectInput(0);
    }
    if (_imp->maskNode) {
        _imp->mergeNode->disconnectInput(_imp->maskNode);
        _imp->maskNode->disconnectInput(0);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->disconnectInput(0);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->disconnectInput(0);
    }
}

void
RotoDrawableItem::deactivateNodes()
{
    if (_imp->effectNode) {
        _imp->effectNode->deactivate(std::list< NodePtr >(), true, false, false, false);
    }
    if (_imp->maskNode) {
        _imp->maskNode->deactivate(std::list< NodePtr >(), true, false, false, false);
    }

    if (_imp->mergeNode) {
        _imp->mergeNode->deactivate(std::list< NodePtr >(), true, false, false, false);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->deactivate(std::list< NodePtr >(), true, false, false, false);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->deactivate(std::list< NodePtr >(), true, false, false, false);
    }
}

void
RotoDrawableItem::activateNodes()
{
    if (_imp->effectNode) {
        _imp->effectNode->activate(std::list< NodePtr >(), false, false);
    }
    _imp->mergeNode->activate(std::list< NodePtr >(), false, false);
    if (_imp->maskNode) {
        _imp->maskNode->activate(std::list< NodePtr >(), false, false);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->activate(std::list< NodePtr >(), false, false);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->activate(std::list< NodePtr >(), false, false);
    }
}

bool
RotoDrawableItem::onKnobValueChanged(const KnobIPtr& knob,
                        ValueChangedReasonEnum reason,
                        double time,
                        ViewSetSpec view,
                        bool originatedFromMainThread)
{
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return false;
    }
    NodePtr node = model->getNode();
    if (!node) {
        return false;
    }

    RotoPaintPtr rotoPaintEffect = toRotoPaint(node->getEffectInstance());


    // Any knob except transform center should break the multi-stroke into a new stroke
    if ( (reason == eValueChangedReasonUserEdited) && (knob->getName() != kRotoBrushCenterParam) && (knob->getName() != kRotoDrawableItemCenterParam)) {
        rotoPaintEffect->onBreakMultiStrokeTriggered();
    }

    if (knob == _imp->compOperator.lock()) {
        ///Since the compositing operator might have changed, we may have to change the rotopaint tree layout
        if (reason == eValueChangedReasonUserEdited) {
            rotoPaintEffect->refreshRotoPaintTree();
        }
    } else if (knob == _imp->invertKnob.lock()) {
        // Since the invert state might have changed, we may have to change the rotopaint tree layout
        if (reason == eValueChangedReasonUserEdited) {
            rotoPaintEffect->refreshRotoPaintTree();
        }
    } else if (knob == _imp->sourceColor.lock()) {
        refreshNodesConnections(rotoPaintEffect->isRotoPaintTreeConcatenatable());
    } else if ( (knob == _imp->timeOffsetMode.lock()) && _imp->timeOffsetNode ) {
        refreshNodesConnections(rotoPaintEffect->isRotoPaintTreeConcatenatable());
    } else {
        return RotoItem::onKnobValueChanged(knob, reason, time, view, originatedFromMainThread);
    }

   
    return true;
} // RotoDrawableItem::onKnobValueChanged



NodePtr
RotoDrawableItem::getEffectNode() const
{
    return _imp->effectNode;
}

NodePtr
RotoDrawableItem::getMergeNode() const
{
    return _imp->mergeNode;
}

NodePtr
RotoDrawableItem::getTimeOffsetNode() const
{
    return _imp->timeOffsetNode;
}


NodePtr
RotoDrawableItem::getMaskNode() const
{
    return _imp->maskNode;
}


NodePtr
RotoDrawableItem::getFrameHoldNode() const
{
    return _imp->frameHoldNode;
}

void
RotoDrawableItem::clearPaintBuffers()
{
    if (_imp->effectNode) {
        _imp->effectNode->setPaintBuffer(ImagePtr());
    }
    if (_imp->maskNode) {
        _imp->maskNode->setPaintBuffer(ImagePtr());
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->setPaintBuffer(ImagePtr());
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->setPaintBuffer(ImagePtr());
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->setPaintBuffer(ImagePtr());
    }

    KnobItemsTablePtr model = getModel();
    NodePtr node;
    if (model) {
        node = model->getNode();
    }
    if (node) {
        std::list<ViewerInstancePtr> viewers;
        node->hasViewersConnected(&viewers);
        for (std::list<ViewerInstancePtr>::iterator it = viewers.begin();
             it != viewers.end();
             ++it) {
            (*it)->clearLastRenderedImage();
        }
    }
}

void
RotoDrawableItem::refreshNodesConnections(bool isTreeConcatenated)
{
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return ;
    }
    NodePtr node = model->getNode();
    if (!node) {
        return;
    }

    RotoPaintPtr rotoPaintNode = toRotoPaint(node->getEffectInstance());
    if (!rotoPaintNode) {
        return;
    }


    RotoDrawableItemPtr previous = boost::dynamic_pointer_cast<RotoDrawableItem>(getNextNonContainerItem());

    NodePtr rotoPaintInput0 = rotoPaintNode->getInternalInputNode(0);

    NodePtr upstreamNode = previous ? previous->getMergeNode() : rotoPaintInput0;
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    RotoStrokeType type;

    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }

    if ( _imp->effectNode &&
         ( type != eRotoStrokeTypeEraser) ) {
        /*
           Tree for this effect goes like this:
                  Effect - <Optional Time Node>------- Reveal input (Reveal/Clone) or Upstream Node otherwise
                /
           Merge
           \--------------------------------------Upstream Node

         */

        NodePtr effectInput;
        if (!_imp->timeOffsetNode) {
            effectInput = _imp->effectNode;
        } else {
            double timeOffsetMode_i = _imp->timeOffsetMode.lock()->getValue();
            if (timeOffsetMode_i == 0) {
                //relative
                effectInput = _imp->timeOffsetNode;
            } else {
                effectInput = _imp->frameHoldNode;
            }
            if (_imp->effectNode->getInput(0) != effectInput) {
                _imp->effectNode->replaceInput(effectInput, 0);
            }
        }

        if (!isTreeConcatenated) {
            /*
             * This case handles: Stroke, Blur, Sharpen, Smear, Clone
             */
            if (_imp->mergeNode->getInput(1) != _imp->effectNode) {
                _imp->mergeNode->replaceInput(_imp->effectNode, 1); // A
            }

            if (_imp->mergeNode->getInput(0) != upstreamNode) {
                _imp->mergeNode->disconnectInput(0);
                if (upstreamNode) {
                    _imp->mergeNode->connectInputBase(upstreamNode, 0); // B
                }
            }



        } else {
            _imp->mergeNode->disconnectInput(0);
            _imp->mergeNode->disconnectInput(1);
            _imp->mergeNode->disconnectInput(2);
        }


        int reveal_i = _imp->sourceColor.lock()->getValue();
        NodePtr revealInput;
        bool shouldUseUpstreamForReveal = true;
        if ( ( (type == eRotoStrokeTypeReveal) ||
               ( type == eRotoStrokeTypeClone) ) && ( reveal_i > 0) ) {
            shouldUseUpstreamForReveal = false;
            revealInput = rotoPaintNode->getInternalInputNode(reveal_i - 1);
        }
        if (!revealInput && shouldUseUpstreamForReveal) {
            if (type != eRotoStrokeTypeSolid) {
                revealInput = upstreamNode;
            }
        }

        if (revealInput) {
            if (effectInput->getInput(0) != revealInput) {
                effectInput->replaceInput(revealInput, 0);
            }
        } else {
            if ( effectInput->getInput(0) ) {
                effectInput->disconnectInput(0);
            }
        }
    } else {
        assert(type == eRotoStrokeTypeEraser ||
               type == eRotoStrokeTypeDodge ||
               type == eRotoStrokeTypeBurn);


        if (type == eRotoStrokeTypeEraser) {
            /*
               Tree for this effect goes like this:
                    Constant or RotoPaint bg input
                    /
               Merge
               \--------------------Upstream Node

             */


            NodePtr eraserInput = rotoPaintInput0 ? rotoPaintInput0 : _imp->effectNode;
            if (_imp->mergeNode->getInput(1) != eraserInput) {
                _imp->mergeNode->disconnectInput(1);
                if (eraserInput) {
                    _imp->mergeNode->connectInputBase(eraserInput, 1); // A
                }
            }


            if (_imp->mergeNode->getInput(0) != upstreamNode) {
                _imp->mergeNode->disconnectInput(0);
                if (upstreamNode) {
                    _imp->mergeNode->connectInputBase(upstreamNode, 0); // B
                }
            }
        }  else if ( (type == eRotoStrokeTypeDodge) || (type == eRotoStrokeTypeBurn) ) {
            /*
               Tree for this effect goes like this:
                            Upstream Node
                            /
               Merge (Dodge/Burn)
                            \Upstream Node

             */


            if (_imp->mergeNode->getInput(1) != upstreamNode) {
                _imp->mergeNode->disconnectInput(1);
                if (upstreamNode) {
                    _imp->mergeNode->connectInputBase(upstreamNode, 1); // A
                }
            }


            if (_imp->mergeNode->getInput(0) != upstreamNode) {
                _imp->mergeNode->disconnectInput(0);
                if (upstreamNode) {
                    _imp->mergeNode->connectInputBase(upstreamNode, 0); // B
                }
            }
        } else {
            //unhandled case
            assert(false);
        }
    } //if (_imp->effectNode &&  type != eRotoStrokeTypeEraser)


    if (_imp->maskNode) {
        if ( _imp->mergeNode->getInput(2) != _imp->maskNode) {
            //Connect the merge node mask to the mask node
            _imp->mergeNode->replaceInput(_imp->maskNode, 2);
        }
    }

} // RotoDrawableItem::refreshNodesConnections

void
RotoDrawableItem::resetNodesThreadSafety()
{
    if (_imp->effectNode) {
        _imp->effectNode->revertToPluginThreadSafety();
    }
    _imp->mergeNode->revertToPluginThreadSafety();
    if (_imp->maskNode) {
        _imp->maskNode->revertToPluginThreadSafety();
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->revertToPluginThreadSafety();
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->revertToPluginThreadSafety();
    }
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return ;
    }
    NodePtr node = model->getNode();
    if (!node) {
        return;
    }
    RotoPaintPtr rotoPaintNode = toRotoPaint(node->getEffectInstance());
    if (!rotoPaintNode) {
        return;
    }
    node->revertToPluginThreadSafety();
}


bool
RotoDrawableItem::isActivated(double time, ViewGetSpec /*view*/) const
{
    if ( !isGloballyActivated() ) {
        return false;
    }
    try {
        int lifetime_i = _imp->lifeTime.lock()->getValue();
        if (lifetime_i == 0) {
            return time == _imp->lifeTimeFrame.lock()->getValue();
        } else if (lifetime_i == 1) {
            return time <= _imp->lifeTimeFrame.lock()->getValue();
        } else if (lifetime_i == 2) {
            return time >= _imp->lifeTimeFrame.lock()->getValue();
        } else {
            return _imp->activated.lock()->getValueAtTime(time);
        }
    } catch (std::runtime_error) {
        return false;
    }
}


void
RotoDrawableItem::getDefaultOverlayColor(double *r, double *g, double *b)
{
    *r = 0.85164;
    *g = 0.196936;
    *b = 0.196936;
}


KnobBoolPtr RotoDrawableItem::getActivatedKnob() const
{
    return _imp->activated.lock();
}

KnobDoublePtr RotoDrawableItem::getOpacityKnob() const
{
    return _imp->opacity.lock();
}

KnobBoolPtr RotoDrawableItem::getInvertedKnob() const
{
    return _imp->invertKnob.lock();
}

KnobChoicePtr RotoDrawableItem::getOperatorKnob() const
{
    return _imp->compOperator.lock();
}

KnobColorPtr RotoDrawableItem::getColorKnob() const
{
    return _imp->color.lock();
}

KnobColorPtr
RotoDrawableItem::getOverlayColorKnob() const
{
    return _imp->overlayColor.lock();
}

KnobIntPtr
RotoDrawableItem::getTimeOffsetKnob() const
{
    return _imp->timeOffset.lock();
}

KnobChoicePtr
RotoDrawableItem::getTimeOffsetModeKnob() const
{
    return _imp->timeOffsetMode.lock();
}

KnobChoicePtr
RotoDrawableItem::getBrushSourceTypeKnob() const
{
    return _imp->sourceColor.lock();
}


KnobDoublePtr
RotoDrawableItem::getCenterKnob() const
{
    return _imp->center.lock();
}

KnobIntPtr
RotoDrawableItem::getLifeTimeFrameKnob() const
{
    return _imp->lifeTimeFrame.lock();
}

KnobDoublePtr
RotoDrawableItem::getBrushSizeKnob() const
{
    return _imp->brushSize.lock();
}

KnobDoublePtr
RotoDrawableItem::getBrushHardnessKnob() const
{
    return _imp->brushHardness.lock();
}

KnobDoublePtr
RotoDrawableItem::getBrushSpacingKnob() const
{
    return _imp->brushSpacing.lock();
}

KnobDoublePtr
RotoDrawableItem::getBrushVisiblePortionKnob() const
{
    return _imp->visiblePortion.lock();
}


void
RotoDrawableItem::setKeyframeOnAllTransformParameters(double time)
{
    KnobDoublePtr translate = _imp->translate.lock();
    translate->setValueAtTime(time, translate->getValue(DimIdx(0)), ViewSetSpec::all(), DimIdx(0));
    translate->setValueAtTime(time, translate->getValue(DimIdx(1)), ViewSetSpec::all(), DimIdx(1));

    KnobDoublePtr scale = _imp->scale.lock();
    scale->setValueAtTime(time, scale->getValue(DimIdx(0)), ViewSetSpec::all(), DimIdx(0));
    scale->setValueAtTime(time, scale->getValue(DimIdx(1)), ViewSetSpec::all(), DimIdx(1));

    KnobDoublePtr rotate = _imp->rotate.lock();
    rotate->setValueAtTime(time, rotate->getValue(DimIdx(0)), ViewSetSpec::all(), DimIdx(0));

    KnobDoublePtr skewX = _imp->skewX.lock();
    KnobDoublePtr skewY = _imp->skewY.lock();
    skewX->setValueAtTime(time, skewX->getValue(DimIdx(0)), ViewSetSpec::all(), DimIdx(0));
    skewY->setValueAtTime(time, skewY->getValue(DimIdx(0)), ViewSetSpec::all(), DimIdx(0));
}


void
RotoDrawableItem::getTransformAtTime(double time,
                                     ViewGetSpec view,
                                     Transform::Matrix3x3* matrix) const
{
    KnobDoublePtr translate = _imp->translate.lock();
    KnobDoublePtr rotate = _imp->rotate.lock();
    KnobBoolPtr scaleUniform = _imp->scaleUniform.lock();
    KnobDoublePtr scale = _imp->scale.lock();
    KnobDoublePtr skewXKnob = _imp->skewX.lock();
    KnobDoublePtr skewYKnob = _imp->skewY.lock();
    KnobDoublePtr centerKnob = _imp->center.lock();
    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    KnobChoicePtr skewOrder = _imp->skewOrder.lock();

    double tx = translate->getValueAtTime(time, DimIdx(0), view);
    double ty = translate->getValueAtTime(time, DimIdx(1), view);
    double sx = scale->getValueAtTime(time, DimIdx(0), view);
    double sy = scaleUniform->getValueAtTime(time) ? sx : scale->getValueAtTime(time, DimIdx(1), view);
    double skewX = skewXKnob->getValueAtTime(time, DimIdx(0), view);
    double skewY = skewYKnob->getValueAtTime(time, DimIdx(0), view);
    double rot = rotate->getValueAtTime(time, DimIdx(0), view);

    rot = Transform::toRadians(rot);
    double centerX = centerKnob->getValueAtTime(time, DimIdx(0), view);
    double centerY = centerKnob->getValueAtTime(time, DimIdx(1), view);
    bool skewOrderYX = skewOrder->getValueAtTime(time) == 1;
    *matrix = Transform::matTransformCanonical(tx, ty, sx, sy, skewX, skewY, skewOrderYX, rot, centerX, centerY);

    Transform::Matrix3x3 extraMat;
    extraMat.a = extraMatrix->getValueAtTime(time, DimIdx(0), view); extraMat.b = extraMatrix->getValueAtTime(time, DimIdx(1), view); extraMat.c = extraMatrix->getValueAtTime(time, DimIdx(2), view);
    extraMat.d = extraMatrix->getValueAtTime(time, DimIdx(3), view); extraMat.e = extraMatrix->getValueAtTime(time, DimIdx(4), view); extraMat.f = extraMatrix->getValueAtTime(time, DimIdx(5), view);
    extraMat.g = extraMatrix->getValueAtTime(time, DimIdx(6), view); extraMat.h = extraMatrix->getValueAtTime(time, DimIdx(7), view); extraMat.i = extraMatrix->getValueAtTime(time, DimIdx(8), view);
    *matrix = Transform::matMul(*matrix, extraMat);
}


void
RotoDrawableItem::setExtraMatrix(bool setKeyframe,
                                 double time,
                                 ViewSetSpec view,
                                 const Transform::Matrix3x3& mat)
{
    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    extraMatrix->beginChanges();
    if (setKeyframe) {
        std::vector<double> matValues(9);
        memcpy(&matValues[0], &mat.a, 9 * sizeof(double));
        extraMatrix->setValueAtTime(time, mat.a, view, DimIdx(0)); extraMatrix->setValueAtTime(time, mat.b, view, DimIdx(1)); extraMatrix->setValueAtTime(time, mat.c, view, DimIdx(2));
        extraMatrix->setValueAtTime(time, mat.d, view, DimIdx(3)); extraMatrix->setValueAtTime(time, mat.e, view, DimIdx(4)); extraMatrix->setValueAtTime(time, mat.f, view, DimIdx(5));
        extraMatrix->setValueAtTime(time, mat.g, view, DimIdx(6)); extraMatrix->setValueAtTime(time, mat.h, view, DimIdx(7)); extraMatrix->setValueAtTime(time, mat.i, view, DimIdx(8));
    } else {
        extraMatrix->setValue(mat.a, view, DimIdx(0)); extraMatrix->setValue(mat.b, view, DimIdx(1)); extraMatrix->setValue(mat.c, view, DimIdx(2));
        extraMatrix->setValue(mat.d, view, DimIdx(3)); extraMatrix->setValue(mat.e, view, DimIdx(4)); extraMatrix->setValue(mat.f, view, DimIdx(5));
        extraMatrix->setValue(mat.g, view, DimIdx(6)); extraMatrix->setValue(mat.h, view, DimIdx(7)); extraMatrix->setValue(mat.i, view, DimIdx(8));
    }
    extraMatrix->endChanges();
}

void
RotoDrawableItem::resetTransformCenter()
{
    double time = getApp()->getTimeLine()->currentFrame();
    RectD bbox =  getBoundingBox(time, ViewIdx(0));
    KnobDoublePtr centerKnob = _imp->center.lock();

    centerKnob->beginChanges();

    //centerKnob->unSlave(DimSpec::all(), ViewSetSpec::all(), false);
    centerKnob->removeAnimation(ViewSetSpec::all(), DimSpec::all());
    
    std::vector<double> values(2);
    values[0] = (bbox.x1 + bbox.x2) / 2.;
    values[1] = (bbox.y1 + bbox.y2) / 2.;
    centerKnob->setValueAcrossDimensions(values);
    /*KnobIPtr masterKnob = linkData.masterKnob.lock();
    if (masterKnob) {
        centerKnob->slaveTo(masterKnob, DimSpec::all(), DimSpec::all(), view, view);
    }*/
    centerKnob->endChanges();
}


void
RotoDrawableItem::onItemRemovedFromModel()
{
    // Disconnect this item nodes from the other nodes in the rotopaint tree
    disconnectNodes();
}

void
RotoDrawableItem::initializeKnobs()
{

    RotoItem::initializeKnobs();
    
    KnobHolderPtr thisShared = shared_from_this();
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    Bezier* isBezier = dynamic_cast<Bezier*>(this);
    RotoStrokeType type = isStroke ? isStroke->getBrushType() : eRotoStrokeTypeSolid;
    // General
    _imp->opacity = createDuplicateOfTableKnob<KnobDouble>(kRotoOpacityParam);
    _imp->lifeTime = createDuplicateOfTableKnob<KnobChoice>(kRotoDrawableItemLifeTimeParam);
    _imp->lifeTimeFrame = createDuplicateOfTableKnob<KnobInt>(kRotoDrawableItemLifeTimeFrameParam);
    _imp->activated = createDuplicateOfTableKnob<KnobBool>(kRotoActivatedParam);
    {
        KnobColorPtr param = AppManager::createKnob<KnobColor>(thisShared, tr(kRotoOverlayColorLabel), 4);
        param->setHintToolTip( tr(kRotoOverlayColorHint) );
        param->setName(kRotoOverlayColor);
        std::vector<double> def(4);
        getDefaultOverlayColor(&def[0], &def[1], &def[2]);
        def[3] = 1.;
        param->setDefaultValues(def, DimIdx(0));
        _imp->overlayColor = param;
    }
    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(thisShared, tr(kRotoCompOperatorParamLabel));
        param->setHintToolTip( tr(kRotoCompOperatorHint) );
        param->setName(kRotoCompOperatorParam);
        std::vector<std::string> operators;
        std::vector<std::string> tooltips;
        Merge::getOperatorStrings(&operators, &tooltips);
        param->populateChoices(operators, tooltips);
        param->setDefaultValueFromLabel( Merge::getOperatorString(eMergeCopy) );
        _imp->compOperator = param;
    }

    // Item types that output a mask may not have an invert parameter
    if (type != eRotoStrokeTypeSolid &&
        type != eRotoStrokeTypeSmear) {
        _imp->invertKnob = createDuplicateOfTableKnob<KnobBool>(kRotoInvertedParam);
    }

    // Color is only useful for solids
    if (type == eRotoStrokeTypeSolid) {
        _imp->color = createDuplicateOfTableKnob<KnobColor>(kRotoColorParam);
    }



    // Brush: only for strokes or open beziers
    if (isStroke || (isBezier && isBezier->isOpenBezier())) {
        _imp->brushSize = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushSizeParam);
        _imp->brushSpacing = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushSpacingParam);
        _imp->brushHardness = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushHardnessParam);
        _imp->visiblePortion = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushVisiblePortionParam);
    }
  

    // The comp item doesn't have a vector graphics mask hence cannot have a transform on it
    if (type != eRotoStrokeTypeComp) {
        // Transform
        _imp->translate = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemTranslateParam);
        _imp->rotate = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemRotateParam);
        _imp->scale = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemScaleParam);
        _imp->scaleUniform = createDuplicateOfTableKnob<KnobBool>(kRotoDrawableItemScaleUniformParam);
        _imp->skewX = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemSkewXParam);
        _imp->skewY = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemSkewYParam);
        _imp->skewOrder = createDuplicateOfTableKnob<KnobChoice>(kRotoDrawableItemSkewOrderParam);
        _imp->center = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemCenterParam);
        _imp->extraMatrix = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemExtraMatrixParam);
    }


    if (type == eRotoStrokeTypeReveal ||
        type == eRotoStrokeTypeClone ||
        type == eRotoStrokeTypeComp) {
        // Source control
        _imp->sourceColor = createDuplicateOfTableKnob<KnobChoice>(kRotoBrushSourceColor);
        _imp->timeOffset = createDuplicateOfTableKnob<KnobInt>(kRotoBrushTimeOffsetParam);
        _imp->timeOffsetMode = createDuplicateOfTableKnob<KnobChoice>(kRotoBrushTimeOffsetModeParam);
    }


    createNodes();
} // initializeKnobs

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_RotoDrawableItem.cpp"
