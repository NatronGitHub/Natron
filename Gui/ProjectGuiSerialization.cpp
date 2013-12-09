//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ProjectGuiSerialization.h"

#include "Global/AppManager.h"
#include "Engine/Project.h"
#include "Gui/NodeGui.h"

 void ProjectGuiSerialization::initialize(const ProjectGui* projectGui){
     std::vector<NodeGui*> activeNodes = projectGui->getInternalProject()->getApp()->getVisibleNodes();
     _serializedNodes.clear();
     for (U32 i = 0; i < activeNodes.size(); ++i) {
         boost::shared_ptr<NodeGuiSerialization> state(new NodeGuiSerialization);
         activeNodes[i]->serialize(state.get());
         _serializedNodes.push_back(state);
     }

 }
