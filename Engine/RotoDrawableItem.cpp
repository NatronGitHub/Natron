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
#include <stdexcept>

#include <QLineF>
#include <QtDebug>

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
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Interpolation.h"
#include "Engine/KnobSerialization.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoDrawableItemSerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoContext.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

#define kMergeOFXParamOperation "operation"
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

//This will enable correct evaluation of beziers
//#define ROTO_USE_MESH_PATTERN_ONLY

// The number of pressure levels is 256 on an old Wacom Graphire 4, and 512 on an entry-level Wacom Bamboo
// 512 should be OK, see:
// http://www.davidrevoy.com/article182/calibrating-wacom-stylus-pressure-on-krita
#define ROTO_PRESSURE_LEVELS 512


using namespace Natron;


////////////////////////////////////RotoDrawableItem////////////////////////////////////

RotoDrawableItem::RotoDrawableItem(const boost::shared_ptr<RotoContext>& context,
                                   const std::string & name,
                                   const boost::shared_ptr<RotoLayer>& parent,
                                   bool isStroke)
    : RotoItem(context,name,parent)
      , _imp( new RotoDrawableItemPrivate(isStroke) )
{
#ifdef NATRON_ROTO_INVERTIBLE
    QObject::connect( _imp->inverted->getSignalSlotHandler().get(), SIGNAL( valueChanged(int,int) ), this, SIGNAL( invertedStateChanged() ) );
#endif
    QObject::connect( this, SIGNAL( overlayColorChanged() ), context.get(), SIGNAL( refreshViewerOverlays() ) );
    QObject::connect( _imp->color->getSignalSlotHandler().get(), SIGNAL( valueChanged(int,int) ), this, SIGNAL( shapeColorChanged() ) );
    QObject::connect( _imp->compOperator->getSignalSlotHandler().get(), SIGNAL( valueChanged(int,int) ), this,
                      SIGNAL( compositingOperatorChanged(int,int) ) );
    
    std::vector<std::string> operators;
    std::vector<std::string> tooltips;
    getNatronCompositingOperators(&operators, &tooltips);
    
    _imp->compOperator->populateChoices(operators,tooltips);
    _imp->compOperator->setDefaultValueFromLabel(getNatronOperationString(eMergeCopy));
    
}

RotoDrawableItem::~RotoDrawableItem()
{
}

void
RotoDrawableItem::addKnob(const boost::shared_ptr<KnobI>& knob)
{
    _imp->knobs.push_back(knob);
}

void
RotoDrawableItem::setNodesThreadSafetyForRotopainting()
{
    
    assert(boost::dynamic_pointer_cast<RotoStrokeItem>(boost::dynamic_pointer_cast<RotoDrawableItem>(shared_from_this())));
    
    getContext()->getNode()->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    getContext()->setWhileCreatingPaintStrokeOnMergeNodes(true);
    if (_imp->effectNode) {
        _imp->effectNode->setWhileCreatingPaintStroke(true);
        _imp->effectNode->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->setWhileCreatingPaintStroke(true);
        _imp->mergeNode->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->setWhileCreatingPaintStroke(true);
        _imp->timeOffsetNode->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->setWhileCreatingPaintStroke(true);
        _imp->frameHoldNode->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    }
}

void
RotoDrawableItem::createNodes(bool connectNodes)
{
    
    const std::list<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(valueChanged(int,int)), this, SLOT(onRotoKnobChanged(int,int)));
    }
    
    boost::shared_ptr<RotoContext> context = getContext();
    boost::shared_ptr<Natron::Node> node = context->getNode();
  
    
    AppInstance* app = node->getApp();
    QString fixedNamePrefix(node->getScriptName_mt_safe().c_str());
    fixedNamePrefix.append('_');
    fixedNamePrefix.append(getScriptName().c_str());
    fixedNamePrefix.append('_');
    fixedNamePrefix.append(QString::number(context->getAge()));
    fixedNamePrefix.append('_');
    
    QString pluginId;
    
    RotoStrokeType type;
    boost::shared_ptr<RotoDrawableItem> thisShared = boost::dynamic_pointer_cast<RotoDrawableItem>(shared_from_this());
    assert(thisShared);
    boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(thisShared);

    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }
  
    switch (type) {
        case Natron::eRotoStrokeTypeBlur:
            pluginId = PLUGINID_OFX_BLURCIMG;
            break;
        case Natron::eRotoStrokeTypeEraser:
            pluginId = PLUGINID_OFX_CONSTANT;
            break;
        case Natron::eRotoStrokeTypeSolid:
            pluginId = PLUGINID_OFX_ROTO;
            break;
        case Natron::eRotoStrokeTypeClone:
        case Natron::eRotoStrokeTypeReveal:
            pluginId = PLUGINID_OFX_TRANSFORM;
            break;
        case Natron::eRotoStrokeTypeBurn:
        case Natron::eRotoStrokeTypeDodge:
            //uses merge
            break;
        case Natron::eRotoStrokeTypeSharpen:
            //todo
            break;
        case Natron::eRotoStrokeTypeSmear:
            pluginId = PLUGINID_NATRON_ROTOSMEAR;
            break;
    }
    
    QString baseFixedName = fixedNamePrefix;
    if (!pluginId.isEmpty()) {
        fixedNamePrefix.append("Effect");
        
        CreateNodeArgs args(pluginId, "",
                            -1,-1,
                            false,
                            INT_MIN,
                            INT_MIN,
                            false,
                            false,
                            false,
                            fixedNamePrefix,
                            CreateNodeArgs::DefaultValuesList(),
                            boost::shared_ptr<NodeCollection>());
        args.createGui = false;
        _imp->effectNode = app->createNode(args);
        assert(_imp->effectNode);
        
        if (type == eRotoStrokeTypeClone || type == eRotoStrokeTypeReveal) {
            {
                fixedNamePrefix = baseFixedName;
                fixedNamePrefix.append("TimeOffset");
                CreateNodeArgs args(PLUGINID_OFX_TIMEOFFSET, "",
                                    -1,-1,
                                    false,
                                    INT_MIN,
                                    INT_MIN,
                                    false,
                                    false,
                                    false,
                                    fixedNamePrefix,
                                    CreateNodeArgs::DefaultValuesList(),
                                    boost::shared_ptr<NodeCollection>());
                args.createGui = false;
                _imp->timeOffsetNode = app->createNode(args);
                assert(_imp->timeOffsetNode);
              
            }
            {
                fixedNamePrefix = baseFixedName;
                fixedNamePrefix.append("FrameHold");
                CreateNodeArgs args(PLUGINID_OFX_FRAMEHOLD, "",
                                    -1,-1,
                                    false,
                                    INT_MIN,
                                    INT_MIN,
                                    false,
                                    false,
                                    false,
                                    fixedNamePrefix,
                                    CreateNodeArgs::DefaultValuesList(),
                                    boost::shared_ptr<NodeCollection>());
                args.createGui = false;
                _imp->frameHoldNode = app->createNode(args);
                assert(_imp->frameHoldNode);
               
            }
        }
    }
    
    fixedNamePrefix = baseFixedName;
    fixedNamePrefix.append("Merge");
    CreateNodeArgs args(PLUGINID_OFX_MERGE, "",
                        -1,-1,
                        false,
                        INT_MIN,
                        INT_MIN,
                        false,
                        false,
                        false,
                        fixedNamePrefix,
                        CreateNodeArgs::DefaultValuesList(),
                        boost::shared_ptr<NodeCollection>());
    args.createGui = false;
    
    bool ok = _imp->mergeNode = app->createNode(args);
    assert(ok);
    if (!ok) {
        throw std::logic_error("RotoDrawableItem::createNodes");
    }
    assert(_imp->mergeNode);
    
    if (type != eRotoStrokeTypeSolid) {
        int maxInp = _imp->mergeNode->getMaxInputCount();
        for (int i = 0; i < maxInp; ++i) {
            if (_imp->mergeNode->getLiveInstance()->isInputMask(i)) {
                
                //Connect this rotopaint node as a mask
                ok = _imp->mergeNode->connectInput(node, i);
                assert(ok);
                break;
            }
        }
    }
    
    boost::shared_ptr<KnobI> mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
    assert(mergeOperatorKnob);
    KnobChoice* mergeOp = dynamic_cast<KnobChoice*>(mergeOperatorKnob.get());
    assert(mergeOp);
    
    boost::shared_ptr<KnobChoice> compOp = getOperatorKnob();

    MergingFunctionEnum op;
    if (type == eRotoStrokeTypeDodge || type == eRotoStrokeTypeBurn) {
        op = (type == eRotoStrokeTypeDodge ?eMergeColorDodge : eMergeColorBurn);
    } else if (type == eRotoStrokeTypeSolid) {
        op = eMergeOver;
    } else {
        op = eMergeCopy;
    }
    mergeOp->setValueFromLabel(getNatronOperationString(op), 0);
    compOp->setValueFromLabel(getNatronOperationString(op), 0);

    if (isStroke) {
        if (type == eRotoStrokeTypeBlur) {
            double strength = isStroke->getBrushEffectKnob()->getValue();
            boost::shared_ptr<KnobI> knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>(knob.get());
            if (isDbl) {
                isDbl->setValues(strength, strength, Natron::eValueChangedReasonNatronInternalEdited);
            }
        } else if (type == eRotoStrokeTypeSharpen) {
            //todo
        } else if (type == eRotoStrokeTypeSmear) {
            boost::shared_ptr<KnobDouble> spacingKnob = isStroke->getBrushSpacingKnob();
            assert(spacingKnob);
            spacingKnob->setValue(0.05, 0);
        }
        
        setNodesThreadSafetyForRotopainting();
    }
    
    ///Attach this stroke to the underlying nodes used
    if (_imp->effectNode) {
        _imp->effectNode->attachRotoItem(thisShared);
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->attachRotoItem(thisShared);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->attachRotoItem(thisShared);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->attachRotoItem(thisShared);
    }
    
    
    
    if (connectNodes) {
        refreshNodesConnections();
    }

}

std::string
RotoDrawableItem::getCacheID() const
{
    assert(_imp->mergeNode);
    //The cache iD is the one of the internal merge node + RotoDrawableItem
    return _imp->mergeNode->getCacheID() + ".RotoDrawableItem";
}

void
RotoDrawableItem::disconnectNodes()
{
    _imp->mergeNode->disconnectInput(0);
    _imp->mergeNode->disconnectInput(1);
    if (_imp->effectNode) {
        _imp->effectNode->disconnectInput(0);
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
        _imp->effectNode->deactivate(std::list< Node* >(),true,false,false,false);
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->deactivate(std::list< Node* >(),true,false,false,false);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->deactivate(std::list< Node* >(),true,false,false,false);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->deactivate(std::list< Node* >(),true,false,false,false);
    }
}

void
RotoDrawableItem::activateNodes()
{
    if (_imp->effectNode) {
        _imp->effectNode->activate(std::list< Node* >(),false,false);
    }
    _imp->mergeNode->activate(std::list< Node* >(),false,false);
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->activate(std::list< Node* >(),false,false);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->activate(std::list< Node* >(),false,false);
    }
}



static RotoDrawableItem* findPreviousOfItemInLayer(RotoLayer* layer, RotoItem* item)
{
    RotoItems layerItems = layer->getItems_mt_safe();
    if (layerItems.empty()) {
        return 0;
    }
    RotoItems::iterator found = layerItems.end();
    if (item) {
        for (RotoItems::iterator it = layerItems.begin(); it != layerItems.end(); ++it) {
            if (it->get() == item) {
                found = it;
                break;
            }
        }
        assert(found != layerItems.end());
    } else {
        found = layerItems.end();
    }
    
    if (found != layerItems.end()) {
        ++found;
        for (; found != layerItems.end(); ++found) {
            
            //We found another stroke below at the same level
            RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>(found->get());
            if (isDrawable) {
                assert(isDrawable != item);
                return isDrawable;
            }
            
            //Cycle through a layer that is at the same level
            RotoLayer* isLayer = dynamic_cast<RotoLayer*>(found->get());
            if (isLayer) {
                RotoDrawableItem* si = findPreviousOfItemInLayer(isLayer, 0);
                if (si) {
                    assert(si != item);
                    return si;
                }
            }
        }
    }
    
    //Item was still not found, find in great parent layer
    boost::shared_ptr<RotoLayer> parentLayer = layer->getParentLayer();
    if (!parentLayer) {
        return 0;
    }
    RotoItems greatParentItems = parentLayer->getItems_mt_safe();
    
    found = greatParentItems.end();
    for (RotoItems::iterator it = greatParentItems.begin(); it != greatParentItems.end(); ++it) {
        if (it->get() == layer) {
            found = it;
            break;
        }
    }
    assert(found != greatParentItems.end());
    RotoDrawableItem* ret = findPreviousOfItemInLayer(parentLayer.get(), layer);
    assert(ret != item);
    return ret;
}

RotoDrawableItem*
RotoDrawableItem::findPreviousInHierarchy()
{
    boost::shared_ptr<RotoLayer> layer = getParentLayer();
    if (!layer) {
        return 0;
    }
    return findPreviousOfItemInLayer(layer.get(), this);
}

void
RotoDrawableItem::onRotoKnobChanged(int /*dimension*/, int reason)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (!handler) {
        return;
    }
    
    boost::shared_ptr<KnobI> triggerKnob = handler->getKnob();
    assert(triggerKnob);
    rotoKnobChanged(triggerKnob, (Natron::ValueChangedReasonEnum)reason);
    
    
}

void
RotoDrawableItem::rotoKnobChanged(const boost::shared_ptr<KnobI>& knob, Natron::ValueChangedReasonEnum reason)
{
    boost::shared_ptr<KnobChoice> compKnob = getOperatorKnob();
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    RotoStrokeType type;
    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }

    if (reason == Natron::eValueChangedReasonSlaveRefresh && knob != _imp->center && knob != _imp->cloneCenter) {
        getContext()->s_breakMultiStroke();
    }
    
    if (knob == compKnob) {
        boost::shared_ptr<KnobI> mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
        KnobChoice* mergeOp = dynamic_cast<KnobChoice*>(mergeOperatorKnob.get());
        if (mergeOp) {
            mergeOp->setValueFromLabel(compKnob->getEntry(compKnob->getValue()), 0);
        }
        
        ///Since the compositing operator might have changed, we may have to change the rotopaint tree layout
        if ( reason == Natron::eValueChangedReasonUserEdited) {
            getContext()->refreshRotoPaintTree();
        }
    } else if (knob == _imp->sourceColor) {
        refreshNodesConnections();
    } else if (knob == _imp->effectStrength) {
        
        double strength = _imp->effectStrength->getValue();
        switch (type) {
            case Natron::eRotoStrokeTypeBlur: {
                boost::shared_ptr<KnobI> knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
                KnobDouble* isDbl = dynamic_cast<KnobDouble*>(knob.get());
                if (isDbl) {
                    isDbl->setValues(strength, strength, Natron::eValueChangedReasonNatronInternalEdited);
                }
            }   break;
            case Natron::eRotoStrokeTypeSharpen: {
                //todo
                break;
            }
            default:
                //others don't have a control
                break;
        }
    } else if (knob == _imp->timeOffset && _imp->timeOffsetNode) {
        
        int offsetMode_i = _imp->timeOffsetMode->getValue();
        boost::shared_ptr<KnobI> offsetKnob;
        
        if (offsetMode_i == 0) {
            offsetKnob = _imp->timeOffsetNode->getKnobByName(kTimeOffsetParamOffset);
        } else {
            offsetKnob = _imp->frameHoldNode->getKnobByName(kFrameHoldParamFirstFrame);
        }
        KnobInt* offset = dynamic_cast<KnobInt*>(offsetKnob.get());
        if (offset) {
            double value = _imp->timeOffset->getValue();
            offset->setValue(value,0);
        }
    } else if (knob == _imp->timeOffsetMode && _imp->timeOffsetNode) {
        refreshNodesConnections();
    }
    
    if (type == eRotoStrokeTypeClone || type == eRotoStrokeTypeReveal) {
        if (knob == _imp->cloneTranslate) {
            boost::shared_ptr<KnobI> translateKnob = _imp->effectNode->getKnobByName(kTransformParamTranslate);
            KnobDouble* translate = dynamic_cast<KnobDouble*>(translateKnob.get());
            if (translate) {
                translate->clone(_imp->cloneTranslate.get());
            }
        } else if (knob == _imp->cloneRotate) {
            boost::shared_ptr<KnobI> rotateKnob = _imp->effectNode->getKnobByName(kTransformParamRotate);
            KnobDouble* rotate = dynamic_cast<KnobDouble*>(rotateKnob.get());
            if (rotate) {
                rotate->clone(_imp->cloneRotate.get());
            }
        } else if (knob == _imp->cloneScale) {
            boost::shared_ptr<KnobI> scaleKnob = _imp->effectNode->getKnobByName(kTransformParamScale);
            KnobDouble* scale = dynamic_cast<KnobDouble*>(scaleKnob.get());
            if (scale) {
                scale->clone(_imp->cloneScale.get());
            }
        } else if (knob == _imp->cloneScaleUniform) {
            boost::shared_ptr<KnobI> uniformKnob = _imp->effectNode->getKnobByName(kTransformParamUniform);
            KnobBool* uniform = dynamic_cast<KnobBool*>(uniformKnob.get());
            if (uniform) {
                uniform->clone(_imp->cloneScaleUniform.get());
            }
        } else if (knob == _imp->cloneSkewX) {
            boost::shared_ptr<KnobI> skewxKnob = _imp->effectNode->getKnobByName(kTransformParamSkewX);
            KnobDouble* skewX = dynamic_cast<KnobDouble*>(skewxKnob.get());
            if (skewX) {
                skewX->clone(_imp->cloneSkewX.get());
            }
        } else if (knob == _imp->cloneSkewY) {
            boost::shared_ptr<KnobI> skewyKnob = _imp->effectNode->getKnobByName(kTransformParamSkewY);
            KnobDouble* skewY = dynamic_cast<KnobDouble*>(skewyKnob.get());
            if (skewY) {
                skewY->clone(_imp->cloneSkewY.get());
            }
        } else if (knob == _imp->cloneSkewOrder) {
            boost::shared_ptr<KnobI> skewOrderKnob = _imp->effectNode->getKnobByName(kTransformParamSkewOrder);
            KnobChoice* skewOrder = dynamic_cast<KnobChoice*>(skewOrderKnob.get());
            if (skewOrder) {
                skewOrder->clone(_imp->cloneSkewOrder.get());
            }
        } else if (knob == _imp->cloneCenter) {
            boost::shared_ptr<KnobI> centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
            KnobDouble* center = dynamic_cast<KnobDouble*>(centerKnob.get());
            if (center) {
                center->clone(_imp->cloneCenter.get());
                
            }
        } else if (knob == _imp->cloneFilter) {
            boost::shared_ptr<KnobI> filterKnob = _imp->effectNode->getKnobByName(kTransformParamFilter);
            KnobChoice* filter = dynamic_cast<KnobChoice*>(filterKnob.get());
            if (filter) {
                filter->clone(_imp->cloneFilter.get());
            }
        } else if (knob == _imp->cloneBlackOutside) {
            boost::shared_ptr<KnobI> boKnob = _imp->effectNode->getKnobByName(kTransformParamBlackOutside);
            KnobBool* bo = dynamic_cast<KnobBool*>(boKnob.get());
            if (bo) {
                bo->clone(_imp->cloneBlackOutside.get());
                
            }
        }
    }

    
    
    incrementNodesAge();

}

void
RotoDrawableItem::incrementNodesAge()
{
    if (_imp->effectNode) {
        _imp->effectNode->incrementKnobsAge();
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

boost::shared_ptr<Natron::Node>
RotoDrawableItem::getEffectNode() const
{
    return _imp->effectNode;
}


boost::shared_ptr<Natron::Node>
RotoDrawableItem::getMergeNode() const
{
    return _imp->mergeNode;
}

boost::shared_ptr<Natron::Node>
RotoDrawableItem::getTimeOffsetNode() const
{
    
    return _imp->timeOffsetNode;
}

boost::shared_ptr<Natron::Node>
RotoDrawableItem::getFrameHoldNode() const
{
    return _imp->frameHoldNode;
}

void
RotoDrawableItem::refreshNodesConnections()
{
    RotoDrawableItem* previous = findPreviousInHierarchy();
    boost::shared_ptr<Node> rotoPaintInput =  getContext()->getNode()->getInput(0);
    
    boost::shared_ptr<Node> upstreamNode = previous ? previous->getMergeNode() : rotoPaintInput;
    
    
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    RotoStrokeType type;
    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }
    
    if (_imp->effectNode &&
        type != eRotoStrokeTypeEraser) {
        
        /*
         Tree for this effect goes like this:
                  Effect - <Optional Time Node>------- Reveal input (Reveal/Clone) or Upstream Node otherwise
                /
         Merge
                \--------------------------------------Upstream Node
         
         */
        
        boost::shared_ptr<Natron::Node> effectInput;
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
                _imp->effectNode->disconnectInput(0);
                _imp->effectNode->connectInputBase(effectInput, 0);
            }
        }
        /*
         * This case handles: Stroke, Blur, Sharpen, Smear, Clone
         */
        if (_imp->mergeNode->getInput(1) != _imp->effectNode) {
            _imp->mergeNode->disconnectInput(1);
            _imp->mergeNode->connectInputBase(_imp->effectNode, 1); // A
        }
        
        if (_imp->mergeNode->getInput(0) != upstreamNode) {
            _imp->mergeNode->disconnectInput(0);
            if (upstreamNode) {
                _imp->mergeNode->connectInputBase(upstreamNode, 0); // B
            }
        }
        
        
        int reveal_i = _imp->sourceColor->getValue();
        boost::shared_ptr<Node> revealInput;
        bool shouldUseUpstreamForReveal = true;
        if ((type == eRotoStrokeTypeReveal ||
             type == eRotoStrokeTypeClone) && reveal_i > 0) {
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
                effectInput->disconnectInput(0);
                effectInput->connectInputBase(revealInput, 0);
            }
        } else {
            if (effectInput->getInput(0)) {
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

            
            boost::shared_ptr<Node> eraserInput = rotoPaintInput ? rotoPaintInput : _imp->effectNode;
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
            
        }  else if (type == eRotoStrokeTypeDodge || type == eRotoStrokeTypeBurn) {
            
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
    
    
}

void
RotoDrawableItem::resetNodesThreadSafety()
{
    if (_imp->effectNode) {
        _imp->effectNode->revertToPluginThreadSafety();
    }
    _imp->mergeNode->revertToPluginThreadSafety();
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
    const std::list<boost::shared_ptr<KnobI> >& otherKnobs = otherDrawable->getKnobs();
    assert(otherKnobs.size() == _imp->knobs.size());
    if (otherKnobs.size() != _imp->knobs.size()) {
        return;
    }
    std::list<boost::shared_ptr<KnobI> >::iterator it = _imp->knobs.begin();
    std::list<boost::shared_ptr<KnobI> >::const_iterator otherIt = otherKnobs.begin();
    for (; it != _imp->knobs.end(); ++it, ++otherIt) {
        (*it)->clone(*otherIt);
    }
    {
        QMutexLocker l(&itemMutex);
        memcpy(_imp->overlayColor, otherDrawable->_imp->overlayColor, sizeof(double) * 4);
    }
    RotoItem::clone(other);
}

static void
serializeRotoKnob(const boost::shared_ptr<KnobI> & knob,
                  KnobSerialization* serialization)
{
    std::pair<int, boost::shared_ptr<KnobI> > master = knob->getMaster(0);
    bool wasSlaved = false;

    if (master.second) {
        wasSlaved = true;
        knob->unSlave(0,false);
    }

    serialization->initialize(knob);

    if (wasSlaved) {
        knob->slaveTo(0, master.second, master.first);
    }
}

void
RotoDrawableItem::save(RotoItemSerialization *obj) const
{
    RotoDrawableItemSerialization* s = dynamic_cast<RotoDrawableItemSerialization*>(obj);

    assert(s);
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        boost::shared_ptr<KnobSerialization> k(new KnobSerialization());
        serializeRotoKnob(*it,k.get());
        s->_knobs.push_back(k);
    }
    {
        
        QMutexLocker l(&itemMutex);
        memcpy(s->_overlayColor, _imp->overlayColor, sizeof(double) * 4);
    }
    RotoItem::save(obj);
}

void
RotoDrawableItem::load(const RotoItemSerialization &obj)
{
    RotoItem::load(obj);

    const RotoDrawableItemSerialization & s = dynamic_cast<const RotoDrawableItemSerialization &>(obj);
    for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = s._knobs.begin(); it!=s._knobs.end(); ++it) {
        
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
            if ((*it2)->getName() == (*it)->getName()) {
                boost::shared_ptr<KnobSignalSlotHandler> s = (*it2)->getSignalSlotHandler();
                s->blockSignals(true);
                (*it2)->clone((*it)->getKnob().get());
                s->blockSignals(false);
                break;
            }
        }
    }
    {
        QMutexLocker l(&itemMutex);
        memcpy(_imp->overlayColor, s._overlayColor, sizeof(double) * 4);
    }
    
    RotoStrokeType type;
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }

    boost::shared_ptr<KnobChoice> compKnob = getOperatorKnob();
    boost::shared_ptr<KnobI> mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
    KnobChoice* mergeOp = dynamic_cast<KnobChoice*>(mergeOperatorKnob.get());
    if (mergeOp) {
        mergeOp->setValueFromLabel(compKnob->getEntry(compKnob->getValue()), 0);
    }

    if (type == eRotoStrokeTypeClone || type == eRotoStrokeTypeReveal) {
        boost::shared_ptr<KnobI> translateKnob = _imp->effectNode->getKnobByName(kTransformParamTranslate);
        KnobDouble* translate = dynamic_cast<KnobDouble*>(translateKnob.get());
        if (translate) {
            translate->clone(_imp->cloneTranslate.get());
        }
        boost::shared_ptr<KnobI> rotateKnob = _imp->effectNode->getKnobByName(kTransformParamRotate);
        KnobDouble* rotate = dynamic_cast<KnobDouble*>(rotateKnob.get());
        if (rotate) {
            rotate->clone(_imp->cloneRotate.get());
        }
        boost::shared_ptr<KnobI> scaleKnob = _imp->effectNode->getKnobByName(kTransformParamScale);
        KnobDouble* scale = dynamic_cast<KnobDouble*>(scaleKnob.get());
        if (scale) {
            scale->clone(_imp->cloneScale.get());
        }
        boost::shared_ptr<KnobI> uniformKnob = _imp->effectNode->getKnobByName(kTransformParamUniform);
        KnobBool* uniform = dynamic_cast<KnobBool*>(uniformKnob.get());
        if (uniform) {
            uniform->clone(_imp->cloneScaleUniform.get());
        }
        boost::shared_ptr<KnobI> skewxKnob = _imp->effectNode->getKnobByName(kTransformParamSkewX);
        KnobDouble* skewX = dynamic_cast<KnobDouble*>(skewxKnob.get());
        if (skewX) {
            skewX->clone(_imp->cloneSkewX.get());
        }
        boost::shared_ptr<KnobI> skewyKnob = _imp->effectNode->getKnobByName(kTransformParamSkewY);
        KnobDouble* skewY = dynamic_cast<KnobDouble*>(skewyKnob.get());
        if (skewY) {
            skewY->clone(_imp->cloneSkewY.get());
        }
        boost::shared_ptr<KnobI> skewOrderKnob = _imp->effectNode->getKnobByName(kTransformParamSkewOrder);
        KnobChoice* skewOrder = dynamic_cast<KnobChoice*>(skewOrderKnob.get());
        if (skewOrder) {
            skewOrder->clone(_imp->cloneSkewOrder.get());
        }
        boost::shared_ptr<KnobI> centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
        KnobDouble* center = dynamic_cast<KnobDouble*>(centerKnob.get());
        if (center) {
            center->clone(_imp->cloneCenter.get());
            
        }
        boost::shared_ptr<KnobI> filterKnob = _imp->effectNode->getKnobByName(kTransformParamFilter);
        KnobChoice* filter = dynamic_cast<KnobChoice*>(filterKnob.get());
        if (filter) {
            filter->clone(_imp->cloneFilter.get());
        }
        boost::shared_ptr<KnobI> boKnob = _imp->effectNode->getKnobByName(kTransformParamBlackOutside);
        KnobBool* bo = dynamic_cast<KnobBool*>(boKnob.get());
        if (bo) {
            bo->clone(_imp->cloneBlackOutside.get());
            
        }
        
        int offsetMode_i = _imp->timeOffsetMode->getValue();
        boost::shared_ptr<KnobI> offsetKnob;
        
        if (offsetMode_i == 0) {
            offsetKnob = _imp->timeOffsetNode->getKnobByName(kTimeOffsetParamOffset);
        } else {
            offsetKnob = _imp->frameHoldNode->getKnobByName(kFrameHoldParamFirstFrame);
        }
        KnobInt* offset = dynamic_cast<KnobInt*>(offsetKnob.get());
        if (offset) {
            offset->clone(_imp->timeOffset.get());
        }
        
        
    } else if (type == eRotoStrokeTypeBlur) {
        boost::shared_ptr<KnobI> knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
        KnobDouble* isDbl = dynamic_cast<KnobDouble*>(knob.get());
        if (isDbl) {
            isDbl->clone(_imp->effectStrength.get());
        }
    }

}

bool
RotoDrawableItem::isActivated(double time) const
{
    if (!isGloballyActivated()) {
        return false;
    }
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
}

void
RotoDrawableItem::setActivated(bool a, double time)
{
    _imp->activated->setValueAtTime(time, a, 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getOpacity(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->opacity->getValueAtTime(time);
}

void
RotoDrawableItem::setOpacity(double o,double time)
{
    _imp->opacity->setValueAtTime(time, o, 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getFeatherDistance(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->feather->getValueAtTime(time);
}

void
RotoDrawableItem::setFeatherDistance(double d,double time)
{
    _imp->feather->setValueAtTime(time, d, 0);
    getContext()->onItemKnobChanged();
}


int
RotoDrawableItem::getNumKeyframesFeatherDistance() const
{
    return _imp->feather->getKeyFramesCount(0);
}

void
RotoDrawableItem::setFeatherFallOff(double f,double time)
{
    _imp->featherFallOff->setValueAtTime(time, f, 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getFeatherFallOff(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->featherFallOff->getValueAtTime(time);
}

#ifdef NATRON_ROTO_INVERTIBLE
bool
RotoDrawableItem::getInverted(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->inverted->getValueAtTime(time);
}

#endif

void
RotoDrawableItem::getColor(double time,
                           double* color) const
{
    color[0] = _imp->color->getValueAtTime(time,0);
    color[1] = _imp->color->getValueAtTime(time,1);
    color[2] = _imp->color->getValueAtTime(time,2);
}

void
RotoDrawableItem::setColor(double time,double r,double g,double b)
{
    _imp->color->setValueAtTime(time, r, 0);
    _imp->color->setValueAtTime(time, g, 1);
    _imp->color->setValueAtTime(time, b, 2);
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
    _imp->compOperator->setValue( op, 0);
}

std::string
RotoDrawableItem::getCompositingOperatorToolTip() const
{
    return _imp->compOperator->getHintToolTipFull();
}

void
RotoDrawableItem::getOverlayColor(double* color) const
{
    QMutexLocker l(&itemMutex);

    memcpy(color, _imp->overlayColor, sizeof(double) * 4);
}

void
RotoDrawableItem::setOverlayColor(const double *color)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&itemMutex);
        memcpy(_imp->overlayColor, color, sizeof(double) * 4);
    }
    Q_EMIT overlayColorChanged();
}

boost::shared_ptr<KnobBool> RotoDrawableItem::getActivatedKnob() const
{
    return _imp->activated;
}

boost::shared_ptr<KnobDouble> RotoDrawableItem::getFeatherKnob() const
{
    return _imp->feather;
}

boost::shared_ptr<KnobDouble> RotoDrawableItem::getFeatherFallOffKnob() const
{
    return _imp->featherFallOff;
}

boost::shared_ptr<KnobDouble> RotoDrawableItem::getOpacityKnob() const
{
    return _imp->opacity;
}

#ifdef NATRON_ROTO_INVERTIBLE
boost::shared_ptr<KnobBool> RotoDrawableItem::getInvertedKnob() const
{
    return _imp->inverted;
}

#endif
boost::shared_ptr<KnobChoice> RotoDrawableItem::getOperatorKnob() const
{
    return _imp->compOperator;
}

boost::shared_ptr<KnobColor> RotoDrawableItem::getColorKnob() const
{
    return _imp->color;
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getBrushSizeKnob() const
{
    return _imp->brushSize;
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getBrushHardnessKnob() const
{
    return _imp->brushHardness;
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getBrushSpacingKnob() const
{
    return _imp->brushSpacing;
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getBrushEffectKnob() const
{
    return _imp->effectStrength;
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getBrushVisiblePortionKnob() const
{
    return _imp->visiblePortion;
}

boost::shared_ptr<KnobBool>
RotoDrawableItem::getPressureOpacityKnob() const
{
    return _imp->pressureOpacity;
}

boost::shared_ptr<KnobBool>
RotoDrawableItem::getPressureSizeKnob() const
{
    return _imp->pressureSize;
}

boost::shared_ptr<KnobBool>
RotoDrawableItem::getPressureHardnessKnob() const
{
    return _imp->pressureHardness;
}

boost::shared_ptr<KnobBool>
RotoDrawableItem::getBuildupKnob() const
{
    return _imp->buildUp;
}

boost::shared_ptr<KnobInt>
RotoDrawableItem::getTimeOffsetKnob() const
{
    return _imp->timeOffset;
}

boost::shared_ptr<KnobChoice>
RotoDrawableItem::getTimeOffsetModeKnob() const
{
    return _imp->timeOffsetMode;
}

boost::shared_ptr<KnobChoice>
RotoDrawableItem::getBrushSourceTypeKnob() const
{
    return _imp->sourceColor;
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getBrushCloneTranslateKnob() const
{
    return _imp->cloneTranslate;
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getCenterKnob() const
{
    return _imp->center;
}

boost::shared_ptr<KnobInt>
RotoDrawableItem::getLifeTimeFrameKnob() const
{
    return _imp->lifeTimeFrame;
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getMotionBlurAmountKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    return _imp->motionBlur;
#else
    return boost::shared_ptr<KnobDouble>();
#endif
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getShutterOffsetKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    return _imp->customOffset;
#else
    return boost::shared_ptr<KnobDouble>();
#endif
}

boost::shared_ptr<KnobDouble>
RotoDrawableItem::getShutterKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    return _imp->shutter;
#else
    return boost::shared_ptr<KnobDouble>();
#endif
}

boost::shared_ptr<KnobChoice>
RotoDrawableItem::getShutterTypeKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    return _imp->shutterType;
#else
    return boost::shared_ptr<KnobChoice>();
#endif
}

void
RotoDrawableItem::setKeyframeOnAllTransformParameters(double time)
{
    _imp->translate->setValueAtTime(time, _imp->translate->getValue(0), 0);
    _imp->translate->setValueAtTime(time, _imp->translate->getValue(1), 1);
    
    _imp->scale->setValueAtTime(time, _imp->scale->getValue(0), 0);
    _imp->scale->setValueAtTime(time, _imp->scale->getValue(1), 1);
    
    _imp->rotate->setValueAtTime(time, _imp->rotate->getValue(0), 0);
    
    _imp->skewX->setValueAtTime(time, _imp->skewX->getValue(0), 0);
    _imp->skewY->setValueAtTime(time, _imp->skewY->getValue(0), 0);
}

const std::list<boost::shared_ptr<KnobI> >&
RotoDrawableItem::getKnobs() const
{
    return _imp->knobs;
}

boost::shared_ptr<KnobI>
RotoDrawableItem::getKnobByName(const std::string& name) const
{
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if ((*it)->getName() == name) {
            return *it;
        }
    }
    return boost::shared_ptr<KnobI>();
}


void
RotoDrawableItem::getTransformAtTime(double time,Transform::Matrix3x3* matrix) const
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
}

/**
 * @brief Set the transform at the given time
 **/
void
RotoDrawableItem::setTransform(double time, double tx, double ty, double sx, double sy, double centerX, double centerY, double rot, double skewX, double skewY)
{
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    
    if (autoKeying) {
        _imp->translate->setValueAtTime(time, tx, 0);
        _imp->translate->setValueAtTime(time, ty, 1);
        
        _imp->scale->setValueAtTime(time, sx, 0);
        _imp->scale->setValueAtTime(time, sy, 1);
        
        _imp->center->setValue(centerX, 0);
        _imp->center->setValue(centerY, 1);
        
        _imp->rotate->setValueAtTime(time, rot, 0);
        
        _imp->skewX->setValueAtTime(time, skewX, 0);
        _imp->skewY->setValueAtTime(time, skewY, 0);
    } else {
        _imp->translate->setValue(tx, 0);
        _imp->translate->setValue(ty, 1);
        
        _imp->scale->setValue(sx, 0);
        _imp->scale->setValue(sy, 1);
        
        _imp->center->setValue(centerX, 0);
        _imp->center->setValue(centerY, 1);
        
        _imp->rotate->setValue(rot, 0);
        
        _imp->skewX->setValue(skewX, 0);
        _imp->skewY->setValue(skewY, 0);

    }
    
    onTransformSet(time);
}

