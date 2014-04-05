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

#ifndef NATRON_ENGINE_OFXNODE_H_
#define NATRON_ENGINE_OFXNODE_H_

#include "Global/Macros.h"
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QStringList>
//ofx
#include "ofxhImageEffect.h"

#include "Engine/EffectInstance.h"

class QReadWriteLock;
class OfxClipInstance;
class Button_Knob;
class OverlaySupport;
class NodeSerialization;
namespace Natron {
class Node;
class OfxImageEffectInstance;
class OfxOverlayInteract;
}

class AbstractOfxEffectInstance : public Natron::OutputEffectInstance {
public:
    
    AbstractOfxEffectInstance(boost::shared_ptr<Natron::Node> node)
    : Natron::OutputEffectInstance(node)
    {
    }
    
    virtual ~AbstractOfxEffectInstance(){}
    
    virtual void createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                              const boost::shared_ptr<AbstractOfxEffectInstance>& thisAsShared,
                                              const std::string& context,const NodeSerialization* serialization) = 0;
    
    static QStringList getPluginGrouping(const std::string& pluginLabel,const std::string& grouping) WARN_UNUSED_RETURN;
    
    static std::string getPluginLabel(const std::string& shortLabel,
                                      const std::string& label,
                                      const std::string& longLabel) WARN_UNUSED_RETURN;
    
    static std::string generateImageEffectClassName(const std::string& shortLabel,
                                                    const std::string& label,
                                                    const std::string& longLabel,
                                                    const std::string& grouping) WARN_UNUSED_RETURN;
};

class OfxEffectInstance : public QObject, public AbstractOfxEffectInstance {
    
    
    Q_OBJECT
    
    Natron::OfxImageEffectInstance* effect_;
    bool _isOutput;//if the OfxNode can output a file somehow

    bool _penDown; // true when the overlay trapped a penDow action
    Natron::OfxOverlayInteract* _overlayInteract; // ptr to the overlay interact if any

    bool _initialized; //true when the image effect instance has been created and populated
    boost::shared_ptr<Button_Knob> _renderButton; //< render button for writers
    mutable EffectInstance::RenderSafety _renderSafety;
    mutable bool _wasRenderSafetySet;
    mutable QReadWriteLock* _renderSafetyLock;
public:
    
    
    OfxEffectInstance(boost::shared_ptr<Natron::Node> node);
    
    virtual ~OfxEffectInstance();
    
    void createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                      const boost::shared_ptr<AbstractOfxEffectInstance>& thisAsShared,
                                      const std::string& context,const NodeSerialization* serialization) OVERRIDE FINAL;
    
    Natron::OfxImageEffectInstance* effectInstance() WARN_UNUSED_RETURN { return effect_; }
    
    const Natron::OfxImageEffectInstance* effectInstance() const WARN_UNUSED_RETURN { return effect_; }

    void setAsOutputNode() {_isOutput = true;}
    
    const std::string& getShortLabel() const WARN_UNUSED_RETURN;
    
    typedef std::vector<OFX::Host::ImageEffect::ClipDescriptor*> MappedInputV;
    MappedInputV inputClipsCopyWithoutOutput() const WARN_UNUSED_RETURN;
    
    ///Returns true if the rod will be set to the project format
    bool ifInfiniteclipRectToProjectDefault(OfxRectD* rod) const;

    /********OVERRIDEN FROM EFFECT INSTANCE*************/
    virtual int majorVersion() const OVERRIDE  FINAL WARN_UNUSED_RETURN;

    virtual int minorVersion() const OVERRIDE  FINAL WARN_UNUSED_RETURN;

    virtual bool isGenerator() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isReader() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isWriter() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool isGeneratorAndFilter() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool isOpenFX() const OVERRIDE FINAL WARN_UNUSED_RETURN {return true;}

    virtual bool makePreviewByDefault() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual int maximumInputs() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual std::string pluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual std::string pluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual std::string description() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual std::string inputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isInputOptional(int inputNb) const OVERRIDE WARN_UNUSED_RETURN;

    virtual Natron::Status getRegionOfDefinition(SequenceTime time,RectI* rod,bool* isProjectFormat) OVERRIDE WARN_UNUSED_RETURN;

    virtual Natron::EffectInstance::RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& renderWindow) OVERRIDE WARN_UNUSED_RETURN;
    
    virtual Natron::EffectInstance::FramesNeededMap getFramesNeeded(SequenceTime time) WARN_UNUSED_RETURN;

    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;

    virtual void initializeOverlayInteract() OVERRIDE FINAL;

    virtual void drawOverlay() OVERRIDE FINAL;

    virtual bool onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyDown(Natron::Key key,Natron::KeyboardModifiers modifiers) OVERRIDE FINAL;

    virtual bool onOverlayKeyUp(Natron::Key key,Natron::KeyboardModifiers modifiers) OVERRIDE FINAL;

    virtual bool onOverlayKeyRepeat(Natron::Key key,Natron::KeyboardModifiers modifiers) OVERRIDE FINAL;

    virtual bool onOverlayFocusGained() OVERRIDE FINAL;

    virtual bool onOverlayFocusLost() OVERRIDE FINAL;

    virtual void setCurrentViewportForOverlays(OverlaySupport* viewport) OVERRIDE FINAL;

    virtual void beginKnobsValuesChanged(Natron::ValueChangedReason reason) OVERRIDE;

    virtual void endKnobsValuesChanged(Natron::ValueChangedReason reason) OVERRIDE;

    virtual void onKnobValueChanged(KnobI* k,Natron::ValueChangedReason reason) OVERRIDE;
    
    virtual void beginEditKnobs() OVERRIDE;

    virtual Natron::Status render(SequenceTime time,RenderScale scale,
                                   const RectI& roi,int view,
                                  bool isSequentialRender,bool isRenderResponseToUserInteraction,
                                  boost::shared_ptr<Natron::Image> output) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool isIdentity(SequenceTime time,RenderScale scale,const RectI& roi,
                                int view,SequenceTime* inputTime,int* inputNb) OVERRIDE;

    virtual Natron::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void purgeCaches() OVERRIDE;

    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void onInputChanged(int inputNo) OVERRIDE FINAL;
    
    virtual void onMultipleInputsChanged() OVERRIDE FINAL;
    
    virtual std::vector<std::string> supportedFileFormats() const OVERRIDE FINAL;
    
    virtual void beginSequenceRender(SequenceTime first,SequenceTime last,
                                     SequenceTime step,bool interactive,RenderScale scale,
                                     bool isSequentialRender,bool isRenderResponseToUserInteraction,int view) OVERRIDE FINAL;
    
    virtual void endSequenceRender(SequenceTime first,SequenceTime last,
                                     SequenceTime step,bool interactive,RenderScale scale,
                                   bool isSequentialRender,bool isRenderResponseToUserInteraction,int view) OVERRIDE FINAL;
    /********OVERRIDEN FROM EFFECT INSTANCE: END*************/

    /**
     * @brief Calls syncPrivateDataAction from another thread than the main thread. The actual
     * call of the action will take place in the main-thread.
     **/
    void syncPrivateData_other_thread() { emit syncPrivateDataRequested(); }

public slots:

    void onSyncPrivateDataRequested();

signals:

    void syncPrivateDataRequested();

private:
    
    OfxClipInstance* getClipCorrespondingToInput(int inputNo) const;

    void tryInitializeOverlayInteracts();

    void initializeContextDependentParams();

};


/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
QStringList ofxExtractAllPartsOfGrouping(const QString& group,const QString& bundlePath);

#endif // NATRON_ENGINE_OFXNODE_H_
