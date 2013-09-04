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
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QObject>
//ofx
#include "ofxhImageEffect.h"

#include "Engine/Node.h"

//ours
class Tab_Knob;
class QHBoxLayout;
class QImage;
namespace Powiter {
    class OfxImageEffectInstance;
}

// FIXME: OFX::Host::ImageEffect::Instance should be a member
class OfxNode : public QObject, public OutputNode
{
    Q_OBJECT

public:
    OfxNode(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
            OFX::Host::ImageEffect::Descriptor         &other,
            const std::string  &context,
            Model* model);
    
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
    
    virtual std::string className() OVERRIDE;
    
    virtual std::string description() OVERRIDE {return "";}
    
    virtual std::string setInputLabel (int inputNb) OVERRIDE;
    
    virtual bool isOpenFXNode() const OVERRIDE {return true;}
        
    virtual ChannelSet supportedComponents() OVERRIDE;
    
    virtual bool _validate(bool) OVERRIDE;
    
    virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out) OVERRIDE;




    

    bool isInputOptional(int inpubNb) const;

    void setAsOutputNode() {_isOutput = true;}

    bool hasPreviewImage() const {return _preview!=NULL;}

    bool canHavePreviewImage() const {return _canHavePreview;}

    void setCanHavePreviewImage() {_canHavePreview = true;}

    QImage* getPreview() const { return _preview; }

    int firstFrame() const { return _frameRange.first; }

    int lastFrame() const { return _frameRange.second; }

    void incrementCurrentFrame() {++_currentFrame;}

    void setCurrentFrameToStart(){_currentFrame = _frameRange.first;}

    void setCurrentFrame(int f){
        if(f >= _frameRange.first && f <= _frameRange.second)
            _currentFrame = f;
    }

    /*Useful only when _isOutput is true*/
    int currentFrame() const {return _currentFrame;}

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
    const std::string& getPluginGrouping() const; // forwarded to OfxImageEffectInstance

public slots:
    void onInstanceChangedAction(const QString&);
    
private:

    QMutex _lock; // lock used in engine(...) function
    std::map<std::string,OFX::Host::Param::Instance*> _params; // used to re-create parenting between params
    Tab_Knob* _tabKnob; // for nuke tab extension: it creates all Group param as a tab
    QHBoxLayout* _lastKnobLayoutWithNoNewLine; // for nuke layout hint extension
    bool _firstTime; //used in engine(...) to operate once per frame
    bool _isOutput;
    int _currentFrame; // valid only when _isOutput is true
    std::pair<int,int> _frameRange; // valid only when _isOutput is true
    QImage* _preview;
    bool _canHavePreview;
    Powiter::OfxImageEffectInstance* effect_; // FIXME: use boost::shared_ptr (cannot be a scope_ptr, because Powiter::OfxHost::newInstance() must return it)
};


/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
std::vector<std::string> ofxExtractAllPartsOfGrouping(const std::string& group);

#endif // POWITER_ENGINE_OFXNODE_H_
