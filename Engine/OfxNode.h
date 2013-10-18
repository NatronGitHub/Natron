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

#ifndef POWITER_ENGINE_OFXNODE_H_
#define POWITER_ENGINE_OFXNODE_H_

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QStringList>
//ofx
#include "ofxhImageEffect.h"

#include "Engine/Node.h"

//ours
class Tab_Knob;
class QHBoxLayout;
class QImage;
class OfxClipInstance;
class QKeyEvent;
namespace Powiter {
    class OfxImageEffectInstance;
    class OfxOverlayInteract;
}

class OfxNode : public OutputNode{
    
    Q_OBJECT
        
    Tab_Knob* _tabKnob; // for nuke tab extension: it creates all Group param as a tab and put it into this knob.
    QHBoxLayout* _lastKnobLayoutWithNoNewLine; // for nuke layout hint extension
    boost::scoped_ptr<Powiter::OfxOverlayInteract> _overlayInteract; // ptr to the overlay interact if any
    bool _penDown; // true when the overlay trapped a penDow action
    Powiter::OfxImageEffectInstance* effect_;
    bool _firstTime; //used in engine(...) to operate once per frame
    QMutex _firstTimeMutex;
    bool _isOutput;//if the OfxNode can output a file somehow
    
    
    std::pair<int,int> _frameRange;
public:
    
    
    OfxNode(Model* model,
            OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
            const std::string& context);
    
    
    virtual ~OfxNode();
    
    Powiter::OfxImageEffectInstance* effectInstance(){ return effect_; }
    
    const Powiter::OfxImageEffectInstance* effectInstance() const{ return effect_; }

    void setTabKnob(Tab_Knob* k){_tabKnob = k;}
    
    Tab_Knob* getTabKnob() const {return _tabKnob;}
    
    void setLastKnobLayoutWithNoNewLine(QHBoxLayout* layout){_lastKnobLayoutWithNoNewLine = layout;}
    
    QHBoxLayout* getLastKnobLayoutWithNoNewLine() const {return _lastKnobLayoutWithNoNewLine;}
    
    void tryInitializeOverlayInteracts();
    
    typedef std::vector<OFX::Host::ImageEffect::ClipDescriptor*> MappedInputV;
    
    MappedInputV inputClipsCopyWithoutOutput() const;
    
    
    
    
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for Powiter::Node
    
    virtual bool isInputNode() const OVERRIDE;
    
    virtual bool isOutputNode() const OVERRIDE;
    
    virtual bool isInputAndProcessingNode() const OVERRIDE;
    
    virtual bool isOpenFXNode() const OVERRIDE {return true;}
    
    void setAsOutputNode() {_isOutput = true;}
    
    virtual bool canMakePreviewImage() const OVERRIDE ;
            
    /*Returns the clips count minus the output clip*/
    virtual int maximumInputs() const OVERRIDE;
    
    virtual int minimumInputs() const OVERRIDE;
    
    virtual bool cacheData() const OVERRIDE {return false;}
    
    virtual std::string className() const OVERRIDE;
    
    virtual std::string description() const OVERRIDE;
    
    virtual std::string setInputLabel (int inputNb) const OVERRIDE;
    
    ChannelSet supportedComponents() const;
    
    virtual Powiter::Status getRegionOfDefinition(SequenceTime time,Box2D* rod) OVERRIDE;
    
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;
    
    virtual Powiter::Status preProcessFrame(SequenceTime /*time*/) OVERRIDE;
    
    virtual void render(SequenceTime time,Powiter::Row* out) OVERRIDE;
    
    virtual void drawOverlay() OVERRIDE;
    
    virtual bool onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos) OVERRIDE;
    
    virtual bool onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos) OVERRIDE;
    
    virtual bool onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos) OVERRIDE;
    
    virtual void onOverlayKeyDown(QKeyEvent* e) OVERRIDE;
    
    virtual void onOverlayKeyUp(QKeyEvent* e) OVERRIDE;
    
    virtual void onOverlayKeyRepeat(QKeyEvent* e) OVERRIDE;
    
    virtual void onOverlayFocusGained() OVERRIDE;
    
    virtual void onOverlayFocusLost() OVERRIDE;

    
    void swapBuffersOfAttachedViewer();
    
    void redrawInteractOnAttachedViewer();
    
    void pixelScaleOfAttachedViewer(double &x,double &y);
    
    void viewportSizeOfAttachedViewer(double &w,double &h);
    
    void backgroundColorOfAttachedViewer(double &r,double &g,double &b);

    bool isInputOptional(int inpubNb) const;
    
    void openFilesForAllFileParams();

    /*group is a string as such:
     Toto/Superplugins/blabla
     This functions extracts the all parts of such a grouping, e.g in this case
     it would return [Toto,Superplugins,blabla].*/
    const QStringList getPluginGrouping() const;
    
    void onInstanceChanged(const std::string& paramName);
    
    const std::string& getShortLabel() const;

    int firstFrame() const {return _frameRange.first;}
    
    int lastFrame() const {return _frameRange.second;}
    
public slots:
    
    void onFrameRangeChanged(int,int);
};



/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
QStringList ofxExtractAllPartsOfGrouping(const QString& group,const QString& bundlePath);

#endif // POWITER_ENGINE_OFXNODE_H_
