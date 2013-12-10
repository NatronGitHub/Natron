
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

#include "Global/AppManager.h"
#include "Engine/NodeSerialization.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/ProjectSerialization.h"

namespace Natron{
ProjectPrivate::ProjectPrivate(Natron::Project* project)
    : _projectName("Untitled." NATRON_PROJECT_FILE_EXT)
    , _hasProjectBeenSavedByUser(false)
    , _ageSinceLastSave(QDateTime::currentDateTime())
    , _formatKnob(NULL)
    , _addFormatKnob(NULL)
    , _viewsCount(NULL)
    , _timeline(new TimeLine(project))
    , _autoSetProjectFormat(true)
    , _projectDataLock()
    , _currentNodes()
    , _availableFormats()
    , _project(project)
    , _knobsAge(0)
    , _lastTimelineSeekCaller(NULL)
    , _isSignificantChange(false)

{
    _isBetweenBeginAndEndValueChanged.second = NULL;
    _isBetweenBeginAndEndValueChanged.first = false;
}


void ProjectPrivate::restoreFromSerialization(const ProjectSerialization& obj){
    
    _project->beginProjectWideValueChanges(Natron::PLUGIN_EDITED,_project);

    /*1st OFF RESTORE THE PROJECT KNOBS*/
    
    /*we must restore the entries in the combobox before restoring the value*/
    std::vector<std::string> entries;
    const std::vector<Format>& objAvailableFormats = obj.getProjectFormats();
    for (U32 i = 0; i < objAvailableFormats.size(); ++i) {
        QString formatStr = Natron::generateStringFromFormat(objAvailableFormats[i]);
        entries.push_back(formatStr.toStdString());
    }
    _availableFormats = objAvailableFormats;
    _formatKnob->populate(entries);
    _autoSetProjectFormat = false;
    
    const std::map<std::string,boost::shared_ptr<KnobSerialization> >& projectSerializedValues = obj.getProjectKnobsValues();
    const std::vector< boost::shared_ptr<Knob> >& projectKnobs = _project->getKnobs();
    
    
    //// restoring values
    ///
    for(U32 i = 0 ; i < projectKnobs.size();++i){
        std::map<std::string,boost::shared_ptr<KnobSerialization> >::const_iterator foundValue =
                projectSerializedValues.find(projectKnobs[i]->getDescription());
        if(foundValue != projectSerializedValues.end()){
            projectKnobs[i]->load(*foundValue->second);
        }
    }
    
    
    
    /* 2nd RESTORE TIMELINE */
    _timeline->setBoundaries(obj.getLeftBoundTime(), obj.getRightBoundTime());
    _timeline->seekFrame(obj.getCurrentTime(),NULL);

    /* 3rd RESTORE NODES */
    const std::vector< boost::shared_ptr<NodeSerialization> >& serializedNodes = obj.getNodesSerialization();
    bool hasProjectAWriter = false;
    /*first create all nodes and restore the knobs values*/
    for (U32 i = 0; i <  serializedNodes.size() ; ++i){

        if (_project->getApp()->isBackground() && serializedNodes[i]->getPluginID() == "Viewer") {
            //if the node is a viewer, don't try to load it in background mode
            continue;
        }

        Natron::Node* n = _project->getApp()->createNode(serializedNodes[i]->getPluginID().c_str(),true);
        if(!n){
            Natron::errorDialog("Loading failed", "Cannot load node " + serializedNodes[i]->getPluginID());
            continue;
        }
        if(n->pluginID() == "Writer" || (n->isOpenFXNode() && n->isOutputNode())){
            hasProjectAWriter = true;
        }

        if(!n){
            _project->clearNodes();
            QString text("Failed to restore the graph! \n The node ");
            text.append(serializedNodes[i]->getPluginID().c_str());
            text.append(" was found in the auto-save script but doesn't seem \n"
                        "to exist in the currently loaded plug-ins.");
            throw std::invalid_argument(text.toStdString());
        }
        n->setName(serializedNodes[i]->getPluginLabel());

        const NodeSerialization::KnobValues& knobsValues = serializedNodes[i]->getKnobsValues();
        //begin changes to params
        for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin();
             it != knobsValues.end();++it) {
            boost::shared_ptr<Knob> knob = n->getKnobByDescription(it->first);
            if (!knob) {
                std::string message = std::string("Couldn't find knob ") + it->first;
                qDebug() << message.c_str();
                throw std::runtime_error(message);
            }
            knob->load(*it->second);
        }

    }

    if(!hasProjectAWriter && _project->getApp()->isBackground()){
        _project->clearNodes();
        throw std::invalid_argument("Project file is missing a writer node. This project cannot render anything.");
    }

    /*now that we have all nodes, just connect them*/
    for(U32 i = 0; i <  serializedNodes.size() ; ++i){

        if(_project->getApp()->isBackground() && serializedNodes[i]->getPluginID() == "Viewer"){
            //ignore viewers on background mode
            continue;
        }

        const std::map<int, std::string>& inputs = serializedNodes[i]->getInputs();
        Natron::Node* thisNode = NULL;
        for (U32 j = 0; j < _currentNodes.size(); ++j) {
            if (_currentNodes[j]->getName() == serializedNodes[i]->getPluginLabel()) {
                thisNode = _currentNodes[j];
                break;
            }
        }
        for (std::map<int, std::string>::const_iterator input = inputs.begin(); input!=inputs.end(); ++input) {
            if(!_project->getApp()->connect(input->first, input->second,thisNode)) {
                std::string message = std::string("Failed to connect node ") + serializedNodes[i]->getPluginLabel() + " to " + input->second;
                qDebug() << message.c_str();
                throw std::runtime_error(message);
            }
        }

    }

    _project->endProjectWideValueChanges(Natron::PLUGIN_EDITED,_project);
}

} // namespace Natron
