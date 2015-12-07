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

#ifndef NATRON_ENGINE_VIEWERNODE_H
#define NATRON_ENGINE_VIEWERNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>

#include "Global/Macros.h"

#include "Engine/OutputEffectInstance.h"
#include "Engine/EngineFwd.h"

class UpdateViewerParams; // ViewerInstancePrivate

typedef std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats > RenderStatsMap;

struct ViewerArgs
{
    Natron::EffectInstance* activeInputToRender;
    bool forceRender;
    int activeInputIndex;
    U64 activeInputHash;
    boost::shared_ptr<Natron::FrameKey> key;
    boost::shared_ptr<UpdateViewerParams> params;
    boost::shared_ptr<RenderingFlagSetter> isRenderingFlag;
    bool draftModeEnabled;
    bool autoContrast;
    Natron::DisplayChannelsEnum channels;
    bool userRoIEnabled;
};

class ViewerInstance
: public Natron::OutputEffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
    friend class ViewerCurrentFrameRequestScheduler;
    
public:
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n) WARN_UNUSED_RETURN;

    ViewerInstance(boost::shared_ptr<Natron::Node> node);

    virtual ~ViewerInstance();
    
    OpenGLViewerI* getUiContext() const WARN_UNUSED_RETURN;

    ///Called upon node creation and then never changed
    void setUiContext(OpenGLViewerI* viewer);

    /**
     * @brief Set the uiContext pointer to NULL, preventing the gui to be deleted twice when
     * the node is deleted.
     **/
    void invalidateUiContext();

    
    
    Natron::StatusEnum getRenderViewerArgsAndCheckCache_public(SequenceTime time,
                                                        bool isSequential,
                                                        bool canAbort,
                                                        int view,
                                                        int textureIndex,
                                                        U64 viewerHash,
                                                        const boost::shared_ptr<Natron::Node>& rotoPaintNode,
                                                        bool useTLS,
                                                        const boost::shared_ptr<RenderStats>& stats,
                                                        ViewerArgs* outArgs);

    
private:
    /**
     * @brief Look-up the cache and try to find a matching texture for the portion to render.
     **/
    Natron::StatusEnum getRenderViewerArgsAndCheckCache(SequenceTime time,
                                                        bool isSequential,
                                                        bool canAbort,
                                                        int view,
                                                        int textureIndex,
                                                        U64 viewerHash,
                                                        const boost::shared_ptr<Natron::Node>& rotoPaintNode,
                                                        bool useTLS,
                                                        U64 renderAge,
                                                        const boost::shared_ptr<RenderStats>& stats,
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
    Natron::StatusEnum renderViewer(int view,bool singleThreaded,bool isSequentialRender,
                                    U64 viewerHash,
                                    bool canAbort,
                                    const boost::shared_ptr<Natron::Node>& rotoPaintNode,
                                    bool useTLS,
                                    boost::shared_ptr<ViewerArgs> args[2],
                                    const boost::shared_ptr<RequestedFrame>& request,
                                    const boost::shared_ptr<RenderStats>& stats) WARN_UNUSED_RETURN;
    
    Natron::StatusEnum getViewerArgsAndRenderViewer(SequenceTime time,
                                                    bool canAbort,
                                                    int view,
                                                    U64 viewerHash,
                                                    const boost::shared_ptr<Natron::Node>& rotoPaintNode,
                                                    const boost::shared_ptr<RotoStrokeItem>& strokeItem,
                                                    const boost::shared_ptr<RenderStats>& stats,
                                                    boost::shared_ptr<ViewerArgs>* argsA,
                                                    boost::shared_ptr<ViewerArgs>* argsB);


    void updateViewer(boost::shared_ptr<UpdateViewerParams> & frame);
    
    /**
     *@brief Bypasses the cache so the next frame will be rendered fully
     **/
    void forceFullComputationOnNextFrame();
    
    virtual void clearLastRenderedImage() OVERRIDE FINAL;

    void disconnectViewer();

    int getLutType() const WARN_UNUSED_RETURN;

    double getGain() const WARN_UNUSED_RETURN;

    int getMipMapLevel() const WARN_UNUSED_RETURN;

    int getMipMapLevelFromZoomFactor() const WARN_UNUSED_RETURN;

    Natron::DisplayChannelsEnum getChannels(int texIndex) const WARN_UNUSED_RETURN;

    /**
     * @brief This is a short-cut, this is primarily used when the user switch the
     * texture mode in the preferences menu. If the hardware doesn't support GLSL
     * it returns false, true otherwise. @see Settings::onKnobValueChanged
     **/
    bool supportsGLSL() const WARN_UNUSED_RETURN;
    
    virtual bool supportsMultipleClipsPAR() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    bool isRenderAbortable(int textureIndex, U64 renderAge) const;


    void setDisplayChannels(Natron::DisplayChannelsEnum channels, bool bothInputs);
    
    void setActiveLayer(const Natron::ImageComponents& layer, bool doRender);
    
    void setAlphaChannel(const Natron::ImageComponents& layer, const std::string& channelName, bool doRender);

    bool isAutoContrastEnabled() const WARN_UNUSED_RETURN;

    void onAutoContrastChanged(bool autoContrast,bool refresh);

    /**
     * @brief Returns the current view, MT-safe
     **/
    int getViewerCurrentView() const;

    void onGainChanged(double exp);
    
    void onGammaChanged(double value);
    
    double getGamma() const WARN_UNUSED_RETURN;

    void onColorSpaceChanged(Natron::ViewerColorSpaceEnum colorspace);

    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;
    
    void refreshActiveInputs(int inputNbChanged);
    
    void setInputA(int inputNb);

    void setInputB(int inputNb);

    void getActiveInputs(int & a,int &b) const;
    
    int getLastRenderedTime() const;
    
    virtual double getCurrentTime() const OVERRIDE WARN_UNUSED_RETURN;
    
    virtual int getCurrentView() const OVERRIDE WARN_UNUSED_RETURN;

    boost::shared_ptr<TimeLine> getTimeline() const;
    
    void getTimelineBounds(int* first,int* last) const;
    
    static const Natron::Color::Lut* lutFromColorspace(Natron::ViewerColorSpaceEnum cs) WARN_UNUSED_RETURN;
    
    virtual bool checkOFXClipPreferences(double time,
                                         const RenderScale & scale,
                                         const std::string & reason,
                                         bool forceGetClipPrefAction) OVERRIDE FINAL;
    
    virtual void onChannelsSelectorRefreshed() OVERRIDE FINAL;
    
    void callRedrawOnMainThread() { Q_EMIT s_callRedrawOnMainThread(); }
    
    struct ViewerInstancePrivate;
    
    float interpolateGammaLut(float value);
    
    void markAllOnRendersAsAborted();
    
    virtual void reportStats(int time, int view, double wallTime, const RenderStatsMap& stats) OVERRIDE FINAL;
    
    ///Only callable on MT
    void setActivateInputChangeRequestedFromViewer(bool fromViewer);
    
    bool isInputChangeRequestedFromViewer() const;
    
public Q_SLOTS:
    
    void s_viewerRenderingStarted() { Q_EMIT viewerRenderingStarted(); }
    
    void s_viewerRenderingEnded() { Q_EMIT viewerRenderingEnded(); }


    void onMipMapLevelChanged(int level);


    /**
     * @brief Redraws the OpenGL viewer. Can only be called on the main-thread.
     **/
    void redrawViewer();
    
    void redrawViewerNow();

  
    void executeDisconnectTextureRequestOnMainThread(int index);


Q_SIGNALS:
    
    void renderStatsAvailable(int time, int view, double wallTime, const RenderStatsMap& stats);
    
    void s_callRedrawOnMainThread();

    void viewerDisconnected();
    
    void refreshOptionalState();

    void activeInputsChanged();
    
    void clipPreferencesChanged();
    
    void availableComponentsChanged();

    void disconnectTextureRequest(int index);
    
    void viewerRenderingStarted();
    void viewerRenderingEnded();

private:
    /*******************************************
      *******OVERRIDEN FROM EFFECT INSTANCE******
    *******************************************/

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

    virtual void getFrameRange(double *first,double *last) OVERRIDE FINAL;
    virtual std::string getInputLabel(int inputNb) const OVERRIDE FINAL
    {
        return QString::number(inputNb + 1).toStdString();
    }

    virtual Natron::RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL
    {
        return Natron::eRenderSafetyFullySafe;
    }

    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    /*******************************************/
    
    
    Natron::StatusEnum renderViewer_internal(int view,
                                             bool singleThreaded,
                                             bool isSequentialRender,
                                             U64 viewerHash,
                                             bool canAbort,
                                             boost::shared_ptr<Natron::Node> rotoPaintNode,
                                             bool useTLS,
                                             const boost::shared_ptr<RequestedFrame>& request,
                                             const boost::shared_ptr<RenderStats>& stats,
                                             ViewerArgs& inArgs) WARN_UNUSED_RETURN;
    
    
    virtual RenderEngine* createRenderEngine() OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    
    void setCurrentlyUpdatingOpenGLViewer(bool updating);
    bool isCurrentlyUpdatingOpenGLViewer() const;
private:
    
    boost::scoped_ptr<ViewerInstancePrivate> _imp;
};

//} // namespace Natron
#endif // NATRON_ENGINE_VIEWERNODE_H
