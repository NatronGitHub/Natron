/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// Regression test for https://github.com/NatronGitHub/Natron/issues/814
// ("Application freeze when copying a node with enabled postage stamp").
//
// When a Group with its preview enabled was pasted or duplicated, its preview was rendered on the preview thread
// right after the (still empty) group node was created, while the main thread was still creating its sub-graph.
// Node::makePreviewImage() then dereferenced the group's missing Output node. The same happens for a group whose
// Output node was deleted by the user. The preview of a group without an Output node must just be empty.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_USING

TEST(NodePreview, GroupWithoutOutputNode)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );

    // A group as NodeGraphPrivate::pasteNode() creates it, before pasting its children: no Input/Output node.
    CreateNodeArgs args( PLUGINID_NATRON_GROUP, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, true);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    NodePtr groupNode = app->createNode(args);
    ASSERT_TRUE( bool(groupNode) );
    NodeGroup* group = groupNode->isEffectGroup();
    ASSERT_TRUE(group != NULL);
    ASSERT_FALSE( bool( group->getOutputNode(false) ) );

    int w = 64;
    int h = 64;
    std::vector<unsigned int> buf(w * h, 0);
    EXPECT_FALSE( groupNode->makePreviewImage(1, &w, &h, &buf.front()) );

    groupNode->destroyNode(true, false);
}
