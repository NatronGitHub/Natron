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


#include <QtGui/QRgb>

#include "Engine/Hash64.h"
#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"
#include "Engine/Row.h"
#include "Engine/Knob.h"
#include "Engine/OfxNode.h"
#include "Engine/TimeLine.h"
#include "Engine/Lut.h"


#include "Readers/Reader.h"
#include "Writers/Writer.h"

#include "Global/AppManager.h"



using namespace std;
using namespace Powiter;
using namespace boost;
namespace {
    void Hash64_appendKnob(Hash64* hash, const Knob& knob){
        const std::vector<U64>& values= knob.getHashVector();
        for(U32 i=0;i<values.size();++i) {
            hash->append(values[i]);
        }
    }
}

Node::Node(AppInstance* app)
: QObject()
, _app(app)
, _outputs()
, _inputs()
, _inputLabelsMap()
, _name()
, _knobs()
, _deactivatedState()
, _hashValue()
, _markedByTopologicalSort(false)
, _renderAborted(false)
{
}

Node::~Node(){
    for (U32 i = 0; i < _knobs.size(); ++i) {
        _knobs[i]->deleteKnob();
    }
}

void Node::deleteNode(){
    emit deleteWanted();
}


void Node::initializeKnobs(){
    initKnobs();
    emit knobsInitialized();
}
void Node::createKnobDynamically(){
    emit knobsInitialized();
}

/*called by hasViewerConnected(Node*) */
static void _hasViewerConnected(Node* node,bool* ok,Node*& out){
    if (*ok == true) {
        return;
    }
    if(node->className() == "Viewer"){
        out = node;
        *ok = true;
    }else{
        const Node::OutputMap& outputs = node->getOutputs();
        for (Node::OutputMap::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
            if(it->second)
                _hasViewerConnected(it->second,ok,out);
        }
    }
}

ViewerNode* Node::hasViewerConnected()  {
    Node* out = 0;
    bool ok=false;
    _hasViewerConnected(this,&ok,out);
    if (ok) {
        return dynamic_cast<ViewerNode*>(out);
    }else{
        return NULL;
    }
    
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

Node* Node::input(int index) const{
    InputMap::const_iterator it = _inputs.find(index);
    if(it == _inputs.end()){
        return NULL;
    }else{
        return it->second;
    }
}

std::string Node::setInputLabel(int inputNb) const {
    string out;
    out.append(1,(char)(inputNb+65));
    return out;
}
const std::string Node::getInputLabel(int inputNb) const{
    map<int,string>::const_iterator it = _inputLabelsMap.find(inputNb);
    if(it == _inputLabelsMap.end()){
        return "";
    }else{
        return it->second;
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
            continue;
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
    _deactivatedState._outputsConnections.clear();
    for (OutputMap::iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if(!it->second)
            continue;
        int inputNb = it->second->disconnectInput(this);
        _deactivatedState._outputsConnections.insert(make_pair(it->second, make_pair(inputNb, it->first)));
    }
    emit deactivated();
    
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
    for (OutputMap::const_iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if(!it->second)
            continue;
        
        OutputConnectionsIterator found = _deactivatedState._outputsConnections.find(it->second);
        if(found == _deactivatedState._outputsConnections.end()){
            cout << "Big issue while activating this node, canceling process." << endl;
            return;
        }
        assert(found->second.first == it->first);
        it->second->connectInput(this,found->second.first);
    }
    emit activated();
    
}




const Format& Node::getProjectDefaultFormat() const{
    return getApp()->getProjectFormat();
}
void Node::removeKnob(Knob* knob){
    for(U32 i = 0; i < _knobs.size() ; ++i){
        if (_knobs[i] == knob) {
            _knobs.erase(_knobs.begin()+i);
            break;
        }
    }
}


/************************OUTPUT NODE*****************************************/
OutputNode::OutputNode(AppInstance* app)
: Node(app)
, _videoEngine()
{
    
    _videoEngine.reset(new VideoEngine(this));
}

void OutputNode::updateTreeAndRender(bool initViewer){
    _videoEngine->updateTreeAndContinueRender(initViewer);
}
void OutputNode::refreshAndContinueRender(bool initViewer){
    _videoEngine->refreshAndContinueRender(initViewer);
}
/************************NODE INSTANCE*****************************************/


boost::shared_ptr<const Row> Node::get(SequenceTime time,int y,int left,int right,const ChannelSet& channels) {
    const Powiter::Cache<Row>& cache = appPTR->getNodeCache();
    RowKey params = Row::makeKey(_hashValue.value(), time, left, y, right,channels );
    shared_ptr<const Row> entry = cache.get(params);
    if (entry) {
        return entry;
    }

    shared_ptr<Row> out;
    if (cacheData()) {
        shared_ptr<CachedValue<Row> > newCacheEntry = cache.newEntry(params,
                                                                     (right-left) * channels.size(),
                                                                     0);
        out = newCacheEntry->getObject();
        render(time,out.get());
    } else {
        out.reset(new Row(left,y,right,channels));
        render(time,out.get());
    }

    return out;
}

void Node::computeTreeHash(std::vector<std::string> &alreadyComputedHash){
    /*If we already computed its hash,return*/
    for(U32 i =0 ; i < alreadyComputedHash.size();++i) {
        if(alreadyComputedHash[i] == getName())
            return;
    }
    /*Clear the values left to compute the hash key*/
    _hashValue.reset();
    /*append all values stored in knobs*/
    for(U32 i = 0 ; i< _knobs.size();++i) {
        Hash64_appendKnob(&_hashValue,*(_knobs[i]));
    }
    /*append the node name*/
    Hash64_appendQString(&_hashValue, QString(className().c_str()));
    /*mark this node as already been computed*/
    alreadyComputedHash.push_back(getName());
    
    /*Recursive call to parents and add their hash key*/
    for (Node::InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if(it->second){
            if(className() == "Viewer"){
                ViewerNode* v = dynamic_cast<ViewerNode*>(this);
                if(it->first != v->activeInput())
                    continue;
            }
            it->second->computeTreeHash(alreadyComputedHash);
            _hashValue.append(it->second->hash().value());
        }
    }
    /*Compute the hash key*/
    _hashValue.computeHash();
}

Powiter::Status Node::getRegionOfDefinition(SequenceTime time,Box2D* rod,Format* displayWindow) {
    for(InputMap::const_iterator it = _inputs.begin() ; it != _inputs.end() ; ++it){
        if (it->second) {
            Box2D inputRod;
            Status st = it->second->getRegionOfDefinition(time, &inputRod);
            if(st == StatFailed)
                return st;
            if(it == _inputs.begin()){
                *rod = inputRod;
            }else{
                rod->merge(inputRod);
            }
        }
    }
    if(displayWindow)
        displayWindow->set(rod->left(), rod->bottom(), rod->right(), rod->top());
    return StatReplyDefault;
}

void Node::getFrameRange(SequenceTime *first,SequenceTime *last){
    for(InputMap::const_iterator it = _inputs.begin() ; it != _inputs.end() ; ++it){
        if (it->second) {
            SequenceTime inpFirst,inpLast;
            it->second->getFrameRange(&inpFirst, &inpLast);
            if(it == _inputs.begin()){
                *first = inpFirst;
                *last = inpLast;
            }else{
                if (inpFirst < *first) {
                    *first = inpFirst;
                }
                if (inpLast > *last) {
                    *last = inpLast;
                }
            }
        }
    }
}
void Node::ifInfiniteclipBox2DToProjectDefault(Box2D* rod) const{
    /*If the rod is infinite clip it to the project's default*/
    const Format& projectDefault = getProjectDefaultFormat();
    if(rod->left() == kOfxFlagInfiniteMin || rod->left() == -std::numeric_limits<double>::infinity()){
        rod->set_left(projectDefault.left());
    }
    if(rod->bottom() == kOfxFlagInfiniteMin || rod->bottom() == -std::numeric_limits<double>::infinity()){
        rod->set_bottom(projectDefault.bottom());
    }
    if(rod->right() == kOfxFlagInfiniteMax || rod->right() == std::numeric_limits<double>::infinity()){
        rod->set_right(projectDefault.right());
    }
    if(rod->top() == kOfxFlagInfiniteMax || rod->top()  == std::numeric_limits<double>::infinity()){
        rod->set_top(projectDefault.top());
    }
    
}


static float clamp(float v, float min = 0.f, float max= 1.f){
    if(v > max) v = max;
    if(v < min) v = min;
    return v;
}


void Node::makePreviewImage(SequenceTime time,int width,int height,unsigned int* buf){
    Box2D rod;
    getRegionOfDefinition(time, &rod);
    int h,w;
    rod.height() < height ? h = rod.height() : h = height;
    rod.width() < width ? w = rod.width() : w = width;
    double yZoomFactor = (double)h/(double)rod.height();
    double xZoomFactor = (double)w/(double)rod.width();
    Powiter::Status stat =  preProcessFrame(time);
    if(stat == StatFailed)
        return;
    for (int i=0; i < h; ++i) {
        double y = (double)i/yZoomFactor;
        int nearestY = (int)(y+0.5);
        
        /*get() calls render and also caches the row!*/
        ChannelSet channels(Mask_RGBA);
        boost::shared_ptr<const Row> row = get(time, nearestY, rod.left(), rod.right(), channels);
        
        U32 *dst_pixels = buf + width*(h-1-i);
        
        const float* red = row->begin(Channel_red);
        const float* green = row->begin(Channel_green);
        const float* blue = row->begin(Channel_blue);
        const float* alpha = row->begin(Channel_alpha);
        for(int j = 0;j < w;++j) {
            double x = (double)j/xZoomFactor;
            int nearestX = (int)(x+0.5);
            float r = red ? clamp(Color::linearrgb_to_srgb(red[nearestX])) : 0.f;
            float g = green ? clamp(Color::linearrgb_to_srgb(green[nearestX])) : 0.f;
            float b = blue ? clamp(Color::linearrgb_to_srgb(blue[nearestX])) : 0.f;
            float a = alpha ? clamp(alpha[nearestX]) : 1.f;
            dst_pixels[j] = qRgba(r*255, g*255, b*255, a*255);
            
        }
        
    }
    
}