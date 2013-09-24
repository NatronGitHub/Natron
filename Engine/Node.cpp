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
#include "Engine/OfxNode.h"
#include "Engine/TimeLine.h"


#include "Readers/Reader.h"
#include "Writers/Writer.h"

#include "Global/AppManager.h"



using namespace std;
using namespace Powiter;

namespace {
    void Hash64_appendKnob(Hash64* hash, const Knob& knob){
        const std::vector<U64>& values= knob.getHashVector();
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
    _firstFrame = 0;
    _lastFrame = 0;
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
void Node::merge_info(bool doFullWork){
	
    clear_info();
	int final_direction=0;
	ChannelSet chans;
    bool displayMode = info().rgbMode();
    int count = 0;
	for (Node::InputMap::const_iterator it = getInputs().begin();
         it!=getInputs().end();++it) {
        if(!it->second)
            continue;
        if(className() == "Viewer"){
            ViewerNode* n = dynamic_cast<ViewerNode*>(this);
            if(n->activeInput()!=it->first)
                continue;
        }
        Node* input = it->second;
        if(count > 0)
            merge_frameRange(input->info().firstFrame(),input->info().lastFrame());
        else{
            _info.set_firstFrame(input->info().firstFrame());
            _info.set_lastFrame(input->info().lastFrame());
        }
        if(doFullWork){
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
        ++count;
    }
    if(isOutputNode()){
        OutputNode* node =  dynamic_cast<OutputNode*>(this);
        assert(node);
        node->setFrameRange(_info.firstFrame(), _info.lastFrame());
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


Node::Node(Model* model)
: QObject()
, _model(model)
, _info()
, _marked(false)
, _inputLabelsMap()
, _name()
, _hashValue()
, _requestedBox()
, _outputs()
, _inputs()
, _knobs()
, _undoStack(new QUndoStack)
, _renderAborted(false)
{
}

Node::~Node(){
    for (U32 i = 0; i < _knobs.size(); ++i) {
        delete _knobs[i];
    }
}

void Node::deleteNode(){
    emit deleteWanted();
}


void Node::initializeInputs(){
    int inputCount = maximumInputs();
    for(int i = 0;i < inputCount;++i){
        if(_inputs.find(i) == _inputs.end()){
            _inputLabelsMap.insert(make_pair(i,setInputLabel(i)));
            _inputs.insert(make_pair(i,(Node*)NULL));
        }
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

bool Node::validate(bool doFullWork){
    if(!isInputNode()){
        merge_info(doFullWork);
    }
    if(!_validate(doFullWork))
        return false;
    emit channelsChanged();
    return true;
}

bool Node::connectInput(Node* input,int inputNumber) {
    assert(input);
    InputMap::iterator it = _inputs.find(inputNumber);
    if (it == _inputs.end()) {
        return false;
    }else{
        if (it->second == NULL) {
            _inputs.erase(it);
            _inputs.insert(make_pair(inputNumber,input));
            emit inputChanged(inputNumber);
            return true;
        }else{
            return false;
        }
    }
}

void Node::connectOutput(Node* output,int outputNumber ){
    assert(output);
    disconnectOutput(output);
    _outputs.insert(make_pair(outputNumber,output));
}

int Node::disconnectInput(int inputNumber) {
    InputMap::iterator it = _inputs.find(inputNumber);
    if (it == _inputs.end() || it->second == NULL) {
        return -1;
    } else {
        _inputs.erase(it);
        _inputs.insert(make_pair(inputNumber, (Node*)NULL));
        emit inputChanged(inputNumber);
        return inputNumber;
    }
}

int Node::disconnectInput(Node* input) {
    assert(input);
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (it->second != input) {
            return -1;
        } else {
            int inputNumber = it->first;
            _inputs.erase(it);
            _inputs.insert(make_pair(inputNumber, (Node*)NULL));
            emit inputChanged(inputNumber);
            return inputNumber;
        }
    }
    return -1;
}

int Node::disconnectOutput(Node* output) {
    assert(output);
    for (OutputMap::iterator it = _outputs.begin(); it != _outputs.end(); ++it) {
        if (it->second == output) {
            int outputNumber = it->first;;
            _outputs.erase(it);
            return outputNumber;
        }
    }
    return -1;
}


/*After this call this node still knows the link to the old inputs/outputs
 but no other node knows this node.*/
void Node::deactivate(){
    /*Removing this node from the output of all inputs*/
    _deactivatedState._inputConnections.clear();
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if(it->second){
            int outputNb = it->second->disconnectOutput(this);
            _deactivatedState._inputConnections.insert(make_pair(it->second, make_pair(outputNb, it->first)));
            it->second = NULL;
        }
    }
    Node* firstChild = 0;
    _deactivatedState._outputsConnections.clear();
    for (OutputMap::iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if(!it->second)
            continue;
        if(it == _outputs.begin()){
            firstChild = it->second;
        }
        int inputNb = it->second->disconnectInput(this);
        _deactivatedState._outputsConnections.insert(make_pair(it->second, make_pair(inputNb, it->first)));
    }
    emit deactivated();
    if(firstChild){
        _model->triggerAutoSaveOnNextEngineRun();
        _model->checkViewersConnection();
    }
}

void Node::activate(){
    for (InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if(!it->second)
            continue;
        InputConnectionsIterator found = _deactivatedState._inputConnections.find(it->second);
        if(found == _deactivatedState._inputConnections.end()){
            cout << "Big issue while activating this node, canceling process." << endl;
            return;
        }
        /*InputNumber must be the same than the one we stored at disconnection time.*/
        assert(found->second.first == it->first);
        it->second->connectOutput(this,found->second.first);
    }
    Node* firstChild = 0;
    for (OutputMap::const_iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if(!it->second)
            continue;
        if(it == _outputs.begin()){
            firstChild = it->second;
        }
        OutputConnectionsIterator found = _deactivatedState._outputsConnections.find(it->second);
        if(found == _deactivatedState._outputsConnections.end()){
            cout << "Big issue while activating this node, canceling process." << endl;
            return;
        }
        assert(found->second.first == it->first);
        it->second->connectInput(this,found->second.first);
    }
    emit activated();

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
        Hash64_appendKnob(&_hashValue,*_knobs[i]);
    }
    /*append the node name*/
    Hash64_appendQString(&_hashValue, QString(className().c_str()));
    /*mark this node as already been computed*/
    alreadyComputedHash.push_back(_name);
    
    /*Recursive call to parents and add their hash key*/
    for (InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if(it->second){
            if(className() == "Viewer"){
                ViewerNode* v = dynamic_cast<ViewerNode*>(this);
                if(it->first!=v->activeInput())
                    continue;
            }
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
    NodeCache* cache = appPTR->getNodeCache();
    int current_frame;
    const VideoEngine::DAG& dag = _info.executingEngine()->getCurrentDAG();

    OutputNode* outputNode = dag.getOutput();
    assert(outputNode);
    current_frame = outputNode->currentFrame();
    std::string filename =  getRandomFrameName(current_frame).toStdString();
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

ViewerNode* Node::hasViewerConnected(Node* node){
    Node* out = 0;
    bool ok=false;
    _hasViewerConnected(node,&ok,out);
    if (ok) {
        return dynamic_cast<ViewerNode*>(out);
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
    return _outputs.size() > 0;
}

void Node::removeKnob(Knob* knob) {
    assert(knob);
    //_knobs.erase( std::remove(_knobs.begin(), _knobs.end(), knob), _knobs.end()); // erase all elements with value knobs
    std::vector<Knob*>::iterator it = std::find(_knobs.begin(), _knobs.end(), knob);
    if (it != _knobs.end()) {
        _knobs.erase(it);
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

int Node::canMakePreviewImage(){
    if (className() == "Reader") {
        return 1;
    }
    if(isOpenFXNode()){
        OfxNode* n = dynamic_cast<OfxNode*>(this);
        if(n->canHavePreviewImage())
            return 2;
        else
            return 0;
    }else{
        return 0;
    }
}

void Node::onFrameRangeChanged(int first,int last){
    _info.set_firstFrame(first);
    _info.set_lastFrame(last);
}

OutputNode::OutputNode(Model* model)
: Node(model)
, _timeline(new TimeLine)
, _videoEngine(new VideoEngine(_model))
{
}

OutputNode::~OutputNode(){
    _videoEngine->quitEngineThread();
    delete _videoEngine;
}

void OutputNode::setFrameRange(int first, int last) {
    _timeline->setFirstFrame(first);
    _timeline->setLastFrame(last);
}

void OutputNode::seekFrame(int frame) {
    return _timeline->seekFrame(frame);
}

void OutputNode::incrementCurrentFrame() {
    return _timeline->incrementCurrentFrame();
}

int OutputNode::currentFrame() const {
    return _timeline->currentFrame();
}

int OutputNode::firstFrame() const {
    return _timeline->firstFrame();
}

int OutputNode::lastFrame() const {
    return _timeline->lastFrame();
}

void OutputNode::updateDAG(bool initViewer){
    _videoEngine->changeDAGAndStartEngine(this, initViewer);
}
