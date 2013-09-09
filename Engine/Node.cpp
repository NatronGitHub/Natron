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
#include <QUndoStack>
#include <QUndoCommand>

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
	for (Node::InputMap::const_iterator it = getInputs().begin();
         it!=getInputs().end();++it) {
        
        Node* input = it->second;
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
    U32 size = getInputs().size();
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


Node::Node(Model* model):
_model(model),
_info(),
_marked(false)
{
	
}

Node::~Node(){
    
    
    for (OutputMap::iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        _app->disconnect(this, it->second);
    }
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        _app->disconnect(it->second, this);
    }
    for (U32 i = 0; i < _knobs.size(); ++i) {
        delete _knobs[i];
    }

    delete _undoStack;
}

void Node::deleteNode(){
    emit deleteWanted();
}


void Node::initializeInputs(){
    int inputCount = maximumInputs();
    for(int i = 0;i < inputCount;++i){
        _inputLabelsMap.insert(make_pair(i,setInputLabel(i)));
        _inputs.insert(make_pair(i,(NodeInstance*)NULL));
    }
    emit inputsInitialized();
}
const std::string Node::getInputLabel(int inputNb) const{
    map<int,string>::const_iterator it = _inputLabelsMap.find(inputNb);
    if(it == _inputLabelsMap.end()){
        return "";
    }else{
        return it->second;
    }
}

Node* Node::input(int index) const{
    InputMap::const_iterator it = _inputs.find(index);
    if(it == _inputs.end()){
        return NULL;
    }else{
        return it->second;
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
    
    //_instance->updateNodeChannelsGui(_info.channels());
    emit channelsChanged();
    return true;
}

bool Node::connectInput(Node* input,int inputNumber){
    InputMap::iterator it = _inputs.find(inputNumber);
    if (it == _inputs.end()) {
        return false;
    }else{
        if (it->second == NULL) {
//            if(!_gui->connectEdge(inputNumber, input->_gui)){
//                return false;
//            }
            emit inputChanged(inputNumber);
            _inputs.erase(it);
            _inputs.insert(make_pair(inputNumber,input));
            return true;
        }else{
            return false;
        }
    }
}

void Node::connectOutput(Node* output,int outputNumber ){
    OutputMap::iterator it = _outputs.find(outputNumber);
    if (it != _outputs.end()) {
        _outputs.erase(it);
    }
    _outputs.insert(make_pair(outputNumber,output));
}

bool Node::disconnectInput(int inputNumber){
    InputMap::const_iterator it = _inputs.find(inputNumber);
    if (it == _inputs.end()) {
        return false;
    }else{
        if(it->second == NULL){
            return false;
        }else{
//            if(!_gui->connectEdge(inputNumber, NULL)){
//                return false;
//            }
            emit inputChanged(inputNumber);
            _inputs.insert(make_pair(inputNumber, (Node*)NULL));
            return true;
        }
    }
}

bool Node::disconnectInput(Node* input){
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (it->second == input) {
//            if(!_gui->connectEdge(it->first, NULL)){
//                return false;
//            }
            emit inputChanged(it->first);
            _inputs.erase(it);
            _inputs.insert(make_pair(it->first, (NodeInstance*)NULL));
            return true;
        }else{
            return false;
        }
    }
    return false;
}

bool Node::disconnectOutput(int outputNumber){
    OutputMap::iterator it = _inputs.find(outputNumber);
    if (it == _outputs.end()) {
        return false;
    }else{
        if(it->second == NULL){
            return false;
        }else{
            _outputs.erase(it);
            _outputs.insert(make_pair(outputNumber, (Node*)NULL));
            return true;
        }
    }
    
}

bool Node::disconnectOutput(Node* output){
    for (OutputMap::iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if (it->second == output) {
            _outputs.erase(it);
            _outputs.insert(make_pair(it->first, (Node*)NULL));
            return true;
        }else{
            return false;
        }
    }
    return false;
}


void Node::deactivate(){
    // _gui->deactivate();
    emit deactivated();
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if(it->second)
            _app->disconnect(it->second,this);
    }
    Node* firstChild = 0;
    for (OutputMap::iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if(!it->second)
            continue;
        if(it == _outputs.begin()){
            firstChild = it->second;
        }
        _app->disconnect(this, it->second);
    }
    if(firstChild){
        _model->triggerAutoSaveOnNextEngineRun();
        _model->checkViewersConnection();
    }
}

void Node::activate(){
    // _gui->activate();
    emit activated();
    for (InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        _app->connect(it->first,it->second, this);
    }
    Node* firstChild = 0;
    for (OutputMap::const_iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if(it == _outputs.begin()){
            firstChild = it->second;
        }
        if(!it->second)
            continue;
        int inputNb = 0;
        for (InputMap::const_iterator it2 = it->second->getInputs().begin();
             it2 != it->second->getInputs().end(); ++it2) {
            if (it2->second == this) {
                inputNb = it2->first;
                break;
            }
        }
        _app->connect(inputNb,this, it->second);
    }
    if(firstChild){
        _model->triggerAutoSaveOnNextEngineRun();
        _model->checkViewersConnection();
    }
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
    for(U32 i = 0 ; i< _knobs.size();++i) {
        Hash64_appendKnob(&_hashValue,_knobs[i]);
    }
    /*append the node name*/
    Hash64_appendQString(&_hashValue, QString(className().c_str()));
    /*mark this node as already been computed*/
    alreadyComputedHash.push_back(_name);
    
    /*Recursive call to parents and add their hash key*/
    for (InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if(it->second){
            it->second->computeTreeHash(alreadyComputedHash);
            _hashValue.append(it->second->hash().value());
        }
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
    emit knobsInitialied();
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
        const OutputMap& outputs = node->getOutputs();
        for (OutputMap::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
            if(it->second)
                _hasViewerConnected(it->second,ok,out);
        }
    }
}
bool Node::isInputConnected(int inputNb) const{
    InputMap::const_iterator it = _inputs.find(inputNb);
    if(it != _inputs.end()){
        return it->second != NULL;
    }else{
        return false;
    }
    
}

bool Node::hasOutputConnected() const{
    for(OutputMap::const_iterator it = _outputs.begin();it!=_outputs.end();++it){
        if(it->second){
            return true;
        }
    }
    return false;
}

void Node::removeKnob(Knob* knob){
    for (U32 i = 0; i < _knobs.size(); ++i) {
        if (_knobs[i] == knob) {
            _knobs.erase(_knobs.begin()+i);
            break;
        }
    }
}

void Node::initializeKnobs(){
    initKnobs();
    emit knobsInitialied();
}

void Node::pushUndoCommand(QUndoCommand* command){
    _undoStack->push(command);
    emit canUndoChanged(_undoStack->canUndo());
    emit canRedoChanged(_undoStack->canRedo());
}
void Node::undoCommand(){
    _undoStack->undo();
    emit canUndoChanged(_undoStack->canUndo());
    emit canRedoChanged(_undoStack->canRedo());
}
void Node::redoCommand(){
    _undoStack->redo();
    emit canUndoChanged(_undoStack->canUndo());
    emit canRedoChanged(_undoStack->canRedo());
}

OutputNode::OutputNode(Model* model):
Node(model),
_mutex(new QMutex),
_openGLCondition(new QWaitCondition),
_videoEngine(new VideoEngine(_model,_openGLCondition,_mutex))
{
}

OutputNode::~OutputNode(){
    _videoEngine->quitEngineThread();
    delete _videoEngine;
    delete _mutex;
    delete _openGLCondition;
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

void OutputNode::updateDAG(bool isViewer,bool initViewer){
    _videoEngine->resetAndMakeNewDag(this,isViewer);
    if(isViewer){
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
        if(hasInputDifferentThanReader || hasFrames){
            _videoEngine->repeatSameFrame(initViewer);
        }else{
            dynamic_cast<ViewerNode*>(this)->disconnectViewer();
        }
    }

}