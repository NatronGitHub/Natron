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
#include <QtGui/QImage>
//ofx
#include "ofxhImageEffect.h"

#include "Engine/Node.h"

//ours
class Tab_Knob;
class QHBoxLayout;
class QImage;
class QKeyEvent;
namespace Powiter {
    class OfxImageEffectInstance;
    class OfxOverlayInteract;
}

class OfxNode : public OutputNode
{    
public:
    OfxNode(Model* model,
             OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
             const std::string& context);
    
    virtual ~OfxNode();

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for Powiter::Node

    virtual bool isInputNode() const OVERRIDE;
    
    virtual bool isOutputNode() const OVERRIDE;
    
    /*Returns the clips count minus the output clip*/
    virtual int maximumInputs() const OVERRIDE;
    
    virtual int minimumInputs() const OVERRIDE;
    
    virtual bool cacheData() const OVERRIDE {return false;}
    
    virtual std::string className() const OVERRIDE;
    
    virtual std::string description() const OVERRIDE;
    
    virtual std::string setInputLabel (int inputNb) OVERRIDE;
    
    virtual bool isOpenFXNode() const OVERRIDE {return true;}
        
    virtual ChannelSet supportedComponents() OVERRIDE;
        
    virtual bool _validate(bool) OVERRIDE;
    
    virtual void engine(Powiter::Row* out) OVERRIDE;

    virtual void drawOverlay();
    
    virtual bool onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos);
    
    virtual bool onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos);
    
    virtual bool onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos);
    
    virtual void onOverlayKeyDown(QKeyEvent* e);
    
    virtual void onOverlayKeyUp(QKeyEvent* e);
    
    virtual void onOverlayKeyRepeat(QKeyEvent* e);
    
    virtual void onOverlayFocusGained();
    
    virtual void onOverlayFocusLost();
    

    bool isInputOptional(int inpubNb) const;

    void setAsOutputNode() {_isOutput = true;}

    bool hasPreviewImage() const {return !_preview.isNull();}

    bool canHavePreviewImage() const {return _canHavePreview;}

    void setCanHavePreviewImage() {_canHavePreview = true;}

    const QImage& getPreview() const { return _preview; }

    void setTabKnob(Tab_Knob* k){_tabKnob = k;}
    
    Tab_Knob* getTabKnob() const {return _tabKnob;}
    
    void setLastKnobLayoutWithNoNewLine(QHBoxLayout* layout){_lastKnobLayoutWithNoNewLine = layout;}
    
    QHBoxLayout* getLastKnobLayoutWithNoNewLine() const {return _lastKnobLayoutWithNoNewLine;}

    typedef std::vector<OFX::Host::ImageEffect::ClipDescriptor*> MappedInputV;

    
    MappedInputV inputClipsCopyWithoutOutput() const;

    void computePreviewImage();

    Powiter::OfxImageEffectInstance* effectInstance() { return effect_; }

    const Powiter::OfxImageEffectInstance* effectInstance() const { return effect_; }
    
    const std::string& getShortLabel() const; // forwarded to OfxImageEffectInstance
    
    /*group is a string as such:
     Toto/Superplugins/blabla
     This functions extracts the all parts of such a grouping, e.g in this case
     it would return [Toto,Superplugins,blabla].*/
    const QStringList getPluginGrouping() const; 

    void openFilesForAllFileParams();
    
    void swapBuffersOfAttachedViewer();
    
    void redrawInteractOnAttachedViewer();
    
    void pixelScaleOfAttachedViewer(double &x,double &y);
    
    void viewportSizeOfAttachedViewer(double &w,double &h);
    
    void backgroundColorOfAttachedViewer(double &r,double &g,double &b);

    void tryInitializeOverlayInteracts();
    
    /**
     * @brief getRandomFrameName Valid only for nodes having a  OfxStringInstance's of type kOfxParamStringIsFilePath with
     * an OfxImageEffectInstance of type kOfxImageEffectContextGenerator
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    virtual const QString getRandomFrameName(int f) const;
    
    void onInstanceChanged(const std::string& paramName);
    
private:

    Tab_Knob* _tabKnob; // for nuke tab extension: it creates all Group param as a tab and put it into this knob.
    QHBoxLayout* _lastKnobLayoutWithNoNewLine; // for nuke layout hint extension
    QMutex _firstTimeMutex; // lock used in engine(...) function, protects _firstTime
    bool _firstTime; //used in engine(...) to operate once per frame
    bool _isOutput;//if the OfxNode can output a file somehow
    QImage _preview; // the preview if _canHavePreview is true
    bool _canHavePreview; // does it have a preview? 
    Powiter::OfxImageEffectInstance* effect_; // FIXME: use boost::shared_ptr (cannot be a scope_ptr, because Powiter::OfxHost::newInstance() must return it)
    Powiter::OfxOverlayInteract* _overlayInteract; // ptr to the overlay interact if any
    bool _penDown; // true when the overlay trapped a penDow action
};


/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
QStringList ofxExtractAllPartsOfGrouping(const QString& group,const QString& bundlePath);

#endif // POWITER_ENGINE_OFXNODE_H_
