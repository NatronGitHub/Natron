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

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp> // iequals
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif


CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Project.h"
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
    {kParamOriginalFrameRange, false},
    {kParamFirstFrame, false},
    {kParamLastFrame, false},
    {kParamBefore, true},
    {kParamAfter, true},
    {kParamTimeDomainUserEdited, false},
    {kParamFilePremult, false},
    {kParamOutputPremult, false},
    {kParamOutputComponents, false},
    {kParamInputSpaceLabel, false},
    {kParamFrameRate, false},
    {kParamCustomFps, false},


    {kOCIOParamConfigFile, true},
    {kNatronReadNodeOCIOParamInputSpace, false},
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
ReadNode::isBundledReader(const std::string& pluginID,
                          bool wasProjectCreatedWithLowerCaseIDs)
{
    if (wasProjectCreatedWithLowerCaseIDs) {
        // Natron 1.x has plugin ids stored in lowercase
        return boost::iequals(pluginID, PLUGINID_OFX_READOIIO) ||
               boost::iequals(pluginID, PLUGINID_OFX_READFFMPEG) ||
               boost::iequals(pluginID, PLUGINID_OFX_READPFM) ||
               boost::iequals(pluginID, PLUGINID_OFX_READPSD) ||
               boost::iequals(pluginID, PLUGINID_OFX_READKRITA) ||
               boost::iequals(pluginID, PLUGINID_OFX_READSVG) ||
               boost::iequals(pluginID, PLUGINID_OFX_READMISC) ||
               boost::iequals(pluginID, PLUGINID_OFX_READORA) ||
               boost::iequals(pluginID, PLUGINID_OFX_READCDR) ||
               boost::iequals(pluginID, PLUGINID_OFX_READPNG) ||
               boost::iequals(pluginID, PLUGINID_OFX_READPDF);
    }

    return pluginID == PLUGINID_OFX_READOIIO ||
           pluginID == PLUGINID_OFX_READFFMPEG ||
           pluginID == PLUGINID_OFX_READPFM ||
           pluginID == PLUGINID_OFX_READPSD ||
           pluginID == PLUGINID_OFX_READKRITA ||
           pluginID == PLUGINID_OFX_READSVG ||
           pluginID == PLUGINID_OFX_READMISC ||
           pluginID == PLUGINID_OFX_READORA ||
           pluginID == PLUGINID_OFX_READCDR ||
           pluginID == PLUGINID_OFX_READPNG ||
           pluginID == PLUGINID_OFX_READPDF;
}

bool
ReadNode::isBundledReader(const std::string& pluginID)
{
    return isBundledReader( pluginID, getApp()->wasProjectCreatedWithLowerCaseIDs() );
}

struct ReadNodePrivate
{
    Q_DECLARE_TR_FUNCTIONS(ReadNode)

public:
    ReadNode* _publicInterface; // can not be a smart ptr
    QMutex embeddedPluginMutex;
    NodePtr embeddedPlugin;
    std::list<KnobSerializationPtr> genericKnobsSerialization;
    boost::weak_ptr<KnobFile> inputFileKnob;

    //Thiese are knobs owned by the ReadNode and not the Reader
    boost::weak_ptr<KnobChoice> pluginSelectorKnob;
    KnobStringWPtr pluginIDStringKnob;
    boost::weak_ptr<KnobSeparator> separatorKnob;
    boost::weak_ptr<KnobButton> fileInfosKnob;
    std::list<KnobIWPtr > readNodeKnobs;

    //MT only
    int creatingReadNode;


    ReadNodePrivate(ReadNode* publicInterface)
        : _publicInterface(publicInterface)
        , embeddedPluginMutex()
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
                        const NodeSerializationPtr& serialization );

    void destroyReadNode();

    void cloneGenericKnobs();

    void refreshPluginSelectorKnob();

    void refreshFileInfoVisibility(const std::string& pluginID);

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


ReadNode::ReadNode(const NodePtr& n)
    : EffectInstance(n)
    , _imp( new ReadNodePrivate(this) )
{
    setSupportsRenderScaleMaybe(eSupportsYes);
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
    KnobIPtr pageKnob = _publicInterface->getKnobByName("Controls");
    KnobPagePtr isPage = toKnobPage(pageKnob);

    if (!isPage) {
        return;
    }
    for (std::list<KnobIWPtr >::iterator it = readNodeKnobs.begin(); it != readNodeKnobs.end(); ++it) {
        KnobIPtr knob = it->lock();
        knob->setParentKnob( KnobIPtr() );
        isPage->removeKnob(knob);
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
        for (std::list<KnobIWPtr >::iterator it = readNodeKnobs.begin(); it != readNodeKnobs.end(); ++it) {
            KnobIPtr knob = it->lock();
            isPage->insertKnob(index, knob);
            ++index;
        }
    }

    children = isPage->getChildren();
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
}

void
ReadNodePrivate::cloneGenericKnobs()
{
    const KnobsVec& knobs = _publicInterface->getKnobs();

    for (std::list<KnobSerializationPtr>::iterator it = genericKnobsSerialization.begin(); it != genericKnobsSerialization.end(); ++it) {
        KnobIPtr serializedKnob = (*it)->getKnob();
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ( (*it2)->getName() == serializedKnob->getName() ) {
                KnobChoicePtr isChoice = toKnobChoice(*it2);
                KnobChoicePtr serializedIsChoice = toKnobChoice(serializedKnob);;
                if (isChoice && serializedIsChoice) {
                    const ChoiceExtraData* extraData = dynamic_cast<const ChoiceExtraData*>( (*it)->getExtraData() );
                    assert(extraData);
                    if (extraData) {
                        isChoice->choiceRestoration(serializedIsChoice, extraData);
                    }
                } else {
                    (*it2)->clone(serializedKnob);
                }
                (*it2)->setSecret( serializedKnob->getIsSecret() );
                if ( (*it2)->getDimension() == serializedKnob->getDimension() ) {
                    for (int i = 0; i < (*it2)->getDimension(); ++i) {
                        (*it2)->setEnabled( i, serializedKnob->isEnabled(i) );
                    }
                }

                break;
            }
        }
    }
}

void
ReadNodePrivate::destroyReadNode()
{
    assert( QThread::currentThread() == qApp->thread() );

    KnobsVec knobs = _publicInterface->getKnobs();

    genericKnobsSerialization.clear();

    std::string serializationString;
    try {
        std::ostringstream ss;
        boost::archive::xml_oarchive oArchive(ss);
        std::list<KnobSerializationPtr> serialized;


        for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {
            if ( !(*it)->isDeclaredByPlugin() ) {
                continue;
            }

            //If it is a knob of this ReadNode, ignore it
            bool isReadNodeKnob = false;
            for (std::list<KnobIWPtr >::iterator it2 = readNodeKnobs.begin(); it2 != readNodeKnobs.end(); ++it2) {
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
            if (isGeneric && mustSerializeKnob) {
                KnobSerializationPtr s( new KnobSerialization(*it) );
                serialized.push_back(s);
            }
            if (!isGeneric) {
                try {
                    _publicInterface->deleteKnob(*it, false);
                } catch (...) {
                    
                }
            }
        }

        int n = (int)serialized.size();
        oArchive << boost::serialization::make_nvp("numItems", n);
        for (std::list<KnobSerializationPtr>::const_iterator it = serialized.begin(); it!= serialized.end(); ++it) {
            oArchive << boost::serialization::make_nvp("item", **it);

        }
        serializationString = ss.str();
    } catch (...) {
        assert(false);
    }

    try {
        std::stringstream ss(serializationString);
        boost::archive::xml_iarchive iArchive(ss);
        int n ;
        iArchive >> boost::serialization::make_nvp("numItems", n);
        for (int i = 0; i < n; ++i) {
            KnobSerializationPtr s(new KnobSerialization);
            iArchive >> boost::serialization::make_nvp("item", *s);
            genericKnobsSerialization.push_back(s);

        }
    } catch (...) {
        assert(false);
    }

    
    //This will remove the GUI of non generic parameters
    _publicInterface->recreateKnobs(true);

    QMutexLocker k(&embeddedPluginMutex);
    embeddedPlugin.reset();
} // ReadNodePrivate::destroyReadNode

void
ReadNodePrivate::createDefaultReadNode()
{
    CreateNodeArgs args(READ_NODE_DEFAULT_READER, NodeCollectionPtr() );

    args.setProperty(kCreateNodeArgsPropNoNodeGUI, true);
    args.setProperty(kCreateNodeArgsPropOutOfProject, true);
    args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "defaultReadNodeReader");
    args.setProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer, _publicInterface->getNode());
    args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
    //args.paramValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, filePattern));


    NodePtr node  = _publicInterface->getApp()->createNode(args);

    if (!node) {
        QString error = tr("The IO.ofx.bundle OpenFX plug-in is required to use this node, make sure it is installed.");
        throw std::runtime_error( error.toStdString() );
    }

    {
        QMutexLocker k(&embeddedPluginMutex);
        embeddedPlugin = node;
    }

    //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
    _publicInterface->getNode()->declarePythonFields();

    //Destroy it to keep the default parameters
    destroyReadNode();

    separatorKnob.lock()->setSecret(true);
}

bool
ReadNodePrivate::checkDecoderCreated(double time,
                                     ViewIdx view)
{
    KnobFilePtr fileKnob = inputFileKnob.lock();

    assert(fileKnob);
    std::string pattern = fileKnob->getFileName(std::floor(time + 0.5), view);
    if ( pattern.empty() ) {
        _publicInterface->setPersistentMessage( eMessageTypeError, tr("Filename empty").toStdString() );

        return false;
    }

    if (!_publicInterface->getEmbeddedReader()) {
        QString s = tr("Decoder was not created for %1, check that the file exists and its format is supported.").arg( QString::fromUtf8( pattern.c_str() ) );
        _publicInterface->setPersistentMessage( eMessageTypeError, s.toStdString() );

        return false;
    }

    return true;
}

static std::string
getFileNameFromSerialization(const std::list<KnobSerializationPtr>& serializations)
{
    std::string filePattern;

    for (std::list<KnobSerializationPtr>::const_iterator it = serializations.begin(); it != serializations.end(); ++it) {
        if ( (*it)->getKnob()->getName() == kOfxImageEffectFileParamName ) {
            KnobStringBasePtr isString = toKnobStringBase( (*it)->getKnob() );
            assert(isString);
            if (isString) {
                filePattern = isString->getValue();
            }
            break;
        }
    }

    return filePattern;
}

void
ReadNodePrivate::createReadNode(bool throwErrors,
                                const std::string& filename,
                                const NodeSerializationPtr& serialization)
{
    if (creatingReadNode) {
        return;
    }

    SetCreatingReaderRAIIFlag creatingNode__(this);
    QString qpattern = QString::fromUtf8( filename.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::string readerPluginID;
    KnobStringPtr pluginIDKnob = pluginIDStringKnob.lock();
    readerPluginID = pluginIDKnob->getValue();


    if ( readerPluginID.empty() ) {
        KnobChoicePtr pluginChoiceKnob = pluginSelectorKnob.lock();
        int pluginChoice_i = pluginChoiceKnob->getValue();
        if (pluginChoice_i == 0) {
            //Use default
            readerPluginID = appPTR->getReaderPluginIDForFileType(ext);
        } else {
            std::vector<std::string> entries = pluginChoiceKnob->getEntries_mt_safe();
            if ( (pluginChoice_i >= 0) && ( pluginChoice_i < (int)entries.size() ) ) {
                readerPluginID = entries[pluginChoice_i];
            }
        }
    }

    //Destroy any previous reader
    //This will store the serialization of the generic knobs
    destroyReadNode();

    bool defaultFallback = false;

    if ( !ReadNode::isBundledReader(readerPluginID, false) ) {
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

        CreateNodeArgs args(readerPluginID, NodeCollectionPtr() );
        args.setProperty(kCreateNodeArgsPropNoNodeGUI, true);
        args.setProperty(kCreateNodeArgsPropOutOfProject, true);
        args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "internalDecoderNode");
        args.setProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer, _publicInterface->getNode());
        args.setProperty<NodeSerializationPtr >(kCreateNodeArgsPropNodeSerialization, serialization);
        args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

        if (serialization) {
            args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
        }

        //Set a pre-value for the inputfile knob only if it did not exist
        if (!filename.empty() && !serialization) {
            args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);

            std::string canonicalFilename = filename;
            _publicInterface->getApp()->getProject()->canonicalizePath(canonicalFilename);

            int firstFrame, lastFrame;
            Node::getOriginalFrameRangeForReader(readerPluginID, canonicalFilename, &firstFrame, &lastFrame);
            std::vector<int> originalRange(2);
            originalRange[0] = firstFrame;
            originalRange[1] = lastFrame;
            args.addParamDefaultValueN(kReaderParamNameOriginalFrameRange, originalRange);
        }


        node = _publicInterface->getApp()->createNode(args);
        {
            QMutexLocker k(&embeddedPluginMutex);
            embeddedPlugin = node;
        }

        if (pluginIDKnob) {
            pluginIDKnob->setValue(readerPluginID);
        }
        placeReadNodeKnobsInPage();

        //We need to explcitly refresh the Python knobs since we attached the embedded node knobs into this node.
        _publicInterface->getNode()->declarePythonFields();
    }


    if (!node) {
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

    KnobIPtr knob = node ? node->getKnobByName(kOfxImageEffectFileParamName) : _publicInterface->getKnobByName(kOfxImageEffectFileParamName);
    if (knob) {
        inputFileKnob = toKnobFile(knob);
    }
} // ReadNodePrivate::createReadNode

void
ReadNodePrivate::refreshFileInfoVisibility(const std::string& pluginID)
{
    KnobButtonPtr fileInfos = fileInfosKnob.lock();
    KnobIPtr hasMetaDatasKnob = _publicInterface->getKnobByName("showMetadata");
    bool hasFfprobe = false;
    if (!hasMetaDatasKnob) {
        QString ffprobePath = getFFProbeBinaryPath();
        hasFfprobe = QFile::exists(ffprobePath);
    } else {
        hasMetaDatasKnob->setSecret(true);
    }


    if ( hasMetaDatasKnob || ( (pluginID == PLUGINID_OFX_READFFMPEG) && hasFfprobe ) ) {
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
    std::string filePattern = fileKnob->getValue();
    std::vector<std::string> entries, help;
    entries.push_back(kPluginSelectorParamEntryDefault);
    help.push_back("Use the default plug-in chosen from the Preferences to read this file format");

    QString qpattern = QString::fromUtf8( filePattern.c_str() );
    std::string ext = QtCompat::removeFileExtension(qpattern).toLower().toStdString();
    std::string pluginID;
    if ( !ext.empty() ) {
        pluginID = appPTR->getReaderPluginIDForFileType(ext);
        IOPluginSetForFormat readersForFormat;
        appPTR->getReadersForFormat(ext, &readersForFormat);

        // Reverse it so that we sort them by decreasing score order
        for (IOPluginSetForFormat::reverse_iterator it = readersForFormat.rbegin(); it != readersForFormat.rend(); ++it) {
            Plugin* plugin = appPTR->getPluginBinary(QString::fromUtf8( it->pluginID.c_str() ), -1, -1, false);
            entries.push_back( plugin->getPluginID().toStdString() );
            std::stringstream ss;
            ss << "Use " << plugin->getPluginLabel().toStdString() << " version ";
            ss << plugin->getMajorVersion() << "." << plugin->getMinorVersion();
            ss << " to read this file format";
            help.push_back( ss.str() );
        }
    }

    KnobChoicePtr pluginChoice = pluginSelectorKnob.lock();

    pluginChoice->populateChoices(entries, help);
    pluginChoice->blockValueChanges();
    pluginChoice->resetToDefaultValue(0);
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

    refreshFileInfoVisibility(pluginID);

} // ReadNodePrivate::refreshPluginSelectorKnob

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
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->isMultiPlanar() : EffectInstance::isMultiPlanar();
}

bool
ReadNode::isViewAware() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->isViewAware() : EffectInstance::isViewAware();
}

bool
ReadNode::supportsTiles() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->supportsTiles() : EffectInstance::supportsTiles();
}

bool
ReadNode::supportsMultiResolution() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->supportsMultiResolution() : EffectInstance::supportsMultiResolution();
}

bool
ReadNode::supportsMultipleClipsBitDepth() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->supportsMultipleClipsBitDepth() : EffectInstance::supportsMultipleClipsBitDepth();
}

RenderSafetyEnum
ReadNode::renderThreadSafety() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->renderThreadSafety() : eRenderSafetyFullySafe;
}

bool
ReadNode::getCanTransform() const
{
    return false;
}

SequentialPreferenceEnum
ReadNode::getSequentialPreference() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->getSequentialPreference() : EffectInstance::getSequentialPreference();
}

EffectInstance::ViewInvarianceLevel
ReadNode::isViewInvariant() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->isViewInvariant() : EffectInstance::isViewInvariant();
}

EffectInstance::PassThroughEnum
ReadNode::isPassThroughForNonRenderedPlanes() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->isPassThroughForNonRenderedPlanes() : EffectInstance::isPassThroughForNonRenderedPlanes();
}

bool
ReadNode::getCreateChannelSelectorKnob() const
{
    return false;
}

bool
ReadNode::isHostChannelSelectorSupported(bool* /*defaultR*/,
                                         bool* /*defaultG*/,
                                         bool* /*defaultB*/,
                                         bool* /*defaultA*/) const
{
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
ReadNode::addAcceptedComponents(int inputNb,
                                std::list<ImageComponents>* comps)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->addAcceptedComponents(inputNb, comps);
    } else {
        comps->push_back( ImageComponents::getRGBAComponents() );
    }
}

void
ReadNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->addSupportedBitDepth(depths);
    } else {
        depths->push_back(eImageBitDepthFloat);
    }
}

void
ReadNode::onInputChanged(int inputNo)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->onInputChanged(inputNo);
    }
}

void
ReadNode::purgeCaches()
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->purgeCaches();
    }
}

StatusEnum
ReadNode::getPreferredMetaDatas(NodeMetadata& metadata)
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->getPreferredMetaDatas(metadata) : EffectInstance::getPreferredMetaDatas(metadata);
}

void
ReadNode::onMetaDatasRefreshed(const NodeMetadata& metadata)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->onMetaDatasRefreshed(metadata);
    }
}

void
ReadNode::initializeKnobs()
{
    KnobPagePtr controlpage = AppManager::createKnob<KnobPage>( shared_from_this(), tr("Controls") );
    KnobButtonPtr fileInfos = AppManager::createKnob<KnobButton>( shared_from_this(), tr("File Info...") );

    fileInfos->setName("fileInfo");
    fileInfos->setHintToolTip( tr("Press to display informations about the file") );
    controlpage->addKnob(fileInfos);
    _imp->fileInfosKnob = fileInfos;
    _imp->readNodeKnobs.push_back(fileInfos);

    KnobChoicePtr pluginSelector = AppManager::createKnob<KnobChoice>( shared_from_this(), tr("Decoder") );
    pluginSelector->setAnimationEnabled(false);
    pluginSelector->setName(kNatronReadNodeParamDecodingPluginChoice);
    pluginSelector->setHintToolTip( tr("Select the internal decoder plug-in used for this file format. By default this uses "
                                       "the plug-in selected for this file extension in the Preferences of Natron") );
    pluginSelector->setEvaluateOnChange(false);
    _imp->pluginSelectorKnob = pluginSelector;
    controlpage->addKnob(pluginSelector);

    _imp->readNodeKnobs.push_back(pluginSelector);

    KnobSeparatorPtr separator = AppManager::createKnob<KnobSeparator>( shared_from_this(), tr("Decoder Options") );
    separator->setName("decoderOptionsSeparator");
    separator->setHintToolTip( tr("Below can be found parameters that are specific to the Reader plug-in.") );
    controlpage->addKnob(separator);
    _imp->separatorKnob = separator;
    _imp->readNodeKnobs.push_back(separator);

    KnobStringPtr pluginID = AppManager::createKnob<KnobString>( shared_from_this(), tr("PluginID") );
    pluginID->setAnimationEnabled(false);
    pluginID->setName(kNatronReadNodeParamDecodingPluginID);
    pluginID->setSecretByDefault(true);
    controlpage->addKnob(pluginID);
    _imp->pluginIDStringKnob = pluginID;
    _imp->readNodeKnobs.push_back(pluginID);
}

void
ReadNode::onEffectCreated(bool mayCreateFileDialog,
                          const CreateNodeArgs& args)
{
    //If we already loaded the Reader, do not do anything
    NodePtr p = getEmbeddedReader();
    if (p) {
        return;
    }
    bool throwErrors = false;
    KnobStringPtr pluginIdParam = _imp->pluginIDStringKnob.lock();
    std::string pattern;

    if (mayCreateFileDialog) {
        if ( !getApp()->isBackground() ) {
            pattern = getApp()->openImageFileDialog();

            // File dialog was closed, ignore
            if ( pattern.empty() ) {
                throw std::runtime_error("");
            }
        }

        //The user selected a file, if it fails to read do not create the node
        throwErrors = true;
    } else {
        std::vector<std::string> defaultParamValues = args.getPropertyN<std::string>(kCreateNodeArgsPropNodeInitialParamValues);
        std::vector<std::string>::iterator foundFileName  = std::find(defaultParamValues.begin(), defaultParamValues.end(), std::string(kOfxImageEffectFileParamName));
        if (foundFileName != defaultParamValues.end()) {
            std::string propName(kCreateNodeArgsPropParamValue);
            propName += "_";
            propName += kOfxImageEffectFileParamName;
            pattern = args.getProperty<std::string>(propName);
        }
    }
    _imp->createReadNode( throwErrors, pattern, NodeSerializationPtr() );
    _imp->refreshPluginSelectorKnob();
}

void
ReadNode::onKnobsAboutToBeLoaded(const NodeSerializationPtr& serialization)
{
    assert(serialization);
    NodePtr node = getNode();

    //Load the pluginID to create first.
    node->loadKnob( _imp->pluginIDStringKnob.lock(), serialization->getKnobsValues() );

    std::string filename = getFileNameFromSerialization( serialization->getKnobsValues() );
    //Create the Reader with the serialization
    _imp->createReadNode(false, filename, serialization);
    _imp->refreshPluginSelectorKnob();
}

bool
ReadNode::knobChanged(const KnobIPtr& k,
                      ValueChangedReasonEnum reason,
                      ViewSpec view,
                      double time,
                      bool originatedFromMainThread)
{
    bool ret =  true;

    if ( ( k == _imp->inputFileKnob.lock() ) && (reason != eValueChangedReasonTimeChanged) ) {

        NodePtr hasMaster = getNode()->getMasterNode();
        if (hasMaster) {
            // Unslave all knobs since we are going to remove some and recreate others
            unslaveAllKnobs();
        }
        if (_imp->creatingReadNode) {
            NodePtr p = getEmbeddedReader();
            if (p) {
                p->getEffectInstance()->knobChanged(k, reason, view, time, originatedFromMainThread);
            }

            return false;
        }
        try {
            _imp->refreshPluginSelectorKnob();
        } catch (const std::exception& e) {
            setPersistentMessage( eMessageTypeError, e.what() );
        }

        KnobFilePtr fileKnob = _imp->inputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getValue();
        try {
            _imp->createReadNode( false, filename, NodeSerializationPtr() );
        } catch (const std::exception& e) {
            setPersistentMessage( eMessageTypeError, e.what() );
        }
        if (hasMaster) {
            slaveAllKnobs(hasMaster->getEffectInstance(), false);
        }
    } else if ( k == _imp->pluginSelectorKnob.lock() ) {
        KnobStringPtr pluginIDKnob = _imp->pluginIDStringKnob.lock();
        std::string entry = _imp->pluginSelectorKnob.lock()->getActiveEntryText_mt_safe();
        if ( entry == pluginIDKnob->getValue() ) {
            return false;
        }

        if (entry == "Default") {
            entry.clear();
        }

        pluginIDKnob->setValue(entry);

        KnobFilePtr fileKnob = _imp->inputFileKnob.lock();
        assert(fileKnob);
        std::string filename = fileKnob->getValue();

        try {
            _imp->createReadNode( false, filename, NodeSerializationPtr() );
        } catch (const std::exception& e) {
            setPersistentMessage( eMessageTypeError, e.what() );
        }
        _imp->refreshFileInfoVisibility(entry);
    } else if ( k == _imp->fileInfosKnob.lock() ) {
        NodePtr p = getEmbeddedReader();
        if (!p) {
            return false;
        }


        KnobIPtr hasMetaDatasKnob = p->getKnobByName("showMetadata");
        if (hasMetaDatasKnob) {
            KnobButtonPtr showMetasKnob = toKnobButton(hasMetaDatasKnob);
            if (showMetasKnob) {
                showMetasKnob->trigger();
            }
        } else {
            QString ffprobePath = ReadNodePrivate::getFFProbeBinaryPath();
            if ( (p->getPluginID() == PLUGINID_OFX_READFFMPEG) && QFile::exists(ffprobePath) ) {
                QProcess proc;
                QStringList ffprobeArgs;
                ffprobeArgs << QString::fromUtf8("-show_streams");
                KnobFilePtr fileKnob = _imp->inputFileKnob.lock();
                assert(fileKnob);
                std::string filename = fileKnob->getValue();
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
        ret |= p->getEffectInstance()->knobChanged(k, reason, view, time, originatedFromMainThread);
    }

    return ret;
} // ReadNode::knobChanged

StatusEnum
ReadNode::getRegionOfDefinition(U64 hash,
                                double time,
                                const RenderScale & scale,
                                ViewIdx view,
                                RectD* rod)
{
    if ( !_imp->checkDecoderCreated(time, view) ) {
        return eStatusFailed;
    }
    NodePtr p = getEmbeddedReader();
    if (p) {
        return p->getEffectInstance()->getRegionOfDefinition(hash, time, scale, view, rod);
    } else {
        return eStatusFailed;
    }
}

void
ReadNode::getFrameRange(double *first,
                        double *last)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        return p->getEffectInstance()->getFrameRange(first, last);
    } else {
        *first = *last = 1;
    }
}

void
ReadNode::getComponentsNeededAndProduced(double time,
                                         ViewIdx view,
                                         EffectInstance::ComponentsNeededMap* comps,
                                         SequenceTime* passThroughTime,
                                         int* passThroughView,
                                         NodePtr* passThroughInput)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->getComponentsNeededAndProduced(time, view, comps, passThroughTime, passThroughView, passThroughInput);
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
                              ViewIdx view,
                              bool isOpenGLRender,
                              const EffectInstance::OpenGLContextEffectDataPtr& glContextData)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        return p->getEffectInstance()->beginSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view, isOpenGLRender, glContextData);
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
                            ViewIdx view,
                            bool isOpenGLRender,
                            const EffectInstance::OpenGLContextEffectDataPtr& glContextData)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        return p->getEffectInstance()->endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view, isOpenGLRender, glContextData);
    } else {
        return eStatusFailed;
    }
}

StatusEnum
ReadNode::render(const RenderActionArgs& args)
{
    if ( !_imp->checkDecoderCreated(args.time, args.view) ) {
        return eStatusFailed;
    }

    NodePtr p = getEmbeddedReader();
    if (p) {
        return p->getEffectInstance()->render(args);
    } else {
        return eStatusFailed;
    }
}

void
ReadNode::getRegionsOfInterest(double time,
                               const RenderScale & scale,
                               const RectD & outputRoD,    //!< full RoD in canonical coordinates
                               const RectD & renderWindow,    //!< the region to be rendered in the output image, in Canonical Coordinates
                               ViewIdx view,
                               RoIMap* ret)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->getRegionsOfInterest(time, scale, outputRoD, renderWindow, view, ret);
    }
}

FramesNeededMap
ReadNode::getFramesNeeded(double time,
                          ViewIdx view)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        return p->getEffectInstance()->getFramesNeeded(time, view);
    } else {
        return FramesNeededMap();
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ReadNode.cpp"
