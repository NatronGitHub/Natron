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
#include "Gui/SettingsPanel.h"
#include "Gui/Knob.h"
#include "Gui/NodeGui.h"
#include "Engine/ChannelSet.h"
#include "Engine/Model.h"
#include "Engine/Format.h"
#include "Engine/NodeCache.h"
#include "Readers/Reader.h"
#include "Global/AppManager.h"
#include "Gui/Timeline.h"
#include "Gui/ViewerTab.h"
#include "Engine/Row.h"
#include "Gui/Gui.h"
#include "Writers/Writer.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"

using namespace std;
using namespace Powiter;

namespace {
    void Hash64_appendKnob(Hash64* hash, const Knob* knob){
        const std::vector<U64>& values= knob->getValues();
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
    if(info().hasBeenModified()){
        _info.merge_dataWindow(parent->info().dataWindow());
    }else{
        _info.set_dataWindow(parent->info().dataWindow());
    }
    _info.set_rgbMode(parent->info().rgbMode());
    _info.set_blackOutside(parent->info().blackOutside());
}
void Node::clear_info(){
	_info.reset();
   
    
}
void Node::Info::reset(){
    _firstFrame = 0;
    _lastFrame = 0;
    _ydirection = 0;
    _channels = Mask_None;
    set(0, 0, 0, 0);
    _displayWindow.set(0,0,0,0);
    _modified = false;
    _blackOutside = false;
}

int Node::inputCount() const {
    return _parents.size();
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
	for (int i =0 ; i < inputCount(); ++i) {
        Node* parent = _parents[i];
        merge_frameRange(parent->info().firstFrame(),parent->info().lastFrame());
        if(forReal){
            final_direction+=parent->info().ydirection();
            chans += parent->info().channels();
            ChannelSet supportedComp = supportedComponents();
            if ((supportedComp & chans) != chans) {
                cout <<"WARNING:( " << getName() << ") does not support one or more of the following channels:\n " ;
                chans.printOut();
                cout << "Coming from node " << parent->getName() << endl;
            }
            if(parent->info().rgbMode()){
                displayMode = true;
            }
            if(parent->info().blackOutside()){
                _info.set_blackOutside(true);
            }
           
        }
        _info.merge_dataWindow(parent->info().dataWindow());
        _info.merge_displayWindow(parent->info().displayWindow());
    }
    if(_parents.size() > 0)
        final_direction = final_direction / _parents.size();
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


Node::Node(Model* model):
_model(model),_info()
{

    _marked = false;
    _knobsCB = new KnobCallback(NULL,this);
	
}

void Node::removeKnob(Knob* knob){
    for(U32 i = 0 ; i < _knobsVector.size() ; ++i) {
        if (knob == _knobsVector[i]) {
            _knobsVector.erase(_knobsVector.begin()+i);
            break;
        }
    }
}


void Node::removeChild(Node* child){
    
    U32 i=0;
    while(i<_children.size()){
        if(_children[i]==child){
            _children.erase(_children.begin()+i);
            break;
        }
        ++i;
    }
}
void Node::removeParent(Node* parent){
    
    U32 i=0;
    while(i<_parents.size()){
        if(_parents[i]==parent){
            _parents.erase(_parents.begin()+i);
            break;
        }
        ++i;
    }
}

void Node::removeThisFromParents(){
    for(U32 i = 0 ; i < _parents.size() ; ++i) {
        _parents[i]->removeChild(this);
    }
}

void Node::removeThisFromChildren(){
    for(U32 i = 0 ; i < _children.size() ; ++i) {
        _children[i]->removeParent(this);
    }

}

void Node::initializeInputs(){
    initInputLabelsMap();
    applyLabelsToInputs();
}


Node* Node::input(int index){
    if((U32)index < _parents.size()){
        return _parents[index];
    }else{
        return NULL;
    }
}


/*To change label names : override setInputLabel to reflect what you want to have for input "inputNb" */
std::string Node::setInputLabel(int inputNb){
    string out;
    out.append(1,(char)(inputNb+65));
    return out;
}
void Node::applyLabelsToInputs(){
    for (std::map<int, std::string>::iterator it = _inputLabelsMap.begin(); it!=_inputLabelsMap.end(); ++it) {
        _inputLabelsMap[it->first] = setInputLabel(it->first);
    }
}
void Node::initInputLabelsMap(){
    int i=0;
    while(i<maximumInputs()){
        char str[2];
        str[0] =i+65;
        str[1]='\0';
        _inputLabelsMap[i]=str;
        ++i;
    }
    
}


bool Node::validate(bool forReal){
    if(isOpenFXNode()){
        if (_parents.size() < (unsigned int)minimumInputs()) {
            return false;
        }
    }
    if(!isInputNode()){
        merge_info(forReal);
    }
    if(!_validate(forReal))
        return false;
    preProcess();
    _nodeGUI->updateChannelsTooltip();
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
    for(U32 i=0;i<_knobsVector.size();++i) {
        Hash64_appendKnob(&_hashValue,_knobsVector[i]);
    }
    /*append the node name*/
    Hash64_appendQString(&_hashValue, QString(className().c_str()));
    /*mark this node as already been computed*/
    alreadyComputedHash.push_back(_name);
    
    /*Recursive call to parents and add their hash key*/
    foreach(Node* parent,_parents){
        parent->computeTreeHash(alreadyComputedHash);
        _hashValue.append(parent->hash().value());
    }
    /*Compute the hash key*/
    _hashValue.computeHash();
}
bool Node::hashChanged(){
    U64 oldHash=_hashValue.value();
    vector<std::string> v;
    computeTreeHash(v);
    return oldHash!=_hashValue.value();
}
void Node::initKnobs(KnobCallback *cb){
    cb->initNodeKnobsVector();
}
void Node::createKnobDynamically(){
    
	_knobsCB->createKnobDynamically();
}


Row* Node::get(int y,int x,int r){
    NodeCache* cache = NodeCache::getNodeCache();
    std::string filename;
    Reader* reader = dynamic_cast<Reader*>(this);
    if(reader){
        int current_frame;
        const VideoEngine::DAG& dag = _model->getVideoEngine()->getCurrentDAG();
        if(dag.isOutputAnOpenFXNode()){
            current_frame = dag.outputAsOpenFXNode()->currentFrame();
        }else{
            if(dag.isOutputAViewer()){
                current_frame = reader->clampToRange(dag.outputAsViewer()->currentFrame());
            }else{
                current_frame = dag.outputAsWriter()->currentFrame();
            }
        }
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

Node::~Node(){
    _parents.clear();
    _children.clear();
    _knobsVector.clear();
    _inputLabelsMap.clear();
    delete _knobsCB;
}


OutputNode::OutputNode(Model* model):Node(model){
    _mutex = new QMutex;
    _openGLCondition = new QWaitCondition;
    
    _videoEngine = new VideoEngine(model,_openGLCondition,_mutex);
}

OutputNode::~OutputNode(){
    delete _videoEngine;
    delete _mutex;
    delete _openGLCondition;
}

