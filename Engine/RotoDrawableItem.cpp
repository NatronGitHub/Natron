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
#include "Engine/RotoContextPrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/BezierCP.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Interpolation.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoShapeRenderNode.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/RotoDrawableItemSerialization.h"
#include "Serialization/NodeSerialization.h"



NATRON_NAMESPACE_ENTER;


////////////////////////////////////RotoDrawableItem////////////////////////////////////

RotoDrawableItem::RotoDrawableItem(const RotoContextPtr& context,
                                   const std::string & name,
                                   const RotoLayerPtr& parent)
    : RotoItem(context, name, parent)
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

    getContext()->getNode()->setRenderThreadSafety(eRenderSafetyInstanceSafe);
    getContext()->setWhileCreatingPaintStrokeOnMergeNodes(true);
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

    initializeKnobsPublic();

    RotoContextPtr context = getContext();
    RotoPaintPtr rotoPaintEffect = toRotoPaint(context->getNode()->getEffectInstance());
    AppInstancePtr app = rotoPaintEffect->getApp();
    QString fixedNamePrefix = QString::fromUtf8( rotoPaintEffect->getNode()->getScriptName_mt_safe().c_str() );
    fixedNamePrefix.append( QLatin1Char('_') );
    fixedNamePrefix.append( QString::fromUtf8( getScriptName().c_str() ) );

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
        //todo
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
            // Link effect knob to size
            KnobIPtr knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
            knob->slaveTo(_imp->effectStrength.lock());

        } else if ( (type == eRotoStrokeTypeClone) || (type == eRotoStrokeTypeReveal) ) {
            // Link transform knobs
            KnobIPtr translateKnob = _imp->effectNode->getKnobByName(kTransformParamTranslate);
            translateKnob->slaveTo(_imp->cloneTranslate.lock());

            KnobIPtr rotateKnob = _imp->effectNode->getKnobByName(kTransformParamRotate);
            rotateKnob->slaveTo(_imp->cloneRotate.lock());

            KnobIPtr scaleKnob = _imp->effectNode->getKnobByName(kTransformParamScale);
            scaleKnob->slaveTo(_imp->cloneScale.lock());

            KnobIPtr uniformKnob = _imp->effectNode->getKnobByName(kTransformParamUniform);
            uniformKnob->slaveTo(_imp->cloneScaleUniform.lock());

            KnobIPtr skewxKnob = _imp->effectNode->getKnobByName(kTransformParamSkewX);
            skewxKnob->slaveTo(_imp->cloneSkewX.lock());

            KnobIPtr skewyKnob = _imp->effectNode->getKnobByName(kTransformParamSkewY);
            skewyKnob->slaveTo(_imp->cloneSkewY.lock());

            KnobIPtr skewOrderKnob = _imp->effectNode->getKnobByName(kTransformParamSkewOrder);
            skewOrderKnob->slaveTo(_imp->cloneSkewOrder.lock());

            KnobIPtr centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
            centerKnob->slaveTo(_imp->cloneCenter.lock());

            KnobIPtr filterKnob = _imp->effectNode->getKnobByName(kTransformParamFilter);
            filterKnob->slaveTo(_imp->cloneFilter.lock());

            KnobIPtr boKnob = _imp->effectNode->getKnobByName(kTransformParamBlackOutside);
            boKnob->slaveTo(_imp->cloneBlackOutside.lock());


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

static RotoDrawableItemPtr
findPreviousOfItemInLayer(const RotoLayerPtr& layer,
                          const RotoItemPtr& item)
{
    RotoItems layerItems = layer->getItems_mt_safe();

    if ( layerItems.empty() ) {
        return RotoDrawableItemPtr();
    }
    RotoItems::iterator found = layerItems.end();
    if (item) {
        for (RotoItems::iterator it = layerItems.begin(); it != layerItems.end(); ++it) {
            if (*it == item) {
                found = it;
                break;
            }
        }
        assert( found != layerItems.end() );
    } else {
        found = layerItems.end();
    }

    if ( found != layerItems.end() ) {
        ++found;
        for (; found != layerItems.end(); ++found) {
            //We found another stroke below at the same level
            RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*found);
            if (isDrawable) {
                assert(isDrawable != item);

                return isDrawable;
            }

            //Cycle through a layer that is at the same level
            RotoLayerPtr isLayer = toRotoLayer(*found);
            if (isLayer) {
                RotoDrawableItemPtr si = findPreviousOfItemInLayer(isLayer, RotoItemPtr());
                if (si) {
                    assert(si != item);

                    return si;
                }
            }
        }
    }

    //Item was still not found, find in great parent layer
    RotoLayerPtr parentLayer = layer->getParentLayer();
    if (!parentLayer) {
        return RotoDrawableItemPtr();
    }
    RotoItems greatParentItems = parentLayer->getItems_mt_safe();

    found = greatParentItems.end();
    for (RotoItems::iterator it = greatParentItems.begin(); it != greatParentItems.end(); ++it) {
        if (*it == layer) {
            found = it;
            break;
        }
    }
    assert( found != greatParentItems.end() );
    RotoDrawableItemPtr ret = findPreviousOfItemInLayer(parentLayer, layer);
    assert(ret != item);

    return ret;
} // findPreviousOfItemInLayer

RotoDrawableItemPtr
RotoDrawableItem::findPreviousInHierarchy()
{
    RotoLayerPtr layer = getParentLayer();

    if (!layer) {
        return RotoDrawableItemPtr();
    }

    return findPreviousOfItemInLayer(layer, toRotoItem(shared_from_this()));
}


bool
RotoDrawableItem::onKnobValueChanged(const KnobIPtr& knob,
                        ValueChangedReasonEnum reason,
                        double /*time*/,
                        ViewSetSpec /*view*/,
                        bool /*originatedFromMainThread*/)
{
    // Any knob except transform center should break the multi-stroke into a new stroke
    if ( (reason == eValueChangedReasonUserEdited) && (knob != _imp->center.lock()) && (knob != _imp->cloneCenter.lock()) ) {
        getContext()->s_breakMultiStroke();
    }

    if (knob == _imp->compOperator.lock()) {
        ///Since the compositing operator might have changed, we may have to change the rotopaint tree layout
        if (reason == eValueChangedReasonUserEdited) {
            getContext()->refreshRotoPaintTree();
        }
    }
#ifdef NATRON_ROTO_INVERTIBLE
    else if (knob == _imp->invertKnob.lock()) {
        // Since the invert state might have changed, we may have to change the rotopaint tree layout
        if (reason == eValueChangedReasonUserEdited) {
            getContext()->refreshRotoPaintTree();
        }
    }
#endif
    else if (knob == _imp->sourceColor.lock()) {
        refreshNodesConnections(getContext()->canConcatenatedRotoPaintTree());
    } else if ( (knob == _imp->timeOffsetMode.lock()) && _imp->timeOffsetNode ) {
        refreshNodesConnections(getContext()->canConcatenatedRotoPaintTree());
    }

    if (isOverlaySlaveParam(knob)) {
        redrawOverlayInteract();
    }
    return true;
} // RotoDrawableItem::onKnobValueChanged

void
RotoDrawableItem::onSignificantEvaluateAboutToBeCalled(const KnobIPtr& knob, ValueChangedReasonEnum reason, DimSpec dimension, double time, ViewSetSpec view)
{

    if (knob) {
        knob->invalidateHashCache();
    }
    
    invalidateHashCache();
    Q_UNUSED(reason);
    Q_UNUSED(dimension);
    Q_UNUSED(time);
    Q_UNUSED(view);
    /*// Call invalidate hash on the mask and effect, this will recurse below on all nodes in the group which is enough to invalidate the hash;
    if (_imp->maskNode) {
        _imp->maskNode->getEffectInstance()->onSignificantEvaluateAboutToBeCalled(KnobIPtr(), reason, dimension, time, view);
    }
    if (_imp->effectNode) {
        _imp->effectNode->getEffectInstance()->onSignificantEvaluateAboutToBeCalled(KnobIPtr(), reason, dimension, time, view);
    }*/
}

void
RotoDrawableItem::evaluate(bool isSignificant, bool refreshMetadatas)
{
    if (!_imp->mergeNode) {
        return;
    }
    // Call evaluate on the merge node
    // If the tree is "concatenated", the merge node will not be used hence call it on the RotoPaint node itself
    if (_imp->mergeNode->getOutputs().empty()) {
        getContext()->getNode()->getEffectInstance()->evaluate(isSignificant, refreshMetadatas);
    } else {
        _imp->mergeNode->getEffectInstance()->evaluate(isSignificant, refreshMetadatas);
    }
}



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
}

void
RotoDrawableItem::refreshNodesConnections(bool isTreeConcatenated)
{
    RotoDrawableItemPtr previous = findPreviousInHierarchy();

    RotoPaintPtr rotoPaintNode = boost::dynamic_pointer_cast<RotoPaint>(getContext()->getNode()->getEffectInstance());
    assert(rotoPaintNode);

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
    getContext()->getNode()->revertToPluginThreadSafety();
}

void
RotoDrawableItem::clone(const RotoItem* other)
{
    const RotoDrawableItem* otherDrawable = dynamic_cast<const RotoDrawableItem*>(other);

    if (!otherDrawable) {
        return;
    }
    const KnobsVec& otherKnobs = otherDrawable->getKnobs();
    const KnobsVec& knobs = getKnobs();
    if (knobs.size() != otherKnobs.size()) {
        return;
    }
    KnobsVec::const_iterator it = knobs.begin();
    KnobsVec::const_iterator otherIt = otherKnobs.begin();
    for (; it != knobs.end(); ++it, ++otherIt) {
        (*it)->copyKnob(*otherIt);
    }
    RotoItem::clone(other);
}

static void
serializeRotoKnob(const KnobIPtr & knob,
                 SERIALIZATION_NAMESPACE::KnobSerializationPtr* serialization)
{
    /*std::pair<int, KnobIPtr > master = knob->getMaster(0);
    if (master.second) {
        master.second->toSerialization(serialization->get());
    } else {*/
        knob->toSerialization(serialization->get());
    //}
}

void
RotoDrawableItem::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{

    SERIALIZATION_NAMESPACE::RotoDrawableItemSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::RotoDrawableItemSerialization*>(obj);
    if (!s) {
        return;
    }

    KnobsVec knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        SERIALIZATION_NAMESPACE::KnobSerializationPtr k(new SERIALIZATION_NAMESPACE::KnobSerialization);
        serializeRotoKnob( *it, &k );
        if (k->_mustSerialize) {
            s->_knobs.push_back(k);
        }
    }
    RotoItem::toSerialization(obj);
}

void
RotoDrawableItem::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::RotoDrawableItemSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::RotoDrawableItemSerialization*>(&obj);
    if (!s) {
        return;
    }

    RotoItem::fromSerialization(obj);

    const KnobsVec& knobs = getKnobs();
    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = s->_knobs.begin(); it != s->_knobs.end(); ++it) {
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ( (*it2)->getName() == (*it)->getName() ) {
                boost::shared_ptr<KnobSignalSlotHandler> slot = (*it2)->getSignalSlotHandler();
                slot->blockSignals(true);
                (*it2)->fromSerialization(**it);
                slot->blockSignals(false);
                break;
            }
        }
    }

} // RotoDrawableItem::load

bool
RotoDrawableItem::isActivated(double time) const
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
RotoDrawableItem::setActivated(bool a,
                               double time)
{
    _imp->activated.lock()->setValueAtTime(time, a);
}

double
RotoDrawableItem::getOpacity(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->opacity.lock()->getValueAtTime(time);
}

void
RotoDrawableItem::setOpacity(double o,
                             double time)
{
    _imp->opacity.lock()->setValueAtTime(time, o);
}

double
RotoDrawableItem::getFeatherDistance(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->feather.lock()->getValueAtTime(time);
}

void
RotoDrawableItem::setFeatherDistance(double d,
                                     double time)
{
    _imp->feather.lock()->setValueAtTime(time, d);
}

int
RotoDrawableItem::getNumKeyframesFeatherDistance() const
{
    return _imp->feather.lock()->getKeyFramesCount(ViewGetSpec::current(), DimIdx(0));
}

void
RotoDrawableItem::setFeatherFallOff(double f,
                                    double time)
{
    _imp->featherFallOff.lock()->setValueAtTime(time, f);
}

double
RotoDrawableItem::getFeatherFallOff(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->featherFallOff.lock()->getValueAtTime(time);
}

bool
RotoDrawableItem::getInverted(double time) const
{
    ///MT-safe thanks to Knob
#ifdef NATRON_ROTO_INVERTIBLE

    return _imp->inverted->getValueAtTime(time);
#else
    Q_UNUSED(time);

    return false;
#endif
}

void
RotoDrawableItem::getColor(double time,
                           double* color) const
{
    KnobColorPtr colorKnob = _imp->color.lock();
    color[0] = colorKnob->getValueAtTime(time, DimIdx(0));
    color[1] = colorKnob->getValueAtTime(time, DimIdx(1));
    color[2] = colorKnob->getValueAtTime(time, DimIdx(2));
}

void
RotoDrawableItem::setColor(double time,
                           double r,
                           double g,
                           double b)
{
    KnobColorPtr colorKnob = _imp->color.lock();
    colorKnob->setValueAtTime(time, r, ViewSetSpec::all(), DimIdx(0));
    colorKnob->setValueAtTime(time, g, ViewSetSpec::all(), DimIdx(1));
    colorKnob->setValueAtTime(time, b, ViewSetSpec::all(), DimIdx(2));
}

int
RotoDrawableItem::getCompositingOperator() const
{
    return _imp->compOperator.lock()->getValue();
}

void
RotoDrawableItem::setCompositingOperator(int op)
{
    _imp->compOperator.lock()->setValue( op);
}

std::string
RotoDrawableItem::getCompositingOperatorToolTip() const
{
    return _imp->compOperator.lock()->getHintToolTipFull();
}

void
RotoDrawableItem::getDefaultOverlayColor(double *r, double *g, double *b)
{
    *r = 0.85164;
    *g = 0.196936;
    *b = 0.196936;
}

void
RotoDrawableItem::getOverlayColor(double* color) const
{
    KnobColorPtr knob = _imp->overlayColor.lock();
    color[0] = knob->getValue(DimIdx(0));
    color[1] = knob->getValue(DimIdx(1));
    color[2] = knob->getValue(DimIdx(2));
    color[3] = knob->getValue(DimIdx(3));
}

void
RotoDrawableItem::setOverlayColor(const double *color)
{

    std::vector<double> values(4);
    for (int i = 0; i < 4; ++i) {
        values[i] = color[i];
    }
    _imp->overlayColor.lock()->setValueAcrossDimensions(values);
}

KnobBoolPtr RotoDrawableItem::getActivatedKnob() const
{
    return _imp->activated.lock();
}

KnobDoublePtr RotoDrawableItem::getFeatherKnob() const
{
    return _imp->feather.lock();
}

KnobDoublePtr RotoDrawableItem::getFeatherFallOffKnob() const
{
    return _imp->featherFallOff.lock();
}

KnobDoublePtr RotoDrawableItem::getOpacityKnob() const
{
    return _imp->opacity.lock();
}

KnobBoolPtr RotoDrawableItem::getInvertedKnob() const
{
#ifdef NATRON_ROTO_INVERTIBLE

    return _imp->inverted.lock();
#else

    return KnobBoolPtr();
#endif
}

KnobChoicePtr RotoDrawableItem::getOperatorKnob() const
{
    return _imp->compOperator.lock();
}

KnobColorPtr RotoDrawableItem::getColorKnob() const
{
    return _imp->color.lock();
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
RotoDrawableItem::getBrushEffectKnob() const
{
    return _imp->effectStrength.lock();
}

KnobDoublePtr
RotoDrawableItem::getBrushVisiblePortionKnob() const
{
    return _imp->visiblePortion.lock();
}

KnobBoolPtr
RotoDrawableItem::getPressureOpacityKnob() const
{
    return _imp->pressureOpacity.lock();
}

KnobBoolPtr
RotoDrawableItem::getPressureSizeKnob() const
{
    return _imp->pressureSize.lock();
}

KnobBoolPtr
RotoDrawableItem::getPressureHardnessKnob() const
{
    return _imp->pressureHardness.lock();
}

KnobBoolPtr
RotoDrawableItem::getBuildupKnob() const
{
    return _imp->buildUp.lock();
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
RotoDrawableItem::getBrushCloneTranslateKnob() const
{
    return _imp->cloneTranslate.lock();
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

KnobChoicePtr
RotoDrawableItem::getFallOffRampTypeKnob() const
{
    return _imp->fallOffRampType.lock();
}


KnobDoublePtr
RotoDrawableItem::getMotionBlurAmountKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    return _imp->motionBlur;
#else

    return KnobDoublePtr();
#endif
}

KnobDoublePtr
RotoDrawableItem::getShutterOffsetKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    return _imp->customOffset.lock();
#else

    return KnobDoublePtr();
#endif
}

KnobDoublePtr
RotoDrawableItem::getShutterKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    return _imp->shutter.lock();
#else

    return KnobDoublePtr();
#endif
}

KnobChoicePtr
RotoDrawableItem::getShutterTypeKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    return _imp->shutterType.lock();
#else

    return KnobChoicePtr();
#endif
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

    double tx = translate->getValueAtTime(time, DimIdx(0));
    double ty = translate->getValueAtTime(time, DimIdx(1));
    double sx = scale->getValueAtTime(time, DimIdx(0));
    double sy = scaleUniform->getValueAtTime(time) ? sx : scale->getValueAtTime(time, DimIdx(1));
    double skewX = skewXKnob->getValueAtTime(time, DimIdx(0));
    double skewY = skewYKnob->getValueAtTime(time, DimIdx(0));
    double rot = rotate->getValueAtTime(time, DimIdx(0));

    rot = Transform::toRadians(rot);
    double centerX = centerKnob->getValueAtTime(time, DimIdx(0));
    double centerY = centerKnob->getValueAtTime(time, DimIdx(1));
    bool skewOrderYX = skewOrder->getValueAtTime(time) == 1;
    *matrix = Transform::matTransformCanonical(tx, ty, sx, sy, skewX, skewY, skewOrderYX, rot, centerX, centerY);

    Transform::Matrix3x3 extraMat;
    extraMat.a = extraMatrix->getValueAtTime(time, DimIdx(0)); extraMat.b = extraMatrix->getValueAtTime(time, DimIdx(1)); extraMat.c = extraMatrix->getValueAtTime(time, DimIdx(2));
    extraMat.d = extraMatrix->getValueAtTime(time, DimIdx(3)); extraMat.e = extraMatrix->getValueAtTime(time, DimIdx(4)); extraMat.f = extraMatrix->getValueAtTime(time, DimIdx(5));
    extraMat.g = extraMatrix->getValueAtTime(time, DimIdx(6)); extraMat.h = extraMatrix->getValueAtTime(time, DimIdx(7)); extraMat.i = extraMatrix->getValueAtTime(time, DimIdx(8));
    *matrix = Transform::matMul(*matrix, extraMat);
}

/**
 * @brief Set the transform at the given time
 **/
void
RotoDrawableItem::setTransform(double time,
                               double tx,
                               double ty,
                               double sx,
                               double sy,
                               double centerX,
                               double centerY,
                               double rot,
                               double skewX,
                               double skewY)
{
    KnobDoublePtr translate = _imp->translate.lock();
    KnobDoublePtr rotate = _imp->rotate.lock();
    KnobBoolPtr scaleUniform = _imp->scaleUniform.lock();
    KnobDoublePtr scale = _imp->scale.lock();
    KnobDoublePtr skewXKnob = _imp->skewX.lock();
    KnobDoublePtr skewYKnob = _imp->skewY.lock();
    KnobDoublePtr centerKnob = _imp->center.lock();
    KnobChoicePtr skewOrder = _imp->skewOrder.lock();

    bool autoKeying = getContext()->isAutoKeyingEnabled();

    if (autoKeying) {
        translate->setValueAtTime(time, tx, ViewSetSpec::all(), DimIdx(0));
        translate->setValueAtTime(time, ty, ViewSetSpec::all(), DimIdx(1));

        scale->setValueAtTime(time, sx, ViewSetSpec::all(), DimIdx(0));
        scale->setValueAtTime(time, sy, ViewSetSpec::all(), DimIdx(1));

        centerKnob->setValueAtTime(time, centerX, ViewSetSpec::all(), DimIdx(0));
        centerKnob->setValueAtTime(time, centerY, ViewSetSpec::all(), DimIdx(1));

        rotate->setValueAtTime(time, rot, ViewSetSpec::all(), DimIdx(0));

        skewXKnob->setValueAtTime(time, skewX, ViewSetSpec::all(), DimIdx(0));
        skewYKnob->setValueAtTime(time, skewY, ViewSetSpec::all(), DimIdx(0));
    } else {
        translate->setValue(tx, ViewSetSpec::all(), DimIdx(0));
        translate->setValue(ty, ViewSetSpec::all(), DimIdx(1));

        scale->setValue(sx, ViewSetSpec::all(), DimIdx(0));
        scale->setValue(sy, ViewSetSpec::all(), DimIdx(1));

        centerKnob->setValue(centerX, ViewSetSpec::all(), DimIdx(0));
        centerKnob->setValue(centerY, ViewSetSpec::all(), DimIdx(1));

        rotate->setValue(rot, ViewSetSpec::all(), DimIdx(0));

        skewXKnob->setValue(skewX, ViewSetSpec::all(), DimIdx(0));
        skewYKnob->setValue(skewY, ViewSetSpec::all(), DimIdx(0));
    }

    onTransformSet(time);
}

void
RotoDrawableItem::setExtraMatrix(bool setKeyframe,
                                 double time,
                                 const Transform::Matrix3x3& mat)
{
    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    extraMatrix->beginChanges();
    if (setKeyframe) {
        extraMatrix->setValueAtTime(time, mat.a, ViewSetSpec::all(), DimIdx(0)); extraMatrix->setValueAtTime(time, mat.b, ViewSetSpec::all(), DimIdx(1)); extraMatrix->setValueAtTime(time, mat.c, ViewSetSpec::all(), DimIdx(2));
        extraMatrix->setValueAtTime(time, mat.d, ViewSetSpec::all(), DimIdx(3)); extraMatrix->setValueAtTime(time, mat.e, ViewSetSpec::all(), DimIdx(4)); extraMatrix->setValueAtTime(time, mat.f, ViewSetSpec::all(), DimIdx(5));
        extraMatrix->setValueAtTime(time, mat.g, ViewSetSpec::all(), DimIdx(6)); extraMatrix->setValueAtTime(time, mat.h, ViewSetSpec::all(), DimIdx(7)); extraMatrix->setValueAtTime(time, mat.i, ViewSetSpec::all(), DimIdx(8));
    } else {
        extraMatrix->setValue(mat.a, ViewSetSpec::all(), DimIdx(0)); extraMatrix->setValue(mat.b, ViewSetSpec::all(), DimIdx(1)); extraMatrix->setValue(mat.c, ViewSetSpec::all(), DimIdx(2));
        extraMatrix->setValue(mat.d, ViewSetSpec::all(), DimIdx(3)); extraMatrix->setValue(mat.e, ViewSetSpec::all(), DimIdx(4)); extraMatrix->setValue(mat.f, ViewSetSpec::all(), DimIdx(5));
        extraMatrix->setValue(mat.g, ViewSetSpec::all(), DimIdx(6)); extraMatrix->setValue(mat.h, ViewSetSpec::all(), DimIdx(7)); extraMatrix->setValue(mat.i, ViewSetSpec::all(), DimIdx(8));
    }
    extraMatrix->endChanges();
}

void
RotoDrawableItem::resetTransformCenter()
{
    double time = getContext()->getNode()->getApp()->getTimeLine()->currentFrame();
    RectD bbox =  getBoundingBox(time);
    KnobDoublePtr centerKnob = _imp->center.lock();

    centerKnob->beginChanges();

    MasterKnobLink linkData;
    if (centerKnob->getMaster(DimIdx(0), ViewIdx(0), &linkData)) {
        centerKnob->unSlave(DimSpec::all(), ViewSetSpec::all(), false);
    }
    centerKnob->removeAnimation(ViewSetSpec::all(), DimSpec::all());
    std::vector<double> values(2);
    values[0] = (bbox.x1 + bbox.x2) / 2.;
    values[1] = (bbox.y1 + bbox.y2) / 2.;
    centerKnob->setValueAcrossDimensions(values);
    KnobIPtr masterKnob = linkData.masterKnob.lock();
    if (masterKnob) {
        centerKnob->slaveTo(masterKnob);
    }
    centerKnob->endChanges();
}

void
RotoDrawableItem::appendToHash(double time, ViewIdx view, Hash64* hash)
{
    KnobHolder::appendToHash(time, view, hash);
    hash->append(isGloballyActivated());
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
    _imp->overlayColor = createDuplicateOfTableKnob<KnobColor>(kRotoOverlayColor);
    _imp->compOperator = createDuplicateOfTableKnob<KnobChoice>(kRotoCompOperatorParam);

    // Item types that output a mask may not have an invert parameter
    if (type != eRotoStrokeTypeSolid &&
        type != eRotoStrokeTypeSmear) {
        _imp->inverted = createDuplicateOfTableKnob<KnobBool>(kRotoInvertedParam);
    }

    // Color is only useful for solids
    if (type == eRotoStrokeTypeSolid) {
        _imp->color = createDuplicateOfTableKnob<KnobColor>(kRotoColorParam);
    }


    // Shape
    if (isBezier) {
        _imp->feather = createDuplicateOfTableKnob<KnobDouble>(kRotoFeatherParam);
        _imp->featherFallOff = createDuplicateOfTableKnob<KnobDouble>(kRotoFeatherFallOffParam);
        _imp->fallOffRampType = createDuplicateOfTableKnob<KnobChoice>(kRotoFeatherFallOffType);
    }


    // The comp item doesn't have a vector data mask hence cannot have a transform on it
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
    if (isStroke) {

            // Brush
        _imp->brushSize = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushSizeParam);
        _imp->brushSpacing = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushSpacingParam);
        _imp->brushHardness = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushHardnessParam);

        // Make a strength parameter when relevant
        if (type == eRotoStrokeTypeBlur ||
            type == eRotoStrokeTypeSharpen) {
            _imp->effectStrength = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushEffectParam);
        }

        _imp->pressureSize = createDuplicateOfTableKnob<KnobBool>(kRotoBrushPressureSizeParam);
        _imp->pressureOpacity = createDuplicateOfTableKnob<KnobBool>(kRotoBrushPressureOpacityParam);
        _imp->pressureHardness = createDuplicateOfTableKnob<KnobBool>(kRotoBrushPressureHardnessParam);
        _imp->buildUp = createDuplicateOfTableKnob<KnobBool>(kRotoBrushBuildupParam);
        _imp->visiblePortion = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushVisiblePortionParam);

        // Clone transform
        _imp->cloneTranslate = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushTranslateParam);
        _imp->cloneRotate = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushRotateParam);
        _imp->cloneScale = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushScaleParam);
        _imp->cloneScaleUniform = createDuplicateOfTableKnob<KnobBool>(kRotoBrushScaleUniformParam);
        _imp->cloneSkewX = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushSkewXParam);
        _imp->cloneSkewY = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushSkewYParam);
        _imp->cloneSkewOrder = createDuplicateOfTableKnob<KnobChoice>(kRotoBrushSkewOrderParam);
        _imp->cloneCenter = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushCenterParam);
        _imp->cloneFilter = createDuplicateOfTableKnob<KnobChoice>(kRotoBrushFilterParam);
        _imp->cloneBlackOutside = createDuplicateOfTableKnob<KnobBool>(kRotoBrushBlackOutsideParam);

    } // isStroke


    if (type == eRotoStrokeTypeReveal ||
        type == eRotoStrokeTypeClone ||
        type == eRotoStrokeTypeComp) {
        // Source control
        _imp->sourceColor = createDuplicateOfTableKnob<KnobChoice>(kRotoBrushSourceColor);
        _imp->timeOffset = createDuplicateOfTableKnob<KnobInt>(kRotoBrushTimeOffsetParam);
        _imp->timeOffsetMode = createDuplicateOfTableKnob<KnobChoice>(kRotoBrushTimeOffsetModeParam);
    }

    // Motion-blur
    if (isBezier) {
        _imp->motionBlur = createDuplicateOfTableKnob<KnobDouble>(kRotoPerShapeMotionBlurParam);
        _imp->shutter = createDuplicateOfTableKnob<KnobDouble>(kRotoPerShapeShutterParam);
        _imp->shutterType = createDuplicateOfTableKnob<KnobChoice>(kRotoPerShapeShutterOffsetTypeParam);
        _imp->customOffset = createDuplicateOfTableKnob<KnobDouble>(kRotoPerShapeShutterCustomOffsetParam);
    }
    
} // initializeKnobs

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_RotoDrawableItem.cpp"
