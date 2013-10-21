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

#include "ExrEncoder.h"

#include <stdexcept>
#include <QtCore/QMutexLocker>
#include <QtCore/QMutex>
#include <ImfChannelList.h>
#include <ImfArray.h>
#include <half.h>

#include "Writers/Writer.h"
#include "Engine/Lut.h"
#include "Gui/KnobGui.h"
#include "Engine/Row.h"

using namespace Powiter;
using std::make_pair;
using std::cout; using std::endl;

namespace EXR {
static std::string const compressionNames[6]={
    "No compression",
    "Zip (1 scanline)",
    "Zip (16 scanlines)",
    "PIZ Wavelet (32 scanlines)",
    "RLE",
    "B44"
};

static Imf::Compression stringToCompression(const std::string& str){
    if(str == compressionNames[0]){
        return Imf::NO_COMPRESSION;
    }else if(str == compressionNames[1]){
        return Imf::ZIPS_COMPRESSION;
    }else if(str == compressionNames[2]){
        return Imf::ZIP_COMPRESSION;
    }else if(str == compressionNames[3]){
        return Imf::PIZ_COMPRESSION;
    }else if(str == compressionNames[4]){
        return Imf::RLE_COMPRESSION;
    }else{
        return Imf::B44_COMPRESSION;
    }
}

static  std::string const depthNames[2] = {
    "16 bit half", "32 bit float"
};

static int depthNameToInt(const std::string& name){
    if(name == depthNames[0]){
        return 16;
    }else{
        return 32;
    }
}

static std::string toExrChannel(Powiter::Channel from){
    if (from == Powiter::Channel_red) {
        return "R";
    }
    if (from == Powiter::Channel_green) {
        return "G";
    }
    if (from == Powiter::Channel_blue) {
        return "B";
    }
    if (from == Powiter::Channel_alpha) {
        return "A";
    }
    if (from == Powiter::Channel_Z) {
        return "Z";
    }
    return getChannelName(from);
}

#if 0 // unused functions
static bool timeCodeFromString(const std::string& str, Imf::TimeCode& attr)
{
    if (str.length() != 11)
        return false;

    int hours = 0, mins = 0, secs = 0, frames = 0;

    sscanf(str.c_str(), "%02d:%02d:%02d:%02d", &hours, &mins, &secs, &frames);
    try {
        attr.setHours(hours);
        attr.setMinutes(mins);
        attr.setSeconds(secs);
        attr.setFrame(frames);
    }
    catch (const std::exception& exc) {
        std::cout << "EXR: Time Code Metadata warning" <<  exc.what() << std::endl;
        return false;
    }
    return true;
}

static bool edgeCodeFromString(const std::string& str, Imf::KeyCode& a)
{
    int mfcCode, filmType, prefix, count, perfOffset;
    sscanf(str.c_str(), "%d %d %d %d %d", &mfcCode, &filmType, &prefix, &count, &perfOffset);

    try {
        a.setFilmMfcCode(mfcCode);
        a.setFilmType(filmType);
        a.setPrefix(prefix);
        a.setCount(count);
        a.setPerfOffset(perfOffset);
    }

    catch (const std::exception& exc) {
        std::cout << "EXR: Edge Code Metadata warning " << exc.what() << std::endl;
        return false;
    }

    return true;
}
#endif //0
} // namespace EXR

ExrEncoder::ExrEncoder(Writer* writer):Encoder(writer),outfile(0),_lock(0){

}
ExrEncoder::~ExrEncoder(){

}

std::vector<std::string> ExrEncoder::fileTypesEncoded() const {
    std::vector<std::string> out;
    out.push_back("exr");
    return out;
}

void ExrEncoderKnobs::initKnobs(const std::string& fileType) {
    std::string separatorDesc(fileType);
    separatorDesc.append(" Options");
    sepKnob = dynamic_cast<Separator_Knob*>(appPTR->getKnobFactory().createKnob("Separator", _writer, separatorDesc));
    
    std::string compressionCBDesc("Compression");
    compressionCBKnob = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", _writer, compressionCBDesc));
    std::vector<std::string> list;
    for (int i =0; i < 6; ++i) {
        list.push_back(EXR::compressionNames[i].c_str());
    }
    compressionCBKnob->populate(list);
    compressionCBKnob->setValue(3);
    
    std::string depthCBDesc("Data type");
    depthCBKnob = static_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", _writer,depthCBDesc));
    list.clear();
    for(int i = 0 ; i < 2 ; ++i) {
        list.push_back(EXR::depthNames[i].c_str());
    }
    depthCBKnob->populate(list);
    depthCBKnob->setValue(1);
    
    /*calling base-class version at the end*/
    EncoderKnobs::initKnobs(fileType);
}
void ExrEncoderKnobs::cleanUpKnobs(){
    sepKnob->deleteKnob();
    compressionCBKnob->deleteKnob();
    depthCBKnob->deleteKnob();
}

bool ExrEncoderKnobs::allValid(){
    return _dataType!="/" && _compression!="/";
}

/*Must implement it to initialize the appropriate colorspace  for
 the file type. You can initialize the _lut member by calling the
 function getLut(datatype) */
void ExrEncoder::initializeColorSpace(){
    _lut = Color::getLut(Color::LUT_DEFAULT_FLOAT);
}

/*This must be implemented to do the output colorspace conversion*/
void ExrEncoder::renderRow(SequenceTime time,int left,int right,int y,const ChannelSet& channels){
    boost::shared_ptr<const Row> row = _writer->input(0)->get(time,y,left,right,channels);
    const float* a = row->begin(Channel_alpha);
   
    Row* toRow = new Row(left,y,right,channels);
    foreachChannels(z, channels){
        const float* from = row->begin(z);
        float* to = toRow->begin(z);
        to_float(z, to , from, a, row->width());
    }

    {
        QMutexLocker locker(_lock);
        _img.insert(make_pair(y,toRow));
    }
}

/*This function initialises the output file/output storage structure and put necessary info in it, like
 meta-data, channels, etc...This is called on the main thread so don't do any extra processing here,
 otherwise it would stall the GUI.*/
void ExrEncoder::setupFile(const QString& filename,const Box2D& rod){
    _lock = new QMutex;
    _dataW = rod;
    _filename = filename.toStdString();
    ExrEncoderKnobs* knobs = dynamic_cast<ExrEncoderKnobs*>(_optionalKnobs);
    compression = EXR::stringToCompression(knobs->_compression);
    depth = EXR::depthNameToInt(knobs->_dataType);
    const ChannelSet& channels = _writer->requestedChannels();
    _dataW = rod;
//    if(op->info().blackOutside()){
//        if(dataW.left() + 2 < dataW.right()){
//            _dataW->set_left(dataW.left()+1);
//            _dataW->set_right(dataW.right()-1);
//        }
//        if(dataW.bottom() +2 < dataW.top()){
//            _dataW->set_bottom(dataW.bottom()+1);
//            _dataW->set_top(dataW.top()-1);
//        }
//    }else{
//        _dataW->set(dataW);
//    }
    exrDataW = new Imath::Box2i;
    exrDispW = new Imath::Box2i;
    exrDataW->min.x = _dataW.left();
    exrDataW->min.y = _dataW.height() - _dataW.top();
    exrDataW->max.x = _dataW.right() - 1;
    exrDataW->max.y = _dataW.height() - _dataW.bottom() - 1;
    
    exrDispW->min.x = 0;
    exrDispW->min.y = 0;
    exrDispW->max.x = _dataW.width() - 1;
    exrDispW->max.y = _dataW.height() - 1;
    
    header=new Imf::Header(*exrDispW, *exrDataW,1.,
                          Imath::V2f(0, 0), 1, Imf::INCREASING_Y, compression);
    
    foreachChannels(z, channels){
        std::string channame = EXR::toExrChannel(z);
        if (depth == 32) {
            header->channels().insert(channame.c_str(), Imf::Channel(Imf::FLOAT));
        }
        else {
            header->channels().insert(channame.c_str(), Imf::Channel(Imf::HALF));
        }
    }


        
    
}

/*This function must fill the pre-allocated structure with the data calculated by engine.
 This function must close the file as writeAllData is the LAST function called before the
 destructor of Write.*/
void ExrEncoder::writeAllData(){
    
    try{
        outfile = new Imf::OutputFile(_filename.c_str(), *header);
        const ChannelSet& channels = _writer->requestedChannels();

        for (int y = _dataW.top()-1; y >= _dataW.bottom(); y--) {
            if(_writer->aborted()){
                break;
            }
            Imf::FrameBuffer fbuf;
            Imf::Array2D<half>* halfwriterow = 0 ;
            Row* row = _img[y];
            assert(row);
            if (depth == 32) {
                foreachChannels(z, channels){
                    std::string channame = EXR::toExrChannel(z);
                    fbuf.insert(channame.c_str(),
                                Imf::Slice(Imf::FLOAT, (char*)row->begin(z), // watch out begin does not point to 0 anymore
                                           sizeof(float), 0));
                }
            }else{
                halfwriterow = new Imf::Array2D<half>(channels.size() , _dataW.width());
                
                int cur = 0;
                foreachChannels(z, channels){
                    std::string channame = EXR::toExrChannel(z);
                    fbuf.insert(channame.c_str(),
                                Imf::Slice(Imf::HALF,
                                           (char*)(&(*halfwriterow)[cur][0] - exrDataW->min.x),
                                           sizeof((*halfwriterow)[cur][0]), 0));
                    const float* from = row->begin(z); // watch out begin does not point to 0 anymore
                    for(int i = exrDataW->min.x; i < exrDataW->max.x ; ++i) {
                        (*halfwriterow)[cur][i - exrDataW->min.x] = from[i];
                    }
                    ++cur;
                }
                delete halfwriterow;
            }
            _img.erase(y);
            delete row;
            // row is unlocked by release()
            outfile->setFrameBuffer(fbuf);
            outfile->writePixels(1);
        }

        delete outfile;
        delete exrDataW;
        delete exrDispW;
        delete _lock;
        delete header;

    }catch (const std::exception& exc) {
        cout << "OpenEXR error: " << exc.what() << endl;
        return;
    }
    
}

void ExrEncoder::debug(){
    int notValidC = 0;
    for (std::map<int,Row*>::iterator it = _img.begin(); it!=_img.end(); ++it) {
        cout << "img[" << it->first << "] = ";
        if(it->second) cout << "valid row" << endl;
        else{
            cout << "NOT VALID" << endl;
            ++notValidC;
        }
    }
    cout << "Img size : " << _img.size() << " . Invalid rows = " << notValidC << endl;
}
