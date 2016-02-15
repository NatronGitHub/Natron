/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PrecompNode.h"

#include <cassert>
#include <stdexcept>

#include "Global/GlobalDefines.h"
#include "Global/QtCompat.h"

#include <ofxNatron.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CLArgs.h"
#include "Engine/Node.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER;

struct PrecompNodePrivate
{
    PrecompNode* _publicInterface;
    
    AppInstance* app;
    
    boost::weak_ptr<KnobFile> projectFileNameKnob;
    //boost::weak_ptr<KnobButton> reloadProjectKnob;
    boost::weak_ptr<KnobButton> editProjectKnob;
    boost::weak_ptr<KnobBool> enablePreRenderKnob;
    boost::weak_ptr<KnobGroup> preRenderGroupKnob;
    boost::weak_ptr<KnobChoice> writeNodesKnob;
    boost::weak_ptr<KnobButton> preRenderKnob;
    boost::weak_ptr<KnobInt> firstFrameKnob,lastFrameKnob;
    boost::weak_ptr<KnobString> outputNodeNameKnob;
    boost::weak_ptr<KnobChoice> errorBehaviourKnbo;
    //kNatronOfxParamStringSublabelName to display the project name
    boost::weak_ptr<KnobString> subLabelKnob;
    
    QMutex dataMutex;
    
    NodesWList precompInputs;
    
    //To read-back the pre-comp image sequence/video
    NodePtr readNode;
    
    NodePtr outputNode;
    
    PrecompNodePrivate(PrecompNode* publicInterface)
    : _publicInterface(publicInterface)
    , app(0)
    , projectFileNameKnob()
    //, reloadProjectKnob()
    , editProjectKnob()
    , enablePreRenderKnob()
    , preRenderGroupKnob()
    , writeNodesKnob()
    , preRenderKnob()
    , firstFrameKnob()
    , lastFrameKnob()
    , outputNodeNameKnob()
    , errorBehaviourKnbo()
    , subLabelKnob()
    , dataMutex()
    , precompInputs()
    , readNode()
    , outputNode()
    {
        
    }
    
    void refreshKnobsVisibility();
    
    void reloadProject(bool setWriteNodeChoice);
    
    void createReadNode();
    
    void setReadNodeErrorChoice();
    
    void setFirstAndLastFrame();
    
    void refreshReadNodeInput();
    
    void populateWriteNodesChoice(bool setPartOfPrecomp, bool setWriteNodeChoice);
    
    NodePtr getWriteNodeFromPreComp() const;
    
    void refreshOutputNode();
    
    void launchPreRender();
};

PrecompNode::PrecompNode(NodePtr n)
: EffectInstance(n)
, _imp(new PrecompNodePrivate(this))
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

PrecompNode::~PrecompNode()
{
    if (_imp->app) {
        _imp->app->quit();
    }
    
}

NodePtr
PrecompNode::getOutputNode() const
{
    bool enablePrecomp = _imp->enablePreRenderKnob.lock()->getValue();
    
    QMutexLocker k(&_imp->dataMutex);
    if (!enablePrecomp) {
        return _imp->outputNode;
    } else {
        return _imp->readNode;
    }
    
}

std::string
PrecompNode::getPluginID() const
{
    return PLUGINID_NATRON_PRECOMP;
}

std::string
PrecompNode::getPluginLabel() const
{
    return "Precomp";
}

std::string
PrecompNode::getPluginDescription() const
{
    return "The Precomp node is like a Group node, but references an external Natron project (.ntp) instead.\n"
    "This allows you to save a subset of the node tree as a separate project. A Precomp node can be useful in at least two ways:\n"
    "It can be used to reduce portions of the node tree to pre-rendered image inputs. "
    "This speeds up render time: Natron only has to process the single image input instead of all the nodes within the project. "
    "Since this is a separate project, you also maintain access to the internal tree and can edit it any time.\n\n"
    "It enables a collaborative project: while one user works on the main project, others can work on other parts referenced by the Precomp node.";
}

void
PrecompNode::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_OTHER);
}

void
PrecompNode::addAcceptedComponents(int /*inputNb*/,
                                 std::list<ImageComponents>* comps)
{
    comps->push_back(ImageComponents::getRGBAComponents());
    comps->push_back(ImageComponents::getRGBComponents());
    comps->push_back(ImageComponents::getAlphaComponents());
}

void
PrecompNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthByte);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthFloat);
}

ImagePremultiplicationEnum
PrecompNode::getOutputPremultiplication() const
{
    NodePtr output = getOutputNode();
    if (output) {
        return output->getEffectInstance()->getOutputPremultiplication();
    } else {
        return eImagePremultiplicationPremultiplied;
    }
}

void
PrecompNode::getPreferredDepthAndComponents(int inputNb,
                                            std::list<ImageComponents>* comp,
                                            ImageBitDepthEnum* depth) const
{
    NodePtr output = getOutputNode();
    if (output) {
        output->getEffectInstance()->getPreferredDepthAndComponents(inputNb,comp,depth);
    } else {
        EffectInstance::getPreferredDepthAndComponents(inputNb, comp, depth);
    }
}

double
PrecompNode::getPreferredAspectRatio() const
{
    NodePtr output = getOutputNode();
    if (output) {
        return output->getEffectInstance()->getPreferredAspectRatio();
    } else {
        return EffectInstance::getPreferredAspectRatio();
    }
}

double
PrecompNode::getPreferredFrameRate() const
{
    NodePtr output = getOutputNode();
    if (output) {
        return output->getEffectInstance()->getPreferredFrameRate();
    } else {
        return EffectInstance::getPreferredFrameRate();
    }
}

void
PrecompNode::initializeKnobs()
{
    {
        CLArgs args;
        _imp->app = appPTR->newBackgroundInstance(args, true);
    }
    
    boost::shared_ptr<KnobPage> mainPage = AppManager::createKnob<KnobPage>(this, "Controls");
    
    boost::shared_ptr<KnobFile> filename = AppManager::createKnob<KnobFile>(this, "Project Filename (." NATRON_PROJECT_FILE_EXT ")");
    filename->setName("projectFilename");
    filename->setHintToolTip("The absolute file path of the project to use as a pre-comp");
    filename->setAnimationEnabled(false);
    filename->setAddNewLine(false);
    mainPage->addKnob(filename);
    _imp->projectFileNameKnob = filename;
    
    /*boost::shared_ptr<KnobButton> reload = AppManager::createKnob<KnobButton>(this, "Reload");
    reload->setName("reload");
    reload->setHintToolTip("Reload the pre-comp project from the file");
    reload->setAddNewLine(false);
    mainPage->addKnob(reload);
    _imp->reloadProjectKnob = reload;*/
    
    boost::shared_ptr<KnobButton> edit = AppManager::createKnob<KnobButton>(this, "Edit Project...");
    edit->setName("editProject");
    edit->setEvaluateOnChange(false);
    edit->setHintToolTip("Opens the specified project in a new " NATRON_APPLICATION_NAME " instance");
    mainPage->addKnob(edit);
    _imp->editProjectKnob = edit;
    
    boost::shared_ptr<KnobBool> enablePreRender = AppManager::createKnob<KnobBool>(this, "Pre-Render");
    enablePreRender->setName("preRender");
    enablePreRender->setAnimationEnabled(false);
    enablePreRender->setDefaultValue(true);
    enablePreRender->setHintToolTip("When checked the output of this node will be the images read directly from what is rendered "
                                    "by the node indicated by \"Write Node\". If no Write is selected, or if the rendered images do not exist "
                                    "this node will have the behavior determined by the \"On Error\" parameter. "
                                    "To pre-render images, select a write node, a frame-range and hit \"Render\".\n\n"
                                    "When unchecked, this node will output the image rendered by the node indicated in the \"Output Node\" parameter "
                                    "by rendering the full-tree of the sub-project. In that case no writing on disk will occur and the images will be "
                                    "cached with the same policy as if the nodes were used in the active project in the first place.");
    mainPage->addKnob(enablePreRender);
    _imp->enablePreRenderKnob = enablePreRender;
    
    boost::shared_ptr<KnobGroup> renderGroup = AppManager::createKnob<KnobGroup>(this, "Pre-Render Settings");
    renderGroup->setName("preRenderSettings");
    renderGroup->setDefaultValue(true);
    mainPage->addKnob(renderGroup);
    _imp->preRenderGroupKnob = renderGroup;
    
    boost::shared_ptr<KnobChoice> writeChoice = AppManager::createKnob<KnobChoice>(this, "Write Node");
    writeChoice->setName("writeNode");
    writeChoice->setHintToolTip("Choose here the Write node in the pre-comp from which to render images then specify a frame-range and "
                                "hit the \"Render\" button");
    writeChoice->setAnimationEnabled(false);
    {
        std::vector<std::string> choices;
        choices.push_back("None");
        writeChoice->populateChoices(choices);
    }
    renderGroup->addKnob(writeChoice);
    _imp->writeNodesKnob = writeChoice;
    
  
    
    boost::shared_ptr<KnobInt> first = AppManager::createKnob<KnobInt>(this, "First-Frame");
    first->setName("first");
    first->setHintToolTip("The first-frame to render");
    first->setAnimationEnabled(false);
    first->setEvaluateOnChange(false);
    first->setAddNewLine(false);
    renderGroup->addKnob(first);
    _imp->firstFrameKnob = first;
    
    boost::shared_ptr<KnobInt> last = AppManager::createKnob<KnobInt>(this, "Last-Frame");
    last->setName("last");
    last->setHintToolTip("The last-frame to render");
    last->setAnimationEnabled(false);
    last->setEvaluateOnChange(false);
    last->setAddNewLine(false);
    renderGroup->addKnob(last);
    _imp->lastFrameKnob = last;
    
    boost::shared_ptr<KnobChoice> error = AppManager::createKnob<KnobChoice>(this, "On Error");
    error->setName("onError");
    error->setHintToolTip("Indicates the behavior when an image is missing from the render of the pre-comp project");
    error->setAnimationEnabled(false);
    {
        std::vector<std::string> choices,helps;
        choices.push_back("Load Previous");
        helps.push_back("Loads the previous frame in the sequence");
        choices.push_back("Load Next");
        helps.push_back("Loads the next frame in the sequence");
        choices.push_back("Load Nearest");
        helps.push_back("Loads the nearest frame in the sequence");
        choices.push_back("Error");
        helps.push_back("Fails the render");
        choices.push_back("Black");
        helps.push_back("Black Image");
        
        error->populateChoices(choices, helps);
    }
    error->setDefaultValue(3);
    renderGroup->addKnob(error);
    _imp->errorBehaviourKnbo = error;
    
    boost::shared_ptr<KnobButton> renderBtn = AppManager::createKnob<KnobButton>(this, "Render");
    renderBtn->setName("render");
    renderGroup->addKnob(renderBtn);
    _imp->preRenderKnob = renderBtn;
    
    boost::shared_ptr<KnobString> outputNode = AppManager::createKnob<KnobString>(this, "Output Node");
    outputNode->setName("outputNode");
    outputNode->setHintToolTip("The script-name of the node to use as output node in the tree of the pre-comp. This can be any node.");
    outputNode->setAnimationEnabled(false);
    outputNode->setSecretByDefault(true);
    mainPage->addKnob(outputNode);
    _imp->outputNodeNameKnob = outputNode;
    
    boost::shared_ptr<KnobString> sublabel = AppManager::createKnob<KnobString>(this, "SubLabel");
    sublabel->setName(kNatronOfxParamStringSublabelName);
    sublabel->setSecretByDefault(true);
    mainPage->addKnob(sublabel);
    _imp->subLabelKnob = sublabel;
    
    
}

void
PrecompNode::onKnobsLoaded()
{
    _imp->reloadProject(false);
    _imp->refreshKnobsVisibility();
}

void
PrecompNode::knobChanged(KnobI* k,
                         ValueChangedReasonEnum reason,
                         ViewSpec /*view*/,
                         double /*time*/,
                         bool /*originatedFromMainThread*/)
{
    if (reason != eValueChangedReasonTimeChanged && k == _imp->projectFileNameKnob.lock().get()) {
        _imp->reloadProject(true);
    } else if (k == _imp->editProjectKnob.lock().get()) {
        std::string filename = _imp->projectFileNameKnob.lock()->getValue();
       (void)getApp()->loadProject(filename);
    } else if (k == _imp->preRenderKnob.lock().get()) {
        _imp->launchPreRender();
    } else if (k == _imp->outputNodeNameKnob.lock().get()) {
        _imp->refreshOutputNode();
    } else if (k == _imp->writeNodesKnob.lock().get()) {
        _imp->createReadNode();
        _imp->setFirstAndLastFrame();
    } else if (k == _imp->errorBehaviourKnbo.lock().get()) {
        _imp->setReadNodeErrorChoice();
    } else if (k == _imp->enablePreRenderKnob.lock().get()) {
        _imp->refreshKnobsVisibility();
        _imp->refreshOutputNode();
    }
}

void
PrecompNodePrivate::setReadNodeErrorChoice()
{
    NodePtr read;
    {
        QMutexLocker k(&dataMutex);
        read = readNode;
    }
    if (read) {
        KnobPtr knob = read->getKnobByName("onMissingFrame");
        if (knob) {
            KnobChoice* choice = dynamic_cast<KnobChoice*>(knob.get());
            if (choice) {
                choice->setValue(errorBehaviourKnbo.lock()->getValue());
            }
        }
    }
}

void
PrecompNodePrivate::refreshKnobsVisibility()
{
    bool preRenderEnabled = enablePreRenderKnob.lock()->getValue();
    outputNodeNameKnob.lock()->setSecret(preRenderEnabled);
    preRenderGroupKnob.lock()->setSecret(!preRenderEnabled);

}

void
PrecompNodePrivate::reloadProject(bool setWriteNodeChoice)
{
    std::string filename = projectFileNameKnob.lock()->getValue();
    
    QFileInfo file(filename.c_str());
    if (!file.exists()) {
        Dialogs::errorDialog(QObject::tr("Pre-Comp").toStdString(), QObject::tr("Pre-comp file not found.").toStdString());
        return;
    }
    QString fileUnPathed = file.fileName();
    
    subLabelKnob.lock()->setValue(fileUnPathed.toStdString());
    
    QString path = file.path() + "/";
    
    precompInputs.clear();
    
    boost::shared_ptr<Project> project = app->getProject();
    project->resetProject();
    {
        //Set a temporary timeline that will be used while loading the project.
        //This is to avoid that the seekFrame call has an effect on this project since they share the same timeline
        boost::shared_ptr<TimeLine> tmpTimeline(new TimeLine(project.get()));
        project->setTimeLine(tmpTimeline);
    }
    
    bool ok  = project->loadProject( path, fileUnPathed);
    if (!ok) {
        project->resetProject();
    }
    
    //Switch the timeline to this instance's timeline
    project->setTimeLine(_publicInterface->getApp()->getTimeLine());
    
    populateWriteNodesChoice(true, setWriteNodeChoice);
    
    if (ok) {
        createReadNode();
    }
    refreshOutputNode();
}

void
PrecompNodePrivate::populateWriteNodesChoice(bool setPartOfPrecomp, bool setWriteNodeChoice)
{
    boost::shared_ptr<KnobChoice> param = writeNodesKnob.lock();
    if (!param) {
        return;
    }
    std::vector<std::string> choices;
    choices.push_back("None");
    
    NodesList nodes;
    app->getProject()->getNodes_recursive(nodes, true);
    boost::shared_ptr<PrecompNode> precomp;
    if (setPartOfPrecomp) {
        precomp = boost::dynamic_pointer_cast<PrecompNode>(_publicInterface->shared_from_this());
        assert(precomp);
        
        //extract all inputs of the tree
        std::list<Project::NodesTree> trees;
        Project::extractTreesFromNodes(nodes, trees);
        for (std::list<Project::NodesTree>::iterator it = trees.begin(); it != trees.end(); ++it) {
            for (std::list<Project::TreeInput>::iterator it2 = it->inputs.begin(); it2 != it->inputs.end(); ++it2) {
                precompInputs.push_back(it2->node);
            }
        }
    }
    
   
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (setPartOfPrecomp) {
            (*it)->setPrecompNode(precomp);
        }
        if ((*it)->getEffectInstance()->isWriter()) {
            choices.push_back((*it)->getFullyQualifiedName());
        }
    }
 
    param->populateChoices(choices);
    
    if (setWriteNodeChoice) {
        if (choices.size() > 1) {
            param->setValue(1);
        }
    }
}

NodePtr
PrecompNodePrivate::getWriteNodeFromPreComp() const
{
    std::string userChoiceNodeName =  writeNodesKnob.lock()->getActiveEntryText_mt_safe();
    if (userChoiceNodeName == "None") {
        return NodePtr();
    }
    NodePtr writeNode = app->getProject()->getNodeByFullySpecifiedName(userChoiceNodeName);
    if (!writeNode) {
        std::stringstream ss;
        ss << QObject::tr("Could not find a node named").toStdString();
        ss << " " << userChoiceNodeName << " ";
        ss << QObject::tr("in the pre-comp project").toStdString();
        Dialogs::errorDialog(QObject::tr("Pre-Comp").toStdString(), ss.str());
        return NodePtr();
    }
    return writeNode;
}

void
PrecompNode::getPrecompInputs(NodesList* nodes) const
{
    QMutexLocker k(&_imp->dataMutex);
    for (NodesWList::const_iterator it = _imp->precompInputs.begin(); it!=_imp->precompInputs.end(); ++it) {
        NodePtr node = it->lock();
        if (!node) {
            continue;
        }
        nodes->push_back(node);
    }
}

void
PrecompNodePrivate::createReadNode()
{

    NodePtr oldReadNode;
    {
        QMutexLocker k(&dataMutex);
        oldReadNode = readNode;
    }
    if (oldReadNode) {
        //Ensure it is no longer part of the tree
        oldReadNode->destroyNode(false);
        
        
        //Reset the pointer
        QMutexLocker k(&dataMutex);
        readNode.reset();
    }

    NodePtr writeNode = getWriteNodeFromPreComp();
    if (!writeNode) {
        return;
    }
    
    KnobPtr fileNameKnob = writeNode->getKnobByName(kOfxImageEffectFileParamName);
    if (!fileNameKnob) {
        return;
    }
    
    KnobOutputFile* fileKnob = dynamic_cast<KnobOutputFile*>(fileNameKnob.get());
    if (!fileKnob) {
        return;
    }
    
    std::string pattern = fileKnob->getValue();
    QString qpattern(pattern.c_str());
    
    std::map<std::string, std::string> readersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);

    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::map<std::string, std::string>::iterator found = readersForFormat.find(ext);
    if ( found == readersForFormat.end() ) {
        std::stringstream ss;
        ss << QObject::tr("No plugin capable of decoding").toStdString();
        ss << " " << ext << " ";
        ss << QObject::tr("was found").toStdString();
        Dialogs::errorDialog(QObject::tr("Pre-Comp").toStdString(), ss.str());
        return;
    }
    
    QString readPluginID(found->second.c_str());
    
    
    
    QString fixedNamePrefix(_publicInterface->getScriptName_mt_safe().c_str());
    fixedNamePrefix.append('_');
    fixedNamePrefix.append("readNode");
    fixedNamePrefix.append('_');
    
    CreateNodeArgs args(readPluginID, eCreateNodeReasonInternal, app->getProject());
    args.createGui = false;
    args.fixedName = fixedNamePrefix;
    args.paramValues.push_back( createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, pattern) );

    
    NodePtr read = app->createNode(args);
    if (!read) {
        return ;
    }
    
    boost::shared_ptr<PrecompNode> precomp = boost::dynamic_pointer_cast<PrecompNode>(_publicInterface->shared_from_this());
    assert(precomp);
    read->setPrecompNode(precomp);
    
    QObject::connect(read.get(), SIGNAL(persistentMessageChanged()), _publicInterface, SLOT(onReadNodePersistentMessageChanged()));
    
    {
        QMutexLocker k(&dataMutex);
        readNode = read;
    }
}

void
PrecompNodePrivate::refreshOutputNode()
{
    bool usePreRender = enablePreRenderKnob.lock()->getValue();
    NodePtr outputnode;
    if (!usePreRender) {
        boost::shared_ptr<KnobString> outputNodeKnob = outputNodeNameKnob.lock();
        std::string outputNodeName = outputNodeKnob->getValue();
        
        outputnode = app->getProject()->getNodeByFullySpecifiedName(outputNodeName);
    }
    
    //Clear any persistent message set
    _publicInterface->clearPersistentMessage(false);
    
    {
        QMutexLocker k(&dataMutex);
        outputNode = outputnode;
    }
    
    ///Notify outputs that the node has changed
    std::map<NodePtr, int> outputs;
    NodePtr thisNode = _publicInterface->getNode();
    thisNode->getOutputsConnectedToThisNode(&outputs);

    for (std::map<NodePtr, int>::iterator it = outputs.begin(); it!=outputs.end(); ++it) {
        it->first->onInputChanged(it->second);
    }
}

void
PrecompNodePrivate::setFirstAndLastFrame()
{
    NodePtr writeNode = getWriteNodeFromPreComp();
    if (!writeNode) {
        return;
    }
    KnobPtr writefirstFrameKnob = writeNode->getKnobByName("firstFrame");
    KnobPtr writelastFrameKnob = writeNode->getKnobByName("lastFrame");
    KnobInt* firstFrame = dynamic_cast<KnobInt*>(writefirstFrameKnob.get());
    KnobInt* lastFrame = dynamic_cast<KnobInt*>(writelastFrameKnob.get());
    if (firstFrame) {
        firstFrameKnob.lock()->setValue(firstFrame->getValue());
    }
    if (lastFrame) {
        lastFrameKnob.lock()->setValue(lastFrame->getValue());
    }
}


void
PrecompNodePrivate::refreshReadNodeInput()
{
    assert(readNode);
    KnobPtr fileNameKnob = readNode->getKnobByName(kOfxImageEffectFileParamName);
    if (!fileNameKnob) {
        return;
    }
    //Remove all images from the cache associated to the reader since we know they are no longer valid.
    //This is a blocking call so that we are sure there's no old image laying around in the cache after this call
    readNode->removeAllImagesFromCache(true);
    readNode->purgeAllInstancesCaches();
    
    //Force the reader to reload the sequence/video
    fileNameKnob->evaluateValueChange(0, _publicInterface->getApp()->getTimeLine()->currentFrame(), ViewIdx(0),eValueChangedReasonUserEdited);
    
}

void
PrecompNodePrivate::launchPreRender()
{
    NodePtr output = getWriteNodeFromPreComp();
    if (!output) {
        Dialogs::errorDialog(QObject::tr("Pre-Render").toStdString(), QObject::tr("Selected write node does not exist").toStdString());
        return;
    }
    AppInstance::RenderWork w(dynamic_cast<OutputEffectInstance*>(output->getEffectInstance().get()),
                              firstFrameKnob.lock()->getValue(),
                              lastFrameKnob.lock()->getValue(),
                              1,
                              false);

    if (w.writer) {
        RenderEngine* engine = w.writer->getRenderEngine();
        if (engine) {
            QObject::connect(engine, SIGNAL(renderFinished(int)), _publicInterface, SLOT(onPreRenderFinished()));
        }
    }
    
    std::list<AppInstance::RenderWork> works;
    works.push_back(w);
    _publicInterface->getApp()->startWritersRendering(false, works);
}

void
PrecompNode::onPreRenderFinished()
{
    NodePtr output = getOutputNode();
    if (!output) {
        return;
    }
    OutputEffectInstance* writer = dynamic_cast<OutputEffectInstance*>(output->getEffectInstance().get());
    assert(writer);
    RenderEngine* engine = writer->getRenderEngine();
    if (engine) {
        QObject::disconnect(engine, SIGNAL(renderFinished(int)), this, SLOT(onPreRenderFinished()));
    }
    
    _imp->refreshReadNodeInput();
}

void
PrecompNode::onReadNodePersistentMessageChanged()
{
    NodePtr node;
    {
        QMutexLocker k(&_imp->dataMutex);
        assert(_imp->readNode);
        node = _imp->readNode;
    }
    
    QString message;
    int type;
    node->getPersistentMessage(&message, &type, false);
    if (message.isEmpty()) {
        clearPersistentMessage(false);
    } else {
        setPersistentMessage((MessageTypeEnum)type, message.toStdString());
    }
}

AppInstance*
PrecompNode::getPrecompApp() const
{
    return _imp->app;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_PrecompNode.cpp"
