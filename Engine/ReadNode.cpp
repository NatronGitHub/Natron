/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#include "ReadNode.h"

#include <sstream> // stringstream

#include "Global/QtCompat.h"


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format off
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
// clang-format on
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

//The plug-in that is instantiated whenever this node is created and doesn't point to any valid or known extension
#define READ_NODE_DEFAULT_READER PLUGINID_OFX_READOIIO
#define kPluginSelectorParamEntryDefault "Default"

NATRON_NAMESPACE_ENTER

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
ReadNode::isBundledReader(const std::string& pluginID,
                          bool wasProjectCreatedWithLowerCaseIDs)
{
    if (wasProjectCreatedWithLowerCaseIDs) {
        // Natron 1.x has plugin ids stored in lowercase
        return ( StrUtils::iequals(pluginID, PLUGINID_OFX_READOIIO) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READFFMPEG) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READPFM) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READPSD) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READKRITA) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READSVG) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READMISC) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READORA) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READCDR) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READPNG) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READPDF) ||
                 StrUtils::iequals(pluginID, PLUGINID_OFX_READBRAW) );
    }

    return (pluginID == PLUGINID_OFX_READOIIO ||
            pluginID == PLUGINID_OFX_READFFMPEG ||
            pluginID == PLUGINID_OFX_READPFM ||
            pluginID == PLUGINID_OFX_READPSD ||
            pluginID == PLUGINID_OFX_READKRITA ||
            pluginID == PLUGINID_OFX_READSVG ||
            pluginID == PLUGINID_OFX_READMISC ||
            pluginID == PLUGINID_OFX_READORA ||
            pluginID == PLUGINID_OFX_READCDR ||
            pluginID == PLUGINID_OFX_READPNG ||
            pluginID == PLUGINID_OFX_READPDF ||
            pluginID == PLUGINID_OFX_READBRAW);
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
    ReadNode* _publicInterface;
    QMutex embeddedPluginMutex;
    NodePtr embeddedPlugin;
    std::list<KnobSerializationPtr> genericKnobsSerialization;
    KnobFileWPtr inputFileKnob;

    //Thiese are knobs owned by the ReadNode and not the Reader
    KnobChoiceWPtr pluginSelectorKnob;
    KnobStringWPtr pluginIDStringKnob;
    KnobSeparatorWPtr separatorKnob;
    KnobButtonWPtr fileInfosKnob;
    std::list<KnobIWPtr> readNodeKnobs;

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
    , pluginIDStringKnob()
    , separatorKnob()
    , fileInfosKnob()
    , readNodeKnobs()
    , creatingReadNode(0)
    , lastPluginIDCreated()
    , wasCreatedAsHiddenNode(false)
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


ReadNode::ReadNode(NodePtr n)
    : EffectInstance(n)
    , _imp( new ReadNodePrivate(this) )
{
    setSupportsRenderScaleMaybe(eSupportsNo);
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
    KnobPage* isPage = dynamic_cast<KnobPage*>( pageKnob.get() );

    if (!isPage) {
        return;
    }
    for (std::list<KnobIWPtr>::iterator it = readNodeKnobs.begin(); it != readNodeKnobs.end(); ++it) {
        KnobIPtr knob = it->lock();
        knob->setParentKnob( KnobIPtr() );
        isPage->removeKnob( knob.get() );
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
        for (std::list<KnobIWPtr>::iterator it = readNodeKnobs.begin(); it != readNodeKnobs.end(); ++it) {
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
                separatorKnob.lock()->setSecret(dynamic_cast<KnobSeparator*>(children[foundSep].get()));
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
                KnobChoice* isChoice = dynamic_cast<KnobChoice*>( (*it2).get() );
                KnobChoice* choiceSerialized = dynamic_cast<KnobChoice*>( serializedKnob.get() );;
                if (isChoice && choiceSerialized) {
                    const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>( (*it)->getExtraData() );
                    assert(choiceData);
                    if (choiceData) {
                        std::string optionID = choiceData->_choiceString;
                        // first, try to get the id the easy way ( see choiceMatch() )
                        int id = isChoice->choiceRestorationId(choiceSerialized, optionID);
#pragma message WARN("TODO: choice id filters")
                        //if (id < 0) {
                        //    // no luck, try the filters
                        //    filterKnobChoiceOptionCompat(getPluginID(), serialization.getPluginMajorVersion(), serialization.getPluginMinorVersion(), projectInfos.vMajor, projectInfos.vMinor, projectInfos.vRev, serializedName, &optionID);
                        //    id = isChoice->choiceRestorationId(choiceSerialized, optionID);
                        //}
                        isChoice->choiceRestoration(choiceSerialized, optionID, id);
                    }
                } else {
                    (*it2)->clone( serializedKnob.get() );
                }
                //(*it2)->setSecret( serializedKnob->getIsSecret() );
                /*if ( (*it2)->getDimension() == serializedKnob->getDimension() ) {
                    for (int i = 0; i < (*it2)->getDimension(); ++i) {
                        (*it2)->setEnabled( i, serializedKnob->isEnabled(i) );
                    }
                }*/

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
    KnobsVec knobs = _publicInterface->getKnobs();

    genericKnobsSerialization.clear();

    std::string serializationString;
    try {
        std::ostringstream ss;
        {   // see http://boost.2283326.n4.nabble.com/the-boost-xml-serialization-to-a-stringstream-does-not-have-an-end-tag-td2580772.html
            // xml_oarchive must be destroyed before obtaining ss.str(), or the </boost_serialization> tag is missing,
            // which throws an exception in boost 1.66.0, due to the following change:
            // https://fossies.org/diffs/boost/1_65_1_vs_1_66_0/libs/serialization/src/basic_xml_grammar.ipp-diff.html
            // see also https://svn.boost.org/trac10/ticket/13400
            // see also https://svn.boost.org/trac10/ticket/13354
            
            boost::archive::xml_oarchive oArchive(ss);
            std::list<KnobSerializationPtr> serialized;
            
            
            for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {
                
                // The internal node still holds a shared ptr to the knob.
                // Since we want to keep some knobs around, ensure they do not get deleted in the destructor of the embedded node
                embeddedPlugin->getEffectInstance()->removeKnobFromList(it->get());
                
                if ( !(*it)->isDeclaredByPlugin() ) {
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
                KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
                if (isPage) {
                    continue;
                }
                
                //This is a knob of the Reader plug-in
                
                //Serialize generic knobs and keep them around until we create a new Reader plug-in
                bool mustSerializeKnob;
                bool isGeneric = isGenericKnob( (*it)->getName(), &mustSerializeKnob );
                if (!isGeneric || mustSerializeKnob) {
                    
                    /* if (!isGeneric && !(*it)->getDefaultIsSecret()) {
                     // Don't save the secret state otherwise some knobs could be invisible when cloning the serialization even if we change format
                     (*it)->setSecret(false);
                     }*/
                    
                    KnobSerializationPtr s = std::make_shared<KnobSerialization>(*it);
                    serialized.push_back(s);
                }
                if (!isGeneric) {
                    try {
                        _publicInterface->deleteKnob(it->get(), false);
                    } catch (...) {
                        
                    }
                }
            }
            
            int n = (int)serialized.size();
            oArchive << boost::serialization::make_nvp("numItems", n);
            for (std::list<KnobSerializationPtr>::const_iterator it = serialized.begin(); it!= serialized.end(); ++it) {
                oArchive << boost::serialization::make_nvp("item", **it);
                
            }
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
            KnobSerializationPtr s = std::make_shared<KnobSerialization>();
            iArchive >> boost::serialization::make_nvp("item", *s);
            genericKnobsSerialization.push_back(s);

        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        assert(false);
    } catch (...) {
        assert(false);
    }

    
    //This will remove the GUI of non generic parameters
    _publicInterface->recreateKnobs(true);
#pragma message WARN("TODO: if Gui, refresh pluginID, version, help tooltip in DockablePanel to reflect embedded node change")

    QMutexLocker k(&embeddedPluginMutex);
    if (embeddedPlugin) {
        embeddedPlugin->destroyNode(true, false);
    }
    embeddedPlugin.reset();

} // ReadNodePrivate::destroyReadNode

void
ReadNodePrivate::createDefaultReadNode()
{
    CreateNodeArgs args(READ_NODE_DEFAULT_READER, NodeCollectionPtr() );

    args.setProperty(kCreateNodeArgsPropNoNodeGUI, true);
    args.setProperty(kCreateNodeArgsPropSilent, true);
    args.setProperty(kCreateNodeArgsPropOutOfProject, true);
    args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "defaultReadNodeReader");
    args.setProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer, _publicInterface->getNode());
    args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

    // This will avoid throwing errors when creating the reader
    args.addParamDefaultValue<bool>("ParamExistingInstance", true);


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
            KnobStringBase* isString = dynamic_cast<KnobStringBase*>( (*it)->getKnob().get() );
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
            std::vector<ChoiceOption> entries = pluginChoiceKnob->getEntries_mt_safe();
            if ( (pluginChoice_i >= 0) && ( pluginChoice_i < (int)entries.size() ) ) {
                readerPluginID = entries[pluginChoice_i].id;
            }
        }
    }

    // If the plug-in is the same, do not create a new decoder.
    if (embeddedPlugin && embeddedPlugin->getPluginID() == readerPluginID) {
        KnobFilePtr fileKnob = inputFileKnob.lock();
        assert(fileKnob);
        if (fileKnob) {
            // Make sure instance changed action is called on the decoder and not caught in our knobChanged handler.
            embeddedPlugin->getEffectInstance()->onKnobValueChanged_public(fileKnob.get(), eValueChangedReasonNatronInternalEdited, _publicInterface->getCurrentTime(), ViewSpec(0), true);

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

    if ( !defaultFallback && !ReadNode::isBundledReader(readerPluginID, _publicInterface->getApp()->wasProjectCreatedWithLowerCaseIDs()) ) {
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
        args.setProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization, serialization);
        args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

        if (serialization || wasCreatedAsHiddenNode) {
            args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
            args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true); // also load deprecated plugins
        }


        node = _publicInterface->getApp()->createNode(args);

        // Set the filename value
        if (node) {
            KnobFilePtr fileKnob = std::dynamic_pointer_cast<KnobFile>(node->getKnobByName(kOfxImageEffectFileParamName));
            if (fileKnob) {
                fileKnob->setValue(filename);
            }
        }
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


    // Clone the old values of the generic knobs if we created the same decoder than before
    if (lastPluginIDCreated == readerPluginID) {
        cloneGenericKnobs();
    }
    lastPluginIDCreated = readerPluginID;


    NodePtr thisNode = _publicInterface->getNode();
    //Refresh accepted bitdepths on the node
    thisNode->refreshAcceptedBitDepths();

    //Refresh accepted components
    thisNode->initializeInputs();

    //This will refresh the GUI with this Reader specific parameters
    _publicInterface->recreateKnobs(true);
#pragma message WARN("TODO: if Gui, refresh pluginID, version, help tooltip in DockablePanel to reflect embedded node change")

    KnobIPtr knob = node ? node->getKnobByName(kOfxImageEffectFileParamName) : _publicInterface->getKnobByName(kOfxImageEffectFileParamName);
    if (knob) {
        inputFileKnob = std::dynamic_pointer_cast<KnobFile>(knob);
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
    std::string filePattern = fileKnob->getValue();
    std::vector<ChoiceOption> entries, help;
    entries.push_back(ChoiceOption(kPluginSelectorParamEntryDefault, "", ReadNode::tr("Use the default plug-in chosen from the Preferences to read this file format").toStdString()));

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
            std::stringstream ss;
            ss << "Use " << plugin->getPluginLabel().toStdString() << " version ";
            ss << plugin->getMajorVersion() << "." << plugin->getMinorVersion();
            ss << " to read this file format";
            entries.push_back( ChoiceOption(plugin->getPluginID().toStdString(), "", ss.str()));
        }
    }

    KnobChoicePtr pluginChoice = pluginSelectorKnob.lock();

    pluginChoice->populateChoices(entries);
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
ReadNode::supportsMultipleClipDepths() const
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->supportsMultipleClipDepths() : EffectInstance::supportsMultipleClipDepths();
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
ReadNode::getNInputs() const
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
                                std::list<ImagePlaneDesc>* comps)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->addAcceptedComponents(inputNb, comps);
    } else {
        comps->push_back( ImagePlaneDesc::getRGBAComponents() );
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
ReadNode::getPreferredMetadata(NodeMetadata& metadata)
{
    NodePtr p = getEmbeddedReader();
    return p ? p->getEffectInstance()->getPreferredMetadata(metadata) : EffectInstance::getPreferredMetadata(metadata);
}

void
ReadNode::onMetadataRefreshed(const NodeMetadata& metadata)
{
    NodePtr p = getEmbeddedReader();
    if (p) {
        p->getEffectInstance()->setMetadataInternal(metadata);
        p->getEffectInstance()->onMetadataRefreshed(metadata);
    }
}

void
ReadNode::initializeKnobs()
{
    KnobPagePtr controlpage = AppManager::createKnob<KnobPage>( this, tr("Controls") );
    KnobButtonPtr fileInfos = AppManager::createKnob<KnobButton>( this, tr("File Info...") );

    fileInfos->setName("fileInfo");
    fileInfos->setHintToolTip( tr("Press to display information about the file") );
    controlpage->addKnob(fileInfos);
    _imp->fileInfosKnob = fileInfos;
    _imp->readNodeKnobs.push_back(fileInfos);

    KnobChoicePtr pluginSelector = AppManager::createKnob<KnobChoice>( this, tr("Decoder") );
    pluginSelector->setAnimationEnabled(false);
    pluginSelector->setName(kNatronReadNodeParamDecodingPluginChoice);
    pluginSelector->setHintToolTip( tr("Select the internal decoder plug-in used for this file format. By default this uses "
                                       "the plug-in selected for this file extension in the Preferences of Natron") );
    pluginSelector->setEvaluateOnChange(false);
    _imp->pluginSelectorKnob = pluginSelector;
    controlpage->addKnob(pluginSelector);

    _imp->readNodeKnobs.push_back(pluginSelector);

    KnobSeparatorPtr separator = AppManager::createKnob<KnobSeparator>( this, tr("Decoder Options") );
    separator->setName("decoderOptionsSeparator");
    separator->setHintToolTip( tr("Below can be found parameters that are specific to the Reader plug-in.") );
    controlpage->addKnob(separator);
    _imp->separatorKnob = separator;
    _imp->readNodeKnobs.push_back(separator);

    KnobStringPtr pluginID = AppManager::createKnob<KnobString>( this, tr("PluginID") );
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
        // Ensure the plug-in ID knob has the same value as the created reader:
        // The reader might have been created in onKnobsAboutToBeLoaded() however the knobs
        // get loaded afterwards and the plug-in ID could not reflect the underlying plugin
        KnobStringPtr pluginIDKnob = _imp->pluginIDStringKnob.lock();
        if (pluginIDKnob) {
            pluginIDKnob->setValue(p->getPluginID());
        }
        return;
    }

    _imp->wasCreatedAsHiddenNode = args.getProperty<bool>(kCreateNodeArgsPropNoNodeGUI);

    bool throwErrors = false;
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
    node->loadKnob( _imp->pluginIDStringKnob.lock(), *serialization );

    std::string filename = getFileNameFromSerialization( serialization->getKnobsValues() );
    //Create the Reader with the serialization
    _imp->createReadNode(false, filename, serialization);
    _imp->refreshPluginSelectorKnob();
}

bool
ReadNode::knobChanged(KnobI* k,
                      ValueChangedReasonEnum reason,
                      ViewSpec view,
                      double time,
                      bool originatedFromMainThread)
{
    bool ret =  true;

    if ( ( k == _imp->inputFileKnob.lock().get() ) && (reason != eValueChangedReasonTimeChanged) ) {

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
            slaveAllKnobs(hasMaster->getEffectInstance().get(), false);
        }
    } else if ( k == _imp->pluginSelectorKnob.lock().get() ) {
        KnobStringPtr pluginIDKnob = _imp->pluginIDStringKnob.lock();
        std::string entry = _imp->pluginSelectorKnob.lock()->getActiveEntry().id;
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
        // Make sure instance changed action is called on the decoder and not caught in our knobChanged handler.
        if (_imp->embeddedPlugin) {
            _imp->embeddedPlugin->getEffectInstance()->onKnobValueChanged_public(fileKnob.get(), eValueChangedReasonNatronInternalEdited, getCurrentTime(), ViewSpec(0), true);
        }
        

        _imp->refreshFileInfoVisibility(entry);
    } else if ( k == _imp->fileInfosKnob.lock().get() ) {
        NodePtr p = getEmbeddedReader();
        if (!p) {
            return false;
        }


        KnobIPtr hasMetadataKnob = p->getKnobByName("showMetadata");
        if (hasMetadataKnob) {
            KnobButton* showMetasKnob = dynamic_cast<KnobButton*>( hasMetadataKnob.get() );
            if (showMetasKnob) {
                showMetasKnob->trigger();
            }
        } else {
            if ( ReadNode::isVideoReader( p->getPluginID() ) ) {
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
                                         double* passThroughTime,
                                         int* passThroughView,
                                         int* passThroughInput)
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

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ReadNode.cpp"
