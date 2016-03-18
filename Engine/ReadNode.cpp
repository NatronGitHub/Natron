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


#include "ReadNode.h"

#include "Global/QtCompat.h"

#include <QCoreApplication>
#include <QProcess>

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
#define READ_NODE_DEFAULT_READER PLUGINID_OFX_READOIIO
#define kPluginSelectorParamEntryDefault "Default"

NATRON_NAMESPACE_ENTER;

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
#define kParamOriginalFrameRange "originalFrameRange"
#define kParamFirstFrame "firstFrame"
#define kParamLastFrame "lastFrame"
#define kParamBefore "before"
#define kParamAfter "after"
#define kParamTimeDomainUserEdited "timeDomainUserEdited"
#define kParamFilePremult "filePremult"
#define kParamOutputComponents "outputComponents"
#define kParamInputSpaceLabel "File Colorspace"
#define kParamFrameRate "frameRate"
#define kParamCustomFps "customFps"

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
 These are names of knobs that are defined in GenericReader and that should stay on the interface
 no matter what the internal Reader is.
 */
struct GenericKnob
{
    const char* scriptName;
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
    {kParamOriginalFrameRange,false},
    {kParamFirstFrame, false},
    {kParamLastFrame, false},
    {kParamBefore, true},
    {kParamAfter, true},
    {kParamTimeDomainUserEdited, false},
    {kParamFilePremult, false},
    {kParamOutputComponents, false},
    {kParamInputSpaceLabel, false},
    {kParamFrameRate, false},
    {kParamCustomFps, false},
    
    
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


static bool isGenericKnob(const std::string& knobName, bool *mustSerialize)
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
    return
    pluginID == PLUGINID_OFX_READOIIO ||
    pluginID == PLUGINID_OFX_READFFMPEG ||
    pluginID == PLUGINID_OFX_READPFM ||
    pluginID == PLUGINID_OFX_READPSD ||
    pluginID == PLUGINID_OFX_READKRITA ||
    pluginID == PLUGINID_OFX_READSVG ||
    pluginID == PLUGINID_OFX_READMISC ||
    pluginID == PLUGINID_OFX_READORA;
}


struct ReadNodePrivate
{
    
    ReadNode* _publicInterface;
    
    NodePtr embeddedPlugin;
    
    std::list<boost::shared_ptr<KnobSerialization> > genericKnobsSerialization;
    
    boost::weak_ptr<KnobFile> inputFileKnob;
    
    //Thiese are knobs owned by the ReadNode and not the Reader
    boost::weak_ptr<KnobChoice> pluginSelectorKnob;
    boost::weak_ptr<KnobString> pluginIDStringKnob;
    boost::weak_ptr<KnobSeparator> separatorKnob;
    boost::weak_ptr<KnobButton> fileInfosKnob;
    
    std::list<boost::weak_ptr<KnobI> > readNodeKnobs;
    
    //MT only
    int creatingReadNode;
    
    
    ReadNodePrivate(ReadNode* publicInterface)
    : _publicInterface(publicInterface)
    , embeddedPlugin()
    , genericKnobsSerialization()
    , inputFileKnob()
    , pluginSelectorKnob()
    , pluginIDStringKnob()
    , separatorKnob()
    , fileInfosKnob()
    , readNodeKnobs()
    , creatingReadNode(0)
    {
        
    }
    
    void placeReadNodeKnobsInPage();
    
    void createReadNode(bool throwErrors,
                        const std::string& filename,
                        const boost::shared_ptr<NodeSerialization>& serialization,
                        const std::list<boost::shared_ptr<KnobSerialization> >& defaultParamValues );
    
    void destroyReadNode();
    
    void cloneGenericKnobs();
    
    void refreshPluginSelectorKnob();
    
    void createDefaultReadNode();
    
    bool checkDecoderCreated(double time, ViewIdx view);
    
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


ReadNode::ReadNode(NodePtr n)
: EffectInstance(n)
, _imp(new ReadNodePrivate(this))
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

ReadNode::~ReadNode()
{
    
}

NodePtr
ReadNode::getEmbeddedReader() const
{
    return _imp->embeddedPlugin;
}

void
ReadNode::setEmbeddedReader(const NodePtr& node)
{
    _imp->embeddedPlugin = node;
}

void
ReadNodePrivate::placeReadNodeKnobsInPage()
{
    KnobPtr pageKnob = _publicInterface->getKnobByName("Controls");
    KnobPage* isPage = dynamic_cast<KnobPage*>(pageKnob.get());
    if (!isPage) {
        return;
    }
    for (std::list<boost::weak_ptr<KnobI> >::iterator it = readNodeKnobs.begin(); it!=readNodeKnobs.end(); ++it) {
        KnobPtr knob = it->lock();
        knob->setParentKnob(KnobPtr());
        isPage->removeKnob(knob.get());
    }

    KnobsVec children = isPage->getChildren();
    int index = -1;
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i]->getName() == kParamCustomFps) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        ++index;
        for (std::list<boost::weak_ptr<KnobI> >::iterator it = readNodeKnobs.begin(); it!=readNodeKnobs.end(); ++it) {
            KnobPtr knob = it->lock();
            isPage->insertKnob(index, knob);
            ++index;
        }
    }
    
}

void
ReadNodePrivate::cloneGenericKnobs()
{
    const KnobsVec& knobs = _publicInterface->getKnobs();
    for (std::list<boost::shared_ptr<KnobSerialization> >::iterator it = genericKnobsSerialization.begin(); it!=genericKnobsSerialization.end(); ++it) {
        KnobPtr serializedKnob = (*it)->getKnob();
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ((*it2)->getName() == serializedKnob->getName()) {
                (*it2)->clone(serializedKnob.get());
                break;
            }
        }
    }
}


void
ReadNodePrivate::destroyReadNode()
{
    assert(QThread::currentThread() == qApp->thread());
    
    KnobsVec knobs = _publicInterface->getKnobs();

    genericKnobsSerialization.clear();
    
    for (KnobsVec::iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        
        if (!(*it)->isDeclaredByPlugin()) {
            continue;
        }
        
        //If it is a knob of this ReadNode, ignore it
        bool isReadNodeKnob = false;
        for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = readNodeKnobs.begin(); it2!=readNodeKnobs.end(); ++it2) {
            if (it2->lock() == *it) {
                isReadNodeKnob = true;
                break;
            }
        }
        if (isReadNodeKnob) {
            continue;
        }
        
        //Keep pages around they will be re-used
        KnobPage* isPage = dynamic_cast<KnobPage*>(it->get());
        if (isPage) {
            continue;
        }
        
        //This is a knob of the Reader plug-in
        
        //Serialize generic knobs and keep them around until we create a new Reader plug-in
        bool mustSerializeKnob;
        bool isGeneric = isGenericKnob((*it)->getName(),&mustSerializeKnob);
        if (isGeneric && mustSerializeKnob) {
            boost::shared_ptr<KnobSerialization> s(new KnobSerialization(*it));
            genericKnobsSerialization.push_back(s);
        }
        if (!isGeneric) {
            _publicInterface->deleteKnob(it->get(), false);
        }
        
    }
    
    
    
    //This will remove the GUI of non generic parameters
    _publicInterface->recreateKnobs(true);
    
    embeddedPlugin.reset();
}


void
ReadNodePrivate::createDefaultReadNode()
{
    CreateNodeArgs args(QString::fromUtf8(READ_NODE_DEFAULT_READER), eCreateNodeReasonInternal, boost::shared_ptr<NodeCollection>());
    args.createGui = false;
    args.addToProject = false;
    args.ioContainer = _publicInterface->getNode();
    args.fixedName = QString::fromUtf8("defaultReadNodeReader");
    //args.paramValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, filePattern));
    embeddedPlugin = _publicInterface->getApp()->createNode(args);
    
    if (!embeddedPlugin) {
        
        QString error = QObject::tr("The IO.ofx.bundle OpenFX plug-in is required to use this node, make sure it is installed.");
        throw std::runtime_error(error.toStdString());
    }
    
    
    //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
    _publicInterface->getNode()->declarePythonFields();

    //Destroy it to keep the default parameters
    destroyReadNode();
    
    separatorKnob.lock()->setSecret(true);
}

bool
ReadNodePrivate::checkDecoderCreated(double time, ViewIdx view)
{
    boost::shared_ptr<KnobFile> fileKnob = inputFileKnob.lock();
    assert(fileKnob);
    std::string pattern = fileKnob->getFileName(std::floor(time + 0.5), view);
    if (pattern.empty()) {
        _publicInterface->setPersistentMessage(eMessageTypeError, QObject::tr("Filename empty").toStdString());
        return false;
    }
    if (!embeddedPlugin) {
        std::stringstream ss;
        ss << QObject::tr("Decoder was not created for ").toStdString() << pattern;
        ss << QObject::tr(" check that the file exists and its format is supported").toStdString();
        _publicInterface->setPersistentMessage(eMessageTypeError, ss.str());
        return false;
    }
    return true;
}


void
ReadNodePrivate::createReadNode(bool throwErrors,
                                const std::string& filename,
                                const boost::shared_ptr<NodeSerialization>& serialization,
                                const std::list<boost::shared_ptr<KnobSerialization> >& defaultParamValues)
{
    if (creatingReadNode) {
        return;
    }
   
    SetCreatingReaderRAIIFlag creatingNode__(this);
    
    std::string filePattern = filename;
    if (filename.empty()) {
        for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = defaultParamValues.begin(); it!=defaultParamValues.end(); ++it) {
            if ((*it)->getKnob()->getName() == kOfxImageEffectFileParamName) {
                Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>((*it)->getKnob().get());
                assert(isString);
                if (isString) {
                    filePattern = isString->getValue();
                }
                break;
            }
        }
    }
    QString qpattern = QString::fromUtf8(filePattern.c_str());
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::string readerPluginID;

    boost::shared_ptr<KnobString> pluginIDKnob = pluginIDStringKnob.lock();
    readerPluginID = pluginIDKnob->getValue();
    
    
    if (readerPluginID.empty()) {
        boost::shared_ptr<KnobChoice> pluginChoiceKnob = pluginSelectorKnob.lock();
        int pluginChoice_i = pluginChoiceKnob->getValue();
        if (pluginChoice_i == 0) {
            //Use default
            std::map<std::string, std::string> readersForFormat;
            appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
            std::map<std::string, std::string>::iterator foundReaderForFormat = readersForFormat.find(ext);
            if (foundReaderForFormat != readersForFormat.end()) {
                readerPluginID = foundReaderForFormat->second;
            }
        } else {
            std::vector<std::string> entries = pluginChoiceKnob->getEntries_mt_safe();
            if (pluginChoice_i >= 0 && pluginChoice_i < (int)entries.size()) {
                readerPluginID = entries[pluginChoice_i];
            }
        }
    }
    
    //Destroy any previous reader
    //This will store the serialization of the generic knobs
    destroyReadNode();
    
    bool defaultFallback = false;
    
    //Find the appropriate reader
    if (readerPluginID.empty() && !serialization) {
        //Couldn't find any reader
        if (!ext.empty()) {
            QString message = QObject::tr("No plugin capable of decoding") + QLatin1Char(' ') + QString::fromUtf8(ext.c_str()) + QLatin1Char(' ') + QObject::tr("was found") + QLatin1Char('.');
            //Dialogs::errorDialog(QObject::tr("Read").toStdString(), message.toStdString(), false);
            if (throwErrors) {
                throw std::runtime_error(message.toStdString());
            }
        }
        defaultFallback = true;
       
    } else {
        CreateNodeArgs args(QString::fromUtf8(readerPluginID.c_str()), serialization ? eCreateNodeReasonProjectLoad : eCreateNodeReasonInternal, boost::shared_ptr<NodeCollection>());
        args.createGui = false;
        args.addToProject = false;
        args.serialization = serialization;
        args.fixedName = QString::fromUtf8("internalDecoderNode");
        args.ioContainer = _publicInterface->getNode();
        
        //Set a pre-value for the inputfile knob only if it did not exist
        if (!filePattern.empty()/* && !inputFileKnob.lock()*/) {
            args.paramValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, filePattern));
        }
        embeddedPlugin = _publicInterface->getApp()->createNode(args);
        if (pluginIDKnob) {
            pluginIDKnob->setValue(readerPluginID);
        }
        placeReadNodeKnobsInPage();
        separatorKnob.lock()->setSecret(false);
        
        
        //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
        _publicInterface->getNode()->declarePythonFields();
    }
    if (!embeddedPlugin) {
        defaultFallback = true;
    }
    
    if (defaultFallback) {
        createDefaultReadNode();
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
        inputFileKnob = boost::dynamic_pointer_cast<KnobFile>(knob);
    }
}

void
ReadNodePrivate::refreshPluginSelectorKnob()
{
    boost::shared_ptr<KnobFile> fileKnob = inputFileKnob.lock();
    assert(fileKnob);
    std::string filePattern = fileKnob->getValue();
    
    std::vector<std::string> entries,help;
    entries.push_back(kPluginSelectorParamEntryDefault);
    help.push_back("Use the default plug-in chosen from the Preferences to read this file format");
    
    QString qpattern = QString::fromUtf8(filePattern.c_str());
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::vector<std::string> readersForFormat;

    std::string pluginID;
    if (!ext.empty()) {
        appPTR->getCurrentSettings()->getReadersForFormat(ext,&readersForFormat);
        pluginID = appPTR->getCurrentSettings()->getReaderPluginIDForFileType(ext);
        for (std::size_t i = 0; i < readersForFormat.size(); ++i) {
            Plugin* plugin = appPTR->getPluginBinary(QString::fromUtf8(readersForFormat[i].c_str()), -1, -1, false);
            entries.push_back(plugin->getPluginID().toStdString());
            std::stringstream ss;
            ss << "Use " << plugin->getPluginLabel().toStdString() << " version ";
            ss << plugin->getMajorVersion() << "." << plugin->getMinorVersion();
            ss << " to read this file format";
            help.push_back(ss.str());
        }
    }
    
    boost::shared_ptr<KnobChoice> pluginChoice = pluginSelectorKnob.lock();
    
    pluginChoice->populateChoices(entries,help);
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
    
    
    
    boost::shared_ptr<KnobButton> fileInfos = fileInfosKnob.lock();
    KnobPtr hasMetaDatasKnob = _publicInterface->getKnobByName("showMetadata");
    
    bool hasFfprobe = false;
    if (!hasMetaDatasKnob) {
        QString ffprobePath = getFFProbeBinaryPath();
        hasFfprobe = QFile::exists(ffprobePath);
    } else {
        hasMetaDatasKnob->setSecret(true);
    }
    
    
    if (hasMetaDatasKnob || (pluginID == PLUGINID_OFX_READFFMPEG && hasFfprobe)) {
        fileInfos->setSecret(false);
    } else {
        fileInfos->setSecret(true);
    }
}

bool
ReadNode::isReader() const
{
    return true;
}

bool
ReadNode::isGenerator() const
{
    return true;
}

bool
ReadNode::isOutput() const
{
    return false;
}



bool
ReadNode::isMultiPlanar() const
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->isMultiPlanar() : EffectInstance::isMultiPlanar();
}

bool
ReadNode::isViewAware() const   
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->isViewAware() : EffectInstance::isViewAware();
}

bool
ReadNode::supportsTiles() const  {
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->supportsTiles() : EffectInstance::supportsTiles();
}

bool
ReadNode::supportsMultiResolution() const  {
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->supportsMultiResolution() : EffectInstance::supportsMultiResolution();
}

bool
ReadNode::supportsMultipleClipsBitDepth() const  {
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->supportsMultipleClipsBitDepth() : EffectInstance::supportsMultipleClipsBitDepth();
}


RenderSafetyEnum
ReadNode::renderThreadSafety() const  {
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->renderThreadSafety() : eRenderSafetyFullySafe;
}

bool
ReadNode::getCanTransform() const  {
    return false;
}

SequentialPreferenceEnum
ReadNode::getSequentialPreference() const  {
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->getSequentialPreference() : EffectInstance::getSequentialPreference();
}

EffectInstance::ViewInvarianceLevel
ReadNode::isViewInvariant() const  {
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->isViewInvariant() : EffectInstance::isViewInvariant();
}

EffectInstance::PassThroughEnum
ReadNode::isPassThroughForNonRenderedPlanes() const  {
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->isPassThroughForNonRenderedPlanes() : EffectInstance::isPassThroughForNonRenderedPlanes();
}


bool
ReadNode::getCreateChannelSelectorKnob() const
{
    return false;
}

bool
ReadNode::isHostChannelSelectorSupported(bool* /*defaultR*/,bool* /*defaultG*/, bool* /*defaultB*/, bool* /*defaultA*/) const {
    return false;
}


int
ReadNode::getMajorVersion() const
{ return 1; }

int
ReadNode::getMinorVersion() const
{ return 0; }

std::string
ReadNode::getPluginID() const
{ return PLUGINID_NATRON_READ; }

std::string
ReadNode::getPluginLabel() const
{ return "Read"; }

std::string
ReadNode::getPluginDescription() const
{
    return "Node used to read images or videos from disk. The image/video is identified by its filename and "
    "its extension. Given the extension, the Reader selected from the Preferences to decode that specific format will be used. ";
}

void
ReadNode::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_IMAGE);
}

int
ReadNode::getMaxInputCount() const
{
    return 1;
}

std::string
ReadNode::getInputLabel (int /*inputNb*/) const
{
    return NATRON_READER_INPUT_NAME;
}

bool
ReadNode::isInputOptional(int /*inputNb*/) const
{
    return true;
}
bool
ReadNode::isInputMask(int /*inputNb*/) const
{
    return false;
}


void
ReadNode::addAcceptedComponents(int inputNb,std::list<ImageComponents>* comps)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->addAcceptedComponents(inputNb, comps);
    } else {
        comps->push_back(ImageComponents::getRGBAComponents());
    }
}


void
ReadNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->addSupportedBitDepth(depths);
    } else {
        depths->push_back(eImageBitDepthFloat);
    }
}

void
ReadNode::onInputChanged(int inputNo)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->onInputChanged(inputNo);
    }
}

void
ReadNode::purgeCaches()
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->purgeCaches();
    }
}

StatusEnum
ReadNode::getPreferredMetaDatas(NodeMetadata& metadata)
{
    return _imp->embeddedPlugin ? _imp->embeddedPlugin->getEffectInstance()->getPreferredMetaDatas(metadata) : EffectInstance::getPreferredMetaDatas(metadata);
}

void
ReadNode::onMetaDatasRefreshed(const NodeMetadata& metadata)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->onMetaDatasRefreshed(metadata);
    }
}

void
ReadNode::initializeKnobs()
{
    boost::shared_ptr<KnobPage> controlpage = AppManager::createKnob<KnobPage>(this, "Controls");
    
    boost::shared_ptr<KnobButton> fileInfos = AppManager::createKnob<KnobButton>(this, "File Info...");
    fileInfos->setName("fileInfo");
    fileInfos->setHintToolTip("Press to display informations about the file");
    controlpage->addKnob(fileInfos);
    _imp->fileInfosKnob = fileInfos;
    _imp->readNodeKnobs.push_back(fileInfos);
    
    boost::shared_ptr<KnobChoice> pluginSelector = AppManager::createKnob<KnobChoice>(this, "Decoder");
    pluginSelector->setAnimationEnabled(false);
    pluginSelector->setName(kNatronReadNodeParamDecodingPluginChoice);
    pluginSelector->setHintToolTip("Select the internal decoder plug-in used for this file format. By default this uses "
                                   "the plug-in selected for this file extension in the Preferences of Natron");
    pluginSelector->setEvaluateOnChange(false);
    _imp->pluginSelectorKnob = pluginSelector;
    controlpage->addKnob(pluginSelector);
    
    _imp->readNodeKnobs.push_back(pluginSelector);

    boost::shared_ptr<KnobSeparator> separator = AppManager::createKnob<KnobSeparator>(this, "Decoder Options");
    separator->setName("decoderOptionsSeparator");
    separator->setHintToolTip("Below can be found parameters that are specific to the Reader plug-in.");
    controlpage->addKnob(separator);
    _imp->separatorKnob = separator;
    _imp->readNodeKnobs.push_back(separator);
    
    boost::shared_ptr<KnobString> pluginID = AppManager::createKnob<KnobString>(this, "PluginID");
    pluginID->setAnimationEnabled(false);
    pluginID->setName(kNatronReadNodeParamDecodingPluginID);
    pluginID->setSecretByDefault(true);
    controlpage->addKnob(pluginID);
    _imp->pluginIDStringKnob = pluginID;
    _imp->readNodeKnobs.push_back(pluginID);
}

void
ReadNode::onEffectCreated(bool mayCreateFileDialog,
                          const std::list<boost::shared_ptr<KnobSerialization> >& defaultParamValues)
{
    //If we already loaded the Reader, do not do anything
    if (_imp->embeddedPlugin) {
        return;
    }
    bool throwErrors = false;
    
    boost::shared_ptr<KnobString> pluginIdParam = _imp->pluginIDStringKnob.lock();
    std::string pattern;
   
    if (mayCreateFileDialog) {
        pattern = getApp()->openImageFileDialog();
        
        //The user selected a file, if it fails to read do not create the node
        throwErrors = true;
    } else {

    }
    _imp->createReadNode(throwErrors, pattern, boost::shared_ptr<NodeSerialization>(), defaultParamValues);
    _imp->refreshPluginSelectorKnob();
}

void
ReadNode::onKnobsAboutToBeLoaded(const boost::shared_ptr<NodeSerialization>& serialization)
{
    assert(serialization);
    NodePtr node = getNode();
    
    //Load the pluginID to create first.
    node->loadKnob(_imp->pluginIDStringKnob.lock(), serialization->getKnobsValues());
    
    //Create the Reader with the serialization
    _imp->createReadNode(false, std::string(), serialization, std::list<boost::shared_ptr<KnobSerialization> >());
}

void
ReadNode::knobChanged(KnobI* k,
                         ValueChangedReasonEnum reason,
                         ViewSpec view,
                         double time,
                         bool originatedFromMainThread)
{

    if (k == _imp->inputFileKnob.lock().get() && reason != eValueChangedReasonTimeChanged) {
        if (_imp->creatingReadNode) {
            if (_imp->embeddedPlugin) {
                _imp->embeddedPlugin->getEffectInstance()->knobChanged(k, reason, view, time, originatedFromMainThread);
            }
            return;
        }
        _imp->refreshPluginSelectorKnob();
        boost::shared_ptr<KnobFile> fileKnob = _imp->inputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getValue();
        try {
            _imp->createReadNode(false, filename, boost::shared_ptr<NodeSerialization>(), std::list<boost::shared_ptr<KnobSerialization> >());
        } catch (const std::exception& e) {
            setPersistentMessage(eMessageTypeError, e.what());
        }
        return;
    } else if (k == _imp->pluginSelectorKnob.lock().get()) {
        boost::shared_ptr<KnobString> pluginIDKnob = _imp->pluginIDStringKnob.lock();
        std::string entry = _imp->pluginSelectorKnob.lock()->getActiveEntryText_mt_safe();
        if (entry == pluginIDKnob->getValue()) {
            return;
        }
    
        pluginIDKnob->setValue(entry);
        
        boost::shared_ptr<KnobFile> fileKnob = _imp->inputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getValue();

        try {
            _imp->createReadNode(false, filename, boost::shared_ptr<NodeSerialization>(), std::list<boost::shared_ptr<KnobSerialization> >());
        } catch (const std::exception& e) {
            setPersistentMessage(eMessageTypeError, e.what());
        }
        return;
    } else if (k == _imp->fileInfosKnob.lock().get()) {
        
        if (!_imp->embeddedPlugin) {
            return;
        }
        
        
        KnobPtr hasMetaDatasKnob = _imp->embeddedPlugin->getKnobByName("showMetadata");
        if (hasMetaDatasKnob) {
            KnobButton* showMetasKnob = dynamic_cast<KnobButton*>(hasMetaDatasKnob.get());
            if (showMetasKnob) {
                showMetasKnob->trigger();
            }
        } else {
            QString ffprobePath = ReadNodePrivate::getFFProbeBinaryPath();
            if (_imp->embeddedPlugin->getPluginID() == PLUGINID_OFX_READFFMPEG && QFile::exists(ffprobePath)) {
                QProcess proc;
                QStringList ffprobeArgs;
                ffprobeArgs << QString::fromUtf8("-show_streams");
                boost::shared_ptr<KnobFile> fileKnob = _imp->inputFileKnob.lock();
                assert(fileKnob);
                std::string filename = fileKnob->getValue();
                ffprobeArgs << QString::fromUtf8(filename.c_str());
                proc.start(ffprobePath, ffprobeArgs);
                proc.waitForFinished();
                
                QString procStdError = QString::fromUtf8(proc.readAllStandardError());
                QString procStdOutput = QString::fromUtf8(proc.readAllStandardOutput());
                Dialogs::informationDialog(getNode()->getLabel(), procStdError.toStdString() + procStdOutput.toStdString());
            }
        }
        
        return;
    }
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->knobChanged(k, reason, view, time, originatedFromMainThread);
    }
}

StatusEnum
ReadNode::getRegionOfDefinition(U64 hash, double time, const RenderScale & scale, ViewIdx view, RectD* rod)
{
    if (!_imp->checkDecoderCreated(time,view)) {
        return eStatusFailed;
    }
    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->getRegionOfDefinition(hash, time, scale, view, rod);
    } else {
        return eStatusFailed;
    }
}

void
ReadNode::getFrameRange(double *first,double *last)
{
    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->getFrameRange(first, last);
    } else {
        *first = *last = 1;
    }

}


void
ReadNode::getComponentsNeededAndProduced(double time, ViewIdx view,
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
ReadNode::beginSequenceRender(double first,
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
        return _imp->embeddedPlugin->getEffectInstance()->beginSequenceRender(first,last,step,interactive,scale,isSequentialRender, isRenderResponseToUserInteraction, draftMode, view);
    } else {
        return eStatusFailed;
    }
}

StatusEnum
ReadNode::endSequenceRender(double first,
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
        return _imp->embeddedPlugin->getEffectInstance()->endSequenceRender(first,last,step,interactive,scale,isSequentialRender, isRenderResponseToUserInteraction, draftMode, view);
    } else {
        return eStatusFailed;
    }
}

StatusEnum
ReadNode::render(const RenderActionArgs& args)
{
    if (!_imp->checkDecoderCreated(args.time,args.view)) {
        return eStatusFailed;
    }

    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->render(args);
    } else {
        return eStatusFailed;
    }

}

void
ReadNode::getRegionsOfInterest(double time,
                                  const RenderScale & scale,
                                  const RectD & outputRoD, //!< full RoD in canonical coordinates
                                  const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                  ViewIdx view,
                                  RoIMap* ret)
{
    if (_imp->embeddedPlugin) {
        _imp->embeddedPlugin->getEffectInstance()->getRegionsOfInterest(time,scale,outputRoD, renderWindow, view, ret);
    }
    
}

FramesNeededMap
ReadNode::getFramesNeeded(double time, ViewIdx view)
{
    if (_imp->embeddedPlugin) {
        return _imp->embeddedPlugin->getEffectInstance()->getFramesNeeded(time,view);
    } else {
        return FramesNeededMap();
    }
}

NATRON_NAMESPACE_EXIT;
