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


#include "WriteNode.h"

#include "Global/QtCompat.h"

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp> // iequals
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <ofxNatron.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/NodeSerialization.h"
#include "Engine/KnobSerialization.h" // createDefaultValueForParam
#include "Engine/Plugin.h"
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
    {kParamInputPremult, true},
    {kParamClipInfo, false},
    {kParamOutputSpaceLabel, false},
    {kParamClipToProject, true},
    {kNatronOfxParamProcessR, true},
    {kNatronOfxParamProcessG, true},
    {kNatronOfxParamProcessB, true},
    {kNatronOfxParamProcessA, true},


    {kOCIOParamConfigFile, true},
    {kOCIOParamInputSpace, false},
    {kOCIOParamOutputSpace, false},
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
WriteNode::isBundledWriter(const std::string& pluginID,
                           bool wasProjectCreatedWithLowerCaseIDs)
{
    if (wasProjectCreatedWithLowerCaseIDs) {
        // Natron 1.x has plugin ids stored in lowercase
        return boost::iequals(pluginID, PLUGINID_OFX_WRITEOIIO) ||
               boost::iequals(pluginID, PLUGINID_OFX_WRITEFFMPEG) ||
               boost::iequals(pluginID, PLUGINID_OFX_WRITEPFM);
    }

    return pluginID == PLUGINID_OFX_WRITEOIIO ||
           pluginID == PLUGINID_OFX_WRITEFFMPEG ||
           pluginID == PLUGINID_OFX_WRITEPFM;
}

bool
WriteNode::isBundledWriter(const std::string& pluginID)
{
    return isBundledWriter( pluginID, getApp()->wasProjectCreatedWithLowerCaseIDs() );
}

struct WriteNodePrivate
{
    WriteNode* _publicInterface;
    NodePtr embeddedPlugin;
    std::list<boost::shared_ptr<KnobSerialization> > genericKnobsSerialization;
    boost::weak_ptr<KnobOutputFile> outputFileKnob;

    //Thiese are knobs owned by the ReadNode and not the Reader
    boost::weak_ptr<KnobInt> frameIncrKnob;
    boost::weak_ptr<KnobChoice> pluginSelectorKnob;
    boost::weak_ptr<KnobString> pluginIDStringKnob;
    boost::weak_ptr<KnobSeparator> separatorKnob;
    boost::weak_ptr<KnobButton> renderButtonKnob;
    std::list<boost::weak_ptr<KnobI> > writeNodeKnobs;

    //MT only
    int creatingWriteNode;


    WriteNodePrivate(WriteNode* publicInterface)
        : _publicInterface(publicInterface)
        , embeddedPlugin()
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

    void createWriteNode(bool throwErrors, const std::string& filename, const boost::shared_ptr<NodeSerialization>& serialization,
                         const std::list<boost::shared_ptr<KnobSerialization> >& defaultParamValues);

    void destroyWriteNode();

    void cloneGenericKnobs();

    void refreshPluginSelectorKnob();

    void createDefaultWriteNode();

    bool checkEncoderCreated(double time, ViewIdx view);
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


WriteNode::WriteNode(NodePtr n)
    : OutputEffectInstance(n)
    , _imp( new WriteNodePrivate(this) )
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

WriteNode::~WriteNode()
{
}

NodePtr
WriteNode::getEmbeddedReader() const
{
    return _imp->embeddedPlugin;
}

void
WriteNode::setEmbeddedWriter(const NodePtr& node)
{
    _imp->embeddedPlugin = node;
}

void
WriteNodePrivate::placeWriteNodeKnobsInPage()
{
    KnobPtr pageKnob = _publicInterface->getKnobByName("Controls");
    KnobPage* isPage = dynamic_cast<KnobPage*>( pageKnob.get() );

    if (!isPage) {
        return;
    }
    for (std::list<boost::weak_ptr<KnobI> >::iterator it = writeNodeKnobs.begin(); it != writeNodeKnobs.end(); ++it) {
        KnobPtr knob = it->lock();
        knob->setParentKnob( KnobPtr() );
        isPage->removeKnob( knob.get() );
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
        for (std::list<boost::weak_ptr<KnobI> >::iterator it = writeNodeKnobs.begin(); it != writeNodeKnobs.end(); ++it) {
            KnobPtr knob = it->lock();
            isPage->insertKnob(index, knob);
            ++index;
        }
    }

    //Set the render button as the last knob
    boost::shared_ptr<KnobButton> renderB = renderButtonKnob.lock();
    if (renderB) {
        renderB->setParentKnob( KnobPtr() );
        isPage->removeKnob( renderB.get() );
        isPage->addKnob(renderB);
    }
}

void
WriteNodePrivate::cloneGenericKnobs()
{
    const KnobsVec& knobs = _publicInterface->getKnobs();

    for (std::list<boost::shared_ptr<KnobSerialization> >::iterator it = genericKnobsSerialization.begin(); it != genericKnobsSerialization.end(); ++it) {
        KnobPtr serializedKnob = (*it)->getKnob();
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ( (*it2)->getName() == serializedKnob->getName() ) {
                (*it2)->clone( serializedKnob.get() );
                break;
            }
        }
    }
}

void
WriteNodePrivate::destroyWriteNode()
{
    assert( QThread::currentThread() == qApp->thread() );

    KnobsVec knobs = _publicInterface->getKnobs();

    genericKnobsSerialization.clear();

    for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( !(*it)->isDeclaredByPlugin() ) {
            continue;
        }

        //If it is a knob of this WriteNode, ignore it
        bool isWriteNodeKnob = false;
        for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = writeNodeKnobs.begin(); it2 != writeNodeKnobs.end(); ++it2) {
            if (it2->lock() == *it) {
                isWriteNodeKnob = true;
                break;
            }
        }
        if (isWriteNodeKnob) {
            continue;
        }

        //Keep pages around they will be re-used
        KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
        if (isPage) {
            continue;
        }

        //This is a knob of the Writer plug-in

        //Serialize generic knobs and keep them around until we create a new Writer plug-in
        bool mustSerializeKnob;
        bool isGeneric = isGenericKnob( (*it)->getName(), &mustSerializeKnob );
        if (isGeneric && mustSerializeKnob) {
            boost::shared_ptr<KnobSerialization> s( new KnobSerialization(*it) );
            genericKnobsSerialization.push_back(s);
        }
        if (!isGeneric) {
            _publicInterface->deleteKnob(it->get(), false);
        }
    }


    //This will remove the GUI of non generic parameters
    _publicInterface->recreateKnobs(true);

    embeddedPlugin.reset();
} // WriteNodePrivate::destroyWriteNode

void
WriteNodePrivate::createDefaultWriteNode()
{
    CreateNodeArgs args( QString::fromUtf8(WRITE_NODE_DEFAULT_WRITER), eCreateNodeReasonInternal, boost::shared_ptr<NodeCollection>() );

    args.createGui = false;
    args.addToProject = false;
    args.ioContainer = _publicInterface->getNode();
    args.fixedName = QString::fromUtf8("defaultWriteNodeWriter");
    //args.paramValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, filePattern));
    embeddedPlugin = _publicInterface->getApp()->createNode(args);

    if (!embeddedPlugin) {
        QString error = QObject::tr("The IO.ofx.bundle OpenFX plug-in is required to use this node, make sure it is installed.");
        throw std::runtime_error( error.toStdString() );
    }


    //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
    _publicInterface->getNode()->declarePythonFields();

    //Destroy it to keep the default parameters
    destroyWriteNode();
    placeWriteNodeKnobsInPage();
    separatorKnob.lock()->setSecret(true);
}

bool
WriteNodePrivate::checkEncoderCreated(double time,
                                      ViewIdx view)
{
    boost::shared_ptr<KnobOutputFile> fileKnob = outputFileKnob.lock();

    assert(fileKnob);
    std::string pattern = fileKnob->generateFileNameAtTime( std::floor(time + 0.5), ViewSpec( view.value() ) ).toStdString();
    if ( pattern.empty() ) {
        _publicInterface->setPersistentMessage( eMessageTypeError, QObject::tr("Filename empty").toStdString() );

        return false;
    }
    if (!embeddedPlugin) {
        std::stringstream ss;
        ss << QObject::tr("Encoder was not created for ").toStdString() << pattern;
        ss << QObject::tr(" check that the file exists and its format is supported").toStdString();
        _publicInterface->setPersistentMessage( eMessageTypeError, ss.str() );

        return false;
    }

    return true;
}

void
WriteNodePrivate::createWriteNode(bool throwErrors,
                                  const std::string& filename,
                                  const boost::shared_ptr<NodeSerialization>& serialization,
                                  const std::list<boost::shared_ptr<KnobSerialization> >& defaultParamValues)
{
    if (creatingWriteNode) {
        return;
    }

    SetCreatingWriterRAIIFlag creatingNode__(this);
    std::string filePattern = filename;
    if ( filename.empty() ) {
        for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = defaultParamValues.begin(); it != defaultParamValues.end(); ++it) {
            if ( (*it)->getKnob()->getName() == kOfxImageEffectFileParamName ) {
                Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( (*it)->getKnob().get() );
                assert(isString);
                if (isString) {
                    filePattern = isString->getValue();
                }
                break;
            }
        }
    }
    QString qpattern = QString::fromUtf8( filePattern.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    boost::shared_ptr<KnobString> pluginIDKnob = pluginIDStringKnob.lock();
    std::string writerPluginID = pluginIDKnob->getValue();

    if ( writerPluginID.empty() ) {
        boost::shared_ptr<KnobChoice> pluginChoiceKnob = pluginSelectorKnob.lock();
        int pluginChoice_i = pluginChoiceKnob->getValue();
        if (pluginChoice_i == 0) {
            //Use default
            std::map<std::string, std::string> writersForFormat;
            appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
            std::map<std::string, std::string>::iterator foundWriterForFormat = writersForFormat.find(ext);
            if ( foundWriterForFormat != writersForFormat.end() ) {
                writerPluginID = foundWriterForFormat->second;
            }
        } else {
            std::vector<std::string> entries = pluginChoiceKnob->getEntries_mt_safe();
            if ( (pluginChoice_i >= 0) && ( pluginChoice_i < (int)entries.size() ) ) {
                writerPluginID = entries[pluginChoice_i];
            }
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
            QString message = QObject::tr("No plugin capable of encoding") + QLatin1Char(' ') + QString::fromUtf8( ext.c_str() ) + QLatin1Char(' ') + QObject::tr("was found") + QLatin1Char('.');
            //Dialogs::errorDialog(QObject::tr("Read").toStdString(), message.toStdString(), false);
            if (throwErrors) {
                throw std::runtime_error( message.toStdString() );
            }
        }
        defaultFallback = true;
    } else {
        CreateNodeArgs args( QString::fromUtf8( writerPluginID.c_str() ), serialization ? eCreateNodeReasonProjectLoad : eCreateNodeReasonInternal, boost::shared_ptr<NodeCollection>() );
        args.createGui = false;
        args.addToProject = false;
        args.fixedName = QString::fromUtf8("internalEncoderNode");
        args.serialization = serialization;
        args.ioContainer = _publicInterface->getNode();

        //Set a pre-value for the inputfile knob only if it did not exist
        if (!filePattern.empty() /* && !inputFileKnob.lock()*/) {
            args.paramValues.push_back( createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, filePattern) );
        }
        embeddedPlugin = _publicInterface->getApp()->createNode(args);
        if (pluginIDKnob) {
            pluginIDKnob->setValue(writerPluginID);
        }
        placeWriteNodeKnobsInPage();
        separatorKnob.lock()->setSecret(false);


        //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
        _publicInterface->getNode()->declarePythonFields();
    }
    if (!embeddedPlugin) {
        defaultFallback = true;
    }

    if (defaultFallback) {
        createDefaultWriteNode();
    }

    //Clone the old values of the generic knobs
    cloneGenericKnobs();


    NodePtr thisNode = _publicInterface->getNode();
    //Refresh accepted bitdepths on the node
    thisNode->refreshAcceptedBitDepths();

    //Refresh accepted components
    thisNode->initializeInputs();

    //This will refresh the GUI with this Reader specific parameters
    _publicInterface->recreateKnobs(true);

    KnobPtr knob = _publicInterface->getKnobByName(kOfxImageEffectFileParamName);
    if (knob) {
        outputFileKnob = boost::dynamic_pointer_cast<KnobOutputFile>(knob);
    }
} // WriteNodePrivate::createWriteNode

void
WriteNodePrivate::refreshPluginSelectorKnob()
{
    boost::shared_ptr<KnobOutputFile> fileKnob = outputFileKnob.lock();

    assert(fileKnob);
    std::string filePattern = fileKnob->getValue();
    std::vector<std::string> entries, help;
    entries.push_back(kPluginSelectorParamEntryDefault);
    help.push_back("Use the default plug-in chosen from the Preferences to write this file format");

    QString qpattern = QString::fromUtf8( filePattern.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::vector<std::string> writersForFormat;
    std::string pluginID;
    if ( !ext.empty() ) {
        appPTR->getCurrentSettings()->getWritersForFormat(ext, &writersForFormat);

        pluginID = appPTR->getCurrentSettings()->getWriterPluginIDForFileType(ext);

        for (std::size_t i = 0; i < writersForFormat.size(); ++i) {
            Plugin* plugin = appPTR->getPluginBinary(QString::fromUtf8( writersForFormat[i].c_str() ), -1, -1, false);
            entries.push_back( plugin->getPluginID().toStdString() );
            std::stringstream ss;
            ss << "Use " << plugin->getPluginLabel().toStdString() << " version ";
            ss << plugin->getMajorVersion() << "." << plugin->getMinorVersion();
            ss << " to write this file format";
            help.push_back( ss.str() );
        }
    }

    boost::shared_ptr<KnobChoice> pluginChoice = pluginSelectorKnob.lock();

    pluginChoice->populateChoices(entries, help);
    pluginChoice->blockValueChanges();
    pluginChoice->resetToDefaultValue(0);
    pluginChoice->unblockValueChanges();
    if (entries.size() <= 2) {
        pluginChoice->setSecret(true);
    } else {
        pluginChoice->setSecret(false);
    }

    boost::shared_ptr<KnobString> pluginIDKnob = pluginIDStringKnob.lock();
    pluginIDKnob->blockValueChanges();
    pluginIDKnob->setValue(pluginID);
    pluginIDKnob->unblockValueChanges();
}

bool
WriteNode::isWriter() const
{
    return true;
}

bool
WriteNode::isVideoWriter() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getPluginID() == PLUGINID_OFX_WRITEFFMPEG : false;
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
WriteNode::isMultiPlanar() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->isMultiPlanar() : OutputEffectInstance::isMultiPlanar();
}

bool
WriteNode::isViewAware() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->isViewAware() : OutputEffectInstance::isViewAware();
}

bool
WriteNode::supportsTiles() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->supportsTiles() : OutputEffectInstance::supportsTiles();
}

bool
WriteNode::supportsMultiResolution() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->supportsMultiResolution() : OutputEffectInstance::supportsMultiResolution();
}

bool
WriteNode::supportsMultipleClipsBitDepth() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->supportsMultipleClipsBitDepth() : OutputEffectInstance::supportsMultipleClipsBitDepth();
}

RenderSafetyEnum
WriteNode::renderThreadSafety() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->renderThreadSafety() : eRenderSafetyFullySafe;
}

bool
WriteNode::getCanTransform() const
{
    return false;
}

SequentialPreferenceEnum
WriteNode::getSequentialPreference() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->getSequentialPreference() : OutputEffectInstance::getSequentialPreference();
}

EffectInstance::ViewInvarianceLevel
WriteNode::isViewInvariant() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->isViewInvariant() : OutputEffectInstance::isViewInvariant();
}

EffectInstance::PassThroughEnum
WriteNode::isPassThroughForNonRenderedPlanes() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->isPassThroughForNonRenderedPlanes() : OutputEffectInstance::isPassThroughForNonRenderedPlanes();
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

int
WriteNode::getMajorVersion() const
{ return 1; }

int
WriteNode::getMinorVersion() const
{ return 0; }

std::string
WriteNode::getPluginID() const
{ return PLUGINID_NATRON_WRITE; }

std::string
WriteNode::getPluginLabel() const
{ return "Write"; }

std::string
WriteNode::getPluginDescription() const
{
    return "Node used to write images or videos on disk. The image/video is identified by its filename and "
           "its extension. Given the extension, the Writer selected from the Preferences to encode that specific format will be used. ";
}

void
WriteNode::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_IMAGE);
}

int
WriteNode::getMaxInputCount() const
{
    return 1;
}

std::string
WriteNode::getInputLabel (int /*inputNb*/) const
{
    return "Source";
}

bool
WriteNode::isInputOptional(int /*inputNb*/) const
{
    return true;
}

bool
WriteNode::isInputMask(int /*inputNb*/) const
{
    return false;
}

void
WriteNode::addAcceptedComponents(int inputNb,
                                 std::list<ImageComponents>* comps)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->addAcceptedComponents(inputNb, comps);
    } else {
        comps->push_back( ImageComponents::getRGBAComponents() );
    }
}

void
WriteNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->addSupportedBitDepth(depths);
    } else {
        depths->push_back(eImageBitDepthFloat);
    }
}

void
WriteNode::onInputChanged(int inputNo)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->onInputChanged(inputNo);
    }
}

void
WriteNode::purgeCaches()
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->purgeCaches();
    }
}

StatusEnum
WriteNode::getPreferredMetaDatas(NodeMetadata& metadata)
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->getPreferredMetaDatas(metadata) : OutputEffectInstance::getPreferredMetaDatas(metadata);
}

void
WriteNode::onMetaDatasRefreshed(const NodeMetadata& metadata)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->onMetaDatasRefreshed(metadata);
    }
}

void
WriteNode::initializeKnobs()
{
    boost::shared_ptr<KnobPage> controlpage = AppManager::createKnob<KnobPage>(this, "Controls");


    ///Find a  "lastFrame" parameter and add it after it
    boost::shared_ptr<KnobInt> frameIncrKnob = AppManager::createKnob<KnobInt>(this, kNatronWriteParamFrameStepLabel, 1, false);

    frameIncrKnob->setName(kNatronWriteParamFrameStep);
    frameIncrKnob->setHintToolTip(kNatronWriteParamFrameStepHint);
    frameIncrKnob->setAnimationEnabled(false);
    frameIncrKnob->setMinimum(1);
    frameIncrKnob->setDefaultValue(1);
    /*if (mainPage) {
        std::vector< KnobPtr > children = mainPage->getChildren();
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


    boost::shared_ptr<KnobChoice> pluginSelector = AppManager::createKnob<KnobChoice>(this, "Encoder");
    pluginSelector->setAnimationEnabled(false);
    pluginSelector->setName(kNatronWriteNodeParamEncodingPluginChoice);
    pluginSelector->setHintToolTip("Select the internal encoder plug-in used for this file format. By default this uses "
                                   "the plug-in selected for this file extension in the Preferences of Natron");
    pluginSelector->setEvaluateOnChange(false);
    _imp->pluginSelectorKnob = pluginSelector;
    controlpage->addKnob(pluginSelector);

    _imp->writeNodeKnobs.push_back(pluginSelector);

    boost::shared_ptr<KnobSeparator> separator = AppManager::createKnob<KnobSeparator>(this, "Encoder Options");
    separator->setName("encoderOptionsSeparator");
    separator->setHintToolTip("Below can be found parameters that are specific to the Writer plug-in.");
    controlpage->addKnob(separator);
    _imp->separatorKnob = separator;
    _imp->writeNodeKnobs.push_back(separator);

    boost::shared_ptr<KnobString> pluginID = AppManager::createKnob<KnobString>(this, "PluginID");
    pluginID->setAnimationEnabled(false);
    pluginID->setName(kNatronWriteNodeParamEncodingPluginID);
    pluginID->setSecretByDefault(true);
    controlpage->addKnob(pluginID);
    _imp->pluginIDStringKnob = pluginID;
    _imp->writeNodeKnobs.push_back(pluginID);
}

void
WriteNode::onEffectCreated(bool mayCreateFileDialog,
                           const std::list<boost::shared_ptr<KnobSerialization> >& defaultParamValues)
{
    if ( !_imp->renderButtonKnob.lock() ) {
        _imp->renderButtonKnob = boost::dynamic_pointer_cast<KnobButton>( getKnobByName("startRender") );
        assert( _imp->renderButtonKnob.lock() );
    }


    //If we already loaded the Writer, do not do anything
    if (_imp->embeddedPlugin) {
        return;
    }
    bool throwErrors = false;
    boost::shared_ptr<KnobString> pluginIdParam = _imp->pluginIDStringKnob.lock();
    std::string pattern;

    if (mayCreateFileDialog) {
        bool useDialogForWriters = appPTR->getCurrentSettings()->isFileDialogEnabledForNewWriters();
        if (useDialogForWriters) {
            pattern = getApp()->saveImageFileDialog();
        }

        //The user selected a file, if it fails to read do not create the node
        throwErrors = true;
    }
    _imp->createWriteNode(throwErrors, pattern, boost::shared_ptr<NodeSerialization>(), defaultParamValues);
    _imp->refreshPluginSelectorKnob();
}

void
WriteNode::onKnobsAboutToBeLoaded(const boost::shared_ptr<NodeSerialization>& serialization)
{
    _imp->renderButtonKnob = boost::dynamic_pointer_cast<KnobButton>( getKnobByName("startRender") );
    assert( _imp->renderButtonKnob.lock() );

    assert(serialization);
    NodePtr node = getNode();

    //Load the pluginID to create first.
    node->loadKnob( _imp->pluginIDStringKnob.lock(), serialization->getKnobsValues() );

    //Create the Reader with the serialization
    _imp->createWriteNode( false, std::string(), serialization, std::list<boost::shared_ptr<KnobSerialization> >() );
}

void
WriteNode::knobChanged(KnobI* k,
                       ValueChangedReasonEnum reason,
                       ViewSpec view,
                       double time,
                       bool originatedFromMainThread)
{
    if ( ( k == _imp->outputFileKnob.lock().get() ) && (reason != eValueChangedReasonTimeChanged) ) {
        if (_imp->creatingWriteNode) {
            if (_imp->embeddedPlugin) {
                _imp->embeddedPlugin->getEffectInstance()->knobChanged(k, reason, view, time, originatedFromMainThread);
            }

            return;
        }
        _imp->refreshPluginSelectorKnob();
        boost::shared_ptr<KnobOutputFile> fileKnob = _imp->outputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getValue();

        try {
            _imp->createWriteNode( false, filename, boost::shared_ptr<NodeSerialization>(), std::list<boost::shared_ptr<KnobSerialization> >() );
        } catch (const std::exception& e) {
            setPersistentMessage( eMessageTypeError, e.what() );
        }

        return;
    } else if ( k == _imp->pluginSelectorKnob.lock().get() ) {
        boost::shared_ptr<KnobString> pluginIDKnob = _imp->pluginIDStringKnob.lock();
        std::string entry = _imp->pluginSelectorKnob.lock()->getActiveEntryText_mt_safe();
        if ( entry == pluginIDKnob->getValue() ) {
            return;
        }

        pluginIDKnob->setValue(entry);

        boost::shared_ptr<KnobOutputFile> fileKnob = _imp->outputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getValue();

        try {
            _imp->createWriteNode( false, filename, boost::shared_ptr<NodeSerialization>(), std::list<boost::shared_ptr<KnobSerialization> >() );
        } catch (const std::exception& e) {
            setPersistentMessage( eMessageTypeError, e.what() );
        }

        return;
    }
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->knobChanged(k, reason, view, time, originatedFromMainThread);
    }
}

StatusEnum
WriteNode::getRegionOfDefinition(U64 hash,
                                 double time,
                                 const RenderScale & scale,
                                 ViewIdx view,
                                 RectD* rod)
{
    if ( !_imp->checkEncoderCreated(time, view) ) {
        return eStatusFailed;
    }
    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->getRegionOfDefinition(hash, time, scale, view, rod);
    } else {
        return eStatusFailed;
    }
}

void
WriteNode::getFrameRange(double *first,
                         double *last)
{
    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->getFrameRange(first, last);
    } else {
        *first = *last = 1;
    }
}

void
WriteNode::getComponentsNeededAndProduced(double time,
                                          ViewIdx view,
                                          EffectInstance::ComponentsNeededMap* comps,
                                          SequenceTime* passThroughTime,
                                          int* passThroughView,
                                          NodePtr* passThroughInput)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->getComponentsNeededAndProduced(time, view, comps, passThroughTime, passThroughView, passThroughInput);
    }
}

StatusEnum
WriteNode::beginSequenceRender(double first,
                               double last,
                               double step,
                               bool interactive,
                               const RenderScale & scale,
                               bool isSequentialRender,
                               bool isRenderResponseToUserInteraction,
                               bool draftMode,
                               ViewIdx view)
{
    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->beginSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view);
    } else {
        return eStatusFailed;
    }
}

StatusEnum
WriteNode::endSequenceRender(double first,
                             double last,
                             double step,
                             bool interactive,
                             const RenderScale & scale,
                             bool isSequentialRender,
                             bool isRenderResponseToUserInteraction,
                             bool draftMode,
                             ViewIdx view)
{
    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view);
    } else {
        return eStatusFailed;
    }
}

StatusEnum
WriteNode::render(const RenderActionArgs& args)
{
    if ( !_imp->checkEncoderCreated(args.time, args.view) ) {
        return eStatusFailed;
    }

    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->render(args);
    } else {
        return eStatusFailed;
    }
}

void
WriteNode::getRegionsOfInterest(double time,
                                const RenderScale & scale,
                                const RectD & outputRoD, //!< full RoD in canonical coordinates
                                const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                ViewIdx view,
                                RoIMap* ret)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->getRegionsOfInterest(time, scale, outputRoD, renderWindow, view, ret);
    }
}

FramesNeededMap
WriteNode::getFramesNeeded(double time,
                           ViewIdx view)
{
    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->getFramesNeeded(time, view);
    } else {
        return FramesNeededMap();
    }
}

NATRON_NAMESPACE_EXIT;
