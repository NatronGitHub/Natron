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
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

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

    ViewerNodePtr shared_from_this() {
        return boost::dynamic_pointer_cast<ViewerNode>(KnobHolder::shared_from_this());
    }

    virtual ~ViewerNode();

    OpenGLViewerI* getUiContext() const WARN_UNUSED_RETURN;

    ///Called upon node creation and then never changed
    void setUiContext(OpenGLViewerI* viewer);

    virtual void getPluginShortcuts(std::list<PluginActionShortcut>* shortcuts) const OVERRIDE FINAL;


    virtual bool isOutput() const OVERRIDE FINAL
    {
        return true;
    }

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
        return PLUGINID_NATRON_VIEWER_GROUP;
    }

    virtual std::string getPluginLabel() const OVERRIDE FINAL
    {
        return "Viewer";
    }

    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL;
    virtual std::string getPluginDescription() const OVERRIDE FINAL
    {
        return "The Viewer node can display the output of a node graph. Shift + double click on the viewer node to customize the viewer display process with a custom node tree.";
    }


    /**
     * @brief Set the uiContext pointer to NULL, preventing the gui to be deleted twice when
     * the node is deleted.
     **/
    void invalidateUiContext();

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

    int getLutType() const WARN_UNUSED_RETURN;

    double getGain() const WARN_UNUSED_RETURN;

    int getMipMapLevel() const WARN_UNUSED_RETURN;

    int getMipMapLevelFromZoomFactor() const WARN_UNUSED_RETURN;

    DisplayChannelsEnum getChannels(int texIndex) const WARN_UNUSED_RETURN;

    void setFullFrameProcessingEnabled(bool fullFrame);
    bool isFullFrameProcessingEnabled() const;

    bool isLatestRender(int textureIndex, U64 renderAge) const;


    void setDisplayChannels(DisplayChannelsEnum channels, bool bothInputs);

    void setActiveLayer(const ImageComponents& layer, bool doRender);

    void setAlphaChannel(const ImageComponents& layer, const std::string& channelName, bool doRender);

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
    virtual void onMetaDatasRefreshed(const NodeMetadata& metadata) OVERRIDE FINAL;
    virtual void onChannelsSelectorRefreshed() OVERRIDE FINAL;

    bool isViewerUIVisible() const;

    void callRedrawOnMainThread() { Q_EMIT s_callRedrawOnMainThread(); }

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

    void availableComponentsChanged();

    void disconnectTextureRequest(int index,bool clearRoD);

    void viewerRenderingStarted();
    void viewerRenderingEnded();

private:
    /*******************************************
       *******OVERRIDEN FROM EFFECT INSTANCE******
     *******************************************/

    virtual bool onOverlayPenDown(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, PenType pen) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool knobChanged(const KnobIPtr& k, ValueChangedReasonEnum reason,
                             ViewSpec /*view*/,
                             double /*time*/,
                             bool /*originatedFromMainThread*/) OVERRIDE FINAL;


    virtual void initializeKnobs() OVERRIDE FINAL;


    /*******************************************/

private:

    boost::scoped_ptr<ViewerNodePrivate> _imp;
};

inline ViewerNodePtr
toViewerNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<ViewerNode>(effect);
}

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_VIEWERNODE_H
