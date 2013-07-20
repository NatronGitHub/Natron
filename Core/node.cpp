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

 

 



#include "Core/node.h"
#include "Core/hash.h"
#include "Gui/dockableSettings.h"
#include "Gui/knob.h"
#include "Gui/knob_callback.h"
#include "Gui/node_ui.h"
#include "Core/channels.h"
#include "Core/model.h"
#include "Core/displayFormat.h"
#include "Core/nodecache.h"
#include "Reader/Reader.h"
#include "Superviser/controler.h"
#include "Gui/timeline.h"
#include "Gui/viewerTab.h"
#include "Core/row.h"
#include "Gui/mainGui.h"
#include "Writer/Writer.h"
#include "Core/VideoEngine.h"
#include "Core/viewerNode.h"
using namespace std;
using namespace Powiter_Enums;
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
    _firstFrame = -1;
    _lastFrame = -1;
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
bool Node::merge_info(bool forReal){
	
    clear_info();
	int final_direction=0;
	ChannelMask chans;
    bool displayMode;
	for (int i =0 ; i < inputCount(); i++) {
        Node* parent = _parents[i];
        merge_frameRange(parent->getInfo()->firstFrame(),parent->getInfo()->lastFrame());
        
        if(forReal){
            final_direction+=parent->getInfo()->getYdirection();
            chans += parent->getInfo()->channels();
            ChannelMask neededChans = channelsNeeded(i);
            if ((neededChans & chans) != neededChans) {
                cout << parent->getName().toStdString() << " does not contain the channels needed by "
                << getName().toStdString()  << endl;
                neededChans.printOut();
                return false;
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
    final_direction/=_parents.size();
	_info->setChannels(chans);
    _info->rgbMode(displayMode);
    _info->setYdirection(final_direction);
    return true;
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


Node::Node(){
    _marked = false;
    _info = new Info;
    _hashValue=new Hash;
	
}
Hash* Node::getHash() const{return _hashValue;}

const std::vector<Knob*>& Node::getKnobs() const{return _knobsVector;}
void Node::addToKnobVector(Knob* knob){_knobsVector.push_back(knob);}
void Node::removeKnob(Knob* knob){
    for(U32 i = 0 ; i < _knobsVector.size() ; i++){
        if (knob == _knobsVector[i]) {
            _knobsVector.erase(_knobsVector.begin()+i);
            break;
        }
    }
}

const std::vector<Node*>& Node::getParents() const {return _parents;}
const std::vector<Node*>& Node::getChildren() const {return _children;}


int Node::getFreeSocketCount() const{return _freeSocketCount;}

void Node::addChild(Node* child){
    
    _children.push_back(child);
}
void Node::addParent(Node* parent){
    
    _parents.push_back(parent);
}
void Node::removeChild(Node* child){
    
    U32 i=0;
    while(i<_children.size()){
        if(_children[i]==child){
            _freeSocketCount++;
            _children.erase(_children.begin()+i);
            break;
        }
        i++;
    }
}
void Node::removeParent(Node* parent){
    
    U32 i=0;
    while(i<_parents.size()){
        if(_parents[i]==parent){
            _parents.erase(_parents.begin()+i);
            break;
        }
        i++;
    }
}

void Node::removeFromParents(){
    for(U32 i = 0 ; i < _parents.size() ; i++){
        _parents[i]->removeChild(this);
    }
}

void Node::removeFromChildren(){
    for(U32 i = 0 ; i < _children.size() ; i++){
        _children[i]->removeParent(this);
    }

}

void Node::releaseSocket(){
    if(!isOutputNode()){
        _freeSocketCount++;
    }
}
void Node::lockSocket(){
    if(!isOutputNode() && getFreeSocketCount() > 0){
        _freeSocketCount--;
    }
}
int Node::maximumSocketCount(){return 1;}
bool Node::isInputNode(){return false;}
bool Node::isOutputNode(){return false;}

void Node::initializeInputs(){
    initInputLabelsMap();
    applyLabelsToInputs();
    
}
int Node::maximumInputs(){return 1;}

int Node::minimumInputs(){return 1;}

Node* Node::input(int index){
    if((U32)index < _parents.size()){
        return _parents[index];
    }else{
        return NULL;
    }
}

const std::map<int, std::string>& Node::getInputLabels() const {return _inputLabelsMap;}

std::string Node::getLabel(int inputNb)  {
    return _inputLabelsMap[inputNb];}
QString Node::getName() {return _name;}
QMutex* Node::getMutex() const {return _mutex;}


void Node::setName(QString name){this->_name=name;}


/*To change label names : override setInputLabel to reflect what you want to have for input "inputNb" */
std::string Node::setInputLabel(int inputNb){
    string out;
    out.append(1,(char)(inputNb+65));
    return out;
}
void Node::applyLabelsToInputs(){
    for(U32 i=0;i<_inputLabelsMap.size();i++){
        std::map<int, std::string>::iterator it;
        it =_inputLabelsMap.find(i);
        _inputLabelsMap.erase(it);
        _inputLabelsMap[i] =setInputLabel(i);
    }
}
void Node::initInputLabelsMap(){
    int i=0;
    while(i<maximumInputs()){
        char str[2];
        str[0] =i+65;
        str[1]='\0';
        _inputLabelsMap[i]=str;
        i++;
    }
    
}



std::string Node::className(){return "Node_Abstract_Class";}

bool Node::validate(bool forReal){
    if(!isInputNode()){
        if(!merge_info(forReal)) return false;
    }
    _validate(forReal);
    preProcess();
    _nodeGUI->updateChannelsTooltip();
    return true;
}


void Node::computeTreeHash(std::vector<std::string> &alreadyComputedHash){
    for(U32 i =0 ; i < alreadyComputedHash.size();i++){
        if(alreadyComputedHash[i] == _name.toStdString())
            return;
    }
    _hashValue->reset();
    for(U32 i=0;i<_knobsVector.size();i++){
        _hashValue->appendKnobToHash(_knobsVector[i]);
    }
    _hashValue->appendQStringToHash(QString(className().c_str()));
    alreadyComputedHash.push_back(_name.toStdString());
    foreach(Node* parent,_parents){
        parent->computeTreeHash(alreadyComputedHash);
        _hashValue->appendNodeHashToHash(parent->getHash()->getHashValue());
    }
    _hashValue->computeHash();
}
bool Node::hashChanged(){
    U64 oldHash=_hashValue->getHashValue();
    vector<std::string> v;
    computeTreeHash(v);
    return oldHash!=_hashValue->getHashValue();
}
void Node::initKnobs(Knob_Callback *cb){
	this->_knobsCB=cb;
    cb->initNodeKnobsVector();
}
void Node::createKnobDynamically(){
    
	_knobsCB->createKnobDynamically();
}

std::string Node::description(){
    return "";
}

void Node::get(int y,int x,int r,ChannelSet channels,InputRow& row,bool keepCached){
    NodeCache* cache = NodeCache::getNodeCache();
    std::string filename;
    Reader* reader = dynamic_cast<Reader*>(this);
    if(reader){
        int current_frame;
        Writer* writer = dynamic_cast<Writer*>(ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG().getOutput());
        if(!writer)
            current_frame = reader->clampToRange(currentViewer->currentFrame());
        else
            current_frame = writer->currentFrame();
        filename = reader->getRandomFrameName(current_frame);
    }
    Row* out = 0;
    U64 key = _hashValue->getHashValue();
    pair<U64,Row*> entry = cache->get(key , filename, x, r, y, channels);
    if(entry.second && entry.first!=0) out = entry.second;
    if(out){
        entry.second->preventFromDeletion();
        row.setInternalRow(out);
        return;
    }else{
        if(cacheData()){
            out = cache->addRow(entry.first,x, r, y, channels, filename);
            if(keepCached){
                out->preventFromDeletion();
            }
            out->notifyCacheForDeletion();
            row.setInternalRow(out);
            if(!out) return;
        }else{
            out = new Row(x,y,r,channels);
            out->allocateRow();
        }
        engine(y, x, r, channels, out);
        row.setInternalRow(out);
        return;
    }
}

Node::~Node(){
    _parents.clear();
    _children.clear();
    _knobsVector.clear();
    _inputLabelsMap.clear();
    delete _hashValue;
    delete _info;
}
