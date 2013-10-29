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

class TabWidget;
class ViewerTab;
class Timer;
namespace Powiter{
    class FrameEntry;
}

class QKeyEvent;

class ViewerNode: public OutputNode
{
    
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
    int _inputsCount;
    int _activeInput;
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
    
    
        
    ViewerNode(AppInstance* app);
    
    virtual ~ViewerNode();
    
    int maximumInputs() const { return _inputsCount; }
    
    int minimumInputs() const { return 1; }
    
    virtual std::string className() const {return "Viewer";}
    
    virtual std::string description() const {return "The Viewer node can display the output of a node graph.";}
        
    bool connectInput(Node* input,int inputNumber,bool autoConnection = false);
    
    virtual int disconnectInput(int inputNumber);
    
    virtual int disconnectInput(Node* input);
    
    bool tryAddEmptyInput();
    
    void addEmptyInput();

    void removeEmptyInputs();
    
    void setActiveInputAndRefresh(int inputNb);
    
    int activeInput() const {return _activeInput;}
    
    /*Add a new viewer tab to the GUI*/
    void initializeViewerTab(TabWidget* where);
        
    ViewerTab* getUiContext() const {return _uiContext;}
    
    void setUiContext(ViewerTab* ptr){_uiContext = ptr;}
    
    void disconnectViewer(){
        emit viewerDisconnected();
    }
    
    
    void connectSlotsToViewerCache();
    
    void disconnectSlotsToViewerCache();

    void redrawViewer();
    
    void swapBuffers();
    
    void pixelScale(double &x,double &y);
    
    void backgroundColor(double &r,double &g,double &b);
    
    void viewportSize(double &w,double &h);
    
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
    
    virtual Powiter::Status getRegionOfDefinition(SequenceTime time,Box2D* rod) OVERRIDE;
    
    virtual RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,const Box2D& renderWindow) OVERRIDE;
    
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;
    
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
    
protected:
    
    virtual Powiter::ChannelSet supportedComponents(){return Powiter::Mask_All;}
    
    virtual std::string setInputLabel(int inputNb) const {
        return QString::number(inputNb+1).toStdString();
    }
    virtual Node::RenderSafety renderThreadSafety() const OVERRIDE {return Node::FULLY_SAFE;}
    
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
