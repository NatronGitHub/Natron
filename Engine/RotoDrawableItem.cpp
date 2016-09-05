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
#include "Engine/RotoShapeRenderNode.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/RotoDrawableItemSerialization.h"
#include "Serialization/NodeSerialization.h"

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


////////////////////////////////////RotoDrawableItem////////////////////////////////////

RotoDrawableItem::RotoDrawableItem(const RotoContextPtr& context,
                                   const std::string & name,
                                   const RotoLayerPtr& parent,
                                   bool isStroke)
    : RotoItem(context, name, parent)
    , _imp( new RotoDrawableItemPrivate(isStroke) )
{
#ifdef NATRON_ROTO_INVERTIBLE
    QObject::connect( _imp->inverted->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SIGNAL(invertedStateChanged()) );
#endif
    QObject::connect( this, SIGNAL(overlayColorChanged()), context.get(), SIGNAL(refreshViewerOverlays()) );
    QObject::connect( _imp->color->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SIGNAL(shapeColorChanged()) );
    QObject::connect( _imp->compOperator->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)), this,
                      SIGNAL(compositingOperatorChanged(ViewSpec,int,int)) );
    std::vector<std::string> operators;
    std::vector<std::string> tooltips;
    Merge::getOperatorStrings(&operators, &tooltips);

    _imp->compOperator->populateChoices(operators, tooltips);
    _imp->compOperator->setDefaultValueFromLabel( Merge::getOperatorString(eMergeCopy) );
}

RotoDrawableItem::~RotoDrawableItem()
{
}

void
RotoDrawableItem::addKnob(const KnobIPtr& knob)
{
    _imp->knobs.push_back(knob);
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
    KnobChoicePtr glRenderKnob = node->getOpenGLEnabledKnob();
    if (glRenderKnob) {
        KnobChoicePtr rotoPaintGLRenderKnob = rotopaintNode->getOpenGLEnabledKnob();
        assert(rotoPaintGLRenderKnob);
        glRenderKnob->slaveTo(0, rotoPaintGLRenderKnob, 0);
    }
}

void
RotoDrawableItem::createNodes(bool connectNodes)
{
    const std::list<KnobIPtr >& knobs = getKnobs();

    for (std::list<KnobIPtr >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        QObject::connect( (*it)->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onRotoKnobChanged(ViewSpec,int,int)) );
    }

    RotoContextPtr context = getContext();
    NodePtr node = context->getNode();
    AppInstancePtr app = node->getApp();
    QString fixedNamePrefix = QString::fromUtf8( node->getScriptName_mt_safe().c_str() );
    fixedNamePrefix.append( QLatin1Char('_') );
    fixedNamePrefix.append( QString::fromUtf8( getScriptName().c_str() ) );
    fixedNamePrefix.append( QLatin1Char('_') );
    fixedNamePrefix.append( QString::number( context->getAge() ) );
    fixedNamePrefix.append( QLatin1Char('_') );

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

    QString maskPluginID = QString::fromUtf8(PLUGINID_NATRON_ROTOSHAPE);

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

        CreateNodeArgs args( pluginId.toStdString(), NodeCollectionPtr() );
        args.setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
        args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

        _imp->effectNode = app->createNode(args);
        if (!_imp->effectNode) {
            throw std::runtime_error("Rotopaint requires the plug-in " + pluginId.toStdString() + " in order to work");
        }
        assert(_imp->effectNode);

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
                CreateNodeArgs args(PLUGINID_OFX_TIMEOFFSET, NodeCollectionPtr() );
                args.setProperty<bool>(kCreateNodeArgsPropVolatile, true);
                args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
                args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());

                _imp->timeOffsetNode = app->createNode(args);
                if (!_imp->timeOffsetNode) {
                    throw std::runtime_error("Rotopaint requires the plug-in " PLUGINID_OFX_TIMEOFFSET " in order to work");
                }
                assert(_imp->timeOffsetNode);
            }
            {
                fixedNamePrefix = baseFixedName;
                fixedNamePrefix.append( QString::fromUtf8("FrameHold") );
                CreateNodeArgs args( PLUGINID_OFX_FRAMEHOLD, NodeCollectionPtr() );
                args.setProperty<bool>(kCreateNodeArgsPropVolatile, true);
                args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
                args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
                _imp->frameHoldNode = app->createNode(args);
                if (!_imp->frameHoldNode) {
                    throw std::runtime_error("Rotopaint requires the plug-in " PLUGINID_OFX_FRAMEHOLD " in order to work");
                }
                assert(_imp->frameHoldNode);
            }
        }
    }

    fixedNamePrefix = baseFixedName;
    fixedNamePrefix.append( QString::fromUtf8("Merge") );

    CreateNodeArgs args( PLUGINID_OFX_MERGE, NodeCollectionPtr() );
    args.setProperty<bool>(kCreateNodeArgsPropVolatile, true);
    args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());

    _imp->mergeNode = app->createNode(args);
    if (!_imp->mergeNode) {
        throw std::runtime_error("Rotopaint requires the plug-in " PLUGINID_OFX_MERGE " in order to work");
    }
    assert(_imp->mergeNode);

    if ( (type != eRotoStrokeTypeSolid) && (type != eRotoStrokeTypeSmear) ) {
        // Create the mask plug-in

        {
            fixedNamePrefix = baseFixedName;
            fixedNamePrefix.append( QString::fromUtf8("Mask") );
            CreateNodeArgs args( maskPluginID.toStdString(), NodeCollectionPtr() );
            args.setProperty<bool>(kCreateNodeArgsPropVolatile, true);
            args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
            args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
            args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
            _imp->maskNode = app->createNode(args);
            if (!_imp->maskNode) {
                throw std::runtime_error("Rotopaint requires the plug-in " + maskPluginID.toStdString() + " in order to work");
            }
            assert(_imp->maskNode);
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
        mergeOp->setValueFromLabel(Merge::getOperatorString(op), 0);
    }
    compOp->setDefaultValueFromLabel(Merge::getOperatorString(op), 0);

    if (isStroke) {
        if (type == eRotoStrokeTypeBlur) {
            double strength = isStroke->getBrushEffectKnob()->getValue();
            KnobIPtr knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
            KnobDoublePtr isDbl = toKnobDouble(knob);
            if (isDbl) {
                isDbl->setValues(strength, strength, ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
            }
        } else if (type == eRotoStrokeTypeSharpen) {
            //todo
        } else if (type == eRotoStrokeTypeSmear) {
            KnobDoublePtr spacingKnob = isStroke->getBrushSpacingKnob();
            assert(spacingKnob);
            spacingKnob->setValue(0.05);
        }

        setNodesThreadSafetyForRotopainting();
    }

    ///Attach this stroke to the underlying nodes used
    if (_imp->effectNode) {
        attachStrokeToNode(_imp->effectNode, node, thisShared);
    }
    if (_imp->maskNode) {
        attachStrokeToNode(_imp->maskNode, node, thisShared);
    }
    if (_imp->mergeNode) {
        attachStrokeToNode(_imp->mergeNode, node, thisShared);
    }
    if (_imp->timeOffsetNode) {
        attachStrokeToNode(_imp->timeOffsetNode, node, thisShared);
    }
    if (_imp->frameHoldNode) {
        attachStrokeToNode(_imp->frameHoldNode, node, thisShared);
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

    return findPreviousOfItemInLayer(layer, shared_from_this());
}

void
RotoDrawableItem::onRotoKnobChanged(ViewSpec /*view*/,
                                    int /*dimension*/,
                                    int reason)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    if (!handler) {
        return;
    }

    KnobIPtr triggerKnob = handler->getKnob();
    assert(triggerKnob);
    rotoKnobChanged(triggerKnob, (ValueChangedReasonEnum)reason);
}

void
RotoDrawableItem::rotoKnobChanged(const KnobIPtr& knob,
                                  ValueChangedReasonEnum reason)
{
    KnobChoicePtr compKnob = getOperatorKnob();

#ifdef NATRON_ROTO_INVERTIBLE
    KnobBoolPtr invertKnob = getInvertedKnob();
#endif

    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    RotoStrokeType type;

    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }

    if ( (reason == eValueChangedReasonSlaveRefresh) && (knob != _imp->center) && (knob != _imp->cloneCenter) ) {
        getContext()->s_breakMultiStroke();
    }

    if (knob == compKnob) {
        KnobIPtr mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
        KnobChoicePtr mergeOp = toKnobChoice(mergeOperatorKnob);
        if (mergeOp) {
            mergeOp->setValueFromLabel(compKnob->getEntry( compKnob->getValue() ), 0);
        }

        ///Since the compositing operator might have changed, we may have to change the rotopaint tree layout
        if (reason == eValueChangedReasonUserEdited) {
            getContext()->refreshRotoPaintTree();
        }
    }
#ifdef NATRON_ROTO_INVERTIBLE
    else if (knob == invertKnob) {
        KnobIPtr mergeMaskInvertKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamInvertMask);
        KnobBoolPtr mergeMaskInv = toKnobBool( mergeMaskInvertKnob );
        if (mergeMaskInv) {
            mergeMaskInv->setValue( invertKnob->getValue() );
        }

        ///Since the invert state might have changed, we may have to change the rotopaint tree layout
        if (reason == eValueChangedReasonUserEdited) {
            getContext()->refreshRotoPaintTree();
        }
    }
#endif
    else if (knob == _imp->sourceColor) {
        refreshNodesConnections(getContext()->canConcatenatedRotoPaintTree());
    } else if (knob == _imp->effectStrength) {
        double strength = _imp->effectStrength->getValue();
        switch (type) {
        case eRotoStrokeTypeBlur: {
            KnobIPtr knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
            KnobDoublePtr isDbl = toKnobDouble(knob);
            if (isDbl) {
                isDbl->setValues(strength, strength, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
            }
            break;
        }
        case eRotoStrokeTypeSharpen: {
            //todo
            break;
        }
        default:
            //others don't have a control
            break;
        }
    } else if ( (knob == _imp->timeOffset) && _imp->timeOffsetNode ) {
        int offsetMode_i = _imp->timeOffsetMode->getValue();
        KnobIPtr offsetKnob;

        if (offsetMode_i == 0) {
            offsetKnob = _imp->timeOffsetNode->getKnobByName(kTimeOffsetParamOffset);
        } else {
            offsetKnob = _imp->frameHoldNode->getKnobByName(kFrameHoldParamFirstFrame);
        }
        KnobIntPtr offset = toKnobInt(offsetKnob);
        if (offset) {
            double value = _imp->timeOffset->getValue();
            offset->setValue(value);
        }
    } else if ( (knob == _imp->timeOffsetMode) && _imp->timeOffsetNode ) {
        refreshNodesConnections(getContext()->canConcatenatedRotoPaintTree());
    }

    if ( (type == eRotoStrokeTypeClone) || (type == eRotoStrokeTypeReveal) ) {
        if (knob == _imp->cloneTranslate) {
            KnobIPtr translateKnob = _imp->effectNode->getKnobByName(kTransformParamTranslate);
            KnobDoublePtr translate = toKnobDouble(translateKnob);
            if (translate) {
                translate->clone(_imp->cloneTranslate);
            }
        } else if (knob == _imp->cloneRotate) {
            KnobIPtr rotateKnob = _imp->effectNode->getKnobByName(kTransformParamRotate);
            KnobDoublePtr rotate = toKnobDouble(rotateKnob);
            if (rotate) {
                rotate->clone(_imp->cloneRotate);
            }
        } else if (knob == _imp->cloneScale) {
            KnobIPtr scaleKnob = _imp->effectNode->getKnobByName(kTransformParamScale);
            KnobDoublePtr scale = toKnobDouble(scaleKnob);
            if (scale) {
                scale->clone(_imp->cloneScale);
            }
        } else if (knob == _imp->cloneScaleUniform) {
            KnobIPtr uniformKnob = _imp->effectNode->getKnobByName(kTransformParamUniform);
            KnobBoolPtr uniform = toKnobBool(uniformKnob);
            if (uniform) {
                uniform->clone(_imp->cloneScaleUniform);
            }
        } else if (knob == _imp->cloneSkewX) {
            KnobIPtr skewxKnob = _imp->effectNode->getKnobByName(kTransformParamSkewX);
            KnobDoublePtr skewX = toKnobDouble(skewxKnob);
            if (skewX) {
                skewX->clone(_imp->cloneSkewX);
            }
        } else if (knob == _imp->cloneSkewY) {
            KnobIPtr skewyKnob = _imp->effectNode->getKnobByName(kTransformParamSkewY);
            KnobDoublePtr skewY = toKnobDouble(skewyKnob);
            if (skewY) {
                skewY->clone(_imp->cloneSkewY);
            }
        } else if (knob == _imp->cloneSkewOrder) {
            KnobIPtr skewOrderKnob = _imp->effectNode->getKnobByName(kTransformParamSkewOrder);
            KnobChoicePtr skewOrder = toKnobChoice(skewOrderKnob);
            if (skewOrder) {
                skewOrder->clone(_imp->cloneSkewOrder);
            }
        } else if (knob == _imp->cloneCenter) {
            KnobIPtr centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
            KnobDoublePtr center = toKnobDouble(centerKnob);
            if (center) {
                center->clone(_imp->cloneCenter);
            }
        } else if (knob == _imp->cloneFilter) {
            KnobIPtr filterKnob = _imp->effectNode->getKnobByName(kTransformParamFilter);
            KnobChoicePtr filter = toKnobChoice(filterKnob);
            if (filter) {
                filter->clone(_imp->cloneFilter);
            }
        } else if (knob == _imp->cloneBlackOutside) {
            KnobIPtr boKnob = _imp->effectNode->getKnobByName(kTransformParamBlackOutside);
            KnobBoolPtr bo = toKnobBool(boKnob);
            if (bo) {
                bo->clone(_imp->cloneBlackOutside);
            }
        }
    }


    incrementNodesAge();
} // RotoDrawableItem::rotoKnobChanged

void
RotoDrawableItem::incrementNodesAge()
{
    if ( getContext()->getNode()->getApp()->getProject()->isLoadingProject() ) {
        return;
    }
    if (_imp->effectNode) {
        _imp->effectNode->incrementKnobsAge();
    }
    if (_imp->maskNode) {
        _imp->maskNode->incrementKnobsAge();
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->incrementKnobsAge();
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->incrementKnobsAge();
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->incrementKnobsAge();
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
    NodePtr rotoPaintInput =  getContext()->getNode()->getInput(0);
    NodePtr upstreamNode = previous ? previous->getMergeNode() : rotoPaintInput;
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
            double timeOffsetMode_i = _imp->timeOffsetMode->getValue();
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


        int reveal_i = _imp->sourceColor->getValue();
        NodePtr revealInput;
        bool shouldUseUpstreamForReveal = true;
        if ( ( (type == eRotoStrokeTypeReveal) ||
               ( type == eRotoStrokeTypeClone) ) && ( reveal_i > 0) ) {
            shouldUseUpstreamForReveal = false;
            revealInput = getContext()->getNode()->getInput(reveal_i - 1);
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


            NodePtr eraserInput = rotoPaintInput ? rotoPaintInput : _imp->effectNode;
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
    const std::list<KnobIPtr >& otherKnobs = otherDrawable->getKnobs();
    assert( otherKnobs.size() == _imp->knobs.size() );
    if ( otherKnobs.size() != _imp->knobs.size() ) {
        return;
    }
    std::list<KnobIPtr >::iterator it = _imp->knobs.begin();
    std::list<KnobIPtr >::const_iterator otherIt = otherKnobs.begin();
    for (; it != _imp->knobs.end(); ++it, ++otherIt) {
        (*it)->clone(*otherIt);
    }
    {
        QMutexLocker l(&itemMutex);
        std::memcpy(_imp->overlayColor, otherDrawable->_imp->overlayColor, sizeof(double) * 4);
    }
    RotoItem::clone(other);
}

static void
serializeRotoKnob(const KnobIPtr & knob,
                 SERIALIZATION_NAMESPACE::KnobSerializationPtr* serialization)
{
    std::pair<int, KnobIPtr > master = knob->getMaster(0);
    serialization->reset(new SERIALIZATION_NAMESPACE::KnobSerialization);
    if (master.second) {
        master.second->toSerialization(serialization->get());
    } else {
        knob->toSerialization(serialization->get());
    }
}

void
RotoDrawableItem::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{

    SERIALIZATION_NAMESPACE::RotoDrawableItemSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::RotoDrawableItemSerialization*>(obj);
    if (!s) {
        return;
    }

    assert(s);
    if (!s) {
        throw std::logic_error("RotoDrawableItem::save()");
    }
    for (std::list<KnobIPtr >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        SERIALIZATION_NAMESPACE::KnobSerializationPtr k;
        serializeRotoKnob( *it, &k );
        if (k->_mustSerialize) {
            s->_knobs.push_back(k);
        }
    }
    {
        QMutexLocker l(&itemMutex);
        std::memcpy(s->_overlayColor, _imp->overlayColor, sizeof(double) * 4);
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

    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = s->_knobs.begin(); it != s->_knobs.end(); ++it) {
        for (std::list<KnobIPtr >::const_iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
            if ( (*it2)->getName() == (*it)->getName() ) {
                boost::shared_ptr<KnobSignalSlotHandler> slot = (*it2)->getSignalSlotHandler();
                slot->blockSignals(true);
                (*it2)->fromSerialization(**it);
                slot->blockSignals(false);
                break;
            }
        }
    }
    {
        QMutexLocker l(&itemMutex);
        std::memcpy(_imp->overlayColor, s->_overlayColor, sizeof(double) * 4);
    }

    RotoStrokeType type;
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }

    KnobChoicePtr compKnob = getOperatorKnob();
    KnobIPtr mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
    KnobChoicePtr mergeOp = toKnobChoice( mergeOperatorKnob );
    if (mergeOp) {
        mergeOp->setValueFromLabel(compKnob->getEntry( compKnob->getValue() ), 0);
    }

    if ( (type == eRotoStrokeTypeClone) || (type == eRotoStrokeTypeReveal) ) {
        KnobIPtr translateKnob = _imp->effectNode->getKnobByName(kTransformParamTranslate);
        KnobDoublePtr translate = toKnobDouble( translateKnob );
        if (translate) {
            translate->clone( _imp->cloneTranslate );
        }
        KnobIPtr rotateKnob = _imp->effectNode->getKnobByName(kTransformParamRotate);
        KnobDoublePtr rotate = toKnobDouble( rotateKnob );
        if (rotate) {
            rotate->clone( _imp->cloneRotate );
        }
        KnobIPtr scaleKnob = _imp->effectNode->getKnobByName(kTransformParamScale);
        KnobDoublePtr scale = toKnobDouble( scaleKnob );
        if (scale) {
            scale->clone( _imp->cloneScale );
        }
        KnobIPtr uniformKnob = _imp->effectNode->getKnobByName(kTransformParamUniform);
        KnobBoolPtr uniform = toKnobBool( uniformKnob );
        if (uniform) {
            uniform->clone( _imp->cloneScaleUniform );
        }
        KnobIPtr skewxKnob = _imp->effectNode->getKnobByName(kTransformParamSkewX);
        KnobDoublePtr skewX = toKnobDouble( skewxKnob );
        if (skewX) {
            skewX->clone( _imp->cloneSkewX );
        }
        KnobIPtr skewyKnob = _imp->effectNode->getKnobByName(kTransformParamSkewY);
        KnobDoublePtr skewY = toKnobDouble( skewyKnob );
        if (skewY) {
            skewY->clone( _imp->cloneSkewY );
        }
        KnobIPtr skewOrderKnob = _imp->effectNode->getKnobByName(kTransformParamSkewOrder);
        KnobChoicePtr skewOrder = toKnobChoice( skewOrderKnob );
        if (skewOrder) {
            skewOrder->clone( _imp->cloneSkewOrder );
        }
        KnobIPtr centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
        KnobDoublePtr center = toKnobDouble( centerKnob );
        if (center) {
            center->clone( _imp->cloneCenter );
        }
        KnobIPtr filterKnob = _imp->effectNode->getKnobByName(kTransformParamFilter);
        KnobChoicePtr filter = toKnobChoice( filterKnob );
        if (filter) {
            filter->clone( _imp->cloneFilter );
        }
        KnobIPtr boKnob = _imp->effectNode->getKnobByName(kTransformParamBlackOutside);
        KnobBoolPtr bo = toKnobBool( boKnob );
        if (bo) {
            bo->clone( _imp->cloneBlackOutside );
        }

        int offsetMode_i = _imp->timeOffsetMode->getValue();
        KnobIPtr offsetKnob;

        if (offsetMode_i == 0) {
            offsetKnob = _imp->timeOffsetNode->getKnobByName(kTimeOffsetParamOffset);
        } else {
            offsetKnob = _imp->frameHoldNode->getKnobByName(kFrameHoldParamFirstFrame);
        }
        KnobIntPtr offset = toKnobInt( offsetKnob );
        if (offset) {
            offset->clone( _imp->timeOffset );
        }
    } else if (type == eRotoStrokeTypeBlur) {
        KnobIPtr knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
        KnobDoublePtr isDbl = toKnobDouble(knob);
        if (isDbl) {
            isDbl->clone( _imp->effectStrength );
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
        int lifetime_i = _imp->lifeTime->getValue();
        if (lifetime_i == 0) {
            return time == _imp->lifeTimeFrame->getValue();
        } else if (lifetime_i == 1) {
            return time <= _imp->lifeTimeFrame->getValue();
        } else if (lifetime_i == 2) {
            return time >= _imp->lifeTimeFrame->getValue();
        } else {
            return _imp->activated->getValueAtTime(time);
        }
    } catch (std::runtime_error) {
        return false;
    }
}

void
RotoDrawableItem::setActivated(bool a,
                               double time)
{
    _imp->activated->setValueAtTime(time, a, ViewSpec::all(), 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getOpacity(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->opacity->getValueAtTime(time);
}

void
RotoDrawableItem::setOpacity(double o,
                             double time)
{
    _imp->opacity->setValueAtTime(time, o, ViewSpec::all(), 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getFeatherDistance(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->feather->getValueAtTime(time);
}

void
RotoDrawableItem::setFeatherDistance(double d,
                                     double time)
{
    _imp->feather->setValueAtTime(time, d, ViewSpec::all(), 0);
    getContext()->onItemKnobChanged();
}

int
RotoDrawableItem::getNumKeyframesFeatherDistance() const
{
    return _imp->feather->getKeyFramesCount(ViewSpec::current(), 0);
}

void
RotoDrawableItem::setFeatherFallOff(double f,
                                    double time)
{
    _imp->featherFallOff->setValueAtTime(time, f, ViewSpec::all(), 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getFeatherFallOff(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->featherFallOff->getValueAtTime(time);
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
    color[0] = _imp->color->getValueAtTime(time, 0);
    color[1] = _imp->color->getValueAtTime(time, 1);
    color[2] = _imp->color->getValueAtTime(time, 2);
}

void
RotoDrawableItem::setColor(double time,
                           double r,
                           double g,
                           double b)
{
    _imp->color->setValueAtTime(time, r, ViewSpec::all(), 0);
    _imp->color->setValueAtTime(time, g, ViewSpec::all(), 1);
    _imp->color->setValueAtTime(time, b, ViewSpec::all(), 2);
    getContext()->onItemKnobChanged();
}

int
RotoDrawableItem::getCompositingOperator() const
{
    return _imp->compOperator->getValue();
}

void
RotoDrawableItem::setCompositingOperator(int op)
{
    _imp->compOperator->setValue( op);
}

std::string
RotoDrawableItem::getCompositingOperatorToolTip() const
{
    return _imp->compOperator->getHintToolTipFull();
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
    QMutexLocker l(&itemMutex);
    std::memcpy(color, _imp->overlayColor, sizeof(double) * 4);
}

void
RotoDrawableItem::setOverlayColor(const double *color)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&itemMutex);
        std::memcpy(_imp->overlayColor, color, sizeof(double) * 4);
    }
    Q_EMIT overlayColorChanged();
}

KnobBoolPtr RotoDrawableItem::getActivatedKnob() const
{
    return _imp->activated;
}

KnobDoublePtr RotoDrawableItem::getFeatherKnob() const
{
    return _imp->feather;
}

KnobDoublePtr RotoDrawableItem::getFeatherFallOffKnob() const
{
    return _imp->featherFallOff;
}

KnobDoublePtr RotoDrawableItem::getOpacityKnob() const
{
    return _imp->opacity;
}

KnobBoolPtr RotoDrawableItem::getInvertedKnob() const
{
#ifdef NATRON_ROTO_INVERTIBLE

    return _imp->inverted;
#else

    return KnobBoolPtr();
#endif
}

KnobChoicePtr RotoDrawableItem::getOperatorKnob() const
{
    return _imp->compOperator;
}

KnobColorPtr RotoDrawableItem::getColorKnob() const
{
    return _imp->color;
}

KnobDoublePtr
RotoDrawableItem::getBrushSizeKnob() const
{
    return _imp->brushSize;
}

KnobDoublePtr
RotoDrawableItem::getBrushHardnessKnob() const
{
    return _imp->brushHardness;
}

KnobDoublePtr
RotoDrawableItem::getBrushSpacingKnob() const
{
    return _imp->brushSpacing;
}

KnobDoublePtr
RotoDrawableItem::getBrushEffectKnob() const
{
    return _imp->effectStrength;
}

KnobDoublePtr
RotoDrawableItem::getBrushVisiblePortionKnob() const
{
    return _imp->visiblePortion;
}

KnobBoolPtr
RotoDrawableItem::getPressureOpacityKnob() const
{
    return _imp->pressureOpacity;
}

KnobBoolPtr
RotoDrawableItem::getPressureSizeKnob() const
{
    return _imp->pressureSize;
}

KnobBoolPtr
RotoDrawableItem::getPressureHardnessKnob() const
{
    return _imp->pressureHardness;
}

KnobBoolPtr
RotoDrawableItem::getBuildupKnob() const
{
    return _imp->buildUp;
}

KnobIntPtr
RotoDrawableItem::getTimeOffsetKnob() const
{
    return _imp->timeOffset;
}

KnobChoicePtr
RotoDrawableItem::getTimeOffsetModeKnob() const
{
    return _imp->timeOffsetMode;
}

KnobChoicePtr
RotoDrawableItem::getBrushSourceTypeKnob() const
{
    return _imp->sourceColor;
}

KnobDoublePtr
RotoDrawableItem::getBrushCloneTranslateKnob() const
{
    return _imp->cloneTranslate;
}

KnobDoublePtr
RotoDrawableItem::getCenterKnob() const
{
    return _imp->center;
}

KnobIntPtr
RotoDrawableItem::getLifeTimeFrameKnob() const
{
    return _imp->lifeTimeFrame;
}

KnobChoicePtr
RotoDrawableItem::getFallOffRampTypeKnob() const
{
    return _imp->fallOffRampType;
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

    return _imp->customOffset;
#else

    return KnobDoublePtr();
#endif
}

KnobDoublePtr
RotoDrawableItem::getShutterKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    return _imp->shutter;
#else

    return KnobDoublePtr();
#endif
}

KnobChoicePtr
RotoDrawableItem::getShutterTypeKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    return _imp->shutterType;
#else

    return KnobChoicePtr();
#endif
}

void
RotoDrawableItem::setKeyframeOnAllTransformParameters(double time)
{
    _imp->translate->setValueAtTime(time, _imp->translate->getValue(0), ViewSpec::all(), 0);
    _imp->translate->setValueAtTime(time, _imp->translate->getValue(1), ViewSpec::all(), 1);

    _imp->scale->setValueAtTime(time, _imp->scale->getValue(0), ViewSpec::all(), 0);
    _imp->scale->setValueAtTime(time, _imp->scale->getValue(1), ViewSpec::all(), 1);

    _imp->rotate->setValueAtTime(time, _imp->rotate->getValue(0), ViewSpec::all(), 0);

    _imp->skewX->setValueAtTime(time, _imp->skewX->getValue(0), ViewSpec::all(), 0);
    _imp->skewY->setValueAtTime(time, _imp->skewY->getValue(0), ViewSpec::all(), 0);
}

const std::list<KnobIPtr >&
RotoDrawableItem::getKnobs() const
{
    return _imp->knobs;
}

KnobIPtr
RotoDrawableItem::getKnobByName(const std::string& name) const
{
    for (std::list<KnobIPtr >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if ( (*it)->getName() == name ) {
            return *it;
        }
    }

    return KnobIPtr();
}

void
RotoDrawableItem::getTransformAtTime(double time,
                                     Transform::Matrix3x3* matrix) const
{
    double tx = _imp->translate->getValueAtTime(time, 0);
    double ty = _imp->translate->getValueAtTime(time, 1);
    double sx = _imp->scale->getValueAtTime(time, 0);
    double sy = _imp->scaleUniform->getValueAtTime(time) ? sx : _imp->scale->getValueAtTime(time, 1);
    double skewX = _imp->skewX->getValueAtTime(time, 0);
    double skewY = _imp->skewY->getValueAtTime(time, 0);
    double rot = _imp->rotate->getValueAtTime(time, 0);

    rot = Transform::toRadians(rot);
    double centerX = _imp->center->getValueAtTime(time, 0);
    double centerY = _imp->center->getValueAtTime(time, 1);
    bool skewOrderYX = _imp->skewOrder->getValueAtTime(time) == 1;
    *matrix = Transform::matTransformCanonical(tx, ty, sx, sy, skewX, skewY, skewOrderYX, rot, centerX, centerY);

    Transform::Matrix3x3 extraMat;
    extraMat.a = _imp->extraMatrix->getValueAtTime(time, 0); extraMat.b = _imp->extraMatrix->getValueAtTime(time, 1); extraMat.c = _imp->extraMatrix->getValueAtTime(time, 2);
    extraMat.d = _imp->extraMatrix->getValueAtTime(time, 3); extraMat.e = _imp->extraMatrix->getValueAtTime(time, 4); extraMat.f = _imp->extraMatrix->getValueAtTime(time, 5);
    extraMat.g = _imp->extraMatrix->getValueAtTime(time, 6); extraMat.h = _imp->extraMatrix->getValueAtTime(time, 7); extraMat.i = _imp->extraMatrix->getValueAtTime(time, 8);
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
    bool autoKeying = getContext()->isAutoKeyingEnabled();

    if (autoKeying) {
        _imp->translate->setValueAtTime(time, tx, ViewSpec::all(), 0);
        _imp->translate->setValueAtTime(time, ty, ViewSpec::all(), 1);

        _imp->scale->setValueAtTime(time, sx, ViewSpec::all(), 0);
        _imp->scale->setValueAtTime(time, sy, ViewSpec::all(), 1);

        _imp->center->setValueAtTime(time, centerX, ViewSpec::all(), 0);
        _imp->center->setValueAtTime(time, centerY, ViewSpec::all(), 1);

        _imp->rotate->setValueAtTime(time, rot, ViewSpec::all(), 0);

        _imp->skewX->setValueAtTime(time, skewX, ViewSpec::all(), 0);
        _imp->skewY->setValueAtTime(time, skewY, ViewSpec::all(), 0);
    } else {
        _imp->translate->setValue(tx, ViewSpec::all(), 0);
        _imp->translate->setValue(ty, ViewSpec::all(), 1);

        _imp->scale->setValue(sx, ViewSpec::all(), 0);
        _imp->scale->setValue(sy, ViewSpec::all(), 1);

        _imp->center->setValue(centerX, ViewSpec::all(), 0);
        _imp->center->setValue(centerY, ViewSpec::all(), 1);

        _imp->rotate->setValue(rot, ViewSpec::all(), 0);

        _imp->skewX->setValue(skewX, ViewSpec::all(), 0);
        _imp->skewY->setValue(skewY, ViewSpec::all(), 0);
    }

    onTransformSet(time);
}

void
RotoDrawableItem::setExtraMatrix(bool setKeyframe,
                                 double time,
                                 const Transform::Matrix3x3& mat)
{
    _imp->extraMatrix->beginChanges();
    if (setKeyframe) {
        _imp->extraMatrix->setValueAtTime(time, mat.a, ViewSpec::all(), 0); _imp->extraMatrix->setValueAtTime(time, mat.b, ViewSpec::all(), 1); _imp->extraMatrix->setValueAtTime(time, mat.c, ViewSpec::all(), 2);
        _imp->extraMatrix->setValueAtTime(time, mat.d, ViewSpec::all(), 3); _imp->extraMatrix->setValueAtTime(time, mat.e, ViewSpec::all(), 4); _imp->extraMatrix->setValueAtTime(time, mat.f, ViewSpec::all(), 5);
        _imp->extraMatrix->setValueAtTime(time, mat.g, ViewSpec::all(), 6); _imp->extraMatrix->setValueAtTime(time, mat.h, ViewSpec::all(), 7); _imp->extraMatrix->setValueAtTime(time, mat.i, ViewSpec::all(), 8);
    } else {
        _imp->extraMatrix->setValue(mat.a, ViewSpec::all(), 0); _imp->extraMatrix->setValue(mat.b, ViewSpec::all(), 1); _imp->extraMatrix->setValue(mat.c, ViewSpec::all(), 2);
        _imp->extraMatrix->setValue(mat.d, ViewSpec::all(), 3); _imp->extraMatrix->setValue(mat.e, ViewSpec::all(), 4); _imp->extraMatrix->setValue(mat.f, ViewSpec::all(), 5);
        _imp->extraMatrix->setValue(mat.g, ViewSpec::all(), 6); _imp->extraMatrix->setValue(mat.h, ViewSpec::all(), 7); _imp->extraMatrix->setValue(mat.i, ViewSpec::all(), 8);
    }
    _imp->extraMatrix->endChanges();
}

void
RotoDrawableItem::resetTransformCenter()
{
    double time = getContext()->getNode()->getApp()->getTimeLine()->currentFrame();
    RectD bbox =  getBoundingBox(time);
    KnobDoublePtr centerKnob = _imp->center;

    centerKnob->beginChanges();

    std::pair<int, KnobIPtr> hasMaster = centerKnob->getMaster(0);
    if (hasMaster.second) {
        for (int i = 0; i < centerKnob->getDimension(); ++i) {
            centerKnob->unSlave(i, false);
        }
    }
    centerKnob->removeAnimation(ViewSpec::all(), 0);
    centerKnob->removeAnimation(ViewSpec::all(), 1);
    centerKnob->setValues( (bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2., ViewSpec::all(), eValueChangedReasonNatronInternalEdited );
    if (hasMaster.second) {
        for (int i = 0; i < centerKnob->getDimension(); ++i) {
            centerKnob->slaveTo(i, hasMaster.second, i);
        }
    }
    centerKnob->endChanges();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_RotoDrawableItem.cpp"
