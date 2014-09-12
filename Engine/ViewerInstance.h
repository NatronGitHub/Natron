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

namespace Natron {
class Image;
namespace Color {
class Lut;
}
}
class TimeLine;
class OpenGLViewerI;
struct TextureRect;

//namespace Natron {

class ViewerInstance
    : public QObject, public Natron::OutputEffectInstance
{
    Q_OBJECT

public:
    enum DisplayChannels
    {
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

    void disconnectViewer();

    int activeInput() const WARN_UNUSED_RETURN;

    int getLutType() const WARN_UNUSED_RETURN;

    double getGain() const WARN_UNUSED_RETURN;

    int getMipMapLevel() const WARN_UNUSED_RETURN;

    ///same as getMipMapLevel but with the zoomFactor taken into account
    int getMipMapLevelCombinedToZoomFactor() const WARN_UNUSED_RETURN;

    DisplayChannels getChannels() const WARN_UNUSED_RETURN;

    /**
     * @brief This is a short-cut, this is primarily used when the user switch the
     * texture mode in the preferences menu. If the hardware doesn't support GLSL
     * it returns false, true otherwise. @see Settings::onKnobValueChanged
     **/
    bool supportsGLSL() const WARN_UNUSED_RETURN;


    void setDisplayChannels(DisplayChannels channels);

    /**
     * @brief Get the color of the currently displayed image at position x,y.
     * @param forceLinear If true, then it will not use the viewer current colorspace
     * to get r,g and b values, otherwise the color returned will be in the same color-space
     * than the one chosen by the user on the gui.
     * X and Y are in CANONICAL COORDINATES
     * @return true if the point is inside the image and colors were set
     **/
    bool getColorAt(double x, double y, bool forceLinear, int textureIndex, float* r, float* g, float* b, float* a) WARN_UNUSED_RETURN;

    // same as getColor, but computes the mean over a given rectangle
    bool getColorAtRect(const RectD &rect, // rectangle in canonical coordinates
                        bool forceLinear, int textureIndex, float* r, float* g, float* b, float* a);

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

    void getActiveInputs(int & a,int &b) const;

    bool isFrameRangeLocked() const;

    boost::shared_ptr<TimeLine> getTimeline() const;
    
    /**
     * @brief Returns true when the main-thread is used to fill the OpenGL texture.
     * This also means that the render thread is waiting for it to be finished.
     **/
    bool isUpdatingOpenGLViewer() const;
    
public slots:


    void onMipMapLevelChanged(int level);

    void onNodeNameChanged(const QString &);

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

    void clearLastRenderedTexture();

signals:

    ///Emitted when the image bit depth and components changes
    void imageFormatChanged(int,int,int);

    void rodChanged(RectD, int);

    void viewerDisconnected();

    void activeInputsChanged();

    void disconnectTextureRequest(int index);

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
        return "Viewer";
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

    virtual Natron::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE FINAL
    {
        return Natron::EffectInstance::FULLY_SAFE;
    }

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
