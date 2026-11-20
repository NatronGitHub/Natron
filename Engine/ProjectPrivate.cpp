/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#include "ProjectPrivate.h"

#include <algorithm>
#include <list>
#include <set>
#include <vector>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <QDir>

#include "Global/PythonUtils.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/GroupInput.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Project.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"


NATRON_NAMESPACE_ENTER

NATRON_NAMESPACE_ANONYMOUS_ENTER

// Append the nodes of app to ordered, each once, a node after the nodes upstream of it: its inputs (Node::getInput()
// already redirects a Group to its GroupOutput's input, a GroupInput to the input of its Group, and a Precomp to its
// output node, which belongs to the Precomp's own app, loaded before), and the nodes inside it for a Group. The
// GroupInput case below is redundant with the redirections, but keeps a GroupInput after the input of its Group if
// getInput() is not redirected (e.g. when the Group has no GroupOutput).
void
appendUpstreamFirst(const NodePtr& node,
                    const AppInstance* app,
                    std::set<Node*>* visited,
                    NodesList* ordered)
{
    if ( !node || (node->getApp().get() != app) || !visited->insert( node.get() ).second ) {
        return;
    }
    for (int i = 0; i < node->getNInputs(); ++i) {
        appendUpstreamFirst(node->getInput(i), app, visited, ordered);
    }
    if ( dynamic_cast<GroupInput*>( node->getEffectInstance().get() ) ) {
        NodeGroupPtr group = std::dynamic_pointer_cast<NodeGroup>( node->getGroup() );
        if (group) {
            std::vector<NodePtr> groupInputs;
            group->getInputs(&groupInputs, false);
            for (std::size_t i = 0; i < groupInputs.size(); ++i) {
                if (groupInputs[i] == node) {
                    appendUpstreamFirst(group->getNode()->getInput( (int)i ), app, visited, ordered);
                }
            }
        }
    }
    NodeGroup* isGroup = node->isEffectGroup();
    if (isGroup) {
        NodesList children = isGroup->getNodes();
        for (NodesList::iterator it = children.begin(); it != children.end(); ++it) {
            if ( (*it)->isActivated() ) {
                appendUpstreamFirst(*it, app, visited, ordered);
            }
        }
    }
    ordered->push_back(node);
}

// Append to names, once each, the names of the nodes of app whose effects are in holders, as the user sees them: the
// Write or Read node rather than the reader or writer it embeds.
void
appendNodeNames(const std::set<KnobHolder*>& holders,
                const AppInstance* app,
                std::vector<std::string>* names)
{
    for (std::set<KnobHolder*>::const_iterator h = holders.begin(); h != holders.end(); ++h) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>(*h);
        NodePtr node = effect ? effect->getNode() : NodePtr();
        if ( !node || (node->getApp().get() != app) ) {
            continue;
        }
#ifdef NATRON_ENABLE_IO_META_NODES
        NodePtr container = node->getIOContainer();
        if (container) {
            node = container;
        }
#endif
        const std::string name = node->getFullyQualifiedName();
        if ( std::find(names->begin(), names->end(), name) == names->end() ) {
            names->push_back(name);
        }
    }
}

// "a, b, c"
QString
joinNames(const std::vector<std::string>& names)
{
    std::string joined;
    for (std::size_t i = 0; i < names.size(); ++i) {
        joined += (i ? ", " : "") + names[i];
    }

    return QString::fromUtf8( joined.c_str() );
}

NATRON_NAMESPACE_ANONYMOUS_EXIT

ProjectPrivate::ProjectPrivate(Project* project)
    : _publicInterface(project)
    , projectLock()
    , hasProjectBeenSavedByUser(false)
    , ageSinceLastSave( QDateTime::currentDateTime() )
    , lastAutoSave()
    , projectCreationTime(ageSinceLastSave)
    , builtinFormats()
    , additionalFormats()
    , formatMutex()
    , envVars()
    , projectName()
    , projectPath()
    , formatKnob()
    , addFormatKnob()
    , previewMode()
    , colorSpace8u()
    , colorSpace16u()
    , colorSpace32f()
    , natronVersion()
    , originalAuthorName()
    , lastAuthorName()
    , projectCreationDate()
    , saveDate()
    , onProjectLoadCB()
    , onProjectSaveCB()
    , onProjectCloseCB()
    , onNodeCreated()
    , onNodeDeleted()
    , timeline( new TimeLine(project) )
    , autoSetProjectFormat( appPTR->getCurrentSettings()->isAutoProjectFormatEnabled() )
    , autoSetProjectFormatPendingReaders()
    , autoSetProjectFormatRetryScheduled(false)
    , isLoadingProjectMutex()
    , isLoadingProject(false)
    , isLoadingProjectInternal(false)
    , isSavingProjectMutex()
    , isSavingProject(false)
    , isSavingProjectCond()
    , savingProjectThread(0)
    , waitingSaves(0)
    , resetCount(0)
    , autoSaveTimer( new QTimer() )
    , projectClosing(false)
    , resetting(false)
    , tlsData( new TLSHolder<Project::ProjectTLSData>() )

{
    autoSaveTimer->setSingleShot(true);
}

bool
ProjectPrivate::restoreFromSerialization(const ProjectSerialization & obj,
                                         const QString& name,
                                         const QString& path,
                                         bool* mustSave)
{
    /*1st OFF RESTORE THE PROJECT KNOBS*/
    bool ok;
    std::vector<std::string> linksErrors;
    {
        CreatingNodeTreeFlag_RAII creatingNodeTreeFlag( _publicInterface->getApp() );

        projectCreationTime = QDateTime::fromMSecsSinceEpoch( obj.getCreationDate() );

        _publicInterface->getApp()->updateProjectLoadStatus( tr("Restoring project settings...") );

        /*we must restore the entries in the combobox before restoring the value*/
        std::vector<ChoiceOption> entries;

        for (std::list<Format>::const_iterator it = builtinFormats.begin(); it != builtinFormats.end(); ++it) {
            QString formatStr = ProjectPrivate::generateStringFromFormat(*it);
            if ( !it->getName().empty() ) {
                entries.push_back( ChoiceOption(it->getName(), formatStr.toStdString(), "") );
            } else {
                entries.push_back( ChoiceOption( formatStr.toStdString() ) );
            }
        }

        const std::list<Format> & objAdditionalFormats = obj.getAdditionalFormats();
        for (std::list<Format>::const_iterator it = objAdditionalFormats.begin(); it != objAdditionalFormats.end(); ++it) {
            QString formatStr = ProjectPrivate::generateStringFromFormat(*it);
            if ( !it->getName().empty() ) {
                entries.push_back( ChoiceOption(it->getName(), formatStr.toStdString(), "") );
            } else {
                entries.push_back( ChoiceOption( formatStr.toStdString() ) );
            }
        }
        additionalFormats = objAdditionalFormats;

        formatKnob->populateChoices(entries);
        autoSetProjectFormat = false;

        const std::list<KnobSerializationPtr> & projectSerializedValues = obj.getProjectKnobsValues();
        const std::vector<KnobIPtr> & projectKnobs = _publicInterface->getKnobs();

        /// 1) restore project's knobs.
        for (U32 i = 0; i < projectKnobs.size(); ++i) {
            ///try to find a serialized value for this knob
            for (std::list<KnobSerializationPtr>::const_iterator it = projectSerializedValues.begin(); it != projectSerializedValues.end(); ++it) {
                if ( (*it)->getName() == projectKnobs[i]->getName() ) {
                    ///EDIT: Allow non persistent params to be loaded if we found a valid serialization for them
                    //if ( projectKnobs[i]->getIsPersistent() ) {

                    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( projectKnobs[i].get() );
                    if (isChoice) {
                        const TypeExtraData* extraData = (*it)->getExtraData();
                        const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>(extraData);
                        assert(choiceData);
                        if (choiceData) {
                            KnobChoice* choiceSerialized = dynamic_cast<KnobChoice*>( (*it)->getKnob().get() );
                            assert(choiceSerialized);
                            if (choiceSerialized) {
                                std::string optionID = choiceData->_choiceString;
                                // first, try to get the id the easy way ( see choiceMatch() )
                                int id = isChoice->choiceRestorationId(choiceSerialized, optionID);
//#pragma message WARN("TODO: choice id filters")
                                //if (id < 0) {
                                //    // no luck, try the filters
                                //    filterKnobChoiceOptionCompat(getPluginID(), serialization.getPluginMajorVersion(), serialization.getPluginMinorVersion(), projectInfos.vMajor, projectInfos.vMinor, projectInfos.vRev, serializedName, &optionID);
                                //    id = isChoice->choiceRestorationId(choiceSerialized, optionID);
                                //}
                                isChoice->choiceRestoration(choiceSerialized, optionID, id);
                            }
                        }
                    } else {
                        projectKnobs[i]->clone( (*it)->getKnob() );
                    }
                    //}
                    break;
                }
            }
            if (projectKnobs[i] == envVars) {
                ///For eAppTypeBackgroundAutoRunLaunchedFromGui don't change the project path since it is controlled
                ///by the main GUI process
                if (appPTR->getAppType() != AppManager::eAppTypeBackgroundAutoRunLaunchedFromGui) {
                    autoSetProjectDirectory(path);
                }
                _publicInterface->onOCIOConfigPathChanged(appPTR->getOCIOConfigPath(), false);
            } else if (projectKnobs[i] == natronVersion) {
                std::string v = natronVersion->getValue();
                if (v == "Natron v1.0.0") {
                    _publicInterface->getApp()->setProjectWasCreatedWithLowerCaseIDs(true);
                }
            }
        }

        /// 2) restore the timeline
        timeline->seekFrame(obj.getCurrentTime(), false, 0, eTimelineChangeReasonOtherSeek);


        /// 3) Restore the nodes

        std::map<std::string, bool> processedModules;
        ok = NodeCollectionSerialization::restoreFromSerialization(obj.getNodesSerialization().getNodesSerialization(),
                                                                   _publicInterface->shared_from_this(), true, &processedModules);
        for (std::map<std::string, bool>::iterator it = processedModules.begin(); it != processedModules.end(); ++it) {
            if (it->second) {
                *mustSave = true;
                break;
            }
        }

        // restore the Knob links stored in the Knobs during NodeCollectionSerialization::restoreFromSerialization()
        NodesList allNodes;
        const std::map<std::string, std::string> oldNewScriptNamesMapping;
        _publicInterface->getNodes_recursive(allNodes, false);
        for (NodesList::iterator n = allNodes.begin(); n != allNodes.end(); ++n) {
            // find all KnobIs in the node, restore links
            std::vector<KnobIPtr> knobs = (*n)->getKnobs(); // copy the knobs vector
            for (std::vector<KnobIPtr>::iterator k = knobs.begin(); k != knobs.end(); ++k) {
                // Even if there are links errors, do our best to restore the project, then display an error dialog.
                try {
                    (*k)->restoreLinks(allNodes, oldNewScriptNamesMapping, true); // error if links cannot be restored
                } catch (const std::exception& e) {
                    linksErrors.push_back( e.what() );
                }
            }
        }

        _publicInterface->getApp()->updateProjectLoadStatus( tr("Restoring graph stream preferences...") );
    } // CreatingNodeTreeFlag_RAII creatingNodeTreeFlag(_publicInterface->getApp());

    _publicInterface->forceComputeInputDependentDataOnAllTrees();

    // All the nodes, links and input connections are restored and the clip preferences computed: tell the effects
    // that their inputs are connected, before anything is rendered (#1107, see OfxEffectInstance::onInputsRestoredAfterProjectLoad()). Upstream nodes first, so that
    // an effect sees the metadata of its inputs as refreshed after their own action.
    // The render thread storage that the actions need is set once on the tree of each root (a node that no node of
    // the project reads), not once per effect: setting it walks the whole upstream tree, so once per effect costs
    // O(n^2) on a chain of n effects (3 s for 1000 Multiply nodes, instead of 8 ms, #1447). It is set again after an
    // action changed a knob (the hashes changed). Setting it may wait for an OpenGL context of the pool, as in
    // Node::onInputChanged().
    // The knob changed handlers aren't called while the project is loading (KnobHolder::endChanges()), and the knobs
    // age isn't incremented: if a knob that affects the render or the metadata changed in an action, give its node,
    // and the nodes whose links or expressions follow it, a new age (hence a new hash, so that no image cached for
    // the old values is used), and refresh their input related data (clip preferences, metadata) and those of their
    // outputs. Their hashes changed: set the render thread storage again for the next action. Display-only knobs,
    // which many plug-ins set from their inputs (e.g. the info parameters of WriteFFmpeg), are neither saved nor
    // rendered: they change nothing. If a saved knob changed, the values the project has are not the saved ones: the
    // project is marked as modified, and the nodes are listed in the Script Editor (stdout in background mode).
    std::vector<std::string> changedDuringLoad; // the nodes whose saved values changed, in the order of the changes
    std::vector<std::string> animatedNotSet; // the nodes whose saved animated values a plug-in tried to set
    {
        const AppInstance* app = _publicInterface->getApp().get();
        NodesList nodes, roots;
        _publicInterface->getNodes_recursive(nodes, true);
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            NodesList outputs;
            (*it)->getOutputsWithGroupRedirection(outputs);
            bool isRoot = true;
            for (NodesList::iterator o = outputs.begin(); o != outputs.end() && isRoot; ++o) {
                isRoot = ( (*o)->getApp().get() != app );
            }
            if (isRoot) {
                roots.push_back(*it);
            }
        }
        // the roots, then any node they don't reach
        roots.insert( roots.end(), nodes.begin(), nodes.end() );
        std::set<Node*> visited;
        for (NodesList::iterator root = roots.begin(); root != roots.end(); ++root) {
            NodesList tree;
            appendUpstreamFirst(*root, app, &visited, &tree);
            ParallelRenderArgsSetterPtr renderArgs;
            for (NodesList::iterator it = tree.begin(); it != tree.end(); ++it) {
                if ( !(*it)->getEffectInstance()->wantsInputsRestoredAfterProjectLoad() ) {
                    continue;
                }
                if (!renderArgs) {
                    renderArgs = (*root)->createRenderArgsForInstanceChanged();
                }
                KnobHolder::ValueChanges changes;
                {
                    KnobHolder::ValueChangesRecorder recorder(&changes);
                    (*it)->onInputsRestoredAfterProjectLoad();
                }
                appendNodeNames(changes.persistent, app, &changedDuringLoad);
                appendNodeNames(changes.animatedNotSet, app, &animatedNotSet);
                if ( changes.significant.empty() ) {
                    continue;
                }
                renderArgs.reset();
                for (std::set<KnobHolder*>::const_iterator h = changes.significant.begin(); h != changes.significant.end(); ++h) {
                    EffectInstance* effect = dynamic_cast<EffectInstance*>(*h);
                    NodePtr node = effect ? effect->getNode() : NodePtr();
                    if ( node && (node->getApp().get() == app) ) {
                        node->incrementKnobsAge();
                        node->forceRefreshAllInputRelatedData();
                    }
                }
            }
        }
    }

    QDateTime time = QDateTime::currentDateTime();
    autoSetProjectFormat = false;
    hasProjectBeenSavedByUser = true;
    projectName->setValue( name.toStdString() );
    projectPath->setValue( path.toStdString() );
    ageSinceLastSave = time;
    lastAutoSave = time;
    if ( !changedDuringLoad.empty() ) {
        // modified, as after loading an auto-save (see Project::loadProjectInternal() and Project::isSaveUpToDate())
        ageSinceLastSave = QDateTime();
        QString message = tr("Project load: plug-ins changed parameter values in: %1 (the project is marked modified)").arg( joinNames(changedDuringLoad) );
        // Project::loadProject() saves a project whose PyPlugs were updated right after the load: that would write
        // the values the plug-ins changed without the user seeing them. The user saves (or not) the project instead.
        if (*mustSave) {
            *mustSave = false;
            message += QLatin1Char(' ');
            message += tr("The updated PyPlugs were not saved: save the project to keep them.");
        }
        _publicInterface->getApp()->appendToScriptEditor( message.toStdString() );
    }
    if ( !animatedNotSet.empty() ) {
        // no keyframe is set while a project is loading (Knob<T>::setValue()): the animation is kept
        _publicInterface->getApp()->appendToScriptEditor( tr("Project load: plug-ins tried to change animated parameters, which keep their animation, in: %1")
                                                          .arg( joinNames(animatedNotSet) ).toStdString() );
    }
    _publicInterface->getApp()->setProjectWasCreatedWithLowerCaseIDs(false);

    if (obj.getVersion() < PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS) {
        _publicInterface->recomputeFrameRangeFromReaders();
    }

    // Even if there are links errors, do our best to restore the project, then display an error dialog.
    if ( !linksErrors.empty() ) {
        const char* const delim = "\n";

        std::ostringstream imploded;
        std::copy(linksErrors.begin(), linksErrors.end(),
                  std::ostream_iterator<std::string>(imploded, delim));
        throw std::runtime_error( imploded.str() );
    }
    return ok;
} // restoreFromSerialization

bool
ProjectPrivate::findFormat(int index,
                           Format* format) const
{
    if ( index >= (int)( builtinFormats.size() + additionalFormats.size() ) ) {
        return false;
    }

    int i = 0;
    if ( index >= (int)builtinFormats.size() ) {
        ///search in the additional formats
        index -= builtinFormats.size();

        for (std::list<Format>::const_iterator it = additionalFormats.begin(); it != additionalFormats.end(); ++it) {
            if (i == index) {
                assert( !it->isNull() );
                *format = *it;

                return true;
            }
            ++i;
        }
    } else {
        ///search in the builtins formats
        for (std::list<Format>::const_iterator it = builtinFormats.begin(); it != builtinFormats.end(); ++it) {
            if (i == index) {
                assert( !it->isNull() );
                *format = *it;

                return true;
            }
            ++i;
        }
    }

    return false;
}

void
ProjectPrivate::autoSetProjectDirectory(const QString& path)
{
    std::string pathCpy = path.toStdString();

    if ( !pathCpy.empty() && (pathCpy[pathCpy.size() - 1] == '/') ) {
        pathCpy.erase(pathCpy.size() - 1, 1);
    }
    std::string env;
    try {
        env = envVars->getValue();
    } catch (...) {
        // ignore
    }

    std::list<std::vector<std::string> > table;
    envVars->decodeFromKnobTableFormat(env, &table);

    ///If there was already a OCIO variable, update it, otherwise create it
    bool foundProject = false;
    for (std::list<std::vector<std::string> >::iterator it = table.begin(); it != table.end(); ++it) {
        if ( (*it)[0] == NATRON_PROJECT_ENV_VAR_NAME ) {
            (*it)[1] = pathCpy;
            foundProject = true;
            break;
        }
    }
    if (!foundProject) {
        std::vector<std::string> vec(2);
        vec[0] = NATRON_PROJECT_ENV_VAR_NAME;
        vec[1] = pathCpy;
        table.push_back(vec);
    }


    std::string newEnv = envVars->encodeToKnobTableFormat(table);
    if (env != newEnv) {
        if ( appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled() ) {
            _publicInterface->fixRelativeFilePaths(NATRON_PROJECT_ENV_VAR_NAME, pathCpy, false);
        }
        envVars->setValue(newEnv);
    }
}

std::string
ProjectPrivate::runOnProjectSaveCallback(const std::string& filename,
                                         bool autoSave)
{
    std::string onProjectSave = _publicInterface->getOnProjectSaveCB();

    if ( !onProjectSave.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(onProjectSave, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to get signature of onProjectSave callback: ")
                                                              + e.what() );

            return filename;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to get signature of onProjectSave callback: " + error);

            return filename;
        } else {
            std::string signatureError;
            signatureError.append("The on project save callback supports the following signature(s):\n");
            signatureError.append("- callback(filename,app,autoSave)");
            if ( (args.size() != 3) || (args[0] != "filename") || (args[1] != "app") || (args[2] != "autoSave") ) {
                _publicInterface->getApp()->appendToScriptEditor("Wrong signature of onProjectSave callback: " + signatureError);

                return filename;
            }
            std::string appID = _publicInterface->getApp()->getAppIDString();
            std::stringstream ss;
            if (appID != "app") {
                ss << "app = " << appID << "\n";
            }
            ss << "ret = " << onProjectSave << "(\"" << filename << "\"," << appID << ",";
            if (autoSave) {
                ss << "True)\n";
            } else {
                ss << "False)\n";
            }

            onProjectSave = ss.str();
            std::string err;
            std::string output;
            if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(onProjectSave, &err, &output) ) {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + err);

                return filename;
            } else {
                PythonGILLocker pgl;

                PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
                assert(mainModule);
                PyObject* ret = PyObject_GetAttrString(mainModule, "ret");
                if (!ret) {
                    return filename;
                }
                std::string filePath = filename;
                if (ret) {
                    filePath = NATRON_PYTHON_NAMESPACE::PyStringToStdString(ret);
                    std::string script = "del ret\n";
                    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
                    assert(ok);
                    if (!ok) {
                        throw std::runtime_error("ProjectPrivate::runOnProjectSaveCallback(): interpretPythonScript(" + script + ") failed!");
                    }
                }
                if ( !output.empty() ) {
                    _publicInterface->getApp()->appendToScriptEditor(output);
                }

                return filePath;
            }
        }
    }

    return filename;
} // ProjectPrivate::runOnProjectSaveCallback

void
ProjectPrivate::runOnProjectCloseCallback()
{
    // In interpreter mode (-t), the callback ran from the atexit hook of
    // launchPythonInterpreter(), before Py_Main() finalized Python (#1253).
    if ( !Py_IsInitialized() ) {
        return;
    }
    std::string onProjectClose = _publicInterface->getOnProjectCloseCB();

    if ( !onProjectClose.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(onProjectClose, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to get signature of onProjectClose callback: ")
                                                              + e.what() );

            return;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to get signature of onProjectClose callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The on project close callback supports the following signature(s):\n");
        signatureError.append("- callback(app)");
        if ( (args.size() != 1) || (args[0] != "app") ) {
            _publicInterface->getApp()->appendToScriptEditor("Wrong signature of onProjectClose callback: " + signatureError);

            return;
        }
        std::string appID = _publicInterface->getApp()->getAppIDString();
        std::string script;
        if (appID != "app") {
            script = script +  "app = " + appID;
        }
        script = script + "\n" + onProjectClose + "(" + appID + ")\n";
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + err);
        } else {
            if ( !output.empty() ) {
                _publicInterface->getApp()->appendToScriptEditor(output);
            }
        }
    }
} // ProjectPrivate::runOnProjectCloseCallback

void
ProjectPrivate::runOnProjectLoadCallback()
{
    std::string cb = _publicInterface->getOnProjectLoadCB();

    if ( !cb.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to get signature of onProjectLoaded callback: ")
                                                              + e.what() );

            return;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to get signature of onProjectLoaded callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The on  project loaded callback supports the following signature(s):\n");
        signatureError.append("- callback(app)");
        if ( (args.size() != 1) || (args[0] != "app") ) {
            _publicInterface->getApp()->appendToScriptEditor("Wrong signature of onProjectLoaded callback: " + signatureError);

            return;
        }

        std::string appID = _publicInterface->getApp()->getAppIDString();
        std::string script;
        if (appID != "app") {
            script =  script + "app = " + appID;
        }
        script =  script + "\n" + cb + "(" + appID + ")\n";
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + err);
        } else {
            if ( !output.empty() ) {
                _publicInterface->getApp()->appendToScriptEditor(output);
            }
        }
    }
} // ProjectPrivate::runOnProjectLoadCallback

void
ProjectPrivate::setProjectFilename(const std::string& filename)
{
    projectName->setValue(filename);
}

std::string
ProjectPrivate::getProjectFilename() const
{
    return projectName->getValue();
}

void
ProjectPrivate::setProjectPath(const std::string& path)
{
    projectPath->setValue(path);
}

std::string
ProjectPrivate::getProjectPath() const
{
    return projectPath->getValue();
}

NATRON_NAMESPACE_EXIT
