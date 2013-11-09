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
#include "Engine/EffectInstance.h"

class Tab_Knob;
class QHBoxLayout;
class QImage;
class OfxClipInstance;
class QKeyEvent;
namespace Powiter {
class Node;
class OfxImageEffectInstance;
class OfxOverlayInteract;
}


class OfxEffectInstance : public Powiter::OutputEffectInstance {
    
    Powiter::OfxImageEffectInstance* effect_;
    bool _isOutput;//if the OfxNode can output a file somehow

    bool _penDown; // true when the overlay trapped a penDow action
    boost::scoped_ptr<Powiter::OfxOverlayInteract> _overlayInteract; // ptr to the overlay interact if any

    Tab_Knob* _tabKnob; // for nuke tab extension: it creates all Group param as a tab and put it into this knob.
    QHBoxLayout* _lastKnobLayoutWithNoNewLine; // for nuke layout hint extension

public:
    
    
    OfxEffectInstance(Powiter::Node* node);
    
    virtual ~OfxEffectInstance();
    
    void createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                      const std::string& context);
    
    Powiter::OfxImageEffectInstance* effectInstance(){ return effect_; }
    
    const Powiter::OfxImageEffectInstance* effectInstance() const{ return effect_; }

    void setAsOutputNode() {_isOutput = true;}

    void setTabKnob(Tab_Knob* k){_tabKnob = k;}

    Tab_Knob* getTabKnob() const {return _tabKnob;}

    void setLastKnobLayoutWithNoNewLine(QHBoxLayout* layout){_lastKnobLayoutWithNoNewLine = layout;}

    QHBoxLayout* getLastKnobLayoutWithNoNewLine() const {return _lastKnobLayoutWithNoNewLine;}
    
    
    const std::string& getShortLabel() const;
    
    typedef std::vector<OFX::Host::ImageEffect::ClipDescriptor*> MappedInputV;
    MappedInputV inputClipsCopyWithoutOutput() const;
    
    void ifInfiniteclipRectToProjectDefault(OfxRectD* rod) const;
    
    /********OVERRIDEN FROM EFFECT INSTANCE*************/
    virtual bool isGenerator() const OVERRIDE;
    
    virtual bool isOutput() const OVERRIDE;
    
    virtual bool isGeneratorAndFilter() const OVERRIDE;
    
    virtual bool isOpenFX() const OVERRIDE {return true;}

    virtual bool makePreviewByDefault() const OVERRIDE ;

    virtual int maximumInputs() const OVERRIDE;

    virtual std::string className() const OVERRIDE;

    virtual std::string description() const OVERRIDE;

    virtual std::string setInputLabel (int inputNb) const OVERRIDE;

    virtual bool isInputOptional(int inputNb) const OVERRIDE;

    virtual Powiter::Status getRegionOfDefinition(SequenceTime time,RectI* rod) OVERRIDE;

    virtual Powiter::EffectInstance::RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& renderWindow) OVERRIDE;

    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;

    virtual Powiter::Status preProcessFrame(SequenceTime /*time*/) OVERRIDE;

    virtual void drawOverlay() OVERRIDE;

    virtual bool onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos) OVERRIDE;

    virtual bool onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos) OVERRIDE;

    virtual bool onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos) OVERRIDE;

    virtual void onOverlayKeyDown(QKeyEvent* e) OVERRIDE;

    virtual void onOverlayKeyUp(QKeyEvent* e) OVERRIDE;

    virtual void onOverlayKeyRepeat(QKeyEvent* e) OVERRIDE;

    virtual void onOverlayFocusGained() OVERRIDE;

    virtual void onOverlayFocusLost() OVERRIDE;

    virtual void beginKnobsValuesChanged(Knob::ValueChangedReason reason) OVERRIDE ;

    virtual void endKnobsValuesChanged(Knob::ValueChangedReason reason) OVERRIDE ;

    virtual void onKnobValueChanged(Knob* k,Knob::ValueChangedReason reason) OVERRIDE;

    virtual Powiter::Status render(SequenceTime time,RenderScale scale,
                                   const RectI& roi,int view,boost::shared_ptr<Powiter::Image> output) OVERRIDE;

    virtual Powiter::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE;


    /*********OVERLAY INTERACT FUNCTIONS********/
    void tryInitializeOverlayInteracts();

    void swapBuffersOfAttachedViewer();

    void redrawInteractOnAttachedViewer();

    void pixelScaleOfAttachedViewer(double &x,double &y);

    void viewportSizeOfAttachedViewer(double &w,double &h);

    void backgroundColorOfAttachedViewer(double &r,double &g,double &b);

    static QStringList getPluginGrouping(const std::string& bundlePath,int pluginsCount,const std::string& grouping);

    static std::string generateImageEffectClassName(const std::string& shortLabel,
                                                const std::string& label,
                                                const std::string& longLabel,
                                                int pluginsCount,
                                                const std::string& bundlePath,
                                                const std::string& grouping);

};


/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
QStringList ofxExtractAllPartsOfGrouping(const QString& group,const QString& bundlePath);

#endif // POWITER_ENGINE_OFXNODE_H_
