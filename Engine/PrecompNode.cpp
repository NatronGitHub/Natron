/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream> // stringstream

#include "Global/GlobalDefines.h"
#include "Global/QtCompat.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <ofxNatron.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CLArgs.h"
#include "Engine/CreateNodeArgs.h"
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


NATRON_NAMESPACE_ENTER

struct PrecompNodePrivate
{
    Q_DECLARE_TR_FUNCTIONS(PrecompNode)

public:
    PrecompNode* _publicInterface;
    AppInstanceWPtr app;
    KnobFileWPtr projectFileNameKnob;
    //KnobButtonWPtr reloadProjectKnob;
    KnobButtonWPtr editProjectKnob;
    KnobBoolWPtr enablePreRenderKnob;
    KnobGroupWPtr preRenderGroupKnob;
    KnobChoiceWPtr writeNodesKnob;
    KnobButtonWPtr preRenderKnob;
    KnobIntWPtr firstFrameKnob, lastFrameKnob;
    KnobStringWPtr outputNodeNameKnob;
    KnobChoiceWPtr errorBehaviourKnbo;
    //kNatronOfxParamStringSublabelName to display the project name
    KnobStringWPtr subLabelKnob;
    QMutex dataMutex;
    NodesWList precompInputs;

    //To read-back the pre-comp image sequence/video
    NodePtr readNode;
    NodePtr outputNode;

    PrecompNodePrivate(PrecompNode* publicInterface)
        : _publicInterface(publicInterface)
        , app()
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
    , _imp( new PrecompNodePrivate(this) )
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

PrecompNode::~PrecompNode()
{
    AppInstancePtr app = _imp->app.lock();

    if (app) {
        app->quit();
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
                                   std::list<ImagePlaneDesc>* comps)
{
    comps->push_back( ImagePlaneDesc::getRGBAComponents() );
    comps->push_back( ImagePlaneDesc::getRGBComponents() );
    comps->push_back( ImagePlaneDesc::getAlphaComponents() );
}

void
PrecompNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthByte);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthFloat);
}

void
PrecompNode::initializeKnobs()
{
    {
        CLArgs args;
        _imp->app = appPTR->newBackgroundInstance(args, true);
    }

    KnobPagePtr mainPage = AppManager::createKnob<KnobPage>( this, tr("Controls") );
    KnobFilePtr filename = AppManager::createKnob<KnobFile>( this, tr("Project Filename (.%1)").arg( QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ) );

    filename->setName("projectFilename");
    filename->setHintToolTip( tr("The absolute file path of the project to use as a pre-comp.").toStdString() );
    filename->setAnimationEnabled(false);
    filename->setAddNewLine(false);
    mainPage->addKnob(filename);
    _imp->projectFileNameKnob = filename;

    /*KnobButtonPtr reload = AppManager::createKnob<KnobButton>(this, "Reload");
       reload->setName("reload");
       reload->setHintToolTip("Reload the pre-comp project from the file");
       reload->setAddNewLine(false);
       mainPage->addKnob(reload);
       _imp->reloadProjectKnob = reload;*/

    KnobButtonPtr edit = AppManager::createKnob<KnobButton>( this, tr("Edit Project...") );
    edit->setName("editProject");
    edit->setEvaluateOnChange(false);
    edit->setHintToolTip( tr("Opens the specified project in a new %1 instance").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
    mainPage->addKnob(edit);
    _imp->editProjectKnob = edit;

    KnobBoolPtr enablePreRender = AppManager::createKnob<KnobBool>( this, tr("Pre-Render") );
    enablePreRender->setName("preRender");
    enablePreRender->setAnimationEnabled(false);
    enablePreRender->setDefaultValue(true);
    enablePreRender->setHintToolTip( tr("When checked the output of this node will be the images read directly from what is rendered "
                                        "by the node indicated by \"Write Node\". If no Write is selected, or if the rendered images do not exist "
                                        "this node will have the behavior determined by the \"On Error\" parameter. "
                                        "To pre-render images, select a write node, a frame-range and hit \"Render\".\n\n"
                                        "When unchecked, this node will output the image rendered by the node indicated in the \"Output Node\" parameter "
                                        "by rendering the full-tree of the sub-project. In that case no writing on disk will occur and the images will be "
                                        "cached with the same policy as if the nodes were used in the active project in the first place.").toStdString() );
    mainPage->addKnob(enablePreRender);
    _imp->enablePreRenderKnob = enablePreRender;

    KnobGroupPtr renderGroup = AppManager::createKnob<KnobGroup>( this, tr("Pre-Render Settings") );
    renderGroup->setName("preRenderSettings");
    renderGroup->setDefaultValue(true);
    mainPage->addKnob(renderGroup);
    _imp->preRenderGroupKnob = renderGroup;

    KnobChoicePtr writeChoice = AppManager::createKnob<KnobChoice>( this, tr("Write Node") );
    writeChoice->setName("writeNode");
    writeChoice->setHintToolTip( tr("Choose here the Write node in the pre-comp from which to render images then specify a frame-range and "
                                    "hit the \"Render\" button.").toStdString() );
    writeChoice->setAnimationEnabled(false);
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("None"));
        writeChoice->populateChoices(choices);
    }
    renderGroup->addKnob(writeChoice);
    _imp->writeNodesKnob = writeChoice;


    KnobIntPtr first = AppManager::createKnob<KnobInt>( this, tr("First-Frame") );
    first->setName("first");
    first->setHintToolTip( tr("The first-frame to render") );
    first->setAnimationEnabled(false);
    first->setEvaluateOnChange(false);
    first->setAddNewLine(false);
    renderGroup->addKnob(first);
    _imp->firstFrameKnob = first;

    KnobIntPtr last = AppManager::createKnob<KnobInt>( this, tr("Last-Frame") );
    last->setName("last");
    last->setHintToolTip( tr("The last-frame to render") );
    last->setAnimationEnabled(false);
    last->setEvaluateOnChange(false);
    last->setAddNewLine(false);
    renderGroup->addKnob(last);
    _imp->lastFrameKnob = last;

    KnobChoicePtr error = AppManager::createKnob<KnobChoice>( this, tr("On Error") );
    error->setName("onError");
    error->setHintToolTip( tr("Indicates the behavior when an image is missing from the render of the pre-comp project").toStdString() );
    error->setAnimationEnabled(false);
    {
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("Load previous", "", tr("Loads the previous frame in the sequence.").toStdString() ));
        choices.push_back(ChoiceOption("Load next", "", tr("Loads the next frame in the sequence.").toStdString()));
        choices.push_back(ChoiceOption("Load nearest", "", tr("Loads the nearest frame in the sequence.").toStdString()));
        choices.push_back(ChoiceOption("Error", "", tr("Fails to render.").toStdString()));
        choices.push_back(ChoiceOption("Black", "", tr("Black Image.").toStdString()));

        error->populateChoices(choices);
    }
    error->setDefaultValue(3);
    renderGroup->addKnob(error);
    _imp->errorBehaviourKnbo = error;

    KnobButtonPtr renderBtn = AppManager::createKnob<KnobButton>( this, tr("Render") );
    renderBtn->setName("render");
    renderGroup->addKnob(renderBtn);
    _imp->preRenderKnob = renderBtn;

    KnobStringPtr outputNode = AppManager::createKnob<KnobString>( this, tr("Output Node") );
    outputNode->setName("outputNode");
    outputNode->setHintToolTip( tr("The script-name of the node to use as output node in the tree of the pre-comp. This can be any node.").toStdString() );
    outputNode->setAnimationEnabled(false);
    outputNode->setSecretByDefault(true);
    mainPage->addKnob(outputNode);
    _imp->outputNodeNameKnob = outputNode;

    KnobStringPtr sublabel = AppManager::createKnob<KnobString>( this, tr("SubLabel") );
    sublabel->setName(kNatronOfxParamStringSublabelName);
    sublabel->setSecretByDefault(true);
    mainPage->addKnob(sublabel);
    _imp->subLabelKnob = sublabel;
} // PrecompNode::initializeKnobs

void
PrecompNode::onKnobsLoaded()
{
    _imp->reloadProject(false);
    _imp->refreshKnobsVisibility();
}

bool
PrecompNode::knobChanged(KnobI* k,
                         ValueChangedReasonEnum reason,
                         ViewSpec /*view*/,
                         double /*time*/,
                         bool /*originatedFromMainThread*/)
{
    bool ret = true;

    if ( (reason != eValueChangedReasonTimeChanged) && ( k == _imp->projectFileNameKnob.lock().get() ) ) {
        _imp->reloadProject(true);
    } else if ( k == _imp->editProjectKnob.lock().get() ) {
        std::string filename = _imp->projectFileNameKnob.lock()->getValue();
        AppInstancePtr appInstance = getApp()->loadProject(filename);
        Q_UNUSED(appInstance);
    } else if ( k == _imp->preRenderKnob.lock().get() ) {
        _imp->launchPreRender();
    } else if ( k == _imp->outputNodeNameKnob.lock().get() ) {
        _imp->refreshOutputNode();
    } else if ( k == _imp->writeNodesKnob.lock().get() ) {
        _imp->createReadNode();
        _imp->setFirstAndLastFrame();
    } else if ( k == _imp->errorBehaviourKnbo.lock().get() ) {
        _imp->setReadNodeErrorChoice();
    } else if ( k == _imp->enablePreRenderKnob.lock().get() ) {
        _imp->refreshKnobsVisibility();
        _imp->refreshOutputNode();
    } else {
        ret = false;
    }

    return ret;
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
        KnobIPtr knob = read->getKnobByName("onMissingFrame");
        if (knob) {
            KnobChoice* choice = dynamic_cast<KnobChoice*>( knob.get() );
            if (choice) {
                choice->setValue( errorBehaviourKnbo.lock()->getValue() );
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
    QFileInfo file( QString::fromUtf8( filename.c_str() ) );

    if ( !file.exists() ) {
        Dialogs::errorDialog( tr("Pre-Comp").toStdString(), tr("Pre-comp file not found.").toStdString() );

        return;
    }
    QString fileUnPathed = file.fileName();

    subLabelKnob.lock()->setValue( fileUnPathed.toStdString() );

    QString path = file.path() + QLatin1Char('/');

    precompInputs.clear();

    ProjectPtr project = app.lock()->getProject();
    project->resetProject();
    {
        //Set a temporary timeline that will be used while loading the project.
        //This is to avoid that the seekFrame call has an effect on this project since they share the same timeline
        TimeLinePtr tmpTimeline( new TimeLine( project.get() ) );
        project->setTimeLine(tmpTimeline);
    }

    bool ok  = project->loadProject( path, fileUnPathed);
    if (!ok) {
        project->resetProject();
    }

    //Switch the timeline to this instance's timeline
    project->setTimeLine( _publicInterface->getApp()->getTimeLine() );

    populateWriteNodesChoice(true, setWriteNodeChoice);

    if (ok) {
        createReadNode();
    }
    refreshOutputNode();
}

void
PrecompNodePrivate::populateWriteNodesChoice(bool setPartOfPrecomp,
                                             bool setWriteNodeChoice)
{
    KnobChoicePtr param = writeNodesKnob.lock();

    if (!param) {
        return;
    }
    std::vector<ChoiceOption> choices;
    choices.push_back(ChoiceOption("None"));

    NodesList nodes;
    app.lock()->getProject()->getNodes_recursive(nodes, true);
    PrecompNodePtr precomp;
    if (setPartOfPrecomp) {
        precomp = boost::dynamic_pointer_cast<PrecompNode>( _publicInterface->shared_from_this() );
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
        if ( (*it)->getEffectInstance()->isWriter() ) {
            choices.push_back( ChoiceOption((*it)->getFullyQualifiedName() ));
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
    ChoiceOption userChoiceNodeName =  writeNodesKnob.lock()->getActiveEntry();

    if (userChoiceNodeName.id == "None") {
        return NodePtr();
    }
    NodePtr writeNode = app.lock()->getProject()->getNodeByFullySpecifiedName(userChoiceNodeName.id);
    if (!writeNode) {
        std::stringstream ss;
        ss << tr("Could not find a node named %1 in the pre-comp project")
            .arg( QString::fromUtf8( userChoiceNodeName.id.c_str() ) ).toStdString();
        Dialogs::errorDialog( tr("Pre-Comp").toStdString(), ss.str() );

        return NodePtr();
    }

    return writeNode;
}

void
PrecompNode::getPrecompInputs(NodesList* nodes) const
{
    QMutexLocker k(&_imp->dataMutex);

    for (NodesWList::const_iterator it = _imp->precompInputs.begin(); it != _imp->precompInputs.end(); ++it) {
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
        oldReadNode->destroyNode(false, false);


        //Reset the pointer
        QMutexLocker k(&dataMutex);
        readNode.reset();
    }

    NodePtr writeNode = getWriteNodeFromPreComp();
    if (!writeNode) {
        return;
    }

    KnobIPtr fileNameKnob = writeNode->getKnobByName(kOfxImageEffectFileParamName);
    if (!fileNameKnob) {
        return;
    }

    KnobOutputFile* fileKnob = dynamic_cast<KnobOutputFile*>( fileNameKnob.get() );
    if (!fileKnob) {
        return;
    }

    std::string pattern = fileKnob->getValue();
    QString qpattern = QString::fromUtf8( pattern.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::string found = appPTR->getReaderPluginIDForFileType(ext);
    if ( found.empty() ) {
        std::stringstream ss;
        ss << tr("No plugin capable of decoding %1 was found")
            .arg( QString::fromUtf8( ext.c_str() ) ).toStdString();
        Dialogs::errorDialog( tr("Pre-Comp").toStdString(), ss.str() );

        return;
    }

    QString readPluginID = QString::fromUtf8( found.c_str() );
    QString fixedNamePrefix = QString::fromUtf8( _publicInterface->getScriptName_mt_safe().c_str() );
    fixedNamePrefix.append( QLatin1Char('_') );
    fixedNamePrefix.append( QString::fromUtf8("readNode") );
    fixedNamePrefix.append( QLatin1Char('_') );

    CreateNodeArgs args( readPluginID.toStdString(), app.lock()->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
    args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
    args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, pattern);


    NodePtr read = app.lock()->createNode(args);
    if (!read) {
        return;
    }

    PrecompNodePtr precomp = boost::dynamic_pointer_cast<PrecompNode>( _publicInterface->shared_from_this() );
    assert(precomp);
    read->setPrecompNode(precomp);

    QObject::connect( read.get(), SIGNAL(persistentMessageChanged()), _publicInterface, SLOT(onReadNodePersistentMessageChanged()) );

    {
        QMutexLocker k(&dataMutex);
        readNode = read;
    }
} // PrecompNodePrivate::createReadNode

void
PrecompNodePrivate::refreshOutputNode()
{
    bool usePreRender = enablePreRenderKnob.lock()->getValue();
    NodePtr outputnode;

    if (!usePreRender) {
        KnobStringPtr outputNodeKnob = outputNodeNameKnob.lock();
        std::string outputNodeName = outputNodeKnob->getValue();

        outputnode = app.lock()->getProject()->getNodeByFullySpecifiedName(outputNodeName);
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

    for (std::map<NodePtr, int>::iterator it = outputs.begin(); it != outputs.end(); ++it) {
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
    KnobIPtr writefirstFrameKnob = writeNode->getKnobByName("firstFrame");
    KnobIPtr writelastFrameKnob = writeNode->getKnobByName("lastFrame");
    KnobInt* firstFrame = dynamic_cast<KnobInt*>( writefirstFrameKnob.get() );
    KnobInt* lastFrame = dynamic_cast<KnobInt*>( writelastFrameKnob.get() );
    if (firstFrame) {
        firstFrameKnob.lock()->setValue( firstFrame->getValue() );
    }
    if (lastFrame) {
        lastFrameKnob.lock()->setValue( lastFrame->getValue() );
    }
}

void
PrecompNodePrivate::refreshReadNodeInput()
{
    assert(readNode);
    KnobIPtr fileNameKnob = readNode->getKnobByName(kOfxImageEffectFileParamName);
    if (!fileNameKnob) {
        return;
    }
    //Remove all images from the cache associated to the reader since we know they are no longer valid.
    //This is a blocking call so that we are sure there's no old image laying around in the cache after this call
    readNode->removeAllImagesFromCache(true);
    readNode->purgeAllInstancesCaches();

    //Force the reader to reload the sequence/video
    fileNameKnob->evaluateValueChange(0, _publicInterface->getApp()->getTimeLine()->currentFrame(), ViewIdx(0), eValueChangedReasonUserEdited);
}

void
PrecompNodePrivate::launchPreRender()
{
    NodePtr output = getWriteNodeFromPreComp();

    if (!output) {
        Dialogs::errorDialog( tr("Pre-Render").toStdString(), tr("Selected write node does not exist.").toStdString() );

        return;
    }
    AppInstance::RenderWork w(dynamic_cast<OutputEffectInstance*>( output->getEffectInstance().get() ),
                              firstFrameKnob.lock()->getValue(),
                              lastFrameKnob.lock()->getValue(),
                              1,
                              false);

    if (w.writer) {
        RenderEnginePtr engine = w.writer->getRenderEngine();
        if (engine) {
            QObject::connect( engine.get(), SIGNAL(renderFinished(int)), _publicInterface, SLOT(onPreRenderFinished()) );
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
    OutputEffectInstance* writer = dynamic_cast<OutputEffectInstance*>( output->getEffectInstance().get() );
    assert(writer);
    if (writer) {
        RenderEnginePtr engine = writer->getRenderEngine();
        if (engine) {
            QObject::disconnect( engine.get(), SIGNAL(renderFinished(int)), this, SLOT(onPreRenderFinished()) );
        }
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
    if ( message.isEmpty() ) {
        clearPersistentMessage(false);
    } else {
        setPersistentMessage( (MessageTypeEnum)type, message.toStdString() );
    }
}

AppInstancePtr
PrecompNode::getPrecompApp() const
{
    return _imp->app.lock();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_PrecompNode.cpp"
