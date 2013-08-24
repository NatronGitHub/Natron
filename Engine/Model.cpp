//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */



#include <QtCore/QMutex>
#include <QtCore/QDir>
#include <cassert>
#include <cstdio>
#include <fstream>
#include "Engine/Model.h"
#include "Global/Controler.h"
#include "Engine/Hash.h"
#include "Engine/Node.h"
#include "Engine/ChannelSet.h"
#include "Readers/Reader.h"
#include "Writers/Writer.h"
#include "Engine/OfxNode.h"
#include "Engine/ViewerNode.h"
#include "Gui/Gui.h"
#include "Gui/ViewerGL.h"
#include "Gui/TabWidget.h"
#include "Engine/VideoEngine.h"
#include "Engine/Format.h"
#include "Engine/Settings.h"
#include "Writers/Write.h"
#include "Readers/Read.h"
#include "Readers/ReadExr.h"
#include "Readers/ReadFfmpeg_deprecated.h"
#include "Readers/ReadQt.h"
#include "Engine/Lut.h"
#include "Engine/NodeCache.h"
#include "Engine/ViewerCache.h"
#include "Gui/Knob.h"
#include "Writers/WriteQt.h"
#include "Writers/WriteExr.h"
#include "Gui/Gui.h"
#include "Gui/NodeGui.h"
#include "Gui/Edge.h"
#include <cassert>


#include <fstream>

// ofx
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxPixels.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhPluginAPICache.h"
#include "ofxhImageEffectAPI.h"
#include "ofxhHost.h"




using namespace std;
using namespace Powiter;
Model::Model():OFX::Host::ImageEffect::Host(), _videoEngine(0), _imageEffectPluginCache(*this)
{
    
    /*node cache initialisation & restoration*/
    _nodeCache = NodeCache::getNodeCache();
    U64 nodeCacheMaxSize = (Settings::getPowiterCurrentSettings()->_cacheSettings.maxCacheMemoryPercent-
                            Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent)*
    getSystemTotalRAM();
    _nodeCache->setMaximumCacheSize(nodeCacheMaxSize);
    
    
    /*viewer cache initialisation & restoration*/
    _viewerCache = ViewerCache::getViewerCache();
    _viewerCache->setMaximumCacheSize((U64)((double)Settings::getPowiterCurrentSettings()->_cacheSettings.maxDiskCache));
    _viewerCache->setMaximumInMemorySize(Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent);
    _viewerCache->restore();
    
    /*loading all plugins*/
    loadAllPlugins();
    
    _knobFactory = KnobFactory::instance();
    
    
    /*allocating lookup tables*/
    Lut::allocateLuts();
    
    _videoEngine = new VideoEngine(this);
    connect(this,SIGNAL(vengineNeeded(int)),_videoEngine,SLOT(startEngine(int)));
    
    /*initializing list of all Formats available*/
    std::vector<std::string> formatNames;
    formatNames.push_back("PC_Video");
    formatNames.push_back("NTSC");
    formatNames.push_back("PAL");
    formatNames.push_back("HD");
    formatNames.push_back("NTSC_16:9");
    formatNames.push_back("PAL_16:9");
    formatNames.push_back("1K_Super_35(full-ap)");
    formatNames.push_back("1K_Cinemascope");
    formatNames.push_back("2K_Super_35(full-ap)");
    formatNames.push_back("2K_Cinemascope");
    formatNames.push_back("4K_Super_35(full-ap)");
    formatNames.push_back("4K_Cinemascope");
    formatNames.push_back("square_256");
    formatNames.push_back("square_512");
    formatNames.push_back("square_1K");
    formatNames.push_back("square_2K");
    
    std::vector< std::vector<float> > resolutions;
    std::vector<float> pcvideo; pcvideo.push_back(640); pcvideo.push_back(480); pcvideo.push_back(1);
    std::vector<float> ntsc; ntsc.push_back(720); ntsc.push_back(486); ntsc.push_back(0.91f);
    std::vector<float> pal; pal.push_back(720); pal.push_back(576); pal.push_back(1.09f);
    std::vector<float> hd; hd.push_back(1920); hd.push_back(1080); hd.push_back(1);
    std::vector<float> ntsc169; ntsc169.push_back(720); ntsc169.push_back(486); ntsc169.push_back(1.21f);
    std::vector<float> pal169; pal169.push_back(720); pal169.push_back(576); pal169.push_back(1.46f);
    std::vector<float> super351k; super351k.push_back(1024); super351k.push_back(778); super351k.push_back(1);
    std::vector<float> cine1k; cine1k.push_back(914); cine1k.push_back(778); cine1k.push_back(2);
    std::vector<float> super352k; super352k.push_back(2048); super352k.push_back(1556); super352k.push_back(1);
    std::vector<float> cine2K; cine2K.push_back(1828); cine2K.push_back(1556); cine2K.push_back(2);
    std::vector<float> super4K35; super4K35.push_back(4096); super4K35.push_back(3112); super4K35.push_back(1);
    std::vector<float> cine4K; cine4K.push_back(3656); cine4K.push_back(3112); cine4K.push_back(2);
    std::vector<float> square256; square256.push_back(256); square256.push_back(256); square256.push_back(1);
    std::vector<float> square512; square512.push_back(512); square512.push_back(512); square512.push_back(1);
    std::vector<float> square1K; square1K.push_back(1024); square1K.push_back(1024); square1K.push_back(1);
    std::vector<float> square2K; square2K.push_back(2048); square2K.push_back(2048); square2K.push_back(1);
    
    resolutions.push_back(pcvideo);
    resolutions.push_back(ntsc);
    resolutions.push_back(pal);
    resolutions.push_back(hd);
    resolutions.push_back(ntsc169);
    resolutions.push_back(pal169);
    resolutions.push_back(super351k);
    resolutions.push_back(cine1k);
    resolutions.push_back(super352k);
    resolutions.push_back(cine2K);
    resolutions.push_back(super4K35);
    resolutions.push_back(cine4K);
    resolutions.push_back(square256);
    resolutions.push_back(square512);
    resolutions.push_back(square1K);
    resolutions.push_back(square2K);
    
    assert(formatNames.size() == resolutions.size());
    for(U32 i =0;i<formatNames.size();++i) {
        const std::vector<float>& v = resolutions[i];
        assert(v.size() >= 3);
        Format* _frmt = new Format(0,0,v[0],v[1],formatNames[i],v[2]);
        assert(_frmt);
        addFormat(_frmt);
    }
    
    
    _properties.setStringProperty(kOfxPropName, "PowiterHost");
    _properties.setStringProperty(kOfxPropLabel, "Powiter");
    _properties.setIntProperty(kOfxImageEffectHostPropIsBackground, 0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsOverlays, 0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultiResolution, 1);
    _properties.setIntProperty(kOfxImageEffectPropSupportsTiles, 1);
    _properties.setIntProperty(kOfxImageEffectPropTemporalClipAccess, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentRGBA, 0);
    _properties.setStringProperty(kOfxImageEffectPropSupportedComponents,  kOfxImageComponentAlpha, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGenerator, 0 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextFilter, 1);
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextGeneral, 2 );
    _properties.setStringProperty(kOfxImageEffectPropSupportedContexts, kOfxImageEffectContextTransition, 3 );
    
    _properties.setStringProperty(kOfxImageEffectPropSupportedPixelDepths,kOfxBitDepthFloat,0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipDepths, 0);
    _properties.setIntProperty(kOfxImageEffectPropSupportsMultipleClipPARs, 0);
    _properties.setIntProperty(kOfxImageEffectPropSetableFrameRate, 0);
    _properties.setIntProperty(kOfxImageEffectPropSetableFielding, 0);
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomInteract, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsStringAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsChoiceAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsBooleanAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropSupportsCustomAnimation, 0 );
    _properties.setIntProperty(kOfxParamHostPropMaxParameters, -1);
    _properties.setIntProperty(kOfxParamHostPropMaxPages, 0);
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 0 );
    _properties.setIntProperty(kOfxParamHostPropPageRowColumnCount, 0, 1 );
    
    
}



Model::~Model(){
    
    _viewerCache->save();
    Lut::deallocateLuts();
    _videoEngine->abort();
    
    writeOFXCache();
    
    // foreach(PluginID* p,_pluginsLoaded) delete p;
    foreach(CounterID* c,_nodeCounters) delete c;
    foreach(Format* f,_formats) delete f;
    for(ReadPluginsIterator it = _readPluginsLoaded.begin(); it!=_readPluginsLoaded.end(); ++it) {
        if(it->second){
            /*finding all other reads that have the same pointer to avoid double free*/
            for(ReadPluginsIterator it2 = _readPluginsLoaded.begin(); it2!=_readPluginsLoaded.end(); ++it2) {
                if(it2->second == it->second && it2->first!=it->first)
                    it2->second = 0;
            }
            delete it->second;
            it->second = 0;
        }
    }
    _nodeCounters.clear();
    delete _videoEngine;
    _videoEngine = 0;
    _currentNodes.clear();
    _formats.clear();
    _nodeNames.clear();
}

void Model::loadAllPlugins(){
    /*loading node plugins*/
    //  loadPluginsAndInitNameList();
    loadBuiltinPlugins();
    
    /*loading read plugins*/
    loadReadPlugins();
    
    /*loading write plugins*/
    loadWritePlugins();
    
    /*loading ofx plugins*/
    loadOFXPlugins();
}




std::pair<int,bool> Model::setVideoEngineRequirements(Node *output,bool isViewer){
    _videoEngine->resetAndMakeNewDag(output,isViewer);
    
    const std::vector<Node*>& inputs = _videoEngine->getCurrentDAG().getInputs();
    bool hasFrames = false;
    bool hasInputDifferentThanReader = false;
    for (U32 i = 0; i< inputs.size(); ++i) {
        assert(inputs[i]);
        Reader* r = dynamic_cast<Reader*>(inputs[i]);
        if (r) {
            if (r->hasFrames()) {
                hasFrames = true;
            }
        }else{
            hasInputDifferentThanReader = true;
        }
    }
    return make_pair(inputs.size(),hasFrames || hasInputDifferentThanReader);
}

void Model::initCounterAndGetDescription(Node*& node){
    assert(node);
    bool found=false;
    foreach(CounterID* counter,_nodeCounters){
        assert(counter);
        string tmp(counter->second);
        string nodeName = node->className();
        if(tmp==nodeName){
            ++(counter->first);
            found=true;
            string str;
            str.append(nodeName.c_str());
            str.append("_");
            char c[50];
            sprintf(c,"%d",counter->first);
            str.append(c);
            node->setName(str);
        }
    }
    if(!found){
        CounterID* count=new CounterID(1,node->className());
        assert(count);
        _nodeCounters.push_back(count);
        string str;
        str.append(node->className().c_str());
        str.append("_");
        char c[50];
        sprintf(c,"%d",count->first);
        str.append(c);
        node->setName(str);
    }
    
    /*adding nodes to the current nodes and
     adding its cache to the watched caches.*/
    _currentNodes.push_back(node);
    
}

bool Model::createNode(Node *&node,const std::string name){
	if(name=="Reader"){
		node=new Reader();
        assert(node);
        node->initializeInputs();
		initCounterAndGetDescription(node);
        return true;
	}else if(name =="Viewer"){
        node=new ViewerNode(_viewerCache);
        assert(node);
        node->initializeInputs();
		initCounterAndGetDescription(node);
        TabWidget* where = ctrlPTR->getGui()->_nextViewerTabPlace;
        if(!where){
            where = ctrlPTR->getGui()->_viewersPane;
        }else{
            ctrlPTR->getGui()->setNewViewerAnchor(NULL); // < reseting anchor to default
        }
        dynamic_cast<ViewerNode*>(node)->initializeViewerTab(where);
        return true;
	}else if(name == "Writer"){
		node=new Writer();
        assert(node);
        node->initializeInputs();
		initCounterAndGetDescription(node);
        return true;
    }else{
        
        OFXPluginsIterator ofxPlugin = _ofxPlugins.find(name);
        if(ofxPlugin != _ofxPlugins.end()){
            OFX::Host::ImageEffect::ImageEffectPlugin* plugin = _imageEffectPluginCache.getPluginById(ofxPlugin->second.first);
            if(plugin){
                const std::set<std::string>& contexts = plugin->getContexts();
                set<string>::iterator found = contexts.find(kOfxImageEffectContextGenerator);
                string context;
                if(found != contexts.end()){
                    context = *found;
                }else{
                    found = contexts.find(kOfxImageEffectContextFilter);
                    if(found != contexts.end()){
                        context = *found;
                    }else{
                        found = contexts.find(kOfxImageEffectContextTransition);
                        if(found != contexts.end()){
                            context = *found;
                        }else{
                            found = contexts.find(kOfxImageEffectContextPaint);
                            if(found != contexts.end()){
                                context = *found;
                            }else{
                                context = kOfxImageEffectContextGeneral;
                            }
                        }
                    }
                }
                bool rval ;
                try{
                    rval = plugin->getPluginHandle();
                }
                catch(const char* str){
                    cout << str << endl;
                }
                if(!rval)
                    return false;
                node = dynamic_cast<Node*>(plugin->createInstance(context, NULL));
                if(node){
                    node->initializeInputs();
                    initCounterAndGetDescription(node);
                    OFX::Host::ImageEffect::Instance* ofxInstance = dynamic_cast<OFX::Host::ImageEffect::Instance*>(node);
                    assert(ofxInstance);
                    ofxInstance->createInstanceAction();
                    ofxInstance->getClipPreferences();//not sure we should do this here
                    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*> clips = ofxInstance->getDescriptor().getClipsByOrder();
                    
                    
                    return true;
                }
            }
        }
        return false;
	}
    
}
// in the future, display the plugins loaded on the loading wallpaper
void Model::displayLoadedPlugins(){
    int i=0;
    for(OFXPluginsIterator it = _ofxPlugins.begin() ; it != _ofxPlugins.end() ; ++it) {
        cout << it->first << endl;
    }
    cout  << i << " plugin(s) loaded." << endl;
}


void Model::addFormat(Format* frmt){_formats.push_back(frmt);}

Format* Model::findExistingFormat(int w, int h, double pixel_aspect){
    
	for(U32 i =0;i< _formats.size();++i) {
		Format* frmt = _formats[i];
        assert(frmt);
		if(frmt->w() == w && frmt->h() == h && frmt->pixel_aspect()==pixel_aspect){
			return frmt;
		}
	}
	return NULL;
}


void Model::loadReadPlugins(){
    QDir d(PLUGINS_PATH);
    if (d.isReadable())
    {
        QStringList filters;
#ifdef __POWITER_WIN32__
        filters << "*.dll";
#elif defined(__POWITER_OSX__)
        filters << "*.dylib";
#elif defined(__POWITER_LINUX__)
        filters << "*.so";
#endif
        d.setNameFilters(filters);
		QStringList fileList = d.entryList();
        for(int i = 0 ; i < fileList.size() ; ++i)
        {
            QString filename = fileList.at(i);
            if(filename.contains(".dll") || filename.contains(".dylib") || filename.contains(".so")){
                QString className;
                int index = filename.size() -1;
                while(filename.at(index) != QChar('.')) {
                    --index;
                }
                className = filename.left(index);
                PluginID* plugin = 0;
#ifdef __POWITER_WIN32__
                HINSTANCE lib;
                string dll;
                dll.append(PLUGINS_PATH);
                dll.append(className.toStdString());
                dll.append(".dll");
                lib=LoadLibrary(dll.c_str());
                if(lib==NULL){
                    cout << " couldn't open library " << qPrintable(className) << endl;
                }else{
                    // successfully loaded the library, we create now an instance of the class
                    //to find out the extensions it can decode and the name of the decoder
                    ReadBuilder builder=(ReadBuilder)GetProcAddress(lib,"BuildRead");
                    if(builder!=NULL){
                        Read* read=builder(NULL);
                        assert(read);
                        std::vector<std::string> extensions = read->fileTypesDecoded();
                        std::string decoderName = read->decoderName();
                        plugin = new PluginID((HINSTANCE)builder,decoderName.c_str());
                        assert(plugin);
                        for (U32 i = 0 ; i < extensions.size(); ++i) {
                            _readPluginsLoaded.push_back(make_pair(extensions[i],plugin));
                        }
                        delete read;
                        
                    }else{
                        cout << "RunTime: couldn't call " << "BuildRead" << endl;
                        continue;
                    }
                    
                }
                
#elif defined(__POWITER_UNIX__)
                string dll;
                dll.append(PLUGINS_PATH);
                dll.append(className.toStdString());
#ifdef __POWITER_OSX__
                dll.append(".dylib");
#elif defined(__POWITER_LINUX__)
                dll.append(".so");
#endif
                void* lib=dlopen(dll.c_str(),RTLD_LAZY);
                if(!lib){
                    cout << " couldn't open library " << qPrintable(className) << endl;
                }
                else{
                    // successfully loaded the library, we create now an instance of the class
                    //to find out the extensions it can decode and the name of the decoder
                    ReadBuilder builder=(ReadBuilder)dlsym(lib,"BuildRead");
                    if(builder!=NULL){
                        Read* read=builder(NULL);
                        assert(read);
                        std::vector<std::string> extensions = read->fileTypesDecoded();
                        std::string decoderName = read->decoderName();
                        plugin = new PluginID((void*)builder,decoderName.c_str());
                        assert(plugin);
                        for (U32 i = 0 ; i < extensions.size(); ++i) {
                            _readPluginsLoaded.push_back(make_pair(extensions[i],plugin));
                        }
                        delete read;
                        
                    }else{
                        cout << "RunTime: couldn't call " << "BuildRead" << endl;
                        continue;
                    }
                }
#endif
            }else{
                continue;
            }
        }
    }
    loadBuiltinReads();
    
    std::map<std::string, PluginID*> defaultMapping;
    for (ReadPluginsIterator it = _readPluginsLoaded.begin(); it!=_readPluginsLoaded.end(); ++it) {
        if(it->first == "exr" && it->second->second == "OpenEXR"){
            defaultMapping.insert(*it);
        }else if (it->first == "dpx" && it->second->second == "FFmpeg"){
            defaultMapping.insert(*it);
        }else if((it->first == "jpg" ||
                  it->first == "bmp" ||
                  it->first == "jpeg"||
                  it->first == "gif" ||
                  it->first == "png" ||
                  it->first == "pbm" ||
                  it->first == "pgm" ||
                  it->first == "ppm" ||
                  it->first == "xbm" ||
                  it->first == "xpm") && it->second->second == "QImage (Qt)"){
            defaultMapping.insert(*it);
            
        }
    }
    Settings::getPowiterCurrentSettings()->_readersSettings.fillMap(defaultMapping);
}

void Model::displayLoadedReads(){
    ReadPluginsIterator it = _readPluginsLoaded.begin();
    for (; it!=_readPluginsLoaded.end(); ++it) {
        cout << it->second->second << " : " << it->first << endl;
    }
}

void Model::loadBuiltinReads(){
    Read* readExr = ReadExr::BuildRead(NULL);
    assert(readExr);
    std::vector<std::string> extensions = readExr->fileTypesDecoded();
    std::string decoderName = readExr->decoderName();
#ifdef __POWITER_WIN32__
    PluginID *EXRplugin = new PluginID((HINSTANCE)&ReadExr::BuildRead,decoderName.c_str());
#else
    PluginID *EXRplugin = new PluginID((void*)&ReadExr::BuildRead,decoderName.c_str());
#endif
    assert(EXRplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.push_back(make_pair(extensions[i],EXRplugin));
    }
    delete readExr;
    
    Read* readQt = ReadQt::BuildRead(NULL);
    assert(readQt);
    extensions = readQt->fileTypesDecoded();
    decoderName = readQt->decoderName();
#ifdef __POWITER_WIN32__
	PluginID *Qtplugin = new PluginID((HINSTANCE)&ReadQt::BuildRead,decoderName.c_str());
#else
	PluginID *Qtplugin = new PluginID((void*)&ReadQt::BuildRead,decoderName.c_str());
#endif
    assert(Qtplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.push_back(make_pair(extensions[i],Qtplugin));
    }
    delete readQt;
    
    Read* readFfmpeg = ReadFFMPEG::BuildRead(NULL);
    assert(readFfmpeg);
    extensions = readFfmpeg->fileTypesDecoded();
    decoderName = readFfmpeg->decoderName();
#ifdef __POWITER_WIN32__
	PluginID *FFMPEGplugin = new PluginID((HINSTANCE)&ReadFFMPEG::BuildRead,decoderName.c_str());
#else
	PluginID *FFMPEGplugin = new PluginID((void*)&ReadFFMPEG::BuildRead,decoderName.c_str());
#endif
    assert(FFMPEGplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.push_back(make_pair(extensions[i],FFMPEGplugin));
    }
    delete readFfmpeg;
    
}
void Model::loadBuiltinPlugins(){
    // these  are built-in nodes
	_nodeNames.append("Reader");
	_nodeNames.append("Viewer");
    _nodeNames.append("Writer");
}

/*loads extra writer plug-ins*/
void Model::loadWritePlugins(){
    QDir d(PLUGINS_PATH);
    if (d.isReadable())
    {
        QStringList filters;
#ifdef __POWITER_WIN32__
        filters << "*.dll";
#elif defined(__POWITER_OSX__)
        filters << "*.dylib";
#elif defined(__POWITER_LINUX__)
        filters << "*.so";
#endif
        d.setNameFilters(filters);
		QStringList fileList = d.entryList();
        for(int i = 0 ; i < fileList.size() ; ++i)
        {
            QString filename = fileList.at(i);
            if(filename.contains(".dll") || filename.contains(".dylib") || filename.contains(".so")){
                QString className;
                int index = filename.size() -1;
                while(filename.at(index) != QChar('.')) {
                    --index;
                }
                className = filename.left(index);
                PluginID* plugin = 0;
#ifdef __POWITER_WIN32__
                HINSTANCE lib;
                string dll;
                dll.append(PLUGINS_PATH);
                dll.append(className.toStdString());
                dll.append(".dll");
                lib=LoadLibrary(dll.c_str());
                if(lib==NULL){
                    cout << " couldn't open library " << qPrintable(className) << endl;
                }else{
                    // successfully loaded the library, we create now an instance of the class
                    //to find out the extensions it can decode and the name of the decoder
                    WriteBuilder builder=(WriteBuilder)GetProcAddress(lib,"BuildRead");
                    if(builder!=NULL){
                        Write* write=builder(NULL);
                        assert(write);
                        std::vector<std::string> extensions = write->fileTypesEncoded();
                        std::string encoderName = write->encoderName();
                        plugin = new PluginID((HINSTANCE)builder,encoderName.c_str());
                        for (U32 i = 0 ; i < extensions.size(); ++i) {
                            _writePluginsLoaded.push_back(make_pair(extensions[i],plugin));
                        }
                        delete write;
                        
                    }else{
                        cout << "RunTime: couldn't call " << "BuildRead" << endl;
                        continue;
                    }
                    
                }
                
#elif defined(__POWITER_UNIX__)
                string dll;
                dll.append(PLUGINS_PATH);
                dll.append(className.toStdString());
#ifdef __POWITER_OSX__
                dll.append(".dylib");
#elif defined(__POWITER_LINUX__)
                dll.append(".so");
#endif
                void* lib=dlopen(dll.c_str(),RTLD_LAZY);
                if(!lib){
                    cout << " couldn't open library " << qPrintable(className) << endl;
                }
                else{
                    // successfully loaded the library, we create now an instance of the class
                    //to find out the extensions it can decode and the name of the decoder
                    WriteBuilder builder=(WriteBuilder)dlsym(lib,"BuildRead");
                    if(builder!=NULL){
                        Write* write=builder(NULL);
                        assert(write);
                        std::vector<std::string> extensions = write->fileTypesEncoded();
                        std::string encoderName = write->encoderName();
                        plugin = new PluginID((void*)builder,encoderName.c_str());
                        for (U32 i = 0 ; i < extensions.size(); ++i) {
                            _readPluginsLoaded.push_back(make_pair(extensions[i],plugin));
                        }
                        delete write;
                        
                    }else{
                        cout << "RunTime: couldn't call " << "BuildRead" << endl;
                        continue;
                    }
                }
#endif
            }else{
                continue;
            }
        }
    }
    loadBuiltinWrites();
    
    std::map<std::string, PluginID*> defaultMapping;
    for (WritePluginsIterator it = _writePluginsLoaded.begin(); it!=_writePluginsLoaded.end(); ++it) {
        if(it->first == "exr" && it->second->second == "OpenEXR"){
            defaultMapping.insert(*it);
        }else if (it->first == "dpx" && it->second->second == "FFmpeg"){
            defaultMapping.insert(*it);
        }else if((it->first == "jpg" ||
                  it->first == "bmp" ||
                  it->first == "jpeg"||
                  it->first == "gif" ||
                  it->first == "png" ||
                  it->first == "pbm" ||
                  it->first == "pgm" ||
                  it->first == "ppm" ||
                  it->first == "xbm" ||
                  it->first == "xpm") && it->second->second == "QImage (Qt)"){
            defaultMapping.insert(*it);
            
        }
    }
    Settings::getPowiterCurrentSettings()->_writersSettings.fillMap(defaultMapping);
}

/*loads writes that are built-ins*/
void Model::loadBuiltinWrites(){
    Write* writeQt = WriteQt::BuildWrite(NULL);
    assert(writeQt);
    std::vector<std::string> extensions = writeQt->fileTypesEncoded();
    string encoderName = writeQt->encoderName();
#ifdef __POWITER_WIN32__
	PluginID *QtWritePlugin = new PluginID((HINSTANCE)&WriteQt::BuildWrite,encoderName.c_str());
#else
	PluginID *QtWritePlugin = new PluginID((void*)&WriteQt::BuildWrite,encoderName.c_str());
#endif
    assert(QtWritePlugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _writePluginsLoaded.push_back(make_pair(extensions[i],QtWritePlugin));
    }
    delete writeQt;
    
    Write* writeEXR = WriteExr::BuildWrite(NULL);
    std::vector<std::string> extensionsExr = writeEXR->fileTypesEncoded();
    string encoderNameExr = writeEXR->encoderName();
#ifdef __POWITER_WIN32__
	PluginID *ExrWritePlugin = new PluginID((HINSTANCE)&WriteExr::BuildWrite,encoderNameExr.c_str());
#else
	PluginID *ExrWritePlugin = new PluginID((void*)&WriteExr::BuildWrite,encoderNameExr.c_str());
#endif
    assert(ExrWritePlugin);
    for (U32 i = 0 ; i < extensionsExr.size(); ++i) {
        _writePluginsLoaded.push_back(make_pair(extensionsExr[i],ExrWritePlugin));
    }
    delete writeEXR;
}

void Model::clearPlaybackCache(){
    _viewerCache->clearInMemoryPortion();
}


void Model::clearDiskCache(){
    _viewerCache->clearInMemoryPortion();
    _viewerCache->clearDiskCache();
}


void  Model::clearNodeCache(){
    _nodeCache->clear();
}


void Model::removeNode(Node* n){
    assert(n);
    /*We DON'T delete as it was already done by the NodeGui associated.*/
    for(U32 i = 0 ; i < _currentNodes.size();++i) {
        if(_currentNodes[i] == n){
            _currentNodes.erase(_currentNodes.begin()+i);
        }
    }
}

void Model::resetInternalDAG(){
    if(_videoEngine){
        _videoEngine->resetDAG();
    }
}



void Model::loadOFXPlugins(){
    assert(OFX::Host::PluginCache::getPluginCache());
    /// set the version label in the global cache
    OFX::Host::PluginCache::getPluginCache()->setCacheVersion("PowiterOFXCachev1");
    
    /// make an image effect plugin cache
    
    /// register the image effect cache with the global plugin cache
    _imageEffectPluginCache.registerInCache(*OFX::Host::PluginCache::getPluginCache());
    
    
#if defined(WINDOWS)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("C:\\Program Files\\Common Files\\OFX\\Nuke");
#endif
#if defined(__linux__)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("/usr/OFX/Nuke");
#endif
#if defined(__APPLE__)
    OFX::Host::PluginCache::getPluginCache()->addFileToPath("/Library/OFX/Nuke");
#endif
    
    /// now read an old cache cache
    std::ifstream ifs("PowiterOFXCache.xml");
    OFX::Host::PluginCache::getPluginCache()->readCache(ifs);
    OFX::Host::PluginCache::getPluginCache()->scanPluginFiles();
    ifs.close();
    
    //  _imageEffectPluginCache.dumpToStdOut();
    
    /*Filling node name list and plugin grouping*/
    const std::vector<OFX::Host::ImageEffect::ImageEffectPlugin *>& plugins = _imageEffectPluginCache.getPlugins();
    for (unsigned int i = 0 ; i < plugins.size(); ++i) {
        OFX::Host::ImageEffect::ImageEffectPlugin* p = plugins[i];
        assert(p);
        if(p->getContexts().size() == 0)
            continue;
        std::string name = p->getDescriptor().getProps().getStringProperty(kOfxPropShortLabel);
        if(name.empty()){
            name = p->getDescriptor().getProps().getStringProperty(kOfxPropLabel);
        }
        std::string rawName = name;
        std::string id = p->getIdentifier();
        std::string grouping = p->getDescriptor().getPluginGrouping();
        vector<string> groups = extractAllPartsOfGrouping(grouping);
        if (groups.size() >= 1) {
            name.append("  [");
            name.append(groups[0]);
            name.append("]");
        }
        assert(p->getBinary());
        std::string iconFilename = p->getBinary()->getBundlePath() + "/Contents/Resources/";
        iconFilename.append(p->getDescriptor().getProps().getStringProperty(kOfxPropIcon,1));
        iconFilename.append(id);
        iconFilename.append(".png");
        std::string groupIconFilename;
        if (groups.size() >= 1) {
            groupIconFilename = p->getBinary()->getBundlePath() + "/Contents/Resources/";
            groupIconFilename.append(p->getDescriptor().getProps().getStringProperty(kOfxPropIcon,1));
            groupIconFilename.append(groups[0]);
            groupIconFilename.append(".png");
        }
        ctrlPTR->stackPluginToolButtons(groups,rawName,iconFilename,groupIconFilename);
        _ofxPlugins.insert(make_pair(name, make_pair(id, grouping)));
        _nodeNames.append(name.c_str());
    }
}
void Model::writeOFXCache(){
    /// and write a new cache, long version with everything in there
    std::ofstream of("PowiterOFXCache.xml");
    assert(OFX::Host::PluginCache::getPluginCache());
    OFX::Host::PluginCache::getPluginCache()->writePluginCache(of);
    of.close();
    //Clean up, to be polite.
    OFX::Host::PluginCache::clearPluginCache();
}

OFX::Host::ImageEffect::Instance* Model::newInstance(void* clientData,
                                                     OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                     OFX::Host::ImageEffect::Descriptor& desc,
                                                     const std::string& context)
{
    assert(plugin);
    return new OfxNode(plugin, desc, context);
}

/// Override this to create a descriptor, this makes the 'root' descriptor
OFX::Host::ImageEffect::Descriptor *Model::makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin* plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(plugin);
    return desc;
}

/// used to construct a context description, rootContext is the main context
OFX::Host::ImageEffect::Descriptor *Model::makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                                                          OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(rootContext, plugin);
    return desc;
}

/// used to construct populate the cache
OFX::Host::ImageEffect::Descriptor *Model::makeDescriptor(const std::string &bundlePath,
                                                          OFX::Host::ImageEffect::ImageEffectPlugin *plugin)
{
    assert(plugin);
    OFX::Host::ImageEffect::Descriptor *desc = new OFX::Host::ImageEffect::Descriptor(bundlePath, plugin);
    return desc;
}

/// message
OfxStatus Model::vmessage(const char* type,
                          const char* ,
                          const char* format,
                          va_list args)
{
    assert(type);
    assert(format);
    bool isQuestion = false;
    const char *prefix = "Message : ";
    if (strcmp(type, kOfxMessageLog) == 0) {
        prefix = "Log : ";
    }
    else if(strcmp(type, kOfxMessageFatal) == 0 ||
            strcmp(type, kOfxMessageError) == 0) {
        prefix = "Error : ";
    }
    else if(strcmp(type, kOfxMessageQuestion) == 0)  {
        prefix = "Question : ";
        isQuestion = true;
    }
    
    // Just dump our message to stdout, should be done with a proper
    // UI in a full ap, and post a dialogue for yes/no questions.
    fputs(prefix, stdout);
    vprintf(format, args);
    printf("\n");
    
    if(isQuestion) {
        /// cant do this properly inour example, as we need to raise a dialogue to ask a question, so just return yes
        return kOfxStatReplyYes;
    }
    else {
        return kOfxStatOK;
    }
}

QString Model::serializeNodeGraph() const{
    const std::vector<NodeGui*>& activeNodes = ctrlPTR->getAllActiveNodes();
    QString ret;
    foreach(NodeGui* n, activeNodes){
        //serialize inputs
        ret.append("Node:");
        assert(n);
        assert(n->getNode());
        if(!n->getNode()->isOpenFXNode()){
            ret.append(n->getNode()->className().c_str());
        }else{
            OfxNode* ofxNode = dynamic_cast<OfxNode*>(n->getNode());
            std::string name = ofxNode->getShortLabel();
            std::string grouping = ofxNode->getPluginGrouping();
            vector<string> groups = extractAllPartsOfGrouping(grouping);
            if (groups.size() >= 1) {
                name.append("  [");
                name.append(groups[0]);
                name.append("]");
            }
            ret.append(name.c_str());
        }
        ret.append(":");
        ret.append(n->getNode()->getName().c_str());
        ret.append("{\n");
        const std::vector<Node*>& parents = n->getNode()->getParents();
        for (U32 i = 0; i < parents.size(); ++i) {
            ret.append("input");
            ret.append(QString::number(i));
            ret.append(":");
            ret.append(parents[i]->getName().c_str());
            ret.append("\n");
        }
        //serialize knobs
        const std::vector<Knob*>& knobs = n->getNode()->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            ret.append("knob");
            ret.append(":");
            ret.append(knobs[i]->getDescription().c_str());
            ret.append(":");
            ret.append(knobs[i]->serialize().c_str());
            ret.append("\n");
        }
        
        //serialize gui infos
        ret.append("pos:");
        ret.append("x=");
        ret.append(QString::number(n->pos().x()));
        ret.append("y=");
        ret.append(QString::number(n->pos().y()));
        ret.append("\n");
        
        //closing brace
        ret.append("}\n");
    }
    return ret;
}

void Model::restoreGraphFromString(const QString& str){
    int i = 0,lastNode = 0;
    std::vector<std::pair<Node*,QString> > actionsMap;
    i = str.indexOf("Node:",lastNode);
    while(i != -1){
        lastNode = i+1;
        
        i += 5;
        QString className;
        while(i < str.size() && str.at(i) != QChar(':')){
            className.append(str.at(i));
            ++i;
        }
        if(i  == str.size()) return; // safety check
        
        ++i;
        QString nodeName;
        while(i < str.size() && str.at(i) != QChar('{')){
            nodeName.append(str.at(i));
            ++i;
        }
        if(i  == str.size()) return; // safety check
        
        ++i; //the '\n' character 
        
        assert(ctrlPTR);
        ctrlPTR->deselectAllNodes();
        Node* n = ctrlPTR->createNode(className);
        if(!n){
            QString text("Failed to restore the graph! \n The node ");
            text.append(className);
            text.append(" was found in the auto-save script but doesn't seem \n"
                        "to exist in the currently loaded plug-ins.");
            ctrlPTR->showErrorDialog("Autosave", text );
            ctrlPTR->clearInternalNodes();
            ctrlPTR->clearNodeGuis();
            ctrlPTR->createNode("Viewer");
            return;
        }

        n->getNodeUi()->setName(nodeName);
        while (i < str.size() && str.at(i) != QChar('}')) {
            QString line;
            while(i < str.size() && str.at(i) != QChar('\n')){
                line.append(str.at(i));
                ++i;
            }
            ++i; // the '\n' character
            actionsMap.push_back(make_pair(n, line));
        }
        i = str.indexOf("Node:",lastNode);
    }
    
    //adjusting knobs & connecting nodes now
    for (U32 i = 0; i < actionsMap.size(); ++i) {
        pair<Node*,QString>& action = actionsMap[i];
        analyseSerializedNodeString(action.first, action.second);
    }
    
}
void Model::analyseSerializedNodeString(Node* n,const QString& str){
    assert(n);
    int type = 0;
    type = str.indexOf("input");
    if(type != -1){
        int i = type + 5;
        QString inputNumberStr;
        while(i < str.size() && str.at(i) != QChar(':')){
            inputNumberStr.append(str.at(i));
            ++i;
        }
        ++i; // the ':' character
        QString inputName;
        while(i < str.size()){
            inputName.append(str.at(i));
            ++i;
        }

        assert(n->getNodeUi());
        int inputNb = inputNumberStr.toInt();
        for (U32 j = 0; j < _currentNodes.size(); ++j) {
            assert(_currentNodes[j]);
            if (_currentNodes[j]->getName() == inputName.toStdString()) {
                n->addParent(_currentNodes[j]);
                _currentNodes[j]->addChild(n);
                n->getNodeUi()->addParent(_currentNodes[j]->getNodeUi());
                _currentNodes[j]->getNodeUi()->addChild(n->getNodeUi());
                
                const std::vector<Edge*>& edges = n->getNodeUi()->getInputsArrows();
                assert(inputNb < (int)edges.size());
                assert(edges[inputNb]);
                edges[inputNb]->setSource(_currentNodes[j]->getNodeUi());
                edges[inputNb]->initLine();
                break;
            }
        }
        return;
    }
    type = str.indexOf("knob");
    if(type != -1){
        int i = type + 4;
        
        ++i; // the ':' character
               
        QString knobDescription;
        while(i < str.size() && str.at(i)!= QChar(':')){
            knobDescription.append(str.at(i));
            ++i;
        }
        
        if(i  == str.size()) return; // safety check
        
        ++i; // the ':' character
        
        QString value;
        while(i < str.size()){
            value.append(str.at(i));
            ++i;
        }
        
        const std::vector<Knob*>& knobs = n->getKnobs();
        for (U32 j = 0; j < knobs.size(); ++j) {
            if (knobs[j]->getDescription() == knobDescription.toStdString()) {
                knobs[j]->restoreFromString(value.toStdString());
                break;
            }
        }
        
        return;
    }
    
    type = str.indexOf("pos");
    if(type != -1){
        int i = type + 3;
        ++i;// the ':' character
        ++i;// the 'x' character
        ++i;// the '=' character
        
        QString xStr,yStr;
        while(i < str.size() && str.at(i)!=QChar('y')){
            xStr.append(str.at(i));
            ++i;
        }
        
        if(i  == str.size()) return; // safety check
        
        ++i; // the 'y' character
        ++i; // the '=' character
        
        while(i < str.size()){
            yStr.append(str.at(i));
            ++i;
        }
        
        assert(n->getNodeUi());
        n->getNodeUi()->setPos(xStr.toDouble(), yStr.toDouble());
        
        const vector<NodeGui*>& children = n->getNodeUi()->getChildren();
        const vector<Edge*>& edges = n->getNodeUi()->getInputsArrows();
        foreach(NodeGui* c,children){
            const vector<Edge*>& childEdges = c->getInputsArrows();
            foreach(Edge* e,childEdges){
                e->initLine();
            }
        }
        foreach(Edge* e,edges){
            e->initLine();
        }
        
        return;
    }
    
}

void Model::loadProject(const QString& filename){
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    restoreGraphFromString(in.readAll());
    file.close();
}
void Model::saveProject(const QString& filename){
    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QTextStream out(&file);
    out << serializeNodeGraph();
    file.close();
}

void Model::clearNodes(){
    foreach(CounterID* n,_nodeCounters){
        delete n;
    }
    _nodeCounters.clear();
    _currentNodes.clear();
}