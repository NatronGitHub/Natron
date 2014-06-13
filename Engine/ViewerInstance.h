//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_ENGINE_VIEWERNODE_H_
#define NATRON_ENGINE_VIEWERNODE_H_

#include <string>

#include "Global/Macros.h"
#include "Engine/Rect.h"
#include "Engine/EffectInstance.h"

namespace Natron{
class Image;
namespace Color{
class Lut;
}
}
class OpenGLViewerI;
struct TextureRect;

//namespace Natron {

class ViewerInstance : public QObject, public Natron::OutputEffectInstance
{
    Q_OBJECT
    
public:
    enum DisplayChannels{
        RGB = 0,
        R,
        G,
        B,
        A,
        LUMINANCE
    };
    

public:
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n) WARN_UNUSED_RETURN;
    
    ViewerInstance(boost::shared_ptr<Natron::Node> node);
    
    virtual ~ViewerInstance();
    
    OpenGLViewerI* getUiContext() const WARN_UNUSED_RETURN;

    void setUiContext(OpenGLViewerI* viewer);
    
    /**
     * @brief Set the uiContext pointer to NULL, preventing the gui to be deleted twice when 
     * the node is deleted.
     **/
    void invalidateUiContext();

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
    Natron::Status renderViewer(SequenceTime time,bool singleThreaded,bool isSequentialRender) WARN_UNUSED_RETURN;


    /**
     *@brief Bypasses the cache so the next frame will be rendered fully
     **/
    void forceFullComputationOnNextFrame();

    /**
     * @brief Activates the SLOT onViewerCacheFrameAdded() and the SIGNALS removedLRUCachedFrame() and  clearedViewerCache()
     * by connecting them to the ViewerCache emitted signals. They in turn are used by the GUI to refresh the "cached line" on
     * the timeline.
     **/
    void connectSlotsToViewerCache();

    /**
     * @brief Deactivates the SLOT onViewerCacheFrameAdded() and the SIGNALS removedLRUCachedFrame() and  clearedViewerCache().
     * This is used when the user has several main windows. Since the ViewerCache is global to the application, we don't want
     * a main window (an AppInstance) draw some cached line because another instance is running some playback or rendering something.
     **/
    void disconnectSlotsToViewerCache();

    void disconnectViewer();

    /**
     * @brief This is used by the VideoEngine::abortRendering() function. abortRendering() might be called from the
     * main thread, i.e the OpenGL thread. However if the viewer is already uploading a texture to OpenGL (i.e: the
     * wait condition usingOpenGL is true) then it will deadlock for sure. What we want to do here is just wake-up the
     * thread (the render thread) waiting for the main-thread to be done with OpenGL.
     **/
    void wakeUpAnySleepingThread();

    int activeInput() const WARN_UNUSED_RETURN;

    int getLutType() const WARN_UNUSED_RETURN ;

    double getGain() const WARN_UNUSED_RETURN ;
    
    int getMipMapLevel() const WARN_UNUSED_RETURN;

    DisplayChannels getChannels() const WARN_UNUSED_RETURN;

    /**
     * @brief This is a short-cut, this is primarily used when the user switch the
     * texture mode in the preferences menu. If the hardware doesn't support GLSL
     * it returns false, true otherwise. @see Settings::onKnobValueChanged
     **/
    bool supportsGLSL() const WARN_UNUSED_RETURN;


    void setDisplayChannels(DisplayChannels channels) ;

    /**
     * @brief Get the color of the currently displayed image at position x,y. 
     * If forceLinear is true, then it will not use the viewer current colorspace
     * to get r,g and b values, otherwise the color returned will be in the same color-space
     * than the one chosen by the user on the gui.
     * @return true if the point is inside the image and colors were set
    **/
    bool getColorAt(int x,int y,float* r,float* g,float* b,float* a,bool forceLinear,int textureIndex) WARN_UNUSED_RETURN;

    bool isAutoContrastEnabled() const WARN_UNUSED_RETURN;

    void onAutoContrastChanged(bool autoContrast,bool refresh);
    
    /**
     * @brief Returns the current view, MT-safe
     **/
    int getCurrentView() const;
    
    void onGainChanged(double exp);
    
    void onColorSpaceChanged(Natron::ViewerColorSpace colorspace);
    
    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;
    
    void setInputA(int inputNb);
    
    void setInputB(int inputNb);
    
    void getActiveInputs(int& a,int &b) const;

public slots:

    void onViewerCacheFrameAdded();
    
    void onMipMapLevelChanged(int level);

    void onNodeNameChanged(const QString&);

    /**
     * @brief Redraws the OpenGL viewer. Can only be called on the main-thread.
     **/
    void redrawViewer();

    /**
     * @brief Called by the Histogram when it wants to refresh. It returns a pointer to the last
     * rendered image by the viewer.
     **/
    boost::shared_ptr<Natron::Image> getLastRenderedImage(int textureIndex) const;

    void executeDisconnectTextureRequestOnMainThread(int index);
signals:
    
    ///Emitted when the image bit depth and components changes
    void imageFormatChanged(int,int,int);

    void rodChanged(RectI,int);
    
    void viewerDisconnected();

    void addedCachedFrame(SequenceTime);

    void removedLRUCachedFrame();

    void clearedViewerCache();
    
    void activeInputsChanged();
    
    void disconnectTextureRequest(int index);
private:
    /*******************************************
     *******OVERRIDEN FROM EFFECT INSTANCE******
     *******************************************/
    
    virtual bool isOutput() const OVERRIDE FINAL {return true;}
    
    virtual int maximumInputs() const OVERRIDE FINAL;

    virtual bool isInputOptional(int /*n*/) const OVERRIDE FINAL;

    virtual int majorVersion() const OVERRIDE FINAL { return 1; }

    virtual int minorVersion() const OVERRIDE FINAL { return 0; }

    virtual std::string pluginID() const OVERRIDE FINAL {return "Viewer";}

    virtual std::string pluginLabel() const OVERRIDE FINAL {return "Viewer";}
    
    virtual std::string description() const OVERRIDE FINAL {return "The Viewer node can display the output of a node graph.";}
    
    virtual Natron::Status getRegionOfDefinition(SequenceTime time,const RenderScale& scale,int view,
                                                 RectI* rod,bool* isProjectFormat) OVERRIDE FINAL;
        
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE FINAL;
    
    virtual std::string inputLabel(int inputNb) const OVERRIDE FINAL {
        return QString::number(inputNb+1).toStdString();
    }
    virtual Natron::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE FINAL {return Natron::EffectInstance::FULLY_SAFE;}
    
    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;
    
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const OVERRIDE FINAL;
    /*******************************************/

    
    Natron::Status renderViewer_internal(SequenceTime time,bool singleThreaded,bool isSequentialRender,
                                         int textureIndex) WARN_UNUSED_RETURN;

private:
    struct ViewerInstancePrivate;
    boost::scoped_ptr<ViewerInstancePrivate> _imp;
};
//} // namespace Natron
#endif // NATRON_ENGINE_VIEWERNODE_H_
