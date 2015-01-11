
//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ProjectSerialization.h"

#include "Engine/TimeLine.h"
#include "Engine/Project.h"
#include "Engine/AppManager.h"


void
ProjectSerialization::initialize(const Natron::Project* project)
{
    ///All the code in this function is MT-safe

    _nodes.initialize(*project);
    
    project->getAdditionalFormats(&_additionalFormats);

    std::vector< boost::shared_ptr<KnobI> > knobs = project->getKnobs_mt_safe();
    for (U32 i = 0; i < knobs.size(); ++i) {
        Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knobs[i].get() );
        Page_Knob* isPage = dynamic_cast<Page_Knob*>( knobs[i].get() );
        Button_Knob* isButton = dynamic_cast<Button_Knob*>( knobs[i].get() );
        if (knobs[i]->getIsPersistant() && !isGroup && !isPage && !isButton) {
            boost::shared_ptr<KnobSerialization> newKnobSer( new KnobSerialization(knobs[i],false) );
            _projectKnobs.push_back(newKnobSer);
        }
    }

    _timelineLeft = project->leftBound();
    _timelineRight = project->rightBound();
    _timelineCurrent = project->currentFrame();

    _creationDate = project->getProjectCreationTime();
}

