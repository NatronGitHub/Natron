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
} // namespace EXR

static float clamp(float v, float min = 0.f, float max= 1.f){
    if(v > max) v = max;
    if(v < min) v = min;
    return v;
}


static std::vector<std::string> split(const std::string& str, char splitChar)
{
    std::vector<std::string> ret;
    size_t i = str.find(splitChar);
    size_t offset = 0;
    while ( i != str.npos ){
        ret.push_back(str.substr(offset, i - offset));
        offset = i + 1;
        i = str.find(splitChar, offset);
        
        // stop once we've found two options
        if ( ret.size() == 2 )
            break;
    }
    ret.push_back(str.substr(offset));
    
    return ret;
}

static bool IsView(const std::string& name, const std::vector<std::string>& views)
{
    for ( size_t i = 0; i < views.size(); ++i ){
        if ( views[i] == name ){
            return true;
        }
    }
    
    return false;
}
static std::string removedigitsfromfront(const std::string& str)
{
    std::string ret = "";
    size_t len = str.length();
    size_t i = 0;
    while (isdigit(str[i]) && (i < len))
        ++i;
    for (; i < len; ++i )
        ret += (str[i]);
    
    return ret;
}

static std::string removeNonAlphaCharacters(const std::string& str)
{
    std::string ret = "";
    size_t len = str.length();
    for ( size_t i = 0; i < len; ++i ){
        if (!isalnum(str[i]))
            ret += '_';
        else
            ret += str[i];
    }
    
    return ret;
}

bool ExrChannelExctractor::extractExrChannelName(const char* channelname, const std::vector<std::string>& views)
{
    _chan.clear();
    _layer.clear();
    _view.clear();
    
    // split
    std::vector<std::string> splits = split(channelname, '.');
    
    // remove digits from the front, and remove empty strings
    std::vector<std::string> newsplits;
    for ( size_t i = 0; i < splits.size(); ++i ){
        std::string s = removedigitsfromfront(splits[i]);
        if ( s.length() > 0 )
            newsplits.push_back(removeNonAlphaCharacters(s));
    }
    
    // get the names out
    //size_t offset = 0;
    if ( newsplits.size() > 1 ){

        
        for (U32 i = 0 ; i < (newsplits.size() - 1); ++i) {
            if (IsView(newsplits[i], views)) {
                _view = newsplits[i];
            } else {
                if (!_layer.empty())
                    _layer += "_";
                _layer += newsplits[i];
            }
        }
        
        _chan = newsplits.back();
    } else {
        _chan = newsplits[0];
    }
    
    // std::string viewpart = (channelName._view.length() > 0) ? channelName._view : heroview;

     //if (viewname(view) == viewpart) {
     //is_stereo= true;
     Channel chan;
     try{
         chan = EXR::fromExrChannel(_chan);
     } catch (const std::exception &e) {
         std::cout << e.what() << endl;
         return false;
     }
     _mappedChannel = chan;
     return true;
     //}

     //    if (viewpart != "" && viewpart != heroview) {
     //        return false;
     //    }

     //    exrToPowiterChannelMapping(channel, otherpart.c_str());

     //    return true;

    //	std::cout << "Turned '"<<channelname<<"' into "<<layer<<"."<<chan<<std::endl;
    
}

std::string ExrChannelExctractor::exrName() const
{
    if (!_layer.empty())
        return _layer + "." + _chan;
    return _chan;
}
void ReadExr::readHeader(const QString& filename, bool){
    
    channel_map.clear();
    views.clear();
    std::map<Imf::PixelType, int> pixelTypes;
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
        if(inputfile){
            delete inputfile;
        }
        inputfile = new Imf::InputFile(ba.constData());
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
        catch (...) { // FIXME: wow wow wow why ignore all exceptions and continue?
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

        // handling channels in the exr file to convert them to powiter channels
        ChannelSet mask;
        bool rgb= true;
        const Imf::ChannelList& imfchannels = inputfile->header().channels();
        Imf::ChannelList::ConstIterator chan;
        
        for (chan = imfchannels.begin(); chan != imfchannels.end(); chan++) {
            if(!strcmp(chan.name(),"")) continue;
            pixelTypes[chan.channel().type]++;
            ExrChannelExctractor exrExctractor(chan.name(), views);
            std::set<Channel> channels;
            if (exrExctractor.isValid()) {
                channels.insert(exrExctractor._mappedChannel);
                //cout << "size : "<< channels.size() << endl;
                if (!channels.empty()) {
                    for (std::set<Channel>::iterator it = channels.begin();
                         it != channels.end();
                         ++it) {
                        Channel channel = *it;
                        //cout <<" channel_map[" << getChannelName(channel) << "] = " << chan.name() << endl;
                        bool writeChannelMapping = true;
                        if(channel_map[channel]){
                            int existingLength = strlen(channel_map[channel]);
                            int newLength = strlen(chan.name());
                            bool existingChannelHasEmptyLayerName = (existingLength > 0) && channel_map[channel][0] == '.';
                            if (existingChannelHasEmptyLayerName && existingLength == (newLength + 1)) {                                writeChannelMapping = true;
                            }
                            else if (existingLength > newLength) {
                                
                                writeChannelMapping = false;
                            }
                        }
                        if(writeChannelMapping){
                            channel_map[channel] = chan.name();
                        }
                        if(!strcmp(chan.name(),"Y") || !strcmp(chan.name(),"BY") || !strcmp(chan.name(),"RY") ) rgb=false;
                        mask += channel;
                    }
                }
                else {
                    //iop->warning("Cannot assign channel number to %s", cName.name().c_str());
                }
            }
        }

        const Imath::Box2i& datawin = inputfile->header().dataWindow();
        const Imath::Box2i& dispwin = inputfile->header().displayWindow();
        Imath::Box2i formatwin(dispwin);
        formatwin.min.x = 0;
        formatwin.min.y = 0;
        dataOffset = 0;
        if (dispwin.min.x != 0) {
            // Shift both to get dispwindow over to 0,0.
            dataOffset = -dispwin.min.x;
            formatwin.max.x = dispwin.max.x + dataOffset;
        }
        formatwin.max.y = dispwin.max.y - dispwin.min.y;
        double aspect = inputfile->header().pixelAspectRatio();
        Format imageFormat(0,0,formatwin.max.x + 1 ,formatwin.max.y + 1,"",aspect);
        Box2D bbox;
        
        int bx = datawin.min.x + dataOffset;
        int by = dispwin.max.y - datawin.max.y;
        int br = datawin.max.x + dataOffset;
        int bt = dispwin.max.y - datawin.min.y;
        if (datawin.min.x != dispwin.min.x || datawin.max.x != dispwin.max.x ||
                datawin.min.y != dispwin.min.y || datawin.max.y != dispwin.max.y) {
            bx--;
            by--;
            br++;
            bt++;
            _readerInfo.set_blackOutside(true);
        }
        bbox.set(bx, by, br+1, bt+1);
        
        int ydirection = -1;
        if (inputfile->header().lineOrder() != Imf::INCREASING_Y){
            ydirection=1;
        }
        set_readerInfo(imageFormat, bbox, filename, mask, ydirection, rgb);
    }
    catch (const std::exception& exc) {
        //iop->error(exc.what());
        cout << "OpenExr error: " << exc.what() << endl;
        delete inputfile;
    }
}

void ReadExr::readScanLine(int y){
    const Imath::Box2i& dispwin = inputfile->header().displayWindow();
    const Imath::Box2i& datawin = inputfile->header().dataWindow();
    
    // Invert to EXR y coordinate:
    int exrY = dispwin.max.y - y;
    const Box2D& bbox = readerInfo().dataWindow();
    // const Format& dispW = readerInfo().displayWindow();
    int r = bbox.right();
    //bbox.right() > dispW.right() ? r = bbox.right() : r = dispW.right();
    int x= bbox.left();
    //bbox.x() < dispW.x() ? x = bbox.x() : x = dispW.x();
    const ChannelSet& channels = readerInfo().channels();
    Row* out = new Row(x,y,r,channels);
    out->allocateRow();
    _img.insert(make_pair(exrY,out));
    // Figure out intersection of x,r with the data in exr file:
    const int X = max(x, datawin.min.x + dataOffset);
    const int R = min(r, datawin.max.x + dataOffset +1);
    
    // Black outside the bbox:
    if(exrY < datawin.min.y || exrY > datawin.max.y || R <= X) {
        out->erase(channels);
        return;
    }
    Imf::FrameBuffer fbuf;
    foreachChannels(z, channels){
        // blacking out what needs to be blacked out
        float* dest = out->writable(z) ;
        for (int xx = x; xx < X; xx++)
            dest[xx] = 0;
        for (int xx = R; xx < r; xx++)
            dest[xx] = 0;
        if(strcmp(channel_map[z],"BY") && strcmp(channel_map[z],"RY")){ // if it is NOT a subsampled buffer
            fbuf.insert(channel_map[z],Imf::Slice(Imf::FLOAT, (char*)(dest + dataOffset),sizeof(float), 0));
        }else{
            fbuf.insert(channel_map[z],Imf::Slice(Imf::FLOAT, (char*)(dest + dataOffset),sizeof(float), 0,2,2));
        }
    }
    {
        try {
            inputfile->setFrameBuffer(fbuf);
            inputfile->readPixels(exrY);
        }
        catch (const std::exception& exc) {
            cout << " ERROR READING PIXELS FROM FILE : " << exc.what() <<  endl;
            //iop->error(exc.what());
        }
    }

}

ReadExr::ReadExr(Reader* op):Read(op),inputfile(0),dataOffset(0){
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
    for(map<int,Row*>::iterator it =_img.begin(); it!= _img.end(); ++it) {
        delete it->second;
    }
    _img.clear();
    delete inputfile;
}

void ReadExr::engine(int y,int offset,int range,ChannelSet channels,Row* out){
    const Imath::Box2i& dispwin = inputfile->header().displayWindow();
    const Imath::Box2i& datawin = inputfile->header().dataWindow();
    // Invert to EXR y coordinate:
    int exrY = dispwin.max.y - y;
    
    // Figure out intersection of x,r with the data in exr file:
    const int X = max(offset, datawin.min.x + dataOffset);
    const int R = min(range, datawin.max.x + dataOffset +1);
    
    Row* from = 0;
    map<int,Row*>::iterator it = _img.find(exrY);
    if(it == _img.end()){
        //cout << "couldn't read: " << exrY << endl;
        return;
    }
    from = it->second;
    if(autoAlpha()){
        out->turnOn(Channel_alpha);
    }
    //  colorspace conversion
    const float* alpha = (*from)[Channel_alpha];
    if(alpha) alpha+= X;
    foreachChannels(z, channels){
        float* to = out->writable(z);
        const float* in = (*from)[z];
        if(in){
            from_float(z,to + X ,in + X,alpha, R-X,1);
        }
    }
}


void ReadExr::make_preview(){
    Imf::Header header = inputfile->header();
    Imath::Box2i &dataWindow = header.dataWindow();
    Imath::Box2i &dispWindow = header.displayWindow();
    int dh = dataWindow.max.y - dataWindow.min.y + 1;
    int dw = dataWindow.max.x - dataWindow.min.x + 1;
    int h,w;
    dh < 64 ? h = dh : h = 64;
    dw < 64 ? w = dw : w = 64;
    float zoomFactor = (float)h/(float)dh;
    for (int i =0 ; i < h; ++i) {
        float y = (float)i*1.f/zoomFactor;
        int nearestY = (int)(y+0.5);
        readScanLine(nearestY);
    }
    QImage img(w,h,QImage::Format_ARGB32);
    for (int i=0; i< h; ++i) {
        double y = i*1.f/zoomFactor;
        int nearestY = (int)(y+0.5);
        int exrY = dispWindow.max.y+1 - nearestY;
        if(exrY < dispWindow.min.y || exrY > dispWindow.max.y) continue;
        Row* from = _img[exrY];
        QRgb *dst_pixels = (QRgb *) img.scanLine(h-1-i);
        const float* red = (*from)[Channel_red];
        const float* green = (*from)[Channel_green];
        const float* blue = (*from)[Channel_blue] ;
        const float* alpha = (*from)[Channel_alpha] ;
        if(red) red+=from->offset();
        if(green) green+=from->offset();
        if(blue) blue+=from->offset();
        if(alpha) alpha+=from->offset();
        for (int j=0; j<w; ++j) {
            double x = j*1.f/zoomFactor;
            int nearestX = (int)(x+0.5);
            float r = red ? clamp(Color::linearrgb_to_srgb(red[nearestX])) : 0.f;
            float g = green ? clamp(Color::linearrgb_to_srgb(green[nearestX])) : 0.f;
            float b = blue ? clamp(Color::linearrgb_to_srgb(blue[nearestX])) : 0.f;
            float a = alpha ? alpha[nearestX] : 1.f;
            QColor c(r*255,g*255,b*255,a*255);
            dst_pixels[j] = qRgba(r*255,g*255,b*255,a*255);
        }
    }
    op->setPreview(img);
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
void ReadExr::debug(){
    int w = _img.begin()->second->right() - _img.begin()->second->offset();
    int h =_img.size();
    map<int,Row*>::iterator it = _img.begin();
    QImage img(w,h,QImage::Format_ARGB32);
    for (int i =0; i < h; ++i) {
        Row* row = it->second;
        const float* r = (*row)[Channel_red] + row->offset();
        const float* g = (*row)[Channel_green] + row->offset();
        const float* b = (*row)[Channel_blue] + row->offset();
        for (int j = 0; j < w; ++j) {
            QColor c(clamp(*r++)*255,clamp(*g++)*255,clamp(*b++)*255);
            img.setPixel(j, i, c.rgb());
        }
        ++it;
    }
    img.save("debug.jpg");
}

