//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_VIEWERNODE_H_
#define NATRON_ENGINE_VIEWERNODE_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <string>

#include "Global/Macros.h"
//#include "Engine/Rect.h"
#include "Engine/EffectInstance.h"

class ParallelRenderArgsSetter;
class RenderingFlagSetter;
struct RequestedFrame;
namespace Natron {
class Image;
class FrameEntry;
namespace Color {
class Lut;
}
}
class UpdateViewerParams;
class TimeLine;
class OpenGLViewerI;
struct TextureRect;

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
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)
    
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
                                    const boost::shared_ptr<RequestedFrame>& request) WARN_UNUSED_RETURN;
    
    Natron::StatusEnum getViewerArgsAndRenderViewer(SequenceTime time,
                                                    bool canAbort,
                                                    int view,
                                                    U64 viewerHash,
                                                    const boost::shared_ptr<Natron::Node>& rotoPaintNode,
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
    
    virtual void restoreClipPreferences() OVERRIDE FINAL;

    void setInputA(int inputNb);

    void setInputB(int inputNb);

    void getActiveInputs(int & a,int &b) const;
    
    int getLastRenderedTime() const;
    
    virtual double getCurrentTime() const OVERRIDE WARN_UNUSED_RETURN;
    
    virtual int getCurrentView() const OVERRIDE WARN_UNUSED_RETURN;

    boost::shared_ptr<TimeLine> getTimeline() const;
    
    void getTimelineBounds(int* first,int* last) const;
    
    static const Natron::Color::Lut* lutFromColorspace(Natron::ViewerColorSpaceEnum cs) WARN_UNUSED_RETURN;
    
    virtual void checkOFXClipPreferences(double time,
                                         const RenderScale & scale,
                                         const std::string & reason,
                                         bool forceGetClipPrefAction) OVERRIDE FINAL;
    
    void callRedrawOnMainThread() { Q_EMIT s_callRedrawOnMainThread(); }
    
    struct ViewerInstancePrivate;
    
    float interpolateGammaLut(float value);
    
    void markAllOnRendersAsAborted();
    
public Q_SLOTS:
    
    void s_viewerRenderingStarted() { Q_EMIT viewerRenderingStarted(); }
    
    void s_viewerRenderingEnded() { Q_EMIT viewerRenderingEnded(); }


    void onMipMapLevelChanged(int level);


    /**
     * @brief Redraws the OpenGL viewer. Can only be called on the main-thread.
     **/
    void redrawViewer();

  
    void executeDisconnectTextureRequestOnMainThread(int index);


Q_SIGNALS:
        
    void s_callRedrawOnMainThread();

    void viewerDisconnected();
    
    void refreshOptionalState();

    void activeInputsChanged();
    
    void clipPreferencesChanged();

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
    virtual std::string getDescription() const OVERRIDE FINAL
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
                                             ViewerArgs& inArgs) WARN_UNUSED_RETURN;
    
    
    virtual RenderEngine* createRenderEngine() OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    
    void setCurrentlyUpdatingOpenGLViewer(bool updating);
    bool isCurrentlyUpdatingOpenGLViewer() const;
private:
    
    boost::scoped_ptr<ViewerInstancePrivate> _imp;
};

//} // namespace Natron
#endif // NATRON_ENGINE_VIEWERNODE_H_
