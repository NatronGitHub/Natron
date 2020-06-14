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

#include "NodePrivate.h"

#include <sstream>

#include "Engine/OfxEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/LoadKnobsCompat.h"
#include "Engine/Settings.h"
#include "Engine/RenderEngine.h"
#include "Engine/InputDescription.h"
#include "Engine/StubNode.h"
#include "Serialization/NodeSerialization.h"

NATRON_NAMESPACE_ENTER



void
Node::initNodeNameFallbackOnPluginDefault()
{
    NodeCollectionPtr group = getGroup();
    std::string name;
    std::string pluginLabel;
    AppManager::AppTypeEnum appType = appPTR->getAppType();
    PluginPtr plugin = getPlugin();
    if ( plugin &&
        ( ( appType == AppManager::eAppTypeBackground) ||
         ( appType == AppManager::eAppTypeGui) ||
         ( appType == AppManager::eAppTypeInterpreter) ) ) {
            pluginLabel = plugin->getLabelWithoutSuffix();
        } else {
            pluginLabel = plugin->getPluginLabel();
        }
    try {
        if (group) {
            group->initNodeName(plugin->getPluginID(), pluginLabel, &name);
        } else {
            name = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(pluginLabel);
        }
    } catch (...) {
    }

    setNameInternal(name.c_str(), false);
}



void
Node::createNodeGuiInternal(const CreateNodeArgsPtr& args)
{

    // The container group UI should have been created so far
    NodePtr thisShared = shared_from_this();
    NodeCollectionPtr group = getGroup();
    NodeGraphI* graph_i = group->getNodeGraph();
    if (graph_i) {
        graph_i->createNodeGui(thisShared, *args);

        // The gui pointer is set in the constructor of NodeGui
        if (!_imp->guiPointer.lock()) {
            throw std::runtime_error(tr("Could not create GUI for node %1").arg(QString::fromUtf8(getScriptName_mt_safe().c_str())).toStdString());
        }
    }
}

void
Node::load(const CreateNodeArgsPtr& args)
{
    // Called from the main thread. MT-safe
    assert( QThread::currentThread() == qApp->thread() );

    // Cannot load twice
    assert(!_imp->effect);

    NodePtr thisShared = shared_from_this();

    // Flag that we are creating a node
    boost::scoped_ptr<AddCreateNode_RAII> creatingNode_raii(new AddCreateNode_RAII(getApp(), thisShared, args));

    // Should this node be persistent
    _imp->isPersistent = !args->getPropertyUnsafe<bool>(kCreateNodeArgsPropVolatile);

    // For Readers & Writers this is a hack to enable the internal decoder/encoder node to have a pointer to the main node the user sees
    _imp->ioContainer = args->getPropertyUnsafe<NodePtr>(kCreateNodeArgsPropMetaNodeContainer);

    NodeCollectionPtr group = getGroup();
    assert(group);
    if (!group) {
        throw std::logic_error("Node::load no container group!");
    }

    // Add the node to the group before initializing anything else
    group->addNode(thisShared);

    // Should we report errors if load fails ?
    _imp->wasCreatedSilently = args->getPropertyUnsafe<bool>(kCreateNodeArgsPropSilent);

    // If this is a pyplug, load its properties
    std::string pyPlugID = args->getPropertyUnsafe<std::string>(kCreateNodeArgsPropPyPlugID);
    if (!pyPlugID.empty()) {
        _imp->pyPlugHandle = appPTR->getPluginBinary(QString::fromUtf8(pyPlugID.c_str()), -1, -1, false);
        _imp->isPyPlug = true;
    }


    // Any serialization from project load or copy/paste ?
    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization = args->getPropertyUnsafe<SERIALIZATION_NAMESPACE::NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);

    // Should we load a preset ?
    std::string presetLabel = args->getPropertyUnsafe<std::string>(kCreateNodeArgsPropPreset);
    if (!presetLabel.empty()) {
        // If there's a preset specified, load serialization from preset

        // Figure out the plugin to use. We cannot use getPlugin() now because the effect is not yet created
        PluginPtr plugin = _imp->pyPlugHandle.lock();
        if (!plugin) {
            plugin = _imp->plugin.lock();
        }
        const std::vector<PluginPresetDescriptor>& presets = plugin->getPresetFiles();
        for (std::vector<PluginPresetDescriptor>::const_iterator it = presets.begin(); it!=presets.end(); ++it) {
            if (it->presetLabel.toStdString() == presetLabel) {

                // We found a matching preset
                _imp->initialNodePreset = presetLabel;
                break;
            }
        }
    } else if (serialization) {
        // The serialization had a preset
        _imp->initialNodePreset = serialization->_presetInstanceLabel;
    }




    std::string argFixedName = args->getPropertyUnsafe<std::string>(kCreateNodeArgsPropNodeInitialName);

    PluginPtr pluginPtr = _imp->plugin.lock();


    // Get the function pointer to create the plug-in instance
    GCC_DIAG_PEDANTIC_OFF
    EffectBuilder createFunc = (EffectBuilder)pluginPtr->getPropertyUnsafe<void*>(kNatronPluginPropCreateFunc);
    GCC_DIAG_PEDANTIC_ON
    assert(createFunc);
    if (!createFunc) {
        throw std::invalid_argument("Node::load: No kNatronPluginPropCreateFunc property set on plug-in!");
    }
    _imp->effect = createFunc(thisShared);
    assert(_imp->effect);
    assert(!_imp->effect->isRenderClone());
    if (!_imp->effect) {
        throw std::runtime_error(tr("Could not create instance of %1").arg(QString::fromUtf8(getPluginID().c_str())).toStdString());
    }

    // Hack for Reader/Writer node
    NodePtr ioContainer = _imp->ioContainer.lock();
    if (ioContainer) {
        ReadNodePtr isReader = ioContainer->isEffectReadNode();
        if (isReader) {
            isReader->setEmbeddedReader(thisShared);
        } else {
            WriteNodePtr isWriter = ioContainer->isEffectWriteNode();
            assert(isWriter);
            if (isWriter) {
                isWriter->setEmbeddedWriter(thisShared);
            }
        }
    }

    bool argsNoNodeGui = args->getPropertyUnsafe<bool>(kCreateNodeArgsPropNoNodeGUI);


    // Make sure knobs initialization does not attempt to call knobChanged or trigger a render.
    _imp->effect->beginChanges();

    // For OpenFX this calls describe & describeInContext if needed and then creates parameters and clips
    _imp->effect->describePlugin();

    { // If the plug-in does not have any supported components or bitdepths, do not create any instance.
        std::bitset<4> outputComps = pluginPtr->getPropertyUnsafe<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents);
        int nSupportedDepth = pluginPtr->getPropertyDimension(kNatronPluginPropOutputSupportedBitDepths);
        if (nSupportedDepth == 0 || (!outputComps[0] && !outputComps[1] && !outputComps[2] && !outputComps[3])) {
            throw std::invalid_argument(tr("Could not create instance of plug-in %1 because it does not support any components or bitdepth in output").arg(QString::fromUtf8(pluginPtr->getPluginID().c_str())).toStdString());
        }
    }


    // For an ouptut node, create its render engine
    if (_imp->effect->isOutput()) {
        _imp->renderEngine = _imp->effect->createRenderEngine();
    }

    // Set the node name
    initNodeScriptName(serialization.get(), QString::fromUtf8(argFixedName.c_str()));

    // Initialize inputs from the plug-in descriptor
    {
        QMutexLocker k(&_imp->inputsMutex);
        std::vector<InputDescriptionPtr> inputs = pluginPtr->getInputsDescription();

        if (pluginPtr->getPropertyUnsafe<bool>(kNatronPluginPropHostMask)) {
            // If plug-in wants host masking, check if we need to add a mask input
            bool hasMask = false;
            for (std::size_t i = 0; i < inputs.size(); ++i) {
                if (inputs[i]->getPropertyUnsafe<bool>(kInputDescPropIsMask)) {
                    hasMask = true;
                    break;
                }
            }
            if (!hasMask) {
                // Check if an input is named "Mask"
                for (std::size_t i = 0; i < inputs.size(); ++i) {
                    if (inputs[i]->getPropertyUnsafe<std::string>(kInputDescPropLabel) == "Mask") {
                        hasMask = true;
                        break;
                    }
                }
            }
            if (!hasMask) {
                // No mask: add one
                InputDescriptionPtr maskInput = InputDescription::create("Mask", "Mask", "", true, true, std::bitset<4>(std::string("0001")));
                maskInput->setProperty(kInputDescPropSupportsTiles, true);
                maskInput->setProperty(kInputDescPropIsHostInput, true);
                inputs.push_back(maskInput);
            }
        }

        _imp->inputs.resize(inputs.size());
        _imp->inputIsRenderingCounter.resize(inputs.size());
        _imp->inputDescriptions.resize(inputs.size());
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            InputDescriptionPtr copy(new InputDescription(*inputs[i]));

            // Default initialize label and script-name properties
            _imp->inputDescriptions[i] = copy;

            std::string inputScriptName = copy->getPropertyUnsafe<std::string>(kInputDescPropScriptName);
            if (inputScriptName.empty()) {
                inputScriptName.append( 1, (char)(i + 65) );
                copy->setProperty(kInputDescPropScriptName, inputScriptName);
            }
            std::string inputLabel = copy->getPropertyUnsafe<std::string>(kInputDescPropLabel);
            if (inputLabel.empty()) {
                inputLabel = copy->getPropertyUnsafe<std::string>(kInputDescPropScriptName);
                copy->setProperty(kInputDescPropLabel, inputLabel);
            }

        }
    }

    // Create knobs
    _imp->effect->initializeKnobs(serialization.get() != 0, !argsNoNodeGui);

    // If this node is a group and we are in gui mode, create the node graph right now before creating any other
    // subnodes (in restoreNodeToDefaultState). This is so that the nodes get a proper position
    {
        NodeGroupPtr isGroupNode = toNodeGroup(_imp->effect);
        if (isGroupNode && isGroupNode->isSubGraphUserVisible() ) {
            getApp()->createGroupGui(thisShared, *args);
        }
    }

    // Restore the node to its default state including internal node graph and such for groups
    restoreNodeToDefaultState(args);

    // if we have initial values set for Knobs in the CreateNodeArgs object, deserialize them now
    setValuesFromSerialization(*args);

    // For OpenFX we create the image effect now
    _imp->effect->createInstanceAction_public();

    // Create overlays
    _imp->effect->initializeOverlayInteract();

    // Refresh metadata once so that parameters get a consistent default value
    _imp->effect->onMetadataChanged_nonRecursive_public();

    // Refresh dynamic props such as tiles support, OpenGL support, multi-thread etc...
    _imp->effect->refreshDynamicProperties();

    // If this plug-in create views (ReadOIIO only) then refresh them
    refreshCreatedViews(!serialization);

    // Notify the container group we added this node
    group->notifyNodeActivated(thisShared);

    // Create gui if needed. For groups this will also create the GUI of all internal nodes
    // unless they are not created yet
    if (!argsNoNodeGui && !getApp()->isBackground()) {
        createNodeGuiInternal(args);
    }

    // This node is now considered created
    _imp->nodeCreated = true;

    // Callback to the effect notifying everything is setup.
    // Generally used by Group derivatives class to initialize internal nodes
    // unless there is a serialization that was loaded before
    _imp->effect->onEffectCreated(*args);

    // Refresh page order so that serialization does not save it if it did not change
    _imp->refreshDefaultPagesOrder();

    // Refresh knobs Viewer UI order so that serialization does not save it if it did not change
    _imp->refreshDefaultViewerKnobsOrder();

    // Run the Python callback
    _imp->runOnNodeCreatedCB(!serialization);

    // If needed compute a preview for this node
    computePreviewImage();

    // Resume knobChanged calls
    _imp->effect->endChanges();

    // Check for invalid expressions which may become valid again
    getApp()->recheckInvalidLinks();
} // load


void
Node::setValuesFromSerialization(const CreateNodeArgs& args)
{

    std::vector<std::string> params = args.getPropertyNUnsafe<std::string>(kCreateNodeArgsPropNodeInitialParamValues);

    assert( QThread::currentThread() == qApp->thread() );
    const std::vector<KnobIPtr> & nodeKnobs = getKnobs();

    for (std::size_t i = 0; i < params.size(); ++i) {
        for (U32 j = 0; j < nodeKnobs.size(); ++j) {
            if (nodeKnobs[j]->getName() == params[i]) {

                KnobBoolBasePtr isBool = toKnobBoolBase(nodeKnobs[j]);
                KnobIntBasePtr isInt = toKnobIntBase(nodeKnobs[j]);
                KnobDoubleBasePtr isDbl = toKnobDoubleBase(nodeKnobs[j]);
                KnobChoicePtr isChoice = toKnobChoice(nodeKnobs[j]);
                KnobStringBasePtr isStr = toKnobStringBase(nodeKnobs[j]);
                int nDims = nodeKnobs[j]->getNDimensions();

                std::string propName = kCreateNodeArgsPropParamValue;
                propName += "_";
                propName += params[i];
                if (isChoice) {
                    // First check if a string is provided, otherwise check if an index is provided
                    try {
                        std::vector<std::string> v = args.getPropertyNUnsafe<std::string>(propName);
                        if (!v.empty()) {
                            isChoice->setActiveEntry(ChoiceOption(v[0], "", ""));
                        }

                    } catch (...) {
                        std::vector<int> v = args.getPropertyNUnsafe<int>(propName);
                        nDims = std::min((int)v.size(), nDims);
                        for (int d = 0; d < nDims; ++d) {
                            isInt->setValue(v[d]);
                        }
                    }

                } else if (isBool) {
                    std::vector<bool> v = args.getPropertyNUnsafe<bool>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isBool->setValue(v[d]);
                    }
                } else if (isInt) {
                    std::vector<int> v = args.getPropertyNUnsafe<int>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isInt->setValue(v[d]);
                    }
                } else if (isDbl) {
                    std::vector<double> v = args.getPropertyNUnsafe<double>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isDbl->setValue(v[d]);
                    }
                } else if (isStr) {
                    std::vector<std::string> v = args.getPropertyNUnsafe<std::string>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isStr->setValue(v[d]);
                    }
                }
                break;
            }
        }
    }


} // setValuesFromSerialization



void
Node::restoreUserKnob(const KnobGroupPtr& group,
                      const KnobPagePtr& page,
                      const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase,
                      bool setUserKnobsAsPluginKnobs,
                      unsigned int recursionLevel)
{

    const SERIALIZATION_NAMESPACE::KnobSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::KnobSerialization*>(&serializationBase);
    const SERIALIZATION_NAMESPACE::GroupKnobSerialization* groupSerialization = dynamic_cast<const SERIALIZATION_NAMESPACE::GroupKnobSerialization*>(&serializationBase);
    assert(serialization || groupSerialization);
    if (!serialization && !groupSerialization) {
        return;
    }

    if (groupSerialization) {
        KnobIPtr found = getKnobByName(groupSerialization->_name);

        bool isPage = false;
        bool isGroup = false;
        if (groupSerialization->_typeName == KnobPage::typeNameStatic()) {
            isPage = true;
        } else if (groupSerialization->_typeName == KnobGroup::typeNameStatic()) {
            isGroup = true;
        } else {
            if (recursionLevel == 0) {
                // Recursion level is 0, so we are a page since pages all knobs must live in a page.
                // We use it because in the past we didn't serialize the typename so we could not know if this was
                // a page or a group.
                isPage = true;
            } else {
                isGroup = true;
            }
        }
        assert(isPage != isGroup);
        if (isPage) {
            KnobPagePtr page;
            if (!found) {
                page = _imp->effect->createKnob<KnobPage>(groupSerialization->_name);
                page->setLabel(groupSerialization->_label);
            } else {
                page = toKnobPage(found);
            }
            if (!page) {
                return;
            }

            page->setKnobDeclarationType(setUserKnobsAsPluginKnobs ? KnobI::eKnobDeclarationTypePlugin : KnobI::eKnobDeclarationTypeUser);


            for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase> >::const_iterator it = groupSerialization->_children.begin(); it != groupSerialization->_children.end(); ++it) {
                restoreUserKnob(KnobGroupPtr(), page, **it, setUserKnobsAsPluginKnobs, recursionLevel + 1);
            }

        } else if (isGroup) { //!ispage
            KnobGroupPtr grp;
            if (!found) {
                grp = _imp->effect->createKnob<KnobGroup>(groupSerialization->_name);
                grp->setLabel(groupSerialization->_label);
            } else {
                grp = toKnobGroup(found);

            }
            if (!grp) {
                return;
            }
            grp->setKnobDeclarationType(setUserKnobsAsPluginKnobs ? KnobI::eKnobDeclarationTypePlugin : KnobI::eKnobDeclarationTypeUser);
            grp->setName(groupSerialization->_name);
            if (groupSerialization->_isSetAsTab) {
                grp->setAsTab();
            }
            assert(page);
            if (page) {
                page->addKnob(grp);
            }
            if (group) {
                group->addKnob(grp);
            }

            grp->setValue(groupSerialization->_isOpened);
            for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase> >::const_iterator it = groupSerialization->_children.begin(); it != groupSerialization->_children.end(); ++it) {
                restoreUserKnob(grp, page, **it, setUserKnobsAsPluginKnobs, recursionLevel + 1);
            }
        } // ispage

    } else {



        assert(serialization->_isUserKnob);
        if (!serialization->_isUserKnob) {
            return;
        }


        bool isFile = serialization->_typeName == KnobFile::typeNameStatic();
        bool isPath = serialization->_typeName == KnobPath::typeNameStatic();
        bool isString = serialization->_typeName == KnobString::typeNameStatic();
        bool isParametric = serialization->_typeName == KnobParametric::typeNameStatic();
        bool isChoice = serialization->_typeName == KnobChoice::typeNameStatic();
        bool isDouble = serialization->_typeName == KnobDouble::typeNameStatic();
        bool isColor = serialization->_typeName == KnobColor::typeNameStatic();
        bool isInt = serialization->_typeName == KnobInt::typeNameStatic();
        bool isBool = serialization->_typeName == KnobBool::typeNameStatic();
        bool isSeparator = serialization->_typeName == KnobSeparator::typeNameStatic();
        bool isButton = serialization->_typeName == KnobButton::typeNameStatic();

        assert(isInt || isDouble || isBool || isChoice || isColor || isString || isFile || isPath || isButton || isSeparator || isParametric);

        KnobIPtr knob;
        KnobIPtr found = getKnobByName(serialization->_scriptName);
        if (found) {
            knob = found;
        } else {
            if (isInt) {
                knob = _imp->effect->createKnob<KnobInt>(serialization->_scriptName, serialization->_dimension);
            } else if (isDouble) {
                knob = _imp->effect->createKnob<KnobDouble>(serialization->_scriptName, serialization->_dimension);
            } else if (isBool) {
                knob = _imp->effect->createKnob<KnobBool>(serialization->_scriptName);
            } else if (isChoice) {
                knob = _imp->effect->createKnob<KnobChoice>(serialization->_scriptName);
            } else if (isColor) {
                knob = _imp->effect->createKnob<KnobColor>(serialization->_scriptName, serialization->_dimension);
            } else if (isString) {
                knob = _imp->effect->createKnob<KnobString>(serialization->_scriptName, serialization->_dimension);
            } else if (isFile) {
                knob = _imp->effect->createKnob<KnobFile>(serialization->_scriptName);
            } else if (isPath) {
                knob = _imp->effect->createKnob<KnobPath>(serialization->_scriptName);
            } else if (isButton) {
                knob = _imp->effect->createKnob<KnobButton>(serialization->_scriptName);
            } else if (isSeparator) {
                knob = _imp->effect->createKnob<KnobSeparator>(serialization->_scriptName);
            } else if (isParametric) {
                knob = _imp->effect->createKnob<KnobParametric>(serialization->_scriptName, serialization->_dimension);
            }

        } // found


        assert(knob);
        if (!knob) {
            return;
        }
        knob->setLabel(serialization->_label);
        knob->fromSerialization(*serialization);
        knob->setKnobDeclarationType(setUserKnobsAsPluginKnobs ? KnobI::eKnobDeclarationTypePlugin : KnobI::eKnobDeclarationTypeUser);


        if (group) {
            group->addKnob(knob);
        } else if (page) {
            page->addKnob(knob);
        }
    } // groupSerialization

} // restoreUserKnob



void
Node::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase)
{

    SERIALIZATION_NAMESPACE::NodeSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::NodeSerialization*>(serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    // All this code is MT-safe as it runs in the serialization thread

    OfxEffectInstancePtr isOfxEffect = boost::dynamic_pointer_cast<OfxEffectInstance>(getEffectInstance());

    if (isOfxEffect) {
        // For OpenFX nodes, we call the sync private data action now to let a chance to the plug-in to synchronize it's
        // private data to parameters that will be saved with the project.
        isOfxEffect->syncPrivateData_other_thread();
    }


    // Check if pages ordering changed, if not do not serialize
    bool pageOrderChanged = true;
    if ((serialization->_encodeFlags & (SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPreset | SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug)) == 0) {
        pageOrderChanged = hasPageOrderChangedSinceDefault();
    } else if (serialization->_encodeFlags & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPreset) {
        // Never serialize page order in a Preset
        pageOrderChanged = false;
    }

    bool isFullSaveMode = appPTR->getCurrentSettings()->getIsFullRecoverySaveModeEnabled();

    // Always store the sub-graph when encoding as a PyPlug
    const bool subGraphEdited = serialization->_encodeFlags  & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug || isSubGraphEditedByUser();

    KnobPagePtr pyPlugPage = getEffectInstance()->getPyPlugPage();

    KnobsVec knobs = getEffectInstance()->getKnobs_mt_safe();
    std::list<KnobIPtr> userPages;
    for (std::size_t i  = 0; i < knobs.size(); ++i) {

        const KnobIPtr& knob = knobs[i];


        KnobGroupPtr isGroup = toKnobGroup(knob);
        KnobPagePtr isPage = toKnobPage(knob);

        // For pages, check if it is a user knob, if so serialialize user knobs recursively
        if (isPage) {
            // Don t save empty pages
            if (isPage->getChildren().empty()) {
                continue;
            }

            // Save pages order if it has changed or if we are encoding a PyPlug
            if (!isPage->getIsSecret() && (pageOrderChanged || serialization->_encodeFlags & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug)) {
                serialization->_pagesIndexes.push_back( knob->getName() );
            }

            // Save user pages if they were added by the user with respect to the initial plug-in state, or if we are encoding as a PyPlug
            if ((serialization->_encodeFlags & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPreset) == 0) {
                if ( knobs[i]->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser /*&& (!knob->isDeclaredByPlugin() || serialization->_encodeFlags & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug)*/) {
                    userPages.push_back(knob);
                }
            }
            continue;
        }

        // A knob might be non persistent but still have an expression, in which case we need to serialize it.
        bool hasExpr = false;
        {
            std::list<ViewIdx> views = knob->getViewsList();
            for (int d = 0; d < knob->getNDimensions(); ++d) {
                for (std::list<ViewIdx>::const_iterator itV = views.begin(); itV != views.end(); ++itV) {
                    if (!knob->getExpression(DimIdx(d), *itV).empty()) {
                        hasExpr = true;
                        break;
                    }
                    KnobDimViewKey linkData;
                    if (knob->getSharingMaster(DimIdx(d), *itV, &linkData)) {
                        hasExpr = true;
                        break;
                    }
                }
                if (hasExpr) {
                    break;
                }
            }
        }
        if (!knob->getIsPersistent() && !hasExpr) {
            // Don't serialize non persistant knobs
            continue;
        }

        if (knob->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser) {
            // Don't serialize user knobs, its taken care of by user pages
            continue;
        }

        if (isGroup || isPage) {
            // Don't serialize these, they don't hold anything
            continue;
        }

        if (!isFullSaveMode && !knob->hasModifications() && !knob->hasDefaultValueChanged() && !hasExpr) {
            // This knob was not modified by the user, don't serialize it
            continue;
        }

        // If the knob is in the PyPlug page, only serialize if the PyPlug page is visible or if we are exporting as a
        // Pyplug
        if (pyPlugPage && pyPlugPage->getIsSecret() && knob->getTopLevelPage() == pyPlugPage && (serialization->_encodeFlags & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug) == 0) {
            continue;
        }

        SERIALIZATION_NAMESPACE::KnobSerializationPtr newKnobSer = boost::make_shared<SERIALIZATION_NAMESPACE::KnobSerialization>();
        knob->toSerialization(newKnobSer.get());
        if (newKnobSer->_mustSerialize) {
            serialization->_knobsValues.push_back(newKnobSer);
        }

    }

    // Serialize user pages now
    for (std::list<KnobIPtr>::const_iterator it = userPages.begin(); it != userPages.end(); ++it) {
        boost::shared_ptr<SERIALIZATION_NAMESPACE::GroupKnobSerialization> s = boost::make_shared<SERIALIZATION_NAMESPACE::GroupKnobSerialization>();
        (*it)->toSerialization(s.get());
        serialization->_userPages.push_back(s);
    }


    serialization->_groupFullyQualifiedScriptName = getContainerGroupFullyQualifiedName();

    serialization->_nodeLabel = getLabel_mt_safe();

    serialization->_nodeScriptName = getScriptName_mt_safe();

    // When serializing as pyplug, always set in the plugin-id the original plug-in ID.
    if (serialization->_encodeFlags & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug) {
        serialization->_pluginID = getOriginalPlugin()->getPluginID();
    } else {
        serialization->_pluginID = getPluginID();
    }

    {
        QMutexLocker k(&_imp->nodePresetMutex);
        serialization->_presetInstanceLabel = _imp->initialNodePreset;
    }

    serialization->_pluginMajorVersion = getMajorVersion();

    serialization->_pluginMinorVersion = getMinorVersion();

    // Only serialize inputs for regular serialization
    if ((serialization->_encodeFlags & (SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPreset | SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug)) == 0) {
        getInputNames(serialization->_inputs, serialization->_masks);
    }

    if (serialization->_encodeFlags & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsSerializeOutputs) {
        OutputNodesMap outputs;
        getOutputs(outputs);
        for (OutputNodesMap::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            SERIALIZATION_NAMESPACE::OutputNodeConnection n;
            n.outputNodeScriptName = it->first->getScriptName_mt_safe();
            n.outputNodeIndices = it->second;
            serialization->_outputs.push_back(n);
        }
    }


    std::list<KnobItemsTablePtr> tables = _imp->effect->getAllItemsTables();
    if (!tables.empty()) {
        for (std::list<KnobItemsTablePtr>::const_iterator it = tables.begin(); it != tables.end(); ++it) {
            if ((*it)->getNumTopLevelItems() > 0) {
                SERIALIZATION_NAMESPACE::KnobItemsTableSerializationPtr tableSer(new SERIALIZATION_NAMESPACE::KnobItemsTableSerialization);
                (*it)->toSerialization(tableSer.get());
                serialization->_tables.push_back(tableSer);
            }
        }
    }

    // For groups, serialize its children if the graph was edited
    NodeGroupPtr isGrp = isEffectNodeGroup();
    if (isGrp && subGraphEdited) {
        NodesList nodes;
        isGrp->getActiveNodes(&nodes);

        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if ( (*it)->isPersistent() ) {

                SERIALIZATION_NAMESPACE::NodeSerializationPtr state;
                StubNodePtr isStub = toStubNode((*it)->getEffectInstance());
                if (isStub) {
                    state = isStub->getNodeSerialization();
                    if (!state) {
                        continue;
                    }
                } else {
                    state = boost::make_shared<SERIALIZATION_NAMESPACE::NodeSerialization>();
                    (*it)->toSerialization(state.get());
                }

                serialization->_children.push_back(state);
            }
        }
    }

    getPosition(&serialization->_nodePositionCoords[0], &serialization->_nodePositionCoords[1]);

    // Only save the size for backdrops, that's the only node where the user can resize
    if (isEffectBackdrop()) {
        getSize(&serialization->_nodeSize[0], &serialization->_nodeSize[1]);
    }

    if (hasColorChangedSinceDefault()) {
        getColor(&serialization->_nodeColor[0], &serialization->_nodeColor[1], &serialization->_nodeColor[2]);
    }
    getOverlayColor(&serialization->_overlayColor[0], &serialization->_overlayColor[1], &serialization->_overlayColor[2]);

    // Only serialize viewer UI knobs order if it has changed

    bool serializeViewerKnobs = (serialization->_encodeFlags & (SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPreset | SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug));
    KnobsVec viewerUIKnobs = getEffectInstance()->getViewerUIKnobs();
    if (!serializeViewerKnobs) {
        if (viewerUIKnobs.size() != _imp->defaultViewerKnobsOrder.size()) {
            std::list<std::string>::const_iterator it2 = _imp->defaultViewerKnobsOrder.begin();
            bool hasChanged = false;
            for (KnobsVec::iterator it = viewerUIKnobs.begin(); it!=viewerUIKnobs.end(); ++it, ++it2) {
                if ((*it)->getName() != *it2) {
                    hasChanged = true;
                    break;
                }
            }
            serializeViewerKnobs |= hasChanged;
        }
    }
    if (serializeViewerKnobs) {
        for (KnobsVec::iterator it = viewerUIKnobs.begin(); it!=viewerUIKnobs.end(); ++it) {
            serialization->_viewerUIKnobsOrder.push_back((*it)->getName());
        }
    }

} // Node::toSerialization

void
Node::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase)
{
    const SERIALIZATION_NAMESPACE::NodeSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::NodeSerialization*>(&serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    // Load all knobs as well as user knobs and roto/tracking data
    loadKnobsFromSerialization(*serialization, false /*setUserKnobsAsPluginKnobs*/);

    // Remember the UI
    {
        QMutexLocker k(&_imp->nodeUIDataMutex);
        _imp->nodePositionCoords[0] = serialization->_nodePositionCoords[0];
        _imp->nodePositionCoords[1] = serialization->_nodePositionCoords[1];
        _imp->nodeSize[0] = serialization->_nodeSize[0];
        _imp->nodeSize[1] = serialization->_nodeSize[1];
        _imp->nodeColor[0] = serialization->_nodeColor[0];
        _imp->nodeColor[1] = serialization->_nodeColor[1];
        _imp->nodeColor[2] = serialization->_nodeColor[2];
        _imp->overlayColor[0] = serialization->_overlayColor[0];
        _imp->overlayColor[1] = serialization->_overlayColor[1];
        _imp->overlayColor[2] = serialization->_overlayColor[2];
    }

    // If we have outputs to restore, do it now
    if (serialization->_encodeFlags & SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsSerializeOutputs) {
        NodePtr thisShared = shared_from_this();
        for (std::list<SERIALIZATION_NAMESPACE::OutputNodeConnection>::const_iterator it = serialization->_outputs.begin(); it != serialization->_outputs.end(); ++it) {
            NodePtr foundNode = getGroup()->getNodeByName(it->outputNodeScriptName);
            if (!foundNode) {
                continue;
            }
            for (std::list<int>::const_iterator it2 = it->outputNodeIndices.begin(); it2 != it->outputNodeIndices.end(); ++it2) {
                foundNode->swapInput(thisShared, *it2);
            }
        }
    }

} // fromSerialization

void
Node::loadInternalNodeGraph(bool initialSetupAllowed,
                            const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                            const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization)
{
    NodeGroupPtr isGrp = isEffectNodeGroup();
    if (!isGrp) {
        return;
    }

    // Only do this when loading the node the first time
    assert(!isNodeCreated());

    {
        PluginPtr pyPlug = _imp->pyPlugHandle.lock();
        // For old PyPlugs based on Python scripts, the nodes are created by the Python script after the Group itself
        // gets created. So don't do anything
        bool isPythonScriptPyPlug = pyPlug && pyPlug->getPropertyUnsafe<bool>(kNatronPluginPropPyPlugIsPythonScript);
        if (isPythonScriptPyPlug) {
            return;
        }
    }

    // PyPlug serialization is only for pyplugs
    assert(!_imp->isPyPlug || pyPlugSerialization);


    // If we are creating the node in the standard way or loading a project and the internal node graph was not edited, initialize the sub-graph.
    // For a standard Group it creates the Input and Output nodes.
    if ((!projectSerialization || projectSerialization->_children.empty()) && !_imp->isPyPlug && initialSetupAllowed) {
        isGrp->setupInitialSubGraphState();
    }


    // Call the nodegroup derivative that is the only one to know what to do
    isGrp->loadSubGraph(projectSerialization, pyPlugSerialization);


} // loadInternalNodeGraph

static void checkForOldStringParametersForChoices(const SERIALIZATION_NAMESPACE::ProjectBeingLoadedInfo& projectInfos, const KnobsVec& knobs, const SERIALIZATION_NAMESPACE::KnobSerializationList& knobValues)
{


    // Before Natron 2.2.3, all dynamic choice parameters for multiplane had a string parameter.
    // The string parameter had the same name as the choice parameter plus "Choice" appended.
    // If we found such a parameter, retrieve the string from it.
    if (projectInfos.vMajor < 2 || projectInfos.vMajor >= 3 ||  projectInfos.vMinor >= 3) {
        return;
    }

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobChoicePtr isChoice = toKnobChoice(*it);
        if (!isChoice) {
            continue;
        }


        std::string stringParamName = isChoice->getName() + "Choice";
        for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = knobValues.begin(); it != knobValues.end(); ++it) {
            if ( (*it)->getName() == stringParamName && (*it)->_dataType == SERIALIZATION_NAMESPACE::eSerializationValueVariantTypeString) {

                const SERIALIZATION_NAMESPACE::KnobSerialization::PerDimensionValueSerializationVec& perDimValues = (*it)->_values["Main"];
                if (perDimValues.size() > 0) {
                    isChoice->setActiveEntry(ChoiceOption(perDimValues[0]._value.isString, "",""));
                }

                break;
            }
        }
    }
} // checkForOldStringParametersForChoices

void
Node::loadKnobsFromSerialization(const SERIALIZATION_NAMESPACE::NodeSerialization& serialization, bool setUserKnobsAsPluginKnobs)
{

    _imp->effect->beginChanges();
    _imp->effect->onKnobsAboutToBeLoaded(serialization);


    // Load the deprecated user planes. It is now a Knob so we don't need custom code
    for (std::list<SERIALIZATION_NAMESPACE::ImagePlaneDescSerialization>::const_iterator it = serialization._userComponents.begin(); it!=serialization._userComponents.end(); ++it) {
        ImagePlaneDesc s;
        s.fromSerialization(*it);
        _imp->effect->addUserComponents(s);
    }

    boost::scoped_ptr<SERIALIZATION_NAMESPACE::ProjectBeingLoadedInfo> projectInfos;
    {
        projectInfos.reset(new SERIALIZATION_NAMESPACE::ProjectBeingLoadedInfo);
        bool gotProjectInfos = getApp()->getProject()->getProjectLoadedVersionInfo(projectInfos.get());
        if (!gotProjectInfos) {
            projectInfos.reset();
        }
    }


    {
        // Load all knobs
        if (projectInfos) {
            checkForOldStringParametersForChoices(*projectInfos, getKnobs(), serialization._knobsValues);
        }

        for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = serialization._knobsValues.begin(); it!=serialization._knobsValues.end(); ++it) {

            std::string knobName = (*it)->_scriptName;
            filterKnobNameCompat(serialization._pluginID, serialization._pluginMajorVersion, serialization._pluginMinorVersion, projectInfos ? projectInfos->vMajor : -1, projectInfos ? projectInfos->vMinor : -1, projectInfos ? projectInfos->vRev : -1, &knobName);
            KnobIPtr knob = getKnobByName(knobName);
            if (!knob) {
                continue;
            }
            knob->fromSerialization(**it);

        }


    }

    KnobIPtr filenameParam = getKnobByName(kOfxImageEffectFileParamName);
    if (filenameParam) {
        getEffectInstance()->computeFrameRangeForReader(filenameParam, false);
    }

    // now restore the roto context if the node has a roto context
    std::list<KnobItemsTablePtr> tables = _imp->effect->getAllItemsTables();
    for (std::list<SERIALIZATION_NAMESPACE::KnobItemsTableSerializationPtr>::const_iterator it = serialization._tables.begin(); it != serialization._tables.end(); ++it) {
        KnobItemsTablePtr table = _imp->effect->getItemsTable((*it)->tableIdentifier);
        if (!table) {
            continue;
        }
        table->fromSerialization(**it);
        table->declareItemsToPython();
    }
    {
        for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::GroupKnobSerialization> >::const_iterator it = serialization._userPages.begin(); it != serialization._userPages.end(); ++it) {
            restoreUserKnob(KnobGroupPtr(), KnobPagePtr(), **it, 0, setUserKnobsAsPluginKnobs);
        }
    }

    declarePythonKnobs();

    if (!serialization._pagesIndexes.empty()) {
        setPagesOrder( serialization._pagesIndexes );
    }

    if (!serialization._viewerUIKnobsOrder.empty()) {
        KnobsVec viewerUIknobs;
        for (std::list<std::string>::const_iterator it = serialization._viewerUIKnobsOrder.begin(); it!=serialization._viewerUIKnobsOrder.end(); ++it) {
            KnobIPtr knob = getKnobByName(*it);
            if (knob) {
                viewerUIknobs.push_back(knob);
            }
        }
        _imp->effect->setViewerUIKnobs(viewerUIknobs);
    }

    // Force update of user knobs on the GUI if we are calling this in restoreNodeDefaults
    _imp->effect->recreateUserKnobs(false);

    _imp->effect->onKnobsLoaded();
    _imp->effect->endChanges();

} // loadKnobsFromSerialization

void
Node::clearPresetFlag()
{
    bool isEmpty;
    {
        QMutexLocker k(&_imp->nodePresetMutex);
        isEmpty = _imp->initialNodePreset.empty();
        _imp->initialNodePreset.clear();
    }
    if (!isEmpty) {
        Q_EMIT nodePresetsChanged();
    }
}

void
Node::loadPresets(const std::string& presetsLabel)
{

    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker k(&_imp->nodePresetMutex);
        _imp->initialNodePreset = presetsLabel;
    }
    restoreNodeToDefaultState(CreateNodeArgsPtr());
    Q_EMIT nodePresetsChanged();
}

void
Node::loadPresetsFromFile(const std::string& presetsFile)
{

    assert(QThread::currentThread() == qApp->thread());

    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization(new SERIALIZATION_NAMESPACE::NodeSerialization);

    // Throws on failure
    getNodeSerializationFromPresetFile(presetsFile, serialization.get());

    {
        QMutexLocker k(&_imp->nodePresetMutex);
        _imp->initialNodePreset = serialization->_presetInstanceLabel;
    }
    restoreNodeToDefaultState(CreateNodeArgsPtr());
    Q_EMIT nodePresetsChanged();
}

void
Node::getNodeSerializationFromPresetFile(const std::string& presetFile, SERIALIZATION_NAMESPACE::NodeSerialization* serialization)
{
    assert(serialization);
    FStreamsSupport::ifstream ifile;
    FStreamsSupport::open(&ifile, presetFile);
    if (!ifile || presetFile.empty()) {
        std::string message = tr("Failed to open file: ").toStdString() + presetFile;
        throw std::runtime_error(message);
    }

    try {
        SERIALIZATION_NAMESPACE::read(NATRON_PRESETS_FILE_HEADER, ifile,serialization);
    } catch (SERIALIZATION_NAMESPACE::InvalidSerializationFileException& e) {
        throw std::runtime_error(tr("Failed to open %1: this file does not appear to be a presets file").arg(QString::fromUtf8(presetFile.c_str())).toStdString());
    }
}



void
Node::getNodeSerializationFromPresetName(const std::string& presetName, SERIALIZATION_NAMESPACE::NodeSerialization* serialization)
{
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        throw std::invalid_argument("Invalid plug-in");
    }

    const std::vector<PluginPresetDescriptor>& presets = plugin->getPresetFiles();
    for (std::vector<PluginPresetDescriptor>::const_iterator it = presets.begin() ;it!=presets.end(); ++it) {
        if (it->presetLabel.toStdString() == presetName) {
            getNodeSerializationFromPresetFile(it->presetFilePath.toStdString(), serialization);
            assert(presetName == serialization->_presetsIdentifierLabel);
            return;
        }
    }


    std::string message = tr("Cannot find loaded preset named %1").arg(QString::fromUtf8(presetName.c_str())).toStdString();
    throw std::invalid_argument(message);
}

void
Node::loadPresetsInternal(const SERIALIZATION_NAMESPACE::NodeSerializationPtr& serialization,bool setKnobsDefault, bool setUserKnobsAsPluginKnobs)
{
    assert(QThread::currentThread() == qApp->thread());


    loadKnobsFromSerialization(*serialization, setUserKnobsAsPluginKnobs);

    if (setKnobsDefault) {
        // set non animated knobs to be their default values
        const KnobsVec& knobs = getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            KnobButtonPtr isBtn = toKnobButton(*it);
            KnobPagePtr isPage = toKnobPage(*it);
            KnobSeparatorPtr isSeparator = toKnobSeparator(*it);
            if ( (isBtn && !isBtn->getIsCheckable())  || isPage || isSeparator) {
                continue;
            }

            if ((*it)->getIsPersistent()) {
                KnobIntBasePtr isInt = toKnobIntBase(*it);
                KnobBoolBasePtr isBool = toKnobBoolBase(*it);
                KnobStringBasePtr isString = toKnobStringBase(*it);
                KnobDoubleBasePtr isDouble = toKnobDoubleBase(*it);
                if ((*it)->hasAnimation()) {
                    continue;
                }
                for (int d = 0; d < (*it)->getNDimensions(); ++d) {
                    if (isInt) {
                        isInt->setDefaultValue(isInt->getValue(DimIdx(d)), DimIdx(d));
                    } else if (isBool) {
                        isBool->setDefaultValue(isBool->getValue(DimIdx(d)), DimIdx(d));
                    } else if (isString) {
                        isString->setDefaultValue(isString->getValue(DimIdx(d)), DimIdx(d));
                    } else if (isDouble) {
                        isDouble->setDefaultValue(isDouble->getValue(DimIdx(d)), DimIdx(d));
                    }
                }
            }
        }
    }

} // Node::loadPresetsInternal

void
Node::exportNodeToPyPlug(const std::string& filePath)
{
    // Only groups can export to PyPlug
    if (!isEffectNodeGroup()) {
        return;
    }
    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open(&ofile, filePath);
    if (!ofile || filePath.empty()) {
        std::string message = tr("Failed to open file: ").toStdString() + filePath;
        throw std::runtime_error(message);
    }

    // Perform checks before writing the file
    {
        std::string pyPlugID = _imp->effect->getPyPlugIDKnob()->getValue();
        if (pyPlugID.empty()) {
            std::string message = tr("The plug-in ID cannot be empty").toStdString();
            throw std::runtime_error(message);
        }
    }
    {
        std::string pyPlugLabel = _imp->effect->getPyPlugLabelKnob()->getValue();
        if (pyPlugLabel.empty()) {
            std::string message = tr("The plug-in label cannot be empty").toStdString();
            throw std::runtime_error(message);
        }
    }

    // Make sure the file paths are relative to the pyplug script directory
    std::string pyPlugDirectoryPath;
    {
        std::size_t foundSlash = filePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            pyPlugDirectoryPath = filePath.substr(0, foundSlash + 1);
        }
    }

    {
        std::string iconFilePath = _imp->effect->getPyPlugIconKnob()->getValue();
        std::string path;
        std::size_t foundSlash = iconFilePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            path = iconFilePath.substr(0, foundSlash + 1);
        }
        if (!path.empty() && path != pyPlugDirectoryPath) {
            std::string message = tr("The plug-in icon file should be located in the same directory as the PyPlug script (%1)").arg(QString::fromUtf8(pyPlugDirectoryPath.c_str())).toStdString();
            throw std::runtime_error(message);
        }
    }
    {
        std::string callbacksFilePath = _imp->effect->getPyPlugExtScriptKnob()->getValue();
        std::string path;
        std::size_t foundSlash = callbacksFilePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            path = callbacksFilePath.substr(0, foundSlash + 1);
        }
        if (!path.empty() && path != pyPlugDirectoryPath) {
            std::string message = tr("The Python callbacks file should be located in the same directory as the PyPlug script (%1)").arg(QString::fromUtf8(pyPlugDirectoryPath.c_str())).toStdString();
            throw std::runtime_error(message);
        }
    }


    // Check that the directory where the file will be is registered in Natron search paths.
    if (!getApp()->isBackground()) {
        bool foundInPath = false;
        QStringList groupSearchPath = appPTR->getAllNonOFXPluginsPaths();
        for (QStringList::iterator it = groupSearchPath.begin(); it != groupSearchPath.end(); ++it) {
            std::string thisPath = it->toStdString();

            // pyPlugDirectoryPath ends with a separator, so ensure this one has one too
            if (!thisPath.empty() && thisPath[thisPath.size() - 1] != '/') {
                thisPath.push_back('/');
            }
            if (thisPath == pyPlugDirectoryPath) {
                foundInPath = true;
                break;
            }
        }

        if (!foundInPath) {
            QString message = tr("Directory \"%1\" is not in the plug-in search path, would you like to add it?").arg(QString::fromUtf8(pyPlugDirectoryPath.c_str()));
            StandardButtonEnum rep = Dialogs::questionDialog(tr("Plug-in path").toStdString(),
                                                             message.toStdString(), false);

            if  (rep == eStandardButtonYes) {
                appPTR->getCurrentSettings()->appendPythonGroupsPath(pyPlugDirectoryPath);
            }
        }
    }


    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization(new SERIALIZATION_NAMESPACE::NodeSerialization);
    serialization->_encodeFlags = SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPyPlug;
    toSerialization(serialization.get());

    SERIALIZATION_NAMESPACE::NodeClipBoard cb;
    cb.nodes.push_back(serialization);

    SERIALIZATION_NAMESPACE::write(ofile, cb, NATRON_PRESETS_FILE_HEADER);
} // exportNodeToPyPlug

void
Node::exportNodeToPresets(const std::string& filePath,
                          const std::string& presetsLabel,
                          const std::string& iconFilePath,
                          int shortcutSymbol,
                          int shortcutModifiers)
{
    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open(&ofile, filePath);
    if (!ofile || filePath.empty()) {
        std::string message = tr("Failed to open file: ").toStdString() + filePath;
        throw std::runtime_error(message);
    }

    // Make sure the file paths are relative to the presets script directory
    std::string pyPlugDirectoryPath;
    {
        std::size_t foundSlash = filePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            pyPlugDirectoryPath = filePath.substr(0, foundSlash + 1);
        }
    }
    {
        std::string path;
        std::size_t foundSlash = iconFilePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            path = iconFilePath.substr(0, foundSlash + 1);
        }
        if (!path.empty() && path != pyPlugDirectoryPath) {
            std::string message = tr("The preset icon file should be located in the same directory as the preset script (%1)").arg(QString::fromUtf8(pyPlugDirectoryPath.c_str())).toStdString();
            throw std::runtime_error(message);
        }
    }


    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization(new SERIALIZATION_NAMESPACE::NodeSerialization);
    serialization->_encodeFlags = SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsPreset;
    serialization->_presetsIdentifierLabel = presetsLabel;
    serialization->_presetsIconFilePath = iconFilePath;
    serialization->_presetShortcutSymbol = shortcutSymbol;
    serialization->_presetShortcutPresetModifiers = shortcutModifiers;

    toSerialization(serialization.get());

    SERIALIZATION_NAMESPACE::NodeClipBoard cb;
    cb.nodes.push_back(serialization);

    SERIALIZATION_NAMESPACE::write(ofile, cb, NATRON_PRESETS_FILE_HEADER);
} // exportNodeToPresets



void
Node::restoreNodeToDefaultState(const CreateNodeArgsPtr& args)
{
    assert(QThread::currentThread() == qApp->thread());

    FlagSetter setter(true, &_imp->restoringDefaults);

    // Make sure the instance does not receive knobChanged now
    ScopedChanges_RAII changes(_imp->effect.get());

    // If the node is not yet created (i.e: this is called in the load() function) then some stuff here doesn't need to be done
    bool nodeCreated = isNodeCreated();
    if (nodeCreated) {
        // Purge any cache when reseting to defaults
        _imp->effect->purgeCaches_public();
    }

    // Check if there is any serialization from presets/pyplug
    std::string nodePreset = getCurrentNodePresets();
    SERIALIZATION_NAMESPACE::NodeSerializationPtr presetSerialization, pyPlugSerialization, projectSerialization;
    if (args) {
        projectSerialization = args->getPropertyUnsafe<SERIALIZATION_NAMESPACE::NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    }
    if (!nodePreset.empty()) {
        try {
            presetSerialization.reset(new SERIALIZATION_NAMESPACE::NodeSerialization);
            getNodeSerializationFromPresetName(nodePreset, presetSerialization.get());
        } catch (...) {

        }
    }

    if (_imp->isPyPlug) {
        PluginPtr pyPlugHandle = _imp->pyPlugHandle.lock();
        if (pyPlugHandle) {
            bool isPythonScriptPyPlug = pyPlugHandle->getPropertyUnsafe<bool>(kNatronPluginPropPyPlugIsPythonScript);
            if (!isPythonScriptPyPlug) {
                std::string filePath = pyPlugHandle->getPropertyUnsafe<std::string>(kNatronPluginPropPyPlugScriptAbsoluteFilePath);
                pyPlugSerialization.reset(new SERIALIZATION_NAMESPACE::NodeSerialization);
                getNodeSerializationFromPresetFile(filePath, pyPlugSerialization.get());
            }
        }
    }
    // Reset all knobs to default first, block value changes and do them all afterwards because the node state can only be restored
    // if all parameters are actually to the good value
    if (nodeCreated) {


        // Restore knob defaults
        const KnobsVec& knobs = getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            if (!(*it)->getIsPersistent() ) {
                continue;
            }
            (*it)->blockValueChanges();
            (*it)->unSplitAllViews();
            (*it)->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
            (*it)->unblockValueChanges();
        }
    }


    // If this is a pyplug, load the node state (and its internal subgraph)
    if (pyPlugSerialization) {
        loadPresetsInternal(pyPlugSerialization, false, true /*setUserKnobsAsPluginKnobs*/);
    }

    if (presetSerialization) {

        // Load presets from serialization if any
        loadPresetsInternal(presetSerialization, true, true /*setUserKnobsAsPluginKnobs*/);
    } else {

        // Reset knob default values to their initial default value if we had a different preset before
        if (nodeCreated) {
            const KnobsVec& knobs = getKnobs();
            for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
                KnobButtonPtr isBtn = toKnobButton(*it);
                KnobPagePtr isPage = toKnobPage(*it);
                KnobSeparatorPtr isSeparator = toKnobSeparator(*it);
                if ( (isBtn && !isBtn->getIsCheckable())  || isPage || isSeparator) {
                    continue;
                }
                if ((*it)->hasAnimation()) {
                    continue;
                }

                if ((*it)->getIsPersistent()) {
                    KnobIntBasePtr isInt = toKnobIntBase(*it);
                    KnobBoolBasePtr isBool = toKnobBoolBase(*it);
                    KnobStringBasePtr isString = toKnobStringBase(*it);
                    KnobDoubleBasePtr isDouble = toKnobDoubleBase(*it);
                    for (int d = 0; d < (*it)->getNDimensions(); ++d) {
                        if (isInt) {
                            isInt->setDefaultValue(isInt->getInitialDefaultValue(DimIdx(d)), DimIdx(d));
                        } else if (isBool) {
                            isBool->setDefaultValue(isBool->getInitialDefaultValue(DimIdx(d)), DimIdx(d));
                        } else if (isString) {
                            isString->setDefaultValue(isString->getInitialDefaultValue(DimIdx(d)), DimIdx(d));
                        } else if (isDouble) {
                            isDouble->setDefaultValue(isDouble->getInitialDefaultValue(DimIdx(d)), DimIdx(d));
                        }
                    }
                }
            }
        }
    }

    // Load serialization before loading internal nodegraph as restoring parameters of the sub-nodegraph could reference user knobs
    // on this node
    if (projectSerialization) {
        fromSerialization(*projectSerialization);
    }

    if (!nodeCreated) {
        bool initialSubGraphSetupAllowed = false;
        if (args) {
            initialSubGraphSetupAllowed = !args->getPropertyUnsafe<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes);
        }

        loadInternalNodeGraph(initialSubGraphSetupAllowed, projectSerialization.get(), pyPlugSerialization.get());
    }


    // If there was a serialization, we most likely removed or created user parameters, so refresh Python knobs
    declarePythonKnobs();

    if (nodeCreated) {
        // Ensure the state of the node is consistent with what the plug-in expects
        TimeValue time(getApp()->getTimeLine()->currentFrame());
        {
            const KnobsVec& knobs = getKnobs();
            for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
                if (!(*it)->getEvaluateOnChange()) {
                    continue;
                }
                // Don't call instanceChanged action on buttons otherwise it could popup a menu for some plug-ins
                KnobButtonPtr isButton = toKnobButton(*it);
                if (isButton) {
                    continue;
                }
                _imp->effect->onKnobValueChanged_public(*it, eValueChangedReasonRestoreDefault, time, ViewIdx(0));

            }
        }
    }

    // Refresh hash & meta-data and trigger a render
    _imp->effect->invalidateCacheHashAndEvaluate(true, true);

} // restoreNodeToDefaultState


void
Node::restoreKnobsLinks(const SERIALIZATION_NAMESPACE::NodeSerialization & serialization,
                        const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& allCreatedNodesInGroup)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    // In Natron 2.1.x and older we serialized the name of the master node
    const std::string & masterNodeName = serialization._masterNodecriptName;
    if ( !masterNodeName.empty() ) {

        // In the past the script-name contained the fully qualified name , e.g: Group1.Blur1
        // This leads to issues when restoring the graph in another Group name.
        // Ensure the name is only the script-name by removing the prefix
        NodePtr masterNode;
        std::size_t foundDot = masterNodeName.find_last_of(".");
        if (foundDot != std::string::npos) {
            masterNode = getGroup()->getNodeByName(masterNodeName.substr(foundDot + 1));
        } else {
            masterNode = getGroup()->getNodeByName(masterNodeName);
        }



        if (!masterNode) {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }

            appPTR->writeToErrorLog_mt_safe( QString::fromUtf8(getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(),
                                            tr("Cannot restore the link between %1 and %2.")
                                            .arg( QString::fromUtf8( serialization._nodeScriptName.c_str() ) )
                                            .arg( QString::fromUtf8( masterNodeName.c_str() ) ) );
        } else {
            linkToNode( masterNode );
        }
        return;
    }


    const SERIALIZATION_NAMESPACE::KnobSerializationList & knobsValues = serialization._knobsValues;
    ///try to find a serialized value for this knob
    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        KnobIPtr knob = getKnobByName((*it)->_scriptName);
        if (!knob) {
            continue;
        }
        try {
            knob->restoreKnobLinks(*it, allCreatedNodesInGroup);
        } catch (const std::exception& e) {
            // For stub nodes don't report errors
            if (!isEffectStubNode()) {
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), QString::fromUtf8(e.what()), false, c);
            }
        }
    }

    const std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::GroupKnobSerialization> >& userKnobs = serialization._userPages;
    for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::GroupKnobSerialization > >::const_iterator it = userKnobs.begin(); it != userKnobs.end(); ++it) {
        KnobIPtr knob = getKnobByName((*it)->_name);
        if (!knob) {
            continue;
        }
        try {
            knob->restoreKnobLinks(*it, allCreatedNodesInGroup);
        } catch (const std::exception& e) {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }
            appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), QString::fromUtf8(e.what()), false, c);

        }
    }
} // restoreKnobsLinks

void
Node::moveToGroup(const NodeCollectionPtr& group)
{
    NodeCollectionPtr currentGroup = getGroup();
    assert(currentGroup);
    assert(group);
    if (currentGroup == group) {
        return;
    }


    bool settingsPanelVisible = isSettingsPanelVisible();

    // Destroy the node gui
    {
        NodeGuiIPtr oldNodeGui = getNodeGui();
        if (oldNodeGui) {
            oldNodeGui->destroyGui();
        }
        _imp->guiPointer.reset();
    }

    // Remove this node from the group
    // Hold a shared_ptr to the node to ensure one is still valid and the node does not get destroyed
    NodePtr thisShared = shared_from_this();
    currentGroup->removeNode(thisShared);



    // Remove the old Python attribute
    {
        std::string currentFullName = getFullyQualifiedName();
        deleteNodeVariableToPython(currentFullName);
    }
    std::string currentScriptName = getScriptName_mt_safe();

    {
        QMutexLocker k(&_imp->groupMutex);
        _imp->group = group;
        group->addNode(thisShared);
    }


    // Refresh the script-name, this will automatically re-declare the attribute to Python
    setScriptName(currentScriptName);

    // Create the new node gui
    NodeGraphI* newGraph = group->getNodeGraph();
    if (newGraph) {
        double position[2];
        getPosition(&position[0], &position[1]);
        CreateNodeArgsPtr args(CreateNodeArgs::create(getPluginID(), group));
        args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, settingsPanelVisible);
        args->setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, position[0], 0);
        args->setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, position[1], 1);

        newGraph->createNodeGui(shared_from_this(), *args);
    }

} // moveToGroup



NATRON_NAMESPACE_ANONYMOUS_ENTER

class NodeDestroyNodeInternalArgs
: public GenericWatcherCallerArgs
{
public:

    bool autoReconnect;

    NodeDestroyNodeInternalArgs()
    : GenericWatcherCallerArgs()
    , autoReconnect(false)
    {}

    virtual ~NodeDestroyNodeInternalArgs() {}
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


void
Node::invalidateExpressionLinks()
{
    NodeCollectionPtr parentCol = getGroup();
    // For all knobs that have listeners, invalidate expressions
    NodeGroupPtr isParentGroup = toNodeGroup(parentCol);

    KnobDimViewKeySet globalListenersSet;
    KnobsVec knobs = _imp->effect->getKnobs_mt_safe();
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobDimViewKeySet listeners;
        knobs[i]->getListeners(listeners, KnobI::eListenersTypeExpression);
        globalListenersSet.insert(listeners.begin(), listeners.end());
    }
    for (KnobDimViewKeySet::iterator it = globalListenersSet.begin(); it != globalListenersSet.end(); ++it) {
        KnobIPtr listener = it->knob.lock();
        if (!listener) {
            continue;
        }
        KnobHolderPtr holder = listener->getHolder();
        if (!holder) {
            continue;
        }
        if ( ( holder == _imp->effect ) || (holder == isParentGroup) ) {
            continue;
        }

        EffectInstancePtr isEffect = toEffectInstance(holder);
        if (!isEffect) {
            continue;
        }

        NodePtr effectNode = isEffect->getNode();
        if (!effectNode) {
            continue;
        }
        NodeCollectionPtr effectParent = effectNode->getGroup();
        if (!effectParent) {
            continue;
        }
        NodeGroupPtr isEffectParentGroup = toNodeGroup(effectParent);
        if ( isEffectParentGroup && (isEffectParentGroup == _imp->effect) ) {
            continue;
        }
        std::string hasExpr = listener->getExpression(it->dimension, it->view);
        if ( !hasExpr.empty() ) {
            std::stringstream ss;
            ss << tr("Missing node ").toStdString();
            ss << getFullyQualifiedName();
            ss << ' ';
            ss << tr("in expression.").toStdString();
            listener->setLinkStatus( it->dimension, it->view, false, ss.str() );
        }
    }

} //invalidateExpressionLinks

bool
Node::isBeingDestroyed() const
{
    QMutexLocker k(&_imp->isBeingDestroyedMutex);
    return _imp->isBeingDestroyed;
}

void
Node::destroyNode()

{
    if (!_imp->effect) {
        // Load has never been called or destroyed is already called
        return;
    }

    // Flag that the node is going to be destroyed so that we do not attempt to further use it during a render
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        _imp->isBeingDestroyed = true;
    }

    quitAnyProcessing_blocking(false);

    // Clear any persistent message linked to this node
    clearAllPersistentMessages(false /*recurse*/);

    // Invalidate expression of any external knob referencing knobs of this node
    invalidateExpressionLinks();

    // If the effect was doing OpenGL rendering and had context(s) bound, dettach them.
    _imp->effect->dettachAllOpenGLContexts();


    // Free all memory used by the plug-in.
    _imp->effect->clearLastRenderedImage();


    // Run on node deleted Python callback
    AppInstancePtr app = getApp();
    if ( app && !app->getProject()->isProjectClosing() ) {
        _imp->runOnNodeDeleteCB();
    }


    // Destroy node gui
    {
        NodeGuiIPtr guiPtr = _imp->guiPointer.lock();
        if (guiPtr) {
            guiPtr->destroyGui();
        }
    }

    // If its a group, clear its nodes
    NodeGroupPtr isGrp = isEffectNodeGroup();
    if (isGrp) {
        isGrp->clearNodesBlocking();
    }


    // Quit any rendering issued by this node
    {
        if (_imp->renderEngine) {
            _imp->renderEngine->quitEngine(false);
        }
    }


    // Remove the Python node
    deleteNodeVariableToPython( getFullyQualifiedName() );

    NodePtr thisShared = shared_from_this();

    // If inside the group, remove it from the group
    // the use_count() after the call to removeNode should be 2 and should be the shared_ptr held by the caller and the
    // thisShared ptr

    NodeCollectionPtr thisGroup = getGroup();
    if ( thisGroup ) {
        thisGroup->removeNode(thisShared);
    }

    // Removing this node might invalidate some expressions, check it now
    if (app) {
        app->recheckInvalidLinks();
    }


    _imp->effect.reset();


} // destroyNode


bool
Node::isSupportedComponent(int inputNb, const ImagePlaneDesc& comp) const
{
    assert(comp.getNumComponents() <= 4);
    std::bitset<4> supported = getSupportedComponents(inputNb);
    return supported[comp.getNumComponents()];
}

std::list<ImageBitDepthEnum>
Node::getSupportedBitDepths() const
{
    // For a Read or Write node, actually return the bits from the embedded plug-in
    ReadNodePtr isRead = toReadNode(_imp->effect);
    WriteNodePtr isWrite = toWriteNode(_imp->effect);
    NodePtr embeddedNode;
    if (isRead) {
        embeddedNode = isRead->getEmbeddedReader();
    } else if (isWrite) {
        embeddedNode = isWrite->getEmbeddedWriter();
    }

    if (embeddedNode) {
        return embeddedNode->getSupportedBitDepths();
    }

    PluginPtr plugin = getPlugin();
    int nSupported = plugin->getPropertyDimension(kNatronPluginPropOutputSupportedBitDepths);
    std::list<ImageBitDepthEnum> ret;

    for (int i = 0; i < nSupported; ++i) {
        ImageBitDepthEnum thisDepth = plugin->getPropertyUnsafe<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, i);
        ret.push_back(thisDepth);
    }
    return ret;
}

int
Node::findClosestSupportedNumberOfComponents(int inputNb,
                                                       int nComps) const
{
    if (nComps < 0 || nComps > 4) {
        // Natron assumes that a layer must have between 1 and 4 channels.
        return 0;
    }
    std::bitset<4> supported = getSupportedComponents(inputNb);

    // Find a greater or equal number of components
    int foundSupportedNComps = -1;
    for (int i = nComps - 1; i < 4; ++i) {
        if (supported[i]) {
            foundSupportedNComps = i + 1;
            break;
        }
    }


    if (foundSupportedNComps == -1) {
        // Find a small number of components
        for (int i = nComps - 2; i >= 0; --i) {
            if (supported[i]) {
                foundSupportedNComps = i + 1;
                break;
            }
        }
    }

    if (foundSupportedNComps == -1) {
        return 0;
    }
    return foundSupportedNComps;

} // findClosestSupportedNumberOfComponents




ImageBitDepthEnum
Node::getClosestSupportedBitDepth(ImageBitDepthEnum depth)
{


    bool foundShort = false;
    bool foundByte = false;
    bool foundFloat = false;

    std::list<ImageBitDepthEnum> bitdepths = getSupportedBitDepths();

    for (std::list<ImageBitDepthEnum>::const_iterator it = bitdepths.begin(); it != bitdepths.end(); ++it) {
        ImageBitDepthEnum thisDepth = *it;
        if (thisDepth == depth) {
            return depth;
        } else if (thisDepth == eImageBitDepthFloat) {
            foundFloat = true;
        } else if (thisDepth == eImageBitDepthShort) {
            foundShort = true;
        } else if (thisDepth == eImageBitDepthByte) {
            foundByte = true;
        }
    }
    if (foundFloat) {
        return eImageBitDepthFloat;
    } else if (foundShort) {
        return eImageBitDepthShort;
    } else if (foundByte) {
        return eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);

        return eImageBitDepthNone;
    }
}

ImageBitDepthEnum
Node::getBestSupportedBitDepth() const
{
    bool foundShort = false;
    bool foundByte = false;

    std::list<ImageBitDepthEnum> bitdepths = getSupportedBitDepths();

    for (std::list<ImageBitDepthEnum>::const_iterator it = bitdepths.begin(); it != bitdepths.end(); ++it) {
        ImageBitDepthEnum thisDepth = *it;
        switch (thisDepth) {
            case eImageBitDepthByte:
            foundByte = true;
            break;

            case eImageBitDepthShort:
            foundShort = true;
            break;
            case eImageBitDepthHalf:
            break;

            case eImageBitDepthFloat:

            return eImageBitDepthFloat;

            case eImageBitDepthNone:
            break;
        }
    }

    if (foundShort) {
        return eImageBitDepthShort;
    } else if (foundByte) {
        return eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);

        return eImageBitDepthNone;
    }
}

bool
Node::isSupportedBitDepth(ImageBitDepthEnum depth) const
{
    std::list<ImageBitDepthEnum> bitdepths = getSupportedBitDepths();
    return std::find(bitdepths.begin(), bitdepths.end(), depth) != bitdepths.end();
}


NATRON_NAMESPACE_EXIT
