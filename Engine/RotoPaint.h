/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef ROTOPAINT_H
#define ROTOPAINT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/NodeGroup.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#define ROTOPAINT_MAX_INPUTS_COUNT 10
//#define ROTOPAINT_MASK_INPUT_INDEX 10

// Same number of masks than of sources
#define LAYERED_COMP_MAX_SOURCE_COUNT 10

#define LAYERED_COMP_MAX_INPUTS_COUNT (LAYERED_COMP_MAX_SOURCE_COUNT * 2)

#define LAYERED_COMP_FIRST_MASK_INPUT_INDEX (LAYERED_COMP_MAX_SOURCE_COUNT + 1)


#define kRotoOverlayColor "overlayColor"
#define kRotoOverlayColorLabel "Overlay Color"
#define kRotoOverlayColorHint "The color of the overlay in the Viewer"

#define kRotoClipToFormatParam "clipToFormat"
#define kRotoClipToFormatParamLabel "Clip To Format"
#define kRotoClipToFormatParamHint "When checked, the output image size is clipped to the format, unless no source is connected and the Output Format is set to Default"


#define kRotoOutputRodType "outputFormatType"
#define kRotoOutputRodTypeLabel "Output Format"
#define kRotoOutputRodTypeHint "Determines the size of the output image. Note that if this is set to anything other than \"Default\", the output format will also be set"

enum RotoPaintOutputRoDTypeEnum
{
    eRotoPaintOutputRoDTypeDefault,
    eRotoPaintOutputRoDTypeFormat,
    eRotoPaintOutputRoDTypeProject
};

#define kRotoOutputRodTypeFormatID "format"
#define kRotoOutputRodTypeFormatLabel "Format"
#define kRotoOutputRodTypeFormatHint "The output format is defined by the the Format specified with the parameter"


#define kRotoOutputRodTypeDefaultID "default"
#define kRotoOutputRodTypeDefaultLabel "Default"
#define kRotoOutputRodTypeDefaultHint "The output format is the source image format (if any)"

#define kRotoOutputRodTypeProjectID "project"
#define kRotoOutputRodTypeProjectLabel "Project"
#define kRotoOutputRodTypeProjectHint "The output format is the project format"

#define kRotoFormatParam kNatronParamFormatChoice
#define kRotoFormatParamLabel "Format"
#define kRotoFormatParamHint "The output format"

#define kRotoFormatSize kNatronParamFormatSize
#define kRotoFormatPar kNatronParamFormatPar

#define kRotoOpacityParam "opacity"
#define kRotoOpacityParamLabel "Opacity"
#define kRotoOpacityHint \
"Controls the opacity of the selected shape(s)."

#define kRotoFeatherParam "feather"
#define kRotoFeatherParamLabel "Feather"
#define kRotoFeatherHint \
"Controls the distance of feather (in pixels) to add around the selected shape(s)"

#define kRotoFeatherFallOffParam "featherFallOff"
#define kRotoFeatherFallOffParamLabel "Feather fall-off"
#define kRotoFeatherFallOffHint \
"Controls the rate at which the feather is applied on the selected shape(s)."

#define kRotoFeatherFallOffType "fallOffType"
#define kRotoFeatherFallOffTypeLabel ""
#define kRotoFeatherFallOffTypeHint "Select the type of interpolation used to create the fall-off ramp between the inner shape and the outter feather edge"

#define kRotoFeatherFallOffTypeLinear "Linear"
#define kRotoFeatherFallOffTypeLinearHint "Linear ramp"

#define kRotoFeatherFallOffTypePLinear "PLinear"
#define kRotoFeatherFallOffTypePLinearHint "Perceptually linear ramp in Rec.709"

#define kRotoFeatherFallOffTypeEaseIn "Ease-in"
#define kRotoFeatherFallOffTypeEaseInHint "Catmull-Rom spline, smooth start, linear end (a.k.a. smooth0)"

#define kRotoFeatherFallOffTypeEaseOut "Ease-out"
#define kRotoFeatherFallOffTypeEaseOutHint "Catmull-Rom spline, linear start, smooth end (a.k.a. smooth1)"

#define kRotoFeatherFallOffTypeSmooth "Smooth"
#define kRotoFeatherFallOffTypeSmoothHint "Traditional smoothstep ramp"


#define kBezierParamFillShape "enableFill"
#define kBezierParamFillShapeLabel "Fill Shape"
#define kBezierParamFillShapeHint "If checked, the shape will be filled with its solid color, otherwise only the contour will be drawn"

#define kRotoLifeTimeCustomRangeParam "customRange"
#define kRotoLifeTimeCustomRangeParamLabel "Custom Range"
#define kRotoLifeTimeCustomRangeParamHint \
"This is used to control whether the selected item(s) should be rendered or not.\n" \
"Typically to set a custom range, you would set a keyframe at a given time with this parameter enabled and " \
"set another keyframe further in time with the checkbox unchecked. This workflows allows to have multiple " \
"distinct range where the item can be enabled/disabled."

#define kRotoLockedHint \
"Control whether the layer/curve is editable or locked."

#define kRotoInvertedParam "invertMask"
#define kRotoInvertedParamLabel "Invert Mask"
#define kRotoInvertedHint "Controls whether the selected item's mask should be inverted"

#define kRotoOverlayHint "Color of the display overlay for this curve. Doesn't affect output."

#define kRotoColorParam "color"
#define kRotoColorParamLabel "Color"
#define kRotoColorHint \
"The color of the shape. This parameter is used when the output components are set to RGBA."

#define kRotoCompOperatorParam "operator"
#define kRotoCompOperatorParamLabel "Operator"
#define kRotoCompOperatorHint \
"The compositing operator controls how this shape is merged with the shapes that have already been rendered.\n" \
"The roto mask is initialised as black and transparent, then each shape is drawn in the selected order, with the selected color and operator.\n" \
"Finally, the mask is composed with the source image, if connected, using the 'over' operator.\n" \
"See http://cairographics.org/operators/ for a full description of available operators."


#define kRotoDrawableItemMergeAInputParam "mergeASource"
#define kRotoDrawableItemMergeAInputParamLabel "Source"
#define kRotoDrawableItemMergeAInputParamHint_RotoPaint "Determine what image is used to blend on to the previous item in the hierarchy." \
"- Foreground: used for Clone/Reveal to copy a portion of the image using the hand-drawn mask with a Transform applied.\n" \
"- Otherwise it indicates the label of a node in the node-graph to use as source. The node must be connected to a Bg input of the LayeredComp node."

#define kRotoDrawableItemMergeAInputParamHint_CompNode "Determine what node is used to blend on to the previous item in the hierarchy.\n" \
"- None: the item will be a pass-through.\n" \
"- Otherwise it indicates the label of a node in the node-graph to use as source. The node must be connected to a Source input of the LayeredComp node."

#define kRotoDrawableItemMergeMaskParam "mergeMask"
#define kRotoDrawableItemMergeMaskParamLabel "Mask"
#define kRotoDrawableItemMergeMaskParamHint "Determine what node is used as mask for blending on to the previous item in the hierarchy.\n" \
"- None: the merge operation will not be masked.\n" \
"- Otherwise it indicates the label of a node in the node-graph to use as a mask. The node must be connected to a mask input of the LayeredComp node."


#define kRotoBrushSizeParam "brushSize"
#define kRotoBrushSizeParamLabel "Brush Size"
#define kRotoBrushSizeParamHint "This is the diameter of the brush in pixels. Shift + drag on the viewer to modify this value"

#define kRotoBrushSpacingParam "brushSpacing"
#define kRotoBrushSpacingParamLabel "Brush Spacing"
#define kRotoBrushSpacingParamHint "Spacing between stamps of the paint brush"

#define kRotoBrushHardnessParam "brushHardness"
#define kRotoBrushHardnessParamLabel "Brush Hardness"
#define kRotoBrushHardnessParamHint "Fall off of the brush effect from the center to the edge"

#define kRotoBrushEffectParam "brushEffect"
#define kRotoBrushEffectParamLabel "Brush effect"
#define kRotoBrushEffectParamHint "The strength of the effect"

#define kRotoBrushVisiblePortionParam "strokeVisiblePortion"
#define kRotoBrushVisiblePortionParamLabel "Visible portion"
#define kRotoBrushVisiblePortionParamHint "Defines the range of the stroke that should be visible: 0 is the start of the stroke and 1 the end."

#define kRotoBrushPressureLabelParam "pressureAlters"
#define kRotoBrushPressureLabelParamLabel "Pressure alters"
#define kRotoBrushPressureLabelParamHint ""

#define kRotoBrushPressureOpacityParam "pressureOpacity"
#define kRotoBrushPressureOpacityParamLabel "Opacity"
#define kRotoBrushPressureOpacityParamHint "Alters the opacity of the paint brush proportionate to changes in pen pressure"

#define kRotoBrushPressureSizeParam "pressureSize"
#define kRotoBrushPressureSizeParamLabel "Size"
#define kRotoBrushPressureSizeParamHint "Alters the size of the paint brush proportionate to changes in pen pressure"

#define kRotoBrushPressureHardnessParam "pressureHardness"
#define kRotoBrushPressureHardnessParamLabel "Hardness"
#define kRotoBrushPressureHardnessParamHint "Alters the hardness of the paint brush proportionate to changes in pen pressure"

#define kRotoBrushBuildupParam "buildUp"
#define kRotoBrushBuildupParamLabel "Build-up"
#define kRotoBrushBuildupParamHint "When checked, the paint stroke builds up when painted over itself"

#define kRotoBrushTimeOffsetParam "timeOffset"
#define kRotoBrushTimeOffsetParamLabel "Time Offset"
#define kRotoBrushTimeOffsetParamHint_Clone "When the Clone tool is used, this determines depending on the time offset mode the source frame to " \
"clone. When in absolute mode, this is the frame number of the source, when in relative mode, this is an offset relative to the current frame."

#define kRotoBrushTimeOffsetParamHint_Comp "The time offset applied to the source before blending it"

#define kRotoBrushTimeOffsetModeParam "timeOffsetMode"
#define kRotoBrushTimeOffsetModeParamLabel "Mode"
#define kRotoBrushTimeOffsetModeParamHint "Time offset mode: when in absolute mode, this is the frame number of the source, when in relative mode, this is an offset relative to the current frame."

#define kRotoBrushTranslateParam "cloneTranslate"
#define kRotoBrushTranslateParamLabel "Translate"
#define kRotoBrushTranslateParamHint ""

#define kRotoBrushRotateParam "cloneRotate"
#define kRotoBrushRotateParamLabel "Rotate"
#define kRotoBrushRotateParamHint ""

#define kRotoBrushScaleParam "cloneScale"
#define kRotoBrushScaleParamLabel "Scale"
#define kRotoBrushScaleParamHint ""

#define kRotoBrushScaleUniformParam "cloneUniform"
#define kRotoBrushScaleUniformParamLabel "Uniform"
#define kRotoBrushScaleUniformParamHint ""

#define kRotoBrushSkewXParam "cloneSkewx"
#define kRotoBrushSkewXParamLabel "Skew X"
#define kRotoBrushSkewXParamHint ""

#define kRotoBrushSkewYParam "cloneSkewy"
#define kRotoBrushSkewYParamLabel "Skew Y"
#define kRotoBrushSkewYParamHint ""

#define kRotoBrushSkewOrderParam "cloneSkewOrder"
#define kRotoBrushSkewOrderParamLabel "Skew Order"
#define kRotoBrushSkewOrderParamHint ""

#define kRotoBrushCenterParam "cloneCenter"
#define kRotoBrushCenterParamLabel "Center"
#define kRotoBrushCenterParamHint ""

#define kRotoBrushFilterParam "cloneFilter"
#define kRotoBrushFilterParamLabel "Filter"
#define kRotoBrushFilterParamHint "Filtering algorithm - some filters may produce values outside of the initial range (*) or modify the values even if there is no movement (+)."

#define kRotoBrushBlackOutsideParam "blackOutside"
#define kRotoBrushBlackOutsideParamLabel "Black Outside"
#define kRotoBrushBlackOutsideParamHint "Fill the area outside the source image with black"


#define kRotoDrawableItemTranslateParam "translate"
#define kRotoDrawableItemTranslateParamLabel "Translate"
#define kRotoDrawableItemTranslateParamHint ""

#define kRotoDrawableItemRotateParam "rotate"
#define kRotoDrawableItemRotateParamLabel "Rotate"
#define kRotoDrawableItemRotateParamHint ""

#define kRotoDrawableItemScaleParam "scale"
#define kRotoDrawableItemScaleParamLabel "Scale"
#define kRotoDrawableItemScaleParamHint ""

#define kRotoDrawableItemScaleUniformParam "uniform"
#define kRotoDrawableItemScaleUniformParamLabel "Uniform"
#define kRotoDrawableItemScaleUniformParamHint ""

#define kRotoDrawableItemSkewXParam "skewx"
#define kRotoDrawableItemSkewXParamLabel "Skew X"
#define kRotoDrawableItemSkewXParamHint ""

#define kRotoDrawableItemSkewYParam "skewy"
#define kRotoDrawableItemSkewYParamLabel "Skew Y"
#define kRotoDrawableItemSkewYParamHint ""

#define kRotoDrawableItemSkewOrderParam "skewOrder"
#define kRotoDrawableItemSkewOrderParamLabel "Skew Order"
#define kRotoDrawableItemSkewOrderParamHint ""

#define kRotoDrawableItemCenterParam "center"
#define kRotoDrawableItemCenterParamLabel "Center"
#define kRotoDrawableItemCenterParamHint ""

#define kRotoDrawableItemExtraMatrixParam "extraMatrix"
#define kRotoDrawableItemExtraMatrixParamLabel "Extra Matrix"
#define kRotoDrawableItemExtraMatrixParamHint "This matrix gets concatenated to the transform resulting from the parameter above."

#define kRotoDrawableItemLifeTimeParam "lifeTime"
#define kRotoDrawableItemLifeTimeParamLabel "Life Time"
#define kRotoDrawableItemLifeTimeParamHint "Controls the life-time of the selected item(s)"

#define kRotoDrawableItemLifeTimeAll "All"
#define kRotoDrawableItemLifeTimeAllHelp "All frames"

#define kRotoDrawableItemLifeTimeSingle "Single"
#define kRotoDrawableItemLifeTimeSingleHelp "Only for the specified frame"

#define kRotoDrawableItemLifeTimeFromStart "From start"
#define kRotoDrawableItemLifeTimeFromStartHelp "From the start of the sequence up to the specified frame"

#define kRotoDrawableItemLifeTimeToEnd "To end"
#define kRotoDrawableItemLifeTimeToEndHelp "From the specified frame to the end of the sequence"

#define kRotoDrawableItemLifeTimeCustom "Custom"
#define kRotoDrawableItemLifeTimeCustomHelp "Use the Activated parameter animation to control the life-time of the shape/stroke using keyframes to specify one or multiple life-time ranges"

enum RotoPaintItemLifeTimeTypeEnum
{
    eRotoPaintItemLifeTimeTypeAll = 0,
    eRotoPaintItemLifeTimeTypeSingle,
    eRotoPaintItemLifeTimeTypeFromStart,
    eRotoPaintItemLifeTimeTypeToEnd,
    eRotoPaintItemLifeTimeTypeCustom
};

#define kRotoDrawableItemLifeTimeFrameParam "lifeTimeFrame"
#define kRotoDrawableItemLifeTimeFrameParamLabel "Frame"
#define kRotoDrawableItemLifeTimeFrameParamHint "Use this to specify the frame when in mode Single/From start/To end"

#define kRotoResetCloneTransformParam "resetCloneTransform"
#define kRotoResetCloneTransformParamLabel "Reset Transform"
#define kRotoResetCloneTransformParamHint "Reset the clone transform to an identity"

#define kRotoResetTransformParam "resetTransform"
#define kRotoResetTransformParamLabel "Reset Transform"
#define kRotoResetTransformParamHint "Reset the transform to an identity"

#define kRotoResetCloneCenterParam "resetCloneCenter"
#define kRotoResetCloneCenterParamLabel "Reset Center"
#define kRotoResetCloneCenterParamHint "Reset the clone transform center"

#define kRotoResetCenterParam "resetTransformCenter"
#define kRotoResetCenterParamLabel "Reset Center"
#define kRotoResetCenterParamHint "Reset the transform center"

#define kRotoTransformInteractive "RotoTransformInteractive"
#define kRotoTransformInteractiveLabel "Interactive"
#define kRotoTransformInteractiveHint "When check, modifying the transform will directly render the shape in the viewer. When unchecked, modifications are applied when releasing the mouse button."

#define kRotoMotionBlurModeParam "motionBlurMode"
#define kRotoMotionBlurModeParamLabel "Mode"
#define kRotoMotionBlurModeParamHint "Per-shape motion blurs applies motion blur independently to each shape and then blends them together." \
" This may produce artifacts when shapes blur over the same portion of the image, but might be more efficient than global motion-blur." \
" Global motion-blur takes into account the interaction between shapes and will not create artifacts at the expense of being slightly " \
"more expensive than the per-shape motion blur. Note that when using the global motion-blur, all shapes will have the same motion-blur " \
"settings applied to them."

#define kRotoMotionBlurModeNone "None"
#define kRotoMotionBlurModeNoneHint "No motion-blur will be applied"
#define kRotoMotionBlurModePerShape "Per-Shape"
#define kRotoMotionBlurModePerShapeHint "Applies motion-blur independently to each shape and then blends them together." \
" This may produce artifacts when shapes blur over the same portion of the image, but might be more efficient than global motion-blur."
#define kRotoMotionBlurModeGlobal "Global"
#define kRotoMotionBlurModeGlobalHint " Global motion-blur takes into account the interaction between shapes and will not create artifacts " \
"at the expense of being more expensive than the per-shape motion blur. " \
"Note that when using the global motion-blur, all shapes will have the same motion-blur settings applied to them."

enum RotoMotionBlurModeEnum
{
    eRotoMotionBlurModeNone,
    eRotoMotionBlurModePerShape,
    eRotoMotionBlurModeGlobal
};

#define kRotoPerShapeMotionBlurParam "motionBlur"
#define kRotoGlobalMotionBlurParam "globalMotionBlur"
#define kRotoMotionBlurParamLabel "Motion Blur"
#define kRotoMotionBlurParamHint "The number of time samples used for blurring along the shutter time. Increase for better quality but slower rendering.\n" \
"Set this parameter to 1 or the Shutter parameter to 0 to disable motion-blur."

#define kRotoPerShapeShutterParam "motionBlurShutter"
#define kRotoGlobalShutterParam "globalMotionBlurShutter"
#define kRotoShutterParamLabel "Shutter"
#define kRotoShutterParamHint "The number of frames during which the shutter should be opened when motion blurring.\n" \
"Set this parameter to 0 or the Motion Blur parameter to 1 to disable motion-blur."

#define kRotoPerShapeShutterOffsetTypeParam "motionBlurShutterOffset"
#define kRotoGlobalShutterOffsetTypeParam "gobalMotionBlurShutterOffset"
#define kRotoShutterOffsetTypeParamLabel "Shutter Offset"
#define kRotoShutterOffsetTypeParamHint "This controls how the shutter operates in respect to the current frame value."

#define kRotoShutterOffsetCenteredHint "Centers the shutter around the current frame, that is the shutter will be opened from f - shutter/2 to " \
"f + shutter/2"
#define kRotoShutterOffsetStartHint "The shutter will open at the current frame and stay open until f + shutter"
#define kRotoShutterOffsetEndHint "The shutter will open at f - shutter until the current frame"
#define kRotoShutterOffsetCustomHint "The shutter will open at the time indicated by the shutter offset parameter"

#define kRotoPerShapeShutterCustomOffsetParam "motionBlurCustomShutterOffset"
#define kRotoGlobalShutterCustomOffsetParam "globalMotionBlurCustomShutterOffset"
#define kRotoShutterCustomOffsetParamLabel "Custom Offset"
#define kRotoShutterCustomOffsetParamHint "If the Shutter Offset parameter is set to Custom then this parameter controls the frame at " \
"which the shutter opens. The value is an offset in frames to the current frame, e.g: -1  would open the shutter 1 frame before the current frame."

#define kLayeredCompMixParam "mix"
#define kLayeredCompMixParamLabel "Mix"
#define kLayeredCompMixParamHint "Mix between the source image at 0 and the full effect at 1"

#define kRotoAddGroupParam "addGroupButton"
#define kRotoAddGroupParamLabel "New Group"
#define kRotoAddGroupParamHint "Adds a new group into which items can be contained"

#define kRotoAddLayerParam "addLayerButton"
#define kRotoAddLayerParamLabel "New Comp Layer"
#define kRotoAddLayerParamHint "Adds a new compositing layer"

#define kRotoRemoveItemParam "removeItemButton"
#define kRotoRemoveItemParamLabel "Remove"
#define kRotoRemoveItemParamHint "Remove the selected item(s) from the table"

#define kRotoOutputComponentsParam "outputComponents"
#define kRotoOutputComponentsParamLabel "Components"
#define kRotoOutputComponentsParamHint "Components type of the image for the solid brush and for shapes. "\
"By default, the Roto node outputs Alpha only whereas the RotoPaint node outputs RGBA"

#define kRotoShapeUserKeyframesParam "shapeKeys"
#define kRotoShapeUserKeyframesParamLabel "Shape Keys"
#define kRotoShapeUserKeyframesParamHint "Navigate throughout the keyframes of the selected shape"

#define kRotoPaintItemsTableName "rotoPaintItems"

class RotoPaintPrivate;
class RotoPaint
    : public NodeGroup
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    enum RotoPaintTypeEnum
    {
        eRotoPaintTypeRotoPaint,
        eRotoPaintTypeRoto,
        eRotoPaintTypeComp
    };

protected: // derives from EffectInstance, parent of RotoNode
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    RotoPaint(const NodePtr& node,
              RotoPaintTypeEnum type);


public:

    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RotoPaint(node, eRotoPaintTypeRotoPaint) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& /*mainInstance*/, const FrameViewRenderKey& /*key*/) WARN_UNUSED_RETURN
    {
        assert(false);
        throw std::logic_error("A group cannot have a render clone");
    }

    static PluginPtr createPlugin() WARN_UNUSED_RETURN;

    virtual ~RotoPaint();

    RotoPaint::RotoPaintTypeEnum getRotoPaintNodeType() const;

    virtual void initializeOverlayInteract() OVERRIDE FINAL;

    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;

    virtual void onKnobsLoaded() OVERRIDE FINAL;
    virtual void onPropertiesChanged(const EffectDescription& description) OVERRIDE FINAL;

    virtual void setupInitialSubGraphState() OVERRIDE FINAL;

    NodePtr getPremultNode() const;

    NodePtr getInternalInputNode(int index) const;

    void getEnabledChannelKnobs(KnobBoolPtr* r,KnobBoolPtr* g, KnobBoolPtr* b, KnobBoolPtr *a) const;

    virtual bool isSubGraphUserVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isSubGraphPersistent() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool getDefaultInput(bool connected, int* inputIndex) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void loadSubGraph(const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                              const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization) OVERRIDE FINAL;

    void refreshRotoPaintTree();

    bool isRotoPaintTreeConcatenatable() const;
    
    RotoLayerPtr getOrCreateBaseLayer();

    /**
     * @brief Create a new layer to the currently selected layer.
     **/
    RotoLayerPtr addLayer();


    RotoLayerPtr addLayerInternal();

    /**
     * @brief Make a new Bezier curve and append it into the currently selected layer.
     * @param baseName A hint to name the item. It can be something like "Bezier", "Ellipse", "Rectangle" , etc...
     **/
    BezierPtr makeBezier(double x, double y, const std::string & baseName, TimeValue time, bool isOpenBezier);
    BezierPtr makeEllipse(double x, double y, double diameter, bool fromCenter, TimeValue time);
    BezierPtr makeSquare(double x, double y, double initialSize, TimeValue time);
    RotoStrokeItemPtr makeStroke(RotoStrokeType type,
                                 bool clearSel);
    CompNodeItemPtr makeCompNodeItem();

    RotoLayerPtr getLayerForNewItem();

    KnobChoicePtr getMergeAInputChoiceKnob() const;

    KnobChoicePtr getMotionBlurTypeKnob() const;

    KnobDoublePtr getMixKnob() const;

    KnobChoicePtr getOutputComponentsKnob() const;

    KnobChoicePtr getOutputRoDTypeKnob() const;

    KnobChoicePtr getOutputFormatKnob() const;

    KnobIntPtr getOutputFormatSizeKnob() const;

    KnobDoublePtr getOutputFormatParKnob() const;

    KnobBoolPtr getClipToFormatKnob() const;

    void refreshSourceKnobs(const RotoDrawableItemPtr& item);

    void getMergeChoices(std::vector<ChoiceOption>* aInput, std::vector<ChoiceOption>* maskInput) const;

    void addSoloItem(const RotoDrawableItemPtr& item);
    void removeSoloItem(const RotoDrawableItemPtr& item);
    bool isAmongstSoloItems(const RotoDrawableItemPtr& item) const;

    bool shouldDrawHostOverlay() const;

    std::list<RotoDrawableItemPtr> getRotoPaintItemsByRenderOrder() const;
    std::list<RotoDrawableItemPtr> getActivatedRotoPaintItemsByRenderOrder(TimeValue time, ViewIdx view) const;

#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    void createPlanarTrackForShape(const RotoDrawableItemPtr& item);
#endif

public Q_SLOTS:

    void onModelSelectionChanged(std::list<KnobTableItemPtr> addedToSelection, std::list<KnobTableItemPtr> removedFromSelection, TableChangeReasonEnum reason);

    void onBreakMultiStrokeTriggered();

    void onSourceNodeLabelChanged(const QString& oldLabel, const QString& newLabel);

#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    void onTrackingStarted(int step);

    void onTrackingEnded();
#endif
    
protected:

    void initLifeTimeKnobs(const KnobPagePtr& generalPage);

    void initGeneralPageKnobs();
    void initShapePageKnobs();
    void initStrokePageKnobs();
    void initTransformPageKnobs();
    void initClonePageKnobs();
    void initMotionBlurPageKnobs();
#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    void initTrackingPageKnobs();
    void initTrackingViewerKnobs(const KnobPagePtr &trackingPage);
    void initializeTrackRangeDialogKnobs(const KnobPagePtr& trackingPage);
#endif
    void initCompNodeKnobs(const KnobPagePtr& page);

    void initViewerUIKnobs(const KnobPagePtr& generalPage);

    void initKnobsTableControls();

private:

    virtual void fetchRenderCloneKnobs() OVERRIDE;
    
    virtual bool knobChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time) OVERRIDE FINAL;

    virtual void refreshExtraStateAfterTimeChanged(bool isPlayback, TimeValue time)  OVERRIDE FINAL;


    friend class BlockTreeRefreshRAII;
    boost::shared_ptr<RotoPaintPrivate> _imp;
};

/**
 * @brief Same as RotoPaint except that by default RGB checkboxes are unchecked and the default selected tool is not the same
 **/
class RotoNode
    : public RotoPaint
{

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    RotoNode(const NodePtr& node)
        : RotoPaint(node, eRotoPaintTypeRoto) {}
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RotoNode(node) );
    }

    static PluginPtr createPlugin();

};


/**
 * @brief Same as RotoPaint except that items in the table are only CompNodeItem
 **/
class LayeredCompNode
: public RotoPaint
{

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    LayeredCompNode(const NodePtr& node)
    : RotoPaint(node, eRotoPaintTypeComp) {}
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new LayeredCompNode(node) );
    }

    static PluginPtr createPlugin();


};

inline RotoPaintPtr
toRotoPaint(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<RotoPaint>(effect);
}

inline RotoNodePtr
toRotoNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<RotoNode>(effect);
}

inline LayeredCompNodePtr
toLayeredCompNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<LayeredCompNode>(effect);
}


class BlockTreeRefreshRAII
{
    RotoPaintPtr node;
public:

    BlockTreeRefreshRAII(const RotoPaintPtr& node);

    ~BlockTreeRefreshRAII();
};


NATRON_NAMESPACE_EXIT

#endif // ROTOPAINT_H
