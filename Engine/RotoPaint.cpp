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

#include "RotoPaint.h"
#include "RotoPaintPrivate.h"

#include <sstream>
#include <cassert>
#include <stdexcept>

#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeMetadata.h"
#include "Engine/MergingEnum.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoUndoCommand.h"
#include "Engine/RotoLayer.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Global/GLIncludes.h"
#include "Global/GlobalDefines.h"

#define kFilterImpulse "Impulse"
#define kFilterImpulseHint "(nearest neighbor / box) Use original values"
#define kFilterBilinear "Bilinear"
#define kFilterBilinearHint "(tent / triangle) Bilinear interpolation between original values"
#define kFilterCubic "Cubic"
#define kFilterCubicHint "(cubic spline) Some smoothing"
#define kFilterKeys "Keys"
#define kFilterKeysHint "(Catmull-Rom / Hermite spline) Some smoothing, plus minor sharpening (*)"
#define kFilterSimon "Simon"
#define kFilterSimonHint "Some smoothing, plus medium sharpening (*)"
#define kFilterRifman "Rifman"
#define kFilterRifmanHint "Some smoothing, plus significant sharpening (*)"
#define kFilterMitchell "Mitchell"
#define kFilterMitchellHint "Some smoothing, plus blurring to hide pixelation (*+)"
#define kFilterParzen "Parzen"
#define kFilterParzenHint "(cubic B-spline) Greatest smoothing of all filters (+)"
#define kFilterNotch "Notch"
#define kFilterNotchHint "Flat smoothing (which tends to hide moire' patterns) (+)"

#define ROTO_DEFAULT_OPACITY 1.
#define ROTO_DEFAULT_FEATHER 1.5
#define ROTO_DEFAULT_FEATHERFALLOFF 1.


#define ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX 5

NATRON_NAMESPACE_ENTER;

static void addPluginShortcuts(const PluginPtr& plugin)
{


    // Viewer buttons
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamAutoKeyingEnabled, kRotoUIParamAutoKeyingEnabledLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamFeatherLinkEnabled, kRotoUIParamFeatherLinkEnabledLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamDisplayFeather, kRotoUIParamDisplayFeatherLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamStickySelectionEnabled, kRotoUIParamStickySelectionEnabledLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamStickyBbox, kRotoUIParamStickyBboxLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRippleEdit, kRotoUIParamRippleEditLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamAddKeyFrame, kRotoUIParamAddKeyFrameLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRemoveKeyframe, kRotoUIParamRemoveKeyframeLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamShowTransform, kRotoUIParamShowTransformLabel, Key_T) );

    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamPressureOpacity, kRotoUIParamPressureOpacityLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamPressureSize, kRotoUIParamPressureSizeLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamPressureHardness, kRotoUIParamPressureHardnessLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamBuildUp, kRotoUIParamBuildUpLabel) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamResetCloneOffset, kRotoUIParamResetCloneOffsetLabel) );

    // Toolbuttons
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamSelectionToolButton, kRotoUIParamSelectionToolButtonLabel, Key_Q) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamEditPointsToolButton, kRotoUIParamEditPointsToolButtonLabel, Key_D) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamBezierEditionToolButton, kRotoUIParamBezierEditionToolButtonLabel, Key_V) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamPaintBrushToolButton, kRotoUIParamPaintBrushToolButtonLabel, Key_N) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamCloneBrushToolButton, kRotoUIParamCloneBrushToolButtonLabel, Key_C) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamEffectBrushToolButton, kRotoUIParamEffectBrushToolButtonLabel, Key_X) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamMergeBrushToolButton, kRotoUIParamMergeBrushToolButtonLabel, Key_E) );

    // Right click actions
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionRemoveItems, kRotoUIParamRightClickMenuActionRemoveItemsLabel, Key_BackSpace) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionCuspItems, kRotoUIParamRightClickMenuActionCuspItemsLabel, Key_Z, eKeyboardModifierShift) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionSmoothItems, kRotoUIParamRightClickMenuActionSmoothItemsLabel, Key_Z) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionRemoveItemsFeather, kRotoUIParamRightClickMenuActionRemoveItemsFeatherLabel, Key_E, eKeyboardModifierShift) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionNudgeLeft, kRotoUIParamRightClickMenuActionNudgeLeftLabel, Key_Left, eKeyboardModifierShift) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionNudgeBottom, kRotoUIParamRightClickMenuActionNudgeBottomLabel, Key_Down, eKeyboardModifierShift) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionNudgeTop, kRotoUIParamRightClickMenuActionNudgeTopLabel, Key_Up, eKeyboardModifierShift) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionNudgeRight, kRotoUIParamRightClickMenuActionNudgeRightLabel, Key_Right, eKeyboardModifierShift) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionSelectAll, kRotoUIParamRightClickMenuActionSelectAllLabel, Key_A, eKeyboardModifierControl) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionOpenClose, kRotoUIParamRightClickMenuActionOpenCloseLabel, Key_Return) );
    plugin->addActionShortcut( PluginActionShortcut(kRotoUIParamRightClickMenuActionLockShapes, kRotoUIParamRightClickMenuActionLockShapesLabel, Key_L, eKeyboardModifierShift) );

} // addPluginShortcuts

PluginPtr
RotoPaint::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_PAINT);
    PluginPtr ret = Plugin::create((void*)RotoPaint::create, PLUGINID_NATRON_ROTOPAINT, "RotoPaint", 1, 0, grouping);

    QString desc = tr("RotoPaint is a vector based free-hand drawing node that helps for tasks such as rotoscoping, matting, etc...");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath, "Images/GroupingIcons/Set2/paint_grouping_2.png");
    ret->setProperty<int>(kNatronPluginPropShortcut, (int)Key_P);
    addPluginShortcuts(ret);
    return ret;
}

PluginPtr
RotoNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_PAINT);
    PluginPtr ret = Plugin::create((void*)RotoNode::create, PLUGINID_NATRON_ROTO, "Roto", 1, 0, grouping);

    QString desc = tr("Create masks and shapes.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath, "Images/rotoNodeIcon.png");
    ret->setProperty<int>(kNatronPluginPropShortcut, (int)Key_O);
    addPluginShortcuts(ret);
    return ret;
}



RotoPaint::RotoPaint(const NodePtr& node,
                     bool isPaintByDefault)
    : NodeGroup(node)
    , _imp( new RotoPaintPrivate(this, isPaintByDefault) )
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

RotoPaint::~RotoPaint()
{
}

bool
RotoPaint::isSubGraphUserVisible() const
{
    return false;
}

bool
RotoPaint::isDefaultBehaviourPaintContext() const
{
    return _imp->isPaintByDefault;
}



bool
RotoPaint::isHostChannelSelectorSupported(bool* defaultR,
                                          bool* defaultG,
                                          bool* defaultB,
                                          bool* defaultA) const
{
    //Use our own selectors, we don't want Natron to copy back channels
    *defaultR = true;
    *defaultG = true;
    *defaultB = true;
    *defaultA = true;

    return false;
}

bool
RotoNode::isHostChannelSelectorSupported(bool* defaultR,
                                         bool* defaultG,
                                         bool* defaultB,
                                         bool* defaultA) const
{
    *defaultR = false;
    *defaultG = false;
    *defaultB = false;
    *defaultA = true;

    return false;
}

NodePtr
RotoPaint::getPremultNode() const
{
    return _imp->premultNode.lock();
}

NodePtr
RotoPaint::getMetadataFixerNode() const
{
    return _imp->premultFixerNode.lock();
}

NodePtr
RotoPaint::getInternalInputNode(int index) const
{
    if (index < 0 || index >= (int)_imp->inputNodes.size()) {
        return NodePtr();
    }
    return _imp->inputNodes[index].lock();
}

void
RotoPaint::getEnabledChannelKnobs(KnobBoolPtr* r,KnobBoolPtr* g, KnobBoolPtr* b, KnobBoolPtr *a) const
{
    *r = _imp->enabledKnobs[0].lock();
    *g = _imp->enabledKnobs[1].lock();
    *b = _imp->enabledKnobs[2].lock();
    *a = _imp->enabledKnobs[3].lock();
}

void
RotoPaint::initGeneralPageKnobs()
{
    EffectInstancePtr effect = shared_from_this();

    KnobPagePtr generalPage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(effect, "generalPage", tr("General"));

    bool isPaintNode = isDefaultBehaviourPaintContext();
    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoOpacityParamLabel), 1);
        param->setHintToolTip( tr(kRotoOpacityHint) );
        param->setName(kRotoOpacityParam);
        param->setRange(0., 1.);
        param->setDisplayRange(0., 1.);
        param->setDefaultValue(ROTO_DEFAULT_OPACITY, DimSpec(0));
        _imp->knobsTable->addPerItemKnobMaster(param);
        generalPage->addKnob(param);
    }

    {
        KnobColorPtr param = AppManager::createKnob<KnobColor>(effect, tr(kRotoColorParamLabel), 3);
        param->setHintToolTip( tr(kRotoColorHint) );
        param->setName(kRotoColorParam);
        std::vector<double> def(3);
        def[0] = def[1] = def[2] = 1.;
        param->setDefaultValues(def, DimIdx(0));
        _imp->knobsTable->addPerItemKnobMaster(param);
        generalPage->addKnob(param);
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoDrawableItemLifeTimeParamLabel), 1);
        param->setHintToolTip( tr(kRotoDrawableItemLifeTimeParamHint) );
        param->setName(kRotoDrawableItemLifeTimeParam);
        param->setAddNewLine(false);
        param->setAnimationEnabled(false);
        {
            std::vector<std::string> choices, helps;
            choices.push_back(kRotoDrawableItemLifeTimeSingle);
            helps.push_back( tr(kRotoDrawableItemLifeTimeSingleHelp).toStdString() );
            choices.push_back(kRotoDrawableItemLifeTimeFromStart);
            helps.push_back( tr(kRotoDrawableItemLifeTimeFromStartHelp).toStdString() );
            choices.push_back(kRotoDrawableItemLifeTimeToEnd);
            helps.push_back( tr(kRotoDrawableItemLifeTimeToEndHelp).toStdString() );
            choices.push_back(kRotoDrawableItemLifeTimeCustom);
            helps.push_back( tr(kRotoDrawableItemLifeTimeCustomHelp).toStdString() );
            param->populateChoices(choices, helps);
        }
        param->setDefaultValue(isPaintNode ? 0 : 3, DimSpec(0));
        _imp->knobsTable->addPerItemKnobMaster(param);
        generalPage->addKnob(param);
        _imp->lifeTimeKnob = param;
    }

    {
        KnobIntPtr param = AppManager::createKnob<KnobInt>(effect, tr(kRotoDrawableItemLifeTimeFrameParamLabel), 1);
        param->setHintToolTip( tr(kRotoDrawableItemLifeTimeFrameParamHint) );
        param->setName(kRotoDrawableItemLifeTimeFrameParam);
        param->setSecret(!isPaintNode);
        param->setAddNewLine(false);
        param->setAnimationEnabled(false);
        _imp->knobsTable->addPerItemKnobMaster(param);
        generalPage->addKnob(param);
        _imp->lifeTimeFrameKnob = param;
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(effect, tr(kRotoActivatedParamLabel), 1);
        param->setHintToolTip( tr(kRotoActivatedHint) );
        param->setName(kRotoActivatedParam);
        param->setAddNewLine(true);
        param->setSecret(isPaintNode);
        param->setDefaultValue(true);
        _imp->knobsTable->addPerItemKnobMaster(param);
        generalPage->addKnob(param);
        _imp->activatedKnob = param;
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(effect, tr(kRotoInvertedParamLabel), 1);
        param->setHintToolTip( tr(kRotoInvertedHint) );
        param->setName(kRotoInvertedParam);
        param->setDefaultValue(false);
        _imp->knobsTable->addPerItemKnobMaster(param);
        generalPage->addKnob(param);
    }
    
} // initGeneralPageKnobs

void
RotoPaint::initShapePageKnobs()
{
    EffectInstancePtr effect = shared_from_this();

    KnobPagePtr shapePage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(effect, "shapePage", tr("Shape"));

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoFeatherParamLabel), 1);
        param->setHintToolTip( tr(kRotoFeatherHint) );
        param->setName(kRotoFeatherParam);
        param->setRange(0, std::numeric_limits<double>::infinity());
        param->setDisplayRange(0, 500);
        param->setDefaultValue(ROTO_DEFAULT_FEATHER);
        _imp->knobsTable->addPerItemKnobMaster(param);
        shapePage->addKnob(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoFeatherFallOffParamLabel), 1);
        param->setHintToolTip( tr(kRotoFeatherFallOffHint) );
        param->setName(kRotoFeatherFallOffParam);
        param->setRange(0.001, 5.);
        param->setDisplayRange(0.2, 5.);
        param->setDefaultValue(ROTO_DEFAULT_FEATHERFALLOFF);
        _imp->knobsTable->addPerItemKnobMaster(param);
        param->setAddNewLine(false);
        shapePage->addKnob(param);
    }


    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoFeatherFallOffTypeLabel), 1);
        param->setHintToolTip( tr(kRotoFeatherFallOffTypeHint) );
        param->setName(kRotoFeatherFallOffType);
        {
            std::vector<std::string> entries,helps;
            entries.push_back(kRotoFeatherFallOffTypeLinear);
            helps.push_back(kRotoFeatherFallOffTypeLinearHint);
            entries.push_back(kRotoFeatherFallOffTypePLinear);
            helps.push_back(kRotoFeatherFallOffTypePLinearHint);
            entries.push_back(kRotoFeatherFallOffTypeEaseIn);
            helps.push_back(kRotoFeatherFallOffTypeEaseInHint);
            entries.push_back(kRotoFeatherFallOffTypeEaseOut);
            helps.push_back(kRotoFeatherFallOffTypeEaseOutHint);
            entries.push_back(kRotoFeatherFallOffTypeSmooth);
            helps.push_back(kRotoFeatherFallOffTypeSmoothHint);
            param->populateChoices(entries, helps);
        }
        shapePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }


} // initShapePageKnobs

void
RotoPaint::initStrokePageKnobs()
{
    EffectInstancePtr effect = shared_from_this();

    KnobPagePtr strokePage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(effect, "strokePage", tr("Stroke"));


    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushSizeParamLabel), 1);
        param->setName(kRotoBrushSizeParam);
        param->setHintToolTip( tr(kRotoBrushSizeParamHint) );
        param->setDefaultValue(25);
        param->setRange(1., 1000);
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushSpacingParamLabel), 1);
        param->setName(kRotoBrushSpacingParam);
        param->setHintToolTip( tr(kRotoBrushSpacingParamHint) );
        param->setDefaultValue(0.1);
        param->setRange(0., 1.);
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushHardnessParamLabel), 1);
        param->setName(kRotoBrushHardnessParam);
        param->setHintToolTip( tr(kRotoBrushHardnessParamHint) );
        param->setDefaultValue(0.2);
        param->setRange(0., 1.);
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushEffectParamLabel), 1);
        param->setName(kRotoBrushEffectParam);
        param->setHintToolTip( tr(kRotoBrushEffectParamHint) );
        param->setDefaultValue(15);
        param->setRange(0., 100.);
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobSeparatorPtr param = AppManager::createKnob<KnobSeparator>( effect, tr(kRotoBrushPressureLabelParamLabel) );
        param->setName(kRotoBrushPressureLabelParam);
        param->setHintToolTip( tr(kRotoBrushPressureLabelParamHint) );
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>( effect, tr(kRotoBrushPressureOpacityParamLabel) );
        param->setName(kRotoBrushPressureOpacityParam);
        param->setHintToolTip( tr(kRotoBrushPressureOpacityParamHint) );
        param->setAnimationEnabled(false);
        param->setDefaultValue(true);
        param->setAddNewLine(false);
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>( effect, tr(kRotoBrushPressureSizeParamLabel) );
        param->setName(kRotoBrushPressureSizeParam);
        param->setHintToolTip( tr(kRotoBrushPressureSizeParamHint) );
        param->setAnimationEnabled(false);
        param->setDefaultValue(false);
        param->setAddNewLine(false);
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>( effect, tr(kRotoBrushPressureHardnessParamLabel) );
        param->setName(kRotoBrushPressureHardnessParam);
        param->setHintToolTip( tr(kRotoBrushPressureHardnessParamHint) );
        param->setAnimationEnabled(false);
        param->setDefaultValue(false);
        param->setAddNewLine(true);
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>( effect, tr(kRotoBrushBuildupParamLabel) );
        param->setName(kRotoBrushBuildupParam);
        param->setHintToolTip( tr(kRotoBrushBuildupParamHint) );
        param->setAnimationEnabled(false);
        param->setDefaultValue(false);
        param->setAddNewLine(true);
        _imp->knobsTable->addPerItemKnobMaster(param);
        strokePage->addKnob(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushVisiblePortionParamLabel), 2);
        param->setName(kRotoBrushVisiblePortionParam);
        param->setHintToolTip( tr(kRotoBrushVisiblePortionParamHint) );
        param->setDefaultValue(0, DimSpec(0));
        param->setDefaultValue(1, DimSpec(1));
        std::vector<double> mins, maxs;
        mins.push_back(0);
        mins.push_back(0);
        maxs.push_back(1);
        maxs.push_back(1);
        param->setRangeAcrossDimensions(mins, maxs);
        strokePage->addKnob(param);
        param->setDimensionName(DimIdx(0), tr("start").toStdString());
        param->setDimensionName(DimIdx(1), tr("end").toStdString());
        _imp->knobsTable->addPerItemKnobMaster(param);
    }
    
} // initStrokePageKnobs

void
RotoPaint::initTransformPageKnobs()
{
    EffectInstancePtr effect = shared_from_this();

    KnobPagePtr transformPage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(effect, "transformPage", tr("Transform"));

    KnobDoublePtr translateKnob, scaleKnob, rotateKnob, skewXKnob, skewYKnob, centerKnob;
    KnobBoolPtr scaleUniformKnob;
    KnobChoicePtr skewOrderKnob;
    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoDrawableItemTranslateParamLabel), 2);
        param->setName(kRotoDrawableItemTranslateParam);
        param->setHintToolTip( tr(kRotoDrawableItemTranslateParamHint) );
        param->setIncrement(10);
        translateKnob = param;
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoDrawableItemRotateParamLabel), 1);
        param->setName(kRotoDrawableItemRotateParam);
        param->setHintToolTip( tr(kRotoDrawableItemRotateParamHint) );
        param->setDisplayRange(-180, 180);
        rotateKnob = param;
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoDrawableItemScaleParamLabel), 2);
        param->setName(kRotoDrawableItemScaleParam);
        param->setHintToolTip( tr(kRotoDrawableItemScaleParamHint) );
        param->setDefaultValue(1, DimSpec(0));
        param->setDefaultValue(1, DimSpec(1));
        param->setDisplayRange(0.1, 10., DimSpec(0));
        param->setDisplayRange(0.1, 10., DimSpec(1));
        param->setAddNewLine(false);
        scaleKnob = param;
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(effect, tr(kRotoDrawableItemScaleUniformParamLabel), 1);
        param->setName(kRotoDrawableItemScaleUniformParam);
        param->setHintToolTip( tr(kRotoDrawableItemScaleUniformParamHint) );
        param->setDefaultValue(true);
        param->setAnimationEnabled(false);
        scaleUniformKnob = param;
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoDrawableItemSkewXParamLabel), 1);
        param->setName(kRotoDrawableItemSkewXParam);
        param->setHintToolTip( tr(kRotoDrawableItemSkewXParamHint) );
        param->setDisplayRange(-1, 1);
        skewXKnob = param;
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }
    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoDrawableItemSkewYParamLabel), 1);
        param->setName(kRotoDrawableItemSkewYParam);
        param->setHintToolTip( tr(kRotoDrawableItemSkewYParamHint) );
        param->setDisplayRange(-1, 1);
        skewYKnob = param;
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoDrawableItemSkewOrderParamLabel), 1);
        param->setName(kRotoDrawableItemSkewOrderParam);
        param->setHintToolTip( tr(kRotoDrawableItemSkewOrderParamHint) );
        param->setDefaultValue(0);
        {
            std::vector<std::string> choices;
            choices.push_back("XY");
            choices.push_back("YX");
            param->populateChoices(choices);
        }
        param->setAnimationEnabled(false);
        skewOrderKnob = param;
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoDrawableItemCenterParamLabel), 2);
        param->setName(kRotoDrawableItemCenterParam);
        param->setHintToolTip( tr(kRotoDrawableItemCenterParamHint) );
        param->setDefaultValuesAreNormalized(true);
        param->setAddNewLine(false);
        param->setDefaultValue(0.5, DimSpec(0));
        param->setDefaultValue(0.5, DimSpec(1));
        centerKnob = param;
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(effect, tr(kRotoResetCenterParamLabel), 1);
        param->setName(kRotoResetCenterParam);
        param->setHintToolTip( tr(kRotoResetCenterParamHint) );
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
        _imp->resetCenterKnob = param;
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(effect, tr(kRotoTransformInteractiveLabel), 1);
        param->setName(kRotoTransformInteractive);
        param->setHintToolTip(tr(kRotoTransformInteractiveHint));
        param->setDefaultValue(true);
        transformPage->addKnob(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoDrawableItemExtraMatrixParamLabel), 9);
        param->setName(kRotoDrawableItemExtraMatrixParam);
        param->setHintToolTip( tr(kRotoDrawableItemExtraMatrixParamHint) );
        // Set to identity
        param->setDefaultValue(1, DimSpec(0));
        param->setDefaultValue(1, DimSpec(4));
        param->setDefaultValue(1, DimSpec(8));
        transformPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(effect, tr(kRotoResetTransformParamLabel), 1);
        param->setName(kRotoResetTransformParam);
        param->setHintToolTip( tr(kRotoResetTransformParamHint) );
        transformPage->addKnob(param);
        _imp->resetTransformKnob = param;
    }

    getNode()->addTransformInteract(translateKnob,
                                    scaleKnob,
                                    scaleUniformKnob,
                                    rotateKnob,
                                    skewXKnob,
                                    skewYKnob,
                                    skewOrderKnob,
                                    centerKnob,
                                    KnobBoolPtr() /*invert*/,
                                    KnobBoolPtr() /*interactive*/);

} // initTransformPageKnobs

void
RotoPaint::initClonePageKnobs()
{
    EffectInstancePtr effect = shared_from_this();

    KnobPagePtr clonePage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(effect, "clonePage", tr("Clone"));
    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoBrushSourceColorLabel), 1);
        param->setName(kRotoBrushSourceColor);
        param->setHintToolTip( tr(kRotoBrushSourceColorHint) );
        param->setDefaultValue(1);
        {
            std::vector<std::string> choices;
            choices.push_back("foreground");
            choices.push_back("background");
            for (int i = 1; i < 10; ++i) {
                std::stringstream ss;
                ss << "background " << i + 1;
                choices.push_back( ss.str() );
            }
            param->populateChoices(choices);
        }
        clonePage->addKnob(param);
    }

    KnobDoublePtr cloneTranslateKnob, cloneRotateKnob, cloneScaleKnob, cloneSkewXKnob, cloneSkewYKnob, cloneCenterKnob;
    KnobBoolPtr cloneScaleUniformKnob;
    KnobChoicePtr cloneSkewOrderKnob;
    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushTranslateParamLabel), 2);
        param->setName(kRotoBrushTranslateParam);
        param->setHintToolTip( tr(kRotoBrushTranslateParamHint) );
        param->setIncrement(10);
        cloneTranslateKnob = param;
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushRotateParamLabel), 1);
        param->setName(kRotoBrushRotateParam);
        param->setHintToolTip( tr(kRotoBrushRotateParamHint) );
        param->setDisplayRange(-180, 180);
        cloneRotateKnob = param;
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushScaleParamLabel), 2);
        param->setName(kRotoBrushScaleParam);
        param->setHintToolTip( tr(kRotoBrushScaleParamHint) );
        param->setDefaultValue(1, DimSpec(0));
        param->setDefaultValue(1, DimSpec(1));
        param->setDisplayRange(0.1, 10., DimSpec(0));
        param->setDisplayRange(0.1, 10., DimSpec(1));
        param->setAddNewLine(false);
        cloneScaleKnob = param;
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(effect, tr(kRotoBrushScaleUniformParamLabel), 1);
        param->setName(kRotoBrushScaleUniformParam);
        param->setHintToolTip( tr(kRotoBrushScaleUniformParamHint) );
        param->setDefaultValue(true);
        param->setAnimationEnabled(false);
        cloneScaleUniformKnob = param;
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushSkewXParamLabel), 1);
        param->setName(kRotoBrushSkewXParam);
        param->setHintToolTip( tr(kRotoBrushSkewXParamHint) );
        param->setDisplayRange(-1, 1, DimSpec(0));
        cloneSkewXKnob = param;
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushSkewYParamLabel), 1);
        param->setName(kRotoBrushSkewYParam);
        param->setHintToolTip( tr(kRotoBrushSkewYParamHint) );
        param->setDisplayRange(-1, 1);
        cloneSkewYKnob = param;
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoBrushSkewOrderParamLabel), 1);
        param->setName(kRotoBrushSkewOrderParam);
        param->setHintToolTip( tr(kRotoBrushSkewOrderParamHint) );
        param->setDefaultValue(0);
        {
            std::vector<std::string> choices;
            choices.push_back("XY");
            choices.push_back("YX");
            param->populateChoices(choices);
        }
        param->setAnimationEnabled(false);
        cloneSkewOrderKnob = param;
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoBrushCenterParamLabel), 2);
        param->setName(kRotoBrushCenterParam);
        param->setHintToolTip( tr(kRotoBrushCenterParamHint) );
        param->setDefaultValuesAreNormalized(true);
        param->setAddNewLine(false);
        param->setDefaultValue(0.5, DimSpec(0));
        param->setDefaultValue(0.5, DimSpec(1));
        cloneCenterKnob = param;
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(effect, tr(kRotoResetCloneCenterParamLabel), 1);
        param->setName(kRotoResetCloneCenterParam);
        param->setHintToolTip( tr(kRotoResetCloneCenterParamHint) );
        clonePage->addKnob(param);
        _imp->resetCloneCenterKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(effect, tr(kRotoResetCloneTransformParamLabel), 1);
        param->setName(kRotoResetCloneTransformParam);
        param->setHintToolTip( tr(kRotoResetCloneTransformParamHint) );
        clonePage->addKnob(param);
        _imp->resetCloneTransformKnob = param;

    }


    getNode()->addTransformInteract(cloneTranslateKnob,
                                    cloneScaleKnob,
                                    cloneScaleUniformKnob,
                                    cloneRotateKnob,
                                    cloneSkewXKnob,
                                    cloneSkewYKnob,
                                    cloneSkewOrderKnob,
                                    cloneCenterKnob,
                                    KnobBoolPtr() /*invert*/,
                                    KnobBoolPtr() /*interactive*/);

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoBrushFilterParamLabel), 1);
        param->setName(kRotoBrushFilterParam);
        param->setHintToolTip( tr(kRotoBrushFilterParamHint) );
        {
            std::vector<std::string> choices, helps;

            choices.push_back(kFilterImpulse);
            helps.push_back(kFilterImpulseHint);
            choices.push_back(kFilterBilinear);
            helps.push_back(kFilterBilinearHint);
            choices.push_back(kFilterCubic);
            helps.push_back(kFilterCubicHint);
            choices.push_back(kFilterKeys);
            helps.push_back(kFilterKeysHint);
            choices.push_back(kFilterSimon);
            helps.push_back(kFilterSimonHint);
            choices.push_back(kFilterRifman);
            helps.push_back(kFilterRifmanHint);
            choices.push_back(kFilterMitchell);
            helps.push_back(kFilterMitchellHint);
            choices.push_back(kFilterParzen);
            helps.push_back(kFilterParzenHint);
            choices.push_back(kFilterNotch);
            helps.push_back(kFilterNotchHint);
            param->populateChoices(choices);
        }
        param->setDefaultValue(2);
        param->setAddNewLine(false);
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(effect, tr(kRotoBrushBlackOutsideParamLabel), 1);
        param->setName(kRotoBrushBlackOutsideParam);
        param->setHintToolTip( tr(kRotoBrushBlackOutsideParamHint) );
        param->setDefaultValue(true);
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobIntPtr param = AppManager::createKnob<KnobInt>(effect, tr(kRotoBrushTimeOffsetParamLabel), 1);
        param->setName(kRotoBrushTimeOffsetParam);
        param->setHintToolTip( tr(kRotoBrushTimeOffsetParamHint) );
        param->setDisplayRange(-100, 100);
        param->setAddNewLine(false);
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoBrushTimeOffsetModeParamLabel), 1);
        param->setName(kRotoBrushTimeOffsetModeParam);
        param->setHintToolTip( tr(kRotoBrushTimeOffsetModeParamHint) );
        {
            std::vector<std::string> modes;
            modes.push_back("Relative");
            modes.push_back("Absolute");
            param->populateChoices(modes);
        }
        clonePage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
    }
    
} // initClonePageKnobs

void
RotoPaint::initMotionBlurPageKnobs()
{
    EffectInstancePtr effect = shared_from_this();

    KnobPagePtr mbPage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(effect, "motionBlurPage", tr("Motion Blur"));

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoMotionBlurModeParamLabel), 1);
        param->setName(kRotoMotionBlurModeParam);
        param->setHintToolTip( tr(kRotoMotionBlurModeParamHint) );
        param->setAnimationEnabled(false);
        {
            std::vector<std::string> entries;
            entries.push_back("Per-Shape");
            entries.push_back("Global");
            param->populateChoices(entries);
        }
        mbPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
        _imp->motionBlurTypeKnob =  param;
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoMotionBlurParamLabel), 1);
        param->setName(kRotoPerShapeMotionBlurParam);
        param->setHintToolTip( tr(kRotoMotionBlurParamHint) );
        param->setDefaultValue(0);
        param->setRange(0, 4);
        param->setDisplayRange(0, 4);
        mbPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
        _imp->motionBlurKnob = param;
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoShutterParamLabel), 1);
        param->setName(kRotoPerShapeShutterParam);
        param->setHintToolTip( tr(kRotoShutterParamHint) );
        param->setDefaultValue(0.5);
        param->setRange(0, 2);
        param->setDisplayRange(0, 2);
        mbPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
        _imp->shutterKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoShutterOffsetTypeParamLabel), 1);
        param->setName(kRotoPerShapeShutterOffsetTypeParam);
        param->setHintToolTip( tr(kRotoShutterOffsetTypeParamHint) );
        param->setDefaultValue(0);
        {
            std::vector<std::string> options, helps;
            options.push_back("Centered");
            helps.push_back(kRotoShutterOffsetCenteredHint);
            options.push_back("Start");
            helps.push_back(kRotoShutterOffsetStartHint);
            options.push_back("End");
            helps.push_back(kRotoShutterOffsetEndHint);
            options.push_back("Custom");
            helps.push_back(kRotoShutterOffsetCustomHint);
            param->populateChoices(options, helps);
        }
        param->setAddNewLine(false);
        mbPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
        _imp->shutterTypeKnob = param;
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoShutterCustomOffsetParamLabel), 1);
        param->setName(kRotoPerShapeShutterCustomOffsetParam);
        param->setHintToolTip( tr(kRotoShutterCustomOffsetParamHint) );
        param->setDefaultValue(0);
        mbPage->addKnob(param);
        _imp->knobsTable->addPerItemKnobMaster(param);
        _imp->customOffsetKnob = param;
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoMotionBlurParamLabel), 1);
        param->setName(kRotoGlobalMotionBlurParam);
        param->setHintToolTip( tr(kRotoMotionBlurParamHint) );
        param->setDefaultValue(0);
        param->setRange(0, 4);
        param->setDisplayRange(0, 4);
        param->setSecret(true);
        mbPage->addKnob(param);
        _imp->globalMotionBlurKnob = param;
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoShutterParamLabel), 1);
        param->setName(kRotoGlobalShutterParam);
        param->setHintToolTip( tr(kRotoShutterParamHint) );
        param->setDefaultValue(0.5);
        param->setRange(0, 2);
        param->setDisplayRange(0, 2);
        param->setSecret(true);
        mbPage->addKnob(param);
        _imp->globalShutterKnob = param;
    }

    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>(effect, tr(kRotoShutterOffsetTypeParamLabel), 1);
        param->setName(kRotoGlobalShutterOffsetTypeParam);
        param->setHintToolTip( tr(kRotoShutterOffsetTypeParamHint) );
        param->setDefaultValue(0);
        {
            std::vector<std::string> options, helps;
            options.push_back("Centered");
            helps.push_back(kRotoShutterOffsetCenteredHint);
            options.push_back("Start");
            helps.push_back(kRotoShutterOffsetStartHint);
            options.push_back("End");
            helps.push_back(kRotoShutterOffsetEndHint);
            options.push_back("Custom");
            helps.push_back(kRotoShutterOffsetCustomHint);
            param->populateChoices(options, helps);
        }
        param->setAddNewLine(false);
        param->setSecret(true);
        mbPage->addKnob(param);
        _imp->globalShutterTypeKnob = param;
    }

    {
        KnobDoublePtr param = AppManager::createKnob<KnobDouble>(effect, tr(kRotoShutterCustomOffsetParamLabel), 1);
        param->setName(kRotoGlobalShutterCustomOffsetParam);
        param->setHintToolTip( tr(kRotoShutterCustomOffsetParamHint) );
        param->setDefaultValue(0);
        param->setSecret(true);
        mbPage->addKnob(param);
        _imp->globalCustomOffsetKnob = param;
    }
    
} // void initMotionBlurPageKnobs();

void
RotoPaint::setupInitialSubGraphState()
{
    RotoPaintPtr thisShared = boost::dynamic_pointer_cast<RotoPaint>(shared_from_this());
    for (int i = 0; i < ROTOPAINT_MAX_INPUTS_COUNT; ++i) {

        std::stringstream ss;
        if (i == 0) {
            ss << "Bg";
        } else if (i == ROTOPAINT_MASK_INPUT_INDEX) {
            ss << "Mask";
        } else {
            ss << "Bg" << i + 1;
        }
        {
            CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, thisShared));
            args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
            args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
            args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, ss.str());
            args->addParamDefaultValue<bool>(kNatronGroupInputIsOptionalParamName, true);
            if (i == ROTOPAINT_MASK_INPUT_INDEX) {
                args->addParamDefaultValue<bool>(kNatronGroupInputIsMaskParamName, true);

            }
            NodePtr input = getApp()->createNode(args);
            assert(input);
            _imp->inputNodes.push_back(input);
        }
    }
    NodePtr outputNode;
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_OUTPUT, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "Output");

        outputNode = getApp()->createNode(args);
        assert(outputNode);
    }
    NodePtr premultNode;
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_OFX_PREMULT, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        // Set premult node to be identity by default
        args->addParamDefaultValue<bool>(kNatronOfxParamProcessR, false);
        args->addParamDefaultValue<bool>(kNatronOfxParamProcessG, false);
        args->addParamDefaultValue<bool>(kNatronOfxParamProcessB, false);
        args->addParamDefaultValue<bool>(kNatronOfxParamProcessA, false);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "AlphaPremult");

        premultNode = getApp()->createNode(args);
        _imp->premultNode = premultNode;

        KnobBoolPtr disablePremultKnob = premultNode->getDisabledKnob();
        try {
            disablePremultKnob->setExpression(DimSpec::all(), ViewSetSpec::all(), "not thisGroup.premultiply.get()", false, true);
        } catch (...) {
            assert(false);
        }

    }

    // Make a no-op that fixes the output premultiplication state
    NodePtr noopNode;
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_OFX_NOOP, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        // Set premult node to be identity by default
        args->addParamDefaultValue<bool>("setPremult", true);
        noopNode = getApp()->createNode(args);
        _imp->premultFixerNode = noopNode;

        KnobIPtr premultChoiceKnob = noopNode->getKnobByName("outputPremult");
        try {
            const char* premultChoiceExpr =
            "premultChecked = thisGroup.premultiply.get()\n"
            "rChecked = thisGroup.doRed.get()\n"
            "gChecked = thisGroup.doGreen.get()\n"
            "bChecked = thisGroup.doBlue.get()\n"
            "aChecked = thisGroup.doAlpha.get()\n"
            "hasColor = rChecked or gChecked or bChecked\n"
            "ret = 0\n"
            "if premultChecked or hasColor or not aChecked:\n"
            "    ret = 1\n" // premult if there's one of RGB checked or none
            "else:\n"
            "    ret = 2\n"
            ;
            premultChoiceKnob->setExpression(DimSpec::all(), ViewSetSpec::all(), premultChoiceExpr, true, true);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            assert(false);
        }

    }
    noopNode->connectInput(premultNode, 0);
    premultNode->connectInput(noopNode, 0);

    // Initialize default connections
    outputNode->connectInput(_imp->inputNodes[0].lock(), 0);
}

void
RotoPaint::initializeKnobs()
{
    RotoPaintPtr thisShared = toRotoPaint(shared_from_this());
    int colsCount = 6;
    _imp->knobsTable.reset(new RotoPaintKnobItemsTable(_imp.get(), KnobItemsTable::eKnobItemsTableTypeTree, colsCount));

    //This page is created in the RotoContext, before initializeKnobs() is called.
    KnobPagePtr generalPage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(thisShared, "generalPage", tr("General"));

    assert(generalPage);


    initGeneralPageKnobs();
    initShapePageKnobs();
    initStrokePageKnobs();
    initTransformPageKnobs();
    initClonePageKnobs();
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    initMotionBlurPageKnobs();
#endif


    KnobSeparatorPtr sep = AppManager::createKnob<KnobSeparator>(thisShared, tr("Output"), 1, false);
    sep->setName("outputSeparator");
    generalPage->addKnob(sep);


    std::string channelNames[4] = {"doRed", "doGreen", "doBlue", "doAlpha"};
    std::string channelLabels[4] = {"R", "G", "B", "A"};
    bool defaultValues[4];
    bool channelSelectorSupported = isHostChannelSelectorSupported(&defaultValues[0], &defaultValues[1], &defaultValues[2], &defaultValues[3]);
    Q_UNUSED(channelSelectorSupported);

    for (int i = 0; i < 4; ++i) {
        KnobBoolPtr enabled =  AppManager::createKnob<KnobBool>(thisShared, channelLabels[i], 1, false);
        enabled->setName(channelNames[i]);
        enabled->setAnimationEnabled(false);
        enabled->setAddNewLine(i == 3);
        enabled->setDefaultValue(defaultValues[i]);
        enabled->setHintToolTip( tr("Enable drawing onto this channel") );
        generalPage->addKnob(enabled);
        _imp->enabledKnobs[i] = enabled;
    }


    KnobBoolPtr premultKnob = AppManager::createKnob<KnobBool>(thisShared, tr("Premultiply"), 1, false);
    premultKnob->setName("premultiply");
    premultKnob->setHintToolTip( tr("When checked, the red, green and blue channels of the output are premultiplied by the alpha channel.\n"
                                    "This will result in the pixels outside of the shapes and paint strokes being black and transparent.\n"
                                    "This should only be used if all the inputs are Opaque or UnPremultiplied, and only the Alpha channel "
                                    "is selected to be drawn by this node.") );
    premultKnob->setDefaultValue(false);
    premultKnob->setAnimationEnabled(false);
    premultKnob->setIsMetadataSlave(true);
    _imp->premultKnob = premultKnob;
    generalPage->addKnob(premultKnob);

    QObject::connect( context.get(), SIGNAL(selectionChanged(int)), this, SLOT(onSelectionChanged(int)) );
    QObject::connect( context.get(), SIGNAL(itemLockedChanged(int)), this, SLOT(onCurveLockedChanged(int)) );


    /// Initializing the viewer interface
    KnobButtonPtr autoKeyingEnabled = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamAutoKeyingEnabledLabel) );
    autoKeyingEnabled->setName(kRotoUIParamAutoKeyingEnabled);
    autoKeyingEnabled->setHintToolTip( tr(kRotoUIParamAutoKeyingEnabledHint) );
    autoKeyingEnabled->setEvaluateOnChange(false);
    autoKeyingEnabled->setCheckable(true);
    autoKeyingEnabled->setDefaultValue(true);
    autoKeyingEnabled->setSecret(true);
    autoKeyingEnabled->setInViewerContextCanHaveShortcut(true);
    autoKeyingEnabled->setIconLabel(NATRON_IMAGES_PATH "autoKeyingEnabled.png", true);
    autoKeyingEnabled->setIconLabel(NATRON_IMAGES_PATH "autoKeyingDisabled.png", false);
    generalPage->addKnob(autoKeyingEnabled);
    _imp->ui->autoKeyingEnabledButton = autoKeyingEnabled;

    KnobButtonPtr featherLinkEnabled = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamFeatherLinkEnabledLabel) );
    featherLinkEnabled->setName(kRotoUIParamFeatherLinkEnabled);
    featherLinkEnabled->setCheckable(true);
    featherLinkEnabled->setEvaluateOnChange(false);
    featherLinkEnabled->setDefaultValue(true);
    featherLinkEnabled->setSecret(true);
    featherLinkEnabled->setInViewerContextCanHaveShortcut(true);
    featherLinkEnabled->setHintToolTip( tr(kRotoUIParamFeatherLinkEnabledHint) );
    featherLinkEnabled->setIconLabel(NATRON_IMAGES_PATH "featherLinkEnabled.png", true);
    featherLinkEnabled->setIconLabel(NATRON_IMAGES_PATH "featherLinkDisabled.png", false);
    generalPage->addKnob(featherLinkEnabled);
    _imp->ui->featherLinkEnabledButton = featherLinkEnabled;

    KnobButtonPtr displayFeatherEnabled = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamDisplayFeatherLabel) );
    displayFeatherEnabled->setName(kRotoUIParamDisplayFeather);
    displayFeatherEnabled->setHintToolTip( tr(kRotoUIParamDisplayFeatherHint) );
    displayFeatherEnabled->setEvaluateOnChange(false);
    displayFeatherEnabled->setCheckable(true);
    displayFeatherEnabled->setDefaultValue(true);
    displayFeatherEnabled->setSecret(true);
    displayFeatherEnabled->setInViewerContextCanHaveShortcut(true);
    addOverlaySlaveParam(displayFeatherEnabled);
    displayFeatherEnabled->setIconLabel(NATRON_IMAGES_PATH "featherEnabled.png", true);
    displayFeatherEnabled->setIconLabel(NATRON_IMAGES_PATH "featherDisabled.png", false);
    generalPage->addKnob(displayFeatherEnabled);
    _imp->ui->displayFeatherEnabledButton = displayFeatherEnabled;

    KnobButtonPtr stickySelection = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamStickySelectionEnabledLabel) );
    stickySelection->setName(kRotoUIParamStickySelectionEnabled);
    stickySelection->setHintToolTip( tr(kRotoUIParamStickySelectionEnabledHint) );
    stickySelection->setEvaluateOnChange(false);
    stickySelection->setCheckable(true);
    stickySelection->setDefaultValue(false);
    stickySelection->setSecret(true);
    stickySelection->setInViewerContextCanHaveShortcut(true);
    stickySelection->setIconLabel(NATRON_IMAGES_PATH "stickySelectionEnabled.png", true);
    stickySelection->setIconLabel(NATRON_IMAGES_PATH "stickySelectionDisabled.png", false);
    generalPage->addKnob(stickySelection);
    _imp->ui->stickySelectionEnabledButton = stickySelection;

    KnobButtonPtr bboxClickAnywhere = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamStickyBboxLabel) );
    bboxClickAnywhere->setName(kRotoUIParamStickyBbox);
    bboxClickAnywhere->setHintToolTip( tr(kRotoUIParamStickyBboxHint) );
    bboxClickAnywhere->setEvaluateOnChange(false);
    bboxClickAnywhere->setCheckable(true);
    bboxClickAnywhere->setDefaultValue(false);
    bboxClickAnywhere->setSecret(true);
    bboxClickAnywhere->setInViewerContextCanHaveShortcut(true);
    bboxClickAnywhere->setIconLabel(NATRON_IMAGES_PATH "viewer_roiEnabled.png", true);
    bboxClickAnywhere->setIconLabel(NATRON_IMAGES_PATH "viewer_roiDisabled.png", false);
    generalPage->addKnob(bboxClickAnywhere);
    _imp->ui->bboxClickAnywhereButton = bboxClickAnywhere;

    KnobButtonPtr rippleEditEnabled = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRippleEditLabel) );
    rippleEditEnabled->setName(kRotoUIParamRippleEdit);
    rippleEditEnabled->setHintToolTip( tr(kRotoUIParamRippleEditLabelHint) );
    rippleEditEnabled->setEvaluateOnChange(false);
    rippleEditEnabled->setCheckable(true);
    rippleEditEnabled->setDefaultValue(false);
    rippleEditEnabled->setSecret(true);
    rippleEditEnabled->setInViewerContextCanHaveShortcut(true);
    rippleEditEnabled->setIconLabel(NATRON_IMAGES_PATH "rippleEditEnabled.png", true);
    rippleEditEnabled->setIconLabel(NATRON_IMAGES_PATH "rippleEditDisabled.png", false);
    generalPage->addKnob(rippleEditEnabled);
    _imp->ui->rippleEditEnabledButton = rippleEditEnabled;

    KnobButtonPtr addKeyframe = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamAddKeyFrameLabel) );
    addKeyframe->setName(kRotoUIParamAddKeyFrame);
    addKeyframe->setEvaluateOnChange(false);
    addKeyframe->setHintToolTip( tr(kRotoUIParamAddKeyFrameHint) );
    addKeyframe->setSecret(true);
    addKeyframe->setInViewerContextCanHaveShortcut(true);
    addKeyframe->setIconLabel(NATRON_IMAGES_PATH "addKF.png");
    generalPage->addKnob(addKeyframe);
    _imp->ui->addKeyframeButton = addKeyframe;

    KnobButtonPtr removeKeyframe = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRemoveKeyframeLabel) );
    removeKeyframe->setName(kRotoUIParamRemoveKeyframe);
    removeKeyframe->setHintToolTip( tr(kRotoUIParamRemoveKeyframeHint) );
    removeKeyframe->setEvaluateOnChange(false);
    removeKeyframe->setSecret(true);
    removeKeyframe->setInViewerContextCanHaveShortcut(true);
    removeKeyframe->setIconLabel(NATRON_IMAGES_PATH "removeKF.png");
    generalPage->addKnob(removeKeyframe);
    _imp->ui->removeKeyframeButton = removeKeyframe;

    KnobButtonPtr showTransform = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamShowTransformLabel) );
    showTransform->setName(kRotoUIParamShowTransform);
    showTransform->setHintToolTip( tr(kRotoUIParamShowTransformHint) );
    showTransform->setEvaluateOnChange(false);
    showTransform->setSecret(true);
    showTransform->setCheckable(true);
    showTransform->setDefaultValue(true);
    showTransform->setInViewerContextCanHaveShortcut(true);
    showTransform->setInViewerContextLayoutType(eViewerContextLayoutTypeAddNewLine);
    showTransform->setIconLabel(NATRON_IMAGES_PATH "rotoShowTransformOverlay.png", true);
    showTransform->setIconLabel(NATRON_IMAGES_PATH "rotoHideTransformOverlay.png", false);
    generalPage->addKnob(showTransform);
    addOverlaySlaveParam(showTransform);
    _imp->ui->showTransformHandle = showTransform;

    // RotoPaint

    KnobBoolPtr multiStroke = AppManager::createKnob<KnobBool>( thisShared, tr(kRotoUIParamMultiStrokeEnabledLabel) );
    multiStroke->setName(kRotoUIParamMultiStrokeEnabled);
    multiStroke->setInViewerContextLabel( tr(kRotoUIParamMultiStrokeEnabledLabel) );
    multiStroke->setHintToolTip( tr(kRotoUIParamMultiStrokeEnabledHint) );
    multiStroke->setEvaluateOnChange(false);
    multiStroke->setDefaultValue(true);
    multiStroke->setSecret(true);
    generalPage->addKnob(multiStroke);
    _imp->ui->multiStrokeEnabled = multiStroke;

    KnobColorPtr colorWheel = AppManager::createKnob<KnobColor>(thisShared, tr(kRotoUIParamColorWheelLabel), 3);
    colorWheel->setName(kRotoUIParamColorWheel);
    colorWheel->setHintToolTip( tr(kRotoUIParamColorWheelHint) );
    colorWheel->setEvaluateOnChange(false);
    colorWheel->setDefaultValue(1., DimIdx(0));
    colorWheel->setDefaultValue(1., DimIdx(1));
    colorWheel->setDefaultValue(1., DimIdx(2));
    colorWheel->setSecret(true);
    generalPage->addKnob(colorWheel);
    _imp->ui->colorWheelButton = colorWheel;

    KnobChoicePtr blendingModes = AppManager::createKnob<KnobChoice>( thisShared, tr(kRotoUIParamBlendingOpLabel) );
    blendingModes->setName(kRotoUIParamBlendingOp);
    blendingModes->setHintToolTip( tr(kRotoUIParamBlendingOpHint) );
    blendingModes->setEvaluateOnChange(false);
    blendingModes->setSecret(true);
    {
        std::vector<std::string> choices, helps;
        Merge::getOperatorStrings(&choices, &helps);
        blendingModes->populateChoices(choices, helps);
    }
    blendingModes->setDefaultValue( (int)eMergeOver );
    generalPage->addKnob(blendingModes);
    _imp->ui->compositingOperatorChoice = blendingModes;


    KnobDoublePtr opacityKnob = AppManager::createKnob<KnobDouble>( thisShared, tr(kRotoUIParamOpacityLabel) );
    opacityKnob->setName(kRotoUIParamOpacity);
    opacityKnob->setInViewerContextLabel( tr(kRotoUIParamOpacityLabel) );
    opacityKnob->setHintToolTip( tr(kRotoUIParamOpacityHint) );
    opacityKnob->setEvaluateOnChange(false);
    opacityKnob->setSecret(true);
    opacityKnob->setDefaultValue(1.);
    opacityKnob->setRange(0., 1.);
    opacityKnob->disableSlider();
    generalPage->addKnob(opacityKnob);
    _imp->ui->opacitySpinbox = opacityKnob;

    KnobButtonPtr pressureOpacity = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamPressureOpacityLabel) );
    pressureOpacity->setName(kRotoUIParamPressureOpacity);
    pressureOpacity->setHintToolTip( tr(kRotoUIParamPressureOpacityHint) );
    pressureOpacity->setEvaluateOnChange(false);
    pressureOpacity->setCheckable(true);
    pressureOpacity->setDefaultValue(true);
    pressureOpacity->setSecret(true);
    pressureOpacity->setInViewerContextCanHaveShortcut(true);
    pressureOpacity->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_on.png", true);
    pressureOpacity->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_off.png", false);
    generalPage->addKnob(pressureOpacity);
    _imp->ui->pressureOpacityButton = pressureOpacity;

    KnobDoublePtr sizeKnob = AppManager::createKnob<KnobDouble>( thisShared, tr(kRotoUIParamSizeLabel) );
    sizeKnob->setName(kRotoUIParamSize);
    sizeKnob->setInViewerContextLabel( tr(kRotoUIParamSizeLabel) );
    sizeKnob->setHintToolTip( tr(kRotoUIParamSizeHint) );
    sizeKnob->setEvaluateOnChange(false);
    sizeKnob->setSecret(true);
    sizeKnob->setDefaultValue(25.);
    sizeKnob->setRange(0., 1000.);
    sizeKnob->disableSlider();
    generalPage->addKnob(sizeKnob);
    _imp->ui->sizeSpinbox = sizeKnob;

    KnobButtonPtr pressureSize = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamPressureSizeLabel) );
    pressureSize->setName(kRotoUIParamPressureSize);
    pressureSize->setHintToolTip( tr(kRotoUIParamPressureSizeHint) );
    pressureSize->setEvaluateOnChange(false);
    pressureSize->setCheckable(true);
    pressureSize->setDefaultValue(false);
    pressureSize->setSecret(true);
    pressureSize->setInViewerContextCanHaveShortcut(true);
    pressureSize->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_on.png", true);
    pressureSize->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_off.png", false);
    generalPage->addKnob(pressureSize);
    _imp->ui->pressureSizeButton = pressureSize;

    KnobDoublePtr hardnessKnob = AppManager::createKnob<KnobDouble>( thisShared, tr(kRotoUIParamHardnessLabel) );
    hardnessKnob->setName(kRotoUIParamHardness);
    hardnessKnob->setInViewerContextLabel( tr(kRotoUIParamHardnessLabel) );
    hardnessKnob->setHintToolTip( tr(kRotoUIParamHardnessHint) );
    hardnessKnob->setEvaluateOnChange(false);
    hardnessKnob->setSecret(true);
    hardnessKnob->setDefaultValue(.2);
    hardnessKnob->setRange(0., 1.);
    hardnessKnob->disableSlider();
    generalPage->addKnob(hardnessKnob);
    _imp->ui->hardnessSpinbox = hardnessKnob;

    KnobButtonPtr pressureHardness = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamPressureHardnessLabel) );
    pressureHardness->setName(kRotoUIParamPressureHardness);
    pressureHardness->setHintToolTip( tr(kRotoUIParamPressureHardnessHint) );
    pressureHardness->setEvaluateOnChange(false);
    pressureHardness->setCheckable(true);
    pressureHardness->setDefaultValue(false);
    pressureHardness->setSecret(true);
    pressureHardness->setInViewerContextCanHaveShortcut(true);
    pressureHardness->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_on.png", true);
    pressureHardness->setIconLabel(NATRON_IMAGES_PATH "rotopaint_pressure_off.png", false);
    generalPage->addKnob(pressureHardness);
    _imp->ui->pressureHardnessButton = pressureHardness;

    KnobButtonPtr buildUp = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamBuildUpLabel) );
    buildUp->setName(kRotoUIParamBuildUp);
    buildUp->setInViewerContextLabel( tr(kRotoUIParamBuildUpLabel) );
    buildUp->setHintToolTip( tr(kRotoUIParamBuildUpHint) );
    buildUp->setEvaluateOnChange(false);
    buildUp->setCheckable(true);
    buildUp->setDefaultValue(true);
    buildUp->setSecret(true);
    buildUp->setInViewerContextCanHaveShortcut(true);
    buildUp->setIconLabel(NATRON_IMAGES_PATH "rotopaint_buildup_on.png", true);
    buildUp->setIconLabel(NATRON_IMAGES_PATH "rotopaint_buildup_off.png", false);
    generalPage->addKnob(buildUp);
    _imp->ui->buildUpButton = buildUp;

    KnobDoublePtr effectStrength = AppManager::createKnob<KnobDouble>( thisShared, tr(kRotoUIParamEffectLabel) );
    effectStrength->setName(kRotoUIParamEffect);
    effectStrength->setInViewerContextLabel( tr(kRotoUIParamEffectLabel) );
    effectStrength->setHintToolTip( tr(kRotoUIParamEffectHint) );
    effectStrength->setEvaluateOnChange(false);
    effectStrength->setSecret(true);
    effectStrength->setDefaultValue(15);
    effectStrength->setRange(0., 100.);
    effectStrength->disableSlider();
    generalPage->addKnob(effectStrength);
    _imp->ui->effectSpinBox = effectStrength;

    KnobIntPtr timeOffsetSb = AppManager::createKnob<KnobInt>( thisShared, tr(kRotoUIParamTimeOffsetLabel) );
    timeOffsetSb->setName(kRotoUIParamTimeOffset);
    timeOffsetSb->setInViewerContextLabel( tr(kRotoUIParamTimeOffsetLabel) );
    timeOffsetSb->setHintToolTip( tr(kRotoUIParamTimeOffsetHint) );
    timeOffsetSb->setEvaluateOnChange(false);
    timeOffsetSb->setSecret(true);
    timeOffsetSb->setDefaultValue(0);
    timeOffsetSb->disableSlider();
    generalPage->addKnob(timeOffsetSb);
    _imp->ui->timeOffsetSpinBox = timeOffsetSb;

    KnobChoicePtr timeOffsetMode = AppManager::createKnob<KnobChoice>( thisShared, tr(kRotoUIParamTimeOffsetLabel) );
    timeOffsetMode->setName(kRotoUIParamTimeOffset);
    timeOffsetMode->setHintToolTip( tr(kRotoUIParamTimeOffsetHint) );
    timeOffsetMode->setEvaluateOnChange(false);
    timeOffsetMode->setSecret(true);
    {
        std::vector<std::string> choices, helps;
        choices.push_back("Relative");
        helps.push_back("The time offset is a frame number in the source");
        choices.push_back("Absolute");
        helps.push_back("The time offset is a relative amount of frames relative to the current frame");
        timeOffsetMode->populateChoices(choices, helps);
    }
    timeOffsetMode->setDefaultValue(0);
    generalPage->addKnob(timeOffsetMode);
    _imp->ui->timeOffsetModeChoice = timeOffsetMode;

    KnobChoicePtr sourceType = AppManager::createKnob<KnobChoice>( thisShared, tr(kRotoUIParamSourceTypeLabel) );
    sourceType->setName(kRotoUIParamSourceType);
    sourceType->setHintToolTip( tr(kRotoUIParamSourceTypeHint) );
    sourceType->setEvaluateOnChange(false);
    sourceType->setSecret(true);
    {
        std::vector<std::string> choices, helps;
        choices.push_back("foreground");
        choices.push_back("background");
        for (int i = 1; i < 10; ++i) {
            QString str = tr("background") + QString::number(i + 1);
            choices.push_back( str.toStdString() );
        }
        sourceType->populateChoices(choices);
    }
    sourceType->setDefaultValue(1);
    generalPage->addKnob(sourceType);
    _imp->ui->sourceTypeChoice = sourceType;


    KnobButtonPtr resetCloneOffset = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamResetCloneOffsetLabel) );
    resetCloneOffset->setName(kRotoUIParamResetCloneOffset);
    resetCloneOffset->setHintToolTip( tr(kRotoUIParamResetCloneOffsetHint) );
    resetCloneOffset->setEvaluateOnChange(false);
    resetCloneOffset->setSecret(true);
    resetCloneOffset->setInViewerContextCanHaveShortcut(true);
    resetCloneOffset->setInViewerContextLayoutType(eViewerContextLayoutTypeAddNewLine);
    generalPage->addKnob(resetCloneOffset);
    addOverlaySlaveParam(resetCloneOffset);
    _imp->ui->resetCloneOffsetButton = resetCloneOffset;


    // Roto
    addKnobToViewerUI(autoKeyingEnabled);
    addKnobToViewerUI(featherLinkEnabled);
    addKnobToViewerUI(displayFeatherEnabled);
    addKnobToViewerUI(stickySelection);
    addKnobToViewerUI(bboxClickAnywhere);
    addKnobToViewerUI(rippleEditEnabled);
    addKnobToViewerUI(addKeyframe);
    addKnobToViewerUI(removeKeyframe);
    addKnobToViewerUI(showTransform);

    // RotoPaint
    addKnobToViewerUI(multiStroke);
    multiStroke->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(colorWheel);
    colorWheel->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(blendingModes);
    blendingModes->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(opacityKnob);
    opacityKnob->setInViewerContextItemSpacing(1);
    addKnobToViewerUI(pressureOpacity);
    pressureOpacity->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(sizeKnob);
    sizeKnob->setInViewerContextItemSpacing(1);
    addKnobToViewerUI(pressureSize);
    pressureSize->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(hardnessKnob);
    hardnessKnob->setInViewerContextItemSpacing(1);
    addKnobToViewerUI(pressureHardness);
    pressureHardness->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(buildUp);
    buildUp->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(effectStrength);
    effectStrength->setInViewerContextItemSpacing(ROTOPAINT_VIEWER_UI_SECTIONS_SPACING_PX);
    addKnobToViewerUI(timeOffsetSb);
    timeOffsetSb->setInViewerContextItemSpacing(1);
    addKnobToViewerUI(timeOffsetMode);
    addKnobToViewerUI(sourceType);
    addKnobToViewerUI(resetCloneOffset);
    resetCloneOffset->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);

    KnobPagePtr toolbar = AppManager::createKnob<KnobPage>( thisShared, std::string(kRotoUIParamToolbar) );
    toolbar->setAsToolBar(true);
    toolbar->setEvaluateOnChange(false);
    toolbar->setSecret(true);
    _imp->ui->toolbarPage = toolbar;
    KnobGroupPtr selectionToolButton = AppManager::createKnob<KnobGroup>( thisShared, tr(kRotoUIParamSelectionToolButtonLabel) );
    selectionToolButton->setName(kRotoUIParamSelectionToolButton);
    selectionToolButton->setAsToolButton(true);
    selectionToolButton->setEvaluateOnChange(false);
    selectionToolButton->setSecret(true);
    selectionToolButton->setInViewerContextCanHaveShortcut(true);
    selectionToolButton->setIsPersistent(false);
    toolbar->addKnob(selectionToolButton);
    _imp->ui->selectToolGroup = selectionToolButton;
    KnobGroupPtr editPointsToolButton = AppManager::createKnob<KnobGroup>( thisShared, tr(kRotoUIParamEditPointsToolButtonLabel) );
    editPointsToolButton->setName(kRotoUIParamEditPointsToolButton);
    editPointsToolButton->setAsToolButton(true);
    editPointsToolButton->setEvaluateOnChange(false);
    editPointsToolButton->setSecret(true);
    editPointsToolButton->setInViewerContextCanHaveShortcut(true);
    editPointsToolButton->setIsPersistent(false);
    toolbar->addKnob(editPointsToolButton);
    _imp->ui->pointsEditionToolGroup = editPointsToolButton;
    KnobGroupPtr editBezierToolButton = AppManager::createKnob<KnobGroup>( thisShared, tr(kRotoUIParamBezierEditionToolButtonLabel) );
    editBezierToolButton->setName(kRotoUIParamBezierEditionToolButton);
    editBezierToolButton->setAsToolButton(true);
    editBezierToolButton->setEvaluateOnChange(false);
    editBezierToolButton->setSecret(true);
    editBezierToolButton->setInViewerContextCanHaveShortcut(true);
    editBezierToolButton->setIsPersistent(false);
    toolbar->addKnob(editBezierToolButton);
    _imp->ui->bezierEditionToolGroup = editBezierToolButton;
    KnobGroupPtr paintToolButton = AppManager::createKnob<KnobGroup>( thisShared, tr(kRotoUIParamPaintBrushToolButtonLabel) );
    paintToolButton->setName(kRotoUIParamPaintBrushToolButton);
    paintToolButton->setAsToolButton(true);
    paintToolButton->setEvaluateOnChange(false);
    paintToolButton->setSecret(true);
    paintToolButton->setInViewerContextCanHaveShortcut(true);
    paintToolButton->setIsPersistent(false);
    toolbar->addKnob(paintToolButton);
    _imp->ui->paintBrushToolGroup = paintToolButton;

    KnobGroupPtr cloneToolButton, effectToolButton, mergeToolButton;
    if (_imp->isPaintByDefault) {
        cloneToolButton = AppManager::createKnob<KnobGroup>( thisShared, tr(kRotoUIParamCloneBrushToolButtonLabel) );
        cloneToolButton->setName(kRotoUIParamCloneBrushToolButton);
        cloneToolButton->setAsToolButton(true);
        cloneToolButton->setEvaluateOnChange(false);
        cloneToolButton->setSecret(true);
        cloneToolButton->setInViewerContextCanHaveShortcut(true);
        cloneToolButton->setIsPersistent(false);
        toolbar->addKnob(cloneToolButton);
        _imp->ui->cloneBrushToolGroup = cloneToolButton;
        effectToolButton = AppManager::createKnob<KnobGroup>( thisShared, tr(kRotoUIParamEffectBrushToolButtonLabel) );
        effectToolButton->setName(kRotoUIParamEffectBrushToolButton);
        effectToolButton->setAsToolButton(true);
        effectToolButton->setEvaluateOnChange(false);
        effectToolButton->setSecret(true);
        effectToolButton->setInViewerContextCanHaveShortcut(true);
        effectToolButton->setIsPersistent(false);
        toolbar->addKnob(effectToolButton);
        _imp->ui->effectBrushToolGroup = effectToolButton;
        mergeToolButton = AppManager::createKnob<KnobGroup>( thisShared, tr(kRotoUIParamMergeBrushToolButtonLabel) );
        mergeToolButton->setName(kRotoUIParamMergeBrushToolButton);
        mergeToolButton->setAsToolButton(true);
        mergeToolButton->setEvaluateOnChange(false);
        mergeToolButton->setSecret(true);
        mergeToolButton->setInViewerContextCanHaveShortcut(true);
        mergeToolButton->setIsPersistent(false);
        toolbar->addKnob(mergeToolButton);
        _imp->ui->mergeBrushToolGroup = mergeToolButton;
    }


    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamSelectAllToolButtonActionLabel) );
        tool->setName(kRotoUIParamSelectAllToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSelectAllToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(true);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "cursor.png");
        tool->setIsPersistent(false);
        selectionToolButton->addKnob(tool);
        _imp->ui->selectAllAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamSelectPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamSelectPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSelectPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "selectPoints.png");
        tool->setIsPersistent(false);
        selectionToolButton->addKnob(tool);
        _imp->ui->selectPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamSelectShapesToolButtonActionLabel) );
        tool->setName(kRotoUIParamSelectShapesToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSelectShapesToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "selectCurves.png");
        tool->setIsPersistent(false);
        selectionToolButton->addKnob(tool);
        _imp->ui->selectCurvesAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamSelectFeatherPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamSelectFeatherPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSelectFeatherPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "selectFeather.png");
        tool->setIsPersistent(false);
        selectionToolButton->addKnob(tool);
        _imp->ui->selectFeatherPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamAddPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamAddPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamAddPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(true);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "addPoints.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->addPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRemovePointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamRemovePointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamRemovePointsToolButtonAction) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "removePoints.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->removePointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamCuspPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamCuspPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamCuspPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "cuspPoints.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->cuspPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamSmoothPointsToolButtonActionLabel) );
        tool->setName(kRotoUIParamSmoothPointsToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamSmoothPointsToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "smoothPoints.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->smoothPointsAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamOpenCloseCurveToolButtonActionLabel) );
        tool->setName(kRotoUIParamOpenCloseCurveToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamOpenCloseCurveToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "openCloseCurve.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->openCloseCurveAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRemoveFeatherToolButtonAction) );
        tool->setName(kRotoUIParamRemoveFeatherToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamRemoveFeatherToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "removeFeather.png");
        tool->setIsPersistent(false);
        editPointsToolButton->addKnob(tool);
        _imp->ui->removeFeatherAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamDrawBezierToolButtonActionLabel) );
        tool->setName(kRotoUIParamDrawBezierToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamDrawBezierToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(true);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "bezier32.png");
        tool->setIsPersistent(false);
        editBezierToolButton->addKnob(tool);
        _imp->ui->drawBezierAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamDrawEllipseToolButtonActionLabel) );
        tool->setName(kRotoUIParamDrawEllipseToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamDrawEllipseToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "ellipse.png");
        tool->setIsPersistent(false);
        editBezierToolButton->addKnob(tool);
        _imp->ui->drawEllipseAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamDrawRectangleToolButtonActionLabel) );
        tool->setName(kRotoUIParamDrawRectangleToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamDrawRectangleToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "rectangle.png");
        tool->setIsPersistent(false);
        editBezierToolButton->addKnob(tool);
        _imp->ui->drawRectangleAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamDrawBrushToolButtonActionLabel) );
        tool->setName(kRotoUIParamDrawBrushToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamDrawBrushToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(true);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_solid.png");
        tool->setIsPersistent(false);
        paintToolButton->addKnob(tool);
        _imp->ui->brushAction = tool;
    }
    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamPencilToolButtonActionLabel) );
        tool->setName(kRotoUIParamPencilToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamPencilToolButtonAction) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "rotoToolPencil.png");
        tool->setIsPersistent(false);
        paintToolButton->addKnob(tool);
        _imp->ui->pencilAction = tool;
    }

    {
        KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamEraserToolButtonActionLabel) );
        tool->setName(kRotoUIParamEraserToolButtonAction);
        tool->setHintToolTip( tr(kRotoUIParamEraserToolButtonActionHint) );
        tool->setCheckable(true);
        tool->setDefaultValue(false);
        tool->setSecret(true);
        tool->setEvaluateOnChange(false);
        tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_eraser.png");
        tool->setIsPersistent(false);
        paintToolButton->addKnob(tool);
        _imp->ui->eraserAction = tool;
    }
    if (_imp->isPaintByDefault) {
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamCloneToolButtonActionLabel) );
            tool->setName(kRotoUIParamCloneToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamCloneToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(true);
            tool->setSecret(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_clone.png");
            tool->setIsPersistent(false);
            cloneToolButton->addKnob(tool);
            _imp->ui->cloneAction = tool;
        }
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRevealToolButtonAction) );
            tool->setName(kRotoUIParamRevealToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamRevealToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(false);
            tool->setSecret(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_reveal.png");
            tool->setIsPersistent(false);
            cloneToolButton->addKnob(tool);
            _imp->ui->revealAction = tool;
        }


        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamBlurToolButtonActionLabel) );
            tool->setName(kRotoUIParamBlurToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamBlurToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(true);
            tool->setSecret(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_blur.png");
            tool->setIsPersistent(false);
            effectToolButton->addKnob(tool);
            _imp->ui->blurAction = tool;
        }
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamSmearToolButtonActionLabel) );
            tool->setName(kRotoUIParamSmearToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamSmearToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(false);
            tool->setSecret(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_smear.png");
            tool->setIsPersistent(false);
            effectToolButton->addKnob(tool);
            _imp->ui->smearAction = tool;
        }
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamDodgeToolButtonActionLabel) );
            tool->setName(kRotoUIParamDodgeToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamDodgeToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(true);
            tool->setSecret(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_dodge.png");
            tool->setIsPersistent(false);
            mergeToolButton->addKnob(tool);
            _imp->ui->dodgeAction = tool;
        }
        {
            KnobButtonPtr tool = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamBurnToolButtonActionLabel) );
            tool->setName(kRotoUIParamBurnToolButtonAction);
            tool->setHintToolTip( tr(kRotoUIParamBurnToolButtonActionHint) );
            tool->setCheckable(true);
            tool->setDefaultValue(false);
            tool->setSecret(true);
            tool->setEvaluateOnChange(false);
            tool->setIconLabel(NATRON_IMAGES_PATH "rotopaint_burn.png");
            tool->setIsPersistent(false);
            mergeToolButton->addKnob(tool);
            _imp->ui->burnAction = tool;
        }
    } // if (_imp->isPaintByDefault) {

    // Right click menu
    KnobChoicePtr rightClickMenu = AppManager::createKnob<KnobChoice>( thisShared, std::string(kRotoUIParamRightClickMenu) );
    rightClickMenu->setName(kRotoUIParamRightClickMenu);
    rightClickMenu->setSecret(true);
    rightClickMenu->setEvaluateOnChange(false);
    generalPage->addKnob(rightClickMenu);
    _imp->ui->rightClickMenuKnob = rightClickMenu;
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionRemoveItemsLabel) );
        action->setName(kRotoUIParamRightClickMenuActionRemoveItems);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->removeItemsMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionCuspItemsLabel) );
        action->setName(kRotoUIParamRightClickMenuActionCuspItems);
        action->setEvaluateOnChange(false);
        action->setSecret(true);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->cuspItemMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionSmoothItemsLabel) );
        action->setName(kRotoUIParamRightClickMenuActionSmoothItems);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->smoothItemMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionRemoveItemsFeatherLabel) );
        action->setName(kRotoUIParamRightClickMenuActionRemoveItemsFeather);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->removeItemFeatherMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionNudgeBottomLabel) );
        action->setName(kRotoUIParamRightClickMenuActionNudgeBottom);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->nudgeBottomMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionNudgeLeftLabel) );
        action->setName(kRotoUIParamRightClickMenuActionNudgeLeft);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->nudgeLeftMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionNudgeTopLabel) );
        action->setName(kRotoUIParamRightClickMenuActionNudgeTop);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->nudgeTopMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionNudgeRightLabel) );
        action->setName(kRotoUIParamRightClickMenuActionNudgeRight);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->nudgeRightMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionSelectAllLabel) );
        action->setName(kRotoUIParamRightClickMenuActionSelectAll);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        generalPage->addKnob(action);
        _imp->ui->selectAllMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionOpenCloseLabel) );
        action->setName(kRotoUIParamRightClickMenuActionOpenClose);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->openCloseMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kRotoUIParamRightClickMenuActionLockShapesLabel) );
        action->setName(kRotoUIParamRightClickMenuActionLockShapes);
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        generalPage->addKnob(action);
        _imp->ui->lockShapeMenuAction = action;
    }

    KnobButtonPtr defaultAction;
    KnobGroupPtr defaultRole;
    if (_imp->isPaintByDefault) {
        defaultAction = _imp->ui->brushAction.lock();
        defaultRole = _imp->ui->paintBrushToolGroup.lock();
    } else {
        defaultAction = _imp->ui->drawBezierAction.lock();
        defaultRole = _imp->ui->bezierEditionToolGroup.lock();
    }
    _imp->ui->setCurrentTool(defaultAction);
    _imp->ui->onRoleChangedInternal(defaultRole);
    _imp->ui->onToolChangedInternal(defaultAction);
} // RotoPaint::initializeKnobs


bool
RotoPaint::shouldPreferPluginOverlayOverHostOverlay() const
{
    return !_imp->ui->ctrlDown;
}

bool
RotoPaint::shouldDrawHostOverlay() const
{
    KnobButtonPtr b = _imp->ui->showTransformHandle.lock();

    if (!b) {
        return true;
    }
    return b->getValue();
}

void
RotoPaint::onKnobsLoaded()
{

    // When loading a roto node, always switch to selection mode by default
    KnobButtonPtr defaultAction = _imp->ui->selectAllAction.lock();
    KnobGroupPtr defaultRole = _imp->ui->selectToolGroup.lock();;

    _imp->ui->setCurrentTool(defaultAction);
    _imp->ui->onRoleChangedInternal(defaultRole);
    _imp->ui->onToolChangedInternal(defaultAction);
}

bool
RotoPaint::knobChanged(const KnobIPtr& k,
                       ValueChangedReasonEnum reason,
                       ViewSetSpec view,
                       double time,
                       bool originatedFromMainThread)
{
    if (!k) {
        return false;
    }

    bool ret = true;
    KnobIPtr kShared = k->shared_from_this();
    KnobButtonPtr isBtn = toKnobButton(kShared);
    KnobGroupPtr isGrp = toKnobGroup(kShared);
    if ( isBtn && _imp->ui->onToolChangedInternal(isBtn) ) {
        return true;
    } else if ( isGrp && _imp->ui->onRoleChangedInternal(isGrp) ) {
        return true;
    } else if ( k == _imp->ui->colorWheelButton.lock() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->pressureOpacityButton.lock() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->pressureSizeButton.lock() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->hardnessSpinbox.lock() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->buildUpButton.lock() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
    } else if ( k == _imp->ui->resetCloneOffsetButton.lock() ) {
        _imp->ui->onBreakMultiStrokeTriggered();
        _imp->ui->cloneOffset.first = _imp->ui->cloneOffset.second = 0;
    } else if ( k == _imp->ui->addKeyframeButton.lock() ) {
        for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
            BezierPtr isBezier = toBezier(*it);
            if (!isBezier) {
                continue;
            }
            isBezier->setKeyframe(time);
            isBezier->invalidateCacheHashAndEvaluate(true, false);
        }

    } else if ( k == _imp->ui->removeKeyframeButton.lock() ) {
        for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
            BezierPtr isBezier = toBezier(*it);
            if (!isBezier) {
                continue;
            }
            isBezier->removeKeyframe(time);
        }
    } else if ( k == _imp->ui->removeItemsMenuAction.lock() ) {
        ///if control points are selected, delete them, otherwise delete the selected beziers
        if ( !_imp->ui->selectedCps.empty() ) {
            pushUndoCommand( new RemovePointUndoCommand(_imp->ui, _imp->ui->selectedCps) );
        } else if ( !_imp->ui->selectedItems.empty() ) {
            pushUndoCommand( new RemoveCurveUndoCommand(_imp->ui, _imp->ui->selectedItems) );
        }
    } else if ( k == _imp->ui->smoothItemMenuAction.lock() ) {
        if ( !_imp->ui->smoothSelectedCurve() ) {
            return false;
        }
    } else if ( k == _imp->ui->cuspItemMenuAction.lock() ) {
        if ( !_imp->ui->cuspSelectedCurve() ) {
            return false;
        }
    } else if ( k == _imp->ui->removeItemFeatherMenuAction.lock() ) {
        if ( !_imp->ui->removeFeatherForSelectedCurve() ) {
            return false;
        }
    } else if ( k == _imp->ui->nudgeLeftMenuAction.lock() ) {
        if ( !_imp->ui->moveSelectedCpsWithKeyArrows(-1, 0) ) {
            return false;
        }
    } else if ( k == _imp->ui->nudgeRightMenuAction.lock() ) {
        if ( !_imp->ui->moveSelectedCpsWithKeyArrows(1, 0) ) {
            return false;
        }
    } else if ( k == _imp->ui->nudgeBottomMenuAction.lock() ) {
        if ( !_imp->ui->moveSelectedCpsWithKeyArrows(0, -1) ) {
            return false;
        }
    } else if ( k == _imp->ui->nudgeTopMenuAction.lock() ) {
        if ( !_imp->ui->moveSelectedCpsWithKeyArrows(0, 1) ) {
            return false;
        }
    } else if ( k == _imp->ui->selectAllMenuAction.lock() ) {
        _imp->ui->iSelectingwithCtrlA = true;
        ///if no bezier are selected, select all beziers
        if ( _imp->ui->selectedItems.empty() ) {
            std::list<RotoDrawableItemPtr > bez = _imp->knobsTable->getRotoItemsByRenderOrder();
            for (std::list<RotoDrawableItemPtr >::const_iterator it = bez.begin(); it != bez.end(); ++it) {
                ctx->select(*it, RotoItem::eSelectionReasonOverlayInteract);
                _imp->ui->selectedItems.push_back(*it);
            }
        } else {
            ///select all the control points of all selected beziers
            _imp->ui->selectedCps.clear();
            for (SelectedItems::iterator it = _imp->ui->selectedItems.begin(); it != _imp->ui->selectedItems.end(); ++it) {
                BezierPtr isBezier = toBezier(*it);
                if (!isBezier) {
                    continue;
                }
                const std::list<BezierCPPtr > & cps = isBezier->getControlPoints();
                const std::list<BezierCPPtr > & fps = isBezier->getFeatherPoints();
                assert( cps.size() == fps.size() );

                std::list<BezierCPPtr >::const_iterator cpIT = cps.begin();
                for (std::list<BezierCPPtr >::const_iterator fpIT = fps.begin(); fpIT != fps.end(); ++fpIT, ++cpIT) {
                    _imp->ui->selectedCps.push_back( std::make_pair(*cpIT, *fpIT) );
                }
            }
            _imp->ui->computeSelectedCpsBBOX();
        }
    } else if ( k == _imp->ui->openCloseMenuAction.lock() ) {
        if ( ( (_imp->ui->selectedTool == eRotoToolDrawBezier) || (_imp->ui->selectedTool == eRotoToolOpenBezier) ) && _imp->ui->builtBezier && !_imp->ui->builtBezier->isCurveFinished() ) {
            pushUndoCommand( new OpenCloseUndoCommand(_imp->ui, _imp->ui->builtBezier) );

            _imp->ui->builtBezier.reset();
            _imp->ui->selectedCps.clear();
            _imp->ui->setCurrentTool( _imp->ui->selectAllAction.lock() );
        }
    } else if ( k == _imp->ui->lockShapeMenuAction.lock() ) {
        _imp->ui->lockSelectedCurves();
    } else if (k == _imp->lifeTimeKnob.lock()) {
        int lifetime_i = _imp->lifeTimeKnob.lock()->getValue();
        _imp->activatedKnob.lock()->setSecret(lifetime_i != 3);
        KnobIntPtr frame = _imp->lifeTimeFrameKnob.lock();
        frame->setSecret(lifetime_i == 3);
        if (lifetime_i != 3) {
            frame->setValue(time);
        }
    } else if ( k == _imp->resetCenterKnob.lock() ) {
        resetTransformCenter();
    } else if ( k == _imp->resetCloneCenterKnob.lock() ) {
        resetCloneTransformCenter();
    } else if ( k == _imp->resetTransformKnob.lock() ) {
        resetTransform();
    } else if ( k == _imp->resetCloneTransformKnob.lock() ) {
        resetCloneTransform();
    } else if ( k == _imp->motionBlurTypeKnob.lock().get() ) {
        int mbType_i = _imp->motionBlurTypeKnob.lock()->getValue();
        bool isPerShapeMB = mbType_i == 0;
        _imp->motionBlurKnob.lock()->setSecret(!isPerShapeMB);
        _imp->shutterKnob.lock()->setSecret(!isPerShapeMB);
        _imp->shutterTypeKnob.lock()->setSecret(!isPerShapeMB);
        _imp->customOffsetKnob.lock()->setSecret(!isPerShapeMB);

        _imp->globalMotionBlurKnob.lock()->setSecret(isPerShapeMB);
        _imp->globalShutterKnob.lock()->setSecret(isPerShapeMB);
        _imp->globalShutterTypeKnob.lock()->setSecret(isPerShapeMB);
        _imp->globalCustomOffsetKnob.lock()->setSecret(isPerShapeMB);
    } else {
        ret = false;
    }

    if (!ret) {
        ret |= ctx->knobChanged(k, reason, view, time, originatedFromMainThread);
    }

    return ret;
} // RotoPaint::knobChanged

void
RotoPaint::refreshExtraStateAfterTimeChanged(bool isPlayback,
                                             double time)
{
    NodeGroup::refreshExtraStateAfterTimeChanged(isPlayback, time);
    if (time != _imp->ui->strokeBeingPaintedTimelineFrame) {
        if ( (_imp->ui->selectedTool == eRotoToolBlur) ||
            ( _imp->ui->selectedTool == eRotoToolBurn) ||
            ( _imp->ui->selectedTool == eRotoToolDodge) ||
            ( _imp->ui->selectedTool == eRotoToolClone) ||
            ( _imp->ui->selectedTool == eRotoToolEraserBrush) ||
            ( _imp->ui->selectedTool == eRotoToolSolidBrush) ||
            ( _imp->ui->selectedTool == eRotoToolReveal) ||
            ( _imp->ui->selectedTool == eRotoToolSmear) ||
            ( _imp->ui->selectedTool == eRotoToolSharpen) ) {
            _imp->ui->onBreakMultiStrokeTriggered();
        }
    }
    _imp->ui->computeSelectedCpsBBOX();
}

StatusEnum
RotoPaint::getPreferredMetaDatas(NodeMetadata& metadata)
{
    metadata.setImageComponents( -1, ImageComponents::getRGBAComponents() );
    /*KnobBoolPtr premultKnob = _imp->premultKnob.lock();
       assert(premultKnob);
       bool premultiply = premultKnob->getValue();
       if (premultiply) {
        metadata.setOutputPremult(eImagePremultiplicationPremultiplied);
       } else {
        ImagePremultiplicationEnum srcPremult = eImagePremultiplicationOpaque;
        EffectInstancePtr input = getInput(0);
        if (input) {
            srcPremult = input->getPremult();
        }
        bool processA = getNode()->getProcessChannel(3);
        if ( (srcPremult == eImagePremultiplicationOpaque) && processA ) {
            metadata.setOutputPremult(eImagePremultiplicationUnPremultiplied);
        } else {
            metadata.setOutputPremult(eImagePremultiplicationPremultiplied);
        }
       }*/
    metadata.setOutputPremult(eImagePremultiplicationPremultiplied);

    return eStatusOK;
}

void
RotoPaint::onInputChanged(int inputNb)
{
    refreshRotoPaintTree();
    NodeGroup::onInputChanged(inputNb);
}


static void
getRotoItemsByRenderOrderInternal(std::list< RotoDrawableItemPtr > * curves,
                                  const RotoLayerPtr& layer,
                                  double time,
                                  bool onlyActives)
{
    std::vector<KnobTableItemPtr> children = layer->getChildren();

    for (std::vector<KnobTableItemPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {

        RotoLayerPtr isChildLayer = toRotoLayer(*it);
        RotoDrawableItemPtr isChildDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);

        if (isChildDrawable) {
            if ( !onlyActives || isChildDrawable->isActivated(time) ) {
                curves->push_front(isChildDrawable);
            }
        } else if ( isChildLayer && isChildLayer->isGloballyActivated() ) {
            getRotoItemsByRenderOrderInternal(curves, isChildLayer, time, onlyActives);
        }
    }
}

std::list< RotoDrawableItemPtr >
RotoPaintKnobItemsTable::getRotoItemsByRenderOrder(bool onlyActivated) const
{
    std::list< RotoDrawableItemPtr > ret;
    double time = _imp->publicInterface->getCurrentTime();
    std::vector<KnobTableItemPtr> topLevelItems = getTopLevelItems();

    // Roto should have only a single top level layer
    assert(topLevelItems.size() == 1);
    if (topLevelItems.size() != 1) {
        return ret;
    }
    RotoLayerPtr layer = toRotoLayer(topLevelItems.front());

    getRotoItemsByRenderOrderInternal(&ret, layer, time, onlyActivated);

    return ret;
}

void
RotoPaintKnobItemsTable::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    ++_imp->treeRefreshBlocked;
    KnobItemsTable::fromSerialization(obj);
    --_imp->treeRefreshBlocked;
    _imp->publicInterface->refreshRotoPaintTree();
}

void
RotoPaintKnobItemsTable::onModelReset()
{
    createBaseLayer();
}

void
RotoPaint::onCurveLockedChanged(int reason)
{
    RotoItemPtr item = getNode()->getRotoContext()->getLastItemLocked();

    if ( item && ( (RotoItem::SelectionReasonEnum)reason != RotoItem::eSelectionReasonOverlayInteract ) ) {
        assert(item);
        bool changed = false;
        if (item) {
            _imp->ui->onCurveLockedChangedRecursive(item, &changed);
        }

        if (changed) {
            redrawOverlayInteract();
        }
    }
}


void
RotoPaintPrivate::resetTransformsCenter(bool doClone,
                                   bool doTransform)
{
    double time = getNode()->getApp()->getTimeLine()->currentFrame();
    RectD bbox;

    getItemsRegionOfDefinition(getSelectedItems(), time, ViewIdx(0), &bbox);
    if (doTransform) {
        KnobDoublePtr centerKnob = _imp->centerKnob.lock();
        centerKnob->beginChanges();
        centerKnob->removeAnimation(ViewSpec::all(), 0);
        centerKnob->removeAnimation(ViewSpec::all(), 1);
        centerKnob->setValues( (bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2., ViewSpec::all(), eValueChangedReasonNatronInternalEdited );
        centerKnob->endChanges();
    }
    if (doClone) {
        KnobDoublePtr centerKnob = _imp->cloneCenterKnob.lock();
        centerKnob->beginChanges();
        centerKnob->removeAnimation(ViewSpec::all(), 0);
        centerKnob->removeAnimation(ViewSpec::all(), 1);
        centerKnob->setValues( (bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2., ViewSpec::all(), eValueChangedReasonNatronInternalEdited );
        centerKnob->endChanges();
    }
}

void
RotoPaintPrivate::resetTransformCenter()
{
    resetTransformsCenter(false, true);
}

void
RotoPaintPrivate::resetCloneTransformCenter()
{
    resetTransformsCenter(true, false);
}

void
RotoPaintPrivate::resetTransformInternal(const KnobDoublePtr& translate,
                                    const KnobDoublePtr& scale,
                                    const KnobDoublePtr& center,
                                    const KnobDoublePtr& rotate,
                                    const KnobDoublePtr& skewX,
                                    const KnobDoublePtr& skewY,
                                    const KnobBoolPtr& scaleUniform,
                                    const KnobChoicePtr& skewOrder,
                                    const KnobDoublePtr& extraMatrix)
{
    std::list<KnobIPtr> knobs;

    knobs.push_back(translate);
    knobs.push_back(scale);
    knobs.push_back(center);
    knobs.push_back(rotate);
    knobs.push_back(skewX);
    knobs.push_back(skewY);
    knobs.push_back(scaleUniform);
    knobs.push_back(skewOrder);
    if (extraMatrix) {
        knobs.push_back(extraMatrix);
    }
    bool wasEnabled = translate->isEnabled(0);
    for (std::list<KnobIPtr>::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        (*it)->resetToDefaultValue(DimSpec::all());
        (*it)->setEnabled(wasEnabled);
    }
}

void
RotoPaintPrivate::resetTransform()
{
    KnobDoublePtr translate = _imp->translateKnob.lock();
    KnobDoublePtr center = _imp->centerKnob.lock();
    KnobDoublePtr scale = _imp->scaleKnob.lock();
    KnobDoublePtr rotate = _imp->rotateKnob.lock();
    KnobBoolPtr uniform = _imp->scaleUniformKnob.lock();
    KnobDoublePtr skewX = _imp->skewXKnob.lock();
    KnobDoublePtr skewY = _imp->skewYKnob.lock();
    KnobChoicePtr skewOrder = _imp->skewOrderKnob.lock();
    KnobDoublePtr extraMatrix = _imp->extraMatrixKnob.lock();

    resetTransformInternal(translate, scale, center, rotate, skewX, skewY, uniform, skewOrder, extraMatrix);
}

void
RotoPaintPrivate::resetCloneTransform()
{
    KnobDoublePtr translate = _imp->cloneTranslateKnob.lock();
    KnobDoublePtr center = _imp->cloneCenterKnob.lock();
    KnobDoublePtr scale = _imp->cloneScaleKnob.lock();
    KnobDoublePtr rotate = _imp->cloneRotateKnob.lock();
    KnobBoolPtr uniform = _imp->cloneUniformKnob.lock();
    KnobDoublePtr skewX = _imp->cloneSkewXKnob.lock();
    KnobDoublePtr skewY = _imp->cloneSkewYKnob.lock();
    KnobChoicePtr skewOrder = _imp->cloneSkewOrderKnob.lock();

    resetTransformInternal( translate, scale, center, rotate, skewX, skewY, uniform, skewOrder, KnobDoublePtr() );
}

void
RotoPaint::onBreakMultiStrokeTriggered()
{
    _imp->ui->onBreakMultiStrokeTriggered();
}

void
RotoPaint::onEnableOpenGLKnobValueChanged(bool /*activated*/)
{
    _imp->ui->onBreakMultiStrokeTriggered();
}

void
RotoPaint::onSelectionChanged(int reason)
{
    if ( (RotoItem::SelectionReasonEnum)reason != RotoItem::eSelectionReasonOverlayInteract ) {
        _imp->ui->selectedItems = getNode()->getRotoContext()->getSelectedCurves();
        redrawOverlayInteract();
    }
}



void
RotoPaint::evaluateNeatStrokeRender()
{
    {
        QMutexLocker k(&_imp->doingNeatRenderMutex);
        _imp->mustDoNeatRender = true;
    }

    invalidateCacheHashAndEvaluate(true, false);

}


bool
RotoPaint::isDoingNeatRender() const
{
    QMutexLocker k(&_imp->doingNeatRenderMutex);

    return _imp->doingNeatRender;
}

bool
RotoPaint::mustDoNeatRender() const
{
    QMutexLocker k(&_imp->doingNeatRenderMutex);

    return _imp->mustDoNeatRender;
}

void
RotoPaint::setIsDoingNeatRender(bool doing)
{
    bool setUserPaintingOff = false;
    {
        QMutexLocker k(&_imp->doingNeatRenderMutex);

        if (doing && _imp->mustDoNeatRender) {
            assert(!_imp->doingNeatRender);
            _imp->doingNeatRender = true;
            _imp->mustDoNeatRender = false;
        } else if (_imp->doingNeatRender) {
            _imp->doingNeatRender = false;
            _imp->mustDoNeatRender = false;
            setUserPaintingOff = true;
        }
    }
    if (setUserPaintingOff) {
        getApp()->setUserIsPainting(getNode(), _imp->ui->strokeBeingPaint, false);
    }
}


KnobTableItemPtr
RotoPaintKnobItemsTable::createItemFromSerialization(const SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr& data)
{

}


bool
RotoPaintPrivate::isRotoPaintTreeConcatenatableInternal(const std::list<RotoDrawableItemPtr >& items,  int* blendingMode) const
{
    bool operatorSet = false;
    int comp_i = -1;

    for (std::list<RotoDrawableItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
        int op = (*it)->getCompositingOperator();
        if (!operatorSet) {
            operatorSet = true;
            comp_i = op;
        } else {
            if (op != comp_i) {
                //2 items have a different compositing operator
                return false;
            }
        }
        RotoStrokeItemPtr isStroke = toRotoStrokeItem(*it);
        if (!isStroke) {
            assert( toBezier(*it) );
        } else {
            if (isStroke->getBrushType() != eRotoStrokeTypeSolid) {
                return false;
            }
        }
    }
    if (operatorSet) {
        *blendingMode = comp_i;

        return true;
    }

    return false;
} // isRotoPaintTreeConcatenatableInternal

bool
RotoPaint::isRotoPaintTreeConcatenatable() const
{
    std::list<RotoDrawableItemPtr > items = _imp->knobsTable->getRotoItemsByRenderOrder();
    int bop;
    return _imp->isRotoPaintTreeConcatenatableInternal(items, &bop);
}


static void setOperationKnob(const NodePtr& node, int blendingOperator)
{
    KnobIPtr mergeOperatorKnob = node->getKnobByName(kMergeOFXParamOperation);
    KnobChoicePtr mergeOp = toKnobChoice( mergeOperatorKnob );
    if (mergeOp) {
        mergeOp->setValue(blendingOperator);
    }
}

NodePtr
RotoPaintPrivate::getOrCreateGlobalMergeNode(int blendingOperator, int *availableInputIndex)
{
    {
        QMutexLocker k(&globalMergeNodesMutex);
        for (NodesList::iterator it = globalMergeNodes.begin(); it != globalMergeNodes.end(); ++it) {
            const std::vector<NodeWPtr > &inputs = (*it)->getInputs();

            // Merge node goes like this: B, A, Mask, A2, A3, A4 ...
            assert( inputs.size() >= 3 && (*it)->getEffectInstance()->isInputMask(2) );
            if ( !inputs[1].lock() ) {
                *availableInputIndex = 1;
                setOperationKnob(*it, blendingOperator);
                return *it;
            }

            //Leave the B empty to connect the next merge node
            for (std::size_t i = 3; i < inputs.size(); ++i) {
                if ( !inputs[i].lock() ) {
                    *availableInputIndex = (int)i;
                    setOperationKnob(*it, blendingOperator);
                    return *it;
                }
            }
        }
    }


    NodePtr node = publicInterface->getNode();
    RotoPaintPtr rotoPaintEffect = toRotoPaint(node->getEffectInstance());


    //We must create a new merge node
    QString fixedNamePrefix = QString::fromUtf8( node->getScriptName_mt_safe().c_str() );

    fixedNamePrefix.append( QLatin1Char('_') );
    fixedNamePrefix.append( QString::fromUtf8("globalMerge") );
    fixedNamePrefix.append( QLatin1Char('_') );


    CreateNodeArgsPtr args(CreateNodeArgs::create( PLUGINID_OFX_MERGE,  rotoPaintEffect ));
    args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
    args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());

    NodePtr mergeNode = node->getApp()->createNode(args);
    if (!mergeNode) {
        return mergeNode;
    }
    if ( node->isDuringPaintStrokeCreation() ) {
        mergeNode->setWhileCreatingPaintStroke(true);
    }



    {
        // Link OpenGL enabled knob to the one on the Rotopaint so the user can control if GPU rendering is used in the roto internal node graph
        KnobChoicePtr glRenderKnob = mergeNode->getOrCreateOpenGLEnabledKnob();
        if (glRenderKnob) {
            KnobChoicePtr rotoPaintGLRenderKnob = node->getOrCreateOpenGLEnabledKnob();
            assert(rotoPaintGLRenderKnob);
            glRenderKnob->slaveTo(rotoPaintGLRenderKnob);
        }
    }
    *availableInputIndex = 1;
    setOperationKnob(mergeNode, blendingOperator);

    {
        // Link the RGBA enabled checkbox of the Rotopaint to the merge output RGBA
        KnobBoolPtr rotoPaintRGBA[4];
        KnobBoolPtr mergeRGBA[4];
        rotoPaintEffect->getEnabledChannelKnobs(&rotoPaintRGBA[0], &rotoPaintRGBA[1], &rotoPaintRGBA[2], &rotoPaintRGBA[3]);
        mergeRGBA[0] = toKnobBool(mergeNode->getKnobByName(kMergeParamOutputChannelsR));
        mergeRGBA[1] = toKnobBool(mergeNode->getKnobByName(kMergeParamOutputChannelsG));
        mergeRGBA[2] = toKnobBool(mergeNode->getKnobByName(kMergeParamOutputChannelsB));
        mergeRGBA[3] = toKnobBool(mergeNode->getKnobByName(kMergeParamOutputChannelsA));
        for (int i = 0; i < 4; ++i) {
            mergeRGBA[i]->slaveTo(rotoPaintRGBA[i]);
        }

        // Link mix
        KnobIPtr rotoPaintMix = rotoPaintEffect->getNode()->getOrCreateHostMixKnob(rotoPaintEffect->getNode()->getOrCreateMainPage());
        KnobIPtr mergeMix = mergeNode->getKnobByName(kMergeOFXParamMix);
        mergeMix->slaveTo(rotoPaintMix);

    }


    QMutexLocker k(&globalMergeNodesMutex);
    globalMergeNodes.push_back(mergeNode);
    
    return mergeNode;
} // getOrCreateGlobalMergeNode

static void connectRotoPaintBottomTreeToItems(const RotoPaintPtr& rotoPaintEffect, const NodePtr& noOpNode, const NodePtr& premultNode, const NodePtr& treeOutputNode, const NodePtr& mergeNode)
{
    if (treeOutputNode->getInput(0) != noOpNode) {
        treeOutputNode->disconnectInput(0);
        treeOutputNode->connectInput(noOpNode, 0);
    }
    if (noOpNode->getInput(0) != premultNode) {
        noOpNode->disconnectInput(0);
        noOpNode->connectInput(premultNode, 0);
    }
    if (premultNode->getInput(0) != mergeNode) {
        premultNode->disconnectInput(0);
        premultNode->connectInput(mergeNode, 0);
    }
    // Connect the mask of the merge to the Mask input
    mergeNode->disconnectInput(2);
    mergeNode->connectInput(rotoPaintEffect->getInternalInputNode(ROTOPAINT_MASK_INPUT_INDEX), 2);

}

void
RotoPaint::refreshRotoPaintTree()
{
    if (_imp->treeRefreshBlocked) {
        return;
    }

    std::list<RotoDrawableItemPtr > items = _imp->knobsTable->getRotoItemsByRenderOrder();

    // Check if the tree can be concatenated into a single merge node
    int blendingOperator;
    bool canConcatenate = isRotoPaintTreeConcatenatableInternal(items, &blendingOperator);
    NodePtr globalMerge;
    int globalMergeIndex = -1;
    NodesList mergeNodes;
    {
        QMutexLocker k(&_imp->rotoContextMutex);
        mergeNodes = _imp->globalMergeNodes;
    }

    // Ensure that all global merge nodes are disconnected
    for (NodesList::iterator it = mergeNodes.begin(); it != mergeNodes.end(); ++it) {
        int maxInputs = (*it)->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            (*it)->disconnectInput(i);
        }
    }
    globalMerge = getOrCreateGlobalMergeNode(blendingOperator, &globalMergeIndex);

    RotoPaintPtr rotoPaintEffect = toRotoPaint(getNode()->getEffectInstance());
    assert(rotoPaintEffect);

    if (canConcatenate) {
        NodePtr rotopaintNodeInput = rotoPaintEffect->getInternalInputNode(0);
        //Connect the rotopaint node input to the B input of the Merge
        if (rotopaintNodeInput) {
            globalMerge->connectInput(rotopaintNodeInput, 0);
        }
    } else {
        // Diconnect all inputs of the globalMerge
        int maxInputs = globalMerge->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            globalMerge->disconnectInput(i);
        }
    }

    // Refresh each item separately
    for (std::list<RotoDrawableItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
        (*it)->refreshNodesConnections(canConcatenate);

        if (canConcatenate) {

            // If we concatenate the tree, connect the global merge to the effect

            NodePtr effectNode = (*it)->getEffectNode();
            assert(effectNode);
            //qDebug() << "Connecting" << (*it)->getScriptName().c_str() << "to input" << globalMergeIndex <<
            //"(" << globalMerge->getInputLabel(globalMergeIndex).c_str() << ")" << "of" << globalMerge->getScriptName().c_str();
            globalMerge->connectInput(effectNode, globalMergeIndex);

            // Refresh for next node
            NodePtr nextMerge = getOrCreateGlobalMergeNode(blendingOperator, &globalMergeIndex);
            if (nextMerge != globalMerge) {
                assert( !nextMerge->getInput(0) );
                nextMerge->connectInput(globalMerge, 0);
                globalMerge = nextMerge;
            }
        }
    }

    // Default to noop node as bottom of the tree
    NodePtr premultNode = rotoPaintEffect->getPremultNode();
    NodePtr noOpNode = rotoPaintEffect->getMetadataFixerNode();
    NodePtr treeOutputNode = rotoPaintEffect->getOutputNode(false);
    if (!premultNode || !noOpNode || !treeOutputNode) {
        return;
    }

    if (canConcatenate) {
        connectRotoPaintBottomTreeToItems(rotoPaintEffect, noOpNode, premultNode, treeOutputNode, _imp->globalMergeNodes.front());
    } else {
        if (!items.empty()) {
            // Connect noop to the first item merge node
            connectRotoPaintBottomTreeToItems(rotoPaintEffect, noOpNode, premultNode, treeOutputNode, items.front()->getMergeNode());

        } else {
            NodePtr treeInputNode0 = rotoPaintEffect->getInternalInputNode(0);
            if (treeOutputNode->getInput(0) != treeInputNode0) {
                treeOutputNode->disconnectInput(0);
                treeOutputNode->connectInput(treeInputNode0, 0);
            }
        }
    }

    {
        // Make sure the premult node has its RGB checkbox checked
        premultNode->getEffectInstance()->beginChanges();
        KnobBoolPtr process[3];
        process[0] = toKnobBool(premultNode->getKnobByName(kNatronOfxParamProcessR));
        process[1] = toKnobBool(premultNode->getKnobByName(kNatronOfxParamProcessG));
        process[2] = toKnobBool(premultNode->getKnobByName(kNatronOfxParamProcessB));
        for (int i = 0; i < 3; ++i) {
            assert(process[i]);
            process[i]->setValue(true);
        }
        premultNode->getEffectInstance()->endChanges();
        
    }
    
    
} // RotoPaint::refreshRotoPaintTree


void
RotoPaintPrivate::getGlobalMotionBlurSettings(const double time,
                                              double* startTime,
                                              double* endTime,
                                              double* timeStep) const
{
    *startTime = time, *timeStep = 1., *endTime = time;
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    double motionBlurAmnt = _imp->globalMotionBlurKnob.lock()->getValueAtTime(time);
    if (motionBlurAmnt == 0) {
        return;
    }
    int nbSamples = std::floor(motionBlurAmnt * 10 + 0.5);
    double shutterInterval = _imp->globalMotionBlurKnob.lock()->getValueAtTime(time);
    if (shutterInterval == 0) {
        return;
    }
    int shutterType_i = _imp->globalMotionBlurKnob.lock()->getValueAtTime(time);
    if (nbSamples != 0) {
        *timeStep = shutterInterval / nbSamples;
    }
    if (shutterType_i == 0) { // centered
        *startTime = time - shutterInterval / 2.;
        *endTime = time + shutterInterval / 2.;
    } else if (shutterType_i == 1) { // start
        *startTime = time;
        *endTime = time + shutterInterval;
    } else if (shutterType_i == 2) { // end
        *startTime = time - shutterInterval;
        *endTime = time;
    } else if (shutterType_i == 3) { // custom
        *startTime = time + _imp->globalCustomOffsetKnob.lock()->getValueAtTime(time);
        *endTime = *startTime + shutterInterval;
    } else {
        assert(false);
    }

    
#endif
}

}

void
RotoPaint::setWhileCreatingPaintStrokeOnMergeNodes(bool b)
{
    getNode()->setWhileCreatingPaintStroke(b);
    QMutexLocker k(&_imp->globalMergeNodesMutex);
    for (NodesList::iterator it = _imp->globalMergeNodes.begin(); it != _imp->globalMergeNodes.end(); ++it) {
        (*it)->setWhileCreatingPaintStroke(b);
    }
}


///Must be done here because at the time of the constructor, the shared_ptr doesn't exist yet but
///addLayer() needs it to get a shared ptr to this
void
RotoPaintPrivate::createBaseLayer()
{
    ////Add the base layer
    RotoLayerPtr base = addLayerInternal(false);

    deselect(base, RotoItem::eSelectionReasonOther);
}

RotoLayerPtr
RotoPaintPrivate::getOrCreateBaseLayer()
{
    QMutexLocker k(&_imp->rotoContextMutex);

    if ( _imp->layers.empty() ) {
        k.unlock();
        addLayer();
        k.relock();
    }
    assert( !_imp->layers.empty() );

    return _imp->layers.front();
}

RotoLayerPtr
RotoPaintPrivate::addLayerInternal(bool declarePython)
{
    RotoContextPtr this_shared = shared_from_this();

    assert(this_shared);

    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    RotoLayerPtr item;
    std::string name = generateUniqueName(kRotoLayerBaseName);
    int indexInLayer = -1;
    {
        RotoLayerPtr deepestLayer;
        RotoLayerPtr parentLayer;
        {
            QMutexLocker l(&_imp->rotoContextMutex);
            deepestLayer = _imp->findDeepestSelectedLayer();

            if (!deepestLayer) {
                ///find out if there's a base layer, if so add to the base layer,
                ///otherwise create the base layer
                for (std::list<RotoLayerPtr >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
                    int hierarchy = (*it)->getHierarchyLevel();
                    if (hierarchy == 0) {
                        parentLayer = *it;
                        break;
                    }
                }
            } else {
                parentLayer = deepestLayer;
            }
        }

        item.reset( new RotoLayer( this_shared, name, RotoLayerPtr() ) );
        item->initializeKnobsPublic();
        if (parentLayer) {
            parentLayer->addItem(item, declarePython);
            indexInLayer = parentLayer->getItems().size() - 1;
        }

        QMutexLocker l(&_imp->rotoContextMutex);

        _imp->layers.push_back(item);

        _imp->lastInsertedItem = item;
    }
    Q_EMIT itemInserted(indexInLayer, RotoItem::eSelectionReasonOther);


    clearSelection(RotoItem::eSelectionReasonOther);
    select(item, RotoItem::eSelectionReasonOther);

    return item;
} // RotoPaintPrivate::addLayerInternal

RotoLayerPtr
RotoPaintPrivate::addLayer()
{
    return addLayerInternal(true);
} // addLayer

void
RotoPaintPrivate::addLayer(const RotoLayerPtr & layer)
{
    std::list<RotoLayerPtr >::iterator it = std::find(_imp->layers.begin(), _imp->layers.end(), layer);

    if ( it == _imp->layers.end() ) {
        _imp->layers.push_back(layer);
    }
}



BezierPtr
RotoPaintPrivate::makeBezier(double x,
                        double y,
                        const std::string & baseName,
                        double time,
                        bool isOpenBezier)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    RotoLayerPtr parentLayer;
    RotoContextPtr this_shared = boost::dynamic_pointer_cast<RotoContext>( shared_from_this() );
    assert(this_shared);
    std::string name = generateUniqueName(baseName);

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        RotoLayerPtr deepestLayer = _imp->findDeepestSelectedLayer();


        if (!deepestLayer) {
            ///if there is no base layer, create one
            if ( _imp->layers.empty() ) {
                l.unlock();
                addLayer();
                l.relock();
            }
            parentLayer = _imp->layers.front();
        } else {
            parentLayer = deepestLayer;
        }
    }
    assert(parentLayer);
    BezierPtr curve( new Bezier(this_shared, name, RotoLayerPtr(), isOpenBezier) );
    curve->createNodes();

    int indexInLayer = -1;
    if (parentLayer) {
        indexInLayer = 0;
        parentLayer->insertItem(curve, 0);
    }
    _imp->lastInsertedItem = curve;

    Q_EMIT itemInserted(indexInLayer, RotoItem::eSelectionReasonOther);


    clearSelection(RotoItem::eSelectionReasonOther);
    select(curve, RotoItem::eSelectionReasonOther);

    if ( isAutoKeyingEnabled() ) {
        curve->setKeyframe( getTimelineCurrentTime() );
    }
    curve->addControlPoint(x, y, time);

    return curve;
} // makeBezier

RotoStrokeItemPtr
RotoPaintPrivate::makeStroke(RotoStrokeType type,
                        const std::string& baseName,
                        bool clearSel)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    RotoLayerPtr parentLayer;
    RotoContextPtr this_shared = boost::dynamic_pointer_cast<RotoContext>( shared_from_this() );
    assert(this_shared);
    std::string name = generateUniqueName(baseName);

    {
        QMutexLocker l(&_imp->rotoContextMutex);

        RotoLayerPtr deepestLayer = _imp->findDeepestSelectedLayer();


        if (!deepestLayer) {
            ///if there is no base layer, create one
            if ( _imp->layers.empty() ) {
                l.unlock();
                addLayer();
                l.relock();
            }
            parentLayer = _imp->layers.front();
        } else {
            parentLayer = deepestLayer;
        }
    }
    assert(parentLayer);
    RotoStrokeItemPtr curve( new RotoStrokeItem( type, this_shared, name, RotoLayerPtr() ) );
    int indexInLayer = -1;
    if (parentLayer) {
        indexInLayer = 0;
        parentLayer->insertItem(curve, 0);
    }
    curve->createNodes();

    _imp->lastInsertedItem = curve;

    Q_EMIT itemInserted(indexInLayer, RotoItem::eSelectionReasonOther);

    if (clearSel) {
        clearSelection(RotoItem::eSelectionReasonOther);
        select(curve, RotoItem::eSelectionReasonOther);
    }

    return curve;
}

BezierPtr
RotoPaintPrivate::makeEllipse(double x,
                         double y,
                         double diameter,
                         bool fromCenter,
                         double time)
{
    double half = diameter / 2.;
    BezierPtr curve = makeBezier(x, fromCenter ? y - half : y, kRotoEllipseBaseName, time, false);

    if (fromCenter) {
        curve->addControlPoint(x + half, y, time);
        curve->addControlPoint(x, y + half, time);
        curve->addControlPoint(x - half, y, time);
    } else {
        curve->addControlPoint(x + diameter, y - diameter, time);
        curve->addControlPoint(x, y - diameter, time);
        curve->addControlPoint(x - diameter, y - diameter, time);
    }

    BezierCPPtr top = curve->getControlPointAtIndex(0);
    BezierCPPtr right = curve->getControlPointAtIndex(1);
    BezierCPPtr bottom = curve->getControlPointAtIndex(2);
    BezierCPPtr left = curve->getControlPointAtIndex(3);
    double topX, topY, rightX, rightY, btmX, btmY, leftX, leftY;
    top->getPositionAtTime(false, time, ViewIdx(0), &topX, &topY);
    right->getPositionAtTime(false, time, ViewIdx(0), &rightX, &rightY);
    bottom->getPositionAtTime(false, time, ViewIdx(0), &btmX, &btmY);
    left->getPositionAtTime(false, time, ViewIdx(0), &leftX, &leftY);

    curve->setLeftBezierPoint(0, time,  (leftX + topX) / 2., topY);
    curve->setRightBezierPoint(0, time, (rightX + topX) / 2., topY);

    curve->setLeftBezierPoint(1, time,  rightX, (rightY + topY) / 2.);
    curve->setRightBezierPoint(1, time, rightX, (rightY + btmY) / 2.);

    curve->setLeftBezierPoint(2, time,  (rightX + btmX) / 2., btmY);
    curve->setRightBezierPoint(2, time, (leftX + btmX) / 2., btmY);

    curve->setLeftBezierPoint(3, time,   leftX, (btmY + leftY) / 2.);
    curve->setRightBezierPoint(3, time, leftX, (topY + leftY) / 2.);
    curve->setCurveFinished(true);

    return curve;
}

BezierPtr
RotoPaintPrivate::makeSquare(double x,
                        double y,
                        double initialSize,
                        double time)
{
    BezierPtr curve = makeBezier(x, y, kRotoRectangleBaseName, time, false);

    curve->addControlPoint(x + initialSize, y, time);
    curve->addControlPoint(x + initialSize, y - initialSize, time);
    curve->addControlPoint(x, y - initialSize, time);
    curve->setCurveFinished(true);
    
    return curve;
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_RotoPaint.cpp"

