//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Model.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <QtCore/QMutex>
#include <QtCore/QDir>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include "Global/MemoryInfo.h"
#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"


#include "Engine/Hash64.h"
#include "Engine/Node.h"
#include "Engine/ChannelSet.h"
#include "Engine/OfxNode.h"
#include "Engine/ViewerNode.h"
#include "Engine/VideoEngine.h"
#include "Engine/Format.h"
#include "Engine/Settings.h"
#include "Engine/Lut.h"
#include "Engine/NodeCache.h"
#include "Engine/OfxHost.h"
#include "Engine/ViewerCache.h"
#include "Engine/Knob.h"

#include "Readers/Reader.h"
#include "Readers/Read.h"
#include "Readers/ReadExr.h"
#include "Readers/ReadFfmpeg_deprecated.h"
#include "Readers/ReadQt.h"
#include "Writers/Writer.h"
#include "Writers/Write.h"
#include "Writers/WriteQt.h"
#include "Writers/WriteExr.h"

#include "Gui/Gui.h"
#include "Gui/ViewerGL.h"
#include "Gui/TabWidget.h"
#include "Gui/Gui.h"
#include "Gui/NodeGui.h"

using namespace std;
using namespace Powiter;

Model::Model(AppInstance* appInstance)
: _appInstance(appInstance),
_currentOutput(0)
, ofxHost(new Powiter::OfxHost(this))
,_generalMutex(new QMutex)
{
    
    /*node cache initialisation & restoration*/
    _nodeCache = NodeCache::getNodeCache();
    U64 nodeCacheMaxSize = (Settings::getPowiterCurrentSettings()->_cacheSettings.maxCacheMemoryPercent-
                            Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent)*
    getSystemTotalRAM();
    _nodeCache->setMaximumCacheSize(nodeCacheMaxSize);
    
    
    /*viewer cache initialisation & restoration*/
    _viewerCache = new ViewerCache(this);
    _viewerCache->setMaximumCacheSize((U64)((double)Settings::getPowiterCurrentSettings()->_cacheSettings.maxDiskCache));
    _viewerCache->setMaximumInMemorySize(Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent);
    _viewerCache->restore();
    
    /*loading all plugins*/
    loadAllPlugins();
    
    _knobFactory = KnobFactory::instance();
    
    
    /*allocating lookup tables*/
    // Powiter::Color::allocateLuts();
    
    //    _videoEngine = new VideoEngine(this);
    //connect(this,SIGNAL(vengineNeeded(int)),_videoEngine,SLOT(startEngine(int)));
    
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
}



Model::~Model() {
    for (U32 i = 0; i < _currentNodes.size(); ++i) {
        _currentNodes[i]->deleteNode();
    }

    _viewerCache->save();
    delete _viewerCache;
    foreach(Format* f,_formats) delete f;
    for(ReadPluginsIterator it = _readPluginsLoaded.begin(); it!=_readPluginsLoaded.end(); ++it) {
        delete it->second.second;
    }
    for(WritePluginsIterator it = _writePluginsLoaded.begin(); it!=_writePluginsLoaded.end(); ++it) {
        delete it->second.second;
    }
    _formats.clear();
    _nodeNames.clear();
    delete _generalMutex;
}

void Model::loadAllPlugins(){
    /*loading node plugins*/
    loadBuiltinPlugins();
    
    /*loading read plugins*/
    loadReadPlugins();
    
    /*loading write plugins*/
    loadWritePlugins();
    
    /*loading ofx plugins*/
    QStringList ofxPluginNames = ofxHost->loadOFXPlugins();
    _nodeNames.append(ofxPluginNames);
    
}

void Model::clearNodes(){
    foreach(Node* n,_currentNodes){
        delete n;
    }
    _currentNodes.clear();
}


void Model::checkViewersConnection(){
    for (U32 i = 0; i < _currentNodes.size(); ++i) {
        if (_currentNodes[i]->className() == "Viewer") {
            dynamic_cast<OutputNode*>(_currentNodes[i])->updateDAG(true);
        }
    }
}

void Model::updateDAG(OutputNode *output,bool isViewer){
    assert(output);
    _currentOutput = output;
    output->updateDAG(isViewer);
}


Node* Model::createNode(const std::string& name) {
	Node* node = 0;
    if(name == "Reader"){
		node = new Reader(this);
	}else if(name =="Viewer"){
        node = new ViewerNode(_viewerCache,this);
	}else if(name == "Writer"){
		node = new Writer(this);
    } else {
        node = ofxHost->createOfxNode(name,this);
    }
    return node;
}
void Model::initNodeCountersAndSetName(Node* n){
    assert(n);
    map<string,int>::iterator it = _nodeCounters.find(n->className());
    if(it != _nodeCounters.end()){
        it->second++;
        n->setName(QString(QString(n->className().c_str())+ "_" + QString::number(it->second)).toStdString());
    }else{
        _nodeCounters.insert(make_pair(n->className(), 1));
        n->setName(QString(QString(n->className().c_str())+ "_" + QString::number(1)).toStdString());
    }
}

bool Model::connect(int inputNumber,const std::string& inputName,Node* output){
    for (U32 i = 0; i < _currentNodes.size(); ++i) {
        if (_currentNodes[i]->getName() == inputName) {
            connect(inputNumber,_currentNodes[i], output);
            return true;
        }
    }
    return false;
}

bool Model::connect(int inputNumber,Node* input,Node* output){
    if(!output->connectInput(input, inputNumber)){
        return false;
    }
    input->connectOutput(output);
    return true;
}

bool Model::disconnect(Node* input,Node* output){
    if(!output->disconnectInput(input)){
        return false;
    }
    if(!input->disconnectOutput(output)){
        return false;
    }
    return true;
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
    vector<string> functions;
    functions.push_back("BuildRead");
    vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(POWITER_READERS_PLUGINS_PATH, functions);
    for (U32 i = 0 ; i < plugins.size(); ++i) {
        pair<bool,ReadBuilder> func = plugins[i]->findFunction<ReadBuilder>("BuildRead");
        if(func.first){
            Read* read = func.second(NULL);
            assert(read);
            vector<string> extensions = read->fileTypesDecoded();
            string decoderName = read->decoderName();
            _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,plugins[i])));
            delete read;
        }
    }
    
    loadBuiltinReads();
    
    std::map<std::string, LibraryBinary*> defaultMapping;
    for (ReadPluginsIterator it = _readPluginsLoaded.begin(); it!=_readPluginsLoaded.end(); ++it) {
        if(it->first == "OpenEXR"){
            defaultMapping.insert(make_pair("exr", it->second.second));
        }else if(it->first == "QImage (Qt)"){
            defaultMapping.insert(make_pair("jpg", it->second.second));
            defaultMapping.insert(make_pair("bmp", it->second.second));
            defaultMapping.insert(make_pair("jpeg", it->second.second));
            defaultMapping.insert(make_pair("gif", it->second.second));
            defaultMapping.insert(make_pair("png", it->second.second));
            defaultMapping.insert(make_pair("pbm", it->second.second));
            defaultMapping.insert(make_pair("pgm", it->second.second));
            defaultMapping.insert(make_pair("ppm", it->second.second));
            defaultMapping.insert(make_pair("xbm", it->second.second));
            defaultMapping.insert(make_pair("xpm", it->second.second));
        }
    }
    Settings::getPowiterCurrentSettings()->_readersSettings.fillMap(defaultMapping);
}

void Model::displayLoadedReads(){
    ReadPluginsIterator it = _readPluginsLoaded.begin();
    for (; it!=_readPluginsLoaded.end(); ++it) {
        cout << it->first << " : ";
        for (U32 i = 0; i < it->second.first.size(); ++i) {
            cout << it->second.first[i] << " ";
        }
        cout << endl;
    }
}

void Model::loadBuiltinReads(){
    Read* readExr = ReadExr::BuildRead(NULL);
    assert(readExr);
    std::vector<std::string> extensions = readExr->fileTypesDecoded();
    std::string decoderName = readExr->decoderName();

    std::map<std::string,void*> EXRfunctions;
    EXRfunctions.insert(make_pair("BuildRead", (void*)&ReadExr::BuildRead));
    LibraryBinary *EXRplugin = new LibraryBinary(EXRfunctions);
    assert(EXRplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,EXRplugin)));
    }
    delete readExr;
    
    Read* readQt = ReadQt::BuildRead(NULL);
    assert(readQt);
    extensions = readQt->fileTypesDecoded();
    decoderName = readQt->decoderName();

    std::map<std::string,void*> Qtfunctions;
    Qtfunctions.insert(make_pair("BuildRead", (void*)&ReadQt::BuildRead));
    LibraryBinary *Qtplugin = new LibraryBinary(Qtfunctions);
    assert(Qtplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,Qtplugin)));
    }
    delete readQt;
    
    Read* readFfmpeg = ReadFFMPEG::BuildRead(NULL);
    assert(readFfmpeg);
    extensions = readFfmpeg->fileTypesDecoded();
    decoderName = readFfmpeg->decoderName();

    std::map<std::string,void*> FFfunctions;
    FFfunctions.insert(make_pair("ReadBuilder", (void*)&ReadFFMPEG::BuildRead));
    LibraryBinary *FFMPEGplugin = new LibraryBinary(FFfunctions);
    assert(FFMPEGplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,FFMPEGplugin)));
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
    
    vector<string> functions;
    functions.push_back("BuildWrite");
    vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(POWITER_WRITERS_PLUGINS_PATH, functions);
    for (U32 i = 0 ; i < plugins.size(); ++i) {
        pair<bool,WriteBuilder> func = plugins[i]->findFunction<WriteBuilder>("BuildWrite");
        if(func.first){
            Write* write = func.second(NULL);
            assert(write);
            vector<string> extensions = write->fileTypesEncoded();
            string encoderName = write->encoderName();
            _writePluginsLoaded.insert(make_pair(encoderName,make_pair(extensions,plugins[i])));
            delete write;
        }
    }
    loadBuiltinWrites();
    std::map<std::string, LibraryBinary*> defaultMapping;
    for (WritePluginsIterator it = _writePluginsLoaded.begin(); it!=_writePluginsLoaded.end(); ++it) {
        if(it->first == "OpenEXR"){
            defaultMapping.insert(make_pair("exr", it->second.second));
        }else if(it->first == "QImage (Qt)"){
            defaultMapping.insert(make_pair("jpg", it->second.second));
            defaultMapping.insert(make_pair("bmp", it->second.second));
            defaultMapping.insert(make_pair("jpeg", it->second.second));
            defaultMapping.insert(make_pair("gif", it->second.second));
            defaultMapping.insert(make_pair("png", it->second.second));
            defaultMapping.insert(make_pair("pbm", it->second.second));
            defaultMapping.insert(make_pair("pgm", it->second.second));
            defaultMapping.insert(make_pair("ppm", it->second.second));
            defaultMapping.insert(make_pair("xbm", it->second.second));
            defaultMapping.insert(make_pair("xpm", it->second.second));
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

    std::map<std::string,void*> Qtfunctions;
    Qtfunctions.insert(make_pair("BuildWrite",(void*)&WriteQt::BuildWrite));
    LibraryBinary *QtWritePlugin = new LibraryBinary(Qtfunctions);
    assert(QtWritePlugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _writePluginsLoaded.insert(make_pair(encoderName,make_pair(extensions,QtWritePlugin)));
    }
    delete writeQt;
    
    Write* writeEXR = WriteExr::BuildWrite(NULL);
    std::vector<std::string> extensionsExr = writeEXR->fileTypesEncoded();
    string encoderNameExr = writeEXR->encoderName();

    std::map<std::string,void*> EXRfunctions;
    EXRfunctions.insert(make_pair("BuildWrite",(void*)&WriteExr::BuildWrite));
    LibraryBinary *ExrWritePlugin = new LibraryBinary(EXRfunctions);
    assert(ExrWritePlugin);
    for (U32 i = 0 ; i < extensionsExr.size(); ++i) {
        _writePluginsLoaded.insert(make_pair(encoderName,make_pair(extensionsExr,ExrWritePlugin)));
    }
    delete writeEXR;
}

void Model::clearPlaybackCache(){
    if (!_currentOutput) {
        return;
    }
    _viewerCache->clearInMemoryPortion();
}


void Model::clearDiskCache(){
    _viewerCache->clearInMemoryPortion();
    _viewerCache->clearDiskCache();
}


void  Model::clearNodeCache(){
    _nodeCache->clear();
}





/*starts the videoEngine for nbFrames. It will re-init the viewer so the
 *frame fit in the viewer.*/
void Model::startVideoEngine(int nbFrames){
    assert(_currentOutput);
    _currentOutput->getVideoEngine()->startEngine(nbFrames);
}

VideoEngine* Model::getVideoEngine() const{
    if(_currentOutput)
        return _currentOutput->getVideoEngine();
    else
        return NULL;
}



QString Model::serializeNodeGraph() const{
    const std::vector<Node*>& activeNodes = _appInstance->getAllActiveNodes();
    QString ret;
    
    QXmlStreamWriter writer(&ret);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("Nodes");
    foreach(Node* n, activeNodes){
        //serialize inputs
        assert(n);
        writer.writeStartElement("Node");
        
        if(!n->isOpenFXNode()){
            writer.writeAttribute("ClassName",n->className().c_str());
        }else{
            OfxNode* ofxNode = dynamic_cast<OfxNode*>(n);
            std::string name = ofxNode->getShortLabel();
            std::string grouping = ofxNode->getPluginGrouping();
            vector<string> groups = ofxExtractAllPartsOfGrouping(grouping);
            if (groups.size() >= 1) {
                name.append("  [");
                name.append(groups[0]);
                name.append("]");
            }
            writer.writeAttribute("ClassName",name.c_str());
        }
        writer.writeAttribute("Label", n->getName().c_str());
        
        writer.writeStartElement("Inputs");
        const Node::InputMap& inputs = n->getInputs();
        for (Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it) {
            writer.writeStartElement("Input");
            writer.writeAttribute("Number", QString::number(it->first));
            writer.writeAttribute("Name", it->second->getName().c_str());
            writer.writeEndElement();// end input
        }
        writer.writeEndElement(); // end inputs
                                  //serialize knobs
        const std::vector<Knob*>& knobs = n->getKnobs();
        writer.writeStartElement("Knobs");
        for (U32 i = 0; i < knobs.size(); ++i) {
            writer.writeStartElement("Knob");
            writer.writeAttribute("Description", knobs[i]->getDescription().c_str());
            writer.writeAttribute("Param", knobs[i]->serialize().c_str());
            writer.writeEndElement();//end knob
        }
        writer.writeEndElement(); // end knobs
        
        //serialize gui infos
        
        writer.writeStartElement("Gui");
        
        NodeGui* nodeGui = _appInstance->getNodeGui(n);
        double x=0,y=0;
        if(nodeGui){
            x = nodeGui->pos().x();
            y = nodeGui->pos().y();
        }
        writer.writeAttribute("X", QString::number(x));
        writer.writeAttribute("Y", QString::number(y));
        writer.writeEndElement();//end gui
        
        writer.writeEndElement();//end node
    }
    writer.writeEndElement(); // end nodes
    writer.writeEndDocument();
    return ret;
}

void Model::restoreGraphFromString(const QString& str){
    // pair< Node, pair< AttributeName, AttributeValue> >
    std::vector<std::pair<Node*, XMLProjectLoader::XMLParsedElement* > > actionsMap;
    QXmlStreamReader reader(str);
    
    while(!reader.atEnd() && !reader.hasError()){
        QXmlStreamReader::TokenType token = reader.readNext();
        /* If token is just StartDocument, we'll go to next.*/
        if(token == QXmlStreamReader::StartDocument) {
            continue;
        }
        /* If token is StartElement, we'll see if we can read it.*/
        if(token == QXmlStreamReader::StartElement) {
            /* If it's named Nodes, we'll go to the next.*/
            if(reader.name() == "Nodes") {
                continue;
            }
            /* If it's named Node, we'll dig the information from there.*/
            if(reader.name() == "Node") {
                /* Let's check that we're really getting a Node. */
                if(reader.tokenType() != QXmlStreamReader::StartElement &&
                   reader.name() == "Node") {
                    _appInstance->showErrorDialog("Loader", QString("Unable to restore the graph:\n") + reader.errorString());
                    return;
                }
                QString className,label;
                QXmlStreamAttributes nodeAttributes = reader.attributes();
                if(nodeAttributes.hasAttribute("ClassName")) {
                    className = nodeAttributes.value("ClassName").toString();
                }
                if(nodeAttributes.hasAttribute("Label")) {
                    label = nodeAttributes.value("Label").toString();
                }
                
                assert(_appInstance);
                _appInstance->deselectAllNodes();
                Node* n = _appInstance->createNode(className);
                if(!n){
                    QString text("Failed to restore the graph! \n The node ");
                    text.append(className);
                    text.append(" was found in the auto-save script but doesn't seem \n"
                                "to exist in the currently loaded plug-ins.");
                    _appInstance->showErrorDialog("Autosave", text );
                    _appInstance->clearNodes();
                    _appInstance->createNode("Viewer");
                    return;
                }
                n->setName(label.toStdString());
                
                reader.readNext();
                while(!(reader.tokenType() == QXmlStreamReader::EndElement &&
                        reader.name() == "Node")) {
                    if(reader.tokenType() == QXmlStreamReader::StartElement) {
                        
                        if(reader.name() == "Inputs") {
                            
                            while(!(reader.tokenType() == QXmlStreamReader::EndElement &&
                                    reader.name() == "Inputs")) {
                                reader.readNext();
                                if(reader.tokenType() == QXmlStreamReader::StartElement) {
                                    if(reader.name() == "Input") {
                                        QXmlStreamAttributes inputAttributes = reader.attributes();
                                        int number = -1;
                                        QString name;
                                        if(inputAttributes.hasAttribute("Number")){
                                            number = inputAttributes.value("Number").toString().toInt();
                                        }
                                        if(inputAttributes.hasAttribute("Name")){
                                            name = inputAttributes.value("Name").toString();
                                        }
                                        actionsMap.push_back(make_pair(n,new XMLProjectLoader::InputXMLParsedElement(name,number)));
                                    }
                                    
                                }
                            }
                        }
                        
                        if(reader.name() == "Knobs") {
                            
                            while(!(reader.tokenType() == QXmlStreamReader::EndElement &&
                                    reader.name() == "Knobs")) {
                                reader.readNext();
                                if(reader.tokenType() == QXmlStreamReader::StartElement) {
                                    if(reader.name() == "Knob") {
                                        QXmlStreamAttributes knobAttributes = reader.attributes();
                                        QString description,param;
                                        if(knobAttributes.hasAttribute("Description")){
                                            description = knobAttributes.value("Description").toString();
                                        }
                                        if(knobAttributes.hasAttribute("Param")){
                                            param = knobAttributes.value("Param").toString();
                                        }
                                        actionsMap.push_back(make_pair(n,new XMLProjectLoader::KnobXMLParsedElement(description,param)));
                                    }
                                }
                            }
                            
                        }
                        
                        if(reader.name() == "Gui") {
                            double x = 0,y = 0;
                            QXmlStreamAttributes posAttributes = reader.attributes();
                            QString description,param;
                            if(posAttributes.hasAttribute("X")){
                                x = posAttributes.value("X").toString().toDouble();
                            }
                            if(posAttributes.hasAttribute("Y")){
                                y = posAttributes.value("Y").toString().toDouble();
                            }
                            
                            
                            actionsMap.push_back(make_pair(n,new XMLProjectLoader::NodeGuiXMLParsedElement(x,y)));
                            
                        }
                    }
                    reader.readNext();
                }
            }
        }
    }
    if(reader.hasError()){
        _appInstance->showErrorDialog("Loader", QString("Unable to restore the graph :\n") + reader.errorString());
        return;
    }
    //adjusting knobs & connecting nodes now
    for (U32 i = 0; i < actionsMap.size(); ++i) {
        pair<Node*,XMLProjectLoader::XMLParsedElement*>& action = actionsMap[i];
        analyseSerializedNodeString(action.first, action.second);
    }
    
}
void Model::analyseSerializedNodeString(Node* n,XMLProjectLoader::XMLParsedElement* v){
    assert(n);
    if(v->_element == "Input"){
        XMLProjectLoader::InputXMLParsedElement* inputEl = static_cast<XMLProjectLoader::InputXMLParsedElement*>(v);
        assert(inputEl);
        int inputNb = inputEl->_number;
        connect(inputNb,inputEl->_name.toStdString(), n);
        //  cout << "Input: " << inputEl->_number << " :" << inputEl->_name.toStdString() << endl;
    }else if(v->_element == "Knob"){
        XMLProjectLoader::KnobXMLParsedElement* inputEl = static_cast<XMLProjectLoader::KnobXMLParsedElement*>(v);
        assert(inputEl);
        const std::vector<Knob*>& knobs = n->getKnobs();
        for (U32 j = 0; j < knobs.size(); ++j) {
            if (knobs[j]->getDescription() == inputEl->_description.toStdString()) {
                knobs[j]->restoreFromString(inputEl->_param.toStdString());
                break;
            }
        }
        //cout << "Knob: " << inputEl->_description.toStdString() << " :" << inputEl->_param.toStdString() << endl;
    }else if(v->_element == "Gui"){
        XMLProjectLoader::NodeGuiXMLParsedElement* inputEl = static_cast<XMLProjectLoader::NodeGuiXMLParsedElement*>(v);
        assert(inputEl);
        NodeGui* nodeGui = _appInstance->getNodeGui(n);
        if(nodeGui){
            nodeGui->refreshPosition(inputEl->_x,inputEl->_y);
        }
        //  cout << "Gui/Pos: " << inputEl->_x << " , " << inputEl->_y << endl;
    }
}

void Model::loadProject(const QString& filename,bool autoSave){
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    QString content = in.readAll();
    if(autoSave){
        QString toRemove = content.left(content.indexOf('\n')+1);
        content = content.remove(toRemove);
    }
    restoreGraphFromString(content);
    file.close();
}
void Model::saveProject(const QString& path,const QString& filename,bool autoSave){
    if(autoSave){
        QFile file(AppInstance::autoSavesDir()+filename);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
            cout <<  file.errorString().toStdString() << endl;
            return;
        }
        QTextStream out(&file);
        if(!path.isEmpty())
            out << path << endl;
        else
            out << "unsaved" << endl;
        out << serializeNodeGraph();
        file.close();
    }else{
        QFile file(path+filename);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
            cout << (path+filename).toStdString() <<  file.errorString().toStdString() << endl;
            return;
        }
        QTextStream out(&file);
        out << serializeNodeGraph();
        file.close();
        
    }
}
void Model::triggerAutoSaveOnNextEngineRun(){
    if(getVideoEngine()){
        getVideoEngine()->triggerAutoSaveOnNextRun();
        _appInstance->setProjectTitleAsModified();
    }
}

