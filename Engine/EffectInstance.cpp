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

#include "EffectInstance.h"

#include <QtConcurrentMap>
#include <QCoreApplication>
#include <QThreadStorage>

#include "Global/AppManager.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Log.h"
#include "Engine/VideoEngine.h"
#include "Engine/Image.h"

#include "Writers/Writer.h"

using namespace Natron;



struct EffectInstance::Implementation {
    Implementation()
    : renderAborted(false)
    , hashValue()
    , hashAge(0)
    , isRenderClone(false)
    , inputs()
    , renderArgs()
    , previewEnabled(false)
    , markedByTopologicalSort(false)
    {
    }

    bool renderAborted; //< was rendering aborted ?
    Hash64 hashValue;//< The hash value of this effect
    int hashAge;//< to check if the hash has the same age than the project's age
    bool isRenderClone;//< is this instance a live instance (i.e interacting with GUI)
    //or a render instance (i.e a snapshot of the live instance at a given time)

    Inputs inputs;//< all the inputs of the effect. Watch out, some might be NULL if they aren't connected
    QThreadStorage<RenderArgs> renderArgs;
    bool previewEnabled;
    bool markedByTopologicalSort;
};

struct EffectInstance::RenderArgs {
    RectI _roi;
    SequenceTime _time;
    RenderScale _scale;
    int _view;
};

EffectInstance::EffectInstance(Node* node)
: KnobHolder(node ? node->getApp() : NULL)
, _node(node)
, _imp(new Implementation)
{
}

EffectInstance::~EffectInstance()
{
}

void EffectInstance::setMarkedByTopologicalSort(bool marked) const {_imp->markedByTopologicalSort = marked;}

bool EffectInstance::isMarkedByTopologicalSort() const {return _imp->markedByTopologicalSort;}

bool EffectInstance::isLiveInstance() const
{
    return !_imp->isRenderClone;
}

const Hash64& EffectInstance::hash() const
{
    return _imp->hashValue;
}

const EffectInstance::Inputs& EffectInstance::getInputs() const
{
    return _imp->inputs;
}

bool EffectInstance::aborted() const
{
    return _imp->renderAborted;
}

void EffectInstance::setAborted(bool b)
{
    _imp->renderAborted = b;
}

bool EffectInstance::isPreviewEnabled() const
{
    return _imp->previewEnabled;
}

void EffectInstance::setAsRenderClone()
{
    _imp->isRenderClone = true;
}

void EffectInstance::clone(SequenceTime time){
    if(!_imp->isRenderClone)
        return;
    cloneKnobs(*(_node->getLiveInstance()));
    refreshAfterTimeChange(time);
    cloneExtras();
    _imp->previewEnabled = _node->getLiveInstance()->isPreviewEnabled();
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
    return _imp->hashAge == getAppAge() && _imp->hashValue.valid();
}
int EffectInstance::hashAge() const{
    return _imp->hashAge;
}


U64 EffectInstance::computeHash(SequenceTime time, const std::vector<U64>& inputsHashs){
    
    _imp->hashAge = getAppAge();
    
    _imp->hashValue.reset();
    _imp->hashValue.append(*(reinterpret_cast<U64*>(&time)));
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        ::Hash64_appendKnob(&_imp->hashValue, *knobs[i]);
    }
    for (U32 i =0; i < inputsHashs.size(); ++i) {
        _imp->hashValue.append(inputsHashs[i]);
    }
    ::Hash64_appendQString(&_imp->hashValue, pluginID().c_str());
    _imp->hashValue.computeHash();
    return _imp->hashValue.value();
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

Natron::EffectInstance* EffectInstance::input(int n) const{
    if (n < (int)_imp->inputs.size()) {
        return _imp->inputs[n];
    }
    return NULL;
}

std::string EffectInstance::inputLabel(int inputNb) const {
    std::string out;
    out.append(1,(char)(inputNb+65));
    return out;
}

boost::shared_ptr<const Natron::Image> EffectInstance::getImage(int inputNb,SequenceTime time,RenderScale scale,int view){
    Natron::Log::beginFunction(getName(),"getImage");
    Natron::Log::print(QString("Input "+QString::number(inputNb)+
                                                      " Scale ("+QString::number(scale.x)+
                                                      ","+QString::number(scale.y)+
                                                     ") Time " + QString::number(time)
                                                      +" View " + QString::number(view)).toStdString());
    

    
    
    const Natron::Cache<Image>& cache = appPTR->getNodeCache();
    //making key with a null RoD since the hash key doesn't take the RoD into account
    //we'll get our image back without the RoD in the key
    EffectInstance* n  = input(inputNb);
    
    //if the node is not connected, return a NULL pointer!
    if(!n){
        return boost::shared_ptr<const Natron::Image>();
    }
    Natron::ImageKey params = Natron::Image::makeKey(n->hash().value(), time,scale,view,RectI());
    boost::shared_ptr<const Image > entry = cache.get(params);
    

    Natron::Log::print(QString("The image was found in the NodeCache with the following hash key: "+
                                                         QString::number(params.getHash())).toStdString());
    if(!entry){
        //if not found in cache render it using the last args passed to render by this thread
        RectI roi;
        if (_imp->renderArgs.hasLocalData()) {
            roi = _imp->renderArgs.localData()._roi;//if the thread was spawned by us we take the last render args
        }else{
            assert(n->getRegionOfDefinition(time, &roi) != Natron::StatFailed);//we have no choice but compute the full region of definition
        }
        entry = n->renderRoI(time, scale, view,roi);
    }
    Natron::Log::endFunction(getName(),"getImage");

    return entry;
}

Natron::Status EffectInstance::getRegionOfDefinition(SequenceTime time,RectI* rod) {
    for(Inputs::const_iterator it = _imp->inputs.begin() ; it != _imp->inputs.end() ; ++it){
        if (*it) {
            RectI inputRod;
            Status st = (*it)->getRegionOfDefinition(time, &inputRod);
            if(st == StatFailed)
                return st;
            if (it == _imp->inputs.begin()) {
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
    for( Inputs::const_iterator it = _imp->inputs.begin() ; it != _imp->inputs.end() ; ++it) {
        if (*it) {
            ret.insert(std::make_pair(*it, renderWindow));
        }
    }
    return ret;
}


void EffectInstance::getFrameRange(SequenceTime *first,SequenceTime *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    for (Inputs::const_iterator it = _imp->inputs.begin() ; it != _imp->inputs.end() ; ++it) {
        if (*it) {
            SequenceTime inpFirst,inpLast;
            (*it)->getFrameRange(&inpFirst, &inpLast);
            if (it == _imp->inputs.begin()) {
                *first = inpFirst;
                *last = inpLast;
            } else {
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

boost::shared_ptr<const Natron::Image> EffectInstance::renderRoI(SequenceTime time,RenderScale scale,
                                                                 int view,const RectI& renderWindow,
                                                                 bool byPassCache){
    Natron::Log::beginFunction(getName(),"renderRoI");
    Natron::Log::print(QString("Time "+QString::number(time)+
                                                      " Scale ("+QString::number(scale.x)+
                                                      ","+QString::number(scale.y)
                        +") View " + QString::number(view) + " RoI: xmin= "+ QString::number(renderWindow.left()) +
                        " ymin= " + QString::number(renderWindow.bottom()) + " xmax= " + QString::number(renderWindow.right())
                        + " ymax= " + QString::number(renderWindow.top())).toStdString());
                        

    /*look-up the cache for any existing image already rendered*/
    boost::shared_ptr<Image> image;
    Natron::ImageKey key = Natron::Image::makeKey(_imp->hashValue.value(), time, scale,view,RectI());
    if(!byPassCache){
        image = boost::const_pointer_cast<Image>(appPTR->getNodeCache().get(key));
    }
    /*if not cached, we store the freshly allocated image in this member*/
    if(!image){
        /*before allocating it we must fill the RoD of the image we want to render*/
        if(getRegionOfDefinition(time, &key._rod) == StatFailed){
            return boost::shared_ptr<const Natron::Image>();
        }
        int cost = 0;
        /*should data be stored on a physical device ?*/
        if(shouldRenderedDataBePersistent()){
            cost = 1;
        }
        /*allocate a new image*/
        if(!byPassCache){
            image = appPTR->getNodeCache().newEntry(key,key._rod.area()*4,cost);
        }else{
            image.reset(new Natron::Image(key._rod,scale,time));
        }
    } else {
        Natron::Log::print(QString("The image was found in the NodeCache with the following hash key: "+
                                                     QString::number(key.getHash())).toStdString());
    }
    _node->addImageBeingRendered(image, time, view);
    /*now that we have our image, we check what is left to render. If the list contains only
     null rects then we already rendered it all*/
    RectI intersection;
    renderWindow.intersect(image->getRoD(), &intersection);
    std::list<RectI> rectsToRender = image->getRestToRender(intersection);
    if(rectsToRender.size() != 1 || !rectsToRender.begin()->isNull()){
        for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
            
        Natron::Log::print(QString("Rect left to render in the image... xmin= "+
                                                          QString::number((*it).left())+" ymin= "+
                                                          QString::number((*it).bottom())+ " xmax= "+
                                                          QString::number((*it).right())+ " ymax= "+
                                                          QString::number((*it).top())).toStdString());

            /*we can set the render args*/
            RenderArgs args;
            args._roi = *it;
            args._time = time;
            args._view = view;
            args._scale = scale;
            _imp->renderArgs.setLocalData(args);

            RoIMap inputsRoi = getRegionOfInterest(time, scale, *it);
            std::list<boost::shared_ptr<const Natron::Image> > inputImages;
            /*we render each input first and store away their image in the inputImages list
             in order to maintain a shared_ptr use_count > 1 so the cache doesn't attempt
             to remove them.*/
            for (RoIMap::const_iterator it2 = inputsRoi.begin(); it2!= inputsRoi.end(); ++it2) {
                boost::shared_ptr<const Natron::Image> inputImg = it2->first->renderRoI(time, scale,view, it2->second,byPassCache);
                if (inputImg) {
                    inputImages.push_back(inputImg);
                }
                if (aborted()) {
                    //if render was aborted, remove the frame from the cache as it contains only garbage
                    appPTR->removeFromNodeCache(image);
                    _node->removeImageBeingRendered(time, view);
                    return image;
                }
            }
            /*depending on the thread-safety of the plug-in we render with a different
             amount of threads*/
            EffectInstance::RenderSafety safety = renderThreadSafety();
            if(safety == UNSAFE){
                QMutex* pluginLock = appPTR->getMutexForPlugin(pluginID().c_str());
                assert(pluginLock);
                pluginLock->lock();
                Natron::Status st = render(time, scale, *it,view, image);
                pluginLock->unlock();
                if(st != Natron::StatOK){
                    throw std::runtime_error("");
                }
                if(!aborted()){
                    image->markForRendered(*it);
                }
            }else if(safety == INSTANCE_SAFE){
                Natron::Status st = render(time, scale, *it,view, image);
                if(st != Natron::StatOK){
                    throw std::runtime_error("");
                }
                if(!aborted()){
                    image->markForRendered(*it);
                }
            }else{ // fully_safe, we do multi-threaded rendering on small tiles
                std::vector<RectI> splitRects = RectI::splitRectIntoSmallerRect(*it, QThread::idealThreadCount());
                QFuture<Natron::Status> ret = QtConcurrent::mapped(splitRects,
                                          boost::bind(&EffectInstance::tiledRenderingFunctor,this,args,_1,image));
                ret.waitForFinished();
                for (QFuture<Natron::Status>::const_iterator it = ret.begin(); it!=ret.end(); ++it) {
                    if ((*it) == Natron::StatFailed) {
                        throw std::runtime_error("");
                    }
                }
            }
        }
    } else {
        Natron::Log::print(QString("Everything is already rendered in this image.").toStdString());
    }
//    QString filename(getName().c_str());
//    filename.append(QString::number(image->getHashKey()));
//    filename.append(".png");
//    Natron::debugImage(image.get(),filename);

    _node->removeImageBeingRendered(time, view);
    
    //we released the input images and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();

    if(aborted()){
        //if render was aborted, remove the frame from the cache as it contains only garbage
        appPTR->removeFromNodeCache(image);
    }
    
    Natron::Log::endFunction(getName(),"renderRoI");
    return image;
}

boost::shared_ptr<Natron::Image> EffectInstance::getImageBeingRendered(SequenceTime time,int view) const{
    return _node->getImageBeingRendered(time, view);
}

Natron::Status EffectInstance::tiledRenderingFunctor(const RenderArgs& args,
                                 const RectI& roi,
                                 boost::shared_ptr<Natron::Image> output)
{
    _imp->renderArgs.setLocalData(args);
    Natron::Status st = render(args._time, args._scale, roi,args._view, output);
    if(st != StatOK){
        return st;
    }
    if(!aborted()){
        output->markForRendered(roi);
    }
    return StatOK;
}




void EffectInstance::createKnobDynamically(){
    _node->createKnobDynamically();
}

void EffectInstance::evaluate(Knob* knob,bool isSignificant){
    assert(_node);
    if(!isOutput()){
        std::list<ViewerInstance*> viewers;
        _node->hasViewersConnected(&viewers);
        bool fitToViewer = knob->typeName() == "InputFile";
        for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
            if(isSignificant){
                //(*it)->abortRendering();
                (*it)->refreshAndContinueRender(fitToViewer);
            }else{
                (*it)->redrawViewer();
            }
        }
    }else{
        /*if this is a writer (openfx or built-in writer)*/
        if (pluginID() != "Viewer") {
            /*if this is a button,we're safe to assume the plug-ins wants to start rendering.*/
            if(knob->typeName() == "Button"){
                QStringList list;
                list << getName().c_str();
                getApp()->startWritersRendering(list);
            }
        }
    }
}


void EffectInstance::openFilesForAllFileKnobs(){
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        boost::shared_ptr<Knob> k = knobs[i];
        if(k->typeName() == "InputFile"){
            boost::dynamic_pointer_cast<File_Knob>(k)->openFile();
        }else if(k->typeName() == "OutputFile"){
            boost::dynamic_pointer_cast<OutputFile_Knob>(k)->openFile();
        }
    }
}

void EffectInstance::abortRendering(){
    if (_imp->isRenderClone) {
        _node->abortRenderingForEffect(this);
    }else if(isOutput()){
        dynamic_cast<OutputEffectInstance*>(this)->getVideoEngine()->abortRendering();
    }
}

void EffectInstance::notifyFrameRangeChanged(int first,int last){
    _node->notifyFrameRangeChanged(first, last);
}

void EffectInstance::togglePreview() {
    _imp->previewEnabled = !_imp->previewEnabled;
}

void EffectInstance::updateInputs(RenderTree* tree) {
    _imp->inputs.clear();
    const Node::InputMap& inputs = _node->getInputs();
    _imp->inputs.reserve(inputs.size());
    
    
    for (Node::InputMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        if (it->second) {
            InspectorNode* insp = dynamic_cast<InspectorNode*>(_node);
            if(insp){
                Node* activeInput = insp->input(insp->activeInput());
                if(it->second != activeInput){
                    _imp->inputs.push_back((EffectInstance*)NULL);
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
            _imp->inputs.push_back(inputEffect);
        }else{
            _imp->inputs.push_back((EffectInstance*)NULL);
        }
    }
    
}



boost::shared_ptr<Knob> EffectInstance::getKnobByDescription(const std::string& desc) const{

    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for(U32 i = 0; i < knobs.size() ; ++i){
        if (knobs[i]->getDescription() == desc) {
            return knobs[i];
        }
    }
    return boost::shared_ptr<Knob>();
}

bool EffectInstance::message(Natron::MessageType type,const std::string& content) const{
    return _node->message(type,content);
}

void EffectInstance::setPersistentMessage(Natron::MessageType type,const std::string& content){
    _node->setPersistentMessage(type, content);
}

void EffectInstance::clearPersistentMessage() {
    _node->clearPersistentMessage();
}

OutputEffectInstance::OutputEffectInstance(Node* node)
: Natron::EffectInstance(node)
, _videoEngine(node?new VideoEngine(this):0)
{
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
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    // an int can not be equal to (or compared to) std::numeric_limits<double>::infinity()
    if (rod->left() == kOfxFlagInfiniteMin || rod->left() == std::numeric_limits<int>::min()) {
        rod->set_left(projectDefault.left());
    }
    if (rod->bottom() == kOfxFlagInfiniteMin || rod->bottom() == std::numeric_limits<int>::min()) {
        rod->set_bottom(projectDefault.bottom());
    }
    if (rod->right() == kOfxFlagInfiniteMax || rod->right() == std::numeric_limits<int>::max()) {
        rod->set_right(projectDefault.right());
    }
    if (rod->top() == kOfxFlagInfiniteMax || rod->top()  == std::numeric_limits<int>::max()) {
        rod->set_top(projectDefault.top());
    }
    
}

void OutputEffectInstance::renderFullSequence(){
    assert(pluginID() != "Viewer"); //< this function is not meant to be called for rendering on the viewer
    getVideoEngine()->refreshTree(_writerFirstFrame);
    getVideoEngine()->render(-1, true,false,true,false);
    
}
