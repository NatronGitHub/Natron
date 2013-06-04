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
#include "Core/channels.h"
#include "Core/model.h"
#include "Core/displayFormat.h"
#include "Superviser/PwStrings.h"

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
    out << "Requested box: " << " x:" << node.get_requested_box().x()
    << " y:" << node.get_requested_box().y()
    << " r:" << node.get_requested_box().right()
    << " t:" << node.get_requested_box().top() << endl;
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


void Node::Info::add_to_channels(Channel z){
    _channels+=z;
}
void Node::Info::add_to_channels(ChannelMask &m){
	_channels+=m;
}

void Node::Info::turnOffChannel(Channel z){
    _channels-=z;
}
void Node::Info::turnOffChannels(ChannelMask &m){
	_channels-=m;
}

void Node::copy_info(){
	Node* parent=parents[0];
	_info->firstFrame(parent->getInfo()->firstFrame());
	_info->lastFrame(parent->getInfo()->lastFrame());
	_info->setYdirection(parent->getInfo()->getYdirection());
	_info->setDisplayWindow(parent->getInfo()->getDisplayWindow());
	_info->set_channels(parent->getInfo()->channels());
	_info->merge(*(parent->getInfo()));
    _info->rgbMode(parent->getInfo()->rgbMode());
}
void Node::clear_info(){
	_info->firstFrame(-1);
	_info->lastFrame(-1);
	_info->setYdirection(0);
	_info->set_channels(Mask_None);
    _info->x(0);
    _info->y(0);
    _info->right(0);
    _info->top(0);
    requested_channels=Mask_None;
    requested_box.set(0, 0, 0, 0);

}
void Node::merge_info(){
	
	int final_direction=0;
	Node* first=parents[0];
	Format expectedFormat=first->getInfo()->getDisplayWindow();
	ChannelMask chans=_info->channels();
    bool displayMode=first->getInfo()->rgbMode();
	foreach(Node* parent,parents){
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
    final_direction/=parents.size();
	_info->set_channels(chans);
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



Node::Node(const Node& ref):parents(ref.parents),inputNb(ref.inputNb),children(ref.children),inputLabels(ref.inputLabels),
    _mutex(ref._mutex),name(ref.name),hashValue(ref.hashValue),_info(ref._info),
    freeOutputNb(ref.freeOutputNb),output_channels(ref.output_channels),requested_box(ref.requested_box),requested_channels(ref.requested_channels),_marked(ref._marked){}

Node::Node(Node* ptr){
    _marked = false;
    _info = new Info;
    hashValue=new Hash();
	requested_channels=Mask_None;

}
Hash* Node::getHash() const{return hashValue;}

std::vector<Knob*> Node::getKnobs() const{return knobs;}
void Node::add_knob_to_vector(Knob* knob){knobs.push_back(knob);}
std::vector<Node*> Node::getParents() const {return parents;}
std::vector<Node*> Node::getChildren() const {return children;}

bool Node::hasOutput(){return true;}

int Node::getFreeOutputNb() const{return freeOutputNb;}

void Node::addChild(Node* child){

    children.push_back(child);
}
void Node::addParent(Node* parent){

    parents.push_back(parent);
}
void Node::removeChild(Node* child){

    int i=0;
    while(i<children.size()){
        Node* c=children[i];
        if(c->getName()==child->getName()){
            Node* tmp=children[i];
            children[i]=children[children.size()-1];
            children[children.size()-1]=tmp;
            children.pop_back();

            break;
        }
        i++;
    }
}
void Node::removeParent(Node* parent){

    int i=0;
    while(i<parents.size()){
        Node* p=parents[i];
        if(p->getName()==parent->getName()){
            Node* tmp=parents[i];
            parents[i]=parents[parents.size()-1];
            parents[parents.size()-1]=tmp;
            parents.pop_back();

            break;
        }
        i++;
    }
}


void Node::decrementFreeOutputNb(){
    if(hasOutput()){
        freeOutputNb--;
    }
}
void Node::incrementFreeOutputNb(){
    if(hasOutput()){
        freeOutputNb++;
    }
}
void Node::setOutputNb(){freeOutputNb=1;}
bool Node::isInputNode(){return false;}
bool Node::isOutputNode(){return false;}

void Node::_inputs(){
    inputNb=inputs();
    initInputsLabels();
    _setLabels();
    
}
int Node::inputs(){return 1;}

int Node::getInputsNb() const {return inputNb;}
const std::map<int, std::string>& Node::getInputLabels() const {return inputLabels;}

std::string Node::getLabel(int inputNb)  {
    return inputLabels[inputNb];}
QString Node::getName() {return name;}
QMutex* Node::getMutex() const {return _mutex;}


void Node::setName(QString name){this->name=name;}


/*To change label names : override setInputLabel to reflect what you want to have for input "inputNb" */
std::string Node::setInputLabel(int inputNb){
    string out;
    out.append(1,(char)(inputNb+65));
    return out;
}
void Node::_setLabels(){
    for(U32 i=0;i<inputLabels.size();i++){
        std::map<int, std::string>::iterator it;
        it =inputLabels.find(i);
        inputLabels.erase(it);
        inputLabels[i] =setInputLabel(i);
    }
}
void Node::initInputsLabels(){
    int i=0;
    while(i<inputNb){
        char str[2];
        str[0] =i+65;
        str[1]='\0';
        inputLabels[i]=str;
        i++;
    }
 
}



std::string Node::className(){return "Node_Abstract_Class";}

void Node::validate(bool first_time){
	/*_validate(bool for_real)
		check intersection between requested and info,if
		some channels are missing from info,it sets them to 0*/
	if(first_time){
		_validate(first_time);
		first_time=false;
	}


}
void Node::_validate(bool first_time){

	foreach(Node* parent,parents){
		parent->validate(first_time);
	}
	if(parents.size()==1){
		copy_info();
	}
	else if(parents.size()>1){
		clear_info();
		merge_info();
	}
	set_output_channels(Mask_All);

}

void Node::request(int y,int top,int offset, int range,ChannelMask channels){
	/*set_requested_channels
		set_requested_box
		validate
		if(not in cache box(channels))
			_request(channels,box)
			setup cache*/
	requested_channels += channels;
	requested_box.merge(offset,y,range,top);
	
	validate(true);

	/* check if channels requested are present*/
	
//	foreachChannels( z,channels){
//		if((z & _info->channels())==0){
//			//std::cout<< "(Request) Channel " << getChannelName(z) << " is requested but does not exist in the output of " << class_name() << std::endl;
//		}
//	}
	/*check if the area requested is defined*/
//	if(!_info->isContained(requested_box)){
//		std::cout << "(Request) The requested area to " <<  class_name() << " has no data for this operator "
//        << ": data window(x: " << _info->x() << " y: " << _info->y() << " range: " << _info->range() << " top: "
//        << _info->top() << " ) , requested(x: "  << requested_box.x() << " y :" << requested_box.y()
//        << " range: " << requested_box.range() << " top: " << requested_box.top() << " )"<< std::endl;
//	}

	_request(y,top,offset,range,requested_channels);
	
	
}
void Node::_request(int y,int top,int offset,int range,ChannelMask channels){
	int i=0;
	foreach(Node* parent,parents){
		in_channels(i,channels);
		parent->request(y,top,offset,range,channels);
		++i;
	}
}

void Node::engine(int y,int offset,int range,ChannelMask channels,Row* out){}


void Node::computeTreeHash(std::vector<std::string> &alreadyComputedHash){
    for(int i =0 ; i < alreadyComputedHash.size();i++){
        if(alreadyComputedHash[i] == name.toStdString())
            return;
    }
    hashValue->reset();
    for(int i=0;i<knobs.size();i++){
        hashValue->appendKnobToHash(knobs[i]);
    }
    hashValue->appendQStringToHash(QString(className().c_str()));
    alreadyComputedHash.push_back(name.toStdString());
    foreach(Node* parent,parents){
        parent->computeTreeHash(alreadyComputedHash);
        hashValue->appendNodeHashToHash(parent->getHash()->getHashValue());
    }
    hashValue->computeHash();
}
bool Node::hashChanged(){
    U64 oldHash=hashValue->getHashValue();
    vector<std::string> v;
    computeTreeHash(v);
    return oldHash!=hashValue->getHashValue();
}
void Node::initKnobs(Knob_Callback *cb){
	this->knob_cb=cb;
    cb->initNodeKnobsVector();
}
void Node::createKnobDynamically(){

	knob_cb->createKnobDynamically();
}

std::string Node::description(){
    return "";
}

Node::~Node(){
    parents.clear();
    children.clear();
    knobs.clear();
    inputLabels.clear();
    delete hashValue;
    delete _info;
}
