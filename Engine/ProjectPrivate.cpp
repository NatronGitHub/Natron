
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
    , formatKnob()
    , availableFormats()
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
    , _knobsAge(0)
    , knobsAgeMutex()
    , lastTimelineSeekCaller(NULL)
    , beginEndMutex(QMutex::Recursive)
    , beginEndBracketsCount(0)
    , evaluationsCount(0)
    , holdersWhoseBeginWasCalled()
    , isSignificantChange(false)
    , lastKnobChanged(NULL)
    , isLoadingProjectMutex()
    , isLoadingProject(false)

{
}


void ProjectPrivate::restoreFromSerialization(const ProjectSerialization& obj){
    
    project->beginProjectWideValueChanges(Natron::OTHER_REASON,project);


    /*1st OFF RESTORE THE PROJECT KNOBS*/
    
    /*we must restore the entries in the combobox before restoring the value*/
    std::vector<std::string> entries;
    const std::vector<Format>& objAvailableFormats = obj.getProjectFormats();
    for (U32 i = 0; i < objAvailableFormats.size(); ++i) {
        QString formatStr = Natron::generateStringFromFormat(objAvailableFormats[i]);
        entries.push_back(formatStr.toStdString());
    }
    availableFormats = objAvailableFormats;
    formatKnob->populate(entries);
    autoSetProjectFormat = false;
    
    const std::vector< boost::shared_ptr<KnobSerialization> >& projectSerializedValues = obj.getProjectKnobsValues();
    const std::vector< boost::shared_ptr<Knob> >& projectKnobs = project->getKnobs();
    
    
    //// restoring values
    ///
    for(U32 i = 0 ; i < projectKnobs.size();++i){
        ///try to find a serialized value for this knob
        for(U32 j = 0 ; j < projectSerializedValues.size();++j){
            if(projectSerializedValues[j]->getLabel() == projectKnobs[i]->getDescription()){
                if (projectKnobs[i]->isPersistent()) {
                    projectKnobs[i]->load(*projectSerializedValues[j]);
                }
                break;
            }
        }
    }
    
    
    
    /* 2nd RESTORE TIMELINE */
    timeline->setBoundaries(obj.getLeftBoundTime(), obj.getRightBoundTime());
    timeline->seekFrame(obj.getCurrentTime(),NULL);

    /* 3rd RESTORE NODES */
    const std::vector< boost::shared_ptr<NodeSerialization> >& serializedNodes = obj.getNodesSerialization();
    bool hasProjectAWriter = false;
    /*first create all nodes and restore the knobs values*/
    for (U32 i = 0; i <  serializedNodes.size() ; ++i){

        if (appPTR->isBackground() && serializedNodes[i]->getPluginID() == "Viewer") {
            //if the node is a viewer, don't try to load it in background mode
            continue;
        }

        Natron::Node* n = 0;
        ///this code may throw an exception which will be caught above
        n = project->getApp()->createNode(serializedNodes[i]->getPluginID().c_str()
                                                        ,serializedNodes[i]->getPluginMajorVersion()
                                                        ,serializedNodes[i]->getPluginMinorVersion(),true);
        if(!n){
            continue;
        }
        if(n->isOutputNode()){
            hasProjectAWriter = true;
        }

        if(!n){
            project->clearNodes();
            QString text("Failed to restore the graph! \n The node ");
            text.append(serializedNodes[i]->getPluginID().c_str());
            text.append(" was found in the auto-save script but doesn't seem \n"
                        "to exist in the currently loaded plug-ins.");
            throw std::invalid_argument(text.toStdString());
        }
        n->setName(serializedNodes[i]->getPluginLabel());
        
        const std::vector< boost::shared_ptr<Knob> >& nodeKnobs = n->getKnobs();
        const NodeSerialization::KnobValues& knobsValues = serializedNodes[i]->getKnobsValues();

        ///for all knobs of the node
        for (U32 j = 0; j < nodeKnobs.size();++j) {
            
            ///try to find a serialized value for this knob
            for (U32 k = 0; k < knobsValues.size(); ++k) {
                if(knobsValues[k]->getLabel() == nodeKnobs[j]->getDescription()){
                    // don't load the value if the Knob is not persistant! (it is just the default value in this case)
                    if (nodeKnobs[j]->isPersistent()) {
                        nodeKnobs[j]->load(*knobsValues[k]);
                    }
                    break;
                }
            }
        }

    }

    if(!hasProjectAWriter && appPTR->isBackground()){
        project->clearNodes();
        throw std::invalid_argument("Project file is missing a writer node. This project cannot render anything.");
    }

    /*now that we have all nodes, just connect them*/
    for(U32 i = 0; i <  serializedNodes.size() ; ++i){

        if(appPTR->isBackground() && serializedNodes[i]->getPluginID() == "Viewer"){
            //ignore viewers on background mode
            continue;
        }

        const std::map<int, std::string>& inputs = serializedNodes[i]->getInputs();
        Natron::Node* thisNode = NULL;
        for (U32 j = 0; j < currentNodes.size(); ++j) {
            if (currentNodes[j]->getName() == serializedNodes[i]->getPluginLabel()) {
                thisNode = currentNodes[j];
                break;
            }
        }
        for (std::map<int, std::string>::const_iterator input = inputs.begin(); input!=inputs.end(); ++input) {
            if(!project->getApp()->getProject()->connectNodes(input->first, input->second,thisNode)) {
                std::string message = std::string("Failed to connect node ") + serializedNodes[i]->getPluginLabel() + " to " + input->second;
                qDebug() << message.c_str();
                throw std::runtime_error(message);
            }
        }

    }
    
    project->endProjectWideValueChanges(project);

}

} // namespace Natron
