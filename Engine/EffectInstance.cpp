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

#include "Engine/AppManager.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Log.h"
#include "Engine/VideoEngine.h"
#include "Engine/Image.h"
#include "Engine/KnobFile.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/AppInstance.h"

using namespace Natron;


class File_Knob;
class OutputFile_Knob;

struct EffectInstance::Implementation {
    Implementation()
    : renderAborted(false)
    , hashValue()
    , hashAge(0)
    , inputs()
    , renderArgs()
    , previewEnabled(false)
    , markedByTopologicalSort(false)
    {
    }

    bool renderAborted; //< was rendering aborted ?
    Hash64 hashValue;//< The hash value of this effect
    int hashAge;//< to check if the hash has the same age than the project's age
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
    return !isClone();
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

void EffectInstance::clone(){
    if(!isClone())
        return;
    cloneKnobs(*(_node->getLiveInstance()));
    //refreshAfterTimeChange(time);
    cloneExtras();
    _imp->previewEnabled = _node->getLiveInstance()->isPreviewEnabled();
    if(isOpenFX()){
        dynamic_cast<OfxEffectInstance*>(this)->effectInstance()->syncPrivateDataAction();
    }
}


bool EffectInstance::isHashValid() const {
    //The hash is valid only if the age is the same than the project's age and the hash has been computed at least once.
    return _imp->hashAge == getAppAge() && _imp->hashValue.valid();
}
int EffectInstance::hashAge() const{
    return _imp->hashAge;
}


U64 EffectInstance::computeHash(const std::vector<U64>& inputsHashs,int knobsAge){
    
    _imp->hashAge = knobsAge;
    
    _imp->hashValue.reset();
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        knobs[i]->appendHashVectorToHash(&_imp->hashValue);
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

boost::shared_ptr<Natron::Image> EffectInstance::getImage(int inputNb,SequenceTime time,RenderScale scale,int view){
    
    EffectInstance* n  = input(inputNb);
    
    //if the node is not connected, return a NULL pointer!
    if(!n){
        return boost::shared_ptr<Natron::Image>();
    }

    ///just call renderRoI which will  do the cache look-up for us and render
    ///the image if it's missing from the cache.
    RectI roi;
    if (_imp->renderArgs.hasLocalData()) {
        roi = _imp->renderArgs.localData()._roi;//if the thread was spawned by us we take the last render args
    } else {
        Natron::Status stat = n->getRegionOfDefinition(time, &roi);
        if(stat == Natron::StatFailed) {//we have no choice but compute the full region of definition
            return boost::shared_ptr<Natron::Image>();
        }
    }
    boost::shared_ptr<Image > entry = n->renderRoI(time, scale, view,roi);

    return entry;
}

Natron::Status EffectInstance::getRegionOfDefinition(SequenceTime time,RectI* rod) {
    
    if (isWriter()) {
        rod->set(getRenderFormat());
        return StatReplyDefault;
    }
    
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

boost::shared_ptr<Natron::Image> EffectInstance::renderRoI(SequenceTime time,RenderScale scale,
                                                                 int view,const RectI& renderWindow,
                                                                 bool byPassCache)
{
#ifdef NATRON_LOG
    Natron::Log::beginFunction(getName(),"renderRoI");
    Natron::Log::print(QString("Time "+QString::number(time)+
                                                      " Scale ("+QString::number(scale.x)+
                                                      ","+QString::number(scale.y)
                        +") View " + QString::number(view) + " RoI: xmin= "+ QString::number(renderWindow.left()) +
                        " ymin= " + QString::number(renderWindow.bottom()) + " xmax= " + QString::number(renderWindow.right())
                        + " ymax= " + QString::number(renderWindow.top())).toStdString());
#endif
    /*first-off check whether the effect is identity, in which case we don't want
    to cache anything or render anything for this effect.*/
    SequenceTime inputTimeIdentity;
    int inputNbIdentity;

    bool identity = isIdentity(time,scale,renderWindow,view,&inputTimeIdentity,&inputNbIdentity);
    if(identity){
        return getImage(inputNbIdentity,inputTimeIdentity,scale,view);
    }

    /*look-up the cache for any existing image already rendered*/
    boost::shared_ptr<Image> image;
    bool isCached = false;
    
    int cost = 0;
    /*should data be stored on a physical device ?*/
    if(shouldRenderedDataBePersistent()){
        cost = 1;
    }
    
    /*before allocating it we must fill the RoD of the image we want to render*/
    RectI rod;
    if(getRegionOfDefinition(time, &rod) == StatFailed){
        ///if getRoD fails, just return a NULL ptr
        return boost::shared_ptr<Natron::Image>();
    }
    
    /*add the window to the project's available formats if the effect is a reader*/
    if (isReader()) {
        Format frmt;
        frmt.set(rod);
        ///FIXME: what about the pixel aspect ratio ?
        getApp()->getProject()->setOrAddProjectFormat(frmt);
    }
    
#pragma message WARN("Specify image components here")
    Natron::ImageKey key = Natron::Image::makeKey(cost,_imp->hashValue.value(), time, scale,view,Natron::ImageComponentRGBA,rod);
    
    if(getCachePolicy(time) == NEVER_CACHE){
        byPassCache = true;
    }
    if(!byPassCache){
        isCached = Natron::getImageFromCache(key, &image);
    }

    /*if not cached, we store the freshly allocated image in this member*/
    if(!isCached){
        
        /*allocate a new image*/
        if(byPassCache){
            assert(!image);
            image.reset(new Natron::Image(Natron::ImageComponentRGBA,key._rod,scale,time));
        }
    } else {
#ifdef NATRON_LOG
        Natron::Log::print(QString("The image was found in the NodeCache with the following hash key: "+
                                                     QString::number(key.getHash())).toStdString());
#endif
    }
    _node->addImageBeingRendered(image, time, view);
    
    /*now that we have our image, we check what is left to render. If the list contains only
     null rects then we already rendered it all*/
    RectI intersection;
    renderWindow.intersect(image->getRoD(), &intersection);
    std::list<RectI> rectsToRender;
    if (supportsTiles()) {
        rectsToRender = image->getRestToRender(intersection);
    } else {
        rectsToRender.push_back(rod);
    }
#ifdef NATRON_LOG
    if (rectsToRender.empty()) {
        Natron::Log::print(QString("Everything is already rendered in this image.").toStdString());
    }
#endif

    for (std::list<RectI>::iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        
        RectI& rectToRender = *it;
        
#ifdef NATRON_LOG
        Natron::Log::print(QString("Rect left to render in the image... xmin= "+
                                   QString::number(rectToRender.left())+" ymin= "+
                                   QString::number(rectToRender.bottom())+ " xmax= "+
                                   QString::number(rectToRender.right())+ " ymax= "+
                                   QString::number(rectToRender.top())).toStdString());
#endif
        /*we can set the render args*/
        RenderArgs args;
        args._roi = rectToRender;
        args._time = time;
        args._view = view;
        args._scale = scale;
        _imp->renderArgs.setLocalData(args);

        RoIMap inputsRoi = getRegionOfInterest(time, scale, rectToRender);
        std::list<boost::shared_ptr<const Natron::Image> > inputImages;
        /*we render each input first and store away their image in the inputImages list
         in order to maintain a shared_ptr use_count > 1 so the cache doesn't attempt
         to remove them.*/
        for (RoIMap::const_iterator it2 = inputsRoi.begin(); it2!= inputsRoi.end(); ++it2) {

            ///notify the node that we're going to render something with the input
            int inputNb = getInputNumber(it2->first);
            assert(inputNb != -1); //< see getInputNumber

            _node->notifyInputNIsRendering(inputNb);

            boost::shared_ptr<const Natron::Image> inputImg = it2->first->renderRoI(time, scale,view, it2->second,byPassCache);
            if (inputImg) {
                inputImages.push_back(inputImg);
            }

            _node->notifyInputNIsFinishedRendering(inputNb);

            if (aborted()) {
                //if render was aborted, remove the frame from the cache as it contains only garbage
                appPTR->removeFromNodeCache(image);
                _node->removeImageBeingRendered(time, view);
                return image;
            }
        }

        ///notify the node we're starting a render
        _node->notifyRenderingStarted();

        /*depending on the thread-safety of the plug-in we render with a different
         amount of threads*/
        EffectInstance::RenderSafety safety = renderThreadSafety();
        
        ///if the project lock is already locked at this point, don't start any othter thread
        ///as it would lead to a deadlock when the project is loading.
        ///Just fall back to Fully_safe
        if (safety == FULLY_SAFE_FRAME) {
            if (!getApp()->getProject()->tryLock()) {
                safety = FULLY_SAFE;
            } else {
                getApp()->getProject()->unlock();
            }
        }
        switch (safety) {
            case FULLY_SAFE_FRAME: // the plugin will perform any per frame SMP threading
            {
                // we can split the frame in tiles and do per frame SMP threading (see kOfxImageEffectPluginPropHostFrameThreading)
                std::vector<RectI> splitRects = RectI::splitRectIntoSmallerRect(rectToRender, QThread::idealThreadCount());
                // the bitmap is checked again at the beginning of EffectInstance::tiledRenderingFunctor()
                QFuture<Natron::Status> ret = QtConcurrent::mapped(splitRects,
                                                                   boost::bind(&EffectInstance::tiledRenderingFunctor,this,args,_1,image));
                ret.waitForFinished();
                for (QFuture<Natron::Status>::const_iterator it2 = ret.begin(); it2!=ret.end(); ++it2) {
                    if ((*it2) == Natron::StatFailed) {
                        throw std::runtime_error("rendering failed");
                    }
                }
            } break;

            case INSTANCE_SAFE: // indicating that any instance can have a single 'render' call at any one time,
            {
#pragma message WARN("INSTANCE_SAFE means that there is only one render per INSTANCE!!! take a per-instance lock here, and read NOTE below")
                // NOTE: the per-instance lock should probably be shared between
                // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
                // and read-write on it.
                // It is probably safer to assume that several clones may write to the same output image only in the FULLY_SAFE case.

                // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
                rectToRender = image->getMinimalRect(rectToRender);
                if (!rectToRender.isNull()) {
                    Natron::Status st = render(time, scale, rectToRender,view, image);
                    if(st != Natron::StatOK){
                        throw std::runtime_error("rendering failed");
                    }
                    if(!aborted()){
                        image->markForRendered(rectToRender);
                    }
                }
            } break;
            case FULLY_SAFE:    // indicating that any instance of a plugin can have multiple renders running simultaneously
            {
#pragma message WARN("FULLY_SAFE means that there is only one render per FRAME for a given instance!!! take a per-frame lock here (the map of per-frame locks belongs to an instance)")
                // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
                rectToRender = image->getMinimalRect(rectToRender);
                if (!rectToRender.isNull()) {
                    Natron::Status st = render(time, scale, rectToRender,view, image);
                    if(st != Natron::StatOK){
                        throw std::runtime_error("rendering failed");
                    }
                    if(!aborted()){
                        image->markForRendered(rectToRender);
                    }
                }
            } break;


            case UNSAFE: // indicating that only a single 'render' call can be made at any time amoung all instances
            default:
            {
                QMutex* pluginLock = appPTR->getMutexForPlugin(pluginID().c_str());
                assert(pluginLock);
                pluginLock->lock();
                // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
                rectToRender = image->getMinimalRect(rectToRender);
                if (!rectToRender.isNull()) {
                    Natron::Status st = render(time, scale, rectToRender,view, image);
                    pluginLock->unlock();
                    if(st != Natron::StatOK){
                        throw std::runtime_error("rendering failed");
                    }
                    if(!aborted()){
                        image->markForRendered(rectToRender);
                    }
                }
            } break;
        }

        ///notify the node we've finished rendering
        _node->notifyRenderingEnded();
    }
    _node->removeImageBeingRendered(time, view);

    //we released the input images and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();

    if(aborted()){
        //if render was aborted, remove the frame from the cache as it contains only garbage
        appPTR->removeFromNodeCache(image);
    }
#ifdef NATRON_LOG
    Natron::Log::endFunction(getName(),"renderRoI");
#endif
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
    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
    RectI rectToRender = output->getMinimalRect(roi);
    if (!rectToRender.isNull()) {
        Natron::Status st = render(args._time, args._scale, rectToRender, args._view, output);
        if(st != StatOK){
            return st;
        }
        if(!aborted()){
            output->markForRendered(roi);
        }
    }
    return StatOK;
}

void EffectInstance::openImageFileKnob() {
    const std::vector< boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
            boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
            assert(fk);
            if (fk->isInputImageFile()) {
                QString file = fk->getValue<QString>();
                if (file.isEmpty()) {
                    fk->open_file();
                }
                break;
            }
        } else if(knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if (fk->isOutputImageFile()) {
                QString file = fk->getValue<QString>();
                if(file.isEmpty()){
                    fk->open_file();
                }
                break;
            }

        }
    }
}


void EffectInstance::createKnobDynamically(){
    _node->createKnobDynamically();
}

void EffectInstance::evaluate(Knob* knob,bool isSignificant){
    assert(_node);
    
    /*if this is a writer (openfx or built-in writer)*/
    if (isWriter()) {
        /*if this is a button and it is a render button,we're safe to assume the plug-ins wants to start rendering.*/
        if(knob && knob->typeName() == Button_Knob::typeNameStatic()){
            Button_Knob* button = dynamic_cast<Button_Knob*>(knob);
            assert(button);
            if(button->isRenderButton()){
                QStringList list;
                list << getName().c_str();
                getApp()->startWritersRendering(list);
                return;
            }
        }
    }
    std::list<ViewerInstance*> viewers;
    _node->hasViewersConnected(&viewers);
    bool fitToViewer = knob && knob->typeName() == File_Knob::typeNameStatic();
    bool forcePreview = getApp()->getProject()->isAutoPreviewEnabled();
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        if(isSignificant){
            (*it)->refreshAndContinueRender(fitToViewer,forcePreview);
        }else{
            (*it)->redrawViewer();
        }
    }
    
}


void EffectInstance::abortRendering(){
    if (isClone()) {
        _node->abortRenderingForEffect(this);
    }else if(isOutput()){
        dynamic_cast<OutputEffectInstance*>(this)->getVideoEngine()->abortRendering(true);
    }
}

void EffectInstance::togglePreview() {
    _imp->previewEnabled = !_imp->previewEnabled;
}

void EffectInstance::updateInputs(RenderTree* tree) {
    Inputs inputsCopy = _imp->inputs;
    _imp->inputs.clear();
    const Node::InputMap& inputs = _node->getInputs();
    InspectorNode* insp = dynamic_cast<InspectorNode*>(_node);
    
    for (Node::InputMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        EffectInstance* inputInstance = NULL;
        if (it->second) {
            if (insp) {
                Node* activeInput = insp->input(insp->activeInput());
                if(it->second == activeInput){
                    if(tree){
                        inputInstance = tree->getEffectForNode(it->second);
                    }else{
                        inputInstance = it->second->getLiveInstance();
                    }
                    assert(inputInstance);
                }
            } else {
                if(tree){
                    inputInstance = tree->getEffectForNode(it->second);
                }else{
                    inputInstance = it->second->getLiveInstance();
                }
                assert(inputInstance);
            }
        }
        _imp->inputs.push_back(inputInstance);
    }
    if (!insp) {
        if (!inputsCopy.empty()) {
            bool hasChanged = false;
            assert(_imp->inputs.size() == inputsCopy.size());
            for (unsigned int i = 0; i < inputsCopy.size(); ++i) {
                if (_imp->inputs[i] != inputsCopy[i]) {
                    onInputChanged(i);
                    hasChanged = true;
                }
            }
            if (hasChanged) {
                onMultipleInputsChanged();
            }
        } else {
            bool hasChanged = false;
            for (unsigned int i = 0; i < inputsCopy.size(); ++i) {
                onInputChanged(i);
                hasChanged = true;
            }
            if (hasChanged) {
                onMultipleInputsChanged();
            }
            
        }
    }
    
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

int EffectInstance::getInputNumber(Natron::EffectInstance* inputEffect) const {
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i] == inputEffect) {
            return i;
        }
    }
    return -1;
}


void EffectInstance::setInputFilesForReader(const QStringList& files) {
    
    if (!isReader()) {
        return;
    }
    
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
            boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
            assert(fk);
            if (fk->isInputImageFile()) {
                if (files.size() > 1 && ! fk->isAnimationEnabled()) {
                    throw std::invalid_argument("This reader does not support image sequences. Please provide a single file.");
                }
                fk->setFiles(files);
                break;
            }
        }
    }
}

void EffectInstance::setOutputFilesForWriter(const QString& pattern) {
    
    if (!isWriter()) {
        return;
    }
    
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if (fk->isOutputImageFile()) {
                fk->setValue<QString>(pattern);
                break;
            }
        }
    }
}

PluginMemory* EffectInstance::newMemoryInstance(size_t nBytes) {
    PluginMemory* ret = new PluginMemory(this);
    bool wasntLocked = ret->alloc(nBytes);
    assert(wasntLocked);
    return ret;
}


void EffectInstance::registerPluginMemory(size_t nBytes) {
    _node->registerPluginMemory(nBytes);
}

void EffectInstance::unregisterPluginMemory(size_t nBytes) {
    _node->unregisterPluginMemory(nBytes);
}

OutputEffectInstance::OutputEffectInstance(Node* node)
: Natron::EffectInstance(node)
, _videoEngine(node?new VideoEngine(this):0)
, _writerCurrentFrame(0)
, _writerFirstFrame(0)
, _writerLastFrame(0)
{
}

OutputEffectInstance::~OutputEffectInstance(){
    if(_videoEngine){
        _videoEngine->quitEngineThread();
    }
}

void OutputEffectInstance::updateTreeAndRender(bool initViewer){
    _videoEngine->updateTreeAndContinueRender(initViewer);
}
void OutputEffectInstance::refreshAndContinueRender(bool initViewer,bool forcePreview){
    _videoEngine->refreshAndContinueRender(initViewer,forcePreview);
}

void OutputEffectInstance::ifInfiniteclipRectToProjectDefault(RectI* rod) const{
    if(!getApp()->getProject()){
        return;
    }
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

void OutputEffectInstance::renderFullSequence(BlockingBackgroundRender* renderController) {
    _renderController = renderController;
    assert(pluginID() != "Viewer"); //< this function is not meant to be called for rendering on the viewer
    getVideoEngine()->refreshTree();
    getVideoEngine()->render(-1,true,true,false,true,false);
    
}

void OutputEffectInstance::notifyRenderFinished() {
    if (_renderController) {
        _renderController->notifyFinished();
        _renderController = 0;
    }
}