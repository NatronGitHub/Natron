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
#include "Engine/Node.h"

class Encoder;
namespace Powiter{
    class Image;
}
class OutputFile_Knob;
class ComboBox_Knob;
class EncoderKnobs;
class Int_Knob;
class Writer: public OutputNode{
    
    Q_OBJECT
    
public:
  
      
    Writer(AppInstance* app);
        
    virtual ~Writer();
    
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
    
    virtual int minimumInputs() const OVERRIDE {return 1;}
    
    
    int firstFrame() const {return _frameRange.first;}
    
    int lastFrame() const {return _frameRange.second;}
    
public slots:
    void onFilesSelected(const QString&);
    void fileTypeChanged();
    void startRendering();
    void onFrameRangeChoosalChanged();
    void onTimelineFrameRangeChanged(int,int);
signals:
    
    void renderingOnDiskStarted(Writer*,QString,int,int);
    
protected:
    
    
	virtual void initKnobs() OVERRIDE;
    
    virtual Node::RenderSafety renderThreadSafety() const OVERRIDE {return Node::FULLY_SAFE;}
    
private:
    
    void renderFunctor(boost::shared_ptr<const Powiter::Image> inputImage,
                       const Box2D& roi,
                       int view,
                       boost::shared_ptr<Encoder> encoder);
    
    boost::shared_ptr<Encoder> makeEncoder(SequenceTime time,int view,int totalViews,const Box2D& rod);
    
    Powiter::ChannelSet _requestedChannels;
    bool _premult;
    EncoderKnobs* _writeOptions;
    std::string _filename;
    std::string _fileType;
    std::vector<std::string> _allFileTypes;
    
    std::pair<int,int> _frameRange;
    
    OutputFile_Knob* _fileKnob;
    ComboBox_Knob* _filetypeCombo;
    ComboBox_Knob* _frameRangeChoosal;
    Int_Knob* _firstFrameKnob;
    Int_Knob* _lastFrameKnob;
    
};
Q_DECLARE_METATYPE(Writer*);
#endif /* defined(POWITER_WRITERS_WRITER_H_) */
