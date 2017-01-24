/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "WriteNode.h"

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
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <ofxNatron.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Serialization/NodeSerialization.h"
#include "Serialization/KnobSerialization.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/Settings.h"

//The plug-in that is instanciated whenever this node is created and doesn't point to any valid or known extension
#define WRITE_NODE_DEFAULT_WRITER PLUGINID_OFX_WRITEOIIO
#define kPluginSelectorParamEntryDefault "Default"

NATRON_NAMESPACE_ENTER;

//Generic Writer
#define kParamFilename kOfxImageEffectFileParamName
#define kParamOutputFormat kNatronParamFormatChoice
#define kParamFormatType "formatType"
#define kParamFormatSize kNatronParamFormatSize
#define kParamFormatPar kNatronParamFormatPar
#define kParamFrameRange "frameRange"
#define kParamFirstFrame "firstFrame"
#define kParamLastFrame "lastFrame"
#define kParamInputPremult "inputPremult"
#define kParamClipInfo "clipInfo"
#define kParamOutputSpaceLabel "File Colorspace"
#define kParamClipToProject "clipToProject"
#define kNatronOfxParamProcessR      "NatronOfxParamProcessR"
#define kNatronOfxParamProcessG      "NatronOfxParamProcessG"
#define kNatronOfxParamProcessB      "NatronOfxParamProcessB"
#define kNatronOfxParamProcessA      "NatronOfxParamProcessA"
#define kParamOutputSpaceSet "ocioOutputSpaceSet"
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

/*
   These are names of knobs that are defined in GenericWriter and that should stay on the interface
   no matter what the internal Reader is.
 */
struct GenericKnob
{
    const char* scriptName;
    bool mustKeepValue;
};

static GenericKnob genericWriterKnobNames[] =
{
    {kParamFilename, false},
    {kParamOutputFormat, true},
    {kParamFormatType, true},
    {kParamFormatSize, true},
    {kParamFormatPar, true},
    {kParamFrameRange, true},
    {kParamFirstFrame, true},
    {kParamLastFrame, true},
    {kParamInputPremult, true}, // keep: don't change useful params behind the user's back
    {kParamClipInfo, false},
    {kParamOutputSpaceLabel, false},
    {kParamClipToProject, true}, // keep: don't change useful params behind the user's back
    {kNatronOfxParamProcessR, true},
    {kNatronOfxParamProcessG, true},
    {kNatronOfxParamProcessB, true},
    {kNatronOfxParamProcessA, true},
    {kParamOutputSpaceSet, true}, // keep: don't change useful params behind the user's back
    {kParamExistingInstance, true}, // don't automatically set parameters when changing the filename, see GenericWriterPlugin::outputFileChanged()


    {kOCIOParamConfigFile, true},
    {kOCIOParamInputSpace, true}, // keep: don't change useful params behind the user's back
    {kOCIOParamOutputSpace, true}, // keep: don't change useful params behind the user's back
    {kOCIOParamInputSpaceChoice, false},
    {kOCIOParamOutputSpaceChoice, false},
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

    while (genericWriterKnobNames[i].scriptName) {
        if (genericWriterKnobNames[i].scriptName == knobName) {
            *mustSerialize = genericWriterKnobNames[i].mustKeepValue;

            return true;
        }
        ++i;
    }

    return false;
}


bool
WriteNode::isBundledWriter(const std::string& pluginID)
{
    return (pluginID == PLUGINID_OFX_WRITEOIIO ||
            pluginID == PLUGINID_OFX_WRITEFFMPEG ||
            pluginID == PLUGINID_OFX_WRITEPFM ||
            pluginID == PLUGINID_OFX_WRITEPNG);
}

PluginPtr
WriteNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_IMAGE);
    PluginPtr ret = Plugin::create((void*)WriteNode::create, PLUGINID_NATRON_WRITE, "Write", 1, 0, grouping);

    QString desc = tr("Node used to write images or videos on disk. The image/video is identified by its filename and "
                      "its extension. Given the extension, the Writer selected from the Preferences to encode that specific format will be used.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    ret->setProperty<int>(kNatronPluginPropShortcut, (int)Key_W);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath, "Images/writeImage.png");
    return ret;
}

struct WriteNodePrivate
{
    Q_DECLARE_TR_FUNCTIONS(WriteNode)

public:
    WriteNode* _publicInterface; // can not be a smart ptr
    NodeWPtr embeddedPlugin, readBackNode, inputNode, outputNode;
    SERIALIZATION_NAMESPACE::KnobSerializationList genericKnobsSerialization;
    KnobFileWPtr outputFileKnob;


    //Thiese are knobs owned by the ReadNode and not the Reader
    KnobIntWPtr frameIncrKnob;
    KnobBoolWPtr readBackKnob;
    KnobChoiceWPtr pluginSelectorKnob;
    KnobStringWPtr pluginIDStringKnob;
    KnobSeparatorWPtr separatorKnob;
    KnobButtonWPtr renderButtonKnob;
    std::list<KnobIWPtr > writeNodeKnobs;

    //MT only
    int creatingWriteNode;


    WriteNodePrivate(WriteNode* publicInterface)
        : _publicInterface(publicInterface)
        , embeddedPlugin()
        , readBackNode()
        , inputNode()
        , outputNode()
        , genericKnobsSerialization()
        , outputFileKnob()
        , frameIncrKnob()
        , pluginSelectorKnob()
        , pluginIDStringKnob()
        , separatorKnob()
        , renderButtonKnob()
        , writeNodeKnobs()
        , creatingWriteNode(0)
    {
    }

    void placeWriteNodeKnobsInPage();

    void createReadNodeAndConnectGraph(const std::string& filename);

    void createWriteNode(bool throwErrors, const std::string& filename, const SERIALIZATION_NAMESPACE::NodeSerialization* serialization);

    void destroyWriteNode();

    void cloneGenericKnobs();

    void refreshPluginSelectorKnob();

    void createDefaultWriteNode();

    bool checkEncoderCreated(TimeValue time, ViewIdx view);

    void setReadNodeOriginalFrameRange();
};


class SetCreatingWriterRAIIFlag
{
    WriteNodePrivate* _p;

public:

    SetCreatingWriterRAIIFlag(WriteNodePrivate* p)
        : _p(p)
    {
        ++p->creatingWriteNode;
    }

    ~SetCreatingWriterRAIIFlag()
    {
        --_p->creatingWriteNode;
    }
};


WriteNode::WriteNode(const NodePtr& n)
    : NodeGroup(n)
    , _imp( new WriteNodePrivate(this) )
{
}

WriteNode::~WriteNode()
{
}

NodePtr
WriteNode::getEmbeddedWriter() const
{
    return _imp->embeddedPlugin.lock();
}

void
WriteNode::setEmbeddedWriter(const NodePtr& node)
{
    _imp->embeddedPlugin = node;
}

void
WriteNodePrivate::placeWriteNodeKnobsInPage()
{
    KnobIPtr pageKnob = _publicInterface->getKnobByName("Controls");
    KnobPagePtr isPage = toKnobPage(pageKnob);

    if (!isPage) {
        return;
    }
    for (std::list<KnobIWPtr >::iterator it = writeNodeKnobs.begin(); it != writeNodeKnobs.end(); ++it) {
        KnobIPtr knob = it->lock();
        knob->setParentKnob( KnobIPtr() );
        isPage->removeKnob(knob);
    }
    KnobsVec children = isPage->getChildren();
    int index = -1;
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i]->getName() == kParamLastFrame) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        ++index;
        for (std::list<KnobIWPtr >::iterator it = writeNodeKnobs.begin(); it != writeNodeKnobs.end(); ++it) {
            KnobIPtr knob = it->lock();
            isPage->insertKnob(index, knob);
            ++index;
        }
    }

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
                separatorKnob.lock()->setSecret(toKnobSeparator(children[foundSep]));
            } else {
                separatorKnob.lock()->setSecret(true);
            }

        } else {
            separatorKnob.lock()->setSecret(true);
        }
    }

    //Set the render button as the last knob
    KnobButtonPtr renderB = renderButtonKnob.lock();
    if (renderB) {
        renderB->setParentKnob( KnobIPtr() );
        isPage->removeKnob( renderB );
        isPage->addKnob(renderB);
    }
}

void
WriteNodePrivate::cloneGenericKnobs()
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
WriteNodePrivate::destroyWriteNode()
{
    assert( QThread::currentThread() == qApp->thread() );
    NodePtr embeddedNode = embeddedPlugin.lock();
    if (!embeddedNode) {
        return;
    }

    genericKnobsSerialization.clear();

    {
        KnobsVec knobs = _publicInterface->getKnobs();
        
        try {


            for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {


                // The internal node still holds a shared ptr to the knob.
                // Since we want to keep some knobs around, ensure they do not get deleted in the desctructor of the embedded node
                embeddedNode->getEffectInstance()->removeKnobFromList(*it);

                if ( !(*it)->isDeclaredByPlugin() ) {
                    continue;
                }

                //If it is a knob of this WriteNode, do not destroy it
                bool isWriteNodeKnob = false;
                for (std::list<KnobIWPtr >::iterator it2 = writeNodeKnobs.begin(); it2 != writeNodeKnobs.end(); ++it2) {
                    if (it2->lock() == *it) {
                        isWriteNodeKnob = true;
                        break;
                    }
                }
                if (isWriteNodeKnob) {
                    continue;

                }

                //Keep pages around they will be re-used

                {
                    KnobPagePtr isPage = toKnobPage(*it);
                    if (isPage) {
                        continue;
                    }
                }

                //This is a knob of the Writer plug-in

                //Serialize generic knobs and keep them around until we create a new Writer plug-in
                bool mustSerializeKnob;
                bool isGeneric = isGenericKnob( (*it)->getName(), &mustSerializeKnob );
                if (!isGeneric || mustSerializeKnob) {

                    if (!isGeneric && !(*it)->getIsSecret()) {
                        // Don't save the secret state otherwise some knobs could be invisible when cloning the serialization even if we change format
                        (*it)->setSecret(false);
                    }
                    SERIALIZATION_NAMESPACE::KnobSerializationPtr s( new SERIALIZATION_NAMESPACE::KnobSerialization );
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
    }
    
    
    //This will remove the GUI of non generic parameters
    _publicInterface->recreateKnobs(true);

    if (embeddedNode) {
        embeddedNode->destroyNode(true, false);
    }
    embeddedPlugin.reset();

    NodePtr readBack = readBackNode.lock();
    if (readBack) {
        readBack->destroyNode(true, false);
    }
    readBackNode.reset();
} // WriteNodePrivate::destroyWriteNode

void
WriteNodePrivate::createDefaultWriteNode()
{
    NodeGroupPtr isGrp = toNodeGroup( _publicInterface->shared_from_this() );
    CreateNodeArgsPtr args(CreateNodeArgs::create( WRITE_NODE_DEFAULT_WRITER, isGrp ));
    args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
    args->setProperty(kCreateNodeArgsPropVolatile, true);
    args->setProperty(kCreateNodeArgsPropSilent, true);
    args->setProperty(kCreateNodeArgsPropMetaNodeContainer, _publicInterface->getNode());
    args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "defaultWriteNodeWriter");
    args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
    embeddedPlugin = _publicInterface->getApp()->createNode(args);

    if ( !embeddedPlugin.lock() ) {
        QString error = tr("The IO.ofx.bundle OpenFX plug-in is required to use this node, make sure it is installed.");
        throw std::runtime_error( error.toStdString() );
    }


    //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
    _publicInterface->getNode()->declarePythonKnobs();

    //Destroy it to keep the default parameters
    destroyWriteNode();
    placeWriteNodeKnobsInPage();
    separatorKnob.lock()->setSecret(true);
}

bool
WriteNodePrivate::checkEncoderCreated(TimeValue time,
                                      ViewIdx view)
{
    KnobFilePtr fileKnob = outputFileKnob.lock();

    assert(fileKnob);
    std::string pattern = fileKnob->getValueAtTime( TimeValue(std::floor(time + 0.5)), DimIdx(0), ViewIdx( view.value() ) );
    if ( pattern.empty() ) {
        _publicInterface->setPersistentMessage( eMessageTypeError, tr("Filename is empty.").toStdString() );

        return false;
    }
    if ( !embeddedPlugin.lock() ) {
        QString s = tr("Encoder was not created for %1. Check that the file exists and its format is supported.")
                    .arg( QString::fromUtf8( pattern.c_str() ) );
        _publicInterface->setPersistentMessage( eMessageTypeError, s.toStdString() );

        return false;
    }

    return true;
}

static std::string
getFileNameFromSerialization(const SERIALIZATION_NAMESPACE::KnobSerializationList& serializations)
{
    std::string filePattern;

    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = serializations.begin(); it != serializations.end(); ++it) {
        if ( (*it)->getName() == kOfxImageEffectFileParamName && (*it)->getTypeName() == KnobFile::typeNameStatic()) {
            SERIALIZATION_NAMESPACE::ValueSerialization& value = (*it)->_values.begin()->second[0];
            filePattern = value._value.isString;
            break;
        }
    }

    return filePattern;
}

void
WriteNodePrivate::setReadNodeOriginalFrameRange()
{
    NodePtr readNode = readBackNode.lock();

    if (!readNode) {
        return;
    }
    NodePtr writeNode = embeddedPlugin.lock();
    if (!writeNode) {
        return;
    }
    RangeD range = {1., 1.};
    {
        GetFrameRangeResultsPtr results;
        ActionRetCodeEnum stat = writeNode->getEffectInstance()->getFrameRange_public(TreeRenderNodeArgsPtr(), &results);
        if (!isFailureRetCode(stat)) {
            results->getFrameRangeResults(&range);
        }
    }

    {
        KnobIPtr originalFrameRangeKnob = readNode->getKnobByName(kReaderParamNameOriginalFrameRange);
        assert(originalFrameRangeKnob);
        KnobIntPtr originalFrameRange = toKnobInt( originalFrameRangeKnob );
        if (originalFrameRange) {
            std::vector<int> values(2);
            values[0] = range.min;
            values[1] = range.max;
            originalFrameRange->setValueAcrossDimensions(values);
        }
    }
    {
        KnobIPtr firstFrameKnob = readNode->getKnobByName(kParamFirstFrame);
        assert(firstFrameKnob);
        KnobIntPtr firstFrame = toKnobInt( firstFrameKnob );
        if (firstFrame) {
            firstFrame->setValue(range.min);
        }
    }
    {
        KnobIPtr lastFrameKnob = readNode->getKnobByName(kParamLastFrame);
        assert(lastFrameKnob);
        KnobIntPtr lastFrame = toKnobInt( lastFrameKnob );
        if (lastFrame) {
            lastFrame->setValue(range.max);
        }
    }
}

void
WriteNodePrivate::createReadNodeAndConnectGraph(const std::string& filename)
{
    QString qpattern = QString::fromUtf8( filename.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    NodeGroupPtr isGrp = toNodeGroup( _publicInterface->shared_from_this() );
    std::string readerPluginID = appPTR->getReaderPluginIDForFileType(ext);
    NodePtr writeNode = embeddedPlugin.lock();

    readBackNode.reset();
    if ( !readerPluginID.empty() ) {
        CreateNodeArgsPtr args(CreateNodeArgs::create(readerPluginID, isGrp ));
        args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty(kCreateNodeArgsPropVolatile, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "internalDecoderNode");
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

        //Set a pre-value for the inputfile knob only if it did not exist
        if ( !filename.empty() ) {
            args->addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
        }

        if (writeNode) {

            RangeD range = {1., 1.};
            {
                GetFrameRangeResultsPtr results;
                ActionRetCodeEnum stat = writeNode->getEffectInstance()->getFrameRange_public(TreeRenderNodeArgsPtr(), &results);
                if (!isFailureRetCode(stat)) {
                    results->getFrameRangeResults(&range);
                }
            }


            std::vector<int> originalRange(2);
            originalRange[0] = (int)range.min;
            originalRange[1] = (int)range.max;
            args->addParamDefaultValueN<int>(kReaderParamNameOriginalFrameRange, originalRange);
            args->addParamDefaultValue<int>(kParamFirstFrame, (int)range.min);
            args->addParamDefaultValue<int>(kParamFirstFrame, (int)range.max);
        }


        readBackNode = _publicInterface->getApp()->createNode(args);
    }

    NodePtr input = inputNode.lock(), output = outputNode.lock();
    assert(input && output);
    bool connectOutputToInput = true;
    if (writeNode) {
        writeNode->swapInput(input, 0);
        NodePtr readNode = readBackNode.lock();
        if (readNode) {

            bool readFile = readBackKnob.lock()->getValue();
            if (readFile) {
                output->swapInput(readNode, 0);
                connectOutputToInput = false;
            }
            readNode->swapInput(input, 0);
            // sync the output colorspace of the reader from input colorspace of the writer

            KnobIPtr outputWriteColorSpace = writeNode->getKnobByName(kOCIOParamOutputSpace);
            KnobIPtr inputReadColorSpace = readNode->getKnobByName(kNatronReadNodeOCIOParamInputSpace);
            if (inputReadColorSpace && outputWriteColorSpace) {
                inputReadColorSpace->linkTo(outputWriteColorSpace);
            }
        }
    }
    if (connectOutputToInput) {
        output->swapInput(input, 0);
    }

} // WriteNodePrivate::createReadNodeAndConnectGraph

void
WriteNodePrivate::createWriteNode(bool throwErrors,
                                  const std::string& filename,
                                  const SERIALIZATION_NAMESPACE::NodeSerialization* serialization)
{
    if (creatingWriteNode) {
        return;
    }

    NodeGroupPtr isGrp = toNodeGroup( _publicInterface->shared_from_this() );
    NodePtr input = inputNode.lock(), output = outputNode.lock();
    //NodePtr maskInput;
    assert( (input && output) || (!input && !output) );
    if (!output) {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_OUTPUT, isGrp));
        args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
        output = _publicInterface->getApp()->createNode(args);
    
        assert(output);
        outputNode = output;
    }
    if (!input) {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, isGrp));
        args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty(kCreateNodeArgsPropVolatile, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "Source");
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
        input = _publicInterface->getApp()->createNode(args);
        assert(input);
        inputNode = input;
    }

    SetCreatingWriterRAIIFlag creatingNode__(this);
    QString qpattern = QString::fromUtf8( filename.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    KnobStringPtr pluginIDKnob = pluginIDStringKnob.lock();
    std::string writerPluginID = pluginIDKnob->getValue();

    if ( writerPluginID.empty() ) {
        KnobChoicePtr pluginChoiceKnob = pluginSelectorKnob.lock();
        int pluginChoice_i = pluginChoiceKnob->getValue();
        if (pluginChoice_i == 0) {
            //Use default
            writerPluginID = appPTR->getWriterPluginIDForFileType(ext);
        } else {
            std::vector<ChoiceOption> entries = pluginChoiceKnob->getEntries();
            if ( (pluginChoice_i >= 0) && ( pluginChoice_i < (int)entries.size() ) ) {
                writerPluginID = entries[pluginChoice_i].id;
            }
        }
    }


    // If the plug-in is the same, do not create a new decoder.
    {
        NodePtr writeNode = embeddedPlugin.lock();
        if (writeNode && writeNode->getPluginID() == writerPluginID) {
            boost::shared_ptr<KnobFile> fileKnob = outputFileKnob.lock();
            assert(fileKnob);
            if (fileKnob) {
                // Make sure instance changed action is called on the decoder and not caught in our knobChanged handler.
                writeNode->getEffectInstance()->onKnobValueChanged_public(fileKnob, eValueChangedReasonUserEdited, _publicInterface->getTimelineCurrentTime(), ViewSetSpec(0));
            }
            return;
        }
    }

    //Destroy any previous reader
    //This will store the serialization of the generic knobs
    destroyWriteNode();

    bool defaultFallback = false;

    //Find the appropriate reader
    if (writerPluginID.empty() && !serialization) {
        //Couldn't find any reader
        if ( !ext.empty() ) {
            QString message = tr("No plugin capable of encoding %1 was found.").arg( QString::fromUtf8( ext.c_str() ) );
            //Dialogs::errorDialog(tr("Read").toStdString(), message.toStdString(), false);
            if (throwErrors) {
                throw std::runtime_error( message.toStdString() );
            }
        }
        defaultFallback = true;
    } else {
        if ( writerPluginID.empty() ) {
            writerPluginID = WRITE_NODE_DEFAULT_WRITER;
        }
        CreateNodeArgsPtr args(CreateNodeArgs::create(writerPluginID, isGrp ));
        args->setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty(kCreateNodeArgsPropVolatile, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "internalEncoderNode");
        SERIALIZATION_NAMESPACE::NodeSerializationPtr s(new SERIALIZATION_NAMESPACE::NodeSerialization);
        if (serialization) {
            *s = *serialization;
            args->setProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr >(kCreateNodeArgsPropNodeSerialization, s);
        }
        args->setProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer, _publicInterface->getNode());
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);


        if (serialization) {
            args->setProperty<bool>(kCreateNodeArgsPropSilent, true);
            args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true); // also load deprecated plugins
        }

        NodePtr writeNode = _publicInterface->getApp()->createNode(args);
        embeddedPlugin = writeNode;
        if (pluginIDKnob) {
            pluginIDKnob->setValue(writerPluginID);
        }

        // Set the filename value
        if (writeNode) {
            KnobFilePtr fileKnob = boost::dynamic_pointer_cast<KnobFile>(writeNode->getKnobByName(kOfxImageEffectFileParamName));
            if (fileKnob) {
                fileKnob->setValue(filename);
            }
        }

        placeWriteNodeKnobsInPage();
        separatorKnob.lock()->setSecret(false);


        //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
        _publicInterface->getNode()->declarePythonKnobs();
    }


    if ( !embeddedPlugin.lock() ) {
        defaultFallback = true;
    }

    if (defaultFallback) {
        createDefaultWriteNode();
    }

    // Make the write node be a pass-through while we do not render
    NodePtr writeNode = embeddedPlugin.lock();

    bool readFromFile = readBackKnob.lock()->getValue();
    if (readFromFile) {
        createReadNodeAndConnectGraph(filename);
    } else {
        NodePtr input = inputNode.lock(), output = outputNode.lock();
        if (writeNode) {
            output->swapInput(writeNode, 0);
            writeNode->swapInput(input, 0);
        } else {
            output->swapInput(input, 0);
        }
    }

    // There should always be a single input node
    assert(_publicInterface->getMaxInputCount() == 1);

    _publicInterface->getNode()->findPluginFormatKnobs();

    //Clone the old values of the generic knobs
    cloneGenericKnobs();


    NodePtr thisNode = _publicInterface->getNode();
    //Refresh accepted bitdepths on the node
    thisNode->refreshAcceptedBitDepths();

    //Refresh accepted components
    thisNode->initializeInputs();

    //This will refresh the GUI with this Reader specific parameters
    _publicInterface->recreateKnobs(true);

    KnobIPtr knob = writeNode ? writeNode->getKnobByName(kOfxImageEffectFileParamName) : _publicInterface->getKnobByName(kOfxImageEffectFileParamName);
    if (knob) {
        outputFileKnob = toKnobFile(knob);
    }
} // WriteNodePrivate::createWriteNode

void
WriteNodePrivate::refreshPluginSelectorKnob()
{
    KnobFilePtr fileKnob = outputFileKnob.lock();

    assert(fileKnob);
    std::string filePattern = fileKnob->getValue();
    std::vector<ChoiceOption> entries;
    entries.push_back(ChoiceOption(kPluginSelectorParamEntryDefault, "", tr("Use the default plug-in chosen from the Preferences to write this file format").toStdString()));

    QString qpattern = QString::fromUtf8( filePattern.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::string pluginID;
    if ( !ext.empty() ) {
        pluginID = appPTR->getWriterPluginIDForFileType(ext);
        IOPluginSetForFormat writersForFormat;
        appPTR->getWritersForFormat(ext, &writersForFormat);

        // Reverse it so that we sort them by decreasing score order
        for (IOPluginSetForFormat::reverse_iterator it = writersForFormat.rbegin(); it != writersForFormat.rend(); ++it) {
            PluginPtr plugin = appPTR->getPluginBinary(QString::fromUtf8( it->pluginID.c_str() ), -1, -1, false);

            QString tooltip = tr("Use %1 version %2.%3 to write this file format").arg(QString::fromUtf8(plugin->getPluginLabel().c_str())).arg( plugin->getProperty<unsigned int>(kNatronPluginPropVersion, 0)).arg(plugin->getProperty<unsigned int>(kNatronPluginPropVersion, 1));
            entries.push_back( ChoiceOption(plugin->getPluginID(), "", tooltip.toStdString()));

        }
    }

    KnobChoicePtr pluginChoice = pluginSelectorKnob.lock();

    pluginChoice->populateChoices(entries);
    pluginChoice->blockValueChanges();
    pluginChoice->resetToDefaultValue();
    pluginChoice->unblockValueChanges();
    if (entries.size() <= 2) {
        pluginChoice->setSecret(true);
    } else {
        pluginChoice->setSecret(false);
    }

    KnobStringPtr pluginIDKnob = pluginIDStringKnob.lock();
    pluginIDKnob->blockValueChanges();
    pluginIDKnob->setValue(pluginID);
    pluginIDKnob->unblockValueChanges();
}

bool
WriteNode::isWriter() const
{
    return true;
}

// static
bool
WriteNode::isVideoWriter(const std::string& pluginID)
{
    return (pluginID == PLUGINID_OFX_WRITEFFMPEG);
}

bool
WriteNode::isVideoWriter() const
{
    NodePtr p = _imp->embeddedPlugin.lock();

    return p ? isVideoWriter( p->getPluginID() ) : false;
}

bool
WriteNode::isGenerator() const
{
    return false;
}

bool
WriteNode::isOutput() const
{
    return true;
}

bool
WriteNode::getCreateChannelSelectorKnob() const
{
    return false;
}

bool
WriteNode::isHostChannelSelectorSupported(bool* /*defaultR*/,
                                          bool* /*defaultG*/,
                                          bool* /*defaultB*/,
                                          bool* /*defaultA*/) const
{
    return false;
}

void
WriteNode::initializeKnobs()
{
    KnobPagePtr controlpage = AppManager::createKnob<KnobPage>( shared_from_this(), tr("Controls") );


    ///Find a  "lastFrame" parameter and add it after it
    KnobIntPtr frameIncrKnob = AppManager::createKnob<KnobInt>( shared_from_this(), tr(kNatronWriteParamFrameStepLabel) );

    frameIncrKnob->setName(kNatronWriteParamFrameStep);
    frameIncrKnob->setHintToolTip( tr(kNatronWriteParamFrameStepHint) );
    frameIncrKnob->setAnimationEnabled(false);
    frameIncrKnob->setRange(1, INT_MAX);
    frameIncrKnob->setDefaultValue(1);
    controlpage->addKnob(frameIncrKnob);
    /*if (mainPage) {
        std::vector< KnobIPtr > children = mainPage->getChildren();
        bool foundLastFrame = false;
        for (std::size_t i = 0; i < children.size(); ++i) {
            if (children[i]->getName() == "lastFrame") {
                mainPage->insertKnob(i + 1, frameIncrKnob);
                foundLastFrame = true;
                break;
            }
        }
        if (!foundLastFrame) {
            mainPage->addKnob(frameIncrKnob);
        }
       }*/
    _imp->frameIncrKnob = frameIncrKnob;
    _imp->writeNodeKnobs.push_back(frameIncrKnob);

    KnobBoolPtr readBack = AppManager::createKnob<KnobBool>( shared_from_this(), tr(kNatronWriteParamReadBackLabel) );
    readBack->setAnimationEnabled(false);
    readBack->setName(kNatronWriteParamReadBack);
    readBack->setHintToolTip( tr(kNatronWriteParamReadBackHint) );
    readBack->setEvaluateOnChange(false);
    readBack->setDefaultValue(false);
    controlpage->addKnob(readBack);
    _imp->readBackKnob = readBack;
    _imp->writeNodeKnobs.push_back(readBack);


    KnobChoicePtr pluginSelector = AppManager::createKnob<KnobChoice>( shared_from_this(), tr("Encoder") );
    pluginSelector->setAnimationEnabled(false);
    pluginSelector->setName(kNatronWriteNodeParamEncodingPluginChoice);
    pluginSelector->setHintToolTip( tr("Select the internal encoder plug-in used for this file format. By default this uses "
                                       "the plug-in selected for this file extension in the Preferences.") );
    pluginSelector->setEvaluateOnChange(false);
    _imp->pluginSelectorKnob = pluginSelector;
    controlpage->addKnob(pluginSelector);

    _imp->writeNodeKnobs.push_back(pluginSelector);

    KnobSeparatorPtr separator = AppManager::createKnob<KnobSeparator>( shared_from_this(), tr("Encoder Options") );
    separator->setName("encoderOptionsSeparator");
    separator->setHintToolTip( tr("Below can be found parameters that are specific to the Writer plug-in.") );
    controlpage->addKnob(separator);
    _imp->separatorKnob = separator;
    _imp->writeNodeKnobs.push_back(separator);

    KnobStringPtr pluginID = AppManager::createKnob<KnobString>( shared_from_this(), tr("PluginID") );
    pluginID->setAnimationEnabled(false);
    pluginID->setName(kNatronWriteNodeParamEncodingPluginID);
    pluginID->setSecret(true);
    controlpage->addKnob(pluginID);
    _imp->pluginIDStringKnob = pluginID;
    _imp->writeNodeKnobs.push_back(pluginID);
} // WriteNode::initializeKnobs

void
WriteNode::onEffectCreated(const CreateNodeArgs& args)
{

    RenderEnginePtr engine = getNode()->getRenderEngine();
    assert(engine);
    QObject::connect(engine.get(), SIGNAL(renderFinished(int)), this, SLOT(onSequenceRenderFinished()));

    if ( !_imp->renderButtonKnob.lock() ) {
        _imp->renderButtonKnob = toKnobButton( getKnobByName("startRender") );
        assert( _imp->renderButtonKnob.lock() );
    }


    //If we already loaded the Writer, do not do anything
    if ( _imp->embeddedPlugin.lock() ) {
        return;
    }
    bool throwErrors = false;
    KnobStringPtr pluginIdParam = _imp->pluginIDStringKnob.lock();
    std::string pattern;


    std::vector<std::string> defaultParamValues = args.getPropertyN<std::string>(kCreateNodeArgsPropNodeInitialParamValues);
    std::vector<std::string>::iterator foundFileName  = std::find(defaultParamValues.begin(), defaultParamValues.end(), std::string(kOfxImageEffectFileParamName));
    if (foundFileName != defaultParamValues.end()) {
        std::string propName(kCreateNodeArgsPropParamValue);
        propName += "_";
        propName += kOfxImageEffectFileParamName;
        pattern = args.getProperty<std::string>(propName);
    }


    _imp->createWriteNode( throwErrors, pattern, 0 );
    _imp->refreshPluginSelectorKnob();
}

void
WriteNode::onKnobsAboutToBeLoaded(const SERIALIZATION_NAMESPACE::NodeSerialization& serialization)
{
    _imp->renderButtonKnob = toKnobButton( getKnobByName("startRender") );
    assert( _imp->renderButtonKnob.lock() );

    NodePtr node = getNode();

    //Load the pluginID to create first.
    node->loadKnob( _imp->pluginIDStringKnob.lock(), serialization._knobsValues );

    std::string filename = getFileNameFromSerialization( serialization._knobsValues );
    //Create the Reader with the serialization

    _imp->createWriteNode(false, filename, &serialization);
    _imp->refreshPluginSelectorKnob();
}

bool
WriteNode::knobChanged(const KnobIPtr& k,
                       ValueChangedReasonEnum reason,
                       ViewSetSpec view,
                       TimeValue time)
{
    bool ret = true;
    NodePtr writer = _imp->embeddedPlugin.lock();

    if ( ( k == _imp->outputFileKnob.lock() ) && (reason != eValueChangedReasonTimeChanged) ) {
        if (_imp->creatingWriteNode) {
            if (writer) {
                writer->getEffectInstance()->knobChanged(k, reason, view, time);
            }

            return false;
        }

        try {
            _imp->refreshPluginSelectorKnob();
        } catch (const std::exception& e) {
            setPersistentMessage( eMessageTypeError, e.what() );
        }

        KnobFilePtr fileKnob = _imp->outputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getValue();

        try {
            _imp->createWriteNode( false, filename, 0 );
        } catch (const std::exception& e) {
            setPersistentMessage( eMessageTypeError, e.what() );
        }
      
    } else if ( k == _imp->pluginSelectorKnob.lock() ) {
        KnobStringPtr pluginIDKnob = _imp->pluginIDStringKnob.lock();
        ChoiceOption entry = _imp->pluginSelectorKnob.lock()->getActiveEntry();
        if ( entry.id == pluginIDKnob->getValue() ) {
            return false;
        }

        if (entry.id == "Default") {
            entry.id.clear();
        }

        pluginIDKnob->setValue(entry.id);

        KnobFilePtr fileKnob = _imp->outputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getValue();

        try {
            _imp->createWriteNode( false, filename, 0 );
        } catch (const std::exception& e) {
            setPersistentMessage( eMessageTypeError, e.what() );
        }
    } else if ( k == _imp->readBackKnob.lock() ) {
        clearPersistentMessage(false);
        bool readFile = _imp->readBackKnob.lock()->getValue();
        KnobButtonPtr button = _imp->renderButtonKnob.lock();
        button->setEnabled(!readFile);
        if (readFile) {
            KnobFilePtr fileKnob = _imp->outputFileKnob.lock();
            assert(fileKnob);
            std::string filename = fileKnob->getValue();
            _imp->createReadNodeAndConnectGraph(filename);
        } else {
            NodePtr input = _imp->inputNode.lock(), output = _imp->outputNode.lock();
            if (input && writer && output) {
                writer->swapInput(input, 0);
                output->swapInput(input, 0);
            }
        }
    } else if ( (k->getName() == kParamFirstFrame) ||
                ( k->getName() == kParamLastFrame) ||
                ( k->getName() == kParamFrameRange) ) {
        _imp->setReadNodeOriginalFrameRange();
        ret = false;
    } else {
        ret = false;
    }
    if (!ret && writer) {
        EffectInstancePtr effect = writer->getEffectInstance();
        if (effect) {
            ret |= effect->knobChanged(k, reason, view, time);
        }
    }

    return ret;
} // WriteNode::knobChanged

bool
WriteNode::isViewAware() const
{
    NodePtr writer = _imp->embeddedPlugin.lock();

    if (writer) {
        EffectInstancePtr effect = writer->getEffectInstance();
        if (effect) {
            return effect->isViewAware();
        }
    }
    return false;
}

ActionRetCodeEnum
WriteNode::getFrameRange(const TreeRenderNodeArgsPtr& render,
                         double *first,
                         double *last)
{
    NodePtr writer = _imp->embeddedPlugin.lock();

    if (writer) {
        EffectInstancePtr effect = writer->getEffectInstance();
        if (effect) {
            return effect->getFrameRange(render, first, last);
        }
    }
    return EffectInstance::getFrameRange(render, first, last);
    
}


void
WriteNode::onSequenceRenderStarted()
{
    NodePtr readNode = _imp->readBackNode.lock();
    NodePtr outputNode = _imp->outputNode.lock();
    if (outputNode->getInput(0) == readNode) {
        outputNode->swapInput(_imp->inputNode.lock(), 0);
    }

}

void
WriteNode::onSequenceRenderFinished()
{

    bool readFile = _imp->readBackKnob.lock()->getValue();

    NodePtr readNode = _imp->readBackNode.lock();
    NodePtr outputNode = _imp->outputNode.lock();
    if (readFile && readNode) {
        outputNode->swapInput(readNode, 0);
    }


}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_WriteNode.cpp"
