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

#ifndef NATRON_ENGINE_VIEWER_INSTANCE_H
#define NATRON_ENGINE_VIEWER_INSTANCE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#define kViewerInstanceParamOutputLayer "outputLayer"
#define kViewerInstanceParamOutputLayerLabel "Layer"
#define kViewerInstanceParamOutputLayerHint "The layer this node will fetch in input and use as the RGB channels"

#define kViewerInstanceParamAlphaChannel "alphaChannel"
#define kViewerInstanceParamAlphaChannelLabel "Alpha"
#define kViewerInstanceParamAlphaChannelHint "The channel to use in input as alpha channel"

#define kViewerInstanceParamDisplayChannels "displayChannels"
#define kViewerInstanceParamDisplayChannelsLabel "Display Channels"
#define kViewerInstanceParamDisplayChannelsHint "The channels to display on the viewer from the selected layer"

#define kViewerInstanceNodeParamGain "gain"
#define kViewerInstanceNodeParamGainLabel "Gain"
#define kViewerInstanceNodeParamGainHint "Gain is shown as f-stops. The image is multipled by pow(2,value) before display"

#define kViewerInstanceParamEnableAutoContrast "autoContrast"
#define kViewerInstanceParamEnableAutoContrastLabel "Auto Contrast"
#define kViewerInstanceParamEnableAutoContrastHint "Automatically adjusts the gain and the offset applied " \
"to the colors of the visible image portion on the viewer"

#define kViewerInstanceParamGamma "gamma"
#define kViewerInstanceParamGammaLabel "Gamma"
#define kViewerInstanceParamGammaHint "Viewer gamma correction level (applied after gain and before colorspace correction)"

#define kViewerInstanceParamColorspace "deviceColorspace"
#define kViewerInstanceParamColorspaceLabel "Device Colorspace"
#define kViewerInstanceParamColorspaceHint "The operation applied to the image before it is displayed " \
"on screen. The image is converted to this colorspace before being displayed on the monitor"


#define kViewerInstanceParamEnableUserRoI "enableRegionOfInterest"
#define kViewerInstanceParamEnableUserRoILabel "Region Of Interest"
#define kViewerInstanceParamEnableUserRoIHint "When active, enables the region of interest that limits " \
"the portion of the viewer that is kept updated. Press %2 to create and drag a new region."

#define kViewerInstanceParamUserRoIBottomLeft "userRoIBtmLeft"
#define kViewerInstanceParamUserRoISize "userRoISize"

#define kViewerInstanceParamClipToFormat "clipToFormat"
#define kViewerInstanceParamClipToFormatLabel "Clip To Format"
#define kViewerInstanceParamClipToFormatHint "Clips the portion of the image displayed " \
"on the viewer to the format upstream. When off everything in " \
"region of definition is displayed"

#define kViewerInstanceParamUIProjectionBottomLeft "projectionBottomLeft"
#define kViewerInstanceParamUIProjectionTopRight "projectionTopRight"

typedef std::map<NodePtr, NodeRenderStats > RenderStatsMap;

struct ViewerInstancePrivate;
class ViewerInstance : public EffectInstance
{


private: // derives from EffectInstance

    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    ViewerInstance(const NodePtr& node);
    ViewerInstance(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN;

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new ViewerInstance(mainInstance, key) );
    }


    ViewerInstancePtr shared_from_this() {
        return boost::dynamic_pointer_cast<ViewerInstance>(KnobHolder::shared_from_this());
    }

    static PluginPtr createPlugin();

    virtual ~ViewerInstance();

    ViewerNodePtr getViewerNodeGroup() const;

    /**
     * @brief We do not want to create a settings panel, most knobs are handled on the viewer group node instead
     **/
    virtual bool getMakeSettingsPanel() const OVERRIDE FINAL { return false; }

    static const Color::Lut* lutFromColorspace(ViewerColorSpaceEnum cs) WARN_UNUSED_RETURN;

    NodePtr getInputRecursive(int inputIndex) const;

    void getChannelOptions(TimeValue time, ImagePlaneDesc* rgbLayer, ImagePlaneDesc* alphaLayer, int* alphaChannelIndex, ImagePlaneDesc* displayChannels) const;

    /**
     * @brief A ViewerNode is composed of 2 ViewerProcess nodes but it only has 1 layer and 1 alpha channel choices.
     * We thus disable the refreshing of the menu from the 2nd ViewerProcess node.
     **/
    void setRefreshLayerAndAlphaChoiceEnabled(bool enabled);

    RectD getViewerRoI();

private:

    virtual void initializeKnobs() OVERRIDE FINAL;

    virtual void fetchRenderCloneKnobs() OVERRIDE FINAL;

    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                                         const RenderScale & scale,
                                         const RectI & roi,
                                         ViewIdx view,
                                         const ImagePlaneDesc& plane,
                                         TimeValue* inputTime,
                                         ViewIdx* inputView,
                                         int* inputNb,
                                         ImagePlaneDesc* inputPlane) OVERRIDE FINAL WARN_UNUSED_RETURN;

    ActionRetCodeEnum getLayersProducedAndNeeded(TimeValue time,
                                          ViewIdx view,
                                          std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                          std::list<ImagePlaneDesc>* layersProduced,
                                          TimeValue* passThroughTime,
                                          ViewIdx* passThroughView,
                                          int* passThroughInputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;


    virtual void onMetadataChanged(const NodeMetadata& metadata) OVERRIDE FINAL;

    virtual ActionRetCodeEnum getTimeInvariantMetadata(NodeMetadata& metadata) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual ActionRetCodeEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash) OVERRIDE;

    virtual bool knobChanged(const KnobIPtr& knob,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time) OVERRIDE FINAL;



private:

    boost::scoped_ptr<ViewerInstancePrivate> _imp;
};


inline ViewerInstancePtr
toViewerInstance(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<ViewerInstance>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_VIEWER_INSTANCE_H
