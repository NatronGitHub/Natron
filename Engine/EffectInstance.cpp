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

#include "EffectInstance.h"

#include <QtConcurrentMap>
#include <QCoreApplication>

#include "Engine/OfxEffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Log.h"

using namespace Powiter;

EffectInstance::EffectInstance(Node* node):
KnobHolder(node ? node->getApp() : NULL)
, _node(node)
, _renderAborted(false)
, _hashValue()
, _hashAge(0)
, _isRenderClone(false)
, _inputs()
, _renderArgs()
{
    //create the renderArgs only if the current thread is different than the main thread.
    // otherwise it would create mem leaks and an error message.
    if(QThread::currentThread() != qApp->thread()){
        _renderArgs.reset(new QThreadStorage<RenderArgs>);
    }
}

void EffectInstance::clone(){
    if(!_isRenderClone)
        return;
    cloneKnobs(*(_node->getLiveInstance()));
    cloneExtras();
}


namespace {
    void Hash64_appendKnob(Hash64* hash, const Knob& knob){
        const std::vector<U64>& values = knob.getHashVector();
        for(U32 i=0;i<values.size();++i) {
            hash->append(values[i]);
        }
    }
}

bool EffectInstance::isHashValid() const {
    //The hash is valid only if the age is the same than the project's age and the hash has been computed at least once.
    return _hashAge == getAppAge() && _hashValue.valid();
}
int EffectInstance::hashAge() const{
    return _hashAge;
}


U64 EffectInstance::computeHash(const std::vector<U64>& inputsHashs){
    
    _hashAge = getAppAge();
    
    _hashValue.reset();
    const std::vector<Knob*>& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        ::Hash64_appendKnob(&_hashValue, *knobs[i]);
    }
    for (U32 i =0; i < inputsHashs.size(); ++i) {
        _hashValue.append(inputsHashs[i]);
    }
    ::Hash64_appendQString(&_hashValue, className().c_str());
    _hashValue.computeHash();
    return _hashValue.value();
}

const std::string& EffectInstance::getName() const{
    return _node->getName();
}

const Format& EffectInstance::getRenderFormat() const{
    return _node->getRenderFormatForEffect(this);
}

int EffectInstance::getRenderViewsCount() const{
    return _node->getRenderViewsCountForEffect(this);
}


bool EffectInstance::hasOutputConnected() const{
    return _node->hasOutputConnected();
}

Powiter::EffectInstance* EffectInstance::input(int n) const{
    if(n < (int)_inputs.size()){
        return _inputs[n];
    }
    return NULL;
}

std::string EffectInstance::setInputLabel(int inputNb) const {
    std::string out;
    out.append(1,(char)(inputNb+65));
    return out;
}

boost::shared_ptr<const Powiter::Image> EffectInstance::getImage(int inputNb,SequenceTime time,RenderScale scale,int view){
#ifdef POWITER_LOG
    Powiter::Log::beginFunction(getName(),"getImage");
    Powiter::Log::print(QString("Input "+QString::number(inputNb)+
                                                      " Scale ("+QString::number(scale.x)+
                                                      ","+QString::number(scale.y)+
                                                     ") Time " + QString::number(time)
                                                      +" View " + QString::number(view)).toStdString());
    
#endif
    
    const Powiter::Cache<Image>& cache = appPTR->getNodeCache();
    //making key with a null RoD since the hash key doesn't take the RoD into account
    //we'll get our image back without the RoD in the key
    EffectInstance* n  = input(inputNb);
    assert(n);
    Powiter::ImageKey params = Powiter::Image::makeKey(n->hash().value(), time,scale,view,RectI());
    boost::shared_ptr<const Image > entry = cache.get(params);
    

#ifdef POWITER_LOG
    Powiter::Log::print(QString("The image was found in the NodeCache with the following hash key: "+
                                                         QString::number(params.getHash())).toStdString());
#endif
    if(!entry){
        //if not found in cache render it using the last args passed to render by this thread
        RectI roi;
        if(_renderArgs && _renderArgs->hasLocalData()){
            roi = _renderArgs->localData()._roi;//if the thread was spawned by us we take the last render args
        }else{
            n->getRegionOfDefinition(time, &roi);//we have no choice but compute the full region of definition
        }
        entry = n->renderRoI(time, scale, view,roi);
    }
#ifdef POWITER_LOG
    Powiter::Log::endFunction(getName(),"getImage");
#endif
    return entry;
}

Powiter::Status EffectInstance::getRegionOfDefinition(SequenceTime time,RectI* rod) {
    for(Inputs::const_iterator it = _inputs.begin() ; it != _inputs.end() ; ++it){
        if (*it) {
            RectI inputRod;
            Status st = (*it)->getRegionOfDefinition(time, &inputRod);
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

EffectInstance::RoIMap EffectInstance::getRegionOfInterest(SequenceTime /*time*/,RenderScale /*scale*/,const RectI& renderWindow){
    RoIMap ret;
    for(Inputs::const_iterator it = _inputs.begin() ; it != _inputs.end() ; ++it){
        if (*it) {
            ret.insert(std::make_pair(*it, renderWindow));
        }
    }
    return ret;
}


void EffectInstance::getFrameRange(SequenceTime *first,SequenceTime *last){
    for(Inputs::const_iterator it = _inputs.begin() ; it != _inputs.end() ; ++it){
        if (*it) {
            SequenceTime inpFirst,inpLast;
            (*it)->getFrameRange(&inpFirst, &inpLast);
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

boost::shared_ptr<const Powiter::Image> EffectInstance::renderRoI(SequenceTime time,RenderScale scale,int view,const RectI& renderWindow){
#ifdef POWITER_LOG
    Powiter::Log::beginFunction(getName(),"renderRoI");
    Powiter::Log::print(QString("Time "+QString::number(time)+
                                                      " Scale ("+QString::number(scale.x)+
                                                      ","+QString::number(scale.y)
                        +") View " + QString::number(view) + " RoI: xmin= "+ QString::number(renderWindow.left()) +
                        " ymin= " + QString::number(renderWindow.bottom()) + " xmax= " + QString::number(renderWindow.right())
                        + " ymax= " + QString::number(renderWindow.top())).toStdString());
                        
#endif
    
    Powiter::ImageKey key = Powiter::Image::makeKey(_hashValue.value(), time, scale,view,RectI());
    /*look-up the cache for any existing image already rendered*/
    boost::shared_ptr<Image> image = boost::const_pointer_cast<Image>(appPTR->getNodeCache().get(key));
    /*if not cached, we store the freshly allocated image in this member*/
    if(!image){
        /*before allocating it we must fill the RoD of the image we want to render*/
        getRegionOfDefinition(time, &key._rod);
        int cost = 0;
        /*should data be stored on a physical device ?*/
        if(shouldRenderedDataBePersistent()){
            cost = 1;
        }
        /*allocate a new image*/
        image = appPTR->getNodeCache().newEntry(key,key._rod.area()*4,cost);
    }
#ifdef POWITER_LOG
    else{
        Powiter::Log::print(QString("The image was found in the NodeCache with the following hash key: "+
                                                     QString::number(key.getHash())).toStdString());
    }
#endif
    _node->addImageBeingRendered(image, time, view);
    /*now that we have our image, we check what is left to render. If the list contains only
     null rects then we already rendered it all*/
    std::list<RectI> rectsToRender = image->getRestToRender(renderWindow);
    if(rectsToRender.size() != 1 || !rectsToRender.begin()->isNull()){
        for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
            
#ifdef POWITER_LOG
        Powiter::Log::print(QString("Rect left to render in the image... xmin= "+
                                                          QString::number((*it).left())+" ymin= "+
                                                          QString::number((*it).bottom())+ " xmax= "+
                                                          QString::number((*it).right())+ " ymax= "+
                                                          QString::number((*it).top())).toStdString());
#endif
            
            /*we can set the render args*/
            RenderArgs args;
            args._roi = *it;
            args._time = time;
            args._view = view;
            args._scale = scale;
            if(_renderArgs){ // if this function is called on the _liveInstance object (i.e: preview)
                             // it has no _renderArgs.
                _renderArgs->setLocalData(args);
            }
            
            RoIMap inputsRoi = getRegionOfInterest(time, scale, *it);
            std::list<boost::shared_ptr<const Powiter::Image> > inputImages;
            /*we render each input first and store away their image in the inputImages list
             in order to maintain a shared_ptr use_count > 1 so the cache doesn't attempt
             to remove them.*/
            for (RoIMap::const_iterator it2 = inputsRoi.begin(); it2!= inputsRoi.end(); ++it2) {
                try{
                    inputImages.push_back(it2->first->renderRoI(time, scale,view, it2->second));
                }catch(const std::exception& e){
                    throw e;
                }
            }
            /*depending on the thread-safety of the plug-in we render with a different
             amount of threads*/
            EffectInstance::RenderSafety safety = renderThreadSafety();
            if(safety == UNSAFE){
                QMutex* pluginLock = appPTR->getMutexForPlugin(className().c_str());
                assert(pluginLock);
                pluginLock->lock();
                Powiter::Status st = render(time, scale, *it,view, image);
                pluginLock->unlock();
                if(st != Powiter::StatOK){
                    throw std::runtime_error("");
                }
                image->markForRendered(*it);
            }else if(safety == INSTANCE_SAFE){
                Powiter::Status st = render(time, scale, *it,view, image);
                if(st != Powiter::StatOK){
                    throw std::runtime_error("");
                }
                image->markForRendered(*it);
            }else{ // fully_safe, we do multi-threaded rendering on small tiles
                std::vector<RectI> splitRects = RectI::splitRectIntoSmallerRect(*it, QThread::idealThreadCount());
                QFuture<Powiter::Status> ret = QtConcurrent::mapped(splitRects,
                                          boost::bind(&EffectInstance::tiledRenderingFunctor,this,args,_1,image));
                ret.waitForFinished();
                for (QFuture<Powiter::Status>::const_iterator it = ret.begin(); it!=ret.end(); ++it) {
                    if ((*it) == Powiter::StatFailed) {
                        throw std::runtime_error("");
                    }
                }
            }
        }
    }
#ifdef POWITER_LOG
    else{
        Powiter::Log::print(QString("Everything is already rendered in this image.").toStdString());
    }
#endif
    _node->removeImageBeingRendered(time, view);
    
    //we released the input images and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();
    
#ifdef POWITER_LOG
    Powiter::Log::endFunction(getName(),"renderRoI");
#endif
    return image;
}

boost::shared_ptr<Powiter::Image> EffectInstance::getImageBeingRendered(SequenceTime time,int view) const{
    return _node->getImageBeingRendered(time, view);
}

Powiter::Status EffectInstance::tiledRenderingFunctor(RenderArgs args,
                                 const RectI& roi,
                                 boost::shared_ptr<Powiter::Image> output){
    
    if(_renderArgs){
        _renderArgs->setLocalData(args);
    }
    Powiter::Status st = render(args._time, args._scale, roi,args._view, output);
    if(st != StatOK){
        return st;
    }
    output->markForRendered(roi);
    return StatOK;
}




void EffectInstance::createKnobDynamically(){
    _node->createKnobDynamically();
}

void EffectInstance::evaluate(Knob* /*knob*/,bool isSignificant){
    assert(_node);
    std::list<ViewerInstance*> viewers;
    _node->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        if(isSignificant){
            (*it)->refreshAndContinueRender();
        }else{
            (*it)->redrawViewer();
        }
    }
}


void EffectInstance::openFilesForAllFileKnobs(){
    const std::vector<Knob*>& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        Knob* k = knobs[i];
        if(k->typeName() == "InputFile"){
            dynamic_cast<File_Knob*>(k)->openFile();
        }else if(k->typeName() == "OutputFile"){
            dynamic_cast<OutputFile_Knob*>(k)->openFile();
        }
    }
}

void EffectInstance::abortRendering(){
    if(_isRenderClone){
        _node->abortRenderingForEffect(this);
    }else if(isOutput()){
        dynamic_cast<OutputEffectInstance*>(this)->getVideoEngine()->abortRendering();
    }
}

void EffectInstance::notifyFrameRangeChanged(int first,int last){
    _node->notifyFrameRangeChanged(first, last);
}


void EffectInstance::updateInputs(RenderTree* tree){
    _inputs.clear();
    const Node::InputMap& inputs = _node->getInputs();
    _inputs.reserve(inputs.size());
    
    
    for (Node::InputMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        if (it->second) {
            InspectorNode* insp = dynamic_cast<InspectorNode*>(_node);
            if(insp){
                Node* activeInput = insp->input(insp->activeInput());
                if(it->second != activeInput){
                    _inputs.push_back((EffectInstance*)NULL);
                    continue;
                }
            }
            EffectInstance* inputEffect = 0;
            if(tree){
                inputEffect = tree->getEffectForNode(it->second);
            }else{
                inputEffect = it->second->getLiveInstance();
            }
            assert(inputEffect);
            _inputs.push_back(inputEffect);
        }else{
            _inputs.push_back((EffectInstance*)NULL);
        }
    }
    
}



Knob* EffectInstance::getKnobByDescription(const std::string& desc) const{

    const std::vector<Knob*>& knobs = getKnobs();
    for(U32 i = 0; i < knobs.size() ; ++i){
        if (knobs[i]->getDescription() == desc) {
            return knobs[i];
        }
    }
    return NULL;
}

bool EffectInstance::message(Powiter::MessageType type,const std::string& content) const{
    return _node->message(type,content);
}

void EffectInstance::setPersistentMessage(Powiter::MessageType type,const std::string& content){
    _node->setPersistentMessage(type, content);
}

void EffectInstance::clearPersistentMessage() {
    _node->clearPersistentMessage();
}

const EffectInstance::RenderArgs& EffectInstance::getArgsForLastRender() const{
    assert(_renderArgs->hasLocalData());
    return _renderArgs->localData();
}

OutputEffectInstance::OutputEffectInstance(Node* node):
Powiter::EffectInstance(node)
, _videoEngine()
{
    if(node){
        _videoEngine.reset(new VideoEngine(this));
    }
}

void OutputEffectInstance::updateTreeAndRender(bool initViewer){
    _videoEngine->updateTreeAndContinueRender(initViewer);
}
void OutputEffectInstance::refreshAndContinueRender(bool initViewer){
    _videoEngine->refreshAndContinueRender(initViewer);
}

void OutputEffectInstance::ifInfiniteclipRectToProjectDefault(RectI* rod) const{
    /*If the rod is infinite clip it to the project's default*/
    const Format& projectDefault = getRenderFormat();
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


