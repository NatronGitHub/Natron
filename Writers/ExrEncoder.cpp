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
#include <ImfOutputFile.h> // FIXME: should be PIMPL'ed
#include <half.h>

#include "Writers/Writer.h"
#include "Engine/Lut.h"
#include "Gui/KnobGui.h"
#include "Engine/Row.h"

using std::make_pair;
using std::cout; using std::endl;

#ifndef OPENEXR_IMF_NAMESPACE
#define OPENEXR_IMF_NAMESPACE Imf
#endif
namespace Imf_ = OPENEXR_IMF_NAMESPACE;

struct ExrEncoder::Implementation {
    Implementation();

    std::string _filename;
    std::map<int,Powiter::Row*> _img;

    Box2D _dataW;
    QMutex _lock;
};

namespace EXR {
static std::string const compressionNames[6]={
    "No compression",
    "Zip (1 scanline)",
    "Zip (16 scanlines)",
    "PIZ Wavelet (32 scanlines)",
    "RLE",
    "B44"
};

static Imf_::Compression stringToCompression(const std::string& str){
    if(str == compressionNames[0]){
        return Imf_::NO_COMPRESSION;
    }else if(str == compressionNames[1]){
        return Imf_::ZIPS_COMPRESSION;
    }else if(str == compressionNames[2]){
        return Imf_::ZIP_COMPRESSION;
    }else if(str == compressionNames[3]){
        return Imf_::PIZ_COMPRESSION;
    }else if(str == compressionNames[4]){
        return Imf_::RLE_COMPRESSION;
    }else{
        return Imf_::B44_COMPRESSION;
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
    return Powiter::getChannelName(from);
}

#if 0 // unused functions
static bool timeCodeFromString(const std::string& str, Imf_::TimeCode& attr)
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

static bool edgeCodeFromString(const std::string& str, Imf_::KeyCode& a)
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

ExrEncoder::ExrEncoder(Writer* writer)
: Encoder(writer)
, _imp(new Implementation)
{
}

ExrEncoder::Implementation::Implementation()
: _filename()
, _img()
, _dataW()
, _lock()
{
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
    _lut = Powiter::Color::getLut(Powiter::Color::LUT_DEFAULT_FLOAT);
}

/*This must be implemented to do the output colorspace conversion*/
void ExrEncoder::renderRow(SequenceTime time,int left,int right,int y,const Powiter::ChannelSet& channels){
    boost::shared_ptr<const Powiter::Row> row = _writer->input(0)->get(time,y,left,right,channels);
    const float* a = row->begin(Powiter::Channel_alpha);
   
    Powiter::Row* toRow = new Powiter::Row(left,y,right,channels);
    foreachChannels(z, channels){
        const float* from = row->begin(z);
        float* to = toRow->begin(z);
        to_float(z, to , from, a, row->width());
    }

    {
        QMutexLocker locker(&_imp->_lock);
        _imp->_img.insert(make_pair(y,toRow));
    }
}

/*This function initialises the output file/output storage structure and put necessary info in it, like
 meta-data, channels, etc...This is called on the main thread so don't do any extra processing here,
 otherwise it would stall the GUI.*/
void ExrEncoder::setupFile(const QString& filename, const Box2D& rod) {
    _imp->_dataW = rod;
    _imp->_filename = filename.toStdString();
}

/*This function must fill the pre-allocated structure with the data calculated by engine.
 This function must close the file as writeAllData is the LAST function called before the
 destructor of Write.*/
void ExrEncoder::writeAllData() {
    ExrEncoderKnobs* knobs = dynamic_cast<ExrEncoderKnobs*>(_optionalKnobs);
    Imf_::Compression compression(EXR::stringToCompression(knobs->_compression));
    int depth = EXR::depthNameToInt(knobs->_dataType);
    Imath::Box2i exrDataW;
    exrDataW.min.x = _imp->_dataW.left();
    exrDataW.min.y = _imp->_dataW.height() - _imp->_dataW.top();
    exrDataW.max.x = _imp->_dataW.right() - 1;
    exrDataW.max.y = _imp->_dataW.height() - _imp->_dataW.bottom() - 1;

    Imath::Box2i exrDispW;
    exrDispW.min.x = 0;
    exrDispW.min.y = 0;
    exrDispW.max.x = _imp->_dataW.width() - 1;
    exrDispW.max.y = _imp->_dataW.height() - 1;

    try {
        const Powiter::ChannelSet& channels = _writer->requestedChannels();
        Imf_::Header exrheader(exrDispW, exrDataW, 1.,
                            Imath::V2f(0, 0), 1, Imf_::INCREASING_Y, compression);

        foreachChannels(z, channels){
            std::string channame = EXR::toExrChannel(z);
            if (depth == 32) {
                exrheader.channels().insert(channame.c_str(), Imf_::Channel(Imf_::FLOAT));
            } else {
                assert(depth == 16);
                exrheader.channels().insert(channame.c_str(), Imf_::Channel(Imf_::HALF));
            }
        }

        Imf_::OutputFile outfile(_imp->_filename.c_str(), exrheader);

        for (int y = _imp->_dataW.top()-1; y >= _imp->_dataW.bottom(); y--) {
            if(_writer->aborted()){
                break;
            }
            Imf_::FrameBuffer fbuf;
            Imf_::Array2D<half>* halfwriterow = 0 ;
            Powiter::Row* row = _imp->_img[y];
            assert(row);
            if (depth == 32) {
                foreachChannels(z, channels){
                    std::string channame = EXR::toExrChannel(z);
                    fbuf.insert(channame.c_str(),
                                Imf_::Slice(Imf_::FLOAT, (char*)row->begin(z), // watch out begin does not point to 0 anymore
                                           sizeof(float), 0));
                }
            } else {
                halfwriterow = new Imf_::Array2D<half>(channels.size() , _imp->_dataW.width());
                
                int cur = 0;
                foreachChannels(z, channels){
                    std::string channame = EXR::toExrChannel(z);
                    fbuf.insert(channame.c_str(),
                                Imf_::Slice(Imf_::HALF,
                                           (char*)(&(*halfwriterow)[cur][0] - exrDataW.min.x),
                                           sizeof((*halfwriterow)[cur][0]), 0));
                    const float* from = row->begin(z); // watch out begin does not point to 0 anymore
                    for (int i = exrDataW.min.x; i < exrDataW.max.x ; ++i) {
                        (*halfwriterow)[cur][i - exrDataW.min.x] = from[i];
                    }
                    ++cur;
                }
                delete halfwriterow;
            }
            _imp->_img.erase(y);
            delete row;
            // row is unlocked by release()
            outfile.setFrameBuffer(fbuf);
            outfile.writePixels(1);
        }
    } catch (const std::exception& exc) {
        cout << "OpenEXR error: " << exc.what() << endl;
        return;
    }
    
}

void ExrEncoder::debug(){
    int notValidC = 0;
    for (std::map<int,Powiter::Row*>::iterator it = _imp->_img.begin(); it != _imp->_img.end(); ++it) {
        cout << "img[" << it->first << "] = ";
        if(it->second) cout << "valid row" << endl;
        else{
            cout << "NOT VALID" << endl;
            ++notValidC;
        }
    }
    cout << "Img size : " << _imp->_img.size() << " . Invalid rows = " << notValidC << endl;
}
