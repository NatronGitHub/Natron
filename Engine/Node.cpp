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

#include <boost/bind.hpp>

#include <QtGui/QRgb>

#include "Engine/Hash64.h"
#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/Row.h"
#include "Engine/Knob.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Engine/Project.h"
#include "Engine/EffectInstance.h"

#include "Readers/Reader.h"
#include "Writers/Writer.h"

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"

using namespace Powiter;
using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;


Node::Node(AppInstance* app,Powiter::LibraryBinary* plugin,const std::string& name)
: QObject()
, _app(app)
, _outputs()
, _inputs()
, _liveInstance(NULL)
, _inputLabelsMap()
, _name()
, _knobs()
, _deactivatedState()
, _markedByTopologicalSort(false)
, _activated(true)
, _nodeInstanceLock()
, _imagesBeingRenderedNotEmpty()
, _imagesBeingRendered()
, _plugin(plugin)
, _renderInstances()
{
    
    try{
        std::pair<bool,EffectBuilder> func = plugin->findFunction<EffectBuilder>("BuildEffect");
        if(func.first){
            _liveInstance = func.second(this);
        }else{ //ofx plugin
            _liveInstance = appPTR->getOfxHost()->createOfxEffect(name,this);
        }
    }catch(const std::exception& e){
        throw e;
    }
    assert(_liveInstance);
}


Node::~Node(){
    for (std::map<RenderTree*,EffectInstance*>::iterator it = _renderInstances.begin(); it!=_renderInstances.end(); ++it) {
        delete it->second;
    }
    delete _liveInstance;
    emit deleteWanted();
}


void Node::initializeKnobs(){
    _liveInstance->initializeKnobs();
    emit knobsInitialized();
}

Powiter::EffectInstance* Node::findOrCreateLiveInstanceClone(RenderTree* tree){
    EffectInstance* ret = 0;
    if(isOutputNode() && !isOpenFXNode()){
        ret = _liveInstance;
    }else{
        std::map<RenderTree*,EffectInstance*>::const_iterator it = _renderInstances.find(tree);
        if(it != _renderInstances.end()){
            ret =  it->second;
        }else{
            if(!isOpenFXNode()){
                std::pair<bool,EffectBuilder> func = _plugin->findFunction<EffectBuilder>("BuildEffect");
                assert(func.first);
                ret =  func.second(this);
            }else{
                ret = appPTR->getOfxHost()->createOfxEffect(_liveInstance->className(),this);
                
            }
            assert(ret);
            ret->initializeKnobs();
            ret->setAsRenderClone();
            _renderInstances.insert(std::make_pair(tree, ret));
        }
        ret->clone(); // copy all knobs values
    }
    assert(ret);
    ret->updateInputs(tree);
    ret->invalidateHash();
    return ret;

}
Powiter::EffectInstance* Node::findExistingEffect(RenderTree* tree) const{
    std::map<RenderTree*,EffectInstance*>::const_iterator it = _renderInstances.find(tree);
    if(it!=_renderInstances.end()){
        return it->second;
    }else{
        return NULL;
    }
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

ViewerInstance* Node::hasViewerConnected() {
    Node* out = 0;
    bool ok = false;
    _hasViewerConnected(this,&ok,out);
    if (ok) {
        return dynamic_cast<ViewerInstance*>(out->getLiveInstance());
    }else{
        return NULL;
    }
    
}

void Node::initializeInputs(){
    int inputCount = maximumInputs();
    for(int i = 0;i < inputCount;++i){
        if(_inputs.find(i) == _inputs.end()){
            _inputLabelsMap.insert(make_pair(i,_liveInstance->setInputLabel(i)));
            _inputs.insert(make_pair(i,(Node*)NULL));
        }
    }
    _liveInstance->updateInputs(NULL);
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


std::string Node::getInputLabel(int inputNb) const{
    std::map<int,std::string>::const_iterator it = _inputLabelsMap.find(inputNb);
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
            _liveInstance->updateInputs(NULL);
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
    _liveInstance->updateInputs(NULL);
}

int Node::disconnectInput(int inputNumber) {
    InputMap::iterator it = _inputs.find(inputNumber);
    if (it == _inputs.end() || it->second == NULL) {
        return -1;
    } else {
        _inputs.erase(it);
        _inputs.insert(make_pair(inputNumber, (Node*)NULL));
        emit inputChanged(inputNumber);
        _liveInstance->updateInputs(NULL);
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
            _liveInstance->updateInputs(NULL);
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
            _liveInstance->updateInputs(NULL);
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
    _activated = false;
    
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
    _activated = true;
}



const Format& Node::getRenderFormatForEffect(const Powiter::EffectInstance* effect) const{
    if(effect == _liveInstance){
        return getApp()->getProjectFormat();
    }else{
        for(std::map<RenderTree*,Powiter::EffectInstance*>::const_iterator it = _renderInstances.begin();
            it!=_renderInstances.end();++it){
            if(it->second == effect){
                return it->first->getRenderFormat();
            }
        }
    }
    return getApp()->getProjectFormat();
}

int Node::getRenderViewsCountForEffect( const Powiter::EffectInstance* effect) const{
    if(effect == _liveInstance){
        return getApp()->getCurrentProjectViewsCount();
    }else{
        for(std::map<RenderTree*,Powiter::EffectInstance*>::const_iterator it = _renderInstances.begin();
            it!=_renderInstances.end();++it){
            if(it->second == effect){
                return it->first->renderViewsCount();
            }
        }
    }
    return getApp()->getCurrentProjectViewsCount();
}


Knob* Node::getKnobByDescription(const std::string& desc) const{
    for(U32 i = 0; i < _knobs.size() ; ++i){
        if (_knobs[i]->getDescription() == desc) {
            return _knobs[i];
        }
    }
    return NULL;
}


boost::shared_ptr<Powiter::Image> Node::getImageBeingRendered(SequenceTime time,int view) const{
    ImagesMap::const_iterator it = _imagesBeingRendered.find(ImageBeingRenderedKey(time,view));
    if(it!=_imagesBeingRendered.end()){
        return it->second;
    }
    return boost::shared_ptr<Powiter::Image>();
}



static float clamp(float v, float min = 0.f, float max= 1.f){
    if(v > max) v = max;
    if(v < min) v = min;
    return v;
}

void Node::addImageBeingRendered(boost::shared_ptr<Powiter::Image> image,SequenceTime time,int view ){
    /*before rendering we add to the _imagesBeingRendered member the image*/
    ImageBeingRenderedKey renderedImageKey(time,view);
    QMutexLocker locker(&_nodeInstanceLock);
    _imagesBeingRendered.insert(std::make_pair(renderedImageKey, image));
}

void Node::removeImageBeingRendered(SequenceTime time,int view ){
    /*now that we rendered the image, remove it from the images being rendered*/
    
    QMutexLocker locker(&_nodeInstanceLock);
    ImageBeingRenderedKey renderedImageKey(time,view);
    std::pair<ImagesMap::iterator,ImagesMap::iterator> it = _imagesBeingRendered.equal_range(renderedImageKey);
    assert(it.first != it.second);
    _imagesBeingRendered.erase(it.first);

    _imagesBeingRenderedNotEmpty.wakeOne(); // wake up any preview thread waiting for render to finish
}

void Node::makePreviewImage(SequenceTime time,int width,int height,unsigned int* buf){
    RectI rod;
    _liveInstance->getRegionOfDefinition(time, &rod);
    int h,w;
    rod.height() < height ? h = rod.height() : h = height;
    rod.width() < width ? w = rod.width() : w = width;
    double yZoomFactor = (double)h/(double)rod.height();
    double xZoomFactor = (double)w/(double)rod.width();
    Powiter::Status stat =  _liveInstance->preProcessFrame(time);
    if(stat == StatFailed)
        return;
    {
        QMutexLocker locker(&_nodeInstanceLock);
        while(!_imagesBeingRendered.empty()){
            _imagesBeingRenderedNotEmpty.wait(&_nodeInstanceLock);
        }
    }
    
    _liveInstance->computeHash(std::vector<U64>());
    RenderScale scale;
    scale.x = scale.y = 1.;
    boost::shared_ptr<const Powiter::Image> img = _liveInstance->renderRoI(time, scale, 0,rod);
    for (int i=0; i < h; ++i) {
        double y = (double)i/yZoomFactor;
        int nearestY = (int)(y+0.5);
        
        U32 *dst_pixels = buf + width*(h-1-i);
        const float* src_pixels = img->pixelAt(0, nearestY);

        for(int j = 0;j < w;++j) {
            double x = (double)j/xZoomFactor;
            int nearestX = (int)(x+0.5);
            float r = clamp(Color::linearrgb_to_srgb(src_pixels[nearestX*4]));
            float g = clamp(Color::linearrgb_to_srgb(src_pixels[nearestX*4+1]));
            float b = clamp(Color::linearrgb_to_srgb(src_pixels[nearestX*4+2]));
            float a = clamp(src_pixels[nearestX*4+3]);
            dst_pixels[j] = qRgba(r*255, g*255, b*255, a*255);
            
        }
    }
}

void Node::openFilesForAllFileKnobs(){
    _liveInstance->openFilesForAllFileKnobs();
}

void Node::abortRenderingForEffect(Powiter::EffectInstance* effect){
    for (std::map<RenderTree*,EffectInstance*>::iterator it = _renderInstances.begin(); it!=_renderInstances.end(); ++it) {
        if(it->second == effect){
            dynamic_cast<OutputEffectInstance*>(it->first->getOutput())->getVideoEngine()->abortRendering();
        }
    }
}


bool Node::isInputNode() const{
    return _liveInstance->isGenerator();
}


bool Node::isOutputNode() const{
    return _liveInstance->isOutput();
}


bool Node::isInputAndProcessingNode() const{
    return _liveInstance->isGeneratorAndFilter();
}


bool Node::isOpenFXNode() const{
    return _liveInstance->isOpenFX();
}

const std::vector<Knob*>& Node::getKnobs() const{
    return _liveInstance->getKnobs();
}

std::string Node::className() const{
    return _liveInstance->className();
}


std::string Node::description() const{
    return _liveInstance->description();
}

int Node::maximumInputs() const {
    return _liveInstance->maximumInputs();
}

bool Node::makePreviewByDefault() const{
    return _liveInstance->makePreviewByDefault();
}

bool Node::aborted() const { return _liveInstance->aborted(); }

void Node::setAborted(bool b){ _liveInstance->setAborted(b); }

void Node::drawOverlay(){ _liveInstance->drawOverlay(); }

bool Node::onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos){
    return _liveInstance->onOverlayPenDown(viewportPos, pos);
}

bool Node::onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos){
    return _liveInstance->onOverlayPenMotion(viewportPos, pos);
}

bool Node::onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos){
    return _liveInstance->onOverlayPenUp(viewportPos, pos);
}

void Node::onOverlayKeyDown(QKeyEvent* e){
    return _liveInstance->onOverlayKeyDown(e);
}

void Node::onOverlayKeyUp(QKeyEvent* e){
    return _liveInstance->onOverlayKeyUp(e);
}

void Node::onOverlayKeyRepeat(QKeyEvent* e){
    return _liveInstance->onOverlayKeyRepeat(e);
}

void Node::onOverlayFocusGained(){
    return _liveInstance->onOverlayFocusGained();
}

void Node::onOverlayFocusLost(){
    return _liveInstance->onOverlayFocusLost();
}



InspectorNode::InspectorNode(AppInstance* app,Powiter::LibraryBinary* plugin,const std::string& name)
: Node(app,plugin,name)
, _inputsCount(1)
, _activeInput(0)
{}


InspectorNode::~InspectorNode(){
    
}

bool InspectorNode::connectInput(Node* input,int inputNumber,bool autoConnection) {
    assert(input);
    InputMap::iterator found = _inputs.find(inputNumber);
//    if(/*input->className() == "Viewer" && */found!=_inputs.end() && !found->second){
//        return false;
//    }
    /*Adding all empty edges so it creates at least the inputNB'th one.*/
    while(_inputsCount <= inputNumber){
        tryAddEmptyInput();
    }
    //#1: first case, If the inputNB of the viewer is already connected & this is not
    // an autoConnection, just refresh it*/
    InputMap::iterator inputAlreadyConnected = _inputs.end();
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (it->second == input) {
            inputAlreadyConnected = it;
            break;
        }
    }
    
    if(found!=_inputs.end() && found->second && !autoConnection &&
       ((inputAlreadyConnected!=_inputs.end()) || (inputAlreadyConnected==_inputs.end()/* && input->className() == "Viewer"*/))){
        setActiveInputAndRefresh(found->first);
        return false;
    }
    /*#2:second case: Before connecting the appropriate edge we search for any other edge connected with the
     selected node, in which case we just refresh the already connected edge.*/
    for (InputMap::const_iterator i = _inputs.begin(); i!=_inputs.end(); ++i) {
        if(i->second && i->second == input){
            setActiveInputAndRefresh(i->first);
            return false;
        }
    }
    if (found != _inputs.end()) {
        _inputs.erase(found);
        _inputs.insert(make_pair(inputNumber,input));
        emit inputChanged(inputNumber);
        tryAddEmptyInput();
        _liveInstance->updateInputs(NULL);
        return true;
    }
    return false;
}
bool InspectorNode::tryAddEmptyInput(){
    if(_inputs.size() <= 10){
        if(_inputs.size() > 0){
            InputMap::const_iterator it = _inputs.end();
            --it;
            if(it->second != NULL){
                addEmptyInput();
                return true;
            }
        }else{
            addEmptyInput();
            return true;
        }
    }
    return false;
}
void InspectorNode::addEmptyInput(){
    _activeInput = _inputsCount-1;
    ++_inputsCount;
    initializeInputs();
}

void InspectorNode::removeEmptyInputs(){
    /*While there're NULL inputs at the tail of the map,remove them.
     Stops at the first non-NULL input.*/
    while (_inputs.size() > 1) {
        InputMap::iterator it = _inputs.end();
        --it;
        if(it->second == NULL){
            InputMap::iterator it2 = it;
            --it2;
            if(it2->second!=NULL)
                break;
            //int inputNb = it->first;
            _inputs.erase(it);
            --_inputsCount;
            _liveInstance->updateInputs(NULL);
            emit inputsInitialized();
        }else{
            break;
        }
    }
}
int InspectorNode::disconnectInput(int inputNumber){
    int ret = Node::disconnectInput(inputNumber);
    if(ret!=-1){
        removeEmptyInputs();
        _activeInput = _inputs.size()-1;
        initializeInputs();
    }
    return ret;
}

int InspectorNode::disconnectInput(Node* input){
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if(it->second == input){
            return disconnectInput(it->first);
        }
    }
    return -1;
}

void InspectorNode::setActiveInputAndRefresh(int inputNb){
    InputMap::iterator it = _inputs.find(inputNb);
    if(it!=_inputs.end() && it->second!=NULL){
        _activeInput = inputNb;
        _liveInstance->requestRender();
    }
}

