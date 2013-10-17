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

#include "ReadExr.h"

#ifdef __POWITER_WIN32__
#include <fstream>
#endif
#include <QtGui/QImage>
#include <QtCore/QByteArray>
#include <ImfPixelType.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#ifdef __POWITER_WIN32__
#include <ImfStdIO.h>
#endif

#include "Readers/Reader.h"
#include "Gui/KnobGui.h"
#include "Engine/Node.h"
#include "Gui/NodeGui.h"
#include "Gui/ViewerGL.h"
#include "Engine/Lut.h"
#include "Engine/Row.h"
#include "Engine/ImageInfo.h"
using namespace std;
using namespace Powiter;

namespace EXR {
static Powiter::Channel fromExrChannel(const std::string& from)
{
    if (from == "R" ||
        from == "r" ||
        from == "Red" ||
        from == "RED" ||
        from == "red" ||
        from == "y" ||
        from == "Y") {
        return Powiter::Channel_red;
    }
    if (from == "G" ||
        from == "g" ||
        from == "Green" ||
        from == "GREEN" ||
        from == "green" ||
        from == "ry" ||
        from == "RY") {
        return Powiter::Channel_green;
    }
    if (from == "B" ||
        from == "b" ||
        from == "Blue" ||
        from == "BLUE" ||
        from == "blue" ||
        from == "by" ||
        from == "BY") {
        return Powiter::Channel_blue;
    }
    if (from == "A" ||
        from == "a" ||
        from == "Alpha" ||
        from == "ALPHA" ||
        from == "alpha") {
        return Powiter::Channel_alpha;
    }
    if (from == "Z" ||
        from == "z" ||
        from == "Depth" ||
        from == "DEPTH" ||
        from == "depth") {
        return Powiter::Channel_Z;
    }
    // The following may throw if from is not a channel name which begins with "Channel_"
    return getChannelByName(from);
}

class ChannelExtractor
{
public:
    ChannelExtractor(const std::string& name, const std::vector<std::string>& views) :
    _mappedChannel(Powiter::Channel_black),
    _valid(false)
    {     _valid = extractExrChannelName(name.c_str(), views);  }

    ~ChannelExtractor() {}

    Powiter::Channel _mappedChannel;
    bool _valid;
    std::string _chan;
    std::string _layer;
    std::string _view;
    
    std::string exrName() const{
        if (!_layer.empty())
            return _layer + "." + _chan;
        return _chan;
    }
    
    bool isValid() const {return _valid;}

private:

    
    static bool IsView(const std::string& name, const std::vector<std::string>& views)
    {
        for ( size_t i = 0; i < views.size(); ++i ){
            if ( views[i] == name ){
                return true;
            }
        }
        
        return false;
    }
 
    bool extractExrChannelName(const QString& channelname,
                               const std::vector<std::string>& views){
        _chan.clear();
        _layer.clear();
        _view.clear();
        
        QStringList splits = channelname.split(QChar('.'),QString::SkipEmptyParts);
        QStringList newSplits;
        //remove prepending digits
        for (int i = 0; i < splits.size(); ++i) {
            QString str = splits.at(i);
            int j = 0;
            while (j < str.size() && str.at(j).isDigit()) { ++j; }
            str = str.remove(0, j);
            if(!str.isEmpty()){
                //remove non alphanumeric chars
                QString finalStr;
                for (int k = 0; k < str.size(); ++k) {
                    QChar c = str.at(k);
                    if (!c.isLetterOrNumber())  {
                        c = '_';
                    }
                    finalStr.append(c);
                }
                newSplits << finalStr;
            }
        }
        
        if (newSplits.size() > 1){
            
            for (int i = 0; i < (newSplits.size() - 1);++i) {
                std::vector<std::string>::const_iterator foundView = std::find(views.begin(), views.end(),newSplits.at(i).toStdString());
                if (foundView != views.end()) {
                    _view = *foundView;
                } else {
                    if (!_layer.empty())
                        _layer += "_";
                    _layer += newSplits.at(i).toStdString();
                }
            }
            _chan = newSplits.back().toStdString();
        } else {
            _chan = newSplits.at(0).toStdString();
        }
        
        // std::string viewpart = (channelName._view.length() > 0) ? channelName._view : heroview;
        
        //if (viewname(view) == viewpart) {
        //is_stereo= true;
        try{
            _mappedChannel = EXR::fromExrChannel(_chan);
        } catch (const std::exception &e) {
            std::cout << e.what() << endl;
            return false;
        }
        return true;
        //}
        
        //    if (viewpart != "" && viewpart != heroview) {
        //        return false;
        //    }
        
        //    exrToPowiterChannelMapping(channel, otherpart.c_str());
        
        //    return true;

    }
};


} // namespace EXR




Powiter::Status ReadExr::readHeader(const QString& filename){
    
    _channel_map.clear();
     views.clear();
    try {
#ifdef __POWITER_WIN32__
        QByteArray ba = filename.toLocal8Bit();
        if(inputStr){
            delete inputStr;
        }
        if(inputStdStream){
            delete inputStdStream;
        }
        if(inputfile){
            delete inputfile;
        }
        inputStr = new std::ifstream(PowiterWindows::s2ws(ba.data()),std::ios_base::binary);
        inputStdStream = new Imf::StdIFStream(*inputStr,ba.data());
        inputfile = new Imf::InputFile(*inputStdStream);
#else
        QByteArray ba = filename.toLatin1();
        if(_inputfile){
            delete _inputfile;
        }
        _inputfile = new Imf::InputFile(ba.constData());
#endif

        // multiview is only supported with OpenEXR >= 1.7.0
#ifdef INCLUDED_IMF_STRINGVECTOR_ATTRIBUTE_H // use ImfStringVectorAttribute.h's #include guard
        //int view = r->view_for_reader();
        const Imf::StringAttribute* stringMultiView = 0;
        const Imf::StringVectorAttribute* vectorMultiView = 0;
        
        try {
            vectorMultiView = inputfile->header().findTypedAttribute<Imf::StringVectorAttribute>("multiView");
            
            if (!vectorMultiView) {
                Imf::Header::ConstIterator it = inputfile->header().find("multiView");
                if (it != inputfile->header().end() && !strcmp(it.attribute().typeName(), "stringvector"))
                    vectorMultiView = static_cast<const Imf::StringVectorAttribute*>(&it.attribute());
            }
            
            stringMultiView = inputfile->header().findTypedAttribute<Imf::StringAttribute>("multiView");
        }
        catch (...) {
            return StatFailed;
        }
        
        if (vectorMultiView) {
            std::vector<std::string> s = vectorMultiView->value();
            
            bool setHero = false;
            
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i].length()) {
                    views.push_back(s[i]);
                    if (!setHero) {
                        heroview = s[i];
                        setHero = true;
                    }
                }
            }
        }
#endif // !OPENEXR_NO_MULTIVIEW
        map<Imf::PixelType, int> pixelTypes;
        // convert exr channels to powiter channels
        ChannelSet mask;
        const Imf::ChannelList& imfchannels = _inputfile->header().channels();
        Imf::ChannelList::ConstIterator chan;
        for (chan = imfchannels.begin(); chan != imfchannels.end(); ++chan) {
            string chanName(chan.name());
            if(chanName.empty())
                continue;
            pixelTypes[chan.channel().type]++;
            EXR::ChannelExtractor exrExctractor(chan.name(), views);
            set<Channel> channels;
            if (exrExctractor.isValid()) {
                channels.insert(exrExctractor._mappedChannel);
                //cout << "size : "<< channels.size() << endl;
                for (set<Channel>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
                    Channel channel = *it;
                    //cout <<" channel_map[" << getChannelName(channel) << "] = " << chan.name() << endl;
                    bool writeChannelMapping = true;
                    ChannelsMap::const_iterator found = _channel_map.find(channel);
                    if(found != _channel_map.end()){
                        int existingLength = found->second.size();
                        int newLength = chanName.size();
                        if ((existingLength > 0) && found->second.at(0) == '.' && existingLength == (newLength + 1)) {                                writeChannelMapping = true;
                        }
                        else if (existingLength > newLength) {
                            writeChannelMapping = false;
                        }
                    }
                    if(writeChannelMapping){
                        _channel_map.insert(make_pair(channel,chanName));
                    }
                    mask += channel;
                }
                
            }else {
                cout << "Cannot decode channel " << chan.name() << endl;
            }
            
        }
        
        const Imath::Box2i& datawin = _inputfile->header().dataWindow();
        const Imath::Box2i& dispwin = _inputfile->header().displayWindow();
        Imath::Box2i formatwin(dispwin);
        formatwin.min.x = 0;
        formatwin.min.y = 0;
        _dataOffset = 0;
        if (dispwin.min.x != 0) {
            // Shift both to get dispwindow over to 0,0.
            _dataOffset = -dispwin.min.x;
            formatwin.max.x = dispwin.max.x + _dataOffset;
        }
        formatwin.max.y = dispwin.max.y - dispwin.min.y;
        double aspect = _inputfile->header().pixelAspectRatio();
        Format imageFormat(0,0,formatwin.max.x + 1 ,formatwin.max.y + 1,"",aspect);
        Box2D bbox;
        
        int bx = datawin.min.x + _dataOffset;
        int by = dispwin.max.y - datawin.max.y;
        int br = datawin.max.x + _dataOffset;
        int bt = dispwin.max.y - datawin.min.y;
        if (datawin.min.x != dispwin.min.x || datawin.max.x != dispwin.max.x ||
                datawin.min.y != dispwin.min.y || datawin.max.y != dispwin.max.y) {
            bx--;
            by--;
            br++;
            bt++;
            //            _readerInfo->setBlackOutside(true);
        }
        bbox.set(bx, by, br+1, bt+1);
        
        setReaderInfo(imageFormat, bbox, mask);
        return StatOK;
    }
    catch (const std::exception& exc) {
        cout << "OpenExr error: " << exc.what() << endl;
        delete _inputfile;
        _inputfile = 0;
        return StatFailed;
    }
    
}


ReadExr::ReadExr(Reader* op):Read(op)
,_inputfile(0)
,_dataOffset(0){
#ifdef __POWITER_WIN32__
    inputStr = NULL;
    inputStdStream = NULL;
#endif
}

void ReadExr::initializeColorSpace(){
    _lut=Color::getLut(Color::LUT_DEFAULT_FLOAT); // linear color-space for exr files
}

ReadExr::~ReadExr(){
#ifdef __POWITER_WIN32__
    delete inputStr ;
    delete inputStdStream ;
#endif
    delete _inputfile;
}

void ReadExr::render(SequenceTime /*time*/,Row* out){
    const ChannelSet& channels = out->channels();
    const Imath::Box2i& dispwin = _inputfile->header().displayWindow();
    const Imath::Box2i& datawin = _inputfile->header().dataWindow();
    int exrY = dispwin.max.y - out->y();
    int r = out->right();
    int x = out->left();
    
    const int X = max(x, datawin.min.x + _dataOffset);
    const int R = min(r, datawin.max.x + _dataOffset +1);
    
    // if we're below or above the data window
    if(exrY < datawin.min.y || exrY > datawin.max.y || R <= X) {
        out->eraseAll();
        return;
    }
    
    Imf::FrameBuffer fbuf;
    foreachChannels(z, channels){
        // blacking out the extra padding we added
        float* dest = out->begin(z) - out->left();
        for (int xx = x; xx < X; xx++)
            dest[xx] = 0;
        for (int xx = R; xx < r; xx++)
            dest[xx] = 0;
        ChannelsMap::const_iterator found = _channel_map.find(z);
        if(found != _channel_map.end()){
            if(found->second != "BY" && found->second != "RY"){ // if it is NOT a subsampled buffer
                fbuf.insert(found->second,Imf::Slice(Imf::FLOAT, (char*)(dest /*+_dataOffset*/),sizeof(float), 0));
            }else{
                fbuf.insert(found->second,Imf::Slice(Imf::FLOAT, (char*)(dest /*+_dataOffset*/),sizeof(float), 0,2,2));
            }
        }else{
            //not found in the file, we zero it out
            // we're responsible for filling all channels requested by the row
            float fillValue = 0.f;
            if (z == Channel_alpha) {
                fillValue = 1.f;
            }
            std::fill(out->begin(z),out->end(z),fillValue);
        }
    }
    {
        QMutexLocker locker(&_lock);
        try {
            _inputfile->setFrameBuffer(fbuf);
            _inputfile->readPixels(exrY);
        }
        catch (const std::exception& exc) {
            cout << exc.what() <<  endl;
            return;
        }
    }
    
    //  colorspace conversion
    const float* alpha = out->begin(Channel_alpha);
    foreachChannels(z, channels){
        float* to = out->begin(z) - out->left();
        const float* from = out->begin(z) - out->left();
        if(from){
            from_float(z,to + X ,from + X,alpha, R-X,1);
        }
    }
}

//META DATA HANDLING:
//    const Imf::Header header = inputfile->header();
//    /*if (pixelTypes[Imf::FLOAT] > 0) {
//     _meta.setData(MetaData::DEPTH, MetaData::DEPTH_FLOAT);
//     }
//     else if (pixelTypes[Imf::UINT] > 0) {
//     _meta.setData(MetaData::DEPTH, MetaData::DEPTH_32);
//     }
//     if (pixelTypes[Imf::HALF] > 0) {
//     _meta.setData(MetaData::DEPTH, MetaData::DEPTH_HALF);
//     }
//     MakeNeededViews(views);*/
//    for (Imf::Header::ConstIterator i = header.begin();
//         i != header.end();
//         ++i) {
//        //const char* type = i.attribute().typeName();
//
//        /*std::string key = std::string(MetaData::EXR::EXR_PREFIX) + i.name();
//
//         if ( inputfile->header().hasTileDescription() ) {
//         _meta.setData( std::string(MetaData::EXR::EXR_TILED), true);
//         }
//
//         if (!strcmp(i.name(), "timeCode")) {
//         key = MetaData::TIMECODE;
//         }
//
//         if (!strcmp(i.name(), "expTime")) {
//         key = MetaData::EXPOSURE;
//         }
//
//         if (!strcmp(i.name(), "framesPerSecond")) {
//         key = MetaData::FRAME_RATE;
//         }
//
//         if (!strcmp(i.name(), "keyCode")) {
//         key = MetaData::EDGECODE;
//         }
//
//         if (!strcmp(i.name(), MetaData::Nuke::NODE_HASH )) {
//         key = MetaData::Nuke::NODE_HASH;
//         }
//
//         if (!strcmp(type, "string")) {
//         const Imf::StringAttribute* attr = static_cast<const Imf::StringAttribute*>(&i.attribute());
//         _meta.setData(key, attr->value());
//         }
//         else if (!strcmp(type, "int")) {
//         const Imf::IntAttribute* attr = static_cast<const Imf::IntAttribute*>(&i.attribute());
//         _meta.setData(key, attr->value());
//         }
//         else if (!strcmp(type, "v2i")) {
//         const Imf::V2iAttribute* attr = static_cast<const Imf::V2iAttribute*>(&i.attribute());
//         int values[2] = {
//         attr->value().x, attr->value().y
//         };
//         _meta.setData(key, values, 2);
//         }
//         else if (!strcmp(type, "v3i")) {
//         const Imf::V3iAttribute* attr = static_cast<const Imf::V3iAttribute*>(&i.attribute());
//         int values[3] = {
//         attr->value().x, attr->value().y, attr->value().z
//         };
//         _meta.setData(key, values, 3);
//         }
//         else if (!strcmp(type, "box2i")) {
//         const Imf::Box2iAttribute* attr = static_cast<const Imf::Box2iAttribute*>(&i.attribute());
//         int values[4] = {
//         attr->value().min.x, attr->value().min.y, attr->value().max.x, attr->value().max.y
//         };
//         _meta.setData(key, values, 4);
//         }
//         else if (!strcmp(type, "float")) {
//         const Imf::FloatAttribute* attr = static_cast<const Imf::FloatAttribute*>(&i.attribute());
//         _meta.setData(key, attr->value());
//         }
//         else if (!strcmp(type, "v2f")) {
//         const Imf::V2fAttribute* attr = static_cast<const Imf::V2fAttribute*>(&i.attribute());
//         float values[2] = {
//         attr->value().x, attr->value().y
//         };
//         _meta.setData(key, values, 2);
//         }
//         else if (!strcmp(type, "v3f")) {
//         const Imf::V3fAttribute* attr = static_cast<const Imf::V3fAttribute*>(&i.attribute());
//         float values[3] = {
//         attr->value().x, attr->value().y, attr->value().z
//         };
//         _meta.setData(key, values, 3);
//         }
//         else if (!strcmp(type, "box2f")) {
//         const Imf::Box2fAttribute* attr = static_cast<const Imf::Box2fAttribute*>(&i.attribute());
//         float values[4] = {
//         attr->value().min.x, attr->value().min.y, attr->value().max.x, attr->value().max.y
//         };
//         _meta.setData(key, values, 4);
//         }
//         else if (!strcmp(type, "m33f")) {
//         const Imf::M33fAttribute* attr = static_cast<const Imf::M33fAttribute*>(&i.attribute());
//         std::vector<float> values;
//         for (int i = 0; i < 3; ++i) {
//         for (int j = 0; j < 3; ++j) {
//         values.push_back((attr->value())[i][j]);
//         }
//         }
//         _meta.setData(key, values);
//         }
//         else if (!strcmp(type, "m44f")) {
//         const Imf::M44fAttribute* attr = static_cast<const Imf::M44fAttribute*>(&i.attribute());
//         std::vector<float> values;
//         for (int i = 0; i < 4; ++i) {
//         for (int j = 0; j < 4; ++j) {
//         values.push_back((attr->value())[i][j]);
//         }
//         }
//         _meta.setData(key, values);
//         }
//         else if (!strcmp(type, "timecode")) {
//         const Imf::TimeCodeAttribute* attr = static_cast<const Imf::TimeCodeAttribute*>(&i.attribute());
//         char timecode[20];
//         sprintf(timecode, "%02i:%02i:%02i:%02i", attr->value().hours(), attr->value().minutes(), attr->value().seconds(), attr->value().frame());
//         _meta.setData(key, timecode);
//         }
//         else if (!strcmp(type, "keycode")) {
//         const Imf::KeyCodeAttribute* attr = static_cast<const Imf::KeyCodeAttribute*>(&i.attribute());
//         char keycode[30];
//         sprintf(keycode, "%02i %02i %06i %04i %02i",
//         attr->value().filmMfcCode(),
//         attr->value().filmType(),
//         attr->value().prefix(),
//         attr->value().count(),
//         attr->value().perfOffset());
//         _meta.setData(key, keycode);
//         }
//         else if (!strcmp(type, "rational")) {
//         const Imf::RationalAttribute* attr = static_cast<const Imf::RationalAttribute*>(&i.attribute());
//         _meta.setData(key, (double)attr->value());
//         //    } else {
//         //      _meta.setData(key, "type " + std::string(type));
//         }*/
//    }
//


