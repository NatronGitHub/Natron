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

#include "ProjectPrivate.h"

#include <list>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "Global/QtCompat.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/FileSystemModel.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/NodeSerialization.h"
#include "Serialization/ProjectSerialization.h"


NATRON_NAMESPACE_ENTER;

ProjectPrivate::ProjectPrivate(Project* project)
    : _publicInterface(project)
    , projectLock()
    , hasProjectBeenSavedByUser(false)
    , ageSinceLastSave( QDateTime::currentDateTime() )
    , lastAutoSave()
    , builtinFormats()
    , additionalFormats()
    , formatMutex(QMutex::Recursive)
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
    , isLoadingProjectMutex()
    , isLoadingProject(false)
    , isLoadingProjectInternal(false)
    , isSavingProjectMutex()
    , isSavingProject(false)
    , autoSaveTimer( new QTimer() )
    , projectClosing(false)
    , tlsData( new TLSHolder<Project::ProjectTLSData>() )

{
    autoSaveTimer->setSingleShot(true);
}

NodePtr
Project::findNodeWithScriptName(const std::string& nodeScriptName, const std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >& allCreatedNodesInGroup)
{
    for (std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >::const_iterator it = allCreatedNodesInGroup.begin(); it!=allCreatedNodesInGroup.end(); ++it) {
        if (!it->second) {
            continue;
        }
        if (it->second->_nodeScriptName == nodeScriptName) {
            return it->first;
        }
    }
    return NodePtr();
}

void
Project::restoreInput(const NodePtr& node,
                      const std::string& inputLabel,
                      const std::string& inputNodeScriptName,
                      const std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >& allCreatedNodesInGroup,
                      bool isMaskInput)
{
    if ( inputNodeScriptName.empty() ) {
        return;
    }
    int index = inputLabel.empty() ? -1 : node->getInputNumberFromLabel(inputLabel);
    if (index == -1) {

        // If the name of the input was not serialized, the string is the index
        bool ok;
        index = QString::fromUtf8(inputLabel.c_str()).toInt(&ok);
        if (!ok) {
            index = -1;
        }
        if (index == -1) {
            appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(node->getScriptName().c_str()), QDateTime::currentDateTime(),
                                            tr("Could not find input named %1")
                                            .arg( QString::fromUtf8( inputNodeScriptName.c_str() ) ) );

        }

        // If the node had a single mask, the inputLabel was "0", indicating the index of the mask
        // So iterate through masks to find it
        if (isMaskInput) {
            // Find the mask corresponding to the index
            int nInputs = node->getMaxInputCount();
            int maskIndex = 0;
            for (int i = 0; i < nInputs; ++i) {
                if (node->getEffectInstance()->isInputMask(i)) {
                    if (maskIndex == index) {
                        index = i;
                        break;
                    }
                    ++maskIndex;
                    break;
                }
            }
        }
    }
    
    NodeCollectionPtr nodeGroup = node->getGroup();
    if (!nodeGroup) {
        return;
    }
    // The nodes created from the serialization may have changed name if another node with the same script-name already existed.
    // By chance since we created all nodes within the same Group at the same time, we have a list of the old node serialization
    // and the corresponding created node (with its new script-name).
    // If we find a match, make sure we use the new node script-name to restore the input.
    NodePtr foundNode = findNodeWithScriptName(inputNodeScriptName, allCreatedNodesInGroup);
    if (!foundNode) {
        return;


        // Commented-out: Do not attempt to get the node in the nodes list: all nodes within a sub-graph should be connected to nodes at this level.
        // If it cannot be found in the allCreatedNodesInGroup then this is likely the user does not want the input to connect.
        //foundNode = nodeGroup->getNodeByName(inputNodeScriptName);
    }

    bool ok = node->getGroup()->connectNodes(index, foundNode, node);
    (void)ok;


} //restoreInput

void
Project::restoreInputs(const NodePtr& node,
                       const std::map<std::string, std::string>& inputsMap,
                       const std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >& allCreatedNodesInGroup,
                       bool isMaskInputs)
{
    for (std::map<std::string, std::string>::const_iterator it = inputsMap.begin(); it != inputsMap.end(); ++it) {
        restoreInput(node, it->first, it->second, allCreatedNodesInGroup, isMaskInputs);
    }
} // restoreInputs

static void
restoreLinksRecursive(const NodeCollectionPtr& group,
                      const SERIALIZATION_NAMESPACE::NodeSerializationList& nodes,
                      const std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >* allCreatedNodes)
{
    for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {

        // The nodes created from the serialization may have changed name if another node with the same script-name already existed.
        // By chance since we created all nodes within the same Group at the same time, we have a list of the old node serialization
        // and the corresponding created node (with its new script-name).
        // If we find a match, make sure we use the new node script-name to restore the input.
        NodePtr foundNode;
        if (allCreatedNodes) {
            foundNode = Project::findNodeWithScriptName((*it)->_nodeScriptName, *allCreatedNodes);
        }
        if (!foundNode) {
            // We did not find the node in the serialized nodes list, the last resort is to look into already created nodes
            // and find an exact match, hoping the script-name of the node did not change.
            foundNode = group->getNodeByName((*it)->_nodeScriptName);
        }
        if (!foundNode) {
            continue;
        }

        // The allCreatedNodes list is useful if the nodes that we created had their script-name changed from what was inside the node serialization object.
        // It may have changed if a node would already exist in the group with the same script-name.
        // This kind of conflict may only occur in the top-level graph that we are restoring: sub-graphs are created entirely so script-names should remain
        // the same between the serilization object and the created node.
        foundNode->restoreKnobsLinks(**it, allCreatedNodes ? *allCreatedNodes : std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >());

        NodeGroupPtr isGroup = toNodeGroup(foundNode->getEffectInstance());
        if (isGroup) {

            // For sub-groupe, we don't have the list of created nodes, and their serialization list, but we should not need it:
            // It's only the top-level group that we create that may have conflicts with script-names, sub-groups are conflict free since
            // we just created them.
            restoreLinksRecursive(isGroup, (*it)->_children, 0);
        }
    }
}

bool
Project::restoreGroupFromSerialization(const SERIALIZATION_NAMESPACE::NodeSerializationList & serializedNodes,
                                       const NodeCollectionPtr& group,
                                       std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >* createdNodesOut)
{
    bool mustShowErrorsLog = false;

    NodeGroupPtr isGrp = toNodeGroup(group);

    QString groupName;
    if (isGrp) {
        groupName = QString::fromUtf8( isGrp->getNode()->getLabel().c_str() );
    } else {
        groupName = tr("top-level");
    }
    group->getApplication()->updateProjectLoadStatus( tr("Creating nodes in group: %1").arg(groupName) );


    std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > > createdNodes;



    // Loop over all node serialization and create them first
    for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {

        NodePtr node = appPTR->createNodeForProjectLoading(*it, group);
        if (!node) {
            QString text( tr("ERROR: The node %1 version %2.%3"
                             " was found in the script but does not"
                             " exist in the loaded plug-ins.")
                         .arg( QString::fromUtf8( (*it)->_pluginID.c_str() ) )
                         .arg((*it)->_pluginMajorVersion).arg((*it)->_pluginMinorVersion) );
            appPTR->writeToErrorLog_mt_safe(tr("Project"), QDateTime::currentDateTime(), text);
            mustShowErrorsLog = true;
            continue;
        } else {
            if (node->getPluginID() == PLUGINID_NATRON_STUB) {
                // If the node could not be created and we made a stub instead, warn the user
                QString text( tr("WARNING: The node %1 (%2 version %3.%4) "
                                 "was found in the script but the plug-in could not be found. It has been replaced by a pass-through node instead.")
                             .arg( QString::fromUtf8( (*it)->_nodeScriptName.c_str() ) )
                             .arg( QString::fromUtf8( (*it)->_pluginID.c_str() ) )
                             .arg((*it)->_pluginMajorVersion)
                             .arg((*it)->_pluginMinorVersion));
                appPTR->writeToErrorLog_mt_safe(tr("Project"), QDateTime::currentDateTime(), text);
                mustShowErrorsLog = true;
            } else if ( (*it)->_pluginMajorVersion != -1 && (node->getMajorVersion() != (int)(*it)->_pluginMajorVersion) ) {
                // If the node has a IOContainer don't do this check: when loading older projects that had a
                // ReadOIIO node for example in version 2, we would now create a new Read meta-node with version 1 instead
                QString text( tr("WARNING: The node %1 (%2 version %3.%4) "
                                 "was found in the script but was loaded "
                                 "with version %5.%6 instead.")
                             .arg( QString::fromUtf8( (*it)->_nodeScriptName.c_str() ) )
                             .arg( QString::fromUtf8( (*it)->_pluginID.c_str() ) )
                             .arg((*it)->_pluginMajorVersion)
                             .arg((*it)->_pluginMinorVersion)
                             .arg( node->getPlugin()->getProperty<unsigned int>(kNatronPluginPropVersion, 0) )
                             .arg( node->getPlugin()->getProperty<unsigned int>(kNatronPluginPropVersion, 1) ) );
                appPTR->writeToErrorLog_mt_safe(tr("Project"), QDateTime::currentDateTime(), text);
            }
        }

        assert(node);

        createdNodes.push_back(std::make_pair(node, *it));

    } // for all node serialization


    group->getApplication()->updateProjectLoadStatus( tr("Restoring graph links in group: %1").arg(groupName) );


    // Connect the nodes together
    for (std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >::const_iterator it = createdNodes.begin(); it != createdNodes.end(); ++it) {


        // Loop over the inputs map
        // This is a map <input label, input node name>
        //
        // When loading projects before Natron 2.2, the inputs contain both masks and inputs.
        //

        restoreInputs(it->first, it->second->_inputs, createdNodes, false /*isMasks*/);

        // After Natron 2.2, masks are saved separatly
        restoreInputs(it->first, it->second->_masks, createdNodes, true /*isMasks*/);

    } // for (std::list< NodeSerializationPtr >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {


    if (createdNodesOut) {
        *createdNodesOut = createdNodes;
    }
    // If we created all sub-groups recursively, then we are in the top-level group. We may now restore all links
    if (!group->getApplication()->isCreatingNode()) {

        // Now that we created all nodes. There may be cross-graph link(s) and they can only be truely restored now.
        restoreLinksRecursive(group, serializedNodes, createdNodesOut);
    }


    return !mustShowErrorsLog;
} // ProjectPrivate::restoreGroupFromSerialization

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
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onProjectSave callback: ")
                                                              + e.what() );

            return filename;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + error);

            return filename;
        } else {
            std::string signatureError;
            signatureError.append("The on project save callback supports the following signature(s):\n");
            signatureError.append("- callback(filename,app,autoSave)");
            if (args.size() != 3) {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + signatureError);

                return filename;
            }
            if ( (args[0] != "filename") || (args[1] != "app") || (args[2] != "autoSave") ) {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + signatureError);

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
    std::string onProjectClose = _publicInterface->getOnProjectCloseCB();

    if ( !onProjectClose.empty() ) {
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(onProjectClose, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onProjectClose callback: ")
                                                              + e.what() );

            return;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The on project close callback supports the following signature(s):\n");
        signatureError.append("- callback(app)");
        if (args.size() != 1) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + signatureError);

            return;
        }
        if (args[0] != "app") {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + signatureError);

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
            _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onProjectLoaded callback: ")
                                                              + e.what() );

            return;
        }

        if ( !error.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + error);

            return;
        }

        std::string signatureError;
        signatureError.append("The on  project loaded callback supports the following signature(s):\n");
        signatureError.append("- callback(app)");
        if (args.size() != 1) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + signatureError);

            return;
        }
        if (args[0] != "app") {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + signatureError);

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

NATRON_NAMESPACE_EXIT;
