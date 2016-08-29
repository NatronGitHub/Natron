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

#include "ProjectPrivate.h"

#include <list>
#include <cassert>
#include <stdexcept>

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
    , projectCreationTime(ageSinceLastSave)
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


void
ProjectPrivate::checkForPyPlugNewVersion(const std::string& pythonModuleName,
                                         bool moduleNameIsScriptPath,
                                         unsigned int savedPythonModuleVersion,
                                         bool* usingPythonModule,
                                         std::string* pluginID,
                                         std::map<std::string, bool>* moduleUpdatesProcessed)
{
    
    *usingPythonModule = false;

    std::string moduleName;
    if (moduleNameIsScriptPath) {
        // Before Natron 2.2, we saved the absolute file path of the Python script instead of just the module name
        QString qPyModulePath = QString::fromUtf8( pythonModuleName.c_str() );
        QtCompat::removeFileExtension(qPyModulePath);
        int foundSlash = qPyModulePath.lastIndexOf(QChar::fromAscii('/'));
        if (foundSlash != -1) {
            moduleName = qPyModulePath.mid(foundSlash + 1).toStdString();
        }
    }

    std::string pythonPluginID, pluginLabel, iconFilePath, pluginGrouping, description, pluginPath;
    unsigned int version;
    bool istoolset;

    if ( !NATRON_PYTHON_NAMESPACE::getGroupInfos(pythonModuleName, &pythonPluginID, &pluginLabel, &iconFilePath, &pluginGrouping, &description, &pluginPath, &istoolset, &version) ) {
        return;
    }

    if (version != savedPythonModuleVersion) {
        std::map<std::string, bool>::iterator found = moduleUpdatesProcessed->find(moduleName);
        if ( found != moduleUpdatesProcessed->end() ) {
            if (found->second) {
                *pluginID = pythonPluginID;
                *usingPythonModule = true;
            }
        } else {
            StandardButtonEnum rep = Dialogs::questionDialog( tr("New PyPlug version").toStdString(),
                                                             ( tr("Version %1 of PyPlug \"%2\" was found.").arg(version).arg( QString::fromUtf8( moduleName.c_str() ) ).toStdString() + '\n' +
                                                              tr("You are currently using version %1.").arg(savedPythonModuleVersion).toStdString() + '\n' +
                                                              tr("Would you like to update your script to use the newer version?").toStdString() ),
                                                             false,
                                                             StandardButtons(eStandardButtonYes | eStandardButtonNo) );
            if (rep == eStandardButtonYes) {
                *pluginID = pythonPluginID;
                *usingPythonModule = true;
            } else {
                *pluginID = PLUGINID_NATRON_GROUP;
                *usingPythonModule = false;
            }
            moduleUpdatesProcessed->insert( std::make_pair(moduleName, rep == eStandardButtonYes) );
        }
    } else {
        *pluginID = pythonPluginID;
        *usingPythonModule = true;
    }

} // checkForPyPlugNewVersion

bool
Project::restoreGroupFromSerialization(const SERIALIZATION_NAMESPACE::NodeSerializationList & serializedNodes,
                                       const NodeCollectionPtr& group,
                                       bool createNodes,
                                       std::map<std::string, bool>* moduleUpdatesProcessed)
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


    // Deprecated: Multi-instances are deprecated in Natron 2.1 and should not exist afterwards
    // If a parent of a multi-instance node doesn't exist anymore but the children do, we must recreate the parent.
    // Problem: we have lost the nodes connections. To do so we restore them using the serialization of a child.
    // This map contains all the parents that must be reconnected and an iterator to the child serialization
    std::map<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator > parentsToReconnect;
    SERIALIZATION_NAMESPACE::NodeSerializationList multiInstancesToRecurse;


    // Loop over all node serialization and create them first
    std::map<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > createdNodes;
    for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {

        std::string pluginID = (*it)->_pluginID;
        /*if ( appPTR->isBackground() && ( (pluginID == PLUGINID_NATRON_VIEWER_GROUP) || (pluginID == "Viewer") ) ) {
            // If the node is a viewer, don't try to load it in background mode
            continue;
        }*/

        // If the node is a multiinstance child find in all the serialized nodes if the parent exists.
        // If not, create it
        // Multi-instances are deprecated in Natron 2.1 and should not exist afterwards
        if ( !(*it)->_multiInstanceParentName.empty() ) {
            bool foundParent = false;
            for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it2 = serializedNodes.begin();
                 it2 != serializedNodes.end(); ++it2) {
                if ( (*it2)->_nodeScriptName == (*it)->_multiInstanceParentName ) {
                    foundParent = true;
                    break;
                }
            }
            if (!foundParent) {
                ///Maybe it was created so far by another child who created it so look into the nodes

                NodePtr parent = group->getNodeByName( (*it)->_multiInstanceParentName );
                if (parent) {
                    foundParent = true;
                }
                ///Create the parent
                if (!foundParent) {
                    CreateNodeArgs args(pluginID, group);
                    args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
                    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
                    args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);

                    NodePtr parent = group->getApplication()->createNode(args);
                    try {
                        parent->setScriptName((*it)->_multiInstanceParentName);
                    } catch (...) {
                    }

                    parentsToReconnect.insert( std::make_pair(parent, it) );
                }
            }
        } // if ( !(*it)->getMultiInstanceParentName().empty() ) {

        // If the node is a PyPlug, load the PyPlug but if the version loaded by Natron is different ask the user whehter he/she
        // wants to update the PyPlug, otherwise load it as a group and keep it as it is in the project file
        const std::string& pythonModuleName = (*it)->_pythonModule;
        NodePtr node;
        bool usingPythonModule = false;
        if ( !pythonModuleName.empty() ) {
            unsigned int savedPythonModuleVersion = (*it)->_pythonModuleVersion;
#ifndef NATRON_BOOST_SERIALIZATION_COMPAT
            bool moduleNameIsScriptPath = false;
#else
            bool moduleNameIsScriptPath = (*it)->_boostSerializationClassVersion < NODE_SERIALIZATION_CHANGE_PYTHON_MODULE_TO_ONLY_NAME;
#endif
            ProjectPrivate::checkForPyPlugNewVersion(pythonModuleName, moduleNameIsScriptPath, savedPythonModuleVersion, &usingPythonModule, &pluginID, moduleUpdatesProcessed);

        } // if (!pythonModuleAbsolutePath.empty()) {

        if (!createNodes) {
            // We are in the case where we loaded a PyPlug: it probably created all the nodes in the group already but didn't
            // load their serialization
            node = group->getNodeByName( (*it)->_nodeScriptName );
        }

        int majorVersion, minorVersion;
        if (usingPythonModule) {
            //We already asked the user whether he/she wanted to load a newer version of the PyPlug, let the loadNode function accept it
            majorVersion = -1;
            minorVersion = -1;
        } else {
            // For regular C++ plug-ins, we do not have a backup, we forced to let the user use the plug-ins that were anyway loaded with
            // the application
            majorVersion = (*it)->_pluginMajorVersion;
            minorVersion = (*it)->_pluginMinorVersion;
        }

        if (!node) {
            CreateNodeArgs args(pluginID, group);
            args.setProperty<int>(kCreateNodeArgsPropPluginVersion, majorVersion, 0);
            args.setProperty<int>(kCreateNodeArgsPropPluginVersion, minorVersion, 1);
            args.setProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr >(kCreateNodeArgsPropNodeSerialization, *it);
            args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
            if (!(*it)->_multiInstanceParentName.empty()) {
                args.setProperty<std::string>(kCreateNodeArgsPropMultiInstanceParentName, (*it)->_multiInstanceParentName);
            }
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
            node = group->getApplication()->createNode(args);
        }
        if (!node) {
            QString text( tr("ERROR: The node %1 version %2.%3"
                             " was found in the script but does not"
                             " exist in the loaded plug-ins.")
                         .arg( QString::fromUtf8( pluginID.c_str() ) )
                         .arg(majorVersion).arg(minorVersion) );
            appPTR->writeToErrorLog_mt_safe(tr("Project"), QDateTime::currentDateTime(), text);
            mustShowErrorsLog = true;
            continue;
        } else {
            if ( !usingPythonModule && node->getPlugin() &&
                (node->getPlugin()->getMajorVersion() != (int)majorVersion) && ( node->getPluginID() == pluginID) ) {
                // If the node has a IOContainer don't do this check: when loading older projects that had a
                // ReadOIIO node for example in version 2, we would now create a new Read meta-node with version 1 instead
                QString text( tr("WARNING: The node %1 (%2) version %3.%4 "
                                 "was found in the script but was loaded "
                                 "with version %5.%6 instead.")
                             .arg( QString::fromUtf8( (*it)->_nodeScriptName.c_str() ) )
                             .arg( QString::fromUtf8( pluginID.c_str() ) )
                             .arg(majorVersion)
                             .arg(minorVersion)
                             .arg( node->getPlugin()->getMajorVersion() )
                             .arg( node->getPlugin()->getMinorVersion() ) );
                appPTR->writeToErrorLog_mt_safe(tr("Project"), QDateTime::currentDateTime(), text);
                mustShowErrorsLog = true;
            }
        }
        if (!createNodes && node) {
            // If we created the node using a PyPlug, deserialize the project too to override any modification made by the user.
            node->fromSerialization(**it);
        }
        assert(node);

        createdNodes[node] = *it;

        // For group, create children
        const SERIALIZATION_NAMESPACE::NodeSerializationList& children = (*it)->_children;
        if ( !children.empty() && !usingPythonModule) {
            NodeGroupPtr isGrp = node->isEffectNodeGroup();
            if (isGrp) {
                EffectInstancePtr sharedEffect = isGrp->shared_from_this();
                NodeGroupPtr sharedGrp = toNodeGroup(sharedEffect);
                Project::restoreGroupFromSerialization(children, sharedGrp, !usingPythonModule, moduleUpdatesProcessed);
            } else {
                // For multi-instances, wait for the group to be entirely created then load the sub-tracks in a separate loop.
                assert( node->isMultiInstance() );
                multiInstancesToRecurse.push_back(*it);
            }
        }
    } // for all node serialization

    // Deprecated: Multi-instances are deprecated in Natron 2.1 and should not exist afterwards
    for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = multiInstancesToRecurse.begin(); it != multiInstancesToRecurse.end(); ++it) {
        Project::restoreGroupFromSerialization( (*it)->_children, group, true, moduleUpdatesProcessed );
    }


    group->getApplication()->updateProjectLoadStatus( tr("Restoring graph links in group: %1").arg(groupName) );


    // Connect the nodes together
    for (std::map<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr >::const_iterator it = createdNodes.begin(); it != createdNodes.end(); ++it) {
        /*if ( appPTR->isBackground() && ( it->first->isEffectViewerInstance() ) ) {
            //ignore viewers on background mode
            continue;
        }*/

        // Deprecated: For all nodes that are part of a multi-instance, fetch the main instance node pointer
        const std::string & parentName = it->second->_multiInstanceParentName;
        if ( !parentName.empty() ) {
            it->first->fetchParentMultiInstancePointer();
            //Do not restore connections as we just use the ones of the parent anyway
            continue;
        }

        // Restore clone state if this node is a clone
        const std::string & masterNodeName = it->second->_masterNodeFullyQualifiedScriptName;
        if ( !masterNodeName.empty() ) {
            // Find master node
            NodePtr masterNode = it->first->getApp()->getNodeByFullySpecifiedName(masterNodeName);

            if (!masterNode) {
                appPTR->writeToErrorLog_mt_safe( tr("Project"), QDateTime::currentDateTime(),
                                                tr("Cannot restore the link between %1 and %2.")
                                                .arg( QString::fromUtf8( it->second->_nodeScriptName.c_str() ) )
                                                .arg( QString::fromUtf8( masterNodeName.c_str() ) ) );
                mustShowErrorsLog = true;
            } else {
                it->first->getEffectInstance()->slaveAllKnobs( masterNode->getEffectInstance(), true );
            }
        }

        if ( !parentName.empty() ) {
            //The parent will have connection mades for its children
            continue;
        }

        // Loop over the inputs map
        // This is a map <input label, input node name>
        const std::map<std::string, std::string>& inputs = it->second->_inputs;
        for (std::map<std::string, std::string>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
            if ( it2->second.empty() ) {
                continue;
            }
            int index = it->first->getInputNumberFromLabel(it2->first);
            if (index == -1) {
                // Prior to Natron 1.1, input names were not serialized, try to convert to index
                bool ok;
                index = QString::fromUtf8(it2->first.c_str()).toInt(&ok);
                if (!ok) {
                    index = -1;
                }
                if (index == -1) {
                    appPTR->writeToErrorLog_mt_safe( tr("Project"), QDateTime::currentDateTime(),
                                                    tr("Could not find input named %1")
                                                    .arg( QString::fromUtf8( it2->first.c_str() ) ) );
                }
                continue;
            }
            if ( !it2->second.empty() && !group->connectNodes(index, it2->second, it->first) ) {
                if (createNodes) {
                    qDebug() << tr("Failed to connect node %1 to %2 (this is normal if loading a PyPlug)")
                    .arg( QString::fromUtf8( it->second->_nodeScriptName.c_str() ) )
                    .arg( QString::fromUtf8( it2->second.c_str() ) );
                }
            }
        }

    } // for (std::list< NodeSerializationPtr >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {

    // Now that the graph is setup, restore expressions and slave/master links for knobs
    NodesList nodes = group->getNodes();
    if (isGrp) {
        nodes.push_back( isGrp->getNode() );
    }

    {
        std::map<std::string, std::string> oldNewScriptNamesMapping;
        for (std::map<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr >::const_iterator it = createdNodes.begin(); it != createdNodes.end(); ++it) {
            if ( appPTR->isBackground() && ( it->first->isEffectViewerInstance() ) ) {
                //ignore viewers on background mode
                continue;
            }
            it->first->restoreKnobsLinks(*it->second, nodes, oldNewScriptNamesMapping);
        }
    }

    // Viewers are specials and needs to be notified once all connections have been made in their container group in order
    // to correctly link the internal nodes
    for (std::map<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr >::const_iterator it = createdNodes.begin(); it != createdNodes.end(); ++it) {
        ViewerNodePtr isViewer = it->first->isEffectViewerNode();
        if (isViewer) {
            try {
                isViewer->onContainerGroupLoaded();
            } catch (...) {
                continue;
            }
        }
    }

    // Deprecated: Also reconnect parents of multiinstance nodes that were created on the fly
    for (std::map<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator >::const_iterator
         it = parentsToReconnect.begin(); it != parentsToReconnect.end(); ++it) {

        const std::map<std::string, std::string>& inputs = (*it->second)->_inputs;
        for (std::map<std::string, std::string>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
            if ( it2->second.empty() ) {
                continue;
            }
            int index = it->first->getInputNumberFromLabel(it2->first);
            if (index == -1) {
                // Prior to Natron 1.1, input names were not serialized, try to convert to index
                bool ok;
                index = QString::fromUtf8(it2->first.c_str()).toInt(&ok);
                if (!ok) {
                    index = -1;
                }
                if (index == -1) {
                    appPTR->writeToErrorLog_mt_safe( tr("Project"), QDateTime::currentDateTime(),
                                                    tr("Could not find input named %1").arg( QString::fromUtf8( it2->first.c_str() ) ) );
                }
                continue;
            }
            if ( !it2->second.empty() && !group->connectNodes(index, it2->second, it->first) ) {
                if (createNodes) {
                    qDebug() << tr("Failed to connect node %1 to %2 (this is normal if loading a PyPlug)")
                    .arg( QString::fromUtf8( it->first->getPluginLabel().c_str() ) )
                    .arg( QString::fromUtf8( it2->second.c_str() ) );
                }
            }
        }

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
