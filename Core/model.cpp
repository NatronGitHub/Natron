//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <QtCore/QMutex>
#include <QtCore/QDir>
#include <cassert>
#include <cstdio>
#include "Core/model.h"
#include "Core/outputnode.h"
#include "Core/inputnode.h"
#include "Superviser/controler.h"
#include "Core/hash.h"
#include "Core/node.h"
#include "Core/channels.h"
#include "Reader/Reader.h"
#include "Core/viewerNode.h"
#include "Gui/mainGui.h"
#include "Core/inputnode.h"
#include "Gui/GLViewer.h"
#include "Core/VideoEngine.h"
#include "Core/displayFormat.h"
#include "Core/settings.h"
#include "Reader/Read.h"
#include "Reader/readExr.h"
#include "Reader/readffmpeg.h"
#include "Reader/readQt.h"
#include "Core/lookUpTables.h"
#include "Core/nodecache.h"
#include "Core/viewercache.h"
#include <cassert>
using namespace std;
Model::Model(): _videoEngine(0),_mutex(0)
{
    /*general mutex shared by all nodes*/
    _mutex = new QMutex;

    /*node cache initialisation & restoration*/
    _nodeCache = NodeCache::getNodeCache();
    _nodeCache->setMaximumCacheSize((U64)((double)Settings::getPowiterCurrentSettings()->_cacheSettings.maxDiskCache*2.0/3.0));
    _nodeCache->setMaximumInMemorySize(Settings::getPowiterCurrentSettings()->_cacheSettings.maxCacheMemoryPercent-
                                       Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent);
    _nodeCache->restore();
    
    
    /*viewer cache initialisation & restoration*/
    _viewerCache = ViewerCache::getViewerCache();
    _viewerCache->setMaximumCacheSize((U64)((double)Settings::getPowiterCurrentSettings()->_cacheSettings.maxDiskCache*1.0/3.0));
    _viewerCache->setMaximumInMemorySize(Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent);
    _viewerCache->restore();
    
    /*loading node plugins*/
    loadPluginsAndInitNameList();
    loadBuiltinPlugins();
    
    /*loading read plugins*/
    loadReadPlugins();
    displayLoadedPlugins();
    
    /*allocating lookup tables*/
    Lut::allocateLuts();
    
    _videoEngine = new VideoEngine(this,_mutex);
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
    for(int i =0;i<formatNames.size();i++){
        std::vector<float> v = resolutions[i];
        Format* _frmt = new Format(0,0,v[0],v[1],formatNames[i],v[2]);
        addFormat(_frmt);
    }
    

}



Model::~Model(){
    _nodeCache->save();
    _viewerCache->save();
    Lut::deallocateLuts();
    _videoEngine->abort();
    foreach(PluginID* p,_pluginsLoaded) delete p;
    foreach(CounterID* c,_nodeCounters) delete c;
    foreach(Format* f,_formats) delete f;
    for(ReadPluginsIterator it = _readPluginsLoaded.begin();it!=_readPluginsLoaded.end();it++){
        if(it->second){
            /*finding all other reads that have the same pointer to avoid double free*/
            for(ReadPluginsIterator it2 = _readPluginsLoaded.begin();it2!=_readPluginsLoaded.end();it2++){
                if(it2->second == it->second && it2->first!=it->first)
                    it2->second = 0;
            }
            delete it->second;
            it->second = 0;
        }
    }
    _readPluginsLoaded.clear();
    _pluginsLoaded.clear();
    _nodeCounters.clear();
    delete _videoEngine;
    _videoEngine = 0;
	foreach(Node* n,_currentNodes) delete n;
    _currentNodes.clear();
    _formats.clear();
    _nodeNames.clear();
    delete _mutex;
}



void Model::loadPluginsAndInitNameList(){ // parses Powiter directory to find classes who inherit Node and adds them to the nodeList
    QDir d(QString(PLUGINS_PATH));
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
		for(int i = 0 ; i < fileList.size() ;i ++)
		{
			QString filename = fileList.at(i);
			if(filename.contains(".dll") || filename.contains(".dylib") || filename.contains(".so")){
				QString className;
				int index = filename.size() -1;
				while(filename.at(index) != QChar('.')) index--;
				className = filename.left(index);
				_nodeNames.append(QString(className));

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
					NodeBuilder builder=(NodeBuilder)GetProcAddress(lib,"BuildNode");
					if(builder){
						Node* test = builder(NULL);
						PluginID* plugin=new PluginID((HINSTANCE)builder,test->getName().toStdString());
						delete test;
						_pluginsLoaded.push_back(plugin);
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
					NodeBuilder builder=(NodeBuilder)dlsym(lib,"BuildNode");
					if(builder){
						Node* test = builder(NULL);
						PluginID* plugin=new PluginID((void*)builder,test->getName().toStdString());
						_pluginsLoaded.push_back(plugin);
						delete test;
					}
				}
#endif
			}else{
                continue;
            }
        }
    }
}

std::string Model::getNextWord(string str){
    string res;
    int i=0;
    while(i!=str.size() && str[i]==' '){
        i++;
    }
    while(i!=str.size() && str[i]!=' ' && str[i]!=':'){
        res.push_back(str[i]);
        i++;
    }
    return res;
}

std::string Model::removePrefixSpaces(std::string str){
    string res;
    int i=0;
    while(i!=str.size() && str[i]==' '){
        i++;
    }
    res=str.substr(i,str.size());
    return res;
    
}



void Model::setVideoEngineRequirements(OutputNode *output){
    _videoEngine->getCurrentDAG().resetAndSort(output);
    _videoEngine->changeTreeVersion();
}

UI_NODE_TYPE Model::initCounterAndGetDescription(Node*& node){
    bool found=false;
    foreach(CounterID* counter,_nodeCounters){
        string tmp(counter->second);
        string nodeName = node->className();
        if(tmp==nodeName){
            (counter->first)++;
            found=true;
            QString str;
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
        
        _nodeCounters.push_back(count);
        QString str;
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
    
    string outputNodeSymbol="OutputNode";
    string inputNodeSymbol="InputNode";
    string flowOpSymbol="FlowOperator";
    string imgOperatorSymbol="ImgOperator";
    string desc =node->description();
    if(desc==outputNodeSymbol){
        return OUTPUT;
    }
    else if(desc==inputNodeSymbol){
		return INPUT_NODE;
    }else if(desc==imgOperatorSymbol || desc==flowOpSymbol){
        return OPERATOR;
    }
    return UNDEFINED;
}

UI_NODE_TYPE Model::createNode(Node *&node,QString& name,QMutex* m){
	if(name=="Reader"){
		UI_NODE_TYPE type;
		node=new Reader(node,_videoEngine->getViewerCache());
        node->setMutex(m);
        node->initializeInputs();
        node->setSocketCount();
		type=initCounterAndGetDescription(node);
		return type;
	}else if(name =="Viewer"){
		UI_NODE_TYPE type;
		node=new Viewer(node);
        node->setMutex(m);
        node->initializeInputs();
        node->setSocketCount();
		type=initCounterAndGetDescription(node);
		return type;
	}else{
        
        UI_NODE_TYPE type=UNDEFINED;
        
		foreach(PluginID* pair,_pluginsLoaded){
			string str(pair->second);
            
			if(str==name.toStdString()){
				

				NodeBuilder builder=(NodeBuilder)pair->first;
				if(builder!=NULL){
					node=builder(node);
                    node->setMutex(m);
                    node->initializeInputs();
                    node->setSocketCount();
					type=initCounterAndGetDescription(node);
                    
				}     
				return type;
			}
            
		}
	}
    return UNDEFINED;
    
    
    
}
// in the future, display the plugins loaded on the loading wallpaper
void Model::displayLoadedPlugins(){
    int i=0;
    foreach(PluginID* plugin,_pluginsLoaded){
        i++;
        cout << "Plugin:  " << plugin->second << endl;
    }
    cout  << i << " plugin(s) loaded." << endl;
}


void Model::addFormat(Format* frmt){_formats.push_back(frmt);}

Format* Model::findExistingFormat(int w, int h, double pixel_aspect){

	for(int i =0;i< _formats.size();i++){
		Format* frmt = _formats[i];
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
        for(int i = 0 ; i < fileList.size() ;i ++)
        {
            QString filename = fileList.at(i);
            if(filename.contains(".dll") || filename.contains(".dylib") || filename.contains(".so")){
                QString className;
                int index = filename.size() -1;
                while(filename.at(index) != QChar('.')) index--;
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
                        std::vector<std::string> extensions = read->fileTypesDecoded();
                        std::string decoderName = read->decoderName();
                        plugin = new PluginID((HINSTANCE)builder,decoderName.c_str());
                        for (U32 i = 0 ; i < extensions.size(); i++) {
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
                        std::vector<std::string> extensions = read->fileTypesDecoded();
                        std::string decoderName = read->decoderName();
                        plugin = new PluginID((void*)builder,decoderName.c_str());
                        for (U32 i = 0 ; i < extensions.size(); i++) {
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
    for (ReadPluginsIterator it = _readPluginsLoaded.begin(); it!=_readPluginsLoaded.end(); it++) {
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
    for (; it!=_readPluginsLoaded.end(); it++) {
        cout << it->second->second << " : " << it->first << endl;
    }
}

void Model::loadBuiltinReads(){
    Read* readExr = ReadExr::BuildRead(NULL);
    std::vector<std::string> extensions = readExr->fileTypesDecoded();
    std::string decoderName = readExr->decoderName();
#ifdef __POWITER_WIN32__
	 PluginID *EXRplugin = new PluginID((HINSTANCE)&ReadExr::BuildRead,decoderName.c_str());
#else
	 PluginID *EXRplugin = new PluginID((void*)&ReadExr::BuildRead,decoderName.c_str());
#endif
   
    for (U32 i = 0 ; i < extensions.size(); i++) {
        _readPluginsLoaded.push_back(make_pair(extensions[i],EXRplugin));
    }
    delete readExr;
    
    Read* readQt = ReadQt::BuildRead(NULL);
    extensions = readQt->fileTypesDecoded();
    decoderName = readQt->decoderName();
#ifdef __POWITER_WIN32__
	PluginID *Qtplugin = new PluginID((HINSTANCE)&ReadQt::BuildRead,decoderName.c_str());
#else
	PluginID *Qtplugin = new PluginID((void*)&ReadQt::BuildRead,decoderName.c_str());
#endif
    for (U32 i = 0 ; i < extensions.size(); i++) {
        _readPluginsLoaded.push_back(make_pair(extensions[i],Qtplugin));
    }
    delete readQt;
    
    Read* readFfmpeg = ReadFFMPEG::BuildRead(NULL);
    extensions = readFfmpeg->fileTypesDecoded();
    decoderName = readFfmpeg->decoderName();
#ifdef __POWITER_WIN32__
	PluginID *FFMPEGplugin = new PluginID((HINSTANCE)&ReadFFMPEG::BuildRead,decoderName.c_str());
#else
	PluginID *FFMPEGplugin = new PluginID((void*)&ReadFFMPEG::BuildRead,decoderName.c_str());
#endif
    for (U32 i = 0 ; i < extensions.size(); i++) {
        _readPluginsLoaded.push_back(make_pair(extensions[i],FFMPEGplugin));
    }
    delete readFfmpeg;
    
}
void Model::loadBuiltinPlugins(){
    // these  are built-in nodes
	_nodeNames.append("Reader");
	_nodeNames.append("Viewer");
}
