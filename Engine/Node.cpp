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
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"

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
        , computingPreview(false)
        , computingPreviewCond()
        , pluginInstanceMemoryUsed(0)
        , memoryUsedMutex()
        , mustQuitProcessing(false)
        , mustQuitProcessingMutex()
        , mustQuitProcessingCond()
        , isUsingInputs(false)
        , isUsingInputsCond()
        , renderInstancesSharedMutex()
        , perFrameMutexesLock()
        , renderInstancesFullySafePerFrameMutexes()
    {
    }

    AppInstance* app; // pointer to the app: needed to access the application's default-project's format
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
    
    bool computingPreview;
    QWaitCondition computingPreviewCond;
    
    size_t pluginInstanceMemoryUsed; //< global count on all EffectInstance's of the memory they use.
    QMutex memoryUsedMutex; //< protects _pluginInstanceMemoryUsed
    
    bool mustQuitProcessing;
    QMutex mustQuitProcessingMutex;
    QWaitCondition mustQuitProcessingCond;
    
    bool isUsingInputs; //< set when a render tree is actively using the connections informations
    QWaitCondition isUsingInputsCond;
    
    QMutex renderInstancesSharedMutex; //< see INSTANCE_SAFE in EffectInstance::renderRoI
                                       //only 1 clone can render at any time
    
    QMutex perFrameMutexesLock; //< protects renderInstancesFullySafePerFrameMutexes
    std::map<int,boost::shared_ptr<QMutex> > renderInstancesFullySafePerFrameMutexes; //< see FULLY_SAFE in EffectInstance::renderRoI
                                                                   //only 1 render per frame
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
        _liveInstance = appPTR->createOFXEffect(name,this);
        _liveInstance->initializeOverlayInteract(); 
        _imp->previewInstance = appPTR->createOFXEffect(name,this);
    }
    assert(_liveInstance);
    assert(_imp->previewInstance);

    _imp->previewInstance->setClone();
    _imp->previewInstance->initializeKnobsPublic();
    _imp->previewRenderTree = new RenderTree(_imp->previewInstance);
    _imp->renderInstances.insert(std::make_pair(_imp->previewRenderTree,_imp->previewInstance));
}

bool Node::isRenderingPreview() const {
    QMutexLocker l(&_imp->previewMutex);
    return _imp->computingPreview;
}

void Node::quitAnyProcessing() {
    if (isOutputNode()) {
        dynamic_cast<Natron::OutputEffectInstance*>(this->getLiveInstance())->getVideoEngine()->quitEngineThread();
    }
    {
        QMutexLocker locker(&_imp->nodeInstanceLock);
        _imp->imagesBeingRenderedNotEmpty.wakeAll();
        if (_imp->computingPreview) {
            QMutexLocker l(&_imp->mustQuitProcessingMutex);
            _imp->mustQuitProcessing = true;
            while (_imp->mustQuitProcessing) {
                _imp->mustQuitProcessingCond.wait(&_imp->mustQuitProcessingMutex);
            }
        }
        
    }
}

Node::~Node()
{
    
    for (std::map<RenderTree*,EffectInstance*>::iterator it = _imp->renderInstances.begin(); it!=_imp->renderInstances.end(); ++it) {
        delete it->second;
    }
    delete _liveInstance;
    delete _imp->previewRenderTree;
    emit deleteWanted(this);
}

const std::map<int, std::string>& Node::getInputLabels() const
{
    return _imp->inputLabelsMap;
}

const Node::OutputMap& Node::getOutputs() const
{
    return _imp->outputs;
}

int Node::getPreferredInputForConnection() const {
    if (maximumInputs() == 0) {
        return -1;
    }
    
    ///we return the first non-optional empty input
    int firstNonOptionalEmptyInput = -1;
    int firstOptionalEmptyInput = -1;
    int index = 0;
    for (InputMap::const_iterator it = _inputs.begin() ; it!=_inputs.end(); ++it) {
        if (!it->second) {
            if (!_liveInstance->isInputOptional(index)) {
                if (firstNonOptionalEmptyInput == -1) {
                    firstNonOptionalEmptyInput = index;
                    break;
                }
            } else {
                if (firstOptionalEmptyInput == -1) {
                    firstOptionalEmptyInput = index;
                }
            }
        }
        ++index;
    }
    
    if (firstNonOptionalEmptyInput != -1) {
        return firstNonOptionalEmptyInput;
    } else if(firstOptionalEmptyInput != -1) {
        return firstOptionalEmptyInput;
    } else {
        return -1;
    }
}

void Node::getOutputsConnectedToThisNode(std::map<Node*,int>* outputs) {
    for (OutputMap::const_iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
        if (it->second) {
            int indexOfThis = it->second->inputIndex(this);
            assert(indexOfThis != -1);
            outputs->insert(std::make_pair(it->second, indexOfThis));
        }
    }
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
    _liveInstance->initializeKnobsPublic();
    emit knobsInitialized();
}

void Node::beginEditKnobs() {
    _liveInstance->beginEditKnobs();
}

EffectInstance* Node::findOrCreateLiveInstanceClone(RenderTree* tree)
{
    if (isOutputNode()) {
        //_liveInstance->updateInputs(tree);
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
    ret->clone();
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
        ret = appPTR->createOFXEffect(_liveInstance->pluginID(),this);
    }
    assert(ret);
    ret->setClone();
    ret->initializeKnobsPublic();
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

int Node::majorVersion() const{
    return _liveInstance->majorVersion();
}

int Node::minorVersion() const{
    return _liveInstance->minorVersion();
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

void Node::outputs(std::vector<Natron::Node*>* outputsV) const{
    for(OutputMap::const_iterator it = _imp->outputs.begin();it!= _imp->outputs.end();++it){
        if(it->second){
            outputsV->push_back(it->second);
        }
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
    std::vector<Natron::Node*> outputsV;
    outputs(&outputsV);
    return outputsV.size() > 0;
}

bool Node::connectInput(Node* input,int inputNumber,bool /*autoConnection*/ )
{
    
    QMutexLocker l(&isUsingInputsMutex);
    waitForRenderTreesToBeDone();


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
    QMutexLocker l(&isUsingInputsMutex);
    waitForRenderTreesToBeDone();


    assert(output);
    _imp->outputs.insert(make_pair(outputNumber,output));
    _liveInstance->updateInputs(NULL);
}

int Node::disconnectInput(int inputNumber)
{
    
    QMutexLocker l(&isUsingInputsMutex);
    waitForRenderTreesToBeDone();


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
    QMutexLocker l(&isUsingInputsMutex);
    waitForRenderTreesToBeDone();


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
    QMutexLocker l(&isUsingInputsMutex);
    waitForRenderTreesToBeDone();


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

int Node::inputIndex(Node* n) const {
    
    if (!n) {
        return -1;
    }
    
    int index = 0;
    for (InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (n == it->second) {
            return index;
        }
        ++index;
    }
    return -1;
}

/*After this call this node still knows the link to the old inputs/outputs
 but no other node knows this node.*/
void Node::deactivate()
{
    
    //first tell the gui to clear any persistent message link to this node
    clearPersistentMessage();

    ///if the node has 1 non-optional input, attempt to connect the outputs to the input of the current node
    ///this node is the node the outputs should attempt to connect to
    Node* inputToConnectTo = 0;
    
    int firstNonOptionalInput = -1;
    bool hasOnlyOneNonOptionalInput = false;
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (it->second) {
            if (!_liveInstance->isInputOptional(it->first)) {
                if (firstNonOptionalInput == -1) {
                    firstNonOptionalInput = it->first;
                    hasOnlyOneNonOptionalInput = true;
                } else {
                    hasOnlyOneNonOptionalInput = false;
                }
            }
        }
    }
    if (hasOnlyOneNonOptionalInput && firstNonOptionalInput != -1) {
        inputToConnectTo = input(firstNonOptionalInput);
    }
    
    /*Removing this node from the output of all inputs*/
    _imp->deactivatedState.inputConnections.clear();
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (it->second) {
            int outputNb = it->second->disconnectOutput(this);
            _imp->deactivatedState.inputConnections.insert(make_pair(it->second, make_pair(outputNb, it->first)));
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
    
    if (inputToConnectTo) {
        for (OutputConnectionsIterator it = _imp->deactivatedState.outputsConnections.begin();
             it!=_imp->deactivatedState.outputsConnections.end(); ++it) {
            getApp()->getProject()->connectNodes(it->second.first, inputToConnectTo, it->first);
        }
    }
    
    ///kill any thread it could have started (e.g: VideoEngine or preview)
    quitAnyProcessing();
    
    emit deactivated();
    _imp->activated = false;
    
}

void Node::activate()
{
    ///for all inputs, reconnect their output to this node
    for (InputConnectionsIterator it = _imp->deactivatedState.inputConnections.begin();
         it!= _imp->deactivatedState.inputConnections.end(); ++it) {
        it->first->connectOutput(this,it->second.first);
    }
   
   

    for (OutputConnectionsIterator it = _imp->deactivatedState.outputsConnections.begin();
         it!= _imp->deactivatedState.outputsConnections.end(); ++it) {
        
        ///before connecting the outputs to this node, disconnect any link that has been made
        ///between the outputs by the user
        Node* outputHasInput = it->first->input(it->second.first);
        if (outputHasInput) {
            bool ok = getApp()->getProject()->disconnectNodes(outputHasInput, it->first);
            assert(ok);
        }

        ///and connect the output to this node
        bool ok = it->first->connectInput(this, it->second.first);
        assert(ok);
    }
    
    emit activated();
    _imp->activated = true;
}



const Format& Node::getRenderFormatForEffect(const EffectInstance* effect) const
{
    if (effect == _liveInstance) {
        return getApp()->getProject()->getProjectDefaultFormat();
    } else {
        for (std::map<RenderTree*,EffectInstance*>::const_iterator it = _imp->renderInstances.begin();
             it!=_imp->renderInstances.end();++it) {
            if (it->second == effect) {
                return it->first->getRenderFormat();
            }
        }
    }
    return getApp()->getProject()->getProjectDefaultFormat();
}

int Node::getRenderViewsCountForEffect( const EffectInstance* effect) const
{
    if(effect == _liveInstance) {
        return getApp()->getProject()->getProjectViewsCount();
    } else {
        for (std::map<RenderTree*,EffectInstance*>::const_iterator it = _imp->renderInstances.begin();
             it!=_imp->renderInstances.end();++it) {
            if (it->second == effect) {
                return it->first->renderViewsCount();
            }
        }
    }
    return getApp()->getProject()->getProjectViewsCount();
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

    {
        QMutexLocker locker(&_imp->nodeInstanceLock);
        while(!_imp->imagesBeingRendered.empty()){
            _imp->imagesBeingRenderedNotEmpty.wait(&_imp->nodeInstanceLock);
        }
    }
    {
        QMutexLocker locker(&_imp->mustQuitProcessingMutex);
        if (_imp->mustQuitProcessing) {
            _imp->mustQuitProcessing = false;
            _imp->mustQuitProcessingCond.wakeOne();
            return;
        }
    }
  
    int knobsAge = _imp->previewInstance->getAppAge();

    QMutexLocker locker(&_imp->previewMutex); /// prevent 2 previews to occur at the same time since there's only 1 preview instance
    _imp->computingPreview = true;
    
    RectI rod;
    _imp->previewRenderTree->refreshTree(knobsAge);
    Natron::Status stat = _imp->previewInstance->getRegionOfDefinition(time, &rod);
    if (stat == StatFailed) {
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    }
    int h,w;
    h = rod.height() < height ? rod.height() : height;
    w = rod.width() < width ? rod.width() : width;
    double yZoomFactor = (double)h/(double)rod.height();
    double xZoomFactor = (double)w/(double)rod.width();

#ifdef NATRON_LOG
    Log::beginFunction(getName(),"makePreviewImage");
    Log::print(QString("Time "+QString::number(time)).toStdString());
#endif
    stat =  _imp->previewRenderTree->preProcessFrame(time);
    if(stat == StatFailed){
#ifdef NATRON_LOG
        Log::print(QString("preProcessFrame returned StatFailed.").toStdString());
        Log::endFunction(getName(),"makePreviewImage");
#endif
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    }

    RenderScale scale;
    scale.x = scale.y = 1.;
    boost::shared_ptr<Image> img;

    // Exceptions are caught because the program can run without a preview,
    // but any exception in renderROI is probably fatal.
    try {
        img = _imp->previewInstance->renderRoI(time, scale, 0,rod);
    } catch (const std::exception& e) {
        qDebug() << "Error: Cannot create preview" << ": " << e.what();
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    } catch (...) {
        qDebug() << "Error: Cannot create preview";
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    }
    
    if (!img) {
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
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
            int r = Color::floatToInt<256>(Natron::Color::to_func_srgb(src_pixels[nearestX*4]));
            int g = Color::floatToInt<256>(Natron::Color::to_func_srgb(src_pixels[nearestX*4+1]));
            int b = Color::floatToInt<256>(Natron::Color::to_func_srgb(src_pixels[nearestX*4+2]));
            dst_pixels[j] = ViewerInstance::toBGRA(r, g, b, 255);

        }
    }
    _imp->computingPreview = false;
    _imp->computingPreviewCond.wakeOne();
#ifdef NATRON_LOG
    Log::endFunction(getName(),"makePreviewImage");
#endif
}


void Node::abortRenderingForEffect(EffectInstance* effect)
{
    for (std::map<RenderTree*,EffectInstance*>::iterator it = _imp->renderInstances.begin(); it!=_imp->renderInstances.end(); ++it) {
        if(it->second == effect){
            dynamic_cast<OutputEffectInstance*>(it->first->getOutput())->getVideoEngine()->abortRendering(true);
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

bool Node::onOverlayKeyDown(Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    return _liveInstance->onOverlayKeyDown(key,modifiers);
}

bool Node::onOverlayKeyUp(Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    return _liveInstance->onOverlayKeyUp(key,modifiers);
}

bool Node::onOverlayKeyRepeat(Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    return _liveInstance->onOverlayKeyRepeat(key,modifiers);
}

bool Node::onOverlayFocusGained()
{
    return _liveInstance->onOverlayFocusGained();
}

bool Node::onOverlayFocusLost()
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
    if (!appPTR->isBackground()) {
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
    if(!appPTR->isBackground()){
        emit persistentMessageCleared();
    }
}

void Node::serialize(NodeSerialization* serializationObject) const {
    serializationObject->initialize(this);
}

void Node::purgeAllInstancesCaches(){
    for(std::map<RenderTree*,EffectInstance*>::iterator it = _imp->renderInstances.begin();
        it != _imp->renderInstances.end();++it){
        it->second->purgeCaches();
    }
    _imp->previewInstance->purgeCaches();
    _liveInstance->purgeCaches();
}

void Node::notifyInputNIsRendering(int inputNb) {
    emit inputNIsRendering(inputNb);
}

void Node::notifyInputNIsFinishedRendering(int inputNb) {
    emit inputNIsFinishedRendering(inputNb);
}

void Node::notifyRenderingStarted() {
    emit renderingStarted();
}

void Node::notifyRenderingEnded() {
    emit renderingEnded();
}


void Node::setInputFilesForReader(const QStringList& files) {
    _liveInstance->setInputFilesForReader(files);
}

void Node::setOutputFilesForWriter(const QString& pattern) {
    _liveInstance->setOutputFilesForWriter(pattern);
}

void Node::registerPluginMemory(size_t nBytes) {
    QMutexLocker l(&_imp->memoryUsedMutex);
    _imp->pluginInstanceMemoryUsed += nBytes;
    emit pluginMemoryUsageChanged((unsigned long long)_imp->pluginInstanceMemoryUsed);
}

void Node::unregisterPluginMemory(size_t nBytes) {
    QMutexLocker l(&_imp->memoryUsedMutex);
    _imp->pluginInstanceMemoryUsed -= nBytes;
    emit pluginMemoryUsageChanged((unsigned long long)_imp->pluginInstanceMemoryUsed);
}


void Node::setRenderTreeIsUsingInputs(bool b) {
    
    ///it doesn't matter if 2 RenderTree are using the inputs at the same time (because it's obvious concurrent
    ///threads are going to use the inputs concurrently after this function) because RenderTree's are just reading
    ///information, the only writer is the node itself.
    QMutexLocker l(&isUsingInputsMutex);
    _imp->isUsingInputs = b;
    if (!b) {
        _imp->isUsingInputsCond.wakeAll();
    }
}

void Node::waitForRenderTreesToBeDone() {
    assert(!isUsingInputsMutex.tryLock());
    while (_imp->isUsingInputs) {
        _imp->isUsingInputsCond.wait(&isUsingInputsMutex);
    }
}

QMutex& Node::getRenderInstancesSharedMutex()
{
    return _imp->renderInstancesSharedMutex;
}

QMutex& Node::getFrameMutex(int time)
{
    QMutexLocker l(&_imp->perFrameMutexesLock);
    std::map<int,boost::shared_ptr<QMutex> >::const_iterator it = _imp->renderInstancesFullySafePerFrameMutexes.find(time);
    if (it != _imp->renderInstancesFullySafePerFrameMutexes.end()) {
        // found the mutex, return it
        return *(it->second);
    }
    // create new map entry containing the mutex for this frame
    shared_ptr<QMutex> m(new QMutex);
    _imp->renderInstancesFullySafePerFrameMutexes.insert(std::make_pair(time,m));
    return *m;
}

void Node::refreshPreviewsRecursively() {
    if (isPreviewEnabled()) {
        refreshPreviewImage(getApp()->getTimeLine()->currentFrame());
    }
    for (OutputMap::iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
        if (it->second) {
            it->second->refreshPreviewsRecursively();
        }
    }
}

InspectorNode::InspectorNode(AppInstance* app,LibraryBinary* plugin,const std::string& name)
    : Node(app,plugin,name)
    , _inputsCount(1)
    , _activeInput(0)
{}


InspectorNode::~InspectorNode(){
    
}

bool InspectorNode::connectInput(Node* input,int inputNumber,bool autoConnection) {
    
    QMutexLocker l(&isUsingInputsMutex);
    waitForRenderTreesToBeDone();
    
    ///cannot connect more than 10 inputs.
    assert(inputNumber <= 10);
    
    assert(input);
    if(input == this){
        return false;
    }
    
    InputMap::iterator found = _inputs.find(inputNumber);
    //    if(/*input->className() == "Viewer" && */found!=_inputs.end() && !found->second){
    //        return false;
    //    }
    /*Adding all empty edges so it creates at least the inputNB'th one.*/
    while (_inputsCount <= inputNumber) {
        
        ///this function might not succeed if we already have 10 inputs OR the last input is already empty
        if (!tryAddEmptyInput()) {
            break;
        }
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
        l.unlock();
        setActiveInputAndRefresh(found->first);
        l.relock();
        _liveInstance->updateInputs(NULL);
        return false;
    }
    /*#2:second case: Before connecting the appropriate edge we search for any other edge connected with the
     selected node, in which case we just refresh the already connected edge.*/
    for (InputMap::const_iterator i = _inputs.begin(); i!=_inputs.end(); ++i) {
        if(i->second && i->second == input){
            l.unlock();
            setActiveInputAndRefresh(i->first);
            l.relock();
            _liveInstance->updateInputs(NULL);
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
bool InspectorNode::tryAddEmptyInput() {
    
    ///if we already reached 10 inputs, just don't do anything
    if (_inputs.size() <= 10) {
        
        
        if (_inputs.size() > 0) {
            ///if there are already liviing inputs, look at the last one
            ///and if it is not connected, just don't add an input.
            ///Otherwise, add an empty input.
            InputMap::const_iterator it = _inputs.end();
            --it;
            if(it->second != NULL){
                addEmptyInput();
                return true;
            }
            
        } else {
            ///there'is no inputs yet, just add one.
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
 
    QMutexLocker l(&isUsingInputsMutex);
    waitForRenderTreesToBeDone();

    
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
   
    QMutexLocker l(&isUsingInputsMutex);
    waitForRenderTreesToBeDone();


    InputMap::iterator it = _inputs.find(inputNb);
    if(it!=_inputs.end() && it->second!=NULL){
        _activeInput = inputNb;
    }
}

