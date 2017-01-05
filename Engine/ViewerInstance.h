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

#include "Engine/EffectInstance.h"
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
    TreeRenderPtr frameArgs;
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
    : public EffectInstance
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

#if 0
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
                                                         const RenderStatsPtr& stats,
                                                         ViewerArgs* outArgs);


    /**
     * @brief Setup the ViewerArgs struct with the info requested by the user from the Viewer UI
     **/
    void setupMinimalUpdateViewerParams(const SequenceTime time,
                                        const ViewIdx view,
                                        const int textureIndex,
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
                                     boost::shared_ptr<ViewerArgs> args[2],
                                     const boost::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs>& request,
                                     const RenderStatsPtr& stats) WARN_UNUSED_RETURN;

#endif

    
    virtual bool getMakeSettingsPanel() const OVERRIDE FINAL { return false; }

    
    virtual void clearLastRenderedImage() OVERRIDE FINAL;


    int getMipMapLevelFromZoomFactor() const WARN_UNUSED_RETURN;

    DisplayChannelsEnum getChannels(int texIndex) const WARN_UNUSED_RETURN;

    virtual bool supportsMultipleClipPARs() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    bool isLatestRender(int textureIndex, U64 renderAge) const;

    static const Color::Lut* lutFromColorspace(ViewerColorSpaceEnum cs) WARN_UNUSED_RETURN;
    
    virtual ActionRetCodeEnum getTimeInvariantMetaDatas(NodeMetadata& metadata) OVERRIDE FINAL;


    struct ViewerInstancePrivate;

    float interpolateGammaLut(float value);


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

    ///Only callable on MT
    void setActivateInputChangeRequestedFromViewer(bool fromViewer);

    bool isInputChangeRequestedFromViewer() const;

    void getInputsComponentsAvailables(std::set<ImageComponents>* comps) const;

    NodePtr getInputRecursive(int inputIndex) const;

    void setCurrentLayer(const ImageComponents& layer);

    void setAlphaChannel(const ImageComponents& layer, const std::string& channelName);


    void callRedrawOnMainThread();

    void fillGammaLut(double gamma);

private:



    void refreshLayerAndAlphaChannelComboBox();



    virtual bool isOutput() const OVERRIDE FINAL
    {
        return true;
    }

    virtual int getMaxInputCount() const OVERRIDE FINAL;
    virtual bool isInputOptional(int /*n*/) const OVERRIDE FINAL;

    virtual std::string getInputLabel(int inputNb) const OVERRIDE FINAL;


    virtual void addAcceptedComponents(int inputNb, std::list<ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

    virtual bool isMultiPlanar() const OVERRIDE FINAL;

    virtual EffectInstance::PassThroughEnum isPassThroughForNonRenderedPlanes() const OVERRIDE FINAL;

    virtual ActionRetCodeEnum attachOpenGLContext(TimeValue time, ViewIdx view, const RenderScale& scale, const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data) OVERRIDE FINAL;

    virtual ActionRetCodeEnum dettachOpenGLContext(const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, const EffectOpenGLContextDataPtr& data) OVERRIDE FINAL;

    virtual ActionRetCodeEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;

    ViewerRenderRetCode renderViewer_internal(ViewIdx view,
                                              bool singleThreaded,
                                              bool isSequentialRender,
                                              const RotoStrokeItemPtr& activeStrokeItem,
                                              const boost::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs>& request,
                                              const RenderStatsPtr& stats,
                                              ViewerArgs& inArgs) WARN_UNUSED_RETURN;



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
