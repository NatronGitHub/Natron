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

#include "ExrDecoder.h"

#ifdef __NATRON_WIN32__
#include <fstream>
#endif
#include <QtGui/QImage>
#include <QtCore/QByteArray>
#include <QtCore/QMutex>
#include <ImfPixelType.h>
#include <ImfChannelList.h>
#include <ImfInputFile.h>
#ifdef __NATRON_WIN32__
#include <ImfStdIO.h>
#endif

#include "Readers/Reader.h"
#include "Engine/Node.h"
#include "Engine/Lut.h"
#include "Engine/Row.h"
#include "Engine/ImageInfo.h"
#include "Engine/Image.h"

#ifndef OPENEXR_IMF_NAMESPACE
#define OPENEXR_IMF_NAMESPACE Imf
#endif
namespace Imf_ = OPENEXR_IMF_NAMESPACE;

using std::cout; using std::endl;

struct ExrDecoder::Implementation {
    Implementation();

    Imf::InputFile* inputfile;
    std::map<Natron::Channel, std::string> channel_map;
    std::vector<std::string> views;
    int dataOffset;
#ifdef __NATRON_WIN32__
    std::ifstream* inputStr;
    Imf::StdIFStream* inputStdStream;
#endif
	QMutex _lock;
};


namespace EXR {
static Natron::Channel fromExrChannel(const std::string& from)
{
    if (from == "R" ||
        from == "r" ||
        from == "Red" ||
        from == "RED" ||
        from == "red" ||
        from == "y" ||
        from == "Y") {
        return Natron::Channel_red;
    }
    if (from == "G" ||
        from == "g" ||
        from == "Green" ||
        from == "GREEN" ||
        from == "green" ||
        from == "ry" ||
        from == "RY") {
        return Natron::Channel_green;
    }
    if (from == "B" ||
        from == "b" ||
        from == "Blue" ||
        from == "BLUE" ||
        from == "blue" ||
        from == "by" ||
        from == "BY") {
        return Natron::Channel_blue;
    }
    if (from == "A" ||
        from == "a" ||
        from == "Alpha" ||
        from == "ALPHA" ||
        from == "alpha") {
        return Natron::Channel_alpha;
    }
    if (from == "Z" ||
        from == "z" ||
        from == "Depth" ||
        from == "DEPTH" ||
        from == "depth") {
        return Natron::Channel_Z;
    }
    // The following may throw if from is not a channel name which begins with "Channel_"
    return Natron::getChannelByName(from);
}

class ChannelExtractor
{
public:
    ChannelExtractor(const std::string& name, const std::vector<std::string>& views) :
    _mappedChannel(Natron::Channel_black),
    _valid(false)
    {     _valid = extractExrChannelName(name.c_str(), views);  }

    ~ChannelExtractor() {}

    Natron::Channel _mappedChannel;
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
        
        try {
            _mappedChannel = EXR::fromExrChannel(_chan);
        } catch (const std::exception& e) {
            qDebug() << "Error while reading EXR file" << ": " << e.what();
            return false;
        } catch (...) {
            qDebug() << "Error while reading EXR file";
            return false;
        }
        return true;
    }
};


} // namespace EXR




Natron::Status ExrDecoder::readHeader(const QString& filename){
    
    _imp->channel_map.clear();
    _imp->views.clear();
    try {
#ifdef __NATRON_WIN32__
        QByteArray ba = filename.toLocal8Bit();
        if(_imp->inputStr){
            delete _imp->inputStr;
        }
        if(_imp->inputStdStream){
            delete _imp->inputStdStream;
        }
        if(_imp->_inputfile){
            delete _imp->_inputfile;
        }
        _imp->inputStr = new std::ifstream(NatronWindows::s2ws(ba.data()),std::ios_base::binary);
        _imp->inputStdStream = new Imf_::StdIFStream(*inputStr,ba.data());
        _imp->_inputfile = new Imf_::InputFile(*inputStdStream);
#else
        QByteArray ba = filename.toLatin1();
        if(_imp->inputfile){
            delete _imp->inputfile;
        }
        _imp->inputfile = new Imf_::InputFile(ba.constData());
#endif

        // multiview is only supported with OpenEXR >= 1.7.0
#ifdef INCLUDED_IMF_STRINGVECTOR_ATTRIBUTE_H // use ImfStringVectorAttribute.h's #include guard
        const Imf_::StringAttribute* stringMultiView = 0;
        const Imf_::StringVectorAttribute* vectorMultiView = 0;
        
        try {
            vectorMultiView = inputfile->header().findTypedAttribute<Imf_::StringVectorAttribute>("multiView");
            
            if (!vectorMultiView) {
                Imf_::Header::ConstIterator it = inputfile->header().find("multiView");
                if (it != inputfile->header().end() && !strcmp(it.attribute().typeName(), "stringvector"))
                    vectorMultiView = static_cast<const Imf_::StringVectorAttribute*>(&it.attribute());
            }
            
            stringMultiView = inputfile->header().findTypedAttribute<Imf_::StringAttribute>("multiView");
        }
        catch (...) {
            return StatFailed;
        }
        
        if (vectorMultiView) {
            std::vector<std::string> s = vectorMultiView->value();
            
            bool setHero = false;
            
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i].length()) {
                    _views.push_back(s[i]);
                    if (!setHero) {
                        heroview = s[i];
                        setHero = true;
                    }
                }
            }
        }
#endif // !OPENEXR_NO_MULTIVIEW
        std::map<Imf_::PixelType, int> pixelTypes;
        // convert exr channels to powiter channels
        Natron::ChannelSet mask;
        const Imf_::ChannelList& imfchannels = _imp->inputfile->header().channels();
        Imf_::ChannelList::ConstIterator chan;
        for (chan = imfchannels.begin(); chan != imfchannels.end(); ++chan) {
            std::string chanName(chan.name());
            if(chanName.empty())
                continue;
            pixelTypes[chan.channel().type]++;
            EXR::ChannelExtractor exrExctractor(chan.name(), _imp->views);
            std::set<Natron::Channel> channels;
            if (exrExctractor.isValid()) {
                channels.insert(exrExctractor._mappedChannel);
                //cout << "size : "<< channels.size() << endl;
                for (std::set<Natron::Channel>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
                    Natron::Channel channel = *it;
                    //cout <<" channel_map[" << Natron::getChannelName(channel) << "] = " << chan.name() << endl;
                    bool writeChannelMapping = true;
                    ChannelsMap::const_iterator found = _imp->channel_map.find(channel);
                    if(found != _imp->channel_map.end()){
                        int existingLength = found->second.size();
                        int newLength = chanName.size();
                        if ((existingLength > 0) && found->second.at(0) == '.' && existingLength == (newLength + 1)) {                                writeChannelMapping = true;
                        }
                        else if (existingLength > newLength) {
                            writeChannelMapping = false;
                        }
                    }
                    if(writeChannelMapping){
                        _imp->channel_map.insert(make_pair(channel,chanName));
                    }
                    mask += channel;
                }
                
            }else {
                cout << "Cannot decode channel " << chan.name() << endl;
            }
            
        }
        
        const Imath::Box2i& datawin = _imp->inputfile->header().dataWindow();
        const Imath::Box2i& dispwin = _imp->inputfile->header().displayWindow();
        Imath::Box2i formatwin(dispwin);
        formatwin.min.x = 0;
        formatwin.min.y = 0;
        _imp->dataOffset = 0;
        if (dispwin.min.x != 0) {
            // Shift both to get dispwindow over to 0,0.
            _imp->dataOffset = -dispwin.min.x;
            formatwin.max.x = dispwin.max.x + _imp->dataOffset;
        }
        formatwin.max.y = dispwin.max.y - dispwin.min.y;
        double aspect = _imp->inputfile->header().pixelAspectRatio();
        Format imageFormat(0,0,formatwin.max.x + 1 ,formatwin.max.y + 1,"",aspect);
        RectI rod;
        
        int left = datawin.min.x + _imp->dataOffset;
        int bottom = dispwin.max.y - datawin.max.y;
        int right = datawin.max.x + _imp->dataOffset;
        int top = dispwin.max.y - datawin.min.y;
        if (datawin.min.x != dispwin.min.x || datawin.max.x != dispwin.max.x ||
                datawin.min.y != dispwin.min.y || datawin.max.y != dispwin.max.y) {
            --left;
            --bottom;
            ++right;
            ++top;
        }
        rod.set(left, bottom, right+1, top+1);
        
        setReaderInfo(imageFormat, rod, mask);
        return Natron::StatOK;
    } catch (const std::exception& e) {
        qDebug() << "OpenEXR error" << ": " << e.what();
        delete _imp->inputfile;
        _imp->inputfile = 0;
        return Natron::StatFailed;
    } catch (...) {
        qDebug() << "OpenEXR error";
        delete _imp->inputfile;
        _imp->inputfile = 0;
        return Natron::StatFailed;
    }
}


ExrDecoder::ExrDecoder(Reader* op)
: Decoder(op)
, _imp(new Implementation)
{
}

ExrDecoder::Implementation::Implementation()
: inputfile(0)
, dataOffset(0)
#ifdef __NATRON_WIN32__
, inputStr(NULL)
, inputStdStream(NULL)
#endif
{
}

void ExrDecoder::initializeColorSpace(){
    _lut=Natron::Color::getLut(Natron::Color::LUT_DEFAULT_FLOAT); // linear color-space for exr files
}

ExrDecoder::~ExrDecoder(){
#ifdef __NATRON_WIN32__
    delete _imp->inputStr ;
    delete _imp->inputStdStream ;
#endif
    delete _imp->inputfile;
}

Natron::Status ExrDecoder::render(SequenceTime /*time*/,RenderScale /*scale*/,const RectI& roi,boost::shared_ptr<Natron::Image> output){
    for (int y = roi.bottom(); y < roi.top(); ++y) {
        Natron::ChannelSet channels(Natron::Mask_RGBA);
        Natron::Row row(output->getRoD().left(),y,output->getRoD().right(),channels);
        const Imath::Box2i& dispwin = _imp->inputfile->header().displayWindow();
        const Imath::Box2i& datawin = _imp->inputfile->header().dataWindow();
        int exrY = dispwin.max.y - row.y();
        int r = row.right();
        int x = row.left();
        
        const int X = std::max(x, datawin.min.x + _imp->dataOffset);
        const int R = std::min(r, datawin.max.x + _imp->dataOffset +1);
        
        // if we're below or above the data window
        if(exrY < datawin.min.y || exrY > datawin.max.y || R <= X) {
            row.eraseAll();
            return Natron::StatOK;
        }
        
        Imf_::FrameBuffer fbuf;
        foreachChannels(z, channels){
            // blacking out the extra padding we added
            float* dest = row.begin(z) - row.left();
            for (int xx = x; xx < X; xx++)
                dest[xx] = 0;
            for (int xx = R; xx < r; xx++)
                dest[xx] = 0;
            ChannelsMap::const_iterator found = _imp->channel_map.find(z);
            if(found != _imp->channel_map.end()){
                if(found->second != "BY" && found->second != "RY"){ // if it is NOT a subsampled buffer
                    fbuf.insert(found->second.c_str(),Imf_::Slice(Imf_::FLOAT, (char*)(dest /*+_imp->_dataOffset*/),sizeof(float), 0));
                }else{
                    fbuf.insert(found->second.c_str(),Imf_::Slice(Imf_::FLOAT, (char*)(dest /*+_imp->_dataOffset*/),sizeof(float), 0,2,2));
                }
            }else{
                //not found in the file, we zero it out
                // we're responsible for filling all channels requested by the row
                float fillValue = 0.f;
                if (z == Natron::Channel_alpha) {
                    fillValue = 1.f;
                }
                row.fill(z,fillValue);
            }
        }
        {
            QMutexLocker locker(&_imp->_lock);
            try {
                _imp->inputfile->setFrameBuffer(fbuf);
                _imp->inputfile->readPixels(exrY);
            } catch (const std::exception& e) {
                _reader->setPersistentMessage(Natron::ERROR_MESSAGE, std::string("OpenEXR error") + ": " + e.what());
                return Natron::StatFailed;
            } catch (...) {
                _reader->setPersistentMessage(Natron::ERROR_MESSAGE, std::string("OpenEXR error"));
                return Natron::StatFailed;
            }
        }
        
        //  colorspace conversion
        const float* alpha = row.begin(Natron::Channel_alpha);
        foreachChannels(z, channels){
            float* to = row.begin(z) - row.left();
            const float* from = row.begin(z) - row.left();
            if(from){
                from_float(z,to + X ,from + X,alpha, R-X,1);
            }
        }
        Natron::copyRowToImage(row, y, row.left(), output.get());
    }
    return Natron::StatOK;
}
