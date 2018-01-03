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

#ifndef NATRON_ENGINE_VIEWERNODE_H
#define NATRON_ENGINE_VIEWERNODE_H

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

#include "Engine/NodeGroup.h"
#include "Engine/ViewIdx.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#define kViewerNodeParamPlayerToolBarPage "playerToolBar"

#define kViewerNodeParamSetInPoint "setInPoint"
#define kViewerNodeParamSetInPointLabel "Set In Point"
#define kViewerNodeParamSetInPointHint "Set the playback in point at the current frame"

#define kViewerNodeParamSetOutPoint "setOutPoint"
#define kViewerNodeParamSetOutPointLabel "Set Out Point"
#define kViewerNodeParamSetOutPointHint "Set the playback out point at the current frame"

#define kViewerNodeParamFirstFrame "goToFirstFrame"
#define kViewerNodeParamFirstFrameLabel "First Frame"
#define kViewerNodeParamFirstFrameHint "Moves the timeline cursor to the playback in point"

#define kViewerNodeParamPlayBackward "playBackward"
#define kViewerNodeParamPlayBackwardLabel "Play Backward"
#define kViewerNodeParamPlayBackwardHint "Starts playback backward. Press again or hit %2 to stop"

#define kViewerNodeParamCurrentFrame "currentFrame"
#define kViewerNodeParamCurrentFrameLabel "Current Frame"
#define kViewerNodeParamCurrentFrameHint "The current frame displayed in the Viewer"

#define kViewerNodeParamPlayForward "playForward"
#define kViewerNodeParamPlayForwardLabel "Play Forward"
#define kViewerNodeParamPlayForwardHint "Starts playback forward. Press again or hit %2 to stop"

#define kViewerNodeParamLastFrame "goToLastFrame"
#define kViewerNodeParamLastFrameLabel "Last Frame"
#define kViewerNodeParamLastFrameHint "Moves the timeline cursor to the playback out point"

#define kViewerNodeParamPreviousFrame "goToPreviousFrame"
#define kViewerNodeParamPreviousFrameLabel "Previous Frame"
#define kViewerNodeParamPreviousFrameHint "Moves the timeline cursor to the previous frame"

#define kViewerNodeParamNextFrame "goToNextFrame"
#define kViewerNodeParamNextFrameLabel "Next Frame"
#define kViewerNodeParamNextFrameHint "Moves the timeline cursor to the next frame"

#define kViewerNodeParamPreviousKeyFrame "goToPreviousKeyFrame"
#define kViewerNodeParamPreviousKeyFrameLabel "Previous KeyFrame"
#define kViewerNodeParamPreviousKeyFrameHint "Moves the timeline cursor to the previous keyframe"

#define kViewerNodeParamNextKeyFrame "goToNextKeyFrame"
#define kViewerNodeParamNextKeyFrameLabel "Next KeyFrame"
#define kViewerNodeParamNextKeyFrameHint "Moves the timeline cursor to the next keyframe"

#define kViewerNodeParamPreviousIncr "goToPreviousIncrement"
#define kViewerNodeParamPreviousIncrLabel "Previous Increment"
#define kViewerNodeParamPreviousIncrHint "Moves the timeline cursor backward by the number of frames indicated by the Frame Increment parameter"

#define kViewerNodeParamFrameIncrement "frameIncrement"
#define kViewerNodeParamFrameIncrementLabel "frameIncrement"
#define kViewerNodeParamFrameIncrementHint "This is the number of frame the timeline will step backward/forward when clicking on the Previous/Next "\
"Increment buttons"

#define kViewerNodeParamNextIncr "goToNextIncrement"
#define kViewerNodeParamNextIncrLabel "Next Increment"
#define kViewerNodeParamNextIncrHint "Moves the timeline cursor forward by the number of frames indicated by the Frame Increment parameter"

typedef std::map<NodePtr, NodeRenderStats > RenderStatsMap;


struct ViewerNodePrivate;
class ViewerNode
    : public NodeGroup
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from EffectInstance
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    ViewerNode(const NodePtr& node);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN;

    static PluginPtr createPlugin() WARN_UNUSED_RETURN;

    ViewerNodePtr shared_from_this() {
        return boost::dynamic_pointer_cast<ViewerNode>(KnobHolder::shared_from_this());
    }


    virtual ~ViewerNode();

    // This node is output as it provides a custom implementation of RenderEngine
    virtual bool isOutput() const OVERRIDE FINAL
    {
        return true;
    }

    virtual RenderEnginePtr createRenderEngine() OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void initializeOverlayInteract() OVERRIDE FINAL;

    /**
     * @brief Returns the actual viewer process node in the internal sub-graph for the given index.
     * Index must be either 0 or 1
     **/
    ViewerInstancePtr getViewerProcessNode(int index) const;


    // Called upon node creation and then never changed
    void setUiContext(OpenGLViewerI* viewer);

    /**
     * @brief Set the uiContext pointer to NULL, preventing the gui to be deleted twice when
     * the node is deleted.
     **/
    void invalidateUiContext();

    void refreshViewsKnobVisibility();
    
    OpenGLViewerI* getUiContext() const WARN_UNUSED_RETURN;

    NodePtr getCurrentAInput() const;

    NodePtr getCurrentBInput() const;

    void refreshInputFromChoiceMenu(int internalInputIdx);

    ViewerCompositingOperatorEnum getCurrentOperator() const;

    void connectInputToIndex(int groupInputIndex, int internalInputIndex);

    void setPickerEnabled(bool enabled);

    void setCurrentView(ViewIdx view);

    virtual ViewIdx getCurrentRenderView() const OVERRIDE FINAL;

    void setZoomComboBoxText(const std::string& text);

    DisplayChannelsEnum getDisplayChannels(int index) const;

    bool isAutoContrastEnabled() const;

    ViewerColorSpaceEnum getColorspace() const;

    void setRefreshButtonDown(bool down);

    bool isViewersSynchroEnabled() const;

    void setViewersSynchroEnabled(bool enabled);

    bool isViewerPaused(int index) const;

    bool isLeftToolbarVisible() const;

    bool isTopToolbarVisible() const;

    bool isTimelineVisible() const;

    bool isPlayerVisible() const;

    bool isClipToFormatEnabled() const;

    bool isInfoBarVisible() const;

    double getWipeAmount() const;

    double getWipeAngle() const;

    QPointF getWipeCenter() const;

    void resetWipe(); 

    bool isCheckerboardEnabled() const;

    bool isUserRoIEnabled() const;

    bool isOverlayEnabled() const;

    bool isFullFrameProcessingEnabled() const;

    RectD getUserRoI() const;

    void setUserRoI(const RectD& rect);

    int getDownscaleMipMapLevelKnobIndex() const;

    KnobChoicePtr getLayerKnob() const;

    KnobChoicePtr getAlphaChannelKnob() const;

    KnobIntPtr getPlaybackInPointKnob() const;

    KnobIntPtr getPlaybackOutPointKnob() const;

    KnobIntPtr getCurrentFrameKnob() const;

    KnobButtonPtr getTurboModeButtonKnob() const;

    virtual void setupInitialSubGraphState() OVERRIDE FINAL;

    virtual void loadSubGraph(const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                              const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization) OVERRIDE FINAL;

    bool isViewerUIVisible() const;

    TimeLinePtr getTimeline() const;

    virtual TimeValue getTimelineCurrentTime() const OVERRIDE FINAL;

    TimeValue getLastRenderedTime() const;

    void getTimelineBounds(int* first, int* last) const;

    double getUIZoomFactor() const;
    
    unsigned int getMipMapLevelFromZoomFactor() const;

    struct UpdateViewerArgs
    {
        TimeValue time;
        ViewIdx view;
        OpenGLViewerI::TextureTransferArgs::TypeEnum type;

        struct TextureUpload
        {

            // A pointer to the image on CPU
            ImagePtr image;

            // Pointers to the color picker images
            ImagePtr colorPickerImage, colorPickerInputImage;

            // The hash of the viewer process node
            ImageCacheKeyPtr viewerProcessImageKey;
        };
        std::list<TextureUpload> viewerUploads[2];
        bool recenterViewer;
        Point viewerCenter;
    };

    void updateViewer(const UpdateViewerArgs& args);

    void disconnectViewer();

    void disconnectTexture(int index, bool clearRod);
    
    void reportStats(int time , double wallTime, const RenderStatsMap& stats) ;

    void refreshFps();

    void s_renderStatsAvailable(int time, double wallTime, const RenderStatsMap& stats)
    {
        Q_EMIT renderStatsAvailable(time, wallTime, stats);
    }

    void s_redrawOnMainThread()
    {
        Q_EMIT redrawOnMainThread();
    }

    void s_viewerDisconnected()
    {
        Q_EMIT viewerDisconnected();
    }

    void s_disconnectTextureRequest(int index,bool clearRoD)
    {
        Q_EMIT disconnectTextureRequest(index, clearRoD);
    }

    virtual void onKnobsLoaded() OVERRIDE FINAL;

    void forceNextRenderWithoutCacheRead();

    bool isRenderWithoutCacheEnabledAndTurnOff();

    /**
     * @brief Used to re-render only selected portions of the texture.
     * This requires that the renderviewer_internal() function gets called on a single thread
     * because the texture will get resized (i.e copied and swapped) to fit new RoIs.
     * After this call, the function isDoingPartialUpdates() will return true until
     * clearPartialUpdateParams() gets called.
     **/
    void setPartialUpdateParams(const std::list<RectD>& rois, bool recenterViewer);
    std::list<RectD> getPartialUpdateRects() const;
    void clearPartialUpdateParams();

    bool getViewerCenterPoint(Point* center) const;

    void setDoingPartialUpdates(bool doing);
    bool isDoingPartialUpdates() const;

    void onViewerProcessNodeMetadataRefreshed(const NodePtr& viewerProcessNode, const NodeMetadata& metadata);

    virtual void clearLastRenderedImage() OVERRIDE FINAL;


public Q_SLOTS:


    void onInputNameChanged(int,QString);

    void executeDisconnectTextureRequestOnMainThread(int index, bool clearRoD);

    void redrawViewer();

    void redrawViewerNow();

    void onEngineStopped();
    
    void onEngineStarted(bool forward);

    void onSetDownPlaybackButtonsTimeout();

Q_SIGNALS:

    void renderStatsAvailable(int time, double wallTime, const RenderStatsMap& stats);

    void redrawOnMainThread();

    void viewerDisconnected();

    void disconnectTextureRequest(int index,bool clearRoD);

    void internalViewerCreated();

private:


    void createViewerProcessNode();

    void setupGraph(bool createViewerProcess);

    void clearGroupWithoutViewerProcess();

    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;


    virtual bool knobChanged(const KnobIPtr& k, ValueChangedReasonEnum reason,
                             ViewSetSpec /*view*/,
                             TimeValue /*time*/) OVERRIDE FINAL;


    virtual void initializeKnobs() OVERRIDE FINAL;


private:

    boost::scoped_ptr<ViewerNodePrivate> _imp;
};

inline ViewerNodePtr
toViewerNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<ViewerNode>(effect);
}

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_VIEWERNODE_H
