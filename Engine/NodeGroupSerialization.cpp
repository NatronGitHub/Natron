/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <QFileInfo>

#include "Engine/AppManager.h"
#include "Engine/Settings.h"
#include "Engine/AppInstance.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoLayer.h"
#include "Engine/ViewerInstance.h"
#include <SequenceParsing.h>

void
NodeCollectionSerialization::initialize(const NodeCollection& group)
{
    NodeList nodes;
    group.getActiveNodes(&nodes);
    
    _serializedNodes.clear();
    
    for (NodeList::iterator it = nodes.begin(); it != nodes.end() ; ++it) {
        if (!(*it)->getParentMultiInstance()) {
            boost::shared_ptr<NodeSerialization> state(new NodeSerialization(*it));
            _serializedNodes.push_back(state);
        }
    }
}

bool
NodeCollectionSerialization::restoreFromSerialization(const std::list< boost::shared_ptr<NodeSerialization> > & serializedNodes,
                                                      const boost::shared_ptr<NodeCollection>& group,
                                                      bool createNodes,
                                                      std::map<std::string,bool>* moduleUpdatesProcessed,
                                                      bool* hasProjectAWriter)
{
    

    bool mustShowErrorsLog = false;
    
    NodeGroup* isNodeGroup = dynamic_cast<NodeGroup*>(group.get());
    QString groupName;
    if (isNodeGroup) {
        groupName = isNodeGroup->getNode()->getLabel().c_str();
    } else {
        groupName = QObject::tr("top-level");
    }
    group->getApplication()->updateProjectLoadStatus(QObject::tr("Creating nodes in group: ") + groupName);
    
    ///If a parent of a multi-instance node doesn't exist anymore but the children do, we must recreate the parent.
    ///Problem: we have lost the nodes connections. To do so we restore them using the serialization of a child.
    ///This map contains all the parents that must be reconnected and an iterator to the child serialization
    std::map<boost::shared_ptr<Natron::Node>, std::list<boost::shared_ptr<NodeSerialization> >::const_iterator > parentsToReconnect;
    
    std::list< boost::shared_ptr<NodeSerialization> > multiInstancesToRecurse;
    
    std::map<NodePtr, boost::shared_ptr<NodeSerialization> > createdNodes;
    for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {
        
        std::string pluginID = (*it)->getPluginID();
        
        if ( appPTR->isBackground() && (pluginID == PLUGINID_NATRON_VIEWER || pluginID == "Viewer") ) {
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
                
                NodePtr parent = group->getNodeByName((*it)->getMultiInstanceParentName());
                if (parent) {
                    foundParent = true;
                }
                ///Create the parent
                if (!foundParent) {
                    boost::shared_ptr<Natron::Node> parent = group->getApplication()->createNode(CreateNodeArgs( pluginID.c_str(),
                                                                                                                "",
                                                                                                                (*it)->getPluginMajorVersion(),
                                                                                                                (*it)->getPluginMinorVersion(),
                                                                                                                true,
                                                                                                                INT_MIN,
                                                                                                                INT_MIN,
                                                                                                                false,
                                                                                                                true,
                                                                                                                false,
                                                                                                                QString(),
                                                                                                                CreateNodeArgs::DefaultValuesList(),
                                                                                                                group));
                    parent->setScriptName( (*it)->getMultiInstanceParentName().c_str() );
                    parentsToReconnect.insert( std::make_pair(parent, it) );
                }
            }
        } // if ( !(*it)->getMultiInstanceParentName().empty() ) {

        const std::string& pythonModuleAbsolutePath = (*it)->getPythonModule();
        
        boost::shared_ptr<Natron::Node> n;
        
        bool usingPythonModule = false;
        if (!pythonModuleAbsolutePath.empty()) {
            
            unsigned int savedPythonModuleVersion = (*it)->getPythonModuleVersion();
            
            QString qPyModulePath(pythonModuleAbsolutePath.c_str());
            //Workaround a bug introduced in Natron where we were not saving the .py extension
            if (!qPyModulePath.endsWith(".py")) {
                qPyModulePath.append(".py");
            }
            ///The path that has been saved in the project might not be corresponding on this computer.
            ///We need to search through all search paths for a match
            std::string pythonModuleUnPathed = qPyModulePath.toStdString();
            std::string s = SequenceParsing::removePath(pythonModuleUnPathed);
            Q_UNUSED(s);
            
            qPyModulePath.clear();
            QStringList natronPaths = appPTR->getAllNonOFXPluginsPaths();
            for (int i = 0; i < natronPaths.size(); ++i) {
                QString path = natronPaths[i];
                if (!path.endsWith("/")) {
                    path.append('/');
                }
                path.append(pythonModuleUnPathed.c_str());
                if (QFile::exists(path)) {
                    qPyModulePath = path;
                    break;
                }
            }
           
            //This is a python group plug-in, try to find the corresponding .py file, maybe a more recent version of the plug-in exists.
            QFileInfo pythonModuleInfo(qPyModulePath);
            if (pythonModuleInfo.exists() && appPTR->getCurrentSettings()->isLoadFromPyPlugsEnabled()) {
                
                std::string pythonPluginID,pythonPluginLabel,pythonIcFilePath,pythonGrouping,pythonDesc;
                unsigned int pyVersion;
                QString pythonModuleName = pythonModuleInfo.fileName();
                if (pythonModuleName.endsWith(".py")) {
                    pythonModuleName = pythonModuleName.remove(pythonModuleName.size() - 3, 3);
                }
                
                std::string stdModuleName = pythonModuleName.toStdString();
                if (getGroupInfos(pythonModuleInfo.path().toStdString() + '/', stdModuleName, &pythonPluginID, &pythonPluginLabel, &pythonIcFilePath, &pythonGrouping, &pythonDesc, &pyVersion)) {
                    
                    if (pyVersion != savedPythonModuleVersion) {
            
                        std::map<std::string,bool>::iterator found = moduleUpdatesProcessed->find(stdModuleName);
                        if (found != moduleUpdatesProcessed->end()) {
                            if (found->second) {
                                pluginID = pythonPluginID;
                                usingPythonModule = true;
                            }
                        } else {
                            
                            Natron::StandardButtonEnum rep = Natron::questionDialog(QObject::tr("New PyPlug version").toStdString()
                                                                                    , QObject::tr("A different version of ").toStdString() +
                                                                                    stdModuleName + " (" +
                                                                                    QString::number(pyVersion).toStdString() + ") " +
                                                                                    QObject::tr("was found.\n").toStdString() +
                                                                                    QObject::tr("You are currently using version ").toStdString() +
                                                                                    QString::number(savedPythonModuleVersion).toStdString() + ".\n" +
                                                                                    QObject::tr("Would you like to update your script?").toStdString()
                                                                                    , false ,
                                                                                    Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo));
                            if (rep == Natron::eStandardButtonYes) {
                                pluginID = pythonPluginID;
                                usingPythonModule = true;
                            }
                            moduleUpdatesProcessed->insert(std::make_pair(stdModuleName, rep == Natron::eStandardButtonYes));
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
            n = group->getNodeByName((*it)->getNodeScriptName());
        }
        
        int majorVersion,minorVersion;
        if (usingPythonModule) {
            //We already asked the user whether he/she wanted to load a newer version of the PyPlug, let the loadNode function accept it
            majorVersion = -1;
            minorVersion = -1;
        } else {
            majorVersion = (*it)->getPluginMajorVersion();
            minorVersion = (*it)->getPluginMinorVersion();
        }
        
        if (!n) {
            n = group->getApplication()->loadNode( LoadNodeArgs(pluginID.c_str()
                                                                ,(*it)->getMultiInstanceParentName()
                                                                ,majorVersion
                                                                ,minorVersion,it->get(),false,group) );
        }
        if (!n) {
            QString text( QObject::tr("ERROR: The node ") );
            text.append( pluginID.c_str() );
            text.append(QObject::tr(" version %1.%2").arg(majorVersion).arg(minorVersion));
            text.append( QObject::tr(" was found in the script but does not"
                                     " exist in the loaded plug-ins.") );
            appPTR->writeToOfxLog_mt_safe(text);
            mustShowErrorsLog = true;
            continue;
        } else {
            if (!usingPythonModule && n->getPlugin() && n->getPlugin()->getMajorVersion() != (int)majorVersion) {
                QString text( QObject::tr("WARNING: The node ") );
                text.append((*it)->getNodeScriptName().c_str());
                text.append(" (");
                text.append( pluginID.c_str() );
                text.append(")");
                text.append(QObject::tr(" version %1.%2").arg(majorVersion).arg(minorVersion));
                text.append( QObject::tr(" was found in the script but was loaded"
                                         " with version %3.%4 instead").arg(n->getPlugin()->getMajorVersion()).arg(n->getPlugin()->getMinorVersion()) );
                appPTR->writeToOfxLog_mt_safe(text);
                mustShowErrorsLog = true;

            }
        }
        assert(n);
        if ( n->isOutputNode() ) {
            *hasProjectAWriter = true;
        }
        createdNodes[n] = *it;
        
        const std::list<boost::shared_ptr<NodeSerialization> >& children = (*it)->getNodesCollection();
        if (!children.empty()) {
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(n->getLiveInstance());
            if (isGrp) {
                boost::shared_ptr<Natron::EffectInstance> sharedEffect = isGrp->shared_from_this();
                boost::shared_ptr<NodeGroup> sharedGrp = boost::dynamic_pointer_cast<NodeGroup>(sharedEffect);
                NodeCollectionSerialization::restoreFromSerialization(children, sharedGrp ,!usingPythonModule, moduleUpdatesProcessed, hasProjectAWriter);
                
            } else {
                ///For multi-instances, wait for the group to be entirely created then load the sub-tracks in a separate loop.
                assert(n->isMultiInstance());
                multiInstancesToRecurse.push_back(*it);
            }
        }
    } // for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {
    
    for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = multiInstancesToRecurse.begin(); it != multiInstancesToRecurse.end(); ++it) {
        NodeCollectionSerialization::restoreFromSerialization((*it)->getNodesCollection(), group, true, moduleUpdatesProcessed,  hasProjectAWriter);
    }
    
    
    group->getApplication()->updateProjectLoadStatus(QObject::tr("Restoring graph links in group: ") + groupName);

    
    /// Connect the nodes together
    for (std::map<NodePtr, boost::shared_ptr<NodeSerialization> >::const_iterator it = createdNodes.begin(); it != createdNodes.end(); ++it) {
        if ( appPTR->isBackground() && (dynamic_cast<ViewerInstance*>((it->first)->getLiveInstance()))) {
            //ignore viewers on background mode
            continue;
        }
        
        ///for all nodes that are part of a multi-instance, fetch the main instance node pointer
        const std::string & parentName = it->second->getMultiInstanceParentName();
        if ( !parentName.empty() ) {
            it->first->fetchParentMultiInstancePointer();
        }
        
        ///restore slave/master link if any
        const std::string & masterNodeName = it->second->getMasterNodeName();
        if ( !masterNodeName.empty() ) {
            ///find such a node
            boost::shared_ptr<Natron::Node> masterNode = it->first->getApp()->getNodeByFullySpecifiedName(masterNodeName);
            
            if (!masterNode) {
                appPTR->writeToOfxLog_mt_safe(QString("Cannot restore the link between " + QString(it->second->getNodeScriptName().c_str()) + " and " + masterNodeName.c_str()));
                mustShowErrorsLog = true;
            } else {
                it->first->getLiveInstance()->slaveAllKnobs( masterNode->getLiveInstance(), true );
            }
        }
        
        const std::vector<std::string> & oldInputs = it->second->getOldInputs();
        if (!oldInputs.empty()) {
            
            /*
             * Prior to Natron v2 OpenFX effects had their inputs reversed internally
             */
            bool isOfxEffect = it->first->isOpenFXNode();
            
            for (U32 j = 0; j < oldInputs.size(); ++j) {
                if ( !oldInputs[j].empty() && !group->connectNodes(isOfxEffect ? oldInputs.size() - 1 - j : j, oldInputs[j],it->first.get()) ) {
                    if (createNodes) {
                        qDebug() << "Failed to connect node" << it->second->getNodeScriptName().c_str() << "to" << oldInputs[j].c_str()
                        << "[This is normal if loading a PyPlug]";
                    }
                }
            }
        } else {
            const std::map<std::string,std::string>& inputs = it->second->getInputs();
            for (std::map<std::string,std::string>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
                if (it2->second.empty()) {
                    continue;
                }
                int index = it->first->getInputNumberFromLabel(it2->first);
                if (index == -1) {
                    appPTR->writeToOfxLog_mt_safe(QString("Could not find input named ") + it2->first.c_str());
                    continue;
                }
                if (!it2->second.empty() && !group->connectNodes(index, it2->second, it->first.get())) {
                    if (createNodes) {
                        qDebug() << "Failed to connect node" << it->second->getNodeScriptName().c_str() << "to" << it2->second.c_str()
                        << "[This is normal if loading a PyPlug]";
                    }
                }
            }
        }
        

    } // for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {
    
    ///Now that the graph is setup, restore expressions
    NodeList nodes = group->getNodes();
    if (isNodeGroup) {
        nodes.push_back(isNodeGroup->getNode());
    }
    for (std::map<NodePtr, boost::shared_ptr<NodeSerialization> >::const_iterator it = createdNodes.begin(); it != createdNodes.end(); ++it) {
        if ( appPTR->isBackground() && (dynamic_cast<ViewerInstance*>((it->first)->getLiveInstance()))) {
            //ignore viewers on background mode
            continue;
        }
        it->first->restoreKnobsLinks(*it->second,nodes);
        
    }
    
    ///Also reconnect parents of multiinstance nodes that were created on the fly
    for (std::map<boost::shared_ptr<Natron::Node>, std::list<boost::shared_ptr<NodeSerialization> >::const_iterator >::const_iterator
         it = parentsToReconnect.begin(); it != parentsToReconnect.end(); ++it) {
        const std::vector<std::string> & oldInputs = (*it->second)->getOldInputs();
        if (!oldInputs.empty()) {
            /*
             * Prior to Natron v2 OpenFX effects had their inputs reversed internally
             */
            bool isOfxEffect = it->first->isOpenFXNode();
            
            for (U32 j = 0; j < oldInputs.size(); ++j) {
                if ( !oldInputs[j].empty() && !group->connectNodes(isOfxEffect ? oldInputs.size() - 1 - j : j, oldInputs[j],it->first.get()) ) {
                    if (createNodes) {
                        qDebug() << "Failed to connect node" << it->first->getPluginLabel().c_str() << "to" << oldInputs[j].c_str()
                        << "[This is normal if loading a PyPlug]";
                    }
                    
                }
            }
        } else {
            const std::map<std::string,std::string>& inputs = (*it->second)->getInputs();
            for (std::map<std::string,std::string>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
                if (it2->second.empty()) {
                    continue;
                }
                int index = it->first->getInputNumberFromLabel(it2->first);
                if (index == -1) {
                    appPTR->writeToOfxLog_mt_safe(QString("Could not find input named ") + it2->first.c_str());
                    continue;
                }
                if (!it2->second.empty() && !group->connectNodes(index, it2->second, it->first.get())) {
                    if (createNodes) {
                        qDebug() << "Failed to connect node" << it->first->getPluginLabel().c_str() << "to" << it2->second.c_str()
                        << "[This is normal if loading a PyPlug]";
                    }
                }
            }
        }
    }
    return !mustShowErrorsLog;
}