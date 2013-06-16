//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
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

ostream& operator<< (ostream &out, Node &node){
    out << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
    out << "DUMPING INFO FOR :" << endl;
    out << "Node: "<<node.className() << endl;
    out << "------------------------------" << endl;
    out << "Parents :" << endl;
    foreach(Node* p,node.getParents()){
        out << "Parent: " << p->className();
    }
    out << endl;
    out << "------------------------------" << endl;
    out << "Children: " << endl;
    foreach(Node* p,node.getChildren()){
        out << "Child: " << p->className();
    }
    out << endl;
    out << "------------------------------" << endl;
    out << "Infos: " << endl;
    out << "info box: " << " x:" << node.getInfo()->x()
    << " y:" << node.getInfo()->y()
    << " r:" << node.getInfo()->right()
    << " t:" << node.getInfo()->top() << endl;
    out << "info channels: " << endl;
    ChannelSet infoset = node.getInfo()->channels();
    foreachChannels( z,infoset){
        out << getChannelName(z) << "\t" ;
    }
    out << endl;
    out << "------------------------------" << endl;
    ChannelSet outset = node.getOutputChannels();
    out << "Output Channels:" << endl;
    foreachChannels( z, outset){
        out << getChannelName(z) << "\t";
    }
    out << endl;
    out << "------------------------------" << endl;
    out << "Requested box: " << " x:" << node.getRequestedBox().x()
    << " y:" << node.getRequestedBox().y()
    << " r:" << node.getRequestedBox().right()
    << " t:" << node.getRequestedBox().top() << endl;
    out << "------------------------------" << endl;
    out << "Requested Channels:" << endl;
    ChannelSet requestedSet = node.getRequestedChannels();
    foreachChannels( z, requestedSet){
        out << getChannelName(z) << "\t";
    }
    out << endl;
    //    out << "------------------------------" << endl;
    //    out << "Rows contained in cache:\t";
    //    std::map<int,Row*>::iterator it;
    //    for( it = node.get_row_cache().begin(); it != node.get_row_cache().end();it++){
    //        out << (*it).first << " ";
    //    }
    //    out << endl;
    out << "------------------------------" << endl;
    return out;
}


void Node::copy_info(Node* parent,bool forReal){
	parent->validate(forReal);
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
    _modified = false;
}

void Node::merge_info(bool forReal){
	foreach(Node* parent,_parents){
		parent->validate(forReal);
	}
	int final_direction=0;
	Node* first=_parents[0];
	Format expectedFormat=first->getInfo()->getDisplayWindow();
	ChannelMask chans=_info->channels();
    bool displayMode=first->getInfo()->rgbMode();
	foreach(Node* parent,_parents){
		merge_frameRange(parent->getInfo()->firstFrame(),parent->getInfo()->lastFrame());
		final_direction+=parent->getInfo()->getYdirection();
		chans += parent->getInfo()->channels();
		if(parent->getInfo()->getDisplayWindow()!=expectedFormat){
			std::cout << "Warning: merge_info: inputs of" << className() << " have a different format " << std::endl;
		}
        if(parent->getInfo()->rgbMode()!=displayMode){
            std::cout << "Warning: merge_info: inputs of" << className() << " have a different display mode" << std::endl;
        }
	}
    final_direction/=_parents.size();
	_info->setChannels(chans);
    _info->rgbMode(displayMode);
    _info->setYdirection(final_direction);
}
void Node::merge_frameRange(int otherFirstFrame,int otherLastFrame){
	
    if (otherFirstFrame<_info->firstFrame())
    {
        _info->firstFrame(otherFirstFrame);
    }
    if(otherLastFrame>_info->lastFrame()){
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



Node::Node(const Node& ref):_parents(ref._parents),_children(ref._children),_inputLabelsMap(ref._inputLabelsMap),
_mutex(ref._mutex),_name(ref._name),_hashValue(ref._hashValue),_info(ref._info),
_freeSocketCount(ref._freeSocketCount),_outputChannels(ref._outputChannels),_requestedBox(ref._requestedBox),_requestedChannels(ref._requestedChannels),_marked(ref._marked){}

Node::Node(Node* ptr){
    _marked = false;
    _info = new Info;
    _hashValue=new Hash();
	_requestedChannels=Mask_None;
    _outputChannels = Mask_All;
    
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


int Node::getFreeOutputCount() const{return _freeSocketCount;}

void Node::addChild(Node* child){
    
    _children.push_back(child);
}
void Node::addParent(Node* parent){
    
    _parents.push_back(parent);
}
void Node::removeChild(Node* child){
    
    int i=0;
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
    
    int i=0;
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
    if(!isOutputNode() && getFreeOutputCount() > 0){
        _freeSocketCount--;
    }
}
void Node::lockSocket(){
    if(!isOutputNode()){
        _freeSocketCount++;
    }
}
void Node::setSocketCount(){_freeSocketCount=1;}
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

void Node::validate(bool forReal){
    _validate(forReal);
    _nodeGUI->updateChannelsTooltip();
}
void Node::_validate(bool forReal){
    
	
	if(_parents.size()==1){
		copy_info(_parents[0],forReal);
	}
	else if(_parents.size()>1){
		clear_info();
		merge_info(forReal);
	}
    setOutputChannels(Mask_All);

    
}

void Node::request(ChannelMask channels){
	
	_requestedChannels += channels;
	_request(_requestedChannels);
	
}
void Node::_request(ChannelMask channels){
	int i=0;
	foreach(Node* parent,_parents){
		in_channels(i,channels);
		parent->request(channels);
		++i;
	}
}


void Node::computeTreeHash(std::vector<std::string> &alreadyComputedHash){
    for(int i =0 ; i < alreadyComputedHash.size();i++){
        if(alreadyComputedHash[i] == _name.toStdString())
            return;
    }
    _hashValue->reset();
    for(int i=0;i<_knobsVector.size();i++){
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

void Node::get(int y,int x,int r,ChannelSet channels,InputRow& row){
    NodeCache* cache = NodeCache::getNodeCache();
    std::string filename;
    Reader* reader = dynamic_cast<Reader*>(this);
    if(reader){
        int current_frame;
        Writer* writer = dynamic_cast<Writer*>(ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG().getOutput());
        if(!writer)
            current_frame = reader->clampToRange(currentViewer->frameSeeker->currentFrame());
        else
            current_frame = writer->currentFrame();
        filename = reader->getRandomFrameName(current_frame);
    }
    Row* out = 0;
    U64 key = _hashValue->getHashValue();
    pair<U64,Row*> entry = cache->get(key , filename, x, r, y, channels);
    if(entry.second && entry.first!=0) out = entry.second;
    if(out){
        row.setInternalRow(out);
        return;
    }else{
        if(cacheData()){
            out = cache->add(entry.first,x, r, y, channels, filename);
            out->notifyCacheForDeletion();
            row.setInternalRow(out);
            if(!out) return;
        }else{
            out = new Row(x,y,r,channels);
            out->allocate();
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
