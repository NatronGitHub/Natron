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

#include "Engine/Hash.h"
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

void Node::copy_info(Node* parent){
    clear_info();
    const Box2D* bboxParent = dynamic_cast<const Box2D*>(parent->getInfo());
	_info->firstFrame(parent->getInfo()->firstFrame());
	_info->lastFrame(parent->getInfo()->lastFrame());
	_info->setYdirection(parent->getInfo()->getYdirection());
	_info->setDisplayWindow(parent->getInfo()->getDisplayWindow());
	_info->setChannels(parent->getInfo()->channels());
    if(_info->hasBeenModified()){
        _info->merge(*(parent->getInfo()));
    }else{
        _info->x(bboxParent->x());
        _info->y(bboxParent->y());
        _info->top(bboxParent->top());
        _info->right(bboxParent->right());
    }
    _info->rgbMode(parent->getInfo()->rgbMode());
    _info->blackOutside(parent->getInfo()->blackOutside());
}
void Node::clear_info(){
	_info->reset();
   
    
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

void Node::Info::mergeDisplayWindow(const Format& other){
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
    bool displayMode = _info->rgbMode();
	for (int i =0 ; i < inputCount(); ++i) {
        Node* parent = _parents[i];
        merge_frameRange(parent->getInfo()->firstFrame(),parent->getInfo()->lastFrame());
        if(forReal){
            final_direction+=parent->getInfo()->getYdirection();
            chans += parent->getInfo()->channels();
            ChannelSet supportedComp = supportedComponents();
            if ((supportedComp & chans) != chans) {
                cout <<"WARNING:( " << getName() << ") does not support one or more of the following channels:\n " ;
                chans.printOut();
                cout << "Coming from node " << parent->getName() << endl;
            }
            if(parent->getInfo()->rgbMode()){
                displayMode = true;
            }
            if(parent->getInfo()->blackOutside()){
                _info->blackOutside(true);
            }
           
        }
        _info->merge(*parent->getInfo());
        _info->mergeDisplayWindow(parent->getInfo()->getDisplayWindow());
    }
    if(_parents.size() > 0)
        final_direction = final_direction / _parents.size();
	_info->setChannels(chans);
    _info->rgbMode(displayMode);
    _info->setYdirection(final_direction);
}
void Node::merge_frameRange(int otherFirstFrame,int otherLastFrame){
	if (_info->firstFrame() == -1) { // if not initialized
        _info->firstFrame(otherFirstFrame);
    }else if(otherFirstFrame < _info->firstFrame()){
         _info->firstFrame(otherFirstFrame);
    }
    
    if (_info->lastFrame() == -1)
    {
        _info->lastFrame(otherLastFrame);
    }
    else if(otherLastFrame > _info->lastFrame()){
        _info->lastFrame(otherLastFrame);
    }
	
}
bool Node::Info::operator==( Node::Info &other){
	if(other.channels()==this->channels() &&
       other.firstFrame()==this->_firstFrame &&
       other.lastFrame()==this->_lastFrame &&
       other.getYdirection()==this->_ydirection &&
       other.getDisplayWindow()==this->_displayWindow
       ){
        return true;
	}else{
		return false;
	}
    
}
void Node::Info::operator=(const Node::Info &other){
    _channels = other._channels;
    _firstFrame = other._firstFrame;
    _lastFrame = other._lastFrame;
    _displayWindow = other._displayWindow;
    setYdirection(other._ydirection);
    set(other);
    rgbMode(other._rgbMode);
    _blackOutside = other._blackOutside;
}


Node::Node(Model* model):_model(model){
    _marked = false;
    _info = new Info;
    _hashValue=new Hash;
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
    _hashValue->reset();
    /*append all values stored in knobs*/
    for(U32 i=0;i<_knobsVector.size();++i) {
        _hashValue->appendKnobToHash(_knobsVector[i]);
    }
    /*append the node name*/
    _hashValue->appendQStringToHash(QString(className().c_str()));
    /*mark this node as already been computed*/
    alreadyComputedHash.push_back(_name);
    
    /*Recursive call to parents and add their hash key*/
    foreach(Node* parent,_parents){
        parent->computeTreeHash(alreadyComputedHash);
        _hashValue->appendValueToHash(parent->getHash()->getHashValue());
    }
    /*Compute the hash key*/
    _hashValue->computeHash();
}
bool Node::hashChanged(){
    U64 oldHash=_hashValue->getHashValue();
    vector<std::string> v;
    computeTreeHash(v);
    return oldHash!=_hashValue->getHashValue();
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
    U64 key = _hashValue->getHashValue();
    pair<U64,Row*> entry = cache->get(key , filename,x,r,y,_info->channels());
    if (entry.second && entry.first != 0) {
        out = entry.second;
    }
    // Shit happens: there may be a completely different cache entry with the same hash
    // FIXME: we should check for more things (frame number...)
    if (!out || out->y() != y || out->offset() != x || out->right() !=  r) {
        if (cacheData()) {
            out = cache->addRow(entry.first,x,r,y, _info->channels(), filename);
            if (!out) {
                return NULL;
            }
        } else {
            out = new Row(x,y,r,_info->channels());
            out->allocateRow();
        }
        assert(out->offset() == x && out->right() == r);
        engine(y,x,r, _info->channels(), out);
    }
    assert(out);
    return out;
}

Node::~Node(){
    _parents.clear();
    _children.clear();
    _knobsVector.clear();
    _inputLabelsMap.clear();
    delete _hashValue;
    delete _info;
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

