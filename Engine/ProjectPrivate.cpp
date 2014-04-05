//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ProjectPrivate.h"

#include <QDebug>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/NodeSerialization.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/ProjectSerialization.h"

namespace Natron{
ProjectPrivate::ProjectPrivate(Natron::Project* project)
    : projectLock()
    , projectName("Untitled." NATRON_PROJECT_FILE_EXT)
    , hasProjectBeenSavedByUser(false)
    , ageSinceLastSave(QDateTime::currentDateTime())
    , lastAutoSave()
    , projectCreationTime(ageSinceLastSave)
    , formatKnob()
    , builtinFormats()
    , additionalFormats()
    , formatMutex()
    , addFormatKnob()
    , viewsCount()
    , viewsCountMutex()
    , previewMode()
    , previewModeMutex()
    , timelineMutex()
    , timeline(new TimeLine(project))
    , autoSetProjectFormat(true)
    , currentNodes()
    , project(project)
    , lastTimelineSeekCaller()
    , beginEndBracketsCount(0)
    , evaluationsCount(0)
    , holdersWhoseBeginWasCalled()
    , isSignificantChange(false)
    , lastKnobChanged(NULL)
    , isLoadingProjectMutex()
    , isLoadingProject(false)
    , isSavingProjectMutex()
    , isSavingProject(false)

{
}


void ProjectPrivate::restoreFromSerialization(const ProjectSerialization& obj){
    
    project->beginProjectWideValueChanges(Natron::OTHER_REASON,project);

    /*1st OFF RESTORE THE PROJECT KNOBS*/
    
    /*we must restore the entries in the combobox before restoring the value*/
    std::vector<std::string> entries;
    
    for (std::list<Format>::const_iterator it = builtinFormats.begin(); it!=builtinFormats.end();++it) {
        QString formatStr = Natron::generateStringFromFormat(*it);
        entries.push_back(formatStr.toStdString());
    }

    const std::list<Format>& objAdditionalFormats = obj.getAdditionalFormats();
    for (std::list<Format>::const_iterator it = objAdditionalFormats.begin(); it!=objAdditionalFormats.end();++it) {
        QString formatStr = Natron::generateStringFromFormat(*it);
        entries.push_back(formatStr.toStdString());
    }
    additionalFormats = objAdditionalFormats;

    formatKnob->populateChoices(entries);
    autoSetProjectFormat = false;
    
    const std::list< boost::shared_ptr<KnobSerialization> >& projectSerializedValues = obj.getProjectKnobsValues();
    const std::vector< boost::shared_ptr<KnobI> >& projectKnobs = project->getKnobs();
    
    
    /// 1) restore project's knobs.
    
    for(U32 i = 0 ; i < projectKnobs.size();++i){
        ///try to find a serialized value for this knob
        for(std::list< boost::shared_ptr<KnobSerialization> >::const_iterator it = projectSerializedValues.begin(); it!=projectSerializedValues.end();++it) {
            if((*it)->getName() == projectKnobs[i]->getDescription()) {
                if (projectKnobs[i]->getIsPersistant()) {
                    projectKnobs[i]->clone((*it)->getKnob());
                }
                break;
            }
        }
    }
    
    
    
    /// 2) restore the timeline
    timeline->setBoundaries(obj.getLeftBoundTime(), obj.getRightBoundTime());
    timeline->seekFrame(obj.getCurrentTime(),boost::shared_ptr<OutputEffectInstance>());

    /// 3) Restore the nodes
    const std::list< NodeSerialization >& serializedNodes = obj.getNodesSerialization();
    bool hasProjectAWriter = false;
    /*first create all nodes*/
    for (std::list< NodeSerialization >::const_iterator it = serializedNodes.begin(); it!=serializedNodes.end();++it) {

        if (appPTR->isBackground() && it->getPluginID() == "Viewer") {
            //if the node is a viewer, don't try to load it in background mode
            continue;
        }

         ///this code may throw an exception which will be caught above
        boost::shared_ptr<Natron::Node> n = project->getApp()->loadNode(it->getPluginID().c_str()
                                                        ,it->getPluginMajorVersion()
                                                        ,it->getPluginMinorVersion(),*it,false);
        if (!n) {
            project->clearNodes();
            QString text("Failed to restore the graph! \n The node ");
            text.append(it->getPluginID().c_str());
            text.append(" was found in the auto-save script but doesn't seem \n"
                        "to exist in the currently loaded plug-ins.");
            throw std::invalid_argument(text.toStdString());
        }
        if (n->isOutputNode()) {
            hasProjectAWriter = true;
        }

    }


    if(!hasProjectAWriter && appPTR->isBackground()){
        project->clearNodes();
        throw std::invalid_argument("Project file is missing a writer node. This project cannot render anything.");
    }
    
    /// 4) connect the nodes together, and restore the slave/master links for all knobs.
    for(std::list< NodeSerialization >::const_iterator it = serializedNodes.begin(); it!=serializedNodes.end();++it) {
        
        if(appPTR->isBackground() && it->getPluginID() == "Viewer"){
            //ignore viewers on background mode
            continue;
        }

        
        boost::shared_ptr<Natron::Node> thisNode;
        for (U32 j = 0; j < currentNodes.size(); ++j) {
            if (currentNodes[j]->getName() == it->getPluginLabel()) {
                thisNode = currentNodes[j];
                break;
            }
        }
        assert(thisNode);
        
        ///restore slave/master link if any
        const std::string& masterNodeName = it->getMasterNodeName();
        if (!masterNodeName.empty()) {
            ///find such a node
            boost::shared_ptr<Natron::Node> masterNode;
            for (U32 j = 0; j < currentNodes.size(); ++j) {
                if (currentNodes[j]->getName() == masterNodeName) {
                    masterNode = currentNodes[j];
                    break;
                }
            }
            if (!masterNode) {
                throw std::runtime_error("Cannot restore the link between " + it->getPluginLabel() + " and " +
                                         masterNodeName);
            }
            thisNode->getLiveInstance()->slaveAllKnobs(masterNode->getLiveInstance());
        } else {
            thisNode->restoreKnobsLinks(*it,currentNodes);
        }
        
        const std::vector<std::string>& inputs = it->getInputs();
        for (U32 j = 0; j < inputs.size();++j) {
            
            if (!inputs[j].empty() && !project->getApp()->getProject()->connectNodes(j, inputs[j],thisNode)) {
                std::string message = std::string("Failed to connect node ") + it->getPluginLabel() + " to " + inputs[j];
                qDebug() << message.c_str();
                throw std::runtime_error(message);
            }
        }

    }
    
    project->endProjectWideValueChanges(project);
    
    nodeCounters = obj.getNodeCounters();
    
    projectCreationTime = QDateTime::fromMSecsSinceEpoch(obj.getCreationDate());

}
    
bool ProjectPrivate::findFormat(int index,Format* format) const
{
    if (index >= (int)(builtinFormats.size() + additionalFormats.size())) {
        return false;
    }
    
    int i = 0;
    if (index >= (int)builtinFormats.size()) {
        ///search in the additional formats
        index -= builtinFormats.size();
        
        for (std::list<Format>::const_iterator it = additionalFormats.begin();it!=additionalFormats.end();++it) {
            if (i == index) {
                assert(!it->isNull());
                *format = *it;
                return true;
            }
            ++i;
        }
    } else {
        ///search in the builtins formats
        for (std::list<Format>::const_iterator it = builtinFormats.begin();it!=builtinFormats.end();++it) {
            if (i == index) {
                assert(!it->isNull());
                *format = *it;
                return true;
            }
            ++i;
        }
    }
    return false;

}

} // namespace Natron
