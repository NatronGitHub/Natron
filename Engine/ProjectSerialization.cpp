//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ProjectSerialization.h"
#include "Engine/TimeLine.h"
#include "Engine/Project.h"
#include "Global/AppManager.h"

void ProjectSerialization::initialize(const Natron::Project* project){
    std::vector<Natron::Node*> activeNodes;
    project->getApp()->getActiveNodes(&activeNodes);
    _serializedNodes.clear();
    for (U32 i = 0; i < activeNodes.size(); ++i) {
        boost::shared_ptr<NodeSerialization> state(new NodeSerialization);
        activeNodes[i]->serialize(state.get());
        _serializedNodes.push_back(state);
    }
    _availableFormats = project->getProjectFormats();
    
    const std::vector< boost::shared_ptr<Knob> >& knobs = project->getKnobs();
    for(U32 i = 0; i < knobs.size();++i){
        if(knobs[i]->isPersistent()){
            _projectKnobs.insert(std::make_pair(knobs[i]->getDescription(),
                                            dynamic_cast<AnimatingParam&>(*knobs[i].get())));
        }
    }

    _timelineLeft = project->getTimeLine()->leftBound();
    _timelineRight = project->getTimeLine()->rightBound();
    _timelineCurrent = project->getTimeLine()->currentFrame();
}
