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

#include <QtConcurrentMap>
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
#include "Engine/Image.h"
#include "Engine/Project.h"

#include "Readers/Reader.h"
#include "Writers/Writer.h"

#include "Global/AppManager.h"

using namespace Powiter;
using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;

namespace {
    void Hash64_appendKnob(Hash64* hash, const Knob& knob){
        const std::vector<U64>& values = knob.getHashVector();
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
, _renderAborted(false)
, _hashValue()
, _inputLabelsMap()
, _name()
, _knobs()
, _deactivatedState()
, _markedByTopologicalSort(false)
, _activated(true)
,_nodeInstanceLock()
,_imagesBeingRenderedNotEmpty()
,_imagesBeingRendered()
,_betweenBeginEndParamChanged(false)
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
    std::string out;
    out.append(1,(char)(inputNb+65));
    return out;
}
const std::string Node::getInputLabel(int inputNb) const{
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

Knob* Node::getKnobByDescription(const std::string& desc) const{
    for(U32 i = 0; i < _knobs.size() ; ++i){
        if (_knobs[i]->getDescription() == desc) {
            return _knobs[i];
        }
    }
    return NULL;
}



boost::shared_ptr<const Powiter::Image> Node::getImage(SequenceTime time,RenderScale scale,int view){
    const Powiter::Cache<Image>& cache = appPTR->getNodeCache();
    //making key with a null RoD since the hash key doesn't take the RoD into account
    //we'll get our image back without the RoD in the key
    Powiter::ImageKey params = Powiter::Image::makeKey(hash().value(), time,scale,view,Box2D());
    shared_ptr<const Image > entry = cache.get(params);
    //the entry MUST be in the cache since we rendered it before and kept the pointer
    //to avoid it being evicted.
    assert(entry);
    return entry;
}

void Node::computeHashAndLockKnobs(std::vector<std::string> &alreadyComputedHash){
    /*If we already computed its hash,return*/
    for(U32 i =0 ; i < alreadyComputedHash.size();++i) {
        if(alreadyComputedHash[i] == getName())
            return;
    }
    /*Clear the values left to compute the hash key*/
    Hash64 hash;
    /*append all values stored in knobs*/
    for(U32 i = 0 ; i< _knobs.size();++i) {
        _knobs[i]->lock();
        Hash64_appendKnob(&hash,*(_knobs[i]));
    }
    /*append the node name*/
    Hash64_appendQString(&hash, QString(className().c_str()));
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
            it->second->computeHashAndLockKnobs(alreadyComputedHash);
            hash.append(it->second->hash().value());
        }
    }
    /*Compute the hash key*/
    hash.computeHash();
    _hashValue.setLocalData(hash);

}
void Node::unlockAllKnobs(){
    for(U32 i = 0 ; i< _knobs.size();++i) {
        _knobs[i]->unlock();
    }
}

Powiter::Status Node::getRegionOfDefinition(SequenceTime time,Box2D* rod) {
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
    return StatReplyDefault;
}

Node::RoIMap Node::getRegionOfInterest(SequenceTime /*time*/,RenderScale /*scale*/,const Box2D& renderWindow){
    RoIMap ret;
    for(InputMap::const_iterator it = _inputs.begin() ; it != _inputs.end() ; ++it){
        if (it->second) {
            ret.insert(std::make_pair(it->second, renderWindow));
        }
    }
    return ret;
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

std::vector<Box2D> Node::splitRectIntoSmallerRect(const Box2D& rect,int splitsCount){
    std::vector<Box2D> ret;
    int averagePixelsPerSplit = std::ceil(double(rect.area()) / (double)splitsCount);
    /*if the splits happen to have less pixels than 1 scan-line contains, just do scan-line rendering*/
    if(averagePixelsPerSplit < rect.width()){
        for (int i = rect.bottom(); i < rect.top(); ++i) {
            ret.push_back(Box2D(rect.left(),i,rect.right(),i+1));
        }
    }else{
        //we round to the ceil
        int scanLinesCount = std::ceil((double)averagePixelsPerSplit/(double)rect.width());
        int startBox = rect.bottom();
        while((startBox + scanLinesCount) < rect.top()){
            ret.push_back(Box2D(rect.left(),startBox,rect.right(),startBox+scanLinesCount));
            startBox += scanLinesCount;
        }
        if(startBox < rect.top()){
            ret.push_back(Box2D(rect.left(),startBox,rect.right(),rect.top()));
        }
    }
    return ret;
}

void Node::tiledRenderingFunctor(SequenceTime time,
                                 RenderScale scale,
                                 const Box2D& roi,
                                 int view,
                                 Hash64 hashValue,
                                 boost::shared_ptr<Powiter::Image> output){

    _hashValue.setLocalData(hashValue);
    render(time, scale, roi,view, output);
    output->markForRendered(roi);
}
boost::shared_ptr<const Powiter::Image> Node::renderRoI(SequenceTime time,RenderScale scale,int view,const Box2D& renderWindow){
    Powiter::ImageKey key = Powiter::Image::makeKey(hash().value(), time, scale,view,Box2D());
    /*look-up the cache for any existing image already rendered*/
    boost::shared_ptr<Image> image = boost::const_pointer_cast<Image>(appPTR->getNodeCache().get(key));
    /*if not cached, we store the freshly allocated image in this member*/
    if(!image){
        /*before allocating     it we must fill the RoD of the image we want to render*/
        getRegionOfDefinition(time, &key._rod);
        int cost = 0;
        /*should data be stored on a physical device ?*/
        if(shouldRenderedDataBePersistent()){
            cost = 1;
        }
        /*allocate a new image*/
        image = appPTR->getNodeCache().newEntry(key,key._rod.area()*4,cost);
    }
    /*before rendering we add to the _imagesBeingRendered member the image*/
    ImageBeingRenderedKey renderedImageKey(time,view);
    {
        QMutexLocker locker(&_nodeInstanceLock);
        _imagesBeingRendered.insert(std::make_pair(renderedImageKey, image));
    }
    /*now that we have our image, we check what is left to render. If the list contains only
     null Box2Ds then we already rendered it all*/
    std::list<Box2D> rectsToRender = image->getRestToRender(renderWindow);
    if(rectsToRender.size() != 1 || !rectsToRender.begin()->isNull()){
        for (std::list<Box2D>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
            RoIMap inputsRoi = getRegionOfInterest(time, scale, *it);
            std::list<boost::shared_ptr<const Powiter::Image> > inputImages;
            /*we render each input first and store away their image in the inputImages list
             in order to maintain a shared_ptr use_count > 1 so the cache doesn't attempt
             to remove them.*/
            for (RoIMap::const_iterator it2 = inputsRoi.begin(); it2!= inputsRoi.end(); ++it2) {
                inputImages.push_back(it2->first->renderRoI(time, scale,view, it2->second));
            }
            /*depending on the thread-safety of the plug-in we render with a different
             amount of threads*/
            Node::RenderSafety safety = renderThreadSafety();
            if(safety == UNSAFE){
                QMutex* pluginLock = appPTR->getMutexForPlugin(className().c_str());
                assert(pluginLock);
                pluginLock->lock();
                render(time, scale, *it,view, image);
                pluginLock->unlock();
                image->markForRendered(*it);
            }else if(safety == INSTANCE_SAFE){
                _nodeInstanceLock.lock();
                render(time, scale, *it,view, image);
                _nodeInstanceLock.unlock();
                image->markForRendered(*it);
            }else{ // fully_safe, we do multi-threaded rendering on small tiles
                std::vector<Box2D> splitRects = splitRectIntoSmallerRect(*it, QThread::idealThreadCount());
                QtConcurrent::blockingMap(splitRects,
                                          boost::bind(&Node::tiledRenderingFunctor,this,time,scale,_1,view,hash(),image));
            }
        }
    }
    
    /*now that we rendered the image, remove it from the images being rendered*/
    {
        QMutexLocker locker(&_nodeInstanceLock);
        ImagesMap::iterator it = _imagesBeingRendered.find(renderedImageKey);
        assert(it != _imagesBeingRendered.end());
        //if another thread is rendering the same image, leave it
        //use count = 3 :
        // the image local var, the _imagesBeingRendered member and the ptr living in the cache
        if(it->second.use_count() == 3){
            _imagesBeingRendered.erase(it);
        }
        _imagesBeingRenderedNotEmpty.wakeOne(); // wake up any preview thread waiting for render to finish
    }
    //we released the input image and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();
    return image;
}


boost::shared_ptr<Powiter::Image> Node::getImageBeingRendered(SequenceTime time,int view) const{
    ImagesMap::const_iterator it = _imagesBeingRendered.find(ImageBeingRenderedKey(time,view));
    if(it!=_imagesBeingRendered.end()){
        return it->second;
    }
    return boost::shared_ptr<Powiter::Image>();
}

void OutputNode::ifInfiniteclipBox2DToProjectDefault(Box2D* rod) const{
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
    {
        QMutexLocker locker(&_nodeInstanceLock);
        while(!_imagesBeingRendered.empty()){
            _imagesBeingRenderedNotEmpty.wait(&_nodeInstanceLock);
        }
    }
    
    std::vector<std::string> alreadyComputedHashs;
    computeHashAndLockKnobs(alreadyComputedHashs);
    getApp()->lockProjectParams();
    RenderScale scale;
    scale.x = scale.y = 1.;
    boost::shared_ptr<const Powiter::Image> img = renderRoI(time, scale, 0,rod);
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
    unlockAllKnobs();
    getApp()->unlockProjectParams();
}


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
