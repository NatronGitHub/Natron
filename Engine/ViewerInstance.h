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
#include <QtCore/QFutureWatcher>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include "Global/Macros.h"
#include "Engine/Node.h"
#include "Engine/ImageInfo.h"
#include "Engine/EffectInstance.h"
#include "Gui/Texture.h"
class TabWidget;
class ViewerTab;
class AppInstance;
class Timer;
namespace Natron{
class Image;
namespace Color{
class Lut;
}
}

class QKeyEvent;



class ViewerInstance : public QObject, public Natron::OutputEffectInstance {
    

    Q_OBJECT
    
public:
    enum DisplayChannels{
        RGBA = 0,
        R,
        G,
        B,
        A,
        LUMINANCE
    };
    
    enum ViewerColorSpace{
        sRGB = 0,
        Linear,
        Rec709
    };
    
    enum BitDepth{
        BYTE = 0,
        HALF_FLOAT ,
        FLOAT
    };

private:
    
    
    struct InterThreadInfos{
        InterThreadInfos():
            _ramBuffer(NULL)
          , _textureRect()
          ,_bytesCount(0){}
        
        unsigned char* _ramBuffer;
        TextureRect _textureRect;
        size_t _bytesCount;
    };
    
    ViewerTab* _uiContext;
    
    int _pboIndex;
    
    int _frameCount;
    
    mutable QMutex _forceRenderMutex;
    bool _forceRender;/*!< true when we want to by-pass the cache*/
    
    
    QWaitCondition _usingOpenGLCond;
    mutable QMutex _usingOpenGLMutex; //!< protects _usingOpenGL
    bool _usingOpenGL;
    
    InterThreadInfos _interThreadInfos;
    
    unsigned char* _buffer;
    bool _mustFreeBuffer;
    
    QMutex _renderArgsMutex; //< protects exposure,colorspace etc..
    double _exposure ;/*!< Current exposure setting, all pixels are multiplied
                      by pow(2,expousre) before they appear on the screen.*/
    
    const Natron::Color::Lut* _colorSpace;/*!< The lut used to do the viewer colorspace conversion when we can't use shaders*/
    // FIXME: why a float to really represent an enum????
    ViewerColorSpace _lut; /*!< a value coding the current color-space used to render.
                 0 = sRGB ,  1 = linear , 2 = Rec 709*/
    
    DisplayChannels _channels;
    
    boost::shared_ptr<Natron::Image> _lastRenderedImage;
    
public:
    
    
    
    
    static Natron::EffectInstance* BuildEffect(Natron::Node* n) WARN_UNUSED_RETURN { return new ViewerInstance(n); }
    
    ViewerInstance(Natron::Node* node);
    
    virtual ~ViewerInstance();
    
    ViewerTab* getUiContext() const WARN_UNUSED_RETURN {return _uiContext;}
    
    void setUiContext(ViewerTab* ptr){_uiContext = ptr;}
    
    /*Add a new viewer tab to the GUI*/
    void initializeViewerTab(TabWidget* where);
    

    /**
     * @brief This function renders the image at time 'time' on the viewer.
     * It first get the region of definition of the image at the given time
     * and then deduce what is the region of interest on the viewer, according
     * to the current render scale.
     * Then it looks-up the ViewerCache to find an already existing frame,
     * in which case it copies directly the cached frame over to the PBO.
     * Otherwise it just calls renderRoi(...) on the active input and
     * and then render to the PBO.
     **/
    Natron::Status renderViewer(SequenceTime time,bool fitToViewer,bool singleThreaded) WARN_UNUSED_RETURN;


    /**
 *@brief Bypasses the cache so the next frame will be rendered fully
 **/
    void forceFullComputationOnNextFrame(){
        QMutexLocker forceRenderLocker(&_forceRenderMutex);
        _forceRender = true;
    }


    void connectSlotsToViewerCache();

    void disconnectSlotsToViewerCache();

    void disconnectViewer();

    void wakeUpAnySleepingThread();

    int activeInput() const WARN_UNUSED_RETURN;

    int getLutType() const WARN_UNUSED_RETURN {return _lut;}

    double getExposure() const WARN_UNUSED_RETURN {return _exposure;}

    const Natron::Color::Lut* getLut() const WARN_UNUSED_RETURN {return _colorSpace;}

    bool supportsGLSL() const WARN_UNUSED_RETURN;
/**
 *@brief Actually converting to ARGB... but it is called BGRA by
 the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
    static U32 toBGRA(U32 r,U32 g,U32 b,U32 a) WARN_UNUSED_RETURN;

    void setDisplayChannels(DisplayChannels channels) ;

    /**
     * @brief Get the color of the currently displayed image at position x,y. 
     * If forceLinear is true, then it will not use the viewer current colorspace
     * to get r,g and b values, otherwise the color returned will be in the same color-space
     * than the one chosen by the user on the gui.
     * @return true if the point is inside the image and colors were set
    **/
    bool getColorAt(int x,int y,float* r,float* g,float* b,float* a,bool forceLinear) WARN_UNUSED_RETURN;

public slots:

    void onViewerCacheFrameAdded();

    void onExposureChanged(double exp);

    void onColorSpaceChanged(const QString& colorspaceName);
    /*
 *@brief Slot called internally by the render() function when it wants to refresh the viewer if
 *the output is a viewer.
 *Do not call this yourself.
 */
    void updateViewer();

    void onNodeNameChanged(const QString&);
    
    void redrawViewer();
    
signals:

    void mustRedraw();
    
    void viewerDisconnected();

    void addedCachedFrame(SequenceTime);

    void removedLRUCachedFrame();

    void clearedViewerCache();
    /**
 *@brief Signal emitted when the engine needs to inform the main thread that it should refresh the viewer
 **/
    void doUpdateViewer();


private:
    /*******************************************
     *******OVERRIDEN FROM EFFECT INSTANCE******
     *******************************************/
    
    virtual bool isOutput() const OVERRIDE FINAL {return true;}
    
    virtual int maximumInputs() const OVERRIDE {return getNode()->maximumInputs();}

    virtual bool isInputOptional(int /*n*/) const OVERRIDE FINAL;

    virtual int majorVersion() const OVERRIDE FINAL { return 1; }

    virtual int minorVersion() const OVERRIDE FINAL { return 0; }

    virtual std::string pluginID() const OVERRIDE FINAL {return "Viewer";}

    virtual std::string pluginLabel() const OVERRIDE FINAL {return "Viewer";}
    
    virtual std::string description() const OVERRIDE FINAL {return "The Viewer node can display the output of a node graph.";}
    
    virtual Natron::Status getRegionOfDefinition(SequenceTime time,RectI* rod) OVERRIDE FINAL;
    
    virtual RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& renderWindow) OVERRIDE FINAL;
    
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE FINAL;
    
    virtual std::string inputLabel(int inputNb) const OVERRIDE FINAL {
        return QString::number(inputNb+1).toStdString();
    }
    virtual Natron::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE FINAL {return Natron::EffectInstance::FULLY_SAFE;}
    /*******************************************/

    virtual void cloneExtras() OVERRIDE FINAL;

    void renderFunctor(boost::shared_ptr<const Natron::Image> inputImage,std::pair<int,int> yRange,
                       const TextureRect& texRect,int closestPowerOf2);


    void scaleToTexture8bits(boost::shared_ptr<const Natron::Image> inputImage,std::pair<int,int> yRange,const TextureRect& texRect,
                             int closestPowerOf2,int rOffset,int gOffset,int bOffset,bool luminance);

    void scaleToTexture32bits(boost::shared_ptr<const Natron::Image> inputImage,std::pair<int,int> yRange,const TextureRect& texRect,
                              int closestPowerOf2,int rOffset,int gOffset,int bOffset,bool luminance);


};

#endif // NATRON_ENGINE_VIEWERNODE_H_
