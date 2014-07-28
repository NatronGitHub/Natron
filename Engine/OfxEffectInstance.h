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
                                      const std::string& context,const NodeSerialization* serialization) OVERRIDE FINAL;
    
    Natron::OfxImageEffectInstance* effectInstance() WARN_UNUSED_RETURN { return effect_; }
    
    const Natron::OfxImageEffectInstance* effectInstance() const WARN_UNUSED_RETURN { return effect_; }

    void setAsOutputNode() {_isOutput = true;}
    
    const std::string& getShortLabel() const WARN_UNUSED_RETURN;
    
    typedef std::vector<OFX::Host::ImageEffect::ClipDescriptor*> MappedInputV;
    MappedInputV inputClipsCopyWithoutOutput() const WARN_UNUSED_RETURN;
    
    bool ifInfiniteApplyHeuristic(OfxTime time,const RenderScale& scale, int view,OfxRectD* rod) const;

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

    virtual void pluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL;

    virtual std::string description() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual std::string inputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isInputOptional(int inputNb) const OVERRIDE WARN_UNUSED_RETURN;

    virtual bool isInputMask(int inputNb) const WARN_UNUSED_RETURN;

    virtual bool isInputRotoBrush(int inputNb) const WARN_UNUSED_RETURN;

    virtual Natron::Status getRegionOfDefinition(SequenceTime time,const RenderScale& scale,int view,RectI* rod) OVERRIDE WARN_UNUSED_RETURN;

    virtual Natron::EffectInstance::RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,
                                                               const RectI& outputRoD,
                                                               const RectI& renderWindow,int view) OVERRIDE WARN_UNUSED_RETURN;
    
    virtual Natron::EffectInstance::FramesNeededMap getFramesNeeded(SequenceTime time) WARN_UNUSED_RETURN;

    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;

    virtual void initializeOverlayInteract() OVERRIDE FINAL;

    virtual bool hasOverlay() const OVERRIDE FINAL; 

    virtual void drawOverlay(double scaleX,double scaleY,const RectI& rod) OVERRIDE FINAL;

    virtual bool onOverlayPenDown(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos
                                  ,const RectI& rod) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool onOverlayPenMotion(double scaleX,double scaleY,
                                    const QPointF& viewportPos,const QPointF& pos
                                    ,const RectI& rod) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool onOverlayPenUp(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos
                                ,const RectI& rod) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyDown(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers
                                  ,const RectI& rod) OVERRIDE FINAL;

    virtual bool onOverlayKeyUp(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers
                                ,const RectI& rod) OVERRIDE FINAL;

    virtual bool onOverlayKeyRepeat(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers
                                    ,const RectI& rod) OVERRIDE FINAL;

    virtual bool onOverlayFocusGained(double scaleX,double scaleY
                                      ,const RectI& rod) OVERRIDE FINAL;

    virtual bool onOverlayFocusLost(double scaleX,double scaleY
                                    ,const RectI& rod) OVERRIDE FINAL;

    virtual void setCurrentViewportForOverlays(OverlaySupport* viewport) OVERRIDE FINAL;

    virtual void beginKnobsValuesChanged(Natron::ValueChangedReason reason) OVERRIDE;

    virtual void endKnobsValuesChanged(Natron::ValueChangedReason reason) OVERRIDE;

    virtual void knobChanged(KnobI* k,Natron::ValueChangedReason reason,const RectI& rod,int view,SequenceTime time) OVERRIDE;
    
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

    virtual bool supportsRenderScale() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void onInputChanged(int inputNo) OVERRIDE FINAL;
    
    virtual void onMultipleInputsChanged() OVERRIDE FINAL;
    
    virtual std::vector<std::string> supportedFileFormats() const OVERRIDE FINAL;
    
    virtual Natron::Status beginSequenceRender(SequenceTime first,SequenceTime last,
                                     SequenceTime step,bool interactive,RenderScale scale,
                                     bool isSequentialRender,bool isRenderResponseToUserInteraction,int view) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual Natron::Status endSequenceRender(SequenceTime first,SequenceTime last,
                                     SequenceTime step,bool interactive,RenderScale scale,
                                   bool isSequentialRender,bool isRenderResponseToUserInteraction,int view) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;

    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const OVERRIDE FINAL;

    virtual void getPreferredDepthAndComponents(int inputNb,Natron::ImageComponents* comp,Natron::ImageBitDepth* depth) const OVERRIDE FINAL;

    virtual Natron::SequentialPreference getSequentialPreference() const OVERRIDE FINAL;

    virtual Natron::ImagePremultiplication getOutputPremultiplication() const OVERRIDE FINAL;
    /********OVERRIDEN FROM EFFECT INSTANCE: END*************/

    const std::string& ofxGetOutputPremultiplication() const;

    static Natron::ImagePremultiplication ofxPremultToNatronPremult(const std::string& str) ;

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

    void checkClipPrefs(double time,const RenderScale& scale,const std::string&  reason);

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
