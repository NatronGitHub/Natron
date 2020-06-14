/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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
#include "Engine/EffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Project.h"
#include "Engine/RenderQueue.h"
#include "Engine/RenderEngine.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"

#include "Serialization/KnobSerialization.h"


NATRON_NAMESPACE_ENTER


PluginPtr
PrecompNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_OTHER);
    PluginPtr ret = Plugin::create(PrecompNode::create, PrecompNode::createRenderClone, PLUGINID_NATRON_PRECOMP, "Precomp", 1, 0, grouping);
    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);


    QString desc = tr( "The Precomp node is like a Group node, but references an external Natron project (.ntp) instead.\n"
                      "This allows you to save a subset of the node tree as a separate project. A Precomp node can be useful in at least two ways:\n"
                      "It can be used to reduce portions of the node tree to pre-rendered image inputs. "
                      "This speeds up render time: Natron only has to process the single image input instead of all the nodes within the project. "
                      "Since this is a separate project, you also maintain access to the internal tree and can edit it any time.\n\n"
                      "It enables a collaborative project: while one user works on the main project, others can work on other parts referenced by the Precomp node.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath, NATRON_IMAGES_PATH "precompNodeIcon.png");
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));
  
    return ret;
}


struct PrecompNodePrivate
{
    Q_DECLARE_TR_FUNCTIONS(PrecompNode)

public:
    PrecompNode* _publicInterface; // can not be a smart ptr
    AppInstanceWPtr app;
    KnobFileWPtr projectFileNameKnob;
    KnobButtonWPtr editProjectKnob;
    KnobBoolWPtr enablePreRenderKnob;
    KnobGroupWPtr preRenderGroupKnob;
    KnobChoiceWPtr writeNodesKnob;
    KnobButtonWPtr preRenderKnob;
    KnobIntWPtr firstFrameKnob, lastFrameKnob;
    KnobStringWPtr outputNodeNameKnob;
    KnobChoiceWPtr errorBehaviourKnbo;
    KnobStringWPtr subLabelKnob;
    QMutex dataMutex;

    // To read-back the pre-comp image sequence/video
    NodePtr readNode;
    NodePtr subProjectOutputNode;

    NodePtr groupOutputNode;

    PrecompNodePrivate(PrecompNode* publicInterface)
    : _publicInterface(publicInterface)
    , app()
    , projectFileNameKnob()
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
    , readNode()
    , subProjectOutputNode()
    , groupOutputNode()
    {
    }

    void refreshKnobsVisibility();

    void reloadProject(bool setWriteNodeChoice);

    void createReadNode();

    void setReadNodeErrorChoice();

    void setFirstAndLastFrame();

    void refreshReadNodeInput();

    void populateWriteNodesChoice(bool setWriteNodeChoice);

    NodePtr getWriteNodeFromPreComp() const;

    void refreshOutputNode();

    void launchPreRender();
};

PrecompNode::PrecompNode(const NodePtr& n)
    : NodeGroup(n)
    , _imp( new PrecompNodePrivate(this) )
{
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
        return _imp->subProjectOutputNode;
    } else {
        return _imp->readNode;
    }
}



void
PrecompNode::setupInitialSubGraphState()
{
    NodeGroupPtr thisShared = toNodeGroup(EffectInstance::shared_from_this());
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_OUTPUT, thisShared));
        args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
        NodePtr outputNode = getApp()->createNode(args);
        assert(outputNode);
        if (!outputNode) {
            throw std::runtime_error( tr("NodeGroup cannot create node %1").arg( QLatin1String(PLUGINID_NATRON_OUTPUT) ).toStdString() );
        }
        _imp->groupOutputNode = outputNode;
    }

} // setupInitialSubGraphState

void
PrecompNode::initializeKnobs()
{
    {
        CLArgs args;
        _imp->app = appPTR->newBackgroundInstance(args, true);
    }

    KnobPagePtr mainPage = createKnob<KnobPage>("controlsPage");
    mainPage->setLabel(tr("Controls"));

    {
        KnobFilePtr param = createKnob<KnobFile>("projectFilename");
        param->setLabel(tr("Project Filename (.%1)").arg( QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ));
        param->setHintToolTip( tr("The absolute file path of the project to use as a pre-comp.").toStdString() );
        param->setAnimationEnabled(false);
        param->setAddNewLine(false);
        mainPage->addKnob(param);
        _imp->projectFileNameKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>("editProject");
        param->setLabel(tr("Edit Project..."));
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr("Opens the specified project in a new %1 instance").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        mainPage->addKnob(param);
        _imp->editProjectKnob = param;
    }

    {
        KnobBoolPtr param = createKnob<KnobBool>("preRender");
        param->setLabel(tr("Pre-Render"));
        param->setAnimationEnabled(false);
        param->setDefaultValue(true);
        param->setHintToolTip( tr("When checked the output of this node will be the images read directly from what is rendered "
                                            "by the node indicated by \"Write Node\". If no Write is selected, or if the rendered images do not exist "
                                            "this node will have the behavior determined by the \"On Error\" parameter. "
                                            "To pre-render images, select a write node, a frame-range and hit \"Render\".\n\n"
                                  "When unchecked, this node will output the image rendered by the node indicated in the \"Output Node\" parameter "
                                  "by rendering the full-tree of the sub-project. In that case no writing on disk will occur and the images will be "
                                  "cached with the same policy as if the nodes were used in the active project in the first place.").toStdString() );
        mainPage->addKnob(param);
        _imp->enablePreRenderKnob = param;
    }

    KnobGroupPtr renderGroup = createKnob<KnobGroup>("preRenderSettings");
    renderGroup->setLabel(tr("Pre-Render Settings"));
    renderGroup->setDefaultValue(true);
    mainPage->addKnob(renderGroup);
    _imp->preRenderGroupKnob = renderGroup;

    {
        KnobChoicePtr param = createKnob<KnobChoice>("writeNode");
        param->setLabel(tr("Write Node"));
        param->setHintToolTip( tr("Choose here the Write node in the pre-comp from which to render images then specify a frame-range and "
                                  "hit the \"Render\" button.").toStdString() );
        param->setAnimationEnabled(false);
        {
            std::vector<ChoiceOption> choices;
            choices.push_back(ChoiceOption("None", "", ""));
            param->populateChoices(choices);
        }
        renderGroup->addKnob(param);
        _imp->writeNodesKnob = param;
    }

    {
        KnobIntPtr param = createKnob<KnobInt>("first");
        param->setLabel(tr("First-Frame"));
        param->setHintToolTip( tr("The first-frame to render") );
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        param->setAddNewLine(false);
        renderGroup->addKnob(param);
        _imp->firstFrameKnob = param;
    }
    {
        KnobIntPtr param = createKnob<KnobInt>("last");
        param->setLabel(tr("Last-Frame"));
        param->setHintToolTip( tr("The last-frame to render") );
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        param->setAddNewLine(false);
        renderGroup->addKnob(param);
        _imp->lastFrameKnob = param;
    }
    {
        KnobChoicePtr param = createKnob<KnobChoice>("onError");
        param->setLabel(tr("On Error"));
        param->setHintToolTip( tr("Indicates the behavior when an image is missing from the render of the pre-comp project").toStdString() );
        param->setAnimationEnabled(false);
        {
            std::vector<ChoiceOption> choices;
            choices.push_back(ChoiceOption("Load previous", "", tr("Loads the previous frame in the sequence.").toStdString() ));
            choices.push_back(ChoiceOption("Load next", "", tr("Loads the next frame in the sequence.").toStdString()));
            choices.push_back(ChoiceOption("Load nearest", "", tr("Loads the nearest frame in the sequence.").toStdString()));
            choices.push_back(ChoiceOption("Error", "", tr("Fails to render.").toStdString()));
            choices.push_back(ChoiceOption("Black", "", tr("Black Image.").toStdString()));

            param->populateChoices(choices);
        }
        param->setDefaultValue(3);
        renderGroup->addKnob(param);
        _imp->errorBehaviourKnbo = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>("render");
        param->setLabel(tr("Render") );
        renderGroup->addKnob(param);
        _imp->preRenderKnob = param;
    }
    {
        KnobStringPtr param = createKnob<KnobString>("outputNode");
        param->setLabel(tr("Output Node"));
        param->setHintToolTip( tr("The script-name of the node to use as output node in the tree of the pre-comp. This can be any node.").toStdString() );
        param->setAnimationEnabled(false);
        param->setSecret(true);
        mainPage->addKnob(param);
        _imp->outputNodeNameKnob = param;
    }
    {
        KnobStringPtr sublabel = createKnob<KnobString>(kNatronOfxParamStringSublabelName);
        sublabel->setSecret(true);
        mainPage->addKnob(sublabel);
        _imp->subLabelKnob = sublabel;
    }
} // PrecompNode::initializeKnobs

void
PrecompNode::onKnobsLoaded()
{
    _imp->reloadProject(false);
    _imp->refreshKnobsVisibility();
}

bool
PrecompNode::knobChanged(const KnobIPtr& k,
                         ValueChangedReasonEnum reason,
                         ViewSetSpec /*view*/,
                         TimeValue /*time*/)
{
    bool ret = true;

    if ( (reason != eValueChangedReasonTimeChanged) && ( k == _imp->projectFileNameKnob.lock() ) ) {
        _imp->reloadProject(true);
    } else if ( k == _imp->editProjectKnob.lock() ) {
        std::string filename = _imp->projectFileNameKnob.lock()->getValue();
        AppInstancePtr appInstance = getApp()->loadProject(filename);
        Q_UNUSED(appInstance);
    } else if ( k == _imp->preRenderKnob.lock() ) {
        _imp->launchPreRender();
    } else if ( k == _imp->outputNodeNameKnob.lock() ) {
        _imp->refreshOutputNode();
    } else if ( k == _imp->writeNodesKnob.lock() ) {
        _imp->createReadNode();
        _imp->setFirstAndLastFrame();
    } else if ( k == _imp->errorBehaviourKnbo.lock() ) {
        _imp->setReadNodeErrorChoice();
    } else if ( k == _imp->enablePreRenderKnob.lock() ) {
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
            KnobChoicePtr choice = toKnobChoice(knob);
            if (choice) {
                choice->setValue( errorBehaviourKnbo.lock()->getValue());
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

    subLabelKnob.lock()->setValue( fileUnPathed.toStdString());

    QString path = file.path() + QLatin1Char('/');

    ProjectPtr project = app.lock()->getProject();
    project->resetProject();
    {
        //Set a temporary timeline that will be used while loading the project.
        //This is to avoid that the seekFrame call has an effect on this project since they share the same timeline
        TimeLinePtr tmpTimeline = boost::make_shared<TimeLine>( project.get() );
        project->setTimeLine(tmpTimeline);
    }

    bool ok  = project->loadProject( path, fileUnPathed);
    if (!ok) {
        project->resetProject();
    }

    //Switch the timeline to this instance's timeline
    project->setTimeLine( _publicInterface->getApp()->getTimeLine() );

    populateWriteNodesChoice(setWriteNodeChoice);

    if (ok) {
        createReadNode();
    }
    refreshOutputNode();
}

void
PrecompNodePrivate::populateWriteNodesChoice(bool setWriteNodeChoice)
{
    KnobChoicePtr param = writeNodesKnob.lock();

    if (!param) {
        return;
    }
    std::vector<ChoiceOption> choices;
    choices.push_back(ChoiceOption("None","",""));

    NodesList nodes;
    app.lock()->getProject()->getNodes_recursive(nodes);


    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->getEffectInstance()->isWriter() ) {
            choices.push_back( ChoiceOption((*it)->getFullyQualifiedName(), "", "") );

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
    ChoiceOption userChoiceNodeName =  writeNodesKnob.lock()->getCurrentEntry();

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
PrecompNodePrivate::createReadNode()
{
    NodePtr oldReadNode;
    {
        QMutexLocker k(&dataMutex);
        oldReadNode = readNode;
    }

    if (oldReadNode) {
        //Ensure it is no longer part of the tree
        oldReadNode->destroyNode();


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

    KnobFilePtr fileKnob = toKnobFile(fileNameKnob);
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

    CreateNodeArgsPtr args(CreateNodeArgs::create( readPluginID.toStdString(), app.lock()->getProject() ));
    args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
    args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
    args->addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, pattern);


    NodePtr read = app.lock()->createNode(args);
    if (!read) {
        return;
    }

    groupOutputNode->swapInput(read, 0);

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

        groupOutputNode->swapInput(outputnode, 0);
    } else {
        groupOutputNode->swapInput(readNode, 0);
    }

    //Clear any persistent message set
    _publicInterface->getNode()->clearAllPersistentMessages(false);

    {
        QMutexLocker k(&dataMutex);
        subProjectOutputNode = outputnode;
    }

    ///Notify outputs that the node has changed
    OutputNodesMap outputs;
    NodePtr thisNode = _publicInterface->getNode();
    thisNode->getOutputs(outputs);

    for (OutputNodesMap::iterator it = outputs.begin(); it != outputs.end(); ++it) {
        const NodePtr& output = it->first;

        for (std::list<int>::iterator it2 = it->second.begin(); it2 !=it->second.end(); ++it2) {
            output->onInputChanged(*it2);
        }
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
    KnobIntPtr firstFrame = toKnobInt(writefirstFrameKnob);
    KnobIntPtr lastFrame = toKnobInt(writelastFrameKnob);
    if (firstFrame) {
        firstFrameKnob.lock()->setValue( firstFrame->getValue());
    }
    if (lastFrame) {
        lastFrameKnob.lock()->setValue( lastFrame->getValue());
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
    readNode->getEffectInstance()->purgeCaches_public();

    //Force the reader to reload the sequence/video
    fileNameKnob->evaluateValueChange(DimSpec::all(), TimeValue(_publicInterface->getApp()->getTimeLine()->currentFrame()), ViewSetSpec::all(), eValueChangedReasonUserEdited);
}

void
PrecompNodePrivate::launchPreRender()
{
    NodePtr output = getWriteNodeFromPreComp();

    if (!output) {
        Dialogs::errorDialog( tr("Pre-Render").toStdString(), tr("Selected write node does not exist.").toStdString() );

        return;
    }
    std::string workLabel = tr("Rendering %1").arg(QString::fromUtf8(projectFileNameKnob.lock()->getValue().c_str())).toStdString();
    RenderQueue::RenderWork w(output,
                              workLabel,
                              TimeValue(firstFrameKnob.lock()->getValue()),
                              TimeValue(lastFrameKnob.lock()->getValue()),
                              TimeValue(1),
                              false);

    if (w.treeRoot) {
        RenderEnginePtr engine = w.treeRoot->getRenderEngine();
        if (engine) {
            QObject::connect( engine.get(), SIGNAL(renderFinished(int)), _publicInterface, SLOT(onPreRenderFinished()) );
        }
    }

    std::list<RenderQueue::RenderWork> works;
    works.push_back(w);
    _publicInterface->getApp()->getRenderQueue()->renderNonBlocking(works);
}

void
PrecompNode::onPreRenderFinished()
{
    NodePtr output = getOutputNode();

    if (!output) {
        return;
    }
    EffectInstancePtr writer = output->getEffectInstance();
    assert(writer);
    if (writer) {
        RenderEnginePtr engine = writer->getNode()->getRenderEngine();
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
    PersistentMessageMap messages;

    node->getPersistentMessage(&messages, false);
    if ( messages.empty() ) {
        getNode()->clearAllPersistentMessages(false);
    } else {
        for (PersistentMessageMap::const_iterator it = messages.begin(); it != messages.end(); ++it) {
            getNode()->setPersistentMessage( it->second.type, it->first, it->second.message);
        }

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
