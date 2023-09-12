/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef NATRON_ENGINE_VIEWERNODE_H
#define NATRON_ENGINE_VIEWERNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include "Engine/OutputEffectInstance.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class UpdateViewerParams; // ViewerInstancePrivate

typedef std::map<NodePtr, NodeRenderStats > RenderStatsMap;

class ViewerArgs
{
public:
    EffectInstancePtr activeInputToRender;
    bool forceRender;
    int activeInputIndex;
    U64 activeInputHash;
    UpdateViewerParamsPtr params;
    RenderingFlagSetterPtr isRenderingFlag;
    bool draftModeEnabled;
    unsigned int mipmapLevelWithDraft, mipmapLevelWithoutDraft;
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

public:
    static EffectInstance* BuildEffect(NodePtr n) WARN_UNUSED_RETURN;

    ViewerInstance(NodePtr node);

    virtual ~ViewerInstance();

    OpenGLViewerI* getUiContext() const WARN_UNUSED_RETURN;

    ///Called upon node creation and then never changed
    void setUiContext(OpenGLViewerI* viewer);

    virtual bool supportsMultipleClipDepths() const OVERRIDE FINAL
    {
        return true;
    }

    virtual bool supportsMultipleClipFPSs() const OVERRIDE FINAL
    {
        return true;
    }

    /**
     * @brief Set the uiContext pointer to NULL, preventing the gui to be deleted twice when
     * the node is deleted.
     **/
    void invalidateUiContext();
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
     * @brief Get the RoI from the Viewer and lookup the cache for a texture at the given mipmapLevel.
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
                                     bool useTLS,
                                     ViewerArgsPtr args[2],
                                     const ViewerCurrentFrameRequestSchedulerStartArgsPtr& request,
                                     const RenderStatsPtr& stats) WARN_UNUSED_RETURN;

    ViewerRenderRetCode getViewerArgsAndRenderViewer(SequenceTime time,
                                                     bool canAbort,
                                                     ViewIdx view,
                                                     U64 viewerHash,
                                                     const NodePtr& rotoPaintNode,
                                                     const RotoStrokeItemPtr& strokeItem,
                                                     const RenderStatsPtr& stats,
                                                     ViewerArgsPtr* argsA,
                                                     ViewerArgsPtr* argsB);

    void aboutToUpdateTextures();

    void updateViewer(UpdateViewerParamsPtr & frame);

    virtual bool getMakeSettingsPanel() const OVERRIDE FINAL { return false; }

    /**
     *@brief Bypasses the cache so the next frame will be rendered fully
     **/
    void forceFullComputationOnNextFrame();
    virtual void clearLastRenderedImage() OVERRIDE FINAL;

    void disconnectViewer();

    void disconnectTexture(int index, bool clearRod);

    int getLutType() const WARN_UNUSED_RETURN;

    double getGain() const WARN_UNUSED_RETURN;

    unsigned int getMipmapLevel() const WARN_UNUSED_RETURN;

    unsigned int getMipmapLevelFromZoomFactor() const WARN_UNUSED_RETURN;

    DisplayChannelsEnum getChannels(int texIndex) const WARN_UNUSED_RETURN;

    void setFullFrameProcessingEnabled(bool fullFrame);
    bool isFullFrameProcessingEnabled() const;


    virtual bool supportsMultipleClipPARs() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    bool isLatestRender(int textureIndex, U64 renderAge) const;


    void setDisplayChannels(DisplayChannelsEnum channels, bool bothInputs);

    void setActiveLayer(const ImagePlaneDesc& layer, bool doRender);

    void setAlphaChannel(const ImagePlaneDesc& layer, const std::string& channelName, bool doRender);

    bool isAutoContrastEnabled() const WARN_UNUSED_RETURN;

    void onAutoContrastChanged(bool autoContrast, bool refresh);

    /**
     * @brief Returns the current view, MT-safe
     **/
    ViewIdx getViewerCurrentView() const;

    void onGainChanged(double exp);

    void onGammaChanged(double value);

    double getGamma() const WARN_UNUSED_RETURN;

    void onColorSpaceChanged(ViewerColorSpaceEnum colorspace);

    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;

    void getActiveInputs(int & a, int &b) const;

    void setInputA(int inputNb);

    void setInputB(int inputNb);

    int getLastRenderedTime() const;

    virtual double getCurrentTime() const OVERRIDE WARN_UNUSED_RETURN;
    virtual ViewIdx getCurrentView() const OVERRIDE WARN_UNUSED_RETURN;
    TimeLinePtr getTimeline() const;

    void getTimelineBounds(int* first, int* last) const;

    static const Color::Lut* lutFromColorspace(ViewerColorSpaceEnum cs) WARN_UNUSED_RETURN;
    virtual void onMetadataRefreshed(const NodeMetadata& metadata) OVERRIDE FINAL;
    virtual void onChannelsSelectorRefreshed() OVERRIDE FINAL;

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

    unsigned int getViewerMipmapLevel() const;

public Q_SLOTS:


    void onMipmapLevelChanged(unsigned int level);


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

    void availableComponentsChanged();

    void disconnectTextureRequest(int index,bool clearRoD);

    void viewerRenderingStarted();
    void viewerRenderingEnded();

private:
    /*******************************************
     ******OVERRIDDEN FROM EFFECT INSTANCE******
     *******************************************/

    virtual bool isOutput() const OVERRIDE FINAL
    {
        return true;
    }

    virtual int getNInputs() const OVERRIDE FINAL;
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
        return PLUGINID_NATRON_VIEWER;
    }

    virtual std::string getPluginLabel() const OVERRIDE FINAL
    {
        return "Viewer";
    }

    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL;
    virtual std::string getPluginDescription() const OVERRIDE FINAL
    {
        return "The Viewer node can display the output of a node graph.";
    }

    virtual void getFrameRange(double *first, double *last) OVERRIDE FINAL;
    virtual std::string getInputLabel(int inputNb) const OVERRIDE FINAL
    {
        return QString::number(inputNb + 1).toStdString();
    }

    virtual RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL
    {
        return eRenderSafetyFullySafe;
    }

    virtual void addAcceptedComponents(int inputNb, std::list<ImagePlaneDesc>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    /*******************************************/


    ViewerRenderRetCode renderViewer_internal(ViewIdx view,
                                              bool singleThreaded,
                                              bool isSequentialRender,
                                              U64 viewerHash,
                                              bool canAbort,
                                              const NodePtr& rotoPaintNode,
                                              bool useTLS,
                                              const ViewerCurrentFrameRequestSchedulerStartArgsPtr& request,
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

    std::unique_ptr<ViewerInstancePrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_VIEWERNODE_H
