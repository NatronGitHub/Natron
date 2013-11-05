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

#ifndef POWITER_WRITERS_WRITER_H_
#define POWITER_WRITERS_WRITER_H_

#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "Global/Macros.h"
#include "Engine/EffectInstance.h"

class Encoder;
namespace Powiter{
    class Image;
}
class OutputFile_Knob;
class ComboBox_Knob;
class EncoderKnobs;
class Button_Knob;
class Int_Knob;
class Writer: public QObject,public Powiter::OutputEffectInstance {
    
    Q_OBJECT
    
public:
  
    static Powiter::EffectInstance* BuildEffect(Powiter::Node* n){
        return new Writer(n);
    }
    
    Writer(Powiter::Node* node);
        
    virtual ~Writer();
    
    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE {return false;}
    
    virtual std::string className() const OVERRIDE;
    
    virtual std::string description() const OVERRIDE;
    
    /*If doFullWork is true, it creates a new Write and set _writeHandle
     to this newly created Write. It also calls Write::setupFile with 
     the filename selected in the GUI. If the buffer is full, it appends
     the Write task to the _queuedTasks vector. They will be launched
     by the notifyWriterForCompletion() function.*/
    virtual bool validate() const;
        
    Powiter::Status renderWriter(SequenceTime time);
	
    /*Returns true if all parameters are OK to start the rendering.
     This is used to know whether it should start the engine or not.*/
    bool validInfosForRendering();
    
    const Powiter::ChannelSet& requestedChannels() const {return _requestedChannels;}
    
    virtual int maximumInputs() const OVERRIDE {return 1;}

    void onKnobValueChanged(Knob* k,Knob::ValueChangedReason reason) OVERRIDE;

    void startRendering();

    void getFirstFrameAndLastFrame(int* first,int *last);

public slots:
    void onTimelineFrameRangeChanged(int,int);

signals:
    
    void renderingOnDiskStarted(Writer*,QString,int,int);
    
protected:
    
    
	virtual void initializeKnobs() OVERRIDE;
    
    virtual Powiter::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE {return Powiter::EffectInstance::FULLY_SAFE;}
    
private:
    
    void renderFunctor(boost::shared_ptr<const Powiter::Image> inputImage,
                       const RectI& roi,
                       int view,
                       boost::shared_ptr<Encoder> encoder);
    
    boost::shared_ptr<Encoder> makeEncoder(SequenceTime time,int view,int totalViews,const RectI& rod);
    
    Powiter::ChannelSet _requestedChannels;
    bool _premult;
    EncoderKnobs* _writeOptions;
    std::string _filename;
    std::string _fileType;
    std::vector<std::string> _allFileTypes;
        
    OutputFile_Knob* _fileKnob;
    ComboBox_Knob* _filetypeCombo;
    ComboBox_Knob* _frameRangeChoosal;
    Int_Knob* _firstFrameKnob;
    Int_Knob* _lastFrameKnob;
    Button_Knob* _renderKnob;
};
Q_DECLARE_METATYPE(Writer*);
#endif /* defined(POWITER_WRITERS_WRITER_H_) */
