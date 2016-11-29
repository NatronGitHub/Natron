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
    boost::shared_ptr<ParallelRenderArgsSetter> frameArgs;
    bool draftModeEnabled;
    unsigned int mipMapLevelWithDraft, mipmapLevelWithoutDraft;
    bool autoContrast;
    DisplayChannelsEnum channels;
    bool userRoIEnabled;
    bool mustComputeRoDAndLookupCache;
    bool isDoingPartialUpdates;
    bool useViewerCache;
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

    static PluginPtr createPlugin();

    virtual ~ViewerInstance();

    ViewerNodePtr getViewerNodeGroup() const;

    OpenGLViewerI* getUiContext() const WARN_UNUSED_RETURN;

    virtual bool supportsMultipleClipDepths() const OVERRIDE FINAL
    {
        return true;
    }

    virtual bool supportsMultipleClipFPSs() const OVERRIDE FINAL
    {
        return true;
    }

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
                                                                bool canAbort,
                                                                const RotoStrokeItemPtr& activeStrokeItem,
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
                                                         const RotoStrokeItemPtr& activeStrokeItem,
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
                                     const RotoStrokeItemPtr& activeStrokeItem,
                                     bool isDoingRotoNeatRender,
                                     boost::shared_ptr<ViewerArgs> args[2],
                                     const boost::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs>& request,
                                     const RenderStatsPtr& stats) WARN_UNUSED_RETURN;



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

    int getMipMapLevelFromZoomFactor() const WARN_UNUSED_RETURN;

    DisplayChannelsEnum getChannels(int texIndex) const WARN_UNUSED_RETURN;

    virtual bool supportsMultipleClipPARs() const OVERRIDE FINAL WARN_UNUSED_RETURN
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

    void getInputsComponentsAvailables(std::set<ImageComponents>* comps) const;

    NodePtr getInputRecursive(int inputIndex) const;

    void setCurrentLayer(const ImageComponents& layer);

    void setAlphaChannel(const ImageComponents& layer, const std::string& channelName);

    void redrawViewer();

    void redrawViewerNow();

    void callRedrawOnMainThread();

    void fillGammaLut(double gamma);

private:



    void refreshLayerAndAlphaChannelComboBox();

    
    /*******************************************
       *******OVERRIDEN FROM EFFECT INSTANCE******
     *******************************************/


    virtual bool isOutput() const OVERRIDE FINAL
    {
        return true;
    }

    virtual int getMaxInputCount() const OVERRIDE FINAL;
    virtual bool isInputOptional(int /*n*/) const OVERRIDE FINAL;

    virtual std::string getInputLabel(int inputNb) const OVERRIDE FINAL;


    virtual void addAcceptedComponents(int inputNb, std::list<ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    /*******************************************/


    ViewerRenderRetCode renderViewer_internal(ViewIdx view,
                                              bool singleThreaded,
                                              bool isSequentialRender,
                                              const RotoStrokeItemPtr& activeStrokeItem,
                                              bool isDoingRotoNeatRender,
                                              const boost::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs>& request,
                                              const RenderStatsPtr& stats,
                                              ViewerArgs& inArgs) WARN_UNUSED_RETURN;

    virtual void getRegionsOfInterest(double time,
                                     const RenderScale & scale,
                                     const RectD & outputRoD,   //!< the RoD of the effect, in canonical coordinates
                                     const RectD & renderWindow,   //!< the region to be rendered in the output image, in Canonical Coordinates
                                     ViewIdx view,
                                     RoIMap* ret) OVERRIDE FINAL;


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
