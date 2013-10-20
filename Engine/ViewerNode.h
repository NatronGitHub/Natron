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

#include "Global/Macros.h"
#include "Engine/Node.h"
#include "Engine/ImageInfo.h"

class TabWidget;
class ViewerTab;
namespace Powiter{
    class FrameEntry;
}

class QKeyEvent;

class ViewerNode: public OutputNode
{
    Q_OBJECT
    
	ViewerTab* _uiContext;
    int _inputsCount;
    int _activeInput;
    int _pboIndex;

public:
    
        
    ViewerNode(Model* model);
    
    virtual ~ViewerNode();
    
    int maximumInputs() const { return _inputsCount; }
    
    int minimumInputs() const { return 1; }
    
    virtual std::string className() const {return "Viewer";}
    
    virtual std::string description() const {return "The Viewer node can display the output of a node graph.";}
    
    virtual bool cacheData() const {return false;}
    
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
    
    void renderRow(SequenceTime time,int left,int right,int y,int textureY);

    /*This function MUST be called in the main thread.*/
    void cachedFrameEngine(boost::shared_ptr<const Powiter::FrameEntry> frame);

    virtual Powiter::Status getRegionOfDefinition(SequenceTime time,Box2D* rod,Format* displayWindow = NULL) OVERRIDE;
    
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;
public slots:
    
    void onCachedFrameAdded();
    
    void onCachedFrameRemoved();
    
    void onViewerCacheCleared();
    
protected:
    
    virtual ChannelSet supportedComponents(){return Powiter::Mask_All;}
    
    virtual std::string setInputLabel(int inputNb) const {
        return QString::number(inputNb+1).toStdString();
    }

    
signals:
    void viewerDisconnected();
    
    void addedCachedFrame(int);
    
    void removedCachedFrame();
    
    void clearedViewerCache();
    
    void mustSwapBuffers();
    
    void mustRedraw();
    
    
};



#endif // POWITER_ENGINE_VIEWERNODE_H_
