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

#include <string>

#include "Global/Macros.h"
#include "Engine/Rect.h"
#include "Engine/EffectInstance.h"

class ParallelRenderArgsSetter;
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

class ViewerInstance
: public Natron::OutputEffectInstance
{
    Q_OBJECT
    
public:
    
    
    
    enum DisplayChannelsEnum
    {
        eDisplayChannelsRGB = 0,
        eDisplayChannelsR,
        eDisplayChannelsG,
        eDisplayChannelsB,
        eDisplayChannelsA,
        eDisplayChannelsY
    };

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

    struct ViewerArgs
    {
        Natron::EffectInstance* activeInputToRender;
        bool forceRender;
        int activeInputIndex;
        U64 activeInputHash;
        boost::shared_ptr<Natron::FrameKey> key;
        boost::shared_ptr<UpdateViewerParams> params;
        boost::shared_ptr<ParallelRenderArgsSetter> frameArgs;
    };
    
    /**
     * @brief Look-up the cache and try to find a matching texture for the portion to render.
     **/
    Natron::StatusEnum getRenderViewerArgsAndCheckCache(SequenceTime time,
                                                        bool isSequential,
                                                        bool canAbort,
                                                        int view, int textureIndex, U64 viewerHash,
                                                        ViewerArgs* outArgs);

    
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
                                boost::shared_ptr<ViewerInstance::ViewerArgs> args[2]) WARN_UNUSED_RETURN;


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

    DisplayChannelsEnum getChannels() const WARN_UNUSED_RETURN;

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


    void setDisplayChannels(DisplayChannelsEnum channels);


    bool isAutoContrastEnabled() const WARN_UNUSED_RETURN;

    void onAutoContrastChanged(bool autoContrast,bool refresh);

    /**
     * @brief Returns the current view, MT-safe
     **/
    int getViewerCurrentView() const;

    void onGainChanged(double exp);

    void onColorSpaceChanged(Natron::ViewerColorSpaceEnum colorspace);

    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;
    
    virtual void restoreClipPreferences() OVERRIDE FINAL;

    void setInputA(int inputNb);

    void setInputB(int inputNb);

    void getActiveInputs(int & a,int &b) const;
    
    int getLastRenderedTime() const;
    
    virtual SequenceTime getCurrentTime() const OVERRIDE WARN_UNUSED_RETURN;
    
    virtual int getCurrentView() const OVERRIDE WARN_UNUSED_RETURN;

    boost::shared_ptr<TimeLine> getTimeline() const;
    
    void getTimelineBounds(int* first,int* last) const;
    
    static const Natron::Color::Lut* lutFromColorspace(Natron::ViewerColorSpaceEnum cs) WARN_UNUSED_RETURN;
    
    virtual void checkOFXClipPreferences(double time,
                                         const RenderScale & scale,
                                         const std::string & reason,
                                         bool forceGetClipPrefAction) OVERRIDE FINAL;
    
    void callRedrawOnMainThread() { emit s_callRedrawOnMainThread(); }

    void s_viewerRenderingStarted() { emit viewerRenderingStarted(); }
    
    void s_viewerRenderingEnded() { emit viewerRenderingEnded(); }
    
    struct ViewerInstancePrivate;

public slots:


    void onMipMapLevelChanged(int level);

    void onNodeNameChanged(const QString &);

    /**
     * @brief Redraws the OpenGL viewer. Can only be called on the main-thread.
     **/
    void redrawViewer();

  
    void executeDisconnectTextureRequestOnMainThread(int index);


signals:
    
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

    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE FINAL;
    virtual std::string getInputLabel(int inputNb) const OVERRIDE FINAL
    {
        return QString::number(inputNb + 1).toStdString();
    }

    virtual Natron::EffectInstance::RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL
    {
        return Natron::EffectInstance::eRenderSafetyFullySafe;
    }

    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponentsEnum>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    /*******************************************/
    
    
    Natron::StatusEnum renderViewer_internal(int view,
                                             bool singleThreaded,
                                             bool isSequentialRender,
                                             U64 viewerHash,
                                             bool canAbort,
                                             const ViewerArgs& inArgs) WARN_UNUSED_RETURN;

    virtual RenderEngine* createRenderEngine() OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    
private:
    
    boost::scoped_ptr<ViewerInstancePrivate> _imp;
};

//} // namespace Natron
#endif // NATRON_ENGINE_VIEWERNODE_H_
