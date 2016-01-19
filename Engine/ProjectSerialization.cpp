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

#include "ProjectSerialization.h"

#include "Engine/AppManager.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/TimeLine.h"

NATRON_NAMESPACE_ENTER;

void
ProjectSerialization::initialize(const Project* project)
{
    ///All the code in this function is MT-safe

    _nodes.initialize(*project);
    
    project->getAdditionalFormats(&_additionalFormats);

    std::vector< boost::shared_ptr<KnobI> > knobs = project->getKnobs_mt_safe();
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobGroup* isGroup = dynamic_cast<KnobGroup*>( knobs[i].get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( knobs[i].get() );
        KnobButton* isButton = dynamic_cast<KnobButton*>( knobs[i].get() );
        if (knobs[i]->getIsPersistant() &&
            !isGroup && !isPage && !isButton &&
            knobs[i]->hasModificationsForSerialization()) {
            boost::shared_ptr<KnobSerialization> newKnobSer( new KnobSerialization(knobs[i]) );
            _projectKnobs.push_back(newKnobSer);
        }
    }

    _timelineCurrent = project->currentFrame();

    _creationDate = project->getProjectCreationTime();
}

NATRON_NAMESPACE_EXIT;
