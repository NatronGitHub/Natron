/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// A render in a separate process ends (ProcessHandler::processFinished()) when its process fails to start:
// QProcess emits finished() only for a process that started, so the render stayed in the render queue
// forever (app.waitForRenders(), hasRenderInProgress(), the renders queued behind it, #1596).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QString>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_USING

TEST(ProcessHandler, FailedToStartFinishesTheRender)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE(app);
    CreateNodeArgs args( PLUGINID_NATRON_DISKCACHE, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    NodePtr node = app->createNode(args);
    ASSERT_TRUE(node);
    OutputEffectInstance* writer = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
    ASSERT_TRUE(writer);

    std::vector<int> returnCodes;
    {
        ProcessHandler handler(QString::fromUtf8("unused.ntp"), writer);
        QObject::connect( &handler, &ProcessHandler::processFinished, [&returnCodes](int retCode) {
            returnCodes.push_back(retCode);
        } );

        // what QProcess::errorOccurred() calls when the process can't be started
        handler.onProcessError(QProcess::FailedToStart);
        ASSERT_EQ( returnCodes.size(), 1u );
        EXPECT_EQ(returnCodes[0], 1);

        // the other errors happen while the process runs: finished() follows (or the process keeps running)
        handler.onProcessError(QProcess::Crashed);
        handler.onProcessError(QProcess::ReadError);
        EXPECT_EQ( returnCodes.size(), 1u );
    }

    node->destroyNode(true, false);
}
