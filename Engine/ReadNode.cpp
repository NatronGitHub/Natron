/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "ReadNode.h"

#include <sstream> // stringstream

#include "Global/QtCompat.h"

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp> // iequals
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif


CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QThread>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Project.h"
#include "Engine/Plugin.h"
#include "Engine/Settings.h"

#include "Serialization/NodeSerialization.h"
#include "Serialization/KnobSerialization.h"



NATRON_NAMESPACE_ENTER


//The plug-in that is instanciated whenever this node is created and doesn't point to any valid or known extension
#define READ_NODE_DEFAULT_READER PLUGINID_OFX_READOIIO
#define kPluginSelectorParamEntryDefault "Default"

#define kNatronPersistentErrorDecoderMissing "NatronPersistentErrorDecoderMissing"

//Generic Reader
#define kParamFilename kOfxImageEffectFileParamName
#define kParamProxy kOfxImageEffectProxyParamName
#define kParamProxyThreshold "proxyThreshold"
#define kParamOriginalProxyScale "originalProxyScale"
#define kParamCustomProxyScale "customProxyScale"
#define kParamOnMissingFrame "onMissingFrame"
#define kParamFrameMode "frameMode"
#define kParamTimeOffset "timeOffset"
#define kParamStartingTime "startingTime"
#define kParamOriginalFrameRange kReaderParamNameOriginalFrameRange
#define kParamFirstFrame "firstFrame"
#define kParamLastFrame "lastFrame"
#define kParamBefore "before"
#define kParamAfter "after"
#define kParamTimeDomainUserEdited "timeDomainUserEdited"
#define kParamFilePremult "filePremult"
#define kParamOutputPremult "outputPremult"
#define kParamOutputComponents "outputComponents"
#define kParamInputSpaceLabel "File Colorspace"
#define kParamFrameRate "frameRate"
#define kParamCustomFps "customFps"
#define kParamInputSpaceSet "ocioInputSpaceSet"
#define kParamExistingInstance "ParamExistingInstance"

//Generic OCIO
#define kOCIOParamConfigFile "ocioConfigFile"
#define kOCIOParamInputSpace "ocioInputSpace"
#define kOCIOParamOutputSpace "ocioOutputSpace"
#define kOCIOParamInputSpaceChoice "ocioInputSpaceIndex"
#define kOCIOParamOutputSpaceChoice "ocioOutputSpaceIndex"
#define kOCIOHelpButton "ocioHelp"
#define kOCIOHelpLooksButton "ocioHelpLooks"
#define kOCIOHelpDisplaysButton "ocioHelpDisplays"
#define kOCIOParamContext "Context"


PluginPtr
ReadNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_IMAGE);
    PluginPtr ret = Plugin::create(ReadNode::create, ReadNode::createRenderClone, PLUGINID_NATRON_READ, "Read", 1, 0, grouping);

    QString desc = tr("Node used to read images or videos from disk. The image/video is identified by its filename and "
                      "its extension. Given the extension, the Reader selected from the Preferences to decode that specific format will be used.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);
    effectDesc->setProperty<bool>(kEffectPropSupportsRenderScale, false);
    ret->setProperty<int>(kNatronPluginPropShortcut, (int)Key_R);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/readImage.png");
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));
    return ret;
}


/*
   These are names of knobs that are defined in GenericReader and that should stay on the interface
   no matter what the internal Reader is.
 */
struct GenericKnob
{
    // The script-name of the knob.
    const char* scriptName;

    // Whether the value should be saved when changing filename.
    bool mustKeepValue;
};

static GenericKnob genericReaderKnobNames[] =
{
    {kParamFilename, false},
    {kParamProxy, false},
    {kParamProxyThreshold, false},
    {kParamOriginalProxyScale, false},
    {kParamCustomProxyScale, false},
    {kParamOnMissingFrame, true},
    {kParamFrameMode, true},
    {kParamTimeOffset, false},
    {kParamStartingTime, false},
    {kParamOriginalFrameRange, false},
    {kParamFirstFrame, false},
    {kParamLastFrame, false},
    {kParamBefore, true},
    {kParamAfter, true},
    {kParamTimeDomainUserEdited, false},
    {kParamFilePremult, true}, // keep: don't change useful params behind the user's back
    {kParamOutputPremult, true}, // keep: don't change useful params behind the user's back
    {kParamOutputComponents, true}, // keep: don't change useful params behind the user's back
    {kParamInputSpaceLabel, false},
    {kParamFrameRate, true}, // keep: don't change useful params behind the user's back
    {kParamCustomFps, true}, // if custom fps was checked, don't uncheck it
    {kParamInputSpaceSet, true},
    {kParamExistingInstance, true}, // don't automatically set parameters when changing the filename, see GenericReaderPlugin::inputFileChanged()

    {kOCIOParamConfigFile, true},
    {kNatronReadNodeOCIOParamInputSpace, false},
    {kOCIOParamInputSpace, false}, // input colorspace must not be kept (depends on file format)
    {kOCIOParamOutputSpace, true}, // output colorspace must be kept
    {kOCIOParamInputSpaceChoice, false},
    {kOCIOParamOutputSpaceChoice, true},
    {kOCIOHelpButton, false},
    {kOCIOHelpLooksButton, false},
    {kOCIOHelpDisplaysButton, false},
    {kOCIOParamContext, false},
    {0, false}
};
static bool
isGenericKnob(const std::string& knobName,
              bool *mustSerialize)
{
    int i = 0;

    while (genericReaderKnobNames[i].scriptName) {
        if (genericReaderKnobNames[i].scriptName == knobName) {
            *mustSerialize = genericReaderKnobNames[i].mustKeepValue;

            return true;
        }
        ++i;
    }

    return false;
}

bool
ReadNode::isBundledReader(const std::string& pluginID)
{
    return ( boost::iequals(pluginID, PLUGINID_OFX_READOIIO) ||
            boost::iequals(pluginID, PLUGINID_OFX_READFFMPEG) ||
            boost::iequals(pluginID, PLUGINID_OFX_READPFM) ||
            boost::iequals(pluginID, PLUGINID_OFX_READPSD) ||
            boost::iequals(pluginID, PLUGINID_OFX_READKRITA) ||
            boost::iequals(pluginID, PLUGINID_OFX_READSVG) ||
            boost::iequals(pluginID, PLUGINID_OFX_READMISC) ||
            boost::iequals(pluginID, PLUGINID_OFX_READORA) ||
            boost::iequals(pluginID, PLUGINID_OFX_READCDR) ||
            boost::iequals(pluginID, PLUGINID_OFX_READPNG) ||
            boost::iequals(pluginID, PLUGINID_OFX_READPDF) );
}

struct ReadNodePrivate
{
    Q_DECLARE_TR_FUNCTIONS(ReadNode)

public:
    ReadNode* _publicInterface; // can not be a smart ptr
    QMutex embeddedPluginMutex;
    NodePtr embeddedPlugin;
    SERIALIZATION_NAMESPACE::KnobSerializationList genericKnobsSerialization;
    KnobFileWPtr inputFileKnob;


    //Thiese are knobs owned by the ReadNode and not the Reader
    KnobChoiceWPtr pluginSelectorKnob;
    KnobSeparatorWPtr separatorKnob;
    KnobButtonWPtr fileInfosKnob;
    std::list<KnobIWPtr> readNodeKnobs;

    NodePtr inputNode, outputNode;

    //MT only
    int creatingReadNode;

    // Plugin-ID of the last read node created.
    // If this is different, we do not load serialized knobs
    std::string lastPluginIDCreated;


    bool wasCreatedAsHiddenNode;


    ReadNodePrivate(ReadNode* publicInterface)
    : _publicInterface(publicInterface)
    , embeddedPluginMutex()
    , embeddedPlugin()
    , genericKnobsSerialization()
    , inputFileKnob()
    , pluginSelectorKnob()
    , separatorKnob()
    , fileInfosKnob()
    , readNodeKnobs()
    , inputNode()
    , outputNode()
    , creatingReadNode(0)
    , lastPluginIDCreated()
    , wasCreatedAsHiddenNode(false)
    {
    }

    void placeReadNodeKnobsInPage();

    void createReadNode(bool throwErrors,
                        const std::string& filename,
                        const SERIALIZATION_NAMESPACE::NodeSerialization* serialization );

    void destroyReadNode();

    void cloneGenericKnobs();

    void refreshPluginSelectorKnob();

    void refreshFileInfoVisibility(const std::string& pluginID);

    void createDefaultReadNode();

    //bool checkDecoderCreated(TimeValue time, ViewIdx view);

    static QString getFFProbeBinaryPath()
    {
        QString appPath = QCoreApplication::applicationDirPath();

        appPath += QLatin1Char('/');
        appPath += QString::fromUtf8("ffprobe");
#ifdef __NATRON_WIN32__
        appPath += QString::fromUtf8(".exe");
#endif

        return appPath;
    }
};


class SetCreatingReaderRAIIFlag
{
    ReadNodePrivate* _p;

public:

    SetCreatingReaderRAIIFlag(ReadNodePrivate* p)
        : _p(p)
    {
        ++p->creatingReadNode;
    }

    ~SetCreatingReaderRAIIFlag()
    {
        --_p->creatingReadNode;
    }
};


ReadNode::ReadNode(const NodePtr& n)
    : NodeGroup(n)
    , _imp( new ReadNodePrivate(this) )
{

}

ReadNode::~ReadNode()
{
}

NodePtr
ReadNode::getEmbeddedReader() const
{
    QMutexLocker k(&_imp->embeddedPluginMutex);
    return _imp->embeddedPlugin;
}

void
ReadNode::setEmbeddedReader(const NodePtr& node)
{
    QMutexLocker k(&_imp->embeddedPluginMutex);
    _imp->embeddedPlugin = node;
}

void
ReadNodePrivate::placeReadNodeKnobsInPage()
{
    KnobPagePtr controlsPage = toKnobPage(_publicInterface->getKnobByName("Controls"));
    if (!controlsPage) {
        return;
    }
    for (std::list<KnobIWPtr>::iterator it = readNodeKnobs.begin(); it != readNodeKnobs.end(); ++it) {
        KnobIPtr knob = it->lock();
        knob->setParentKnob( KnobIPtr() );
        controlsPage->removeKnob(knob);
    }

    KnobsVec children = controlsPage->getChildren();

    int index = -1;
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i]->getName() == kParamCustomFps) {
            index = i;
            break;
        }
    }
    {
        ++index;
        for (std::list<KnobIWPtr>::iterator it = readNodeKnobs.begin(); it != readNodeKnobs.end(); ++it) {
            KnobIPtr knob = it->lock();
            controlsPage->insertKnob(index, knob);
            ++index;
        }
    }

    children = controlsPage->getChildren();
    // Find the separatorKnob in the page and if the next parameter is also a separator, hide it
    int foundSep = -1;
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i]== separatorKnob.lock()) {
            foundSep = i;
            break;
        }
    }
    if (foundSep != -1) {
        ++foundSep;
        if (foundSep < (int)children.size()) {
            bool isSecret = children[foundSep]->getIsSecret();
            while (isSecret && foundSep < (int)children.size()) {
                ++foundSep;
                isSecret = children[foundSep]->getIsSecret();
            }
            if (foundSep < (int)children.size()) {
                separatorKnob.lock()->setSecret( bool( toKnobSeparator(children[foundSep]) ) );
            } else {
                separatorKnob.lock()->setSecret(true);
            }
            
        } else {
            separatorKnob.lock()->setSecret(true);
        }
    }
}

void
ReadNodePrivate::cloneGenericKnobs()
{
    const KnobsVec& knobs = _publicInterface->getKnobs();

    for (SERIALIZATION_NAMESPACE::KnobSerializationList::iterator it = genericKnobsSerialization.begin(); it != genericKnobsSerialization.end(); ++it) {
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ( (*it2)->getName() == (*it)->getName() ) {
                (*it2)->fromSerialization(**it);
                break;
            }
        }
    }
}

void
ReadNodePrivate::destroyReadNode()
{
    assert( QThread::currentThread() == qApp->thread() );
    if (!embeddedPlugin) {
        return;
    }

    // Disconnect the node so it doesn't get used by something else on another thread
    // between the knobs destruction and the node destruction
    outputNode->swapInput(NodePtr(), 0);
    embeddedPlugin->swapInput(NodePtr(), 0);
    {
        KnobsVec knobs = _publicInterface->getKnobs();

        genericKnobsSerialization.clear();

        try {

            for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {

                // The internal node still holds a shared ptr to the knob.
                // Since we want to keep some knobs around, ensure they do not get deleted in the desctructor of the embedded node
                if (!embeddedPlugin->getEffectInstance()->removeKnobFromList(*it)) {
                    continue;
                }


                if ( (*it)->getKnobDeclarationType() != KnobI::eKnobDeclarationTypePlugin ) {
                    continue;
                }

                //If it is a knob of this ReadNode, do not destroy it
                bool isReadNodeKnob = false;
                for (std::list<KnobIWPtr>::iterator it2 = readNodeKnobs.begin(); it2 != readNodeKnobs.end(); ++it2) {
                    if (it2->lock() == *it) {
                        isReadNodeKnob = true;
                        break;
                    }
                }
                if (isReadNodeKnob) {
                    continue;
                }


                //Keep pages around they will be re-used
                KnobPagePtr isPage = toKnobPage(*it);
                if (isPage) {
                    continue;
                }

                //This is a knob of the Reader plug-in

                //Serialize generic knobs and keep them around until we create a new Reader plug-in
                bool mustSerializeKnob;
                bool isGeneric = isGenericKnob( (*it)->getName(), &mustSerializeKnob );

                if (!isGeneric || mustSerializeKnob) {

                    if (!isGeneric && !(*it)->getIsSecret()) {
                        // Don't save the secret state otherwise some knobs could be invisible when cloning the serialization even if we change format
                        (*it)->setSecret(false);
                    }

                    SERIALIZATION_NAMESPACE::KnobSerializationPtr s = boost::make_shared<SERIALIZATION_NAMESPACE::KnobSerialization>();
                    (*it)->toSerialization(s.get());
                    genericKnobsSerialization.push_back(s);
                }
                if (!isGeneric) {
                    try {
                        _publicInterface->deleteKnob(*it, false);
                    } catch (...) {
                        
                    }
                }
            }
        } catch (...) {
            assert(false);
        }

        QMutexLocker k(&embeddedPluginMutex);
        embeddedPlugin->destroyNode();
        embeddedPlugin.reset();
    }
    //This will remove the GUI of non generic parameters
    _publicInterface->recreateKnobs(true);

} // ReadNodePrivate::destroyReadNode

void
ReadNodePrivate::createDefaultReadNode()
{
    CreateNodeArgsPtr args(CreateNodeArgs::create(READ_NODE_DEFAULT_READER, toNodeGroup(_publicInterface->EffectInstance::shared_from_this())));

    args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
    args->setProperty(kCreateNodeArgsPropSilent, true);
    args->setProperty(kCreateNodeArgsPropVolatile, true);
    args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "defaultReadNodeReader");
    args->setProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer, _publicInterface->getNode());
    args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

    // This will avoid throwing errors when creating the reader
    args->addParamDefaultValue<bool>(kParamExistingInstance, true);


    NodePtr node  = _publicInterface->getApp()->createNode(args);

    if (!node) {
        QString error = tr("The IO.ofx.bundle OpenFX plug-in is required to use this node, make sure it is installed.");
        throw std::runtime_error( error.toStdString() );
    }

    {
        QMutexLocker k(&embeddedPluginMutex);
        embeddedPlugin = node;
    }

    separatorKnob.lock()->setSecret(true);
}


static std::string
getFileNameFromSerialization(const SERIALIZATION_NAMESPACE::KnobSerializationList& serializations)
{
    std::string filePattern;

    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = serializations.begin(); it != serializations.end(); ++it) {
        if ( (*it)->getName() == kOfxImageEffectFileParamName && (*it)->_dataType == SERIALIZATION_NAMESPACE::eSerializationValueVariantTypeString) {
            SERIALIZATION_NAMESPACE::ValueSerialization& value = (*it)->_values.begin()->second[0];
            filePattern = value._value.isString;
            break;
        }
    }

    return filePattern;
}

void
ReadNodePrivate::createReadNode(bool throwErrors,
                                const std::string& filename,
                                const SERIALIZATION_NAMESPACE::NodeSerialization* serialization)
{
    if (creatingReadNode) {
        return;
    }

    SetCreatingReaderRAIIFlag creatingNode__(this);
    QString qpattern = QString::fromUtf8( filename.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::string readerPluginID;
    readerPluginID = pluginSelectorKnob.lock()->getCurrentEntry().id;


    if ( readerPluginID.empty() || readerPluginID ==  kPluginSelectorParamEntryDefault) {
        //Use default
        readerPluginID = appPTR->getReaderPluginIDForFileType(ext);
    }

    // If the plug-in is the same, do not create a new decoder.
    if (embeddedPlugin && embeddedPlugin->getPluginID() == readerPluginID) {
        KnobFilePtr fileKnob = inputFileKnob.lock();
        assert(fileKnob);
        if (fileKnob) {
            // Make sure instance changed action is called on the decoder and not caught in our knobChanged handler.
            embeddedPlugin->getEffectInstance()->onKnobValueChanged_public(fileKnob, eValueChangedReasonUserEdited, _publicInterface->getCurrentRenderTime(), ViewSetSpec(0));

        }

        return;
    }
    //Destroy any previous reader
    //This will store the serialization of the generic knobs
    destroyReadNode();

    bool defaultFallback = false;

    if (readerPluginID.empty()) {
        if (throwErrors) {
            QString message = tr("Could not find a decoder to read %1 file format")
            .arg( QString::fromUtf8( ext.c_str() ) );
            throw std::runtime_error( message.toStdString() );
        }
        defaultFallback = true;

    }

    if ( !defaultFallback && !ReadNode::isBundledReader(readerPluginID) ) {
        if (throwErrors) {
            QString message = tr("%1 is not a bundled reader, please create it from the Image->Readers menu or with the tab menu in the Nodegraph")
                              .arg( QString::fromUtf8( readerPluginID.c_str() ) );
            throw std::runtime_error( message.toStdString() );
        }
        defaultFallback = true;
    }


    NodePtr node;
    //Find the appropriate reader
    if (readerPluginID.empty() && !serialization) {
        //Couldn't find any reader
        if ( !ext.empty() ) {
            QString message = tr("No plugin capable of decoding %1 was found.")
                              .arg( QString::fromUtf8( ext.c_str() ) );
            //Dialogs::errorDialog(tr("Read").toStdString(), message.toStdString(), false);
            if (throwErrors) {
                throw std::runtime_error( message.toStdString() );
            }
        }
        defaultFallback = true;
    } else {
        if ( readerPluginID.empty() ) {
            readerPluginID = READ_NODE_DEFAULT_READER;
        }

        CreateNodeArgsPtr args(CreateNodeArgs::create(readerPluginID, toNodeGroup(_publicInterface->EffectInstance::shared_from_this()) ));
        args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty(kCreateNodeArgsPropVolatile, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "internalDecoderNode");
        args->setProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer, _publicInterface->getNode());

        SERIALIZATION_NAMESPACE::NodeSerializationPtr s(new SERIALIZATION_NAMESPACE::NodeSerialization);
        if (serialization) {
            *s = *serialization;
            args->setProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization, s);
        }

        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

        if (serialization || wasCreatedAsHiddenNode) {
            args->setProperty<bool>(kCreateNodeArgsPropSilent, true);
            args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true); // also load deprecated plugins
        }


        node = _publicInterface->getApp()->createNode(args);

        // Duplicate all knobs

        // Set the filename value
        if (node) {
            KnobFilePtr fileKnob = boost::dynamic_pointer_cast<KnobFile>(node->getKnobByName(kOfxImageEffectFileParamName));
            if (fileKnob) {
                fileKnob->setValue(filename);
            }
        }
        {
            QMutexLocker k(&embeddedPluginMutex);
            embeddedPlugin = node;
        }
        placeReadNodeKnobsInPage();

    }


    if (!node) {
        defaultFallback = true;
    }

    // Refreh sub-graph connections
    if (outputNode) {
        if (node) {
            outputNode->swapInput(node, 0);
            node->swapInput(inputNode, 0);
        } else {
            outputNode->swapInput(inputNode, 0);
        }
    }
    
    if (defaultFallback) {
        createDefaultReadNode();
    }

    //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
    _publicInterface->getNode()->declarePythonKnobs();


    // Clone the old values of the generic knobs if we created the same decoder than before
    if (lastPluginIDCreated == readerPluginID) {
        cloneGenericKnobs();
    }
    lastPluginIDCreated = readerPluginID;

    //This will refresh the GUI with this Reader specific parameters
    _publicInterface->recreateKnobs(true);

    _publicInterface->getNode()->s_mustRefreshPluginInfo();

    KnobIPtr knob = node ? node->getKnobByName(kOfxImageEffectFileParamName) : _publicInterface->getKnobByName(kOfxImageEffectFileParamName);
    if (knob) {
        inputFileKnob = toKnobFile(knob);
    }
} // ReadNodePrivate::createReadNode

void
ReadNodePrivate::refreshFileInfoVisibility(const std::string& pluginID)
{
    KnobButtonPtr fileInfos = fileInfosKnob.lock();
    KnobIPtr hasMetadataKnob = _publicInterface->getKnobByName("showMetadata");
    bool hasFfprobe = false;
    if (!hasMetadataKnob) {
        QString ffprobePath = getFFProbeBinaryPath();
        hasFfprobe = QFile::exists(ffprobePath);
    } else {
        hasMetadataKnob->setSecret(true);
    }


    if ( hasMetadataKnob || ( ReadNode::isVideoReader(pluginID) && hasFfprobe ) ) {
        fileInfos->setSecret(false);
    } else {
        fileInfos->setSecret(true);
    }
}

void
ReadNodePrivate::refreshPluginSelectorKnob()
{
    KnobFilePtr fileKnob = inputFileKnob.lock();

    assert(fileKnob);
    std::string filePattern = fileKnob->getRawFileName();
    std::vector<ChoiceOption> entries;

    entries.push_back(ChoiceOption(kPluginSelectorParamEntryDefault, "", ReadNode::tr("Use the default plug-in chosen from the Preferences to read this file format.").toStdString()));

    QString qpattern = QString::fromUtf8( filePattern.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::string pluginID;
    if ( !ext.empty() ) {
        pluginID = appPTR->getReaderPluginIDForFileType(ext);
        IOPluginSetForFormat readersForFormat;
        appPTR->getReadersForFormat(ext, &readersForFormat);

        // Reverse it so that we sort them by decreasing score order
        for (IOPluginSetForFormat::reverse_iterator it = readersForFormat.rbegin(); it != readersForFormat.rend(); ++it) {
            PluginPtr plugin;
            try {
                plugin = appPTR->getPluginBinary(QString::fromUtf8( it->pluginID.c_str() ), -1, -1, false);
            } catch (...) {
                
            }

            QString tooltip = tr("Use %1 version %2.%3 to read this file format.").arg(QString::fromUtf8(plugin->getPluginLabel().c_str())).arg( plugin->getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 0)).arg(plugin->getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 1));
            entries.push_back( ChoiceOption(plugin->getPluginID(), "", tooltip.toStdString()));

        }
    }

    KnobChoicePtr pluginChoice = pluginSelectorKnob.lock();

    pluginChoice->populateChoices(entries);
    pluginChoice->blockValueChanges();
    pluginChoice->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
    pluginChoice->unblockValueChanges();
    if (entries.size() <= 2) {
        pluginChoice->setSecret(true);
    } else {
        pluginChoice->setSecret(false);
    }

    refreshFileInfoVisibility(pluginID);

} // ReadNodePrivate::refreshPluginSelectorKnob

bool
ReadNode::isReader() const
{
    return true;
}

// static
bool
ReadNode::isVideoReader(const std::string& pluginID)
{
    return (pluginID == PLUGINID_OFX_READFFMPEG);
}

bool
ReadNode::isVideoReader() const
{
    NodePtr p = getEmbeddedReader();

    return p ? isVideoReader( p->getPluginID() ) : false;
}

bool
ReadNode::isGenerator() const
{
    return true;
}


void
ReadNode::initializeKnobs()
{
    KnobPagePtr controlpage = createKnob<KnobPage>("Controls");
    controlpage->setLabel(tr("Controls"));
    {
        KnobButtonPtr param = createKnob<KnobButton>("fileInfo");
        param->setLabel(tr("File Info..."));
        param->setHintToolTip( tr("Press to display informations about the file") );
        controlpage->addKnob(param);
        _imp->fileInfosKnob = param;
        _imp->readNodeKnobs.push_back(param);
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kNatronReadNodeParamDecodingPluginChoice);
        param->setAnimationEnabled(false);
        param->setLabel(tr("Decoder"));
        param->setHintToolTip( tr("Select the internal decoder plug-in used for this file format. By default this uses "
                                           "the plug-in selected for this file extension in the Preferences of Natron") );
        param->setEvaluateOnChange(false);
        _imp->pluginSelectorKnob = param;
        controlpage->addKnob(param);
        _imp->readNodeKnobs.push_back(param);

    }
    {
        KnobSeparatorPtr param = createKnob<KnobSeparator>("decoderOptionsSeparator");
        param->setLabel(tr("Decoder Options"));
        param->setHintToolTip( tr("Below can be found parameters that are specific to the Reader plug-in.") );
        controlpage->addKnob(param);
        _imp->separatorKnob = param;
        _imp->readNodeKnobs.push_back(param);
    }
}

void
ReadNode::setupInitialSubGraphState()
{
    NodeGroupPtr thisShared = toNodeGroup(EffectInstance::shared_from_this());
    NodePtr inputNode, outputNode;
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_OUTPUT, thisShared));
        args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
        outputNode = getApp()->createNode(args);
        assert(outputNode);
        if (!outputNode) {
            throw std::runtime_error( tr("NodeGroup cannot create node %1").arg( QLatin1String(PLUGINID_NATRON_OUTPUT) ).toStdString() );
        }
        _imp->outputNode = outputNode;
    }
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, thisShared));
        args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty(kCreateNodeArgsPropVolatile, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "Source");
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
        inputNode = getApp()->createNode(args);
        assert(inputNode);
        if (!inputNode) {
            throw std::runtime_error( tr("NodeGroup cannot create node %1").arg( QLatin1String(PLUGINID_NATRON_INPUT) ).toStdString() );
        }
        _imp->inputNode = inputNode;
    }
    if (_imp->embeddedPlugin) {
        outputNode->swapInput(_imp->embeddedPlugin, 0);
        _imp->embeddedPlugin->swapInput(inputNode, 0);
    } else {
        outputNode->connectInput(inputNode, 0);
    }

} // setupInitialSubGraphState

void
ReadNode::onEffectCreated(const CreateNodeArgs& args)
{
    //If we already loaded the Reader, do not do anything
    NodePtr p = getEmbeddedReader();
    if (p) {
        // Ensure the plug-in ID knob has the same value as the created reader:
        // The reader might have been created in onKnobsAboutToBeLoaded() however the knobs
        // get loaded afterwards and the plug-in ID could not reflect the underlying plugin
        KnobChoicePtr pluginIDKnob = _imp->pluginSelectorKnob.lock();
        if (pluginIDKnob) {
            pluginIDKnob->setActiveEntry(ChoiceOption(p->getPluginID()), ViewIdx(0), eValueChangedReasonPluginEdited);
        }
        return;
    }

    _imp->wasCreatedAsHiddenNode = args.getPropertyUnsafe<bool>(kCreateNodeArgsPropNoNodeGUI);


    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization = args.getPropertyUnsafe<SERIALIZATION_NAMESPACE::NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    if (serialization) {
        return;
    }

    bool throwErrors = false;
    std::string pattern;

    std::vector<std::string> defaultParamValues = args.getPropertyNUnsafe<std::string>(kCreateNodeArgsPropNodeInitialParamValues);
    std::vector<std::string>::iterator foundFileName  = std::find(defaultParamValues.begin(), defaultParamValues.end(), std::string(kOfxImageEffectFileParamName));
    if (foundFileName != defaultParamValues.end()) {
        std::string propName(kCreateNodeArgsPropParamValue);
        propName += "_";
        propName += kOfxImageEffectFileParamName;
        pattern = args.getPropertyUnsafe<std::string>(propName);
    }


    _imp->createReadNode( throwErrors, pattern, serialization.get() );
    _imp->refreshPluginSelectorKnob();
}

void
ReadNode::onKnobsAboutToBeLoaded(const SERIALIZATION_NAMESPACE::NodeSerialization& serialization)
{
    NodePtr node = getNode();

    //Load the pluginID to create first.
    node->loadKnob( _imp->pluginSelectorKnob.lock(), serialization._knobsValues );


    std::string filename = getFileNameFromSerialization( serialization._knobsValues );
    //Create the Reader with the serialization
    _imp->createReadNode(false, filename, &serialization);
    _imp->refreshPluginSelectorKnob();
}

bool
ReadNode::knobChanged(const KnobIPtr& k,
                      ValueChangedReasonEnum reason,
                      ViewSetSpec view,
                      TimeValue time)
{
    bool ret =  true;

    if ( ( k == _imp->inputFileKnob.lock() ) && (reason != eValueChangedReasonTimeChanged) ) {


        if (_imp->creatingReadNode) {
            NodePtr p = getEmbeddedReader();
            if (p) {
                p->getEffectInstance()->knobChanged(k, reason, view, time);
            }

            return false;
        }
        try {
            _imp->refreshPluginSelectorKnob();
        } catch (const std::exception& /*e*/) {
            assert(false);
        }

        KnobFilePtr fileKnob = _imp->inputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getRawFileName();
        try {
            _imp->createReadNode( false, filename, 0 );
            getNode()->clearPersistentMessage(kNatronPersistentErrorDecoderMissing);
        } catch (const std::exception& e) {
            getNode()->setPersistentMessage( eMessageTypeError, kNatronPersistentErrorDecoderMissing, e.what() );
        }

    } else if ( k == _imp->pluginSelectorKnob.lock() && reason != eValueChangedReasonPluginEdited ) {
        ChoiceOption entry = _imp->pluginSelectorKnob.lock()->getCurrentEntry();


        if (entry.id == "Default") {
            entry.id.clear();
        }


        KnobFilePtr fileKnob = _imp->inputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getRawFileName();

        try {
            _imp->createReadNode( false, filename, 0 );
            getNode()->clearPersistentMessage(kNatronPersistentErrorDecoderMissing);
        } catch (const std::exception& e) {
            getNode()->setPersistentMessage( eMessageTypeError, kNatronPersistentErrorDecoderMissing, e.what() );
        }

        // Make sure instance changed action is called on the decoder and not caught in our knobChanged handler.
        if (_imp->embeddedPlugin) {
            _imp->embeddedPlugin->getEffectInstance()->onKnobValueChanged_public(fileKnob, eValueChangedReasonUserEdited, getTimelineCurrentTime(), ViewSetSpec(0));
        }



        _imp->refreshFileInfoVisibility(entry.id);
    } else if ( k == _imp->fileInfosKnob.lock() ) {

        NodePtr p = getEmbeddedReader();
        if (!p) {
            return false;
        }


        KnobIPtr hasMetadataKnob = p->getKnobByName("showMetadata");
        if (hasMetadataKnob) {
            KnobButtonPtr showMetasKnob = toKnobButton(hasMetadataKnob);
            if (showMetasKnob) {
                showMetasKnob->trigger();
            }
        } else {
            if ( isVideoReader( p->getPluginID() ) ) {
                QString ffprobePath = ReadNodePrivate::getFFProbeBinaryPath();
                if( !QFile::exists(ffprobePath) ) {
                    throw std::runtime_error( tr("Error: ffprobe binary not available").toStdString() );
                }
                QProcess proc;
                QStringList ffprobeArgs;
                ffprobeArgs << QString::fromUtf8("-show_streams");
                KnobFilePtr fileKnob = _imp->inputFileKnob.lock();
                assert(fileKnob);
                std::string filename = fileKnob->getValue();
                getApp()->getProject()->canonicalizePath(filename); // substitute project variables
                ffprobeArgs << QString::fromUtf8( filename.c_str() );
                proc.start(ffprobePath, ffprobeArgs);
                proc.waitForFinished();

                QString procStdError = QString::fromUtf8( proc.readAllStandardError() );
                QString procStdOutput = QString::fromUtf8( proc.readAllStandardOutput() );
                Dialogs::informationDialog( getNode()->getLabel(), procStdError.toStdString() + procStdOutput.toStdString() );
            }
        }
    } else {
        ret = false;
    }

    NodePtr p = getEmbeddedReader();
    if (!ret && p) {
        ret |= p->getEffectInstance()->knobChanged(k, reason, view, time);
    }

    return ret;
} // ReadNode::knobChanged

void
ReadNode::refreshDynamicProperties()
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->refreshDynamicProperties();
    }
}

ActionRetCodeEnum
ReadNode::getFrameRange(double *first, double *last)
{
    NodePtr p = getEmbeddedReader();
    if (!p) {
        return EffectInstance::getFrameRange(first, last);
    } else {
        return p->getEffectInstance()->getFrameRange(first, last);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ReadNode.cpp"
