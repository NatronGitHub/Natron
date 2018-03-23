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

#include "NodeGroupSerialization.h"

#include <cassert>
#include <stdexcept>

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>

#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Settings.h"
#include "Engine/AppInstance.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoLayer.h"
#include "Engine/ViewerInstance.h"

#include <SequenceParsing.h> // for removePath

NATRON_NAMESPACE_ENTER

void
NodeCollectionSerialization::initialize(const NodeCollection& group)
{
    NodesList nodes;

    group.getActiveNodes(&nodes);

    _serializedNodes.clear();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( !(*it)->getParentMultiInstance() && (*it)->isPartOfProject() ) {
            boost::shared_ptr<NodeSerialization> state = boost::make_shared<NodeSerialization>(*it);
            _serializedNodes.push_back(state);
        }
    }
}

static QString lookForFileRecursively(const QString& dirPath, const QString& filenameUnPathed)
{
    QDir d(dirPath);
    if (!d.exists()) {
        return QString();
    }
    if (d.exists(filenameUnPathed)) {
        return d.absoluteFilePath(filenameUnPathed);
    }
    QStringList subDirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (QStringList::const_iterator it = subDirs.begin(); it != subDirs.end(); ++it) {
        QString ret = lookForFileRecursively(*it, filenameUnPathed);
        if (!ret.isEmpty()) {
            return ret;
        }
    }
    return QString();

}

bool
NodeCollectionSerialization::restoreFromSerialization(const std::list< boost::shared_ptr<NodeSerialization> > & serializedNodes,
                                                      const boost::shared_ptr<NodeCollection>& group,
                                                      bool createNodes,
                                                      std::map<std::string, bool>* moduleUpdatesProcessed)
{
    bool mustShowErrorsLog = false;
    NodeGroup* isNodeGroup = dynamic_cast<NodeGroup*>( group.get() );
    QString groupName;

    if (isNodeGroup) {
        groupName = QString::fromUtf8( isNodeGroup->getNode()->getLabel().c_str() );
    } else {
        groupName = tr("top-level");
    }
    AppInstPtr appInst = group->getApplication();
    if (!appInst) {
        return !mustShowErrorsLog;
    }
    appInst->updateProjectLoadStatus( tr("Creating nodes in group: %1").arg(groupName) );

    ///If a parent of a multi-instance node doesn't exist anymore but the children do, we must recreate the parent.
    ///Problem: we have lost the nodes connections. To do so we restore them using the serialization of a child.
    ///This map contains all the parents that must be reconnected and an iterator to the child serialization
    std::map<NodePtr, std::list<boost::shared_ptr<NodeSerialization> >::const_iterator > parentsToReconnect;
    std::list< boost::shared_ptr<NodeSerialization> > multiInstancesToRecurse;
    std::map<NodePtr, boost::shared_ptr<NodeSerialization> > createdNodes;
    for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {
        std::string pluginID = (*it)->getPluginID();

        if ( appPTR->isBackground() && ( (pluginID == PLUGINID_NATRON_VIEWER) || (pluginID == "Viewer") ) ) {
            //if the node is a viewer, don't try to load it in background mode
            continue;
        }

        ///If the node is a multiinstance child find in all the serialized nodes if the parent exists.
        ///If not, create it

        if ( !(*it)->getMultiInstanceParentName().empty() ) {
            bool foundParent = false;
            for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it2 = serializedNodes.begin();
                 it2 != serializedNodes.end(); ++it2) {
                if ( (*it2)->getNodeScriptName() == (*it)->getMultiInstanceParentName() ) {
                    foundParent = true;
                    break;
                }
            }
            if (!foundParent) {
                ///Maybe it was created so far by another child who created it so look into the nodes

                NodePtr parent = group->getNodeByName( (*it)->getMultiInstanceParentName() );
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
                    
                    NodePtr parent = appInst->createNode(args);
                    try {
                        parent->setScriptName( (*it)->getMultiInstanceParentName().c_str() );
                    } catch (...) {
                    }

                    parentsToReconnect.insert( std::make_pair(parent, it) );
                }
            }
        } // if ( !(*it)->getMultiInstanceParentName().empty() ) {

        const std::string& pythonModuleAbsolutePath = (*it)->getPythonModule();
        NodePtr n;
        bool usingPythonModule = false;
        if ( !pythonModuleAbsolutePath.empty() ) {
            unsigned int savedPythonModuleVersion = (*it)->getPythonModuleVersion();
            QString qPyModulePath = QString::fromUtf8( pythonModuleAbsolutePath.c_str() );
            //Workaround a bug introduced in Natron where we were not saving the .py extension
            if ( !qPyModulePath.endsWith( QString::fromUtf8(".py") ) ) {
                qPyModulePath.append( QString::fromUtf8(".py") );
            }
            // The path that has been saved in the project might not be corresponding on this computer.
            // First try with the saved path
            // Then search through all PyPlug search paths recursively for a match
            if (!QFile::exists(qPyModulePath)) {
                std::string pythonModuleUnPathed = qPyModulePath.toStdString();
                std::string s = SequenceParsing::removePath(pythonModuleUnPathed);
                Q_UNUSED(s);

                qPyModulePath.clear();
                QStringList natronPaths = appPTR->getAllNonOFXPluginsPaths();
                for (int i = 0; i < natronPaths.size(); ++i) {
                    qPyModulePath = lookForFileRecursively(natronPaths[i], QString::fromUtf8(pythonModuleAbsolutePath.c_str()));
                    if (!qPyModulePath.isEmpty()) {
                        break;
                    }
                }
            }


            //This is a python group plug-in, try to find the corresponding .py file, maybe a more recent version of the plug-in exists.
            QFileInfo pythonModuleInfo(qPyModulePath);
            if (!pythonModuleInfo.exists() || appPTR->getCurrentSettings()->isLoadFromPyPlugsEnabled() ) {
                pluginID = PLUGINID_NATRON_GROUP;
                usingPythonModule = false;
            } else {
                std::string pythonPluginID, pythonPluginLabel, pythonIcFilePath, pythonGrouping, pythonDesc;
                unsigned int pyVersion;
                QString pythonModuleName = pythonModuleInfo.fileName();
                if ( pythonModuleName.endsWith( QString::fromUtf8(".py") ) ) {
                    pythonModuleName = pythonModuleName.remove(pythonModuleName.size() - 3, 3);
                }

                std::string stdModuleName = pythonModuleName.toStdString();
                bool istoolset;
                if ( NATRON_PYTHON_NAMESPACE::getGroupInfos(pythonModuleInfo.path().toStdString() + '/', stdModuleName, &pythonPluginID, &pythonPluginLabel, &pythonIcFilePath, &pythonGrouping, &pythonDesc, &istoolset, &pyVersion) ) {
                    if (pyVersion != savedPythonModuleVersion) {
                        std::map<std::string, bool>::iterator found = moduleUpdatesProcessed->find(stdModuleName);
                        if ( found != moduleUpdatesProcessed->end() ) {
                            if (found->second) {
                                pluginID = pythonPluginID;
                                usingPythonModule = true;
                            }
                        } else {
                            StandardButtonEnum rep = Dialogs::questionDialog( ( (pyVersion > savedPythonModuleVersion) ? tr("New PyPlug version") : tr("Old PyPlug version") ).toStdString(),
                                                                              ( tr("Version %1 of PyPlug \"%2\" was found in the PyPlug search path.").arg(pyVersion).arg( QString::fromUtf8( stdModuleName.c_str() ) ).toStdString() + '\n' +
                                                                                tr("This file is using version %1.").arg(savedPythonModuleVersion).toStdString() + '\n' +
                                                                               ( (pyVersion > savedPythonModuleVersion) ? tr("Would you like to update this file to use the newer version?") : tr("Would you like to update this file to use the older version?") ).toStdString() + '\n' +
                                                                               tr("If you click 'No', the PyPlyg found in this file will be used instead.").toStdString() ),
                                                                              false,
                                                                              StandardButtons(eStandardButtonYes | eStandardButtonNo) );
                            if (rep == eStandardButtonYes) {
                                pluginID = pythonPluginID;
                                usingPythonModule = true;
                            } else {
                                pluginID = PLUGINID_NATRON_GROUP;
                                usingPythonModule = false;
                            }
                            moduleUpdatesProcessed->insert( std::make_pair(stdModuleName, rep == eStandardButtonYes) );
                        }
                    } else {
                        pluginID = pythonPluginID;
                        usingPythonModule = true;
                    }
                }
            }
        } // if (!pythonModuleAbsolutePath.empty()) {

        if (!createNodes) {
            ///We are in the case where we loaded a PyPlug: it probably created all the nodes in the group already but didn't
            ///load their serialization
            n = group->getNodeByName( (*it)->getNodeScriptName() );
        }

        int majorVersion, minorVersion;
        if (usingPythonModule) {
            //We already asked the user whether he/she wanted to load a newer version of the PyPlug, let the loadNode function accept it
            majorVersion = -1;
            minorVersion = -1;
        } else {
            majorVersion = (*it)->getPluginMajorVersion();
            minorVersion = (*it)->getPluginMinorVersion();
        }

        if (!n) {
            CreateNodeArgs args(pluginID, group);
            args.setProperty<int>(kCreateNodeArgsPropPluginVersion, majorVersion, 0);
            args.setProperty<int>(kCreateNodeArgsPropPluginVersion, minorVersion, 1);
            args.setProperty<boost::shared_ptr<NodeSerialization> >(kCreateNodeArgsPropNodeSerialization, *it);
            args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
            args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true); // also load deprecated plugins
            if (!(*it)->getMultiInstanceParentName().empty()) {
                args.setProperty<std::string>(kCreateNodeArgsPropMultiInstanceParentName, (*it)->getMultiInstanceParentName());
            }
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            n = appInst->createNode(args);
        }
        if (!n) {
            QString text( tr("ERROR: The node %1 version %2.%3"
                             " was found in the script but does not"
                             " exist in the loaded plug-ins.")
                          .arg( QString::fromUtf8( pluginID.c_str() ) )
                          .arg(majorVersion).arg(minorVersion) );
            appPTR->writeToErrorLog_mt_safe(tr("Project"), QDateTime::currentDateTime(), text);
            mustShowErrorsLog = true;
            continue;
        } else {
            if ( !usingPythonModule && n->getPlugin() &&
                 (n->getPlugin()->getMajorVersion() != (int)majorVersion) && ( n->getPluginID() == pluginID) ) {
                // If the node has a IOContainer don't do this check: when loading older projects that had a
                // ReadOIIO node for example in version 2, we would now create a new Read meta-node with version 1 instead
                QString text( tr("WARNING: The node %1 (%2) version %3.%4 "
                                 "was found in the script but was loaded "
                                 "with version %5.%6 instead.")
                              .arg( QString::fromUtf8( (*it)->getNodeScriptName().c_str() ) )
                              .arg( QString::fromUtf8( pluginID.c_str() ) )
                              .arg(majorVersion)
                              .arg(minorVersion)
                              .arg( n->getPlugin()->getMajorVersion() )
                              .arg( n->getPlugin()->getMinorVersion() ) );
                appPTR->writeToErrorLog_mt_safe(tr("Project"), QDateTime::currentDateTime(), text);
                mustShowErrorsLog = true;
            }
        }
        if (!createNodes && n) {
            // If we created the node using a PyPlug, deserialize the project too to override any modification made by the user.
            n->loadKnobs(**it);
        }
        assert(n);

        createdNodes[n] = *it;

        const std::list<boost::shared_ptr<NodeSerialization> >& children = (*it)->getNodesCollection();
        if ( !children.empty() && !usingPythonModule) {
            NodeGroup* isGrp = n->isEffectGroup();
            if (isGrp) {
                EffectInstancePtr sharedEffect = isGrp->shared_from_this();
                boost::shared_ptr<NodeGroup> sharedGrp = boost::dynamic_pointer_cast<NodeGroup>(sharedEffect);
                NodeCollectionSerialization::restoreFromSerialization(children, sharedGrp, !usingPythonModule, moduleUpdatesProcessed);
            } else {
                ///For multi-instances, wait for the group to be entirely created then load the sub-tracks in a separate loop.
                assert( n->isMultiInstance() );
                multiInstancesToRecurse.push_back(*it);
            }
        }
    } // for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {

    for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = multiInstancesToRecurse.begin(); it != multiInstancesToRecurse.end(); ++it) {
        NodeCollectionSerialization::restoreFromSerialization( (*it)->getNodesCollection(), group, true, moduleUpdatesProcessed );
    }


    appInst->updateProjectLoadStatus( tr("Restoring graph links in group: %1").arg(groupName) );


    /// Connect the nodes together
    for (std::map<NodePtr, boost::shared_ptr<NodeSerialization> >::const_iterator it = createdNodes.begin(); it != createdNodes.end(); ++it) {
        if ( appPTR->isBackground() && ( it->first->isEffectViewer() ) ) {
            //ignore viewers on background mode
            continue;
        }

        ///for all nodes that are part of a multi-instance, fetch the main instance node pointer
        const std::string & parentName = it->second->getMultiInstanceParentName();
        if ( !parentName.empty() ) {
            it->first->fetchParentMultiInstancePointer();
            //Do not restore connections as we just use the ones of the parent anyway
            continue;
        }

        ///restore slave/master link if any
        const std::string & masterNodeName = it->second->getMasterNodeName();
        if ( !masterNodeName.empty() ) {
            ///find such a node
            NodePtr masterNode = it->first->getApp()->getNodeByFullySpecifiedName(masterNodeName);

            if (!masterNode) {
                appPTR->writeToErrorLog_mt_safe( tr("Project"), QDateTime::currentDateTime(),
                                                 tr("Cannot restore the link between %1 and %2.")
                                                 .arg( QString::fromUtf8( it->second->getNodeScriptName().c_str() ) )
                                                 .arg( QString::fromUtf8( masterNodeName.c_str() ) ) );
                mustShowErrorsLog = true;
            } else {
                it->first->getEffectInstance()->slaveAllKnobs( masterNode->getEffectInstance().get(), true );
            }
        }

        if ( !parentName.empty() ) {
            //The parent will have connection mades for its children
            continue;
        }

        const std::vector<std::string> & oldInputs = it->second->getOldInputs();
        if ( !oldInputs.empty() ) {
            /*
             * Prior to Natron v2 OpenFX effects had their inputs reversed internally
             */
            bool isOfxEffect = it->first->isOpenFXNode();

            for (U32 j = 0; j < oldInputs.size(); ++j) {
                if ( !oldInputs[j].empty() && !group->connectNodes(isOfxEffect ? oldInputs.size() - 1 - j : j, oldInputs[j], it->first) ) {
                    if (createNodes) {
                        qDebug() << tr("Failed to connect node %1 to %2 (this is normal if loading a PyPlug)")
                                    .arg( QString::fromUtf8( it->second->getNodeScriptName().c_str() ) )
                                    .arg( QString::fromUtf8( oldInputs[j].c_str() ) );
                    }
                }
            }
        } else {
            const std::map<std::string, std::string>& inputs = it->second->getInputs();
            for (std::map<std::string, std::string>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
                if ( it2->second.empty() ) {
                    continue;
                }
                int index = it->first->getInputNumberFromLabel(it2->first);
                if (index == -1) {
                    appPTR->writeToErrorLog_mt_safe( tr("Project"), QDateTime::currentDateTime(),
                                                     tr("Could not find input named %1")
                                                     .arg( QString::fromUtf8( it2->first.c_str() ) ) );
                    continue;
                }
                if ( !it2->second.empty() && !group->connectNodes(index, it2->second, it->first) ) {
                    if (createNodes) {
                        qDebug() << tr("Failed to connect node %1 to %2 (this is normal if loading a PyPlug)")
                                    .arg( QString::fromUtf8( it->second->getNodeScriptName().c_str() ) )
                                    .arg( QString::fromUtf8( it2->second.c_str() ) );
                    }
                }
            }
        }
    } // for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {

    ///Now that the graph is setup, restore expressions
    NodesList nodes = group->getNodes();
    if (isNodeGroup) {
        nodes.push_back( isNodeGroup->getNode() );
    }

    {
        std::map<std::string, std::string> oldNewScriptNamesMapping;
        for (std::map<NodePtr, boost::shared_ptr<NodeSerialization> >::const_iterator it = createdNodes.begin(); it != createdNodes.end(); ++it) {
            if ( appPTR->isBackground() && ( it->first->isEffectViewer() ) ) {
                //ignore viewers on background mode
                continue;
            }
            it->first->restoreKnobsLinks(*it->second, nodes, oldNewScriptNamesMapping);
        }
    }

    ///Also reconnect parents of multiinstance nodes that were created on the fly
    for (std::map<NodePtr, std::list<boost::shared_ptr<NodeSerialization> >::const_iterator >::const_iterator
         it = parentsToReconnect.begin(); it != parentsToReconnect.end(); ++it) {
        const std::vector<std::string> & oldInputs = (*it->second)->getOldInputs();
        if ( !oldInputs.empty() ) {
            /*
             * Prior to Natron v2 OpenFX effects had their inputs reversed internally
             */
            bool isOfxEffect = it->first->isOpenFXNode();

            for (U32 j = 0; j < oldInputs.size(); ++j) {
                if ( !oldInputs[j].empty() && !group->connectNodes(isOfxEffect ? oldInputs.size() - 1 - j : j, oldInputs[j], it->first) ) {
                    if (createNodes) {
                        qDebug() << tr("Failed to connect node %1 to %2 (this is normal if loading a PyPlug)")
                                    .arg( QString::fromUtf8( it->first->getPluginLabel().c_str() ) )
                                    .arg( QString::fromUtf8( oldInputs[j].c_str() ) );
                    }
                }
            }
        } else {
            const std::map<std::string, std::string>& inputs = (*it->second)->getInputs();
            for (std::map<std::string, std::string>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
                if ( it2->second.empty() ) {
                    continue;
                }
                int index = it->first->getInputNumberFromLabel(it2->first);
                if (index == -1) {
                    appPTR->writeToErrorLog_mt_safe( tr("Project"), QDateTime::currentDateTime(),
                                                     tr("Could not find input named %1").arg( QString::fromUtf8( it2->first.c_str() ) ) );
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
    }

    return !mustShowErrorsLog;
} // NodeCollectionSerialization::restoreFromSerialization

NATRON_NAMESPACE_EXIT
