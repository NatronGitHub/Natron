//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_ENGINE_VIEWERNODE_H_
#define POWITER_ENGINE_VIEWERNODE_H_

#include <string>
#include <QtCore/QFutureWatcher>
#include <QtCore/QMutex>

#include "Global/Macros.h"
#include "Engine/Node.h"
#include "Engine/ImageInfo.h"
#include "Engine/EffectInstance.h"

class TabWidget;
class ViewerTab;
class AppInstance;
class Timer;
namespace Powiter{
class FrameEntry;
}

class QKeyEvent;

class ViewerInstance : public QObject, public Powiter::OutputEffectInstance {
    
    Q_OBJECT
    
    
    struct InterThreadInfos{
        InterThreadInfos():
            _cachedEntry()
          , _textureRect()
          ,_pixelsCount(0){}
        
        boost::shared_ptr<const Powiter::FrameEntry> _cachedEntry;
        TextureRect _textureRect;
        size_t _pixelsCount;
    };
    
    ViewerTab* _uiContext;
    
    int _pboIndex;
    
    int _frameCount;
    
    mutable QMutex _forceRenderMutex;
    bool _forceRender;/*!< true when we want to by-pass the cache*/
    
    
    QWaitCondition _pboUnMappedCondition;
    mutable QMutex _pboUnMappedMutex; //!< protects _pboUnMappedCount
    int _pboUnMappedCount;
    
    InterThreadInfos _interThreadInfos;
    
    mutable QMutex _timerMutex;///protects timer
    boost::scoped_ptr<Timer> _timer; /*!< Timer regulating the engine execution. It is controlled by the GUI.*/
public:
    
    
    
    
    static Powiter::EffectInstance* BuildEffect(Node* n) { return new ViewerInstance(n); }
    
    ViewerInstance(Node* node);
    
    virtual ~ViewerInstance();
    
    ViewerTab* getUiContext() const {return _uiContext;}
    
    void setUiContext(ViewerTab* ptr){_uiContext = ptr;}
    
    /*Add a new viewer tab to the GUI*/
    void initializeViewerTab(TabWidget* where);
    
    /*******************************************
     *******OVERRIDEN FROM EFFECT INSTANCE******
     *******************************************/
    
    virtual bool isOutput() const OVERRIDE {return true;}
    
    virtual int maximumInputs() const OVERRIDE {return getNode()->maximumInputs();}

    virtual bool isInputOptional(int /*n*/) const OVERRIDE;

    virtual std::string className() const OVERRIDE {return "Viewer";}
    
    virtual std::string description() const OVERRIDE {return "The Viewer node can display the output of a node graph.";}
    
    virtual Powiter::Status getRegionOfDefinition(SequenceTime time,RectI* rod) OVERRIDE;
    
    virtual RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& renderWindow) OVERRIDE;
    
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;
    
    virtual std::string setInputLabel(int inputNb) const OVERRIDE {
        return QString::number(inputNb+1).toStdString();
    }
    virtual Powiter::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE {return Powiter::EffectInstance::FULLY_SAFE;}

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
    Powiter::Status renderViewer(SequenceTime time,bool fitToViewer);


    /**
 *@brief Bypasses the cache so the next frame will be rendered fully
 **/
    void forceFullComputationOnNextFrame(){
        QMutexLocker forceRenderLocker(&_forceRenderMutex);
        _forceRender = true;
    }

    /**
 *@brief Tells all the nodes in the grpah to draw their overlays
 **/
    /*All the overlay methods are forwarding calls to the default node instance*/
    void drawOverlays() const;

    void notifyOverlaysPenDown(const QPointF& viewportPos,const QPointF& pos);

    void notifyOverlaysPenMotion(const QPointF& viewportPos,const QPointF& pos);

    void notifyOverlaysPenUp(const QPointF& viewportPos,const QPointF& pos);

    void notifyOverlaysKeyDown(QKeyEvent* e);

    void notifyOverlaysKeyUp(QKeyEvent* e);

    void notifyOverlaysKeyRepeat(QKeyEvent* e);

    void notifyOverlaysFocusGained();

    void notifyOverlaysFocusLost();


    void connectSlotsToViewerCache();

    void disconnectSlotsToViewerCache();

    void disconnectViewer(){
        emit viewerDisconnected();
    }


    void redrawViewer();

    void swapBuffers();

    void pixelScale(double &x,double &y);

    void backgroundColor(double &r,double &g,double &b);

    void viewportSize(double &w,double &h);

    int activeInput() const;


public slots:

    void onCachedFrameAdded();

    void onCachedFrameRemoved();

    void onViewerCacheCleared();


    /**
 *@brief The slot called by the GUI to set the requested fps.
 **/
    void setDesiredFPS(double d);

    /*
 *@brief Slot called internally by the render() function when it wants to refresh the viewer if
 *the output is a viewer.
 *Do not call this yourself.
 */
    void updateViewer();

    /*
 *@brief Slot called internally by the render() function when it found a frame in cache.
 *Do not call this yourself
 **/
    void cachedEngine();

    /*
 *@brief Slot called internally by the render() function when the output is a viewer and it
 *needs to allocate the output buffer for the current frame.
 *Do not call this yourself.
 */
    void allocateFrameStorage();

signals:

    void viewerDisconnected();

    void addedCachedFrame(int);

    void removedCachedFrame();

    void clearedViewerCache();

    void mustSwapBuffers();

    void mustRedraw();

    /**
 *@brief Signal emitted when the function waits the time due to display the frame.
 **/
    void fpsChanged(double d);

    /**
 *@brief Signal emitted when the engine needs to inform the main thread that it should refresh the viewer
 **/
    void doUpdateViewer();

    /**
 *@brief Signal emitted when the engine needs to pass-on to the main thread the rendering of a cached frame.
 **/
    void doCachedEngine();

    /**
 *@brief Signal emitted when the engine needs to warn the main thread that the storage for the current frame
 *should be allocated .
 **/
    void doFrameStorageAllocation();

private:

    void renderFunctor(boost::shared_ptr<const Powiter::Image> inputImage,
                       const std::vector<std::pair<int,int> >& rows,
                       const std::vector<int>& columns);




};

#endif // POWITER_ENGINE_VIEWERNODE_H_
