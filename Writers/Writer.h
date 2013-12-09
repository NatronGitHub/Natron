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
    Bool_Knob* _premultKnob;
    EncoderKnobs* _writeOptions;
        
    OutputFile_Knob* _fileKnob;
    Choice_Knob* _filetypeCombo;
    Choice_Knob* _frameRangeChoosal;
    Int_Knob* _firstFrameKnob;
    Int_Knob* _lastFrameKnob;
    Button_Knob* _renderKnob;
    Bool_Knob* _continueOnError;
};
#endif /* defined(NATRON_WRITERS_WRITER_H_) */
