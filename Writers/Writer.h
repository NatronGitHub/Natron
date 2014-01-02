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

#ifndef NATRON_WRITERS_WRITER_H_
#define NATRON_WRITERS_WRITER_H_

#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "Global/Macros.h"
#include "Engine/EffectInstance.h"
#include "Engine/ChannelSet.h"
#if 0 // deprecated

class Encoder;
namespace Natron{
    class Image;
}
class OutputFile_Knob;
class Choice_Knob;
class EncoderKnobs;
class Bool_Knob;
class Button_Knob;
class Int_Knob;
class Writer: public QObject,public Natron::OutputEffectInstance {
    
    Q_OBJECT
    
public:
  
    static Natron::EffectInstance* BuildEffect(Natron::Node* n){
        return new Writer(n);
    }
    
    Writer(Natron::Node* node);
        
    virtual ~Writer();
    
    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE {return false;}

    virtual int majorVersion() const OVERRIDE { return 1; }

    virtual int minorVersion() const OVERRIDE { return 0;}

    virtual std::string pluginID() const OVERRIDE;

    virtual std::string pluginLabel() const OVERRIDE;
    
    virtual std::string description() const OVERRIDE;

    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;

    Natron::Status renderWriter(SequenceTime time);

    const Natron::ChannelSet& requestedChannels() const {return _requestedChannels;}
    
    virtual int maximumInputs() const OVERRIDE {return 1;}

    void onKnobValueChanged(Knob* k,Natron::ValueChangedReason reason) OVERRIDE;

    bool continueOnError() const;

    std::string getOutputFileName() const;

public slots:
    void onTimelineFrameRangeChanged(SequenceTime,SequenceTime);

    
protected:
    
    
	virtual void initializeKnobs() OVERRIDE;
    
    virtual Natron::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE {return Natron::EffectInstance::FULLY_SAFE;}
    
private:
    
    void renderFunctor(boost::shared_ptr<const Natron::Image> inputImage,
                       const RectI& roi,
                       int view,
                       boost::shared_ptr<Encoder> encoder);
    
    boost::shared_ptr<Encoder> makeEncoder(SequenceTime time,int view,int totalViews,const RectI& rod);
    
    Natron::ChannelSet _requestedChannels;
    boost::shared_ptr<Bool_Knob> _premultKnob;
    EncoderKnobs* _writeOptions;
        
    boost::shared_ptr<OutputFile_Knob> _fileKnob;
    boost::shared_ptr<Choice_Knob> _filetypeCombo;
    boost::shared_ptr<Choice_Knob> _frameRangeChoosal;
    boost::shared_ptr<Int_Knob> _firstFrameKnob;
    boost::shared_ptr<Int_Knob> _lastFrameKnob;
    boost::shared_ptr<Button_Knob> _renderKnob;
    boost::shared_ptr<Bool_Knob> _continueOnError;

};

#endif

#endif /* defined(NATRON_WRITERS_WRITER_H_) */
