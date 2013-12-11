//  Natron
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
#include "Engine/Log.h"
#include "Engine/NodeSerialization.h"

#include "Readers/Reader.h"
#include "Writers/Writer.h"

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"

using namespace Natron;
using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;

namespace { // protect local classes in anonymous namespace

/*A key to identify an image rendered for this node.*/
struct ImageBeingRenderedKey{

    ImageBeingRenderedKey():_time(0),_view(0){}

    ImageBeingRenderedKey(int time,int view):_time(time),_view(view){}

    SequenceTime _time;
    int _view;

    bool operator==(const ImageBeingRenderedKey& other) const {
        return _time == other._time &&
                _view == other._view;
    }

    bool operator<(const ImageBeingRenderedKey& other) const {
        return _time < other._time ||
                _view < other._view;
    }
};

typedef std::multimap<ImageBeingRenderedKey,boost::shared_ptr<Image> > ImagesMap;

typedef std::map<Node*,std::pair<int,int> >::const_iterator OutputConnectionsIterator;
typedef OutputConnectionsIterator InputConnectionsIterator;

struct DeactivatedState{
    /*The output node was connected from inputNumber to the outputNumber of this...*/
    std::map<Node*,std::pair<int,int> > outputsConnections;

    /*The input node was connected from outputNumber to the inputNumber of this...*/
    std::map<Node*,std::pair<int,int> > inputConnections;
};

}

struct Node::Implementation {
    Implementation(AppInstance* app_,LibraryBinary* plugin_)
        : app(app_)
        , outputs()
        , previewInstance(NULL)
        , previewRenderTree(NULL)
        , previewMutex()
        , inputLabelsMap()
        , name()
        , deactivatedState()
        , activated(true)
        , nodeInstanceLock()
        , imagesBeingRenderedNotEmpty()
        , imagesBeingRendered()
        , plugin(plugin_)
        , renderInstances()
    {
    }

    AppInstance* app; // pointer to the model: needed to access the application's default-project's format
    std::multimap<int,Node*> outputs; //multiple outputs per slot
    EffectInstance* previewInstance;//< the instance used only to render a preview image
    RenderTree* previewRenderTree;//< the render tree used to render the preview
    mutable QMutex previewMutex;

    std::map<int, std::string> inputLabelsMap; // inputs name
    std::string name; //node name set by the user

    DeactivatedState deactivatedState;

    bool activated;
    QMutex nodeInstanceLock;
    QWaitCondition imagesBeingRenderedNotEmpty; //to avoid computing preview in parallel of the real rendering


    ImagesMap imagesBeingRendered; //< a map storing the ongoing render for this node
    LibraryBinary* plugin;
    std::map<RenderTree*,EffectInstance*> renderInstances;
};

Node::Node(AppInstance* app,LibraryBinary* plugin,const std::string& name)
    : QObject()
    , _inputs()
    , _liveInstance(NULL)
    , _imp(new Implementation(app,plugin))
{
    std::pair<bool,EffectBuilder> func = plugin->findFunction<EffectBuilder>("BuildEffect");
    if (func.first) {
        _liveInstance         = func.second(this);
        _imp->previewInstance = func.second(this);
    } else { //ofx plugin
        _liveInstance = appPTR->getOfxHost()->createOfxEffect(name,this);
        _imp->previewInstance = appPTR->getOfxHost()->createOfxEffect(name,this);
    }
    assert(_liveInstance);
    assert(_imp->previewInstance);

    _imp->previewInstance->setAsRenderClone();
    _imp->previewInstance->notifyProjectBeginKnobsValuesChanged(Natron::OTHER_REASON);
    _imp->previewInstance->initializeKnobs();
    _imp->previewInstance->notifyProjectEndKnobsValuesChanged(Natron::OTHER_REASON);
    _imp->previewRenderTree = new RenderTree(_imp->previewInstance);
    _imp->renderInstances.insert(std::make_pair(_imp->previewRenderTree,_imp->previewInstance));
}


Node::~Node()
{
    for (std::map<RenderTree*,EffectInstance*>::iterator it = _imp->renderInstances.begin(); it!=_imp->renderInstances.end(); ++it) {
        delete it->second;
    }
    delete _liveInstance;
    delete _imp->previewRenderTree;
    emit deleteWanted();
}

const std::map<int, std::string>& Node::getInputLabels() const
{
    return _imp->inputLabelsMap;
}

const Node::OutputMap& Node::getOutputs() const
{
    return _imp->outputs;
}

const std::string& Node::getName() const
{
    return _imp->name;
}

void Node::setName(const std::string& name)
{
    _imp->name = name;
    emit nameChanged(name.c_str());
}

AppInstance* Node::getApp() const
{
    return _imp->app;
}

bool Node::isActivated() const
{
    return _imp->activated;
}

void Node::onGUINameChanged(const QString& str){
    _imp->name = str.toStdString();
}

void Node::initializeKnobs(){
    _liveInstance->notifyProjectBeginKnobsValuesChanged(Natron::OTHER_REASON);
    _liveInstance->initializeKnobs();
    _liveInstance->notifyProjectEndKnobsValuesChanged(Natron::OTHER_REASON);
    emit knobsInitialized();
}

EffectInstance* Node::findOrCreateLiveInstanceClone(RenderTree* tree)
{
    if (isOutputNode()) {
        _liveInstance->updateInputs(tree);
        return _liveInstance;
    }
    EffectInstance* ret = 0;
    std::map<RenderTree*,EffectInstance*>::const_iterator it = _imp->renderInstances.find(tree);
    if (it != _imp->renderInstances.end()) {
        ret =  it->second;
    } else {
        ret = createLiveInstanceClone();
        _imp->renderInstances.insert(std::make_pair(tree, ret));
    }
    
    assert(ret);
    ret->clone(0); //the time parameter is meaningless here since we just create it.
    //ret->updateInputs(tree);
    return ret;

}

EffectInstance*  Node::createLiveInstanceClone()
{
    EffectInstance* ret = NULL;
    if (!isOpenFXNode()) {
        std::pair<bool,EffectBuilder> func = _imp->plugin->findFunction<EffectBuilder>("BuildEffect");
        assert(func.first);
        ret =  func.second(this);
    } else {
        ret = appPTR->getOfxHost()->createOfxEffect(_liveInstance->pluginID(),this);
    }
    assert(ret);
    ret->notifyProjectBeginKnobsValuesChanged(Natron::OTHER_REASON);
    ret->initializeKnobs();
    ret->notifyProjectEndKnobsValuesChanged(Natron::OTHER_REASON);
    ret->setAsRenderClone();
    return ret;
}

EffectInstance* Node::findExistingEffect(RenderTree* tree) const
{
    if (isOutputNode()) {
        return _liveInstance;
    }
    std::map<RenderTree*,EffectInstance*>::const_iterator it = _imp->renderInstances.find(tree);
    if (it!=_imp->renderInstances.end()) {
        return it->second;
    } else {
        return NULL;
    }
}

void Node::createKnobDynamically()
{
    emit knobsInitialized();
}


void Node::hasViewersConnected(std::list<ViewerInstance*>* viewers) const
{
    if(pluginID() == "Viewer") {
        ViewerInstance* thisViewer = dynamic_cast<ViewerInstance*>(_liveInstance);
        assert(thisViewer);
        std::list<ViewerInstance*>::const_iterator alreadyExists = std::find(viewers->begin(), viewers->end(), thisViewer);
        if(alreadyExists == viewers->end()){
            viewers->push_back(thisViewer);
        }
    } else {
        for (Node::OutputMap::const_iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
            if(it->second) {
                it->second->hasViewersConnected(viewers);
            }
        }
    }
}

void Node::initializeInputs()
{
    int inputCount = maximumInputs();
    for (int i = 0;i < inputCount;++i) {
        if (_inputs.find(i) == _inputs.end()) {
            _imp->inputLabelsMap.insert(make_pair(i,_liveInstance->inputLabel(i)));
            _inputs.insert(make_pair(i,(Node*)NULL));
        }
    }
    _liveInstance->updateInputs(NULL);
    emit inputsInitialized();
}

Node* Node::input(int index) const
{
    InputMap::const_iterator it = _inputs.find(index);
    if(it == _inputs.end()){
        return NULL;
    }else{
        return it->second;
    }
}


std::string Node::getInputLabel(int inputNb) const
{
    std::map<int,std::string>::const_iterator it = _imp->inputLabelsMap.find(inputNb);
    if (it == _imp->inputLabelsMap.end()) {
        return "";
    } else {
        return it->second;
    }
}


bool Node::isInputConnected(int inputNb) const
{
    InputMap::const_iterator it = _inputs.find(inputNb);
    if(it != _inputs.end()){
        return it->second != NULL;
    }else{
        return false;
    }
    
}

bool Node::hasOutputConnected() const
{
    return _imp->outputs.size() > 0;
}

bool Node::connectInput(Node* input,int inputNumber)
{
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

void Node::connectOutput(Node* output,int outputNumber )
{
    assert(output);
    _imp->outputs.insert(make_pair(outputNumber,output));
    _liveInstance->updateInputs(NULL);
}

int Node::disconnectInput(int inputNumber)
{
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

int Node::disconnectInput(Node* input)
{
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

int Node::disconnectOutput(Node* output)
{
    assert(output);
    for (OutputMap::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
        if (it->second == output) {
            int outputNumber = it->first;;
            _imp->outputs.erase(it);
            _liveInstance->updateInputs(NULL);
            return outputNumber;
        }
    }
    return -1;
}


/*After this call this node still knows the link to the old inputs/outputs
 but no other node knows this node.*/
void Node::deactivate()
{
    //first tell the gui to clear any persistent message link to this node
    clearPersistentMessage();

    /*Removing this node from the output of all inputs*/
    _imp->deactivatedState.inputConnections.clear();
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (it->second) {
            int outputNb = it->second->disconnectOutput(this);
            _imp->deactivatedState.inputConnections.insert(make_pair(it->second, make_pair(outputNb, it->first)));
            it->second = NULL;
        }
    }
    _imp->deactivatedState.outputsConnections.clear();
    for (OutputMap::iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
        if (!it->second) {
            continue;
        }
        int inputNb = it->second->disconnectInput(this);
        _imp->deactivatedState.outputsConnections.insert(make_pair(it->second, make_pair(inputNb, it->first)));
    }
    emit deactivated();
    _imp->activated = false;
    
}

void Node::activate()
{
    for (InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (!it->second) {
            continue;
        }
        InputConnectionsIterator found = _imp->deactivatedState.inputConnections.find(it->second);
        if (found == _imp->deactivatedState.inputConnections.end()) {
            cout << "Big issue while activating this node, canceling process." << endl;
            return;
        }
        /*InputNumber must be the same than the one we stored at disconnection time.*/
        assert(found->second.first == it->first);
        it->second->connectOutput(this,found->second.first);
    }
    for (OutputMap::const_iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
        if (!it->second) {
            continue;
        }
        OutputConnectionsIterator found = _imp->deactivatedState.outputsConnections.find(it->second);
        if (found == _imp->deactivatedState.outputsConnections.end()) {
            cout << "Big issue while activating this node, canceling process." << endl;
            return;
        }
        assert(found->second.second == it->first);
        it->second->connectInput(this,found->second.first);
    }
    emit activated();
    _imp->activated = true;
}



const Format& Node::getRenderFormatForEffect(const EffectInstance* effect) const
{
    if (effect == _liveInstance) {
        return getApp()->getProjectFormat();
    } else {
        for (std::map<RenderTree*,EffectInstance*>::const_iterator it = _imp->renderInstances.begin();
             it!=_imp->renderInstances.end();++it) {
            if (it->second == effect) {
                return it->first->getRenderFormat();
            }
        }
    }
    return getApp()->getProjectFormat();
}

int Node::getRenderViewsCountForEffect( const EffectInstance* effect) const
{
    if(effect == _liveInstance) {
        return getApp()->getCurrentProjectViewsCount();
    } else {
        for (std::map<RenderTree*,EffectInstance*>::const_iterator it = _imp->renderInstances.begin();
             it!=_imp->renderInstances.end();++it) {
            if (it->second == effect) {
                return it->first->renderViewsCount();
            }
        }
    }
    return getApp()->getCurrentProjectViewsCount();
}


boost::shared_ptr<Knob> Node::getKnobByDescription(const std::string& desc) const
{
    return _liveInstance->getKnobByDescription(desc);
}


boost::shared_ptr<Image> Node::getImageBeingRendered(SequenceTime time,int view) const
{
    ImagesMap::const_iterator it = _imp->imagesBeingRendered.find(ImageBeingRenderedKey(time,view));
    if (it!=_imp->imagesBeingRendered.end()) {
        return it->second;
    }
    return boost::shared_ptr<Image>();
}



static float clamp(float v, float min = 0.f, float max= 1.f){
    if(v > max) v = max;
    if(v < min) v = min;
    return v;
}

void Node::addImageBeingRendered(boost::shared_ptr<Image> image,SequenceTime time,int view )
{
    /*before rendering we add to the _imp->imagesBeingRendered member the image*/
    ImageBeingRenderedKey renderedImageKey(time,view);
    QMutexLocker locker(&_imp->nodeInstanceLock);
    _imp->imagesBeingRendered.insert(std::make_pair(renderedImageKey, image));
}

void Node::removeImageBeingRendered(SequenceTime time,int view )
{
    /*now that we rendered the image, remove it from the images being rendered*/
    
    QMutexLocker locker(&_imp->nodeInstanceLock);
    ImageBeingRenderedKey renderedImageKey(time,view);
    std::pair<ImagesMap::iterator,ImagesMap::iterator> it = _imp->imagesBeingRendered.equal_range(renderedImageKey);
    assert(it.first != it.second);
    _imp->imagesBeingRendered.erase(it.first);

    _imp->imagesBeingRenderedNotEmpty.wakeOne(); // wake up any preview thread waiting for render to finish
}

void Node::makePreviewImage(SequenceTime time,int width,int height,unsigned int* buf)
{

    QMutexLocker locker(&_imp->previewMutex); /// prevent 2 previews to occur at the same time since there's only 1 preview instance

    RectI rod;
    _imp->previewRenderTree->refreshTree(time);
    Natron::Status stat = _imp->previewInstance->getRegionOfDefinition(time, &rod);
    if (stat == StatFailed) {
        return;
    }
    int h,w;
    h = rod.height() < height ? rod.height() : height;
    w = rod.width() < width ? rod.width() : width;
    double yZoomFactor = (double)h/(double)rod.height();
    double xZoomFactor = (double)w/(double)rod.width();
    {
        QMutexLocker locker(&_imp->nodeInstanceLock);
        while(!_imp->imagesBeingRendered.empty()){
            _imp->imagesBeingRenderedNotEmpty.wait(&_imp->nodeInstanceLock);
        }
    }

    Log::beginFunction(getName(),"makePreviewImage");
    Log::print(QString("Time "+QString::number(time)).toStdString());

    stat =  _imp->previewRenderTree->preProcessFrame(time);
    if(stat == StatFailed){
        Log::print(QString("preProcessFrame returned StatFailed.").toStdString());
        Log::endFunction(getName(),"makePreviewImage");
        return;
    }

    RenderScale scale;
    scale.x = scale.y = 1.;
    boost::shared_ptr<const Image> img;

    // Exceptions are caught because the program can run without a preview,
    // but any exception in renderROI is probably fatal.
    try {
        img = _imp->previewInstance->renderRoI(time, scale, 0,rod);
    } catch (const std::exception& e) {
        qDebug() << "Error: Cannot create preview" << ": " << e.what();
        return;
    } catch (...) {
        qDebug() << "Error: Cannot create preview";
        return;
    }
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

    Log::endFunction(getName(),"makePreviewImage");
}

void Node::openFilesForAllFileKnobs()
{
    _liveInstance->openFilesForAllFileKnobs();
}

void Node::abortRenderingForEffect(EffectInstance* effect)
{
    for (std::map<RenderTree*,EffectInstance*>::iterator it = _imp->renderInstances.begin(); it!=_imp->renderInstances.end(); ++it) {
        if(it->second == effect){
            dynamic_cast<OutputEffectInstance*>(it->first->getOutput())->getVideoEngine()->abortRendering();
        }
    }
}


bool Node::isInputNode() const{
    return _liveInstance->isGenerator();
}


bool Node::isOutputNode() const
{
    return _liveInstance->isOutput();
}


bool Node::isInputAndProcessingNode() const
{
    return _liveInstance->isGeneratorAndFilter();
}


bool Node::isOpenFXNode() const
{
    return _liveInstance->isOpenFX();
}

const std::vector< boost::shared_ptr<Knob> >& Node::getKnobs() const
{
    return _liveInstance->getKnobs();
}

std::string Node::pluginID() const
{
    return _liveInstance->pluginID();
}

std::string Node::pluginLabel() const
{
    return _liveInstance->pluginLabel();
}

std::string Node::description() const
{
    return _liveInstance->description();
}

int Node::maximumInputs() const
{
    return _liveInstance->maximumInputs();
}

bool Node::makePreviewByDefault() const
{
    return _liveInstance->makePreviewByDefault();
}

void Node::togglePreview()
{
    _liveInstance->togglePreview();
}

bool Node::isPreviewEnabled() const
{
    return _liveInstance->isPreviewEnabled();
}

bool Node::aborted() const
{
    return _liveInstance->aborted();
}

void Node::setAborted(bool b)
{
    _liveInstance->setAborted(b);
}

void Node::drawOverlay()
{
    _liveInstance->drawOverlay();
}

bool Node::onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos)
{
    return _liveInstance->onOverlayPenDown(viewportPos, pos);
}

bool Node::onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos)
{
    return _liveInstance->onOverlayPenMotion(viewportPos, pos);
}

bool Node::onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos)
{
    return _liveInstance->onOverlayPenUp(viewportPos, pos);
}

void Node::onOverlayKeyDown(QKeyEvent* e)
{
    return _liveInstance->onOverlayKeyDown(e);
}

void Node::onOverlayKeyUp(QKeyEvent* e)
{
    return _liveInstance->onOverlayKeyUp(e);
}

void Node::onOverlayKeyRepeat(QKeyEvent* e)
{
    return _liveInstance->onOverlayKeyRepeat(e);
}

void Node::onOverlayFocusGained()
{
    return _liveInstance->onOverlayFocusGained();
}

void Node::onOverlayFocusLost()
{
    return _liveInstance->onOverlayFocusLost();
}


bool Node::message(MessageType type,const std::string& content) const
{
    switch (type) {
        case INFO_MESSAGE:
            informationDialog(getName(), content);
            return true;
        case WARNING_MESSAGE:
            warningDialog(getName(), content);
            return true;
        case ERROR_MESSAGE:
            errorDialog(getName(), content);
            return true;
        case QUESTION_MESSAGE:
            return questionDialog(getName(), content) == Yes;
        default:
            return false;
    }
}

void Node::setPersistentMessage(MessageType type,const std::string& content)
{
    if (!getApp()->isBackground()) {
        //if the message is just an information, display a popup instead.
        if (type == INFO_MESSAGE) {
            message(type,content);
            return;
        }
        QString message;
        message.append(getName().c_str());
        if (type == ERROR_MESSAGE) {
            message.append(" error: ");
        } else if(type == WARNING_MESSAGE) {
            message.append(" warning: ");
        }
        message.append(content.c_str());
        emit persistentMessageChanged((int)type,message);
    }else{
        std::cout << "Persistent message" << std::endl;
        std::cout << content << std::endl;
    }
}

void Node::clearPersistentMessage()
{
    if(!getApp()->isBackground()){
        emit persistentMessageCleared();
    }
}

void Node::serialize(NodeSerialization* serializationObject) const {
    serializationObject->initialize(this);
}

InspectorNode::InspectorNode(AppInstance* app,LibraryBinary* plugin,const std::string& name)
    : Node(app,plugin,name)
    , _inputsCount(1)
    , _activeInput(0)
{}


InspectorNode::~InspectorNode(){
    
}

bool InspectorNode::connectInput(Node* input,int inputNumber,bool autoConnection) {
    assert(input);
    if(input == this){
        return false;
    }
    
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
            ((inputAlreadyConnected!=_inputs.end()) )){
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
    }
}

