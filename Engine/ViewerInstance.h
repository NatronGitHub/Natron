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
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/OutputEffectInstance.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


#define kViewerNodeParamLayers "layer"
#define kViewerNodeParamLayersLabel "Layer"
#define kViewerNodeParamLayersHint "The layer that the Viewer node will fetch upstream in the tree. " \
"The channels of the layer will be mapped to the RGBA channels of the viewer according to " \
"its number of channels. (e.g: UV would be mapped to RG)"

#define kViewerNodeParamAlphaChannel "alphaChannel"
#define kViewerNodeParamAlphaChannelLabel "Alpha Channel"
#define kViewerNodeParamAlphaChannelHint "Select here a channel of any layer that will be used when displaying the " \
"alpha channel with the Channels choice on the right"

#define kViewerNodeParamDisplayChannels "displayChannels"
#define kViewerNodeParamDisplayChannelsB "displayChannelsB"
#define kViewerNodeParamDisplayChannelsLabel "Display Channels"
#define kViewerNodeParamDisplayChannelsHint "The channels to display on the viewer from the selected layer"

#define kViewerNodeParamClipToProject "clipToProject"
#define kViewerNodeParamClipToProjectLabel "Clip To Project"
#define kViewerNodeParamClipToProjectHint "Clips the portion of the image displayed " \
"on the viewer to the project format. " \
"When off, everything in the union of all nodes " \
"region of definition is displayed"

#define kViewerNodeParamFullFrame "fullFrame"
#define kViewerNodeParamFullFrameLabel "Full Frame"
#define kViewerNodeParamFullFrameHint "When checked, the viewer will render the image in its entirety (at full resolution) not just the visible portion. This may be useful when panning/zooming during playback"

#define kViewerNodeParamEnableUserRoI "enableRegionOfInterest"
#define kViewerNodeParamEnableUserRoILabel "Region Of Interest"
#define kViewerNodeParamEnableUserRoIHint "When active, enables the region of interest that limits " \
"the portion of the viewer that is kept updated. Press %2 to create and drag a new region."

#define kViewerNodeParamUserRoIBottomLeft "userRoIBtmLeft"
#define kViewerNodeParamUserRoISize "userRoISize"

#define kViewerNodeParamEnableProxyMode "proxyMode"
#define kViewerNodeParamEnableProxyModeLabel "Proxy Mode"
#define kViewerNodeParamEnableProxyModeHint "Activates the downscaling by the amount indicated by the value on the right. " \
"The rendered images are degraded and as a result of this the whole rendering pipeline is much faster"

#define kViewerNodeParamProxyLevel "proxyLevel"
#define kViewerNodeParamProxyLevelLabel "Proxy Level"
#define kViewerNodeParamProxyLevelHint "When proxy mode is activated, it scales down the rendered image by this factor " \
"to accelerate the rendering"

#define kViewerNodeParamRefreshViewport "refreshViewport"
#define kViewerNodeParamRefreshViewportLabel "Refresh Viewport"
#define kViewerNodeParamRefreshViewportHint "Forces a new render of the current frame. Press %2 to activate in-depth render statistics useful " \
"for debugging the composition"

#define kViewerNodeParamPauseRender "pauseUpdates"
#define kViewerNodeParamPauseRenderB "pauseUpdatesB"
#define kViewerNodeParamPauseRenderLabel "Pause Updates"
#define kViewerNodeParamPauseRenderHint "When activated the viewer will not update after any change that would modify the image " \
"displayed in the viewport. Use %2 to pause both input A and B"

#define kViewerNodeParamAInput "aInput"
#define kViewerNodeParamAInputLabel "A"
#define kViewerNodeParamAInputHint "What node to display in the viewer input A"

#define kViewerNodeParamBInput "bInput"
#define kViewerNodeParamBInputLabel "B"
#define kViewerNodeParamBInputHint "What node to display in the viewer input B"

#define kViewerNodeParamOperation "operation"
#define kViewerNodeParamOperationLabel "Operation"
#define kViewerNodeParamOperationHint "Operation applied between viewer inputs A and B. a and b are the alpha components of each input. d is the wipe dissolve factor, controlled by the arc handle"

#define kViewerNodeParamOperationWipeUnder "Wipe Under"
#define kViewerNodeParamOperationWipeUnderHint "A(1 - d) + Bd"

#define kViewerNodeParamOperationWipeOver "Wipe Over"
#define kViewerNodeParamOperationWipeOverHint "A + B(1 - a)d"

#define kViewerNodeParamOperationWipeMinus "Wipe Minus"
#define kViewerNodeParamOperationWipeMinusHint "A - B"

#define kViewerNodeParamOperationWipeOnionSkin "Wipe Onion skin"
#define kViewerNodeParamOperationWipeOnionSkinHint "A + B"

#define kViewerNodeParamOperationStackUnder "Stack Under"
#define kViewerNodeParamOperationStackUnderHint "B"

#define kViewerNodeParamOperationStackOver "Stack Over"
#define kViewerNodeParamOperationStackOverHint "A + B(1 - a)"

#define kViewerNodeParamOperationStackMinus "Stack Minus"
#define kViewerNodeParamOperationStackMinusHint "A - B"

#define kViewerNodeParamOperationStackOnionSkin "Stack Onion skin"
#define kViewerNodeParamOperationStackOnionSkinHint "A + B"

#define kViewerNodeParamEnableGain "enableGain"
#define kViewerNodeParamEnableGainLabel "Enable Gain"
#define kViewerNodeParamEnableGainHint "Switch between \"neutral\" 1.0 gain f-stop and the previous setting"

#define kViewerNodeParamGain "gain"
#define kViewerNodeParamGainLabel "Gain"
#define kViewerNodeParamGainHint "Gain is shown as f-stops. The image is multipled by pow(2,value) before display"

#define kViewerNodeParamEnableAutoContrast "autoContrast"
#define kViewerNodeParamEnableAutoContrastLabel "Auto Contrast"
#define kViewerNodeParamEnableAutoContrastHint "Automatically adjusts the gain and the offset applied " \
"to the colors of the visible image portion on the viewer"

#define kViewerNodeParamEnableGamma "enableGamma"
#define kViewerNodeParamEnableGammaLabel "Enable Gamma"
#define kViewerNodeParamEnableGammaHint "Gamma correction: Switch between gamma=1.0 and user setting"

#define kViewerNodeParamGamma "gamma"
#define kViewerNodeParamGammaLabel "Gamma"
#define kViewerNodeParamGammaHint "Viewer gamma correction level (applied after gain and before colorspace correction)"

#define kViewerNodeParamColorspace "deviceColorspace"
#define kViewerNodeParamColorspaceLabel "Device Colorspace"
#define kViewerNodeParamColorspaceHint "The operation applied to the image before it is displayed " \
"on screen. The image is converted to this colorspace before being displayed on the monitor"

#define kViewerNodeParamView "activeView"
#define kViewerNodeParamViewLabel "Active View"
#define kViewerNodeParamViewHint "The view displayed on the viewer"

NATRON_NAMESPACE_ENTER;

class UpdateViewerParams; // ViewerInstancePrivate

typedef std::map<NodePtr, NodeRenderStats > RenderStatsMap;

struct ViewerArgs
{
    EffectInstancePtr activeInputToRender;
    bool forceRender;
    U64 activeInputHash;
    boost::shared_ptr<UpdateViewerParams> params;
    boost::shared_ptr<RenderingFlagSetter> isRenderingFlag;
    bool draftModeEnabled;
    unsigned int mipMapLevelWithDraft, mipmapLevelWithoutDraft;
    bool autoContrast;
    DisplayChannelsEnum channels;
    bool userRoIEnabled;
    bool mustComputeRoDAndLookupCache;
    bool isDoingPartialUpdates;
};

class ViewerInstance
    : public OutputEffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    friend class ViewerCurrentFrameRequestScheduler;

private: // derives from EffectInstance
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    ViewerInstance(const NodePtr& node);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN;

    ViewerInstancePtr shared_from_this() {
        return boost::dynamic_pointer_cast<ViewerInstance>(KnobHolder::shared_from_this());
    }

    virtual ~ViewerInstance();

    ViewerNodePtr getViewerNodeGroup() const;

    OpenGLViewerI* getUiContext() const WARN_UNUSED_RETURN;

    KnobChoicePtr getAInputChoice() const;
    KnobChoicePtr getBInputChoice() const;

    virtual bool supportsMultipleClipsBitDepth() const OVERRIDE FINAL
    {
        return true;
    }

    virtual bool supportsMultipleClipsFPS() const OVERRIDE FINAL
    {
        return true;
    }

    RectD getUserRoI() const;
    void setUserRoI(const RectD& rect);

    enum ViewerRenderRetCode
    {
        //The render failed and should clear to black the viewer and stop any ongoing playback
        eViewerRenderRetCodeFail = 0,

        //The render did nothing requiring updating the current texture
        //but just requires a redraw (something like aborted generally)
        eViewerRenderRetCodeRedraw,

        //The viewer needs to be cleared out to black but should not interrupt playback
        eViewerRenderRetCodeBlack,

        //The viewer did update or requires and update to the texture displayed
        eViewerRenderRetCodeRender,
    };


    ViewerRenderRetCode getRenderViewerArgsAndCheckCache_public(SequenceTime time,
                                                                bool isSequential,
                                                                ViewIdx view,
                                                                int textureIndex,
                                                                U64 viewerHash,
                                                                bool canAbort,
                                                                const NodePtr& rotoPaintNode,
                                                                const bool isDoingRotoNeatRender,
                                                                const RenderStatsPtr& stats,
                                                                ViewerArgs* outArgs);

private:
    /**
     * @brief Look-up the cache and try to find a matching texture for the portion to render.
     **/
    ViewerRenderRetCode getRenderViewerArgsAndCheckCache(SequenceTime time,
                                                         bool isSequential,
                                                         ViewIdx view,
                                                         int textureIndex,
                                                         U64 viewerHash,
                                                         const NodePtr& rotoPaintNode,
                                                         const bool isDoingRotoNeatRender,
                                                         const AbortableRenderInfoPtr& abortInfo,
                                                         const RenderStatsPtr& stats,
                                                         ViewerArgs* outArgs);


    /**
     * @brief Setup the ViewerArgs struct with the info requested by the user from the Viewer UI
     **/
    void setupMinimalUpdateViewerParams(const SequenceTime time,
                                        const ViewIdx view,
                                        const int textureIndex,
                                        const AbortableRenderInfoPtr& abortInfo,
                                        const bool isSequential,
                                        ViewerArgs* outArgs);


    /**
     * @brief Get the RoI from the Viewer and lookup the cache for a texture at the given mipMapLevel.
     * setupMinimalUpdateViewerParams(...) MUST have been called before.
     * When returning this function, the UpdateViewerParams will have been filled entirely
     * and if the texture was found in the cache, the shared pointer outArgs->params->cachedFrame will be valid.
     * This function may fail or ask to just redraw or ask to clear the viewer to black depending on it's return
     * code.
     **/
    ViewerRenderRetCode getViewerRoIAndTexture(const RectD& rod,
                                               const U64 viewerHash,
                                               const bool useCache,
                                               const bool isDraftMode,
                                               const unsigned int mipmapLevel,
                                               const RenderStatsPtr& stats,
                                               ViewerArgs* outArgs);


    /**
     * @brief Calls getViewerRoIAndTexture(). If called on the main-thread, the parameter
     * useOnlyRoDCache should be set to true. If it did not lookup the cache it will not
     * set the member mustComputeRoDAndLookupCache of the ViewerArgs struct. In that case
     * this function should be called again later on by the render thread with the parameter
     * useOnlyRoDCache to false.
     **/
    ViewerRenderRetCode getRoDAndLookupCache(const bool useOnlyRoDCache,
                                             const U64 viewerHash,
                                             const NodePtr& rotoPaintNode,
                                             const bool isDoingRotoNeatRender,
                                             const RenderStatsPtr& stats,
                                             ViewerArgs* outArgs);

public:


    /**
     * @brief This function renders the image at time 'time' on the viewer.
     * It first get the region of definition of the image at the given time
     * and then deduce what is the region of interest on the viewer, according
     * to the current render scale.
     * Then it looks-up the ViewerCache to find an already existing frame,
     * in which case it copies directly the cached frame over to the PBO.
     * Otherwise it just calls renderRoi(...) on the active input
     * and then render to the PBO.
     **/
    ViewerRenderRetCode renderViewer(ViewIdx view,
                                     bool singleThreaded,
                                     bool isSequentialRender,
                                     U64 viewerHash,
                                     bool canAbort,
                                     const NodePtr& rotoPaintNode,
                                     const RotoStrokeItemPtr& strokeItem,
                                     bool isDoingRotoNeatRender,
                                     bool useTLS,
                                     boost::shared_ptr<ViewerArgs> args[2],
                                     const boost::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs>& request,
                                     const RenderStatsPtr& stats) WARN_UNUSED_RETURN;

    ViewerRenderRetCode getViewerArgsAndRenderViewer(SequenceTime time,
                                                     bool canAbort,
                                                     ViewIdx view,
                                                     U64 viewerHash,
                                                     const NodePtr& rotoPaintNode,
                                                     const RotoStrokeItemPtr& strokeItem,
                                                     const RenderStatsPtr& stats,
                                                     boost::shared_ptr<ViewerArgs>* argsA,
                                                     boost::shared_ptr<ViewerArgs>* argsB);

    void aboutToUpdateTextures();

    void updateViewer(boost::shared_ptr<UpdateViewerParams> & frame);

    virtual bool getMakeSettingsPanel() const OVERRIDE FINAL { return false; }

    /**
     *@brief Bypasses the cache so the next frame will be rendered fully
     **/
    void forceFullComputationOnNextFrame();
    virtual void clearLastRenderedImage() OVERRIDE FINAL;

    void disconnectViewer();

    void disconnectTexture(int index, bool clearRod);

    int getMipMapLevel() const WARN_UNUSED_RETURN;

    int getMipMapLevelFromZoomFactor() const WARN_UNUSED_RETURN;

    DisplayChannelsEnum getChannels(int texIndex) const WARN_UNUSED_RETURN;

    virtual bool supportsMultipleClipsPAR() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    bool isLatestRender(int textureIndex, U64 renderAge) const;

    int getLastRenderedTime() const;

    virtual double getCurrentTime() const OVERRIDE WARN_UNUSED_RETURN;
    virtual ViewIdx getCurrentView() const OVERRIDE WARN_UNUSED_RETURN;
    TimeLinePtr getTimeline() const;

    void getTimelineBounds(int* first, int* last) const;

    static const Color::Lut* lutFromColorspace(ViewerColorSpaceEnum cs) WARN_UNUSED_RETURN;
    virtual void onMetaDatasRefreshed(const NodeMetadata& metadata) OVERRIDE FINAL;

    bool isViewerUIVisible() const;

    void callRedrawOnMainThread() { Q_EMIT s_callRedrawOnMainThread(); }

    struct ViewerInstancePrivate;

    float interpolateGammaLut(float value);

    void markAllOnGoingRendersAsAborted(bool keepOldestRender);

    /**
     * @brief Used to re-render only selected portions of the texture.
     * This requires that the renderviewer_internal() function gets called on a single thread
     * because the texture will get resized (i.e copied and swapped) to fit new RoIs.
     * After this call, the function isDoingPartialUpdates() will return true until
     * clearPartialUpdateParams() gets called.
     **/
    void setPartialUpdateParams(const std::list<RectD>& rois, bool recenterViewer);
    void clearPartialUpdateParams();

    void setDoingPartialUpdates(bool doing);
    bool isDoingPartialUpdates() const;

    virtual void reportStats(int time, ViewIdx view, double wallTime, const RenderStatsMap& stats) OVERRIDE FINAL;

    ///Only callable on MT
    void setActivateInputChangeRequestedFromViewer(bool fromViewer);

    bool isInputChangeRequestedFromViewer() const;

    void setViewerPaused(bool paused, bool allInputs);

    bool isViewerPaused(int texIndex) const;

    unsigned int getViewerMipMapLevel() const;

    void getInputsComponentsAvailables(std::set<ImageComponents>* comps) const;

public Q_SLOTS:


    void onMipMapLevelChanged(int level);


    /**
     * @brief Redraws the OpenGL viewer. Can only be called on the main-thread.
     **/
    void redrawViewer();

    void redrawViewerNow();


    void executeDisconnectTextureRequestOnMainThread(int index, bool clearRoD);


Q_SIGNALS:

    void renderStatsAvailable(int time, ViewIdx view, double wallTime, const RenderStatsMap& stats);

    void s_callRedrawOnMainThread();

    void viewerDisconnected();

    void clipPreferencesChanged();

    void disconnectTextureRequest(int index,bool clearRoD);

    void viewerRenderingStarted();
    void viewerRenderingEnded();

private:


    NodePtr getInputRecursive(int inputIndex) const;

    void refreshLayerAndAlphaChannelComboBox();

    
    /*******************************************
       *******OVERRIDEN FROM EFFECT INSTANCE******
     *******************************************/

    virtual bool knobChanged(const KnobIPtr& k, ValueChangedReasonEnum reason,
                             ViewSpec /*view*/,
                             double /*time*/,
                             bool /*originatedFromMainThread*/) OVERRIDE FINAL;


    virtual void initializeKnobs() OVERRIDE FINAL;

    virtual bool isOutput() const OVERRIDE FINAL
    {
        return true;
    }

    virtual int getMaxInputCount() const OVERRIDE FINAL;
    virtual bool isInputOptional(int /*n*/) const OVERRIDE FINAL;
    virtual int getMajorVersion() const OVERRIDE FINAL
    {
        return 1;
    }

    virtual int getMinorVersion() const OVERRIDE FINAL
    {
        return 0;
    }

    virtual std::string getPluginID() const OVERRIDE FINAL
    {
        return PLUGINID_NATRON_VIEWER_INTERNAL;
    }

    virtual std::string getPluginLabel() const OVERRIDE FINAL
    {
        return "ViewerProcess";
    }

    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL;
    virtual std::string getPluginDescription() const OVERRIDE FINAL
    {
        return "The Viewer node can display the output of a node graph.";
    }

    virtual std::string getInputLabel(int inputNb) const OVERRIDE FINAL
    {
        return QString::number(inputNb + 1).toStdString();
    }

    virtual RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL
    {
        return eRenderSafetyFullySafe;
    }

    virtual void addAcceptedComponents(int inputNb, std::list<ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    /*******************************************/


    ViewerRenderRetCode renderViewer_internal(ViewIdx view,
                                              bool singleThreaded,
                                              bool isSequentialRender,
                                              U64 viewerHash,
                                              bool canAbort,
                                              const NodePtr& rotoPaintNode,
                                              const RotoStrokeItemPtr& strokeItem,
                                              bool isDoingRotoNeatRender,
                                              bool useTLS,
                                              const boost::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs>& request,
                                              const RenderStatsPtr& stats,
                                              ViewerArgs& inArgs) WARN_UNUSED_RETURN;


    virtual RenderEngine* createRenderEngine() OVERRIDE FINAL WARN_UNUSED_RETURN;

private:

    boost::scoped_ptr<ViewerInstancePrivate> _imp;
};

inline ViewerInstancePtr
toViewerInstance(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<ViewerInstance>(effect);
}

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_VIEWER_INSTANCE_H
