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

#include "Writer.h"

#include <boost/bind.hpp>

#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QtCore/QFile>

#include "Global/LibraryBinary.h"
#include "Global/AppManager.h"

#include "Engine/Image.h"
#include "Engine/Settings.h"
#include "Engine/Settings.h"
#include "Engine/Knob.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"

#include "Writers/Encoder.h"

using namespace Natron;
using std::cout; using std::endl;
using std::make_pair;

Writer::Writer(Node* node):
    Natron::OutputEffectInstance(node)
  , _requestedChannels(Mask_RGBA) // temporary
  , _premultKnob(0)
  , _writeOptions(0)
  , _frameRangeChoosal(0)
  , _firstFrameKnob(0)
  , _lastFrameKnob(0)
  , _continueOnError(0)
{
    if(node){
        QObject::connect(getNode()->getApp()->getTimeLine().get(),
                         SIGNAL(frameRangeChanged(SequenceTime,SequenceTime)),
                         this,
                         SLOT(onTimelineFrameRangeChanged(SequenceTime, SequenceTime)));
    }
}

Writer::~Writer(){
    if(_writeOptions){
        delete _writeOptions;
    }
}

std::string Writer::pluginID() const {
    return "Writer";
}
std::string Writer::pluginLabel() const{
    return "Writer";
}

std::string Writer::description() const {
    return "The Writer node can render on disk the output of a node graph.";
}

static QString viewToString(int view,int viewsCount){
    if(viewsCount == 1){
        return "";
    }else if(viewsCount == 2){
        if(view == 0){
            return "_l";
        }else{
            return "_r";
        }
    }else{
        return QString("_view")+QString::number(view);
    }
}

boost::shared_ptr<Encoder> Writer::makeEncoder(SequenceTime time,int view,int totalViews,const RectI& rod){
    const std::string& fileType = _filetypeCombo->getActiveEntryText();
    const std::string& fileName = _fileKnob->getFileName();
    Natron::LibraryBinary* binary = appPTR->getCurrentSettings()._writersSettings.encoderForFiletype(fileType);
    Encoder* encoder = NULL;
    if(!binary){
        std::string exc("Couldn't find an appropriate encoder for filetype: ");
        exc.append(fileType);
        exc.append(" (");
        exc.append(getName());
        exc.append(")");
        throw std::invalid_argument(exc);
    }else{
        std::pair<bool,WriteBuilder> func = binary->findFunction<WriteBuilder>("BuildWrite");
        if(func.first)
            encoder = func.second(this);
        else{
            throw std::runtime_error("Library failed to create encoder for unknown reason.");
        }
    }
    
    encoder->premultiplyByAlpha(_premultKnob->getValue<bool>());
    /*check if the filename already contains the extension, otherwise appending it*/
    QString filename(fileName.c_str());
    int i = filename.lastIndexOf(QChar('.'));
    if(i != -1){
        filename.truncate(i); // truncate the extension
    }
    i = filename.lastIndexOf(QChar('#'));
    QString n = viewToString(view,totalViews) + QString("_") +  QString::number(time)  ;
    if(i != -1){
        filename = filename.replace(i,1,n);
    }else{
        i = filename.lastIndexOf(QChar('.'));
        filename = filename.insert(i, n);
    }
    filename.append('.');
    filename.append(fileType.c_str());
    
    encoder->setOptionalKnobsPtr(_writeOptions);
    encoder->initializeColorSpace();
    Status stat = encoder->_setupFile(filename, rod);
    if(stat == StatFailed){
        return boost::shared_ptr<Encoder>();
    }
    return boost::shared_ptr<Encoder>(encoder);
}

void Writer::initializeKnobs(){
    std::string fileDesc("File");
    _fileKnob = dynamic_cast<OutputFile_Knob*>(appPTR->getKnobFactory().createKnob("OutputFile", this, fileDesc));
    assert(_fileKnob);
    
    std::string renderDesc("Render");
    _renderKnob = dynamic_cast<Button_Knob*>(appPTR->getKnobFactory().createKnob("Button", this, renderDesc));
    assert(_renderKnob);
    
    std::string premultString("Premultiply by alpha");
    _premultKnob = dynamic_cast<Bool_Knob*>(appPTR->getKnobFactory().createKnob("Bool", this, premultString));
    _premultKnob->setValue(false);
    
    std::string filetypeStr("File type");
    _filetypeCombo = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", this, filetypeStr));
    const std::map<std::string,Natron::LibraryBinary*>& _encoders = appPTR->getCurrentSettings()._writersSettings.getFileTypesMap();
    std::map<std::string,Natron::LibraryBinary*>::const_iterator it = _encoders.begin();
    std::vector<std::string> fileTypes;
    for(;it!=_encoders.end();++it) {
        fileTypes.push_back(it->first.c_str());
    }
    _filetypeCombo->populate(fileTypes);
    
    
    _frameRangeChoosal = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", this, "Frame range"));
    std::vector<std::string> frameRangeChoosalEntries;
    frameRangeChoosalEntries.push_back("Inputs union");
    frameRangeChoosalEntries.push_back("Timeline bounds");
    frameRangeChoosalEntries.push_back("Manual");
    _frameRangeChoosal->populate(frameRangeChoosalEntries);
    
    _continueOnError = dynamic_cast<Bool_Knob*>(appPTR->getKnobFactory().createKnob("Bool", this, "Continue on error"));
    _continueOnError->setHintToolTip("If true, when an error arises for a frame,it will skip that frame and resume rendering.");
    _continueOnError->setInsignificant(true);
    _continueOnError->setValue(false);
    
}
bool Writer::continueOnError() const{
    return _continueOnError->getValue<bool>();
}

Natron::Status Writer::renderWriter(SequenceTime time){
    const Format& renderFormat = getRenderFormat();
    int viewsCount = getRenderViewsCount();
    //null RoD for key because the key hash computation doesn't take into account the box
    RenderScale scale;
    scale.x = scale.y = 1.;
    /*now that we have our image, we check what is left to render. If the list contains only
     null rect then we already rendered it all*/
    
    RoIMap inputsRoi = getRegionOfInterest(time, scale, renderFormat);
    //inputsRoi only contains 1 element
    RoIMap::const_iterator roi = inputsRoi.begin();
    for(int i = 0 ; i < viewsCount ; ++i){
        boost::shared_ptr<Encoder> encoder;
        try {
            encoder = makeEncoder(time,i,viewsCount,renderFormat);
        } catch (const std::exception& e) {
            setPersistentMessage(Natron::ERROR_MESSAGE, std::string("Cannot create encoder for frame") + ": " + e.what());
            return StatFailed;
        } catch (...) {
            setPersistentMessage(Natron::ERROR_MESSAGE, std::string("Cannot create encoder for frame"));
            return StatFailed;
        }
        if(!encoder){
            return StatFailed;
        }
        boost::shared_ptr<const Natron::Image> inputImage = roi->first->renderRoI(time, scale,i,roi->second);

        if(aborted()){
            return StatOK;
        }

        Natron::Status st = encoder->render(inputImage, i, renderFormat);
        if(st != StatOK){
            QFile::remove(encoder->filename());
            return st;
        }
        if(!aborted()){
            // finalize file if needed
            encoder->finalizeFile();
        }else{//remove file!
            QFile::remove(encoder->filename());
        }

    }
    //we released the input image and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();
    return StatOK;
}

void Writer::renderFunctor(boost::shared_ptr<const Natron::Image> inputImage,
                           const RectI& roi,
                           int view,
                           boost::shared_ptr<Encoder> encoder){
    encoder->render(inputImage,view,roi);
}


void Writer::getFrameRange(SequenceTime *first,SequenceTime *last){
    int index = _frameRangeChoosal->getValue<int>();
    if(index == 0){
        EffectInstance* inp = input(0);
        if(inp){
            inp->getFrameRange(first, last);
        }else{
            *first = 0;
            *last = 0;
        }
    }else if(index == 1){
        *first = getApp()->getTimeLine()->leftBound();
        *last = getApp()->getTimeLine()->rightBound();
    }else{
        *first = _firstFrameKnob->getValue<int>();
        *last = _lastFrameKnob->getValue<int>();
    }
}
std::string Writer::getOutputFileName() const{
    return _fileKnob->getFileName();
}

void Writer::onKnobValueChanged(Knob* k,Knob::ValueChangedReason /*reason*/){
    if(k == _filetypeCombo){
        const std::string& fileType = _filetypeCombo->getActiveEntryText();
        const std::string& fileName = _fileKnob->getFileName();
        if(_writeOptions){
            _writeOptions->cleanUpKnobs();
            delete _writeOptions;
            _writeOptions = 0;
        }
        Natron::LibraryBinary* isValid = appPTR->getCurrentSettings()._writersSettings.encoderForFiletype(fileType);
        if(!isValid) return;
        
        QString file(fileName.c_str());
        int pos = file.lastIndexOf(QChar('.'));
        if(pos != -1){
            //found an extension
            file = file.left(file.lastIndexOf(QChar('.'))); // remove existing extension
        }
        file.append(".");
        file.append(fileType.c_str());
        _fileKnob->setValue(file);
        
        /*checking if channels are supported*/
        std::pair<bool,WriteBuilder> func = isValid->findFunction<WriteBuilder>("BuildWrite");
        if(func.first){
            Encoder* write = func.second(this);
            _writeOptions = write->initSpecificKnobs();
            if(_writeOptions)
                _writeOptions->initKnobs(fileType);
            delete write;
        }

    }else if(k == _frameRangeChoosal){
        int index = _frameRangeChoosal->getValue<int>();
        if(index != 2){
            if(_firstFrameKnob){
                _firstFrameKnob->remove();;
                _firstFrameKnob = 0;
            }
            if(_lastFrameKnob){
                _lastFrameKnob->remove();
                _lastFrameKnob = 0;
            }
        }else{
            int first = getApp()->getTimeLine()->firstFrame();
            int last = getApp()->getTimeLine()->lastFrame();
            if(!_firstFrameKnob){
                _firstFrameKnob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int", this, "First frame"));
                _firstFrameKnob->turnOffAnimation();
                _firstFrameKnob->setValue(first);
                _firstFrameKnob->setDisplayMinimum(first);
                _firstFrameKnob->setDisplayMaximum(last);
                
            }
            if(!_lastFrameKnob){
                _lastFrameKnob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int", this, "Last frame"));
                _lastFrameKnob->turnOffAnimation();
                _lastFrameKnob->setValue(last);
                _lastFrameKnob->setDisplayMinimum(first);
                _lastFrameKnob->setDisplayMaximum(last);
                
            }
            createKnobDynamically();
        }

    }
}

void Writer::onTimelineFrameRangeChanged(SequenceTime f,SequenceTime l){
    
    if(_firstFrameKnob){
        _firstFrameKnob->setValue(f);
        _firstFrameKnob->setDisplayMinimum(f);
        _firstFrameKnob->setDisplayMaximum(l);
        
    }
    if(_lastFrameKnob){
        _lastFrameKnob->setValue(l);
        _lastFrameKnob->setDisplayMinimum(f);
        _lastFrameKnob->setDisplayMaximum(l);
        
    }

}

