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

#ifndef NATRON_READERS_READQT_H_
#define NATRON_READERS_READQT_H_

#include <vector>
#include <string>

#include <QtCore/QMutex>

#include "Engine/EffectInstance.h"

namespace Natron {
    namespace Color {
        class Lut;
    }
}

class File_Knob;
class Choice_Knob;
class Int_Knob;
class QtReader : public Natron::EffectInstance {


public:
    static Natron::EffectInstance* BuildEffect(Natron::Node* n){
        return new QtReader(n);
    }
    
    static void supportedFileFormats(std::vector<std::string>* formats);
    
    QtReader(Natron::Node* node);
    
    virtual ~QtReader();
  
    virtual bool makePreviewByDefault() const OVERRIDE {return true;}
    
    virtual int majorVersion() const OVERRIDE { return 1; }
    
    virtual int minorVersion() const OVERRIDE { return 0;}
    
    virtual std::string pluginID() const OVERRIDE;
    
    virtual std::string pluginLabel() const OVERRIDE;
    
    virtual std::string description() const OVERRIDE;
    
    virtual Natron::Status getRegionOfDefinition(SequenceTime time,RectI* rod) OVERRIDE;
	
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;

    virtual int maximumInputs() const OVERRIDE {return 0;}
    
    virtual bool isGenerator() const OVERRIDE {return true;}
    
    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE { return false; }

    virtual Natron::Status render(SequenceTime time,RenderScale scale,
                                  const RectI& roi,int view,boost::shared_ptr<Natron::Image> output) OVERRIDE;
    
    virtual void initializeKnobs() OVERRIDE;

    virtual void onKnobValueChanged(Knob* k, Natron::ValueChangedReason reason) OVERRIDE;

    virtual Natron::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE {return Natron::EffectInstance::INSTANCE_SAFE;}
        
    virtual Natron::EffectInstance::CachePolicy getCachePolicy(SequenceTime time) const OVERRIDE;
    
    
    
private:

    void getSequenceTimeDomain(SequenceTime& first,SequenceTime& last);

    void timeDomainFromSequenceTimeDomain(SequenceTime& first,SequenceTime& last,bool mustSetFrameRange);

    SequenceTime getSequenceTime(SequenceTime t);

    void getFilenameAtSequenceTime(SequenceTime time, std::string &filename);


    const Natron::Color::Lut* _lut;
    std::string _filename;
    QImage* _img;
    QMutex _lock;
    boost::shared_ptr<File_Knob> _fileKnob;
    boost::shared_ptr<Int_Knob> _firstFrame;
    boost::shared_ptr<Choice_Knob> _before;
    boost::shared_ptr<Int_Knob> _lastFrame;
    boost::shared_ptr<Choice_Knob> _after;
    boost::shared_ptr<Choice_Knob> _missingFrameChoice;

    boost::shared_ptr<Choice_Knob> _frameMode;
    boost::shared_ptr<Int_Knob> _startingFrame;
    boost::shared_ptr<Int_Knob> _timeOffset;
    bool _settingFrameRange;
};

#endif /* defined(NATRON_READERS_READQT_H_) */
