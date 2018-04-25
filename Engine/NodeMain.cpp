/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "NodePrivate.h"

#include <QtCore/QThread>

#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/LibraryBinary.h"
#include "Engine/ReadNode.h"
#include "Engine/WriteNode.h"
#include "Engine/EffectInstance.h"
#include "Engine/Project.h"
#include "Engine/NodeGuiI.h"
#include "Engine/NodeSerialization.h"
#include "Engine/GenericSchedulerThreadWatcher.h"


NATRON_NAMESPACE_ENTER

void
Node::load(const CreateNodeArgs& args)
{
    ///Called from the main thread. MT-safe
    assert( QThread::currentThread() == qApp->thread() );

    ///cannot load twice
    assert(!_imp->effect);
    _imp->isPartOfProject = !args.getProperty<bool>(kCreateNodeArgsPropOutOfProject);

#ifdef NATRON_ENABLE_IO_META_NODES
    _imp->ioContainer = args.getProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer);
#endif

    NodeCollectionPtr group = getGroup();
    std::string multiInstanceParentName = args.getProperty<std::string>(kCreateNodeArgsPropMultiInstanceParentName);
    if ( !multiInstanceParentName.empty() ) {
        _imp->multiInstanceParentName = multiInstanceParentName;
        _imp->isMultiInstance = false;
        fetchParentMultiInstancePointer();
    }


    _imp->wasCreatedSilently = args.getProperty<bool>(kCreateNodeArgsPropSilent);

    NodePtr thisShared = shared_from_this();
    LibraryBinary* binary = _imp->plugin->getLibraryBinary();
    std::pair<bool, EffectBuilder> func;
    if (binary) {
        func = binary->findFunction<EffectBuilder>("BuildEffect");
    }


    NodeSerializationPtr serialization = args.getProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);

    bool isSilentCreation = args.getProperty<bool>(kCreateNodeArgsPropSilent);
#ifndef NATRON_ENABLE_IO_META_NODES
    bool hasUsedFileDialog = false;
#endif

    bool hasDefaultFilename = false;
    {
        std::vector<std::string> defaultParamValues = args.getPropertyN<std::string>(kCreateNodeArgsPropNodeInitialParamValues);
        std::vector<std::string>::iterator foundFileName  = std::find(defaultParamValues.begin(), defaultParamValues.end(), std::string(kOfxImageEffectFileParamName));
        if (foundFileName != defaultParamValues.end()) {
            std::string propName(kCreateNodeArgsPropParamValue);
            propName += "_";
            propName += kOfxImageEffectFileParamName;
            hasDefaultFilename = !args.getProperty<std::string>(propName).empty();
        }
    }
    bool canOpenFileDialog = !isSilentCreation && !serialization && _imp->isPartOfProject && !hasDefaultFilename && getGroup();

    std::string argFixedName = args.getProperty<std::string>(kCreateNodeArgsPropNodeInitialName);


    if (func.first) {
        /*
           We are creating a built-in plug-in
         */
        _imp->effect.reset( func.second(thisShared) );
        assert(_imp->effect);


#ifdef NATRON_ENABLE_IO_META_NODES
        NodePtr ioContainer = _imp->ioContainer.lock();
        if (ioContainer) {
            ReadNode* isReader = dynamic_cast<ReadNode*>( ioContainer->getEffectInstance().get() );
            if (isReader) {
                isReader->setEmbeddedReader(thisShared);
            } else {
                WriteNode* isWriter = dynamic_cast<WriteNode*>( ioContainer->getEffectInstance().get() );
                assert(isWriter);
                if (isWriter) {
                    isWriter->setEmbeddedWriter(thisShared);
                }
            }
        }
#endif
        initNodeScriptName(serialization.get(), QString::fromUtf8(argFixedName.c_str()));

        _imp->effect->initializeData();

        createRotoContextConditionnally();
        createTrackerContextConditionnally();
        initializeInputs();
        initializeKnobs(serialization.get() != 0);

        refreshAcceptedBitDepths();

        _imp->effect->setDefaultMetadata();

        if (serialization) {

            _imp->effect->onKnobsAboutToBeLoaded(serialization);
            loadKnobs(*serialization);
        }
        setValuesFromSerialization(args);



#ifndef NATRON_ENABLE_IO_META_NODES
        std::string images;
        if (_imp->effect->isReader() && canOpenFileDialog) {
            images = getApp()->openImageFileDialog();
        } else if (_imp->effect->isWriter() && canOpenFileDialog) {
            images = getApp()->saveImageFileDialog();
        }
        if ( !images.empty() ) {
            hasUsedFileDialog = true;
            KnobSerializationPtr defaultFile = createDefaultValueForParam(kOfxImageEffectFileParamName, images);
            CreateNodeArgs::DefaultValuesList list;
            list.push_back(defaultFile);

            std::string canonicalFilename = images;
            getApp()->getProject()->canonicalizePath(canonicalFilename);
            int firstFrame, lastFrame;
            Node::getOriginalFrameRangeForReader(getPluginID(), canonicalFilename, &firstFrame, &lastFrame);
            list.push_back( createDefaultValueForParam(kReaderParamNameOriginalFrameRange, firstFrame, lastFrame) );
            setValuesFromSerialization(args);
        }
#endif
    } else {
        //ofx plugin
#ifndef NATRON_ENABLE_IO_META_NODES
        _imp->effect = appPTR->createOFXEffect(thisShared, args, canOpenFileDialog, &hasUsedFileDialog);
#else
        _imp->effect = appPTR->createOFXEffect(thisShared, args);
#endif
        assert(_imp->effect);
    }

    _imp->effect->initializeOverlayInteract();


    if ( _imp->supportedDepths.empty() ) {
        //From the spec:
        //The default for a plugin is to have none set, the plugin \em must define at least one in its describe action.
        throw std::runtime_error("Plug-in does not support 8bits, 16bits or 32bits floating point image processing.");
    }

    /*
       Set modifiable props
     */
    refreshDynamicProperties();
    onOpenGLEnabledKnobChangedOnProject(getApp()->getProject()->isOpenGLRenderActivated());

    if ( isTrackerNodePlugin() ) {
        _imp->isMultiInstance = true;
    }

    /*if (isMultiInstanceChild && !args.serialization) {
        updateEffectLabelKnob( QString::fromUtf8( getScriptName().c_str() ) );
     }*/
    restoreSublabel();

    declarePythonFields();
    if  ( getRotoContext() ) {
        declareRotoPythonField();
    }
    if ( getTrackerContext() ) {
        declareTrackerPythonField();
    }


    if (group) {
        group->notifyNodeActivated(thisShared);
    }

    //This flag is used for the Roto plug-in and for the Merge inside the rotopaint tree
    //so that if the input of the roto node is RGB, it gets converted with alpha = 0, otherwise the user
    //won't be able to paint the alpha channel
    const QString& pluginID = _imp->plugin->getPluginID();
    if ( isRotoPaintingNode() || ( pluginID == QString::fromUtf8(PLUGINID_OFX_ROTO) ) ) {
        _imp->useAlpha0ToConvertFromRGBToRGBA = true;
    }

    if (!serialization) {
        computeHash();
    }

    assert(_imp->effect);

    _imp->pluginSafety = _imp->effect->renderThreadSafety();
    _imp->currentThreadSafety = _imp->pluginSafety;


    bool isLoadingPyPlug = getApp()->isCreatingPythonGroup();

    _imp->effect->onEffectCreated(canOpenFileDialog, args);

    _imp->nodeCreated = true;

    if ( !getApp()->isCreatingNodeTree() ) {
        refreshAllInputRelatedData(!serialization);
    }


    _imp->runOnNodeCreatedCB(!serialization && !isLoadingPyPlug);
} // load


void
Node::setValuesFromSerialization(const CreateNodeArgs& args)
{

    std::vector<std::string> params = args.getPropertyN<std::string>(kCreateNodeArgsPropNodeInitialParamValues);

    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->knobsInitialized);
    const std::vector<KnobIPtr> & nodeKnobs = getKnobs();

    for (std::size_t i = 0; i < params.size(); ++i) {
        for (U32 j = 0; j < nodeKnobs.size(); ++j) {
            if (nodeKnobs[j]->getName() == params[i]) {

                KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(nodeKnobs[j].get());
                KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(nodeKnobs[j].get());
                KnobDoubleBase* isDbl = dynamic_cast<KnobDoubleBase*>(nodeKnobs[j].get());
                KnobStringBase* isStr = dynamic_cast<KnobStringBase*>(nodeKnobs[j].get());
                int nDims = nodeKnobs[j]->getDimension();

                std::string propName = kCreateNodeArgsPropParamValue;
                propName += "_";
                propName += params[i];
                if (isBool) {
                    std::vector<bool> v = args.getPropertyN<bool>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isBool->setValue(v[d], ViewSpec(0), d);
                    }
                } else if (isInt) {
                    std::vector<int> v = args.getPropertyN<int>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isInt->setValue(v[d], ViewSpec(0), d);
                    }
                } else if (isDbl) {
                    std::vector<double> v = args.getPropertyN<double>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isDbl->setValue(v[d], ViewSpec(0), d );
                    }
                } else if (isStr) {
                    std::vector<std::string> v = args.getPropertyN<std::string>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isStr->setValue(v[d],ViewSpec(0), d );
                    }
                }
                break;
            }
        }
    }


}


void
Node::restoreUserKnobs(const NodeSerialization& serialization)
{
    const std::list<GroupKnobSerializationPtr>& userPages = serialization.getUserPages();

    for (std::list<GroupKnobSerializationPtr>::const_iterator it = userPages.begin(); it != userPages.end(); ++it) {
        KnobIPtr found = getKnobByName( (*it)->getName() );
        KnobPagePtr page;
        if (!found) {
            page = AppManager::createKnob<KnobPage>(_imp->effect.get(), (*it)->getLabel(), 1, false);
            page->setAsUserKnob(true);
            page->setName( (*it)->getName() );
        } else {
            page = boost::dynamic_pointer_cast<KnobPage>(found);
        }
        if (page) {
            _imp->restoreUserKnobsRecursive( (*it)->getChildren(), KnobGroupPtr(), page );
        }
    }
    setPagesOrder( serialization.getPagesOrdered() );
}

void
Node::Implementation::restoreUserKnobsRecursive(const std::list<KnobSerializationBasePtr>& knobs,
                                                const KnobGroupPtr& group,
                                                const KnobPagePtr& page)
{
    for (std::list<KnobSerializationBasePtr>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>( it->get() );
        KnobSerialization* isRegular = dynamic_cast<KnobSerialization*>( it->get() );
        assert(isGrp || isRegular);

        KnobIPtr found = _publicInterface->getKnobByName( (*it)->getName() );

        if (isGrp) {
            KnobGroupPtr grp;
            if (!found) {
                grp = AppManager::createKnob<KnobGroup>(effect.get(), isGrp->getLabel(), 1, false);
            } else {
                grp = boost::dynamic_pointer_cast<KnobGroup>(found);
                if (!grp) {
                    continue;
                }
            }
            grp->setAsUserKnob(true);
            grp->setName( (*it)->getName() );
            if ( isGrp && isGrp->isSetAsTab() ) {
                grp->setAsTab();
            }
            page->addKnob(grp);
            if (group) {
                group->addKnob(grp);
            }
            grp->setValue( isGrp->isOpened() );
            restoreUserKnobsRecursive(isGrp->getChildren(), grp, page);
        } else if (isRegular) {
            assert( isRegular->isUserKnob() );
            KnobIPtr sKnob = isRegular->getKnob();
            KnobIPtr knob;
            KnobInt* isInt = dynamic_cast<KnobInt*>( sKnob.get() );
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>( sKnob.get() );
            KnobBool* isBool = dynamic_cast<KnobBool*>( sKnob.get() );
            KnobChoice* isChoice = dynamic_cast<KnobChoice*>( sKnob.get() );
            KnobColor* isColor = dynamic_cast<KnobColor*>( sKnob.get() );
            KnobString* isStr = dynamic_cast<KnobString*>( sKnob.get() );
            KnobFile* isFile = dynamic_cast<KnobFile*>( sKnob.get() );
            KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>( sKnob.get() );
            KnobPath* isPath = dynamic_cast<KnobPath*>( sKnob.get() );
            KnobButton* isBtn = dynamic_cast<KnobButton*>( sKnob.get() );
            KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( sKnob.get() );
            KnobParametric* isParametric = dynamic_cast<KnobParametric*>( sKnob.get() );

            assert(isInt || isDbl || isBool || isChoice || isColor || isStr || isFile || isOutFile || isPath || isBtn || isSep || isParametric);

            if (isInt) {
                KnobIntPtr k;

                if (!found) {
                    k = AppManager::createKnob<KnobInt>(effect.get(), isRegular->getLabel(),
                                                        sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobInt>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    std::vector<int> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < k->getDimension(); ++i) {
                        minimums.push_back(data->min);
                        maximums.push_back(data->max);
                        dminimums.push_back(data->dmin);
                        dmaximums.push_back(data->dmax);
                    }
                    k->setMinimumsAndMaximums(minimums, maximums);
                    k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                }
                knob = k;
            } else if (isDbl) {
                KnobDoublePtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobDouble>(effect.get(), isRegular->getLabel(),
                                                           sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobDouble>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    std::vector<double> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < k->getDimension(); ++i) {
                        minimums.push_back(data->min);
                        maximums.push_back(data->max);
                        dminimums.push_back(data->dmin);
                        dmaximums.push_back(data->dmax);
                    }
                    k->setMinimumsAndMaximums(minimums, maximums);
                    k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                }
                knob = k;

                if ( isRegular->getUseHostOverlayHandle() ) {
                    KnobDouble* isDbl = dynamic_cast<KnobDouble*>( knob.get() );
                    if (isDbl) {
                        isDbl->setHasHostOverlayHandle(true);
                    }
                }
            } else if (isBool) {
                KnobBoolPtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobBool>(effect.get(), isRegular->getLabel(),
                                                         sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobBool>(found);
                    if (!k) {
                        continue;
                    }
                }
                knob = k;
            } else if (isChoice) {
                KnobChoicePtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobChoice>(effect.get(), isRegular->getLabel(),
                                                           sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobChoice>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    std::vector<ChoiceOption> options(data->_entries.size());
                    for (std::size_t i =  0; i < options.size(); ++i) {
                        options[i].id = data->_entries[i];
                        if (i < data->_helpStrings.size()) {
                            options[i].tooltip = data->_helpStrings[i];
                        }
                    }
                    k->populateChoices(options);
                }
                knob = k;
            } else if (isColor) {
                KnobColorPtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobColor>(effect.get(), isRegular->getLabel(),
                                                          sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobColor>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    std::vector<double> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < k->getDimension(); ++i) {
                        minimums.push_back(data->min);
                        maximums.push_back(data->max);
                        dminimums.push_back(data->dmin);
                        dmaximums.push_back(data->dmax);
                    }
                    k->setMinimumsAndMaximums(minimums, maximums);
                    k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                }
                knob = k;
            } else if (isStr) {
                KnobStringPtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobString>(effect.get(), isRegular->getLabel(),
                                                           sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobString>(found);
                    if (!k) {
                        continue;
                    }
                }
                const TextExtraData* data = dynamic_cast<const TextExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    if (data->label) {
                        k->setAsLabel();
                    } else if (data->multiLine) {
                        k->setAsMultiLine();
                        if (data->richText) {
                            k->setUsesRichText(true);
                        }
                    }
                }
                knob = k;
            } else if (isFile) {
                KnobFilePtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobFile>(effect.get(), isRegular->getLabel(),
                                                         sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobFile>(found);
                    if (!k) {
                        continue;
                    }
                }
                const FileExtraData* data = dynamic_cast<const FileExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data && data->useSequences) {
                    k->setAsInputImage();
                }
                knob = k;
            } else if (isOutFile) {
                KnobOutputFilePtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobOutputFile>(effect.get(), isRegular->getLabel(),
                                                               sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobOutputFile>(found);
                    if (!k) {
                        continue;
                    }
                }
                const FileExtraData* data = dynamic_cast<const FileExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data && data->useSequences) {
                    k->setAsOutputImageFile();
                }
                knob = k;
            } else if (isPath) {
                KnobPathPtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobPath>(effect.get(), isRegular->getLabel(),
                                                         sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobPath>(found);
                    if (!k) {
                        continue;
                    }
                }
                const PathExtraData* data = dynamic_cast<const PathExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data && data->multiPath) {
                    k->setMultiPath(true);
                }
                knob = k;
            } else if (isBtn) {
                KnobButtonPtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobButton>(effect.get(), isRegular->getLabel(),
                                                           sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobButton>(found);
                    if (!k) {
                        continue;
                    }
                }
                knob = k;
            } else if (isSep) {
                KnobSeparatorPtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobSeparator>(effect.get(), isRegular->getLabel(),
                                                              sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobSeparator>(found);
                    if (!k) {
                        continue;
                    }
                }
                knob = k;
            } else if (isParametric) {
                KnobParametricPtr k;
                if (!found) {
                    k = AppManager::createKnob<KnobParametric>(effect.get(), isRegular->getLabel(), sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobParametric>(found);
                    if (!k) {
                        continue;
                    }
                }
                knob = k;
            }

            assert(knob);
            if (!knob) {
                continue;
            }
            knob->cloneDefaultValues( sKnob.get() );
            if (isChoice) {
                const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>( isRegular->getExtraData() );
                assert(choiceData);
                KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knob.get() );
                assert(isChoice);
                if (choiceData && isChoice) {
                    KnobChoice* choiceSerialized = dynamic_cast<KnobChoice*>( sKnob.get() );
                    if (choiceSerialized) {
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
                }
            } else {
                knob->clone( sKnob.get() );
            }
            knob->setAsUserKnob(true);
            if (group) {
                group->addKnob(knob);
            } else if (page) {
                page->addKnob(knob);
            }

            knob->setIsPersistent( isRegular->isPersistent() );
            knob->setAnimationEnabled( isRegular->isAnimationEnabled() );
            knob->setEvaluateOnChange( isRegular->getEvaluatesOnChange() );
            knob->setName( isRegular->getName() );
            knob->setHintToolTip( isRegular->getHintToolTip() );
            if ( !isRegular->triggerNewLine() ) {
                knob->setAddNewLine(false);
            }
        }
    }
} // Node::Implementation::restoreUserKnobsRecursive


void
Node::Implementation::restoreKnobLinksRecursive(const GroupKnobSerialization* group,
                                                const NodesList & allNodes,
                                                const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    const std::list<KnobSerializationBasePtr>&  children = group->getChildren();

    for (std::list<KnobSerializationBasePtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
        GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>( it->get() );
        KnobSerialization* isRegular = dynamic_cast<KnobSerialization*>( it->get() );
        assert(isGrp || isRegular);
        if (isGrp) {
            restoreKnobLinksRecursive(isGrp, allNodes, oldNewScriptNamesMapping);
        } else if (isRegular) {
            KnobIPtr knob =  _publicInterface->getKnobByName( isRegular->getName() );
            if (!knob) {
                LogEntry::LogEntryColor c;
                if (_publicInterface->getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }

                QString err = tr("Could not find a parameter named %1").arg( QString::fromUtf8( (*it)->getName().c_str() ) );
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( _publicInterface->getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), err, false, c);
                continue;
            }
            isRegular->restoreKnobLinks(knob, allNodes, oldNewScriptNamesMapping);
            isRegular->restoreExpressions(knob, oldNewScriptNamesMapping);
        }
    }
}

void
Node::restoreKnobsLinks(const NodeSerialization & serialization,
                        const NodesList & allNodes,
                        const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    const NodeSerialization::KnobValues & knobsValues = serialization.getKnobsValues();
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        KnobIPtr knob = getKnobByName( (*it)->getName() );
        if (!knob) {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }

            QString err = tr("Could not find a parameter named %1").arg( QString::fromUtf8( (*it)->getName().c_str() ) );
            appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), err, false, c);
            continue;
        }
        (*it)->restoreKnobLinks(knob, allNodes, oldNewScriptNamesMapping);
        (*it)->restoreExpressions(knob, oldNewScriptNamesMapping);
    }

    const std::list<GroupKnobSerializationPtr>& userKnobs = serialization.getUserPages();
    for (std::list<GroupKnobSerializationPtr>::const_iterator it = userKnobs.begin(); it != userKnobs.end(); ++it) {
        _imp->restoreKnobLinksRecursive( (*it).get(), allNodes, oldNewScriptNamesMapping );
    }
}


void
Node::doDestroyNodeInternalEnd(bool autoReconnect)
{
    ///Remove the node from the project
    deactivate(NodesList(),
               true,
               autoReconnect,
               true,
               false);


    {
        NodeGuiIPtr guiPtr = _imp->guiPointer.lock();
        if (guiPtr) {
            guiPtr->destroyGui();
        }
    }

    ///If its a group, clear its nodes
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGrp) {
        isGrp->clearNodesBlocking();
    }


    ///Quit any rendering
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( _imp->effect.get() );
    if (isOutput) {
        isOutput->getRenderEngine()->quitEngine(true);
    }

    ///Remove all images in the cache associated to this node
    ///This will not remove from the disk cache if the project is closing
    removeAllImagesFromCache(false);

    AppInstancePtr app = getApp();
    if (app) {
        app->recheckInvalidExpressions();
    }

    ///Remove the Python node
    deleteNodeVariableToPython( getFullyQualifiedName() );

    ///Disconnect all inputs
    /*int maxInputs = getNInputs();
       for (int i = 0; i < maxInputs; ++i) {
       disconnectInput(i);
       }*/

    ///Kill the effect
    if (_imp->effect) {
        _imp->effect->clearPluginMemoryChunks();
    }
    _imp->effect.reset();

    ///If inside the group, remove it from the group
    ///the use_count() after the call to removeNode should be 2 and should be the shared_ptr held by the caller and the
    ///thisShared ptr

    ///If not inside a gorup or inside fromDest the shared_ptr is probably invalid at this point
    NodeCollectionPtr thisGroup = getGroup();
    if ( thisGroup ) {
        thisGroup->removeNode(this);
    }

} // Node::doDestroyNodeInternalEnd


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

typedef boost::shared_ptr<NodeDestroyNodeInternalArgs> NodeDestroyNodeInternalArgsPtr;

NATRON_NAMESPACE_ANONYMOUS_EXIT


void
Node::onProcessingQuitInDestroyNodeInternal(int taskID,
                                            const GenericWatcherCallerArgsPtr& args)
{
    assert(_imp->renderWatcher);
    assert(taskID == (int)NodeRenderWatcher::eBlockingTaskQuitAnyProcessing);
    Q_UNUSED(taskID);
    assert(args);
    NodeDestroyNodeInternalArgs* thisArgs = dynamic_cast<NodeDestroyNodeInternalArgs*>( args.get() );
    assert(thisArgs);
    doDestroyNodeInternalEnd(thisArgs ? thisArgs->autoReconnect : false);
    _imp->renderWatcher.reset();
}


void
Node::destroyNode(bool blockingDestroy, bool autoReconnect)
{
    if (!_imp->effect) {
        return;
    }

    {
        QMutexLocker k(&_imp->activatedMutex);
        _imp->isBeingDestroyed = true;
    }

    bool allProcessingQuit = areAllProcessingThreadsQuit();
    if (allProcessingQuit || blockingDestroy) {
        if (!allProcessingQuit) {
            quitAnyProcessing_blocking(false);
        }
        doDestroyNodeInternalEnd(false);
    } else {
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>( _imp->effect.get() );
        NodesList nodesToWatch;
        nodesToWatch.push_back( shared_from_this() );
        if (isGrp) {
            isGrp->getNodes_recursive(nodesToWatch, false);
        }
        _imp->renderWatcher = boost::make_shared<NodeRenderWatcher>(nodesToWatch);
        QObject::connect( _imp->renderWatcher.get(), SIGNAL(taskFinished(int,WatcherCallerArgsPtr)), this, SLOT(onProcessingQuitInDestroyNodeInternal(int,WatcherCallerArgsPtr)) );
        NodeDestroyNodeInternalArgsPtr args = boost::make_shared<NodeDestroyNodeInternalArgs>();
        args->autoReconnect = autoReconnect;
        _imp->renderWatcher->scheduleBlockingTask(NodeRenderWatcher::eBlockingTaskQuitAnyProcessing, args);

    }
} // Node::destroyNodeInternal


bool
Node::isSupportedComponent(int inputNb,
                           const ImagePlaneDesc& comp) const
{
    QMutexLocker l(&_imp->inputsMutex);

    if (inputNb >= 0) {
        assert( inputNb < (int)_imp->inputsComponents.size() );
        std::list<ImagePlaneDesc>::const_iterator found =
            std::find(_imp->inputsComponents[inputNb].begin(), _imp->inputsComponents[inputNb].end(), comp);

        return found != _imp->inputsComponents[inputNb].end();
    } else {
        assert(inputNb == -1);
        std::list<ImagePlaneDesc>::const_iterator found =
            std::find(_imp->outputComponents.begin(), _imp->outputComponents.end(), comp);

        return found != _imp->outputComponents.end();
    }
}


ImageBitDepthEnum
Node::getClosestSupportedBitDepth(ImageBitDepthEnum depth)
{
    bool foundShort = false;
    bool foundByte = false;

    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->supportedDepths.begin(); it != _imp->supportedDepths.end(); ++it) {
        if (*it == depth) {
            return depth;
        } else if (*it == eImageBitDepthFloat) {
            return eImageBitDepthFloat;
        } else if (*it == eImageBitDepthShort) {
            foundShort = true;
        } else if (*it == eImageBitDepthByte) {
            foundByte = true;
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

ImageBitDepthEnum
Node::getBestSupportedBitDepth() const
{
    bool foundShort = false;
    bool foundByte = false;

    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->supportedDepths.begin(); it != _imp->supportedDepths.end(); ++it) {
        switch (*it) {
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
    return std::find(_imp->supportedDepths.begin(), _imp->supportedDepths.end(), depth) != _imp->supportedDepths.end();
}


NATRON_NAMESPACE_EXIT
