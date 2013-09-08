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

#include "Node.h"

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include "Engine/Hash64.h"
#include "Engine/ChannelSet.h"
#include "Engine/Model.h"
#include "Engine/Format.h"
#include "Engine/NodeCache.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"
#include "Engine/Row.h"
#include "Engine/Knob.h"

#include "Readers/Reader.h"
#include "Writers/Writer.h"

#include "Global/AppManager.h"
#include "Global/NodeInstance.h"
#include "Global/KnobInstance.h"



using namespace std;
using namespace Powiter;

namespace {
    void Hash64_appendKnob(Hash64* hash, const Knob* knob){
        const std::vector<U64>& values= knob->getHashVector();
        for(U32 i=0;i<values.size();++i) {
            hash->append(values[i]);
        }
    }
}

void Node::copy_info(Node* parent){
    clear_info();
	_info.set_firstFrame(parent->info().firstFrame());
	_info.set_lastFrame(parent->info().lastFrame());
	_info.set_ydirection(parent->info().ydirection());
	_info.set_displayWindow(parent->info().displayWindow());
	_info.set_channels(parent->info().channels());
    _info.set_dataWindow(parent->info().dataWindow());
    _info.set_rgbMode(parent->info().rgbMode());
    _info.set_blackOutside(parent->info().blackOutside());
}
void Node::clear_info(){
	_info.reset();
   
    
}
void Node::Info::reset(){
    _firstFrame = -1;
    _lastFrame = -1;
    _ydirection = 0;
    _channels = Mask_None;
    _dataWindow.set(0, 0, 0, 0);
    _displayWindow.set(0,0,0,0);
    _blackOutside = false;
    _executingEngine = 0;
}



void Node::Info::merge_displayWindow(const Format& other){
    _displayWindow.merge(other);
    _displayWindow.pixel_aspect(other.pixel_aspect());
    if(_displayWindow.name().empty()){
        _displayWindow.name(other.name());
    }
}
void Node::merge_info(bool forReal){
	
    clear_info();
	int final_direction=0;
	ChannelSet chans;
    bool displayMode = info().rgbMode();
	for (NodeInstance::InputMap::const_iterator it = _instance->getInputs().begin();
         it!=_instance->getInputs().end();++it) {
        
        Node* input = it->second->getNode();
        merge_frameRange(input->info().firstFrame(),input->info().lastFrame());
        if(forReal){
            final_direction+=input->info().ydirection();
            chans += input->info().channels();
            ChannelSet supportedComp = supportedComponents();
            if ((supportedComp & chans) != chans) {
                cout <<"WARNING:( " << getName() << ") does not support one or more of the following channels:\n " ;
                chans.printOut();
                cout << "Coming from node " << input->getName() << endl;
            }
            if(input->info().rgbMode()){
                displayMode = true;
            }
            if(input->info().blackOutside()){
                _info.set_blackOutside(true);
            }
           
        }
        _info.merge_dataWindow(input->info().dataWindow());
        _info.merge_displayWindow(input->info().displayWindow());
    }
    U32 size = _instance->getInputs().size();
    if(size > 0)
        final_direction = final_direction / size;
	_info.set_channels(chans);
    _info.set_rgbMode(displayMode);
    _info.set_ydirection(final_direction);
}
void Node::merge_frameRange(int otherFirstFrame,int otherLastFrame){
	if (info().firstFrame() == -1) { // if not initialized
        _info.set_firstFrame(otherFirstFrame);
    } else if (otherFirstFrame < info().firstFrame()) {
         _info.set_firstFrame(otherFirstFrame);
    }
    
    if (info().lastFrame() == -1)
    {
        _info.set_lastFrame(otherLastFrame);
    }
    else if (otherLastFrame > info().lastFrame()) {
        _info.set_lastFrame(otherLastFrame);
    }
	
}
bool Node::Info::operator==( Node::Info &other){
	if(other.channels()      == this->channels() &&
       other.firstFrame()    == this->_firstFrame &&
       other.lastFrame()     == this->_lastFrame &&
       other.ydirection()    == this->_ydirection &&
       other.dataWindow()    == this->_dataWindow && // FIXME: [FD] added this line, is it OK?
       other.displayWindow() == this->_displayWindow
       ) {
        return true;
	} else {
		return false;
	}
    
}
void Node::Info::operator=(const Node::Info &other){
    set_channels(other.channels());
    set_firstFrame(other.firstFrame());
    set_lastFrame(other.lastFrame());
    set_displayWindow(other.displayWindow());
    set_ydirection(other.ydirection());
    set_dataWindow(other.dataWindow());
    set_rgbMode(other.rgbMode());
    set_blackOutside(other.blackOutside());
}


Node::Node(NodeInstance* instance):
_model(NULL),
_info(),
_marked(false),
_instance(instance)
{
	
}


void Node::initializeInputs(){
    for(int i = 0;i < maximumInputs();++i){
        _inputLabelsMap.insert(make_pair(i,setInputLabel(i)));
    }
}
const std::string Node::getInputLabel(int inputNb) const{
    map<int,string>::const_iterator it = _inputLabelsMap.find(inputNb);
    if(it == _inputLabelsMap.end()){
        return "";
    }else{
        return it->second;
    }
}

Node* Node::input(int index){
    NodeInstance* n = _instance->input(index);
    if(!n){
        return NULL;
    }else{
        return n->getNode();
    }
}


/*To change label names : override setInputLabel to reflect what you want to have for input "inputNb" */
std::string Node::setInputLabel(int inputNb){
    string out;
    out.append(1,(char)(inputNb+65));
    return out;
}

bool Node::validate(bool forReal){
    if(!isInputNode()){
        merge_info(forReal);
    }
    if(!_validate(forReal))
        return false;
    preProcess();
    _instance->updateNodeChannelsGui(_info.channels());
    return true;
}


void Node::computeTreeHash(std::vector<std::string> &alreadyComputedHash){
    /*If we already computed its hash,return*/
    for(U32 i =0 ; i < alreadyComputedHash.size();++i) {
        if(alreadyComputedHash[i] == _name)
            return;
    }
    /*Clear the values left to compute the hash key*/
    _hashValue.reset();
    /*append all values stored in knobs*/
    const vector<KnobInstance*>& knobs = _instance->getKnobs();
    for(U32 i=0;i<knobs.size();++i) {
        Hash64_appendKnob(&_hashValue,knobs[i]->getKnob());
    }
    /*append the node name*/
    Hash64_appendQString(&_hashValue, QString(className().c_str()));
    /*mark this node as already been computed*/
    alreadyComputedHash.push_back(_name);
    
    /*Recursive call to parents and add their hash key*/
    const NodeInstance::InputMap& inputs = _instance->getInputs();
    for (NodeInstance::InputMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        it->second->getNode()->computeTreeHash(alreadyComputedHash);
        _hashValue.append(it->second->getNode()->hash().value());
    }
    /*Compute the hash key*/
    _hashValue.computeHash();
}
bool Node::hashChanged(){
    U64 oldHash=_hashValue.value();
    std::vector<std::string> v;
    computeTreeHash(v);
    return oldHash!=_hashValue.value();
}

void Node::createKnobDynamically(){
    _instance->createKnobGuiDynamically();
}


Row* Node::get(int y,int x,int r){
    NodeCache* cache = NodeCache::getNodeCache();
    std::string filename;
    Reader* reader = dynamic_cast<Reader*>(this);
    if(reader){
        int current_frame;
        const VideoEngine::DAG& dag = _info.executingEngine()->getCurrentDAG();
        current_frame = reader->clampToRange(dag.getOutput()->getTimeLine().currentFrame());
        filename = reader->getRandomFrameName(current_frame);
    }
    Row* out = 0;
    U64 key = _hashValue.value();
    pair<U64,Row*> entry = cache->get(key , filename,x,r,y,info().channels());
    if (entry.second && entry.first != 0) {
        out = entry.second;
    }
    // Shit happens: there may be a completely different cache entry with the same hash
    // FIXME: we should check for more things (frame number...)
    if (!out || out->y() != y || out->offset() != x || out->right() !=  r) {
        if (cacheData()) {
            out = cache->addRow(entry.first,x,r,y, info().channels(), filename);
            if (!out) {
                return NULL;
            }
        } else {
            out = new Row(x,y,r,info().channels());
            out->allocateRow();
        }
        assert(out->offset() == x && out->right() == r);
        engine(y,x,r, info().channels(), out);
    }
    assert(out);
    return out;
}

Node* Node::hasViewerConnected(Node* node){
    Node* out = 0;
    bool ok=false;
    _hasViewerConnected(node,&ok,out);
    if (ok) {
        return out;
    }else{
        return NULL;
    }
    
}
void Node::_hasViewerConnected(Node* node,bool* ok,Node*& out){
    if (*ok == true) {
        return;
    }
    if(node->className() == "Viewer"){
        out = node;
        *ok = true;
    }else{
        const NodeInstance::OutputMap& outputs = node->getNodeInstance()->getOutputs();
        for (NodeInstance::OutputMap::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
            if(it->second)
                _hasViewerConnected(it->second->getNode(),ok,out);
        }
    }
}
bool Node::hasOutputConnected() const{
    return _instance->hasOutputConnected();
}
bool Node::isInputConnected(int inputNb) const{
    return _instance->isInputConnected(inputNb);
}

Node::~Node(){
    _inputLabelsMap.clear();
}

void Node::setModel(Model* model){
    _model = model;
    if(isOutputNode()){
        dynamic_cast<OutputNode*>(this)->initVideoEngine();
    }
}

OutputNode::OutputNode(NodeInstance* instance):Node(instance),_videoEngine(0){
    _mutex = new QMutex;
    _openGLCondition = new QWaitCondition;
    
    
}

OutputNode::~OutputNode(){
    _videoEngine->quitEngineThread();
    delete _videoEngine;
    delete _mutex;
    delete _openGLCondition;
}
void OutputNode::initVideoEngine(){
    _videoEngine = new VideoEngine(_model,_openGLCondition,_mutex);
}

void TimeLine::seek(int frame){
    assert(frame <= _lastFrame && frame >= _firstFrame);
    _currentFrame = frame;
    emit frameChanged(_currentFrame);
}

void TimeLine::seek_noEmit(int frame){
    assert(frame <= _lastFrame && frame >= _firstFrame);
    _currentFrame = frame;
}
